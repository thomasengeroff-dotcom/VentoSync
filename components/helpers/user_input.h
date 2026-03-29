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
// File:        user_input.h
// Description: Processing of physical button and slider interactions.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "helpers/globals.h"

inline void set_ventilation_timer(float value) {
  if (ventilation_ctrl == nullptr) return;
  auto *v = ventilation_ctrl;

  // Prevent overflow
  if (value > static_cast<float>(UINT32_MAX / (60 * 1000))) {
    ESP_LOGW("input", "Timer value too large: %f", value);
    return;
  }

  uint32_t ms = static_cast<uint32_t>(value * 60 * 1000);
  if (v->state_machine.ventilation_duration_ms == ms)
    return;

  // Update the duration but keep current mode
  v->state_machine.ventilation_duration_ms = ms;
  v->trigger_sync();
}

/// @brief Converts a minutes value from the UI slider to ms and updates sync
/// interval.
inline void set_sync_interval_handler(float value) {
  if (ventilation_ctrl == nullptr) return;
  auto *v = ventilation_ctrl;

  // Prevent overflow
  if (value > static_cast<float>(UINT32_MAX / (60 * 1000))) {
    ESP_LOGW("input", "Sync interval value too large: %f", value);
    return;
  }

  v->set_sync_interval(static_cast<uint32_t>(value * 60 * 1000));
}

/// @brief Handles the fan intensity slider: updates global, notifies
/// controller, refreshes LEDs.
inline void set_fan_intensity_slider(float value) {
  if (fan_intensity_level == nullptr || ventilation_ctrl == nullptr ||
      fan_speed_update == nullptr || ui_active == nullptr || 
      update_leds == nullptr || ui_timeout_script == nullptr) return;

  int val = static_cast<int>(value);
  if (val < 1 || val > 10) {
    ESP_LOGW("input", "Invalid fan intensity: %d", val);
    return;
  }

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
  if (current_mode_index == nullptr || ui_active == nullptr || 
      update_leds == nullptr || ui_timeout_script == nullptr) return;

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
  if (current_mode_index == nullptr || ui_active == nullptr || 
      update_leds == nullptr || ui_timeout_script == nullptr) return;

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
  if (ventilation_enabled == nullptr || current_mode_index == nullptr || 
      fan_speed_update == nullptr || ui_timeout_script == nullptr) return;

  if (!ventilation_enabled->value()) {
    ventilation_enabled->value() = true;
    ESP_LOGI("power", "System turned ON by short press - restoring mode %d",
             current_mode_index->value());
    cycle_operating_mode(current_mode_index->value());
    fan_speed_update->execute();
    ui_timeout_script->execute();
  } else {
    ESP_LOGI("power", "System turned OFF by short press");
    cycle_operating_mode(4); // Mode index 4 is "Aus", triggers system_sleep via cycle_operating_mode
    ui_timeout_script->execute();
  }
}

/// @brief Power button long press (>=5s): turns ventilation_enabled OFF.
/// Stops fan and PWM outputs.
inline void handle_button_power_long_click() {
  if (ventilation_enabled == nullptr || system_on == nullptr || 
      ventilation_ctrl == nullptr || lueftung_fan == nullptr || 
      fan_pwm_primary == nullptr || ui_timeout_script == nullptr) return;

  if (ventilation_enabled->value()) {
    system_on->value() = false;
    ventilation_enabled->value() = false;
    ESP_LOGI("power", "System turned OFF by long press (>5s)");
    auto *v = ventilation_ctrl;
    v->set_mode(esphome::MODE_OFF);
    lueftung_fan->turn_off().perform(); // perform() avoids null derefs inside fan components
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
  if (ventilation_enabled == nullptr || fan_intensity_level == nullptr || 
      fan_intensity_display == nullptr || ventilation_ctrl == nullptr || 
      fan_speed_update == nullptr || ui_active == nullptr || 
      update_leds == nullptr || ui_timeout_script == nullptr) return;

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
  if (ventilation_enabled == nullptr || fan_intensity_level == nullptr || 
      intensity_bounce_up == nullptr || fan_intensity_display == nullptr || 
      ventilation_ctrl == nullptr || fan_speed_update == nullptr || 
      ui_active == nullptr || update_leds == nullptr || 
      ui_timeout_script == nullptr) return;

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
  if (ventilation_ctrl == nullptr) return std::vector<uint8_t>();
  auto *v = ventilation_ctrl;
  
  // Get the base vector containing the core logic payload
  std::vector<uint8_t> data = v->build_packet(type);

  // Cast it to our known struct
  esphome::VentilationPacket *pkt = (esphome::VentilationPacket *)data.data();

  // Populate all HA configurations with null checks
  pkt->current_mode_index = current_mode_index != nullptr ? current_mode_index->value() : 0;
  pkt->co2_auto_enabled = co2_auto_enabled != nullptr ? co2_auto_enabled->value() : true;
  pkt->automatik_min_fan_level = automatik_min_fan_level != nullptr ? (uint8_t)automatik_min_fan_level->value() : 2;
  pkt->automatik_max_fan_level = automatik_max_fan_level != nullptr ? (uint8_t)automatik_max_fan_level->value() : 7;
  pkt->auto_co2_threshold_val = auto_co2_threshold_val != nullptr ? (uint16_t)auto_co2_threshold_val->value() : 1000;
  pkt->auto_humidity_threshold_val = auto_humidity_threshold_val != nullptr ? (uint8_t)auto_humidity_threshold_val->value() : 60;
  pkt->auto_presence_val = auto_presence_val != nullptr ? (int8_t)auto_presence_val->value() : 0;

  // Timers
  pkt->sync_interval_min = (uint16_t)(v->sync_interval_ms / 60 / 1000);
  pkt->vent_timer_min = (uint16_t)(v->state_machine.ventilation_duration_ms / 60 / 1000);

  // UI Settings
  pkt->max_led_brightness = max_led_brightness != nullptr ? max_led_brightness->value() : 0.8f;

  return data;
}

// --- Boot-time Initialization Helpers ----------------------------------

/// @brief Synchronizes persistent configuration values from HA numbers to the
/// C++ controller. Called during on_boot to ensure the controller has the
/// correct IDs before any ESP-NOW traffic.
