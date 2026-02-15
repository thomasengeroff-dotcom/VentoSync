#pragma once

/// @file ventilation_logic.h
/// @brief Pure utility functions for IAQ classification, fan control math,
/// and heat recovery calculation.  No ESPHome or hardware dependencies —
/// all methods are static and unit-testable.

#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

/// @class VentilationLogic
/// @brief Static helper library — pure functions with no side effects.
/// Used by automation_helpers.h (YAML lambdas) and by the native C++ unit tests.
class VentilationLogic {
public:
    /// @brief Maps a raw BME680 IAQ index to a German human-readable label.
    /// @param iaq_val  IAQ value (0–500+).
    /// @return Classification string ("Ausgezeichnet" … "Gesundheitsgefährdend").
    static std::string get_iaq_classification(float iaq_val);

    /// @brief Generates an RGB byte vector for a traffic-light LED based on IAQ.
    /// Sent over ESP-NOW so the peer can mirror the colour.
    /// @param iaq_val  IAQ value (0–500+).
    /// @return 3-byte vector {R, G, B}.
    static std::vector<uint8_t> get_iaq_traffic_light_data(float iaq_val);

    /// @brief Calculates heat-recovery efficiency (WRG) as a percentage.
    /// Formula: (T_supply − T_outside) / (T_indoor − T_outside) × 100.
    /// Returns 0 if ΔT < 1 °C (avoids division by zero).
    static float calculate_heat_recovery_efficiency(float t_raum, float t_zuluft, float t_aussen);

    /// @brief Returns true if the manual speed slider is below the "off" threshold.
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
};
