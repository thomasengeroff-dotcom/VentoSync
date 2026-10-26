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
// File:        ventilation_packet.h
// Description: ESP-NOW wire format (VentilationPacket), peer state and the
//              pure packet validation. No ESPHome dependencies — unit-tested.
// Author:      Thomas Engeroff
// Created:     2026-10-18
// Modified:    2026-10-26
// ==========================================================================
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>

#include "ventilation_state_machine.h"
#include "../ventilation_logic/espnow_auth.h"
#include "../ventilation_logic/summer_bypass.h"

/** @brief Magic header for packet validation. */
static constexpr uint8_t PACKET_MAGIC = 0x42;

namespace ventosync {
namespace packet {
/// Size of VentilationPacket::device_name incl. the terminating NUL (v16).
/// The HA text "Gerätename" allows DEVICE_NAME_SIZE - 1 bytes (max_length).
constexpr size_t DEVICE_NAME_SIZE = 32;
}  // namespace packet
}  // namespace ventosync

namespace esphome {

/// @brief ESP-NOW packet type identifiers.
/// Value 1 (MSG_DISCOVERY) is retired: discovery uses the plaintext
/// ROOM_DISC / ROOM_CONF payloads (network_sync.h).
enum MessageType {
  MSG_SYNC = 2,           ///< Periodic time-sync packet (cycle position).
  MSG_STATE = 3,          ///< Mode or fan-intensity change notification.
  MSG_STATUS_REQUEST = 4, ///< Request current full status from peers.
  MSG_STATUS_RESPONSE = 5 ///< Reply with full status (mode, intensity).
};

/// Ensure breaking packet schema changes are detected across nodes.
/// Bump this whenever the VentilationPacket layout or semantics change.
static constexpr uint8_t PROTOCOL_VERSION = 16; // Bumped: device name

/// @name VentilationPacket::room_flags bits (protocol v10)
/// @{
static constexpr uint8_t ROOM_FLAG_HVAC_ENABLED = 0x01; ///< Room-wide "Klima-Koordination" switch (setting, synced like the sliders).
static constexpr uint8_t ROOM_FLAG_AC_ACTIVE = 0x02;    ///< Sender's OWN Home Assistant AC state (fresh) — never a fused value.
static constexpr uint8_t ROOM_FLAG_WINDOW_OPEN = 0x04;  ///< Sender's OWN Home Assistant window state (fresh) — never a fused value.
/// Sender's OWN radar presence (held 30 s) — never a fused value. Added in
/// 0.10.23 without a protocol bump: same layout, older v10 firmware simply
/// never sets it (its presence is then not shared).
static constexpr uint8_t ROOM_FLAG_PRESENCE = 0x08;
/// NOx boost (0.10.38, no protocol bump — same layout; older firmware never
/// sets these bits and ignores them):
/// Room-wide "NOx-Stoßlüftung" switch (setting, synced like the HVAC switch).
static constexpr uint8_t ROOM_FLAG_NOX_BOOST_ENABLED = 0x10;
/// Sender's OWN NOx boost (own SGP41 detector) — never a fused value.
static constexpr uint8_t ROOM_FLAG_NOX_ACTIVE = 0x20;
/// Sender's OWN boost is a strong event (index >= 150) — never a fused value.
static constexpr uint8_t ROOM_FLAG_NOX_STRONG = 0x40;
/// Sender's configured phase is B (0.10.56, no protocol bump — same layout;
/// older firmware never sets it, so its devices read as phase A). Only shown
/// by the dashboard's setup wizard; the cycle logic does not use it.
static constexpr uint8_t ROOM_FLAG_PHASE_B = 0x80;
/// @}
/// @brief Binary packet exchanged between peer devices via ESP-NOW.
/// Layout is packed and must be identical on all firmware builds.
/// IMPORTANT: protocol_version is the second byte — increment PROTOCOL_VERSION
/// and do a simultaneous OTA rollout on all nodes whenever this struct changes.
struct __attribute__((packed)) VentilationPacket {
  uint8_t magic_header; ///< Always PACKET_MAGIC — used for basic validation.
  uint8_t
      protocol_version; ///< FIXED K2: Schema version — reject mismatched peers.
  uint8_t floor_id;     ///< Floor group (filters unrelated devices).
  uint8_t room_id;      ///< Room group within the floor.
  uint8_t device_id;    ///< Unique sender ID (used to ignore own packets).
  uint8_t msg_type;     ///< MessageType enum value.
  uint8_t current_mode; ///< VentilationMode enum value.
  uint8_t current_mode_index; ///< exact UI mode index.

