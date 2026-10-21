# 🧠 Heat Recovery, Sensor Physics & Efficiency Calculation

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_heat-recovery-and-efficiency.md)


This document provides in-depth technical details on the regenerative ceramic heat recovery cycle, phase-locked NTC temperature filtering, the BME680 IAQ engine, energy-based efficiency integration (DIN EN 13141-8), and multi-unit ESP-NOW push-pull synchronization.

---

### Operating Cycle (50s to 70s per phase)

```mermaid
graph LR
    A[Phase 1: EXHAUST 70s] -->|Ceramic heats up| B[Phase 2: SUPPLY 70s]
    B -->|Ceramic gives off heat| A
    
    style A fill:#ff6b6b
    style B fill:#4ecdc4
```

### Phase 1: Exhaust Air (Blowing Out) - 70 Seconds

```text
Interior (warm) → Ceramic heat exchanger → Exterior
    21°C              ↓ Store          5°C
                    heat
```

**What happens:**

- 🔥 Warm room air (21°C) flows through the ceramic heat exchanger
- 📈 Ceramic heats up and stores energy
- 🌡️ **Indoor NTC** measures the true room temperature at the end
- 💨 Cooled air (~10°C) is blown to the outside

### Phase 2: Supply Air (Blowing In) - 70 Seconds

```text
Exterior → Ceramic heat exchanger → Interior (pre-heated)
 5°C     ↑ Give off         ~16°C
        heat
```

**What happens:**

- ❄️ Cold outside air (5°C) flows through the warm ceramic heat exchanger
- 🔄 Ceramic gives off stored heat
- 🌡️ **Outdoor NTC** measures outside temperature
- 🌡️ **Indoor NTC** measures pre-heated supply air (~16°C)
- 🏠 Pre-heated air flows into the room

### NTC Sensors (Temperature Stabilization)

The NTC sensors measure the temperature at the ceramic heat exchanger inside and outside (`temp_zuluft` and `temp_abluft`). Since the fan direction in heat recovery mode changes cyclically (e.g., every 70 seconds), the sensors require a certain amount of time due to their thermal mass to adapt to the new air temperature. To make the measurement as accurate as possible, very small NTC sensors are used with the lowest possible mass and high accuracy. This makes the adaptation to the changing temperature, depending on the ventilation direction, as fast and precise as possible.
To avoid incorrect intermediate values in Home Assistant and to accurately capture the true thermal limits, both sensors use **intelligent, unified, and phase-aware temperature stabilization**:

- **Phase-Lock:** The system explicitly discards measurement values during the "wrong" airflow direction (e.g., indoor sensor during supply phase). This prevents the sliding window from being contaminated with recovered heat instead of true room air.
- **Thermal Wait:** After each change of direction (Push/Pull), measurement value transmission is paused for **40% of the cycle duration (min. 15s)** to allow the NTC to adapt to the new air stream.
- **Combined Filter Logic:** All stages (Phase-Lock, History Invalidation, Stability Check, and Seasonal Selection) are merged into a single, high-performance C++ function (`filter_ntc_combined`).
- **Dynamic Seasonal Selection:**
  - **Winter/Transition:** The outdoor sensor takes the minimum value (true cold outside air), and the indoor sensor takes the maximum value (true warm room air).
  - **Summer Cooling:** When outside air is hotter than inside air, the logic automatically reverses (outdoor takes max, indoor takes min).
- **Median Fallback:** If the reference sensor is temporarily unavailable, the system uses the median of the last 3 values as a safe compromise.
- **120s Failsafe Timeout:** A generous watchdog ensures the sensors stay "online" in Home Assistant even during long phases where values are legitimately blocked by the Phase-Lock.

*Note on redundancy:* `temp_zuluft` (Outdoor NTC) provides the actual outside temperature when the airflow is directed inward. `temp_abluft` (Indoor NTC) provides the room temperature when the airflow is directed outward and serves as redundancy for the more precise SCD43 sensor.

Specifically, the following sensor is used:

