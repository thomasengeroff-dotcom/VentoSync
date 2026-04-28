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

// --- Operating Mode Name Constants -------------------------------------
// Single source of truth for mode name strings.
// IMPORTANT: Must match `luefter_modus` select options in ui_controls.yaml EXACTLY.
// Index order: 0=Smart-Automatik, 1=WRG, 2=Durchlüften, 3=Stoßlüftung, 4=Aus
static constexpr const char *MODE_NAME_AUTO       = "Smart-Automatik";
static constexpr const char *MODE_NAME_WRG        = "Wärmerückgewinnung";
static constexpr const char *MODE_NAME_VENT       = "Durchlüften";
static constexpr const char *MODE_NAME_BOOST      = "Stoßlüftung";
static constexpr const char *MODE_NAME_OFF        = "Aus";

/// @brief All mode names indexed by mode index (0–4), for array-based lookup.
static constexpr const char *MODE_NAMES[] = {
    MODE_NAME_AUTO,   // 0
    MODE_NAME_WRG,    // 1
    MODE_NAME_VENT,   // 2
    MODE_NAME_BOOST,  // 3
    MODE_NAME_OFF,    // 4
};
static_assert(sizeof(MODE_NAMES) / sizeof(MODE_NAMES[0]) == 5,
              "MODE_NAMES array size must match number of operating modes");

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

// --- Thread-Safe ESP-NOW Send-ACK Event Queue (K-1 Fix) -------------------
/// @brief ACK result event enqueued by the WiFi-task send callback.
/// The callback MUST NOT mutate peer_cache directly (Data Race risk) — it
/// only pushes a PeerEvent here; the main loop drains and applies mutations.
struct PeerEvent {
  std::array<uint8_t, 6> mac;
  enum class Type : uint8_t { SEND_OK, SEND_FAIL } type;
};

/// Mutex protecting peer_event_queue (write: WiFi task, read: main loop).
inline std::mutex peer_event_mutex;
/// FIFO queue of send-ACK results waiting for main-loop processing.
inline std::queue<PeerEvent> peer_event_queue;

// --- Component pointer declarations (extern) ---------------------------
// FIXED #8: All pointers are declared `extern` so this header can be compiled
// independently. The definitions are the `static` variables in
// ESPHome-generated main.cpp, visible here because we are #included from
// main.cpp.

/// @name Global variables
/// @{
extern esphome::globals::GlobalsComponent<bool>
    *const system_on; ///< Master power state.
extern esphome::globals::GlobalsComponent<bool>
    *const ventilation_enabled; ///< Ventilation enabled flag.
extern esphome::globals::RestoringGlobalsComponent<int>
    *const current_mode_index; ///< Active mode index (0–3).
extern esphome::globals::RestoringGlobalsComponent<int>
    *const fan_intensity_level; ///< Fan intensity (1–10).
extern esphome::globals::RestoringGlobalsComponent<int>
    *const automatik_min_fan_level; ///< Min fan level for auto control (moisture
                              ///< protection).
extern esphome::globals::RestoringGlobalsComponent<int>
    *const automatik_max_fan_level; ///< Max fan level for auto control (noise limit).

extern esphome::globals::RestoringGlobalsComponent<bool> *const auto_mode_active;
extern esphome::globals::RestoringGlobalsComponent<int> *const auto_co2_threshold_val;
extern esphome::globals::RestoringGlobalsComponent<int>
    *const auto_humidity_threshold_val;
extern esphome::globals::RestoringGlobalsComponent<int> *const auto_presence_val;
extern esphome::globals::RestoringGlobalsComponent<int>
    *const summer_cooling_threshold; ///< Indoor temp threshold for summer cooling (°C).
extern esphome::globals::GlobalsComponent<float> *const co2_pid_result;
extern esphome::globals::GlobalsComponent<float> *const humidity_pid_result;
extern esphome::globals::RestoringGlobalsComponent<float>
    *const filter_operating_hours;
