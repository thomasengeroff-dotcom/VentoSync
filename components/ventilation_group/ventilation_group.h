// ==========================================================================
// WRG Wohnraumlüftung – ESPHome Custom Component
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
// File:        ventilation_group.h
// Description: Definitions for the ventilation group component.
// Author:      Thomas Engeroff
// Created:     2026-01-28
// Modified:    2026-03-21
// ==========================================================================
#pragma once

/// @file ventilation_group.h
/// @brief ESPHome component that wraps VentilationStateMachine and adds
/// hardware I/O (fan, direction switch) and ESP-NOW group synchronization.

#include "esphome.h"
#include "ventilation_state_machine.h"
#include <cmath>
#include <vector>
#include <esp_task_wdt.h>

// Forward declaration of the global fan update function (defined in
// automation_helpers.h)
void update_fan_logic();

namespace esphome {

// ---------------------------------------------------------
// ENUMS & STRUCTS
// ---------------------------------------------------------

// Enum moved to ventilation_state_machine.h

/// @brief ESP-NOW packet type identifiers.
enum MessageType {
  MSG_DISCOVERY = 1,      ///< Announce presence after boot.
  MSG_SYNC = 2,           ///< Periodic time-sync packet (cycle position).
  MSG_STATE = 3,          ///< Mode or fan-intensity change notification.
  MSG_STATUS_REQUEST = 4, ///< Request current full status from peers.
  MSG_STATUS_RESPONSE = 5 ///< Reply with full status (mode, intensity).
};

/// Ensure breaking packet schema changes are detected across nodes.
/// Bump this whenever the VentilationPacket layout or semantics change.
static const uint8_t PROTOCOL_VERSION = 7; // Bumped: added RPM, Board-T, Room-T
/// @brief Binary packet exchanged between peer devices via ESP-NOW.
/// Layout is packed and must be identical on all firmware builds.
/// IMPORTANT: protocol_version is the second byte — increment PROTOCOL_VERSION
/// and do a simultaneous OTA rollout on all nodes whenever this struct changes.
struct __attribute__((packed)) VentilationPacket {
  uint8_t magic_header; ///< Always 0x42 — used for basic validation.
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
      remaining_duration_ms; ///< Remaining ventilation timer (0 = infinite).
  bool phase_state;          ///< Sender's current global phase (A or B).
  float t_in;                ///< Sender's local indoor temperature (or NAN).
  float t_out;               ///< Sender's local outdoor temperature (or NAN).
  float pid_demand;          ///< Sender's local evaluated PID demand (0.0–1.0).
  float fan_rpm;             ///< Sender's current fan RPM.
  float board_temp;          ///< Sender's board temperature (BMP390).
  float room_temp;           ///< Sender's room temperature (SCD41/BME680).

  // Control & Settings Synced States
  uint8_t fan_intensity; ///< Current 1-10 level

  // Automatik Config payload
  uint8_t automatik_min_fan_level;     ///< 1-10 minimum level
  uint8_t automatik_max_fan_level;     ///< 1-10 maximum level
  uint16_t auto_co2_threshold_val;     ///< Setpoint, e.g. 1000 ppm (16-bit)
  uint8_t auto_humidity_threshold_val; ///< Setpoint, e.g. 60 % (8-bit)
  int8_t auto_presence_val;            ///< Presence compensation (-5 to +5)

  // Timer Settings payload
  uint16_t sync_interval_min; ///< ESP-NOW Broadcast Interval
  uint16_t vent_timer_min;    ///< Duration for Stoß/Durchlüften mode (minutes)

