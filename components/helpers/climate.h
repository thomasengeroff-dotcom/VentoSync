// ==========================================================================
// VentoSync HRV – ESPHome Custom Component
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
// File:        climate.h
// Description: Helper functions for climate monitoring.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "globals.h"

/**
 * @brief   Maps a raw CO2 ppm value to a human-readable air quality string.
 */
inline std::string get_co2_classification(float co2_ppm) {
  if (std::isnan(co2_ppm) || co2_ppm < 0.0f) return "Unbekannt";
  if (co2_ppm > 10000.0f) return "Sensor-Fehler"; // Physikalisch unplausibel
  return VentilationLogic::get_co2_classification(co2_ppm);
}

/**
 * @brief   Calculates the heat recovery efficiency fraction (0.0 – 1.0).
 *
 * @details Only samples data during stable "Zuluft" (intake) phases in
 *          WRG mode. This is necessary because the NTC sensors require
 *          time to stabilize as the ceramic heat exchanger reaches its
 *          thermal equilibrium after a direction flip.
 *
 * @param[in] t_raum      Indoor room temperature.
 * @param[in] t_zuluft    Fresh air temperature (after the exchanger).
 * @param[in] t_aussen    Outside ambient air temperature.
 * @param[in] current_eff The last calculated efficiency (to hold during flips).
 */
inline float calculate_heat_recovery_efficiency(float t_raum, float t_zuluft,
                                                float t_aussen,
                                                float current_eff) {
  if (ventilation_ctrl == nullptr)
    return NAN;

  const uint32_t now = millis();
  const auto mode = ventilation_ctrl->state_machine.current_mode;
  const auto hw = ventilation_ctrl->state_machine.get_target_state(now);

  const bool is_wrg = (mode == esphome::MODE_ECO_RECOVERY);
  const bool is_intake = hw.direction_in;

  // Boot-Guard: Keine Messung in den ersten 60s nach Boot
  static bool boot_complete = false;
  if (!boot_complete) {
    if (now < 60000) return current_eff;
    boot_complete = true;
  }

  // Thermal stabilization: Wait at least 30s into the cycle before sampling
  constexpr uint32_t stable_time_ms = 30000;
  const uint32_t time_since_flip = now - last_direction_change_time;
  // Sicherstellen dass mindestens EIN echter Richtungswechsel stattgefunden hat
  const bool had_real_flip = (last_direction_change_time > 0);
  const bool is_stable = had_real_flip && (time_since_flip > stable_time_ms);

  if (is_wrg && is_intake && is_stable) {
    if (std::isnan(t_raum) || std::isnan(t_zuluft) || std::isnan(t_aussen)) {
      ESP_LOGD("climate", "WRG Efficiency: Sensor data NaN, holding: %.1f%%", current_eff);
      return current_eff;
    }
    float eff = VentilationLogic::calculate_heat_recovery_efficiency(
        t_raum, t_zuluft, t_aussen);
        
    // NaN abfangen (z.B. wenn T_raum == T_aussen -> Division durch 0)
    if (std::isnan(eff) || std::isinf(eff)) {
        ESP_LOGW("climate", 
            "WRG efficiency calculation returned invalid value "
            "(T_raum=%.1f, T_zuluft=%.1f, T_aussen=%.1f) — holding %.1f%%",
            t_raum, t_zuluft, t_aussen, current_eff * 100.0f);
        return current_eff;
    }

    // Physikalisch sinnvoller Bereich: 0% bis 110%
    if (eff < 0.0f || eff > 1.1f) {
        ESP_LOGW("climate",
            "WRG efficiency out of plausible range: %.1f%% "
            "(T_raum=%.1f, T_zuluft=%.1f, T_aussen=%.1f)",
            eff * 100.0f, t_raum, t_zuluft, t_aussen);
        return current_eff; // Halte letzten gültigen Wert
    }

    // Soft-Clamp für Anzeige: Werte zwischen 0% und 100%
    eff = std::clamp(eff, 0.0f, 1.0f);

    ESP_LOGD("climate", "WRG Efficiency update: %.1f%% (Room:%.1f Zuluft:%.1f Out:%.1f)", 
             eff * 100.0f, t_raum, t_zuluft, t_aussen);
    return eff;
  }

  // Debugging holding reasons (only in WRG mode to avoid log spam)
  if (is_wrg) {
    if (!is_intake) {
      ESP_LOGD("climate", "WRG Efficiency: Holding during exhaust phase (Abluft)");
    } else if (!is_stable) {
      ESP_LOGD("climate", "WRG Efficiency: Waiting for stabilization (%us remaining)", 
               (stable_time_ms - time_since_flip) / 1000);
    }
  }

  // Not in a valid sampling window → hold last known value
  return current_eff;
}

