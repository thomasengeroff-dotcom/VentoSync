// ==========================================================================
// WRG Wohnraumlüftung – ESPHome Custom Component
// https://github.com/thomasengeroff-dotcom/VentoSync
//
// Copyright (c) 2026 Thomas Engeroff
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
// File:        globals.h
// Description: Global component pointers and constants.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once

/// @file automation_helpers.h
/// @brief Inline helper functions called from ESPHome YAML lambdas.
/// Bridges the gap between YAML automations and the C++ VentilationController /
/// VentilationLogic classes.
///
/// NOTE (Bug #8 FIXED): All component pointers below use `extern` so this
/// header can be compiled standalone. The actual definitions live in the
/// ESPHome-generated main.cpp as `static` variables; because this file is
/// #included directly from main.cpp, they are visible without linkage issues.

#include "esp_mac.h"
#include "esp_now.h"
#include "esphome.h"
#include <algorithm>
#include <deque>
#include <string>
#include <vector>


// Core and Component Headers
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/globals/globals_component.h"
#include "esphome/components/homeassistant/sensor/homeassistant_sensor.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/ntc/ntc.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/script/script.h"
#include "esphome/components/sntp/sntp_component.h"
#include "esphome/components/template/number/template_number.h"
#include "esphome/components/template/select/template_select.h"
#include "esphome/components/template/sensor/template_sensor.h"
#include "esphome/components/template/switch/template_switch.h"
#include "esphome/components/template/text_sensor/template_text_sensor.h"

// Custom Component Header
#include "esphome/components/ventilation_group/ventilation_group.h"
#include "esphome/components/ventilation_logic/ventilation_logic.h"

// --- Constants ---------------------------------------------------------
static constexpr float SUMMER_COOLING_THRESHOLD_INDOOR = 22.0f;
static constexpr float SUMMER_COOLING_THRESHOLD_INDOOR_HYSTERESIS = 21.5f;
static constexpr float SUMMER_COOLING_MIN_DELTA = 1.5f;
static constexpr float SUMMER_COOLING_HYSTERESIS = 0.5f;
static constexpr uint32_t PEER_TIMEOUT_MS = 300000; // 5 minutes
static constexpr float PID_SYNC_THRESHOLD = 0.05f;  // Minimum demand change to trigger sync

// --- Component pointer declarations (extern) ---------------------------
// FIXED #8: All pointers are declared `extern` so this header can be compiled
// independently. The definitions are the `static` variables in
// ESPHome-generated main.cpp, visible here because we are #included from
// main.cpp.

/// @name Global variables
/// @{
extern esphome::globals::GlobalsComponent<bool>
    *system_on; ///< Master power state.
extern esphome::globals::GlobalsComponent<bool>
    *ventilation_enabled; ///< Ventilation enabled flag.
extern esphome::globals::RestoringGlobalsComponent<int>
    *current_mode_index; ///< Active mode index (0–3).
extern esphome::globals::RestoringGlobalsComponent<int>
    *fan_intensity_level; ///< Fan intensity (1–10).
extern esphome::globals::RestoringGlobalsComponent<int>
    *automatik_min_fan_level; ///< Min fan level for auto control (moisture
                              ///< protection).
extern esphome::globals::RestoringGlobalsComponent<int>
    *automatik_max_fan_level; ///< Max fan level for auto control (noise limit).

extern esphome::globals::RestoringGlobalsComponent<bool> *auto_mode_active;
extern esphome::globals::RestoringGlobalsComponent<int> *auto_co2_threshold_val;
extern esphome::globals::RestoringGlobalsComponent<int>
    *auto_humidity_threshold_val;
extern esphome::globals::RestoringGlobalsComponent<int> *auto_presence_val;
extern esphome::globals::GlobalsComponent<float> *co2_pid_result;
extern esphome::globals::GlobalsComponent<float> *humidity_pid_result;
extern esphome::globals::RestoringGlobalsComponent<float>
    *filter_operating_hours;
extern esphome::globals::RestoringGlobalsComponent<int> *filter_last_change_ts;
/// @}

/// @name Template UI components
/// @{
extern esphome::template_::TemplateSelectWithSetAction<false, false, false, 0>
    *luefter_modus; ///< Mode selector (WRG/Stoß/Durchlüften/Aus).
// auto_presence_behavior removed
extern esphome::template_::TemplateNumber
    *vent_timer; ///< Ventilation timer (minutes).
extern esphome::template_::TemplateNumber
    *fan_intensity_display; ///< Fan intensity display number.
extern esphome::template_::TemplateNumber
    *automatik_min_luefterstufe; ///< Min fan level for automated modes.
extern esphome::template_::TemplateNumber
    *automatik_max_luefterstufe; ///< Max fan level for automated modes.
extern esphome::template_::TemplateNumber
    *auto_presence_slider; ///< Presence compensation slider.
extern esphome::template_::TemplateNumber
    *auto_co2_threshold; ///< CO2 automated threshold.
extern esphome::template_::TemplateNumber
    *auto_humidity_threshold; ///< Humidity automated threshold.
