// ==========================================================================
// VentoSync HRV – ESPHome Custom Component
// https://github.com/thomasengeroff-dotcom/VentoSync
//
// Copyright (c) 2025 Thomas Engeroff
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
// Description: Helper functions for climate monitoring and NTC filtering.
// Author:      Thomas Engeroff
// Created:     2025-03-29
// Modified:    2025-06-XX
// ==========================================================================
#pragma once
#include "globals.h"


/**
 * @brief   Maps a raw CO2 ppm value to a human-readable air quality string.
 */
inline std::string get_co2_classification(float co2_ppm) {
  if (std::isnan(co2_ppm) || co2_ppm < 0.0f) return "Unbekannt";
  if (co2_ppm > 10000.0f) return "Sensor-Fehler";
  return VentilationLogic::get_co2_classification(co2_ppm);
}


// =====================================================================
// NTC Stabilization Constants
// =====================================================================

/// Maximum allowed deviation (°C) within the sliding window for a reading
/// to be considered "stable". Derived from sensor accuracy (±0.2°C) plus
/// ADC quantization noise margin.
constexpr float NTC_MAX_DEVIATION = 0.3f;

/// Minimum thermal settling time (ms) after a direction change.
/// The ceramic heat exchanger core has high thermal inertia; NTC readings
/// during the transition period reflect core temperature, not air temperature.
constexpr uint32_t NTC_MIN_WAIT_MS = 15000;

/// Number of consecutive stable readings required before accepting a value.
constexpr size_t NTC_WINDOW_SIZE = 3;


// =====================================================================
// Shared Helper: Dynamic Wait Time Calculation
// =====================================================================

/**
 * @brief   Calculates the dynamic thermal wait time based on cycle duration.
 *
 * @details Uses 40% of the cycle as settling time, clamped between
 *          NTC_MIN_WAIT_MS and (cycle_ms - 5000ms) to guarantee a
 *          measurement window of at least 5 seconds per phase.
 *
 * @param[in] cycle_ms  Current cycle duration in milliseconds.
 * @return    Wait time in milliseconds.
 */
inline uint32_t calc_ntc_wait_ms(uint32_t cycle_ms) {
    if (cycle_ms == 0) return NTC_MIN_WAIT_MS;
    return std::clamp(
        static_cast<uint32_t>(cycle_ms * 0.4f),
        NTC_MIN_WAIT_MS,
        cycle_ms > 5000 ? cycle_ms - 5000 : cycle_ms / 2
    );
}


// =====================================================================
// Heat Recovery Efficiency Calculation
// =====================================================================

/**
 * @brief   Calculates the heat recovery efficiency fraction (0.0 – 100.0).
 *
 * @details Only samples data during stable "Zuluft" (intake) phases in
 *          WRG mode. This is necessary because the NTC sensors require
 *          time to stabilize as the ceramic heat exchanger reaches its
 *          thermal equilibrium after a direction flip.
 *
 * @param[in] t_raum        Indoor room temperature (from SCD41).
 * @param[in] t_zuluft_live Fresh air temperature after the exchanger (NTC).
 * @param[in] t_aussen      Outside ambient air temperature (NTC).
 * @param[in] current_eff   The last calculated efficiency (held during flips).
 * @return    Efficiency percentage [0.0, 100.0] or current_eff if not measurable.
 */
