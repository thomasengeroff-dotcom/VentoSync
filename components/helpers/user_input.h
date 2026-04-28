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
// File:        user_input.h
// Description: Processing of physical button and slider interactions.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "globals.h"

// =========================================================
// SECTION: UI Slider & Number Handlers
// =========================================================

/**
 * @brief   Updates the countdown timer for timed ventilation modes.
 *
 * @details Converts the input value from minutes to milliseconds and updates
 *          the state machine. To prevent system crashes, the value is clamped
 *          against UINT32 overflow.
 *
 * @param[in] value  Timer duration in decimal minutes (e.g., 30.0)
 *
 * @note    Triggers an immediate network sync to ensure all devices in the
 *          room group stop/start at the same time.
 */
inline void set_ventilation_timer(float value) {
  if (std::isnan(value) || ventilation_ctrl == nullptr) return;
  auto *v = ventilation_ctrl;

  // FIXED K-1: Clamp float to safe, physically sensible domain boundaries before cast
  constexpr float MIN_TIMER_MIN = 1.0f;
  constexpr float MAX_TIMER_MIN = 1440.0f; // 24 hours
  
  if (value < MIN_TIMER_MIN || value > MAX_TIMER_MIN) {
      ESP_LOGW("input", "Timer value out of range: %.1f min (valid: %.0f-%.0f)", 
               value, MIN_TIMER_MIN, MAX_TIMER_MIN);
      value = std::clamp(value, MIN_TIMER_MIN, MAX_TIMER_MIN);
  }

  const uint32_t ms = static_cast<uint32_t>(value) * 60u * 1000u;
  if (v->state_machine.ventilation_duration_ms == ms)
    return;

  // Update the duration but keep current mode
  v->state_machine.ventilation_duration_ms = ms;
  v->trigger_sync();
  sync_settings_to_peers(); // Emit MSG_STATE explicitly to force peers
}

/**
 * @brief   Sets the periodic state synchronization interval.
 *
 * @details Adjusts how often the device broadcasts its status (heartbeat).
 *          Increasing this value reduces RF traffic but may lead to
 *          longer latencies in the WRG dashboard status updates.
 *
 * @param[in] value  Interval duration in minutes.
 *
 * @see     sync_settings_to_peers()
 */
inline void set_sync_interval_handler(float value) {
  if (std::isnan(value) || ventilation_ctrl == nullptr) return;
  auto *v = ventilation_ctrl;

  // FIXED K-1: Clamp float to safe domain boundaries before cast
  constexpr float MIN_INTERVAL_MIN = 1.0f;
  constexpr float MAX_INTERVAL_MIN = 1440.0f; // 24 hours
  
  if (value < MIN_INTERVAL_MIN || value > MAX_INTERVAL_MIN) {
      ESP_LOGW("input", "Sync interval out of range: %.1f min (valid: %.0f-%.0f)", 
               value, MIN_INTERVAL_MIN, MAX_INTERVAL_MIN);
      value = std::clamp(value, MIN_INTERVAL_MIN, MAX_INTERVAL_MIN);
  }

  v->set_sync_interval(static_cast<uint32_t>(value) * 60u * 1000u);
  sync_settings_to_peers(); // Emit MSG_STATE explicitly to force peers
}

/**
 * @brief   Handles manual fan intensity adjustments from the Home Assistant slider.
 *
 * @details Processes input from 1 to 10. This handler is the bridge between
 *          the virtual UI sensor and the physical PWM control. It triggers
 *          LED updates and resets the UI timeout timer to keep the display bright.
 *
 * @param[in] value  Target intensity level (1.0 to 10.0)
 */
