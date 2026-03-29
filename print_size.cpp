#include <iostream>
#include <cstdint>
struct __attribute__((packed)) VentilationPacket {
  uint8_t magic_header;
  uint8_t protocol_version;
  uint8_t floor_id;
  uint8_t room_id;
  uint8_t device_id;
  uint8_t msg_type;
  uint8_t current_mode;
  uint8_t current_mode_index;
  uint32_t timestamp_ms;
  uint32_t cycle_pos_ms;
  uint32_t remaining_duration_ms;
  bool phase_state;
  float t_in;
  float t_out;
  float pid_demand;
  uint8_t fan_intensity;
  bool co2_auto_enabled;
  uint8_t automatik_min_fan_level;
  uint8_t automatik_max_fan_level;
  uint16_t auto_co2_threshold_val;
  uint8_t auto_humidity_threshold_val;
  int8_t auto_presence_val;
  uint16_t sync_interval_min;
  uint16_t vent_timer_min;
  float max_led_brightness;
};
int main() {
    std::cout << "Size: " << sizeof(VentilationPacket) << std::endl;
    return 0;
}
