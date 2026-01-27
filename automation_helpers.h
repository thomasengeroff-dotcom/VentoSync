#pragma once

#include "esphome.h"
#include <string>

// --- IAQ Classification ---
inline std::string get_iaq_classification(float iaq_val) {
  int val = (int)iaq_val;
  if (val <= 50) {
    return "Excellent";
  } else if (val >= 51 && val <= 100) {
    return "Good";
  } else if (val >= 101 && val <= 150) {
    return "Lightly polluted";
  } else if (val >= 151 && val <= 200) {
    return "Moderately polluted";
  } else if (val >= 201 && val <= 250) {
    return "Heavily polluted";
  } else if (val >= 251 && val <= 350) {
    return "Severely polluted";
  } else if (val >= 351 && val <= 500) {
    return "Extremely polluted";
  } else {
    return "Error";
  }
}

// --- Fan Ramp Logic ---
// 0 -> 100% in 100 steps
inline float calculate_ramp_up(int iteration) {
  // iteration is 0..99
  return (float)(iteration + 1) / 100.0f;
}

// 100% -> 0% in 100 steps
inline float calculate_ramp_down(int iteration) {
  // iteration is 0..99
  // 100 - 0 - 1 = 99 -> 0.99
  // 100 - 99 - 1 = 0 -> 0.00
  return (float)(100 - iteration - 1) / 100.0f;
}

// --- IAQ Threshold Checks ---
inline bool is_iaq_good(float x) {
  return x <= 80;
}

inline bool is_iaq_medium(float x) {
  return x > 80 && x <= 200;
}

inline bool is_iaq_bad(float x) {
  return x > 200;
}

// --- IAQ Traffic Light Helper ---
// Returns [0x03] for Good, [0x02] for Medium, [0x01] for Bad
inline std::vector<uint8_t> get_iaq_traffic_light_data(float iaq_val) {
  if (is_iaq_good(iaq_val)) return {0x03};
  if (is_iaq_medium(iaq_val)) return {0x02};
  return {0x01};
}

// --- Fan Slider Logic ---
inline bool is_fan_slider_off(float x) {
  return x == 0;
}
