# VentoSync — Claude Code Context

## Project Overview

VentoSync is an advanced ESPHome-based smart heat recovery ventilation (HRV) controller
for the VentoMaxx V-WRG Series. It runs on a custom PCB with a **Seeed XIAO ESP32-C6**
and replaces the proprietary VentoMaxx control unit entirely.

**Key facts:**

- Platform: ESP32-C6 (RISC-V), ESPHome `2026.8.0`
- Hardware: Custom PCB with Traco power supply, MCP23017 GPIO expander, PCA9685 LED driver
- Sensors: SCD41 (CO2), BME680 (IAQ fallback), BMP390 (pressure), 2× NTC thermistors, HLK-LD2450 (mmWave radar)
- Communication: ESP-NOW (v8) for multi-device sync (no Wi-Fi router required between units), ESPHome Native API to Home Assistant
- Language policy: **Code comments and all internal developer documentation in English.** HA entity names, UI labels, and user-facing strings remain in **German**.
- License: GPL v3
- Current version: see `version.json` (single source of truth, auto-bumped by `version_bump.py`)

---

## Repository Structure

```text
VentoSync/
├── .github/workflows/         # CI/CD (GitHub Actions) for automated build, lint & release
├── components/                # Custom ESPHome C++ external components & helper libraries
│   ├── helpers/               # Modular C++ headers (PID logic, ESP-NOW sync, IAQ engine, NTC filters)
│   ├── ventilation_group/     # Core group state machine & multi-device coordination
│   ├── ventilation_logic/     # IAQ classification, comfort logic & mathematical helpers
│   └── wrg_dashboard/         # Built-in Web UI dashboard (Tailwind CSS & Chart.js)
├── documentation/             # Technical deep-dive guides, datasheets & HA setup tutorials
│   ├── en/                    # English documentation
│   ├── de/                    # German documentation
│   ├── datasheets/            # Component PDF datasheets
│   └── screenshots/           # UI & dashboard screenshots
├── EasyEDA-Pro/               # PCB hardware files (Schematics, Gerber, BOM, Photos)
├── ESPHome-VentoMaxx-Analyser/# Hardware analysis & PWM oscilloscope verification tools
├── ha_integration_example/    # Home Assistant dashboard templates & master-node configs
├── json/                      # Deployment manifests & GitHub release templates
├── packages/                  # Modular YAML configuration packages
│   ├── actuators/             # PID controllers, automations, safety & maintenance logic
│   ├── base/                  # ESP32-C6 core, device base config & Wi-Fi/OTA
│   ├── communication/         # ESP-NOW unicast & broadcast protocols
│   ├── globals/               # Separated global variables (automation, network, UI, fan)
│   ├── integration/           # Home Assistant entity exposures & data exchange
│   ├── io/                    # Fan PWM, buttons, PCA9685/MCP23017 hardware pinouts
│   ├── sensors/               # Drivers & Mocks for SCD41, BME680, Radar, BMP390, NTCs
│   └── ui/                    # On-device front panel controls & diagnostic entities
├── tests/                     # C++ unit tests & native test runner suite
├── ventosync.yaml             # Main configuration entry point (Full sensor variant)
├── ventosync_bme680_only.yaml # Hardware variant: BME680 fallback (no SCD41/Radar)
├── ventosync_radar_only.yaml  # Hardware variant: Radar presence only (no climate sensors)
├── ventosync_nosensor.yaml    # Hardware variant: Core HRV fan control without sensors
├── ventosync_NTConly.yaml     # Hardware variant: Core HRV with NTC temperature sensors only
├── upload_all.sh              # Multi-device batch compilation & OTA flash script
└── version.json               # Current semantic release version & metadata
```

---

## Hardware Variants & Build Flags

Each top-level YAML is a thin wrapper that includes `ventosync_base.yaml` plus modular sensor packages.
C++ preprocessor flags control conditional compilation in `globals.h`:

