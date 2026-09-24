# VentoSync Logic Test Suite

This directory contains the automated testing environment for the VentoSync firmware. Since the ventilation control logic is complex (PID control, boost ventilation cycles, phase synchronization), we use a native C++ simulation environment to validate the core logic independently of the ESP32 hardware and the ESPHome framework.

## Architecture

The testing environment is based on a "mocking" approach:
- **`simple_test_runner.cpp`**: The central test runner. It emulates the ESPHome environment (globals, sensors, logging) and provides mock objects for hardware components.
- **Logic Integration**: The actual C++ source files from `components/` are compiled directly into the test runner. This ensures we are testing the exact code that runs on the microcontroller.
- **State Simulation**: The runner simulates the passage of time (`millis()`) and sensor signals to verify the behavior of the state machines.

## Running Tests

### Prerequisites
- An installed C++ compiler (GCC/g++).
- For Windows, MinGW is recommended.

### Windows (Quick Start)
Simply run the batch file:
```cmd
run_tests.bat
```
This file looks for `g++`, compiles the runner, and executes it.

### Linux / macOS
Use the following command in the `tests/` directory:
```bash
g++ -std=c++17 -static -I../components/helpers -I../components/ventilation_logic -I../components/ventilation_group -o test_runner.exe simple_test_runner.cpp ../components/ventilation_logic/ventilation_logic.cpp ../components/ventilation_group/ventilation_state_machine.cpp && ./test_runner.exe
```

## Test Coverage

Currently, the following scenarios are automatically verified:

1.  **IAQ Classification**: Validation of CO2 classification (ppm -> text status) including hysteresis behavior.
2.  **WRG Efficiency**: Verification of efficiency calculation based on room, intake, and exhaust temperatures.
3.  **Fan Logic**: Testing the PWM mapping function (level 1-10) and bidirectional motor control (50% stop threshold).
4.  **Mode OFF**: Ensuring the fan safely remains at 50% PWM (hardware stop) in the OFF state.
5.  **Ventilation Timer**: Validation of automatic switching back after the boost ventilation timer expires.
6.  **Sync Time (ESP-NOW)**: Verification of cycle synchronization between Master and Slaves.
7.  **Boost Ventilation Cycle**: Testing rotation ramping (gentle spin-up) during an intensive cycle.
8.  **Phase Logic**: Correct calculation of rotation direction based on Phase A/B assignment.
9.  **Smart Climate Control (HVAC Coordinator)**: T-7a–T-7j — disabled transparency, standby, throttled profile (CO2-only, level cap, ECO lock), CO2 emergency hysteresis, emergency margin guard, AC release debounce, fail-safe unknown AC state, suspension without CO2, mold guard, latch reset.
10. **Room-wide fusion & Smart Climate Control inputs**: T-7k–T-7s — room CO2 / demand / humidity fusion (freshness, no feedback loop), AC state sources (HA push expiry, API link, peer flag), configuration ranges and fusion window, Smart-Automatik level mapping that always enforces the level window (HVAC cap regression), window state (T-7q) and room-wide radar presence hold (T-7r), held readings for unmeasurable NTCs in continuous ventilation (T-7s).

## Adding New Tests

To add a new test case:
1.  Create a new function `bool test_feature()` in `simple_test_runner.cpp`.
2.  Use the mock objects (e.g., `id(fan_intensity_level)->value = X`) to simulate states.
3.  Call the logic function to be tested.
4.  Verify the result with `if (result == expected) return true;`.
5.  Register the function in the `main()` block of the runner.

---
> [!TIP]
> Tests should be run before every major release or refactoring of `components/helpers` or `ventilation_logic` to prevent regression errors.
