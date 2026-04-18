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
// File:        simple_test_runner.cpp
// Description: Unit test runner for core ventilation logic.
// Author:      Thomas Engeroff
// Created:     2026-02-15
// Modified:    2026-03-23
// ==========================================================================

#include "../components/ventilation_logic/ventilation_logic.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>


// ============================================================
// Simple Test Framework
// ============================================================
#define TEST_ASSERT(cond)                                                      \
  if (!(cond)) {                                                               \
    std::cerr << "FAILED: " << #cond << " at line " << __LINE__ << std::endl;  \
    return false;                                                              \
  }

bool test_heat_recovery() {
  // T_in=20, T_out=0, T_supply=16 -> Eff = (16-0)/(20-0) = 80%
  float eff =
      VentilationLogic::calculate_heat_recovery_efficiency(20.0f, 16.0f, 0.0f);
  TEST_ASSERT(std::abs(eff - 80.0f) < 0.1f);

  // ΔT < 1 °C → 0 (Division-by-zero-Schutz)
  eff =
      VentilationLogic::calculate_heat_recovery_efficiency(20.0f, 16.0f, 20.0f);
  TEST_ASSERT(eff == 0.0f);

  // Ergebnis > 100 % (Sensor-Rauschen) → auf 100 % gekappt (Changelog:
  // Clamping)
  eff =
      VentilationLogic::calculate_heat_recovery_efficiency(20.0f, 25.0f, 0.0f);
  TEST_ASSERT(eff == 100.0f);

  // Negativer Wert → 0 % gekappt
  eff =
      VentilationLogic::calculate_heat_recovery_efficiency(20.0f, -5.0f, 0.0f);
  TEST_ASSERT(eff == 0.0f);

  return true;
}

bool test_ebmpapst_pwm_mapping() {
  // Stopp: speed=0 (< 0.05) → exakt 50 % egal welche Richtung
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_pwm(0.0f, 0) - 0.5f) <
              0.001f);
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_pwm(0.0f, 1) - 0.5f) <
              0.001f);

  // Soft-Stop-Grenze: speed=0.04 → 50 % (noch in Zone)
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_pwm(0.04f, 1) - 0.5f) <
              0.001f);

  // Richtung A (Abluft): Min Speed (0.1) -> 30% PWM
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_pwm(0.1f, 0) - 0.30f) <
              0.001f);
  // Richtung A (Abluft): Max Speed (1.0) -> 5% PWM
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_pwm(1.0f, 0) - 0.05f) <
              0.001f);
  // Richtung A: speed=0.55 (Mitte 0.1..1.0) -> 0.30 - 0.5 * 0.25 = 0.175
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_pwm(0.55f, 0) - 0.175f) <
              0.001f);

  // Richtung B (Zuluft): Min Speed (0.1) -> 70% PWM
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_pwm(0.1f, 1) - 0.70f) <
              0.001f);
  // Richtung B (Zuluft): Max Speed (1.0) -> 95% PWM
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_pwm(1.0f, 1) - 0.95f) <
              0.001f);

  return true;
}

bool test_min_speed_mapping() {
  // Stufe 1: Mindestdrehzahl 10 %
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_speed_from_intensity(1) -
                       0.10f) < 0.001f);

  // Stufe 10: 100 %
  TEST_ASSERT(
      std::abs(VentilationLogic::calculate_fan_speed_from_intensity(10) -
               1.00f) < 0.001f);

  // Stufe 5: Mittelwert = 0.005*(4^2) + 0.055*4 + 0.1 = 0.08 + 0.22 + 0.1 = 0.40
  TEST_ASSERT(std::abs(VentilationLogic::calculate_fan_speed_from_intensity(5) -
                        0.40f) < 0.001f);

  return true;
}

bool test_dynamic_cycle_duration() {
  TEST_ASSERT(VentilationLogic::calculate_dynamic_cycle_duration(1) == 70000);
  TEST_ASSERT(VentilationLogic::calculate_dynamic_cycle_duration(10) == 50000);
  // Level 5 (ca. Mitte) = 70000 - (4 * 2222) = 70000 - 8888 = 61111 (rounded to
  // 61000)
  TEST_ASSERT(VentilationLogic::calculate_dynamic_cycle_duration(5) == 61000);
  return true;
}