| Variant                     | Preprocessor Flags                                          | Description |
|-----------------------------|-------------------------------------------------------------|-------------|
| `ventosync.yaml`            | *(none — all sensors enabled)*                              | Full variant (SCD41 + BME680 + Radar + NTC) |
| `ventosync_bme680_only.yaml`| `-DVENTOSYNC_NO_SCD41`                                      | Fallback: BME680 IAQ, no SCD41, no Radar |
| `ventosync_radar_only.yaml` | `-DVENTOSYNC_NO_CLIMATE`                                    | Radar presence only, no climate sensors |
| `ventosync_nosensor.yaml`   | `-DVENTOSYNC_NO_SCD41 -DVENTOSYNC_NO_BME680 -DVENTOSYNC_NO_RADAR` | Basic fan control without I2C sensors |
| `ventosync_NTConly.yaml`     | `-DVENTOSYNC_NO_SCD41 -DVENTOSYNC_NO_BME680 -DVENTOSYNC_NO_RADAR` | Core HRV with NTC supply/room sensors only |

Missing sensors are replaced by `mock_*.yaml` packages that return clean `NaN`/`false` values.
Never add real sensor YAML without the corresponding mock for the fallback variants.

---

## ESPHome Commands (VS Code Integrated Terminal / WSL)

```bash
# Activate Python virtual environment
source venv/bin/activate

# Validate configuration (syntax check, no compilation)
esphome config ventosync.yaml
esphome config ventosync_nosensor.yaml

# Compile only (generates .bin, no upload)
esphome compile ventosync.yaml

# Compile + OTA upload to specific device
esphome run ventosync.yaml --device <IP> --no-logs

# Upload pre-compiled binary
esphome upload ventosync.yaml --device <IP> --no-logs

# Bulk upload to all network devices
./upload_all.sh

# Initial USB flash via UART/USB-C
esphome run ventosync.yaml --device /dev/ttyACM0

# Run native C++ unit tests
cd tests && g++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined simple_test_runner.cpp -o test_runner && ./test_runner
```

Always validate before uploading. The CI (`build.yaml`) pins ESPHome to `2026.8.0` —
use the same version locally to avoid config drift.

---

## Operating Modes

| Mode              | German UI Name       | HA Entity Value     | LED_WRG | LED_VEN | Behavior |
|-------------------|----------------------|---------------------|---------|---------|----------|
| Smart automatic   | `Smart-Automatik`    | `Smart-Automatik`   | pulses  | off     | Sensor-driven PID demand (CO2, Enthalpy), summer bypass |
| Heat recovery     | `Wärmerückgewinnung` | `Eco Recovery`      | on      | off     | Alternating push-pull cycles (default 70s half-cycle) |
| Cross-ventilation | `Durchlüften`        | `Ventilation`       | on      | on      | Continuous one-directional ventilation with optional timer |
| Boost ventilation | `Stoßlüftung`        | `Boost Ventilation` | off     | on      | 2h cycle: 15 min active boost / 105 min pause |
| Off               | `Aus`                | `Off`               | off     | off     | Fan stopped, standby |

**Critical:** The string `"Smart-Automatik"` must match exactly in both YAML select options
and all C++ code (`system_lifecycle.h`, `network_sync.h`, `user_input.h`).
Mismatch causes silent mode-switch failures (see CHANGELOG 0.8.169).

---

## ESP-NOW Protocol & Cluster Synchronization

- **Protocol Version:** `v8` (magic header byte `0x42`, version byte `0x08` in `VentilationPacket`; v8 added `room_co2` and the room-wide Smart Climate Control thresholds)
- **Discovery:** Broadcast `ROOM_DISC` packet on boot → matching Floor + Room ID → unicast pairing
- **Dynamic Peer Cache:** LRU cache capped at 10 peers in `VentilationController` to prevent heap fragmentation
- **Master/Slave Authority:** Device with ID=1 acts as Master; Slaves mirror target mode and discrete intensity
- **Peer Timeout:** `PEER_TIMEOUT_MS = 900000` (15 minutes of silence drops peer)
- **Heartbeat Sync Interval:** Configurable runtime setting `sync_interval_config` (default 60 seconds)
- **Safe Deserialization:** Always deserialize via `std::memcpy` into stack-allocated structs — **never** cast raw pointers

