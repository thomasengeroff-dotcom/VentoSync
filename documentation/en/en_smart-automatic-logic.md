# 🤖 Smart automatic Mode (Auto Logic)

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_smart-automatic-logic.md)


The **Smart automatic Mode** is the "brain" of VentoSync. It provides fully autonomous, sensor-driven ventilation control optimized for air quality, energy efficiency, and comfort. This document describes the technical implementation and the decision-making logic behind this mode.

---

## 🏗️ Architecture & File Structure

The logic is distributed across several layers to ensure maintainability and high performance on the ESP32-C6.

| Component | File | Responsibility |
| :--- | :--- | :--- |
| **Main Loop** | [`logic_automation.yaml`](../../packages/actuators/logic_automation.yaml) | Triggers the evaluation cycle every 10 seconds. Calls `evaluate_auto_mode()`. |
| **Core Logic (C++)** | [`auto_mode.h`](../../components/helpers/auto_mode.h) | The "Engine". Implements math, sensor fusion, and mode switching logic. |
| **PID Controllers** | [`logic_pid.yaml`](../../packages/actuators/logic_pid.yaml) | Defines the internal CO2 and Humidity PID climate controllers and their dummy outputs. |
| **Climate Sensors** | [`sensors_climate.yaml`](../../packages/sensors/sensors_climate.yaml) | Defines input sensors (SCD43, BME680, Home Assistant sensors) and efficiency metrics. |
| **UI & Thresholds** | [`ui_controls.yaml`](../../packages/ui/ui_controls.yaml) | Provides Home Assistant entities for runtime configuration (limits, targets). |
| **Global State** | [`globals.h`](../../components/helpers/globals.h) | Shared pointers and variables accessible by both YAML and C++. |

---

## 🔄 Logic Flow: The 10-Second Decision Cycle

Every 10 seconds, the `evaluate_auto_mode()` function runs the following process:

```mermaid
graph TD
    Start([10s Interval Trigger]) --> Flags[Refresh room flags: AC / window / presence]
    Flags --> Sync[Sensor Fusion: local -. peer -. held reading]
    Sync --> HVAC{Smart Climate Control:<br/>AC active?}

    subgraph Mode_Management [Mode Decision]
    HVAC -- Yes --> ModeRec[Target Mode: ECO RECOVERY forced]
    HVAC -- No --> Season{Summer Mode active?}
    Season -- No --> ModeRec
    Season -- Yes --> Cooling{Indoor > Threshold & Outdoor Cooler?}
    Cooling -- No --> ModeRec
    Cooling -- Yes --> Guard{Both temperatures<br/>still measurable?}
    Guard -- No --> ModeRec
    Guard -- Yes --> ModeVent[Target Mode: VENTILATION]
    end

    ModeVent --> Demand[Calculate Combined PID Demand]
    ModeRec --> Demand

    subgraph Demand_Logic [Demand Calculation]
    Demand --> CO2[Evaluate CO2 PID]
    Demand --> Hum[Evaluate Humidity PID]
    CO2 -- "Demand >= 0.01" --> Priority[CO2 Priority: Grab Control]
    CO2 -- "Demand < 0.005" --> Balanced[Release: Balance CO2 & Humidity]
    CO2 -- "0.005 to 0.01" --> Hold[Hysteresis Hold: Keep Current State]
    Priority --> Fuse
    Balanced --> Fuse
    Hold --> Fuse[Room fusion: max of local and fresh peer demands]
    end

    Fuse --> Holdoff{Mode-switch<br/>hold-off active?}
    Holdoff -- Yes --> Zero[Demand = 0, broadcast NaN]
    Holdoff -- No --> NaNChk{Demand is NaN?}
    NaNChk -- Yes --> HoldState([Hold last state and abort])
    Zero --> Master{I am Master?}
    NaNChk -- No --> Master

    subgraph Level_Commit [Intensity Commitment]
    Master -- Yes --> CalcLevel[Calc Level from Demand + Hysteresis]
    Master -- No --> Follow[Follow Master's Discrete Level]
    CalcLevel --> Clamp
    Follow --> Clamp[Clamp into the local level window]
    Clamp --> Ramp[Soft Ramp: Max +/- 1 per 10s]
    end

    Ramp --> Final([Apply PWM & Notify Peers])
```

---

## 🧪 Detailed Logic Components

### 1. Sensor Fusion & Fallbacks
The system ensures stability even if a local sensor fails.
- **CO2 Fallback Chain** (in `effective_co2` template sensor): Local SCD43 → Local BME680 IAQ eCO2 → Hold last value (up to 5 min) → NaN.
- **Temperature Fallback Chain** (in `auto_mode.h`, `get_effective_temperatures()`): Local SCD43 Temperature → NTC Phase-Locked Values → **fresh peer data via ESP-NOW** → **own last valid reading** (`HeldReading`, up to 30 min) → NaN.
- **Phase-Locked NTC Sensors**: The NTC sensors are physically fixed in the air duct, so the assignment never changes: `temp_zuluft` is the outdoor sensor, `temp_abluft` the indoor one. A phase-lock filter in `climate.h` lets each NTC publish only during the ventilation phase in which it actually faces its own air stream (indoor NTC during exhaust, outdoor NTC during intake) and freezes it at its last value otherwise.
  - In **heat recovery** the direction alternates every 50–70 s, so both sensors are refreshed within one cycle and both values are used.
  - In **continuous ventilation** (summer bypass) the direction no longer alternates, so one NTC stays frozen permanently. That frozen value is deliberately **not used and not broadcast** — the fusion chain above fills the gap, and `guard_summer_bypass()` (see section 4) returns to heat recovery if it cannot.
