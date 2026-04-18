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
// File:        system_lifecycle.h
// Description: System-wide mode orchestration and lifecycle hooks.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include <esp_system.h>
#include "globals.h"

/**
 * @brief   Tracks operating hours for the ventilation filter.
 *
 * @details Increments the counter whenever the system is powered and enabled.
 *          This data is used to trigger maintenance alarms in Home Assistant.
 */
inline void update_filter_analytics() {
  if (system_on == nullptr || ventilation_enabled == nullptr || filter_operating_hours == nullptr) return;

  static uint32_t last_update_ms = 0;
  const uint32_t now_ms = millis();

  if (last_update_ms == 0) {
    last_update_ms = now_ms;
    return;
  }

  if (system_on->value() && ventilation_enabled->value()) {
    const uint32_t elapsed_ms = now_ms - last_update_ms;
    // FIXED H-1: Akkumuliere in uint32_t um Float-Präzisionsverlust zu vermeiden
    static uint32_t filter_ms_accumulator = 0;
    filter_ms_accumulator += elapsed_ms;

    if (filter_ms_accumulator >= 60000) {
      filter_operating_hours->value() += static_cast<float>(filter_ms_accumulator) / 3600000.0f;
      filter_ms_accumulator = 0;
    }
  }
  last_update_ms = now_ms;

  if (sntp_time == nullptr || filter_last_change_ts == nullptr) return;
  auto now = sntp_time->now();
  if (filter_last_change_ts->value() == 0 && now.is_valid()) {
    filter_last_change_ts->value() = now.timestamp;
    ESP_LOGI("filter", "Initial filter timestamp set: %d", filter_last_change_ts->value());
  }
}

/**
 * @brief   Central orchestrator for mode transitions.
 *
 * @details This function is the "brain" for all mode changes, whether from
 *          buttons or the UI. It handles the specific requirements of each
 *          mode (e.g., auto-timers for Ventilation, motor-stop for Off).
 *
 * @param[in] mode_index  Target mode (0: Auto, 1: WRG, 2: Vent, 3: Boost, 4: Off).
 *
 * @note    In the "Aus" (Off) mode, the system remains active to stay reachable
 *          via WiFi, but the motor is commanded to 50% PWM (neutral stop).
 */
