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

/// @brief Calculates Wärmerückgewinnung (heat recovery) efficiency.
/// Returns 0 when the indoor/outdoor ΔT is < 0.3 °C, clamped to [0, 100] %.
float VentilationLogic::calculate_heat_recovery_efficiency(float t_raum, float t_zuluft, float t_aussen) {
  // Efficiency = (T_supply - T_outside) / (T_indoor - T_outside) * 100
  // Avoid division by zero
  if (std::abs(t_raum - t_aussen) < 0.3) {
    return 0.0;
  }
  float eff = (t_zuluft - t_aussen) / (t_raum - t_aussen) * 100.0;
  if (eff < 0) return 0.0;
  if (eff > 100) return 100.0; // Cap at 100% (can happen with sensor noise)
  return eff;
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
    // Level 1 @ 10%, Level 10 @ 100%
    int clamped_intensity = std::max(1, std::min(10, intensity));
    return 0.10f + ((float)(clamped_intensity - 1) / 9.0f) * 0.90f;
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

// --- Adaptive CO2 Control ---

/// @brief Determines the target fan level for a given CO2 concentration.
///
/// The algorithm uses asymmetric thresholds (hysteresis) to prevent
/// "rapid cycling" — the fan ramps up at one threshold but only ramps
/// down when CO2 drops 100 ppm below that threshold.
///
/// Thresholds (based on DIN EN 13779 / Umweltbundesamt):
///   ≤ 600 ppm  → Level 1  (excellent air, minimal fan noise)
///   ≤ 800 ppm  → Level 2  (good air)
///   ≤ 1000 ppm → Level 3  (moderate — Pettenkofer limit)
///   ≤ 1200 ppm → Level 5  (elevated)
///   ≤ 1400 ppm → Level 7  (poor)
///   > 1400 ppm → Level 9  (unacceptable — maximum ventilation)
///
/// Hysteresis: When the current level is above the nominal target,
/// CO2 must drop 100 ppm below the threshold before the fan steps down.
/// This prevents oscillation when CO2 hovers near a boundary.
int VentilationLogic::get_co2_fan_level(float co2_ppm, int current_level, int min_level, int max_level) {
    if (std::isnan(co2_ppm) || co2_ppm <= 0) {
        return current_level;  // Sensor unavailable → keep current level
    }

    // Threshold table: {up_threshold, down_threshold, target_level}
    // "up" = CO2 above this → switch to target_level
    // "down" = CO2 below this → allow stepping back below target_level
    struct Threshold {
        float up;
        float down;
        int level;
    };

    // Ordered from highest to lowest — first match wins
    static const Threshold thresholds[] = {
        {1400.0f, 1300.0f, 9},  // > 1400 ppm → Level 9
        {1200.0f, 1100.0f, 7},  // > 1200 ppm → Level 7
        {1000.0f,  900.0f, 5},  // > 1000 ppm → Level 5
        { 800.0f,  700.0f, 3},  // >  800 ppm → Level 3
        { 600.0f,  500.0f, 2},  // >  600 ppm → Level 2
    };

    int target = 1;  // Default: lowest level for ≤ 600 ppm

    for (const auto &t : thresholds) {
        if (co2_ppm >= t.up) {
            // CO2 is above the "up" threshold → step up
            target = t.level;
            break;
        }
        if (current_level >= t.level && co2_ppm >= t.down) {
            // CO2 is in hysteresis band and fan is already at or above this level
            // → hold (don't step down yet)
            target = t.level;
            break;
        }
    }

    // Clamp min_level and max_level to valid range
    if (min_level < 1) min_level = 1;
    if (min_level > 10) min_level = 10;
    if (max_level < 1) max_level = 1;
    if (max_level > 10) max_level = 10;
    if (min_level > max_level) min_level = max_level;

    // Clamp target to [min_level, max_level]
    if (target < min_level) target = min_level;
    if (target > max_level) target = max_level;

    return target;
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