  // Live Data Synced States
  uint32_t timestamp_ms; ///< Sender's millis() at packet creation.
  uint32_t cycle_pos_ms; ///< Sender's position in the direction cycle.
  uint32_t
      remaining_duration_ms; ///< MODE_VENTILATION: remaining timer (0 = infinite);
                             ///< MODE_STOSSLUEFTUNG: remaining super-cycle time.
  bool phase_state;          ///< Sender's current global phase (A or B).
  float t_in;                ///< Sender's local indoor temperature (or NAN).
  float t_out;               ///< Sender's local outdoor temperature (or NAN).
  float pid_demand;          ///< Sender's LOCAL-sensor PID demand (0.0–1.0, NaN without sensors) — never a fused value.
  float fan_rpm;             ///< Sender's current fan RPM.
  float board_temp;          ///< Sender's board temperature (BMP390).
  float room_temp;           ///< Sender's room temperature (SCD41/BME680).
  float room_co2;            ///< Sender's effective CO2 in ppm (or NAN) — shared for Smart Climate Control.
  float room_humidity;       ///< Sender's room relative humidity in % (or NAN) — shared for the dashboard.

  // Control & Settings Synced States
  uint8_t fan_intensity; ///< Current 1-10 level

  // Automatik Config payload
  uint8_t automatik_min_fan_level;     ///< 1-10 minimum level
  uint8_t automatik_max_fan_level;     ///< 1-10 maximum level
  uint16_t auto_co2_threshold_val;     ///< Setpoint, e.g. 1000 ppm (16-bit)
  uint8_t auto_humidity_threshold_val; ///< Setpoint, e.g. 60 % (8-bit)
  int8_t auto_presence_val;            ///< Presence compensation (-5 to +5)

  // Smart Climate Control (HVAC Coordination) config payload — room-wide values
  uint16_t hvac_co2_threshold;  ///< Relaxed CO2 setpoint while the AC is active (ppm)
  uint16_t hvac_emergency_co2;  ///< CO2 emergency override threshold (ppm)
  uint8_t hvac_max_fan_level;   ///< Fan level cap while the AC is active (1-10)

  // Timer Settings payload
  uint16_t sync_interval_min; ///< ESP-NOW Broadcast Interval
  uint16_t vent_timer_min;    ///< Duration for Stoß/Durchlüften mode (minutes)

  // UI Settings payload
  float max_led_brightness; ///< Shared LED brightness limit (0.1–1.0)

  // Room flags (v10) — see ROOM_FLAG_*
  uint8_t room_flags;

  // Vacation snapshot (v11) — the pre-vacation room state, sent ONLY by the
  // device currently leading a vacation; every other device sends
  // VACATION_SNAPSHOT_NONE. Followers adopt it but never re-broadcast it.
  uint8_t vacation_pre_mode_index; ///< Mode index 0-4 before the vacation, or 0xFF.
  uint8_t vacation_pre_intensity;  ///< Fan level 1-10 before the vacation, or 0.

  // Smart-Automatik level override (v12) — room-wide state like the mode
  // (see level_override.h). Adopted from MSG_STATE and the Master heartbeat.
  uint8_t auto_override_level;        ///< Override level 1-10, 0 = none.
  uint16_t auto_override_remaining_s; ///< Remaining override time in seconds (max 18000).
  uint16_t auto_override_duration_min; ///< Room-wide override duration setting (30-300 min).

  // Vacation preset (v14) — room-wide setting like the sliders: the mode and
  // level the room leader applies when the vacation starts.
  uint8_t vacation_preset_mode_index; ///< Mode index 0-4 ("Urlaubsmodus Betriebsmodus").
  uint8_t vacation_preset_level;      ///< Fan level 1-10 ("Urlaubsmodus Intensität").

