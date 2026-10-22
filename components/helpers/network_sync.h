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
// File:        network_sync.h
// Description: ESP-NOW peer synchronization and state mirroring.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-10-22
// ==========================================================================
#pragma once
#include "globals.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <optional>
#include <queue>
#include <string_view>

// =========================================================
// SECTION: MAC & Peer Helpers
// =========================================================

constexpr uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr size_t MAX_PAYLOAD_LEN = 100;

/// Maximum depth for the peer_event_queue. Prevents unbounded growth
/// when a peer is offline and SEND_FAIL events accumulate.
constexpr size_t PEER_EVENT_QUEUE_MAX_DEPTH = 64;

/**
 * @brief   Safely reads the local floor and room IDs with NaN protection.
 *
 * @details Centralises the NaN→int cast guard so every call site that
 *          needs floor/room IDs goes through a single checked path.
 *          Returns std::nullopt if either config pointer is null or
 *          the underlying float state is NaN.
 *
 * @return  std::optional<std::pair<int,int>>  {floor_id, room_id} or nullopt.
 */
inline std::optional<std::pair<int,int>> get_local_room_ids() {
  if (config_floor_id == nullptr || config_room_id == nullptr) return std::nullopt;
  if (std::isnan(config_floor_id->state) || std::isnan(config_room_id->state)) return std::nullopt;
  return std::make_pair(static_cast<int>(config_floor_id->state),
                        static_cast<int>(config_room_id->state));
}

/**
 * @brief   This device's STA MAC — the sender address peers see for our
 *          ESP-NOW frames (also bound into the frame authentication tag).
 *
 * @details Reads the local STA MAC exactly once using std::call_once, which
 *          provides a guaranteed-single, thread-safe initialisation even when
 *          called concurrently from the WiFi task and the main loop.
 *          (K-2 Fix: replaced non-atomic `cached` bool with std::call_once)
 */
inline const uint8_t *local_sta_mac() {
  static uint8_t local_mac[6] = {0};
  static std::once_flag init_flag;
  std::call_once(init_flag, []() {
    esp_read_mac(local_mac, ESP_MAC_WIFI_STA);
    ESP_LOGI("espnow_disc", "Local MAC detected: %02X:%02X:%02X:%02X:%02X:%02X",
             local_mac[0], local_mac[1], local_mac[2], local_mac[3], local_mac[4], local_mac[5]);
  });
  return local_mac;
}

/** @brief True if `mac` is this device's STA MAC (see local_sta_mac()). */
inline bool is_local_mac(const uint8_t *mac) {
  return memcmp(mac, local_sta_mac(), 6) == 0;
}

// --- Peer Cache Helpers -------------------------------------------------

/**
 * @brief   Formats a MAC address as a human-readable string.
 * @param[in] mac  Pointer to a 6-byte MAC address.
 * @return  std::string in format "AA:BB:CC:DD:EE:FF".
 */
inline std::string format_mac(const uint8_t *mac) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return std::string(buf);
}

/**
 * @brief   Locates a peer in the runtime cache.
 *
 * @details Performs a linear scan over the vector. While O(n), this is
 *          negligible given the small number of peers (typically 2–4).
 *
 * @param[in] mac  Pointer to a 6-byte MAC address.
 *
 * @return  PeerEntry*  Pointer to the entry or nullptr if not found.
 */
inline PeerEntry* find_peer_in_cache(const uint8_t *mac) {
  for (auto &entry : peer_cache) {
    if (memcmp(entry.mac.data(), mac, 6) == 0) return &entry;
  }
  return nullptr;
}

/// Minimum interval between NVS (flash) writes of the peer list. The
/// in-RAM peer_cache and the UI display string are always updated
/// immediately on every topology change; only the persisted espnow_peers
/// string is throttled, so a flapping peer (repeatedly hitting
/// MAX_PEER_SEND_FAILURES and getting rediscovered) cannot cause unbounded
/// flash writes. Mirrors the flash-wear-protection pattern already used
/// for filter_operating_hours (see system_lifecycle.h).
constexpr uint32_t PEER_NVS_SAVE_MIN_INTERVAL_MS = 60000; // 60s

/// True if peer_cache changed since the last successful NVS write and the
/// write is still waiting for the throttle window to elapse.
inline bool peer_nvs_write_pending = false;
/// millis() of the last successful NVS write of the peer list (0 = never).
inline uint32_t last_peer_nvs_save_ms = 0;

/**
 * @brief Rebuilds the peer display string from peer_cache and refreshes the UI.
 *
 * @details Updates the HA diagnostic text sensor immediately and unconditionally
 *          from the authoritative in-RAM peer_cache. The actual flash (NVS)
 *          write of the persisted espnow_peers string is rate-limited to at
 *          most once per PEER_NVS_SAVE_MIN_INTERVAL_MS — if called again
 *          within the window, the write is deferred (peer_nvs_write_pending
 *          is set) and completed later by flush_pending_peer_nvs_save().
 *
 * @note  (H-2 note) Builds into a local string first, then assigns via std::move.
 *        This avoids any ambiguity about mutating the underlying storage of
 *        espnow_peers->value() while holding a reference to it.
 */
