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
// File:        ventilation_state_machine.h
// Description: State machine for ventilation modes and timing.
// Author:      Thomas Engeroff
// Created:     2026-02-15
// Modified:    2026-09-24
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
/// IMPORTANT: These values are serialized into VentilationPacket (uint8_t)
/// and transmitted via ESP-NOW. Do NOT renumber existing values — this
/// would break protocol compatibility. Bump PROTOCOL_VERSION if adding modes.
enum VentilationMode : uint8_t {
  MODE_OFF = 0,            ///< System idle — fan stopped.
  MODE_ECO_RECOVERY = 1,   ///< Heat recovery — alternating IN/OUT each half-cycle.
  MODE_VENTILATION = 2,    ///< Continuous ventilation — single direction with optional timer.
  MODE_STOSSLUEFTUNG = 3,  ///< Burst ventilation — 15 min ON / 105 min OFF cycle.
};

static_assert(sizeof(VentilationMode) == sizeof(uint8_t),
    "VentilationMode must fit in uint8_t for VentilationPacket compatibility");

/// @brief Snapshot of desired hardware outputs, computed by the state machine.
struct HardwareState {
  bool fan_enabled;  ///< true = fan should run, false = fan off.
  bool direction_in; ///< true = intake (IN), false = exhaust (OUT).
  float ramp_factor; ///< 0.0 (stop) to 1.0 (full target speed).
  // FIXED H-3: Removed dead field 'needs_update' — was always false, never read.
  // Caller (VentilationController) compares ramp_factor/dirty flag instead.
};

/**
 * @class   VentilationStateMachine
 * @brief   Pure logic core for driving a single ventilation unit's cycle.
 *
 * @details This class is hardware-agnostic and manages the timing of
 *          intake/exhaust cycles, the transition through ramping phases,
 *          and the global synchronization phase.
 *
 * @note    Designed to be deterministic; given the same millis() and state,
 *          it will always produce the same target HardwareState.
 */
class VentilationStateMachine {
public:
  static constexpr uint32_t RAMP_DURATION_MS =
      5000; ///< 5s ramp up/down duration.

  // FIXED M-1: Named default for compile-time verification against RAMP_DURATION_MS
  static constexpr uint32_t DEFAULT_CYCLE_DURATION_MS = 70000;
  static_assert(DEFAULT_CYCLE_DURATION_MS >= 2 * RAMP_DURATION_MS,
      "Default cycle_duration_ms must be >= 2x RAMP_DURATION_MS "
      "for ramp-up and ramp-down to fit without overlap");

  // --- Configuration ---
  bool is_phase_a =
      true; ///< Phase group assignment (A starts IN, B starts OUT).
  uint32_t cycle_duration_ms =
      DEFAULT_CYCLE_DURATION_MS; ///< Half-cycle duration in ms (default 70 s).

  // --- State ---
  VentilationMode current_mode = MODE_ECO_RECOVERY; ///< Active operating mode.
  uint32_t ventilation_duration_ms =
      0;                    ///< Timer for MODE_VENTILATION (0 = infinite).
  bool global_phase = true; ///< true = Phase A active in the direction cycle.

  // --- Internal Timing ---
  uint32_t ventilation_start_time =
      0; ///< millis() when MODE_VENTILATION started.
  /// Set by update() when the MODE_VENTILATION timer expired and the machine
  /// fell back to MODE_ECO_RECOVERY. The ESPHome glue consumes (clears) it to
  /// update the UI mode index / HA select, which the pure state machine
  /// cannot reach.
  bool ventilation_timer_expired = false;
  int32_t time_offset_ms =
      0; ///< Offset applied via sync_time() for peer alignment.

  // --- Stoßlüftung Timing ---
  // The burst schedule is a 4 h super-cycle of two 2 h cycles (15 min burst +
  // 105 min pause). During a burst the fan runs in ONE direction (Phase A in,
  // Phase B out); the second burst of the super-cycle inverts both directions.
  // Active phase and direction are derived from the position in the
  // super-cycle, so a single number (get_remaining_duration()) is enough to
  // align all devices of a room (sync_stoss()).
  uint32_t stoss_cycle_start =
      0; ///< millis() anchor of the current Stoß super-cycle.
  bool stoss_active_phase = true; ///< true = burst (fan running), false = pause.
  bool stoss_direction_flip =
      false; ///< false = first burst (A in / B out), true = second burst (inverted).

  /// Stoßlüftung constants (15 min active, 105 min pause = 2 h total cycle).
  // FIXED K-2: Use u suffix + static_assert for compile-time overflow verification
  static constexpr uint32_t STOSS_ACTIVE_MS = 15u * 60u * 1000u;
  static constexpr uint32_t STOSS_PAUSE_MS  = 105u * 60u * 1000u;

