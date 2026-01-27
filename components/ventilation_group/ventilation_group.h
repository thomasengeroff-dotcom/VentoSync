#pragma once

#include "esphome.h"
#include <vector>

namespace esphome {

// ---------------------------------------------------------
// ENUMS & STRUCTS
// ---------------------------------------------------------

enum VentilationMode {
  MODE_OFF = 0,         // Fan Off
  MODE_ECO_RECOVERY = 1, // Alternating Direction (Heat Recovery)
  MODE_VENTILATION = 2   // Constant Direction (Durchlüften)
};

enum MessageType {
  MSG_DISCOVERY = 1,   // "I am here"
  MSG_SYNC = 2,        // "Here is the time based on my clock"
  MSG_STATE = 3        // "My mode changed"
};

// Data packet structure (Must match on all devices!)
struct __attribute__((packed)) VentilationPacket {
  uint8_t magic_header; // 0x42
  uint8_t floor_id;
  uint8_t room_id;
  uint8_t device_id;
  uint8_t msg_type;     // MessageType
  uint8_t current_mode; // VentilationMode
  uint32_t timestamp_ms;// Sender's millis()
  uint32_t cycle_pos_ms;// Position in the global cycle
  bool phase_state;     // Global phase (A or B)
};

// ---------------------------------------------------------
// CONTROLLER CLASS
// ---------------------------------------------------------

class VentilationController : public Component {
 public:
  // --- CONFIGURATION ---
  uint8_t floor_id = 1;
  uint8_t room_id = 1;
  uint8_t device_id = 1;
  bool is_phase_a = true; // true = Group A (starts IN), false = Group B (starts OUT)
  
  // Timing Config
  uint32_t cycle_duration_ms = 70000; // 70s per direction (140s total cycle)
  uint32_t sync_interval_ms = 10800000; // 3 hours
  
  // --- STATE ---
  VentilationMode current_mode = MODE_ECO_RECOVERY;
  bool global_phase = true; // true = Phase A, false = Phase B
  uint32_t ventilation_duration_ms = 0; // 0 = Infinite
  
  // --- INTERNAL ---
  uint32_t last_sync_tx = 0;
  int32_t time_offset_ms = 0;
  uint32_t ventilation_start_time = 0;
  bool pending_broadcast = false;

  // --- HARDWARE REFS ---
  fan::Fan *main_fan{nullptr};
  switch_::Switch *direction_switch{nullptr}; // ON = IN, OFF = OUT

  // --- SETTERS for Codegen ---
  void set_floor_id(uint8_t id) { floor_id = id; }
  void set_room_id(uint8_t id) { room_id = id; }
  void set_device_id(uint8_t id) { device_id = id; }
  void set_is_phase_a(bool phase_a) { is_phase_a = phase_a; }
  void set_main_fan(fan::Fan *fan) { main_fan = fan; }
  void set_direction_switch(switch_::Switch *sw) { direction_switch = sw; }

  VentilationController() {}

  void setup() override {
    ESP_LOGCONFIG("vent", "Ventilation Group Setup: Floor %d, Room %d, Device %d, Phase %s", 
                  floor_id, room_id, device_id, is_phase_a ? "A" : "B");
    pending_broadcast = true; // Announce presence on boot
  }

  void loop() override {
    uint32_t now = millis();

    // 1. Handle Ventilation Timer
    if (current_mode == MODE_VENTILATION && ventilation_duration_ms > 0) {
        if (now - ventilation_start_time > ventilation_duration_ms) {
            ESP_LOGI("vent", "Ventilation timer expired. Switching to Heat Recovery.");
            set_mode(MODE_ECO_RECOVERY);
        }
    }

    // 2. Cycle Logic (Heat Recovery Timing)
    // Calculate global position: (millis + offset) % (2 * cycle_time)
    uint32_t period = cycle_duration_ms * 2;
    uint32_t pos = (now + time_offset_ms) % period;
    bool new_phase_a_active = (pos < cycle_duration_ms); // First half is Phase A active

    if (new_phase_a_active != global_phase) {
        global_phase = new_phase_a_active;
        ESP_LOGD("vent", "Global phase flip. A_Active: %s", global_phase ? "YES" : "NO");
        update_hardware();
    }

    // 3. Auto Sync Broadcast (Every 3h)
    if (now - last_sync_tx > sync_interval_ms) {
        pending_broadcast = true; // Let YAML trigger the send
        last_sync_tx = now;
    }
  }

  // --- ACTIONS ---

