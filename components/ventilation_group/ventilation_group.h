#pragma once

#include "esphome.h"
#include <vector>

namespace esphome {

// ---------------------------------------------------------
// ENUMS & STRUCTS
// ---------------------------------------------------------

enum VentilationMode {
  MODE_OFF = 0,              // Fan Off
  MODE_ECO_RECOVERY = 1,     // Alternating Direction (Heat Recovery)
  MODE_VENTILATION = 2,      // Constant Direction (Durchlüften)
  MODE_STOSSLUEFTUNG = 3     // Stoßlüftung: 15min WRG + 105min Pause (2h Cycle)
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
  uint8_t fan_intensity; // Fan Intensity (1-10)
  uint32_t timestamp_ms;// Sender's millis()
  uint32_t cycle_pos_ms;// Position in the global cycle
  uint32_t remaining_duration_ms; // Remaining time for current mode (0=infinite)
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
  uint8_t current_fan_intensity = 5; // Default to 5 (matches YAML global)
  
  // --- INTERNAL ---
  uint32_t last_sync_tx = 0;
  int32_t time_offset_ms = 0;
  uint32_t ventilation_start_time = 0;
  bool pending_broadcast = false;

  // Stoßlüftung State
  uint32_t stoss_cycle_start = 0;       // Start of current 2h cycle
  bool stoss_active_phase = true;       // true = 15min WRG active, false = 105min pause
  bool stoss_direction_flip = false;    // Flips after each 2h cycle
  static constexpr uint32_t STOSS_ACTIVE_MS  = 15UL * 60 * 1000;  // 15 minutes
  static constexpr uint32_t STOSS_PAUSE_MS   = 105UL * 60 * 1000; // 105 minutes
  static constexpr uint32_t STOSS_CYCLE_MS   = STOSS_ACTIVE_MS + STOSS_PAUSE_MS; // 2 hours

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

    // 1b. Handle Stoßlüftung Cycle (15min WRG + 105min Pause)
    if (current_mode == MODE_STOSSLUEFTUNG) {
        uint32_t elapsed = now - stoss_cycle_start;
        if (stoss_active_phase) {
            // Active phase: 15 min WRG
            if (elapsed >= STOSS_ACTIVE_MS) {
                // Switch to pause phase
                stoss_active_phase = false;
                stoss_cycle_start = now;
                ESP_LOGI("vent", "Stoßlüftung: Active phase done, pausing for 105 min");
                if (main_fan && main_fan->state) main_fan->turn_off().perform();
            }
        } else {
            // Pause phase: 105 min
            if (elapsed >= STOSS_PAUSE_MS) {
                // Start new 2h cycle with reversed direction
                stoss_active_phase = true;
                stoss_direction_flip = !stoss_direction_flip;
                stoss_cycle_start = now;
                ESP_LOGI("vent", "Stoßlüftung: New cycle, direction flipped to %s",
                         stoss_direction_flip ? "reversed" : "normal");
                if (main_fan && !main_fan->state) main_fan->turn_on().perform();
                update_hardware();
            }
            return; // During pause, skip cycle logic
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

  void set_fan_intensity(uint8_t intensity) {
      if (current_fan_intensity == intensity) return;
      current_fan_intensity = intensity;
      ESP_LOGI("vent", "Fan Intensity updated to %d", intensity);
      pending_broadcast = true;
  }

  void set_mode(VentilationMode mode, uint32_t duration = 0) {
      // If same mode and duration is mostly same, return? 
      // But receiving sync might need to force update timer
      bool changed = (current_mode != mode);
      if (!changed && duration == ventilation_duration_ms) return;
      
      ESP_LOGI("vent", "Mode change: %d -> %d (Duration: %d ms)", current_mode, mode, duration);
      current_mode = mode;
      ventilation_duration_ms = duration;
      
      if (mode == MODE_VENTILATION) {
          ventilation_start_time = millis();
      }
      if (mode == MODE_STOSSLUEFTUNG) {
          stoss_cycle_start = millis();
          stoss_active_phase = true;
          stoss_direction_flip = false;
          ESP_LOGI("vent", "Stoßlüftung started: 15min WRG + 105min Pause cycle");
      }
      
      update_hardware();
      pending_broadcast = true; // Notify peers
  }

  bool on_packet_received(std::vector<uint8_t> data) {
      if (data.size() != sizeof(VentilationPacket)) return false;
      VentilationPacket *pkt = (VentilationPacket*)data.data();

      // Filter
      if (pkt->magic_header != 0x42) return false;
      if (pkt->floor_id != floor_id || pkt->room_id != room_id) return false;
      if (pkt->device_id == device_id) return false; // Ignore self

      bool changed = false;

      // Handle Sync
      if (pkt->msg_type == MSG_SYNC) {
          sync_time(pkt->cycle_pos_ms);
      }
      
      // Handle Mode & Timer Sync
      if (pkt->current_mode != current_mode || 
         (pkt->current_mode == MODE_VENTILATION && abs((int)(pkt->remaining_duration_ms - get_remaining_duration())) > 2000)) {
          
          ESP_LOGI("vent", "Syncing mode from peer: %d (Duration: %d)", pkt->current_mode, pkt->remaining_duration_ms);
          set_mode((VentilationMode)pkt->current_mode, pkt->remaining_duration_ms);
          changed = true;
      }

      // Handle Fan Intensity Sync
      if (pkt->fan_intensity > 0 && pkt->fan_intensity <= 10 && pkt->fan_intensity != current_fan_intensity) {
          ESP_LOGI("vent", "Syncing fan intensity from peer: %d", pkt->fan_intensity);
          current_fan_intensity = pkt->fan_intensity;
          changed = true;
      }
      
      return changed;
  }

  // --- HELPERS ---
  
  uint32_t get_remaining_duration() {
      if (ventilation_duration_ms == 0) return 0;
      uint32_t elapsed = millis() - ventilation_start_time;
      if (elapsed >= ventilation_duration_ms) return 0;
      return ventilation_duration_ms - elapsed;
  }

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

      if (current_mode == MODE_OFF) {
          // OFF mode handled below in fan state section
      } else if (current_mode == MODE_VENTILATION) {
          // In Ventilation, we stick to our physical orientation
          target_in = is_phase_a; 
      } else if (current_mode == MODE_STOSSLUEFTUNG) {
          // Stoßlüftung: During active phase, behave like ECO_RECOVERY
          // but apply direction flip after each 2h cycle
          if (stoss_active_phase) {
              if (global_phase) {
                  target_in = stoss_direction_flip ? !is_phase_a : is_phase_a;
              } else {
                  target_in = stoss_direction_flip ? is_phase_a : !is_phase_a;
              }
          }
      } else {
          // ECO_RECOVERY: We alternate
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
          if (current_mode == MODE_OFF || (current_mode == MODE_STOSSLUEFTUNG && !stoss_active_phase)) {
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
      pkt.remaining_duration_ms = get_remaining_duration();
      pkt.fan_intensity = current_fan_intensity;
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
