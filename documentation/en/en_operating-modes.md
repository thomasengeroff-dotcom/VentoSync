# 🔄 Operating Modes & Program Logic

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_operating-modes.md)


VentoSync provides 5 distinct operating modes to balance indoor air quality, thermal efficiency, acoustic comfort, and passive cooling. The device can be controlled either via the physical **Mode button (M)** on the unit, the local web dashboard, or via Home Assistant.

---

## 🔘 Switching Modes & Button Sequence

Pressing the **Mode button (M)** cycles through the programs in the following order:

```text
Auto (Smart) ──► Heat Recovery (Eco) ──► Ventilation (Cross-Vent) ──► Boost Ventilation ──► Off ──► Auto...
```

Upon initial power-on or microcontroller reset, **Mode 1 (Smart Automatic)** is active by default.

---

## 📊 Overview of Operating Modes

| # | Mode | Panel LEDs (`WRG` / `VEN`) | Fan Behavior | Cycle Time | HA Entity / Selection |
| :-: | :--- | :---: | :--- | :--- | :--- |
| **1** | **🤖 Smart Automatic** *(Standard)* | 🟢 *(pulses)* / ⚫ | Dynamic PID (Levels 1–10) based on CO2 & Humidity | 50s – 70s dynamic | `select.luefter_modus` → `Smart-Automatik` |
| **2** | **❄️ Heat Recovery** *(Eco)* | 🟢 / ⚫ | Constant manual level (1–10) with push-pull heat exchange | 50s – 70s dynamic | `select.luefter_modus` → `Wärmerückgewinnung` |
| **3** | **🌬️ Cross-Ventilation** *(Summer)* | 🟢 / 🟢 | Constant airflow without direction change (Phase A in, Phase B out) | Continuous / Timer | `select.luefter_modus` → `Durchlüften` |
| **4** | **💨 Boost Ventilation** | ⚫ / 🟢 | Intensive ventilation (15 min run, 105 min pause) | Continuous (15 min) | `select.luefter_modus` → `Stoßlüftung` |
| **5** | **⭕ Off** *(Monitoring)* | ⚫ / ⚫ | Fan stopped (0 RPM); all sensors & web UI remain fully active | — | `select.luefter_modus` → `Aus` |

---

## ⚙️ Detailed Mode Descriptions

### 1. 🤖 Smart Automatic *(Standard / Recommended)* — `LED_WRG` 🟢 (pulses slowly)

**This mode is the standard upon powering on** and handles all ventilation tasks autonomously ("Set and forget"). The system regulates itself continuously based on indoor and outdoor environmental sensor data.

#### Active Smart Features

| Feature | Sensor(s) | Threshold / Control Method |
| :--- | :--- | :--- |
| ✅ **CO2 Control (PID)** | SCD43 (`sensor.scd41_co2`) | `number.auto_co2_threshold` (Target, e.g. 800 ppm) |
| ✅ **Humidity Management (PID)** | SCD43 (`sensor.scd41_humidity`) + HA `sensor.outdoor_humidity` | Dehumidification via absolute humidity check |
| ✅ **Summer Cooling Function** | NTC sensors + ESP-NOW group temperature | 22°C indoor temperature threshold |
| ✅ **Group Unicast Sync** | ESP-NOW | Synchronizes fan levels and sensor demand across all units in the room |

#### Logic in Detail

- **Basic Operation:** Continuous heat recovery (`MODE_ECO_RECOVERY`) at the configured minimum fan level (`automatik_min_luefterstufe`, default: Level 2). Change intervals adapt dynamically to fan speed (70s at Level 1 to 50s at Level 10).
- **🎛️ Intelligent PID Control (CO2 & Humidity):** Instead of noisy binary switching, VentoSync uses a dual-loop PID controller:

  > **What is a PID controller?**
  > Think of driving a car: if you are barely over the speed limit you ease off the accelerator only slightly; if you are far over it you brake harder; and if you have been slightly over it for a while you apply a little more brake. VentoSync treats CO2 and humidity the same way — no abrupt switching, just gentle, continuous correction.

  - **P (Proportional):** Reacts instantly to deviations above the threshold.
  - **I (Integral):** Slowly accumulates persistent deviations (e.g., several people in a room) and gently increases fan levels over time.
  - **Gentle Tuning:** The I-gain is tuned extremely slowly (`0.0000005`) to ignore short-term spikes (e.g. opening a bottle of carbonated water).

#### Real-World Example (CO2 Target: 800 ppm, Level Range: 2–7)

| Elapsed Time | CO2 Reading | Action & Fan Response |
| :--- | :--- | :--- |
| **0 min** | 820 ppm | Slight deviation (+20 ppm) → Proportional demand small → **Fan stays at Level 2 (Min)** |
| **15 min** | 870 ppm | Elevated (+70 ppm), Integral slowly builds → **Fan stays at Level 2** |
| **30 min** | 920 ppm | Persistent deviation (+120 ppm), Integral accumulated → **Fan smoothly steps to Level 3** |
| **50 min** | 960 ppm | Continuous demand → **Fan steps to Level 4** |
| **70 min** | 900 ppm | Air quality improves, Integral decays → **Fan steps down to Level 3** |
| **90 min** | 790 ppm | Below threshold → Demand resets to zero → **Fan returns to Level 2 (Min)** |