bool test_virtual_rpm_calculation() {
  // 100% speed, Zuluft, no ramp -> 4200 RPM
  TEST_ASSERT(
      std::abs(VentilationLogic::calculate_virtual_fan_rpm(1.0f, true, 1.0f) -
               4200.0f) < 0.1f);
  // 50% speed, Abluft, no ramp -> -2100 RPM
  TEST_ASSERT(
      std::abs(VentilationLogic::calculate_virtual_fan_rpm(0.5f, false, 1.0f) -
               (-2100.0f)) < 0.1f);
  // 100% speed, Zuluft, 50% ramp -> 2100 RPM
  TEST_ASSERT(
      std::abs(VentilationLogic::calculate_virtual_fan_rpm(1.0f, true, 0.5f) -
               2100.0f) < 0.1f);
  // Low speed (< 0.05) -> 0 RPM
  TEST_ASSERT(VentilationLogic::calculate_virtual_fan_rpm(0.04f, true, 1.0f) ==
              0.0f);

  return true;
}

bool test_fan_logic() {
  TEST_ASSERT(VentilationLogic::is_fan_slider_off(0.5f) == true);
  TEST_ASSERT(VentilationLogic::is_fan_slider_off(1.5f) == false);
  // Exakte Grenze (< 1.0)
  TEST_ASSERT(VentilationLogic::is_fan_slider_off(1.0f) == false);
  TEST_ASSERT(VentilationLogic::is_fan_slider_off(0.99f) == true);

  // Cycle check
  TEST_ASSERT(VentilationLogic::get_next_fan_level(1) == 2);
  TEST_ASSERT(VentilationLogic::get_next_fan_level(9) == 10);
  TEST_ASSERT(VentilationLogic::get_next_fan_level(10) == 1);
  return true;
}

// ============================================================
// [Unreleased] Ramp-Up / Ramp-Down Randfälle
// ============================================================
bool test_ramp_functions() {
  // Ramp Up: iteration 0 → 0.0, iteration 100 → 1.0
  TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_up(0) - 0.0f) < 0.001f);
  TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_up(50) - 0.5f) <
              0.001f);
  TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_up(100) - 1.0f) <
              0.001f);

  // Ramp Down: iteration 0 → 1.0, iteration 100 → 0.0
  TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_down(0) - 1.0f) <
              0.001f);
  TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_down(50) - 0.5f) <
              0.001f);
  TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_down(100) - 0.0f) <
              0.001f);

  // Symmetrie: ramp_up(i) + ramp_down(i) == 1.0
  for (int i = 0; i <= 100; i += 25) {
    float sum = VentilationLogic::calculate_ramp_up(i) +
                VentilationLogic::calculate_ramp_down(i);
    TEST_ASSERT(std::abs(sum - 1.0f) < 0.001f);
  }

  return true;
}

bool test_co2_logic() {
  // Classification
  TEST_ASSERT(VentilationLogic::get_co2_classification(400) == "Ausgezeichnet");
  TEST_ASSERT(VentilationLogic::get_co2_classification(700) == "Gut");
  TEST_ASSERT(VentilationLogic::get_co2_classification(900) == "Mäßig");
  TEST_ASSERT(VentilationLogic::get_co2_classification(1100) == "Erhöht");
  TEST_ASSERT(VentilationLogic::get_co2_classification(1300) == "Schlecht");
  TEST_ASSERT(VentilationLogic::get_co2_classification(1500) == "Inakzeptabel");

  // NaN-Klassifikation → "Unbekannt"
  TEST_ASSERT(VentilationLogic::get_co2_classification(
                  std::numeric_limits<float>::quiet_NaN()) == "Unbekannt");
  TEST_ASSERT(VentilationLogic::get_co2_classification(0.0f) == "Unbekannt");

  return true;
}

#include "../components/ventilation_group/ventilation_state_machine.h"
#include <limits>

// ============================================================
// [Unreleased] MODE_OFF — Lüfter gestoppt
// ============================================================
bool test_mode_off() {
  esphome::VentilationStateMachine sm;
  sm.setup();
  sm.set_mode(esphome::MODE_OFF, 0);

  esphome::HardwareState state = sm.get_target_state(0);
  TEST_ASSERT(state.fan_enabled == false);

  // Auch nach mehreren Updates bleibt der Lüfter aus
  sm.update(60000);
  state = sm.get_target_state(60000);
  TEST_ASSERT(state.fan_enabled == false);

  return true;
}

