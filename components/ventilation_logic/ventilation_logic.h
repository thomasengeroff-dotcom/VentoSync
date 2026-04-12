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
  /// @brief Calculates heat-recovery efficiency (WRG) as a percentage.
  /// Formula: (T_supply − T_outside) / (T_indoor − T_outside) × 100.
  /// Returns 0 if ΔT < 2 °C (avoids division by zero and noise).
  static float calculate_heat_recovery_efficiency(float t_raum, float t_zuluft,
                                                  float t_aussen);

  /// @brief Returns true if the manual speed slider is below the "off"
  /// threshold.
  /// @param value  Slider position (0–100).
  static bool is_fan_slider_off(float value);

  /// @brief Linear ramp-up: maps iteration (0–99) to duty cycle (0.0–1.0).
  static float calculate_ramp_up(int iteration);
  /// @brief Linear ramp-down: maps iteration (0–99) to duty cycle (1.0–0.0).
  static float calculate_ramp_down(int iteration);

  /// @brief Cycles fan intensity level 1→2→…→10→1.
  /// @param current_level  Current level (1–10).
  /// @return Next level.
  static int get_next_fan_level(int current_level);

  /// @brief Maps a user-facing level (1-10) to actual hardware motor speed (10%
  /// - 100%).
  /// @param intensity  Fan intensity level (1–10).
  /// @return Target speed as duty cycle fraction (0.1–1.0).
  static float calculate_fan_speed_from_intensity(int intensity);

  /// @brief Calculates the single-signal PWM duty cycle for the VentoMaxx fan.
  /// Uses the V-curve measured from original hardware: 50% = STOP,
  /// < 50% = Direction A (Abluft), > 50% = Direction B (Zuluft).
  /// @param speed      Target speed fraction (0.1 = min, 1.0 = max).
  /// @param direction  Target direction (0 = Abluft, 1 = Zuluft).
  /// @return PWM duty cycle (0.02 - 0.98).
  static float calculate_fan_pwm(float speed, int direction);

  /// @brief Calculates the dynamic WRG cycle duration based on fan intensity.
  /// Linear scaling: Level 1 @ 70s, Level 10 @ 50s.
  /// @param intensity  Fan intensity level (1–10).
  /// @return Cycle duration in milliseconds.
  static uint32_t calculate_dynamic_cycle_duration(int intensity);

  /// @brief Calculates the virtual fan RPM based on speed and direction.
  /// Base: 4200 RPM @ 100% speed.
  /// @param speed               Current target speed (0.0–1.0).
  /// @param direction_is_intake True if direction is Zuluft (positive RPM).
  /// @param ramp_factor         Current software ramping factor (0.0–1.0).
  /// @return Virtual RPM (negative for Abluft).
  static float calculate_virtual_fan_rpm(float speed, bool direction_is_intake,
                                         float ramp_factor = 1.0f);

  /// @brief Returns a human-readable German CO2 classification.
  /// @param co2_ppm  CO2 concentration in ppm.
  /// @return Classification string ("Ausgezeichnet" … "Inakzeptabel").
  static std::string get_co2_classification(float co2_ppm);
};
