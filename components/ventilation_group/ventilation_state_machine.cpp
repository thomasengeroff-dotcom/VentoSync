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
// File:        ventilation_state_machine.cpp
// Description: Implementation of the ventilation state machine.
// Author:      Thomas Engeroff
// Created:     2026-02-15
// Modified:    2026-03-19
// ==========================================================================
#include "ventilation_state_machine.h"
#include <climits>

namespace esphome {

/// @brief Initializes the state machine. Called once at boot.
/// Reserved for future use (e.g. restoring persisted state).
void VentilationStateMachine::setup() {
    // Initial setup if needed
}

/// @brief Main tick — called every loop iteration to advance timers and cycle phase.
/// Handles ventilation timer expiry, Stoßlüftung on/off cycling, and global
/// direction phase transitions. Returns true if any state changed (hardware update needed).
/// @param now  Current millis() timestamp.
/// @return true if hardware outputs should be refreshed.
bool VentilationStateMachine::update(uint32_t now) {
    bool dirty = false;

    // 1. Handle Ventilation Timer
    if (current_mode == MODE_VENTILATION && ventilation_duration_ms > 0) {
        if (now - ventilation_start_time > ventilation_duration_ms) {
            set_mode(MODE_ECO_RECOVERY, now);
            dirty = true;
        }
    }

    // 2. Handle Stoßlüftung Cycle
    if (current_mode == MODE_STOSSLUEFTUNG) {
        uint32_t elapsed = now - stoss_cycle_start;
        if (stoss_active_phase) {
            if (elapsed >= STOSS_ACTIVE_MS) {
                stoss_active_phase = false;
                stoss_cycle_start = now;
                dirty = true;
            }
        } else {
            if (elapsed >= STOSS_PAUSE_MS) {
                stoss_active_phase = true;
                stoss_direction_flip = !stoss_direction_flip;
                stoss_cycle_start = now;
                dirty = true;
            }
        }
    }

    // 3. Cycle Logic
    uint32_t pos = get_cycle_pos(now);
    bool new_phase_a_active = (pos < cycle_duration_ms);

    if (new_phase_a_active != global_phase) {
        // ESP_LOGD is not available here (pure C++ class), but we return dirty
        // so the caller (VentilationController) logs the transition.
        global_phase = new_phase_a_active;
        dirty = true;
    }

    return dirty;
}

/// @brief Switches the operating mode and resets mode-specific timers.
/// No-op if mode and duration are identical to current values.
/// @param mode      Target mode (MODE_OFF, MODE_ECO_RECOVERY, MODE_STOSSLUEFTUNG, MODE_VENTILATION).
/// @param now       Current millis() timestamp (used to start timers).
/// @param duration  For MODE_VENTILATION: auto-stop duration in ms (0 = infinite).
void VentilationStateMachine::set_mode(VentilationMode mode, uint32_t now, uint32_t duration) {
    // FIXED K-3: Removed dead variable 'changed'; simplified early-return
    if (current_mode == mode && duration == ventilation_duration_ms) return;

    current_mode = mode;
    ventilation_duration_ms = duration;

    if (mode == MODE_VENTILATION) {
        ventilation_start_time = now;
    }
    if (mode == MODE_STOSSLUEFTUNG) {
        stoss_cycle_start = now;
        stoss_active_phase = true;
        stoss_direction_flip = false;
    }
}

/// @brief Updates the half-cycle duration (one direction) for the alternating airflow.
/// A full cycle = 2 × cycle_duration_ms (intake + exhaust).
/// Maintains proportional progress in the current cycle to avoid abrupt jumps.
/// @param now Current millis() timestamp.
/// @param ms  Half-cycle duration in milliseconds (e.g. 70000 = 70 s per direction).
void VentilationStateMachine::set_cycle_duration(uint32_t now, uint32_t ms) {
    // FIXED H-3: Guard against invalid input value
    if (ms == 0) return;
    if (cycle_duration_ms == ms) return;
    if (cycle_duration_ms == 0) {
        cycle_duration_ms = ms;
        return;
    }

    // FIXED H-3: Overflow guard for ms * 2
    const uint32_t new_period = ms * 2;
    if (new_period < ms) return; // uint32_t overflow

    const uint32_t old_half = cycle_duration_ms; // Guaranteed > 0 here
    uint32_t old_pos = get_cycle_pos(now);
    
    // Calculate proportional position for the new cycle
    uint64_t exact_new_pos;
    if (old_pos < old_half) {
        // We are in phase A
        exact_new_pos = (uint64_t)old_pos * (uint64_t)ms / (uint64_t)old_half;
    } else {
        // We are in phase B
        uint32_t progress_in_b = old_pos - old_half;
        exact_new_pos = (uint64_t)ms + ((uint64_t)progress_in_b * (uint64_t)ms / (uint64_t)old_half);
    }
    
    uint32_t new_pos = (uint32_t)exact_new_pos;
    
    // We want: (now + target_offset) % new_period == new_pos
    int64_t n = now;
    int64_t target_offset = (int64_t)new_pos - n;
    
    // Normalize target_offset to be within [-new_period, new_period]
    target_offset %= (int64_t)new_period;
    if (target_offset < -(int64_t)new_period) {
        target_offset += new_period; // Handle negative wrapping
    }
    
    // FIXED K-1: Clamp before int32_t cast to prevent overflow
    constexpr int64_t MAX_OFFSET = static_cast<int64_t>(INT32_MAX);
    target_offset = std::clamp(target_offset, -MAX_OFFSET, MAX_OFFSET);
    time_offset_ms = static_cast<int32_t>(target_offset);
    cycle_duration_ms = ms;
}

/// @brief Synchronizes the local cycle phase with a peer device received via ESP-NOW.
/// Adjusts the internal time_offset so that both devices switch direction at the same time.
/// Only applies correction if the drift exceeds 200 ms (jitter suppression).
/// @param now            Current millis() timestamp.
/// @param target_pos_ms  Cycle position reported by the peer (0 … 2×cycle_duration_ms).
void VentilationStateMachine::sync_time(uint32_t now, uint32_t target_pos_ms) {
    // FIXED H-2: Guard against division-by-zero if cycle not yet configured
    if (cycle_duration_ms == 0) return;

    uint32_t period = cycle_duration_ms * 2;
    // Use the safe get_cycle_pos() helper to avoid rollover issues
    uint32_t my_pos = get_cycle_pos(now);

    int32_t diff = (int32_t)target_pos_ms - (int32_t)my_pos;
    if (diff > (int32_t)cycle_duration_ms) diff -= period;
    if (diff < -(int32_t)cycle_duration_ms) diff += period;

    // FIXED: Increased threshold from 200ms to 500ms to reduce jitter
    // near direction flip boundaries.
    if (std::abs(diff) > 500) {
        time_offset_ms += diff;
        // Keep offset within [-period, period] to avoid int32_t overflow after months of syncs
        time_offset_ms %= static_cast<int32_t>(period);
    }
}

/// @brief Returns the remaining time for MODE_VENTILATION in milliseconds.
/// Returns 0 if no timer is set (infinite mode) or if the timer has already expired.
/// @param now  Current millis() timestamp.
uint32_t VentilationStateMachine::get_remaining_duration(uint32_t now) const {
    if (ventilation_duration_ms == 0) return 0;
    uint32_t elapsed = now - ventilation_start_time;
    if (elapsed >= ventilation_duration_ms) return 0;
    return ventilation_duration_ms - elapsed;
}

/// @brief Returns the current position within the full direction cycle.
/// Used for ESP-NOW sync packets so peers can align their phase.
/// @param now  Current millis() timestamp.
/// @return Position in ms within [0 … 2×cycle_duration_ms).
uint32_t VentilationStateMachine::get_cycle_pos(uint32_t now) const {
    // FIXED K-4: Guard against division-by-zero when cycle not yet configured
    if (cycle_duration_ms == 0) return 0;

    uint32_t period = cycle_duration_ms * 2;
    if (period < cycle_duration_ms) return 0; // Overflow guard

    int64_t raw_pos = (int64_t)now + (int64_t)time_offset_ms;
    int64_t mod_pos = raw_pos % (int64_t)period;
    if (mod_pos < 0) {
        mod_pos += period;
    }
    return (uint32_t)mod_pos;
}

/// @brief Computes the desired hardware outputs (fan on/off, direction) based on
/// the current mode, cycle phase, and Stoßlüftung state.
/// The caller compares the result to the previous state to decide whether to
/// actually toggle GPIOs.
/// @return HardwareState with fan_enabled and direction_in fields set.
HardwareState VentilationStateMachine::get_target_state(uint32_t now) const {
    HardwareState state;
    state.fan_enabled = true;
    state.direction_in = true;
    state.ramp_factor = 1.0f; // Default: full speed
    state.needs_update = false;

    if (current_mode == MODE_OFF) {
        state.fan_enabled = false;
        state.ramp_factor = 0.0f;
        return state;
    }

    if (current_mode == MODE_STOSSLUEFTUNG && !stoss_active_phase) {
        state.fan_enabled = false;
        state.ramp_factor = 0.0f;
        return state;
    }

    // --- Ramping Logic (for WRG and Stoßlüftung) ---
    // Only apply ramping in modes that have cyclic direction changes
    // FIXED K-2: Guard against RAMP_DURATION_MS == 0 (defense-in-depth,
    //   currently constexpr 5000) and ramp overlap when half-cycle is
    //   shorter than 2× ramp duration.
    // FIXED S-1: Removed dead variable 'full'.
    if ((current_mode == MODE_ECO_RECOVERY || current_mode == MODE_STOSSLUEFTUNG) &&
        RAMP_DURATION_MS > 0 && cycle_duration_ms >= 2 * RAMP_DURATION_MS) {
        const uint32_t pos = get_cycle_pos(now);
        const uint32_t half = cycle_duration_ms;

        // Simplify position to half-cycle relative [0 ... half)
        const uint32_t phase_pos = pos % half;
        
        if (phase_pos < RAMP_DURATION_MS) {
            // 1. Ramp Up: 0.0 -> 1.0 in the first 5s
            state.ramp_factor = static_cast<float>(phase_pos)
                              / static_cast<float>(RAMP_DURATION_MS);
        } else if (phase_pos > (half - RAMP_DURATION_MS)) {
            // 2. Ramp Down: 1.0 -> 0.0 in the last 5s
            const uint32_t remaining = half - phase_pos;
            state.ramp_factor = static_cast<float>(remaining)
                              / static_cast<float>(RAMP_DURATION_MS);
        }
    }

    // Direction Logic
    if (current_mode == MODE_VENTILATION) {
        state.direction_in = is_phase_a;
    } else if (current_mode == MODE_STOSSLUEFTUNG) {
        // Active phase
        if (global_phase) {
            state.direction_in = stoss_direction_flip ? !is_phase_a : is_phase_a;
        } else {
            state.direction_in = stoss_direction_flip ? is_phase_a : !is_phase_a;
        }
    } else {
        // ECO_RECOVERY
        if (global_phase) {
            state.direction_in = is_phase_a;
        } else {
            state.direction_in = !is_phase_a;
        }
    }
    
    return state;
}

} // namespace esphome