// ============================================================
// [Unreleased] MODE_VENTILATION mit Timer (get_remaining_duration)
// ============================================================
bool test_ventilation_timer() {
  esphome::VentilationStateMachine sm;
  sm.setup();

  uint32_t start = 1000;
  uint32_t duration_ms = 5 * 60 * 1000; // 5 Minuten
  sm.set_mode(esphome::MODE_VENTILATION, start, duration_ms);

  // Lüfter läuft direkt nach Start
  esphome::HardwareState state = sm.get_target_state(start);
  TEST_ASSERT(state.fan_enabled == true);

  // Verbleibende Zeit kurz nach Start ≈ 5 min
  uint32_t remaining = sm.get_remaining_duration(start + 1000);
  TEST_ASSERT(remaining > 0);
  TEST_ASSERT(remaining <= duration_ms);

  // Nach Timer-Ablauf: keine verbleibende Zeit mehr
  remaining = sm.get_remaining_duration(start + duration_ms + 1000);
  TEST_ASSERT(remaining == 0);

  // Infinite Timer (duration=0): get_remaining_duration gibt 0 zurück
  sm.set_mode(esphome::MODE_VENTILATION, start, 0);
  remaining = sm.get_remaining_duration(start + 10000);
  TEST_ASSERT(remaining == 0);

  return true;
}

// ============================================================
// [Unreleased] sync_time: Phasen-Offset aus ESP-NOW
// ============================================================
bool test_sync_time() {
  esphome::VentilationStateMachine sm;
  sm.setup();
  sm.cycle_duration_ms = 10000; // 10 s Halbzyklus für leichte Berechnung
  sm.is_phase_a = true;

  uint32_t now = 50000;

  // Sync auf Zielposition = 0 ms → Offset so setzen, dass Zyklus neu beginnt
  // get_cycle_pos(now + offset) soll ≈ 0 ergeben
  sm.sync_time(now, 0);
  uint32_t pos_after_sync = sm.get_cycle_pos(now);
  TEST_ASSERT(pos_after_sync < 10000); // muss innerhalb eines Halbzyklus liegen

  // Sync auf halbe Zyklusposition → Offset verschiebt Phase
  sm.sync_time(now, 5000);
  pos_after_sync = sm.get_cycle_pos(now);
  TEST_ASSERT(pos_after_sync <
              sm.cycle_duration_ms * 2); // innerhalb eines vollen Zyklus

  return true;
}

bool test_stosslueftung_cycle() {
  esphome::VentilationStateMachine sm;
  sm.setup();

  uint32_t start_time = 100000;
  sm.set_mode(esphome::MODE_STOSSLUEFTUNG, start_time);

  // 1. Initial State: Active (Fan ON)
  esphome::HardwareState state = sm.get_target_state(start_time);
  TEST_ASSERT(state.fan_enabled == true);
  TEST_ASSERT(sm.stoss_active_phase == true);

  // 2. Advance 14 minutes -> Still Active
  sm.update(start_time + 14 * 60 * 1000);
  state = sm.get_target_state(start_time + 14 * 60 * 1000);
  TEST_ASSERT(state.fan_enabled == true);

  // 3. Advance 16 minutes -> Pause (Fan OFF)
  sm.update(start_time + 16 * 60 * 1000);
  state = sm.get_target_state(start_time + 16 * 60 * 1000);
  TEST_ASSERT(state.fan_enabled == false);
  TEST_ASSERT(sm.stoss_active_phase == false);

  // 4. Advance 119 minutes (Total) -> Still Pause
  sm.update(start_time + 119 * 60 * 1000);
  state = sm.get_target_state(start_time + 119 * 60 * 1000);
  TEST_ASSERT(state.fan_enabled == false);

  // 5. Advance 121 minutes (Total) -> New Cycle (Fan ON, Direction Flipped)
  bool initial_direction = state.direction_in;
  // Note: We need to capture direction from active phase to compare

  sm.update(start_time + 121 * 60 * 1000);
  state = sm.get_target_state(start_time + 121 * 60 * 1000);
  TEST_ASSERT(state.fan_enabled == true);
  TEST_ASSERT(sm.stoss_active_phase == true);
  TEST_ASSERT(sm.stoss_direction_flip == true);

  return true;
}

bool test_phase_logic() {
  esphome::VentilationStateMachine sm;
  sm.setup();
  sm.cycle_duration_ms = 1000; // 1 second for easy math
  sm.is_phase_a = true;

  // t=0 -> Phase A Active (0-1000)
  // Group A (is_phase_a=true) should be IN
  sm.update(0);
  esphome::HardwareState state = sm.get_target_state(0);
  TEST_ASSERT(sm.global_phase == true);
  TEST_ASSERT(state.direction_in == true);

  // t=1100 -> Phase B Active (1000-2000)
  // Group A should be OUT
  sm.update(1100);
  state = sm.get_target_state(1100);
  TEST_ASSERT(sm.global_phase == false);
  TEST_ASSERT(state.direction_in == false);

  return true;
}