extern esphome::template_::TemplateNumber
    *sync_interval_config; ///< Time between auto-sync packets in minutes.
extern esphome::template_::TemplateNumber
    *config_floor_id; ///< Persistent Floor ID number.
extern esphome::template_::TemplateNumber
    *config_room_id; ///< Persistent Room ID number.
extern esphome::template_::TemplateNumber
    *config_device_id; ///< Persistent Device ID number.
extern esphome::template_::TemplateSelectWithSetAction<false, true, true, 0>
    *config_phase; ///< Persistent Phase A/B selection.
/// @}

extern esphome::globals::RestoringGlobalsComponent<bool>
    *peer_check_enabled; ///< ESP-NOW peer check on/off.
extern esphome::template_::TemplateSwitch
    *peer_check_switch; ///< HA switch for peer check.
extern esphome::globals::GlobalsComponent<bool>
    *intensity_bounce_up; ///< Hold-to-cycle direction.
extern esphome::globals::RestoringGlobalsComponent<float>
    *max_led_brightness; ///< Global LED brightness limit.
extern esphome::globals::GlobalsComponent<bool>
    *thermal_warning_active; ///< True if BMP390 detects >50°C.

/// @name Scripts
/// @{
extern esphome::script::RestartScript<>
    *update_leds; ///< Refreshes all status LEDs.
extern esphome::script::RestartScript<>
    *ui_timeout_script; ///< UI 30s timeout script.
extern esphome::script::RestartScript<>
    *fan_speed_update; ///< Re-applies fan speed.
extern esphome::script::SingleScript<float, int>
    *set_fan_speed_and_direction;               ///< Sets PWM + direction.
extern esphome::sntp::SNTPComponent *sntp_time; ///< SNTP Time component.
/// @}

/// @name Fan hardware
/// @{
extern esphome::speed::SpeedFan *lueftung_fan; ///< Main fan (speed platform).
extern esphome::ledc::LEDCOutput
    *fan_pwm_primary; ///< Primary PWM output (GPIO19).
/// @}

/// @name Sensors
/// @{
extern esphome::sensor::Sensor *scd41_co2; ///< SCD41 CO2 sensor.
extern esphome::template_::TemplateSensor
    *effective_co2; ///< Unified CO2 sensor (SCD41 or BME680 fallback).
extern esphome::sensor::Sensor *temperature; ///< Room Temperature (SCD41)
extern esphome::ntc::NTC *temp_zuluft; ///< Supply Air Temperature (NTC Inside)
extern esphome::ntc::NTC
    *temp_abluft; ///< Exhaust Air Temperature (NTC Outside)
extern esphome::sensor::Sensor *scd41_humidity; ///< Indoor Humidity
extern esphome::homeassistant::HomeassistantSensor
    *outdoor_humidity; ///< Outdoor Humidity (HA)
extern esphome::binary_sensor::BinarySensor
    *radar_presence; ///< Presence Sensor (Radar)
/// @}

/// @name Status LEDs (monochromatic light components)
/// @{
extern esphome::light::LightState *status_led_l1;    ///< Level indicator LED 1.
extern esphome::light::LightState *status_led_l2;    ///< Level indicator LED 2.
extern esphome::light::LightState *status_led_l3;    ///< Level indicator LED 3.
extern esphome::light::LightState *status_led_l4;    ///< Level indicator LED 4.
extern esphome::light::LightState *status_led_l5;    ///< Level indicator LED 5.
extern esphome::light::LightState *status_led_power; ///< Power status LED.
extern esphome::light::LightState
    *status_led_master; ///< Master status LED (error indicator).
extern esphome::light::LightState
    *status_led_mode_wrg; ///< Mode LED: Wärmerückgewinnung.
extern esphome::light::LightState
    *status_led_mode_vent; ///< Mode LED: Ventilation.
/// @}

/// Ventilation controller component instance.
extern esphome::VentilationController *ventilation_ctrl;

// --- ESP-NOW Dynamic Discovery & Persistence -------------------------
inline std::vector<uint8_t>
build_and_populate_packet(esphome::MessageType type);

// Helper to parse MAC address strings (e.g., "AA:BB:CC:DD:EE:FF")
inline esphome::optional<std::array<uint8_t, 6>>
parse_mac_local(const std::string &str) {
  std::array<uint8_t, 6> res;
  int values[6];
  if (sscanf(str.c_str(), "%X:%X:%X:%X:%X:%X", &values[0], &values[1],
             &values[2], &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; ++i)
      res[i] = (uint8_t)values[i];
    return res;
  }
  return {};
}

// NOTE: espnow_peers is a static variable declared in the ESPHome-generated
// main.cpp. Since this header is #included from main.cpp, it is visible
// without any extern declaration.
// NOTE: We use esphome::espnow::global_esp_now which is properly declared
// as extern in espnow_component.h.

/** @brief Helper to check if a MAC matches the local device. */
inline float last_fan_pwm_level = 0.5f; // Default: Stop

/// @name NTC Stabilization Logic
/// @{