inline float calculate_heat_recovery_efficiency(float t_raum, float t_zuluft_live,
                                                float t_aussen,
                                                float current_eff) {
  if (ventilation_ctrl == nullptr)
    return NAN;

  const uint32_t now = millis();
  const auto mode = ventilation_ctrl->state_machine.current_mode;
  const auto hw = ventilation_ctrl->state_machine.get_target_state(now);

  const bool is_wrg = (mode == esphome::MODE_ECO_RECOVERY);
  const bool is_intake = hw.direction_in;

  // Abort if any sensor is disconnected or invalid
  if (std::isnan(t_zuluft_live) || std::isnan(t_aussen) || std::isnan(t_raum)) {
    return current_eff;
  }

  // Boot-Guard: No measurement in the first 60s to let sensors stabilize
  static bool boot_complete = false;
  if (!boot_complete) {
    if (now < 60000) return current_eff;
    boot_complete = true;
  }

  const uint32_t time_since_flip = now - last_direction_change_time;
  const bool had_real_flip = (last_direction_change_time > 0);

  // Dynamic wait time (shared formula via calc_ntc_wait_ms)
  const uint32_t cycle_ms = ventilation_ctrl->state_machine.cycle_duration_ms;
  const uint32_t wait_ms = calc_ntc_wait_ms(cycle_ms);

  const bool is_stable = had_real_flip && (time_since_flip > wait_ms);

  // Only measure during intake phase, AFTER thermal stabilization
  if (is_wrg && is_intake && is_stable) {
    float eff = VentilationLogic::calculate_heat_recovery_efficiency(
        t_raum, t_zuluft_live, t_aussen);

    if (std::isnan(eff) || std::isinf(eff)) {
        return current_eff;
    }

    // Plausible physical range: 0% to 110% (>100% possible with enthalpy effects)
    if (eff < 0.0f || eff > 110.0f) {
        ESP_LOGW("climate",
            "WRG efficiency out of plausible range: %.1f%% "
            "(T_raum=%.1f, T_zuluft_live=%.1f, T_aussen=%.1f)",
            eff, t_raum, t_zuluft_live, t_aussen);
        return current_eff;
    }

    // Soft-clamp for display purposes
    eff = std::clamp(eff, 0.0f, 100.0f);

    ESP_LOGD("climate", "WRG Efficiency update: %.1f%% (Room:%.1f Zuluft_Live:%.1f Out:%.1f)",
             eff, t_raum, t_zuluft_live, t_aussen);
    return eff;
  }

  return current_eff;
}


// =====================================================================
// Combined NTC Filter: Phase-Lock + Seasonal Selection + Stability
// =====================================================================

/**
 * @brief   Unified NTC filter combining phase-lock, seasonal min/max
 *          selection, and thermal stability validation.
 *
 * @details This function merges what was previously three separate filter
 *          stages (YAML Stufe 2 + Stufe 3) into a single, coherent pipeline.
 *          This is critical because the seasonal sliding window MUST only
 *          receive phase-correct values — otherwise the window gets
 *          contaminated with readings from the wrong airflow direction.
 *
 *          Filter pipeline (all in one call):
 *          1. Phase-Lock: Discard values from the wrong airflow direction
 *          2. Thermal Wait: Discard values during ceramic core settling
 *          3. History Invalidation: Clear window on direction change
 *          4. Sliding Window: Collect NTC_WINDOW_SIZE stable readings
 *          5. Stability Check: Verify deviation < NTC_MAX_DEVIATION
 *          6. Seasonal Selection: Pick min/max based on thermal environment
 *
 *          Thread safety: This function is called exclusively from the
 *          ESPHome main loop (sensor filter chain). All accessed globals
 *          (last_direction_change_time, ntc_history, ventilation_ctrl)
 *          are written only from the main loop — no mutex required.
 *
 * @param[in] sensor_idx  0 = temp_abluft (indoor), 1 = temp_zuluft (outdoor)
 * @param[in] new_value   Raw NTC temperature reading (post-clamp from YAML)
 * @param[in] ref_temp    Current state of the opposing sensor for seasonal logic
 *
 * @return  Selected temperature value, or empty optional to discard
 */
