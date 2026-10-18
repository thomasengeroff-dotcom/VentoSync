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
// File:        auto_level.h
// Description: Pure Smart-Automatik level helpers: soft ramp and following
//              the Master's level. No ESPHome dependencies — unit-tested.
// Author:      Thomas Engeroff
// Created:     2026-10-18
// Modified:    2026-10-18
// ==========================================================================
#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

/// @file auto_level.h
/// @brief Step 5/6 of evaluate_auto_mode() (auto_mode.h) as pure functions.
namespace ventosync {
namespace auto_level {

/**
 * @brief   State of the soft ramp, kept across evaluation cycles.
 *
 * @details Lives in auto_mode_state (auto_mode.h) — one instance for the
 *          whole firmware, it persists across mode switches.
 */
struct RampState {
  bool initialized = false;  ///< False until the first evaluation.
  uint8_t last_mode = 0;     ///< VentilationMode of the previous evaluation.
};

/**
 * @brief   Limits a level change to ±1 per evaluation (±2 in the first
 *          evaluation after the operating mode changed, e.g. summer bypass).
 *
 * @param[in]     current  Current fan level.
 * @param[in]     target   Level the automatic wants.
 * @param[in]     mode     Current VentilationMode (as integer).
 * @param[in,out] state    Ramp state (updated).
 * @return  The level to apply in this evaluation.
 */
inline int ramp_level(int current, int target, uint8_t mode, RampState &state) {
  int max_step = 1;
  if (!state.initialized) {
    state.initialized = true;
    state.last_mode = mode;
  } else if (mode != state.last_mode) {
    max_step = 2;
    state.last_mode = mode;
  }
  if (target > current) return std::min(target, current + max_step);
  if (target < current) return std::max(target, current - max_step);
  return current;
}

/**
 * @brief   The fan level of the room's Master, if a fresh packet from it exists.
 *
 * @details Slaves follow the Master's discrete level in Smart-Automatik so the
 *          devices of a room never fight over level boundaries. Templated over
 *          the peer container (PeerState: device_id, last_seen_ms,
 *          fan_intensity). Wrap-safe freshness check.
 *
 * @return  The Master's level, or std::nullopt if no fresh Master is known.
 */
template <typename Peers>
inline std::optional<int> master_level(const Peers &peers, uint32_t now_ms, uint32_t timeout_ms,
                                       uint8_t master_id) {
  for (const auto &peer : peers) {
    if (peer.device_id == master_id && now_ms - peer.last_seen_ms < timeout_ms) {
      return static_cast<int>(peer.fan_intensity);
    }
  }
  return std::nullopt;
}

}  // namespace auto_level
}  // namespace ventosync
