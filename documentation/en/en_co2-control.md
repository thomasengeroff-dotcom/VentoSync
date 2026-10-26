# 🤖 CO2-Driven Adaptive Fan Control

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_co2-control.md)

## Overview

The CO2 automation automatically adjusts the ventilation intensity based on the current carbon dioxide level in the room.
As CO2 rises, the fan speed increases step by step — when the level decreases, the fan gently scales back down.
A configurable **Minimum Fan Level** ensures that base ventilation for moisture protection remains active even during periods of absence (low CO2), adhering to DIN 1946-6.

**Requirement:** An SCD43 CO2 sensor must be connected to the I2C bus (Header H2).
Without a connected sensor, this automated function remains inactive.

> **SGP41 + SHT45 board (`ventosync_sgp41` / `ventosync_v2_sgp41`):** instead of real CO2, the SGP41 VOC index is mapped to a CO2 equivalent (`400 + 4 × (index − 50)`, 400–2200 ppm; index 200 ≙ 1000 ppm) and drives the same CO2 regulation. The VOC index is relative to the last 24 h — it reacts to events (cooking, showering, cleaning agents), not to a constant pollution level.

---

## Thresholds (DIN EN 13779 / Federal Environment Agency)

The threshold values are aligned with DIN EN 13779 and the recommendations of the German Federal Environment Agency (UBA) for indoor air quality:

| CO2 (ppm) | Rating | Fan Level | Description |
| ---------:| :----- | :-------: | :---------- |
|    ≤ 600  | Excellent | 1 | Minimal speed, virtually silent |
|    ≤ 800  | Good | 2 | Quiet continuous operation |
|   ≤ 1000  | Moderate | 3 | Pettenkofer threshold (standard ventilation) |
|   ≤ 1200  | Elevated | 5 | Noticeable ventilation |
|   ≤ 1400  | Poor | 7 | Intensive ventilation |
|   > 1400  | Unacceptable | 9 | Maximum ventilation (emergency mode) |

> **Note:** Outdoor ambient air typically has 400–420 ppm CO2.
> The Pettenkofer threshold (1000 ppm) has been a recognized benchmark for acceptable indoor air for over 150 years.

---

## Hysteresis (Anti-Oscillation)

To prevent constant hunting near threshold boundaries, the controller utilizes a **100 ppm hysteresis band**:

- **Ramp-Up:** Triggers exactly at the threshold (e.g., 1000 ppm → Level 5)
- **Ramp-Down:** Only triggers 100 ppm below the threshold (e.g., 900 ppm → permits drop below Level 5)

```text
                CO2 (ppm)
                   ▲
    1400 ──────────┤─── Level 9 ────────── ▲ up
    1300 ──────────┤                       ▼ down (Hysteresis)
    1200 ──────────┤─── Level 7 ────────── ▲ up
    1100 ──────────┤                       ▼ down
    1000 ──────────┤─── Level 5 ────────── ▲ up
     900 ──────────┤                       ▼ down
     800 ──────────┤─── Level 3 ────────── ▲ up
     700 ──────────┤                       ▼ down
     600 ──────────┤─── Level 2 ────────── ▲ up
     500 ──────────┤                       ▼ down
                   └──────────────────────
```

---

## Noise Control (Max-Level Limiter)

The **"CO2 Max Lüfterstufe"** slider in Home Assistant caps the maximum speed the CO2 automation is permitted to request.

| Setting | Purpose |
| :-----: | :------ |
|   10    | Full power permitted (no noise limit) |
|    7    | **Standard** — optimal compromise between acoustic comfort and airflow |
|    5    | Moderate ventilation, very quiet |
|    3    | Minimal ventilation, whisper-quiet |

> **Tip:** In **bedrooms**, a max level of **5** or lower is recommended.
> In **living rooms** or **offices**, levels **7–8** provide a great balance.

---

## Base Ventilation (Min-Level) — Moisture Protection

The **"CO2 Min Lüfterstufe"** slider guarantees a **minimum continuous ventilation baseline**, even when CO2 levels are very low (e.g. unoccupied room, night). This is essential for:

- **Moisture Protection** — Mold prevention during absences (DIN 1946-6: "Ventilation for moisture protection")
- **VOC Dissipation** — Off-gassing from furniture, building materials, and paints
- **Air Hygiene** — Baseline air exchange against stale air buildup

| Setting | Recommendation |
| :-----: | :------------- |
|    1    | Absolute minimum (only for well-ventilated rooms) |
|    2    | **Standard** — quiet baseline for moisture protection |
|    3    | For rooms with elevated moisture loads (bathrooms, kitchens) |
|    4    | For new builds with high residual construction moisture |

> **Note:** In CO2 mode, the fan **never regulates below this minimum level** — even when CO2 drops to 400 ppm. This preserves building fabric integrity.

---

## Activation

### Home Assistant

1. **Switch:** `CO2 Automatik` → On/Off
2. **Slider:** `CO2 Min Lüfterstufe` → 1–10 (Default: 2, Moisture Protection)
3. **Slider:** `CO2 Max Lüfterstufe` → 1–10 (Default: 7, Noise Control)

When enabled, the system verifies that the SCD43 sensor is connected. If unavailable (`NaN`), the automation remains inactive and a warning is logged.

### On-Device Control Panel

CO2 automation is primarily configured via Home Assistant or the web dashboard.

---

## Architecture

```text
┌─────────────┐    30s Interval     ┌──────────────────────┐
│ SCD43 Sensor │ ──── CO2 ppm ────▶ │ apply_co2_auto_control│
│ (I2C @ H2)  │                     │ (automation_helpers.h) │
└─────────────┘                     └──────────┬───────────┘
                                               │
                                    ┌──────────▼───────────┐
                                    │  VentilationLogic::   │
                                    │  get_co2_fan_level()  │
                                    │  (Hysteresis + Clamp) │
                                    └──────────┬───────────┘
                                               │
                                    ┌──────────▼───────────┐
                                    │  Adjust Fan PWM       │
                                    │  Update Panel LEDs   │
                                    │  Publish HA State    │
                                    └──────────────────────┘
```

### Files & Responsibilities

| File | Content |
| :--- | :--- |
| `components/ventilation_logic/ventilation_logic.h/.cpp` | Pure C++ logic: `get_co2_fan_level()`, `get_co2_classification()` |
| `components/helpers/auto_mode.h` | Core evaluation engine: `calculate_combined_demand()` |
| `packages/actuators/logic_pid.yaml` | PID controllers and dummy output bindings |
| `ventosync.yaml` | Globals, switches, sliders, text sensors, and evaluation intervals |

---

## Troubleshooting

| Issue | Solution |
| :--- | :--- |
| CO2 automation cannot be activated | SCD43 not connected → Connect to header H2 |
| Fan does not react to CO2 increase | Check the `CO2 Automatik` switch state in HA |
| Fan speed is too loud | Lower the `CO2 Max Lüfterstufe` slider |
| Fan never turns off completely | Check `CO2 Min Lüfterstufe` — this is intended behavior (moisture protection) |
| Log: "SCD43 not connected" | Verify I2C wiring (SDA/SCL, cable continuity) |
| CO2 readings fluctuate sharply | Median filtering is built into the SCD43 sensor driver (`window_size: 5`) |
| CO2 / temperature / humidity of the SCD43 become `unknown` | The SCD43 has not answered for > 2 min (or was not found at boot). The firmware then sets its values to NaN instead of freezing the last reading — `Effektiver CO2 Wert` turns NaN after 5 more minutes and the room temperature falls back to the indoor NTC. Check the sensor board at H2 (log: `Sensor '…' stale`) |