inline void rebuild_peers_string() {
  if (espnow_peers == nullptr) return;

  // Build into a local buffer from the authoritative in-RAM cache.
  std::string new_list;
  new_list.reserve(peer_cache.size() * 18); // "XX:XX:XX:XX:XX:XX,"
  for (size_t i = 0; i < peer_cache.size(); i++) {
    if (i > 0) new_list += ',';
    new_list += format_mac(peer_cache[i].mac.data());
  }

  // Update UI with formatted display string — always immediate, never
  // throttled, so the dashboard reflects peer_cache in real time.
  if (espnow_peers_display != nullptr) {
    if (new_list.empty()) {
      espnow_peers_display->publish_state("Keine Peers");
    } else {
      // Add spaces after commas for HA line wrapping
      std::string display = new_list;
      size_t pos = 0;
      while ((pos = display.find(',', pos)) != std::string::npos) {
          display.insert(pos + 1, " ");
          pos += 2;
      }
      espnow_peers_display->publish_state(display);
    }
  }

  // Nothing to persist if the string is unchanged from what's already stored.
  if (new_list == espnow_peers->value()) {
    peer_nvs_write_pending = false;
    return;
  }

  const uint32_t now = millis();
  if (last_peer_nvs_save_ms != 0 &&
      now - last_peer_nvs_save_ms < PEER_NVS_SAVE_MIN_INTERVAL_MS) {
    // Within the throttle window — defer the flash write. peer_cache (RAM)
    // and the UI are already current; flush_pending_peer_nvs_save() persists
    // this once the window elapses (called from peer_presence_watchdog()).
    peer_nvs_write_pending = true;
    ESP_LOGD("espnow_disc", "Peer NVS write deferred (throttle window active)");
    return;
  }

  espnow_peers->value() = std::move(new_list);
  last_peer_nvs_save_ms = now;
  peer_nvs_write_pending = false;
}

/**
 * @brief Flushes a deferred peer-list NVS write once the throttle window has elapsed.
 *
 * @details Called periodically (see peer_presence_watchdog() in
 *          espnow_helpers.h) to guarantee that a write deferred by
 *          rebuild_peers_string() during a burst of topology changes is
 *          eventually persisted, rather than being silently dropped.
 */
inline void flush_pending_peer_nvs_save() {
  if (!peer_nvs_write_pending) return;
  // Re-run the throttled rebuild; it will persist now if the window has
  // elapsed, or keep deferring otherwise.
  rebuild_peers_string();
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

  ESP_LOGW("espnow_recovery", "Stale peer %s removed (cache: %zu peers remaining)",
           mac_str.c_str(), peer_cache.size());
}


// =========================================================
// SECTION: Frame Authentication (protocol v13, espnow_auth.h)
// =========================================================

namespace espnow_auth_state {
  /// Newest accepted (epoch, seq) per sender — rejects replayed frames.
  inline ventosync::espnow_auth::ReplayGuard replay_guard;
  /// Own frame counter: epoch persisted once per boot, seq per frame.
  inline ventosync::espnow_auth::Counter tx{};
  inline bool tx_ready = false;
  /// Dropped frames since the last summary log (rate-limited warning).
  inline uint32_t dropped = 0;
  inline uint32_t last_drop_log_ms = 0;
  inline bool missing_key_logged = false;
  /// HMAC known-answer test: 0 = not run, 1 = passed, -1 = failed.
  inline int8_t self_test_result = 0;
  constexpr uint32_t DROP_LOG_INTERVAL_MS = 60000;
}

/**
 * @brief   True if the room key is configured (espnow_key) and the HMAC
 *          implementation passed its known-answer test; fail closed otherwise.
 */
inline bool espnow_auth_key_ready() {
  if (espnow_auth_state::self_test_result == 0) {
    const bool ok = ventosync::espnow_auth::self_test();
    espnow_auth_state::self_test_result = ok ? 1 : -1;
    if (!ok) ESP_LOGE("espnow_auth", "HMAC-SHA256 self-test FAILED — ESP-NOW disabled");
  }
  if (espnow_auth_state::self_test_result < 0) return false;
  if (ventilation_ctrl != nullptr && ventilation_ctrl->espnow_auth_key_set) return true;
  if (!espnow_auth_state::missing_key_logged) {
    ESP_LOGE("espnow_auth", "No ESP-NOW room key configured (espnow_key) — ESP-NOW disabled");
    espnow_auth_state::missing_key_logged = true;
  }
  return false;
}

/**
 * @brief   Starts this boot's frame counter: increments the persisted epoch.
 *
 * @details Written to flash immediately (one write per boot), so the epoch of
 *          the next boot is always larger and receivers accept it (seq
 *          restarts at 0). If the write fails, the epoch can repeat after a
 *          reboot; peers then reject this device until its frames are newer
 *          again or the replay entry expired (15 min).
 */
inline void espnow_auth_start_tx() {
  if (espnow_auth_state::tx_ready) return;
  auto pref = esphome::global_preferences->make_preference<uint32_t>(
      esphome::fnv1_hash("ventosync_espnow_tx_epoch"), true);
  uint32_t epoch = 0;
  if (!pref.load(&epoch)) epoch = 0;
  epoch++;
  if (!pref.save(&epoch) || !esphome::global_preferences->sync()) {
    ESP_LOGE("espnow_auth", "Could not persist the ESP-NOW frame epoch %u", static_cast<unsigned>(epoch));
  }
  espnow_auth_state::tx.epoch = epoch;
  espnow_auth_state::tx.seq = 0;
  espnow_auth_state::tx_ready = true;
  ESP_LOGI("espnow_auth", "Frame authentication active (epoch %u)", static_cast<unsigned>(epoch));
}

/**
 * @brief   Appends the authentication trailer to an outgoing frame.
 * @return  false if no room key is configured (the frame must not be sent).
 */
inline bool espnow_seal_frame(std::vector<uint8_t> &frame) {
  if (!espnow_auth_key_ready()) return false;
  espnow_auth_start_tx();
  espnow_auth_state::tx.seq++;
  ventosync::espnow_auth::seal(ventilation_ctrl->espnow_auth_key, local_sta_mac(), frame,
                               espnow_auth_state::tx);
  return true;
}

/**
 * @brief   Verifies a received frame (tag + replay) and strips the trailer.
 *
 * @details Every rejected frame is counted; a summary warning is logged at
 *          most once per minute, so a neighbour's devices with another key —
 *          or an attacker — cannot flood the log.
 *
 * @param[out] payload  The authenticated payload (without trailer).
 * @return  true if the frame is authentic and newer than the last one of
 *          this sender.
 */
