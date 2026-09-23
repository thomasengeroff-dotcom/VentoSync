# 📦 VentoSync Packages

This directory contains the modularized ESPHome configuration files (packages). By splitting the configuration into smaller, functional blocks, maintainability, hardware flexibility, and build clarity are significantly improved.

## 📂 File Overview

### 🏗️ Base (`base/`)
| File | Description |
| :--- | :--- |
| **`ventosync_base.yaml`** | Master base package linking core platform settings, web server, and component initialization hooks. |
| **`esp32c6_common.yaml`** | Shared base configuration for the ESP32-C6 platform, common substitutions, versioning, and framework settings. |
| **`device_config.yaml`** | Dynamic device-specific configuration (IDs, Room IDs, Phases) persisted in NVS. |
| **`wifi_ota.yaml`** | WiFi and Safe-Mode OTA configuration without hardcoded secrets (CI/GitHub Actions compatible). |

### 🧠 Actuators & Logic (`actuators/`)
| File | Description |
| :--- | :--- |
| **`logic_automation.yaml`** | The automation brain. Cyclic 10s decision loops, Auto Mode PID evaluation, and summer cooling bypass. |
| **`logic_pid.yaml`** | Configuration for internal PID climate controllers (CO2 & Enthalpy/Humidity). |
| **`logic_safety.yaml`** | Safety-critical automation and emergency shutdown logic (thermal overload, hardware disconnects). |
| **`logic_maintenance.yaml`** | Predictive filter operating hours tracker and maintenance reset triggers. |

### 🌐 Global State & NVS (`globals/`)
| File | Description |
| :--- | :--- |
| **`globals_ventilation.yaml`** | Core ventilation state (fan intensity, active operating mode index, timer duration, Phase A/B). |
| **`globals_automation.yaml`** | Runtime setpoints, PID demand variables, and climate guard thresholds. |
| **`globals_network.yaml`** | ESP-NOW cluster sync state, dynamic peer registration, and heartbeat flags. |
| **`globals_ui.yaml`** | Physical LED brightness configuration, night-mode dimming levels, and UI state tracking. |

### 🔌 Hardware I/O (`io/`)
| File | Description |
| :--- | :--- |
| **`hardware_io.yaml`** | Physical hardware buses and port expanders: I2C buses, MCP23017, PCA9685, and GPIO pin mapping. |
| **`hardware_fan.yaml`** | Central fan actuator configuration, PWM frequency/parameters, and tachometer RPM inputs. |
| **`logic_buttons.yaml`** | Physical button input debouncing, click handlers, and long-press overrides (e.g. Child Protection Mode). |

### 🌐 Communication (`communication/`)
| File | Description |
| :--- | :--- |
| **`esp_now.yaml`** | Low-latency ESP-NOW v10 mesh protocol setup and wireless peer synchronization. |

### 🔗 Integration (`integration/`)
| File | Description |
| :--- | :--- |
| **`homeassistant.yaml`** | External data points imported from Home Assistant (outdoor climate sensors, vacation switch, window contacts) and the API action `set_ac_active` (Smart Climate Control AC state). |
| **`ha_fan_entity.yaml`** | Native Home Assistant `fan` platform integration exposing preset modes, speed percentage, and directional control for `ventosync-card`. |

### 🎛️ User Interface (`ui/`)
| File | Description |
| :--- | :--- |
| **`ui_controls.yaml`** | Entities exposed to Home Assistant for manual control (Sliders, Dropdowns, Setpoints, and Timers). |
| **`ui_lights.yaml`** | LED indicator channels, brightness curves, fill-bar mappings, and diagnostic flash sequences. |
| **`ui_diagnostics.yaml`** | Diagnostics and observability entities (ESP-NOW peer counts, system health, and heap metrics). |

### 🌡️ Sensors (`sensors/`)
| File | Description |
| :--- | :--- |
| **`sensors_climate.yaml`** | Climate calculations, absolute humidity ($g/m^3$), dew point, and fallback temperature selection. |
| **`sensor_hrv_efficiency.yaml`** | Real-time sensible and latent heat recovery efficiency calculations (DIN EN 13141-8). |
| **`sensor_SCD41.yaml`** | High-precision Sensirion SCD41 photoacoustic CO2, temperature, and relative humidity sensor. |
| **`sensor_BMP390.yaml`** | Bosch BMP390 barometric pressure sensor and pressure trend metrics. |
| **`sensor_BME680.yaml`** | Bosch BME680 environmental sensor (VOC, IAQ, temperature, pressure, humidity). |
| **`sensor_LD2450.yaml`** | HLK-LD2450 24 GHz mmWave radar sensor for invisible human presence detection. |
| **`sensor_NTC.yaml`** | Analog NTC temperature thermistors for supply air and room air measurement. |
| **`mock_*.yaml`** | Mock sensor definitions (`mock_scd41`, `mock_bme680`, `mock_radar`) allowing modular firmware builds without physical sensors. |

---

## 🛠️ Modifying Packages

- **Naming Convention**: Use consistent IDs (`id: ...`) across packages to ensure logical cross-linking.
- **Hardware Modularity**: Only include the hardware sensor packages that match the physical board assembly (e.g. `sensor_SCD41.yaml` vs `mock_scd41.yaml`) to prevent compilation overhead and boot errors.
- **Platform Base**: Platform settings and global substitutions are maintained in `esp32c6_common.yaml`.