- **Room-wide CO2 and humidity**: For the Smart Climate Control guards the logic does not use the local sensor alone but the **room-wide worst case** — the highest CO2 (`get_room_max_co2()`) and the highest relative humidity (wettest spot) among the local sensor and all fresh peers.

### 2. Humidity Management (Enthalpy Logic)
VentoSync prevents "moisture intake" during humid summer days or rainy weather.
- **Scientific Foundation**: The logic uses the **Magnus Formula** to calculate **Absolute Humidity ($g/m^3$)**.
- **Guard Condition**: Dehumidification via PID is only allowed if:
  $$Absolute\_Humidity_{Outdoor} < Absolute\_Humidity_{Indoor}$$
  This ensures that ventilation actually removes water from the building rather than bringing it in.

### 3. Dual-PID Priority Control
Two independent PID controllers run in the background (defined in [`logic_pid.yaml`](../../packages/actuators/logic_pid.yaml)):
1. **PID CO2**: Target: 1000 ppm (configurable).
2. **PID Humidity**: Target: 60% rH (configurable).

**Conflict Resolution (Hysteresis)**:
- **CO2 Grab**: If CO2 demand exceeds **1%**, CO2 takes priority control of the hysteresis state machine.
- **CO2 Release**: Only when CO2 demand falls below **0.5%**, control is handed over to the Humidity PID.
- **Hold**: Between 0.5% and 1%, the current state is maintained (no switching) to prevent oscillation.
- **Priority with Boost**: Even while CO2 has priority, the effective demand is `max(CO2, Humidity)` — humidity can boost the fan speed above CO2's request, but cannot reduce it. This ensures both air quality and moisture safety.

**Room-wide Demand Fusion**:
The result above is the demand of the **local** sensors. On top of that, every device adopts the highest demand any fresh peer reports: `effective demand = max(local, highest fresh peer demand)` (`ventosync::room::room_max_peer_demand()`, freshness `max(5 min, two heartbeats)`).

- Peers compute their value with their full CO2 **and** humidity PID, including the integral term, the priority hysteresis and the enthalpy guard — so a Master **without its own sensors** regulates the room exactly like the sensor device does.
- Each device broadcasts **only its own local-sensor demand** (`local_pid_demand`), never the fused result, and `NaN` when it has no sensor of its own. Re-broadcasting a fused value would let two devices latch each other at a high level (feedback loop, CHANGELOG 0.10.21).
- While the mode-switch hold-off (see below) is active, the device broadcasts `NaN` as well, because its own PID output is not yet trustworthy.

