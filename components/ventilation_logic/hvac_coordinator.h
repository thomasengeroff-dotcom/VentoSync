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
// File:        hvac_coordinator.h
// Description: Smart Climate Control (HVAC coordination) decision logic.
//              Pure, hardware-agnostic state machine that decides how the
//              Smart-Automatik controller must be restricted while a room
//              air conditioner is active.
// Author:      Thomas Engeroff
// Created:     2026-09-02
// Modified:    2026-09-23
// ==========================================================================
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

/// @file hvac_coordinator.h
/// @brief Pure decision logic for the Smart Climate Control feature.
///
/// The coordinator has no ESPHome or hardware dependencies so it can be
/// compiled into the native unit-test runner. The ESPHome glue code in
/// `components/helpers/auto_mode.h` gathers the inputs from HA entities and
/// sensors, calls `Coordinator::evaluate()`, and applies the resulting
/// `Decision` to the Smart-Automatik controller.
///
/// Design summary (see documentation/en/en_smart-climate-control.md):
///  - The feature is a *modifier* of Smart-Automatik, not a separate mode.
///  - While the AC is active the controller is restricted to a CO2-only
///    loop with a relaxed setpoint and a hard fan-level cap.
///  - Two health guards lift the restrictions: a CO2 emergency and a
///    mold guard (high indoor humidity that ventilation can actually fix).
///  - AC deactivation is debounced so compressor cycling cannot make the
///    fan level oscillate.