// Stabilization parameter constants
constexpr float NTC_MAX_DEVIATION = 0.3f;
constexpr uint32_t NTC_MIN_WAIT_MS = 15000;
constexpr size_t NTC_WINDOW_SIZE = 3;

/**
 * @brief   NTC Sliding Window Stabilization Filter.
 *
 * @details Implements a mandatory thermal adjustment wait time (min 15s)
 *          followed by a sliding window standard deviation check. 
 *          This is critical because NTC sensors are thermally coupled
 *          to the ceramic core, which has high thermal inertia and
 *          requires time to "settle" after each direction change.
 *
 * @param[in] sensor_idx  0 for temp_zuluft, 1 for temp_abluft.
 * @param[in] new_value   The raw value reported by the physical NTC component.
 *
 * @return  The original value if stable, else an empty optional to discard the update.
 */
inline esphome::optional<float> filter_ntc_stable(int sensor_idx,
                                                  float new_value) {
  if (std::isnan(new_value)) {
    return {};
  }

  // Bounds-Check für sensor_idx um Out-of-Bounds Zugriffe zu verhindern
  if (sensor_idx < 0 || sensor_idx > 1) {
    return new_value; 
  }

  if (ventilation_ctrl == nullptr) {
    return new_value;
  }

  const uint32_t cycle_ms = ventilation_ctrl->state_machine.cycle_duration_ms;
  if (cycle_ms == 0) return new_value;

  // Zu kurze Zyklen: Filter kann nicht sinnvoll arbeiten
  if (cycle_ms < NTC_MIN_WAIT_MS) {
      ESP_LOGW("ntc_filter",
          "Cycle too short (%ums < %ums min) for NTC stabilization. "
          "Filter bypassed — raw values will be used.",
          cycle_ms, NTC_MIN_WAIT_MS);
      return new_value; 
  }

  // --- NEU: History bei Richtungswechsel invalidieren ---
  static uint32_t last_known_direction_change[2] = {0, 0};
  
  auto &history = ntc_history[sensor_idx];

  if (last_known_direction_change[sensor_idx] != last_direction_change_time) {
      history.clear();
      last_known_direction_change[sensor_idx] = last_direction_change_time;
      ESP_LOGD("ntc_filter", "NTC[%d] history cleared after direction change", sensor_idx);
  }

  // Dynamic wait time: 40% of the cycle, aber mindestens 15 Sekunden.
  // Maximal cycle_ms - 5s, um noch Zeit zum Messen zu haben.
  uint32_t wait_ms = std::clamp(
      static_cast<uint32_t>(cycle_ms * 0.4f),
      NTC_MIN_WAIT_MS,
      cycle_ms > 5000 ? cycle_ms - 5000 : cycle_ms / 2
  );

  // 1. Check if we are still within the mandatory thermal adjustment wait time
  if (millis() - last_direction_change_time < wait_ms) {
    return {}; 
  }

  // 2. Add current value to history window
  history.push_back(new_value);
  if (history.size() > NTC_WINDOW_SIZE) {
    history.pop_front();
  }

  // 3. Wait until the window is full for a reliable stabilization check
  if (history.size() < NTC_WINDOW_SIZE) {
    return {}; 
  }

  // 4. Calculate deviation via STL
  auto [min_it, max_it] = std::minmax_element(history.begin(), history.end());
  
  // 5. Evaluate stability
  if ((*max_it - *min_it) <= NTC_MAX_DEVIATION) {
    return new_value;
  } else {
    return {};
  }
}