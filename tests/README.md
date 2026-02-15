To run the tests on Windows:

**Method 1: Automatic (Recommended)**
Double-click `run_tests.bat`. It will find the compiler and run the tests.

**Method 2: Manual (if g++ is in PATH)**

1. Open terminal in this directory.
2. Compile:
   `g++ -static -o test_runner.exe simple_test_runner.cpp ../components/ventilation_logic/ventilation_logic.cpp`
3. Run:
   `.\test_runner.exe`

Expected Output:
Running VentilationLogic Tests...
[PASS] IAQ Classification
[PASS] Heat Recovery
[PASS] Fan Logic

ALL TESTS PASSED!
