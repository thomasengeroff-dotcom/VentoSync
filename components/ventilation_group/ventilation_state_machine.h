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
// File:        ventilation_state_machine.h
// Description: State machine for ventilation modes and timing.
// Author:      Thomas Engeroff
// Created:     2026-02-15
// Modified:    2026-03-19
// ==========================================================================
#pragma once

/// @file ventilation_state_machine.h
/// @brief Pure-logic state machine for ventilation control.
/// Manages operating modes, direction cycling, and Stoßlüftung timing
/// without any hardware dependencies (GPIO, ESPHome components).

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>


namespace esphome {

/// @brief Operating modes for the ventilation system.
enum VentilationMode {
  MODE_OFF = 0, ///< System idle — fan stopped.
  MODE_ECO_RECOVERY =
      1, ///< Heat recovery — alternating IN/OUT each half-cycle.
  MODE_VENTILATION =
      2, ///< Continuous ventilation — single direction with optional timer.
  MODE_STOSSLUEFTUNG = 3 ///< Burst ventilation — 15 min ON / 105 min OFF cycle.
};

/// @brief Snapshot of desired hardware outputs, computed by the state machine.
struct HardwareState {
  bool fan_enabled;  ///< true = fan should run, false = fan off.
  bool direction_in; ///< true = intake (IN), false = exhaust (OUT).
  float ramp_factor; ///< 0.0 (stop) to 1.0 (full target speed).
  bool needs_update; ///< Reserved — caller compares against previous state.
};

/// @class VentilationStateMachine
/// @brief Hardware-independent state machine that drives the ventilation logic.
/// Tracks the current mode, direction cycle phase, Stoßlüftung timing, and
/// peer synchronization offset.  Call update() every loop iteration.
class VentilationStateMachine {
public:
  static constexpr uint32_t RAMP_DURATION_MS =
      5000; ///< 5s ramp up/down duration.
  // --- Configuration ---
  bool is_phase_a =
      true; ///< Phase group assignment (A starts IN, B starts OUT).
  uint32_t cycle_duration_ms =
      70000; ///< Half-cycle duration in ms (default 70 s).

  // --- State ---
  VentilationMode current_mode = MODE_ECO_RECOVERY; ///< Active operating mode.
  uint32_t ventilation_duration_ms =
      0;                    ///< Timer for MODE_VENTILATION (0 = infinite).
  bool global_phase = true; ///< true = Phase A active in the direction cycle.

  // --- Internal Timing ---
  uint32_t ventilation_start_time =
      0; ///< millis() when MODE_VENTILATION started.
  int32_t time_offset_ms =
      0; ///< Offset applied via sync_time() for peer alignment.

  // --- Stoßlüftung Timing ---
  uint32_t stoss_cycle_start =
      0; ///< millis() when current Stoß sub-phase started.
  bool stoss_active_phase = true; ///< true = fan running, false = pause phase.
  bool stoss_direction_flip =
      false; ///< Alternates direction each active phase.

  /// Stoßlüftung constants (15 min active, 105 min pause = 2 h total cycle).
  static constexpr uint32_t STOSS_ACTIVE_MS = 15UL * 60 * 1000;
  static constexpr uint32_t STOSS_PAUSE_MS = 105UL * 60 * 1000;

  /// @brief One-time initialization (reserved for future use).
  void setup();

  /// @brief Main tick — call every loop(). Returns true if hardware state may
  /// have changed.
  bool update(uint32_t now);

  // --- Actions ---

  /// @brief Switch operating mode and (re)start mode-specific timers.
  void set_mode(VentilationMode mode, uint32_t now, uint32_t duration = 0);
  /// @brief Update the half-cycle duration (one direction) for alternating
  /// airflow, maintaining the current cycle proportion.
  void set_cycle_duration(uint32_t now, uint32_t ms);
  /// @brief Align local cycle phase with a peer's reported position (ESP-NOW
  /// sync).
  void sync_time(uint32_t now, uint32_t target_pos_ms);

  // --- Getters ---

  /// @brief Compute desired fan + direction state from current mode and phase.
  HardwareState get_target_state(uint32_t now) const;
  /// @brief Remaining time in MODE_VENTILATION (ms). 0 if infinite or expired.
  uint32_t get_remaining_duration(uint32_t now) const;
  /// @brief Current position in the full direction cycle (for sync packets).
  uint32_t get_cycle_pos(uint32_t now) const;
};

} // namespace esphome