**Mode-switch Hold-off**:
Right after switching **into** Smart-Automatik the PID outputs are still stale for a few cycles (the controller's proportional term overwrites the values `system_lifecycle.h` reset to zero). For `MODE_SWITCH_HOLDOFF_MS` (15 s ≈ 1.5 CO2 PID cycles) the demand is therefore forced to `0`, so the fan starts from the minimum level and only ramps up once a genuine sensor cycle has completed.

**No data at all**: if neither the local sensors nor any peer deliver a usable demand, the result is `NaN` and the cycle aborts **without changing the fan level** — the last state is held rather than falling back to a default.

### 4. Summer Cooling (Bypass Simulation)
Since decentralized units typically lack a physical bypass flap, the logic simulates a bypass by disabling the reversing cycle.
- **Condition**: Indoor Temp > threshold (slider, default 22°C) AND Outdoor Temp < (Indoor - 1.5°C) AND HA "Sommerbetrieb" is ON AND no active AC (Smart Climate Control).
- **Release**: "Sommerbetrieb" OFF, or Outdoor ≥ Indoor − 0.5°C, or Indoor < threshold − 0.5°C.
- **Action**: Switch to `MODE_VENTILATION` (one-way flow, no timer).
- **Temperatures**: In one-way flow only the NTC facing its own air stream is measurable (intake: outdoor, exhaust: indoor). The other value comes from a peer, else from the device's last reading (≤ 30 min); if still unknown, `guard_summer_bypass()` returns to heat recovery and blocks re-entry for 5 min (`SUMMER_COOLING_REMEASURE_MS`) to re-measure.
- **Benefit**: Draws in cool night air efficiently without warming it up in the ceramic heat exchanger.

### 5. Master/Slave Synchronization (Room Authority)
To avoid different fans in the same room running at different speeds (which causes pressure imbalance), the system uses an **Authority Rule**:
- **Master (ID=1)**: Calculates the target level (1–10) from the room demand (local + fused peer demand) via `VentilationLogic::calculate_auto_target_level()`.
- **Slaves (ID > 1)**: Mirror the Master's discrete level instead of computing one from their own demand — but still **clamp it into their own level window** `[min, max]`. Because the window settings and the AC state are room-wide, that window is normally identical to the Master's; it only differs transiently, e.g. until the next heartbeat carries a changed AC state.
- **Master offline**: if no packet from device ID 1 arrived within `PEER_TIMEOUT_MS` (15 min), the slave falls back to computing the level from the demand itself, so the room keeps regulating.
- **Soft Ramping**: All devices apply a max transition of **+/- 1 level per 10 seconds** (±2 in the cycle right after a mode change) for silent and motor-friendly speed changes. A shrinking window — e.g. the Smart Climate Control cap — is therefore approached one level per cycle, not in one jump.

### 6. Smart Climate Control (HVAC Coordination)
An optional modifier evaluated at the start of every 10-second cycle (`auto_mode::evaluate_hvac_coordination()`). While the room-wide `Klima-Koordination` switch is on **and** Home Assistant reports the AC as active (API action `set_ac_active`, shared room-wide), the cycle runs with a restricted profile:
- **CO2-only loop**: the humidity PID demand is ignored, the CO2 PID setpoint is switched to `hvac_co2_threshold` (default 1200 ppm) and re-asserted every cycle.
- **Level window**: `[1, hvac_max_fan_level]` (default 1–3) replaces `automatik_min/max_fan_level`.
- **Mode lock**: `determine_auto_operating_mode()` always returns heat recovery — no summer bypass while the AC runs.
- **Health guards**: a CO2 emergency (≥ `hvac_emergency_co2`, release ≤ `hvac_co2_threshold`) and a mold guard (≥ 70 % rH while outdoor air is drier, release ≤ 65 %) restore the normal limits and dual-PID. Both are fed with the **room-wide worst case** (highest CO2 / highest rH of the local sensor and all fresh peers), so the worst air anywhere in the room decides. If no device in the room reports CO2, nothing is throttled at all.
- **Debounce**: AC "off" must persist 120 s before the restrictions are released; the ramp back is the standard ±1 level per cycle.

Details, state machine and HA setup: [📄 Smart Climate Control — HVAC Coordination](en_smart-climate-control.md).

---

## ⚙️ Configuration Entities

| HA Entity (German UI name) | Entity ID | Global (C++) | Default | Purpose |
| :--- | :--- | :--- | :---: | :--- |
| `Smart-Automatik Min Lüfterstufe` | `automatik_min_luefterstufe` | `automatik_min_fan_level` | 2 | Minimum speed (moisture base protection). |
| `Smart-Automatik Max Lüfterstufe` | `automatik_max_luefterstufe` | `automatik_max_fan_level` | 7 | Maximum speed (noise limiter for nights). |
| `Smart-Automatik: CO2 Grenzwert` | `auto_co2_threshold` | `auto_co2_threshold_val` | 1000 ppm | Target setpoint for the CO2 PID. |
| `Smart-Automatik: Feuchte Grenzwert` | `auto_humidity_threshold` | `auto_humidity_threshold_val` | 60 % | Target setpoint for the humidity PID. |
| `Smart-Automatik: Sommerkühlung Schwelle` | `auto_summer_cooling_threshold` | `summer_cooling_threshold` | 22 °C | Indoor temperature above which the summer bypass may engage (section 4). |
| `Sommerbetrieb` | `sommerbetrieb` | — | (binary) | Season gate from HA; `false` while HA is offline (heat recovery stays active). |
| `Klima-Koordination` | `smart_climate_control` | `hvac_enabled_val` | Off | Enables the HVAC coordination modifier, room-wide (section 6). |
| `Klima-Koordination: CO2 Grenzwert` | `hvac_co2_threshold` | `hvac_co2_threshold_val` | 1200 ppm | Relaxed CO2 setpoint while the AC is active. |
| `Klima-Koordination: Max Lüfterstufe` | `hvac_max_fan_level` | `hvac_max_fan_level_val` | 3 | Fan level cap while the AC is active. |
| `Klima-Koordination: CO2 Notfallgrenze` | `hvac_emergency_co2` | `hvac_emergency_co2_val` | 1500 ppm | CO2 emergency override threshold. |

All sliders and the HVAC switch are room-wide settings: a change on any device is sent to its peers as `MSG_STATE` and re-asserted by the Master's heartbeat.

---

> [!TIP]
> **Advanced Tuning**: The PID parameters ($K_p$, $K_i$) are defined in [`logic_pid.yaml`](../../packages/actuators/logic_pid.yaml). They are tuned for very slow, silent transitions to ensure the ventilation remains "forgotten" in the background. The derivative term ($K_d$) is explicitly set to zero — trend-based regulation would amplify sensor noise on the SCD43 and is unsuitable for residential ventilation.