inline void set_fan_intensity_slider(float value) {
  if (std::isnan(value) || fan_intensity_level == nullptr || ventilation_ctrl == nullptr ||
      fan_speed_update == nullptr || ui_active == nullptr || 
      update_leds == nullptr || ui_timeout_script == nullptr) return;

  // FIXED H-1: Use std::round before cast to prevent integer truncation off-by-one errors
  int val = static_cast<int>(std::round(value));
  if (val < 1 || val > 10) {
    ESP_LOGW("input", "Invalid fan intensity after rounding: %d (raw: %.2f)", val, value);
    return;
  }

  fan_intensity_level->value() = val;
  ventilation_ctrl->set_fan_intensity(val);
  sync_settings_to_peers(); // Emit MSG_STATE explicitly to force peers
  fan_speed_update->execute();
  ui_active->value() = true;
  update_leds->execute();
  ui_timeout_script->execute();
}

/**
 * @brief   Maps Home Assistant mode selection strings to internal enum indices.
 *
 * @details This unified entry point ensures that mode changes from the cloud/app
 *          are treated identical to physical button presses, maintaining
 *          synchronicity across the room group.
 *
 * @param[in] x  Selected mode string (e.g., "Wärmerückgewinnung")
 *
 * @note    Unknown strings are logged as warnings and ignored to prevent
 *          invalid state machine transitions.
 */
inline void set_operating_mode_select(const std::string &x) {
  if (ventilation_ctrl == nullptr || current_mode_index == nullptr || ui_active == nullptr || 
      update_leds == nullptr || ui_timeout_script == nullptr) return;

  // FIXED M-1: Normalize HA strings to strip accidental whitespace
  auto trim = [](const std::string &s) -> std::string {
      size_t start = s.find_first_not_of(" \t\r\n");
      size_t end = s.find_last_not_of(" \t\r\n");
      return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
  };
  const std::string x_norm = trim(x);

  int mode_index = -1;
  if (x_norm == "Smart-Automatik") // FIXED: Must match YAML exactly
    mode_index = 0;
  else if (x_norm == "Wärmerückgewinnung")
    mode_index = 1;
  else if (x_norm == "Durchlüften")
    mode_index = 2;
  else if (x_norm == "Stoßlüftung")
    mode_index = 3;
  else if (x_norm == "Aus")
    mode_index = 4;
  else {
    ESP_LOGW("input", "Unknown operating mode string: '%s' — ignoring", x_norm.c_str());
    return; // Do NOT apply unknown mode
  }

  current_mode_index->value() = mode_index;
  cycle_operating_mode(mode_index);
  sync_settings_to_peers(); // Emit MSG_STATE explicitly to force peers
  ui_active->value() = true;
  update_leds->execute();
  ui_timeout_script->execute();
}

// =========================================================
// SECTION: Physical Button Callbacks
// =========================================================

/**
 * @brief   Cycles through operating modes on a physical button click.
 *
 * @details Increments the mode index (0->1->2->3->4->0).
 *          0: Auto, 1: WRG, 2: Vent, 3: Boost, 4: Off.
 *          The UI is activated immediately to provide visual feedback via LEDs.
 */
inline void handle_button_mode_click() {
  if (ventilation_ctrl == nullptr || current_mode_index == nullptr || ui_active == nullptr || 
      update_leds == nullptr || ui_timeout_script == nullptr) return;

  // Child Protection Mode: Block physical button input when locked.
  // Suppress stale on_click events that fire after a combo toggle (500ms cooldown).
  if (child_lock_active != nullptr && child_lock_active->value()) {
    if (millis() - child_lock_combo_triggered_ms > 500) {
      if (flash_leds_child_lock_3x != nullptr) flash_leds_child_lock_3x->execute();
      ESP_LOGI("button", "Mode button BLOCKED by child lock");
    }
    return;
  }
  if (millis() - child_lock_combo_triggered_ms < 500) return; // Combo cooldown

  // FIXED H-2: Clamp modulo operand to guard against NVS value corruption
  int current = static_cast<int>(current_mode_index->value());
  current = std::clamp(current, 0, 4);
  current_mode_index->value() = (current + 1) % 5;
  cycle_operating_mode(current_mode_index->value());
  sync_settings_to_peers(); // Emit MSG_STATE explicitly to force peers
  ui_active->value() = true;    // Activate UI immediately so LEDs render
  update_leds->execute();       // Show new mode LEDs now
  ui_timeout_script->execute(); // Start 30s auto-dim timer
  ESP_LOGI("button", "Mode button pressed, new index: %d",
           current_mode_index->value());
}

