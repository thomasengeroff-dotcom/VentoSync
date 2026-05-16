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
//              peer-to-peer room synchronization. Handles packet receive
//              dispatching, broadcast processing, WiFi reconnect detection,
//              and periodic peer rediscovery.
// Author:      Thomas Engeroff
// Created:     2026-05-16
// Modified:    2026-05-16
//
// Dependencies: globals.h (peer_cache)
//               espnow_protocol.h / network_sync.h
//                 (handle_espnow_receive, build_and_populate_packet,
//                  send_sync_to_all_peers, send_discovery_broadcast,
//                  load_peers_from_runtime_cache, request_peer_status,
//                  MSG_SYNC)
//               VentilationController (ventilation_ctrl, pending_broadcast)
//               ESPHome WiFi component (wifi::global_wifi_component)
// ==========================================================================

#pragma once

#include "esphome.h"

// ---------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------

namespace ventosync {
namespace espnow {

/// Receive source identifiers for logging/diagnostics.
/// Used by the unified receive dispatcher to tag log output.
enum class RxSource : uint8_t {
    BROADCAST,     ///< Received via on_broadcast (group packet)
    UNICAST,       ///< Received via on_receive (registered peer)
    UNKNOWN_PEER,  ///< Received via on_unknown_peer (discovery)
};

/// Human-readable labels for RxSource (indexed by enum value).
static constexpr const char *RX_SOURCE_LABELS[] = {
    "BROADCAST",
    "UNICAST",
    "UNKNOWN_PEER",
};

/// Peer watchdog interval in seconds.
/// If no peers are found, discovery is re-triggered at this rate.
/// 60s is aggressive enough for fast recovery after network splits,
/// but not so frequent that it floods the RF channel.
static constexpr uint32_t PEER_WATCHDOG_INTERVAL_S = 60;

}  // namespace espnow
}  // namespace ventosync

// ---------------------------------------------------------
// UNIFIED RECEIVE DISPATCHER
// ---------------------------------------------------------

/**
 * @brief Unified ESP-NOW packet receive handler.
 *
 * Dispatches incoming packets from all three ESPHome ESP-NOW callbacks
 * (on_broadcast, on_receive, on_unknown_peer) through a single code path.
 * This eliminates the previous 3× code duplication and provides a single
 * point for logging, validation, and forwarding to the protocol handler.
 *
 * @param data       Raw packet data pointer (from ESPHome callback)
 * @param size       Packet size in bytes
 * @param src_addr   Source MAC address (6 bytes, from ESPHome callback info)
 * @param source     Which callback triggered this receive (for logging)
 *
 * @note Zero-copy design: The raw pointer is passed directly to
 *       handle_espnow_receive() without creating an intermediate
 *       std::vector. This avoids a heap allocation on every received
 *       packet – critical for a 24/7 mesh with multiple peers sending
 *       sync packets every second.
 *
 * @note If handle_espnow_receive() requires a std::vector internally,
 *       the copy should happen there (once, at the protocol boundary),
 *       not in every callback.
 */
inline void dispatch_espnow_packet(const uint8_t *data, size_t size,
                                   const uint8_t *src_addr,
                                   ventosync::espnow::RxSource source) {
    using namespace ventosync::espnow;

    const char *source_label = RX_SOURCE_LABELS[static_cast<uint8_t>(source)];

    ESP_LOGD("espnow_rx", "[%s] Packet received, size=%u, src=%02X:%02X:%02X:%02X:%02X:%02X",
             source_label,
             static_cast<unsigned>(size),
             src_addr[0], src_addr[1], src_addr[2],
             src_addr[3], src_addr[4], src_addr[5]);

    // Minimal validation: reject empty or suspiciously small packets
    if (size < 2) {
        ESP_LOGW("espnow_rx", "[%s] Packet too small (%u bytes), ignoring",
                 source_label, static_cast<unsigned>(size));
        return;
    }

    // Forward to protocol handler (defined in network_sync.h)
    // NOTE: If handle_espnow_receive() still expects std::vector<uint8_t>,
    // the conversion happens here at the boundary – single allocation point.
    std::vector<uint8_t> packet(data, data + size);
    handle_espnow_receive(packet, src_addr);
}

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
 *
 * @note Called every 2s from interval in logic_automation.yaml.
 */
inline bool wifi_just_connected() {
    static bool was_connected = false;

    const bool is_connected = wifi::global_wifi_component->is_connected();

    if (is_connected && !was_connected) {
        was_connected = true;
        ESP_LOGI("wifi_edge", "WiFi connection established — triggering reconnect actions");
        return true;
    }

    if (!is_connected) {
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
 * local state changes that need to be synchronized to peer devices.
 *
 * @note Called every 1s from interval in logic_automation.yaml.
 */
inline void process_pending_broadcast() {
    auto *v = get_controller();

    if (!v->pending_broadcast) {
        return;
    }

    ESP_LOGD("espnow_sync", "Processing pending broadcast → building SYNC packet");

    auto data = build_and_populate_packet(esphome::MSG_SYNC);

    send_sync_to_all_peers(data);

    ESP_LOGD("espnow_sync", "SYNC packet sent to %u peer(s)",
             static_cast<unsigned>(peer_cache.size()));
}

// ---------------------------------------------------------
// PEER WATCHDOG
// ---------------------------------------------------------

/**
 * @brief Peer presence watchdog – retriggers discovery if cache is empty.
 *
 * Unified watchdog that replaces both the 60s interval in esp_now.yaml
 * and the 5min interval in logic_automation.yaml. Call this from a
 * single interval at the desired frequency.
 *
 * @note Recommended interval: 60s for fast recovery, with a log
 *       rate-limiter to avoid spamming at higher frequencies.
 */
inline void peer_presence_watchdog() {
    if (peer_cache.empty()) {
        ESP_LOGI("espnow_disc",
                 "Peer watchdog: cache empty — retrying discovery");
        send_discovery_broadcast();
    } else {
        ESP_LOGV("espnow_disc",
                 "Peer watchdog: %u peer(s) in cache, no action needed",
                 static_cast<unsigned>(peer_cache.size()));
    }
}

// ---------------------------------------------------------
// PERIODIC PEER REDISCOVERY (Legacy alias)
// ---------------------------------------------------------

/**
 * @brief Alias for peer_presence_watchdog().
 * @deprecated Use peer_presence_watchdog() directly. This alias exists
 *             for backward compatibility with logic_automation.yaml.
 */
inline void periodic_peer_rediscovery() {
    peer_presence_watchdog();
}
