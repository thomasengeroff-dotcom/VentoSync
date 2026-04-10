// ==========================================================================
// WRG Wohnraumlüftung – ESPHome Custom Component
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
// File:        auto_mode.h
// Description: Logic for the Ventilation Auto Mode Controller.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "globals.h"

/**
 * @brief Logic for the Ventilation Auto Mode Controller.
 * Final refactored version addressing all safety and maintainability concerns.
 */

namespace auto_mode {

/**
 * @brief   Validates that all required components for auto mode are initialized.
 *
 * @details Ensures that sensors, controllers, and UI entities are non-null
 *          before the evaluation logic starts to prevent segmentation faults.
 *
 * @return  true if the system is safe to evaluate.
 */
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

/**
 * @brief   Combines local and peer sensor data to determine room temperatures.
 *
 * @details This function implements "Sensor Fusion". If a local sensor is
 *          offline or NaN, it attempts to use data shared by peers over ESP-NOW.
 *          It also dynamically maps NTC sensors based on the active airflow
 *          direction to differentiate between Indoor and Outdoor ambient temps.
 *
 * @param[in]  now      Current system time.
 * @param[out] eff_in   Effective indoor temperature calculation result.
 * @param[out] eff_out  Effective outdoor temperature calculation result.
 * @param[in]  current_mode  Current operation mode for direction mapping.
 */
inline void get_effective_temperatures(uint32_t now, float &eff_in, float &eff_out, esphome::VentilationMode current_mode) {
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
  const esphome::HardwareState hw_state = v->state_machine.get_target_state(now);
  const bool is_intake = hw_state.direction_in;
  const int internal_mode = (int)current_mode;

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

/**
 * @brief   Logic for Summer Cooling / Bypass mode.
 *
 * @details Switches from Heat Recovery (MODE_ECO_RECOVERY) to Ventilation
 *          (MODE_VENTILATION) if:
 *          1. The HA "Sommerbetrieb" binary sensor confirms warm season
 *             (April-October AND outdoor temp > 18°C).
 *          2. The indoor temperature exceeds 22°C.
 *          3. The outdoor air is at least 1.5°C cooler.
 *          Includes a deactivation hysteresis to prevent mode oscillations.
 *          When HA is offline, sommerbetrieb defaults to false (safe: heat
 *          recovery stays active).
 */
inline esphome::VentilationMode determine_auto_operating_mode(float eff_in, float eff_out, esphome::VentilationMode current_mode) {
  // Summer cooling requires the HA "Sommerbetrieb" sensor to confirm warm season.
  // Default false when HA is offline → heat recovery stays active (safe fallback).
  const bool is_summer = (sommerbetrieb != nullptr && sommerbetrieb->has_state() && sommerbetrieb->state);

  if (std::isnan(eff_in) || std::isnan(eff_out)) {
    return (current_mode == esphome::MODE_VENTILATION || current_mode == esphome::MODE_ECO_RECOVERY) 
           ? current_mode : esphome::MODE_ECO_RECOVERY;
  }

  if (current_mode != esphome::MODE_VENTILATION) {
    // Activation: Only when summer AND indoor > threshold AND outdoor is sufficiently cooler
    if (is_summer && eff_in > SUMMER_COOLING_THRESHOLD_INDOOR && eff_out < (eff_in - SUMMER_COOLING_MIN_DELTA)) {
      ESP_LOGI("auto_mode", "Sommer-Kühlung AKTIVIERT: Innen=%.1f°C, Außen=%.1f°C", eff_in, eff_out);
      return esphome::MODE_VENTILATION;
    }
    return esphome::MODE_ECO_RECOVERY;
  } else {
    // Deactivation: When NOT summer anymore OR hysteresis triggers
    if (!is_summer || eff_out >= (eff_in - SUMMER_COOLING_HYSTERESIS) || eff_in < SUMMER_COOLING_THRESHOLD_INDOOR_HYSTERESIS) {
      ESP_LOGI("auto_mode", "Sommer-Kühlung DEAKTIVIERT%s: Innen=%.1f°C, Außen=%.1f°C",
               !is_summer ? " (Sommerbetrieb OFF)" : " (Hysterese)", eff_in, eff_out);
      return esphome::MODE_ECO_RECOVERY;
    }
    return esphome::MODE_VENTILATION;
  }
}

/**
 * @brief   Aggregates PID demands with CO2 priority and group-wide coordination.
 *
 * @details CO2 demand always takes priority over humidity to ensure air quality.
 *          Hysteresis (Grab at 0.01 / Release at 0.005) ensures stable behavior
 *          near setpoints. Peer demands are integrated to ensure the entire
 *          room group scales up to the highest measured requirement.
 */
inline float calculate_combined_demand(uint32_t now) {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return 0.0f;

  // Thresholds for hysteresis
  constexpr float CO2_GRAB_THRESHOLD   = 0.01f;  // CO2 takes exclusive control
  constexpr float CO2_RELEASE_THRESHOLD = 0.005f; // CO2 releases control to humidity

  float local_demand = NAN;

  // 1. CO2 Demand (always evaluated)
  float co2_demand = 0.0f;
  bool has_co2_data = false;
  if (co2_pid_result != nullptr) {
    const float co2_val = co2_pid_result->value();
    if (!std::isnan(co2_val)) {
      co2_demand = std::max(0.0f, co2_val);
      has_co2_data = true;
    }
  }

  // 2. Humidity Demand (always evaluated, but only used when CO2 is satisfied)
  float hum_demand = 0.0f;
  bool has_hum_data = false;
  if (scd41_humidity != nullptr && outdoor_humidity != nullptr && humidity_pid_result != nullptr) {
    const float in_hum = scd41_humidity->state;
    const float out_hum = outdoor_humidity->state;
    const float hum_pid_val = humidity_pid_result->value();
    if (!std::isnan(in_hum) && !std::isnan(out_hum) && !std::isnan(hum_pid_val)) {
      has_hum_data = true;
      // Only apply humidity demand if outside air is drier (ventilation would help)
      if (out_hum < in_hum) {
        hum_demand = std::max(0.0f, hum_pid_val);
      }
    }
  }

  // 3. CO2 Priority Logic with Hysteresis
  if (v->co2_is_controlling) {
    // CO2 is currently in control — only release when demand drops below lower threshold
    if (co2_demand < CO2_RELEASE_THRESHOLD && has_co2_data) {
      v->co2_is_controlling = false;
      ESP_LOGD("auto_mode", "CO2 demand released (%.3f < %.3f), humidity PID enabled", co2_demand, CO2_RELEASE_THRESHOLD);
    }
  } else {
    // CO2 is not in control — grab if demand exceeds upper threshold
    if (co2_demand >= CO2_GRAB_THRESHOLD) {
      v->co2_is_controlling = true;
      ESP_LOGD("auto_mode", "CO2 demand grabbed control (%.3f >= %.3f), humidity PID suppressed", co2_demand, CO2_GRAB_THRESHOLD);
    }
  }

  // 4. Select demand based on priority
  if (v->co2_is_controlling) {
    // CO2 is the sole controller
    local_demand = co2_demand;
  } else if (has_hum_data) {
    // CO2 is satisfied, use humidity demand
    local_demand = hum_demand;
  } else if (has_co2_data) {
    // Have CO2 data but no humidity data — use CO2 (even if near zero)
    local_demand = co2_demand;
  }
  // else: local_demand stays NAN → triggers "hold state" guard

  // 5. Peer Integration (Symmetric group behavior)
  float effective_demand = local_demand;
  if (!std::isnan(v->last_peer_pid_demand) && (now - v->last_peer_pid_demand_time < PEER_TIMEOUT_MS)) {
    if (std::isnan(effective_demand) || v->last_peer_pid_demand > effective_demand) {
      if (v->last_peer_pid_demand > (effective_demand + 0.05f)) {
        ESP_LOGI("auto_mode", "Adopting higher peer demand: %.2f (Local: %.2f)", v->last_peer_pid_demand, effective_demand);
      }
      effective_demand = v->last_peer_pid_demand;
    }
  }

  // 6. Network Sync Logic
  if (!std::isnan(effective_demand)) {
    v->local_pid_demand = effective_demand; // Update value for 60s heartbeat
    return std::clamp(effective_demand, 0.0f, 1.0f);
  }

  return NAN;
}

} // namespace auto_mode

/**
 * @brief   Main entry point for the Smart-Automatik logic.
 *
 * @details Periodically evaluates climate state, syncs with peers, and updates 
 *          the local fan intensity. Implements the "Master Authority" rule:
 *          Slaves follow the Master's (ID=1) discrete intensity level to 
 *          ensure perfectly synchronized across the room.
 *
 * @param[in] force  If true, bypasses the 2s evaluation rate-limit.
 */
inline void evaluate_auto_mode(bool force) {
  if (!auto_mode::is_system_ready()) return;

  auto *v = ventilation_ctrl;
  // v is guaranteed non-null by is_system_ready()
  const uint32_t now = millis();

  // Rate-limiting: Prevent multiple calls (e.g., from timer + network handler) 
  // from bypassing ramping limits.
  static uint32_t last_eval_ms = 0;
  if (!force && now - last_eval_ms < 2000) return; // Allow max once per 2 seconds
  last_eval_ms = now;

  // 1. Snapshot common states to avoid race conditions during evaluation
  esphome::VentilationMode current_mode = (esphome::VentilationMode)v->state_machine.current_mode;
  int current_level = static_cast<int>(fan_intensity_level->value());

  // 2. Climate Sensor Fusion
  float eff_in = NAN, eff_out = NAN;
  auto_mode::get_effective_temperatures(now, eff_in, eff_out, current_mode);

  // 3. Mode Management (Summer Cooling)
  esphome::VentilationMode target_mode = auto_mode::determine_auto_operating_mode(eff_in, eff_out, current_mode);
  
  if (current_mode != target_mode) {
    v->set_mode(target_mode);
    current_mode = target_mode; // Update local snapshot for correct demand mapping
  }

  // 3. Air Quality Power Management
  float demand = auto_mode::calculate_combined_demand(now);

  // FIXED: If demand is NAN (e.g., all sensors offline/unstable), we abort to hold the last state
  if (std::isnan(demand)) {
     ESP_LOGV("auto_mode", "Demand is NAN, skipping update to hold last state.");
     return;
  }
  
  // 5. Calculate target level with hysteresis and ramping
  int min_l = static_cast<int>(automatik_min_fan_level->value());
  int max_l = static_cast<int>(automatik_max_fan_level->value());
  if (min_l > max_l) std::swap(min_l, max_l);
  
  int target_level = min_l;

  // AUTHORITY RULE: In Auto Mode, Slaves should follow the Master's discrete intensity.
  // This prevents the "jumping" behavior where nodes fight over level boundaries.
  bool slave_following_master = false;
  if (v->device_id != 1) { // I am a Slave
    // Check if we have a recent state from the Master
    for (const auto &peer : v->peers) {
      if (peer.device_id == 1 && (now - peer.last_seen_ms < PEER_TIMEOUT_MS)) {
        target_level = peer.fan_intensity;
        slave_following_master = true;
        break;
      }
    }
  }

  if (!slave_following_master) {
    // I am the Master OR the Master is offline -> Calculate level from demand
    float scaled = static_cast<float>(min_l) + demand * static_cast<float>(max_l - min_l);
    int raw_target = std::clamp(static_cast<int>(std::round(scaled)), 1, 10);
    
    // Hysteresis: Only change level if demand clearly crosses the boundary
    static constexpr float LEVEL_HYSTERESIS = 0.25f; // 25% of one level step
    float step_size = (max_l > min_l) ? 1.0f / static_cast<float>(max_l - min_l) : 1.0f;
    float hysteresis_band = step_size * LEVEL_HYSTERESIS;
    
    // Current demand center relative to current level
    float current_demand_center = static_cast<float>(current_level - min_l) * step_size;

    if (demand > current_demand_center + step_size * 0.5f + hysteresis_band) {
      target_level = raw_target; // Clearly above -> go up
    } else if (demand < current_demand_center - step_size * 0.5f - hysteresis_band) {
      target_level = raw_target; // Clearly below -> go down
    } else {
      target_level = current_level; // Within hysteresis band -> hold
    }
  }

  // Soft Ramping: Maximum ±1 level per evaluation cycle (10s)
  // Boost: Allow ±2 steps if mode just changed to reach target faster
  static bool first_eval = true;
  static esphome::VentilationMode last_committed_mode = esphome::MODE_OFF;
  int max_ramp_step = 1;
  
  if (first_eval) {
    last_committed_mode = current_mode;
    first_eval = false;
  } else if (current_mode != last_committed_mode) {
    max_ramp_step = 2;
    last_committed_mode = current_mode;
  }

  if (target_level > current_level) {
    target_level = std::min(target_level, current_level + max_ramp_step);
  } else if (target_level < current_level) {
    target_level = std::max(target_level, current_level - max_ramp_step);
  }

  // 6. Commit State Updates
  if (current_level != target_level) {
    if (slave_following_master) {
       ESP_LOGD("auto_mode", "Slave following Master level: %d (Local demand: %.2f)", target_level, demand);
    }
    
    fan_intensity_level->value() = target_level;
    fan_intensity_display->publish_state(target_level);
    v->current_fan_intensity = target_level;
    
    fan_speed_update->execute();
    update_leds->execute();
    
    ESP_LOGI("auto_mode", "Automatic level %s %d -> %d (demand=%.2f)", 
             slave_following_master ? "synced" : "ramped", current_level, target_level, demand);
  }
}
