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
// File:        system_lifecycle.h
// Description: System-wide mode orchestration and lifecycle hooks.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "helpers/globals.h"

inline void update_filter_analytics() {
  if (system_on == nullptr || ventilation_enabled == nullptr || filter_operating_hours == nullptr) return;

  if (system_on->value() && ventilation_enabled->value()) {
    filter_operating_hours->value() += (1.0f / 60.0f);
  }

  if (sntp_time == nullptr || filter_last_change_ts == nullptr) return;
  auto now = sntp_time->now();
  if (filter_last_change_ts->value() == 0 && now.is_valid()) {
    filter_last_change_ts->value() = now.timestamp;
    ESP_LOGI("filter", "Initial filter timestamp set: %d", filter_last_change_ts->value());
  }
}

/// @brief Updates fan speed and direction based on intensity and mode.
/// Reads fan_direction switch for direction, calculates speed from
/// intensity/PID, then calls set_fan_logic() which maps both into the single
/// VarioPro PWM signal.
inline void cycle_operating_mode(int mode_index) {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return;

  if (auto_mode_active != nullptr) {
    auto_mode_active->value() = false; // Reset by default
  }

  // Mode names for HA select sync (must match luefter_modus options exactly)
  static constexpr const char *mode_names[] = {"Automatik", "Wärmerückgewinnung",
                                               "Durchlüften", "Stoßlüftung", "Aus"};
  static_assert(sizeof(mode_names) / sizeof(mode_names[0]) == 5, "Mode names array size mismatch");

  switch (mode_index) {
  case 0: // Automatik
    if (system_on != nullptr) system_on->value() = true;
    if (ventilation_enabled != nullptr) ventilation_enabled->value() = true;
    if (auto_mode_active != nullptr) auto_mode_active->value() = true;
    
    // Proper ESPHome state management interaction
    v->set_mode(esphome::MODE_ECO_RECOVERY);
    v->pending_broadcast = true;
    evaluate_auto_mode(); // Immediate PID/sensor eval
    break;

  case 1: // Wärmerückgewinnung (manual)
    if (system_on != nullptr) system_on->value() = true;
    if (ventilation_enabled != nullptr) ventilation_enabled->value() = true;
    
    v->set_mode(esphome::MODE_ECO_RECOVERY);
    v->pending_broadcast = true;
    break;

  case 2: // Durchlüften — timer from vent_timer number component
    if (system_on != nullptr) system_on->value() = true;
    if (ventilation_enabled != nullptr) ventilation_enabled->value() = true;
    
    if (vent_timer != nullptr) {
      const uint32_t duration_ms = static_cast<uint32_t>(vent_timer->state * 60.0f * 1000.0f);
      v->set_mode(esphome::MODE_VENTILATION, duration_ms);
    } else {
      v->set_mode(esphome::MODE_VENTILATION);
    }
    break;

  case 3: // Stoßlüftung
    if (system_on != nullptr) system_on->value() = true;
    if (ventilation_enabled != nullptr) ventilation_enabled->value() = true;
    
    v->set_mode(esphome::MODE_STOSSLUEFTUNG);
    break;

  case 4: // Aus
    if (system_on != nullptr) system_on->value() = false;
    if (ventilation_enabled != nullptr) ventilation_enabled->value() = false;
    
    v->set_mode(esphome::MODE_OFF);
    if (lueftung_fan != nullptr) lueftung_fan->turn_off().perform();
    if (fan_pwm_primary != nullptr) fan_pwm_primary->set_level(0.5f); // Neutral stop for VarioPro
    break;
  }

  // Sync HA select dropdown
  if (mode_index >= 0 && mode_index < 5) {
    std::string mode_str = mode_names[mode_index];
    if (luefter_modus != nullptr && std::string(luefter_modus->current_option()) != mode_str) {
      luefter_modus->publish_state(mode_str);
    }
  }

  if (mode_index != 4) {
    if (system_wakeup != nullptr) system_wakeup->execute();
    if (fan_speed_update != nullptr) fan_speed_update->execute();
  } else {
    if (system_sleep != nullptr) system_sleep->execute();
  }

  ESP_LOGI("mode", "Mode changed to index %d", mode_index);
}


/// @brief Refreshes all status LEDs based on system_on, current mode, and fan
/// level. Maps fan intensity 1–10 onto the 5 level LEDs and toggles the two
/// mode LEDs. ALL LEDs (except Master error blink and Power LED) turn off when
/// ui_active is false. Power LED dims to 20% when ui_active is false.
inline void sync_config_to_controller() {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return;

  uint8_t floor = (config_floor_id != nullptr) ? (uint8_t)config_floor_id->state : 0;
  uint8_t room = (config_room_id != nullptr) ? (uint8_t)config_room_id->state : 0;
  uint8_t dev = (config_device_id != nullptr) ? (uint8_t)config_device_id->state : 0;

  // Guard: If values are still 0, the restore hasn't happened yet
  if (floor == 0 && room == 0 && dev == 0) {
    ESP_LOGW("boot", "Config IDs are all 0 — restore_value not yet loaded. Will retry on next interval.");
    return;
  }

  v->set_floor_id(floor);
  v->set_room_id(room);
  v->set_device_id(dev);

  if (config_phase != nullptr) {
    bool is_phase_a = (std::string(config_phase->current_option()) == "Phase A (Startet mit Zuluft)");
    v->set_is_phase_a(is_phase_a);

    ESP_LOGI("boot", "Synced Config to Controller: Floor %d, Room %d, Device %d, Phase: %s",
             v->floor_id, v->room_id, v->device_id, is_phase_a ? "A" : "B");
  }
}

/// @brief Runs a 3-second visual self-test by turning on all physical status LEDs.
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
  if (status_led_l1) status_led_l1->turn_on().perform();
  if (status_led_l2) status_led_l2->turn_on().perform();
  if (status_led_l3) status_led_l3->turn_on().perform();
  if (status_led_l4) status_led_l4->turn_on().perform();
  if (status_led_l5) status_led_l5->turn_on().perform();

  ESP_LOGI("boot", "LED self-test: all LEDs on");
}
