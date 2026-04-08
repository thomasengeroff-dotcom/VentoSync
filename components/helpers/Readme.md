# 🛠️ VentoSync C++ Helper Library

This folder contains the core C++ implementation for the VentoSync project. These headers provide the complex logic that would be too cumbersome or inefficient to implement directly within ESPHome YAML lambdas.

## 📖 Component Overview

| File | Description |
| :--- | :--- |
| **`globals.h`** | **Central Registry**: Provides `extern` declarations for all ESPHome components (sensors, selects, numbers). This allows all other helper files to access the current system state without naming conflicts. |
| **`network_sync.h`** | **Communication Backbone**: Implements the ESP-NOW protocol (v4). Handles dynamic peer discovery, room-wide state synchronization (mode/intensity mirroring), and packet validation. |
| **`fan_control.h`** | **Motor Management**: Implements the "V-Curve" control logic for reversible 12V fans. Includes slew-rate limiting (soft-start/brake) and phase position tracking. |
| **`auto_mode.h`** | **Intelligence Layer**: The logic behind "Standard Automatic". Coordinates between CO2 PID demand and Humidity PID demand to determine the optimal fan speed. |
| **`climate.h`** | **Environmental Processing**: Implements thermal stabilization filters for NTC sensors. Pauses measurement during fan direction changes to ensure accurate temperature data. |
| **`led_feedback.h`** | **Visual Interface**: Orchestrates the 9-LED original VentoMaxx panel. Manages dimming, wake-up effects, and diagnostic blink codes for the Master LED. |
| **`user_input.h`** | **Interaction Logic**: Handlers for physical button presses (Power/Mode/Level) and updates from Home Assistant UI elements. Implements debouncing and multi-click logic. |
| **`system_lifecycle.h`** | **System Hooks**: Manages the boot sequence (`run_system_boot_initialization`), watchdog restart detection, and component initialization priority. |

## 🏗️ Architecture Design

- **Header-Only Approach**: Most logic is implemented as `inline` functions within headers. This simplifies the ESPHome build process as no separate `.cpp` compilation units need to be managed by the user.
- **Extern Linkage**: The project uses a centralized `globals.h` to bridge the gap between ESPHome's generated code and these custom helpers.
- **Doxygen Documentation**: All functions and classes are documented using Doxygen standards. To understand the "Why" behind specific implementations (like 50% PWM being "Off"), refer to the comments within the code.

## ⚠️ Maintenance Note

When adding new sensors or UI elements to the YAML configuration, ensure they are also declared in `globals.h` and initialized in the appropriately priority boot hook in `system_lifecycle.h` if they are required by the C++ logic.