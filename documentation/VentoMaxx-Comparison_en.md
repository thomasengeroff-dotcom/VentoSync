# Feature Comparison: VentoSync vs. VentoMaxx Original

This comparison is based on the analysis of the VentoMaxx manual (`MA-BA_Endmontage_V1_V3-WRG_DE_2502.pdf`) and the current functionality of the ESPHome firmware (as of March 2026).

## 🏆 Summary

The **VentoSync** offers **100% of the original functionality** and massively extends it by:

- **Smart Home Integration** (WiFi, Home Assistant, OTA updates)
- **Precision Sensors** (SCD41 CO2, BMP390 air pressure, HLK-LD2450 radar, NTC temperatures)
- **Intelligent Control Algorithms** (PID-controlled CO2/humidity regulation, adaptive cycle times)
- **Predictive Maintenance** (Filter change alarm based on operating hours + calendar time)
- **Wireless Mesh Network** (ESP-NOW for autonomous group operation)

---

## ⚙️ Operating Modes & Logic

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart WRG (This Project) | ESPHome Advantage |
| :--- | :--- | :--- | :--- |
| **Heat Recovery (HRV)** | Static cycle: **70 sec.** (fixed) | **Dynamic cycle**: 50s – 90s (depending on level/temp) | ✅ **Higher efficiency** through adjusted cycle times |
| **Cross Ventilation (Summer)** | Only possible in paired configuration | ✅ Yes (via App/Panel/Automation) | ⚖️ Equivalent |
| **Auto Summer Mode** | ❌ No (manual switch required) | ✅ **Automatic cross ventilation** when outdoor temp < indoor temp (NTC + ESP-NOW group data) | ✅ **No manual switching necessary** |
| **Boost Ventilation** | 15 min. intensive, then pause | ✅ Yes (Configurable: time/level) | ✅ **More flexible** |
| **Fan Control** | 3 fixed levels | **10 levels + stepless PID control** (silent, no audible speed jumps) | ✅ **Fine-grained & silent** |
| **Automation (CO2)** | ❌ No (only optional VOC estimation) | ✅ **SCD41 (Real CO2)**: Stepless PID control with deadband hysteresis, configurable min/max levels | ✅ **Precise & quiet** |
| **Automation (Humidity)** | Fixed thresholds: 55%, 65%, 75% rH | ✅ **PID controller** with configurable limit (40-100%), outdoor humidity comparison, deadband hysteresis (±2%) | ✅ **Customizable** + Outdoor check |
| **Presence** | ❌ No | ✅ **mmWave Radar (HLK-LD2450)**: 4 profiles (No adjustment, Intensive, Normal, Low) | ✅ **Demand-based** per room |
| **Night Mode** | ❌ No (manual shutdown) | 📋 Planned: Scheduled throttling/dimming | ✅ **Comfort** (in planning) |

---

## 🛡️ Sensors & Monitoring

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart WRG (This Project) | ESPHome Advantage |
| :--- | :--- | :--- | :--- |
| **CO2 Measurement** | ❌ No (only optional VOC module) | ✅ **SCD41**: Photoacoustic sensing, 400-5000 ppm | ✅ **Real CO2 values** instead of estimation |
| **Temperature** | ❌ No display | ✅ **NTC sensors** (Supply/Extract air) + HA weather data | ✅ **Visualization & Automation** |
| **Air Humidity** | Rudimentary (fixed thresholds) | ✅ **SCD41** (Internal) + **Outdoor humidity** (HA weather service/sensor) | ✅ **PID-controlled + Outdoor check** |
| **Air Pressure** | ❌ No | ✅ **BMP390**: Weather trend, storm warning, SCD41 altitude compensation | ✅ **Additional environmental data** |
| **Presence** | ❌ No | ✅ **HLK-LD2450 mmWave Radar**: Presence, Moving/Still Targets, Target Count | ✅ **Room-accurate detection** |
| **RPM** | Tacho signal (LED flashes on error) | ✅ **Pulse Counter**: RPM value in Home Assistant + error alarm | ✅ **Quantitative monitoring** |

---

