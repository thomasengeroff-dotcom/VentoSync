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
// File:        network_sync.h
// Description: ESP-NOW peer synchronization and state mirroring.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-03-29
// ==========================================================================
#pragma once
#include "globals.h"

// --- Constants ---
constexpr uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr size_t MAX_PAYLOAD_LEN = 100;

inline bool is_local_mac(const uint8_t *mac) {
  static uint8_t local_mac[6] = {0};
  static bool cached = false;
  if (!cached) {
    esp_read_mac(local_mac, ESP_MAC_WIFI_STA);
    cached = true;
    ESP_LOGI("espnow_disc", "Local MAC detected: %02X:%02X:%02X:%02X:%02X:%02X",
             local_mac[0], local_mac[1], local_mac[2], local_mac[3], local_mac[4], local_mac[5]);
  }
  return memcmp(mac, local_mac, 6) == 0;
}

/** @brief Checks if this is the only device in the room group. */
inline bool is_single_device_group() {
  return peer_cache.empty();
}

// --- Peer Cache Helpers -------------------------------------------------

/** @brief Formats a MAC address as a human-readable string. */
inline std::string format_mac(const uint8_t *mac) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return std::string(buf);
}

/** @brief Finds a peer in the binary cache by MAC address. Returns nullptr if not found. */
inline PeerEntry* find_peer_in_cache(const uint8_t *mac) {
  for (auto &entry : peer_cache) {
    if (memcmp(entry.mac.data(), mac, 6) == 0) return &entry;
  }
  return nullptr;
}

/** @brief Rebuilds the NVS-backed espnow_peers string from peer_cache and updates UI. */
inline void rebuild_peers_string() {
  if (espnow_peers == nullptr) return;
  std::string &list = espnow_peers->value();
  list.clear();
  for (size_t i = 0; i < peer_cache.size(); i++) {
    if (i > 0) list += ",";
    list += format_mac(peer_cache[i].mac.data());
  }

  // Update UI with formatted display string
  if (espnow_peers_display != nullptr) {
    if (list.empty()) {
      espnow_peers_display->publish_state("Keine Peers");
    } else {
      // Add spaces after commas for HA line wrapping
      std::string display = list;
      size_t pos = 0;
      while ((pos = display.find(",", pos)) != std::string::npos) {
          display.insert(pos + 1, " ");
          pos += 2;
      }
      espnow_peers_display->publish_state(display);
    }
  }
}

/** @brief Removes a stale peer from peer_cache, NVS string, and ESP-NOW peer list. */
inline void remove_stale_peer(const uint8_t *mac) {
  std::string mac_str = format_mac(mac);

  // Remove from ESP-NOW hardware peer list
  esphome::espnow::global_esp_now->del_peer(mac);

  // Remove from binary cache
  peer_cache.erase(
    std::remove_if(peer_cache.begin(), peer_cache.end(),
      [mac](const PeerEntry &e) { return memcmp(e.mac.data(), mac, 6) == 0; }),
    peer_cache.end());

  // Rebuild the NVS string from the authoritative cache
  rebuild_peers_string();

  ESP_LOGW("espnow_recovery", "Stale peer %s removed (cache: %d peers remaining)",
           mac_str.c_str(), peer_cache.size());
}

/** @brief Triggers a re-discovery broadcast with 30s throttle. */
inline void trigger_re_discovery() {
  static uint32_t last_re_discovery = 0;
  uint32_t now = millis();
  if (last_re_discovery != 0 && now - last_re_discovery < 30000) {
    ESP_LOGD("espnow_recovery", "Re-discovery throttled (cooldown active)");
    return;
  }
  last_re_discovery = now;
  ESP_LOGI("espnow_recovery", "Triggering re-discovery after peer loss...");
  send_discovery_broadcast();
  request_peer_status();
}

/** @brief Resets the fail counter for a peer on successful communication. */
inline void reset_peer_fail_count(const uint8_t *mac) {
  PeerEntry *entry = find_peer_in_cache(mac);
  if (entry != nullptr && entry->fail_count > 0) {
    ESP_LOGD("espnow_recovery", "Peer %s recovered (was %d failures)",
             format_mac(mac).c_str(), entry->fail_count);
    entry->fail_count = 0;
    entry->last_seen = millis();
  } else if (entry != nullptr) {
    entry->last_seen = millis();
  }
}

