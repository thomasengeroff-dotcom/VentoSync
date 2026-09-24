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
#include "../components/ventilation_logic/hvac_coordinator.h"
#include "../components/ventilation_logic/room_fusion.h"
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
// [0.10.13] Smart Climate Control — HVAC Coordinator
// Pure state machine from components/ventilation_logic/hvac_coordinator.h
// ============================================================
namespace {
ventosync::hvac::Inputs hvac_inputs(bool enabled, bool ac_on, float co2, uint32_t now) {
  ventosync::hvac::Inputs in;
  in.enabled = enabled;
  in.ha_connected = true;
  in.ac_has_state = true;
  in.ac_reported_active = ac_on;
  in.co2_ppm = co2;
  in.indoor_rh_percent = 50.0f;
  in.ventilation_can_dry = true;
  in.now_ms = now;
  return in;
}
} // namespace

// T-7a: Disabled switch → Smart-Automatik untouched, even with AC on
bool test_hvac_disabled_is_transparent() {
  using namespace ventosync::hvac;
  Coordinator c;
  Decision d = c.evaluate(hvac_inputs(false, true, 1800.0f, 0));
  TEST_ASSERT(d.state == State::DISABLED);
  TEST_ASSERT(!d.restrict_levels);
  TEST_ASSERT(!d.relaxed_co2_setpoint);
  TEST_ASSERT(!d.suppress_humidity);
  TEST_ASSERT(!d.lock_eco_mode);
  TEST_ASSERT(!d.ac_active);
  return true;
}

// T-7b: Enabled, AC off → STANDBY, no restrictions
bool test_hvac_standby_when_ac_off() {
  using namespace ventosync::hvac;
  Coordinator c;
  Decision d = c.evaluate(hvac_inputs(true, false, 900.0f, 0));
  TEST_ASSERT(d.state == State::STANDBY);
  TEST_ASSERT(!d.restrict_levels);
  TEST_ASSERT(!d.lock_eco_mode);
  TEST_ASSERT(!d.suppress_humidity);
  return true;
}

// T-7c: AC on → THROTTLED: CO2-only, relaxed setpoint, level cap [1..3], ECO lock
bool test_hvac_throttled_profile() {
  using namespace ventosync::hvac;
  Coordinator c;
  Decision d = c.evaluate(hvac_inputs(true, true, 900.0f, 0));
  TEST_ASSERT(d.state == State::THROTTLED);
  TEST_ASSERT(d.ac_active);
  TEST_ASSERT(d.restrict_levels);
  TEST_ASSERT(d.min_level == MIN_FAN_LEVEL);
  TEST_ASSERT(d.max_level == DEFAULT_MAX_FAN_LEVEL);
  TEST_ASSERT(d.relaxed_co2_setpoint);
  TEST_ASSERT(std::abs(d.co2_setpoint - DEFAULT_CO2_THRESHOLD_PPM) < 0.01f);
  TEST_ASSERT(d.suppress_humidity);
  TEST_ASSERT(d.lock_eco_mode);

  // Configurable cap is honoured and clamped to hardware range
  ventosync::hvac::Inputs in = hvac_inputs(true, true, 900.0f, 0);
  in.max_fan_level = 5;
  in.co2_threshold_ppm = 1100.0f;
  d = c.evaluate(in);
  TEST_ASSERT(d.max_level == 5);
  TEST_ASSERT(std::abs(d.co2_setpoint - 1100.0f) < 0.01f);
  in.max_fan_level = 42;
  d = c.evaluate(in);
  TEST_ASSERT(d.max_level == MAX_FAN_LEVEL_CONFIG_MAX); // clamped to the slider range (1-5)
  in.max_fan_level = 0;
  d = c.evaluate(in);
  TEST_ASSERT(d.max_level == MIN_FAN_LEVEL);
  return true;
}

