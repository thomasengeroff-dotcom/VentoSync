# Home Assistant Entities: VentoSync

This file lists all Home Assistant entities provided by the `ventosync.yaml` configuration (and its included packages). It is conceptually grouped by topics to provide a basis for clean naming and integration into dashboards.

## 1. Control & Operating Mode (Ventilation)

These entities are for primary control of the fan and operating mode.

* **`select.ventilation_mode`** ("Ventilation Mode")
  * *Type:* Select
  * *Documentation:* Allows selecting the primary operating mode: `Heat Recovery`, `Boost Ventilation`, `Cross Ventilation`, `Automatic`, `Off`.
* **`number.fan_intensity_display`** ("Fan Intensity")
  * *Type:* Number (Slider)
  * *Documentation:* Manual setting of the fan level (1 to 10). Overwritten in Automatic mode.
* **`sensor.fan_rpm`** ("Fan Speed")
  * *Type:* Sensor
  * *Documentation:* Shows the fan speed in RPM. If no physical tacho sensor (4-pin fan) is connected, a calculated value based on the power level is output (100% = 4200 RPM).
* **`text_sensor.direction_display`** ("Current Direction")
  * *Type:* Text Sensor
  * *Documentation:* Shows the current direction of the airflow: "Supply (In)", "Extract (Out)", or "Standstill".
* **`sensor.ventilation_timer_remaining`** ("Remaining Time")
  * *Type:* Sensor
  * *Documentation:* Shows how much time is left in timed modes (e.g., Boost).

## 2. Automation & Regulation (Sensor-driven)

These entities configure the behavior of the "Automatic" mode.

* **`number.auto_co2_threshold`** ("Automatic: CO2 Threshold")
  * *Type:* Number (Slider)
  * *Documentation:* Sets the target CO2 value (setpoint) in ppm for the PID controller. The CO2 regulation is always active in Automatic mode and has priority over humidity regulation.
* **`number.auto_humidity_threshold`** ("Automatic: Humidity Threshold")
  * *Type:* Number (Slider)
  * *Documentation:* An optional humidity threshold (%), above which the ventilation also increases.
* **`number.automatic_min_fan_level`** ("Automatic Min Fan Level")
  * *Type:* Number (Slider)
  * *Documentation:* Basic ventilation (mold protection) in automatic mode, below which the fan does not drop (Level 1-10).
* **`number.automatic_max_fan_level`** ("Automatic Max Fan Level")
  * *Type:* Number (Slider)
  * *Documentation:* Upper limit (e.g., for noise protection) for automatic mode (Level 1-10).
* **`number.auto_presence_slider`** ("Presence Fan Adjustment")
  * *Type:* Number (Slider)
  * *Documentation:* Allows setting (from -5 to +5) how many levels the ventilation should be increased/decreased when the radar sensor detects a person in the room.

## 3. Times & Intervals

* **`number.vent_timer`** ("Ventilation Duration (min)")
  * *Type:* Number
  * *Documentation:* Sets how long (in minutes) the timer for boost or summer cross-ventilation runs before the device returns to the previous mode (0 = continuous operation).
* **`number.sync_interval_config`** ("Sync Interval")
  * *Type:* Number
  * *Documentation:* Defines the rhythm for synchronizing device groups via WiFi / ESP-NOW.

## 4. Sensor Data & Climate

These entities represent values read from hardware sensors.

* **`sensor.scd41_temperature`** ("SCD41 Temperature")
  * *Type:* Sensor (SCD4x)
  * *Documentation:* Indoor temperature at the control unit.
* **`sensor.scd41_humidity`** ("SCD41 Humidity")
  * *Type:* Sensor (SCD4x)
  * *Documentation:* Indoor humidity at the control unit.
* **`sensor.scd41_co2`** ("SCD41 CO2")
  * *Type:* Sensor (SCD4x)
  * *Documentation:* Real CO2 measurement (ppm).
