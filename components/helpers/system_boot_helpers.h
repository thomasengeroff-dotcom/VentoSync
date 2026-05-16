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
// File:        system_boot_helpers.h
// Description: Early boot-time hardware initialization and ESP-NOW
//              discovery sequencing for the Seeed XIAO ESP32-C6.
//              These functions run during esphome on_boot at various
//              priorities and must be non-blocking (except where
//              explicitly noted for hardware stabilization).
// Author:      Thomas Engeroff
// Created:     2026-05-16
// Modified:    2026-05-16
//
// Dependencies: ESP-IDF GPIO driver (driver/gpio.h)
//               FreeRTOS (freertos/task.h)
//               globals.h (ignore_window_guard_switch, ventilation_ctrl)
//               espnow_helpers.h (send_discovery_broadcast)
// ==========================================================================

#pragma once

#include "esphome.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ---------------------------------------------------------
// HARDWARE CONSTANTS – XIAO ESP32-C6 RF SWITCH
// ---------------------------------------------------------

namespace ventosync {
namespace hw {

/// @brief GPIO pin that enables/disables the RF switch circuitry.
/// LOW = RF switch active (antenna path connected).
/// This pin controls the power/enable line of the antenna switch IC.
static constexpr gpio_num_t PIN_WIFI_ENABLE = GPIO_NUM_3;

/// @brief GPIO pin that selects the antenna path.
/// HIGH = U.FL external antenna connector selected.
/// LOW  = On-board PCB antenna selected (default after reset).
static constexpr gpio_num_t PIN_WIFI_ANT_SELECT = GPIO_NUM_14;

/// @brief Stabilization delay after activating the RF switch.
/// The antenna switch IC needs time to settle after power-on.
/// 100ms is conservative; most switches settle in <10ms,
/// but we're in a boot context where extra time is acceptable.
static constexpr uint32_t RF_SWITCH_SETTLE_MS = 100;

}  // namespace hw
}  // namespace ventosync

// ---------------------------------------------------------
// ANTENNA INITIALIZATION (Priority 900 – before WiFi)
// ---------------------------------------------------------

/**
 * @brief Configures the XIAO ESP32-C6 RF switch for external U.FL antenna.
 *
 * The Seeed XIAO ESP32-C6 has a hardware RF switch that can route the
 * WiFi signal to either the on-board PCB antenna or an external U.FL
 * connector. For the VentoSync PCB installation (metal enclosure),
 * the external antenna provides significantly better range and
 * reliability.
 *
 * Sequence:
 *   1. Reset GPIO 3 to default state, configure as output
 *   2. Drive GPIO 3 LOW → enables the RF switch IC
 *   3. Wait for RF switch to stabilize (100ms)
 *   4. Reset GPIO 14 to default state, configure as output
 *   5. Drive GPIO 14 HIGH → selects U.FL connector path
 *
 * @warning This MUST run at on_boot priority 900 (before WiFi starts
 *          at ~800). If WiFi initializes with the wrong antenna path,
 *          the connection will be unreliable or fail entirely.
 *
 * @note The gpio_reset_pin() calls clear any previous pin configuration
 *       (e.g. from a previous boot cycle or deep sleep wake), ensuring
 *       a clean state before reconfiguration.
 *
 * @note The vTaskDelay() here is acceptable because:
 *       - We're in the boot sequence, not the main loop
 *       - No other tasks depend on this completing quickly
 *       - Hardware stabilization requires a real time delay
 */
inline void init_external_antenna() {
    using namespace ventosync::hw;

    // ── Step 1+2: Enable RF switch circuitry ──
    gpio_reset_pin(PIN_WIFI_ENABLE);
    gpio_set_direction(PIN_WIFI_ENABLE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_WIFI_ENABLE, 0);  // LOW = RF switch enabled

    // ── Step 3: Wait for switch IC to stabilize ──
    vTaskDelay(pdMS_TO_TICKS(RF_SWITCH_SETTLE_MS));

    // ── Step 4+5: Select external U.FL antenna path ──
    gpio_reset_pin(PIN_WIFI_ANT_SELECT);
    gpio_set_direction(PIN_WIFI_ANT_SELECT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_WIFI_ANT_SELECT, 1);  // HIGH = U.FL selected

    ESP_LOGI("boot", "RF Switch configured: External U.FL antenna active "
             "(EN=GPIO%d LOW, SEL=GPIO%d HIGH)",
             static_cast<int>(PIN_WIFI_ENABLE),
             static_cast<int>(PIN_WIFI_ANT_SELECT));
}