  void set_cycle_duration(uint32_t ms) {
      if (ms == cycle_duration_ms) return;
      cycle_duration_ms = ms;
      ESP_LOGI("vent", "Cycle duration updated to %d ms", cycle_duration_ms);
      update_hardware();
      pending_broadcast = true;
  }

  void set_sync_interval(uint32_t ms) {
      sync_interval_ms = ms;
      ESP_LOGI("vent", "Sync interval updated to %d ms", sync_interval_ms);
  }

  void set_mode(VentilationMode mode, uint32_t duration = 0) {
      if (current_mode == mode && ventilation_duration_ms == duration) return;
      
      ESP_LOGI("vent", "Mode change: %d -> %d (Duration: %d ms)", current_mode, mode, duration);
      current_mode = mode;
      ventilation_duration_ms = duration;
      
      if (mode == MODE_VENTILATION) {
          ventilation_start_time = millis();
      }
      
      update_hardware();
      pending_broadcast = true; // Notify peers
  }

  void on_packet_received(std::vector<uint8_t> data) {
      if (data.size() != sizeof(VentilationPacket)) return;
      VentilationPacket *pkt = (VentilationPacket*)data.data();

      // Filter
      if (pkt->magic_header != 0x42) return;
      if (pkt->floor_id != floor_id || pkt->room_id != room_id) return;
      if (pkt->device_id == device_id) return; // Ignore self

      // Handle Sync
      if (pkt->msg_type == MSG_SYNC) {
          sync_time(pkt->cycle_pos_ms);
      }
      
      // Handle Mode Sync
      // Only accept mode changes from peers if we are in a different mode
      // Logic could be stricter (Master only?), but for now simple mesh-like sync
      if (pkt->current_mode != current_mode) {
          ESP_LOGI("vent", "Syncing mode from peer: %d", pkt->current_mode);
          set_mode((VentilationMode)pkt->current_mode, 0); // Slave doesn't inherit timer, just mode? Or should it?
          // TODO: Improve timer sync if needed
      }
  }

  // --- HELPERS ---

  void sync_time(uint32_t target_pos_ms) {
      uint32_t now = millis();
      uint32_t period = cycle_duration_ms * 2;
      uint32_t my_pos = (now + time_offset_ms) % period;
      
      // Calculate diff accounting for wrap-around
      int32_t diff = (int32_t)target_pos_ms - (int32_t)my_pos;
       // Shortest path logic
      if (diff > (int32_t)cycle_duration_ms) diff -= period;
      if (diff < -(int32_t)cycle_duration_ms) diff += period;
      
      if (abs(diff) > 200) { // Hysteresis to prevent jitter
          time_offset_ms += diff;
          ESP_LOGD("vent", "Time Sync: Adjusted by %d ms", diff);
          update_hardware(); // Re-eval hardware in case phase jumped
      }
  }

  void update_hardware() {
      bool target_in = true; // Default IN

      if (current_mode == MODE_VENTILATION) {
          // In Ventilation, we stick to our physical orientation
          target_in = is_phase_a; 
      } else {
          // In Recovery, we alternate
          // global_phase = TRUE means Phase A devices are IN
          if (global_phase) {
              target_in = is_phase_a;
          } else {
              target_in = !is_phase_a;
          }
      }

      // 1. Set Direction Switch
      if (direction_switch) {
          bool current_sw = direction_switch->state;
          // Assuming Switch ON = IN, OFF = OUT
          if (target_in != current_sw) {
              if (target_in) direction_switch->turn_on();
              else direction_switch->turn_off();
          }
      }

      // 2. Set Fan State
      if (main_fan) {
          if (current_mode == MODE_OFF) {
               if (main_fan->state) main_fan->turn_off().perform();
          } else {
               if (!main_fan->state) main_fan->turn_on().perform();
          }
      }
  }

  // Called by YAML to get packet to send
  std::vector<uint8_t> build_packet(MessageType type) {
      VentilationPacket pkt;
      pkt.magic_header = 0x42;
      pkt.floor_id = floor_id;
      pkt.room_id = room_id;
      pkt.device_id = device_id;
      pkt.msg_type = type;
      pkt.current_mode = current_mode;
      pkt.timestamp_ms = millis();
      
      uint32_t period = cycle_duration_ms * 2;
      pkt.cycle_pos_ms = (millis() + time_offset_ms) % period;
      pkt.phase_state = global_phase;
      
      std::vector<uint8_t> data(sizeof(VentilationPacket));
      memcpy(data.data(), &pkt, sizeof(VentilationPacket));
      
      pending_broadcast = false; // Clear flag
      return data;
  }
};

} // namespace esphome
