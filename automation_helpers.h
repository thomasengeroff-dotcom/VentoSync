#pragma once

#include "esphome.h"
#include <string>
#include <vector>

// Core and Component Headers
#include "esphome/components/globals/globals_component.h"
#include "esphome/components/script/script.h"
#include "esphome/components/template/select/template_select.h"
#include "esphome/components/template/number/template_number.h"
#include "esphome/components/template/switch/template_switch.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/light/light_state.h"
// Custom Component Header
#include "esphome/components/ventilation_group/ventilation_group.h"

// Extern Declarations (making components available)
extern esphome::globals::GlobalsComponent<bool> *system_on;
extern esphome::globals::GlobalsComponent<bool> *ventilation_enabled;
extern esphome::globals::GlobalsComponent<int> *current_mode_index;
extern esphome::globals::GlobalsComponent<int> *fan_intensity_level;

extern esphome::template_::TemplateSelect *selected_mode;
extern esphome::template_::TemplateSelect *fan_mode_select;
extern esphome::template_::TemplateNumber *vent_timer;
extern esphome::template_::TemplateNumber *fan_intensity_display;
extern esphome::template_::TemplateSwitch *fan_direction;

extern esphome::script::RestartScript<> *update_leds;
extern esphome::script::RestartScript<> *fan_speed_update;
extern esphome::script::SingleScript<float, int> *set_fan_speed_and_direction;

extern esphome::speed::SpeedFan *lueftung_fan;
extern esphome::ledc::LEDCOutput *fan_pwm_primary;
extern esphome::ledc::LEDCOutput *fan_pwm_secondary;

extern esphome::light::LightState *status_led_l1;
extern esphome::light::LightState *status_led_l2;
extern esphome::light::LightState *status_led_l3;
extern esphome::light::LightState *status_led_l4;
extern esphome::light::LightState *status_led_l5;
extern esphome::light::LightState *status_led_power;
extern esphome::light::LightState *status_led_master; // Not used?
extern esphome::light::LightState *status_led_mode_wrg;
extern esphome::light::LightState *status_led_mode_vent;

// Ventilation Controller
extern esphome::VentilationController *ventilation_ctrl;

