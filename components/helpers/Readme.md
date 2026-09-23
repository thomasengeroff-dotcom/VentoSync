# 🛠️ VentoSync C++ Helper Library

This folder contains the modular C++ helper headers for the VentoSync firmware. These headers implement complex calculation, hardware abstraction, and state-machine logic that would be inefficient or impractical within plain ESPHome YAML lambdas.

## 📖 Component Overview

| Header File | Responsibility / Description |
| :--- | :--- |
| **`globals.h`** | **Central Registry**: `extern` declarations and shared pointers for all ESPHome sensors, numbers, switches, and selects. |
| **`auto_mode.h`** | **Smart Auto Engine**: Dual-PID control logic (CO2 & Enthalpy/Humidity), hysteresis arbitration, and passive summer cooling bypass. |
| **`automation_helpers.h`** | **Motor & Fan Actuation**: Fan PWM scaling, bidirectional V-curve voltage control, soft ramping, and emergency thermal cutoffs. |
| **`bme680_iaq_engine.h`** | **BME680 IAQ Engine**: Indoor Air Quality (IAQ) indexing, absolute humidity calculation ($g/m^3$), and sensor calibration algorithms. |
| **`climate.h`** | **Thermal & Environmental Processing**: Phase-locked NTC thermal stabilization filter, sensor fallbacks, and human-readable AQI formatting. |
| **`config_helpers.h`** | **Dynamic Device Configuration**: Runtime device provisioning (Device ID, Room, Floor, Phase) with validation and NVS persistence. |
| **`espnow_helpers.h`** | **ESP-NOW Dispatcher**: Unified incoming packet validation, source tagging, and dispatch routing. |
| **`ha_fan_helpers.h`** | **Home Assistant Fan Bridge**: Bidirectional state translation and loopback-suppression between ESPHome and the HA Fan entity. |
| **`health_helpers.h`** | **System Watchdog & Health**: Controller loop liveness monitoring, task freeze detection, and stack/heap diagnostics. |
| **`hrv_efficiency.h`** | **HRV Efficiency Engine**: Real-time sensible and latent heat recovery efficiency calculation per DIN EN 13141-8. |
| **`led_feedback.h`** | **LED & Panel UI**: Controls the original VentoMaxx panel (PCA9685/MCP23017) with 10-level bar visualization, dimming, and Master LED diagnostic codes. |
| **`network_sync.h`** | **Wireless Mesh (ESP-NOW v10)**: Low-latency peer discovery, LRU peer caching, room-level push-pull synchronization, and packet validation. |
| **`system_boot_helpers.h`** | **Hardware Boot Sequence**: Low-level GPIO configuration, RF-switch antenna path activation, and early peripheral setup. |
| **`system_lifecycle.h`** | **Lifecycle & Maintenance**: Multi-stage boot orchestration, persistent filter operating hours tracking, and graceful reboot hooks. |
| **`user_input.h`** | **Button & Interaction Logic**: Debouncing, physical button handling (Power/Mode/Level), timed boost countdowns, and child-lock protection. |
| **`vacation_helpers.h`** | **Vacation Mode Engine**: Low-intensity interval ventilation management to protect indoor climate during absence. |

## 🏗️ Architecture Design

- **Header-Only Approach**: Most logic is implemented as `inline` functions within headers. This simplifies the ESPHome build process as no separate `.cpp` compilation units need to be managed by the user.
- **Extern Linkage**: The project uses a centralized `globals.h` to bridge the gap between ESPHome's generated code and these custom helpers.
- **Doxygen Documentation**: All functions and classes are documented using Doxygen standards. To understand the "Why" behind specific implementations (like 50% PWM being "Off"), refer to the comments within the code.

## ⚠️ Maintenance Note

When adding new sensors or UI elements to the YAML configuration, ensure they are also declared in `globals.h` and initialized in the appropriate priority boot hook in `system_lifecycle.h` if they are required by the C++ logic.
