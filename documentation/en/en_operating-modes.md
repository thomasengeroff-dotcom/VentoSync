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
| **1** | **🤖 Smart Automatic** *(Standard)* | 🟢 *(pulses)* / ⚫ | Dynamic PID (Levels 1–10) based on CO2 & Humidity | 50s – 70s dynamic | `select.luftermodus` → `Smart-Automatik` |
| **2** | **❄️ Heat Recovery** *(Eco)* | 🟢 / ⚫ | Constant manual level (1–10) with push-pull heat exchange | 50s – 70s dynamic | `select.luftermodus` → `Wärmerückgewinnung` |
| **3** | **🌬️ Cross-Ventilation** *(Summer)* | 🟢 / 🟢 | Constant airflow without direction change (Phase A in, Phase B out) | Continuous / Timer | `select.luftermodus` → `Durchlüften` |
| **4** | **💨 Boost Ventilation** | ⚫ / 🟢 | One-way burst at the manual level (Phase A in, Phase B out), then pause; direction inverted every second burst | 2 h cycle (15 min burst / 105 min pause) | `select.luftermodus` → `Stoßlüftung` |
| **5** | **⭕ Off** *(Monitoring)* | ⚫ / ⚫ | Fan stopped (0 RPM), room-wide; all sensors, Wi-Fi & web UI remain fully active | — | `select.luftermodus` → `Aus` |

---

## ⚙️ Detailed Mode Descriptions

### 1. 🤖 Smart Automatic *(Standard / Recommended)* — `LED_WRG` 🟢 (pulses slowly)

**This mode is the standard upon powering on** and handles all ventilation tasks autonomously ("Set and forget"). The system regulates itself continuously based on indoor and outdoor environmental sensor data.

#### Active Smart Features

| Feature | Sensor(s) | Threshold / Control Method |
| :--- | :--- | :--- |
| ✅ **CO2 Control (PID)** | SCD43 (`sensor.scd41_co2`) | `number.smart_automatik_co2_grenzwert` (Target, e.g. 800 ppm) |
| ✅ **Humidity Management (PID)** | SCD43 (`sensor.scd41_luftfeuchtigkeit`) + HA `sensor.outdoor_humidity` | Dehumidification via absolute humidity check |
| ✅ **Summer Cooling Function** | NTC sensors + ESP-NOW group temperature + HA `binary_sensor.sommerbetrieb` | Indoor threshold slider (default 22°C), outdoor ≥ 1.5°C cooler |
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
6. **Room-wide Demand Fusion:** Every device broadcasts the demand of its **own** CO2 and humidity PIDs via ESP-NOW (no sensors → no value). Each device regulates on the **maximum** of its local demand and the freshest (≤ 5 min) demand of every peer. A device without its own sensors (the `nosensor`, `radar_only` and `NTConly` variants — e.g. a Master without SCD41) therefore follows the sensor device with its full PID behaviour, for CO2 **and** humidity. Adopted values are never re-broadcast, so a high demand cannot latch between devices.

> [!TIP]
> For the complete technical background and C++ logic implementation, see **[📄 smart-automatic-logic.md](en_smart-automatic-logic.md)** and **[📄 humidity-management.md](en_humidity-management.md)**.

---

### 2. ❄️ Heat Recovery (Eco Recovery) — `LED_WRG` 🟢 (solid)

- **HA Entity:** `select.luftermodus` → `Wärmerückgewinnung`
- **Function:** Manual heat recovery operation without automatic PID scaling. The air direction changes periodically and the ceramic storage mass recovers the heat of the exhaust air (the manufacturer quotes up to 85 %; the firmware measures the actual value with the NTC sensors as `sensor.wrg_effizienz` ("WRG Effizienz"), see [Heat Recovery Efficiency](en_heat-recovery-and-efficiency.md)).
- **Fan level:** Constant manual level (1–10), changed via the +/- buttons, the HA fan entity or the dashboard. When switching from Smart Automatic, the level last set by the automatic is kept as the starting point.
- **Direction interval:** The time per air direction depends on the fan level — `round(70 − (level − 1) · 20/9)` seconds:

  | Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  | :--- | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
  | Seconds per direction | 70 | 68 | 66 | 63 | 61 | 59 | 57 | 54 | 52 | 50 |

  A full push-pull cycle (in **and** out) takes twice as long (140 s … 100 s). Each direction phase starts with a 5 s soft ramp-up and ends with a 5 s ramp-down, so the fan runs at full speed for the interval minus 10 s.
- **Synchronization:** Device pairs operate in push-pull arrangement (Phase A supplies fresh air while Phase B exhausts stale air), keeping room pressure balanced. The Master (device ID 1) keeps the direction phase and the fan level of all devices in the room aligned via ESP-NOW.
- **Presence adjustment:** With the slider `number.radar_lufter_anpassung` ("Radar Lüfter-Anpassung", -5 … +5, `0` = off) the level is shifted **while presence is detected** — e.g. `+2` for more air while occupied, `-2` for quieter operation while occupied. Nothing is applied without presence. Detection is **room-wide**: as soon as any device of the room with an LD2450 radar (`full` / `radar_only` variant) detects a person, every device applies the same offset, so supply and exhaust stay balanced. Presence is held for 30 s after the last detection. The adjustment applies in all manual modes (Heat Recovery, Cross-Ventilation, Boost), never in Smart Automatic; the panel LEDs keep showing the base level.

---

### 3. 🌬️ Cross-Ventilation / Ventilation (Summer Mode) — `LED_WRG` 🟢 + `LED_VEN` 🟢 (solid)