---

## C++ Architecture & Coding Rules

### Namespaces

Use `ventosync::` namespace for project-wide constants, guards, and helper structs.
Sub-namespaces in use: `ventosync::vacation`, `ventosync::health`, `ventosync::config`, `ventosync::hw`.

### Modular C++ Headers (`components/helpers/`)

Complex YAML lambda logic is extracted into focused header files:

- **`globals.h`** — Central `extern` registry and shared pointers for all ESPHome sensors and entities
- **`auto_mode.h`** — Dual-PID demand evaluation, CO2 priority hysteresis, summer bypass, and room sync
- **`automation_helpers.h`** — Fan motor actuation, V-curve PWM duty calculation, soft ramps, thermal cutoff
- **`bme680_iaq_engine.h`** — BME680 IAQ index estimation, absolute humidity, and calibration logic
- **`climate.h`** — Phase-locked NTC stabilization filter, sensor mapping, and human-readable AQI formatting
- **`config_helpers.h`** — Dynamic runtime configuration (Room, Floor, Device ID, Phase) and NVS persistence
- **`espnow_helpers.h`** — Unified incoming packet validation, source tagging (`RxSource`), and dispatch routing
- **`ha_fan_helpers.h`** — HA Fan platform bridge, bidirectional state sync, and loopback suppression
- **`health_helpers.h`** — System watchdog, loop freeze detection, and stack/heap monitoring
- **`hrv_efficiency.h`** — Real-time sensible and latent heat recovery calculation (DIN EN 13141-8)
- **`led_feedback.h`** — Original VentoMaxx panel LED control (PCA9685/MCP23017), dimming, diagnostic blinks
- **`network_sync.h`** — ESP-NOW v8 mesh communication, packet handlers, peer caching, and room sync
- **`system_boot_helpers.h`** — Low-level GPIO configuration, RF-switch antenna path activation, boot discovery
- **`system_lifecycle.h`** — Multi-stage boot orchestration, filter operating hours tracking, reboot hooks
- **`user_input.h`** — Button debouncing, click/long-press handlers, timed boost countdowns, Child Lock
- **`vacation_helpers.h`** — Vacation mode scheduling and low-intensity interval ventilation

### Custom Components (`components/`)

- **`ventilation_group`** (`VentilationController`, `VentilationStateMachine`): Manages multi-device coordination, 5s soft ramps (`RAMP_DURATION_MS`), and push-pull timing.
- **`ventilation_logic`** (`VentilationLogic`): Hardware-agnostic static math and physics utility library. Also hosts `hvac_coordinator.h` (`ventosync::hvac::Coordinator`): the pure, unit-tested Smart Climate Control state machine (AC debounce, CO2 emergency, mold guard) that `auto_mode.h` applies as a modifier to Smart-Automatik.
- **`wrg_dashboard`** (`WrgDashboard`): Async web server hosting the local SPA (`/ui`, `/state`, `/set`).

### Type Safety & Best Practices

- Use `static_cast<>` everywhere; no C-style casts
- Prefer `constexpr` over `const` for compile-time constants
- Add `static_assert` for critical constants (packet sizes, enum types, timing bounds)
- Use `std::clamp` for all float/integer bounds enforcement — never allow silent overflow
- ESP-NOW packet deserialization: use `std::memcpy` to stack-local struct, **never** cast `uint8_t*` directly to struct pointer (strict aliasing UB)
- Use `const std::vector<uint8_t>&` for packet receive parameters to avoid heap copies

### ESPHome-Specific Patterns

- Always call `->publish_state()` after direct state mutation; never mutate `->state` without publishing
- Use `make_call()` for fan state changes, not direct state assignment
- `internal: true` on HA entities that depend on sensors absent in some hardware variants
- Template sensors reading C++ globals require an explicit `update_interval` (e.g., `1s` or `5s`)