// ============================================================
// T-1: set_mode() — K-3 Fix Regression (Early-Return Logic)
// Validates that the removed 'changed' variable doesn't alter
// behavior, and that ventilation_start_time is correctly reset
// when duration changes within the same mode.
// ============================================================
bool test_set_mode_early_return() {
  esphome::VentilationStateMachine sm;
  sm.setup();

  // --- Fall 1: Gleicher Mode, gleiche Duration → no-op (Early-Return) ---
  uint32_t now = 10000;
  sm.set_mode(esphome::MODE_ECO_RECOVERY, now, 0);
  uint32_t saved_start = sm.ventilation_start_time;

  sm.set_mode(esphome::MODE_ECO_RECOVERY, now + 5000, 0);
  // Must be a no-op: ventilation_start_time must NOT change
  TEST_ASSERT(sm.ventilation_start_time == saved_start);
  TEST_ASSERT(sm.current_mode == esphome::MODE_ECO_RECOVERY);

  // --- Fall 2: Gleicher Mode (VENTILATION), andere Duration → Update ---
  // This is the key regression test for K-3:
  // ventilation_start_time MUST be reset when duration changes.
  now = 20000;
  sm.set_mode(esphome::MODE_VENTILATION, now, 30000);
  TEST_ASSERT(sm.ventilation_start_time == now);
  TEST_ASSERT(sm.ventilation_duration_ms == 30000);

  uint32_t now2 = now + 5000; // 5s later
  sm.set_mode(esphome::MODE_VENTILATION, now2, 60000);
  // Duration changed → must NOT early-return
  TEST_ASSERT(sm.ventilation_duration_ms == 60000);
  // ventilation_start_time must be refreshed to now2
  TEST_ASSERT(sm.ventilation_start_time == now2);

  // --- Fall 3: Anderer Mode, gleiche Duration → Update ---
  now = 30000;
  sm.set_mode(esphome::MODE_ECO_RECOVERY, now, 0);
  TEST_ASSERT(sm.current_mode == esphome::MODE_ECO_RECOVERY);

  sm.set_mode(esphome::MODE_VENTILATION, now + 1000, 0);
  TEST_ASSERT(sm.current_mode == esphome::MODE_VENTILATION);
  TEST_ASSERT(sm.ventilation_start_time == now + 1000);

  // --- Fall 4: MODE_OFF → kein Stoßlüftungs-Timer-Reset ---
  sm.stoss_cycle_start = 99999; // Arbitrary value
  sm.set_mode(esphome::MODE_OFF, 40000, 0);
  TEST_ASSERT(sm.current_mode == esphome::MODE_OFF);
  TEST_ASSERT(sm.stoss_cycle_start == 99999); // Must not be touched

  // --- Fall 5: MODE_STOSSLUEFTUNG → Timer und Flags korrekt ---
  sm.set_mode(esphome::MODE_STOSSLUEFTUNG, 50000, 0);
  TEST_ASSERT(sm.stoss_cycle_start == 50000);
  TEST_ASSERT(sm.stoss_active_phase == true);
  TEST_ASSERT(sm.stoss_direction_flip == false);

  return true;
}

// ============================================================
// T-2: get_cycle_pos() — K-4 Fix Regression (Div-by-Zero Guard)
// Validates that cycle_duration_ms == 0 returns 0 instead of
// crashing, and that the overflow guard for period = ms*2 works.
// ============================================================
bool test_get_cycle_pos_guards() {
  esphome::VentilationStateMachine sm;
  sm.setup();

  // --- Fall 1: cycle_duration_ms == 0 → return 0, no crash ---
  sm.cycle_duration_ms = 0;
  TEST_ASSERT(sm.get_cycle_pos(0) == 0);
  TEST_ASSERT(sm.get_cycle_pos(50000) == 0);
  TEST_ASSERT(sm.get_cycle_pos(UINT32_MAX) == 0);

  // --- Fall 2: Normalbetrieb → korrekte Position ---
  sm.cycle_duration_ms = 10000; // 10s half-cycle, 20s full period
  sm.time_offset_ms = 0;
  // pos = now % (10000 * 2) = now % 20000
  TEST_ASSERT(sm.get_cycle_pos(0) == 0);
  TEST_ASSERT(sm.get_cycle_pos(5000) == 5000);
  TEST_ASSERT(sm.get_cycle_pos(15000) == 15000);
  TEST_ASSERT(sm.get_cycle_pos(20000) == 0); // Wrap at period boundary
  TEST_ASSERT(sm.get_cycle_pos(25000) == 5000); // After one full period

  // --- Fall 3: Mit negativem time_offset ---
  sm.time_offset_ms = -3000;
  // raw_pos = 5000 + (-3000) = 2000, mod 20000 = 2000
  TEST_ASSERT(sm.get_cycle_pos(5000) == 2000);
  // raw_pos = 1000 + (-3000) = -2000, mod 20000 → -2000 + 20000 = 18000
  TEST_ASSERT(sm.get_cycle_pos(1000) == 18000);

  // --- Fall 4: Overflow guard (cycle_duration_ms nahe UINT32_MAX/2) ---
  // period = ms * 2 would overflow if ms > UINT32_MAX/2
  sm.cycle_duration_ms = (UINT32_MAX / 2) + 1; // 2147483648
  // period = 2147483648 * 2 = 4294967296 → overflows to 0
  // Guard: period < cycle_duration_ms → return 0
  TEST_ASSERT(sm.get_cycle_pos(50000) == 0);

  return true;
}