extern esphome::globals::RestoringGlobalsComponent<int> *const filter_last_change_ts;
extern esphome::globals::RestoringGlobalsComponent<int> *const watchdog_restarts_count;
/// @}

/// @name Template UI components
/// @{
extern esphome::template_::TemplateSelectWithSetAction<false, false, false, 0>
    *const luefter_modus; ///< Mode selector (WRG/Stoß/Durchlüften/Aus).
// auto_presence_behavior removed
extern esphome::template_::TemplateNumber
    *const vent_timer; ///< Ventilation timer (minutes).
extern esphome::template_::TemplateNumber
    *const fan_intensity_display; ///< Fan intensity display number.
extern esphome::template_::TemplateNumber
    *const automatik_min_luefterstufe; ///< Min fan level for automated modes.
extern esphome::template_::TemplateNumber
    *const automatik_max_luefterstufe; ///< Max fan level for automated modes.
extern esphome::template_::TemplateNumber
    *const auto_presence_slider; ///< Presence compensation slider.
extern esphome::template_::TemplateNumber
    *const auto_co2_threshold; ///< CO2 automated threshold.
extern esphome::template_::TemplateNumber
    *const auto_humidity_threshold; ///< Humidity automated threshold.
extern esphome::template_::TemplateNumber
    *const sync_interval_config; ///< Time between auto-sync packets in minutes.
extern esphome::template_::TemplateNumber
    *const config_floor_id; ///< Persistent Floor ID number.
extern esphome::template_::TemplateNumber
    *const config_room_id; ///< Persistent Room ID number.
extern esphome::template_::TemplateNumber
    *const config_device_id;
extern esphome::template_::TemplateSelectWithSetAction<false, true, true, 0>
    *const config_phase;
extern esphome::template_::TemplateSensor *const watchdog_restarts;
extern esphome::template_::TemplateTextSensor *const espnow_peers_display; ///< Peer list display.

namespace led_state {
  inline std::string last_master_effect = "__none__";
  inline float last_master_brightness = -1.0f;
}
/// @}

extern esphome::globals::RestoringGlobalsComponent<bool>
    *const peer_check_enabled; ///< ESP-NOW peer check on/off.
extern esphome::template_::TemplateSwitch
    *const peer_check_switch; ///< HA switch for peer check.
extern esphome::template_::TemplateSwitch
    *const fan_direction; ///< Hardware fan direction switch.
extern esphome::globals::RestoringGlobalsComponent<int>
    *const auto_presence_val; ///< Presence bitmask.
extern esphome::globals::GlobalsComponent<bool>
    *const ui_active; ///< Flag if UI should be active / bright.
extern esphome::globals::RestoringGlobalsComponent<float>
    *const max_led_brightness; ///< Max brightness for LEDs.
extern esphome::globals::GlobalsComponent<bool>
    *const intensity_bounce_up; ///< True if we are cycling UP, false if DOWN.
extern esphome::globals::GlobalsComponent<bool>
    *const thermal_warning_active; ///< True if BMP390 detects >50°C.
extern esphome::globals::RestoringGlobalsComponent<bool>
    *const child_lock_active; ///< Child protection mode (Kindersicherung).
extern esphome::template_::TemplateSwitch
    *const child_lock_switch; ///< HA switch for child protection mode.

/// @name Scripts
/// @{
extern esphome::script::RestartScript<>
    *const update_leds; ///< Refreshes all status LEDs.
extern esphome::script::RestartScript<>
    *const ui_timeout_script; ///< UI 30s timeout script.
extern esphome::script::RestartScript<>
    *const fan_speed_update; ///< Re-applies fan speed.
extern esphome::script::SingleScript<float, int>
    *const set_fan_speed_and_direction;               ///< Sets PWM + direction.
extern esphome::pid::PIDClimate *const pid_co2;       ///< CO2 PID controller.
extern esphome::pid::PIDClimate *const pid_humidity;  ///< Humidity PID controller.
extern esphome::sntp::SNTPComponent *const sntp_time; ///< SNTP Time component.
extern esphome::script::RestartScript<>
    *const flash_leds_child_lock_3x; ///< 3x LED flash for child lock reject.