| Manufacturer | Part Number | Source | Accuracy | Data Sheet |
| :--- | :--- | :--- | :--- | :--- |
| **VARIOHM** | `ENTC-EI-10K9777-02` | [Reichelt Elektronik](https://www.reichelt.de/de/de/shop/produkt/thermistor_NTC_-40_bis_125_c-350474) | ± 0.2 °C | [PDF](../datasheets/ENTC_EI-10K9777-02E.pdf) |

### Air Quality & Gas Sensors (BME680)

To provide precise Indoor Air Quality (IAQ) data, the system features a highly optimized **BME680 Advanced IAQ Engine**. Since the BSEC2 library is too heavy and restricted, VentoSync uses a custom, thread-safe C++ implementation:

- **Optimized Heater Profile:** The gas sensor operates at **300°C for 150ms** (Bosch recommendation for IAQ). This reduces self-heating and extends the sensor's lifespan compared to default settings.
- **Dynamic Thermal Compensation:** Temperature readings are dynamically corrected based on ambient conditions (interpolated offset between -1.0°C and -2.0°C) to compensate for the heater's thermal impact.
- **Smart Flash Wear-Leveling:** The gas baseline is only persisted to the ESP32's flash memory if it has changed by more than **2%** and at least **1 hour** has passed. This maximizes the flash memory's longevity.
- **Health Watchdog:** A dedicated monitoring logic detects I2C communication failures or "stuck" values and reports a sensor health problem to Home Assistant after 10 consecutive failures.
- **Change-Detection Trend:** The IAQ trend and classification sensors use change-detection logic to minimize network traffic and database growth in Home Assistant.

### Efficiency Calculation (Energy-Based)

The true heat recovery efficiency of a ceramic regenerator over a complete cycle is energy-based, not based on instantaneous temperatures (according to DIN EN 13141-8).

At the end of the supply air phase, the system calculates the efficiency using **numerical trapezoidal integration** over the entire phase duration:

$$
 \eta_{WRG} = \frac{\int (\text{T}_{Supply} - \text{T}_{Outside}) dt}{\int (\text{T}_{Room} - \text{T}_{Outside}) dt}
$$

**Why this is mathematically superior:**
If the efficiency was calculated as a simple average of instantaneous point-in-time efficiencies, it would become highly inaccurate and numerically unstable (exploding values) when the temperature difference ($\Delta T$) is very small (e.g., during the transition seasons). By integrating the temperature deltas over time, the calculation remains physically accurate, stable, and provides a true representation of the thermal energy recovered during the cycle.

**Recovered Thermal Energy (Wh):**
In addition to the percentage efficiency, the system calculates the actual recovered thermal energy in **Watt-hours (Wh)**. This is achieved by a non-linear mapping of the 10 fan levels to the real volumetric flow rates of the Ventomaxx v-wrg-1 (ranging from approx. 17 m³/h at level 1 up to 43 m³/h at level 10) and integrating the actual temperature difference ($T_{supply} - T_{outside}$) over time. This allows you to track exactly how much heating energy (or cooling energy in summer) the system has "saved" during each cycle.

#### Supply Air Temperature after Heat Recovery

During the supply air phase the indoor NTC sits in the air that has just passed the ceramic and is blown into the room
— the phase-lock discards these readings for the extract air value, but the efficiency integration uses them as
$T_{Supply}$. Since 0.10.51 their **mean over each supply air phase** is published as `sensor.temp_zuluft_nach_wrg`
("Temp. Zuluft (nach WRG)") and shown in the web dashboard (card, *Wärmerückgewinnung* chart, statistics, plus the gain
over the outdoor air in K). The three temperatures make the heat recovery tangible, e.g. in winter:

| Outdoor air | Supply air after heat recovery | Extract air (room) | Efficiency |
|---|---|---|---|
| 2.0 °C | 18.4 °C | 21.0 °C | ≈ 86 % |

The mean covers the whole phase — at its start the ceramic is still warm, towards the end the supply air gets cooler —
so it matches the energy-based efficiency ($\eta \approx (T_{Supply} - T_{Outside}) / (T_{Room} - T_{Outside})$).
It is only published in heat recovery mode (incl. Smart-Automatik); in *Durchlüften*, *Stoßlüftung* and *Aus* there is
no recovery and the sensor is *unknown*. A phase longer than one half-cycle (e.g. left over from a mode change) is
discarded. The value needs all three NTC readings (supply, extract and outdoor air) — without NTCs (`nosensor`) the
entity stays hidden.

**Interpretation:**

- **> 70%:** Excellent heat recovery
- **50-70%:** Good heat recovery
- **< 50%:** Ceramic too cold, cycle too short, or temperature difference too small

### Optimizing Efficiency

| Parameter | Impact | Recommendation |
| :--- | :--- | :--- |
| **Cycle Duration** | Longer cycles = better storage | 70-90s optimal |
| **Fan Speed** | Slower = more heat transfer | 60-80% |
| **Ceramic Volume** | More mass = more storage | Larger is better |
| **Outside Temperature** | Colder = higher efficiency possible | - |

### Synchronization of Multiple Devices

When using several devices in the same room:

**Pair Operation (2 devices):**

```text
Device A: Phase A (Supply)  ←→  Device B: Phase B (Exhaust)
         ↓ 70s switch ↓
Device A: Phase B (Exhaust) ←→  Device B: Phase A (Supply)
```

**Advantages:**

- ✅ Continuous air exchange
- ✅ No pressure fluctuations
- ✅ Optimal heat recovery
- ✅ Synchronized via ESP-NOW
