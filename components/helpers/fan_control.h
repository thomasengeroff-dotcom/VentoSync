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
// File:        fan_control.h
// Description: Hardware fan speed and direction management.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "globals.h"

// --- Constants ---
constexpr float MIN_PHYSICAL_RPM = 10.0f; // Minimum valid RPM reading from tacho
constexpr float RAMPING_UPPER_BOUND = 0.99f; // Upper bound for active ramping phase
constexpr float RAMPING_LOWER_BOUND = 0.01f; // Lower bound for active ramping phase

/**
 * @brief   Sets the physical fan motor PWM level and direction.
 *
 * @details Translates the target speed and direction into a hardware-specific
 *          PWM duty cycle. Uses VentilationLogic to handle the complexities
 *          of bidirectional motor drivers.
 *
 * @param[in] speed      Target speed as duty cycle fraction (0.1 to 1.0)
 * @param[in] direction  0 = Exhaust (Raus), 1 = Intake (Rein)
 *
 * @see     VentilationLogic::calculate_fan_pwm()
 */
inline void set_fan_logic(float speed, int direction) {
  if (fan_pwm_primary == nullptr) return;

  const float pwm = VentilationLogic::calculate_fan_pwm(speed, direction);
  fan_pwm_primary->set_level(pwm);
  last_fan_pwm_level = pwm;
}

/**
 * @brief Converts a user-facing intensity level (1-10) to a hardware speed fraction.
 * @param level User-facing fan level (1.0 to 10.0)
 * @return Target speed fraction (0.1 to 1.0)
 */
inline float level_to_speed(float level) {
  return VentilationLogic::calculate_fan_speed_from_intensity(static_cast<int>(std::round(level)));
}

/**
 * @brief Extracts base parameters for manual speed calculations (with presence compensation).
 * @param base_intensity Current UI slider intensity
 */
inline float calculate_manual_demand(float base_intensity) {
  float intensity = base_intensity;
  
  if (radar_presence != nullptr && radar_presence->has_state() && radar_presence->state) {
    if (auto_presence_val != nullptr) {
      const float comp = static_cast<float>(auto_presence_val->value());
      if (comp != 0.0f) {
        intensity += comp;
        ESP_LOGD("fan", "Applying presence compensation: %.1f (Base: %.1f)", comp, base_intensity);
      }
    }
  }
  return std::clamp(intensity, 1.0f, 10.0f);
}

/**
 * @brief   Calculates the exact target speed based on active mode logic.
 *
 * @details This is the central decision point for speed. In manual modes,
 *          it applies presence compensation; in Smart-Automatik, it
 *          delegates to the PID results.
 *
 * @return  float  Target hardware speed fraction (0.0 to 1.0).
 */
inline float get_current_target_speed() {
  // If the fan component is logically OFF (e.g. Window Guard active or manual OFF),
  // we must return 0.0 to allow the slew-rate limiter to ramp it down.
  if (lueftung_fan != nullptr && !lueftung_fan->state) {
    return 0.0f;
  }
  if (fan_intensity_level == nullptr) return 0.0f;

  float intensity = static_cast<float>(fan_intensity_level->value());

  // Only apply presence compensation in manual modes.
  // In Automatic mode, evaluate_auto_mode() already handles all factors.
  if (auto_mode_active == nullptr || !auto_mode_active->value()) {
    intensity = calculate_manual_demand(intensity);
  }

  // Final safety clamp to valid 1.0-10.0 level domain
  intensity = std::clamp(intensity, 1.00f, 10.00f);
  return level_to_speed(intensity);
}

/**
 * @brief   Calculates the virtual fan RPM.
 *
 * @details Prioritizes physical tacho signals (4-pin fans) if available.
 *          If no physical signal is present, it uses a software model
 *          based on current PWM and ramping state to simulate the RPM
 *          for the WRG dashboard.
 *
 * @param[in] raw_rpm  The physical RPM reading from the tacho pulse counter.
 *
 * @return  float  Signed virtual RPM (negative for exhaust/raus).
 */
inline float calculate_virtual_fan_rpm(float raw_rpm) {
  // 1. If physical tacho signal is valid (4-pin fan), use it immediately
  if (raw_rpm > MIN_PHYSICAL_RPM) {
    bool direction_in = true;
    if (ventilation_ctrl != nullptr) {
      direction_in = ventilation_ctrl->state_machine.get_target_state(millis())
                         .direction_in;
    } else if (fan_direction != nullptr) {
      direction_in = fan_direction->state;
    }
    return direction_in ? raw_rpm : -raw_rpm;
  }

  // 2. Fallback for 3-pin fans (virtual RPM calculation)
  const uint32_t now = millis();
  float ramp_factor = 1.0f;
  bool direction_in = true;

  if (ventilation_ctrl != nullptr) {
    const esphome::HardwareState state =
        ventilation_ctrl->state_machine.get_target_state(now);
    ramp_factor = state.ramp_factor;
    direction_in = state.direction_in;
  } else if (fan_direction != nullptr) {
    direction_in = fan_direction->state;
  }

  const float speed = get_current_target_speed();
  return VentilationLogic::calculate_virtual_fan_rpm(speed, direction_in,
                                                     ramp_factor);
}