  // UI Settings payload
  float max_led_brightness; ///< Shared LED brightness limit (0.1–1.0)
};

/// @brief Represents the latest known state of a peer device in the same room.
struct PeerState {
  uint32_t last_seen_ms;
  uint8_t device_id;
  uint8_t current_mode;
  uint8_t fan_intensity;
  bool phase_state;
  float t_in;
  float t_out;
  float pid_demand;
  float fan_rpm;
  float board_temp;
  float room_temp;
};

// ---------------------------------------------------------
// CONTROLLER CLASS
// ---------------------------------------------------------

/// @class VentilationController
/// @brief ESPHome Component that drives one ventilation unit.
/// Owns a VentilationStateMachine for pure logic, adds hardware I/O
/// (fan, direction switch) and ESP-NOW packet handling for group sync.
class VentilationController : public Component {
public:
  // --- CONFIGURATION ---
  uint8_t floor_id = 1;   ///< Floor group for ESP-NOW filtering.
  uint8_t room_id = 1;    ///< Room group for ESP-NOW filtering.
  uint8_t device_id = 1;  ///< Unique device ID within the room.
  bool is_phase_a = true; ///< Phase group: A starts IN, B starts OUT.

  // --- STATE ---
  VentilationStateMachine
      state_machine; ///< Pure-logic state machine (no HW deps).

  uint32_t sync_interval_ms =
      60000; ///< Auto-sync broadcast interval (default 1 min for dashboard).
  uint8_t current_fan_intensity = 5; ///< Cached fan intensity (1–10).
  uint32_t last_loop_ms{0};           ///< Activity timestamp for health monitoring.
  bool was_ramping = false;           ///< Tracking for 'ramping complete' log.

  // --- TEMPERATURE SENSOR SHARING ---
  float local_t_in =
      NAN; ///< Local valid indoor temperature (from SCD41 or NTC logic).
  float local_t_out =
      NAN; ///< Local valid outdoor temperature (from NTC logic).

  float last_peer_t_in =
      NAN; ///< Last valid indoor temperature received from a peer.
  uint32_t last_peer_t_in_time = 0; ///< millis() when peer T_in was received.

  float last_peer_t_out =
      NAN; ///< Last valid outdoor temperature received from a peer.
  uint32_t last_peer_t_out_time = 0; ///< millis() when peer T_out was received.

  // --- PID CONTROL SHARING ---
  // Synchronizes the calculated continuous cooling/ventilation demand across
  // the room. Crucial Benefit: If one device measures high CO2 (e.g., above a
  // bed) while another measures low CO2 (e.g., near an open door), both devices
  // share their calculated demand. The system then dynamically scales all fans
  // to the identically highest required speed necessary to clear the room,
  // without fighting each other or creating noise artifacts.
  float local_pid_demand = 0.0f; ///< Local PID demand requirement (0.0 to 1.0)
  float last_peer_pid_demand =
      0.0f; ///< Last valid PID demand received from a peer
  uint32_t last_peer_pid_demand_time =
      0; ///< millis() when peer PID demand was received
  /// FIXED W3: Explicit flag avoids millis()-overflow false-positive when
  /// last_peer_pid_demand_time == 0 is used as a sentinel after 49.7 days.
  bool has_peer_pid_demand =
      false; ///< True once any peer PID demand has been received.
  bool co2_is_controlling =
      false; ///< Hysteresis state for CO2 priority (runtime only).

  // --- PEER TRACKING (For Dashboard) ---
  std::vector<PeerState> peers; ///< List of recently seen peers
  bool is_state_synced =
      false; ///< tracks if state has been synced from peer after boot

  // --- INTERNAL ---
  uint32_t last_sync_tx = 0;     ///< millis() of last sync broadcast.
  uint32_t last_hw_log_ms = 0;   ///< millis() of last hardware status log.
  uint32_t last_ramp_log_ms = 0; ///< millis() of last ramp log.
  bool pending_broadcast =
      false; ///< True = YAML should send a packet next loop.

  // --- HARDWARE REFS (set by codegen) ---
  fan::Fan *main_fan{nullptr};                ///< ESPHome fan component.
  switch_::Switch *direction_switch{nullptr}; ///< ON = intake, OFF = exhaust.
  sensor::Sensor *fan_rpm_sensor_{nullptr};   ///< Local RPM sensor.
  sensor::Sensor *board_temp_sensor_{nullptr}; ///< Local board temp (BMP390).
  sensor::Sensor *scd41_temp_sensor_{nullptr}; ///< Local room temp (SCD41).
  sensor::Sensor *bme680_temp_sensor_{nullptr}; ///< Fallback room temp (BME680).

