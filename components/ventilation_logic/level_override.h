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
// File:        level_override.h
// Description: Pure logic of the temporary Smart-Automatik level override.
// Author:      Thomas Engeroff
// Created:     2026-10-11
// Modified:    2026-10-11
// ==========================================================================
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

/// @file level_override.h
/// @brief Temporary manual fan level on top of Smart-Automatik.
///
/// Pressing the Level button while Smart-Automatik is active does not fight
/// the controller any more: it starts a room-wide override that holds the
/// chosen level for the configured duration (30–300 min, default 60, room-wide
/// HA setting "Smart-Automatik: Übersteuerung Dauer"). Afterwards the automatic resumes
/// from that level with its normal ±1 level ramp.
///
/// Rules:
///  - The override is room-wide state like the operating mode: it travels in
///    every ESP-NOW packet as (level, remaining seconds). MSG_STATE from any
///    device is adopted in full (incl. "none"), so the last interaction wins.
///    The Master heartbeat only starts / re-aligns an override and never ends
///    one — a freshly rebooted Master (override lost from RAM) must not cancel
///    the room's override before it has re-synced from its peers.
///  - Each device counts the remaining time down on its own clock; adopting
///    a packet never restarts the countdown, so re-sending cannot extend it.
///  - A mode change ends it (`cycle_operating_mode()`).
///  - Safety wins: the Smart Climate Control cap clamps the level, and a
///    health emergency (CO2 / mold guard) lets the automatic raise the level
///    above it — see `resolve_level()`.

namespace ventosync {
namespace auto_override {

/// Override duration setting (minutes) — must match the HA number
/// `auto_override_duration` in ui_controls.yaml.
constexpr int DEFAULT_DURATION_MIN = 60;
constexpr int MIN_DURATION_MIN = 30;
constexpr int MAX_DURATION_MIN = 300;
/// Longest possible override; also the upper bound for adopted packet values.
constexpr uint32_t MAX_DURATION_MS = static_cast<uint32_t>(MAX_DURATION_MIN) * 60u * 1000u;
constexpr uint32_t DEFAULT_DURATION_MS = static_cast<uint32_t>(DEFAULT_DURATION_MIN) * 60u * 1000u;
static_assert(MIN_DURATION_MIN <= DEFAULT_DURATION_MIN && DEFAULT_DURATION_MIN <= MAX_DURATION_MIN,
              "Default override duration outside its range");
static_assert(MAX_DURATION_MS / 1000u <= 0xFFFFu,
              "Remaining override time must fit the uint16_t packet field (seconds)");

/// @brief True if `minutes` is a valid duration setting (30–300).
inline bool valid_duration_min(int minutes) {
  return minutes >= MIN_DURATION_MIN && minutes <= MAX_DURATION_MIN;
}

/// @brief Duration setting in ms, clamped into 30–300 min.
inline uint32_t duration_ms_from_min(int minutes) {
  return static_cast<uint32_t>(std::clamp(minutes, MIN_DURATION_MIN, MAX_DURATION_MIN)) * 60u * 1000u;
}

/// Adopted remaining times closer than this to the local countdown are not
/// re-aligned (packet latency, heartbeat jitter).
constexpr uint32_t REALIGN_TOLERANCE_MS = 5000u;

/// @brief True for a valid fan level (1–10).
inline bool valid_level(int level) { return level >= 1 && level <= 10; }

/**
 * @brief   Room-wide manual level that temporarily replaces the automatic.
 *
 * @details `level == 0` means inactive. All time comparisons are
 *          `now - start < duration` (wrap-safe across the millis() wrap).
 */
struct LevelOverride {
  uint8_t level = 0;        ///< Override level 1–10, 0 = inactive.
  uint32_t start_ms = 0;    ///< millis() at which the countdown started.
  uint32_t duration_ms = 0; ///< Countdown length from `start_ms`.

  /// @brief Starts (or restarts) the override at `new_level` for `duration`.
  /// @return false if `new_level` is not a valid fan level (nothing changes).
  bool start(int new_level, uint32_t now_ms, uint32_t duration = DEFAULT_DURATION_MS) {
    if (!valid_level(new_level) || duration == 0) return false;
    level = static_cast<uint8_t>(new_level);
    start_ms = now_ms;
    duration_ms = std::min(duration, MAX_DURATION_MS);
    return true;
  }

  /// @brief Ends the override.
  void clear() {
    level = 0;
    start_ms = 0;
    duration_ms = 0;
  }

  /// @brief True while a level is set and the countdown has not elapsed.
  bool active(uint32_t now_ms) const {
    return level != 0 && static_cast<uint32_t>(now_ms - start_ms) < duration_ms;
  }

  /// @brief Remaining time in ms (0 when inactive or elapsed).
  uint32_t remaining_ms(uint32_t now_ms) const {
    if (!active(now_ms)) return 0;
    return duration_ms - static_cast<uint32_t>(now_ms - start_ms);
  }