extern esphome::script::RestartScript<>
    *const flash_leds_child_lock_2x; ///< 2x LED flash for child lock toggle ack.
/// @}

/// @name Fan hardware
/// @{
extern esphome::speed::SpeedFan *const lueftung_fan; ///< Main fan (speed platform).
extern esphome::ledc::LEDCOutput
    *const fan_pwm_primary; ///< Primary PWM output (GPIO19).
/// @}

/// @name Sensors
/// @{
#ifdef VENTOSYNC_NO_SCD41
extern esphome::template_::TemplateSensor *const scd41_co2; ///< SCD41 CO2 sensor.
extern esphome::template_::TemplateSensor *const scd41_humidity; ///< Indoor Humidity
#else
extern esphome::sensor::Sensor *const scd41_co2; ///< SCD41 CO2 sensor.
extern esphome::sensor::Sensor *const scd41_humidity; ///< Indoor Humidity
#endif

#ifdef VENTOSYNC_NO_RADAR
extern esphome::template_::TemplateBinarySensor *const radar_presence; ///< Presence Sensor (Radar)
#else
extern esphome::binary_sensor::BinarySensor *const radar_presence; ///< Presence Sensor (Radar)
#endif

extern esphome::template_::TemplateSensor
    *const effective_co2; ///< Unified CO2 sensor (SCD41 or BME680 fallback).
extern esphome::sensor::Sensor *const temperature; ///< Room Temperature (SCD41)
extern esphome::ntc::NTC *const temp_zuluft; ///< Supply Air Temperature (NTC Inside)
extern esphome::ntc::NTC
    *const temp_abluft; ///< Exhaust Air Temperature (NTC Outside)
extern esphome::homeassistant::HomeassistantSensor
    *const outdoor_humidity; ///< Outdoor Humidity (HA)
extern esphome::homeassistant::HomeassistantBinarySensor
    *const sommerbetrieb; ///< Summer mode gate (HA: season + outdoor temp > 18°C)
extern esphome::homeassistant::HomeassistantBinarySensor
    *const window_locked; ///< Window lock gate (HA: open windows in room)
/// @}

/// @name Status LEDs (monochromatic light components)
/// @{
extern esphome::light::LightState *const status_led_l1;    ///< Level indicator LED 1.
extern esphome::light::LightState *const status_led_l2;    ///< Level indicator LED 2.
extern esphome::light::LightState *const status_led_l3;    ///< Level indicator LED 3.
extern esphome::light::LightState *const status_led_l4;    ///< Level indicator LED 4.
extern esphome::light::LightState *const status_led_l5;    ///< Level indicator LED 5.
extern esphome::light::LightState *const status_led_power; ///< Power status LED.
extern esphome::light::LightState
    *const status_led_master; ///< Master status LED (error indicator).
extern esphome::light::LightState
    *const status_led_mode_wrg; ///< Mode LED: Wärmerückgewinnung.
extern esphome::light::LightState
    *const status_led_mode_vent; ///< Mode LED: Ventilation.
/// @}

/**
 * @brief   Global pointer to the central Ventilation Controller.
 *
 * @details This is the primary bridge between the helper functions and the
 *          custom component logic.
 */
extern esphome::VentilationController *const ventilation_ctrl;

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
    *const espnow_peers; ///< ESP-NOW dynamic peer list (JSON-formatted MACs).

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

/// @name Child Protection Mode (Kindersicherung) State
/// @{
/// Timestamp of last combo trigger (Mode+Level held 5s) to suppress stale on_click events.
inline uint32_t child_lock_combo_triggered_ms = 0;
/// @}

