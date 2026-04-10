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
// File:        ventilation_logic.cpp
// Description: Core ventilation math and logic implementation.
// Author:      Thomas Engeroff
// Created:     2026-02-15
// Modified:    2026-03-23
// ==========================================================================
#include "ventilation_logic.h"

#ifdef ESPHOME
#include "esphome/core/log.h"
#endif

/// @brief Calculates Wärmerückgewinnung (heat recovery) efficiency.
/// @returns Efficiency in [0, 100] %, or 0 if inputs are invalid 
///          or indoor/outdoor ΔT is too small for meaningful calculation.
float VentilationLogic::calculate_heat_recovery_efficiency(float t_raum, float t_zuluft, float t_aussen) {
  // Guard against NaN from sensor read failures
  if (std::isnan(t_raum) || std::isnan(t_zuluft) || std::isnan(t_aussen)) {
#ifdef ESPHOME
    ESP_LOGD("ventilation_logic", "WRG Math: NaN in (Room:%.1f, Zuluft:%.1f, Out:%.1f)", t_raum, t_zuluft, t_aussen);
#endif
    return 0.0f;
  }

  // Below this ΔT, sensor noise dominates → result is meaningless
  static constexpr float kMinDeltaT = 2.0f;

  const float delta_t = t_raum - t_aussen;
  if (std::abs(delta_t) < kMinDeltaT) {
#ifdef ESPHOME
    ESP_LOGD("ventilation_logic", "WRG Math: DeltaT too small: %.2f (min: %.1f)", delta_t, kMinDeltaT);
#endif
    return 0.0f;
  }

  // η = (T_supply - T_outside) / (T_indoor - T_outside) × 100
  const float efficiency = (t_zuluft - t_aussen) / delta_t * 100.0f;
  const float result = std::clamp(efficiency, 0.0f, 100.0f);

#ifdef ESPHOME
  ESP_LOGD("ventilation_logic", "WRG Math: Calculated %.1f%% (raw: %.1f%%)", result, efficiency);
#endif

  return result;
}

/// @brief Determines if the manual speed slider is below the "off" threshold (< 1.0).
bool VentilationLogic::is_fan_slider_off(float value) {
    return value < 1.0; 
}

/// @brief Linear ramp-up over 100 iterations: returns iteration / 100.
float VentilationLogic::calculate_ramp_up(int iteration) {
    if (iteration < 0) return 0.0f;
    if (iteration > 100) return 1.0f;
    // 0 to 100 in 100 steps
    return (float)iteration / 100.0f;
}

/// @brief Linear ramp-down over 100 iterations: returns (100 − iteration) / 100.
float VentilationLogic::calculate_ramp_down(int iteration) {
    if (iteration < 0) return 1.0f;
    if (iteration > 100) return 0.0f;
    // 100 to 0 in 100 steps
    return (100.0f - (float)iteration) / 100.0f;
}

/// @brief Cycles fan intensity: 1→2→…→10→1 (wraps at 10).
int VentilationLogic::get_next_fan_level(int current_level) {
    return (current_level % 10) + 1;  // 1->2->...->10->1
}

float VentilationLogic::calculate_fan_speed_from_intensity(int intensity) {
    // Quadratic mapping for finer control at low levels.
    // Formula: v(x) = 0.005 * (x-1)^2 + 0.055 * (x-1) + 0.1
    // Matches: L1=10%, L6=50%, L10=100%
    float x = (float)std::max(1, std::min(10, intensity)) - 1.0f;
    return 0.005f * x * x + 0.055f * x + 0.10f;
}

float VentilationLogic::calculate_fan_pwm(float speed, int direction) {
    // Clamp speed to valid range
    speed = std::max(0.0f, std::min(1.0f, speed));

    // Stop zone: below 5% speed
    if (speed < 0.05f) {
        return 0.50f;
    }

    float pwm;
    if (direction == 0) {
        // Direction Abluft (Raus): Min Speed (0.1) -> 30% PWM, Max Speed (1.0) -> 5% PWM
        pwm = 0.30f - ((speed - 0.1f) / 0.9f) * 0.25f;
    } else {
        // Direction Zuluft (Rein): Min Speed (0.1) -> 70% PWM, Max Speed (1.0) -> 95% PWM
        pwm = 0.70f + ((speed - 0.1f) / 0.9f) * 0.25f;
    }

    // Clamp to absolute hardware safety limits
    return std::max(0.02f, std::min(0.98f, pwm));
}

uint32_t VentilationLogic::calculate_dynamic_cycle_duration(int intensity) {
    // Level 1: 70s, Level 10: 50s
    int clamped_intensity = std::max(1, std::min(10, intensity));
    float duration_s = 70.0f - ((float)(clamped_intensity - 1) * (20.0f / 9.0f));
    return (uint32_t)(std::round(duration_s) * 1000.0f);
}

float VentilationLogic::calculate_virtual_fan_rpm(float speed, bool direction_is_intake, float ramp_factor) {
    float effective_speed = speed * ramp_factor;
    if (effective_speed < 0.05f) return 0.0f;
    
    float rpm = effective_speed * 4200.0f;
    return direction_is_intake ? rpm : -rpm;
}

/// @brief Maps CO2 ppm to a human-readable German air-quality label.
/// Follows Umweltbundesamt (German Federal Environment Agency) guidance.
std::string VentilationLogic::get_co2_classification(float co2_ppm) {
    if (std::isnan(co2_ppm) || co2_ppm <= 0) return "Unbekannt";
    int val = (int)co2_ppm;
    if (val <= 600)  return "Ausgezeichnet";
    if (val <= 800)  return "Gut";
    if (val <= 1000) return "Mäßig";
    if (val <= 1200) return "Erhöht";
    if (val <= 1400) return "Schlecht";
    return "Inakzeptabel";
}
