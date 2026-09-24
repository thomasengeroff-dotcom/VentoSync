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
// Modified:    2026-09-24
// ==========================================================================
#include "ventilation_state_machine.h"
#include <climits>

namespace esphome {

/**
 * @brief   Initializes the state machine. Called once at boot.
 * @details Reserved for future use (e.g. restoring persisted state).
 */
void VentilationStateMachine::setup() {
    // Initial setup if needed
}

/**
 * @brief   Main tick — advanced every loop cycle.
 * @details Handles ventilation timer expiry, Stoßlüftung on/off cycling, and global
 *          direction phase transitions.
 * @param[in] now  Current system time in milliseconds.
 * @return  true if any state changed (hardware update needed).
 */
bool VentilationStateMachine::update(uint32_t now) {
    bool dirty = false;

    // 1. Handle Ventilation Timer
    if (current_mode == MODE_VENTILATION && ventilation_duration_ms > 0) {
        if (now - ventilation_start_time > ventilation_duration_ms) {
            set_mode(MODE_ECO_RECOVERY, now);
            ventilation_timer_expired = true; // consumed by the UI glue
            dirty = true;
        }
    }

    // 2. Handle Stoßlüftung Cycle (15 min burst / 105 min pause, direction
    //    inverted every second burst — derived from the super-cycle position)
    if (current_mode == MODE_STOSSLUEFTUNG) {
        // Keep the anchor close to now so (now - anchor) never wraps.
        const uint32_t elapsed = now - stoss_cycle_start;
        if (elapsed >= STOSS_SUPER_CYCLE_MS) {
            stoss_cycle_start += (elapsed / STOSS_SUPER_CYCLE_MS) * STOSS_SUPER_CYCLE_MS;
        }
        const uint32_t pos = get_stoss_pos(now);
        const bool active = (pos % STOSS_CYCLE_MS) < STOSS_ACTIVE_MS;
        const bool flip = pos >= STOSS_CYCLE_MS;
        if (active != stoss_active_phase || flip != stoss_direction_flip) {
            stoss_active_phase = active;
            stoss_direction_flip = flip;
            dirty = true;
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

/**
 * @brief   Switches the operating mode and resets mode-specific timers.
 * @details No-op if mode and duration are identical to current values.
 * @param[in] mode      Target mode (MODE_OFF, MODE_ECO_RECOVERY, etc.).
 * @param[in] now       Current system time in milliseconds.
 * @param[in] duration  For MODE_VENTILATION: auto-stop duration in ms (0 = infinite).
 *                      For MODE_STOSSLUEFTUNG: 0 = start a new schedule unless one
 *                      is running, > 0 = peer's remaining super-cycle time to align with.
 */
void VentilationStateMachine::set_mode(VentilationMode mode, uint32_t now, uint32_t duration) {
    // Stoßlüftung: re-selecting keeps the running schedule; a duration is a
    // peer's schedule position, not a timer (ventilation_duration_ms stays 0).
    if (mode == MODE_STOSSLUEFTUNG) {
        if (current_mode != MODE_STOSSLUEFTUNG) {
            current_mode = MODE_STOSSLUEFTUNG;
            ventilation_duration_ms = 0;
            stoss_cycle_start = now;
            stoss_active_phase = true;
            stoss_direction_flip = false;
        }
        if (duration > 0) sync_stoss(now, duration);
        return;
    }

    // FIXED K-3: Removed dead variable 'changed'; simplified early-return.
    // NOTE: This is intentional for timed modes (MODE_VENTILATION, MODE_STOSSLUEFTUNG):
    // re-selecting the same mode with the same duration does NOT restart the timer.
    // The user must change duration or switch modes to reset. Debouncing for
    // accidental double-presses is handled at the UI layer.
    if (current_mode == mode && duration == ventilation_duration_ms) return;

    current_mode = mode;
    ventilation_duration_ms = duration;

    if (mode == MODE_VENTILATION) {
        ventilation_start_time = now;
    }
}

/**
 * @brief   Updates the half-cycle duration (one direction) for the alternating airflow.
 * @details Maintains proportional progress in the current cycle to avoid abrupt jumps.
 * @param[in] now  Current system time in milliseconds.
 * @param[in] ms   Half-cycle duration in milliseconds (e.g. 70000 = 70 s).
 */
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
    
    // FIXED CR-2: Normalize target_offset to be within [-new_period, new_period].
    // The modulo guarantees |result| < new_period, so no additional bounds check needed.
    target_offset %= (int64_t)new_period;
    
    // FIXED K-1: Clamp before int32_t cast to prevent overflow
    constexpr int64_t MAX_OFFSET = static_cast<int64_t>(INT32_MAX);
    target_offset = std::clamp(target_offset, -MAX_OFFSET, MAX_OFFSET);
    time_offset_ms = static_cast<int32_t>(target_offset);
    cycle_duration_ms = ms;
}

/**
 * @brief   Synchronizes the local cycle phase with a peer device.
 * @details Adjusts the internal time_offset so that both devices switch direction
 *          simultaneously. Only applies correction if drift exceeds 500 ms.
 * @param[in] now            Current system time in milliseconds.
 * @param[in] target_pos_ms  Cycle position reported by the peer (0 to 2xHalfCycle).
 */
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
        // FIXED CR-1: Widen to int64 before addition to prevent INT32_MAX overflow
        // after months of one-sided drift accumulation (~4000 syncs at +501ms each).
        int64_t wide_offset = (int64_t)time_offset_ms + diff;
        // Keep offset within [-period, period] to avoid unbounded growth
        wide_offset %= (int64_t)period;
        time_offset_ms = static_cast<int32_t>(wide_offset);
    }
}

/**
 * @brief   Aligns the Stoßlüftung schedule with a peer's super-cycle position.
 * @param[in] now           Current system time in milliseconds.
 * @param[in] remaining_ms  Peer's remaining super-cycle time (1 … STOSS_SUPER_CYCLE_MS).
 */
void VentilationStateMachine::sync_stoss(uint32_t now, uint32_t remaining_ms) {
    if (remaining_ms == 0 || remaining_ms > STOSS_SUPER_CYCLE_MS) return;
    const uint32_t pos = (STOSS_SUPER_CYCLE_MS - remaining_ms) % STOSS_SUPER_CYCLE_MS;
    stoss_cycle_start = now - pos;
    stoss_active_phase = (pos % STOSS_CYCLE_MS) < STOSS_ACTIVE_MS;
    stoss_direction_flip = pos >= STOSS_CYCLE_MS;
}

/**
 * @brief   Returns the position in the Stoßlüftung super-cycle.
 * @param[in] now  Current system time in milliseconds.
 * @return  Position in ms within [0, STOSS_SUPER_CYCLE_MS).
 */
uint32_t VentilationStateMachine::get_stoss_pos(uint32_t now) const {
    return (now - stoss_cycle_start) % STOSS_SUPER_CYCLE_MS;
}

/**
 * @brief   Returns the remaining time of the timed modes in milliseconds.
 * @details MODE_VENTILATION: remaining timer. MODE_STOSSLUEFTUNG: remaining
 *          time in the super-cycle (peers align their bursts with it).
 * @param[in] now  Current system time in milliseconds.
 * @return  Remaining duration in milliseconds (0 if expired, infinite or untimed).
 */
uint32_t VentilationStateMachine::get_remaining_duration(uint32_t now) const {
    if (current_mode == MODE_STOSSLUEFTUNG) {
        return STOSS_SUPER_CYCLE_MS - get_stoss_pos(now);
    }
    if (ventilation_duration_ms == 0) return 0;
    uint32_t elapsed = now - ventilation_start_time;
    if (elapsed >= ventilation_duration_ms) return 0;
    return ventilation_duration_ms - elapsed;
}

/**
 * @brief   Returns the current position within the full direction cycle.
 * @details Used for ESP-NOW sync packets so peers can align their phase.
 * @param[in] now  Current system time in milliseconds.
 * @return  Position in ms within [0 to 2×cycle_duration_ms).
 */
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

/**
 * @brief   Computes the desired hardware outputs based on mode and phase.
 * @details Determines fan state, direction, and ramp factor.
 * @param[in] now  Current system time in milliseconds.
 * @return  HardwareState struct containing target outputs.
 */
HardwareState VentilationStateMachine::get_target_state(uint32_t now) const {
    HardwareState state;
    state.fan_enabled = true;
    state.direction_in = true;
    state.ramp_factor = 1.0f; // Default: full speed

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

    // --- Ramping Logic (WRG only) ---
    // Only apply ramping in modes that have cyclic direction changes.
    // Stoßlüftung runs one-way during a burst (see burst ramp below).
    // FIXED K-2: Guard against RAMP_DURATION_MS == 0 (defense-in-depth,
    //   currently constexpr 5000) and ramp overlap when half-cycle is
    //   shorter than 2× ramp duration.
    // FIXED S-1: Removed dead variable 'full'.
    if (current_mode == MODE_ECO_RECOVERY &&
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

    // --- Stoßlüftung Burst Ramp (soft-start from pause, soft-stop before pause) ---
    // FIXED CR-3: Smooths the pause↔active transitions to avoid abrupt
    // mechanical stress on the fan motor after 105 min of standstill. The
    // direction only changes during the pause, so no other ramp is needed.
    if (current_mode == MODE_STOSSLUEFTUNG && stoss_active_phase && RAMP_DURATION_MS > 0) {
        const uint32_t burst_elapsed = get_stoss_pos(now) % STOSS_CYCLE_MS;
        if (burst_elapsed < RAMP_DURATION_MS) {
            // Ramp-up at start of active burst
            float burst_ramp = static_cast<float>(burst_elapsed)
                             / static_cast<float>(RAMP_DURATION_MS);
            state.ramp_factor = std::min(state.ramp_factor, burst_ramp);
        } else if (STOSS_ACTIVE_MS > RAMP_DURATION_MS &&
                   burst_elapsed > (STOSS_ACTIVE_MS - RAMP_DURATION_MS) &&
                   burst_elapsed < STOSS_ACTIVE_MS) {
            // Ramp-down before entering pause phase
            const uint32_t remaining = STOSS_ACTIVE_MS - burst_elapsed;
            float burst_ramp = static_cast<float>(remaining)
                             / static_cast<float>(RAMP_DURATION_MS);
            state.ramp_factor = std::min(state.ramp_factor, burst_ramp);
        }
    }

    // Direction Logic
    if (current_mode == MODE_VENTILATION) {
        state.direction_in = is_phase_a;
    } else if (current_mode == MODE_STOSSLUEFTUNG) {
        // One-way burst: Phase A in / Phase B out, inverted every second burst
        // (the flip happens during the pause, never under load).
        state.direction_in = stoss_direction_flip ? !is_phase_a : is_phase_a;
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
