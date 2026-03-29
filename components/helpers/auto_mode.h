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
// File:        auto_mode.h
// Description: Logic for the Ventilation Auto Mode Controller.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "helpers/globals.h"

/**
 * @brief Logic for the Ventilation Auto Mode Controller.
 * Final refactored version addressing all safety and maintainability concerns.
 */

namespace auto_mode {

/** @brief Validates that all required components for auto mode are initialized and ready. */
inline bool is_system_ready() {
  if (auto_mode_active == nullptr || !auto_mode_active->value()) return false;
  if (system_on == nullptr || !system_on->value()) return false;
  if (ventilation_enabled == nullptr || !ventilation_enabled->value()) return false;
  
  return (ventilation_ctrl != nullptr && 
          automatik_min_fan_level != nullptr && 
          automatik_max_fan_level != nullptr &&
          fan_intensity_level != nullptr &&
          fan_intensity_display != nullptr &&
          fan_speed_update != nullptr &&
          update_leds != nullptr);
}

/** @brief Combines local and peer sensor data to determine effective room temperatures. */
inline void get_effective_temperatures(uint32_t now, float &eff_in, float &eff_out) {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return;

  float local_in = NAN;
  float local_out = NAN;

  // 1. Local Indoor Temp (SCD41 preferred)
  if (temperature != nullptr && !std::isnan(temperature->state)) {
    local_in = temperature->state;
  }

  // 2. Map NTC sensors based on current ventilation direction
  // Safe by-value return structure (no null pointer risk possible here in C++)
  const int internal_mode = v->state_machine.current_mode;
  const esphome::HardwareState hw_state = v->state_machine.get_target_state(now);
  const bool is_intake = hw_state.direction_in;

  if (internal_mode == esphome::MODE_VENTILATION) {
    if (is_intake) {
        if (temp_abluft != nullptr) local_out = temp_abluft->state;
    } else {
        if (std::isnan(local_in) && temp_zuluft != nullptr) local_in = temp_zuluft->state;
    }
  } else if (internal_mode == esphome::MODE_ECO_RECOVERY) {
    if (temp_abluft != nullptr) local_out = temp_abluft->state;
    if (std::isnan(local_in) && temp_zuluft != nullptr) local_in = temp_zuluft->state;
  }

  // Update controller state for networking
  v->local_t_in = local_in;
  v->local_t_out = local_out;

  // 3. Sensor Fusion (Peer fallback if local is missing)
  eff_in = local_in;
  eff_out = local_out;

  if (std::isnan(eff_in) && !std::isnan(v->last_peer_t_in) &&
      (now - v->last_peer_t_in_time < PEER_TIMEOUT_MS)) {
    eff_in = v->last_peer_t_in;
  }
  if (std::isnan(eff_out) && !std::isnan(v->last_peer_t_out) &&
      (now - v->last_peer_t_out_time < PEER_TIMEOUT_MS)) {
    eff_out = v->last_peer_t_out;
  }
}

/** @brief Evaluates whether to switch to summer cooling (Mode: Ventilation). */
inline esphome::VentilationMode determine_auto_operating_mode(float eff_in, float eff_out, esphome::VentilationMode current_mode) {
  if (std::isnan(eff_in) || std::isnan(eff_out)) {
    return (current_mode == esphome::MODE_VENTILATION || current_mode == esphome::MODE_ECO_RECOVERY) 
           ? current_mode : esphome::MODE_ECO_RECOVERY;
  }

  if (current_mode != esphome::MODE_VENTILATION) {
    // Check for activation threshold
    if (eff_in > SUMMER_COOLING_THRESHOLD_INDOOR && eff_out < (eff_in - SUMMER_COOLING_MIN_DELTA)) {
      ESP_LOGI("auto_mode", "Sommer-Kühlung AKTIVIERT: Innen=%.1f°C, Außen=%.1f°C", eff_in, eff_out);
      return esphome::MODE_VENTILATION;
    }
    return esphome::MODE_ECO_RECOVERY;
  } else {
    // Check for deactivation via hysteresis
    if (eff_out >= (eff_in - SUMMER_COOLING_HYSTERESIS) || eff_in < SUMMER_COOLING_THRESHOLD_INDOOR_HYSTERESIS) {
      ESP_LOGI("auto_mode", "Sommer-Kühlung DEAKTIVIERT (Hysterese): Innen=%.1f°C, Außen=%.1f°C", eff_in, eff_out);
      return esphome::MODE_ECO_RECOVERY;
    }
    return esphome::MODE_VENTILATION;
  }
}

/** @brief Aggregates PID demands from CO2 and Humidity, integrating peer data. */
inline float calculate_combined_demand(uint32_t now) {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return 0.0f;

  float max_demand = 0.0f;

  // 1. CO2 Demand
  if (co2_auto_enabled != nullptr && co2_auto_enabled->value() && co2_pid_result != nullptr) {
    max_demand = std::max(max_demand, co2_pid_result->value());
  }

  // 2. Humidity Demand (Outdoor Check)
  if (scd41_humidity != nullptr && outdoor_humidity != nullptr && humidity_pid_result != nullptr) {
    const float in_hum = scd41_humidity->state;
    const float out_hum = outdoor_humidity->state;
    if (!std::isnan(in_hum) && !std::isnan(out_hum) && out_hum < in_hum) {
      max_demand = std::max(max_demand, humidity_pid_result->value());
    }
  }

  // 3. Network Sync Trigger
  if (std::abs(max_demand - v->local_pid_demand) > PID_SYNC_THRESHOLD) {
    v->trigger_sync();
  }
  v->local_pid_demand = max_demand;

  // 4. Peer Integration (Symmetric group behavior)
  if (!std::isnan(v->last_peer_pid_demand) && (now - v->last_peer_pid_demand_time < PEER_TIMEOUT_MS)) {
    max_demand = std::max(max_demand, v->last_peer_pid_demand);
  }

  return std::clamp(max_demand, 0.0f, 1.0f);
}

} // namespace auto_mode

