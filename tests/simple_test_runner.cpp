#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cmath>
#include "../components/ventilation_logic/ventilation_logic.h"

// ============================================================
// Simple Test Framework
// ============================================================
#define TEST_ASSERT(cond) \
    if (!(cond)) { \
        std::cerr << "FAILED: " << #cond << " at line " << __LINE__ << std::endl; \
        return false; \
    }

bool test_heat_recovery() {
    // T_in=20, T_out=0, T_supply=16 -> Eff = (16-0)/(20-0) = 80%
    float eff = VentilationLogic::calculate_heat_recovery_efficiency(20.0f, 16.0f, 0.0f);
    TEST_ASSERT(std::abs(eff - 80.0f) < 0.1f);
    
    // ΔT < 1 °C → 0 (Division-by-zero-Schutz)
    eff = VentilationLogic::calculate_heat_recovery_efficiency(20.0f, 16.0f, 20.0f);
    TEST_ASSERT(eff == 0.0f);

    // Ergebnis > 100 % (Sensor-Rauschen) → auf 100 % gekappt (Changelog: Clamping)
    eff = VentilationLogic::calculate_heat_recovery_efficiency(20.0f, 25.0f, 0.0f);
    TEST_ASSERT(eff == 100.0f);

    // Negativer Wert → 0 % gekappt
    eff = VentilationLogic::calculate_heat_recovery_efficiency(20.0f, -5.0f, 0.0f);
    TEST_ASSERT(eff == 0.0f);

    return true;
}

// ============================================================
// [Unreleased] ebm-papst VarioPro: Single-PWM-Mapping Formel
// direction 0 → pwm = 0.5 - (speed × 0.5)   (Richtung A, Abluft)
// direction 1 → pwm = 0.5 + (speed × 0.5)   (Richtung B, Zuluft)
// Soft-Stop: speed < 0.05 → immer exakt 0.50
// ============================================================
float calc_single_pwm(float speed, int direction) {
    if (speed < 0.05f) return 0.5f;  // Soft-Stop-Zone
    if (direction == 0)
        return 0.5f - (speed * 0.5f);
    else
        return 0.5f + (speed * 0.5f);
}

bool test_ebmpapst_pwm_mapping() {
    // Stopp: speed=0 (< 0.05) → exakt 50 % egal welche Richtung
    TEST_ASSERT(std::abs(calc_single_pwm(0.0f, 0) - 0.5f) < 0.001f);
    TEST_ASSERT(std::abs(calc_single_pwm(0.0f, 1) - 0.5f) < 0.001f);

    // Soft-Stop-Grenze: speed=0.04 → 50 % (noch in Zone)
    TEST_ASSERT(std::abs(calc_single_pwm(0.04f, 1) - 0.5f) < 0.001f);

    // Richtung A (Abluft): speed=1.0 → pwm = 0.5 - 0.5 = 0.0
    TEST_ASSERT(std::abs(calc_single_pwm(1.0f, 0) - 0.0f) < 0.001f);
    // Richtung A: speed=0.5 → pwm = 0.5 - 0.25 = 0.25
    TEST_ASSERT(std::abs(calc_single_pwm(0.5f, 0) - 0.25f) < 0.001f);

    // Richtung B (Zuluft): speed=1.0 → pwm = 0.5 + 0.5 = 1.0
    TEST_ASSERT(std::abs(calc_single_pwm(1.0f, 1) - 1.0f) < 0.001f);
    // Richtung B: speed=0.5 → pwm = 0.5 + 0.25 = 0.75
    TEST_ASSERT(std::abs(calc_single_pwm(0.5f, 1) - 0.75f) < 0.001f);

    // Symmetrie: Richtung A und B sind gespiegelt um 0.5
    float pwm_a = calc_single_pwm(0.8f, 0);
    float pwm_b = calc_single_pwm(0.8f, 1);
    TEST_ASSERT(std::abs((pwm_a + pwm_b) - 1.0f) < 0.001f);  // a + b = 1.0

    return true;
}

// ============================================================
// [Unreleased] Mindestdrehzahl Stufe 1 = 10 %
// speed = 0.10 + ((intensity - 1) / 9) × 0.90
// Stufe 1 → 10 %, Stufe 10 → 100 %
// ============================================================
float calc_fan_speed_from_intensity(int intensity) {
    return 0.10f + ((float)(intensity - 1) / 9.0f) * 0.90f;
}

