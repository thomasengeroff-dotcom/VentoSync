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
// File:        nox_boost.h
// Description: NOx boost ventilation (gas hob, fireplace) for Smart-Automatik.
//              Pure, unit-tested state machine — no ESPHome dependencies.
// Author:      Thomas Engeroff
// Created:     2026-10-16
// Modified:    2026-10-16
// ==========================================================================
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

/// @file nox_boost.h
/// @brief Event-driven boost on the SGP41 NOx index (sgp41 variants).
///
/// Rules:
///  - Only while the room-wide switch "NOx-Stoßlüftung" is on AND
///    Smart-Automatik is active (the caller passes `enabled`).
///  - Start: NOx index >= threshold (HA number, 20–100) for `TRIGGER_MS`.
///  - Active until the index has stayed below the release level (half the
///    threshold) for `HOLD_MS`; capped at `MAX_DURATION_MS`.
///  - Outdoor-source test: while active, every intake window (the device
///    blows outdoor air in) must LOWER the index (value at the end of the
///    window < value at its start). If it does not, the NOx most likely comes
///    from outside (road traffic) and more ventilation would import it — the
///    boost is aborted immediately. Short windows (< `INTAKE_TEST_MIN_MS`,
///    e.g. interrupted by a direction change) are not judged; continuous
///    intake (summer bypass) is judged every `INTAKE_TEST_MAX_MS`.
///  - After an abort (outdoor air or max. duration) the detector stays locked
///    until the index has fallen below the release level, so the same event
///    cannot re-trigger it.
///  - The caller broadcasts only this device's own `active()` / `strong()`
///    (ESP-NOW room flags) — never the room-wide OR.

namespace ventosync {
namespace nox {

/// Threshold setting — must match the HA number in sensor_SGP41_SHT45.yaml.
constexpr int THRESHOLD_MIN = 20;
constexpr int THRESHOLD_MAX = 100;
constexpr int THRESHOLD_DEFAULT = 20;
static_assert(THRESHOLD_MIN <= THRESHOLD_DEFAULT && THRESHOLD_DEFAULT <= THRESHOLD_MAX,
              "Default NOx threshold outside its range");

/// Index at which an active boost counts as a health emergency: it lifts the
/// Smart Climate Control fan cap and a user level override.
constexpr float STRONG_INDEX = 150.0f;

constexpr uint32_t TRIGGER_MS = 60u * 1000u;               ///< Above threshold this long → start.
constexpr uint32_t HOLD_MS = 10u * 60u * 1000u;            ///< Below release this long → end.
constexpr uint32_t MAX_DURATION_MS = 60u * 60u * 1000u;    ///< Hard cap of one boost.
constexpr uint32_t INTAKE_TEST_MIN_MS = 20u * 1000u;       ///< Shortest intake window judged.
constexpr uint32_t INTAKE_TEST_MAX_MS = 60u * 1000u;       ///< Continuous intake: judge every 60 s.
static_assert(INTAKE_TEST_MIN_MS < INTAKE_TEST_MAX_MS, "Intake test window bounds inverted");

/// @brief Threshold setting clamped into 20–100.
inline float clamp_threshold(float threshold) {
  if (std::isnan(threshold)) return static_cast<float>(THRESHOLD_DEFAULT);
  return std::clamp(threshold, static_cast<float>(THRESHOLD_MIN), static_cast<float>(THRESHOLD_MAX));
}

/// @brief Release level (hysteresis): half the threshold.
inline float release_level(float threshold) { return clamp_threshold(threshold) * 0.5f; }

enum class State : uint8_t {
  IDLE,    ///< Waiting for the index to exceed the threshold.
  ARMING,  ///< Above threshold, waiting for TRIGGER_MS.
  ACTIVE,  ///< Boost running, index above the release level.
  HOLD,    ///< Boost running, index below release — counting HOLD_MS down.
  LOCKOUT, ///< Aborted; re-armed once the index falls below release.
};

enum class Event : uint8_t {
  NONE,
  STARTED,             ///< Boost started.
  ENDED,               ///< Boost ended normally (index back to normal, switched off, sensor lost).
  ABORTED_OUTDOOR,     ///< Index did not fall while outdoor air was blown in.
  ABORTED_MAX_DURATION ///< MAX_DURATION_MS reached.
};

struct Inputs {
  uint32_t now_ms = 0;
  bool enabled = false;   ///< Room-wide switch on AND Smart-Automatik active.
  float nox_index = NAN;  ///< Own SGP41 NOx index (NaN = no sensor / no data).
  bool intake = false;    ///< This device currently blows outdoor air in (fan running).
  float threshold = static_cast<float>(THRESHOLD_DEFAULT);
};

/// @brief NOx boost detector of ONE device (its own sensor).
class Detector {
 public:
  State state = State::IDLE;
  uint32_t state_since_ms = 0;  ///< Entry time of ARMING / HOLD / LOCKOUT.
  uint32_t boost_start_ms = 0;  ///< Start of the running boost.
  float last_index = NAN;