/**
 * @brief Main entry point for the automatic ventilation logic.
 * Called periodically from YAML.
 */
inline void evaluate_auto_mode() {
  if (!auto_mode::is_system_ready()) return;

  auto *v = ventilation_ctrl;
  // v is guaranteed non-null by is_system_ready()
  const uint32_t now = millis();

  // 1. Climate Sensor Fusion
  float eff_in, eff_out;
  auto_mode::get_effective_temperatures(now, eff_in, eff_out);

  // 2. Mode Management (Summer Cooling)
  esphome::VentilationMode current_mode = (esphome::VentilationMode)v->state_machine.current_mode;
  esphome::VentilationMode target_mode = auto_mode::determine_auto_operating_mode(eff_in, eff_out, current_mode);
  
  if (current_mode != target_mode) {
    v->set_mode(target_mode);
  }

  // 3. Air Quality Power Management
  float demand = auto_mode::calculate_combined_demand(now);
  
  // 4. Calculate Intensity level (1-10) for UI feedback
  const int min_l = static_cast<int>(automatik_min_fan_level->value());
  const int max_l = static_cast<int>(automatik_max_fan_level->value());
  
  int target_level = min_l;
  if (demand > 0.01f) {
    float scaled = static_cast<float>(min_l) + demand * static_cast<float>(max_l - min_l);
    target_level = std::clamp(static_cast<int>(std::round(scaled)), 1, 10);
    ESP_LOGD("auto_mode", "Demand=%.2f -> Level %d", demand, target_level);
  }

  // 5. Commit State Updates
  if (fan_intensity_level->value() != target_level) {
    // Use the established project pattern for GlobalsComponent: ->value() = ...
    fan_intensity_level->value() = target_level;
    
    // For TemplateNumber, we must use publish_state to update the UI entity
    fan_intensity_display->publish_state(target_level);
    
    // Inform the controller and trigger hardware updates
    v->set_fan_intensity(target_level);
    fan_speed_update->execute();
    update_leds->execute();
    
    ESP_LOGI("auto_mode", "Automatic mode adjusted level to %d", target_level);
  }
}