// T-7d: CO2 emergency with hysteresis (enter ≥ 1500, release ≤ 1200)
bool test_hvac_co2_emergency_hysteresis() {
  using namespace ventosync::hvac;
  Coordinator c;
  Decision d = c.evaluate(hvac_inputs(true, true, 1400.0f, 0));
  TEST_ASSERT(d.state == State::THROTTLED);

  d = c.evaluate(hvac_inputs(true, true, 1500.0f, 10));
  TEST_ASSERT(d.state == State::EMERGENCY_CO2);
  TEST_ASSERT(!d.restrict_levels);        // Level cap lifted
  TEST_ASSERT(!d.relaxed_co2_setpoint);   // Normal user setpoint again
  TEST_ASSERT(!d.suppress_humidity);      // Full dual-PID again
  TEST_ASSERT(d.lock_eco_mode);           // Still no summer bypass while AC runs

  // Between 1200 and 1500 the emergency latch holds
  d = c.evaluate(hvac_inputs(true, true, 1350.0f, 20));
  TEST_ASSERT(d.state == State::EMERGENCY_CO2);

  // Released once back at/below the relaxed setpoint
  d = c.evaluate(hvac_inputs(true, true, 1200.0f, 30));
  TEST_ASSERT(d.state == State::THROTTLED);
  return true;
}

// T-7e: Emergency threshold is forced ≥ setpoint + 100 ppm
bool test_hvac_emergency_margin_guard() {
  using namespace ventosync::hvac;
  Coordinator c;
  ventosync::hvac::Inputs in = hvac_inputs(true, true, 1450.0f, 0);
  in.co2_threshold_ppm = 1400.0f;
  in.emergency_co2_ppm = 1200.0f; // Misconfigured: below the setpoint
  Decision d = c.evaluate(in);
  TEST_ASSERT(d.state == State::THROTTLED); // 1450 < 1400 + 100 → not yet emergency
  in.co2_ppm = 1500.0f;
  d = c.evaluate(in);
  TEST_ASSERT(d.state == State::EMERGENCY_CO2);
  return true;
}

// T-7f: AC "off" is debounced (compressor cycling must not toggle the fan)
bool test_hvac_ac_release_delay() {
  using namespace ventosync::hvac;
  Coordinator c;
  Decision d = c.evaluate(hvac_inputs(true, true, 900.0f, 1000));
  TEST_ASSERT(d.state == State::THROTTLED);

  // AC reports off → still throttled until the release delay elapsed
  d = c.evaluate(hvac_inputs(true, false, 900.0f, 2000));
  TEST_ASSERT(d.state == State::THROTTLED);
  TEST_ASSERT(d.ac_active);
  d = c.evaluate(hvac_inputs(true, false, 900.0f, 2000 + AC_RELEASE_DELAY_MS - 1));
  TEST_ASSERT(d.state == State::THROTTLED);

  // Short compressor pause followed by "on" resets the timer
  d = c.evaluate(hvac_inputs(true, true, 900.0f, 2000 + AC_RELEASE_DELAY_MS));
  TEST_ASSERT(d.state == State::THROTTLED);
  d = c.evaluate(hvac_inputs(true, false, 900.0f, 3000 + AC_RELEASE_DELAY_MS));
  TEST_ASSERT(d.state == State::THROTTLED);

  // Continuous "off" for the full delay → STANDBY
  d = c.evaluate(hvac_inputs(true, false, 900.0f, 3000 + 2 * AC_RELEASE_DELAY_MS));
  TEST_ASSERT(d.state == State::STANDBY);
  TEST_ASSERT(!d.ac_active);
  return true;
}

// T-7g: Unknown AC state (HA offline / entity unavailable) is fail-safe = inactive
bool test_hvac_unknown_ac_state_is_inactive() {
  using namespace ventosync::hvac;
  Coordinator c;
  ventosync::hvac::Inputs in = hvac_inputs(true, true, 900.0f, 0);
  in.ac_has_state = false;
  Decision d = c.evaluate(in);
  TEST_ASSERT(d.state == State::STANDBY);

  in = hvac_inputs(true, true, 900.0f, 0);
  in.ha_connected = false;
  d = c.evaluate(in);
  TEST_ASSERT(d.state == State::STANDBY);
  return true;
}