inline bool espnow_open_frame(const std::vector<uint8_t> &frame, const uint8_t *src_mac,
                              std::vector<uint8_t> &payload) {
  namespace ea = ventosync::espnow_auth;
  if (!espnow_auth_key_ready()) return false;
  const uint32_t now = millis();
  // Replay memory must outlive two heartbeats of the configured interval.
  espnow_auth_state::replay_guard.set_idle_expiry_ms(
      ventosync::espnow_auth::replay_expiry_ms(ventilation_ctrl->sync_interval_ms));
  size_t payload_len = 0;
  ea::Counter ctr;
  const ea::Verdict verdict =
      ea::verify(ventilation_ctrl->espnow_auth_key, src_mac, frame.data(), frame.size(), payload_len, ctr);
  const char *reason = nullptr;
  if (verdict == ea::Verdict::TOO_SHORT) {
    reason = "unsigned (firmware < 0.10.43?)";
  } else if (verdict == ea::Verdict::BAD_TAG) {
    reason = "bad tag (other espnow_key, firmware < 0.10.43 or forged)";
  } else if (!espnow_auth_state::replay_guard.accept(src_mac, ctr, now)) {
    reason = "replayed / outdated counter";
  }
  if (reason != nullptr) {
    espnow_auth_state::dropped++;
    if (espnow_auth_state::last_drop_log_ms == 0 ||
        now - espnow_auth_state::last_drop_log_ms >= espnow_auth_state::DROP_LOG_INTERVAL_MS) {
      ESP_LOGW("espnow_auth", "Dropped %u unauthenticated ESP-NOW frame(s); last from %s: %s",
               static_cast<unsigned>(espnow_auth_state::dropped), format_mac(src_mac).c_str(), reason);
      espnow_auth_state::dropped = 0;
      espnow_auth_state::last_drop_log_ms = now;
    }
    return false;
  }
  payload.assign(frame.begin(), frame.begin() + static_cast<std::ptrdiff_t>(payload_len));
  return true;
}

/**
 * @brief   Seals and sends one ESP-NOW frame (the only send path, so every
 *          frame — unicast, broadcast, discovery — is authenticated).
 */
inline void espnow_send_frame(const uint8_t *dest, std::vector<uint8_t> frame,
                              const esphome::espnow::send_callback_t &callback = nullptr) {
  if (!esphome::espnow::global_esp_now || !espnow_seal_frame(frame)) return;
  esphome::espnow::global_esp_now->send(dest, frame, callback);
}

// =========================================================
// SECTION: Discovery & Registration
// =========================================================

/**
 * @brief   Triggers an active peer discovery phase.
 *
 * @details Sends a ROOM_DISC broadcast to find peers on the same floor/room.
 *          Includes a 30s throttle to prevent "Discovery Storms" which could
 *          congest the 2.4GHz band.
 */
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

/**
 * @brief   Resets the fail counter for a peer on successful communication.
 * @param[in] mac  Pointer to the 6-byte MAC address.
 */
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

/**
 * @brief   Registers a new peer in the system.
 *
 * @details Adds the peer to the ESPHome lower-level driver, populates the
 *          runtime binary cache, and persists the MAC to NVS memory.
 *          This ensures peers are remembered across reboots.
 *
 * @param[in] mac  Pointer to the 6-byte MAC of the new peer.
 *
 * @note    Silently ignores our own MAC and prevents duplicate entries.
 */
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

  // (H-1 Fix) peer_cache is the single source of truth.
  // NVS string is ONLY derived from peer_cache via rebuild_peers_string().
  // Direct NVS string manipulation is removed to prevent divergence between
  // the two representations.

  // 1. Add to binary peer cache (if not already present)
  if (find_peer_in_cache(mac) == nullptr) {
    // Enforce peer-count cap to stay within NVS string limits (~254 chars)
    // and match the LRU cap in VentilationController (H-5).
    if (peer_cache.size() >= MAX_PEERS) {
      ESP_LOGW("espnow_disc", "Peer cache full (%zu/%zu), rejecting %s",
               peer_cache.size(), MAX_PEERS, mac_str.c_str());
      return;
    }

    PeerEntry entry{};
    std::copy(mac, mac + 6, entry.mac.begin());
    entry.last_seen = millis();
    entry.fail_count = 0;
    peer_cache.push_back(entry);

    // Rebuild NVS string from the authoritative cache (single path)
    rebuild_peers_string();
    ESP_LOGI("espnow_disc", "New peer added to cache and NVS: %s", mac_str.c_str());
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
  ESP_LOGI("espnow_disc", "Peer cache loaded: %zu peers from NVS", peer_cache.size());
  
  // Ensure the Home Assistant dashboard sensor is freshly populated with what we loaded
  rebuild_peers_string();
}

// =========================================================
// SECTION: Synchronization Logic
// =========================================================

/// Rate-limit state of sync_settings_to_peers().
namespace settings_sync {
  inline uint32_t last_ms = 0;     ///< millis() of the last MSG_STATE.
  inline bool deferred = false;    ///< A MSG_STATE was rate-limited and is still owed.
  constexpr uint32_t MIN_INTERVAL_MS = 500;
}

/**
 * @brief   Sends a state update to all registered peers via Unicast.
 *
 * @details Unicast is prioritized over broadcast because it uses hardware
 *          Acknowledgement (ACK), allowing the system to detect and recover
 *          from packet loss or stale peer entries.
 */
inline void sync_settings_to_peers() {
  if (ventilation_ctrl == nullptr || !esphome::espnow::global_esp_now) return;
  
  // Rate-limit: max 1 sync per 500ms. A suppressed sync is sent later by
  // flush_deferred_settings_sync(), so the LAST change (e.g. fast Level
  // clicks) always reaches the room — otherwise the Master heartbeat would
  // push an older level / override back.
  const uint32_t now = millis();
  if (settings_sync::last_ms != 0 && now - settings_sync::last_ms < settings_sync::MIN_INTERVAL_MS) {
    settings_sync::deferred = true;
    ESP_LOGD("vent_sync", "Settings sync deferred (rate-limit active)");
    return;
  }
  settings_sync::last_ms = now;
  settings_sync::deferred = false;

  auto data = build_and_populate_packet(esphome::MSG_STATE);
  send_sync_to_all_peers(data);
  ESP_LOGI("vent_sync", "Sent state change via UNICAST to %zu peer(s)", peer_cache.size());
}