// ============================================================
// T-5: set_cycle_duration() — K-1 + H-3 Fix Regression
// Validates proportional position preservation, ms=0 guard,
// and the int64→int32 clamp for time_offset_ms.
// ============================================================
bool test_set_cycle_duration_guards() {
  esphome::VentilationStateMachine sm;
  sm.setup();

  // --- Fall 1: ms == 0 → Early-Return, no change ---
  sm.cycle_duration_ms = 70000;
  sm.set_cycle_duration(10000, 0);
  TEST_ASSERT(sm.cycle_duration_ms == 70000); // Unchanged

  // --- Fall 2: Same value → Early-Return ---
  sm.set_cycle_duration(10000, 70000);
  TEST_ASSERT(sm.cycle_duration_ms == 70000); // Still unchanged

  // --- Fall 3: First-time init (cycle_duration_ms == 0) ---
  sm.cycle_duration_ms = 0;
  sm.set_cycle_duration(10000, 50000);
  TEST_ASSERT(sm.cycle_duration_ms == 50000);

  // --- Fall 4: Proportional position — Phase A midpoint ---
  // Start with 70s cycle, position at midpoint of phase A
  sm.cycle_duration_ms = 70000;
  sm.time_offset_ms = 0;
  // At now=35000: get_cycle_pos(35000) = 35000 (midpoint of phase A)
  // Change to 50000ms cycle
  // Expected: old_pos=35000, old_half=70000, in phase A
  // new_pos = 35000 * 50000 / 70000 = 25000
  sm.set_cycle_duration(35000, 50000);
  uint32_t new_pos = sm.get_cycle_pos(35000);
  TEST_ASSERT(sm.cycle_duration_ms == 50000);
  // Position should be proportionally mapped (~25000)
  TEST_ASSERT(new_pos >= 24000 && new_pos <= 26000);

  // --- Fall 5: Proportional position — Phase B midpoint ---
  sm.cycle_duration_ms = 70000;
  sm.time_offset_ms = 0;
  // At now=105000: cycle_pos = 105000 % 140000 = 105000
  // progress_in_b = 105000 - 70000 = 35000 (midpoint of phase B)
  // new_pos = 50000 + (35000 * 50000 / 70000) = 50000 + 25000 = 75000
  sm.set_cycle_duration(105000, 50000);
  new_pos = sm.get_cycle_pos(105000);
  TEST_ASSERT(sm.cycle_duration_ms == 50000);
  // Should be ~75000 (midpoint of phase B in new cycle)
  TEST_ASSERT(new_pos >= 74000 && new_pos <= 76000);

  // --- Fall 6: Overflow guard — ms so large that ms*2 overflows ---
  sm.cycle_duration_ms = 50000; // Normal starting value
  uint32_t huge_ms = (UINT32_MAX / 2) + 100; // ms*2 would overflow
  sm.set_cycle_duration(10000, huge_ms);
  // Must be rejected by overflow guard → cycle_duration_ms unchanged
  TEST_ASSERT(sm.cycle_duration_ms == 50000);

  return true;
}

