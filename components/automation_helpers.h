// ==========================================================================
// WRG Wohnraumlüftung – ESPHome Custom Component
// https://github.com/thomasengeroff-dotcom/ESPHome-Wohnraumlueftung
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
// File:        automation_helpers.h
// Description: Helper functions for ESPHome YAML automations.
// Author:      Thomas Engeroff
// Created:     2026-03-07
// Modified:    2026-03-26
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
#include "esphome.h"
#include <algorithm>
#include <deque>
#include <string>
#include <vector>
#include "esp_now.h"

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
extern esphome::globals::GlobalsComponent<int>
    *current_mode_index; ///< Active mode index (0–3).
extern esphome::globals::GlobalsComponent<int>
    *fan_intensity_level; ///< Fan intensity (1–10).
extern esphome::globals::RestoringGlobalsComponent<bool>
    *co2_auto_enabled; ///< CO2 auto-control enabled flag.
extern esphome::globals::RestoringGlobalsComponent<int>
    *automatik_min_fan_level; ///< Min fan level for auto control (moisture
                              ///< protection).
extern esphome::globals::RestoringGlobalsComponent<int>
    *automatik_max_fan_level; ///< Max fan level for auto control (noise limit).

extern esphome::globals::GlobalsComponent<bool> *auto_mode_active;
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
extern esphome::template_::TemplateSwitch
    *automatik_co2; ///< CO2 auto-control on/off switch.
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
inline std::vector<uint8_t> build_and_populate_packet(esphome::MessageType type);

