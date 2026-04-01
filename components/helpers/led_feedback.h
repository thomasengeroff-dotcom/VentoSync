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
// File:        led_feedback.h
// Description: Visual system status feedback via physical LEDs.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-04-01
// ==========================================================================
#pragma once
#include "helpers/globals.h"

/**
 * @brief Checks for error conditions and blinks the Master LED accordingly.
 * Priority Ladder:
 * 1. Peer Sync Error (2 Pulses) - HIGHEST
 * 2. WiFi Disconnected (3 Pulses)
 * 3. Thermal Warning (4 Pulses)
 */
inline void check_master_led_error() {
  if (status_led_master == nullptr) return;

  // Apply or remove the blink effect statefully to prevent I2C bus flooding
  static std::string last_active_effect = "__none__";
  static float last_active_brightness = -1.0f;

  // Robust Connection Check (Network + API Fallback)
  // Use ESPHome native network abstraction + API state to prevent false positives.
  bool network_up = esphome::network::is_connected();
  bool api_up = (esphome::api::global_api_server != nullptr) && 
                esphome::api::global_api_server->is_connected();
  
  bool is_functionally_connected = network_up || api_up;
  
  // Implement 30s Hysteresis to suppress "phantom" errors during brief signal drops (roaming)
  static uint32_t last_connected_ms = millis();
  if (is_functionally_connected) {
    last_connected_ms = millis();
  }
  
  // LED Error only triggers if offline for more than 30 seconds
  bool wifi_error_triggered = (millis() - last_connected_ms > 30000);
  
  bool peer_sync_error = false;
  bool peer_check_active = (peer_check_enabled != nullptr) && peer_check_enabled->value();
  
  if (ventilation_ctrl != nullptr && peer_check_active) {
    // If we have peers expected but none seen for 3 minutes (Reduced from 5min)
    // We only trigger this after the system has been up for at least 3 minutes to allow for discovery.
    if (ventilation_ctrl->has_peer_pid_demand && 
        (millis() > 180000) &&
        (millis() - ventilation_ctrl->last_peer_pid_demand_time > 180000)) {
      peer_sync_error = true;
    }
  }

  bool thermal_warning = (thermal_warning_active != nullptr) && thermal_warning_active->value();

  std::string target_effect = "None";
  float target_brightness = 0.0f;

  if (peer_sync_error) {
    target_effect = "Error Peer";
    target_brightness = 1.0f;
  } else if (wifi_error_triggered) {
    target_effect = "Error WiFi";
    target_brightness = 1.0f;
  } else if (thermal_warning) {
    target_effect = "Warning Safety";
    target_brightness = 1.0f;
  }

  if (target_effect != last_active_effect || std::abs(target_brightness - last_active_brightness) > 0.1f) {
    auto call = status_led_master->make_call();
    call.set_effect(target_effect);
    if (target_brightness > 0) {
      call.set_state(true);
      call.set_brightness(target_brightness);
    } else {
      call.set_state(false);
    }
    call.perform();
    
    last_active_effect = target_effect;
    last_active_brightness = target_brightness;
  }
}