// ============================================================
// T-3: get_target_state() — K-2 Fix Regression (Ramp Overlap Guard)
// Validates that ramp_factor behaves correctly for normal cycles,
// is disabled (1.0f) for too-short cycles, and handles the
// boundary case (half == 2 × RAMP_DURATION_MS) correctly.
// ============================================================
bool test_ramp_factor_overlap_guard() {
  esphome::VentilationStateMachine sm;
  sm.setup();
  sm.set_mode(esphome::MODE_ECO_RECOVERY, 0);
  sm.is_phase_a = true;

  // --- Fall 1: Normal cycle (70s) — ramp active ---
  sm.cycle_duration_ms = 70000;
  sm.time_offset_ms = 0;

  // At very start of half-cycle (pos=0): ramp_factor should be 0.0
  esphome::HardwareState state = sm.get_target_state(0);
  TEST_ASSERT(state.fan_enabled == true);
  TEST_ASSERT(std::abs(state.ramp_factor - 0.0f) < 0.01f);

  // At 2.5s into half-cycle: ramp_factor = 2500/5000 = 0.5
  state = sm.get_target_state(2500);
  TEST_ASSERT(std::abs(state.ramp_factor - 0.5f) < 0.01f);

  // At 5s into half-cycle: ramp_factor should be ~1.0 (plateau)
  state = sm.get_target_state(5000);
  TEST_ASSERT(std::abs(state.ramp_factor - 1.0f) < 0.01f);

  // At mid-cycle (35s): should be full speed (plateau)
  state = sm.get_target_state(35000);
  TEST_ASSERT(std::abs(state.ramp_factor - 1.0f) < 0.01f);

  // At 5s before half-cycle end (65s): start ramp-down
  // phase_pos = 65000, half - RAMP = 65000, remaining = 5000
  // At exactly the boundary: phase_pos > 65000 triggers ramp-down
  state = sm.get_target_state(67500); // 2.5s before flip
  TEST_ASSERT(state.ramp_factor < 1.0f);
  TEST_ASSERT(state.ramp_factor > 0.0f);
  // Specifically: remaining = 70000 - 67500 = 2500, factor = 2500/5000 = 0.5
  TEST_ASSERT(std::abs(state.ramp_factor - 0.5f) < 0.01f);

  // --- Fall 2: Short cycle (8s < 2×5s) — ramp DISABLED ---
  sm.cycle_duration_ms = 8000; // 8s < 10s = 2 * RAMP_DURATION_MS
  sm.time_offset_ms = 0;

  // At any position: ramp_factor must be 1.0f (no ramping)
  state = sm.get_target_state(0);
  TEST_ASSERT(std::abs(state.ramp_factor - 1.0f) < 0.001f);

  state = sm.get_target_state(4000); // Midpoint
  TEST_ASSERT(std::abs(state.ramp_factor - 1.0f) < 0.001f);

  state = sm.get_target_state(7999); // Just before flip
  TEST_ASSERT(std::abs(state.ramp_factor - 1.0f) < 0.001f);

  // --- Fall 3: Exact boundary (10s == 2×5s) — ramp ACTIVE ---
  sm.cycle_duration_ms = 10000; // Exactly 2 * RAMP_DURATION_MS
  sm.time_offset_ms = 0;

  // pos=0: ramp-up start
  state = sm.get_target_state(0);
  TEST_ASSERT(std::abs(state.ramp_factor - 0.0f) < 0.01f);

  // pos=5000: ramp-up complete, but also ramp-down starts
  // (edge-to-edge, no plateau)
  state = sm.get_target_state(5000);
  // At pos=5000: 5000 < RAMP(5000) is false, and 5000 > (10000-5000=5000) is false
  // So neither ramp applies → stays at default 1.0f
  TEST_ASSERT(std::abs(state.ramp_factor - 1.0f) < 0.01f);

  // --- Fall 4: MODE_OFF — no ramp logic at all ---
  sm.cycle_duration_ms = 70000;
  sm.set_mode(esphome::MODE_OFF, 0);
  state = sm.get_target_state(2500);
  TEST_ASSERT(state.fan_enabled == false);
  TEST_ASSERT(std::abs(state.ramp_factor - 0.0f) < 0.001f);

  // --- Fall 5: MODE_VENTILATION — no ramp (continuous, no direction switching) ---
  sm.cycle_duration_ms = 70000;
  sm.set_mode(esphome::MODE_VENTILATION, 0);
  state = sm.get_target_state(2500);
  TEST_ASSERT(state.fan_enabled == true);
  TEST_ASSERT(std::abs(state.ramp_factor - 1.0f) < 0.001f);

  return true;
}