- **HA Entity:** `select.luftermodus` → `Durchlüften` + `number.durchluften_dauer_min` ("Durchlüften Dauer (min)", 0–120 min in 5-min steps, default 30, **0 = continuous**)
- **Function:** Unidirectional constant airflow without periodic direction reversal (no 5 s direction ramps).
- **Operation:** Phase-A units continuously pull outside air in, while Phase-B units continuously blow inside air out, creating an effective cross-draft through the living area for passive night cooling.
- **Fan level:** Manual level (1–10); the room-wide presence adjustment applies (see Heat Recovery).
- **Timer:** The timer starts when the mode is selected. When it expires, the room returns to **Heat Recovery** — the HA select, the fan preset and the panel LEDs switch to `Wärmerückgewinnung` accordingly. With `0` the mode runs until another mode is selected. Changing the timer while the mode runs takes effect relative to the original start.
- **Automatic trigger (Smart Automatic only):** Smart Automatic switches to continuous cross-ventilation on its own (no timer; the panel keeps the pulsing `LED_WRG`) when **all** of the following hold:
  - the HA binary sensor `binary_sensor.sommerbetrieb` is `on` (HA template: April–October **and** outdoor > 18 °C; without HA it counts as off → no bypass),
  - indoor temperature > the slider "Smart-Automatik: Sommerkühlung Schwelle" (18–30 °C, default **22 °C**),
  - outdoor temperature at least **1.5 °C** below indoor,
  - Smart Climate Control does not report an active AC (heat recovery is enforced while the AC runs).

  It switches back to heat recovery when `sommerbetrieb` turns off, outdoor ≥ indoor − 0.5 °C, or indoor < threshold − 0.5 °C. There is no time-of-day condition — "night cooling" results from the temperature condition.
- **Temperatures during cross-ventilation:** With constant airflow each unit can measure only one of its two NTCs (intake units: outdoor, exhaust units: indoor). The missing value comes from a peer of the room, otherwise the unit's last measurement is used for up to 30 min. If no temperature is available at all (e.g. a single Phase-B unit), the automatic returns to heat recovery for at least 5 min to re-measure both temperatures before it may re-enter the bypass.

---

### 4. 💨 Boost Ventilation — `LED_VEN` 🟢 (solid)

- **HA Entity:** `select.luftermodus` → `Stoßlüftung`
- **Function:** Burst ventilation for rapid air renewal (e.g., after cooking or showering). Runs until another mode is selected.
- **2-Hour Sequence:**
  - **15 minutes burst:** The fan runs in **one direction** — like `Durchlüften`, Phase A devices blow in, Phase B devices extract. There is no push-pull alternation during the burst: the air leaves the room directly, which exchanges more air and removes moisture better than heat recovery (no heat recovery during the burst).
  - **105 minutes pause:** Fan stopped (0 RPM), the ceramic core regenerates.
  - **Soft start / stop:** 5-second ramp at the start and at the end of each burst.
- **Level:** The burst runs at the **manually set fan level** (`number.lufter_intensitat`, 1–10) plus the radar presence offset — there is no separate boost level. For an intensive burst select a high level; the vacation mode uses this mode deliberately at level 1.
- **Alternating Direction:** Every second burst inverts the direction (Phase A extracts, Phase B blows in), so the ceramic cores and both sides of the building are loaded evenly. The direction only changes during the pause, never under load.
- **Room synchronization:** The Master (device ID 1) shares its position in the 4-hour schedule (two bursts) with every heartbeat; all devices of the room pause and burst together and push-pull pairs always run in opposite directions — also after a device restarts.
- **Winter note:** Without heat recovery the supply side draws in outdoor air for 15 of 120 minutes. If that is undesirable (e.g., during vacation in winter), use `Wärmerückgewinnung` instead (vacation: `select.urlaubsmodus_betriebsmodus`).

---

### 5. ⭕ Off (Monitoring Mode) — both mode LEDs ⚫

- **HA Entity:** `select.luftermodus` → `Aus` (or HA fan entity *off*)
- **Function:** The fan motor is stopped (50 % PWM = standstill, 0 RPM). Only the power LED stays lit (dimmed after 30 s).
- **Room-wide:** Like every other mode, `Aus` applies to the **whole room** — switching off on any device (HA, web dashboard, Mode or Power button) stops all devices of the room, and switching on on any device starts them all again.
- **Active Sensors:** Wi-Fi, Home Assistant API, web dashboard, ESP-NOW and all sensors (SCD43, BMP390, BME680, radar, NTCs) stay active for uninterrupted data collection; the device keeps sharing its sensor data with the room.
- **Power button:** A press (< 10 s) toggles between `Aus` and the **last active mode** (default `Smart-Automatik`) — room-wide. The HA fan entity's *turn on* restores the same mode. A very long press (> 10 s) restarts the device; the mode is kept.
- **No sleep mode:** The hardware (rev. 1 PCB) cannot wake the ESP32 from deep sleep with a button, so there is no power-saving sleep state. The former long press (> 5 s, Wi-Fi off) was removed in 0.10.26 — it saved almost nothing (CPU, sensors and radar kept running) and ended by itself after 15 minutes.

---

## 🔗 Related Documentation

- **[📄 Control Panel Operation Guide](en_control-panel-operation.md)** — Button actions, LED brightness levels, and blink diagnostic codes.
- **[📄 Automatic Mode Logic Deep Dive](en_smart-automatic-logic.md)** — Architectural details and C++ state engine.
- **[📄 Humidity Management & HA Sensor Setup](en_humidity-management.md)** — Absolute humidity formulas and template sensor setup.
- **[📄 ESP-NOW Communication Protocol](en_esp-now-communication.md)** — Room group discovery and unicast synchronization.
