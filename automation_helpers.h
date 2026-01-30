#pragma once

#include "esphome.h"
#include <string>

// --- IAQ Classification ---
inline std::string get_iaq_classification(float iaq_val) {
  int val = (int)iaq_val;
  if (val <= 50) {
    return "Ausgezeichnet";
  } else if (val <= 100) {
    return "Gut";
  } else if (val <= 150) {
    return "Leicht verschmutzt";
  } else if (val <= 200) {
    return "Mäßig verschmutzt";
  } else if (val <= 250) {
    return "Stark verschmutzt";
  } else if (val <= 350) {
    return "Sehr stark verschmutzt";
  } else if (val <= 500) {
    return "Extrem verschmutzt";
  } else {
    return "Fehler";
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

// --- Heat Recovery Efficiency Calculation ---
// Calculates the efficiency of heat recovery based on temperature difference
// Formula: Efficiency = (T_zuluft - T_außen) / (T_raum - T_außen) * 100%
// Returns efficiency percentage (0-100%) or NAN if calculation is not possible
inline float calculate_heat_recovery_efficiency(float t_raum, float t_zuluft, float t_aussen) {
  // Only calculate if all sensors have valid readings
  if (isnan(t_raum) || isnan(t_zuluft) || isnan(t_aussen)) {
    return NAN;
  }
  
  // Avoid division by zero
  float delta_total = t_raum - t_aussen;
  if (abs(delta_total) < 1.0) {
    return 0.0;
  }
  
  float delta_recovered = t_zuluft - t_aussen;
  float efficiency = (delta_recovered / delta_total) * 100.0;
  
  // Clamp to reasonable range (0-100%)
  if (efficiency < 0.0) return 0.0;
  if (efficiency > 100.0) return 100.0;
  
  return efficiency;
}

// --- Fan Intensity Control ---
// Converts intensity level (1-10) to PWM percentage (12%-100%)
// Level 1 = 12% (minimum for fan startup)
// Level 10 = 100% (maximum speed)
inline float get_fan_speed_for_level(int level) {
  if (level < 1) level = 1;
  if (level > 10) level = 10;
  
  // Linear mapping: Level 1->12%, Level 10->100%
  // Formula: speed = 12 + (level - 1) * (100 - 12) / 9
  float speed = 12.0f + ((float)(level - 1) * 88.0f / 9.0f);
  return speed;
}

// Sets fan to specified intensity level
inline void set_fan_intensity_level(int level) {
  float speed_percent = get_fan_speed_for_level(level);
  
  auto call = id(lueftung_fan).turn_on();
  call.set_speed(static_cast<int>(speed_percent));
  call.perform();
  
  ESP_LOGI("fan_control", "Intensity Level %d -> %.1f%% speed", level, speed_percent);
}

// --- Mode Cycling ---
// Cycles through operating modes: 0=WRG, 1=Durchlüften, 2=Aus
inline void cycle_operating_mode(int mode_index) {
  auto *v = (esphome::VentilationController*)&id(ventilation_ctrl);
  
  switch(mode_index) {
    case 0:  // Wärmerückgewinnung
      v->set_mode(esphome::MODE_ECO_RECOVERY);
      ESP_LOGI("mode", "Switched to: Wärmerückgewinnung");
      break;
    case 1:  // Durchlüften
      v->set_mode(esphome::MODE_VENTILATION);
      ESP_LOGI("mode", "Switched to: Durchlüften");
      break;
    case 2:  // Aus
      v->set_mode(esphome::MODE_OFF);
      id(lueftung_fan).turn_off();
      ESP_LOGI("mode", "Switched to: Aus");
      break;
  }
}

// --- System Power Toggle ---
// --- System Power Toggle ---
// Toggles entire system on/off (fan, operations)
inline void toggle_system_power() {
  bool current_state = id(system_on);
  id(system_on) = !current_state;
  
  if (id(system_on)) {
    // Power ON
    ESP_LOGI("system", "System POWER ON");
    // Restore previous mode
    cycle_operating_mode(id(current_mode_index));
  } else {
    // Power OFF
    ESP_LOGI("system", "System POWER OFF");
    id(lueftung_fan).turn_off();
  }
}

// --- Status LED Update Logic ---
// Updates the 5 LEDs based on system state and fan level
// Mapped to MCP23017 outputs
inline void update_leds_logic() {
  // 1. Power LED (Always ON if system is ON)
  if (id(system_on)) {
    id(status_led_power).turn_on();
  } else {
     // Optional: Blink or Off? Let's keep it OFF for now
    id(status_led_power).turn_off();
  }

  // 2. Mode LEDs
  id(status_led_mode_wrg).turn_off();
  id(status_led_mode_vent).turn_off();
  
  if (id(system_on)) {
    if (id(selected_mode).state == "Wärmerückgewinnung") {
      id(status_led_mode_wrg).turn_on();
    } else if (id(selected_mode).state == "Durchlüften") {
       id(status_led_mode_vent).turn_on();
    }
  }

  // 3. Level LEDs (1-10 mapped to 5 LEDs)
  // Reset all first
  id(status_led_l1).turn_off();
  id(status_led_l2).turn_off();
  id(status_led_l3).turn_off();
  id(status_led_l4).turn_off();
  id(status_led_l5).turn_off();

  if (!id(system_on) || id(selected_mode).state == "Aus") return;

  int level = id(fan_intensity_level);
  
  // Mapping Logic (1-10 -> 5 LEDs)
  if (level >= 10) {
    id(status_led_l1).turn_on(); id(status_led_l2).turn_on();
    id(status_led_l3).turn_on(); id(status_led_l4).turn_on();
    id(status_led_l5).turn_on();
  } else {
     switch(level) {
       case 1: id(status_led_l1).turn_on(); break;
       case 2: id(status_led_l1).turn_on(); id(status_led_l2).turn_on(); break;
       case 3: id(status_led_l2).turn_on(); break;
       case 4: id(status_led_l2).turn_on(); id(status_led_l3).turn_on(); break;
       case 5: id(status_led_l3).turn_on(); break;
       case 6: id(status_led_l3).turn_on(); id(status_led_l4).turn_on(); break;
       case 7: id(status_led_l4).turn_on(); break;
       case 8: id(status_led_l4).turn_on(); id(status_led_l5).turn_on(); break;
       case 9: id(status_led_l5).turn_on(); break;
     }
  }
}