// ============================================================
// T-4: sync_time() — H-2 Fix Regression (Zero Guard + Jitter)
// Validates that sync_time() is a no-op when cycle_duration_ms=0,
// suppresses jitter below 500ms, and correctly applies corrections.
// ============================================================
bool test_sync_time_guards() {
  esphome::VentilationStateMachine sm;
  sm.setup();

  // --- Fall 1: cycle_duration_ms == 0 → no-op, no crash ---
  sm.cycle_duration_ms = 0;
  sm.time_offset_ms = 42; // Sentinel
  sm.sync_time(10000, 5000);
  TEST_ASSERT(sm.time_offset_ms == 42); // Must be unchanged

  // --- Fall 2: Small diff (≤ 500ms) → jitter suppression ---
  sm.cycle_duration_ms = 70000;
  sm.time_offset_ms = 0;
  uint32_t now = 50000;
  uint32_t my_pos = sm.get_cycle_pos(now); // = 50000
  // Peer reports pos 200ms ahead
  sm.sync_time(now, my_pos + 200);
  TEST_ASSERT(sm.time_offset_ms == 0); // Not corrected (|200| < 500)

  // Peer reports pos 499ms behind
  sm.sync_time(now, my_pos - 499);
  TEST_ASSERT(sm.time_offset_ms == 0); // Still not corrected

  // --- Fall 3: Larger diff (> 500ms) → correction applied ---
  sm.time_offset_ms = 0;
  my_pos = sm.get_cycle_pos(now);
  sm.sync_time(now, my_pos + 1000); // 1s ahead
  TEST_ASSERT(sm.time_offset_ms == 1000);

  // --- Fall 4: Verify corrected position matches target ---
  sm.time_offset_ms = 0;
  sm.cycle_duration_ms = 10000; // Simple period for calculation
  now = 50000;
  my_pos = sm.get_cycle_pos(now); // 50000 % 20000 = 10000
  uint32_t target_pos = 12000;
  sm.sync_time(now, target_pos);
  // After sync, our position should now match the target
  uint32_t synced_pos = sm.get_cycle_pos(now);
  TEST_ASSERT(synced_pos == target_pos);

  // --- Fall 5: Negative diff (we're ahead of peer) ---
  sm.time_offset_ms = 0;
  sm.cycle_duration_ms = 10000;
  now = 50000;
  my_pos = sm.get_cycle_pos(now); // 10000
  sm.sync_time(now, 8000); // Peer is 2s behind us
  // diff = 8000 - 10000 = -2000, |diff| > 500 → applied
  TEST_ASSERT(sm.time_offset_ms == -2000);

  return true;
}

// ============================================================
// T-6: Type Layout Verification — H-2/H-3 Header Fixes
// Validates enum underlying type and struct size after removing
// the dead needs_update field.
// ============================================================
bool test_type_layout_verification() {
  // --- H-2: VentilationMode must be uint8_t for packet compat ---
  TEST_ASSERT(sizeof(esphome::VentilationMode) == sizeof(uint8_t));
  TEST_ASSERT(sizeof(esphome::VentilationMode) == 1);

  // Spot-check that enum values fit in uint8_t
  TEST_ASSERT(esphome::MODE_OFF == 0);
  TEST_ASSERT(esphome::MODE_ECO_RECOVERY == 1);
  TEST_ASSERT(esphome::MODE_VENTILATION == 2);
  TEST_ASSERT(esphome::MODE_STOSSLUEFTUNG == 3);

  // --- H-3: HardwareState no longer contains needs_update ---
  // Expected layout: bool + bool + float (+padding) = typically 8 bytes
  // With needs_update it was 12 bytes (bool+bool+float+bool+padding)
  // This just checks it doesn't contain the old extra field
  esphome::HardwareState hs;
  hs.fan_enabled = true;
  hs.direction_in = false;
  hs.ramp_factor = 0.75f;
  // If this compiles, needs_update field is successfully removed
  // Verify struct is smaller than it was with the bool field
  TEST_ASSERT(sizeof(esphome::HardwareState) <= 8);

  // --- Verify DEFAULT_CYCLE_DURATION_MS is accessible and correct ---
  TEST_ASSERT(esphome::VentilationStateMachine::DEFAULT_CYCLE_DURATION_MS == 70000);
  TEST_ASSERT(esphome::VentilationStateMachine::RAMP_DURATION_MS == 5000);
  TEST_ASSERT(esphome::VentilationStateMachine::DEFAULT_CYCLE_DURATION_MS >=
              2 * esphome::VentilationStateMachine::RAMP_DURATION_MS);

  return true;
}