inline void cycle_operating_mode(int mode_index) {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return;

  // FIXED K-3: Bounds-Check für mode_index
  if (mode_index < 0 || mode_index > 4) {
      ESP_LOGE("mode", "Invalid mode_index %d — defaulting to WRG (1). Possible NVS corruption.", mode_index);
      mode_index = 1;
  }

  if (auto_mode_active != nullptr) {
    auto_mode_active->value() = false; // Reset by default
  }

  // Mode names for HA select sync (must match luefter_modus options exactly)
  // FIXED: "Smart-Automatik" must match ui_controls.yaml select options EXACTLY.
  // Previously "Automatik" caused ESPHome to reject the publish_state() call
  // and revert the select to the last valid option ("Wärmerückgewinnung").
  static constexpr const char *mode_names[] = {"Smart-Automatik", "Wärmerückgewinnung",
                                               "Durchlüften", "Stoßlüftung", "Aus"};
  static_assert(sizeof(mode_names) / sizeof(mode_names[0]) == 5, "Mode names array size mismatch");

  switch (mode_index) {
  case 0: // Automatik
    if (system_on != nullptr) system_on->value() = true;
    if (ventilation_enabled != nullptr) ventilation_enabled->value() = true;
    if (auto_mode_active != nullptr) auto_mode_active->value() = true;
    
    // Proper ESPHome state management interaction
    v->set_mode(esphome::MODE_ECO_RECOVERY);
    evaluate_auto_mode(true); // Force immediate PID evaluation
    break;

  case 1: // Wärmerückgewinnung (manual)
    if (system_on != nullptr) system_on->value() = true;
    if (ventilation_enabled != nullptr) ventilation_enabled->value() = true;
    
    v->set_mode(esphome::MODE_ECO_RECOVERY);
    break;

  case 2: // Durchlüften — timer from vent_timer number component
    if (system_on != nullptr) system_on->value() = true;
    if (ventilation_enabled != nullptr) ventilation_enabled->value() = true;
    
    if (vent_timer != nullptr) {
      // FIXED K-1: Clamp vor Cast und Grenzen definiert 
      constexpr float MAX_VENT_MIN = 1440.0f;
      constexpr float MIN_VENT_MIN = 1.0f;
      
      const float timer_min = std::clamp(vent_timer->state, MIN_VENT_MIN, MAX_VENT_MIN);
      if (vent_timer->state != timer_min) {
          ESP_LOGW("mode", "vent_timer clamped: %.1f -> %.1f min", vent_timer->state, timer_min);
      }
      
      const uint32_t duration_ms = static_cast<uint32_t>(timer_min * 60.0f * 1000.0f);
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
    // Mod: Stay "ON" but ventilation is disabled to keep motor stopped at 50%.
    // This keeps WLAN/API active.
    if (system_on != nullptr) system_on->value() = true;
    if (ventilation_enabled != nullptr) ventilation_enabled->value() = false;
    
    v->set_mode(esphome::MODE_OFF);
    
    // FIXED K-2: UI: Visible fan status OFF, keeping ESPHome internal state clean
    if (lueftung_fan != nullptr) {
        auto call = lueftung_fan->make_call();
        call.set_state(false);
        lueftung_fan->publish_state();  // State synchronisieren
    }
    // Hardware: VarioPro motor controller requires 50% PWM as "stop" signal.
    // Override after call just in case lueftung_fan sent 0.0f via PWM handler.
    if (fan_pwm_primary != nullptr) fan_pwm_primary->set_level(0.5f);
    break;
  }

  // Update state trackers for networking and UI persistence
  if (current_mode_index != nullptr) {
    current_mode_index->value() = mode_index;
  }

  // Sync HA select dropdown
  if (mode_index >= 0 && mode_index < 5) {
    std::string mode_str = mode_names[mode_index];
    if (luefter_modus != nullptr && std::string(luefter_modus->current_option()) != mode_str) {
      luefter_modus->publish_state(mode_str);
    }
  }

  if (mode_index != 4) {
    if (fan_speed_update != nullptr) fan_speed_update->execute();
  }

  ESP_LOGI("mode", "Mode changed to index %d", mode_index);
}


/**
 * @brief   Syncs persistent configuration from HA entities to the C++ core.
 *
 * @details Ensures that the VentilationController has the correct Floor, Room,
 *          and Device IDs before starting any synchronization. This prevents
 *          network collisions during the early boot phase.
 */
inline void sync_config_to_controller() {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return;

  // FIXED H-2: Expliziter Bereichscheck vor uint8_t Fallback Cast
  auto safe_to_uint8 = [](float val, const char* name) -> uint8_t {
      if (val < 0.0f || val > 255.0f) {
          ESP_LOGW("boot", "%s out of uint8 range: %.1f — using 0", name, val);
          return 0;
      }
      return static_cast<uint8_t>(val);
  };

  uint8_t floor = (config_floor_id != nullptr && !std::isnan(config_floor_id->state)) ? safe_to_uint8(config_floor_id->state, "floor_id") : 0;
  uint8_t room = (config_room_id != nullptr && !std::isnan(config_room_id->state)) ? safe_to_uint8(config_room_id->state, "room_id") : 0;
  uint8_t dev = (config_device_id != nullptr && !std::isnan(config_device_id->state)) ? safe_to_uint8(config_device_id->state, "device_id") : 0;

  // Abort if NVS has not yet restored valid non-zero IDs
  if (floor == 0 || room == 0 || dev == 0) {
    ESP_LOGW("boot", "Config IDs not yet restored (F:%d R:%d D:%d) — skipping controller sync.", 
             floor, room, dev);
    return;
  }

  v->set_floor_id(floor);
  v->set_room_id(room);
  v->set_device_id(dev);

  if (config_phase != nullptr) {
    bool is_phase_a = (std::string(config_phase->current_option()) == "Phase A (Startet mit Zuluft)");
    v->set_is_phase_a(is_phase_a);
  }

  ESP_LOGI("boot", "Status Sync: Floor %d, Room %d, Device %d", 
           v->floor_id, v->room_id, v->device_id);
}

/**
 * @brief   Main system initialization after NVS and WiFi are ready.
 * 
 * @details Detects watchdog resets for stability tracking, initializes
 *          the C++ controller from persistent HA state, and boots
 *          the PID controllers for automated modes.
 *
 * @note    Moved from YAML to C++ for better maintainability.
 */
inline void run_system_boot_initialization() {
  // 1. Watchdog Reset Detection
  esp_reset_reason_t reason = esp_reset_reason();
  bool is_critical_reset = false;
  const char* reset_reason_str = "Unknown";
  
  switch (reason) {
      case ESP_RST_WDT:       reset_reason_str = "WDT"; is_critical_reset = true; break;
      case ESP_RST_TASK_WDT:  reset_reason_str = "TASK_WDT"; is_critical_reset = true; break;
      case ESP_RST_INT_WDT:   reset_reason_str = "INT_WDT (ISR blocked!)"; is_critical_reset = true; break;
      case ESP_RST_PANIC:     reset_reason_str = "PANIC (crash/stack overflow)"; is_critical_reset = true; break;
      case ESP_RST_BROWNOUT:  reset_reason_str = "BROWNOUT (power issue!)"; is_critical_reset = true; break;
      case ESP_RST_SW:        reset_reason_str = "Software reset (OTA/reboot cmd)"; break;
      case ESP_RST_POWERON:   reset_reason_str = "Power-on"; break;
      default:                break;
  }
  
  ESP_LOGI("boot", "Reset reason: %s", reset_reason_str);
  
  if (is_critical_reset && watchdog_restarts_count != nullptr) {
      watchdog_restarts_count->value()++;
      if (watchdog_restarts != nullptr) {
          watchdog_restarts->publish_state(watchdog_restarts_count->value());
      }
      ESP_LOGW("system", "Critical reset #%d: %s", 
               (int)watchdog_restarts_count->value(), reset_reason_str);
  }

  // 2. Controller & Mode Init
  sync_config_to_controller();
  if (current_mode_index != nullptr) {
    int saved_mode = current_mode_index->value();
    if (saved_mode < 0 || saved_mode > 4) {
        ESP_LOGW("boot", "Saved mode %d invalid — resetting to WRG", saved_mode);
        saved_mode = 1;
        current_mode_index->value() = saved_mode;
    }
    cycle_operating_mode(saved_mode);
  }

  // 3. PID Controllers
  if (pid_co2 != nullptr) {
    auto call = pid_co2->make_call();
    call.set_mode(esphome::climate::CLIMATE_MODE_COOL);
    if (auto_co2_threshold_val != nullptr) {
      call.set_target_temperature(auto_co2_threshold_val->value());
    }
    call.perform();
  }
  if (pid_humidity != nullptr) {
    auto call = pid_humidity->make_call();
    call.set_mode(esphome::climate::CLIMATE_MODE_COOL);
    if (auto_humidity_threshold_val != nullptr) {
      call.set_target_temperature(auto_humidity_threshold_val->value());
    }
    call.perform();
  }

  // 4. Load saved peers from NVS
  load_peers_from_runtime_cache();

  ESP_LOGI("boot", "Phase 1 complete: Controller, PID, Peers loaded");
}

/**
 * @brief   Flashes all status LEDs for a bootstrap self-test.
 *
 * @details Provides visual confirmation that the PCA9685 LED driver is
 *          functioning and that all 8 LEDs are wired correctly.
 */
inline void run_led_self_test() {
  // FIXED H-4: Kritische LEDs: System kann ohne sie keinen sinnvollen Self-Test machen
  if (status_led_power == nullptr) {
      ESP_LOGW("boot", "Power LED not ready — self-test skipped");
      return;
  }

  auto safe_turn_on = [](esphome::light::LightState *led) {
      if (led != nullptr) led->turn_on().perform();
  };

  // 1. Turn on all mode and status LEDs
  if (status_led_mode_wrg) {
      auto call_wrg = status_led_mode_wrg->turn_on();
      call_wrg.set_effect("None");
      call_wrg.perform();
  }

  safe_turn_on(status_led_mode_vent);
  safe_turn_on(status_led_power);

  // 2. Turn on all level indicator LEDs
  safe_turn_on(status_led_l1);
  safe_turn_on(status_led_l2);
  safe_turn_on(status_led_l3);
  safe_turn_on(status_led_l4);
  safe_turn_on(status_led_l5);

  // 3. Turn on Master LED for test
  if (status_led_master) {
      auto call_master = status_led_master->turn_on();
      call_master.set_effect("None");
      call_master.set_brightness(1.0f);
      call_master.perform();
  }
  
  // Force state tracking to 1.0/None so the control logic realizes it's ON
  // and needs to be turned OFF if this is not a Master device.
  led_state::last_master_effect = "None";
  led_state::last_master_brightness = 1.0f;

  ESP_LOGI("boot", "LED self-test: all LEDs on");
}