#### Key Behavior Rules

1. **Ramp Rate Limiting:** The fan speed changes by **at most ±1 level per 10-second evaluation cycle** to prevent audible jumps.
2. **Bounds Enforcement:** The speed never drops below `automatik_min_luefterstufe` (Level 2) and never exceeds `automatik_max_luefterstufe` (Level 7 by default).
3. **Signal Arbitration:** The system takes the **maximum** of CO2 demand and Humidity demand, ensuring neither parameter is neglected.
4. **Smooth Mode Entry:** Switching *into* Smart Automatic resets PID integrals to zero so the fan always starts at the minimum level and ramps up only if needed.
5. **Absolute Humidity Guard:** Dehumidification only increases fan speeds if outdoor absolute humidity is actually lower than indoor air (using the Magnus formula). If outdoor air is more humid (e.g. raining), humidity ventilation demand is set to 0.
6. **Group Fallback:** A device without its own sensors (the `nosensor`, `radar_only` and `NTConly` variants) automatically adopts the highest demand of the room group via ESP-NOW.

> [!TIP]
> For the complete technical background and C++ logic implementation, see **[📄 smart-automatic-logic.md](en_smart-automatic-logic.md)** and **[📄 humidity-management.md](en_humidity-management.md)**.

---

### 2. ❄️ Heat Recovery (Eco Recovery) — `LED_WRG` 🟢 (solid)

- **HA Entity:** `select.luefter_modus` → `Wärmerückgewinnung`
- **Function:** Manual heat recovery operation without automatic PID scaling. The air direction changes periodically, recovering up to 85% of thermal energy.
- **Cycle Times:** Dynamically match the selected fan level:
  - Level 1: **70 seconds**
  - Level 5: **60 seconds**
  - Level 10: **50 seconds**
- **Synchronization:** Device pairs operate in push-pull arrangement (Phase A supplies fresh air while Phase B exhausts stale air), keeping room pressure balanced.
- **Presence Boost:** When presence detection is enabled, the fan speed can optionally adjust by `-5` to `+5` levels based on occupancy.

---

### 3. 🌬️ Cross-Ventilation / Ventilation (Summer Mode) — `LED_WRG` 🟢 + `LED_VEN` 🟢 (solid)

- **HA Entity:** `select.luefter_modus` → `Durchlüften` + `number.vent_timer` (Timer, 0 = continuous)
- **Function:** Unidirectional constant airflow without periodic direction reversal.
- **Operation:** Phase-A units continuously pull outside air in, while Phase-B units continuously blow inside air out, creating an effective cross-draft through the living area for passive night cooling.
- **Automatic Trigger:** In Smart Automatic mode, cross-ventilation activates automatically during summer nights when indoor temperature exceeds 22°C and outdoor temperature is lower by at least 1.5°C.

---

### 4. 💨 Boost Ventilation — `LED_VEN` 🟢 (solid)

- **HA Entity:** `select.luefter_modus` → `Stoßlüftung`
- **Function:** Intensive burst ventilation for rapid air renewal (e.g., after cooking or showering).
- **2-Hour Sequence:**
  - **15 minutes:** High-intensity ventilation at the configured boost level.
  - **105 minutes:** Idle pause (0 RPM) allowing the ceramic core to regenerate and moisture to dissipate.
  - **Cycle Repeat:** Repeats automatically every 2 hours until deactivated.
- **Alternating Direction:** Each boost burst alternates starting direction to maintain thermal and moisture balance in the ceramic core.

---

### 5. ⭕ Off (Monitoring Mode) — both LEDs ⚫

- **HA Entity:** `select.luefter_modus` → `Aus`
- **Function:** The fan motor and PWM drive are completely shut down (0 RPM).
- **Active Sensors:** Environmental sensors (SCD43 CO2/temp/humidity, BMP390, BME680, Radar presence) and the local web dashboard remain active for uninterrupted data collection in Home Assistant.
- **Ultra-Low-Power Light Sleep:** Long-pressing the physical Power button for **> 5s** enters deep light sleep (disables Wi-Fi, LEDs, and radar; power consumption < 0.1W). A single short press immediately wakes the unit and reconnects to the network.

---

## 🔗 Related Documentation

- **[📄 Control Panel Operation Guide](en_control-panel-operation.md)** — Button actions, LED brightness levels, and blink diagnostic codes.
- **[📄 Automatic Mode Logic Deep Dive](en_smart-automatic-logic.md)** — Architectural details and C++ state engine.
- **[📄 Humidity Management & HA Sensor Setup](en_humidity-management.md)** — Absolute humidity formulas and template sensor setup.
- **[📄 ESP-NOW Communication Protocol](en_esp-now-communication.md)** — Room group discovery and unicast synchronization.
