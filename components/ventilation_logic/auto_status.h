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
// File:        auto_status.h
// Description: "What is the ventilation doing and why" — reason selection
//              and German status text for Home Assistant. Pure, unit-tested.
// Author:      Thomas Engeroff
// Created:     2026-10-16
// Modified:    2026-10-16
// ==========================================================================
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

/// @file auto_status.h
/// @brief Status text ("Automatik-Status"), reason key ("Automatik-Grund")
///        and the debounced reason change that fires the HA event.

namespace ventosync {
namespace auto_status {

/// Reason for the current fan level, highest priority first.
enum class Reason : uint8_t {
  OFF,                ///< Mode "Aus".
  VACATION,           ///< Vacation mode.
  WINDOW,             ///< Paused by the window guard.
  MANUAL,             ///< Manual mode (Wärmerückgewinnung / Durchlüften / Stoßlüftung).
  NOX,                ///< NOx boost (gas hob, fireplace).
  EMERGENCY_CO2,      ///< CO2 emergency (Smart Climate Control or override guard).
  EMERGENCY_HUMIDITY, ///< Mold guard (Smart Climate Control).
  OVERRIDE,           ///< User level override (Level button, HA slider).
  NO_DATA,            ///< No sensor data — last level held.
  CO2,                ///< CO2 above the setpoint.
  HUMIDITY,           ///< Humidity above the setpoint.
  BASE,               ///< Base ventilation, no demand.
};

/// Stable key of a reason — the "Automatik-Grund" text sensor and the HA
/// event types (event entity "Lüftung Ereignis") use exactly these strings.
inline const char *reason_key(Reason r) {
  switch (r) {
    case Reason::OFF:                return "aus";
    case Reason::VACATION:           return "urlaub";
    case Reason::WINDOW:             return "fenster";
    case Reason::MANUAL:             return "manuell";
    case Reason::NOX:                return "nox";
    case Reason::EMERGENCY_CO2:      return "co2_notfall";
    case Reason::EMERGENCY_HUMIDITY: return "schimmelschutz";
    case Reason::OVERRIDE:           return "uebersteuerung";
    case Reason::NO_DATA:            return "keine_daten";
    case Reason::CO2:                return "co2";
    case Reason::HUMIDITY:           return "feuchte";
    case Reason::BASE:               return "grundlueftung";
  }
  return "grundlueftung";
}

/// Demand below which the automatic counts as "no demand" (matches the CO2
/// PID grab threshold in auto_mode.h).
constexpr float DEMAND_EPSILON = 0.01f;

/// Snapshot of everything the status needs (filled by auto_mode.h).
struct Inputs {
  bool mode_off = false;
  bool vacation = false;
  bool window_paused = false;
  bool auto_mode = false;
  const char *mode_label = "";   ///< German mode name (manual modes).
  int level = 0;                 ///< Current fan level 1–10.
  bool summer_cooling = false;   ///< Smart-Automatik runs the summer bypass.

  bool nox_active = false;       ///< Room-wide NOx boost.
  float nox_index = NAN;         ///< Own NOx index (NaN on devices without SGP41).
  bool nox_from_peer = false;    ///< Boost reported by another device of the room.
  bool nox_strong = false;       ///< Strong event — lifts the AC cap.

  bool emergency_co2 = false;
  bool emergency_humidity = false;
  bool override_active = false;
  int override_remaining_min = 0;

  float demand = NAN;            ///< Combined room demand 0–1 (NaN = no data).
  bool co2_controlling = false;  ///< CO2 holds priority (PID hysteresis).
  float room_co2 = NAN;          ///< Room-wide max CO2 (ppm, may be a VOC equivalent).
  float co2_setpoint = NAN;
  float room_rh = NAN;           ///< Room-wide max humidity (%).
  float rh_setpoint = NAN;

