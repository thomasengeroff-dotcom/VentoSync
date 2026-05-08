# 📦 VentoSync Packages

This directory contains the modularized ESPHome configuration files (packages). By splitting the configuration into smaller, functional blocks, maintainability and clarity are significantly improved.

## 📂 File Overview

### 🏗️ Base (`base/`)
| File | Description |
| :--- | :--- |
| **`esp32c6_common.yaml`** | Shared base configuration for the ESP32-C6 platform, including common substitutions, versioning, and basic components. |
| **`device_config.yaml`** | Dynamic device-specific configuration (IDs, Room IDs, Phases). This is the target for per-device customizations. |
| **`wifi_ota.yaml`** | WiFi and OTA configuration without secrets, optimized for CI/GitHub Actions builds. |

### 🧠 Actuators & Logic (`actuators/`)
| File | Description |
| :--- | :--- |
| **`logic_automation.yaml`** | The "brain" of the project. Contains cyclic scripts, interval-based updates, fan mode handling, and high-level automation. |
| **`logic_pid.yaml`** | Configuration for internal PID climate controllers (CO2 & Humidity). |
| **`logic_safety.yaml`** | Safety-critical automation and emergency shutdown logic (e.g. thermal overload). |
| **`logic_maintenance.yaml`** | Filter maintenance timers and reset logic. |

### 🔌 Hardware I/O (`io/`)
| File | Description |
| :--- | :--- |
| **`hardware_io.yaml`** | Physical hardware interfaces: I2C buses, port expanders (MCP23017, PCA9685), and basic Pinout setup. |
| **`hardware_fan.yaml`** | Central fan configuration including PWM parameters, RPM calculation fallbacks, and the `fan` entity itself. |
| **`logic_buttons.yaml`** | Physical button input debouncing, click handlers, and long-press overrides (e.g. for Child Protection). |

### 🌐 Communication (`communication/`)
| File | Description |
| :--- | :--- |
| **`esp_now.yaml`** | ESP-NOW communication protocol setup and peer discovery for the device cluster. |

### 🔗 Integration (`integration/`)
| File | Description |
| :--- | :--- |
| **`homeassistant.yaml`** | External data points imported from Home Assistant (e.g. outdoor climate, vacation mode, window sensors). |

### 🎛️ User Interface (`ui/`)
| File | Description |
| :--- | :--- |
| **`ui_controls.yaml`** | Defines all entities exposed to Home Assistant for manual control (Sliders, Dropdowns, and UI-specific numbers). |
| **`ui_lights.yaml`** | LED indicator configurations, strobe patterns, and animations. |
| **`ui_diagnostics.yaml`** | Network and system diagnostic UI entities (e.g. ESP-NOW peers display). |

### 🌡️ Sensors (`sensors/`)
| File | Description |
| :--- | :--- |
| **`sensors_climate.yaml`** | Higher-level climate logic, including heat recovery efficiency calculations and consolidated statistics. |
| **`sensor_SCD41.yaml`** | Integration for the high-precision Sensirion SCD41 CO2 sensor (Photoacoustic). |
| **`sensor_BMP390.yaml`** | Integration for the Bosch BMP390 pressure sensor, including pressure trend analysis. |
| **`sensor_BME680.yaml`** | Integration for the Bosch BME680 gas sensor (used for IAQ / fallback air quality). |
| **`sensor_LD2450.yaml`** | Integration for the HLK-LD2450 mmWave Radar sensor (Presence Detection). |
| **`sensor_NTC.yaml`** | Configuration for the analog NTC probes used to measure supply/exhaust air temperature. |
| **`mock_*.yaml`** | Mock sensor definitions (SCD41, BME680, Radar) to allow firmware compilation even when specific hardware is omitted. |

The main configuration file `ventosync.yaml` (in the root directory) uses the `packages:` instruction to merge these modules and the core logic defined in `packages/base/ventosync_base.yaml`.

This modular approach is necessary because **ESPHome does not support deactivating hardware components via software at runtime.** In ESPHome, every hardware component (sensors, displays, etc.) declared in the YAML is compiled into the firmware and initialized on boot. If a component is defined but the physical hardware is missing, the firmware would continuously log errors or potentially crash during initialization. By using packages, you ensure that only the drivers for the physically present hardware are compiled into the firmware.

## 🛠️ Modifying Packages

- **Naming Convention**: Use `id: ...` consistently across packages to ensure logical links (e.g., `pid_co2` in `logic_automation.yaml` refers to sensors defined in `sensor_SCD41.yaml`).
- **Dependencies**: Note that some packages depend on others (e.g., `logic_automation.yaml` depends on the presence of specific sensor IDs).
- **Versioning**: Global versions and common platform settings should be modified in `esp32c6_common.yaml`.