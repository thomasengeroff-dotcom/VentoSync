# 🗺️ Roadmap & Future Enhancements

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_roadmap-and-future-enhancements.md)


This document outlines planned features, architectural concepts, and upcoming hardware/software enhancements for **VentoSync**.

---

## 📑 Planned Enhancements Overview

- [🎛️ Intuitive Group Control & Visual Single-Tile Management](#️-intuitive-group-control--visual-single-tile-management)
- [🌙 Intelligent Night & Silent Sleep Mode](#-intelligent-night--silent-sleep-mode)
- [🏠 Away-From-Home Mode & Short-Term Absence Logic](#-away-from-home-mode--short-term-absence-logic)
- [❄️ Frost Protection Automation](#️-frost-protection-automation)
- [📅 Self-Sufficient On-Device Weekly Schedules](#-self-sufficient-on-device-weekly-schedules)
- [🔔 Advanced Alarm & Filter Notification System](#-advanced-alarm--filter-notification-system)
- [🔄 Closed-Loop Speed & RPM Monitoring](#-closed-loop-speed--rpm-monitoring)
- [🌬️ Aerodynamic Reverse Compensation](#️-aerodynamic-reverse-compensation)
- [🧠 AI-Powered Predictive Ventilation Control](#-ai-powered-predictive-ventilation-control)
- [🔌 Hardware Expansion & Industrial Smart Home Gateways](#-hardware-expansion--industrial-smart-home-gateways)
- [❄️🔥 Smart Climate Control — HVAC Coordination](#️-smart-climate-control--hvac-coordination)

---

## 🎛️ Intuitive Group Control & Visual Single-Tile Management

* **Group-Controller Concept**: Using ESP-NOW, multiple paired ventilation units in the same room can be combined into a single unified entity in Home Assistant (e.g., using Mushroom Cards or custom Lovelace cards).
* **Benefits**: Massive reduction in Wi-Fi traffic, simplified UI for family members (high WAF - wife acceptance factor), and synchronized state feedback.
* *Detailed YAML examples and concepts can be explored in the [ha_integration_example/](../../ha_integration_example/) directory.*

---

## 🌙 Intelligent Night & Silent Sleep Mode

* **Ambient Light Sensor Integration**: Automatic activation of a "Whisper-Quiet" low-RPM profile when room darkness is detected via an external hardware twilight sensor (LDR or BH1750 I2C sensor).
* **Silent Sleep Mode via mmWave**: Utilizing mmWave radar micro-movement tracking (breathing detection) to switch to the lowest speed level and extend direction-reversal intervals, virtually eliminating mechanical commutation and airflow sounds during sleep.
* **CO2 Safeguard**: Ensures air quality stays within healthy boundaries even in ultra-quiet night mode.

---

## 🏠 Away-From-Home Mode & Short-Term Absence Logic

* **Home Assistant Geofencing / Alarm Integration**: When Home Assistant signals that all residents have left (via GPS geofencing or alarm arming), all units switch to a minimal hygienic base-ventilation level to conserve maximum energy.
* **Radar-Based Short-Term Absence Reduction**: Automatic down-throttling when a room is temporarily unoccupied, ramping back up instantly upon entry.

---

## ❄️ Frost Protection Automation

* **Thermal Core Regeneration**: At sub-zero outdoor temperatures, moisture inside the ceramic regenerator can freeze. By monitoring the outdoor NTC sensor, VentoSync will automatically lengthen exhaust cycles or temporarily deactivate intake flow to thaw the heat exchanger safely.

---

## 📅 Self-Sufficient On-Device Weekly Schedules

* **Autonomous ESP32 Time Engine**: Native schedule execution running directly on the ESP32 (via SNTP time sync) to maintain time-based profiles even if the central smart home server or Wi-Fi network is offline.
* **Collision Protection**: Smart priority logic to ensure native on-device schedules harmonize with Home Assistant automations.

---

## 🔔 Advanced Alarm & Filter Notification System

* **Multi-Channel Alerts**: Visual blink codes on the Master LED combined with Home Assistant push notifications for events such as critical indoor humidity (>75% rH), frost alerts, sensor failures, or filter cleaning maintenance cycles (based on fan runtime hours).

---

## 🔄 Closed-Loop Speed & RPM Monitoring

* **Tachometer Pulse Feedback**: Continuous real-time measurement of actual motor RPM via GPIO20 pulse counter on 4-PIN PWM fans (e.g. AxiRev).
* **Constant Volume Flow**: Automatic PWM trim compensation when filters become dirty or backpressure increases due to strong outdoor wind gusts.

---

## 🌬️ Aerodynamic Reverse Compensation

* **Asymmetric Fan Efficiency**: Standard axial fans (like the ebm-papst 4412 F/2 GLL) are aerodynamically optimized for a single direction of airflow. When reversing the rotation for heat recovery exhaust, volumetric efficiency drops significantly (typically 30-50% loss) due to blade profile camber and strut blockages.
* **Software-Based Flow Balancing**: Introduction of a configurable `REVERSE_COMPENSATION_FACTOR` in the PWM calculation to automatically run the fan at a proportionally higher speed during the reverse cycle. This guarantees balanced airflow ($V_{in} \approx V_{out}$), which is critical for maximizing the thermal efficiency of the ceramic regenerator core and preventing room over/underpressure.

---

## 🧠 AI-Powered Predictive Ventilation Control

* **Proactive Adaptation**: Machine learning models and weather forecast integrations to pre-ventilate spaces before high humidity or heatwaves set in.
* **Occupancy Estimation & Person Counting**: Estimating the number of occupants via mmWave multi-target tracking to dynamically calculate required CFM / m³ airflow rates.
* *For technical architecture, see [📄 AI-Powered Ventilation Control](en_ai-powered-ventilation-control.md).*

---

## 🔌 Hardware Expansion & Industrial Smart Home Gateways

* **Mixed-Air IAQ Logic (CO2 + VOC)**: Combining Bosch BME680 VOC/gas measurements with Sensirion SCD4x CO2 data for an all-encompassing Indoor Air Quality control loop.
* **Building Automation Gateways (Modbus / KNX)**: Standardized interfaces to bridge VentoSync units into professional commercial BMS (Building Management Systems) for HVAC and heating coordination.

---

## ❄️🔥 Smart Climate Control — HVAC Coordination

> ✅ **Implemented in version 0.10.13.** Kept here for reference; the concept below was reviewed and refined during implementation.

* **Intelligent AC Coordination**: When a room air conditioner is active, VentoSync automatically throttles ventilation to the minimum level required for healthy indoor air quality (CO2-based), preventing energy waste from importing hot outdoor air. Acts as a modifier of `Smart-Automatik` only; manual modes are untouched.
* **Dedicated Enable/Disable Switch**: The `smart_climate_control` switch ("Klima-Koordination") allows per-device activation of the HVAC coordination feature.
* **CO2-Only Air Quality Guard**: Switches from dual-PID (CO2 + Humidity) to a relaxed CO2-only control loop (target: 1200 ppm / DIN EN 13779 IDA 3) with a hard fan level cap (default: Level 3) and enforced heat recovery (no summer bypass).
* **Health Guards**: A CO2 emergency override (default 1500 ppm, hysteresis back to 1200 ppm) and a mold guard (≥ 70 % rH while outdoor air is drier) lift the restrictions automatically. Without a CO2 reading the feature never throttles.
* **Robust AC Detection**: 120 s release debounce and a fail-safe "AC inactive" fallback when Home Assistant is offline; the HA mapping uses the selected `hvac_mode`, not the cycling compressor `hvac_action`.
* *For the complete technical specification, state machine, and configuration entities, see [📄 Smart Climate Control — HVAC Coordination](en_smart-climate-control.md).*