/** @brief Registers a MAC address as an ESP-NOW peer, persists to NVS, and adds to binary cache. */
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

  std::string mac_str = format_mac(mac);

  // Persist to NVS string
  std::string &current_list = espnow_peers->value();
  if (current_list.find(mac_str) == std::string::npos) {
    if (!current_list.empty())
      current_list += ",";
    current_list += mac_str;
    ESP_LOGI("espnow_disc", "New peer discovered and saved: %s",
             mac_str.c_str());
  }

  // 1. Add to binary peer cache (if not already present)
  if (find_peer_in_cache(mac) == nullptr) {
    PeerEntry entry{};
    std::copy(mac, mac + 6, entry.mac.begin());
    entry.last_seen = millis();
    entry.fail_count = 0;
    peer_cache.push_back(entry);
    
    // Save to NVS string for persistence
    rebuild_peers_string();
    ESP_LOGI("espnow_disc", "New peer added to cache: %s", mac_str.c_str());
  }

  // 2. Register hardware peer (ESPHome)
  // ESPHome properly manages the wifi_channel internally.
  esp_err_t res = esphome::espnow::global_esp_now->add_peer(mac);
  if (res == ESP_OK) {
    ESP_LOGI("espnow_disc", "Peer %s registered via ESPHome", mac_str.c_str());
  } else {
    ESP_LOGE("espnow_disc", "Peer %s registration FAILED (err=%d)", mac_str.c_str(), res);
  }
}

/** @brief Loads all peers saved in the runtime string cache and populates peer_cache. */
inline void load_peers_from_runtime_cache() {
  if (!esphome::espnow::global_esp_now || espnow_peers == nullptr)
    return;
  std::string current_list = espnow_peers->value();
  if (current_list.empty()) {
    if (espnow_peers_display != nullptr) espnow_peers_display->publish_state("Keine Peers");
    return;
  }

  // Clear runtime cache before rebuilding from NVS
  peer_cache.clear();

  size_t start = 0;
  size_t end = current_list.find(",");
  while (true) {
    std::string mac_str = (end == std::string::npos)
                              ? current_list.substr(start)
                              : current_list.substr(start, end - start);
    auto mac = parse_mac_local(mac_str);
    if (mac.has_value()) {
      // 1. Register with hardware (ESPHome)
      esphome::espnow::global_esp_now->add_peer(mac->data());

      // 2. Populate binary cache
      if (find_peer_in_cache(mac->data()) == nullptr) {
        PeerEntry entry{};
        entry.mac = mac.value();
        entry.last_seen = millis();
        entry.fail_count = 0;
        peer_cache.push_back(entry);
      }
      ESP_LOGI("espnow_disc", "Restored peer from flash: %s", mac_str.c_str());
    }
    if (end == std::string::npos)
      break;
    start = end + 1;
    end = current_list.find(",", start);
  }
  ESP_LOGI("espnow_disc", "Peer cache loaded: %d peers from NVS", peer_cache.size());
  
  // Ensure the Home Assistant dashboard sensor is freshly populated with what we loaded
  rebuild_peers_string();
}