// ---------------------------------------------------------
// CORE SYSTEM INITIALIZATION (Priority -10)
// ---------------------------------------------------------

/**
 * @brief Runs the core system initialization after WiFi and NVS are ready.
 *
 * This is the "Phase 1" of the boot sequence. It initializes the
 * VentilationController with persisted configuration and applies
 * runtime overrides from Home Assistant switches.
 *
 * @note Called at on_boot priority -10, which guarantees:
 *       - WiFi is connected (or attempting connection)
 *       - NVS (flash storage) is mounted and globals are restored
 *       - All ESPHome components are initialized
 *
 * @note The window guard override is applied here because the
 *       ignore_window_guard_switch state is restored from flash
 *       and must be pushed to the controller before the first
 *       automation cycle runs.
 */
inline void init_core_system() {
    // Run the main boot initialization (defined in system_lifecycle.h)
    // This syncs all persisted globals → VentilationController
    run_system_boot_initialization();

    // Apply window guard override from persisted HA switch state
    auto *v = get_controller();
    v->set_ignore_window_guard(id(ignore_window_guard_switch).state);

    ESP_LOGI("boot", "Phase 1 complete: Core system initialized, "
             "window guard override = %s",
             id(ignore_window_guard_switch).state ? "IGNORED" : "ACTIVE");
}

// ---------------------------------------------------------
// ESP-NOW DISCOVERY SEQUENCE (Priority -10, Phase 2)
// ---------------------------------------------------------

/**
 * @brief Executes the staggered ESP-NOW discovery broadcast sequence.
 *
 * After core initialization, we need to discover peer devices in the
 * same room. The discovery uses multiple staggered broadcasts because:
 *
 *   1. **First broadcast**: Basic announcement. Some peers may still
 *      be booting (e.g. after a power outage affecting all devices).
 *
 *   2. **Second broadcast** (+2s): Catches peers that were in their
 *      own boot sequence during the first broadcast. Also handles
 *      RF congestion that's common when multiple ESP32s boot
 *      simultaneously on the same channel.
 *
 *   3. **Third broadcast** (+2s): Final sweep. By now, all peers in
 *      the room should be online. This also serves as a "heartbeat"
 *      that triggers peers to send their current state back.
 *
 *   4. **Status request** (+1s): Explicitly asks all discovered peers
 *      for their current state, ensuring this device adopts the
 *      Master's configuration immediately rather than waiting for
 *      the next periodic sync.
 *
 * The delays between broadcasts are implemented in YAML (not here)
 * because ESPHome's `delay:` action is non-blocking and cooperative,
 * while a C++ delay would block the main loop.
 *
 * @param phase  Discovery phase number (1, 2, or 3) for logging.
 *               Phase 4 is the status request (separate function).
 *
 * @note Called from on_boot priority -10 in ventosync.yaml.
 *       The YAML orchestrates the timing:
 *       ```yaml
 *       - lambda: "boot_discovery_broadcast(1);"
 *       - delay: 2s
 *       - lambda: "boot_discovery_broadcast(2);"
 *       - delay: 2s
 *       - lambda: "boot_discovery_broadcast(3);"
 *       - delay: 1s
 *       - lambda: "boot_finish_discovery();"
 *       ```
 */
inline void boot_discovery_broadcast(int phase) {
    ESP_LOGI("boot", "Discovery broadcast %d/3 — scanning for room peers", phase);
    send_discovery_broadcast();
}

/**
 * @brief Completes the boot discovery sequence by requesting peer status.
 *
 * After all discovery broadcasts have been sent and peers have had
 * time to respond, this function requests the current operational
 * state from all known peers. This ensures the newly booted device
 * immediately synchronizes with the room's current Master state
 * (mode, intensity, direction) rather than operating independently
 * until the next periodic sync.
 *
 * @note Called as the final step of the on_boot -10 sequence.
 */
inline void boot_finish_discovery() {
    request_peer_status();
    ESP_LOGI("boot", "Phase 2 complete: Discovery finished, "
             "status requested from %u peer(s)",
             static_cast<unsigned>(peer_cache.size()));
}