namespace ventosync {
namespace hvac {

// --- Defaults & Constants ------------------------------------------------

/// Relaxed CO2 setpoint while the AC is active (DIN EN 13779 IDA 3).
constexpr float DEFAULT_CO2_THRESHOLD_PPM = 1200.0f;
/// CO2 level at which normal automatic regulation resumes regardless of AC.
constexpr float DEFAULT_EMERGENCY_CO2_PPM = 1500.0f;
/// Hard fan-level cap while the AC is active.
constexpr int DEFAULT_MAX_FAN_LEVEL = 3;
/// Absolute minimum fan level while throttled (DIN 1946-6 base ventilation).
constexpr int MIN_FAN_LEVEL = 1;
/// Hardware limit of the VentoMaxx fan levels.
constexpr int HARDWARE_MAX_FAN_LEVEL = 10;
/// The emergency threshold is always kept at least this far above the
/// relaxed setpoint so the emergency hysteresis cannot collapse.
constexpr float MIN_EMERGENCY_MARGIN_PPM = 100.0f;
/// Mold guard: indoor relative humidity that lifts the restrictions when
/// ventilation is able to dry the room (outdoor air is drier in absolute terms).
constexpr float MOLD_GUARD_RH_ON_PERCENT = 70.0f;
/// Mold guard release threshold (hysteresis).
constexpr float MOLD_GUARD_RH_OFF_PERCENT = 65.0f;
/// AC "off" must persist this long before the restrictions are lifted.
/// Absorbs compressor cycling and short Home Assistant reconnects.
constexpr uint32_t AC_RELEASE_DELAY_MS = 120000u;

static_assert(DEFAULT_EMERGENCY_CO2_PPM >= DEFAULT_CO2_THRESHOLD_PPM + MIN_EMERGENCY_MARGIN_PPM,
              "Default emergency CO2 must lie above the relaxed setpoint");
static_assert(MOLD_GUARD_RH_ON_PERCENT > MOLD_GUARD_RH_OFF_PERCENT,
              "Mold guard hysteresis must be positive");
static_assert(DEFAULT_MAX_FAN_LEVEL >= MIN_FAN_LEVEL && DEFAULT_MAX_FAN_LEVEL <= HARDWARE_MAX_FAN_LEVEL,
              "Default HVAC fan cap out of range");

/// @brief Coordinator state reported to the UI / diagnostics.
enum class State : uint8_t {
  DISABLED = 0,          ///< Feature switch is off — Smart-Automatik unchanged.
  STANDBY = 1,           ///< Enabled, AC inactive — Smart-Automatik unchanged.
  SUSPENDED_NO_CO2 = 2,  ///< AC active but no CO2 reading in the room — cannot guarantee health, no throttling.
  THROTTLED = 3,         ///< AC active — CO2-only loop, relaxed setpoint, level cap.
  EMERGENCY_CO2 = 4,     ///< AC active but CO2 too high — restrictions lifted.
  EMERGENCY_HUMIDITY = 5 ///< AC active but mold risk — restrictions lifted.
};

/// @brief Snapshot of everything the coordinator needs for one evaluation.
struct Inputs {
  bool enabled = false;             ///< `smart_climate_control` switch state.
  bool ha_connected = true;         ///< Home Assistant API link is up.
  bool ac_has_state = false;        ///< AC binary sensor has received a value.
  bool ac_reported_active = false;  ///< AC binary sensor value.
  float co2_ppm = NAN;              ///< Effective CO2 (NaN = unavailable).
  float indoor_rh_percent = NAN;    ///< Indoor relative humidity (NaN = unavailable).
  bool ventilation_can_dry = false; ///< Outdoor absolute humidity < indoor.
  float co2_threshold_ppm = DEFAULT_CO2_THRESHOLD_PPM;
  float emergency_co2_ppm = DEFAULT_EMERGENCY_CO2_PPM;
  int max_fan_level = DEFAULT_MAX_FAN_LEVEL;
  uint32_t now_ms = 0;              ///< Current millis().
};

/// @brief Result of one evaluation — what the Smart-Automatik loop must apply.
struct Decision {
  State state = State::DISABLED;
  bool ac_active = false;            ///< Debounced AC state.
  bool restrict_levels = false;      ///< Apply `min_level` / `max_level`.
  bool relaxed_co2_setpoint = false; ///< Use `co2_setpoint` for the CO2 PID.
  bool suppress_humidity = false;    ///< Ignore the humidity PID demand.
  bool lock_eco_mode = false;        ///< Force MODE_ECO_RECOVERY (no summer bypass).
  int min_level = MIN_FAN_LEVEL;
  int max_level = HARDWARE_MAX_FAN_LEVEL;
  float co2_setpoint = DEFAULT_CO2_THRESHOLD_PPM;
};

/// @brief Human-readable (German UI) label for a coordinator state.
inline const char *state_label(State s) {
  switch (s) {
    case State::DISABLED:           return "Deaktiviert";
    case State::STANDBY:            return "Bereit (Klima aus)";
    case State::SUSPENDED_NO_CO2:   return "Ausgesetzt (kein CO2-Wert im Raum)";
    case State::THROTTLED:          return "Aktiv (gedrosselt)";
    case State::EMERGENCY_CO2:      return "Notfall (CO2)";
    case State::EMERGENCY_HUMIDITY: return "Notfall (Feuchte)";
  }
  return "Unbekannt";
}

/**
 * @class   Coordinator
 * @brief   Stateful HVAC coordination state machine.
 *
 * @details Keeps the debounce timer for AC deactivation and the two
 *          emergency latches between evaluations. All thresholds are
 *          taken from `Inputs` on every call so runtime configuration
 *          changes take effect immediately.
 */
class Coordinator {
public:
  /**
   * @brief   Evaluates the coordinator for the current inputs.
   * @param[in] in  Snapshot of sensor and configuration values.
   * @return  Decision to be applied by the Smart-Automatik loop.
   */
  Decision evaluate(const Inputs &in) {
    Decision d;

    // --- Sanitize configuration ------------------------------------------
    const float threshold = std::isnan(in.co2_threshold_ppm)
                                ? DEFAULT_CO2_THRESHOLD_PPM
                                : in.co2_threshold_ppm;
    float emergency = std::isnan(in.emergency_co2_ppm)
                          ? DEFAULT_EMERGENCY_CO2_PPM
                          : in.emergency_co2_ppm;
    emergency = std::max(emergency, threshold + MIN_EMERGENCY_MARGIN_PPM);
    const int cap = std::clamp(in.max_fan_level, MIN_FAN_LEVEL, HARDWARE_MAX_FAN_LEVEL);

    d.co2_setpoint = threshold;

    // --- Feature disabled ------------------------------------------------
    if (!in.enabled) {
      reset_runtime_state();
      d.state = State::DISABLED;
      return d;
    }

    // --- AC state with release debounce ---------------------------------
    // Fail-safe: an unknown AC state (HA offline, entity unavailable) is
    // treated as "inactive" so the ventilation is never throttled blindly.
    const bool reported_active = in.ha_connected && in.ac_has_state && in.ac_reported_active;

    if (reported_active) {
      ac_active_ = true;
      ac_inactive_since_ms_ = 0;
      ac_inactive_pending_ = false;
    } else if (ac_active_) {
      if (!ac_inactive_pending_) {
        ac_inactive_pending_ = true;
        ac_inactive_since_ms_ = in.now_ms;
      } else if (in.now_ms - ac_inactive_since_ms_ >= AC_RELEASE_DELAY_MS) {
        ac_active_ = false;
        ac_inactive_pending_ = false;
      }
    }

    d.ac_active = ac_active_;

    if (!ac_active_) {
      emergency_co2_ = false;
      emergency_humidity_ = false;
      d.state = State::STANDBY;
      return d;
    }

    // --- AC active: heat recovery only, never summer bypass -------------
    d.lock_eco_mode = true;

    // --- Health guard 1: CO2 emergency (hysteresis) ---------------------
    if (std::isnan(in.co2_ppm)) {
      // Without a CO2 reading the health guarantee cannot be given —
      // do not throttle, fall back to the unrestricted automatic logic.
      emergency_co2_ = false;
      d.state = State::SUSPENDED_NO_CO2;
      return d;
    }

    if (emergency_co2_) {
      if (in.co2_ppm <= threshold) emergency_co2_ = false;
    } else if (in.co2_ppm >= emergency) {
      emergency_co2_ = true;
    }

    // --- Health guard 2: mold guard (hysteresis) -------------------------
    // Only meaningful when ventilation can actually remove moisture; if the
    // outdoor air is more humid (absolute), ventilating would not help and
    // the AC (in cooling/dry mode) remains the better dehumidifier.
    if (!std::isnan(in.indoor_rh_percent)) {
      if (emergency_humidity_) {
        if (in.indoor_rh_percent <= MOLD_GUARD_RH_OFF_PERCENT || !in.ventilation_can_dry) {
          emergency_humidity_ = false;
        }
      } else if (in.indoor_rh_percent >= MOLD_GUARD_RH_ON_PERCENT && in.ventilation_can_dry) {
        emergency_humidity_ = true;
      }
    } else {
      emergency_humidity_ = false;
    }

    if (emergency_co2_) {
      d.state = State::EMERGENCY_CO2;
      return d;
    }
    if (emergency_humidity_) {
      d.state = State::EMERGENCY_HUMIDITY;
      return d;
    }

    // --- Throttled: CO2-only loop with relaxed target and level cap -----
    d.state = State::THROTTLED;
    d.restrict_levels = true;
    d.relaxed_co2_setpoint = true;
    d.suppress_humidity = true;
    d.min_level = MIN_FAN_LEVEL;
    d.max_level = cap;
    return d;
  }

  /// @brief True while the debounced AC state is "active".
  bool is_ac_active() const { return ac_active_; }
  /// @brief True while the CO2 emergency latch is set.
  bool is_co2_emergency() const { return emergency_co2_; }
  /// @brief True while the mold-guard latch is set.
  bool is_humidity_emergency() const { return emergency_humidity_; }

  /// @brief Clears all latches and timers (used when the feature is disabled).
  void reset_runtime_state() {
    ac_active_ = false;
    ac_inactive_pending_ = false;
    ac_inactive_since_ms_ = 0;
    emergency_co2_ = false;
    emergency_humidity_ = false;
  }

private:
  bool ac_active_ = false;
  bool ac_inactive_pending_ = false;
  uint32_t ac_inactive_since_ms_ = 0;
  bool emergency_co2_ = false;
  bool emergency_humidity_ = false;
};

} // namespace hvac
} // namespace ventosync