/** @brief Sends a discovery broadcast to identify peers in the same room. */
inline void send_discovery_broadcast() {
  if (!esphome::espnow::global_esp_now || 
      !config_floor_id || !config_room_id) return;

  // ✅ NaN-Check VOR dem Cast (UB-Prevention)
  if (std::isnan(config_floor_id->state) || 
      std::isnan(config_room_id->state)) {
    ESP_LOGD("espnow_disc", "IDs are NaN — deferring broadcast");
    return;
  }

  uint8_t floor = (uint8_t)config_floor_id->state;
  uint8_t room = (uint8_t)config_room_id->state;

  if (floor == 0 || room == 0) {
    ESP_LOGD("espnow_disc", 
             "IDs are default (0:0) — skipping broadcast");
    return;
  }

  // LOG CHANNEL for coexistence diagnostics (Essential for ESP32-C6)
  uint8_t primary_chan = esphome::espnow::global_esp_now->get_wifi_channel();

  char buffer[64];
  int written =
      snprintf(buffer, sizeof(buffer), "ROOM_DISC:%d:%d", floor, room);
  if (written < 0 || written >= sizeof(buffer)) {
    ESP_LOGE("espnow_disc", "Buffer overflow prevented in discovery message");
    return;
  }

  std::string msg(buffer);
  std::vector<uint8_t> data(msg.begin(), msg.end());
  esphome::espnow::global_esp_now->send(BROADCAST_MAC, data,
                                        [msg, primary_chan](esp_err_t err) {
                                          if (err == ESP_OK) {
                                            ESP_LOGI("espnow_disc", "Sent discovery broadcast: %s (Channel: %d)", msg.c_str(), primary_chan);
                                          } else {
                                            ESP_LOGE("espnow_disc", "Discovery broadcast SEND FAILED! (Err: %d, Channel: %d)", err, primary_chan);
                                          }
                                        });
}

/** @brief Requests current status from all known peers.
 *  Uses unicast when peers are known, falls back to broadcast otherwise. */