// T-7h: No CO2 reading → health cannot be guaranteed → no throttling
bool test_hvac_suspended_without_co2() {
  using namespace ventosync::hvac;
  Coordinator c;
  Decision d = c.evaluate(hvac_inputs(true, true, std::numeric_limits<float>::quiet_NaN(), 0));
  TEST_ASSERT(d.state == State::SUSPENDED_NO_CO2);
  TEST_ASSERT(!d.restrict_levels);
  TEST_ASSERT(!d.suppress_humidity);
  TEST_ASSERT(d.lock_eco_mode); // AC still active → no summer bypass
  return true;
}

// T-7i: Mold guard — high indoor rH lifts the restrictions only when ventilation can dry
bool test_hvac_mold_guard() {
  using namespace ventosync::hvac;
  Coordinator c;
  ventosync::hvac::Inputs in = hvac_inputs(true, true, 900.0f, 0);
  in.indoor_rh_percent = 72.0f;
  in.ventilation_can_dry = false; // Outdoor air muggier → ventilating would not help
  Decision d = c.evaluate(in);
  TEST_ASSERT(d.state == State::THROTTLED);

  in.ventilation_can_dry = true;
  d = c.evaluate(in);
  TEST_ASSERT(d.state == State::EMERGENCY_HUMIDITY);
  TEST_ASSERT(!d.restrict_levels);
  TEST_ASSERT(!d.suppress_humidity);

  // Hysteresis: 67 % keeps the latch, 65 % releases it
  in.indoor_rh_percent = 67.0f;
  d = c.evaluate(in);
  TEST_ASSERT(d.state == State::EMERGENCY_HUMIDITY);
  in.indoor_rh_percent = 65.0f;
  d = c.evaluate(in);
  TEST_ASSERT(d.state == State::THROTTLED);

  // CO2 emergency has priority over the mold guard in the reported state
  in.indoor_rh_percent = 80.0f;
  in.co2_ppm = 1600.0f;
  d = c.evaluate(in);
  TEST_ASSERT(d.state == State::EMERGENCY_CO2);
  return true;
}

// Minimal stand-in for esphome::PeerState (only the fields the fusion reads)
struct TestPeer {
  uint32_t last_seen_ms;
  float pid_demand;
  float room_co2;
  float room_humidity;
  float room_temp;
  bool hvac_ac_active = false;
  bool window_open = false;
  bool presence = false;
};

// T-7k: Room-wide CO2 fusion — max of local + fresh peers, stale/mock values rejected
bool test_room_co2_fusion() {
  using namespace ventosync::room;
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const uint32_t now = 1000000u;
  std::vector<TestPeer> peers = {
    {now - 1000u, nan, 1400.0f, nan, nan},                        // fresh, high
    {now - PEER_DATA_MAX_AGE_MS - 1u, nan, 2500.0f, nan, nan},    // stale → ignored
    {now - 500u, nan, 0.0f, nan, nan},                            // mock sensor → ignored
  };
  // Peer higher than local → peer wins
  TEST_ASSERT(std::abs(room_max_co2(900.0f, peers, now) - 1400.0f) < 0.01f);
  // Local higher than peer → local wins
  TEST_ASSERT(std::abs(room_max_co2(1600.0f, peers, now) - 1600.0f) < 0.01f);
  // No local sensor → fresh peer
  TEST_ASSERT(std::abs(room_max_co2(nan, peers, now) - 1400.0f) < 0.01f);
  // Only stale / invalid peers and no local → NaN
  peers.erase(peers.begin());
  TEST_ASSERT(std::isnan(room_max_co2(nan, peers, now)));
  TEST_ASSERT(std::isnan(room_max_co2(0.0f, peers, now)));
  // millis() wrap-around: peer seen just before the overflow is still fresh
  std::vector<TestPeer> wrap = {{0xFFFFFF00u, nan, 1200.0f, nan, nan}};
  TEST_ASSERT(std::abs(room_max_co2(nan, wrap, 0x00000100u) - 1200.0f) < 0.01f);

  // Room CO2 feeds the coordinator: device without sensor still throttles / escalates
  using namespace ventosync::hvac;
  std::vector<TestPeer> room = {{now - 1000u, nan, 1000.0f, nan, nan}};
  Coordinator c;
  ventosync::hvac::Inputs in = hvac_inputs(true, true, room_max_co2(nan, room, now), 0);
  TEST_ASSERT(c.evaluate(in).state == State::THROTTLED);
  room[0].room_co2 = 1600.0f;
  in.co2_ppm = room_max_co2(nan, room, now);
  TEST_ASSERT(c.evaluate(in).state == State::EMERGENCY_CO2);
  in.co2_ppm = room_max_co2(nan, room, now + PEER_DATA_MAX_AGE_MS * 2);
  TEST_ASSERT(c.evaluate(in).state == State::SUSPENDED_NO_CO2);
  return true;
}