  // --- SETTERS (called by ESPHome codegen from YAML config) ---
  void set_floor_id(uint8_t id) { floor_id = id; }   ///< Set floor group.
  void set_room_id(uint8_t id) { room_id = id; }     ///< Set room group.
  void set_device_id(uint8_t id) {
    if (device_id == id) return;
    device_id = id;
    ESP_LOGI("vent", "Controller Device ID updated to: %d", id);
  } ///< Set unique device ID.
  void set_is_phase_a(bool phase_a) {
    state_machine.is_phase_a = phase_a;
    update_hardware();
  } ///< Set phase group.
  void set_main_fan(fan::Fan *fan) {
    main_fan = fan;
  } ///< Bind the fan component.
  void set_direction_switch(switch_::Switch *sw) {
    direction_switch = sw;
  } ///< Bind the direction switch.
  void set_fan_rpm_sensor(sensor::Sensor *s) { fan_rpm_sensor_ = s; }
  void set_board_temp_sensor(sensor::Sensor *s) { board_temp_sensor_ = s; }
  void set_scd41_temp_sensor(sensor::Sensor *s) { scd41_temp_sensor_ = s; }
  void set_bme680_temp_sensor(sensor::Sensor *s) { bme680_temp_sensor_ = s; }

  VentilationController() {}

  /// @brief Component setup — logs config and schedules initial discovery
  /// broadcast.
  void setup() override {
    ESP_LOGCONFIG(
        "vent",
        "Ventilation Group Setup: Floor %d, Room %d, Device %d, Phase %s",
        floor_id, room_id, device_id, is_phase_a ? "A" : "B");
        
    // Register the current task (ESPHome main loop) with the ESP-IDF Task Watchdog (TWDT)
    esp_task_wdt_add(NULL); 
    
    pending_broadcast = true; // Announce presence on boot
  }

  /// @brief Main loop — ticks the state machine, refreshes hardware, and
  /// schedules periodic sync broadcasts.
  void loop() override {
    uint32_t now = millis();

    // 1. Update State Machine (returns true on discrete state flip)
    bool dirty = state_machine.update(now);

    // 2. Hardware Update (always update during ramping phases)
    HardwareState state = state_machine.get_target_state(now);

    // We update hardware if:
    // a) The state machine reported a discrete change (e.g. direction flip)
    // b) We are currently in a ramping phase (ramp_factor != 1.0)
    // c) System is off (to ensure it stays off/ramps down)
    if (dirty || (1.0f - state.ramp_factor) > 0.01f || !state.fan_enabled) {
      if (dirty) {
        // Log details about what caused the dirty flag if possible (handled in state machine)
        ESP_LOGD("vent", "State transition detected (flip/mode timer). "
                         "Triggering sync broadcast.");
        trigger_sync();
      }
      update_hardware(state, dirty);
    }

    // 3. Auto Sync Broadcast (Dashboard Heartbeat every 60s)
    // Ensures all peers regularly announce their presence even if they don't
    // flip directions (like in 'Aus' or 'Durchlüften' modes). This prevents the
    // dashboard from dropping peers after the 5-minute timeout.
    if (now - last_sync_tx > 60000) {
      ESP_LOGI("vent", "Triggering periodic sync broadcast (60s heartbeat)");
      pending_broadcast = true; // Let YAML trigger the send
      last_sync_tx = now;
    }

    // 4. Cleanup old peers (5 minutes timeout)
    auto it = peers.begin();
    while (it != peers.end()) {
      if (now - it->last_seen_ms > 300000) {
        ESP_LOGD("vent", "Removing stale peer %d due to timeout",
                 it->device_id);
        it = peers.erase(it);
      } else {
        ++it;
      }
    }

    // 5. Watchdog Feed
    // This ensures that the ESP reboots if the main loop hangs for longer 
    // than the configured CONFIG_ESP_TASK_WDT_TIMEOUT_S (15s).
    esp_task_wdt_reset();
    
    last_loop_ms = now;
  }

  // --- ACTIONS ---