// Helper to parse MAC address strings (e.g., "AA:BB:CC:DD:EE:FF")
inline esphome::optional<std::array<uint8_t, 6>>
parse_mac_local(const std::string &str) {
  std::array<uint8_t, 6> res;
  int values[6];
  if (sscanf(str.c_str(), "%X:%X:%X:%X:%X:%X", &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) == 6) {
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
inline bool is_local_mac(const uint8_t *mac) {
  uint8_t local_mac[6];
  esp_read_mac(local_mac, ESP_MAC_WIFI_STA);
  for (int i = 0; i < 6; i++) {
    if (mac[i] != local_mac[i])
      return false;
  }
  return true;
}

/** @brief Registers a MAC address as an ESP-NOW peer and persists it to NVS. */
inline void register_peer_dynamic(const uint8_t *mac) {
  if (!esphome::espnow::global_esp_now) {
    ESP_LOGW("espnow_disc",
             "global_esp_now not ready, skipping register_peer_dynamic");
    return;
  }
  if (is_local_mac(mac)) {
    return; // Don't register ourselves
  }
  if (espnow_peers == nullptr) {
    ESP_LOGE("espnow_disc", "espnow_peers global is null!");
    return;
  }
  char mac_buf[20];
  snprintf(mac_buf, sizeof(mac_buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);
  std::string mac_str(mac_buf);

  std::string &current_list = espnow_peers->value();
  if (current_list.find(mac_str) == std::string::npos) {
    if (!current_list.empty())
      current_list += ",";
    current_list += mac_str;
    ESP_LOGI("espnow_disc", "New peer discovered and saved: %s",
             mac_str.c_str());
  }

  // FIXED: Register peer with current WiFi channel for reliable unicast
  esp_now_peer_info_t peer_info = {};
  memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
  memcpy(peer_info.peer_addr, mac, 6);
  peer_info.channel = 0; // 0 = use current channel (auto)
  peer_info.encrypt = false;
  peer_info.ifidx = WIFI_IF_STA;

  // Remove first if already exists (channel may have changed)
  esp_now_del_peer(mac);
  esp_err_t err = esp_now_add_peer(&peer_info);

  if (err == ESP_OK) {
    ESP_LOGI("espnow_disc", "Peer %s registered (channel: current)",
             mac_str.c_str());
  } else {
    ESP_LOGW("espnow_disc", "Failed to register peer %s: %d", mac_str.c_str(),
             err);
  }
}

/** @brief Loads all peers saved in the runtime string cache and registers them
 *  with the ESP-NOW stack.
 *
 *  NOTE W4: Despite the previous name (load_peers_from_flash), this does NOT
 *  restore from NVS/flash. espnow_peers is a GlobalsComponent<std::string>
 *  WITHOUT restore_value, so its contents are lost on every reboot.
 *  Peers are automatically re-discovered via the ROOM_DISC broadcast. This
 *  function is called for completeness but will usually be a no-op after boot.
 *
 *  TODO: If persistent peer storage is required, change espnow_peers to
 *  restore_value: true (NVS) and add NVS size validation.
 */
inline void load_peers_from_runtime_cache() {
  if (!esphome::espnow::global_esp_now) {
    ESP_LOGW(
        "espnow_disc",
        "global_esp_now not ready, skipping load_peers_from_runtime_cache");
    return;
  }
  std::string current_list = espnow_peers->value();
  if (current_list.empty())
    return;

  size_t start = 0;
  size_t end = current_list.find(",");
  while (true) {
    std::string mac_str = (end == std::string::npos)
                              ? current_list.substr(start)
                              : current_list.substr(start, end - start);
    auto mac = parse_mac_local(mac_str);
    if (mac.has_value()) {
      esphome::espnow::global_esp_now->add_peer(mac->data());
      ESP_LOGI("espnow_disc", "Restored peer from flash: %s", mac_str.c_str());
    }
    if (end == std::string::npos)
      break;
    start = end + 1;
    end = current_list.find(",", start);
  }
}

/** @brief Sends a discovery broadcast to identify peers in the same room. */
inline void send_discovery_broadcast() {
  if (!esphome::espnow::global_esp_now) {
    ESP_LOGW("espnow_disc",
             "global_esp_now not ready, skipping send_discovery_broadcast");
    return;
  }
  if (!config_floor_id || !config_room_id) {
    ESP_LOGW("espnow_disc",
             "config IDs not ready, skipping send_discovery_broadcast");
    return;
  }
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "ROOM_DISC:%d:%d",
           (int)config_floor_id->state, (int)config_room_id->state);
  std::string msg(buffer);
  std::vector<uint8_t> data(msg.begin(), msg.end());
  uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // FIXED #3: ESPNowComponent::send() signature is send(peer, payload, size).
  // Passing std::vector directly was silently wrong — must use .data()/.size().
  esphome::espnow::global_esp_now->send(broadcast_mac, data, [](esp_err_t err) {
    /* no-op callback required: ESPNow crashes with std::bad_function_call
       without it */
  });
  ESP_LOGI("espnow_disc", "Sent discovery broadcast: %s", msg.c_str());
}

/** @brief Sends a unicast confirmation to a discovered peer. */
inline void send_discovery_confirmation(const uint8_t *target_mac) {
  if (!esphome::espnow::global_esp_now)
    return;
  if (!config_floor_id || !config_room_id)
    return;
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "ROOM_CONF:%d:%d",
           (int)config_floor_id->state, (int)config_room_id->state);
  std::string msg(buffer);
  std::vector<uint8_t> data(msg.begin(), msg.end());
  // FIXED #3: Use correct send(peer, payload_ptr, size) signature.
  esphome::espnow::global_esp_now->send(target_mac, data, [](esp_err_t err) {
    /* no-op callback required: ESPNow crashes with std::bad_function_call
       without it */
  });
}

/** @brief Sends a sync packet to all registered peers via unicast. */
inline void send_sync_to_all_peers(const std::vector<uint8_t> &data) {
  if (!esphome::espnow::global_esp_now)
    return;
  if (espnow_peers == nullptr)
    return;
  std::string current_list = espnow_peers->value();
  if (current_list.empty()) {
    ESP_LOGW("espnow_sync", "No peers in list — cannot send unicast!");
    return;
  }

  ESP_LOGI("espnow_sync", "Sending unicast to peers: %s", current_list.c_str());

  size_t start = 0;
  size_t end = current_list.find(",");
  while (true) {
    std::string mac_str = (end == std::string::npos)
                              ? current_list.substr(start)
                              : current_list.substr(start, end - start);
    auto mac = parse_mac_local(mac_str);
    if (mac.has_value())
      // FIXED #3: Use correct send(peer, payload_ptr, size) signature.
      esphome::espnow::global_esp_now->send(mac->data(), data,
                                            [](esp_err_t err) {
                                              /* no-op callback required: ESPNow
                                                 crashes with
                                                 std::bad_function_call without
                                                 it */
                                            });
    if (end == std::string::npos)
      break;
    start = end + 1;
    end = current_list.find(",", start);
  }
}

/** @brief Parses incoming discovery strings. */
inline bool handle_discovery_payload(const std::string &payload,
                                     const uint8_t *src_addr) {
  if (payload.find("ROOM_DISC:") == 0 || payload.find("ROOM_CONF:") == 0) {
    if (is_local_mac(src_addr)) {
      return true; // Recognize but ignore self-packets
    }
    int floor, room;
    if (sscanf(payload.c_str() + 10, "%d:%d", &floor, &room) == 2) {
      if (floor == (int)config_floor_id->state &&
          room == (int)config_room_id->state) {
        ESP_LOGI("espnow_disc", "Matching room discovery from %02X:%02X:%02X",
                 src_addr[3], src_addr[4], src_addr[5]);
        register_peer_dynamic(src_addr);
        if (payload.find("ROOM_DISC:") == 0)
          send_discovery_confirmation(src_addr);
        return true;
      }
    }
  }
  return false;
}

/** @brief Sends a sync packet to all registered peers via unicast.
 *  This is used for real-time mirroring of settings (CO2, fan levels, etc.)
 */
inline void sync_settings_to_peers() {
  if (ventilation_ctrl == nullptr)
    return;
  // Build a MSG_STATE packet which includes all current settings
  auto data = build_and_populate_packet(esphome::MSG_STATE);

  // OPTIMIZATION: For immediate state changes (button/slider), we use
  // a room-wide broadcast (FF:FF:FF:FF:FF:FF). This is much more robust
  // than unicast because it works even if a peer's MAC is not yet discovered
  // or registered. Periodic sync still uses unicast.
  uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esphome::espnow::global_esp_now->send(broadcast_mac, data, [](esp_err_t err) {});

  ESP_LOGI("vent_sync", "Sent state change via BROADCAST to room members.");
}

// --- Inline wrappers (called from YAML lambdas) -----------------------

// Global constants
static const uint32_t PEER_TIMEOUT_MS = 300000; // 5 minutes

/// Last PWM level sent to fan_pwm_primary via set_level() (0.0–1.0).
/// Used by the 'Lüfter PWM Ansteuerung' template sensor since LEDCOutput has no
/// get_level().
// FIXED #9: Changed static→inline for mutable state (C++17, safe for single TU)
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
inline std::string get_co2_classification(float co2_ppm) {
  return VentilationLogic::get_co2_classification(co2_ppm);
}

/// @brief Calculates CO2-based fan level with hysteresis.
inline int get_co2_fan_level(float co2_ppm, int current_level, int min_level,
                             int max_level) {
  return VentilationLogic::get_co2_fan_level(co2_ppm, current_level, min_level,
                                             max_level);
}

/// @brief Applies CO2-based auto-control if enabled and SCD41 is connected.
/// Checks sensor availability (NaN = not connected), calculates target level,
/// and applies it only if it differs from the current level (gradual change).
// Legacy CO2 control removed in favor of evaluate_auto_mode()

/// @brief Calculates Wärmerückgewinnung (heat recovery) efficiency safely.
inline float calculate_heat_recovery_efficiency(float t_raum, float t_zuluft,
                                                float t_aussen) {
  if (std::isnan(t_raum) || std::isnan(t_zuluft) || std::isnan(t_aussen)) {
    return NAN;
  }
  return VentilationLogic::calculate_heat_recovery_efficiency(t_raum, t_zuluft,
                                                              t_aussen);
}

/// @brief Core logic for the new Standard-Automatik mode.
inline void evaluate_auto_mode() {
  if (!auto_mode_active->value() || !system_on->value() ||
      !ventilation_enabled->value())
    return;

  auto *v = ventilation_ctrl;
  // Default to the configured base level from Home Assistant (Moisture
  // protection / base ventilation)
  int target_level = automatik_min_fan_level->value();
  int internal_mode =
      v->state_machine.current_mode; // Copy current mode as starting point

  // Determine current hardware direction to correctly map NTC sensors during
  // MODE_VENTILATION
  if (v == nullptr)
    return;
  const bool is_intake =
      v->state_machine.get_target_state(millis()).direction_in;

  // --- 1. Compute Local Temperatures ---
  float local_in = NAN;
  float local_out = NAN;

  // SCD41 is always a valid indoor temperature if available
  if (temperature != nullptr && !std::isnan(temperature->state)) {
    local_in = temperature->state;
  }

  if (internal_mode == esphome::MODE_VENTILATION) {
    // Continuous flow mode: NTC sensors show true external/internal air
    // depending on direction
    if (is_intake) {
      // Blowing IN: Exhaust NTC gets true outside air, Supply NTC gets
      // mixed/flow air (ignore)
      if (temp_abluft != nullptr)
        local_out = temp_abluft->state;
    } else {
      // Blowing OUT: Supply NTC gets true inside air, Exhaust NTC gets
      // mixed/flow (ignore)
      if (std::isnan(local_in) && temp_zuluft != nullptr)
        local_in = temp_zuluft->state;
    }
  } else if (internal_mode == esphome::MODE_ECO_RECOVERY) {
    // WRG mode: Stable NTC values are valid representations (thanks to the 45s
    // filter logic)
    if (temp_abluft != nullptr)
      local_out = temp_abluft->state;
    if (std::isnan(local_in) && temp_zuluft != nullptr)
      local_in = temp_zuluft->state;
  }

  // Store in controller for ESP-NOW broadcast
  v->local_t_in = local_in;
  v->local_t_out = local_out;

  // --- 2. Compute Effective Temperatures (Group-wide) ---
  float eff_in = local_in;
  float eff_out = local_out;

  const uint32_t now = millis();
  // Use peer indoor temp if local is missing and peer value is fresh (<5 mins)
  if (std::isnan(eff_in) && !std::isnan(v->last_peer_t_in) &&
      (now - v->last_peer_t_in_time < PEER_TIMEOUT_MS)) {
    eff_in = v->last_peer_t_in;
  }
  // Use peer outdoor temp if local is missing and peer value is fresh (<5 mins)
  if (std::isnan(eff_out) && !std::isnan(v->last_peer_t_out) &&
      (now - v->last_peer_t_out_time < PEER_TIMEOUT_MS)) {
    eff_out = v->last_peer_t_out;
  }

  // --- 3. Summer Cooling Logic ---
  // [NEW v4] Strictly follows:
  // - Activation: Indoor > 22.0°C AND Outdoor cooler by at least 1.5°C
  // - Hysteresis: Disengage when Outdoor is < 0.5°C cooler than Indoor (or
  // warmer)
  if (!std::isnan(eff_in) && !std::isnan(eff_out)) {
    if (internal_mode != esphome::MODE_VENTILATION) {
      if (eff_in > 22.0f && eff_out < (eff_in - 1.5f)) {
        internal_mode = esphome::MODE_VENTILATION;
        ESP_LOGI("auto_mode",
                 "Sommer-Kühlung AKTIVIERT: Innen=%.1f°C, Außen=%.1f°C (Delta "
                 "> 1.5°C)",
                 eff_in, eff_out);
      } else if (internal_mode != esphome::MODE_ECO_RECOVERY) {
        internal_mode = esphome::MODE_ECO_RECOVERY;
      }
    } else {
      // Return to WRG if outdoor is no longer significantly cooler or indoor is
      // cool enough (<21.5 for basic hysteresis)
      if (eff_out >= (eff_in - 0.5f) || eff_in < 21.5f) {
        internal_mode = esphome::MODE_ECO_RECOVERY;
        ESP_LOGI("auto_mode",
                 "Sommer-Kühlung DEAKTIVIERT (Hysterese erreicht): "
                 "Innen=%.1f°C, Außen=%.1f°C",
                 eff_in, eff_out);
      }
    }
  } else if (internal_mode != esphome::MODE_ECO_RECOVERY &&
             internal_mode != esphome::MODE_VENTILATION) {
    internal_mode = esphome::MODE_ECO_RECOVERY;
  }

  // --- 4. Air Quality logic (CO2 & Humidity via PID) ---
  float max_pid_demand = 0.0f;

  if (co2_auto_enabled->value() && co2_pid_result != nullptr) {
    max_pid_demand = std::max(max_pid_demand, co2_pid_result->value());
  }

  if (scd41_humidity != nullptr && outdoor_humidity != nullptr) {
    const float in_hum = scd41_humidity->state;
    const float out_hum = outdoor_humidity->state;

    if (!std::isnan(in_hum) && !std::isnan(out_hum)) {
      if (out_hum < in_hum && humidity_pid_result != nullptr) {
        // Only use humidity demand if outdoor is drier
        max_pid_demand = std::max(max_pid_demand, humidity_pid_result->value());
      }
    }
  }

  // Save local demand to controller for ESP-NOW broadcast
  // Every second, this device tells the group how much ventilation it currently
  // demands.
  if (std::abs(max_pid_demand - v->local_pid_demand) > 0.05f) {
    v->trigger_sync();
  }
  v->local_pid_demand = max_pid_demand;

  // Compare with peer demand if fresh (<5 mins)
  // Synchronized ESP-NOW PID Logic:
  // If a peer device in the same room reports a higher air-clearing demand
  // (because someone is breathing near it, or humidity spiked locally), we
  // adopt that higher demand. This forces all devices in the room to scale up
  // together symmetrically, preventing situations where one device runs loudly
  // at 100% while the other idles at 10%.
  if (!std::isnan(v->last_peer_pid_demand) &&
      (now - v->last_peer_pid_demand_time < PEER_TIMEOUT_MS)) {
    max_pid_demand = std::max(max_pid_demand, v->last_peer_pid_demand);
  }

  if (max_pid_demand > 0.01f) {
    int min_level = automatik_min_fan_level->value();
    int max_level = automatik_max_fan_level->value();

    // Calculate proportional LED target level (1-10 display feedback)
    // Note: The physical motor receives a precision float (0.0 - 1.0) via
    // update_fan_logic() for absolutely silent, stepless transitions. This
    // 'target_level' is kept just so the intensity LEDs on the unit faceplate
    // show a representative average.
    float scaled_level = min_level + max_pid_demand * (max_level - min_level);
    target_level = std::max(target_level, (int)std::round(scaled_level));
    ESP_LOGD("auto_mode",
             "PID Demand=%.2f (Local=%.2f, Peer=%.2f) -> Boost level to %d",
             max_pid_demand, v->local_pid_demand, v->last_peer_pid_demand,
             target_level);
  }

  // 5. Presence logic - Removed from Automatik mode as requested.
  // Presence compensation is now handled directly in get_current_target_speed()
  // for manual modes instead.

  // Apply the computed logic
  if (v->state_machine.current_mode != internal_mode) {
    v->set_mode((esphome::VentilationMode)internal_mode);
  }

  if (fan_intensity_level->value() != target_level) {
    fan_intensity_level->value() = target_level;
    fan_intensity_display->publish_state(target_level);
    ventilation_ctrl->set_fan_intensity(target_level);
    fan_speed_update->execute();
    update_leds->execute();
    ESP_LOGI("auto_mode", "Automode updated fan level to %d", target_level);
  }
}

/// @brief Sets fan PWM for the ebm-papst VarioPro reversible fan.
/// This fan uses a SINGLE PWM signal for both speed and direction:
///   - 50.0% duty cycle = STOP (active brake)
///   - 50% → 0%   = Direction: Abluft (Raus) (Stufe 1 @ 30%, Stufe 10 @ 5%)
///   - 50% → 100% = Direction: Zuluft (Rein) (Stufe 1 @ 70%, Stufe 10 @ 95%)
/// @param speed     Target speed as duty cycle fraction (0.1 = min, 1.0 = max).
/// @param direction Target direction: 0 = Abluft (PWM < 50%), 1 = Zuluft (PWM >
/// 50%).
inline void set_fan_logic(float speed, int direction) {
  if (fan_pwm_primary == nullptr)
    return;

  float pwm = VentilationLogic::calculate_fan_pwm(speed, direction);

  fan_pwm_primary->set_level(pwm);
  last_fan_pwm_level = pwm;
}

/// @brief Converts a user-facing level (1-10) to actual hardware PWM speed (10%
/// - 100%)
inline float level_to_speed(float level) {
  return VentilationLogic::calculate_fan_speed_from_intensity(
      (int)std::round(level));
}

/// @brief Calculates the exact target speed (0.0 to 1.0) based on intensity
/// level and auto PID rules.
inline float get_current_target_speed() {
  if (fan_intensity_level == nullptr)
    return 0.0f;

  float intensity = (float)fan_intensity_level->value();

  if (auto_mode_active != nullptr && auto_mode_active->value()) {
    // --- 1. Automatik Mode (PID based) ---
    float max_pid_demand = 0.0f;
    if (co2_auto_enabled->value() && co2_pid_result != nullptr) {
      max_pid_demand = std::max(max_pid_demand, co2_pid_result->value());
    }
    if (humidity_pid_result != nullptr) {
      max_pid_demand = std::max(max_pid_demand, humidity_pid_result->value());
    }
    if (ventilation_ctrl != nullptr) {
      const uint32_t now = millis();
      // FIXED W3: Use has_peer_pid_demand flag — time==0 sentinel unreliable
      // after overflow
      if (ventilation_ctrl->has_peer_pid_demand &&
          (now - ventilation_ctrl->last_peer_pid_demand_time <
           PEER_TIMEOUT_MS)) {
        max_pid_demand =
            std::max(max_pid_demand, ventilation_ctrl->last_peer_pid_demand);
      }
    }

    float min_l = (float)automatik_min_fan_level->value();
    float max_l = (float)automatik_max_fan_level->value();

    if (max_pid_demand > 0.01f) {
      intensity = min_l + max_pid_demand * (max_l - min_l);
    } else {
      intensity = min_l;
    }
  } else {
    // --- 2. Manual Modes (WRG, Ventilation, Stoßlüftung) ---
    // Apply presence compensation only in non-automatic modes
    if (radar_presence != nullptr && radar_presence->has_state() &&
        radar_presence->state) {
      float comp = (float)auto_presence_val->value();
      if (comp != 0.0f) {
        intensity += comp;
        ESP_LOGD("fan", "Applying presence compensation: %.1f (Base: %d)", comp,
                 fan_intensity_level->value());
      }
    }
  }

  // Clamp to valid 1-10 level domain
  intensity = std::max(1.00f, std::min(10.00f, intensity));
  return level_to_speed(intensity);
}

/// @brief Calculates the virtual fan RPM including ramping and air direction.
/// @param raw_rpm The physical RPM reading from the pulse counter (if any).
inline float calculate_virtual_fan_rpm(float raw_rpm) {
  if (raw_rpm > 10.0f) { // Real 4-pin signal
    return raw_rpm;
  }

  float speed = get_current_target_speed();
  bool direction_in = true;
  float ramp_factor = 1.0f;

  if (ventilation_ctrl != nullptr) {
    esphome::HardwareState state =
        ventilation_ctrl->state_machine.get_target_state(millis());
    ramp_factor = state.ramp_factor;
    direction_in = state.direction_in;
  } else if (fan_direction != nullptr && !fan_direction->state) {
    direction_in = false;
  }

  return VentilationLogic::calculate_virtual_fan_rpm(speed, direction_in,
                                                     ramp_factor);
}

/// @brief Updates filter operating hours and initializes transition timestamp.
/// Called every 60s from logic_automation.yaml.
inline void update_filter_analytics() {
  if (system_on->value() && ventilation_enabled->value()) {
    filter_operating_hours->value() += (1.0f / 60.0f);
  }
  auto now = sntp_time->now();
  if (filter_last_change_ts->value() == 0 && now.is_valid()) {
    filter_last_change_ts->value() = now.timestamp;
    // FIXED #2: filter_last_change_ts is int (not int64_t) → use %d, not %lld.
    ESP_LOGI("filter", "Initial filter timestamp set: %d",
             filter_last_change_ts->value());
  }
}

/// @brief Updates fan speed and direction based on intensity and mode.
/// Reads fan_direction switch for direction, calculates speed from
/// intensity/PID, then calls set_fan_logic() which maps both into the single
/// VarioPro PWM signal.
inline void update_fan_logic() {
  if (fan_intensity_level == nullptr)
    return;
  const int intensity = fan_intensity_level->value();

  // Dynamic Cycle duration mapped to fan level:
  if (ventilation_ctrl != nullptr) {
    uint32_t dynamic_cycle_ms =
        VentilationLogic::calculate_dynamic_cycle_duration(intensity);
    if (ventilation_ctrl->state_machine.cycle_duration_ms != dynamic_cycle_ms) {
      ventilation_ctrl->set_cycle_duration(dynamic_cycle_ms);
    }
  }

  float speed = get_current_target_speed();

  // Apply software ramping factor if a controller is available
  if (ventilation_ctrl != nullptr) {
    // FIXED #4: Use fully-qualified esphome::HardwareState (matches
    // calculate_virtual_fan_rpm)
    esphome::HardwareState state =
        ventilation_ctrl->state_machine.get_target_state(millis());
    speed *= state.ramp_factor;

    // Log ramping during transition phases for debugging
    if (state.ramp_factor < 0.99f && state.ramp_factor > 0.01f) {
      ESP_LOGD("fan_ramp", "Ramping speed: %.2f (Base: %.2f, Factor: %.2f)",
               speed, get_current_target_speed(), state.ramp_factor);
    }
  }

  // Read current direction from the fan_direction switch:
  // OFF = Direction: Abluft (Raus) (PWM < 50%)
  // ON  = Direction: Zuluft (Rein) (PWM > 50%)
  const int direction =
      (fan_direction != nullptr && fan_direction->state) ? 1 : 0;

  set_fan_logic(speed, direction);
}

/// @brief Returns true if the manual speed slider is below the off threshold.
inline bool is_fan_slider_off(float value) {
  return VentilationLogic::is_fan_slider_off(value);
}

/// @brief Linear ramp-up for soft fan start (0.0…1.0 over 100 iterations).
inline float calculate_ramp_up(int iteration) {
  return VentilationLogic::calculate_ramp_up(iteration);
}

/// @brief Linear ramp-down for soft fan stop (1.0…0.0 over 100 iterations).
inline float calculate_ramp_down(int iteration) {
  return VentilationLogic::calculate_ramp_down(iteration);
}

/// @brief Applies the operating mode corresponding to the given index.
/// Index mapping: 0 = Automatik, 1 = WRG, 2 = Durchlüften, 3 = Stoßlüftung, 4 =
/// Aus. Manages system_on/ventilation_enabled state, syncs HA select, and
/// triggers fan update.
inline void cycle_operating_mode(int mode_index) {
  auto *v = ventilation_ctrl;
  auto_mode_active->value() = false; // Reset by default

  // Mode names for HA select sync (must match luefter_modus options exactly)
  static const char *mode_names[] = {"Automatik", "Wärmerückgewinnung",
                                     "Durchlüften", "Stoßlüftung", "Aus"};

  switch (mode_index) {
  case 0: // Automatik
    system_on->value() = true;
    ventilation_enabled->value() = true;
    auto_mode_active->value() = true;
    // Directly set mode to bypass set_mode() early-return guard
    // (Automatik and WRG both use MODE_ECO_RECOVERY)
    v->state_machine.current_mode = esphome::MODE_ECO_RECOVERY;
    v->update_hardware();
    v->pending_broadcast = true;
    evaluate_auto_mode(); // Immediate PID/sensor eval
    break;
  case 1: // Wärmerückgewinnung (manual)
    system_on->value() = true;
    ventilation_enabled->value() = true;
    // Directly set mode to bypass set_mode() early-return guard
    v->state_machine.current_mode = esphome::MODE_ECO_RECOVERY;
    v->update_hardware();
    v->pending_broadcast = true;
    break;
  case 2: // Durchlüften — timer from vent_timer number component
    system_on->value() = true;
    ventilation_enabled->value() = true;
    v->set_mode(esphome::MODE_VENTILATION,
                (uint32_t)(vent_timer->state * 60 * 1000));
    break;
  case 3: // Stoßlüftung
    system_on->value() = true;
    ventilation_enabled->value() = true;
    v->set_mode(esphome::MODE_STOSSLUEFTUNG);
    break;
  case 4: // Aus
    system_on->value() = false;
    ventilation_enabled->value() = false;
    v->set_mode(esphome::MODE_OFF);
    lueftung_fan->turn_off();
    fan_pwm_primary->set_level(0.5f); // Neutral stop for VarioPro
    break;
  }

  // Sync HA select dropdown
  if (mode_index >= 0 && mode_index <= 4) {
    std::string mode_str = mode_names[mode_index];
    if (std::string(luefter_modus->current_option()) != mode_str) {
      luefter_modus->publish_state(mode_str);
    }
  }

  if (mode_index != 4) {
    if (system_wakeup != nullptr)
      system_wakeup->execute();
    fan_speed_update->execute();
  } else {
    if (system_sleep != nullptr)
      system_sleep->execute();
  }

  ESP_LOGI("mode", "Mode changed to index %d", mode_index);
}

void check_master_led_error();

/// @brief Refreshes all status LEDs based on system_on, current mode, and fan
/// level. Maps fan intensity 1–10 onto the 5 level LEDs and toggles the two
/// mode LEDs. ALL LEDs (except Master error blink and Power LED) turn off when
/// ui_active is false. Power LED dims to 20% when ui_active is false.
inline void update_leds_logic() {
  auto *v = ventilation_ctrl;
  const int current_mode = v->state_machine.current_mode;

  // 0. Case: System is OFF
  if (!system_on->value()) {
    status_led_power->turn_off().perform();
    status_led_mode_wrg->turn_off().perform();
    status_led_mode_vent->turn_off().perform();
    status_led_l1->turn_off().perform();
    status_led_l2->turn_off().perform();
    status_led_l3->turn_off().perform();
    status_led_l4->turn_off().perform();
    status_led_l5->turn_off().perform();
    check_master_led_error(); // Still show error independently
    return;
  }

  const float max_b = max_led_brightness->value();

  // 1. Case: UI Inactive (Dimming/Night mode)
  if (!ui_active->value()) {
    // Dim Power LED to 20% of max
    status_led_power->turn_on().set_brightness(0.1f * max_b).perform();

    // Turn off all others
    status_led_mode_wrg->turn_off().perform();
    status_led_mode_vent->turn_off().perform();
    status_led_l1->turn_off().perform();
    status_led_l2->turn_off().perform();
    status_led_l3->turn_off().perform();
    status_led_l4->turn_off().perform();
    status_led_l5->turn_off().perform();

    check_master_led_error();
    return;
  }

  // 2. Case: UI Active and System On (Normal operation)

  // 1. Power LED (100% of max)
  status_led_power->turn_on().set_brightness(max_b).perform();

  // 2. Mode LEDs
  status_led_mode_wrg->turn_off().perform();
  status_led_mode_vent->turn_off().perform();

  if (current_mode != esphome::MODE_OFF) {
    if (auto_mode_active->value()) {
      // Smart-Automatik: Left LED pulses slowly to distinguish from manual WRG
      auto call = status_led_mode_wrg->turn_on();
      call.set_effect("Automatik Pulse");
      // Note: Pulse uses YAML-defined min/max but respects the output's overall
      // scale if set. Since we don't have output-level scaling, the pulse will
      // use its YAML values. But we can at least ensure it starts with the
      // right target.
      call.perform();
    } else if (current_mode == esphome::MODE_ECO_RECOVERY) {
      // Manual WRG: Left LED solid on
      auto call = status_led_mode_wrg->turn_on();
      call.set_effect("None");
      call.set_brightness(max_b);
      call.perform();
    } else if (current_mode == esphome::MODE_STOSSLUEFTUNG) {
      // Stoßlüftung: Right LED on
      status_led_mode_vent->turn_on().set_brightness(max_b).perform();
    } else if (current_mode == esphome::MODE_VENTILATION) {
      // Durchlüften: Both LEDs on (solid)
      auto call_wrg = status_led_mode_wrg->turn_on();
      call_wrg.set_effect("None");
      call_wrg.set_brightness(max_b);
      call_wrg.perform();
      status_led_mode_vent->turn_on().set_brightness(max_b).perform();
    }
  }

  // 3. Level LEDs (1-10 mapped to 5 LEDs)
  // Reset all first
  status_led_l1->turn_off().perform();
  status_led_l2->turn_off().perform();
  status_led_l3->turn_off().perform();
  status_led_l4->turn_off().perform();
  status_led_l5->turn_off().perform();

  if (current_mode != esphome::MODE_OFF) {
    const int level = fan_intensity_level->value();

    // Mapping Logic (1-10 -> 5 LEDs)
    switch (level) {
    case 10:
      status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l3->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l4->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l5->turn_on().set_brightness(1.0f * max_b).perform();
      break;
    default: {
      // Level 1-9: 5 LEDs with half/full brightness (2 steps per LED)
      // Stufe 1: L1 50%, Stufe 2: L1 100%, Stufe 3: L1 100%+L2 50%, etc.
      int full_leds = (level) / 2;
      bool half_step = (level % 2 != 0);

      if (level == 1) { // Special case for very first step
        status_led_l1->turn_on().set_brightness(0.5f * max_b).perform();
      } else {
        if (full_leds >= 1)
          status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
        if (full_leds >= 2)
          status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
        if (full_leds >= 3)
          status_led_l3->turn_on().set_brightness(1.0f * max_b).perform();
        if (full_leds >= 4)
          status_led_l4->turn_on().set_brightness(1.0f * max_b).perform();
        if (full_leds >= 5)
          status_led_l5->turn_on().set_brightness(1.0f * max_b).perform();

        if (half_step) {
          if (full_leds == 1)
            status_led_l2->turn_on().set_brightness(0.5f * max_b).perform();
          else if (full_leds == 2)
            status_led_l3->turn_on().set_brightness(0.5f * max_b).perform();
          else if (full_leds == 3)
            status_led_l4->turn_on().set_brightness(0.5f * max_b).perform();
          else if (full_leds == 4)
            status_led_l5->turn_on().set_brightness(0.5f * max_b).perform();
        }
      }
      break;
    }
    }
  }

  // Also ensure the Master LED evaluates its UI status immediately
  check_master_led_error();
}

/// @brief Checks for error conditions and blinks the Master LED accordingly.
/// Error conditions: WiFi disconnected OR no ESP-NOW peer message within 5
/// minutes. Uses the "Error Blink" strobe effect defined on the
/// status_led_master light.
inline void check_master_led_error() {
  bool has_error = false;

  // 1. Check WiFi connectivity (ESPHome API, works on both Arduino and ESP-IDF)
  if (!esphome::wifi::global_wifi_component->is_connected()) {
    has_error = true;
  }

  // 2. Check ESP-NOW peer freshness (5 minutes = 300000ms) — only if enabled
  if (peer_check_enabled != nullptr && peer_check_enabled->value() &&
      ventilation_ctrl != nullptr) {
    const uint32_t now = millis();
    // FIXED W3: Use has_peer_pid_demand flag instead of time==0 sentinel.
    // The time==0 check was unreliable after a millis() overflow (~49.7 days).
    if (!ventilation_ctrl->has_peer_pid_demand ||
        (now - ventilation_ctrl->last_peer_pid_demand_time > PEER_TIMEOUT_MS)) {
      has_error = true;
    }
  }

  // 3. Apply or remove the blink effect
  if (has_error) {
    // Only activate if not already blinking (avoid re-triggering every
    // interval)
    auto call = status_led_master->turn_on();
    call.set_effect("Error Blink");
    // Effect takes care of its own brightness normally, but we could try to
    // scale it. For now, let error blink be prominent or respect max_b.
    call.perform();
  } else {
    // All clear: check if UI is active so we can show Master status, else turn
    // off
    if (ui_active->value()) {
      auto call = status_led_master->turn_on();
      call.set_effect("None");
      call.set_brightness(max_led_brightness->value());
      call.perform();
    } else {
      status_led_master->turn_off().perform();
    }
  }
}

/// @brief Handles an incoming ESP-NOW packet.
/// Delegates parsing to VentilationController, then updates UI globals,
/// select component, LEDs, and fan speed if state changed.
inline void handle_espnow_receive(const std::vector<uint8_t> &data) {
  auto *v = ventilation_ctrl;
  if (v == nullptr)
    return;

  bool changed = v->on_packet_received(data);

  if (changed) {
    // 1. Sync Fan Intensity
    if (fan_intensity_level != nullptr &&
        v->current_fan_intensity != fan_intensity_level->value()) {
      fan_intensity_level->value() = v->current_fan_intensity;
      if (fan_intensity_display != nullptr)
        fan_intensity_display->publish_state(v->current_fan_intensity);
    }

    // 2. Sync Mode & Globals
    int new_mode_idx = 0;
    std::string mode_str = "Wärmerückgewinnung";

    if (v->state_machine.current_mode == esphome::MODE_OFF) {
      new_mode_idx = 4;
      mode_str = "Aus";
      if (ventilation_enabled)
        ventilation_enabled->value() = false;
      if (system_on)
        system_on->value() = false;
      if (auto_mode_active)
        auto_mode_active->value() = false;
    } else {
      if (ventilation_enabled)
        ventilation_enabled->value() = true;
      if (system_on)
        system_on->value() = true;

      if (auto_mode_active != nullptr && auto_mode_active->value()) {
        new_mode_idx = 0;
        mode_str = "Automatik";
      } else if (v->state_machine.current_mode == esphome::MODE_ECO_RECOVERY) {
        new_mode_idx = 1;
        mode_str = "Wärmerückgewinnung";
      } else if (v->state_machine.current_mode == esphome::MODE_VENTILATION) {
        new_mode_idx = 2;
        mode_str = "Durchlüften";
      } else if (v->state_machine.current_mode == esphome::MODE_STOSSLUEFTUNG) {
        new_mode_idx = 3;
        mode_str = "Stoßlüftung";
      }
    }

    if (current_mode_index != nullptr)
      current_mode_index->value() = new_mode_idx;

    // Update Select Component if different
    if (luefter_modus != nullptr &&
        std::string(luefter_modus->current_option()) != mode_str) {
      luefter_modus->publish_state(mode_str);
    }

    // Update Visuals
    update_leds->execute();
    fan_speed_update->execute();
  }

  // --- Configuration Settings Sync ---
  // ESP-NOW payloads carry all user settings. If another node has different
  // configuration values, we adopt them to ensure room-wide parity.
  if (data.size() != sizeof(esphome::VentilationPacket)) {
    return;
  }
  esphome::VentilationPacket *pkt = (esphome::VentilationPacket *)data.data();

  // FIXED #11: Also check magic header and protocol version here to avoid
  // processing invalid/old packets in this bridge function.
  if (pkt->magic_header != 0x42 ||
      pkt->protocol_version != esphome::PROTOCOL_VERSION) {
    return;
  }

  // Guard against corrupt packets with out-of-range values that could
  // crash/break logic
  bool dirty = false;

  // 1. CO2 Automatik (Switch)
  if (pkt->co2_auto_enabled != co2_auto_enabled->value()) {
    co2_auto_enabled->value() = pkt->co2_auto_enabled;
    automatik_co2->publish_state(pkt->co2_auto_enabled);
    ESP_LOGI("vent_sync", "Synced co2_auto_enabled from peer: %d",
             pkt->co2_auto_enabled);
    dirty = true;
  }

  // 2. Automatik Min/Max Levels (Sliders 1-10)
  if (pkt->automatik_min_fan_level >= 1 && pkt->automatik_min_fan_level <= 10 &&
      pkt->automatik_min_fan_level != automatik_min_fan_level->value()) {
    automatik_min_fan_level->value() = pkt->automatik_min_fan_level;
    automatik_min_luefterstufe->publish_state(pkt->automatik_min_fan_level);
    ESP_LOGI("vent_sync", "Synced automatik_min_fan_level from peer: %d",
             pkt->automatik_min_fan_level);
    dirty = true;
  }

  if (pkt->automatik_max_fan_level >= 1 && pkt->automatik_max_fan_level <= 10 &&
      pkt->automatik_max_fan_level != automatik_max_fan_level->value()) {
    automatik_max_fan_level->value() = pkt->automatik_max_fan_level;
    automatik_max_luefterstufe->publish_state(pkt->automatik_max_fan_level);
    ESP_LOGI("vent_sync", "Synced automatik_max_fan_level from peer: %d",
             pkt->automatik_max_fan_level);
    dirty = true;
  }

  // 3. Automatik Thresholds (Numbers)
  if (pkt->auto_co2_threshold_val >= 400 &&
      pkt->auto_co2_threshold_val <= 5000 &&
      pkt->auto_co2_threshold_val != auto_co2_threshold_val->value()) {
    auto_co2_threshold_val->value() = pkt->auto_co2_threshold_val;
    auto_co2_threshold->publish_state(pkt->auto_co2_threshold_val);
    ESP_LOGI("vent_sync", "Synced auto_co2_threshold from peer: %d ppm",
             pkt->auto_co2_threshold_val);
    dirty = true;
  }

  if (pkt->auto_humidity_threshold_val >= 0 &&
      pkt->auto_humidity_threshold_val <= 100 &&
      pkt->auto_humidity_threshold_val !=
          auto_humidity_threshold_val->value()) {
    auto_humidity_threshold_val->value() = pkt->auto_humidity_threshold_val;
    auto_humidity_threshold->publish_state(pkt->auto_humidity_threshold_val);
    ESP_LOGI("vent_sync", "Synced auto_humidity_threshold from peer: %d %%",
             pkt->auto_humidity_threshold_val);
    dirty = true;
  }

  // 4. Presence Compensation Slider (-5 to +5)
  if (pkt->auto_presence_val >= -5 && pkt->auto_presence_val <= 5 &&
      pkt->auto_presence_val != auto_presence_val->value()) {
    auto_presence_val->value() = pkt->auto_presence_val;
    auto_presence_slider->publish_state(pkt->auto_presence_val);
    ESP_LOGI("vent_sync", "Synced auto_presence_val from peer: %d",
             pkt->auto_presence_val);
    dirty = true;
  }

  // 5. System Timers
  if (pkt->sync_interval_min >= 1 && pkt->sync_interval_min <= 1440) {
    if (v->sync_interval_ms != (uint32_t)(pkt->sync_interval_min * 60 * 1000)) {
      v->set_sync_interval(pkt->sync_interval_min * 60 * 1000);
      sync_interval_config->publish_state(pkt->sync_interval_min);
    }
  }

  if (pkt->vent_timer_min <= 1440 &&
      (uint32_t)(pkt->vent_timer_min * 60 * 1000) !=
          v->state_machine.ventilation_duration_ms) {
    v->state_machine.ventilation_duration_ms = pkt->vent_timer_min * 60 * 1000;
    vent_timer->publish_state(pkt->vent_timer_min);
    ESP_LOGI("vent_sync", "Synced vent_timer from peer: %d min",
             pkt->vent_timer_min);
    // Note: setting the timer does not change the active mode.
  }

  // If any configs changed, re-evaluate the auto mode immediately to reflect
  // new boundaries — but ONLY if auto mode is actually active.
  // FIXED K1/W6: Calling evaluate_auto_mode() unconditionally caused a
  // Ping-Pong: evaluate_auto_mode() → trigger_sync() → pending_broadcast=true
  // → send to peer → peer syncs config → loops indefinitely.
  if (dirty && auto_mode_active != nullptr && auto_mode_active->value()) {
    evaluate_auto_mode();
  }
}

/// @brief Updates the ventilation duration on-the-fly without switching mode.
/// Called when the user adjusts the vent_timer number component.
inline void set_ventilation_timer(float value) {
  auto *v = ventilation_ctrl;
  uint32_t ms = (uint32_t)value * 60 * 1000;
  if (v->state_machine.ventilation_duration_ms == ms)
    return;

  // Update the duration but keep current mode
  v->state_machine.ventilation_duration_ms = ms;
  v->trigger_sync();
}

/// @brief Converts a minutes value from the UI slider to ms and updates sync
/// interval.
inline void set_sync_interval_handler(float value) {
  auto *v = ventilation_ctrl;
  v->set_sync_interval((uint32_t)value * 60 * 1000);
}

/// @brief Handles the fan intensity slider: updates global, notifies
/// controller, refreshes LEDs.
inline void set_fan_intensity_slider(float value) {
  int val = (int)value;
  fan_intensity_level->value() = val;

  ventilation_ctrl->set_fan_intensity(val);
  fan_speed_update->execute();
  ui_active->value() = true;
  update_leds->execute();
  ui_timeout_script->execute();
}

/// @brief Handles the mode select dropdown from HA: maps string option to mode
/// index and delegates to cycle_operating_mode() for unified state management.
inline void set_operating_mode_select(const std::string &x) {
  int mode_index = 0;
  if (x == "Automatik")
    mode_index = 0;
  else if (x == "Wärmerückgewinnung")
    mode_index = 1;
  else if (x == "Durchlüften")
    mode_index = 2;
  else if (x == "Stoßlüftung")
    mode_index = 3;
  else if (x == "Aus")
    mode_index = 4;

  current_mode_index->value() = mode_index;
  cycle_operating_mode(mode_index);
  ui_active->value() = true;
  update_leds->execute();
  ui_timeout_script->execute();
}

// --- Button callback handlers ------------------------------------------

/// @brief Mode button press: cycles mode index 0→1→2→3→4→0 and applies.
inline void handle_button_mode_click() {
  current_mode_index->value() = (current_mode_index->value() + 1) % 5;
  cycle_operating_mode(current_mode_index->value());
  ui_active->value() = true;    // Activate UI immediately so LEDs render
  update_leds->execute();       // Show new mode LEDs now
  ui_timeout_script->execute(); // Start 30s auto-dim timer
  ESP_LOGI("button", "Mode button pressed, new index: %d",
           current_mode_index->value());
}

/// @brief Power button short press: turns ventilation_enabled ON.
/// Restores previous mode + fan speed.
inline void handle_button_power_short_click() {
  if (!ventilation_enabled->value()) {
    ventilation_enabled->value() = true;
    ESP_LOGI("power", "System turned ON by short press - restoring mode %d",
             current_mode_index->value());
    cycle_operating_mode(current_mode_index->value());
    fan_speed_update->execute();
    ui_timeout_script->execute();
  } else {
    ESP_LOGI("power", "System turned OFF by short press");
    cycle_operating_mode(4); // Mode index 4 is "Aus", triggers system_sleep via
                             // cycle_operating_mode
    ui_timeout_script->execute();
  }
}

/// @brief Power button long press (>=5s): turns ventilation_enabled OFF.
/// Stops fan and PWM outputs.
inline void handle_button_power_long_click() {
  if (ventilation_enabled->value()) {
    // FIXED #5: Also set system_on=false (was missing, unlike
    // cycle_operating_mode(4))
    system_on->value() = false;
    ventilation_enabled->value() = false;
    ESP_LOGI("power", "System turned OFF by long press (>5s)");
    auto *v = ventilation_ctrl;
    v->set_mode(esphome::MODE_OFF);
    lueftung_fan->turn_off();
    fan_pwm_primary->set_level(0.5f);
    if (system_sleep != nullptr)
      system_sleep->execute();
    ui_timeout_script->execute();
  } else {
    ESP_LOGD("power", "System already OFF, ignoring long press");
  }
}

/// @brief Level button press: cycles fan intensity 1→10→1 and updates UI +
/// hardware.
inline void handle_button_level_click() {
  if (!ventilation_enabled->value())
    return; // Ignore if system is off
  int level = fan_intensity_level->value();

  // Delegate logic to Pure Class
  level = VentilationLogic::get_next_fan_level(level);

  fan_intensity_level->value() = level;
  fan_intensity_display->publish_state(level);

  ventilation_ctrl->set_fan_intensity(level);
  fan_speed_update->execute();
  ui_active->value() = true;    // Activate UI immediately so LEDs render
  update_leds->execute();       // Show new intensity level LEDs now
  ui_timeout_script->execute(); // Start 30s auto-dim timer
  ESP_LOGI("button", "Intensity level: %d", level);
}

/// @brief Handles one step of the "hold-to-cycle" bounce logic.
inline void handle_intensity_bounce() {
  if (!ventilation_enabled->value())
    return;
  int current_level = fan_intensity_level->value();
  bool up = intensity_bounce_up->value();

  if (up) {
    if (current_level < 10) {
      current_level++;
    } else {
      // Reached top, switch to down
      current_level = 9;
      intensity_bounce_up->value() = false;
    }
  } else {
    if (current_level > 1) {
      current_level--;
    } else {
      // Reached bottom, switch to up
      current_level = 2;
      intensity_bounce_up->value() = true;
    }
  }

  // Apply the new level (sync with UI, hardware, and LEDs)
  fan_intensity_level->value() = current_level;
  fan_intensity_display->publish_state(current_level);
  ventilation_ctrl->set_fan_intensity(current_level);
  fan_speed_update->execute();
  ui_active->value() = true;
  update_leds->execute();
  ui_timeout_script->execute();

  ESP_LOGI("button", "Intensity bounce: %d (Direction: %s)", current_level,
           up ? "UP" : "DOWN");
}

/// @brief Central wrapper to create a fully populated VentilationPacket.
/// Calls the standard C++ class method to build the base layout, and then
/// appends the local Home Assistant configuration variables to ensure they
/// traverse the network.
inline std::vector<uint8_t>
build_and_populate_packet(esphome::MessageType type) {
  auto *v = ventilation_ctrl;
  // Get the base vector containing the core logic payload
  std::vector<uint8_t> data = v->build_packet(type);

  // Cast it to our known struct
  esphome::VentilationPacket *pkt = (esphome::VentilationPacket *)data.data();

  // Populate all HA configurations
  pkt->co2_auto_enabled = co2_auto_enabled->value();
  pkt->automatik_min_fan_level = (uint8_t)automatik_min_fan_level->value();
  pkt->automatik_max_fan_level = (uint8_t)automatik_max_fan_level->value();
  pkt->auto_co2_threshold_val = (uint16_t)auto_co2_threshold_val->value();
  pkt->auto_humidity_threshold_val =
      (uint8_t)auto_humidity_threshold_val->value();
  pkt->auto_presence_val = (int8_t)auto_presence_val->value();

  // Timers
  pkt->sync_interval_min = (uint16_t)(v->sync_interval_ms / 60 / 1000);
  pkt->vent_timer_min =
      (uint16_t)(v->state_machine.ventilation_duration_ms / 60 / 1000);

  return data;
}

// --- Boot-time Initialization Helpers ----------------------------------

/// @brief Synchronizes persistent configuration values from HA numbers to the
/// C++ controller. Called during on_boot to ensure the controller has the
/// correct IDs before any ESP-NOW traffic.
inline void sync_config_to_controller() {
  auto *v = ventilation_ctrl;
  if (v == nullptr)
    return;

  uint8_t floor = (uint8_t)config_floor_id->state;
  uint8_t room = (uint8_t)config_room_id->state;
  uint8_t dev = (uint8_t)config_device_id->state;

  // Guard: If values are still 0, the restore hasn't happened yet
  if (floor == 0 && room == 0 && dev == 0) {
    ESP_LOGW("boot", "Config IDs are all 0 — restore_value not yet loaded. "
                     "Will retry on next interval.");
    return;
  }

  v->set_floor_id(floor);
  v->set_room_id(room);
  v->set_device_id(dev);

  bool is_phase_a =
      (config_phase->current_option() == "Phase A (Startet mit Zuluft)");
  v->set_is_phase_a(is_phase_a);

  ESP_LOGI("boot",
           "Synced Config to Controller: Floor %d, Room %d, Device %d, Phase: %s",
           v->floor_id, v->room_id, v->device_id, is_phase_a ? "A" : "B");
}

/// @brief Runs a 3-second visual self-test by turning on all physical status
/// LEDs.
inline void run_led_self_test() {
  if (status_led_mode_wrg == nullptr || status_led_mode_vent == nullptr ||
      status_led_power == nullptr || status_led_master == nullptr) {
    ESP_LOGW("boot", "Status LEDs not ready for self-test");
    return;
  }
  // 1. Turn on all mode and status LEDs
  auto call_wrg = status_led_mode_wrg->turn_on();
  call_wrg.set_effect("None");
  call_wrg.perform();

  status_led_mode_vent->turn_on().perform();
  status_led_power->turn_on().perform();
  status_led_master->turn_on().perform();

  // 2. Turn on all level indicator LEDs
  if (status_led_l1)
    status_led_l1->turn_on().perform();
  if (status_led_l2)
    status_led_l2->turn_on().perform();
  if (status_led_l3)
    status_led_l3->turn_on().perform();
  if (status_led_l4)
    status_led_l4->turn_on().perform();
  if (status_led_l5)
    status_led_l5->turn_on().perform();

  ESP_LOGI("boot", "LED self-test: all LEDs on");
}
