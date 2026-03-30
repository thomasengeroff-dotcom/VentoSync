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
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "helpers/globals.h"

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
    case 9:
      status_led_l4->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l5->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 8:
      status_led_l4->turn_on().set_brightness(1.0f * max_b).perform();
      // status_led_l5->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 7:
      status_led_l3->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l4->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 6:
      status_led_l3->turn_on().set_brightness(1.0f * max_b).perform();
      // status_led_l4->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 5:
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      status_led_l3->turn_on().set_brightness(0.5f * max_b).perform();
      break;
    case 4:
      status_led_l2->turn_on().set_brightness(1.0f * max_b).perform();
      // status_led_l3->turn_on().set_brightness(0.5f * max_b).perform();
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
    default: {
      // do nothing
    } break;
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
