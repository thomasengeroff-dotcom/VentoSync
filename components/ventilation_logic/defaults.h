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
// File:        defaults.h
// Description: Factory defaults of the room-wide Smart-Automatik and vacation settings
//              (C++ fallbacks). Mirror of the default_* substitutions in
//              packages/base/ventosync_core.yaml — CI-checked.
// Author:      Thomas Engeroff
// Created:     2026-10-18
// Modified:    2026-10-22
// ==========================================================================
#pragma once

/// @file defaults.h
/// @brief First-boot values of the Smart-Automatik settings.
///
/// The YAML globals and HA numbers take their initial values from the
/// `default_*` substitutions in packages/base/ventosync_core.yaml; the C++
/// code uses these constants wherever it needs a fallback (e.g. a packet
/// built before a global is bound). `.github/scripts/ci_config.py check`
/// (lint workflow) fails if both sides differ. The Smart Climate Control
/// defaults live in hvac_coordinator.h (DEFAULT_*), checked the same way.
namespace ventosync {
namespace defaults {

constexpr int AUTO_MIN_FAN_LEVEL = 2;           ///< default_auto_min_fan_level (moisture protection, DIN 1946-6)
constexpr int AUTO_MAX_FAN_LEVEL = 7;           ///< default_auto_max_fan_level (noise limit)
constexpr int AUTO_CO2_THRESHOLD_PPM = 1000;    ///< default_auto_co2_threshold (Pettenkofer)
constexpr int AUTO_HUMIDITY_THRESHOLD_PCT = 60; ///< default_auto_humidity_threshold (mold protection)
constexpr int LED_MAX_BRIGHTNESS_PCT = 80;      ///< default_led_max_brightness (status LEDs)
constexpr int VACATION_MODE_INDEX = 3;          ///< default_vacation_mode_index (Stoßlüftung)
constexpr int VACATION_LEVEL = 1;               ///< default_vacation_level (lowest level)

static_assert(AUTO_MIN_FAN_LEVEL >= 1 && AUTO_MIN_FAN_LEVEL <= 3, "HA slider range 1-3");
static_assert(AUTO_MAX_FAN_LEVEL >= 5 && AUTO_MAX_FAN_LEVEL <= 10, "HA slider range 5-10");
static_assert(AUTO_CO2_THRESHOLD_PPM >= 400 && AUTO_CO2_THRESHOLD_PPM <= 2000, "HA slider range 400-2000");
static_assert(AUTO_HUMIDITY_THRESHOLD_PCT >= 40 && AUTO_HUMIDITY_THRESHOLD_PCT <= 100, "HA slider range 40-100");
static_assert(LED_MAX_BRIGHTNESS_PCT >= 5 && LED_MAX_BRIGHTNESS_PCT <= 100, "HA slider range 5-100");
static_assert(VACATION_MODE_INDEX >= 0 && VACATION_MODE_INDEX <= 4, "Mode index 0-4 (MODE_NAMES)");
static_assert(VACATION_LEVEL >= 1 && VACATION_LEVEL <= 10, "HA slider range 1-10");

}  // namespace defaults
}  // namespace ventosync