  /// Boost running (this device's own input).
  bool active() const { return state == State::ACTIVE || state == State::HOLD; }

  /// Running boost with a very high index (health emergency).
  bool strong() const { return active() && !std::isnan(last_index) && last_index >= STRONG_INDEX; }

  /// Remaining time until the hard cap (0 when not active).
  uint32_t remaining_max_ms(uint32_t now) const {
    if (!active()) return 0;
    const uint32_t elapsed = now - boost_start_ms;
    return elapsed >= MAX_DURATION_MS ? 0 : MAX_DURATION_MS - elapsed;
  }

  /**
   * @brief   Advances the state machine; call every evaluation cycle (~10 s).
   * @return  The event of this step (at most one).
   */
  Event update(const Inputs &in) {
    const uint32_t now = in.now_ms;
    last_index = in.nox_index;
    const bool was_active = active();

    // Switched off / left Smart-Automatik / sensor lost: end silently-normal.
    if (!in.enabled || std::isnan(in.nox_index)) {
      reset_(now);
      return was_active ? Event::ENDED : Event::NONE;
    }

    const float threshold = clamp_threshold(in.threshold);
    const float release = release_level(threshold);
    const float idx = in.nox_index;

    switch (state) {
      case State::IDLE:
        if (idx >= threshold) enter_(State::ARMING, now);
        break;

      case State::ARMING:
        if (idx < threshold) {
          enter_(State::IDLE, now);
        } else if (now - state_since_ms >= TRIGGER_MS) {
          enter_(State::ACTIVE, now);
          boost_start_ms = now;
          start_window_(in.intake, idx, now);
          return Event::STARTED;
        }
        break;

      case State::ACTIVE:
      case State::HOLD: {
        if (now - boost_start_ms >= MAX_DURATION_MS) {
          enter_(State::LOCKOUT, now);
          return Event::ABORTED_MAX_DURATION;
        }
        if (state == State::ACTIVE && intake_test_failed_(in.intake, idx, now)) {
          enter_(State::LOCKOUT, now);
          return Event::ABORTED_OUTDOOR;
        }
        if (state == State::ACTIVE) {
          if (idx < release) enter_(State::HOLD, now);
        } else {  // HOLD
          if (idx >= release) {
            state = State::ACTIVE;  // keep boost_start_ms
            start_window_(in.intake, idx, now);
          } else if (now - state_since_ms >= HOLD_MS) {
            enter_(State::IDLE, now);
            return Event::ENDED;
          }
        }
        break;
      }

      case State::LOCKOUT:
        if (idx < release) enter_(State::IDLE, now);
        break;
    }
    return Event::NONE;
  }

 private:
  bool in_intake_ = false;
  uint32_t window_start_ms_ = 0;
  float window_start_index_ = NAN;

  void enter_(State s, uint32_t now) {
    state = s;
    state_since_ms = now;
  }

  void reset_(uint32_t now) {
    enter_(State::IDLE, now);
    in_intake_ = false;
    window_start_index_ = NAN;
  }

  void start_window_(bool intake, float idx, uint32_t now) {
    in_intake_ = intake;
    window_start_ms_ = now;
    window_start_index_ = idx;
  }

  /// True if a completed intake window did not lower the index.
  bool intake_test_failed_(bool intake, float idx, uint32_t now) {
    if (intake && !in_intake_) {  // intake window starts
      start_window_(true, idx, now);
      return false;
    }
    if (!in_intake_) return false;  // exhaust / stopped: nothing to judge
    const uint32_t duration = now - window_start_ms_;
    const bool window_closed = !intake;
    if (!window_closed && duration < INTAKE_TEST_MAX_MS) return false;  // still collecting
    const bool judged = duration >= INTAKE_TEST_MIN_MS && !std::isnan(window_start_index_);
    const bool failed = judged && !(idx < window_start_index_);
    // Continuous intake: the next window starts right away.
    if (window_closed) {
      in_intake_ = false;
      window_start_index_ = NAN;
    } else {
      start_window_(true, idx, now);
    }
    return failed;
  }
};

/// @brief Short German label of an event (logs, HA event description).
inline const char *event_label(Event e) {
  switch (e) {
    case Event::NONE:                 return "";
    case Event::STARTED:              return "NOx-Stoßlüftung gestartet";
    case Event::ENDED:                return "NOx-Stoßlüftung beendet";
    case Event::ABORTED_OUTDOOR:      return "NOx-Stoßlüftung abgebrochen (NOx aus der Außenluft)";
    case Event::ABORTED_MAX_DURATION: return "NOx-Stoßlüftung abgebrochen (Maximaldauer)";
  }
  return "";
}

}  // namespace nox
}  // namespace ventosync
