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
// File:        config_helpers.h
// Description: Device configuration update handlers for floor/room/device
//              IDs, phase selection, and diagnostic summaries.
//              Centralizes VentilationController config mutations,
//              peer cache invalidation, and ESP-NOW rediscovery triggers.
// Author:      Thomas Engeroff
// Created:     2026-05-16
// Modified:    2026-05-16
//
// Dependencies: globals.h (peer_cache, espnow_peers)
//               VentilationController (ventilation_ctrl)
//               espnow_helpers.h (send_discovery_broadcast) – optional
// ==========================================================================

#pragma once

#include "esphome.h"

// ---------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------

namespace ventosync {
namespace config {

/// Phase option strings – single source of truth.
/// Must match the `options:` list in device_config.yaml exactly.
static constexpr const char *PHASE_A_LABEL = "Phase A (Startet mit Zuluft)";
static constexpr const char *PHASE_B_LABEL = "Phase B (Startet mit Abluft)";

/// Identifies which config field changed (for unified handler).
enum class ConfigField : uint8_t {
    FLOOR_ID,
    ROOM_ID,
    DEVICE_ID,
};

}  // namespace config
}  // namespace ventosync

// ---------------------------------------------------------
// PEER CACHE INVALIDATION
// ---------------------------------------------------------

/**
 * @brief Clears the ESP-NOW peer cache and triggers rediscovery.
 *
 * Called when a topology-relevant config value changes (floor or room ID).
 * Peers are grouped by floor+room, so any change in these values means
 * the current peer list is stale and must be rebuilt.
 *
 * @note Device ID changes do NOT invalidate peers – all devices in the
 *       same floor+room remain valid peers regardless of their device ID.
 *       The device ID is only used for ordering/priority within the room.
 */
inline void invalidate_peer_cache_and_rediscover() {
    id(espnow_peers) = "";
    peer_cache.clear();

    ESP_LOGI("config", "Peer cache cleared — triggering ESP-NOW rediscovery");
    send_discovery_broadcast();
}

// ---------------------------------------------------------
// UNIFIED CONFIG ID UPDATE
// ---------------------------------------------------------

/**
 * @brief Handles a configuration ID change (floor, room, or device).
 *
 * Unified handler that:
 *   1. Applies the new value to the VentilationController via the
 *      appropriate setter
 *   2. Logs the change
 *   3. Invalidates the peer cache if the change affects topology
 *      (floor or room – but NOT device ID)
 *
 * @param field  Which config field changed
 * @param value  The new value (will be cast to uint8_t)
 *
 * @note Called from set_action lambdas in device_config.yaml.
 *       Each lambda becomes a one-liner: `update_config_id(ConfigField::FLOOR_ID, x);`
 */
inline void update_config_id(ventosync::config::ConfigField field, float value) {
    auto *v = get_controller();
    const auto id_val = static_cast<uint8_t>(value);

    switch (field) {
        case ventosync::config::ConfigField::FLOOR_ID:
            v->set_floor_id(id_val);
            ESP_LOGI("config", "Floor ID updated to: %u", id_val);
            invalidate_peer_cache_and_rediscover();
            break;

        case ventosync::config::ConfigField::ROOM_ID:
            v->set_room_id(id_val);
            ESP_LOGI("config", "Room ID updated to: %u", id_val);
            invalidate_peer_cache_and_rediscover();
            break;

        case ventosync::config::ConfigField::DEVICE_ID:
            v->set_device_id(id_val);
            ESP_LOGI("config", "Device ID updated to: %u", id_val);
            // No peer invalidation – device ID is intra-room identity,
            // not a topology change. Peers in the same floor+room remain valid.
            break;
    }
}

// ---------------------------------------------------------
// PHASE UPDATE
// ---------------------------------------------------------

/**
 * @brief Handles phase selection changes (Phase A / Phase B).
 *
 * Parses the human-readable select option string and maps it to the
 * boolean is_phase_a flag on the VentilationController.
 *
 * Phase A: Device starts with supply air (Zuluft) direction
 * Phase B: Device starts with exhaust air (Abluft) direction
 *
 * In a multi-device room, alternating phases between adjacent devices
 * ensures that at any given moment, at least one device is supplying
 * fresh air while the other exhausts – maximizing heat recovery
 * efficiency.
 *
 * @param selected_option  The option string from the HA select entity
 *
 * @note Called from config_phase set_action in device_config.yaml.
 */
