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
//              operating parameters, and the room-wide vacation preset.
// Author:      Thomas Engeroff
// Created:     2026-05-16
// Modified:    2026-10-22
//
// Dependencies: globals.h (MODE_NAMES, MODE_NAME_AUTO, sync_settings_to_peers)
//               automation_helpers.h (set_operating_mode_select,
//                                     set_fan_intensity_slider)
// ==========================================================================

#pragma once

#include "esphome.h"
#include "esphome/components/ventilation_logic/defaults.h"
#include "esphome/components/ventilation_logic/room_fusion.h"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------

namespace ventosync {
namespace vacation {

/// Valid intensity range for vacation mode configuration.
static constexpr float INTENSITY_MIN = 1.0f;
static constexpr float INTENSITY_MAX = 10.0f;

/// Fallback level when the stored preset is out of range (factory default).
static constexpr float INTENSITY_FALLBACK = static_cast<float>(ventosync::defaults::VACATION_LEVEL);

/// Values of the persistent `vacation_state` global.
static constexpr int VACATION_INACTIVE  = 0;
static constexpr int VACATION_FOLLOWING = 1;  ///< Active, the room leader applied it
static constexpr int VACATION_LEADING   = ventosync::room::VACATION_STATE_LEADING;  ///< Active, this device applied it

}  // namespace vacation
}  // namespace ventosync

// ---------------------------------------------------------
// VACATION PRESET (room-wide setting, ESP-NOW v14)
// ---------------------------------------------------------

/// @brief Preset mode name for the HA select "Urlaubsmodus Betriebsmodus".
inline std::string vacation_preset_mode_name() {
    const int idx = std::clamp(id(vacation_preset_mode_index_val), 0, MODE_INDEX_COUNT - 1);
    return MODE_NAMES[idx];
}

/**
 * @brief   Sets the vacation preset mode from Home Assistant or the web dashboard.
 *
 * @details Room-wide setting like the Smart-Automatik sliders: stored, shown
 *          and sent to the room (MSG_STATE), so every device of the room
 *          shows and — when it has to lead the vacation — applies the same
 *          preset.
 *
 * @param[in] option  One of MODE_NAMES (select option).
 */
inline void vacation_set_preset_mode(const std::string &option) {
    for (int i = 0; i < MODE_INDEX_COUNT; ++i) {
        if (option != MODE_NAMES[i]) continue;
        id(vacation_preset_mode_index_val) = i;
        id(vacation_mode_select).publish_state(option);
        sync_settings_to_peers();
        ESP_LOGI("vacation", "Urlaubsmodus Betriebsmodus set to: %s (room-wide)", option.c_str());
        return;
    }
    ESP_LOGW("vacation", "Unknown vacation mode '%s' — ignored", option.c_str());
}

/**
 * @brief   Sets the vacation preset fan level (1–10) from Home Assistant or
 *          the web dashboard; room-wide like vacation_set_preset_mode().
 */
inline void vacation_set_preset_level(float value) {
    if (std::isnan(value)) return;
    const int level = std::clamp(static_cast<int>(std::lround(value)), 1, 10);
    id(vacation_preset_level_val) = level;
    id(vacation_intensity_number).publish_state(level);
    sync_settings_to_peers();
    ESP_LOGI("vacation", "Urlaubsmodus Intensität set to: %d (room-wide)", level);
}

/**
 * @brief   Takes over the vacation preset of a peer's MSG_STATE / Master heartbeat.
 *
 * @details Called from handle_config_sync() (same authority rule as the
 *          other room-wide settings). parse() has range-checked both fields.
 *          No re-broadcast — the setting reaches every device directly.
 *
 * @return  true if a value changed.
 */
inline bool vacation_adopt_preset(const esphome::VentilationPacket *pkt) {
    bool changed = false;
    if (pkt->vacation_preset_mode_index != id(vacation_preset_mode_index_val)) {
        id(vacation_preset_mode_index_val) = pkt->vacation_preset_mode_index;
        id(vacation_mode_select).publish_state(vacation_preset_mode_name());
        changed = true;
    }
    if (pkt->vacation_preset_level != id(vacation_preset_level_val)) {
        id(vacation_preset_level_val) = pkt->vacation_preset_level;
        id(vacation_intensity_number).publish_state(pkt->vacation_preset_level);
        changed = true;
    }
    if (changed) {
        ESP_LOGI("vacation", "Vacation preset from device %d: %s, level %d", pkt->device_id,
                 vacation_preset_mode_name().c_str(), id(vacation_preset_level_val));
    }
    return changed;
}

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
    if (v == nullptr || v->device_id == MASTER_DEVICE_ID) return true;
    const uint32_t now = millis();
    for (const auto &peer : v->peers) {
        if (peer.device_id == MASTER_DEVICE_ID && now - peer.last_seen_ms < PEER_TIMEOUT_MS) {
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
 * @brief Activates vacation mode with the room-wide vacation preset.
 *
 * Workflow:
 *   1. Snapshots current operating mode index and fan intensity for later
 *      restoration (stored in globals: pre_vacation_mode_index,
 *      pre_vacation_intensity).
 *   2. Reads the preset (vacation_preset_mode_index_val,
 *      vacation_preset_level_val — room-wide settings, ESP-NOW v14).
 *   3. Applies safe fallback defaults if a stored value is out of range
 *      (factory defaults, ventosync::defaults).
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

    // ── 2. Read the room-wide vacation preset ──
    int mode_idx = id(vacation_preset_mode_index_val);
    float target_intensity = static_cast<float>(id(vacation_preset_level_val));

    // ── 3. Validate and apply fallbacks ──
    if (mode_idx < 0 || mode_idx >= MODE_INDEX_COUNT) {
        ESP_LOGW("vacation", "Vacation mode index %d out of range, falling back to '%s'",
                 mode_idx, MODE_NAMES[ventosync::defaults::VACATION_MODE_INDEX]);
        mode_idx = ventosync::defaults::VACATION_MODE_INDEX;
    }
    const std::string target_mode = MODE_NAMES[mode_idx];

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
    // MODE_NAMES is defined in globals.h with MODE_INDEX_COUNT entries.
    // Any index outside this range indicates data corruption or a version
    // mismatch → fall back to AUTO mode for safe unattended operation.
    std::string mode_str;
    if (mode_idx >= 0 && mode_idx < MODE_INDEX_COUNT) {
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
