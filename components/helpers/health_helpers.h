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
// File:        health_helpers.h
// Description: System health monitoring, brain-freeze detection, and
//              watchdog management for 24/7 continuous operation.
// Author:      Thomas Engeroff
// Created:     2026-05-16
// Modified:    2026-05-16
//
// Dependencies: globals.h (watchdog_restarts_count)
//               VentilationController (ventilation_ctrl, last_loop_ms)
// ==========================================================================

#pragma once

#include "esphome.h"
#include <esp_task_wdt.h>
#include <esp_system.h>

// ---------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------

namespace ventosync {
namespace health {

/// Maximum allowed silence from VentilationController::loop() before
/// declaring a brain freeze. 10 minutes = 600'000 ms.
/// Rationale: The controller loop runs every ~100ms. Even with heavy
/// WiFi/ESP-NOW traffic, 10 minutes of silence is catastrophic.
static constexpr uint32_t BRAIN_FREEZE_TIMEOUT_MS = 600000;

/// Brief delay before esp_restart() to allow the UART TX buffer to flush
/// the critical error log message. This is the ONE acceptable use of
/// blocking delay() in the entire codebase – we're about to reboot anyway.
static constexpr uint32_t PRE_RESTART_FLUSH_DELAY_MS = 200;

/// Interval for the health check timer (must match YAML interval).
/// Documented here for cross-reference; the actual scheduling is in YAML.
static constexpr uint32_t HEALTH_CHECK_INTERVAL_S = 300;

}  // namespace health
}  // namespace ventosync

// ---------------------------------------------------------
// BRAIN FREEZE DETECTION
// ---------------------------------------------------------

/**
 * @brief Checks if the VentilationController main loop is still alive.
 *
 * The VentilationController custom component updates `last_loop_ms` on every
 * iteration of its `loop()` method. This function compares that timestamp
 * against the current time. If the gap exceeds BRAIN_FREEZE_TIMEOUT_MS,
 * the system is considered frozen and a controlled restart is triggered.
 *
 * Recovery sequence:
 *   1. Log CRITICAL error message
 *   2. Feed the hardware watchdog once (prevents WDT reset before our
 *      controlled restart, ensuring log output reaches UART)
 *   3. Brief delay for UART buffer flush
 *   4. esp_restart() – clean software reset
 *
 * @note The watchdog_restarts_count global is incremented before restart
 *       and persisted to flash (restore_value: yes), enabling post-mortem
 *       analysis via the HA sensor.
 *
 * @note Called every 300s from `health_check_timer` interval in
 *       logic_automation.yaml.
 */
inline void check_brain_health() {
    using namespace ventosync::health;

    auto *v = get_controller();
    const uint32_t now = millis();

    // ── Guard: Wait for the controller to complete its first loop ──
    // During boot, last_loop_ms is 0. We must not trigger a false positive
    // restart before the controller has even started.
    if (v->last_loop_ms == 0) {
        ESP_LOGD("health", "Controller not yet started, skipping health check");
        return;
    }

    // ── Calculate loop silence duration ──
    const uint32_t silence_ms = now - v->last_loop_ms;

    if (silence_ms > BRAIN_FREEZE_TIMEOUT_MS) {
        // ── CRITICAL: Main loop has stalled ──
        ESP_LOGE("health",
                 "CRITICAL: Brain freeze detected! Loop silent for %lu ms "
                 "(threshold: %lu ms). Forcing recovery restart.",
                 static_cast<unsigned long>(silence_ms),
                 static_cast<unsigned long>(BRAIN_FREEZE_TIMEOUT_MS));

        // Increment persistent restart counter for post-mortem diagnostics
        id(watchdog_restarts_count) += 1;
        ESP_LOGE("health", "Watchdog restart count: %d",
                 id(watchdog_restarts_count));

        // Feed WDT to prevent an uncontrolled WDT reset during our
        // controlled shutdown sequence
        esp_task_wdt_reset();

        // Allow UART TX buffer to flush the critical log messages.
        // This is the ONLY acceptable blocking delay in the codebase –
        // we are about to reboot anyway.
        delay(PRE_RESTART_FLUSH_DELAY_MS);

        // Controlled software restart
        esp_restart();
    }

    // ── Nominal: Log health status at DEBUG level ──
    ESP_LOGD("health", "Brain health OK: loop active %lu ms ago",
             static_cast<unsigned long>(silence_ms));
}

// ---------------------------------------------------------
// BOOT DIAGNOSTICS
// ---------------------------------------------------------

/**
 * @brief Logs boot diagnostics after a restart.
 *
 * Call this once during the boot sequence (e.g. from esphome.on_boot
 * at priority 600) to log whether the previous shutdown was clean or
 * triggered by the health watchdog.
 *
 * @note Uses esp_reset_reason() from ESP-IDF to distinguish between
 *       power-on, software reset, panic, brownout, etc.
 */
inline void log_boot_diagnostics() {
    const esp_reset_reason_t reason = esp_reset_reason();
    const int restart_count = id(watchdog_restarts_count);

    const char *reason_str;
    switch (reason) {
        case ESP_RST_POWERON:  reason_str = "Power-on";       break;
        case ESP_RST_SW:       reason_str = "Software reset";  break;
        case ESP_RST_PANIC:    reason_str = "Exception/panic"; break;
        case ESP_RST_INT_WDT:  reason_str = "Interrupt WDT";   break;
        case ESP_RST_TASK_WDT: reason_str = "Task WDT";        break;
        case ESP_RST_WDT:      reason_str = "Other WDT";       break;
        case ESP_RST_BROWNOUT: reason_str = "Brownout";        break;
        case ESP_RST_SDIO:     reason_str = "SDIO";            break;
        default:               reason_str = "Unknown";         break;
    }

    ESP_LOGI("health", "╔══════════════════════════════════════╗");
    ESP_LOGI("health", "║       VentoSync Boot Diagnostics     ║");
    ESP_LOGI("health", "╠══════════════════════════════════════╣");
    ESP_LOGI("health", "║  Reset reason : %-20s ║", reason_str);
    ESP_LOGI("health", "║  WD restarts  : %-20d ║", restart_count);
    ESP_LOGI("health", "║  Free heap    : %-15lu bytes ║",
             static_cast<unsigned long>(esp_get_free_heap_size()));
    ESP_LOGI("health", "╚══════════════════════════════════════╝");

    // Warn if we've had multiple watchdog restarts
    if (restart_count > 3) {
        ESP_LOGW("health",
                 "WARNING: %d watchdog restarts recorded. "
                 "Investigate root cause!",
                 restart_count);
    }
}
