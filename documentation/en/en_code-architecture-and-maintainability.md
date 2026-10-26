# 🏗️ Code Architecture, Modularity & Maintainability

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_code-architecture-and-maintainability.md)


This document provides a comprehensive technical overview of VentoSync's multi-tier software architecture, YAML package modularization, native C++ helper component design, performance optimizations, and system initialization lifecycle.

---

## 🏛️ Architecture Overview

The firmware follows a **multi-stage modular architectural approach**, maximizing maintainability and extensibility:

#### **1. YAML Modularization (Packages)**

The formerly enormous main file was drastically slimmed down to simplify readability and maintenance. The project intensively uses the ESPHome `packages:` function to outsource self-contained logic building blocks into separate YAML files. Since version 0.8.171, the `packages/` directory is strictly hierarchically structured:

- **`base/`**: Contains the fundamental ESP32-C6 device configuration, Wi-Fi, and OTA update logic.
- **`communication/`**: Configures the high-performance ESP-NOW broadcast and unicast protocol pipelines.
- **`globals/`**: Encapsulates runtime global variables (automation, network sync, UI, fan states) into clean, separated packages.
- **`io/`**: Encapsulates the physical hardware. Includes I2C buses, port expanders, basic pinouts, and central fan configuration.
- **`sensors/`**: Contains the entire measurement periphery (SCD43, SGP41 + SHT45, Radar, NTCs, BMP390).
  - 🧩 **Sensor Mocks**: If a sensor is missing (e.g., SCD43), mocks (`mock_scd41.yaml`) automatically step in. These prevent compile errors, suppress log spamming, and seamlessly hide non-existent sensors from Home Assistant using `internal: true`.
- **`actuators/`**: The "brain" of the system. This is where high-performance automations, PID climate controllers, vacation logic, and safety-critical thermal shutdown (`logic_safety.yaml`) reside.
- **`integration/`**: Isolates all external Home Assistant data points (`homeassistant.yaml`) to keep the system capable of running autonomously.
- **`ui/`**: Contains the on-device control panel feedback, diagnostic entities, and local web UI controls.

The main files (`ventosync.yaml` etc.) now merely act as slim "wrappers" that import `packages/base/ventosync_base.yaml` and load specific sensors or mocks depending on the variant.

#### **2. `automation_helpers.h` - Central Helper Library**

All complex lambda functions were banished from the YAML code and outsourced into reusable native C++ helper functions:

**Advantages:**

- ✅ **Better Readability**: YAML remains clear, logic is documented in C++
- ✅ **Reusability**: Functions can be used in several places
- ✅ **Type Safety**: Compiler checks at compile time instead of runtime errors
- ✅ **IDE Support**: Syntax highlighting, auto-completion, and refactoring tools
- ✅ **Easier Maintenance**: Changes are made in one place instead of in several YAML lambdas

**Included Functions:**

- `handle_espnow_receive()` - ESP-NOW packet processing and state synchronization
- `handle_button_*_click()` - Button event handlers (Power, Mode, Level)
- `set_*_handler()` - UI element callbacks (Timer, Cycle Duration, Fan Intensity)
- `update_leds_logic()` - LED status update based on system state
- `cycle_operating_mode()` - Operating mode change logic
- `calculate_heat_recovery_efficiency()` - Heat recovery calculation

**Example:**

```yaml
# Before: Complex lambda directly in YAML
binary_sensor:
  - platform: gpio
    on_press:
      - lambda: |-
          id(current_mode_index) = (id(current_mode_index) + 1) % 5;
          cycle_operating_mode(id(current_mode_index));
          id(update_leds).execute();

# After: Clean call of the helper function
binary_sensor:
  - platform: gpio
    on_press:
      - lambda: handle_button_mode_click();

```

#### **3. 🚀 Performance & Technical Excellence**

To ensure 24/7 reliability and premium performance on the ESP32-C6, the firmware implements several high-end C++ and architectural optimizations:

- **C++ Pro Performance & Thread Safety**:
  - ✅ **Thread Safety**: Manual LwIP semaphores replaced by C++ Standard Library `<mutex>` and `std::lock_guard` for 100% exception-safe HTTP event queuing.
  - ✅ **Memory Management**: Use of Move Semantics (`std::move`) and strict const-correctness to minimize RAM fragmentation and CPU overhead.
  - ✅ **DRY Architecture**: Dedicated, anonymous lambda helper functions for Web-JSON building to eliminate redundant logic.
  - ✅ **Footprint Reduction**: Optimized RAM usage by removing outdated Web-UI cache concepts.

- **🛡️ System Stability & Reliability**:
  - ✅ **NaN-Safe PID Control**: Hardened demand calculation against invalid sensor data. The system holds the last valid state if sensors fail, preventing erratic fan toggling.
  - ✅ **Unified Control Authority**: Centralized intensity calculation (`evaluate_auto_mode`) to eliminate race conditions between independent update intervals.
  - ✅ **Smart Group Sync**: Automatic propagation of modes and configurations across peer devices via ESP-NOW with built-in loop prevention.
  - ✅ **Config Safety**: Added validation for min/max fan levels (swap-guard) to prevent inverted scaling on UI misconfiguration.
  - ✅ **NVS Wear Protection**: Write access to the internal flash memory is minimized by buffering non-critical data like filter operating hours and writing them only once every 8 hours (3x per day) to maximize memory longevity.
  - ✅ **LED Self-Test**: During the boot sequence, the system performs a 3-second hardware check by forcing all LEDs to 100% brightness, ensuring visual feedback reliability before restoring user settings.
  - ✅ **Combined NTC Filtering**: Replaced fragmented YAML lambdas with a unified, high-performance C++ filter (`filter_ntc_combined`). This merges Phase-Lock, Thermal Wait, and Seasonal Selection into a single coherent pipeline, ensuring 100% data integrity for thermal measurements.
  - ✅ **Robust Failsafe**: Implemented customizable plausibility ranges and an extended 120s timeout to prevent "unavailable" states in Home Assistant during long ventilation phases.

#### **4. 🏁 System Boot Flow**

The following diagram visualizes the robust initialization sequence required for stable networking and sensor discovery:

```text
Boot (t=0)
  │
  ├─ on_boot (priority -10)
  │   ├─ delay 2s
  │   ├─ sync_config_to_controller()
  │   ├─ cycle_operating_mode()
  │   ├─ load_peers_from_runtime_cache()  ← Load NVS
  │   ├─ delay 1s
  │   ├─ send_discovery_broadcast()       ← Search for Peers
  │   ├─ delay 3s
  │   └─ request_peer_status()            ← State sync
  │
  ├─ interval 60s (repeated)
  │   └─ if peer_cache.empty() → send_discovery_broadcast()
  │
  └─ Normal Operation
```
