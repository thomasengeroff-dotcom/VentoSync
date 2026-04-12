// ==========================================================================
// VentoSync HRV – ESPHome Custom Component
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
#include "esp_wifi.h"
#include "esphome.h"
#include <algorithm>
#include <array>
#include <deque>
#include <mutex>
#include <queue>
#include <string>
#include <vector>


// Core and Component Headers
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/globals/globals_component.h"
#include "esphome/components/homeassistant/sensor/homeassistant_sensor.h"
#include "esphome/components/homeassistant/binary_sensor/homeassistant_binary_sensor.h"
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
#include "esphome/components/pid/pid_climate.h"
#include "esphome/components/ventilation_group/ventilation_group.h"
#include "esphome/components/ventilation_logic/ventilation_logic.h"

// --- Constants ---------------------------------------------------------
static constexpr float SUMMER_COOLING_THRESHOLD_INDOOR = 22.0f;
static constexpr float SUMMER_COOLING_THRESHOLD_INDOOR_HYSTERESIS = 21.5f;
static constexpr float SUMMER_COOLING_MIN_DELTA = 1.5f;
static constexpr float SUMMER_COOLING_HYSTERESIS = 0.5f;

/** @brief Threshold for stale peer detection. 5 minutes is used to allow for
 * temporary WiFi drops without losing sync. */
static constexpr uint32_t PEER_TIMEOUT_MS = 300000;

/** @brief Sensitivity for state broadcasts. Demand changes below this are
 * ignored to prevent network jitter. */
static constexpr float PID_SYNC_THRESHOLD = 0.05f;

/** @brief Hardware ACK failure limit before a peer is considered dead. */
static constexpr uint8_t MAX_PEER_SEND_FAILURES = 10;

// --- Binary Peer Cache (Runtime) ----------------------------------------
/// @brief Runtime peer entry for fast MAC lookup during send operations.
/// Uses linear scan over std::vector (O(n), negligible for typical 2-4 peers).
/// The NVS-backed espnow_peers string remains the persistent source of truth.
struct PeerEntry {
  std::array<uint8_t, 6> mac;
  uint32_t last_seen;   ///< millis() timestamp of last successful interaction
  uint8_t fail_count;   ///< consecutive unicast send failures
};

inline std::vector<PeerEntry> peer_cache;

// --- Thread-Safe ESP-NOW Receive Queue ------------------------------------
/// @brief Incoming packet captured from the WiFi task for deferred processing.
/// The WiFi/ESP-NOW callback must not perform heavy operations (flash writes,
/// memory allocation, peer registration) because it runs in a high-priority
/// interrupt-like context. Instead we copy the raw data here and process it
/// in the main ESPHome loop.
struct IncomingPacket {
  std::vector<uint8_t> data;       ///< Raw payload bytes.
  std::array<uint8_t, 6> src_mac;  ///< Sender MAC address.
};

/// Maximum number of queued packets before dropping (prevents OOM under flood).
inline constexpr size_t RX_QUEUE_MAX_DEPTH = 16;

/// Mutex protecting rx_queue. Acquired briefly by WiFi task (push) and main loop (drain).
inline std::mutex rx_queue_mutex;
/// FIFO queue of packets waiting for main-loop processing.
inline std::queue<IncomingPacket> rx_queue;

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
extern esphome::globals::RestoringGlobalsComponent<int>
    *summer_cooling_threshold; ///< Indoor temp threshold for summer cooling (°C).
extern esphome::globals::GlobalsComponent<float> *co2_pid_result;
extern esphome::globals::GlobalsComponent<float> *humidity_pid_result;
extern esphome::globals::RestoringGlobalsComponent<float>
    *filter_operating_hours;
extern esphome::globals::RestoringGlobalsComponent<int> *filter_last_change_ts;
extern esphome::globals::RestoringGlobalsComponent<int> *watchdog_restarts_count;
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
    *config_device_id;
extern esphome::template_::TemplateSelectWithSetAction<false, true, true, 0>
    *config_phase;
extern esphome::template_::TemplateSensor *watchdog_restarts;
extern esphome::template_::TemplateTextSensor *espnow_peers_display; ///< Peer list display.

namespace led_state {
  inline std::string last_master_effect = "__none__";
  inline float last_master_brightness = -1.0f;
}
/// @}

extern esphome::globals::RestoringGlobalsComponent<bool>
    *peer_check_enabled; ///< ESP-NOW peer check on/off.
extern esphome::template_::TemplateSwitch
    *peer_check_switch; ///< HA switch for peer check.
extern esphome::template_::TemplateSwitch
    *fan_direction; ///< Hardware fan direction switch.
extern esphome::globals::RestoringGlobalsComponent<int>
    *auto_presence_val; ///< Presence bitmask.
extern esphome::globals::GlobalsComponent<bool>
    *ui_active; ///< Flag if UI should be active / bright.
