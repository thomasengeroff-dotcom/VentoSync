#include "ventilation_logic.h"

/// @brief Maps raw IAQ index to a human-readable German classification.
/// Thresholds follow the Bosch BME680 BSEC recommendation.
std::string VentilationLogic::get_iaq_classification(float iaq_val) {
  int val = (int)iaq_val;
  if (val <= 50) {
    return "Ausgezeichnet";
  } else if (val <= 100) {
    return "Gut";
  } else if (val <= 150) {
    return "Mäßig";
  } else if (val <= 200) {
    return "Schlecht";
  } else if (val <= 300) {
    return "Sehr Schlecht";
  } else {
    return "Gesundheitsgefährdend";
  }
}

/// @brief Converts IAQ value to an RGB traffic-light colour vector.
/// Green (≤50) → Yellow-Green → Yellow → Orange → Red (>200).
/// The 3-byte result is sent to the peer device via ESP-NOW.
std::vector<uint8_t> VentilationLogic::get_iaq_traffic_light_data(float iaq_val) {
  uint8_t r = 0, g = 0, b = 0;
  int val = (int)iaq_val;
  
  if (val <= 50) { // Green
    g = 255; 
  } else if (val <= 100) { // Yellow-Green
    r = 128; g = 255;
  } else if (val <= 150) { // Yellow
    r = 255; g = 255;
  } else if (val <= 200) { // Orange
    r = 255; g = 128;
  } else { // Red
    r = 255; 
  }
  
  return {r, g, b};
}

/// @brief Calculates Wärmerückgewinnung (heat recovery) efficiency.
/// Returns 0 when the indoor/outdoor ΔT is < 1 °C, clamped to [0, 100] %.
float VentilationLogic::calculate_heat_recovery_efficiency(float t_raum, float t_zuluft, float t_aussen) {
  // Efficiency = (T_supply - T_outside) / (T_indoor - T_outside) * 100
  // Avoid division by zero
  if (std::abs(t_raum - t_aussen) < 1.0) {
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
    // 0 to 100 in 100 steps
    return (float)iteration / 100.0f;
}

/// @brief Linear ramp-down over 100 iterations: returns (100 − iteration) / 100.
float VentilationLogic::calculate_ramp_down(int iteration) {
    // 100 to 0 in 100 steps
    return (100.0f - (float)iteration) / 100.0f;
}

/// @brief Cycles fan intensity: 1→2→…→10→1 (wraps at 10).
int VentilationLogic::get_next_fan_level(int current_level) {
    return (current_level % 10) + 1;  // 1->2->...->10->1
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