// T-7l: Room-wide demand fusion — no feedback loop, stale demand expires
bool test_room_demand_fusion() {
  using namespace ventosync::room;
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const uint32_t now = 1000000u;

  // max_valid ignores NaN operands
  TEST_ASSERT(std::abs(max_valid(nan, 0.3f) - 0.3f) < 1e-6f);
  TEST_ASSERT(std::abs(max_valid(0.7f, nan) - 0.7f) < 1e-6f);
  TEST_ASSERT(std::isnan(max_valid(nan, nan)));

  // Highest fresh peer demand wins; NaN (no sensor) and stale peers ignored; clamped to 1.0
  std::vector<TestPeer> peers = {
    {now - 1000u, 0.2f, nan, nan, nan},
    {now - 2000u, nan, nan, nan, nan},                          // sensorless peer
    {now - PEER_DATA_MAX_AGE_MS - 1u, 0.9f, nan, nan, nan},     // stale
  };
  TEST_ASSERT(std::abs(room_max_peer_demand(peers, now) - 0.2f) < 1e-6f);
  peers[0].pid_demand = 1.7f;
  TEST_ASSERT(std::abs(room_max_peer_demand(peers, now) - 1.0f) < 1e-6f);
  peers[0].pid_demand = -0.1f;
  TEST_ASSERT(std::isnan(room_max_peer_demand(peers, now)));

  // Feedback-loop regression (0.10.20 bug): sensor device S and sensorless
  // master M. Each device broadcasts only its LOCAL demand, so once S's CO2
  // drops, M follows within one heartbeat and nothing latches at 1.0.
  float s_local = 1.0f;          // S: CO2 spike
  const float m_local = nan;     // M: no sensors → broadcasts NaN
  for (int round = 0; round < 3; ++round) {
    std::vector<TestPeer> seen_by_m = {{now, s_local, nan, nan, nan}};
    std::vector<TestPeer> seen_by_s = {{now, m_local, nan, nan, nan}};
    const float m_eff = max_valid(m_local, room_max_peer_demand(seen_by_m, now));
    const float s_eff = max_valid(s_local, room_max_peer_demand(seen_by_s, now));
    if (round == 0) {
      TEST_ASSERT(std::abs(m_eff - 1.0f) < 1e-6f);  // M adopts the spike
    } else {
      TEST_ASSERT(std::abs(m_eff - 0.1f) < 1e-6f);  // M follows the drop
      TEST_ASSERT(std::abs(s_eff - 0.1f) < 1e-6f);  // S is not pulled back up
    }
    s_local = 0.1f;              // CO2 back to normal after the first round
  }
  return true;
}

