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
// File:        vacation_helpers.h
// Description: Vacation mode activation/deactivation logic.
//              Manages state snapshots and restoration of pre-vacation
//              operating parameters.
// Author:      Thomas Engeroff
// Created:     2026-05-16
// Modified:    2026-05-16
//
// Dependencies: globals.h (MODE_NAMES, MODE_NAME_BOOST, MODE_NAME_AUTO)
//               automation_helpers.h (set_operating_mode_select,
//                                     set_fan_intensity_slider)
// ==========================================================================

#pragma once

#include "esphome.h"

// ---------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------

namespace ventosync {
namespace vacation {

/// Valid intensity range for vacation mode configuration.
static constexpr float INTENSITY_MIN = 1.0f;
static constexpr float INTENSITY_MAX = 10.0f;

/// Default fallback intensity when HA entity is uninitialized or out of range.
static constexpr float INTENSITY_FALLBACK = 1.0f;

}  // namespace vacation
}  // namespace ventosync

// ---------------------------------------------------------
// VACATION MODE – ACTIVATION
// ---------------------------------------------------------

/**
 * @brief Activates vacation mode with configured parameters from Home Assistant.
 *
 * Workflow:
 *   1. Snapshots current operating mode index and fan intensity for later
 *      restoration (stored in globals: pre_vacation_mode_index,
 *      pre_vacation_intensity).
 *   2. Reads target mode and intensity from HA config entities
 *      (vacation_mode_select, vacation_intensity_number).
 *   3. Applies safe fallback defaults if entities are uninitialized
 *      (empty string → MODE_NAME_BOOST, out-of-range intensity → 1.0).
 *   4. Applies the vacation configuration via the unified setters.
 *
 * @note Called from script `handle_vacation_mode_on` in logic_automation.yaml.
 * @note This function is idempotent – calling it multiple times while already
 *       in vacation mode simply re-snapshots the (already vacation) state.
 *       The YAML layer should guard against redundant calls.
 */
inline void activate_vacation_mode() {
    using namespace ventosync::vacation;

    // ── 1. Snapshot current state for restoration on deactivation ──
    id(pre_vacation_mode_index) = id(current_mode_index);
    id(pre_vacation_intensity)  = id(fan_intensity_level);

    ESP_LOGD("vacation", "State snapshot: mode_index=%d, intensity=%d",
             id(current_mode_index),
             static_cast<int>(id(fan_intensity_level)));

    // ── 2. Read configured vacation parameters from HA entities ──
    std::string target_mode = id(vacation_mode_select).current_option();
    float target_intensity  = id(vacation_intensity_number).state;

    // ── 3. Validate and apply fallbacks ──
    if (target_mode.empty()) {
        ESP_LOGW("vacation", "Vacation mode entity empty, falling back to '%s'",
                 MODE_NAME_BOOST);
        target_mode = MODE_NAME_BOOST;
    }

    if (target_intensity < INTENSITY_MIN || target_intensity > INTENSITY_MAX) {
        ESP_LOGW("vacation",
                 "Vacation intensity %.1f out of range [%.0f–%.0f], "
                 "falling back to %.0f",
                 target_intensity, INTENSITY_MIN, INTENSITY_MAX,
                 INTENSITY_FALLBACK);
        target_intensity = INTENSITY_FALLBACK;
    }

    // ── 4. Apply vacation configuration ──
    set_operating_mode_select(target_mode);
    set_fan_intensity_slider(target_intensity);

    ESP_LOGI("vacation", "Vacation Mode ACTIVATED: mode='%s', intensity=%.0f",
             target_mode.c_str(), target_intensity);
}

// ---------------------------------------------------------
// VACATION MODE – DEACTIVATION
// ---------------------------------------------------------

/**
 * @brief Deactivates vacation mode and restores the pre-vacation state.
 *
 * Reads the previously snapshotted mode index and intensity from globals,
 * converts the mode index back to a string using the centralized MODE_NAMES
 * lookup table, and restores both operating mode and fan intensity.
 *
 * @note If the stored mode index is out of bounds (e.g. due to a firmware
 *       update adding/removing modes), falls back to MODE_NAME_AUTO as the
 *       safest default for unattended operation.
 *
 * @note Called from script `handle_vacation_mode_off` in logic_automation.yaml.
 */
inline void deactivate_vacation_mode() {
    const int mode_idx = id(pre_vacation_mode_index);
    const int saved_intensity = static_cast<int>(id(pre_vacation_intensity));

    // ── Resolve mode index → mode name string ──
    // MODE_NAMES is defined in globals.h with exactly 5 entries [0..4].
    // Any index outside this range indicates data corruption or a version
    // mismatch → fall back to AUTO mode for safe unattended operation.
    std::string mode_str;
    if (mode_idx >= 0 && mode_idx < 5) {
        mode_str = MODE_NAMES[mode_idx];
    } else {
        ESP_LOGW("vacation",
                 "Stored mode index %d out of range [0–4], "
                 "falling back to '%s'",
                 mode_idx, MODE_NAME_AUTO);
        mode_str = MODE_NAME_AUTO;
    }

    // ── Restore pre-vacation state ──
    set_operating_mode_select(mode_str);
    set_fan_intensity_slider(id(pre_vacation_intensity));

    ESP_LOGI("vacation",
             "Vacation Mode DEACTIVATED: restored mode='%s' (idx=%d), "
             "intensity=%d",
             mode_str.c_str(), mode_idx, saved_intensity);
}
