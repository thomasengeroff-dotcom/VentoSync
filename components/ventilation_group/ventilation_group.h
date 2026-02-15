#pragma once

/// @file ventilation_group.h
/// @brief ESPHome component that wraps VentilationStateMachine and adds
/// hardware I/O (fan, direction switch) and ESP-NOW group synchronization.

#include "esphome.h"
#include <vector>
#include "ventilation_state_machine.h"

namespace esphome {

// ---------------------------------------------------------
// ENUMS & STRUCTS
// ---------------------------------------------------------

// Enum moved to ventilation_state_machine.h

/// @brief ESP-NOW packet type identifiers.
enum MessageType {
  MSG_DISCOVERY = 1,   ///< Announce presence after boot.
  MSG_SYNC = 2,        ///< Periodic time-sync packet (cycle position).
  MSG_STATE = 3        ///< Mode or fan-intensity change notification.
};

/// @brief Binary packet exchanged between peer devices via ESP-NOW.
/// Layout is packed and must be identical on all firmware builds.
struct __attribute__((packed)) VentilationPacket {
  uint8_t magic_header;            ///< Always 0x42 — used for basic validation.
  uint8_t floor_id;                ///< Floor group (filters unrelated devices).
  uint8_t room_id;                 ///< Room group within the floor.
  uint8_t device_id;               ///< Unique sender ID (used to ignore own packets).
  uint8_t msg_type;                ///< MessageType enum value.
  uint8_t current_mode;            ///< VentilationMode enum value.
  uint8_t fan_intensity;           ///< Fan intensity level (1–10).
  uint32_t timestamp_ms;           ///< Sender's millis() at packet creation.
  uint32_t cycle_pos_ms;           ///< Sender's position in the direction cycle.
  uint32_t remaining_duration_ms;  ///< Remaining ventilation timer (0 = infinite).
  bool phase_state;                ///< Sender's current global phase (A or B).
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
  uint8_t floor_id = 1;      ///< Floor group for ESP-NOW filtering.
  uint8_t room_id = 1;       ///< Room group for ESP-NOW filtering.
  uint8_t device_id = 1;     ///< Unique device ID within the room.
  bool is_phase_a = true;    ///< Phase group: A starts IN, B starts OUT.
  
  // --- STATE ---
  VentilationStateMachine state_machine;  ///< Pure-logic state machine (no HW deps).
  
  uint32_t sync_interval_ms = 10800000;   ///< Auto-sync broadcast interval (default 3 h).
  uint8_t current_fan_intensity = 5;      ///< Cached fan intensity (1–10).

  // --- INTERNAL ---
  uint32_t last_sync_tx = 0;       ///< millis() of last sync broadcast.
  bool pending_broadcast = false;  ///< True = YAML should send a packet next loop.

  // --- HARDWARE REFS (set by codegen) ---
  fan::Fan *main_fan{nullptr};               ///< ESPHome fan component.
  switch_::Switch *direction_switch{nullptr}; ///< ON = intake, OFF = exhaust.

  // --- SETTERS (called by ESPHome codegen from YAML config) ---
  void set_floor_id(uint8_t id) { floor_id = id; }            ///< Set floor group.
  void set_room_id(uint8_t id) { room_id = id; }              ///< Set room group.
  void set_device_id(uint8_t id) { device_id = id; }          ///< Set unique device ID.
  void set_is_phase_a(bool phase_a) { state_machine.is_phase_a = phase_a; } ///< Set phase group.
  void set_main_fan(fan::Fan *fan) { main_fan = fan; }        ///< Bind the fan component.
  void set_direction_switch(switch_::Switch *sw) { direction_switch = sw; } ///< Bind the direction switch.

  VentilationController() {}

  /// @brief Component setup — logs config and schedules initial discovery broadcast.
  void setup() override {
    ESP_LOGCONFIG("vent", "Ventilation Group Setup: Floor %d, Room %d, Device %d, Phase %s", 
                  floor_id, room_id, device_id, is_phase_a ? "A" : "B");
    pending_broadcast = true; // Announce presence on boot
  }

  /// @brief Main loop — ticks the state machine, refreshes hardware, and
  /// schedules periodic sync broadcasts.
  void loop() override {
    uint32_t now = millis();

    // 1. Update State Machine
    bool dirty = state_machine.update(now);

    // 2. Hardware Update
    if (dirty) update_hardware();

    // 3. Auto Sync Broadcast (Every sync_interval_ms)
    if (now - last_sync_tx > sync_interval_ms) {
        pending_broadcast = true; // Let YAML trigger the send
        last_sync_tx = now;
    }
  }

  // --- ACTIONS ---

  /// @brief Updates the half-cycle duration and refreshes hardware / notifies peers.
  void set_cycle_duration(uint32_t ms) {
      if (ms == state_machine.cycle_duration_ms) return;
      state_machine.set_cycle_duration(ms);
      ESP_LOGI("vent", "Cycle duration updated to %d ms", ms);
      update_hardware();
      pending_broadcast = true;
  }