// T-7m: Room-wide humidity for the mold guard — wettest spot with matching temperature
bool test_room_humidity_fusion() {
  using namespace ventosync::room;
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const uint32_t now = 1000000u;
  std::vector<TestPeer> peers = {
    {now - 1000u, nan, nan, 72.0f, 19.5f},
    {now - PEER_DATA_MAX_AGE_MS - 1u, nan, nan, 95.0f, 18.0f},  // stale
    {now - 1000u, nan, nan, 120.0f, 20.0f},                     // implausible
  };
  // Device without humidity sensor uses the peer value and the peer's temperature
  HumiditySource h = room_max_humidity(nan, 22.0f, peers, now);
  TEST_ASSERT(std::abs(h.rh_percent - 72.0f) < 0.01f);
  TEST_ASSERT(std::abs(h.temp_c - 19.5f) < 0.01f);
  TEST_ASSERT(h.from_peer);
  // Wetter local sensor wins and keeps the local temperature
  h = room_max_humidity(80.0f, 22.0f, peers, now);
  TEST_ASSERT(std::abs(h.rh_percent - 80.0f) < 0.01f);
  TEST_ASSERT(std::abs(h.temp_c - 22.0f) < 0.01f);
  TEST_ASSERT(!h.from_peer);
  // Nothing usable → NaN
  peers.erase(peers.begin());
  h = room_max_humidity(nan, 22.0f, peers, now);
  TEST_ASSERT(std::isnan(h.rh_percent));
  return true;
}

// T-7n: AC state pushed by HA — expiry, API link and room-wide peer flag
bool test_hvac_ac_state_sources() {
  using namespace ventosync::hvac;
  Coordinator c;
  // Fresh local push → throttled
  Inputs in = hvac_inputs(true, true, 900.0f, 0);
  in.ac_state_age_ms = AC_STATE_MAX_AGE_MS;
  TEST_ASSERT(local_ac_active(in));
  TEST_ASSERT(c.evaluate(in).state == State::THROTTLED);
  // Expired push → treated as inactive (release after the debounce delay)
  in.ac_state_age_ms = AC_STATE_MAX_AGE_MS + 1;
  TEST_ASSERT(!local_ac_active(in));
  in.now_ms = 1000;
  TEST_ASSERT(c.evaluate(in).state == State::THROTTLED); // debounce running
  in.now_ms = 1000 + AC_RELEASE_DELAY_MS;
  TEST_ASSERT(c.evaluate(in).state == State::STANDBY);
  // API link down → local state ignored
  in = hvac_inputs(true, true, 900.0f, 0);
  in.ha_connected = false;
  TEST_ASSERT(!local_ac_active(in));
  // ...but a fresh peer reporting its own HA AC state keeps the room throttled
  in.peer_ac_active = true;
  Coordinator c2;
  TEST_ASSERT(c2.evaluate(in).state == State::THROTTLED);
  // Never pushed → inactive
  in = hvac_inputs(true, false, 900.0f, 0);
  in.ac_has_state = false;
  Coordinator c3;
  TEST_ASSERT(c3.evaluate(in).state == State::STANDBY);

  // Peer AC flag fusion: only fresh peers count
  const uint32_t now = 1000000u;
  std::vector<TestPeer> peers = {{now - 1000u, 0.0f, 0.0f, 0.0f, 0.0f, false}};
  TEST_ASSERT(!ventosync::room::any_fresh_peer_ac_active(peers, now));
  peers.push_back({now - ventosync::room::PEER_DATA_MAX_AGE_MS - 1u, 0.0f, 0.0f, 0.0f, 0.0f, true});
  TEST_ASSERT(!ventosync::room::any_fresh_peer_ac_active(peers, now)); // stale
  peers.push_back({now - 2000u, 0.0f, 0.0f, 0.0f, 0.0f, true});
  TEST_ASSERT(ventosync::room::any_fresh_peer_ac_active(peers, now));
  return true;
}

