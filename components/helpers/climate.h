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