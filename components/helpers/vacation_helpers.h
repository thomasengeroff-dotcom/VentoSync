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
// Modified:    2026-09-25
//
// Dependencies: globals.h (MODE_NAMES, MODE_NAME_BOOST, MODE_NAME_AUTO)
//               automation_helpers.h (set_operating_mode_select,
//                                     set_fan_intensity_slider)
// ==========================================================================

#pragma once

#include "esphome.h"

#include <cmath>

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

/// Values of the persistent `vacation_state` global.
static constexpr int VACATION_INACTIVE  = 0;
static constexpr int VACATION_FOLLOWING = 1;  ///< Active, the room leader applied it
static constexpr int VACATION_LEADING   = 2;  ///< Active, this device applied it

}  // namespace vacation
}  // namespace ventosync

// ---------------------------------------------------------
// ROOM LEADERSHIP
// ---------------------------------------------------------

/**
 * @brief True if this device applies/restores vacation mode for the room.
 *
 * Every device of a room receives the HA vacation toggle at almost the same
 * time. If every device snapshotted and restored on its own, a peer's
 * MSG_STATE could switch a device into the vacation mode *before* its own
 * trigger — it would snapshot the vacation state and restore it (and
 * broadcast it room-wide) at the end of the vacation. Therefore only the
 * Master (device ID 1) acts; the other devices follow its MSG_STATE. A slave
 * acts on its own only while no Master of the room is reachable.
 */
inline bool vacation_is_room_leader() {
    auto *v = ventilation_ctrl;
    if (v == nullptr || v->device_id == 1) return true;
    const uint32_t now = millis();
    for (const auto &peer : v->peers) {
        if (peer.device_id == 1 && now - peer.last_seen_ms < PEER_TIMEOUT_MS) {
            return false;  // Master present — it leads the room
        }
    }
    return true;  // No Master reachable — act locally
}

/**
 * @brief Adopts the vacation snapshot shared by the room's leader.
 *
 * Every device snapshots its own state when Home Assistant switches the
 * vacation toggle on — but a follower can capture the *vacation* state
 * instead: the leader applies the mode and broadcasts MSG_STATE immediately
 * (`set_operating_mode_select()`), and that packet can reach the follower
 * before its own HA push does (both arrive within milliseconds, and the
 * order is whatever Home Assistant's subscriber order happens to be).
 *
 * The leader therefore shares its snapshot (protocol v11, only while
 * `vacation_state == VACATION_LEADING`) and followers adopt it. That keeps
 * the "Master unreachable → restore locally" fallback correct, which is
 * exactly the path a corrupted snapshot would break.
 *
 * Writes to the persistent globals are free when nothing changed:
 * `RestoringGlobalsComponent` only saves on a real value change.
 *
 * @note Called every 10 s from `logic_automation.yaml` — the leader has to be
 *       reachable while it still leads, so this cannot wait until the restore.
 */
inline void vacation_adopt_leader_snapshot() {
    using namespace ventosync::vacation;
    auto *v = ventilation_ctrl;
    if (v == nullptr) return;
    // Only a follower adopts; a leader owns the authoritative snapshot.
    if (id(vacation_state) != VACATION_FOLLOWING) return;

    const uint32_t now = millis();
    const auto snap = ventosync::room::leader_vacation_snapshot(v->peers, now, PEER_TIMEOUT_MS);
    if (!snap.valid()) return;

    const int idx = static_cast<int>(snap.mode_index);
    const int lvl = static_cast<int>(snap.intensity);
    if (id(pre_vacation_mode_index) == idx && id(pre_vacation_intensity) == lvl) return;

    ESP_LOGI("vacation", "Adopted room leader's pre-vacation snapshot: mode_index=%d, intensity=%d "
             "(was %d/%d)", idx, lvl, id(pre_vacation_mode_index), id(pre_vacation_intensity));
    id(pre_vacation_mode_index) = idx;
    id(pre_vacation_intensity) = lvl;
}

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
 * @note Idempotent via the persistent `vacation_state`. Every device takes a
 *       snapshot (fallback), but only the room leader (vacation_is_room_leader())
 *       applies the vacation mode; the other devices follow its MSG_STATE.
 */
inline void activate_vacation_mode() {
    using namespace ventosync::vacation;

    // Idempotent: a repeated trigger must never re-snapshot the vacation state.
    if (id(vacation_state) != VACATION_INACTIVE) {
        ESP_LOGD("vacation", "Vacation mode already active — ignoring trigger");
        return;
    }

    // ── 1. Snapshot current state (used by the leader; fallback for the others) ──
    id(pre_vacation_mode_index) = id(current_mode_index);
    id(pre_vacation_intensity)  = id(fan_intensity_level);

    ESP_LOGD("vacation", "State snapshot: mode_index=%d, intensity=%d",
             id(current_mode_index),
             static_cast<int>(id(fan_intensity_level)));

    if (!vacation_is_room_leader()) {
        id(vacation_state) = VACATION_FOLLOWING;
        ESP_LOGI("vacation", "Vacation Mode ACTIVATED — following the Master's room state");
        return;
    }
    id(vacation_state) = VACATION_LEADING;

    // ── 2. Read configured vacation parameters from HA entities ──
    std::string target_mode = id(vacation_mode_select).current_option();
    float target_intensity  = id(vacation_intensity_number).state;

    // ── 3. Validate and apply fallbacks ──
    if (target_mode.empty()) {
        ESP_LOGW("vacation", "Vacation mode entity empty, falling back to '%s'",
                 MODE_NAME_BOOST);
        target_mode = MODE_NAME_BOOST;
    }

    // NaN slips through a plain range check (every comparison with NaN is
    // false), so it is tested explicitly — an uninitialized HA number would
    // otherwise apply the vacation mode without its fan level.
    if (std::isnan(target_intensity) ||
        target_intensity < INTENSITY_MIN || target_intensity > INTENSITY_MAX) {
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
 * @note Restores only on the device that applied the vacation mode or, if the
 *       Master is unreachable, on the device that has to lead now; the other
 *       devices follow its MSG_STATE.
 */
inline void deactivate_vacation_mode() {
    using namespace ventosync::vacation;

    const int state = id(vacation_state);
    if (state == VACATION_INACTIVE) {
        ESP_LOGD("vacation", "Vacation mode not active — ignoring trigger");
        return;
    }
    id(vacation_state) = VACATION_INACTIVE;

    // Restore if this device applied the vacation mode, or if it has to lead
    // now (Master unreachable). Otherwise the leader's MSG_STATE restores it.
    if (state != VACATION_LEADING && !vacation_is_room_leader()) {
        ESP_LOGI("vacation", "Vacation Mode DEACTIVATED — following the Master's room state");
        return;
    }

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
