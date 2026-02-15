#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cmath>
#include "../components/ventilation_logic/ventilation_logic.h"

// Simple Test Framework
#define TEST_ASSERT(cond) \
    if (!(cond)) { \
        std::cerr << "FAILED: " << #cond << " at line " << __LINE__ << std::endl; \
        return false; \
    }

bool test_iaq_classification() {
    TEST_ASSERT(VentilationLogic::get_iaq_classification(25.0f) == "Ausgezeichnet");
    TEST_ASSERT(VentilationLogic::get_iaq_classification(50.0f) == "Ausgezeichnet");
    TEST_ASSERT(VentilationLogic::get_iaq_classification(51.0f) == "Gut");
    TEST_ASSERT(VentilationLogic::get_iaq_classification(350.0f) == "Gesundheitsgefährdend");
    return true;
}

bool test_heat_recovery() {
    // T_in=20, T_out=0, T_supply=16 -> Eff = (16-0)/(20-0) = 80%
    float eff = VentilationLogic::calculate_heat_recovery_efficiency(20.0f, 16.0f, 0.0f);
    TEST_ASSERT(std::abs(eff - 80.0f) < 0.1f);
    
    // Test division by zero protection
    eff = VentilationLogic::calculate_heat_recovery_efficiency(20.0f, 16.0f, 20.0f);
    TEST_ASSERT(eff == 0.0f);
    return true;
}

bool test_fan_logic() {
    TEST_ASSERT(VentilationLogic::is_fan_slider_off(0.5f) == true);
    TEST_ASSERT(VentilationLogic::is_fan_slider_off(1.5f) == false);
    
    // Cycle check
    TEST_ASSERT(VentilationLogic::get_next_fan_level(1) == 2);
    TEST_ASSERT(VentilationLogic::get_next_fan_level(9) == 10);
    TEST_ASSERT(VentilationLogic::get_next_fan_level(10) == 1);
    return true;
}

#include "../components/ventilation_group/ventilation_state_machine.h"

// ... existing tests ...

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
    if (test_iaq_classification()) std::cout << "[PASS] IAQ Classification" << std::endl; else all_passed = false;
    if (test_heat_recovery()) std::cout << "[PASS] Heat Recovery" << std::endl; else all_passed = false;
    if (test_fan_logic()) std::cout << "[PASS] Fan Logic" << std::endl; else all_passed = false;
    
    std::cout << "Running VentilationStateMachine Tests..." << std::endl;
    if (test_stosslueftung_cycle()) std::cout << "[PASS] Stoßlüftung Cycle" << std::endl; else all_passed = false;
    if (test_phase_logic()) std::cout << "[PASS] Phase Logic" << std::endl; else all_passed = false;

    if (all_passed) {
        std::cout << "\nALL TESTS PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSOME TESTS FAILED!" << std::endl;
        return 1;
    }
}
