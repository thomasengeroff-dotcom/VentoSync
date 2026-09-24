# 🌟 Comfort & Safety Features

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_comfort-and-safety-features.md)


This document describes the advanced control, comfort, and protection features of **VentoSync**.

---

## 📑 Table of Contents

- [📈 Phase Position Continuity & Dynamic Cycle Adaptation](#-phase-position-continuity--dynamic-cycle-adaptation)
- [🌊 Soft Start (Slew-Rate Limiter)](#-soft-start-slew-rate-limiter)
- [⚙️ Virtual RPM Calculation & Tachometer](#️-virtual-rpm-calculation--tachometer)
- [🔄 Plain Text Direction Display](#-plain-text-direction-display)
- [🌴 Vacation Mode](#-vacation-mode)
- [🔒 Child Protection Mode](#-child-protection-mode)

---

## 📈 Phase Position Continuity & Dynamic Cycle Adaptation

When adjusting the ventilation level (e.g. from level 2 to level 6 in Auto or Manual mode), the overall heat recovery cycle duration changes (e.g. from 65s to 50s).

To prevent abrupt resets or prematurely reversing fan direction, VentoSync proportionally scales the remaining cycle time:

$$\text{New Remaining Time} = \text{New Total Duration} \times \left(1 - \frac{\text{Elapsed Time}}{\text{Old Total Duration}}\right)$$

* **Benefit**: The fan seamlessly continues its active heat absorption/release cycle without disrupting the thermal regeneration balance.

---

## 🌊 Soft Start (Slew-Rate Limiter)

To protect the motor electronics and minimize acoustic disruption, all speed transitions are regulated via a software slew-rate limiter.

* **Ramp Speed**: ~**5% PWM per second**
* **Smooth Reversals**: During directional changes (Heat Recovery) and at the start / end of each Boost Ventilation burst, the fan follows a smooth 5-second deceleration and acceleration curve.
* **Benefit**: Prevents voltage dips and current spikes on the 12V rail and eliminates audible load jumps.

---

## ⚙️ Virtual RPM Calculation & Tachometer

Not all installed fans include a physical tachometer output (e.g., the 3-PIN ebm-papst 4412 F/2 GLL).

* **Virtual Calculation**: For 3-PIN fans without tachometer output, VentoSync calculates the estimated RPM based on the non-linear V-curve (up to 4200 RPM @ 100%).
* **Physical Tachometer**: When modern 4-PIN fans (e.g. AxiRev) are installed, pulse counts on GPIO20 are tracked in real-time for closed-loop monitoring.

---

## 🔄 Plain Text Direction Display

To simplify diagnostics and live monitoring of ESP-NOW multi-device synchronization, VentoSync provides a plain-text status sensor in Home Assistant:

* `sensor.fan_direction` / `sensor.luefter_richtung`:
  * 🟢 **"Supply Air (In)"**
  * 🔵 **"Exhaust Air (Out)"**
  * ⚫ **"Standstill"**

---

## 🌴 Vacation Mode

An automated, energy-saving mode designed for extended absences.

### How It Works
1. **State Preservation**: When enabled, VentoSync saves the current operating mode and fan level of all room devices.
2. **Preset Activation**: All synchronized devices switch to the configured vacation preset (Default: *Boost Ventilation at level 1*).
3. **Restoration**: Disabling vacation mode automatically restores the previous operating states across all units.

### Home Assistant Entities
Configurable under the device *Configuration* section:

| Entity | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `select.urlaubsmodus_betriebsmodus` | Select | `Stoßlüftung` | Target mode during vacation |
| `number.urlaubsmodus_intensitat` | Number | `1` | Fan intensity level (1–10) during vacation |

> [!TIP]
> For a complete setup guide using a room-wide Home Assistant Toggle Helper, refer to the **[Home Assistant Vacation Mode Setup Guide](en_vacation-mode-ha-setup.md)**.

---

## 🔒 Child Protection Mode

Child Protection Mode locks the physical buttons on the device panel to prevent accidental or unauthorized changes.

### Controls & Operation

* **Via Home Assistant**:
  * Entity: `switch.kindersicherung` (in device *Configuration*).
  * Control via Home Assistant remains **completely unblocked**.

* **On the Physical Device**:
  * **Toggle (Lock/Unlock)**: Press and hold **Mode** and **Level** buttons simultaneously for **5 seconds**.
  * **Confirmation**: All 9 LEDs flash **2 times** to confirm state change.

* **Feedback on Blocked Press**:
  * If a button is pressed while locked, the input is ignored and all LEDs flash **3 times** as a visual alert.

### Reliability & Persistence
* Lock state is persisted in non-volatile storage (NVS via `restore_value: true`) and persists across power cycles or reboots.
* A 500ms combo cooldown prevents unintentional button presses immediately following unlocking.
