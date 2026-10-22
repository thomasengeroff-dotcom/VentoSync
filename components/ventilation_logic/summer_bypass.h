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
// File:        summer_bypass.h
// Description: Pure summer bypass decision (Smart-Automatik "Sommer-Kühlung")
//              incl. hysteresis and the re-measure guard. Unit-tested.
// Author:      Thomas Engeroff
// Created:     2026-10-18
// Modified:    2026-10-22
// ==========================================================================
#pragma once

#include <cmath>
#include <cstdint>

/// @file summer_bypass.h
/// @brief In Smart-Automatik the room switches from heat recovery to
/// continuous ventilation (bypass) to cool with outdoor air when it is summer
/// (HA "Sommerbetrieb"), indoors is warmer than the threshold and outdoors is
/// sufficiently cooler. The glue (auto_mode.h) maps `bypass` to
/// MODE_VENTILATION / MODE_ECO_RECOVERY and logs the reason.
namespace ventosync {
namespace summer {

constexpr float DEFAULT_THRESHOLD_C = 22.0f;  ///< Factory default (default_summer_cooling_threshold).
/// Range of the room-wide HA number "Sommerkühlung Schwelle" (°C) — also the
/// range ventosync::packet::parse() accepts (protocol v15).
constexpr int THRESHOLD_MIN_C = 18;
constexpr int THRESHOLD_MAX_C = 30;
static_assert(DEFAULT_THRESHOLD_C >= THRESHOLD_MIN_C && DEFAULT_THRESHOLD_C <= THRESHOLD_MAX_C,
              "HA slider range 18-30");
constexpr float MIN_DELTA_C = 1.5f;           ///< Outdoor must be this much cooler to start.
constexpr float DELTA_HYSTERESIS_C = 0.5f;    ///< Stop when outdoor is less than this cooler.
constexpr float INDOOR_HYSTERESIS_C = 0.5f;   ///< Stop when indoor falls this far below the threshold.
/// After the bypass ended because a temperature was not measurable, stay in
/// heat recovery at least this long so both NTCs publish fresh values
/// (> 2 full push-pull cycles incl. NTC stabilisation) before re-entering.
constexpr uint32_t REMEASURE_MS = 300000u;

/// Why decide() chose its result (for the log).
enum class Reason : uint8_t {
  HOLD,                    ///< No change of the current state.
  AC_LOCK,                 ///< Smart Climate Control: heat recovery enforced.
  NO_DATA,                 ///< A temperature is unknown — keep the state.
  ACTIVATED,               ///< Bypass starts.
  DEACTIVATED_SEASON,      ///< Not summer any more.
  DEACTIVATED_HYSTERESIS,  ///< Outdoor not cool enough / indoor cooled down.
};

struct Decision {
  bool bypass = false;
  Reason reason = Reason::HOLD;
};

/**
 * @brief   Summer bypass with activation thresholds and hysteresis.
 *
 * @param[in] bypass_active  The bypass (MODE_VENTILATION) is currently active.
 * @param[in] eff_in         Effective indoor temperature (NaN = unknown).
 * @param[in] eff_out        Effective outdoor temperature (NaN = unknown).
 * @param[in] is_summer      HA "Sommerbetrieb" (false while HA is offline).
 * @param[in] threshold      Indoor threshold in °C (HA number).
 * @param[in] ac_lock        Smart Climate Control enforces heat recovery.
 */
inline Decision decide(bool bypass_active, float eff_in, float eff_out, bool is_summer, float threshold,
                       bool ac_lock) {
  if (ac_lock) return {false, Reason::AC_LOCK};
  if (std::isnan(eff_in) || std::isnan(eff_out)) return {bypass_active, Reason::NO_DATA};
  if (!bypass_active) {
    if (is_summer && eff_in > threshold && eff_out < eff_in - MIN_DELTA_C) return {true, Reason::ACTIVATED};
    return {false, Reason::HOLD};
  }
  if (!is_summer) return {false, Reason::DEACTIVATED_SEASON};
  if (eff_out >= eff_in - DELTA_HYSTERESIS_C || eff_in < threshold - INDOOR_HYSTERESIS_C) {
    return {false, Reason::DEACTIVATED_HYSTERESIS};
  }
  return {true, Reason::HOLD};
}

/**
 * @brief   Re-measure guard: in continuous ventilation an exhaust-only device
 *          cannot measure outdoors (an intake-only one not indoors). If no peer
 *          or held reading fills the gap, end the bypass and block it for
 *          REMEASURE_MS so both NTCs are measured again in heat recovery —
 *          otherwise decide() would hold the bypass forever on missing data.
 */
class RemeasureGuard {
 public:
  /// @param[out] ended_unmeasurable  True if this call ended the bypass (log).
  /// @return The bypass state to apply.
  bool apply(bool bypass_active, bool target_bypass, float eff_in, float eff_out, uint32_t now_ms,
             bool &ended_unmeasurable) {
    ended_unmeasurable = false;
    if (bypass_active && (std::isnan(eff_in) || std::isnan(eff_out))) {
      blocked_ = true;
      since_ms_ = now_ms;
      ended_unmeasurable = true;
      return false;
    }
    if (blocked_) {
      if (now_ms - since_ms_ < REMEASURE_MS) return false;
      blocked_ = false;
    }
    return target_bypass;
  }

  bool blocked() const { return blocked_; }

 private:
  bool blocked_ = false;
  uint32_t since_ms_ = 0;
};

}  // namespace summer
}  // namespace ventosync