  // Summer cooling threshold (v15) — room-wide Smart-Automatik setting like the
  // sliders, so all devices of a room start the summer bypass at the same
  // indoor temperature.
  uint8_t summer_cooling_threshold; ///< Indoor threshold 18-30 °C ("Sommerkühlung Schwelle").

  // Device name (v16) — the sender's own "Gerätename" (UTF-8, NUL-padded,
  // empty = none), shown on the peers' dashboards. Display only.
  char device_name[ventosync::packet::DEVICE_NAME_SIZE];
};

static_assert(sizeof(VentilationPacket) + ventosync::espnow_auth::TRAILER_SIZE <= 250,
    "VentilationPacket + authentication trailer must fit into a single ESP-NOW payload (250 bytes)");

/// @brief Represents the latest known state of a peer device in the same room.
struct PeerState {
  uint32_t last_seen_ms;
  std::array<uint8_t, 6> mac;
  uint8_t device_id;
  uint8_t current_mode;
  uint8_t current_mode_index; ///< Sender's UI mode index (0 = Smart-Automatik … 4 = Aus, validated by parse()).
  uint8_t fan_intensity;
  bool phase_state;
  float t_in;
  float t_out;
  float pid_demand;
  float fan_rpm;
  float board_temp;
  float room_temp;
  float room_co2;
  float room_humidity;
  bool hvac_ac_active; ///< Peer's own HA AC state (ROOM_FLAG_AC_ACTIVE).
  bool window_open;    ///< Peer's own HA window state (ROOM_FLAG_WINDOW_OPEN).
  bool presence;       ///< Peer's own radar presence (ROOM_FLAG_PRESENCE).
  bool nox_active;     ///< Peer's own NOx boost (ROOM_FLAG_NOX_ACTIVE).
  bool nox_strong;     ///< Peer's own strong NOx event (ROOM_FLAG_NOX_STRONG).
  bool phase_b;        ///< Peer's configured phase is B (ROOM_FLAG_PHASE_B).
  uint8_t vacation_pre_mode_index; ///< Leader's pre-vacation mode index, else 0xFF.
  uint8_t vacation_pre_intensity;  ///< Leader's pre-vacation fan level, else 0.
  std::array<char, ventosync::packet::DEVICE_NAME_SIZE> name{}; ///< Peer's "Gerätename" (NUL-terminated, may be empty).
};

}  // namespace esphome

