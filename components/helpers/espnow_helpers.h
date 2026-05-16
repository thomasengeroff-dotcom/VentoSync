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
// File:        espnow_helpers.h
// Description: ESP-NOW mesh communication helpers for router-independent
//              peer-to-peer room synchronization. Handles broadcast
//              processing, WiFi reconnect detection, and periodic
//              peer rediscovery.
// Author:      Thomas Engeroff
// Created:     2026-05-16
// Modified:    2026-05-16
//
// Dependencies: globals.h (peer_cache)
//               espnow_protocol.h (build_and_populate_packet,
//                                   send_sync_to_all_peers,
//                                   send_discovery_broadcast,
//                                   MSG_SYNC)
//               VentilationController (ventilation_ctrl, pending_broadcast)
//               ESPHome WiFi component (wifi::global_wifi_component)
// ==========================================================================

#pragma once

#include "esphome.h"

// ---------------------------------------------------------
// WIFI RECONNECT EDGE DETECTION
// ---------------------------------------------------------

/**
 * @brief Detects a WiFi rising-edge event (disconnected → connected).
 *
 * Uses an internal static flag to track the previous connection state.
 * Returns true exactly ONCE per reconnect cycle, enabling one-shot
 * actions like ESP-NOW rediscovery without repeated triggering while
 * the connection remains stable.
 *
 * State machine:
 *   - disconnected → connected : returns TRUE (once), sets flag
 *   - connected    → connected : returns FALSE (flag already set)
 *   - connected    → disconnected : returns FALSE, resets flag
 *   - disconnected → disconnected : returns FALSE
 *
 * @return true on the first call after WiFi reconnects, false otherwise.
 *
 * @note Thread-safety: This function is called exclusively from the
 *       ESPHome main loop (interval component), so no mutex is needed.
 *       If called from multiple contexts in the future, add
 *       synchronization.
 *
 * @note Called every 2s from interval in logic_automation.yaml.
 */
inline bool wifi_just_connected() {
    static bool was_connected = false;

    const bool is_connected = wifi::global_wifi_component->is_connected();

    if (is_connected && !was_connected) {
        // Rising edge: just reconnected
        was_connected = true;
        ESP_LOGI("wifi_edge", "WiFi connection established – triggering reconnect actions");
        return true;
    }

    if (!is_connected) {
        // Reset flag so we detect the next reconnect
        if (was_connected) {
            ESP_LOGW("wifi_edge", "WiFi connection lost");
        }
        was_connected = false;
    }

    return false;
}

// ---------------------------------------------------------
// PENDING BROADCAST PROCESSING
// ---------------------------------------------------------

/**
 * @brief Processes pending ESP-NOW state broadcasts.
 *
 * The VentilationController sets `pending_broadcast = true` whenever
 * local state changes that need to be synchronized to peer devices
 * (e.g. mode change, intensity change, sensor updates).
 *
 * This function:
 *   1. Checks the pending_broadcast flag on the controller
 *   2. If set, builds a MSG_SYNC packet with current state
 *   3. Unicasts the packet to all registered peers in peer_cache
 *   4. The build function automatically clears pending_broadcast
 *
 * @note Called every 1s from interval in logic_automation.yaml.
 *       The 1s polling rate is a deliberate trade-off:
 *       - Fast enough for responsive sync (human-imperceptible delay)
 *       - Slow enough to naturally coalesce rapid successive changes
 *         (e.g. user spinning the intensity dial) into a single packet
 *
 * @note The unicast-to-all-peers strategy (vs. ESP-NOW broadcast) is
 *       intentional: it provides delivery confirmation per peer and
 *       avoids polluting the broadcast domain in multi-AP environments.
 */
inline void process_pending_broadcast() {
    auto *v = get_controller();
    if (!v->pending_broadcast) return;

    ESP_LOGD("espnow_sync", "Processing pending broadcast → building SYNC packet");
    auto data = build_and_populate_packet(esphome::MSG_SYNC);
    send_sync_to_all_peers(data);
    ESP_LOGD("espnow_sync", "SYNC packet sent to peers");
}

// ---------------------------------------------------------
// PERIODIC PEER REDISCOVERY
// ---------------------------------------------------------

/**
 * @brief Re-triggers ESP-NOW discovery if no peers are known.
 *
 * In a VentoSync mesh, each device maintains a local peer_cache of
 * known room partners. This cache can become empty due to:
 *   - First boot (no peers discovered yet)
 *   - All peers simultaneously rebooting (e.g. after power outage)
 *   - Prolonged WiFi interference causing peer timeout/eviction
 *
 * This function is called periodically (every 5 minutes) as a safety
 * net. It only sends a discovery broadcast when the cache is actually
 * empty, avoiding unnecessary RF traffic during normal operation.
 *
 * @note Discovery broadcasts use the ESP-NOW broadcast address
 *       (FF:FF:FF:FF:FF:FF). Peers respond with their identity,
 *       which populates the local peer_cache.
 *
 * @note Called every 5min from interval in logic_automation.yaml.
 */
inline void periodic_peer_rediscovery() {
    if (peer_cache.empty()) {
        ESP_LOGI("espnow_disc",
                 "Periodic peer check — cache empty, sending discovery broadcast");
        send_discovery_broadcast();
    } else {
        ESP_LOGV("espnow_disc",
                 "Periodic peer check — %u peer(s) in cache, no action needed",
                 static_cast<unsigned>(peer_cache.size()));
    }
}

// ---------------------------------------------------------
// CONFIG SYNC GUARD
// ---------------------------------------------------------

/**
 * @brief Ensures the VentilationController has a valid device_id.
 *
 * During boot, there's a race condition where the controller may start
 * its automation loop before Home Assistant has pushed the device
 * configuration (including the device_id). This guard detects the
 * uninitialized state (device_id == 0) and forces a config resync.
 *
 * @return true if a resync was triggered (caller may want to skip
 *         further processing this cycle), false if config is valid.
 *
 * @note Called every 10s from auto_mode_interval in logic_automation.yaml,
 *       before evaluate_auto_mode(). The 60s periodic sync_config_to_controller()
 *       handles ongoing drift; this function handles the boot-time edge case.
 */
inline bool guard_config_sync() {
    auto *v = get_controller();

    if (v->device_id == 0) {
        ESP_LOGW("config_guard",
                 "Controller device_id is 0 (uninitialized) — forcing config sync");
        sync_config_to_controller();
        return true;
    }

    return false;
}
