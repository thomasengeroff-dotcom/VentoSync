#include "ventilation_state_machine.h"

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
    bool changed = (current_mode != mode);
    if (!changed && duration == ventilation_duration_ms) return;

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
/// @param ms  Half-cycle duration in milliseconds (e.g. 70000 = 70 s per direction).
void VentilationStateMachine::set_cycle_duration(uint32_t ms) {
    cycle_duration_ms = ms;
}

/// @brief Synchronizes the local cycle phase with a peer device received via ESP-NOW.
/// Adjusts the internal time_offset so that both devices switch direction at the same time.
/// Only applies correction if the drift exceeds 200 ms (jitter suppression).
/// @param now            Current millis() timestamp.
/// @param target_pos_ms  Cycle position reported by the peer (0 … 2×cycle_duration_ms).
void VentilationStateMachine::sync_time(uint32_t now, uint32_t target_pos_ms) {
    uint32_t period = cycle_duration_ms * 2;
    uint32_t my_pos = (now + time_offset_ms) % period;

    int32_t diff = (int32_t)target_pos_ms - (int32_t)my_pos;
    if (diff > (int32_t)cycle_duration_ms) diff -= period;
    if (diff < -(int32_t)cycle_duration_ms) diff += period;

    if (std::abs(diff) > 200) {
        time_offset_ms += diff;
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
    uint32_t period = cycle_duration_ms * 2;
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
HardwareState VentilationStateMachine::get_target_state() const {
    HardwareState state;
    state.fan_enabled = true;
    state.direction_in = true;
    state.needs_update = false; // Caller determines if diff warrants update

    if (current_mode == MODE_OFF) {
        state.fan_enabled = false;
        return state;
    }

    if (current_mode == MODE_STOSSLUEFTUNG && !stoss_active_phase) {
        state.fan_enabled = false;
        return state;
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