/**
 * @brief   Handles short-press logic for the power button.
 *
 * @details Toggles the system state. If turning ON, it restores the last known
 *          mode but waits for a network sync before broadcasting its own state
 *          to avoid "State Whiplash" (where a booting node overwrites a running group).
 *
 * @see     cycle_operating_mode()
 */
inline void handle_button_power_short_click() {
  if (ventilation_ctrl == nullptr || ventilation_enabled == nullptr || current_mode_index == nullptr || 
      fan_speed_update == nullptr || ui_timeout_script == nullptr) return;

  // Child Protection Mode: Block physical button input when locked.
  if (child_lock_active != nullptr && child_lock_active->value()) {
    if (flash_leds_child_lock_3x != nullptr) flash_leds_child_lock_3x->execute();
    ESP_LOGI("button", "Power button BLOCKED by child lock");
    return;
  }

  if (!ventilation_enabled->value()) {
    ventilation_enabled->value() = true;
    ESP_LOGI("power", "System turned ON by short press - restoring mode %d locally, awaiting network sync",
             current_mode_index->value());
    cycle_operating_mode(current_mode_index->value());
    
    // Mute the outgoing state broadcast to prevent overwriting the group with our slept state.
    // Instead we arm the sync flag to adopt the Master's mode when WiFi connects.
    if (ventilation_ctrl != nullptr) {
      ventilation_ctrl->pending_broadcast = false;
      ventilation_ctrl->is_state_synced = false;
      // FIXED K-2: Setup a relative start timer instead of an absolute timestamp logic
      ventilation_ctrl->sync_timeout_ms = millis(); // Bootstart / Start-Zeitpunkt for timeout comparison
    }
    
    fan_speed_update->execute();
    ui_timeout_script->execute();
  } else {
    ESP_LOGI("power", "System turned OFF by short press");
    cycle_operating_mode(4); // Mode index 4 is "Aus", triggers system_sleep via cycle_operating_mode
    sync_settings_to_peers(); // Emit MSG_STATE explicitly to force peers
    ui_timeout_script->execute();
  }
}

/**
 * @brief   Enforces a full system-stop via long-press (>=5s).
 *
 * @details This is a safety/maintenance override. It halts the fan using 50% PWM
 *          (neutral for bidirectional drivers) and puts the device into
 *          low-power sleep.
 *
 * @warning Bypasses standard transition ramps for immediate halt.
 */
inline void handle_button_power_long_click() {
  // FIXED H-4: Removed lueftung_fan from the guard since long press must work even if fan is undefined
  if (ventilation_enabled == nullptr || system_on == nullptr || 
      ventilation_ctrl == nullptr || 
      fan_pwm_primary == nullptr || ui_timeout_script == nullptr) return;

  // Child Protection Mode: Block physical button input when locked.
  if (child_lock_active != nullptr && child_lock_active->value()) {
    if (flash_leds_child_lock_3x != nullptr) flash_leds_child_lock_3x->execute();
    ESP_LOGI("button", "Power long press BLOCKED by child lock");
    return;
  }

  if (ventilation_enabled->value()) {
    system_on->value() = false;
    ventilation_enabled->value() = false;
    ESP_LOGI("power", "System turned OFF by long press (>5s)");
    auto *v = ventilation_ctrl;
    v->set_mode(esphome::MODE_OFF);
    sync_settings_to_peers(); // Emit MSG_STATE explicitly to force peers
    // 50% PWM = true motor stop (bidirectional driver).
    // lueftung_fan->turn_off() is intentionally omitted as it would send 0.0f (full reverse).
    fan_pwm_primary->set_level(0.5f);
    if (system_sleep != nullptr)
      system_sleep->execute();
    ui_timeout_script->execute();
  } else {
    ESP_LOGD("power", "System already OFF, ignoring long press");
  }
}