bool test_min_speed_mapping() {
    // Stufe 1: Mindestdrehzahl 10 % (nicht mehr 0 % / Soft-Stop)
    TEST_ASSERT(std::abs(calc_fan_speed_from_intensity(1) - 0.10f) < 0.001f);

    // Stufe 10: 100 %
    TEST_ASSERT(std::abs(calc_fan_speed_from_intensity(10) - 1.00f) < 0.001f);

    // Stufe 5: Mittelwert = 0.10 + (4/9)*0.90 = 0.10 + 0.40 = 0.50
    TEST_ASSERT(std::abs(calc_fan_speed_from_intensity(5) - 0.50f) < 0.001f);

    // Stufe 1 muss > Soft-Stop-Grenze (0.05) liegen → Lüfter dreht wirklich
    TEST_ASSERT(calc_fan_speed_from_intensity(1) > 0.05f);

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
    TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_up(0)   - 0.0f) < 0.001f);
    TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_up(50)  - 0.5f) < 0.001f);
    TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_up(100) - 1.0f) < 0.001f);

    // Ramp Down: iteration 0 → 1.0, iteration 100 → 0.0
    TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_down(0)   - 1.0f) < 0.001f);
    TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_down(50)  - 0.5f) < 0.001f);
    TEST_ASSERT(std::abs(VentilationLogic::calculate_ramp_down(100) - 0.0f) < 0.001f);

    // Symmetrie: ramp_up(i) + ramp_down(i) == 1.0
    for (int i = 0; i <= 100; i += 25) {
        float sum = VentilationLogic::calculate_ramp_up(i) + VentilationLogic::calculate_ramp_down(i);
        TEST_ASSERT(std::abs(sum - 1.0f) < 0.001f);
    }

    return true;
}

bool test_co2_logic() {
    // Classification
    TEST_ASSERT(VentilationLogic::get_co2_classification(400)  == "Ausgezeichnet");
    TEST_ASSERT(VentilationLogic::get_co2_classification(700)  == "Gut");
    TEST_ASSERT(VentilationLogic::get_co2_classification(900)  == "Mäßig");
    TEST_ASSERT(VentilationLogic::get_co2_classification(1100) == "Erhöht");
    TEST_ASSERT(VentilationLogic::get_co2_classification(1300) == "Schlecht");
    TEST_ASSERT(VentilationLogic::get_co2_classification(1500) == "Inakzeptabel");

    // [Unreleased] CO2 NaN / 0-Guard: Sensor nicht angeschlossen → aktuellen Level beibehalten
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(std::numeric_limits<float>::quiet_NaN(), 5, 2, 10) == 5);
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(0.0f, 3, 2, 10) == 3);
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(-1.0f, 4, 2, 10) == 4);
    // NaN-Klassifikation → "Unbekannt"
    TEST_ASSERT(VentilationLogic::get_co2_classification(std::numeric_limits<float>::quiet_NaN()) == "Unbekannt");
    TEST_ASSERT(VentilationLogic::get_co2_classification(0.0f) == "Unbekannt");
    
    // Fan Level Logic (Thresholds: >600->2, >800->3, >1000->5, >1200->7, >1400->9)
    int min = 2;
    int max = 7;
    
    // Base case (low CO2) -> Should be min level (2)
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(400, 5, min, max) == 2);
    
    // Threshold increases
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(650, 2, min, max) == 2); // >600 -> 2
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(850, 2, min, max) == 3); // >800 -> 3
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(1050, 3, min, max) == 5); // >1000 -> 5
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(1250, 5, min, max) == 7); // >1200 -> 7 (clamped)
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(1450, 7, min, max) == 7); // >1400 -> 9 (clamped)
    
    // Max level clamping (Noise Control)
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(1450, 7, min, 10) == 9); // Max 10 -> 9 allowed
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(1450, 7, min, 5) == 5); // Max 5 -> clamped to 5
    
    // Min level clamping (Moisture Protection)
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(400, 5, 4, max) == 4); // Min 4 -> 4
    
    // Hysteresis (Down thresholds: 1300, 1100, 900, 700, 500)
    // Current=5 (from >1000). Drop to 950. >900 (down threshold) -> Stay at 5
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(950, 5, min, max) == 5);
    
    // Drop below down threshold (850 < 900) -> Drop to 3 (because >800)
    TEST_ASSERT(VentilationLogic::get_co2_fan_level(850, 5, min, max) == 3);
    
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

    esphome::HardwareState state = sm.get_target_state();
    TEST_ASSERT(state.fan_enabled == false);

    // Auch nach mehreren Updates bleibt der Lüfter aus
    sm.update(60000);
    state = sm.get_target_state();
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
    esphome::HardwareState state = sm.get_target_state();
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
    TEST_ASSERT(pos_after_sync < sm.cycle_duration_ms * 2); // innerhalb eines vollen Zyklus

    return true;
}

