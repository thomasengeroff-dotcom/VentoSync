# 🖥️ On-Device Control Panel Operation (VentoMaxx V-WRG-1)

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_control-panel-operation.md)


To ensure an optimal user experience, VentoSync retains and enhances the original control panel of the VentoMaxx V-WRG-1 ventilation units (14-Pin FFC interface controlled via MCP23017 and PCA9685).

<p align="center">
  <img src="../../images/Ventomax%20V-WRG-1/PXL_20260128_232625674.jpg" alt="VentoSync Control Panel in Housing" width="500" />
</p>

---

## 🔘 Button Assignment & Functions

The control panel features 3 tactile buttons:

| Button | Function | Operation / Action |
| :--- | :--- | :--- |
| **Power (I/O)** | System On / Off / Sleep | • **Short press**: Toggles ventilation ON / OFF (OFF stops fan at 50% PWM; remains online in Sensor Monitoring mode; ON restores last active mode and wakes from Light Sleep).<br>• **Long press (> 5s)**: Enters **Light Sleep Mode** (turns off fan, LEDs, and disables Wi-Fi radio to conserve power).<br>• **Very long press (> 10s)**: Reboots the ESP32 microcontroller. |
| **Mode (M)** | Operating Mode | • **Short press**: Cycles through programs: **Auto → Heat Recovery → Ventilation → Boost Ventilation → Off → Auto...** |
| **Level (+)** | Fan Intensity | • **Short press**: Cycles through 10 speed levels.<br>• **Hold**: Automatically cycles smoothly up and down through all levels (1 level/sec) until released. |

---

## 🔆 Status LEDs & Visual Feedback

The panel features 9 green LEDs providing real-time system state feedback:

| LED | Quantity | Position | Behavior |
| :--- | :---: | :--- | :--- |
| **Power** | 🟢 1x | Panel Top | Illuminates bright during active operation. Dims to 20% brightness after the 30s inactivity timeout. |
| **Master** | 🟢 1x | Center | Lights up solid (dimmed) on the Master Device (Device ID = 1). Signals diagnostic error states via pulse blink patterns (see below). |
| **Mode L** (`LED_WRG`) | 🟢 1x | Left | **Pulses slowly** in Smart Automatic mode. Stays permanently ON for Heat Recovery and Ventilation. |
| **Mode R** (`LED_VEN`) | 🟢 1x | Right | Permanently ON for Boost Ventilation and Ventilation. |
| **Intensity (1–5)** | 🟢 5x | Bar | Displays current fan speed level 1–10 using 50% / 100% LED brightness levels. |

---

### 📊 Mode LED Matrix

| Mode | `LED_WRG` (left) | `LED_VEN` (right) |
| :--- | :---: | :---: |
| **Smart Automatic (Default)** | 🟢 (pulses slowly) | ⚫ |
| **Heat Recovery (Eco)** | 🟢 (solid) | ⚫ |
| **Boost Ventilation** | ⚫ | 🟢 (solid) |
| **Ventilation (Summer Cross-Vent)** | 🟢 (solid) | 🟢 (solid) |
| **Off / Standby** | ⚫ | ⚫ |

---

### 📶 Intensity LED Fill Bar (10 Levels via 5 LEDs)

Each LED represents 2 speed levels (50% brightness for the odd level, 100% brightness for the even level), providing a clean volume-bar style visual:

* **Level 1**:  ◖ ◯ ◯ ◯ ◯  *(LED 1 @ 50%)*
* **Level 2**:  ⬤ ◯ ◯ ◯ ◯  *(LED 1 @ 100%)*
* **Level 3**:  ⬤ ◖ ◯ ◯ ◯  *(LED 2 starts @ 50%)*
* **Level 4**:  ⬤ ⬤ ◯ ◯ ◯  *(LED 2 @ 100%)*
* **Level 5**:  ⬤ ⬤ ◖ ◯ ◯  *(LED 3 starts @ 50%)*
* **Level 6**:  ⬤ ⬤ ⬤ ◯ ◯  *(LED 3 @ 100%)*
* **Level 7**:  ⬤ ⬤ ⬤ ◖ ◯  *(LED 4 starts @ 50%)*
* **Level 8**:  ⬤ ⬤ ⬤ ⬤ ◯  *(LED 4 @ 100%)*
* **Level 9**:  ⬤ ⬤ ⬤ ⬤ ◖  *(LED 5 starts @ 50%)*
* **Level 10**: ⬤ ⬤ ⬤ ⬤ ⬤  *(LED 5 @ 100%)*

---

## 🚨 Diagnostic Blink Codes (Master LED)

When a malfunction or special operational state occurs, the center **Master LED** pulses a specific blink code (repeating with a pause):

| Pattern | Meaning / Error | Description & System Action |
| :--- | :--- | :--- |
| **2x Blinks** | Peer Sync Lost | Synchronization lost with peer devices in the room group (> 3 minutes without ESP-NOW heartbeat). *(Active when peer monitoring is enabled.)* |
| **3x Blinks** | Wi-Fi Disconnected | Connection to the local Wi-Fi router lost (> 30 seconds continuous disconnect to suppress roaming drops). |
| **4x Blinks** | Overheat Warning | Housing internal temperature is critical (50–60°C). Check air paths; automatic thermal shutdown engages at > 60°C. |
| **Slow Pulse** *(1s ON, 2s OFF)* | Window Guard Active | Room windows are open. Fans are paused; pulse automatically turns off after 35 seconds to prevent night light pollution. |

---

## ✨ Group Synchronization & Auto-Dimming

* **Real-time Peer Wake-Up**: When a user changes the mode or speed level on any unit in the room, all partner units in the group wake up their displays immediately and show the updated status for 30 seconds.
* **30s Auto-Dimming**: To prevent light pollution in living spaces and bedrooms, all status LEDs (Mode, Intensity, Master) smoothly fade out 30 seconds after the last interaction (`ui_timeout_script` in `packages/ui/ui_controls.yaml`). The **Power LED** stays dimmed at 20% to indicate operational readiness.
* **Instant Reactivation**: Pressing any button immediately reactivates all status LEDs.
* **Persistent Diagnostic Feedback**: The **Master LED continues to signal error states and diagnostic blink codes** (e.g. Wi-Fi loss, peer sync lost, overheat warning), even while other LEDs are in the dimmed/timeout state.
