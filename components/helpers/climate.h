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
#include "globals.h"

/**
 * @brief   Maps a raw CO2 ppm value to a human-readable air quality string.
 */
inline std::string get_co2_classification(float co2_ppm) {
  return VentilationLogic::get_co2_classification(co2_ppm);
}

/**
 * @brief   Calculates the heat recovery efficiency fraction (0.0 – 1.0).
 *
 * @details Only samples data during stable "Zuluft" (intake) phases in
 *          WRG mode. This is necessary because the NTC sensors require
 *          time to stabilize as the ceramic heat exchanger reaches its
 *          thermal equilibrium after a direction flip.
 *
 * @param[in] t_raum      Indoor room temperature.
 * @param[in] t_zuluft    Fresh air temperature (after the exchanger).
 * @param[in] t_aussen    Outside ambient air temperature.
 * @param[in] current_eff The last calculated efficiency (to hold during flips).
 */
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
  const uint32_t time_since_flip = now - last_direction_change_time;
  const bool is_stable = time_since_flip > stable_time_ms;

  if (is_wrg && is_intake && is_stable) {
    if (std::isnan(t_raum) || std::isnan(t_zuluft) || std::isnan(t_aussen)) {
      ESP_LOGD("climate", "WRG Efficiency: Sensor data NaN, holding: %.1f%%", current_eff);
      return current_eff;
    }
    float eff = VentilationLogic::calculate_heat_recovery_efficiency(
        t_raum, t_zuluft, t_aussen);
    ESP_LOGD("climate", "WRG Efficiency update: %.1f%% (Room:%.1f Zuluft:%.1f Out:%.1f)", 
             eff, t_raum, t_zuluft, t_aussen);
    return eff;
  }

  // Debugging holding reasons (only in WRG mode to avoid log spam)
  if (is_wrg) {
    if (!is_intake) {
      ESP_LOGD("climate", "WRG Efficiency: Holding during exhaust phase (Abluft)");
    } else if (!is_stable) {
      ESP_LOGD("climate", "WRG Efficiency: Waiting for stabilization (%us remaining)", 
               (stable_time_ms - time_since_flip) / 1000);
    }
  }

  // Not in a valid sampling window → hold last known value
  return current_eff;
}

// Stabilization parameter constants
constexpr float NTC_MAX_DEVIATION = 0.3f;
constexpr uint32_t NTC_MIN_WAIT_MS = 15000;
constexpr size_t NTC_WINDOW_SIZE = 3;

/**
 * @brief   NTC Sliding Window Stabilization Filter.
 *
 * @details Implements a mandatory thermal adjustment wait time (min 15s)
 *          followed by a sliding window standard deviation check. 
 *          This is critical because NTC sensors are thermally coupled
 *          to the ceramic core, which has high thermal inertia and
 *          requires time to "settle" after each direction change.
 *
 * @param[in] sensor_idx  0 for temp_zuluft, 1 for temp_abluft.
 * @param[in] new_value   The raw value reported by the physical NTC component.
 *
 * @return  The original value if stable, else an empty optional to discard the update.
 */
inline esphome::optional<float> filter_ntc_stable(int sensor_idx,
                                                  float new_value) {
  if (std::isnan(new_value)) {
    return {}; // FIXED P3: Discard invalid sensor readings to prevent history pollution
  }

  if (ventilation_ctrl == nullptr) {
    return new_value; // Fallback if controller is not bound
  }

  const uint32_t cycle_ms = ventilation_ctrl->state_machine.cycle_duration_ms;
  if (cycle_ms == 0) return new_value;

  // Warn if cycle is too short for meaningful NTC stabilization.
  if (cycle_ms < 30000) {
    ESP_LOGW("ntc_filter",
             "cycle_duration_ms=%u is very short (<30s). NTC stabilization "
             "filter may never pass a value.",
             cycle_ms);
  }

  // Dynamic wait time: 40% of the cycle, but minimum 15 seconds
  uint32_t wait_ms =
      std::max(NTC_MIN_WAIT_MS, static_cast<uint32_t>(cycle_ms * 0.4f));

  // Safety check so we don't wait longer than the cycle itself minus 5s
  if (wait_ms >= cycle_ms) {
    wait_ms = cycle_ms > 5000 ? cycle_ms - 5000 : 0;
  }

  // 1. Check if we are still within the mandatory thermal adjustment wait time
  if (millis() - last_direction_change_time < wait_ms) {
    return {}; // Discard value while ceramic adjusts
  }

  // 2. Add current value to history window
  auto &history = ntc_history[sensor_idx];
  history.push_back(new_value);

  // Maintain maximum window size limit
  if (history.size() > NTC_WINDOW_SIZE) {
    history.pop_front();
  }

  // 3. Wait until the window is full for a reliable stabilization check
  if (history.size() < NTC_WINDOW_SIZE) {
    return {}; // Discard value while checking buffer window fills up
  }

  // 4. Calculate deviation via STL
  auto [min_it, max_it] = std::minmax_element(history.begin(), history.end());
  
  // 5. Evaluate stability
  if ((*max_it - *min_it) <= NTC_MAX_DEVIATION) {
    // Temperature is stable!
    return new_value;
  } else {
    // Still fluctuating, discard update
    return {};
  }
}