bool test_stosslueftung_cycle() {
    esphome::VentilationStateMachine sm;
    sm.setup();
    
    uint32_t start_time = 100000;
    sm.set_mode(esphome::MODE_STOSSLUEFTUNG, start_time);
    
    // 1. Initial State: Active (Fan ON)
    esphome::HardwareState state = sm.get_target_state();
    TEST_ASSERT(state.fan_enabled == true);
    TEST_ASSERT(sm.stoss_active_phase == true);
    
    // 2. Advance 14 minutes -> Still Active
    sm.update(start_time + 14 * 60 * 1000);
    state = sm.get_target_state();
    TEST_ASSERT(state.fan_enabled == true);
    
    // 3. Advance 16 minutes -> Pause (Fan OFF)
    sm.update(start_time + 16 * 60 * 1000);
    state = sm.get_target_state();
    TEST_ASSERT(state.fan_enabled == false);
    TEST_ASSERT(sm.stoss_active_phase == false);

    // 4. Advance 119 minutes (Total) -> Still Pause
    sm.update(start_time + 119 * 60 * 1000);
    state = sm.get_target_state();
    TEST_ASSERT(state.fan_enabled == false);

    // 5. Advance 121 minutes (Total) -> New Cycle (Fan ON, Direction Flipped)
    bool initial_direction = state.direction_in;
    // Note: We need to capture direction from active phase to compare
    
    sm.update(start_time + 121 * 60 * 1000);
    state = sm.get_target_state();
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
    esphome::HardwareState state = sm.get_target_state();
    TEST_ASSERT(sm.global_phase == true);
    TEST_ASSERT(state.direction_in == true);
    
    // t=1100 -> Phase B Active (1000-2000)
    // Group A should be OUT
    sm.update(1100);
    state = sm.get_target_state();
    TEST_ASSERT(sm.global_phase == false);
    TEST_ASSERT(state.direction_in == false);
    
    return true;
}

int main() {
    std::cout << "Running VentilationLogic Tests..." << std::endl;
    
    bool all_passed = true;
    if (test_co2_logic())         { std::cout << "[PASS] CO2 Logic" << std::endl; }              else { std::cout << "[FAIL] CO2 Logic" << std::endl;              all_passed = false; }
    if (test_heat_recovery())     { std::cout << "[PASS] Heat Recovery" << std::endl; }           else { std::cout << "[FAIL] Heat Recovery" << std::endl;           all_passed = false; }
    if (test_fan_logic())         { std::cout << "[PASS] Fan Logic" << std::endl; }               else { std::cout << "[FAIL] Fan Logic" << std::endl;               all_passed = false; }
    if (test_ramp_functions())    { std::cout << "[PASS] Ramp Up/Down" << std::endl; }            else { std::cout << "[FAIL] Ramp Up/Down" << std::endl;            all_passed = false; }

    std::cout << "Running [Unreleased] Tests..." << std::endl;
    if (test_ebmpapst_pwm_mapping()){ std::cout << "[PASS] ebm-papst Single-PWM Mapping" << std::endl; } else { std::cout << "[FAIL] ebm-papst Single-PWM Mapping" << std::endl; all_passed = false; }
    if (test_min_speed_mapping()) { std::cout << "[PASS] Mindestdrehzahl Stufe 1 (10%)" << std::endl; } else { std::cout << "[FAIL] Mindestdrehzahl Stufe 1 (10%)" << std::endl; all_passed = false; }

    std::cout << "Running VentilationStateMachine Tests..." << std::endl;
    if (test_mode_off())          { std::cout << "[PASS] Mode OFF" << std::endl; }                else { std::cout << "[FAIL] Mode OFF" << std::endl;                all_passed = false; }
    if (test_ventilation_timer()) { std::cout << "[PASS] Ventilation Timer" << std::endl; }      else { std::cout << "[FAIL] Ventilation Timer" << std::endl;      all_passed = false; }
    if (test_sync_time())         { std::cout << "[PASS] Sync Time (ESP-NOW)" << std::endl; }    else { std::cout << "[FAIL] Sync Time (ESP-NOW)" << std::endl;    all_passed = false; }
    if (test_stosslueftung_cycle()){ std::cout << "[PASS] Stoßlüftung Cycle" << std::endl; }     else { std::cout << "[FAIL] Stoßlüftung Cycle" << std::endl;     all_passed = false; }
    if (test_phase_logic())       { std::cout << "[PASS] Phase Logic" << std::endl; }             else { std::cout << "[FAIL] Phase Logic" << std::endl;             all_passed = false; }

    if (all_passed) {
        std::cout << "\nALL TESTS PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSOME TESTS FAILED!" << std::endl;
        return 1;
    }
}