extern esphome::globals::RestoringGlobalsComponent<float>
    *max_led_brightness; ///< Max brightness for LEDs.
extern esphome::globals::GlobalsComponent<bool>
    *intensity_bounce_up; ///< True if we are cycling UP, false if DOWN.
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
extern esphome::pid::PIDClimate *pid_co2;       ///< CO2 PID controller.
extern esphome::pid::PIDClimate *pid_humidity;  ///< Humidity PID controller.
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
extern esphome::homeassistant::HomeassistantBinarySensor
    *sommerbetrieb; ///< Summer mode gate (HA: season + outdoor temp > 18°C)
extern esphome::homeassistant::HomeassistantBinarySensor
    *window_locked; ///< Window lock gate (HA: open windows in room)
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

/**
 * @brief   Global pointer to the central Ventilation Controller.
 *
 * @details This is the primary bridge between the helper functions and the
 *          custom component logic.
 */
extern esphome::VentilationController *ventilation_ctrl;

// --- ESP-NOW Dynamic Discovery & Persistence -------------------------
inline std::vector<uint8_t>
build_and_populate_packet(esphome::MessageType type);

// Helper to parse MAC address strings (e.g., "AA:BB:CC:DD:EE:FF")
// HIGH PERFORMANCE: Avoids sscanf for embedded efficiency.
inline esphome::optional<std::array<uint8_t, 6>>
parse_mac_local(const std::string &str) {
  if (str.length() != 17)
    return {};
  std::array<uint8_t, 6> res;
  for (int i = 0; i < 6; i++) {
    char high = str[i * 3];
    char low = str[i * 3 + 1];
    auto parse_hex = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return -1;
    };
    int h = parse_hex(high);
    int l = parse_hex(low);
    if (h == -1 || l == -1) return {};
    res[i] = (h << 4) | l;
    if (i < 5 && str[i * 3 + 2] != ':') return {};
  }
  return res;
}

extern esphome::globals::RestoringGlobalStringComponent<std::string, 255>
    *espnow_peers; ///< ESP-NOW dynamic peer list (JSON-formatted MACs).

// NOTE: We use esphome::espnow::global_esp_now which is properly declared
// as extern in espnow_component.h.

/** @brief Helper to check if a MAC matches the local device. */
inline float last_fan_pwm_level = 0.5f; // Default: Stop
inline float current_smoothed_speed = 0.0f; // Track active slew-rate mapped speed
inline uint32_t last_slew_update_ms = 0;    // Timestamp of last slew step

/// @name NTC Stabilization State
/// @{
// FIXED #9: Changed static→inline for mutable state (C++17, safe for single TU)
inline std::deque<float> ntc_history[2];
inline uint32_t last_direction_change_time = 0;
/// @}

// --- FORWARD DECLARATIONS ---
inline void notify_fan_direction_changed();
inline esphome::optional<float> filter_ntc_stable(int sensor_idx, float new_value);
inline bool is_local_mac(const uint8_t *mac);
inline void register_peer_dynamic(const uint8_t *mac);
inline void load_peers_from_runtime_cache();
inline void send_discovery_broadcast();
inline void request_peer_status();
inline void send_discovery_confirmation(const uint8_t *target_mac);
inline void send_sync_to_all_peers(const std::vector<uint8_t> &data);
inline void sync_settings_to_peers();
inline void handle_espnow_receive(const std::vector<uint8_t> &data, const uint8_t *src_mac);
inline void process_queued_packets();
inline void remove_stale_peer(const uint8_t *mac);
inline void trigger_re_discovery();
inline void reset_peer_fail_count(const uint8_t *mac);
inline void rebuild_peers_string();
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
inline void run_system_boot_initialization();
inline void run_led_self_test();
inline void update_leds_logic();
inline void check_master_led_error();

/**
 * @brief   Determines if this specific device is the Master in the group.
 *
 * @details The Master (ID=1) is responsible for driving the global timing
 *          cycle and broadcasting configuration updates.
 *
 * @return  true  if this is the Master device.
 */
inline bool is_master() {
  return ventilation_ctrl != nullptr && ventilation_ctrl->device_id == 1;
}

/**
 * @brief   Checks if a received packet originated from the Master.
 *
 * @param[in] pkt  Pointer to the received VentilationPacket.
 *
 * @return  true  if the packet's device_id is 1.
 */
inline bool is_from_master(const esphome::VentilationPacket *pkt) {
  return pkt->device_id == 1;
}
inline void set_fan_logic(float speed, int direction);
inline float level_to_speed(float level);
inline float get_current_target_speed();
inline float calculate_virtual_fan_rpm(float raw_rpm);
inline void update_fan_logic();
inline bool is_fan_slider_off(float value);
inline float calculate_ramp_up(int iteration);
inline float calculate_ramp_down(int iteration);