  /// @brief Clears an elapsed override. @return true on the expiry edge.
  bool expire(uint32_t now_ms) {
    if (level == 0 || active(now_ms)) return false;
    clear();
    return true;
  }

  /// @brief Level for the packet (0 when inactive).
  uint8_t wire_level(uint32_t now_ms) const { return active(now_ms) ? level : 0; }

  /// @brief Remaining seconds for the packet, rounded up (0 when inactive).
  uint16_t wire_remaining_s(uint32_t now_ms) const {
    const uint32_t ms = remaining_ms(now_ms);
    return static_cast<uint16_t>(std::min<uint32_t>((ms + 999u) / 1000u, MAX_DURATION_MS / 1000u));
  }

  /**
   * @brief   Adopts the room's override state from a peer packet.
   *
   * @details `wire_level == 0` (or no remaining time) ends a local override
   *          if `allow_cancel` (explicit interaction, MSG_STATE); otherwise
   *          it is ignored (heartbeat).
   *          A different level, or an override this device did not know,
   *          is taken over with the sender's remaining time. The same level
   *          only re-aligns the countdown when it deviates by more than
   *          `REALIGN_TOLERANCE_MS` — never a visible change. Out-of-range
   *          levels are ignored (corrupt or foreign packet).
   *
   * @return  true if the visible state changed (started, level changed or
   *          ended) — the caller then applies the level and refreshes LEDs.
   */
  bool adopt(uint8_t wire_lvl, uint16_t wire_remaining, uint32_t now_ms, bool allow_cancel = true) {
    if (wire_lvl == 0 || wire_remaining == 0) {
      if (!allow_cancel) return false;
      const bool was_active = active(now_ms);
      clear();
      return was_active;
    }
    if (!valid_level(wire_lvl)) return false;

    const uint32_t remote_ms =
        std::min<uint32_t>(static_cast<uint32_t>(wire_remaining) * 1000u, MAX_DURATION_MS);
    if (!active(now_ms) || level != wire_lvl) {
      start(wire_lvl, now_ms, remote_ms);
      return true;
    }
    const uint32_t local_ms = remaining_ms(now_ms);
    const uint32_t diff = (local_ms > remote_ms) ? local_ms - remote_ms : remote_ms - local_ms;
    if (diff > REALIGN_TOLERANCE_MS) start(wire_lvl, now_ms, remote_ms);
    return false;
  }
};

/**
 * @brief   Health guard of the override: CO2 far above the setpoint.
 *
 * @details Independent of Smart Climate Control, so an override to a low
 *          level can never let the room's CO2 run away. Latches at
 *          `on_ppm` (the `hvac_emergency_co2` slider, default 1500 ppm) and
 *          releases at `off_ppm` (the normal CO2 setpoint). NaN (no CO2 in
 *          the room) releases the latch — nothing to protect against.
 */
struct Co2Guard {
  bool latched = false;

  /// @return true while the guard is latched.
  bool update(float co2_ppm, float on_ppm, float off_ppm) {
    if (std::isnan(co2_ppm) || std::isnan(on_ppm) || std::isnan(off_ppm)) {
      latched = false;
      return false;
    }
    if (off_ppm >= on_ppm) off_ppm = on_ppm - 1.0f; // keep a hysteresis band
    if (latched) {
      if (co2_ppm <= off_ppm) latched = false;
    } else if (co2_ppm >= on_ppm) {
      latched = true;
    }
    return latched;
  }
};

/**
 * @brief   Fan level target while an override is active.
 *
 * @param[in] override_level  Level chosen by the user (1–10).
 * @param[in] auto_level      Level the automatic would target right now
 *                            (already inside its window).
 * @param[in] restrict_levels Smart Climate Control cap active.
 * @param[in] min_level       Cap window minimum (used if `restrict_levels`).
 * @param[in] max_level       Cap window maximum (used if `restrict_levels`).
 * @param[in] emergency       Health emergency (CO2 / mold guard).
 *
 * @return  The override level, clamped into the cap window while the AC
 *          throttles the room; during an emergency at least `auto_level`.
 *          The user's automatic min/max window is deliberately NOT applied —
 *          the override may go below or above it.
 */
inline int resolve_level(int override_level, int auto_level, bool restrict_levels,
                         int min_level, int max_level, bool emergency) {
  int level = std::clamp(override_level, 1, 10);
  if (restrict_levels) {
    if (min_level > max_level) std::swap(min_level, max_level);
    level = std::clamp(level, std::clamp(min_level, 1, 10), std::clamp(max_level, 1, 10));
  }
  if (emergency) level = std::max(level, std::clamp(auto_level, 1, 10));
  return level;
}

} // namespace auto_override
} // namespace ventosync