  /// @brief Updates the half-cycle duration and refreshes hardware / notifies
  /// peers.
  void set_cycle_duration(uint32_t ms, bool refresh_hw = true, bool notify = true) {
    if (ms == state_machine.cycle_duration_ms)
      return;
    state_machine.set_cycle_duration(ms);
    ESP_LOGI("vent", "Cycle duration updated to %d ms", ms);
    if (refresh_hw) {
      update_hardware();
    }
    if (notify) pending_broadcast = true;
  }

  /// @brief Changes the auto-sync broadcast interval.
  void set_sync_interval(uint32_t ms) {
    sync_interval_ms = ms;
    ESP_LOGI("vent", "Sync interval updated to %d ms", sync_interval_ms);
    trigger_sync();
  }

  /// @brief Manually triggers a broadcast on the next 1s interval.
  void trigger_sync() { pending_broadcast = true; }

  /// @brief Sets the fan intensity level (1–10) and notifies peers.
  void set_fan_intensity(uint8_t intensity, bool notify = true) {
    if (current_fan_intensity == intensity)
      return;
    current_fan_intensity = intensity;
    ESP_LOGI("vent", "Fan Intensity updated to %d. Refreshing hardware.",
             intensity);
    update_hardware();
    if (notify) pending_broadcast = true;
  }

  /// @brief Switches operating mode, delegates to state machine, refreshes HW,
  /// notifies peers.
  /// @param mode     Target VentilationMode.
  /// @param duration For MODE_VENTILATION: auto-stop timer in ms (0 =
  /// infinite).
  void set_mode(VentilationMode mode, uint32_t duration = 0, bool notify = true) {
    if (state_machine.current_mode == mode &&
        duration == state_machine.ventilation_duration_ms)
      return;

    ESP_LOGI("vent", "Mode change: %d -> %d (Duration: %d ms, Notify: %s)",
             state_machine.current_mode, mode, duration, notify ? "YES" : "NO");
    state_machine.set_mode(mode, millis(), duration);

    update_hardware();
    if (notify) pending_broadcast = true;
  }