inline esphome::optional<float> filter_ntc_combined(int sensor_idx,
                                                     float new_value,
                                                     float ref_temp) {
    // ── Input validation ────────────────────────────────────
    if (std::isnan(new_value)) return {};

    if (sensor_idx < 0 || sensor_idx > 1) {
        ESP_LOGW("ntc_filter", "Invalid sensor_idx: %d", sensor_idx);
        return new_value;
    }

    // Graceful degradation: if controller not yet initialized, pass through
    if (ventilation_ctrl == nullptr) return new_value;

    // ── 1. Phase-Lock ───────────────────────────────────────
    // NTC 0 (Indoor/Abluft): only valid during exhaust phase
    //   → During intake, sensor reads heated/cooled exchanger air
    // NTC 1 (Outdoor/Zuluft): only valid during intake phase
    //   → During exhaust, sensor reads warmed air from exchanger
    const auto hw = ventilation_ctrl->state_machine.get_target_state(millis());
    if (sensor_idx == 0 && hw.direction_in) return {};
    if (sensor_idx == 1 && !hw.direction_in) return {};

    // ── 2. Cycle sanity check ───────────────────────────────
    const uint32_t cycle_ms = ventilation_ctrl->state_machine.cycle_duration_ms;
    if (cycle_ms == 0) return new_value;

    if (cycle_ms < NTC_MIN_WAIT_MS) {
        ESP_LOGW("ntc_filter",
            "Cycle too short (%ums < %ums min) for NTC stabilization. "
            "Filter bypassed — raw values will be used.",
            cycle_ms, NTC_MIN_WAIT_MS);
        return new_value;
    }

    // ── 3. History invalidation on direction change ─────────
    static uint32_t last_known_flip[2] = {0, 0};
    auto &history = ntc_history[sensor_idx];

    if (last_known_flip[sensor_idx] != last_direction_change_time) {
        history.clear();
        last_known_flip[sensor_idx] = last_direction_change_time;
        ESP_LOGD("ntc_filter", "NTC[%d] history cleared after direction change",
                 sensor_idx);
    }

    // ── 4. Thermal wait (ceramic core settling) ─────────────
    const uint32_t wait_ms = calc_ntc_wait_ms(cycle_ms);
    if (millis() - last_direction_change_time < wait_ms) return {};

    // ── 5. Sliding window accumulation ──────────────────────
    history.push_back(new_value);
    if (history.size() > NTC_WINDOW_SIZE) {
        history.pop_front();
    }

    if (history.size() < NTC_WINDOW_SIZE) return {};

    // ── 6. Stability check ──────────────────────────────────
    auto [min_it, max_it] = std::minmax_element(history.begin(), history.end());
    const float min_v = *min_it;
    const float max_v = *max_it;

    if ((max_v - min_v) > NTC_MAX_DEVIATION) {
        ESP_LOGD("ntc_filter",
            "NTC[%d] unstable: spread=%.2f°C (max=%.2f, limit=%.2f)",
            sensor_idx, max_v - min_v, max_v, NTC_MAX_DEVIATION);
        return {};
    }

    // ── 7. Seasonal min/max selection ───────────────────────
    // The HRV housing creates a thermal bias on the NTC sensors:
    //
    // INDOOR sensor (channel 0):
    //   Winter: Cold outdoor air cools the housing → reads LOW → use MAX
    //   Summer: Warm outdoor air heats the housing → reads HIGH → use MIN
    //
    // OUTDOOR sensor (channel 1):
    //   Winter: True cold readings are most accurate → use MIN
    //   Summer: Housing can't cool below room temp → use MAX
    //
    // Fallback: When reference sensor is NaN, use MEDIAN as safest
    // compromise — avoids committing to a wrong seasonal assumption.

    if (std::isnan(ref_temp)) {
        std::array<float, 3> sorted;
        std::copy(history.begin(), history.end(), sorted.begin());
        std::sort(sorted.begin(), sorted.end());
        ESP_LOGD("ntc_filter",
            "NTC[%d] ref sensor unavailable, using median: %.1f",
            sensor_idx, sorted[1]);
        return sorted[1];
    }

    if (sensor_idx == 0) {
        // Indoor: ref_temp = outdoor temperature
        return (ref_temp > new_value) ? min_v : max_v;
    } else {
        // Outdoor: ref_temp = indoor temperature
        return (new_value > ref_temp) ? max_v : min_v;
    }
}
