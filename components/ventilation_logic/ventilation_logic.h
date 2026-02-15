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

    // --- Adaptive CO2 Control ---

    /// @brief Calculates the target fan intensity level (1–10) based on CO2 concentration.
    /// Uses DIN EN 13779 / Umweltbundesamt thresholds with 100 ppm hysteresis
    /// to prevent rapid cycling.  The result is clamped to [min_level, max_level].
    ///
    /// Thresholds (rising):
    ///   ≤ 600 ppm → Level 1 (minimal noise)
    ///   ≤ 800 ppm → Level 2
    ///   ≤ 1000 ppm → Level 3
    ///   ≤ 1200 ppm → Level 5
    ///   ≤ 1400 ppm → Level 7
    ///   > 1400 ppm → Level 9
    ///
    /// @param co2_ppm        Current CO2 reading (ppm).
    /// @param current_level  Current fan intensity level (for hysteresis direction).
    /// @param min_level      User-configurable lower limit (moisture protection), default 2.
    /// @param max_level      User-configurable upper limit (noise control), default 10.
    /// @return Target fan intensity level (min_level–max_level).
    static int get_co2_fan_level(float co2_ppm, int current_level, int min_level = 2, int max_level = 10);

    /// @brief Returns a human-readable German CO2 classification.
    /// @param co2_ppm  CO2 concentration in ppm.
    /// @return Classification string ("Ausgezeichnet" … "Inakzeptabel").
    static std::string get_co2_classification(float co2_ppm);
};