inline void update_phase_config(const std::string &selected_option) {
    auto *v = get_controller();

    const bool is_phase_a = (selected_option == ventosync::config::PHASE_A_LABEL);
    v->set_is_phase_a(is_phase_a);

    ESP_LOGI("config", "Phase updated to: %s (%s)",
             is_phase_a ? "A" : "B",
             selected_option.c_str());
}

// ---------------------------------------------------------
// CONFIGURATION SUMMARY
// ---------------------------------------------------------

/**
 * @brief Builds a human-readable configuration summary string.
 *
 * Format: "Floor: X | Room: Y | Device: Z | Phase: A/B"
 *
 * @return Formatted summary string for the HA diagnostic text sensor.
 *
 * @note Called from device_config_summary lambda in device_config.yaml.
 * @note Uses snprintf with a fixed buffer – total output is well under
 *       128 chars, so truncation is not a concern.
 */
inline std::string build_config_summary() {
    // Determine phase label (short form for readability)
    const char *phase_short =
        (id(config_phase).current_option() == ventosync::config::PHASE_A_LABEL)
            ? "A"
            : "B";

    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "Floor: %d | Room: %d | Device: %d | Phase: %s",
             static_cast<int>(id(config_floor_id).state),
             static_cast<int>(id(config_room_id).state),
             static_cast<int>(id(config_device_id).state),
             phase_short);

    return std::string(buffer);
}

// ---------------------------------------------------------
// DASHBOARD PHASE HELPER
// ---------------------------------------------------------

/**
 * @brief Returns the short-form phase identifier ("A" or "B").
 *
 * @return "A" if Phase A is selected, "B" otherwise.
 *
 * @note Called from dashboard_phase lambda in device_config.yaml.
 */
inline std::string get_phase_short() {
    if (id(config_phase).current_option() == ventosync::config::PHASE_A_LABEL) {
        return std::string("A");
    }
    return std::string("B");
}

// ---------------------------------------------------------
// MANUAL DISCOVERY DIAGNOSTICS
// ---------------------------------------------------------

/**
 * @brief Logs detailed ESP-NOW diagnostic state and triggers manual discovery.
 *
 * Intended for the "Force ESPNOW Discovery" debug button. Logs the state
 * of all critical subsystems before sending the discovery broadcast,
 * making it easy to diagnose connectivity issues from the log output.
 *
 * Checks:
 *   - global_esp_now component initialization
 *   - config_floor_id entity existence and value
 *   - config_room_id entity existence and value
 *   - Current peer_cache size
 *
 * @note This is a DEBUG/DIAGNOSTIC function. In production, the automatic
 *       discovery mechanisms (wifi_just_connected, periodic_peer_rediscovery)
 *       handle peer management. This button exists for manual troubleshooting.
 */
inline void force_discovery_with_diagnostics() {
    ESP_LOGW("espnow_diag", "╔══════════════════════════════════════╗");
    ESP_LOGW("espnow_diag", "║     MANUAL DISCOVERY TRIGGERED       ║");
    ESP_LOGW("espnow_diag", "╠══════════════════════════════════════╣");

    // Check ESP-NOW component
    ESP_LOGW("espnow_diag", "║  ESP-NOW component : %-15s ║",
             esphome::espnow::global_esp_now ? "OK" : "NULL (!)");

    // Check config entities
    ESP_LOGW("espnow_diag", "║  Floor ID entity   : %-15s ║",
             config_floor_id ? "OK" : "NULL (!)");
    if (config_floor_id) {
        ESP_LOGW("espnow_diag", "║  Floor ID value    : %-15.0f ║",
                 config_floor_id->state);
    }

    ESP_LOGW("espnow_diag", "║  Room ID entity    : %-15s ║",
             config_room_id ? "OK" : "NULL (!)");
    if (config_room_id) {
        ESP_LOGW("espnow_diag", "║  Room ID value     : %-15.0f ║",
                 config_room_id->state);
    }

    ESP_LOGW("espnow_diag", "║  Peer cache size   : %-15u ║",
             static_cast<unsigned>(peer_cache.size()));

    ESP_LOGW("espnow_diag", "╚══════════════════════════════════════╝");

    // Execute discovery
    send_discovery_broadcast();

    ESP_LOGI("espnow_diag", "Discovery broadcast sent");
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