/// @brief Updates the physical LEDs based on current system state.
/// This function is stateful to prevent redundant I2C commands and log spamming.
inline void update_leds_logic() {
  // Always evaluate master LED errors first and independently
  // (It brings its own state-guard inside the function)
  check_master_led_error();

  auto *v = ventilation_ctrl;
  if (v == nullptr) return;

  const int current_mode = v->state_machine.current_mode;
  const bool is_on = (system_on != nullptr) ? system_on->value() : true;
  const bool is_ui_active = (ui_active != nullptr) ? ui_active->value() : true;
  const float max_b = (max_led_brightness != nullptr) ? max_led_brightness->value() : 1.0f;
  const bool is_auto = (auto_mode_active != nullptr) ? auto_mode_active->value() : false;
  const int intensity = (fan_intensity_level != nullptr) ? (int)fan_intensity_level->value() : 1;

  // FIXED: State tracking to prevent log spam and I2C flooding
  // Initialize with values that ensure the first run always executes
  static bool last_system_on = !is_on;
  static bool last_ui_active = !is_ui_active;
  static int last_mode = -1;
  static int last_intensity = -1;
  static float last_max_b = -1.0f;
  static bool last_auto = !is_auto;

  bool changed = (is_on != last_system_on) || (is_ui_active != last_ui_active) || 
                 (current_mode != last_mode) || (intensity != last_intensity) || 
                 (std::abs(max_b - last_max_b) > 0.01f) || (is_auto != last_auto);

  if (!changed) {
    return;
  }

  // Store new state
  last_system_on = is_on;
  last_ui_active = is_ui_active;
  last_mode = current_mode;
  last_intensity = intensity;
  last_max_b = max_b;
  last_auto = is_auto;

  // 0. Case: System is OFF
  if (!is_on) {
    status_led_power->turn_off().perform();
    status_led_mode_wrg->turn_off().perform();
    status_led_mode_vent->turn_off().perform();
    status_led_l1->turn_off().perform();
    status_led_l2->turn_off().perform();
    status_led_l3->turn_off().perform();
    status_led_l4->turn_off().perform();
    status_led_l5->turn_off().perform();
    return;
  }

  // 1. Case: UI Inactive (Dimming/Night mode)
  if (!is_ui_active) {
    // Dim Power LED to 10% of max
    status_led_power->turn_on().set_brightness(0.1f * max_b).perform();

    // Turn off all others
    status_led_mode_wrg->turn_off().perform();
    status_led_mode_vent->turn_off().perform();
    status_led_l1->turn_off().perform();
    status_led_l2->turn_off().perform();
    status_led_l3->turn_off().perform();
    status_led_l4->turn_off().perform();
    status_led_l5->turn_off().perform();
    return;
  }

  // 2. Case: UI Active and System On (Normal operation)
  status_led_power->turn_on().set_brightness(max_b).perform();

  // Mode LEDs
  status_led_mode_wrg->turn_off().perform();
  status_led_mode_vent->turn_off().perform();

  if (current_mode != esphome::MODE_OFF) {
    if (is_auto) {
      // Smart-Automatik: Left LED pulses slowly
      auto call = status_led_mode_wrg->turn_on();
      call.set_effect("Automatik Pulse");
      call.perform();
    } else if (current_mode == esphome::MODE_ECO_RECOVERY) {
      // Manual WRG: Left LED solid on
      status_led_mode_wrg->turn_on().set_effect("None").set_brightness(max_b).perform();
    } else if (current_mode == esphome::MODE_STOSSLUEFTUNG) {
      // Stoßlüftung: Right LED on
      status_led_mode_vent->turn_on().set_brightness(max_b).perform();
    } else if (current_mode == esphome::MODE_VENTILATION) {
      // Durchlüften: Both LEDs on (solid)
      status_led_mode_wrg->turn_on().set_effect("None").set_brightness(max_b).perform();
      status_led_mode_vent->turn_on().set_brightness(max_b).perform();
    }
  }

  // 3. Level LEDs (1-10 mapped to 5 LEDs)
  status_led_l1->turn_off().perform();
  status_led_l2->turn_off().perform();
  status_led_l3->turn_off().perform();
  status_led_l4->turn_off().perform();
  status_led_l5->turn_off().perform();

  if (current_mode != esphome::MODE_OFF) {
    switch (intensity) {
    case 10:
      status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l3->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l4->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l5->turn_on().set_brightness(1.0f * max_b).perform();
      break;
    case 9:
      status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l3->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l4->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l5->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 8:
      status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l3->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l4->turn_on().set_brightness(1.0f * max_b).perform();
      break;
    case 7:
      status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l3->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l4->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 6:
      status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l3->turn_on().set_brightness(1.0f * max_b).perform();
      break;
    case 5:
      status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l3->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 4:
      status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      break;
    case 3:
      status_led_l1->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l2->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 2:
      status_led_l1->turn_on().set_brightness(0.5f * max_b).perform();
      status_led_l2->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 1:
      status_led_l1->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    default: break;
    }
  }
}
