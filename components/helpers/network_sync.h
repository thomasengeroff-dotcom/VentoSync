// ==========================================================================
// WRG Wohnraumlüftung – ESPHome Custom Component
// https://github.com/thomasengeroff-dotcom/ESPHome-Wohnraumlueftung
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
// File:        network_sync.h
// Description: ESP-NOW peer synchronization and state mirroring.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "helpers/globals.h"

// --- Constants ---
constexpr uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr size_t MAX_PAYLOAD_LEN = 100;

inline bool is_local_mac(const uint8_t *mac) {
  uint8_t local_mac[6];
  esp_read_mac(local_mac, ESP_MAC_WIFI_STA);
  for (int i = 0; i < 6; i++) {
    if (mac[i] != local_mac[i])
      return false;
  }
  return true;
}

/** @brief Checks if this is the only device in the room group. */
inline bool is_single_device_group() {
  if (espnow_peers == nullptr) return true;
  std::string peer_list = espnow_peers->value();
  return peer_list.empty();
}

/** @brief Registers a MAC address as an ESP-NOW peer and persists it to NVS. */
inline void register_peer_dynamic(const uint8_t *mac) {
  if (!esphome::espnow::global_esp_now) {
    ESP_LOGW("espnow_disc",
             "global_esp_now not ready, skipping register_peer_dynamic");
    return;
  }
  if (is_local_mac(mac))
    return; // Don't register ourselves

  if (espnow_peers == nullptr) {
    ESP_LOGE("espnow_disc", "espnow_peers global is null!");
    return;
  }
  char mac_buf[20];
  snprintf(mac_buf, sizeof(mac_buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);
  std::string mac_str(mac_buf);

  std::string &current_list = espnow_peers->value();
  if (current_list.find(mac_str) == std::string::npos) {
    if (!current_list.empty())
      current_list += ",";
    current_list += mac_str;
    ESP_LOGI("espnow_disc", "New peer discovered and saved: %s",
             mac_str.c_str());
  }

  esp_now_peer_info_t peer_info = {};
  memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
  memcpy(peer_info.peer_addr, mac, 6);
  peer_info.channel = 0; // 0 = use current channel (auto)
  peer_info.encrypt = false;
  peer_info.ifidx = WIFI_IF_STA;

  esp_now_del_peer(mac);
  esp_err_t err = esp_now_add_peer(&peer_info);

  if (err == ESP_OK) {
    ESP_LOGI("espnow_disc", "Peer %s registered (channel: current)",
             mac_str.c_str());
  } else {
    ESP_LOGW("espnow_disc", "Failed to register peer %s: %d", mac_str.c_str(),
             err);
  }
}

/** @brief Loads all peers saved in the runtime string cache. */
inline void load_peers_from_runtime_cache() {
  if (!esphome::espnow::global_esp_now || espnow_peers == nullptr)
    return;
  std::string current_list = espnow_peers->value();
  if (current_list.empty())
    return;

  size_t start = 0;
  size_t end = current_list.find(",");
  while (true) {
    std::string mac_str = (end == std::string::npos)
                              ? current_list.substr(start)
                              : current_list.substr(start, end - start);
    auto mac = parse_mac_local(mac_str);
    if (mac.has_value()) {
      esphome::espnow::global_esp_now->add_peer(mac->data());
      ESP_LOGI("espnow_disc", "Restored peer from flash: %s", mac_str.c_str());
    }
    if (end == std::string::npos)
      break;
    start = end + 1;
    end = current_list.find(",", start);
  }
}

/** @brief Sends a discovery broadcast to identify peers in the same room. */
inline void send_discovery_broadcast() {
  if (!esphome::espnow::global_esp_now || !config_floor_id || !config_room_id) return;
  
  // FIXED: Removed blocking delay to prevent main-loop stalls.
  // Simultaneous boot storms are better handled by the random_uint32() in the trigger itself.

  char buffer[64];
  int written =
      snprintf(buffer, sizeof(buffer), "ROOM_DISC:%d:%d",
               (int)config_floor_id->state, (int)config_room_id->state);
  if (written < 0 || written >= sizeof(buffer)) {
    ESP_LOGE("espnow_disc", "Buffer overflow prevented in discovery message");
    return;
  }

  std::string msg(buffer);
  std::vector<uint8_t> data(msg.begin(), msg.end());
  esphome::espnow::global_esp_now->send(BROADCAST_MAC, data,
                                        [](esp_err_t err) {});
  ESP_LOGI("espnow_disc", "Sent discovery broadcast: %s", msg.c_str());
}

/** @brief Requests current status from all known peers. */
inline void request_peer_status() {
  if (!esphome::espnow::global_esp_now)
    return;
  ESP_LOGI("vent_sync", "Requesting current status from peers...");
  auto data = build_and_populate_packet(esphome::MSG_STATUS_REQUEST);
  esphome::espnow::global_esp_now->send(BROADCAST_MAC, data,
                                        [](esp_err_t err) {});
}

/** @brief Sends a unicast confirmation to a discovered peer. */
inline void send_discovery_confirmation(const uint8_t *target_mac) {
  if (!esphome::espnow::global_esp_now || !config_floor_id || !config_room_id)
    return;

  char buffer[64];
  int written =
      snprintf(buffer, sizeof(buffer), "ROOM_CONF:%d:%d",
               (int)config_floor_id->state, (int)config_room_id->state);
  if (written < 0 || written >= sizeof(buffer)) {
    ESP_LOGE("espnow_disc",
             "Buffer overflow prevented in confirmation message");
    return;
  }

  std::string msg(buffer);
  std::vector<uint8_t> data(msg.begin(), msg.end());
  esphome::espnow::global_esp_now->send(target_mac, data, [](esp_err_t err) {});
}

/** @brief Sends a sync packet to all registered peers via unicast. */
inline void send_sync_to_all_peers(const std::vector<uint8_t> &data) {
  if (!esphome::espnow::global_esp_now || espnow_peers == nullptr)
    return;
  std::string current_list = espnow_peers->value();
  if (current_list.empty())
    return;

  size_t start = 0;
  size_t end = current_list.find(",");
  while (true) {
    std::string mac_str = (end == std::string::npos)
                              ? current_list.substr(start)
                              : current_list.substr(start, end - start);
    auto mac = parse_mac_local(mac_str);
    if (mac.has_value()) {
      esphome::espnow::global_esp_now->send(mac->data(), data,
                                            [](esp_err_t err) {});
    }
    if (end == std::string::npos)
      break;
    start = end + 1;
    end = current_list.find(",", start);
  }
}

/** @brief Parses incoming discovery strings. */
inline bool handle_discovery_payload(const std::string &payload,
                                     const uint8_t *src_addr) {
  if (payload.length() > MAX_PAYLOAD_LEN) {
    ESP_LOGW("espnow_disc", "Discovery payload too long: %d", payload.length());
    return false;
  }
  if (payload.find("ROOM_DISC:") == 0 || payload.find("ROOM_CONF:") == 0) {
    if (is_local_mac(src_addr))
      return true;

    int floor, room;
    if (sscanf(payload.c_str() + 10, "%d:%d", &floor, &room) == 2) {
      if (floor == (int)config_floor_id->state &&
          room == (int)config_room_id->state) {
        ESP_LOGI("espnow_disc", "Matching room discovery from %02X:%02X:%02X",
                 src_addr[3], src_addr[4], src_addr[5]);
        register_peer_dynamic(src_addr);
        if (payload.find("ROOM_DISC:") == 0) {
          send_discovery_confirmation(src_addr);
        }
        return true;
      }
    }
  }
  return false;
}

/** @brief Sends a sync packet to all registered peers via room-wide broadcast. */
inline void sync_settings_to_peers() {
  if (ventilation_ctrl == nullptr || !esphome::espnow::global_esp_now) return;
  
  // Broadcast loop guard (max 1 per 500ms)
  static uint32_t last_settings_sync = 0;
  if (millis() - last_settings_sync < 500) {
    ESP_LOGI("vent_sync", "Settings sync broadcast suppressed (loop prevention)");
    return;
  }
  last_settings_sync = millis();

  auto data = build_and_populate_packet(esphome::MSG_STATE);
  esphome::espnow::global_esp_now->send(BROADCAST_MAC, data, [](esp_err_t err) {});
  ESP_LOGI("vent_sync", "Sent state change via BROADCAST to room members.");
}

namespace espnow_handler {

  inline bool validate_packet(const std::vector<uint8_t> &data) {
    if (data.empty()) return false;
    
    // Check header and protocol version first for better diagnostics
    if (data.size() >= 2) {
      if (data[0] != 0x42) {
        ESP_LOGW("vent_sync", "Invalid magic header: 0x%02X", data[0]);
        return false;
      }
      if (data[1] != esphome::PROTOCOL_VERSION) {
        ESP_LOGW("vent_sync", "Protocol version mismatch! Got v%d, expected v%d. Devices on the network are running mismatched firmware.", 
                 data[1], esphome::PROTOCOL_VERSION);
        return false;
      }
    }

    // Now safely check strict size
    if (data.size() != sizeof(esphome::VentilationPacket)) {
      ESP_LOGW("vent_sync", "Invalid packet size: %d", data.size());
      return false;
    }

    const auto *pkt = reinterpret_cast<const esphome::VentilationPacket *>(data.data());
    
    // Validate configuration ranges to prevent potential out-of-bounds exploits
    if (pkt->fan_intensity > 10) return false;
    if (pkt->automatik_min_fan_level < 1 || pkt->automatik_min_fan_level > 10) return false;
    if (pkt->automatik_max_fan_level < 1 || pkt->automatik_max_fan_level > 10) return false;
    if (pkt->sync_interval_min < 1 || pkt->sync_interval_min > 1440) return false;

    return true;
  }

inline void handle_status_request(const esphome::VentilationPacket *pkt) {
  if (config_floor_id != nullptr && config_room_id != nullptr &&
      pkt->floor_id == (int)config_floor_id->state &&
      pkt->room_id == (int)config_room_id->state) {
    ESP_LOGI("vent_sync", "Status request from peer %d. Sending response...",
             pkt->device_id);
    auto resp = build_and_populate_packet(esphome::MSG_STATUS_RESPONSE);
    if (esphome::espnow::global_esp_now) {
      esphome::espnow::global_esp_now->send(BROADCAST_MAC, resp,
                                            [](esp_err_t err) {});
    }
  }
}

inline void handle_config_sync(const esphome::VentilationPacket *pkt) {
  bool dirty = false;
  auto *v = ventilation_ctrl;

  // 1. CO2 Automatik (Switch)
  if (co2_auto_enabled != nullptr && automatik_co2 != nullptr &&
      pkt->co2_auto_enabled != co2_auto_enabled->value()) {
    co2_auto_enabled->value() = pkt->co2_auto_enabled;
    automatik_co2->publish_state(pkt->co2_auto_enabled);
    ESP_LOGI("vent_sync", "Synced co2_auto_enabled: %d", pkt->co2_auto_enabled);
    dirty = true;
  }

  // 2. Automatik Min/Max Levels (Sliders 1-10)
  if (automatik_min_fan_level != nullptr &&
      automatik_min_luefterstufe != nullptr &&
      pkt->automatik_min_fan_level >= 1 && pkt->automatik_min_fan_level <= 10 &&
      pkt->automatik_min_fan_level != automatik_min_fan_level->value()) {
    automatik_min_fan_level->value() = pkt->automatik_min_fan_level;
    automatik_min_luefterstufe->publish_state(pkt->automatik_min_fan_level);
    dirty = true;
  }

  if (automatik_max_fan_level != nullptr &&
      automatik_max_luefterstufe != nullptr &&
      pkt->automatik_max_fan_level >= 1 && pkt->automatik_max_fan_level <= 10 &&
      pkt->automatik_max_fan_level != automatik_max_fan_level->value()) {
    automatik_max_fan_level->value() = pkt->automatik_max_fan_level;
    automatik_max_luefterstufe->publish_state(pkt->automatik_max_fan_level);
    dirty = true;
  }

  // 3. Thresholds
  if (auto_co2_threshold_val != nullptr && auto_co2_threshold != nullptr &&
      pkt->auto_co2_threshold_val >= 400 &&
      pkt->auto_co2_threshold_val <= 5000 &&
      pkt->auto_co2_threshold_val != auto_co2_threshold_val->value()) {
    auto_co2_threshold_val->value() = pkt->auto_co2_threshold_val;
    auto_co2_threshold->publish_state(pkt->auto_co2_threshold_val);
    dirty = true;
  }

  if (auto_humidity_threshold_val != nullptr &&
      auto_humidity_threshold != nullptr &&
      pkt->auto_humidity_threshold_val >= 0 &&
      pkt->auto_humidity_threshold_val <= 100 &&
      pkt->auto_humidity_threshold_val !=
          auto_humidity_threshold_val->value()) {
    auto_humidity_threshold_val->value() = pkt->auto_humidity_threshold_val;
    auto_humidity_threshold->publish_state(pkt->auto_humidity_threshold_val);
    dirty = true;
  }

  if (auto_presence_val != nullptr && auto_presence_slider != nullptr &&
      pkt->auto_presence_val >= -5 && pkt->auto_presence_val <= 5 &&
      pkt->auto_presence_val != auto_presence_val->value()) {
    auto_presence_val->value() = pkt->auto_presence_val;
    auto_presence_slider->publish_state(pkt->auto_presence_val);
    dirty = true;
  }

  // 5. System Timers
  if (sync_interval_config != nullptr && pkt->sync_interval_min >= 1 &&
      pkt->sync_interval_min <= 1440 &&
      v->sync_interval_ms != (uint32_t)(pkt->sync_interval_min * 60 * 1000)) {
    v->sync_interval_ms = (uint32_t)(pkt->sync_interval_min * 60 * 1000);
    sync_interval_config->publish_state(pkt->sync_interval_min);
    dirty = true;
  }

  if (vent_timer != nullptr && pkt->vent_timer_min <= 1440 &&
      (uint32_t)(pkt->vent_timer_min * 60 * 1000) !=
          v->state_machine.ventilation_duration_ms) {
    v->state_machine.ventilation_duration_ms = pkt->vent_timer_min * 60 * 1000;
    vent_timer->publish_state(pkt->vent_timer_min);
  }

  // 6. UI Settings (LED Brightness)
  if (max_led_brightness != nullptr && led_max_brightness_config != nullptr &&
      update_leds != nullptr && pkt->max_led_brightness >= 0.05f &&
      pkt->max_led_brightness <= 1.0f &&
      std::abs(pkt->max_led_brightness - max_led_brightness->value()) > 0.01f) {
    max_led_brightness->value() = pkt->max_led_brightness;
    led_max_brightness_config->publish_state(pkt->max_led_brightness * 100.0f);
    update_leds->execute();
    dirty = true;
  }

  if (dirty && auto_mode_active != nullptr && auto_mode_active->value()) {
    evaluate_auto_mode();
  }
}

inline void handle_state_sync(const esphome::VentilationPacket *pkt, bool force = false) {
  auto *v = ventilation_ctrl;
  if (v == nullptr)
    return;

  if (fan_intensity_level != nullptr && fan_intensity_display != nullptr &&
      (v->current_fan_intensity != fan_intensity_level->value() || force)) {
    fan_intensity_level->value() = v->current_fan_intensity;
    fan_intensity_display->publish_state(v->current_fan_intensity);
  }

  int new_mode_idx = -1; // -1 = Don't update unless determined
  std::string mode_str = "Wärmerückgewinnung";

  if (v->state_machine.current_mode == esphome::MODE_OFF) {
    new_mode_idx = 4;
    mode_str = "Aus";
    if (ventilation_enabled)
      ventilation_enabled->value() = false;
    if (system_on)
      system_on->value() = false;
    if (auto_mode_active)
      auto_mode_active->value() = false;
  } else {
    if (ventilation_enabled)
      ventilation_enabled->value() = true;
    if (system_on)
      system_on->value() = true;

    if (auto_mode_active != nullptr && auto_mode_active->value()) {
      new_mode_idx = 0;
      mode_str = "Automatik";
    } else if (v->state_machine.current_mode == esphome::MODE_ECO_RECOVERY) {
      new_mode_idx = 1;
      mode_str = "Wärmerückgewinnung";
    } else if (v->state_machine.current_mode == esphome::MODE_VENTILATION) {
      new_mode_idx = 2;
      mode_str = "Durchlüften";
    } else if (v->state_machine.current_mode == esphome::MODE_STOSSLUEFTUNG) {
      new_mode_idx = 3;
      mode_str = "Stoßlüftung";
    }
  }

  if (current_mode_index != nullptr && (new_mode_idx != current_mode_index->value() || force)) {
    current_mode_index->value() = new_mode_idx;
  }

  if (luefter_modus != nullptr &&
      std::string(luefter_modus->current_option()) != mode_str) {
    luefter_modus->publish_state(mode_str);
  }

  if (ui_active != nullptr && ui_timeout_script != nullptr &&
      update_leds != nullptr && fan_speed_update != nullptr) {
    ui_active->value() = true;
    ui_timeout_script->execute();
    update_leds->execute();
    fan_speed_update->execute();
  }
}
} // namespace espnow_handler

/**
 * @brief Thread safety note: ESPHome's internal architecture buffers ESP-NOW
 * events and executes lambda callbacks within the main loop() context. Global
 * variable modifications here are safe from concurrent HW interrupts.
 */
inline void handle_espnow_receive(const std::vector<uint8_t> &data) {
  if (!espnow_handler::validate_packet(data))
    return;

  const auto *pkt =
      reinterpret_cast<const esphome::VentilationPacket *>(data.data());
  auto *v = ventilation_ctrl;
  if (v == nullptr)
    return;

  if (static_cast<esphome::MessageType>(pkt->msg_type) ==
      esphome::MSG_STATUS_REQUEST) {
    espnow_handler::handle_status_request(pkt);
    return;
  }

  bool changed = v->on_packet_received(data);

  if (static_cast<esphome::MessageType>(pkt->msg_type) ==
          esphome::MSG_STATUS_RESPONSE &&
      !v->is_state_synced) {
    if (config_floor_id != nullptr && config_room_id != nullptr &&
        pkt->floor_id == (int)config_floor_id->state &&
        pkt->room_id == (int)config_room_id->state) {
      ESP_LOGI("vent_sync", "Adopting state from peer %d (REBOOT SYNC)",
               pkt->device_id);

      int mode_idx = pkt->current_mode_index;

      cycle_operating_mode(mode_idx);

      if (fan_intensity_level != nullptr)
        fan_intensity_level->value() = 0;
      if (fan_speed_update != nullptr)
        fan_speed_update->execute();

      v->set_fan_intensity(pkt->fan_intensity);
      if (fan_intensity_level != nullptr)
        fan_intensity_level->value() = pkt->fan_intensity;

      v->is_state_synced = true;
      v->pending_broadcast = false; // Mute echo to prevent spamming old state on boot
      return;
    }
  }

  if (changed) {
    espnow_handler::handle_state_sync(pkt, pkt->msg_type == esphome::MSG_STATE);
  }

  bool should_sync_config = false;
  if (pkt->msg_type == esphome::MSG_STATE) {
    should_sync_config = true;
  } else if (pkt->msg_type == esphome::MSG_SYNC && pkt->device_id == 1 && v->device_id != 1) {
    should_sync_config = true;
  }

  if (should_sync_config) {
    espnow_handler::handle_config_sync(pkt);
  }
}