### NVS / Flash wear
- Filter runtime: accumulate in RAM, write to NVS every **8 hours** max (was 30 min → caused 1440 writes/day)
- BME680 baseline counter: RAM-based; flash save gated by `save_interval_ms`
  (1h minimum) and `save_delta_pct` (2% change) in `bme680_iaq_engine.h`
- `restore_mode` native for UI switches where possible (no extra global variable)

---

## PID Controller & Smart-Automatik Mode

- **CO2 PID:** `kp = 0.001`, `ki = 0.0000005` (slow integral — smooth level transitions every 20–30 min)
- **Humidity PID:** `kp = 0.05`, `ki = 0.00001`
- **Conflict Resolution (Hysteresis):**
  - CO2 Grab: `co2_demand >= 0.01` takes exclusive priority over Humidity PID
  - CO2 Release: `co2_demand < 0.005` releases exclusive hold, allowing humidity control
- **Soft Rate Limiting:** Fan changes by **at most ±1 level per 10-second evaluation cycle**
- **Dynamic Limits:** Clamped between `automatik_min_luefterstufe` (default 2) and `automatik_max_luefterstufe` (default 7)
- **Enthalpy Guard (Magnus Formula):** Dehumidification via PID is suppressed when outdoor absolute humidity ($g/m^3$) exceeds indoor air. Requires `sensor.outdoor_humidity` in Home Assistant.

---

## Fan PWM Curve (VentoMaxx V-Curve)

50% PWM = standstill. Direction A (Exhaust / Abluft): 50% → 5% (increasing speed). Direction B (Supply / Zuluft): 50% → 95%.

| Level | PWM Dir A | PWM Dir B | RPM (approx.) |
|-------|-----------|-----------|---------------|
| OFF   | 50.0%     | 50.0%     | 0             |
| 1     | 30.0%     | 70.0%     | 420           |
| 5     | 21.7%     | 78.3%     | 1680          |
| 10    | 5.0%      | 95.0%     | 4200          |

Slew-rate limiter: ~5% per second, 1-second call interval.
Direction changes execute a 5-second linear brake and soft-start ramp.

---

## CI / GitHub Actions

- `build.yaml`: 5-variant matrix build (`ventosync`, `bme680_only`, `radar_only`, `nosensor`, `NTConly`), pinned to ESPHome `2026.8.0`
- `lint.yaml`: YAML validation + C++ checks with `-std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined`
- Release: Git tag → automated generation of `.ota.bin`, `.factory.bin`, and `manifest-<variant>.json` release assets
- Secrets stripped from release binaries — devices use NVS-stored Wi-Fi credentials

---

## Files to Never Modify / Commit

- `secrets.yaml` — gitignored, use `secrets_example.yaml` as template
- `build.log` — gitignored build artifact
- `.version_bump_lock` — lockfile managed by `version_bump.py` during release builds

---

## CHANGELOG Convention

Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), Semantic Versioning.
Sections: `Added`, `Changed`, `Fixed`, `Removed`, `Security / Stability`.
Security items are tagged **K-n** (Kritisch/Critical), **H-n** (High), **M-n** (Medium).
Version is bumped automatically by `version_bump.py` — do not edit `version.json` manually.

---

## Documentation Structure

All documentation is organized in language subdirectories:
- **English:** `documentation/en/*.md` (e.g., `documentation/en/en_home-assistant-entities.md`)
- **German:** `documentation/de/*.md` (e.g., `documentation/de/de_home-assistant-entities.md`)
- **Datasheets:** `documentation/datasheets/*.pdf`
- **Main Readmes:** `Readme.md` (EN) and `Readme_de.md` (DE) in repository root.

When modifying features, behavior, or configuration entities, always update both README files, the corresponding guides in `documentation/en/` and `documentation/de/`, and `CHANGELOG.md`.
