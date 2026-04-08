# 🔌 Guide: Universal Fan Interface (4-Pin Screw Terminal)

This guide describes the circuitry for a **universal fan control system**, implemented via a standard **4-pin screw terminal**. This interface supports both standard 4-pin PWM fans and special 3-pin VentoSync/VentoMaxx fans (ebm-papst VarioPro) by simply adjusting the wiring.

> ℹ️ **Note on 3-Pin PWM fans:**
> In addition to classic 4-pin PWM fans, there are special fans that do **not have a tacho signal** and therefore only have **3 pins** (GND, 12V, PWM). This is also the case for the EBM-Papst fan installed by VentoMaxx.
> These can be operated without physical changes to the circuit by simply leaving the tacho pin (Pin 3 at the terminal) unused. Note, however, that without a tacho signal, no direct monitoring of the speed (RPM) or stall detection is possible via software.

## 🛠️ Functional Principle: Open-Drain PWM

The control is based on a simple **Open-Drain PWM driver**:

- **4-Pin PWM Fan**: The PWM input (Pin 4) is pulled to GND by the MOSFET. The internal pull-up in the fan provides the High signal.

> ✅ **Verified**: This logic was successfully tested with `espslaveNTC.yaml` and 2kHz PWM.

---

## 1. Pin Assignment (PCB & Terminal)

We use a standardized **4-pin screw terminal** on the PCB.

| Terminal Pin | Function | Description | ESP32 GPIO |
| :--- | :--- | :--- | :--- |
| **1** | **GND** | Common Ground | - |
| **2** | **+12V** | Constant 12V supply voltage | - |
| **3** | **TACHO** | Speed signal (Input) | GPIO17 (with pull-up) |
| **4** | **PWM** | PWM control signal (Open-Drain Output) | **GPIO6** (via MOSFET) |

---

## 2. Fan Wiring

By connecting the wires differently to the screw terminal, both fan types are supported.

### A. Standard 4-Pin PWM Fan (PC fan, ebm-papst AxiRev)

Colors may vary (often black/yellow/green/blue), but the pin sequence on the connector is standardized.

| Fan Function | Terminal Pin |
| :--- | :--- |
| **GND** (Pin 1) | -> **1 (GND)** |
| **+12V** (Pin 2) | -> **2 (+12V)** |
| **Tacho** (Pin 3) | -> **3 (TACHO)** |
| **PWM** (Pin 4) | -> **4 (PWM)** |

### B. VentoMaxx / ebm-papst VarioPro (3-Pin)

This fan typically has open wires or a special connector.
**IMPORTANT**: Pay attention to the wire colors! (Red = +12V, Blue = GND, White = Signal/PWM - *Example, please check datasheet!*).
According to user validation:

- Wire 1: +12V
- Wire 2: GND
- Wire 3 (Center): Control Signal (PWM)

| Fan Wire | Function | Terminal Pin |
| :--- | :--- | :--- |
| **Wire 2** | GND / Ground | -> **1 (GND)** |
| **Wire 1** | +12V DC | -> **2 (+12V)** |
| **-** | (Not used) | **3 (TACHO)** (leave empty) |
| **Wire 3** | Control Signal | -> **4 (PWM)** |

> ℹ️ **Note**: The control signal of the VentoMaxx is connected to the **PWM pin** of the terminal, as it is controlled by the same MOSFET as the PWM input of a 4-pin fan.

---

## 3. Guide for EasyEDA Pro

### Step 1: Adding (Universal Circuit)

We only need a single MOSFET for the PWM channel.

1. **Component**: Place **1x N-channel MOSFET** (e.g., **PMV16XNR** or 2N7002, SOT-23). Let's call it **Q_PWM**.
2. **Source**: Connect the Source of Q_PWM directly to **GND**.
3. **Gate**:
    - Connect the Gate of Q_PWM to **GPIO6** via a **1kΩ resistor (R_Gate)**.
    - Add a **10kΩ pull-down (R_PD)** directly from the Gate to GND (ensures "off" state during boot).
4. **Drain**: Connect the Drain of Q_PWM directly to **Terminal Pin 4 (PWM)**.
5. **Protection**: (Optional but recommended) Add a small **Schottky diode (e.g., B5819WS)** between Drain (cathode) and GND (anode) as ESD/induction protection if long cables are used.

### Step 2: Tacho & Power

1. **Terminal Pin 1**: Directly to **GND Plane**.
2. **Terminal Pin 2**: Directly to **+12V**.
3. **Terminal Pin 3 (Tacho)**:
    - Connect to **GPIO17**.
    - Add a **4.7kΩ - 10kΩ pull-up resistor** to +3.3V (important for open-collector tacho).
    - Add a **1kΩ series resistor** for GPIO protection.
    - Optional: Zener diode (3.3V) to protect the GPIO from overvoltage.

### Schematic Summary (New)

```
       +12V ─────────────────────────────── Terminal Pin 2
                                         ┌─ Terminal Pin 1
         GND ─────────────────────────────┴─ Source (Q_PWM)

GPIO6 ──[1k]──┬── Gate (Q_PWM)
              │
             [10k]
              │
             GND

Drain (Q_PWM) ───────────────────────────── Terminal Pin 4 (PWM Output)

                                         ┌─ +3.3V
                                         │
                                       [10k]
                                         │
GPIO17 ──[1k]────────────────────────────┼─ Terminal Pin 3 (Tacho Input)
```

---

## 4. ESPHome Configuration (Software)

This configuration corresponds to the successfully tested `espslaveNTC.yaml`.

```yaml
# PWM Output Configuration
output:
  - platform: ledc
    pin: GPIO6
    id: fan_pwm_output
    frequency: 2000Hz  # IMPORTANT for VentoMaxx/VarioPro! (PC fans often 25kHz, but 2kHz mostly ok too)

# Tacho Sensor (Optional)
sensor:
  - platform: pulse_counter
    pin: GPIO17
    name: "Fan RPM"
    id: fan_rpm
    unit_of_measurement: 'RPM'
    filters:
      - multiply: 0.5  # Standard PC Fan: 2 pulses per revolution

# Fan Component (Optional, for integration into HA)
fan:
  - platform: speed
    output: fan_pwm_output
    name: "Living Space Ventilation"
```

> [!TIP]
> **Frequency**: 2000 Hz is **mandatory** for the VentoMaxx.
> Standard PC fans often specify 25 kHz but usually run without problems at 2 kHz. If the PC fan makes whining noises, you could increase the frequency – however, the VentoMaxx strictly requires ~2 kHz.
