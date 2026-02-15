@echo off
set GPP="C:\Users\thomas.engeroff\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.MCF.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe"
echo Compiling tests...
%GPP% -static -o test_runner.exe simple_test_runner.cpp ../components/ventilation_logic/ventilation_logic.cpp ../components/ventilation_group/ventilation_state_machine.cpp
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed!
    exit /b %ERRORLEVEL%
)
echo Running tests...
test_runner.exe
pause
