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
// File:        ha_fan_helpers.h
// Description: Home Assistant fan entity integration helpers.
//              Manages the bidirectional state sync between the VentoSync
//              system and the HA fan entity (used by ventosync-card).
//              Implements a feedback-loop guard to prevent infinite
//              re-triggering between card input and system state sync.
// Author:      Thomas Engeroff
// Created:     2026-05-16
// Modified:    2026-05-16
//
// Dependencies: globals.h (MODE_NAMES, MODE_NAME_OFF, MODE_NAME_AUTO,
//                           system_on, ventilation_enabled,
//                           current_mode_index, fan_intensity_level)
//               automation_helpers.h (set_operating_mode_select,
//                                     set_fan_intensity_slider)
//               ha_fan_entity.yaml (ventosync_hrv_fan, ha_fan_syncing,
//                                    ha_fan_guard_set_ms)
// ==========================================================================

#pragma once

#include "esphome.h"
#include <algorithm>  // std::clamp

// ---------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------

namespace ventosync {
namespace ha_fan {

/// Propagation window after a sync write.
/// ESPHome processes call.perform() asynchronously in the next loop cycle.
/// We must block on_speed_set/on_preset_set during this window to prevent
/// the feedback loop: sync → perform → on_speed_set → set_intensity → sync...
static constexpr uint32_t GUARD_PROPAGATION_MS = 200;

/// Maximum time the guard may stay active before forced reset.
/// Safety net against permanent lockout if something goes wrong.
static constexpr uint32_t GUARD_STUCK_TIMEOUT_MS = 5000;

/// Mode index that represents "Aus" (Off).
static constexpr int MODE_INDEX_OFF = 4;

/// Maximum number of active preset modes (0–3, excluding "Aus").
static constexpr int MODE_INDEX_MAX_ACTIVE = 3;

}  // namespace ha_fan
}  // namespace ventosync

// ---------------------------------------------------------
// GUARD MANAGEMENT
// ---------------------------------------------------------

/**
 * @brief Checks if the sync guard is currently active.
 *
 * When the guard is active, all on_speed_set / on_preset_set /
 * on_turn_on / on_turn_off callbacks should return immediately
 * without processing the event – it was triggered by our own
 * sync interval, not by user input.
 *
 * @return true if the guard is active (skip callback processing)
 */
inline bool ha_fan_guard_active() {
    return id(ha_fan_syncing);
}

/**
 * @brief Attempts to release the sync guard if the propagation window has elapsed.
 *
 * Called at the top of each sync cycle. The guard is released when:
 *   - Normal: >200ms have elapsed since it was set
 *   - Emergency: >5s have elapsed (stuck guard, should never happen)
 *
 * @return true if the guard was active and we should skip this sync cycle
 *         (still within propagation window).
 *         false if the guard is released and sync can proceed.
 */
inline bool ha_fan_try_release_guard() {
    using namespace ventosync::ha_fan;

    if (!id(ha_fan_syncing)) {
        return false;  // Guard not active, proceed normally
    }

    const uint32_t elapsed = millis() - id(ha_fan_guard_set_ms);

    // Emergency: Force-reset stuck guard
    if (elapsed > GUARD_STUCK_TIMEOUT_MS) {
        ESP_LOGW("ha_fan", "Guard stuck for %lu ms → forced reset",
                 static_cast<unsigned long>(elapsed));
        id(ha_fan_syncing) = false;
        return false;  // Released, proceed with sync
    }

    // Normal: Release after propagation window
    if (elapsed > GUARD_PROPAGATION_MS) {
        id(ha_fan_syncing) = false;
        return false;  // Released, proceed with sync
    }

    // Still within propagation window – skip this cycle
    return true;
}

/**
 * @brief Activates the sync guard.
 *
 * Must be called BEFORE call.perform() to block any re-entrant
 * triggers from on_speed_set / on_preset_set that fire when
 * ESPHome processes the state change.
 */
inline void ha_fan_set_guard() {
    id(ha_fan_syncing) = true;
    id(ha_fan_guard_set_ms) = millis();
}

// ---------------------------------------------------------
// CARD → SYSTEM: Input Handlers
// ---------------------------------------------------------

/**
 * @brief Handles speed changes from the ventosync-card circular slider.
 *
 * @param speed  Speed value from ESPHome (1–10 for speed_count: 10,
 *               0 = turn off via HA service call with percentage: 0)
 */
inline void ha_fan_on_speed_set(int speed) {
    if (ha_fan_guard_active()) return;

    // Speed 0 = turn off (handles HA service calls that send
    // percentage: 0 instead of fan.turn_off)
    if (speed <= 0) {
        set_operating_mode_select(MODE_NAME_OFF);
        ESP_LOGI("ha_fan", "Speed 0 received → turning off");
        return;
    }

    const int step = std::clamp(speed, 1, 10);
    set_fan_intensity_slider(static_cast<float>(step));
    ESP_LOGI("ha_fan", "Card set intensity to step %d", step);
}

