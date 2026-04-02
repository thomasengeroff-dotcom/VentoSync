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
// File:        climate.h
// Description: Helper functions for climate monitoring.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "helpers/globals.h"

inline std::string get_co2_classification(float co2_ppm) {
  return VentilationLogic::get_co2_classification(co2_ppm);
}

/// @brief Calculates CO2-based fan level with hysteresis.
inline int get_co2_fan_level(float co2_ppm, int current_level, int min_level,
                             int max_level) {
  return VentilationLogic::get_co2_fan_level(co2_ppm, current_level, min_level,
                                             max_level);
}

/// @brief Calculates Wärmerückgewinnung (heat recovery) efficiency safely.
/// Only performs calculation during stable Intake (Zuluft) phase in WRG mode.
/// Returns current_eff (hold) when conditions are not met.
inline float calculate_heat_recovery_efficiency(float t_raum, float t_zuluft,
                                                float t_aussen,
                                                float current_eff) {
  if (ventilation_ctrl == nullptr)
    return NAN;

  const uint32_t now = millis();
  const auto mode = ventilation_ctrl->state_machine.current_mode;
  const auto hw = ventilation_ctrl->state_machine.get_target_state(now);

  const bool is_wrg = (mode == esphome::MODE_ECO_RECOVERY);
  const bool is_intake = hw.direction_in;

  // Thermal stabilization: Wait at least 30s into the cycle before sampling
  constexpr uint32_t stable_time_ms = 30000;
  const bool is_stable = (now - last_direction_change_time) > stable_time_ms;

  if (is_wrg && is_intake && is_stable) {
    if (std::isnan(t_raum) || std::isnan(t_zuluft) || std::isnan(t_aussen)) {
      return current_eff;
    }
    return VentilationLogic::calculate_heat_recovery_efficiency(
        t_raum, t_zuluft, t_aussen);
  }

  return current_eff; // Hold last value outside stable intake phase
}

// Stabilization parameter constants
static const float NTC_MAX_DEVIATION = 0.3f; // Max allowed deviation in window

/**
 * @brief NTC Sliding Window Stabilization Filter.
 * Discards values while ceramic adjusts or during high fluctuation.
 * @param sensor_idx 0 for temp_zuluft, 1 for temp_abluft
 * @param new_value The raw value reported by the physical NTC component
 * @return The original value if stable, else empty optional to discard update
 */
inline esphome::optional<float> filter_ntc_stable(int sensor_idx,
                                                  float new_value) {
  if (ventilation_ctrl == nullptr ||
      ventilation_ctrl->state_machine.cycle_duration_ms == 0) {
    return new_value; // Fallback if controller is not bound
  }

  uint32_t cycle_duration_ms =
      ventilation_ctrl->state_machine.cycle_duration_ms;

  // Warn if cycle is too short for meaningful NTC stabilization.
  if (cycle_duration_ms < 30000) {
    ESP_LOGW("ntc_filter",
             "cycle_duration_ms=%u is very short (<30s). NTC stabilization "
             "filter may never pass a value.",
             cycle_duration_ms);
  }

  // Dynamic wait time: 40% of the cycle, but minimum 15 seconds
  uint32_t wait_time_ms =
      std::max((uint32_t)15000, (uint32_t)(cycle_duration_ms * 0.4f));

  // Safety check so we don't wait longer than the cycle itself minus 5s
  if (wait_time_ms >= cycle_duration_ms) {
    wait_time_ms = cycle_duration_ms > 5000 ? cycle_duration_ms - 5000 : 0;
  }

  // Use a smaller, fixed window of 3 samples (approx 9-10s) for stability.
  const size_t target_window_size = 3;

  // 1. Check if we are still within the mandatory thermal adjustment wait time
  if (millis() - last_direction_change_time < wait_time_ms) {
    return {}; // Discard value while ceramic adjusts
  }

  // 2. Add current value to history window
  auto &history = ntc_history[sensor_idx];
  history.push_back(new_value);

  // Maintain maximum window size limit
  while (history.size() > target_window_size) {
    history.pop_front();
  }

  // 3. Wait until the window is full for a reliable stabilization check
  if (history.size() < target_window_size) {
    return {}; // Discard value while checking buffer window fills up
  }

  // 4. Calculate deviation
  auto [min_it, max_it] = std::minmax_element(history.begin(), history.end());
  float min_val = *min_it;
  float max_val = *max_it;

  // 5. Evaluate stability
  if ((max_val - min_val) <= NTC_MAX_DEVIATION) {
    // Temperature is stable!
    return new_value;
  } else {
    // Still fluctuating, keep last known state
    return {};
  }
}