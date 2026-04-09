#include <iostream>
#include <cstdint>

/**
 * @brief Utility script to verify the binary size of the VentilationPacket.
 * 
 * ESP-NOW has a hard payload limit of 250 bytes. This struct MUST stay
 * below that limit. The packed attribute ensures no padding is added.
 * 
 * This definition must be kept in sync with:
 * components/ventilation_group/ventilation_group.h
 */

struct __attribute__((packed)) VentilationPacket {
  uint8_t magic_header;
  uint8_t protocol_version;
  uint8_t floor_id;
  uint8_t room_id;
  uint8_t device_id;
  uint8_t msg_type;
  uint8_t current_mode;
  uint8_t current_mode_index;

  // Live Data Synced States
  uint32_t timestamp_ms;
  uint32_t cycle_pos_ms;
  uint32_t remaining_duration_ms;
  bool phase_state;
  float t_in;
  float t_out;
  float pid_demand;
  float fan_rpm;
  float board_temp;
  float room_temp;

  // Control & Settings Synced States
  uint8_t fan_intensity;

  // Automatik Config payload
  uint8_t automatik_min_fan_level;
  uint8_t automatik_max_fan_level;
  uint16_t auto_co2_threshold_val;
  uint8_t auto_humidity_threshold_val;
  int8_t auto_presence_val;

  // Timer Settings payload
  uint16_t sync_interval_min;
  uint16_t vent_timer_min;

  // UI Settings payload
  float max_led_brightness;
};

int main() {
    std::cout << "--- VentoSync Packet Size Check ---" << std::endl;
    std::cout << "Struct: VentilationPacket" << std::endl;
    std::cout << "Size:   " << sizeof(VentilationPacket) << " bytes" << std::endl;
    std::cout << "Limit:  250 bytes (ESP-NOW)" << std::endl;
    
    if (sizeof(VentilationPacket) <= 250) {
        std::cout << "Status: OK (within limits)" << std::endl;
    } else {
        std::cout << "Status: ERROR (EXCEEDS LIMIT!)" << std::endl;
    }
    
    return 0;
}