/**
 * @brief   Main periodic tick for physical fan management.
 *
 * @details Dispatches calculated cycle timings and ramping speeds to hardware.
 *          Implements "Sanftanlauf" (soft-start) via a slew-rate limiter
 *          (~10% per second) to reduce mechanical stress on the motor and
 *          minimize audible noise transitions.
 */
inline void update_fan_logic() {
  if (fan_intensity_level == nullptr) return;
  
  const uint32_t now = millis();
  const int intensity = static_cast<int>(fan_intensity_level->value());

  // 1. Handle Dynamic Cycle duration scaling
  if (ventilation_ctrl != nullptr) {
    const uint32_t dynamic_cycle_ms = VentilationLogic::calculate_dynamic_cycle_duration(intensity);
    if (ventilation_ctrl->state_machine.cycle_duration_ms != dynamic_cycle_ms) {
      // W7: Pass false to skip redundant update_hardware() call since we are already in one.
      ventilation_ctrl->set_cycle_duration(dynamic_cycle_ms, false);
    }
  }

  // 2. Base Speed Calculation with Smooth Slew-Rate (Sanftanlauf)
  const float base_speed = get_current_target_speed();
  
  if (last_slew_update_ms == 0) {
    current_smoothed_speed = base_speed;
    last_slew_update_ms = now;
  }
  
  uint32_t dt = now - last_slew_update_ms;
  last_slew_update_ms = now;
  
  if (dt > 1000) dt = 1000; // Cap dt after long pauses
  
  // Slew rate: ~10% (0.10) per second
  const float slew_rate = 0.10f * (float)dt / 1000.0f;
  
  if (std::abs(base_speed - current_smoothed_speed) <= slew_rate) {
    current_smoothed_speed = base_speed;
  } else if (base_speed > current_smoothed_speed) {
    current_smoothed_speed += slew_rate;
  } else {
    current_smoothed_speed -= slew_rate;
  }

  float speed = current_smoothed_speed;

  // 3. Hardware Ramping Application
  if (ventilation_ctrl != nullptr) {
    const esphome::HardwareState state = ventilation_ctrl->state_machine.get_target_state(now);
    speed *= state.ramp_factor;

    // Rate-limited log during transition phases
    if (state.ramp_factor < RAMPING_UPPER_BOUND && state.ramp_factor > RAMPING_LOWER_BOUND &&
        (now - ventilation_ctrl->last_ramp_log_ms > 1000)) {
      ESP_LOGD("fan_ramp", "Ramping speed: %.2f (Base: %.2f, Factor: %.2f)",
               speed, base_speed, state.ramp_factor);
      ventilation_ctrl->last_ramp_log_ms = now;
      ventilation_ctrl->was_ramping = true;
    } else if (ventilation_ctrl->was_ramping && (state.ramp_factor >= RAMPING_UPPER_BOUND || state.ramp_factor <= RAMPING_LOWER_BOUND)) {
      ESP_LOGD("fan_ramp", "Ramping complete. Target speed reached: %.2f (Factor: %.2f)",
               speed, state.ramp_factor);
      ventilation_ctrl->was_ramping = false;
    }
  }

  // 4. Direction Resolution
  // Read current direction from the fan_direction switch component pseudo-state:
  // OFF = Direction: Abluft (Raus) (PWM < 50%)
  // ON  = Direction: Zuluft (Rein) (PWM > 50%)
  const int direction = (fan_direction != nullptr && fan_direction->state) ? 1 : 0;

  set_fan_logic(speed, direction);
}

/**
 * @brief Call this when fan direction (switch) changes to reset thermal stabilization.
 */
inline void notify_fan_direction_changed() {
  last_direction_change_time = millis();
  ntc_history[0].clear();
  ntc_history[1].clear();
  ESP_LOGD("ntc_filter",
           "Fan direction changed. Resetting NTC history buffers.");
}

/**
 * @brief Checks if the given manual slider intensity falls below the active threshold.
 * @param value Slider intensity position.
 */
inline bool is_fan_slider_off(float value) {
  return VentilationLogic::is_fan_slider_off(value);
}

/**
 * @brief Supplies a linear ramp-up curve for soft motor starts.
 * @param iteration Cycle step (0-99).
 */
inline float calculate_ramp_up(int iteration) {
  return VentilationLogic::calculate_ramp_up(iteration);
}

/**
 * @brief Supplies a linear ramp-down curve for soft motor stops.
 * @param iteration Cycle step (0-99).
 */
inline float calculate_ramp_down(int iteration) {
  return VentilationLogic::calculate_ramp_down(iteration);
}
