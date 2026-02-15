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
