# 🧠 Ventilation Logic Component

This component implements the core algorithmic processing for determining ventilation intensity based on environmental sensors.

## 📄 File Overview

| File | Description |
| :--- | :--- |
| **`ventilation_logic.h` / `.cpp`** | **Automation Engine**: Contains the logic for processing CO2 and Humidity sensor readings through setpoint-based algorithms (PIDs). It translates environmental "demand" into a 0-100% intensity value. |
| **`__init__.py`** | **ESPHome Integration**: Registers the logic component within ESPHome, allowing users to define setpoints and thresholds in the YAML configuration. |

## ⚙️ Key Mechanisms

- **Demand Calculation**: Uses a modular approach to calculate demand from multiple sources (e.g., CO2 ppm and Humidity %).
- **Smoothing**: Ensures that calculated intensity values do not jitter or cause rapid fan oscillations.
- **Setpoints**: Manages the target values (e.g., 800ppm CO2) provided by the Home Assistant UI or YAML defaults.