  bool ac_cap = false;           ///< Smart Climate Control caps the level.
  int ac_max_level = 0;
};

struct Status {
  Reason reason = Reason::BASE;
  std::string text;
};

namespace detail {
inline std::string fmt(const char *format, double a = 0, double b = 0) {
  char buf[96];
  std::snprintf(buf, sizeof(buf), format, a, b);
  return std::string(buf);
}
}  // namespace detail

/// @brief Picks the reason (priority order of `Reason`).
inline Reason select_reason(const Inputs &in) {
  if (in.mode_off) return Reason::OFF;
  if (in.vacation) return Reason::VACATION;
  if (in.window_paused) return Reason::WINDOW;
  if (!in.auto_mode) return Reason::MANUAL;
  if (in.nox_active) return Reason::NOX;
  if (in.emergency_co2) return Reason::EMERGENCY_CO2;
  if (in.emergency_humidity) return Reason::EMERGENCY_HUMIDITY;
  if (in.override_active) return Reason::OVERRIDE;
  if (std::isnan(in.demand)) return Reason::NO_DATA;
  if (in.demand < DEMAND_EPSILON) return Reason::BASE;
  const bool co2_high = !std::isnan(in.room_co2) && !std::isnan(in.co2_setpoint) &&
                        in.room_co2 >= in.co2_setpoint;
  if (in.co2_controlling || co2_high) return Reason::CO2;
  const bool rh_high = !std::isnan(in.room_rh) && !std::isnan(in.rh_setpoint) && in.room_rh >= in.rh_setpoint;
  if (rh_high) return Reason::HUMIDITY;
  // Demand from a peer whose sensors this device does not see in detail.
  return std::isnan(in.room_co2) && !std::isnan(in.room_rh) ? Reason::HUMIDITY : Reason::CO2;
}

/**
 * @brief   German status text, e.g. "CO2 erhöht: 1150 ppm (Soll 1000) → Stufe 5".
 * @details Kept well below the 255-character limit of HA text states.
 */
inline Status describe(const Inputs &in) {
  Status s;
  s.reason = select_reason(in);
  const std::string level = " → Stufe " + std::to_string(in.level);
  std::string text;
  switch (s.reason) {
    case Reason::OFF:
      text = "Aus";
      break;
    case Reason::VACATION:
      text = "Urlaubsmodus" + level;
      break;
    case Reason::WINDOW:
      text = "Pausiert: Fenster offen";
      break;
    case Reason::MANUAL:
      text = std::string("Manuell: ") + in.mode_label + level;
      break;
    case Reason::NOX:
      if (!std::isnan(in.nox_index) && !in.nox_from_peer) {
        text = detail::fmt("NOx-Stoßlüftung: Index %.0f", in.nox_index);
      } else {
        text = "NOx-Stoßlüftung (anderes Gerät im Raum)";
      }
      if (in.nox_strong) text += ", stark";
      text += level;
      break;
    case Reason::EMERGENCY_CO2:
      text = std::isnan(in.room_co2) ? std::string("CO2-Notfall")
                                     : detail::fmt("CO2-Notfall: %.0f ppm", in.room_co2);
      text += level;
      break;
    case Reason::EMERGENCY_HUMIDITY:
      text = std::isnan(in.room_rh) ? std::string("Schimmelschutz")
                                    : detail::fmt("Schimmelschutz: %.0f %%", in.room_rh);
      text += level;
      break;
    case Reason::OVERRIDE:
      text = detail::fmt("Übersteuerung: noch %.0f min", in.override_remaining_min) + level;
      break;
    case Reason::NO_DATA:
      text = "Keine Sensordaten: Stufe " + std::to_string(in.level) + " gehalten";
      break;
    case Reason::CO2:
      text = std::isnan(in.room_co2) ? std::string("CO2 erhöht")
             : std::isnan(in.co2_setpoint)
                 ? detail::fmt("CO2 erhöht: %.0f ppm", in.room_co2)
                 : detail::fmt("CO2 erhöht: %.0f ppm (Soll %.0f)", in.room_co2, in.co2_setpoint);
      text += level;
      break;
    case Reason::HUMIDITY:
      text = std::isnan(in.room_rh) ? std::string("Feuchte erhöht")
             : std::isnan(in.rh_setpoint)
                 ? detail::fmt("Feuchte erhöht: %.0f %%", in.room_rh)
                 : detail::fmt("Feuchte erhöht: %.0f %% (Soll %.0f %%)", in.room_rh, in.rh_setpoint);
      text += level;
      break;
    case Reason::BASE:
      text = "Grundlüftung: kein Bedarf" + level;
      break;
  }
  const bool automatic = s.reason != Reason::OFF && s.reason != Reason::VACATION &&
                         s.reason != Reason::WINDOW && s.reason != Reason::MANUAL;
  if (automatic && in.ac_cap && !(s.reason == Reason::NOX && in.nox_strong) &&
      s.reason != Reason::EMERGENCY_CO2 && s.reason != Reason::EMERGENCY_HUMIDITY) {
    text += " (Klimaanlage: max. Stufe " + std::to_string(in.ac_max_level) + ")";
  }
  if (automatic && in.summer_cooling) text += " · Sommer-Kühlung";
  s.text = text;
  return s;
}

/// Time a new reason must persist before the HA event fires.
constexpr uint32_t EVENT_DEBOUNCE_MS = 30u * 1000u;

/**
 * @brief   Debounces reason changes for the HA event entity.
 * @details A short flip (e.g. CO2 hysteresis, a single cycle without data)
 *          does not fire an event; only a reason that persisted
 *          `EVENT_DEBOUNCE_MS` does. The first reason after boot is taken
 *          over silently.
 */
class ReasonTracker {
 public:
  /// @return true if `out` holds a new, stable reason to announce.
  bool update(Reason r, uint32_t now, Reason &out) {
    if (!initialized_) {
      initialized_ = true;
      published_ = pending_ = r;
      since_ms_ = now;
      return false;
    }
    if (r != pending_) {
      pending_ = r;
      since_ms_ = now;
    }
    if (pending_ != published_ && now - since_ms_ >= EVENT_DEBOUNCE_MS) {
      published_ = pending_;
      out = published_;
      return true;
    }
    return false;
  }
  Reason published() const { return published_; }

 private:
  bool initialized_ = false;
  Reason published_ = Reason::BASE;
  Reason pending_ = Reason::BASE;
  uint32_t since_ms_ = 0;
};

}  // namespace auto_status
}  // namespace ventosync