  /// @brief Changes the auto-sync broadcast interval.
  void set_sync_interval(uint32_t ms) {
      sync_interval_ms = ms;
      ESP_LOGI("vent", "Sync interval updated to %d ms", sync_interval_ms);
  }

  /// @brief Sets the fan intensity level (1–10) and notifies peers.
  void set_fan_intensity(uint8_t intensity) {
      if (current_fan_intensity == intensity) return;
      current_fan_intensity = intensity;
      ESP_LOGI("vent", "Fan Intensity updated to %d", intensity);
      pending_broadcast = true;
  }

  /// @brief Switches operating mode, delegates to state machine, refreshes HW, notifies peers.
  /// @param mode     Target VentilationMode.
  /// @param duration For MODE_VENTILATION: auto-stop timer in ms (0 = infinite).
  void set_mode(VentilationMode mode, uint32_t duration = 0) {
      if (state_machine.current_mode == mode && duration == state_machine.ventilation_duration_ms) return;
      
      ESP_LOGI("vent", "Mode change: %d -> %d (Duration: %d ms)", state_machine.current_mode, mode, duration);
      state_machine.set_mode(mode, millis(), duration);
      
      update_hardware();
      pending_broadcast = true;
  }

  /// @brief Processes an incoming ESP-NOW packet.
  /// Validates header, filters by floor/room, then syncs mode, timer, intensity.
  /// @param data  Raw byte vector received via ESP-NOW.
  /// @return true if any local state was changed (caller should update UI).
  bool on_packet_received(std::vector<uint8_t> data) {
      if (data.size() != sizeof(VentilationPacket)) return false;
      VentilationPacket *pkt = (VentilationPacket*)data.data();

      // Filter: magic, group, self
      if (pkt->magic_header != 0x42) return false;
      if (pkt->floor_id != floor_id || pkt->room_id != room_id) return false;
      if (pkt->device_id == device_id) return false;

      bool changed = false;

      // 1. Time sync (aligns direction cycle phase)
      if (pkt->msg_type == MSG_SYNC) {
          state_machine.sync_time(millis(), pkt->cycle_pos_ms);
      }
      
      // 2. Mode & timer sync (>2 s drift triggers re-sync)
      if (pkt->current_mode != state_machine.current_mode || 
         (pkt->current_mode == MODE_VENTILATION && abs((int)(pkt->remaining_duration_ms - state_machine.get_remaining_duration(millis()))) > 2000)) {
          
          ESP_LOGI("vent", "Syncing mode from peer: %d (Duration: %d)", pkt->current_mode, pkt->remaining_duration_ms);
          set_mode((VentilationMode)pkt->current_mode, pkt->remaining_duration_ms);
          changed = true;
      }

      // 3. Fan intensity sync
      if (pkt->fan_intensity > 0 && pkt->fan_intensity <= 10 && pkt->fan_intensity != current_fan_intensity) {
          ESP_LOGI("vent", "Syncing fan intensity from peer: %d", pkt->fan_intensity);
          current_fan_intensity = pkt->fan_intensity;
          changed = true;
      }
      
      return changed;
  }

  // --- HELPERS ---
  
  /// @brief Convenience wrapper — returns remaining ventilation timer in ms.
  uint32_t get_remaining_duration() {
      return state_machine.get_remaining_duration(millis());
  }

  /// @brief Applies the state machine's target state to the physical hardware.
  /// Sets the direction switch and turns the fan on/off as needed.
  void update_hardware() {
      HardwareState state = state_machine.get_target_state();
      bool target_in = state.direction_in;
      bool enable_fan = state.fan_enabled;

      // 1. Direction switch (only toggle if changed)
      if (direction_switch) {
          bool current_sw = direction_switch->state;
          if (target_in != current_sw) {
              if (target_in) direction_switch->turn_on();
              else direction_switch->turn_off();
          }
      }

      // 2. Fan on/off (only toggle if changed)
      if (main_fan) {
          if (!enable_fan) {
               if (main_fan->state) main_fan->turn_off().perform();
          } else {
               if (!main_fan->state) main_fan->turn_on().perform();
          }
      }
  }

  /// @brief Serializes the current state into a VentilationPacket byte vector.
  /// Called by the YAML on_send lambda. Clears pending_broadcast flag.
  /// @param type  MessageType to stamp into the packet.
  /// @return Byte vector ready for espnow.send().
  std::vector<uint8_t> build_packet(MessageType type) {
      VentilationPacket pkt;
      pkt.magic_header = 0x42;
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
      
      std::vector<uint8_t> data(sizeof(VentilationPacket));
      memcpy(data.data(), &pkt, sizeof(VentilationPacket));
      
      pending_broadcast = false;
      return data;
  }
};

} // namespace esphome