/**
 * @brief Handles preset mode changes from the ventosync-card.
 *
 * @param preset  Preset mode string from ESPHome (StringRef)
 */
inline void ha_fan_on_preset_set(const std::string &preset) {
    if (ha_fan_guard_active()) return;

    set_operating_mode_select(preset);
    ESP_LOGI("ha_fan", "Card set mode: %s", preset.c_str());
}

/**
 * @brief Handles turn-on from the ventosync-card or HA service call.
 *
 * Resumes the last active preset mode. If no preset was set or
 * the last preset was "Aus", defaults to Smart-Automatik.
 * Also ensures the system globals (ventilation_enabled, system_on)
 * are set to true.
 */
inline void ha_fan_on_turn_on() {
    if (ha_fan_guard_active()) return;

    // Determine target mode: resume last preset or default to Auto
    std::string target_mode = MODE_NAME_AUTO;

    auto &fan = id(ventosync_hrv_fan);
    if (fan.has_preset_mode()) {
        std::string last = fan.get_preset_mode().str();
        if (last != MODE_NAME_OFF) {
            target_mode = last;
        }
    }

    // Ensure system is enabled
    if (!id(ventilation_enabled)) {
        id(ventilation_enabled) = true;
        id(system_on) = true;
    }

    set_operating_mode_select(target_mode);
    ESP_LOGI("ha_fan", "Turned ON → mode: %s", target_mode.c_str());
}

/**
 * @brief Handles turn-off from the ventosync-card or HA service call.
 */
inline void ha_fan_on_turn_off() {
    if (ha_fan_guard_active()) return;

    set_operating_mode_select(MODE_NAME_OFF);
    ESP_LOGI("ha_fan", "Turned OFF");
}

// ---------------------------------------------------------
// SYSTEM → CARD: State Sync Engine
// ---------------------------------------------------------

/**
 * @brief Synchronizes the current system state to the HA fan entity.
 *
 * Called every 3s from the sync interval. Reads the actual system state
 * (mode, intensity, on/off) and publishes it to the fan entity so the
 * ventosync-card stays in sync.
 *
 * Implements a 3-phase approach:
 *   1. Guard management (release previous cycle's guard)
 *   2. Read system state + detect changes
 *   3. Apply changes via fan.make_call() + set guard
 *
 * The guard prevents the feedback loop:
 *   sync → call.perform() → on_speed_set fires → set_intensity
 *   → intensity changes → next sync detects change → loop forever
 *
 * By setting the guard BEFORE call.perform() and releasing it at the
 * TOP of the NEXT sync cycle (after 200ms propagation), we ensure
 * ESPHome has fully processed the state change before accepting
 * new user input.
 */
inline void ha_fan_sync_state() {
    using namespace ventosync::ha_fan;

    // ── Phase 0: Guard management ──
    if (ha_fan_try_release_guard()) {
        return;  // Still within propagation window, skip
    }

    // ── Phase 1: Read current system state ──
    bool sys_on = id(system_on) && id(ventilation_enabled);
    const int mode_idx = id(current_mode_index);

    // Mode index 4 = "Aus" → system is logically off
    if (mode_idx == MODE_INDEX_OFF) {
        sys_on = false;
    }

    const int intensity = std::clamp(
        static_cast<int>(id(fan_intensity_level)), 1, 10);

    // Map mode index → preset name (only for active modes 0–3)
    const char *target_preset = nullptr;
    if (sys_on && mode_idx >= 0 && mode_idx <= MODE_INDEX_MAX_ACTIVE) {
        target_preset = MODE_NAMES[mode_idx];
    }

    // ── Phase 2: Detect changes ──
    auto &fan = id(ventosync_hrv_fan);

    const bool state_changed = (fan.state != sys_on);
    const bool speed_changed = (sys_on && fan.speed != intensity);

    bool preset_changed = false;
    if (sys_on && target_preset != nullptr) {
        std::string current_preset;
        if (fan.has_preset_mode()) {
            current_preset = fan.get_preset_mode().str();
        }
        preset_changed = (current_preset != target_preset);
    }

    // Nothing changed → no work
    if (!state_changed && !speed_changed && !preset_changed) {
        return;
    }

    // ── Phase 3: Apply changes + set guard ──
    ha_fan_set_guard();

    auto call = fan.make_call();
    if (sys_on) {
        call.set_state(true);
        call.set_speed(intensity);
        if (target_preset != nullptr) {
            call.set_preset_mode(target_preset);
        }
    } else {
        call.set_state(false);
    }
    call.perform();

    ESP_LOGD("ha_fan", "Synced → on:%s speed:%d preset:%s",
             sys_on ? "yes" : "no",
             intensity,
             target_preset ? target_preset : "n/a");
}
