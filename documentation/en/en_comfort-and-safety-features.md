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

In heat recovery the fan changes direction after a fixed time **per direction** that depends on the level (70 s at level 1 → 50 s at level 10; e.g. level 2 = 68 s, level 6 = 59 s). When the level changes, this duration changes too.

To prevent abrupt resets or a premature change of direction, VentoSync keeps the **relative position within the current direction** and scales it to the new duration:

$$\text{New Remaining Time} = \text{New Duration per Direction} \times \left(1 - \frac{\text{Elapsed Time}}{\text{Old Duration per Direction}}\right)$$

* **Benefit**: The fan seamlessly continues its active heat absorption/release phase without disrupting the thermal regeneration balance. The Master keeps the phases of all devices of the room aligned.

---

## 🌊 Soft Start (Slew-Rate Limiter)

To protect the motor electronics and minimize acoustic disruption, all speed transitions are regulated via a software slew-rate limiter.

* **Ramp Speed**: **10 % of full speed per second** (≈ 2.8 % PWM per second) — from level 1 to level 10 in about 9 s.
* **Smooth Reversals**: During directional changes (Heat Recovery) and at the start / end of each Boost Ventilation burst, the fan follows a smooth 5-second deceleration and acceleration curve.
* **Benefit**: Prevents voltage dips and current spikes on the 12V rail and eliminates audible load jumps.

---

## ⚙️ Virtual RPM Calculation & Tachometer

Not all installed fans include a physical tachometer output (e.g., the 3-PIN ebm-papst 4412 F/2 GLL).

* **Virtual Calculation**: For fans without tachometer output, VentoSync estimates the RPM as *speed fraction × 4200 RPM* (the speed fraction follows the non-linear level curve: level 1 = 10 %, level 6 = 50 %, level 10 = 100 %), including the 5 s ramps.
* **Physical Tachometer**: When a 4-PIN fan with tacho output (e.g. AxiRev) is installed, its pulses on GPIO20 (hardware pulse counter) are used instead. The value is for display and diagnostics only — the fan speed is not closed-loop controlled.
* **Entity**: `sensor.lufter_drehzahl` ("Lüfter Drehzahl"), negative while extracting.

---

## 🔄 Plain Text Direction Display

To simplify diagnostics and live monitoring of ESP-NOW multi-device synchronization, VentoSync provides a plain-text status sensor in Home Assistant:

* `text_sensor.lufter_richtung` ("Lüfter Richtung"):
  * 🟢 **"Zuluft (Rein)"** — supply air (in)
  * 🔵 **"Abluft (Raus)"** — exhaust air (out)
  * ⚫ **"Stillstand"** — standstill

---

## 🌴 Vacation Mode

An automated, energy-saving mode designed for extended absences, switched by a Home Assistant toggle helper (default `input_boolean.ventosync_vacation_mode`, substitution `vacation_sensor_id`).

### How It Works
1. **Activation (room-wide)**: The **Master** (device ID 1) of each room saves its current operating mode and fan level and switches the room to the configured vacation preset (default: *Boost Ventilation at level 1*). The other devices follow the Master over ESP-NOW — they do not apply the preset themselves, so no device can accidentally save the vacation state as its "previous" state.
2. **Restoration**: Disabling vacation mode makes the Master restore the saved mode and level for the whole room.
3. **Without a reachable Master** a device applies and restores the vacation mode on its own. The vacation state is stored persistently (`vacation_state`), so a reboot or a repeated trigger during the vacation never overwrites the saved state.

> [!NOTE]
> Devices that were already in vacation mode when updating to 0.10.27 do not know about the active vacation yet: toggle the helper off and on once, or restore the mode manually after the vacation.

### Home Assistant Entities
Configurable under the device *Configuration* section (set them on the Master — it applies the preset for the room):

| Entity | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `select.urlaubsmodus_betriebsmodus` | Select | `Stoßlüftung` | Target mode during vacation |
| `number.urlaubsmodus_intensitat` | Number | `1` | Fan intensity level (1–10) during vacation |

> [!TIP]
> For a complete setup guide using a room-wide Home Assistant Toggle Helper, refer to the **[Home Assistant Vacation Mode Setup Guide](en_vacation-mode-ha-setup.md)**.

---

## 🔒 Child Protection Mode

Child Protection Mode locks the physical buttons on the device panel to prevent accidental or unauthorized changes. It applies per device.

### Controls & Operation

* **Via Home Assistant**:
  * Entity: `switch.kindersicherung` (in device *Configuration*).
  * Control via Home Assistant and the web dashboard remains **completely unblocked**.

* **On the Physical Device**:
  * **Toggle (Lock/Unlock)**: Press and hold the **Mode** button for about **5 seconds** (confirmed after 4.5 s of continuous hold).
  * **Confirmation**: The 8 panel LEDs (power, both mode LEDs, 5 level LEDs) flash **2 times** to confirm the state change.

* **Feedback on Blocked Press**:
  * If a button is pressed while locked, the input is ignored and the LEDs flash **3 times** as a visual alert.

### Reliability & Persistence
* Lock state is persisted in non-volatile storage (`child_lock_active`, `restore_value: true`) and persists across power cycles, reboots and OTA updates (the HA switch uses `restore_mode: DISABLED`, so it never overwrites the stored state at boot — fixed in 0.10.27).
* A 500 ms combo cooldown suppresses a stale button event right after toggling.