// T-7o: Config values are clamped to the HA slider ranges
bool test_hvac_config_ranges() {
  using namespace ventosync::hvac;
  Coordinator c;
  Inputs in = hvac_inputs(true, true, 900.0f, 0);
  in.co2_threshold_ppm = 5000.0f;   // e.g. from an old peer / NVS leftover
  in.emergency_co2_ppm = 400.0f;
  Decision d = c.evaluate(in);
  TEST_ASSERT(std::abs(d.co2_setpoint - static_cast<float>(CO2_THRESHOLD_MAX_PPM)) < 0.01f);
  in.co2_threshold_ppm = 100.0f;
  d = c.evaluate(in);
  TEST_ASSERT(std::abs(d.co2_setpoint - static_cast<float>(CO2_THRESHOLD_MIN_PPM)) < 0.01f);
  TEST_ASSERT(in_range(1200, CO2_THRESHOLD_MIN_PPM, CO2_THRESHOLD_MAX_PPM));
  TEST_ASSERT(!in_range(5000, CO2_THRESHOLD_MIN_PPM, CO2_THRESHOLD_MAX_PPM));
  TEST_ASSERT(!in_range(6, MAX_FAN_LEVEL_CONFIG_MIN, MAX_FAN_LEVEL_CONFIG_MAX));
  // Emergency below the minimum is lifted to EMERGENCY_CO2_MIN_PPM (and the margin)
  in.co2_threshold_ppm = 1000.0f;
  in.emergency_co2_ppm = 400.0f;
  in.co2_ppm = static_cast<float>(EMERGENCY_CO2_MIN_PPM) - 1.0f;
  Coordinator c2;
  TEST_ASSERT(c2.evaluate(in).state == State::THROTTLED);
  in.co2_ppm = static_cast<float>(EMERGENCY_CO2_MIN_PPM);
  TEST_ASSERT(c2.evaluate(in).state == State::EMERGENCY_CO2);

  // Fusion window follows the ESP-NOW heartbeat interval
  using ventosync::room::fusion_max_age_ms;
  TEST_ASSERT(fusion_max_age_ms(60000u, 900000u) == ventosync::room::PEER_DATA_MAX_AGE_MS);
  TEST_ASSERT(fusion_max_age_ms(300000u, 900000u) == 630000u);
  TEST_ASSERT(fusion_max_age_ms(21600000u, 900000u) == 900000u); // capped at the peer timeout
  return true;
}

// T-7p: Auto level mapping — window always enforced (HVAC cap regression)
bool test_auto_target_level() {
  // Normal mapping 2..7
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(0.0f, 2, 2, 7) == 2);
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(1.0f, 2, 2, 7) == 7);
  // Hysteresis: small demand change around level 4 (center 0.4) holds
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(0.45f, 4, 2, 7) == 4);
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(0.70f, 4, 2, 7) == 6);
  // Regression: running at 6, AC cap shrinks the window to 1..3 — the hold
  // path used to return the unclamped level 6 for any demand >= 0.625
  for (float demand : {0.0f, 0.3f, 0.63f, 0.8f, 1.0f}) {
    const int t = VentilationLogic::calculate_auto_target_level(demand, 6, 1, 3);
    TEST_ASSERT(t >= 1 && t <= 3);
  }
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(0.8f, 6, 1, 3) == 3);
  // Window grows back (AC off: min 1 -> 2): level 1 is lifted into the window
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(0.0f, 1, 2, 7) == 2);
  // Degenerate / invalid inputs
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(0.5f, 5, 3, 3) == 3);
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(0.5f, 5, 7, 2) >= 2);  // swapped window
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(std::numeric_limits<float>::quiet_NaN(), 5, 2, 7) >= 2);
  TEST_ASSERT(VentilationLogic::calculate_auto_target_level(5.0f, 2, 2, 7) == 7); // demand clamped
  return true;
}