inline void request_peer_status() {
  if (!esphome::espnow::global_esp_now)
    return;

  auto data = build_and_populate_packet(esphome::MSG_STATUS_REQUEST);
  if (peer_cache.empty()) {
    // No known peers — broadcast required
    ESP_LOGI("vent_sync", "No known peers, broadcasting status request...");
    esphome::espnow::global_esp_now->send(BROADCAST_MAC, data,
                                          [](esp_err_t err) {});
  } else {
    // Known peers — unicast (more reliable due to HW-ACK)
    ESP_LOGI("vent_sync", "Requesting status via UNICAST from %d peer(s)...",
             peer_cache.size());
    send_sync_to_all_peers(data);
  }
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

/** @brief Sends a sync packet to all registered peers via unicast with ACK tracking. */
inline void send_sync_to_all_peers(const std::vector<uint8_t> &data) {
  if (!esphome::espnow::global_esp_now || peer_cache.empty())
    return;

  // ✅ Snapshot der MACs erstellen, um Iterator-Invalidierung zu vermeiden
  std::vector<std::array<uint8_t, 6>> mac_snapshot;
  mac_snapshot.reserve(peer_cache.size());
  for (const auto &entry : peer_cache) {
    mac_snapshot.push_back(entry.mac);
  }

  for (const auto &peer_mac : mac_snapshot) {
    esphome::espnow::global_esp_now->send(
        peer_mac.data(), data, [peer_mac](esp_err_t err) {
          PeerEntry *p = find_peer_in_cache(peer_mac.data());
          if (p == nullptr)
            return; // Peer was already removed

          if (err == ESP_OK) {
            // Success: reset fail counter, update timestamp
            if (p->fail_count > 0) {
              ESP_LOGI("espnow_ack",
                       "Peer %s recovered after %d failures",
                       format_mac(peer_mac.data()).c_str(), p->fail_count);
              p->fail_count = 0;
            }
            p->last_seen = millis();
          } else {
            // Failure: increment counter
            p->fail_count++;
            ESP_LOGW("espnow_ack",
                     "Send to %s failed (err=%d, failures: %d/%d)",
                     format_mac(peer_mac.data()).c_str(), err, p->fail_count,
                     MAX_PEER_SEND_FAILURES);

            if (p->fail_count >= MAX_PEER_SEND_FAILURES) {
              ESP_LOGE("espnow_ack",
                       "Peer %s exceeded max failures — removing",
                       format_mac(peer_mac.data()).c_str());
              remove_stale_peer(peer_mac.data());
              trigger_re_discovery();
            }
          }
        });
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
    if (is_local_mac(src_addr)) {
      ESP_LOGD("espnow_disc", "Ignoring own discovery loopback");
      return true;
    }

    int floor, room;
    if (sscanf(payload.c_str() + 10, "%d:%d", &floor, &room) == 2) {
      if (config_floor_id == nullptr || config_room_id == nullptr ||
          std::isnan(config_floor_id->state) ||
          std::isnan(config_room_id->state)) {
        ESP_LOGW("espnow_disc", "Config IDs not ready (null or NaN), ignoring discovery (Payload: %s)", payload.c_str());
        return false;
      }
      
      int local_floor = (int)config_floor_id->state;
      int local_room = (int)config_room_id->state;
      
      ESP_LOGI("espnow_disc", "Discovery match check: [Payload %d:%d vs Local %d:%d]", floor, room, local_floor, local_room);

      if (floor == local_floor && room == local_room) {
        ESP_LOGW("espnow_disc", "✅ ALL CRITERIA MET - Registering peer: %s",
                 format_mac(src_addr).c_str());
        register_peer_dynamic(src_addr);
        if (payload.find("ROOM_DISC:") == 0) {
          send_discovery_confirmation(src_addr);
        }
        return true;
      } else {
        ESP_LOGI("espnow_disc", "❌ Group mismatch - ignoring.");
      }
    } else {
      ESP_LOGW("espnow_disc", "Failed to parse discovery payload format: %s", payload.c_str());
    }
  }
  return false;
}

/** @brief Sends a sync packet to all registered peers via UNICAST. */
inline void sync_settings_to_peers() {
  if (ventilation_ctrl == nullptr || !esphome::espnow::global_esp_now) return;
  
  // Rate-limit beibehalten (max 1 per 500ms)
  static uint32_t last_settings_sync = 0;
  if (millis() - last_settings_sync < 500) {
    ESP_LOGD("vent_sync", "Settings sync suppressed (rate-limit active)");
    return;
  }
  last_settings_sync = millis();

  auto data = build_and_populate_packet(esphome::MSG_STATE);
  send_sync_to_all_peers(data);
  ESP_LOGI("vent_sync", "Sent state change via UNICAST to %d peer(s)", peer_cache.size());
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

inline void handle_status_request(const esphome::VentilationPacket *pkt, const uint8_t *src_mac) {
  if (config_floor_id != nullptr && config_room_id != nullptr &&
      pkt->floor_id == (int)config_floor_id->state &&
      pkt->room_id == (int)config_room_id->state) {
    // FIX: Register the requester as a peer immediately
    register_peer_dynamic(src_mac);

    ESP_LOGI("vent_sync", "Status request from peer %d. Sending UNICAST response...",
             pkt->device_id);
    auto resp = build_and_populate_packet(esphome::MSG_STATUS_RESPONSE);
    if (esphome::espnow::global_esp_now) {
      esphome::espnow::global_esp_now->send(src_mac, resp,
                                            [](esp_err_t err) {});
    }
  }
}

inline void handle_config_sync(const esphome::VentilationPacket *pkt) {
  bool dirty = false;
  auto *v = ventilation_ctrl;

  // 1. Automatik Min/Max Levels (Sliders)
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

  // Issue #3 FIX: Propagate auto_mode_active from the sender's mode index.
  // Devices in the same room must always share the same operating mode.
  // current_mode_index == 0 means "Automatik" on the sender.
  bool sender_in_auto = (pkt->current_mode_index == 0);
  if (auto_mode_active != nullptr && auto_mode_active->value() != sender_in_auto) {
    auto_mode_active->value() = sender_in_auto;
    ESP_LOGI("vent_sync", "Synced auto_mode_active to %s (from peer mode_index %d)",
             sender_in_auto ? "ON" : "OFF", pkt->current_mode_index);
  }

  // Issue #4 FIX: Only override fan intensity from peer in manual modes.
  // In Automatik mode, the local PID controller is the authority for intensity.
  bool is_auto = (auto_mode_active != nullptr && auto_mode_active->value());
  if (!is_auto && fan_intensity_level != nullptr && 
      fan_intensity_display != nullptr &&
      (v->current_fan_intensity != pkt->fan_intensity || force)) {
    v->set_fan_intensity(pkt->fan_intensity, false);  // ← Aktualisiert PWM
    fan_intensity_level->value() = pkt->fan_intensity;
    fan_intensity_display->publish_state(pkt->fan_intensity);
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
inline void handle_espnow_receive(const std::vector<uint8_t> &data, const uint8_t *src_mac) {
  // ✅ Deduplication guard (prevent double-processing within 50ms)
  static uint32_t last_rx_time = 0;
  static uint32_t last_rx_hash = 0;
  uint32_t now = millis();
  
  uint32_t hash = (uint32_t)data.size();
  for (size_t i = 0; i < std::min(data.size(), (size_t)8); i++)
    hash = hash * 31 + data[i];
  // ✅ FIX: Alle 6 MAC-Bytes einbeziehen statt nur 2
  for (size_t i = 0; i < 6; i++)
    hash = hash * 31 + src_mac[i];

  if (hash == last_rx_hash && (now - last_rx_time) < 50) {
    return; // Duplicate suppressed (likely from overlapping broadcast/unknown_peer lambdas)
  }
  last_rx_hash = hash;
  last_rx_time = now;

  // --- START DEBUG LOGGING ---
  if (!data.empty()) {
    ESP_LOGD("espnow_raw", "RX from %s | Len: %d | First: 0x%02X", 
             format_mac(src_mac).c_str(), data.size(), data[0]);
  }
  // --- END DEBUG LOGGING ---

  // ✅ Discovery-Strings VOR validate_packet() abfangen
  // Discovery-Pakete sind Plaintext, keine VentilationPacket-Strukturen
  if (!data.empty()) {
    std::string payload(data.begin(), data.end());
    if (payload.find("ROOM_DISC:") == 0 || payload.find("ROOM_CONF:") == 0) {
      handle_discovery_payload(payload, src_mac);
      return; // Nicht weiter als VentilationPacket verarbeiten
    }
  }

  if (!espnow_handler::validate_packet(data)) {
    // Only log error for non-discovery packets that fail validation
    if (!data.empty() && data[0] == 0x42) {
       ESP_LOGW("vent_sync", "Packet validation failed for message from %s",
                format_mac(src_mac).c_str());
    }
    return;
  }

  // Successful packet reception — reset fail counter for this peer
  reset_peer_fail_count(src_mac);

  const auto *pkt =
      reinterpret_cast<const esphome::VentilationPacket *>(data.data());
  auto *v = ventilation_ctrl;
  if (v == nullptr)
    return;

  if (static_cast<esphome::MessageType>(pkt->msg_type) ==
      esphome::MSG_STATUS_REQUEST) {
    espnow_handler::handle_status_request(pkt, src_mac);
    return;
  }

  bool changed = v->on_packet_received(data);

  if (static_cast<esphome::MessageType>(pkt->msg_type) ==
          esphome::MSG_STATUS_RESPONSE &&
      !v->is_state_synced) {
    if (config_floor_id != nullptr && config_room_id != nullptr &&
        pkt->floor_id == (int)config_floor_id->state &&
        pkt->room_id == (int)config_room_id->state) {
      // Prefer Master's response; accept non-Master only as fallback
      if (!is_from_master(pkt)) {
        ESP_LOGI("vent_sync", "Non-master peer %d responded. Accepting as fallback (master may be offline).",
                 pkt->device_id);
      }
      ESP_LOGI("vent_sync", "Adopting state from peer %d (REBOOT SYNC%s)",
               pkt->device_id, is_from_master(pkt) ? " - MASTER" : " - FALLBACK");
      
      // FIX: Register the peer we just synced from
      register_peer_dynamic(src_mac);

      int mode_idx = pkt->current_mode_index;

      cycle_operating_mode(mode_idx);

      if (fan_intensity_level != nullptr)
        fan_intensity_level->value() = 0;
      if (fan_speed_update != nullptr)
        fan_speed_update->execute();

      // FIXED: Pass notify=false to avoid eco-sync loops during boot status adoption
      v->set_fan_intensity(pkt->fan_intensity, false);
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
  } else if (pkt->msg_type == esphome::MSG_SYNC && is_from_master(pkt) && !is_master()) {
    should_sync_config = true;
  }

  if (should_sync_config) {
    espnow_handler::handle_config_sync(pkt);
  }
}