  /// @brief Processes an incoming ESP-NOW packet.
  /// Validates header, filters by floor/room, then syncs mode, timer,
  /// intensity.
  /// @param data  Raw byte vector received via ESP-NOW.
  /// @return true if any local state was changed (caller should update UI).
  bool on_packet_received(std::vector<uint8_t> data) {
    ESP_LOGI("vent", "on_packet_received() called");
    if (data.size() != sizeof(VentilationPacket)) {
      ESP_LOGI("vent_sync", "Size mismatch! Expected %d, got %d",
               sizeof(VentilationPacket), data.size());
      return false;
    }
    VentilationPacket *pkt = (VentilationPacket *)data.data();

    // Filter: magic header
    if (pkt->magic_header != 0x42) {
      ESP_LOGI("vent_sync", "Magic header mismatch! Expected 0x42, got 0x%02X",
               pkt->magic_header);
      return false;
    }
    // FIXED K2: Reject packets from nodes running a different protocol version.
    // Always do a simultaneous OTA rollout when PROTOCOL_VERSION changes.
    if (pkt->protocol_version != PROTOCOL_VERSION) {
      ESP_LOGI("vent_sync",
               "Protocol version mismatch! Got v%d, expected v%d — "
               "update firmware on all nodes simultaneously.",
               pkt->protocol_version, PROTOCOL_VERSION);
      return false;
    }
    if (pkt->floor_id != floor_id || pkt->room_id != room_id) {
      ESP_LOGI("vent_sync",
               "Group mismatch! Received: Floor %d, Room %d | Local: Floor %d, "
               "Room %d. Peer ignored.",
               pkt->floor_id, pkt->room_id, floor_id, room_id);
      return false;
    }
    if (pkt->device_id == device_id) {
      ESP_LOGI("vent_sync",
               "Ignored own packet (device %d). Check for ID collision if this "
               "is not a loopback!",
               device_id);
      return false;
    }

    ESP_LOGI("vent_sync", "Valid packet received from device %d! (Type: %d)",
             pkt->device_id, pkt->msg_type);

    bool found_peer = false;
    for (auto &peer : peers) {
      if (peer.device_id == pkt->device_id) {
        peer.last_seen_ms = millis();
        peer.current_mode = pkt->current_mode;
        peer.fan_intensity = pkt->fan_intensity;
        peer.phase_state = pkt->phase_state;
        peer.t_in = pkt->t_in;
        peer.t_out = pkt->t_out;
        peer.pid_demand = pkt->pid_demand;
        peer.fan_rpm = pkt->fan_rpm;
        peer.board_temp = pkt->board_temp;
        peer.room_temp = pkt->room_temp;
        found_peer = true;
        break;
      }
    }
    if (!found_peer) {
      PeerState new_peer;
      new_peer.device_id = pkt->device_id;
      new_peer.last_seen_ms = millis();
      new_peer.current_mode = pkt->current_mode;
      new_peer.fan_intensity = pkt->fan_intensity;
      new_peer.phase_state = pkt->phase_state;
      new_peer.t_in = pkt->t_in;
      new_peer.t_out = pkt->t_out;
      new_peer.pid_demand = pkt->pid_demand;
      new_peer.fan_rpm = pkt->fan_rpm;
      new_peer.board_temp = pkt->board_temp;
      new_peer.room_temp = pkt->room_temp;
      peers.push_back(new_peer);
    }

    bool changed = false;

    // Determine if we should adopt this packet's mode/intensity/timing
    bool should_sync = false;
    if (pkt->msg_type == MSG_STATE) {
      should_sync = true; // Always adopt explicit group interactions from any device
    } else if (pkt->msg_type == MSG_SYNC && device_id != pkt->device_id) {
      // Only the Master (device_id == 1) drives cycle timing and config sync
      if (pkt->device_id == 1) {
        should_sync = true;
      } else {
        ESP_LOGD("vent_sync",
                 "Ignored sync heartbeat from non-master device %d",
                 pkt->device_id);
      }
    }

    if (should_sync) {
      // 1. Time sync (aligns direction cycle phase) — Master only for MSG_SYNC
      if (pkt->msg_type == MSG_SYNC && pkt->device_id == 1) {
        state_machine.sync_time(millis(), pkt->cycle_pos_ms);
      }
      int32_t time_diff =
          (int32_t)pkt->remaining_duration_ms -
          (int32_t)state_machine.get_remaining_duration(millis());
      if (pkt->current_mode != state_machine.current_mode ||
          (pkt->current_mode == MODE_VENTILATION &&
           std::abs(time_diff) > 2000)) {

        ESP_LOGI("vent", "Syncing mode from peer %d (Type: %d): %d",
                 pkt->device_id, pkt->msg_type, pkt->current_mode);
        // Set logical state without triggering an echo broadcast
        state_machine.set_mode((VentilationMode)pkt->current_mode, millis(),
                               pkt->remaining_duration_ms);
        update_hardware();
        changed = true;
      }

      // 3. Fan intensity sync
      if (pkt->fan_intensity > 0 && pkt->fan_intensity <= 10 &&
          pkt->fan_intensity != current_fan_intensity) {
        ESP_LOGI("vent", "Syncing fan intensity from peer %d: %d",
                 pkt->device_id, pkt->fan_intensity);
        // FIXED: Use set_fan_intensity with notify=false to avoid echo loops
        set_fan_intensity(pkt->fan_intensity, false);
        changed = true;
      }
    }

    // 5. Temperature sync
    if (!std::isnan(pkt->t_in)) {
      last_peer_t_in = pkt->t_in;
      last_peer_t_in_time = millis();
    }
    if (!std::isnan(pkt->t_out)) {
      last_peer_t_out = pkt->t_out;
      last_peer_t_out_time = millis();
    }

    // 6. PID Demand sync
    if (!std::isnan(pkt->pid_demand)) {
      last_peer_pid_demand = pkt->pid_demand;
      last_peer_pid_demand_time = millis();
      has_peer_pid_demand = true;
    }

    return changed;
  }

  // --- HELPERS ---

  /// @brief Convenience wrapper — returns remaining ventilation timer in ms.
  uint32_t get_remaining_duration() {
    return state_machine.get_remaining_duration(millis());
  }