/**
 * @brief   Sends a MSG_STATE that sync_settings_to_peers() had to defer.
 *
 * @note    Called every second (logic_automation.yaml).
 */
inline void flush_deferred_settings_sync() {
  if (!settings_sync::deferred) return;
  if (millis() - settings_sync::last_ms < settings_sync::MIN_INTERVAL_MS) return;
  sync_settings_to_peers();
}

/** @brief Sends a discovery broadcast to identify peers in the same room. */
inline void send_discovery_broadcast() {
  if (!esphome::espnow::global_esp_now || 
      !config_floor_id || !config_room_id) return;

  // NaN-Check before cast (UB-Prevention)
  auto ids = get_local_room_ids();
  if (!ids) {
    ESP_LOGD("espnow_disc", "IDs are NaN — deferring broadcast");
    return;
  }

  uint8_t floor = static_cast<uint8_t>(ids->first);
  uint8_t room = static_cast<uint8_t>(ids->second);

  if (floor == 0 || room == 0) {
    ESP_LOGD("espnow_disc", 
             "IDs are default (0:0) — skipping broadcast");
    return;
  }

  // Log channel for coexistence diagnostics (Essential for ESP32-C6)
  uint8_t primary_chan = esphome::espnow::global_esp_now->get_wifi_channel();

  char buffer[64];
  int written =
      snprintf(buffer, sizeof(buffer), "ROOM_DISC:%d:%d", floor, room);
  if (written < 0 || written >= static_cast<int>(sizeof(buffer))) {
    ESP_LOGE("espnow_disc", "Buffer overflow prevented in discovery message");
    return;
  }

  std::string msg(buffer);
  std::vector<uint8_t> data(msg.begin(), msg.end());
  espnow_send_frame(BROADCAST_MAC, std::move(data), [msg, primary_chan](esp_err_t err) {
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
    espnow_send_frame(BROADCAST_MAC, std::move(data));
  } else {
    // Known peers — unicast (more reliable due to HW-ACK)
    ESP_LOGI("vent_sync", "Requesting status via UNICAST from %zu peer(s)...",
             peer_cache.size());
    send_sync_to_all_peers(data);
  }
}

/** @brief Sends a unicast confirmation to a discovered peer. */
inline void send_discovery_confirmation(const uint8_t *target_mac) {
  if (!esphome::espnow::global_esp_now) return;

  auto ids = get_local_room_ids();
  if (!ids) {
    ESP_LOGD("espnow_disc", "IDs not ready (null or NaN), deferring confirmation");
    return;
  }

  char buffer[64];
  int written =
      snprintf(buffer, sizeof(buffer), "ROOM_CONF:%d:%d",
               ids->first, ids->second);
  if (written < 0 || written >= static_cast<int>(sizeof(buffer))) {
    ESP_LOGE("espnow_disc",
             "Buffer overflow prevented in confirmation message");
    return;
  }

  std::string msg(buffer);
  std::vector<uint8_t> data(msg.begin(), msg.end());
  espnow_send_frame(target_mac, std::move(data));
}

/**
 * @brief   Processes deferred send-ACK events from the peer_event_queue.
 *
 * @details (K-1 Fix) The WiFi-task send callback MUST NOT mutate peer_cache
 *          directly (Data Race). Instead it enqueues a PeerEvent. This
 *          function drains peer_event_queue in the main-loop context where
 *          it is safe to mutate peer_cache, call remove_stale_peer(), etc.
 *          Call this from process_queued_packets() once per loop iteration.
 */
inline void process_peer_events() {
  std::queue<PeerEvent> local;
  {
    std::lock_guard<std::mutex> lock(peer_event_mutex);
    if (peer_event_queue.empty()) return;
    std::swap(local, peer_event_queue);
  }
  while (!local.empty()) {
    const auto &ev = local.front();
    PeerEntry *p = find_peer_in_cache(ev.mac.data()); // Safe: main-loop context
    if (p != nullptr) {
      if (ev.type == PeerEvent::Type::SEND_OK) {
        if (p->fail_count > 0) {
          ESP_LOGI("espnow_ack", "Peer %s recovered after %d failures",
                   format_mac(ev.mac.data()).c_str(), p->fail_count);
          p->fail_count = 0;
        }
        p->last_seen = millis();
      } else { // SEND_FAIL
        p->fail_count++;
        ESP_LOGW("espnow_ack", "Send to %s failed (failures: %d/%d)",
                 format_mac(ev.mac.data()).c_str(), p->fail_count,
                 MAX_PEER_SEND_FAILURES);
        if (p->fail_count >= MAX_PEER_SEND_FAILURES) {
          ESP_LOGE("espnow_ack", "Peer %s exceeded max failures — removing",
                   format_mac(ev.mac.data()).c_str());
          remove_stale_peer(ev.mac.data()); // Safe: main-loop context
          trigger_re_discovery();
        }
      }
    }
    local.pop();
  }
}

/**
 * @brief Sends a sync packet to all registered peers via unicast with ACK tracking.
 *
 * @note  (K-1 Fix) The send callback captures only the peer MAC and the
 *        success/failure result, then enqueues a PeerEvent for deferred
 *        processing. It does NOT access peer_cache or call remove_stale_peer()
 *        directly — those operations run safely in the main loop via
 *        process_peer_events().
 */
inline void send_sync_to_all_peers(const std::vector<uint8_t> &data) {
  if (!esphome::espnow::global_esp_now || peer_cache.empty())
    return;

  // Seal once: every peer receives the identical frame (same counter).
  std::vector<uint8_t> frame = data;
  if (!espnow_seal_frame(frame)) return;

  // ✅ MAC snapshot prevents iterator-invalidation if cache mutates later
  std::vector<std::array<uint8_t, 6>> mac_snapshot;
  mac_snapshot.reserve(peer_cache.size());
  for (const auto &entry : peer_cache) {
    mac_snapshot.push_back(entry.mac);
  }

  for (const auto &peer_mac : mac_snapshot) {
    esphome::espnow::global_esp_now->send(
        peer_mac.data(), frame,
        // ✅ WiFi-task callback: ONLY enqueue — never read/write peer_cache!
        [peer_mac](esp_err_t err) {
          PeerEvent ev;
          ev.mac = peer_mac;
          ev.type = (err == ESP_OK) ? PeerEvent::Type::SEND_OK
                                    : PeerEvent::Type::SEND_FAIL;
          std::lock_guard<std::mutex> lock(peer_event_mutex);
          // Cap event queue to prevent unbounded growth when peers are offline
          if (peer_event_queue.size() < PEER_EVENT_QUEUE_MAX_DEPTH) {
            peer_event_queue.push(ev);
          }
        });
  }
}

/**
 * @brief Parses incoming discovery strings.
 *
 * @note  (H-3 Fix) Prefix strings are now named constexpr constants.
 *        The sscanf offset is computed from the matched prefix length
 *        rather than the magic hardcoded literal `10`, so renaming
 *        either prefix is safe without a hidden offset update.
 */
static constexpr std::string_view DISC_PREFIX_DISC = "ROOM_DISC:";
static constexpr std::string_view DISC_PREFIX_CONF = "ROOM_CONF:";

inline bool handle_discovery_payload(const std::string &payload,
                                     const uint8_t *src_addr) {
  if (payload.length() > MAX_PAYLOAD_LEN) {
    ESP_LOGW("espnow_disc", "Discovery payload too long: %zu", payload.length());
    return false;
  }

  // Determine which prefix matched and derive the data offset from it
  std::string_view sv(payload);
  size_t prefix_len = 0;
  bool is_disc = false;

  if (sv.substr(0, DISC_PREFIX_DISC.size()) == DISC_PREFIX_DISC) {
    prefix_len = DISC_PREFIX_DISC.size();
    is_disc = true;
  } else if (sv.substr(0, DISC_PREFIX_CONF.size()) == DISC_PREFIX_CONF) {
    prefix_len = DISC_PREFIX_CONF.size();
    is_disc = false;
  } else {
    return false; // Not a discovery packet
  }

  if (is_local_mac(src_addr)) {
    ESP_LOGD("espnow_disc", "Ignoring own discovery loopback");
    return true;
  }

  int floor, room;
  if (sscanf(payload.c_str() + prefix_len, "%d:%d", &floor, &room) == 2) {
    auto local_ids = get_local_room_ids();
    if (!local_ids) {
      ESP_LOGW("espnow_disc", "Config IDs not ready (null or NaN), ignoring discovery (Payload: %s)", payload.c_str());
      return false;
    }

    int local_floor = local_ids->first;
    int local_room  = local_ids->second;

    ESP_LOGI("espnow_disc", "Discovery match check: [Payload %d:%d vs Local %d:%d]",
             floor, room, local_floor, local_room);

    if (floor == local_floor && room == local_room) {
      ESP_LOGW("espnow_disc", "✅ ALL CRITERIA MET - Registering peer: %s",
               format_mac(src_addr).c_str());
      register_peer_dynamic(src_addr);
      if (is_disc) {
        send_discovery_confirmation(src_addr);
      }
      return true;
    } else {
      ESP_LOGI("espnow_disc", "❌ Group mismatch - ignoring.");
    }
  } else {
    ESP_LOGW("espnow_disc", "Failed to parse discovery payload format: %s", payload.c_str());
  }
  return false;
}



// =========================================================
// SECTION: Packet Processing (espnow_handler)
// =========================================================

namespace espnow_handler {

  /**
   * @brief   Validates and parses a raw ESP-NOW byte buffer into a VentilationPacket.
   *
   * @details Thin logging wrapper around ventosync::packet::parse()
   *          (ventilation_packet.h) — the single, unit-tested deserialisation
   *          point (header, version, exact size, memcpy, field ranges).
   *
   * @param[in] data  Authenticated payload (trailer already stripped).
   *
   * @return  std::optional<VentilationPacket>  Populated packet on success,
   *          std::nullopt if any validation check fails.
   */
  inline std::optional<esphome::VentilationPacket>
  validate_and_parse_packet(const std::vector<uint8_t> &data) {
    // Pure + unit-tested (ventilation_packet.h); only the logging lives here.
    ventosync::packet::ParseResult res;
    auto pkt = ventosync::packet::parse(data.data(), data.size(), res);
    if (!pkt) {
      using ventosync::packet::ParseError;
      if (res.error == ParseError::VERSION_MISMATCH) {
        ESP_LOGW("vent_sync",
                 "Protocol version mismatch! Got v%d, expected v%d. "
                 "Devices on the network are running mismatched firmware.",
                 res.value, esphome::PROTOCOL_VERSION);
      } else if (res.error == ParseError::OUT_OF_RANGE) {
        ESP_LOGW("vent_sync", "%s out of range: %d", res.field, res.value);
      } else {
        ESP_LOGW("vent_sync", "Packet rejected: %s (%d)", ventosync::packet::parse_error_label(res.error),
                 res.value);
      }
    }
    return pkt;
  }

  /**
   * @brief   Handles an incoming MSG_STATUS_REQUEST.
   * @details Verifies the Room/Floor ID and immediately responds with a 
   *          MSG_STATUS_RESPONSE via Unicast.
   * @param[in] pkt       Pointer to the parsed VentilationPacket.
   * @param[in] src_mac   MAC address of the requester.
   */
inline void handle_status_request(const esphome::VentilationPacket *pkt, const uint8_t *src_mac) {
  auto ids = get_local_room_ids();
  if (ids &&
      pkt->floor_id == ids->first &&
      pkt->room_id == ids->second) {
    // FIX: Register the requester as a peer immediately
    register_peer_dynamic(src_mac);

    ESP_LOGI("vent_sync", "Status request from peer %d. Sending UNICAST response...",
             pkt->device_id);
    espnow_send_frame(src_mac, build_and_populate_packet(esphome::MSG_STATUS_RESPONSE));
  }
}

/**
 * @brief   Synchronizes configuration fields from a received packet.
 * @details Updates local Home Assistant template entities (thresholds,
 *          timers, brightness) to match the group leader (Master).
 * @param[in] pkt  Pointer to the parsed VentilationPacket.
 */
inline void handle_config_sync(const esphome::VentilationPacket *pkt) {
  // The packet was range-checked by ventosync::packet::parse() (levels 1-10,
  // sync interval 1-1440, vent timer <= 1440) — only ranges parse() does not
  // cover (HA slider / setting ranges) are checked here.
  bool dirty = false;
  auto *v = ventilation_ctrl;
  if (v == nullptr) return;

  // 1. Automatik Min/Max levels (Sliders)
  if (automatik_min_fan_level != nullptr &&
      automatik_min_luefterstufe != nullptr &&
      pkt->automatik_min_fan_level != automatik_min_fan_level->value()) {
    automatik_min_fan_level->value() = pkt->automatik_min_fan_level;
    automatik_min_luefterstufe->publish_state(pkt->automatik_min_fan_level);
    dirty = true;
  }

  if (automatik_max_fan_level != nullptr &&
      automatik_max_luefterstufe != nullptr &&
      pkt->automatik_max_fan_level != automatik_max_fan_level->value()) {
    automatik_max_fan_level->value() = pkt->automatik_max_fan_level;
    automatik_max_luefterstufe->publish_state(pkt->automatik_max_fan_level);
    dirty = true;
  }

  // 2. Thresholds
  if (auto_co2_threshold_val != nullptr && auto_co2_threshold != nullptr &&
      pkt->auto_co2_threshold_val >= 400 &&
      pkt->auto_co2_threshold_val <= 5000 &&
      pkt->auto_co2_threshold_val != auto_co2_threshold_val->value()) {
    auto_co2_threshold_val->value() = pkt->auto_co2_threshold_val;
    auto_co2_threshold->publish_state(pkt->auto_co2_threshold_val);
    if (pid_co2 != nullptr) {
      auto call = pid_co2->make_call();
      call.set_target_temperature(pkt->auto_co2_threshold_val);
      call.perform();
    }
    dirty = true;
  }

  if (auto_humidity_threshold_val != nullptr &&
      auto_humidity_threshold != nullptr &&
      pkt->auto_humidity_threshold_val <= 100 &&
      pkt->auto_humidity_threshold_val !=
          auto_humidity_threshold_val->value()) {
    auto_humidity_threshold_val->value() = pkt->auto_humidity_threshold_val;
    auto_humidity_threshold->publish_state(pkt->auto_humidity_threshold_val);
    if (pid_humidity != nullptr) {
      auto call = pid_humidity->make_call();
      call.set_target_temperature(pkt->auto_humidity_threshold_val);
      call.perform();
    }
    dirty = true;
  }

  if (auto_presence_val != nullptr && auto_presence_slider != nullptr &&
      pkt->auto_presence_val >= -5 && pkt->auto_presence_val <= 5 &&
      pkt->auto_presence_val != auto_presence_val->value()) {
    auto_presence_val->value() = pkt->auto_presence_val;
    auto_presence_slider->publish_state(pkt->auto_presence_val);
    dirty = true;
  }

  // 2b. Smart Climate Control (room-wide, same authority rule).
  //     Only values inside the HA slider ranges are adopted (shared
  //     constants in hvac_coordinator.h) — a peer can never push a value the
  //     UI could not represent.
  using namespace ventosync::hvac;
  if (hvac_co2_threshold_val != nullptr && hvac_co2_threshold != nullptr &&
      in_range(pkt->hvac_co2_threshold, CO2_THRESHOLD_MIN_PPM, CO2_THRESHOLD_MAX_PPM) &&
      pkt->hvac_co2_threshold != hvac_co2_threshold_val->value()) {
    hvac_co2_threshold_val->value() = pkt->hvac_co2_threshold;
    hvac_co2_threshold->publish_state(pkt->hvac_co2_threshold);
    dirty = true;
  }
  if (hvac_emergency_co2_val != nullptr && hvac_emergency_co2 != nullptr &&
      in_range(pkt->hvac_emergency_co2, EMERGENCY_CO2_MIN_PPM, EMERGENCY_CO2_MAX_PPM) &&
      pkt->hvac_emergency_co2 != hvac_emergency_co2_val->value()) {
    hvac_emergency_co2_val->value() = pkt->hvac_emergency_co2;
    hvac_emergency_co2->publish_state(pkt->hvac_emergency_co2);
    dirty = true;
  }
  if (hvac_max_fan_level_val != nullptr && hvac_max_fan_level != nullptr &&
      in_range(pkt->hvac_max_fan_level, MAX_FAN_LEVEL_CONFIG_MIN, MAX_FAN_LEVEL_CONFIG_MAX) &&
      pkt->hvac_max_fan_level != hvac_max_fan_level_val->value()) {
    hvac_max_fan_level_val->value() = pkt->hvac_max_fan_level;
    hvac_max_fan_level->publish_state(pkt->hvac_max_fan_level);
    dirty = true;
  }
  // Room-wide enable switch (protocol v10). The switch entity mirrors the
  // global via its lambda; publish immediately for a responsive HA UI.
  const bool peer_hvac_enabled = (pkt->room_flags & esphome::ROOM_FLAG_HVAC_ENABLED) != 0;
  if (hvac_enabled_val != nullptr && peer_hvac_enabled != hvac_enabled_val->value()) {
    hvac_enabled_val->value() = peer_hvac_enabled;
    if (smart_climate_control != nullptr) smart_climate_control->publish_state(peer_hvac_enabled);
    ESP_LOGI("hvac", "Klima-Koordination %s (room-wide sync from device %d)",
             peer_hvac_enabled ? "aktiviert" : "deaktiviert", pkt->device_id);
    dirty = true;
  }

  // Room-wide NOx boost switch (room flag bit, 0.10.38). Older firmware never
  // sets the bit, so an older Master keeps the room's switch off (safe side).
  const bool peer_nox_enabled = (pkt->room_flags & esphome::ROOM_FLAG_NOX_BOOST_ENABLED) != 0;
  if (nox_boost_enabled_val != nullptr && peer_nox_enabled != nox_boost_enabled_val->value()) {
    nox_boost_enabled_val->value() = peer_nox_enabled;
    if (nox_boost_switch != nullptr) nox_boost_switch->publish_state(peer_nox_enabled);
    ESP_LOGI("nox", "NOx-Stoßlüftung %s (room-wide sync from device %d)",
             peer_nox_enabled ? "aktiviert" : "deaktiviert", pkt->device_id);
    dirty = true;
  }

  // 2c. Level override duration (room-wide setting, protocol v12). Only
  //     values inside the HA number range are adopted.
  if (auto_override_duration_val != nullptr && auto_override_duration != nullptr &&
      ventosync::auto_override::valid_duration_min(pkt->auto_override_duration_min) &&
      pkt->auto_override_duration_min != auto_override_duration_val->value()) {
    auto_override_duration_val->value() = pkt->auto_override_duration_min;
    auto_override_duration->publish_state(pkt->auto_override_duration_min);
  }

  // 3. System Timers
  if (sync_interval_config != nullptr &&
      v->sync_interval_ms != static_cast<uint32_t>(pkt->sync_interval_min * 60 * 1000)) {
    v->sync_interval_ms = static_cast<uint32_t>(pkt->sync_interval_min * 60 * 1000);
    sync_interval_config->publish_state(pkt->sync_interval_min);
    dirty = true;
  }

  // Note: vent_timer intentionally does not set dirty because it does not
  // affect auto-mode evaluation (only the ventilation duration timer).
  if (vent_timer != nullptr &&
      static_cast<uint32_t>(pkt->vent_timer_min * 60 * 1000) !=
          v->state_machine.ventilation_duration_ms) {
    v->state_machine.ventilation_duration_ms = pkt->vent_timer_min * 60 * 1000;
    vent_timer->publish_state(pkt->vent_timer_min);
  }

  // 4. UI Settings (LED Brightness)
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

/**
 * @brief   Synchronizes global system state (Mode, Intensity, Power) from a peer.
 * @details Maps external VentilationPacket indices back to local UI components.
 * @param[in] pkt    Pointer to the parsed VentilationPacket.
 * @param[in] force  If true, the mode is applied even if no change is detected.
 */
inline void handle_state_sync(const esphome::VentilationPacket *pkt, bool force = false) {
  auto *v = ventilation_ctrl;
  if (v == nullptr)
    return;

  // Issue #4 FIX: Only override fan intensity from peer in manual modes.
  // In Automatik mode, the local PID controller is the authority for intensity.
  // Decided by the packet's mode too and compared with the level the fan runs
  // on — on_packet_received() has already updated v->current_fan_intensity
  // (see ventosync::packet::level_to_adopt()).
  const bool is_auto = (auto_mode_active != nullptr && auto_mode_active->value());
  if (fan_intensity_level != nullptr && fan_intensity_display != nullptr) {
    const uint8_t level = ventosync::packet::level_to_adopt(*pkt, fan_intensity_level->value(), is_auto);
    if (level != 0) {
      v->set_fan_intensity(level, false);  // Updates PWM output
      fan_intensity_level->value() = level;
      fan_intensity_display->publish_state(level);
    }
  }

  // Trust the sender's explicit UI index (range-checked in validate_and_parse_packet())
  const int new_mode_idx = pkt->current_mode_index;

  // 1. Sync global power switches based on the transmitted core state.
  //    NOTE: Reads v->state_machine.current_mode (local state) which was
  //    already mutated by on_packet_received() before this function is called.
  //    This ordering dependency is by design — the caller must invoke
  //    on_packet_received() first.
  if (ventilation_enabled) {
    ventilation_enabled->value() = (v->state_machine.current_mode != esphome::MODE_OFF);
  }

  // 2. Map the peer's UI mode index to a local text state and set auto_mode.
  //    Issue #3 FIX: auto_mode_active is derived solely from the mode index
  //    in this single location (consolidated from a previous double-write).
  const std::string mode_str = MODE_NAMES[new_mode_idx];
  if (auto_mode_active) auto_mode_active->value() = (new_mode_idx == MODE_INDEX_AUTO);

  if (current_mode_index != nullptr && (new_mode_idx != current_mode_index->value() || force)) {
    current_mode_index->value() = new_mode_idx;
  }
  remember_active_mode(new_mode_idx); // Power button restores the room's last mode

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

// =========================================================
// SECTION: Master Loop Callback
// =========================================================

/**
 * @brief   Entry point for all incoming ESP-NOW traffic (WiFi task context).
 *
 * @details This function runs in the high-priority WiFi task. To prevent
 *          Interrupt WDT crashes, it MUST NOT perform heavy operations
 *          (flash writes, memory allocation, peer registration, logging).
 *          Instead it captures the raw packet into a thread-safe queue
 *          for deferred processing in the main ESPHome loop.
 *
 * @param[in] data      Incoming byte vector.
 * @param[in] src_mac   MAC address of the sender.
 *
 * @note    Thread safety: This function is called from the WiFi task.
 *          All shared state access is protected by rx_queue_mutex.
 */
inline void handle_espnow_receive(const std::vector<uint8_t> &data, const uint8_t *src_mac) {
  // ✅ Exact-duplicate guard (same frame within 50 ms is not queued twice).
  // Hashes the WHOLE frame incl. the authentication trailer, whose counter
  // makes every genuine frame unique — so distinct frames are never dropped
  // (hashing only the header bytes could drop a second, different frame).
  // Uses only local statics — no shared state, no mutex needed. Replays are
  // rejected later by the ReplayGuard (espnow_open_frame()).
  static uint32_t last_rx_time = 0;
  static uint32_t last_rx_hash = 0;
  uint32_t now = millis();

  uint32_t hash = 2166136261u; // FNV-1a
  for (uint8_t b : data) hash = (hash ^ b) * 16777619u;
  for (size_t i = 0; i < 6; i++) hash = (hash ^ src_mac[i]) * 16777619u;

  if (hash == last_rx_hash && (now - last_rx_time) < 50) {
    return; // Exact duplicate suppressed
  }
  last_rx_hash = hash;
  last_rx_time = now;

  // ✅ Enqueue for main-loop processing (minimal critical section)
  {
    std::lock_guard<std::mutex> lock(rx_queue_mutex);
    if (rx_queue.size() >= RX_QUEUE_MAX_DEPTH) {
      return; // Drop oldest-prevention: just reject new packets under flood
    }
    IncomingPacket pkt;
    pkt.data.assign(data.begin(), data.end());
    std::copy(src_mac, src_mac + 6, pkt.src_mac.begin());
    rx_queue.push(std::move(pkt));
  }
}

// =========================================================
// SECTION: Main-Loop Packet Processing
// =========================================================

/**
 * @brief   Processes a single ESP-NOW packet in the main loop context.
 *
 * @details This contains the full processing logic that was previously
 *          in handle_espnow_receive. Running in the main loop guarantees
 *          safe access to globals, flash/NVS, peer registration, and UI
 *          components without risking Interrupt WDT timeouts.
 *
 * @param[in] frame     Received frame (payload + authentication trailer).
 * @param[in] src_mac   Sender MAC address.
 */
inline void process_espnow_packet_local(const std::vector<uint8_t> &frame, const uint8_t *src_mac) {
  // --- DEBUG LOGGING ---
  if (!frame.empty()) {
    ESP_LOGD("espnow_raw", "RX from %s | Len: %zu | First: 0x%02X",
             format_mac(src_mac).c_str(), frame.size(), frame[0]);
  }

  // Protocol v13: nothing is interpreted before the frame is authenticated
  // (tag over sender MAC + payload + counter, replay guard). Covers the
  // plaintext discovery strings as well as VentilationPackets.
  std::vector<uint8_t> data;
  if (!espnow_open_frame(frame, src_mac, data)) return;

  // Intercept discovery strings before validate_packet().
  // Discovery packets are plaintext, not VentilationPacket structs.
  if (!data.empty()) {
    std::string payload(data.begin(), data.end());
    const std::string_view sv(payload);
    if (sv.substr(0, DISC_PREFIX_DISC.size()) == DISC_PREFIX_DISC ||
        sv.substr(0, DISC_PREFIX_CONF.size()) == DISC_PREFIX_CONF) {
      handle_discovery_payload(payload, src_mac);
      return; // Not a VentilationPacket — skip structured processing
    }
  }

  if (is_local_mac(src_mac)) {
    ESP_LOGD("vent_sync", "Ignored own packet (loopback).");
    return;
  }

  // ✅ N-2: Single validated parse — validate_and_parse_packet() is the
  // only cast point. It checks all invariants AND deserialises via memcpy.
  auto maybe_pkt = espnow_handler::validate_and_parse_packet(data);
  if (!maybe_pkt) {
    // Only log for non-discovery packets that looked like VentSync traffic
    if (!data.empty() && data[0] == PACKET_MAGIC) {
      ESP_LOGW("vent_sync", "Packet validation failed for message from %s",
               format_mac(src_mac).c_str());
    }
    return;
  }

  // Successful packet reception — reset fail counter for this peer
  reset_peer_fail_count(src_mac);

  const esphome::VentilationPacket &pkt_val = *maybe_pkt;
  const esphome::VentilationPacket *pkt = &pkt_val;
  auto *v = ventilation_ctrl;
  if (v == nullptr)
    return;

  if (static_cast<esphome::MessageType>(pkt->msg_type) ==
      esphome::MSG_STATUS_REQUEST) {
    espnow_handler::handle_status_request(pkt, src_mac);
    return;
  }

  bool changed = v->on_packet_received(pkt_val, src_mac);

  if (static_cast<esphome::MessageType>(pkt->msg_type) ==
          esphome::MSG_STATUS_RESPONSE &&
      !v->is_state_synced) {
    auto ids = get_local_room_ids();
    if (ids &&
        pkt->floor_id == ids->first &&
        pkt->room_id == ids->second) {
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

      auto_override_adopt(pkt); // Rejoin a running level override

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
    // Room-wide level override: same authority as the mode (any MSG_STATE,
    // Master heartbeat). Runs after the mode sync above.
    auto_override_adopt(pkt);
  }
}

/**
 * @brief   Drains the rx_queue and processes all pending packets.
 *
 * @details Called once per main-loop iteration from VentilationController::loop().
 *          Acquires the mutex briefly to swap the queue contents into a local
 *          buffer, then processes each packet without holding the lock.
 *          This minimizes contention with the WiFi task producer.
 */
inline void process_queued_packets() {
  // (K-1 Fix) Always drain send-ACK events, even when no RX packets are
  // pending. Without this, SEND_FAIL events from offline peers accumulate
  // in peer_event_queue but are never processed — blocking stale-peer
  // removal, re-discovery, and causing unbounded queue growth.
  process_peer_events();

  // (H-4 Fix) Guard check without acquiring the mutex.
  // On the single-core ESP32-C6 (RISC-V) preemption between tasks is the
  // only concurrency mechanism. A stale empty() check here causes at most
  // one missed iteration — not a data-corruption risk — because:
  //   • std::queue::size() is a simple integer read (naturally atomic on
  //     aligned RISC-V 32-bit targets at esp-idf O2).
  //   • The real protection against corruption is the lock_guard below.
  // We keep this fast-path to avoid mutex overhead on the hot main loop.
  if (rx_queue.empty()) return;

  // Drain the RX packet queue under its lock into a local buffer,
  // then release before processing.
  std::queue<IncomingPacket> local_queue;
  {
    std::lock_guard<std::mutex> lock(rx_queue_mutex);
    std::swap(local_queue, rx_queue);
  }

  // Process all RX packets in main-loop context (safe for flash, UI, peers)
  while (!local_queue.empty()) {
    auto &pkt = local_queue.front();
    process_espnow_packet_local(pkt.data, pkt.src_mac.data());
    local_queue.pop();
  }
}
