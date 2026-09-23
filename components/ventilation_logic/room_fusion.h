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
// File:        room_fusion.h
// Description: Pure room-wide sensor/demand fusion helpers (ESP-NOW peers).
// Author:      Thomas Engeroff
// Created:     2026-09-23
// Modified:    2026-09-23
// ==========================================================================
#pragma once

#include <cmath>
#include <cstdint>

/// @file room_fusion.h
/// @brief Hardware-agnostic helpers that fuse local and peer values room-wide.
///
/// Rules that keep the fusion free of feedback loops:
///  - Every device broadcasts only values derived from its OWN sensors
///    (`room_co2`, `room_humidity`, `pid_demand`). Fused values are never
///    re-broadcast, so a high value cannot latch between two devices.
///  - Peer values are only trusted for `PEER_DATA_MAX_AGE_MS`.
///
/// The helpers are templates over the peer container so the native unit-test
/// runner can use a minimal struct instead of the ESPHome `PeerState`.

namespace ventosync {
namespace room {

/// A value shared by a peer is trusted for at most this long. Matches the
/// 5-minute staleness hold of the local `effective_co2` sensor; with the
/// default 60 s heartbeat this tolerates several lost packets.
constexpr uint32_t PEER_DATA_MAX_AGE_MS = 300000u;

/// @brief True if a peer seen at `last_seen_ms` is still fresh (wrap-safe).
inline bool is_fresh(uint32_t now_ms, uint32_t last_seen_ms,
                     uint32_t max_age_ms = PEER_DATA_MAX_AGE_MS) {
  return static_cast<uint32_t>(now_ms - last_seen_ms) <= max_age_ms;
}

/// @brief NaN-aware maximum: NaN operands are ignored, NaN only if both are NaN.
inline float max_valid(float a, float b) {
  if (std::isnan(a)) return b;
  if (std::isnan(b)) return a;
  return (a > b) ? a : b;
}

/**
 * @brief   Highest valid value of one field across all fresh peers.
 *
 * @param[in] peers       Container of peer records exposing `last_seen_ms`.
 * @param[in] now_ms      Current millis().
 * @param[in] get         Accessor returning the field (float) of one peer.
 * @param[in] min_valid   Values below this are treated as "no data"
 *                        (e.g. mock sensors reporting 0 ppm).
 * @param[in] max_age_ms  Freshness window.
 *
 * @return  Maximum value, or NaN if no fresh peer carries a valid value.
 */
template <typename Peers, typename Getter>
inline float max_fresh_peer_value(const Peers &peers, uint32_t now_ms, Getter get,
                                  float min_valid = 0.0f,
                                  uint32_t max_age_ms = PEER_DATA_MAX_AGE_MS) {
  float best = NAN;
  for (const auto &peer : peers) {
    if (!is_fresh(now_ms, peer.last_seen_ms, max_age_ms)) continue;
    const float val = get(peer);
    if (std::isnan(val) || val < min_valid) continue;
    best = max_valid(best, val);
  }
  return best;
}

/**
 * @brief   Room-wide worst-case CO2: local sensor vs. all fresh peers.
 *
 * @param[in] local_ppm  Local effective CO2 (NaN = no sensor / stale).
 * @param[in] peers      Peer records exposing `last_seen_ms` and `room_co2`.
 * @param[in] now_ms     Current millis().
 *
 * @return  Highest CO2 in ppm, or NaN if no source is available.
 */
template <typename Peers>
inline float room_max_co2(float local_ppm, const Peers &peers, uint32_t now_ms) {
  const float local = (!std::isnan(local_ppm) && local_ppm > 0.0f) ? local_ppm : NAN;
  // 0 ppm is physically impossible — strictly positive values only.
  const float peer = max_fresh_peer_value(
      peers, now_ms, [](const auto &p) { return p.room_co2; }, 1.0f);
  return max_valid(local, peer);
}

/**
 * @brief   Highest ventilation demand broadcast by any fresh peer.
 *
 * @details Peers broadcast only their LOCAL sensor demand (NaN without
 *          sensors), so adopting the maximum cannot create a feedback loop.
 *          Values are clamped to the valid 0.0–1.0 range.
 *
 * @return  Peer demand in 0.0–1.0, or NaN if no fresh peer reports one.
 */
template <typename Peers>
inline float room_max_peer_demand(const Peers &peers, uint32_t now_ms) {
  const float d = max_fresh_peer_value(
      peers, now_ms, [](const auto &p) { return p.pid_demand; }, 0.0f);
  if (std::isnan(d)) return NAN;
  return (d > 1.0f) ? 1.0f : d;
}

/// @brief Humidity reading used by the mold guard, with its source temperature.
struct HumiditySource {
  float rh_percent = NAN; ///< Relative humidity (NaN = unavailable).
  float temp_c = NAN;     ///< Temperature at the same spot (NaN = unknown).
  bool from_peer = false; ///< True if the value came from a peer.
};

/**
 * @brief   Room-wide worst-case humidity for the mold guard.
 *
 * @details Picks the highest relative humidity among the local sensor and all
 *          fresh peers (mold risk is driven by rH at the wettest spot). The
 *          temperature of the same source is returned so the caller can
 *          compute absolute humidity for the "can ventilation dry?" check.
 *
 * @param[in] local_rh    Local indoor rH (NaN = no sensor).
 * @param[in] local_temp  Local indoor temperature matching `local_rh`.
 * @param[in] peers       Peer records exposing `last_seen_ms`,
 *                        `room_humidity` and `room_temp`.
 * @param[in] now_ms      Current millis().
 */
template <typename Peers>
inline HumiditySource room_max_humidity(float local_rh, float local_temp,
                                        const Peers &peers, uint32_t now_ms) {
  HumiditySource out;
  if (!std::isnan(local_rh) && local_rh > 0.0f && local_rh <= 100.0f) {
    out.rh_percent = local_rh;
    out.temp_c = local_temp;
  }
  for (const auto &peer : peers) {
    if (!is_fresh(now_ms, peer.last_seen_ms)) continue;
    const float rh = peer.room_humidity;
    if (std::isnan(rh) || rh <= 0.0f || rh > 100.0f) continue;
    if (std::isnan(out.rh_percent) || rh > out.rh_percent) {
      out.rh_percent = rh;
      out.temp_c = peer.room_temp;
      out.from_peer = true;
    }
  }
  return out;
}

} // namespace room
} // namespace ventosync