  static_assert(STOSS_ACTIVE_MS == 900000u,
      "STOSS_ACTIVE_MS unexpected value — check for multiplication overflow");
  static_assert(STOSS_PAUSE_MS == 6300000u,
      "STOSS_PAUSE_MS unexpected value — check for multiplication overflow");
  static_assert(STOSS_ACTIVE_MS + STOSS_PAUSE_MS > STOSS_ACTIVE_MS,
      "STOSS total cycle overflows uint32_t");
  /// One burst + pause (2 h).
  static constexpr uint32_t STOSS_CYCLE_MS = STOSS_ACTIVE_MS + STOSS_PAUSE_MS;
  /// Two cycles with opposite burst directions (4 h) — the synced period.
  static constexpr uint32_t STOSS_SUPER_CYCLE_MS = 2u * STOSS_CYCLE_MS;
  static_assert(STOSS_SUPER_CYCLE_MS == 14400000u,
      "STOSS_SUPER_CYCLE_MS unexpected value — check for multiplication overflow");

  /**
   * @brief   One-time initialization (reserved for future use).
   */
  void setup();

  /**
   * @brief   Main tick — advanced every loop cycle.
   * @details Advanced timers (Stoßlüftung, Manual Timer) and direction cycle.
   * @param[in] now  Current system time in milliseconds.
   * @return  true if any state change occurred (hardware update needed).
   */
  bool update(uint32_t now);

  // --- Actions ---

  /**
   * @brief   Switch operating mode and (re)start mode-specific timers.
   * @param[in] mode      Target VentilationMode.
   * @param[in] now       Current system time in milliseconds.
   * @param[in] duration  For MODE_VENTILATION: auto-stop timer in ms (0 = infinite).
   *                      For MODE_STOSSLUEFTUNG: 0 = start a new schedule (if not
   *                      already running), > 0 = peer's remaining super-cycle time
   *                      to align with (see sync_stoss()).
   */
  void set_mode(VentilationMode mode, uint32_t now, uint32_t duration = 0);
  /**
   * @brief   Update the half-cycle duration (one direction) for alternating
   *          airflow, maintaining the current cycle proportion.
   * @param[in] now  Current system time in milliseconds.
   * @param[in] ms   New half-cycle duration in milliseconds.
   */
  void set_cycle_duration(uint32_t now, uint32_t ms);
  /**
   * @brief   Align local cycle phase with a peer's reported position.
   * @param[in] now            Current system time in milliseconds.
   * @param[in] target_pos_ms  Target cycle position (0 to 2xHalfCycle).
   */
  void sync_time(uint32_t now, uint32_t target_pos_ms);
  /**
   * @brief   Align the Stoßlüftung schedule with a peer (normally the Master).
   * @param[in] now           Current system time in milliseconds.
   * @param[in] remaining_ms  Peer's remaining time in the super-cycle
   *                          (1 … STOSS_SUPER_CYCLE_MS); 0 or out of range is ignored.
   */
  void sync_stoss(uint32_t now, uint32_t remaining_ms);

  // --- Getters ---

  /**
   * @brief   Calculates the target hardware state for a given timestamp.
   *
   * @details Determines if the fan should be enabled, which direction it
   *          should spin, and the intensity ramp factor (for smooth
   *          direction changes).
   *
   * @param[in] now_ms  The current system time in milliseconds.
   *
   * @return  HardwareState  A composite struct for the low-level drivers.
   */
  HardwareState get_target_state(uint32_t now) const;
  /**
   * @brief   Calculates the remaining duration for timed modes.
   *
   * @details MODE_VENTILATION: remaining timer (0 = continuous).
   *          MODE_STOSSLUEFTUNG: remaining time in the 4 h super-cycle
   *          (1 … STOSS_SUPER_CYCLE_MS), used by peers to align the bursts.
   *          Returns 0 for all other modes.
   */
  uint32_t get_remaining_duration(uint32_t now) const;
  /**
   * @brief   Position in the Stoßlüftung super-cycle (0 … STOSS_SUPER_CYCLE_MS-1).
   * @param[in] now  Current system time in milliseconds.
   */
  uint32_t get_stoss_pos(uint32_t now) const;
  /**
   * @brief   Returns the current position in the full direction cycle.
   * @param[in] now  Current system time in milliseconds.
   * @return  Position in milliseconds (0 to 2xHalfCycle).
   */
  uint32_t get_cycle_pos(uint32_t now) const;
};

} // namespace esphome
