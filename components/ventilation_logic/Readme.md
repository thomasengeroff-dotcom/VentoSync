# 🧠 Ventilation Logic Component

This component provides a pure, hardware-agnostic C++ utility library (`VentilationLogic`) containing deterministic math, fan PWM calculations, and thermal physics formulas. It has zero external dependencies, making it fully testable via native C++ unit tests (`tests/`).

## 📄 File Overview

| File | Description |
| :--- | :--- |
| **`ventilation_logic.h` / `.cpp`** | **Static Math & Logic Utilities (`VentilationLogic`)**: Stateless, pure functions for heat recovery efficiency calculation, bidirectional fan PWM mapping, dynamic cycle timing, virtual RPM estimation, linear ramping, and the SGP41 VOC index → CO2 equivalent mapping (`voc_index_to_co2eq`). |
| **`hvac_coordinator.h`** | **Smart Climate Control state machine (`ventosync::hvac::Coordinator`)**: AC debounce and expiry, CO2 emergency and mold guard, configuration ranges shared with the HA sliders. |
| **`room_fusion.h`** | **Room-wide fusion helpers (`ventosync::room`)**: NaN-aware maxima of local and fresh (≥ 5 min, two heartbeats) ESP-NOW peer values for CO2, humidity, PID demand, room AC, window and radar presence state; `HaPushedFlag` (expiring HA-pushed inputs), `PresenceHold` (30 s off-delay). |
| **`level_override.h`** | **Smart-Automatik level override (`ventosync::auto_override`)**: room-wide override (30–300 min, default 60) started with the Level button — wrap-safe countdown, adoption from ESP-NOW packets (never restarted, heartbeat cannot cancel), CO2 health guard and `resolve_level()` (HVAC cap clamps, emergency lets the automatic go higher). |
| **`nox_boost.h`** | **NOx boost (`ventosync::nox`)**: SGP41 NOx-index detector — 60 s trigger above the HA threshold (20–100), 10 min hold below half of it, 60 min cap, outdoor-air test (every intake window must lower the index, else immediate abort + lockout), strong event (≥ 150) lifts the AC cap. |
| **`auto_level.h`** | **Smart-Automatik level helpers (`ventosync::auto_level`)**: soft ramp `ramp_level()` (±1 level per evaluation, ±2 once after an operating mode change, state in `RampState`) and `master_level()` (level of a fresh Master for slaves, wrap-safe). |
| **`summer_bypass.h`** | **Summer bypass (`ventosync::summer`)**: `decide()` — activation (summer, indoor > threshold, outdoor ≥ 1.5 °C cooler), hysteresis, Smart Climate Control lock, reason for the log — and `RemeasureGuard` (ends the bypass when a temperature is unmeasurable and blocks it for 5 min). |
| **`espnow_auth.h`** | **ESP-NOW frame authentication (`ventosync::espnow_auth`, protocol v13)**: dependency-free SHA-256 / HMAC-SHA256, the 16-byte frame trailer (`seal()` / `verify()`: epoch, seq, 8-byte tag over sender MAC + payload + counter) and the `ReplayGuard` (newest counter per sender, 16 senders, 15 min idle expiry). Unit-tested with the FIPS 180-4 / RFC 4231 vectors. |
| **`auto_status.h`** | **Automatik-Status (`ventosync::auto_status`)**: reason selection (priority order), German status text, stable reason keys and the 30 s debounce (`ReasonTracker`) for the HA event. |
| **`__init__.py`** | **ESPHome Code Injector**: Registers the component with ESPHome and injects the `#include` header globally into the generated firmware build. |

## ⚙️ Key Calculations & Functions

- **Heat Recovery Efficiency ($\eta_{WRG}$)**: Computes sensible heat recovery percentage $\frac{T_{\text{supply}} - T_{\text{outdoor}}}{T_{\text{indoor}} - T_{\text{outdoor}}} \times 100$ with $\Delta T \ge 2\,^\circ\text{C}$ guard condition against sensor noise.
- **Bidirectional Fan PWM**: Translates target speed $[0.1 \dots 1.0]$ and direction into the reversible VentoMaxx PWM duty cycle ($50\% = \text{Stop}$, $<50\% = \text{Exhaust}$, $>50\% = \text{Intake}$).
- **Dynamic WRG Cycle Duration**: Scales the direction reversal interval dynamically from 70 seconds (Level 1) down to 50 seconds (Level 10) to optimize heat transfer at higher airflow volumes.
- **Smart-Automatik Level Mapping** (`calculate_auto_target_level`): maps the combined demand onto the current level window with ±25 % hysteresis; the result is always inside the window (enforces the Smart Climate Control cap).
- **Virtual RPM & Software Ramping**: Provides linear acceleration ramp calculations ($0.0 \dots 1.0$) and virtual tachometer RPM estimation.
- **Unit Test Coverage**: Directly validated by native desktop test runners without requiring an ESP32 hardware target.