// T-7q: Window Guard inputs — HA push expiry / API link and room-wide peer flag
bool test_window_guard_inputs() {
  using namespace ventosync::room;
  HaPushedFlag w;
  // Never pushed -> closed
  TEST_ASSERT(!w.active(1000u, true));
  TEST_ASSERT(w.age_ms(1000u) == UINT32_MAX);
  // Push "open": change reported, active while fresh and connected
  TEST_ASSERT(w.set(true, 1000u));
  TEST_ASSERT(!w.set(true, 2000u)); // same value -> no change
  TEST_ASSERT(w.active(2000u + HA_PUSH_MAX_AGE_MS, true));
  // Expired or API down -> reads as closed (fan never stays stopped forever)
  TEST_ASSERT(!w.active(2001u + HA_PUSH_MAX_AGE_MS, true));
  TEST_ASSERT(!w.active(3000u, false));
  // Wrap-around of millis()
  HaPushedFlag wrap;
  wrap.set(true, 0xFFFFFF00u);
  TEST_ASSERT(wrap.active(0x00000100u, true));
  // Push "closed"
  TEST_ASSERT(w.set(false, 5000u));
  TEST_ASSERT(!w.active(5000u, true));

  // Room-wide: any fresh peer reporting its own "open" counts, stale ones not
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const uint32_t now = 1000000u;
  std::vector<TestPeer> peers = {{now - 1000u, nan, nan, nan, nan, false, false}};
  TEST_ASSERT(!any_fresh_peer_window_open(peers, now));
  peers.push_back({now - PEER_DATA_MAX_AGE_MS - 1u, nan, nan, nan, nan, false, true});
  TEST_ASSERT(!any_fresh_peer_window_open(peers, now));
  peers.push_back({now - 500u, nan, nan, nan, nan, true, true});
  TEST_ASSERT(any_fresh_peer_window_open(peers, now));
  TEST_ASSERT(any_fresh_peer_ac_active(peers, now));
  return true;
}

// T-7r: Room-wide radar presence — off-delay hold and peer flag
bool test_room_presence() {
  using namespace ventosync::room;
  PresenceHold h;
  TEST_ASSERT(!h.update(false, 1000u));                 // never seen
  TEST_ASSERT(h.update(true, 2000u));                   // rising edge immediate
  TEST_ASSERT(h.update(false, 2000u + PRESENCE_HOLD_MS - 1u)); // still held
  TEST_ASSERT(!h.update(false, 2000u + PRESENCE_HOLD_MS));     // released
  PresenceHold wrap;
  wrap.update(true, 0xFFFFFF00u);
  TEST_ASSERT(wrap.update(false, 0x00000100u));         // millis() wrap-safe

  const float nan = std::numeric_limits<float>::quiet_NaN();
  const uint32_t now = 1000000u;
  std::vector<TestPeer> peers = {{now - 1000u, nan, nan, nan, nan, false, false, false}};
  TEST_ASSERT(!any_fresh_peer_presence(peers, now));
  peers.push_back({now - PEER_DATA_MAX_AGE_MS - 1u, nan, nan, nan, nan, false, false, true});
  TEST_ASSERT(!any_fresh_peer_presence(peers, now));    // stale peer
  peers.push_back({now - 500u, nan, nan, nan, nan, false, false, true});
  TEST_ASSERT(any_fresh_peer_presence(peers, now));
  return true;
}

// T-7s: Held reading — stand-in for a temporarily unmeasurable temperature
bool test_held_reading() {
  using namespace ventosync::room;
  const float nan = std::numeric_limits<float>::quiet_NaN();
  HeldReading r;
  TEST_ASSERT(std::isnan(r.get(1000u)));            // nothing stored
  r.store(18.5f, 1000u);
  r.store(nan, 5000u);                              // NaN is ignored
  TEST_ASSERT(std::abs(r.get(1000u + HELD_READING_MAX_AGE_MS) - 18.5f) < 1e-6f);
  TEST_ASSERT(std::isnan(r.get(1001u + HELD_READING_MAX_AGE_MS)));  // expired
  r.store(21.0f, 0xFFFFFF00u);
  TEST_ASSERT(std::abs(r.get(0x00000100u) - 21.0f) < 1e-6f);        // wrap-safe
  return true;
}