  /// @brief Applies the state machine's target state to the physical hardware.
  /// Sets the direction switch and handles fan on/off bridging.
  /// The actual PWM/speed logic is handled by update_fan_logic().
  void update_hardware(const HardwareState &state, bool force_log = false) {
    bool target_in = state.direction_in;
    bool enable_fan = state.fan_enabled;

    // Rate-limited log during loop calls to avoid loop spam
    uint32_t now = millis();
    if (force_log || (now - last_hw_log_ms > 1000)) {
      ESP_LOGD("vent",
               "Hardware Refresh: Mode %d, Intensity %d, Phase: %s, Direction: "
               "%s, Ramp: %.2f",
               state_machine.current_mode, current_fan_intensity,
               state_machine.is_phase_a ? "A" : "B",
               target_in ? "ZULUFT (IN)" : "ABLUFT (OUT)", state.ramp_factor);
      last_hw_log_ms = now;
    }

    // 1. Direction switch (only toggle if changed)
    if (direction_switch) {
      bool current_sw = direction_switch->state;
      if (target_in != current_sw) {
        if (target_in)
          direction_switch->turn_on();
        else
          direction_switch->turn_off();
      }
    }

    // 2. Fan on/off bridging
    // Note: We turn the fan component on/off but delegating the actual
    // PWM calculation to update_fan_logic() which uses the ramp_factor.
    if (main_fan) {
      if (!enable_fan) {
        if (main_fan->state)
          main_fan->turn_off().perform();
      } else {
        if (!main_fan->state)
          main_fan->turn_on().perform();
      }
    }

    // 3. Trigger PWM update (includes ramp factor)
    // This refers to the global function in automation_helpers.h
    ::update_fan_logic();
  }

  /// @brief Convenience wrapper that calculates current state and applies it.
  void update_hardware() {
    update_hardware(state_machine.get_target_state(millis()));
  }

  /// @brief Serializes the current state into a VentilationPacket byte vector.
  /// Called by the YAML on_send lambda. Clears pending_broadcast flag.
  /// @param type  MessageType to stamp into the packet.
  /// @return Byte vector ready for espnow.send().
  std::vector<uint8_t> build_packet(MessageType type) {
    ESP_LOGI("vent_sync",
             "Building packet type %d from device %d. Clearing pending_broadcast.", type, device_id);
    VentilationPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.magic_header = 0x42;
    pkt.protocol_version =
        PROTOCOL_VERSION; // FIXED K2: stamp version for peer validation
    pkt.floor_id = floor_id;
    pkt.room_id = room_id;
    pkt.device_id = device_id;
    pkt.msg_type = type;
    pkt.current_mode = state_machine.current_mode;
    pkt.remaining_duration_ms = get_remaining_duration();
    pkt.fan_intensity = current_fan_intensity;
    pkt.timestamp_ms = millis();

    pkt.cycle_pos_ms = state_machine.get_cycle_pos(millis());
    pkt.phase_state = state_machine.global_phase;

    // Fill sensor data from local components if bound
    pkt.fan_rpm = (fan_rpm_sensor_ && fan_rpm_sensor_->has_state()) ? fan_rpm_sensor_->state : (float)NAN;
    pkt.board_temp = (board_temp_sensor_ && board_temp_sensor_->has_state()) ? board_temp_sensor_->state : (float)NAN;
    
    // Room Temp Logic: SCD41 (Primary) -> BME680 (Fallback)
    float r_temp = (float)NAN;
    if (scd41_temp_sensor_ && scd41_temp_sensor_->has_state()) {
        r_temp = scd41_temp_sensor_->state;
    } else if (bme680_temp_sensor_ && bme680_temp_sensor_->has_state()) {
        r_temp = bme680_temp_sensor_->state;
    }
    pkt.room_temp = r_temp;
    
    // Sync PID demand and NTC values
    pkt.pid_demand = local_pid_demand;
    pkt.t_in = local_t_in;
    pkt.t_out = local_t_out;

    std::vector<uint8_t> data(sizeof(VentilationPacket));
    memcpy(data.data(), &pkt, sizeof(VentilationPacket));

    pending_broadcast = false;
    return data;
  }
};

} // namespace esphome
