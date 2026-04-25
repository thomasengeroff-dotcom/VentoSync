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
// File:        ventilation_logic.h
// Description: Core ventilation math and logic utilities.
// Author:      Thomas Engeroff
// Created:     2026-02-15
// Modified:    2026-03-23
// ==========================================================================
#pragma once

/// @file ventilation_logic.h
/// @brief Pure utility functions for IAQ classification, fan control math,
/// and heat recovery calculation.  No ESPHome or hardware dependencies —
/// all methods are static and unit-testable.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>


/// @class VentilationLogic
/// @brief Static helper library — pure functions with no side effects.
/// Used by automation_helpers.h (YAML lambdas) and by the native C++ unit
/// tests.
class VentilationLogic {
public:
  /**
   * @brief   Calculates heat-recovery efficiency (WRG) as a percentage.
   * @details Formula: (T_supply − T_outside) / (T_indoor − T_outside) × 100.
   *          Returns 0 if ΔT < 2 °C (avoids division by zero and noise).
   * @param[in] t_raum    Indoor room temperature.
   * @param[in] t_zuluft  Supply air temperature (after the exchanger).
   * @param[in] t_aussen  Outside ambient air temperature.
   * @return  Efficiency in percent (0.0 to 100.0).
   */
  static float calculate_heat_recovery_efficiency(float t_raum, float t_zuluft,
                                                  float t_aussen);

  /**
   * @brief   Checks if the manual fan slider is in the "Off" position.
   * @param[in] value  Slider intensity (1 to 10).
   * @return  true if value < 1.0.
   */
  static bool is_fan_slider_off(float value);

  /**
   * @brief   Linear ramp-up: maps iteration (0–100) to duty cycle (0.0–1.0).
   * @param[in] iteration  Step index.
   * @return  Ramp factor (0.0 to 1.0).
   */
  static float calculate_ramp_up(int iteration);
  /**
   * @brief   Linear ramp-down: maps iteration (0–100) to duty cycle (1.0–0.0).
   * @param[in] iteration  Step index.
   * @return  Ramp factor (0.0 to 1.0).
   */
  static float calculate_ramp_down(int iteration);

  /**
   * @brief   Cycles fan intensity level 1→2→…→10→1.
   * @param[in] current_level  Current level (1–10).
   * @return  Next level.
   */
  static int get_next_fan_level(int current_level);

  /**
   * @brief   Maps a user-facing level (1-10) to hardware speed (0.1–1.0).
   * @param[in] intensity  Fan intensity level (1–10).
   * @return  Target speed fraction (0.1–1.0).
   */
  static float calculate_fan_speed_from_intensity(int intensity);

  /**
   * @brief   Calculates the PWM duty cycle for the VentoMaxx fan.
   * @details 50% = STOP, < 50% = Direction A, > 50% = Direction B.
   * @param[in] speed      Target speed fraction (0.1 to 1.0).
   * @param[in] direction  Target direction (0 = Exhaust, 1 = Intake).
   * @return  PWM duty cycle (0.02 to 0.98).
   */
  static float calculate_fan_pwm(float speed, int direction);

  /**
   * @brief   Calculates the dynamic WRG cycle duration based on intensity.
   * @details Linear scaling: Level 1 @ 70s, Level 10 @ 50s.
   * @param[in] intensity  Fan intensity level (1–10).
   * @return  Cycle duration in milliseconds.
   */
  static uint32_t calculate_dynamic_cycle_duration(int intensity);

  /**
   * @brief   Calculates the virtual fan RPM based on speed and direction.
   * @param[in] speed                Current target speed (0.0–1.0).
   * @param[in] direction_is_intake  True if direction is Zuluft.
   * @param[in] ramp_factor          Current software ramping factor (0.0–1.0).
   * @return  Virtual RPM (negative for Exhaust).
   */
  static float calculate_virtual_fan_rpm(float speed, bool direction_is_intake,
                                         float ramp_factor = 1.0f);

  /**
   * @brief   Returns a human-readable German CO2 classification.
   * @param[in] co2_ppm  CO2 concentration in ppm.
   * @return  Classification string (e.g. "Gut", "Schlecht").
   */
  static std::string get_co2_classification(float co2_ppm);
};