// T-7j: Disabling the switch clears all latches; AC off clears emergencies
bool test_hvac_latch_reset() {
  using namespace ventosync::hvac;
  Coordinator c;
  Decision d = c.evaluate(hvac_inputs(true, true, 1600.0f, 0));
  TEST_ASSERT(d.state == State::EMERGENCY_CO2);
  TEST_ASSERT(c.is_co2_emergency());

  d = c.evaluate(hvac_inputs(false, true, 1600.0f, 10));
  TEST_ASSERT(d.state == State::DISABLED);
  TEST_ASSERT(!c.is_co2_emergency());
  TEST_ASSERT(!c.is_ac_active());

  // Re-enable at moderate CO2 → throttled again (no stale latch)
  d = c.evaluate(hvac_inputs(true, true, 1300.0f, 20));
  TEST_ASSERT(d.state == State::THROTTLED);
  return true;
}

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

  // Infinite timer never expires and raises no expiry event
  sm.ventilation_timer_expired = false;
  sm.update(start + 48u * 3600u * 1000u);
  TEST_ASSERT(sm.current_mode == esphome::MODE_VENTILATION);
  TEST_ASSERT(!sm.ventilation_timer_expired);

  // Timer expiry: falls back to heat recovery and raises the UI event once
  esphome::VentilationStateMachine sm2;
  sm2.setup();
  sm2.set_mode(esphome::MODE_VENTILATION, start, duration_ms);
  sm2.update(start + duration_ms / 2);
  TEST_ASSERT(!sm2.ventilation_timer_expired);
  sm2.update(start + duration_ms + 1u);
  TEST_ASSERT(sm2.current_mode == esphome::MODE_ECO_RECOVERY);
  TEST_ASSERT(sm2.ventilation_timer_expired);

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

  struct HvacCase { const char *name; bool (*fn)(); };
  const HvacCase hvac_cases[] = {
    {"T-7a: HVAC Coordinator disabled is transparent", test_hvac_disabled_is_transparent},
    {"T-7b: HVAC Coordinator standby when AC off", test_hvac_standby_when_ac_off},
    {"T-7c: HVAC Coordinator throttled profile (CO2-only, cap, ECO lock)", test_hvac_throttled_profile},
    {"T-7d: HVAC Coordinator CO2 emergency hysteresis", test_hvac_co2_emergency_hysteresis},
    {"T-7e: HVAC Coordinator emergency margin guard", test_hvac_emergency_margin_guard},
    {"T-7f: HVAC Coordinator AC release delay (debounce)", test_hvac_ac_release_delay},
    {"T-7g: HVAC Coordinator unknown AC state is fail-safe", test_hvac_unknown_ac_state_is_inactive},
    {"T-7h: HVAC Coordinator suspended without CO2", test_hvac_suspended_without_co2},
    {"T-7i: HVAC Coordinator mold guard", test_hvac_mold_guard},
    {"T-7j: HVAC Coordinator latch reset", test_hvac_latch_reset},
    {"T-7k: Room-wide CO2 fusion (local / fresh peers)", test_room_co2_fusion},
    {"T-7l: Room-wide demand fusion (no feedback loop)", test_room_demand_fusion},
    {"T-7m: Room-wide humidity fusion (mold guard)", test_room_humidity_fusion},
    {"T-7n: HVAC AC state sources (HA push, expiry, peers)", test_hvac_ac_state_sources},
    {"T-7o: HVAC config ranges + fusion window", test_hvac_config_ranges},
    {"T-7p: Auto level mapping enforces the window (HVAC cap)", test_auto_target_level},
    {"T-7q: Window Guard inputs (HA push expiry, peers)", test_window_guard_inputs},
    {"T-7r: Room-wide radar presence (hold, peers)", test_room_presence},
    {"T-7s: Held reading (unmeasurable NTC in continuous ventilation)", test_held_reading},
  };
  for (const auto &tc : hvac_cases) {
    if (tc.fn()) {
      std::cout << "[PASS] " << tc.name << std::endl;
    } else {
      std::cout << "[FAIL] " << tc.name << std::endl;
      all_passed = false;
    }
  }

  if (all_passed) {
    std::cout << "\nALL TESTS PASSED!" << std::endl;
    return 0;
  } else {
    std::cout << "\nSOME TESTS FAILED!" << std::endl;
    return 1;
  }
}