/**
 * @brief   Increments fan intensity via the physical "Level" button.
 *
 * @details Implements a 10s boot-guard to prevent spurious triggers while
 *          MCP23017 GPIOs are stabilizing.
 *
 * @code
 *   // Example: Level 3 -> click -> Level 4
 *   handle_button_level_click();
 * @endcode
 */
inline void handle_button_level_click() {
  if (ventilation_enabled == nullptr || fan_intensity_level == nullptr || 
      fan_intensity_display == nullptr || ventilation_ctrl == nullptr || 
      fan_speed_update == nullptr || ui_active == nullptr || 
      update_leds == nullptr || ui_timeout_script == nullptr) return;

  // MCP23017 GPIOs may float LOW during boot → inverted: true reads as pressed
  if (millis() < 10000) return;

  // Child Protection Mode: Block physical button input when locked.
  if (child_lock_active != nullptr && child_lock_active->value()) {
    if (millis() - child_lock_combo_triggered_ms > 500) {
      if (flash_leds_child_lock_3x != nullptr) flash_leds_child_lock_3x->execute();
      ESP_LOGI("button", "Level button BLOCKED by child lock");
    }
    return;
  }
  if (millis() - child_lock_combo_triggered_ms < 500) return; // Combo cooldown

  if (!ventilation_enabled->value())
    return; // Ignore if system is off
  int level = fan_intensity_level->value();

  // Delegate logic to Pure Class
  level = VentilationLogic::get_next_fan_level(level);

  fan_intensity_level->value() = level;
  fan_intensity_display->publish_state(level);

  ventilation_ctrl->set_fan_intensity(level);
  sync_settings_to_peers(); // Emit MSG_STATE explicitly to force peers
  fan_speed_update->execute();
  ui_active->value() = true;    // Activate UI immediately so LEDs render
  update_leds->execute();       // Show new intensity level LEDs now
  ui_timeout_script->execute(); // Start 30s auto-dim timer
  ESP_LOGI("button", "Intensity level: %d", level);
}

/**
 * @brief   Logic for the "Hold-to-Cycle" fan intensity feature.
 *
 * @details When the level button is held, this function is called repeatedly.
 *          It "bounces" the intensity between 1 and 10 (1->10->1) to allow
 *          fast adjustment without multiple clicks.
 *
 * @note    Includes a 10s boot-guard for GPIO stability.
 */
inline void handle_intensity_bounce() {
  if (ventilation_enabled == nullptr || fan_intensity_level == nullptr || 
      intensity_bounce_up == nullptr || fan_intensity_display == nullptr || 
      ventilation_ctrl == nullptr || fan_speed_update == nullptr || 
      ui_active == nullptr || update_leds == nullptr || 
      ui_timeout_script == nullptr) return;

  // MCP23017 GPIOs may float LOW during boot → inverted: true reads as pressed
  if (millis() < 10000) return;

  // Child Protection Mode: Block hold-to-cycle when locked (no flash — click handler flashes).
  if (child_lock_active != nullptr && child_lock_active->value()) return;

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

// =========================================================
// SECTION: Protocol & Data Helpers
// =========================================================

/**
 * @brief   Constructs and serializes a state packet for ESP-NOW.
 *
 * @details Fetches current runtime data, stamps it with the device ID,
 *          and prepares the binary payload.
 *
 * @param[in] type  The MessageType intent (STATE, SYNC, etc.)
 *
 * @return  std::vector<uint8_t>  Packed binary data ready for transmission.
 */
inline std::vector<uint8_t>
build_and_populate_packet(esphome::MessageType type) {
  if (ventilation_ctrl == nullptr) return std::vector<uint8_t>();
  
  // Now simply call the base method, as it handles all population autonomously
  return ventilation_ctrl->build_packet(type);
}