## 🔧 Safety & Maintenance

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart WRG (This Project) | ESPHome Advantage |
| :--- | :--- | :--- | :--- |
| **Fan Monitoring** | Tacho signal (LED flashes on error) | ✅ **Tacho Monitor**: Alarm in Home Assistant + LED | ✅ **Push notification** on failure |
| **Frost Protection** | Shutdown if supply air < 5°C | ✅ **NTC Monitoring** (Shutdown/Pre-heating coil) | ✅ **Visualization** of temperatures |
| **Filter Replacement** | Timer-based (LED flashes) | ✅ **Predictive Alarm**: Operating hours (>365d) + calendar time (>3 years) + HA push notification + reset button | ✅ **More precise & proactive** |
| **Error Detection** | LED flashes on tacho error | ✅ **Master LED Strobe**: WiFi outage + ESP-NOW timeout (5 min) | ✅ **More comprehensive** |
| **Communication** | Wired (control line) | **ESP-NOW wireless** (fail-safe, no router required) | ✅ **No phase couplers needed** |
| **Synchronization** | Master/Slave (DIP switch) | **Mesh network** (Auto group discovery, PID demand sync) | ✅ **Automatic + identical fan power** |
| **Power Consumption** | Standby < 1W | Standby **~0.5W** (ESP32-C6) | ⚖️ Comparable |

---

## 📱 Operation & Interfaces

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart WRG (This Project) | ESPHome Advantage |
| :--- | :--- | :--- | :--- |
| **Control Panel** | Buttons + LEDs | ✅ **Supports Original Panel** (full functionality) | ⚖️ **Identical look & feel** |
| **Display** | ❌ No | ✅ **SH1106 OLED** (128x64): 3 diagnostic pages (System, Climate, Network) | ✅ **Immediate diagnosis on device** |
| **Remote Control** | IR remote (optional) | **Any smartphone / tablet / PC** | ✅ **No additional costs** |
| **Smart Home** | ❌ No (proprietary) | ✅ **Native Home Assistant API** | ✅ **Full integration** |
| **Group Control** | Master/Slave via cable | ✅ **ESP-NOW Group Controller**: All devices as one unit in HA dashboard | ✅ **Intuitive + wireless** |
| **Data Transparency** | ❌ None (Black box) | ✅ **Full history** (Temp, CO2, Humidity, Pressure, RPM, Op-hours) | ✅ **Data-driven optimization** |
| **Config Sync** | ❌ No (each device individually) | ✅ **Automatic via ESP-NOW**: All settings synchronized group-wide | ✅ **Set once, active everywhere** |
| **Updates** | ❌ No (hardware swap needed) | ✅ **OTA Updates** (Over-the-Air) | ✅ **Future-proof** |
| **Configuration** | DIP switch on PCB | **Web interface / YAML / HA Slider** | ✅ **Convenient** |
| **System & License** | Closed (proprietary) | ✅ **100% Open Source (MIT)** | ✅ **No vendor lock-in** |

---

## 💡 Conclusion

The ESPHome solution is a **"drop-in replacement"** for the original VentoMaxx control. It maintains the proven operation via the original panel but eliminates the limitations of the original and extends the functional range many times over:

| Category | VentoMaxx (Original) | ESPHome Smart WRG |
| :--- | :---: | :---: |
| Operating Modes | 3 | **5+** (incl. automation) |
| Sensors | 0-1 (opt. VOC) | **6** (CO2, Temp, Humidity, Pressure, Radar, Tacho) |
| Fan Levels | 3 | **10 + stepless (PID)** |
| Smart Home | ❌ | ✅ Home Assistant |
| Maintenance Alarm | Timer LED | ✅ **Predictive + Push** |
| Cable for Sync | Yes | ❌ **Wireless (ESP-NOW)** |
| Open Source | ❌ | ✅ MIT License |

**Recommendation:** Those looking for **full smart home integration** and wanting to avoid **pulling new cables** will find a massively extended feature set with the ESPHome solution while maintaining the same mechanical compatibility — including precision sensors, predictive maintenance, and intelligent automation.