// Stabilization parameter constants
static const float NTC_MAX_DEVIATION = 0.3f; // Max allowed deviation in window

// State for each NTC sensor (0 = zuluft, 1 = abluft)
// FIXED #9: Changed static→inline for mutable state (C++17, safe for single TU)
inline std::deque<float> ntc_history[2];
inline uint32_t last_direction_change_time = 0;

/// Call this when fan direction (switch) changes
inline void notify_fan_direction_changed() {
  last_direction_change_time = millis();
  ntc_history[0].clear();
  ntc_history[1].clear();
  ESP_LOGD("ntc_filter",
           "Fan direction changed. Resetting NTC history buffers.");
}

/// NTC Sliding Window Stabilization Filter
/// @param sensor_idx 0 for temp_zuluft, 1 for temp_abluft
/// @param new_value The raw value reported by the physical NTC component
/// @return The original value if stable, else empty optional to discard update
inline esphome::optional<float> filter_ntc_stable(int sensor_idx,
                                                  float new_value) {
  if (ventilation_ctrl == nullptr ||
      ventilation_ctrl->state_machine.cycle_duration_ms == 0) {
    return new_value; // Fallback if controller is not bound
  }

  uint32_t cycle_duration_ms =
      ventilation_ctrl->state_machine.cycle_duration_ms;

  // FIXED W1: Warn if cycle is too short for meaningful NTC stabilization.
  // Theoretical minimum for the dynamic cycle (Level 10 = 50s) is well above
  // 30s. If this fires, something is wrong with the cycle configuration.
  if (cycle_duration_ms < 30000) {
    ESP_LOGW("ntc_filter",
             "cycle_duration_ms=%u is very short (<30s). NTC stabilization "
             "filter may never pass a value.",
             cycle_duration_ms);
  }

  // Dynamic wait time: 40% of the cycle (was 60%), but minimum 15 seconds (was
  // 20)
  uint32_t wait_time_ms =
      std::max((uint32_t)15000, (uint32_t)(cycle_duration_ms * 0.4f));

  // Safety check so we don't wait longer than the cycle itself minus 5s
  if (wait_time_ms >= cycle_duration_ms) {
    wait_time_ms = cycle_duration_ms > 5000 ? cycle_duration_ms - 5000 : 0;
  }

  // FIXED W5: Use a smaller, fixed window of 3 samples (9s) for stability.
  // Dynamic window based on remaining time was too restrictive, often
  // preventing any value from being published during the 70s cycle.
  const size_t target_window_size = 3;

  // 1. Check if we are still within the mandatory thermal adjustment wait time
  if (millis() - last_direction_change_time < wait_time_ms) {
    return {}; // Discard value while ceramic adjusts
  }

  // 2. Add current value to history window
  auto &history = ntc_history[sensor_idx];
  history.push_back(new_value);

  // Maintain maximum window size limit
  while (history.size() > target_window_size) {
    history.pop_front();
  }

  // 3. Wait until the window is full for a reliable stabilization check
  if (history.size() < target_window_size) {
    return {}; // Discard value while checking buffer window fills up
  }

  // 4. Calculate deviation
  auto [min_it, max_it] = std::minmax_element(history.begin(), history.end());
  float min_val = *min_it;
  float max_val = *max_it;

  // 5. Evaluate stability
  if ((max_val - min_val) <= NTC_MAX_DEVIATION) {
    // Temperature is stable!
    return new_value;
  } else {
    // Still fluctuating, keep last known state
    return {};
  }
}
/// @}

/// @brief Returns a human-readable German CO2 classification.

// --- FORWARD DECLARATIONS ---
inline bool is_local_mac(const uint8_t *mac);
inline void register_peer_dynamic(const uint8_t *mac);
inline void load_peers_from_runtime_cache();
inline void send_discovery_broadcast();
inline void request_peer_status();
inline void send_discovery_confirmation(const uint8_t *target_mac);
inline void send_sync_to_all_peers(const std::vector<uint8_t> &data);
inline void sync_settings_to_peers();
inline void handle_espnow_receive(const std::vector<uint8_t> &data);
inline void set_ventilation_timer(float value);
inline void set_sync_interval_handler(float value);
inline void set_fan_intensity_slider(float value);
inline void set_operating_mode_select(const std::string &x);
inline void handle_button_mode_click();
inline void handle_button_power_short_click();
inline void handle_button_power_long_click();
inline void handle_button_level_click();
inline void handle_intensity_bounce();
inline void evaluate_auto_mode(bool force = false);
inline void update_filter_analytics();
inline void cycle_operating_mode(int mode_index);
inline void sync_config_to_controller();
inline void run_led_self_test();
inline void update_leds_logic();
inline void check_master_led_error();
inline void set_fan_logic(float speed, int direction);
inline float level_to_speed(float level);
inline float get_current_target_speed();
inline float calculate_virtual_fan_rpm(float raw_rpm);
inline void update_fan_logic();
inline bool is_fan_slider_off(float value);
inline float calculate_ramp_up(int iteration);
inline float calculate_ramp_down(int iteration);