namespace ventosync {
namespace packet {

/// Number of UI operating mode indices on the wire (0 = Smart-Automatik …
/// 4 = Aus). globals.h asserts that MODE_NAMES matches.
constexpr uint8_t MODE_INDEX_COUNT = 5;

/// Why parse() rejected a packet (for the caller's log message).
enum class ParseError : uint8_t {
  NONE,
  TOO_SMALL,         ///< Fewer than 2 bytes (no header).
  BAD_MAGIC,         ///< Not a VentoSync packet.
  VERSION_MISMATCH,  ///< Other PROTOCOL_VERSION — devices run mismatched firmware.
  BAD_SIZE,          ///< Size differs from sizeof(VentilationPacket).
  OUT_OF_RANGE,      ///< A field is outside its valid range (see `field`).
};

/// Result details of parse().
struct ParseResult {
  ParseError error = ParseError::NONE;
  const char *field = "";  ///< Offending field for OUT_OF_RANGE.
  int value = 0;           ///< Offending value (field value, version or size).
};

/**
 * @brief   Validates a raw (authenticated, trailer-stripped) payload and
 *          deserialises it into a VentilationPacket.
 *
 * @details The single deserialisation point: header, protocol version and
 *          exact size are checked BEFORE the strict-aliasing-safe memcpy
 *          (never cast the buffer to a struct pointer), then the semantic
 *          ranges of the parsed fields.
 */
inline std::optional<esphome::VentilationPacket> parse(const uint8_t *data, size_t size, ParseResult &res) {
  res = ParseResult{};
  if (data == nullptr || size < 2) {
    res.error = ParseError::TOO_SMALL;
    res.value = static_cast<int>(size);
    return std::nullopt;
  }
  if (data[0] != PACKET_MAGIC) {
    res.error = ParseError::BAD_MAGIC;
    res.value = data[0];
    return std::nullopt;
  }
  if (data[1] != esphome::PROTOCOL_VERSION) {
    res.error = ParseError::VERSION_MISMATCH;
    res.value = data[1];
    return std::nullopt;
  }
  if (size != sizeof(esphome::VentilationPacket)) {
    res.error = ParseError::BAD_SIZE;
    res.value = static_cast<int>(size);
    return std::nullopt;
  }

  esphome::VentilationPacket pkt;
  std::memcpy(&pkt, data, sizeof(pkt));

  auto out_of_range = [&res](const char *field, int value) {
    res.error = ParseError::OUT_OF_RANGE;
    res.field = field;
    res.value = value;
    return std::nullopt;
  };
  if (pkt.fan_intensity > 10) return out_of_range("fan_intensity", pkt.fan_intensity);
  if (pkt.automatik_min_fan_level < 1 || pkt.automatik_min_fan_level > 10)
    return out_of_range("automatik_min_fan_level", pkt.automatik_min_fan_level);
  if (pkt.automatik_max_fan_level < 1 || pkt.automatik_max_fan_level > 10)
    return out_of_range("automatik_max_fan_level", pkt.automatik_max_fan_level);
  if (pkt.hvac_max_fan_level < 1 || pkt.hvac_max_fan_level > 10)
    return out_of_range("hvac_max_fan_level", pkt.hvac_max_fan_level);
  if (pkt.sync_interval_min < 1 || pkt.sync_interval_min > 1440)
    return out_of_range("sync_interval_min", pkt.sync_interval_min);
  if (pkt.current_mode_index >= MODE_INDEX_COUNT)
    return out_of_range("current_mode_index", pkt.current_mode_index);
  if (pkt.current_mode > esphome::MODE_STOSSLUEFTUNG) return out_of_range("current_mode", pkt.current_mode);
  if (pkt.vent_timer_min > 1440) return out_of_range("vent_timer_min", pkt.vent_timer_min);
  if (pkt.vacation_preset_mode_index >= MODE_INDEX_COUNT)
    return out_of_range("vacation_preset_mode_index", pkt.vacation_preset_mode_index);
  if (pkt.vacation_preset_level < 1 || pkt.vacation_preset_level > 10)
    return out_of_range("vacation_preset_level", pkt.vacation_preset_level);
  if (pkt.summer_cooling_threshold < ventosync::summer::THRESHOLD_MIN_C ||
      pkt.summer_cooling_threshold > ventosync::summer::THRESHOLD_MAX_C)
    return out_of_range("summer_cooling_threshold", pkt.summer_cooling_threshold);
  // Display-only text: terminate instead of rejecting the packet
  pkt.device_name[DEVICE_NAME_SIZE - 1] = '\0';
  return pkt;
}

/**
 * @brief   Copies a device name into a fixed field (VentilationPacket::device_name).
 *
 * @details At most `size - 1` bytes, cut at a UTF-8 character boundary (never
 *          half an umlaut) and before the first control character; the rest
 *          of the field is NUL-padded.
 */
inline void copy_device_name(char *dst, size_t size, const char *src, size_t len) {
  if (dst == nullptr || size == 0) return;
  std::memset(dst, 0, size);
  if (src == nullptr) return;
  size_t n = 0;
  while (n < len && n < size - 1 && static_cast<unsigned char>(src[n]) >= 0x20 && src[n] != 0x7F) n++;
  // Dropped a continuation byte (10xxxxxx)? Then also drop its lead byte(s).
  if (n < len && n == size - 1) {
    while (n > 0 && (static_cast<unsigned char>(src[n]) & 0xC0) == 0x80) n--;
  }
  std::memcpy(dst, src, n);
}

/// @brief Short English label of a ParseError (log output).
inline const char *parse_error_label(ParseError e) {
  switch (e) {
    case ParseError::NONE: return "ok";
    case ParseError::TOO_SMALL: return "too small";
    case ParseError::BAD_MAGIC: return "invalid magic header";
    case ParseError::VERSION_MISMATCH: return "protocol version mismatch";
    case ParseError::BAD_SIZE: return "invalid size";
    case ParseError::OUT_OF_RANGE: return "field out of range";
  }
  return "?";
}

/**
 * @brief   Stores a received packet as the sender's PeerState (dashboard,
 *          room-wide fusion). Decodes the room flags.
 *
 * @param[in] now_ms  millis() of the reception (PeerState::last_seen_ms).
 */
inline void update_peer_state(esphome::PeerState &peer, const esphome::VentilationPacket &pkt, const uint8_t *src_mac,
                              uint32_t now_ms) {
  std::memcpy(peer.mac.data(), src_mac, peer.mac.size());
  peer.last_seen_ms = now_ms;
  peer.device_id = pkt.device_id;
  peer.current_mode = pkt.current_mode;
  peer.current_mode_index = pkt.current_mode_index;
  peer.fan_intensity = pkt.fan_intensity;
  peer.phase_state = pkt.phase_state;
  peer.t_in = pkt.t_in;
  peer.t_out = pkt.t_out;
  peer.pid_demand = pkt.pid_demand;
  peer.fan_rpm = pkt.fan_rpm;
  peer.board_temp = pkt.board_temp;
  peer.room_temp = pkt.room_temp;
  peer.room_co2 = pkt.room_co2;
  peer.room_humidity = pkt.room_humidity;
  peer.hvac_ac_active = (pkt.room_flags & esphome::ROOM_FLAG_AC_ACTIVE) != 0;
  peer.window_open = (pkt.room_flags & esphome::ROOM_FLAG_WINDOW_OPEN) != 0;
  peer.presence = (pkt.room_flags & esphome::ROOM_FLAG_PRESENCE) != 0;
  peer.nox_active = (pkt.room_flags & esphome::ROOM_FLAG_NOX_ACTIVE) != 0;
  peer.nox_strong = (pkt.room_flags & esphome::ROOM_FLAG_NOX_STRONG) != 0;
  peer.phase_b = (pkt.room_flags & esphome::ROOM_FLAG_PHASE_B) != 0;
  peer.vacation_pre_mode_index = pkt.vacation_pre_mode_index;
  peer.vacation_pre_intensity = pkt.vacation_pre_intensity;
  std::memcpy(peer.name.data(), pkt.device_name, peer.name.size());
  peer.name.back() = '\0';
}

/// Smart-Automatik on the wire (UI mode index 0, globals.h MODE_INDEX_AUTO).
constexpr uint8_t MODE_INDEX_AUTO = 0;

/**
 * @brief   Fan level a receiver takes over from a peer's state packet.
 *
 * @details The manual modes run the whole room on the sender's level. In
 *          Smart-Automatik the local automatic decides (a slave follows the
 *          Master in auto_level.h), so a receiver that already runs it keeps
 *          its level.
 *
 *          Decided by the packet's mode as well: a packet that switches the
 *          room from Smart-Automatik to a manual mode (e.g. the vacation
 *          preset) arrives while the receiver still runs the automatic — its
 *          level has to be taken over all the same.
 *
 *          Compared with the level the fan runs on (`local_level`), not with
 *          VentilationController::current_fan_intensity: on_packet_received()
 *          has already updated that cache, so a level that arrived first in a
 *          heartbeat (MSG_SYNC) would otherwise never reach the fan.
 *
 * @param[in] local_level  Level the receiver's fan runs on (fan_intensity_level).
 * @param[in] local_auto   Receiver runs Smart-Automatik before this packet.
 * @return  Level to apply (1–10), or 0 to keep the local level.
 */
inline uint8_t level_to_adopt(const esphome::VentilationPacket &pkt, int local_level, bool local_auto) {
  if (pkt.fan_intensity < 1 || pkt.fan_intensity > 10) return 0;
  if (local_auto && pkt.current_mode_index == MODE_INDEX_AUTO) return 0;
  if (pkt.fan_intensity == local_level) return 0;
  return pkt.fan_intensity;
}

}  // namespace packet
}  // namespace ventosync