int main() {
  std::cout << "Running VentilationLogic Tests..." << std::endl;

  bool all_passed = true;
  if (test_co2_logic()) {
    std::cout << "[PASS] CO2 Logic" << std::endl;
  } else {
    std::cout << "[FAIL] CO2 Logic" << std::endl;
    all_passed = false;
  }
  if (test_heat_recovery()) {
    std::cout << "[PASS] Heat Recovery" << std::endl;
  } else {
    std::cout << "[FAIL] Heat Recovery" << std::endl;
    all_passed = false;
  }
  if (test_fan_logic()) {
    std::cout << "[PASS] Fan Logic" << std::endl;
  } else {
    std::cout << "[FAIL] Fan Logic" << std::endl;
    all_passed = false;
  }
  if (test_ramp_functions()) {
    std::cout << "[PASS] Ramp Up/Down" << std::endl;
  } else {
    std::cout << "[FAIL] Ramp Up/Down" << std::endl;
    all_passed = false;
  }

  std::cout << "Running [Unreleased] Tests..." << std::endl;
  if (test_ebmpapst_pwm_mapping()) {
    std::cout << "[PASS] ebm-papst Single-PWM Mapping" << std::endl;
  } else {
    std::cout << "[FAIL] ebm-papst Single-PWM Mapping" << std::endl;
    all_passed = false;
  }
  if (test_min_speed_mapping()) {
    std::cout << "[PASS] Mindestdrehzahl Stufe 1 (10%)" << std::endl;
  } else {
    std::cout << "[FAIL] Mindestdrehzahl Stufe 1 (10%)" << std::endl;
    all_passed = false;
  }
  if (test_dynamic_cycle_duration()) {
    std::cout << "[PASS] Dynamic Cycle Duration" << std::endl;
  } else {
    std::cout << "[FAIL] Dynamic Cycle Duration" << std::endl;
    all_passed = false;
  }
  if (test_virtual_rpm_calculation()) {
    std::cout << "[PASS] Virtual RPM Calculation" << std::endl;
  } else {
    std::cout << "[FAIL] Virtual RPM Calculation" << std::endl;
    all_passed = false;
  }

  std::cout << "Running VentilationStateMachine Tests..." << std::endl;
  if (test_mode_off()) {
    std::cout << "[PASS] Mode OFF" << std::endl;
  } else {
    std::cout << "[FAIL] Mode OFF" << std::endl;
    all_passed = false;
  }
  if (test_ventilation_timer()) {
    std::cout << "[PASS] Ventilation Timer" << std::endl;
  } else {
    std::cout << "[FAIL] Ventilation Timer" << std::endl;
    all_passed = false;
  }
  if (test_sync_time()) {
    std::cout << "[PASS] Sync Time (ESP-NOW)" << std::endl;
  } else {
    std::cout << "[FAIL] Sync Time (ESP-NOW)" << std::endl;
    all_passed = false;
  }
  if (test_stosslueftung_cycle()) {
    std::cout << "[PASS] Stoßlüftung Cycle" << std::endl;
  } else {
    std::cout << "[FAIL] Stoßlüftung Cycle" << std::endl;
    all_passed = false;
  }
  if (test_phase_logic()) {
    std::cout << "[PASS] Phase Logic" << std::endl;
  } else {
    std::cout << "[FAIL] Phase Logic" << std::endl;
    all_passed = false;
  }

  std::cout << "Running Code Review Regression Tests..." << std::endl;
  if (test_set_mode_early_return()) {
    std::cout << "[PASS] T-1: set_mode() Early-Return (K-3 Fix)" << std::endl;
  } else {
    std::cout << "[FAIL] T-1: set_mode() Early-Return (K-3 Fix)" << std::endl;
    all_passed = false;
  }
  if (test_get_cycle_pos_guards()) {
    std::cout << "[PASS] T-2: get_cycle_pos() Div-by-Zero Guard (K-4 Fix)" << std::endl;
  } else {
    std::cout << "[FAIL] T-2: get_cycle_pos() Div-by-Zero Guard (K-4 Fix)" << std::endl;
    all_passed = false;
  }
  if (test_set_cycle_duration_guards()) {
    std::cout << "[PASS] T-5: set_cycle_duration() Overflow + Proportional (K-1/H-3 Fix)" << std::endl;
  } else {
    std::cout << "[FAIL] T-5: set_cycle_duration() Overflow + Proportional (K-1/H-3 Fix)" << std::endl;
    all_passed = false;
  }
  if (test_ramp_factor_overlap_guard()) {
    std::cout << "[PASS] T-3: get_target_state() Ramp Overlap Guard (K-2 Fix)" << std::endl;
  } else {
    std::cout << "[FAIL] T-3: get_target_state() Ramp Overlap Guard (K-2 Fix)" << std::endl;
    all_passed = false;
  }
  if (test_sync_time_guards()) {
    std::cout << "[PASS] T-4: sync_time() Zero Guard + Jitter (H-2 Fix)" << std::endl;
  } else {
    std::cout << "[FAIL] T-4: sync_time() Zero Guard + Jitter (H-2 Fix)" << std::endl;
    all_passed = false;
  }
  if (test_type_layout_verification()) {
    std::cout << "[PASS] T-6: Type Layout (enum : uint8_t + HardwareState)" << std::endl;
  } else {
    std::cout << "[FAIL] T-6: Type Layout (enum : uint8_t + HardwareState)" << std::endl;
    all_passed = false;
  }

  if (all_passed) {
    std::cout << "\nALL TESTS PASSED!" << std::endl;
    return 0;
  } else {
    std::cout << "\nSOME TESTS FAILED!" << std::endl;
    return 1;
  }
}