* **`text_sensor.scd41_co2_evaluation`** ("SCD41 CO2 Evaluation")
  * *Type:* Text Sensor
  * *Documentation:* Provides CO2 quality in text form (e.g., "Excellent", "Poor").
* **`sensor.temp_supply_air`** ("Temperature Supply Air (Indoor)")
  * *Type:* Sensor (NTC)
  * *Documentation:* Monitors the supply air temperature in the tube.
* **`sensor.temp_extract_air`** ("Temperature Extract Air (Outdoor)")
  * *Type:* Sensor (NTC)
  * *Documentation:* Monitors the extract air temperature towards the outer wall.
* **`sensor.heat_recovery_efficiency`** ("Heat Recovery Efficiency")
  * *Type:* Sensor (Template)
  * *Documentation:* Continuously calculates the theoretical efficiency (%) based on indoor, outdoor, and tube temperature differentials.
* **`sensor.temperature`** ("BMP390 Temperature") / **`sensor.pressure`** ("BMP390 Air Pressure")
  * *Type:* Sensor
  * *Documentation:* Optional sensor for air pressure (used for auto-calibration of the CO2 sensor at specific altitudes).

## 5. Radar / Presence (HLK-LD2450)

* **`binary_sensor.radar_presence`** ("Radar Presence")
  * *Type:* Binary Sensor
  * *Documentation:* General motion detection (Active/Inactive).
* **Further Targets:**
  * `binary_sensor.radar_moving_target`
  * `binary_sensor.radar_still_target`
  * `sensor.radar_total_target_count`

## 6. Maintenance & Hardware

* **`binary_sensor.filter_change_alarm`** ("Filter Change Alarm")
  * *Type:* Binary Sensor
  * *Documentation:* Becomes active as soon as the calculated operating days exceed the maintenance interval.
* **`sensor.filter_operating_days_sensor`** ("Filter Operating Days")
  * *Type:* Sensor (Template)
  * *Documentation:* Counter of active operating days since the last filter change.
* **`button.filter_reset_btn`** ("Filter Changed (Reset)")
  * *Type:* Button
  * *Documentation:* Resets the operating days counter to 0 after a physical filter replacement.
* **`button.system_reboot`** ("System Reboot")
  * *Type:* Button
  * *Documentation:* Triggers a restart of the ESP32.

## 7. Status LEDs & Visual Feedback

Physical LEDs on the controller interface.

* `light.status_led_power` ("Status LED Power")
* `light.status_led_master` ("Status LED Master" - with "Error Blink" warnings)
* `light.status_led_mode_wrg` ("Status LED Mode HRV" - with "Automatic Pulse")
* `light.status_led_mode_vent` ("Status LED Mode Vent")
* `light.status_led_l1` to `light.status_led_l5` ("Status LED L1" - "L5" for intensity display)

## 8. Setup & Device Configuration

(Note: These entities are primarily intended for initial configuration after installing the hardware)

* **`number.config_floor_id`**, **`number.config_room_id`**, **`number.config_device_id`**
  * *Type:* Number
  * *Documentation:* Address configuration of the device for group synchronization via ESP-NOW. Devices with the same Floor+Room form a multi-device group.
* **`switch.config_phase`** ("Device Phase (A/B)")
  * *Type:* Switch
  * *Documentation:* Changes the initial fan rhythm for push-pull pairs (Phase A starts blowing, Phase B starts sucking).
* **`button.device_config_summary`** ("Device Configuration")
  * *Type:* Button
  * *Documentation:* Saves and applies the group IDs configured above (Device Builder Helper).
* **`button.reset_to_defaults`** ("Reset to Defaults")
  * *Type:* Button
  * *Documentation:* Resets EEPROM/VLAN values of the room assignments.

*(Additionally, the project provides standard sensors for WiFi, uptime, and IP information as diagnostic entities).*