// --- FORWARD DECLARATIONS ---
// --- FORWARD DECLARATIONS ---
/** @brief Resets thermal stabilization timers after a direction flip. */
inline void notify_fan_direction_changed();
/** @brief Filter for NTC stabilization. Returns NaN if not stable. */
inline esphome::optional<float> filter_ntc_stable(int sensor_idx, float new_value);
/** @brief Checks if a MAC matches this device. */
inline bool is_local_mac(const uint8_t *mac);
/** @brief Registers a new peer in the dynamic discovery list. */
inline void register_peer_dynamic(const uint8_t *mac);
/** @brief Loads saved peers from NVS into runtime cache. */
inline void load_peers_from_runtime_cache();
/** @brief Broadcasts a discovery packet to find other units. */
inline void send_discovery_broadcast();
/** @brief Requests status from a specific peer. */
inline void request_peer_status();
/** @brief Acknowledges a discovery request from a peer. */
inline void send_discovery_confirmation(const uint8_t *target_mac);
/** @brief Sends a packet to all registered peers. */
inline void send_sync_to_all_peers(const std::vector<uint8_t> &data);
/** @brief Broadcasts current settings to all peers. */
inline void sync_settings_to_peers();
/** @brief High-level ESP-NOW receive entry point. */
inline void handle_espnow_receive(const std::vector<uint8_t> &data, const uint8_t *src_mac);
/** @brief Drains the thread-safe RX queue. */
inline void process_queued_packets();
/** @brief Removes a peer from the runtime and persistent list. */
inline void remove_stale_peer(const uint8_t *mac);
/** @brief Triggers a new discovery phase. */
inline void trigger_re_discovery();
/** @brief Resets the failure counter for a specific peer. */
inline void reset_peer_fail_count(const uint8_t *mac);
/** @brief Rebuilds the JSON string for peer display. */
inline void rebuild_peers_string();
/** @brief Sets the ventilation timer from a UI value. */
inline void set_ventilation_timer(float value);
/** @brief Sets the sync interval from a UI value. */
inline void set_sync_interval_handler(float value);
/** @brief Sets the fan intensity from a UI slider. */
inline void set_fan_intensity_slider(float value);
/** @brief Sets the operating mode from a UI select. */
inline void set_operating_mode_select(const std::string &x);
/** @brief Cycles mode on button click. */
inline void handle_button_mode_click();
/** @brief Toggles power on button short-click. */
inline void handle_button_power_short_click();
/** @brief Shuts down system on button long-click. */
inline void handle_button_power_long_click();
/** @brief Cycles intensity on button click. */
inline void handle_button_level_click();
/** @brief Cycles intensity on button hold. */
inline void handle_intensity_bounce();
/** @brief Evaluates Smart-Automatik logic. */
inline void evaluate_auto_mode(bool force = false);
/** @brief Updates filter wear-out analytics. */
inline void update_filter_analytics();
/** @brief Orchestrates a mode transition. */
inline void cycle_operating_mode(int mode_index);
/** @brief Syncs YAML config to C++ controller. */
inline void sync_config_to_controller();
/** @brief Main system boot sequence. */
inline void run_system_boot_initialization();
/** @brief Hardware LED self-test. */
inline void run_led_self_test();
/** @brief Periodic LED update logic. */
inline void update_leds_logic();
/** @brief Checks for system errors via Master LED. */
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
/** @brief Dispatches PWM and direction to hardware. */
inline void set_fan_logic(float speed, int direction);
/** @brief Converts intensity level to speed fraction. */
inline float level_to_speed(float level);
/** @brief Calculates the target speed based on mode. */
inline float get_current_target_speed();
/** @brief Returns RPM (physical or virtual fallback). */
inline float calculate_virtual_fan_rpm(float raw_rpm);
/** @brief Main fan control loop tick. */
inline void update_fan_logic();
/** @brief Checks if the fan is logically OFF. */
inline bool is_fan_slider_off(float value);
/** @brief Returns ramp-up multiplier (0.0 to 1.0). */
inline float calculate_ramp_up(int iteration);
/** @brief Returns ramp-down multiplier (1.0 to 0.0). */
inline float calculate_ramp_down(int iteration);