// --- IAQ Classification ---
inline std::string get_iaq_classification(float iaq_val) {
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

// --- Traffic Light ---
// Used for ESP-NOW visualization
inline std::vector<uint8_t> get_iaq_traffic_light_data(float iaq_val) {
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
  
  std::vector<uint8_t> data = {r, g, b};
  return data;
}

// --- Heat Recovery Efficiency ---
inline float calculate_heat_recovery_efficiency(float t_raum, float t_zuluft, float t_aussen) {
  // Efficiency = (T_supply - T_outside) / (T_indoor - T_outside) * 100
  // Avoid division by zero
  if (abs(t_raum - t_aussen) < 1.0) {
    return 0.0;
  }
  float eff = (t_zuluft - t_aussen) / (t_raum - t_aussen) * 100.0;
  if (eff < 0) return 0.0;
  if (eff > 100) return 100.0; // Cap at 100% (can happen with sensor noise)
  return eff;
}

// --- Fan Slider Logic ---
// Off/min-threshold handling
// Replaced complex lambda in slider set_action
inline bool is_fan_slider_off(float value) {
    return value < 1.0; 
}

// --- Fan Cycle Logic (Ramp Functions) ---
// Used by fan_cycle_logic script
inline float calculate_ramp_up(int iteration) {
    // 0 to 100 in 100 steps
    return (float)iteration / 100.0f;
}

inline float calculate_ramp_down(int iteration) {
    // 100 to 0 in 100 steps
    return (100.0f - (float)iteration) / 100.0f;
}

// --- Mode Cycling ---
// Cycles through operating modes: 0=WRG, 1=Stoßlüftung, 2=Durchlüften, 3=Aus
inline void cycle_operating_mode(int mode_index) {
  auto *v = ventilation_ctrl;
  
  switch(mode_index) {
    case 0:  // Wärmerückgewinnung
      v->set_mode(esphome::MODE_ECO_RECOVERY);
      break;
    case 1:  // Stoßlüftung
      v->set_mode(esphome::MODE_STOSSLUEFTUNG);
      break;
    case 2:  // Durchlüften (Timer)
      // Default 30 min if triggered by button without timer context?
      // Or use configured timer? Let's use 30 min here for button cycle simplicity
      // Or read the timer value?
      // Using vent_timer state:
      v->set_mode(esphome::MODE_VENTILATION, (uint32_t)(vent_timer->state * 60 * 1000));
      break;
    case 3:  // Aus
      v->set_mode(esphome::MODE_OFF);
      break;
  }
}

// --- Status LED Update Logic ---
// Updates the 5 LEDs based on system state and fan level
// Uses VentilationController mode (integer) instead of fragile string comparisons
inline void update_leds_logic() {
  auto *v = ventilation_ctrl;
  int current_mode = v->current_mode;

  // 1. Power LED (Always ON if system is ON)
  if (system_on->value()) {
    status_led_power->turn_on();
  } else {
     // Optional: Blink or Off? Let's keep it OFF for now
    status_led_power->turn_off();
  }

  // 2. Mode LEDs
  status_led_mode_wrg->turn_off();
  status_led_mode_vent->turn_off();
  
  if (system_on->value()) {
    if (current_mode == esphome::MODE_ECO_RECOVERY) {
      // WRG: Left LED on
      status_led_mode_wrg->turn_on();
    } else if (current_mode == esphome::MODE_STOSSLUEFTUNG) {
      // Stoßlüftung: Right LED on
      status_led_mode_vent->turn_on();
    } else if (current_mode == esphome::MODE_VENTILATION) {
      // Durchlüften: Both LEDs on
      status_led_mode_wrg->turn_on();
      status_led_mode_vent->turn_on();
    }
    // MODE_OFF: Both LEDs off (already reset above)
  }

  // 3. Level LEDs (1-10 mapped to 5 LEDs)
  // Reset all first
  status_led_l1->turn_off();
  status_led_l2->turn_off();
  status_led_l3->turn_off();
  status_led_l4->turn_off();
  status_led_l5->turn_off();

  if (!system_on->value() || current_mode == esphome::MODE_OFF) return;

  int level = fan_intensity_level->value();
  
  // Mapping Logic (1-10 -> 5 LEDs)
  if (level >= 10) {
    status_led_l1->turn_on(); status_led_l2->turn_on();
    status_led_l3->turn_on(); status_led_l4->turn_on();
    status_led_l5->turn_on();
  } else {
     switch(level) {
       case 1: status_led_l1->turn_on(); break;
       case 2: status_led_l1->turn_on(); status_led_l2->turn_on(); break;
       case 3: status_led_l2->turn_on(); break;
       case 4: status_led_l2->turn_on(); status_led_l3->turn_on(); break;
       case 5: status_led_l3->turn_on(); break;
       case 6: status_led_l3->turn_on(); status_led_l4->turn_on(); break;
       case 7: status_led_l4->turn_on(); break;
       case 8: status_led_l4->turn_on(); status_led_l5->turn_on(); break;
       case 9: status_led_l5->turn_on(); break;
     }
  }
}

// --- Helper for ESP-NOW Receive Logic ---
// Parsons packet and synchronizes state, updating all UI elements
inline void handle_espnow_receive(std::vector<uint8_t> data) {
    auto *v = ventilation_ctrl;
    bool changed = v->on_packet_received(data);

    if (changed) {
        // 1. Sync Fan Intensity
        if (v->current_fan_intensity != fan_intensity_level->value()) {
            fan_intensity_level->value() = v->current_fan_intensity;
            fan_intensity_display->publish_state(v->current_fan_intensity);
        }

        // 2. Sync Mode & Globals
        int new_mode_idx = 0;
        std::string mode_str = "Wärmerückgewinnung";

        if (v->current_mode == esphome::MODE_OFF) {
            new_mode_idx = 3; mode_str = "Aus";
            ventilation_enabled->value() = false;
            system_on->value() = false;
        } else {
            ventilation_enabled->value() = true;
            system_on->value() = true;
            if (v->current_mode == esphome::MODE_ECO_RECOVERY) { new_mode_idx = 0; mode_str = "Wärmerückgewinnung"; }
            else if (v->current_mode == esphome::MODE_STOSSLUEFTUNG) { new_mode_idx = 1; mode_str = "Stoßlüftung"; }
            else if (v->current_mode == esphome::MODE_VENTILATION) { new_mode_idx = 2; mode_str = "Durchlüften"; }
        }

        current_mode_index->value() = new_mode_idx;

        // Update Select Component if different
        if (selected_mode->current_option() != mode_str) {
            selected_mode->publish_state(mode_str);
        }

        // Update Visuals
        update_leds->execute();
        fan_speed_update->execute();
    }
}

// --- Number Helpers ---

inline void set_ventilation_timer(float value) {
    auto *v = ventilation_ctrl;
    uint32_t ms = (uint32_t)value * 60 * 1000;
    // Update the duration but keep current mode
    v->ventilation_duration_ms = ms;
}

inline void set_cycle_duration_handler(float value) {
    auto *v = ventilation_ctrl;
    v->set_cycle_duration((uint32_t)value * 1000);
}

inline void set_sync_interval_handler(float value) {
    auto *v = ventilation_ctrl;
    v->set_sync_interval((uint32_t)value * 60 * 1000);
}

inline void set_fan_intensity_slider(float value) {
    int val = (int)value;
    fan_intensity_level->value() = val;
    // set_fan_intensity_level(val); // Removed as undefined
    
    ventilation_ctrl->set_fan_intensity(val);
    update_leds->execute();
}

// --- Select Helper ---

inline void set_operating_mode_select(std::string x) {
    auto *v = ventilation_ctrl;
    if (x == "Wärmerückgewinnung") {
        v->set_mode(esphome::MODE_ECO_RECOVERY);
    } else if (x == "Stoßlüftung") {
        v->set_mode(esphome::MODE_STOSSLUEFTUNG);
    } else if (x == "Durchlüften") {
        // Use the configured timer duration (0 = Infinite)
        uint32_t duration = (uint32_t)(vent_timer->state * 60 * 1000);
        v->set_mode(esphome::MODE_VENTILATION, duration);
    } else if (x == "Aus") {
        v->set_mode(esphome::MODE_OFF);
    }
    update_leds->execute();
}

// --- Button Helper Functions ---

inline void handle_button_mode_click() {
    current_mode_index->value() = (current_mode_index->value() + 1) % 4;
    cycle_operating_mode(current_mode_index->value());
    update_leds->execute();
}

inline void handle_button_power_click() {
    ventilation_enabled->value() = !ventilation_enabled->value();
    if (ventilation_enabled->value()) {
        // Power ON: Restore previous mode and fan speed
        ESP_LOGI("power", "System ON - restoring mode %d", current_mode_index->value());
        cycle_operating_mode(current_mode_index->value());
        fan_speed_update->execute();
    } else {
        // Power OFF: Stop fan and set mode to OFF
        ESP_LOGI("power", "System OFF");
        auto *v = ventilation_ctrl;
        v->set_mode(esphome::MODE_OFF);
        lueftung_fan->turn_off();
        fan_pwm_primary->set_level(0.0);
        fan_pwm_secondary->set_level(0.0);
    }
    update_leds->execute();
}

inline void handle_button_level_click() {
    if (!ventilation_enabled->value()) return;  // Ignore if system is off
    int level = fan_intensity_level->value();
    level = (level % 10) + 1;  // 1→2→...→10→1
    fan_intensity_level->value() = level;
    fan_intensity_display->publish_state(level);
    
    ventilation_ctrl->set_fan_intensity(level);
    fan_speed_update->execute();
    update_leds->execute();
    ESP_LOGI("button", "Intensity level: %d", level);
}
