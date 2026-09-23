# VentoSync — Claude Code Context

## Project Overview

VentoSync is an advanced ESPHome-based smart heat recovery ventilation (HRV) controller
for the VentoMaxx V-WRG Series. It runs on a custom PCB with a **Seeed XIAO ESP32-C6**
and replaces the proprietary VentoMaxx control unit entirely.

**Key facts:**

- Platform: ESP32-C6 (RISC-V), ESPHome `2026.8.0`
- Hardware: Custom PCB with Traco power supply, MCP23017 GPIO expander, PCA9685 LED driver
- Sensors: Sensirion **SCD43** (CO2 / temperature / humidity), BME680 (IAQ fallback), BMP390 (pressure),
  2× NTC thermistors, HLK-LD2450 (mmWave radar).
  **Naming:** the SCD43 is driven by the ESPHome `scd4x` platform; all IDs, files and globals are
  historically named `scd41_*` (`sensor_SCD41.yaml`, `mock_scd41.yaml`, `VENTOSYNC_NO_SCD41`). Keep that
  naming — do not rename to `scd43_*`.
- Communication: ESP-NOW (protocol v9) for multi-device sync (no Wi-Fi router required between units),
  ESPHome Native API to Home Assistant (optional MQTT variant)
- Language policy: **Code comments and all internal developer documentation in English.** HA entity names,
  UI labels, and user-facing strings remain in **German**.
- License: GPL v3
- Current version: `version.json` (single source of truth — see [Versioning & Release](#versioning--release))

---

## Before Every Commit (Checklist)

1. **Unit tests pass** (exact CI command, see [Commands](#commands)).
2. **YAML still validates** for the affected variants (`esphome config <variant>.yaml`) when YAML changed.
3. **Behaviour / entity / configuration changed →** update `CHANGELOG.md`, both READMEs (`Readme.md`,
   `Readme_de.md`) and the matching guides in `documentation/en/` **and** `documentation/de/`.
4. **Change should reach the devices →** bump `version.json` to the version of the new CHANGELOG entry
   (see [Versioning & Release](#versioning--release)). Without a bump no new release is published.
5. **`VentilationPacket` changed →** follow the [ESP-NOW rules](#esp-now-protocol--cluster-synchronization)
   (protocol version bump, size assert, update this file).
6. **Never commit** build artifacts, backups or local files (see [Files to Never Commit](#files-to-never-commit)).
7. Update the `Modified:` date in the header of every source file you touch.

---

## Repository Structure

```text
VentoSync/
├── .github/workflows/         # CI/CD: build.yaml, lint.yaml, codeql.yaml, security.yaml (see CI section)
├── components/                # Custom ESPHome C++ external components & helper libraries
│   ├── helpers/               # Modular C++ headers included by the YAML lambdas (see list below)
│   ├── ventilation_group/     # VentilationController (ESP-NOW group, peers) & VentilationStateMachine
│   ├── ventilation_logic/     # Pure, unit-tested logic: math/PWM, HVAC coordinator, room fusion
│   └── wrg_dashboard/         # Built-in Web UI dashboard (Tailwind CSS & Chart.js)
├── documentation/             # Technical deep-dive guides, datasheets & HA setup tutorials
│   ├── en/                    # English documentation (en_*.md)
│   ├── de/                    # German documentation (de_*.md)
│   ├── datasheets/            # Component PDF datasheets
│   └── screenshots/           # UI & dashboard screenshots
├── EasyEDA-Pro/               # PCB hardware files (Schematics, Gerber, BOM, Photos)
├── ESPHome-VentoMaxx-Analyser/# Hardware analysis & PWM oscilloscope verification tools
├── ha_integration_example/    # Home Assistant dashboard templates & master-node configs
├── images/                    # Images referenced by the READMEs
├── include -> components/helpers  # Symlink (IDE / PlatformIO include path) — edit the originals only
├── json/                      # manifest_template.json for OTA release manifests
├── packages/                  # Modular YAML configuration packages
│   ├── actuators/             # PID controllers, automations, safety & maintenance logic
│   ├── base/                  # ESP32-C6 core, device base config, Wi-Fi/OTA, ventosync_base.yaml
│   ├── communication/         # ESP-NOW unicast & broadcast
│   ├── globals/               # Global variables (automation, network, UI, ventilation)
│   ├── integration/           # Home Assistant entities, HA fan entity, optional MQTT
│   ├── io/                    # Fan PWM, buttons, PCA9685/MCP23017 hardware pinouts
│   ├── sensors/               # Drivers & mocks for SCD4x, BME680, Radar, BMP390, NTCs
│   └── ui/                    # On-device front panel controls & diagnostic entities
├── tests/                     # Native C++ unit tests (simple_test_runner.cpp) & hardware test guide
├── ventosync*.yaml            # Variant entry points (see Hardware Variants)
├── upload_all.sh              # Multi-device batch compilation & OTA flash script (bumps version locally)
├── version_bump.py            # Local build version bump (disabled in CI)
└── version.json               # Current semantic release version & metadata
```

---

## Hardware Variants & Build Flags

Every top-level YAML includes `packages/base/ventosync_base.yaml` (which always contains the NTC and HRV
efficiency packages) plus sensor packages or their mocks. Preprocessor flags control conditional
compilation in `globals.h`; substitutions control which entities are visible in HA.

| Variant YAML | Build flags | Sensor packages | `hide_ntc_sensors` | Hardware |
|---|---|---|---|---|
| `ventosync.yaml` | *(none)* | SCD41, BME680, LD2450 | `false` | Full: SCD43 + BME680 + radar + NTCs |
| `ventosync_bme680_only.yaml` | `NO_SCD41`, `NO_RADAR` | BME680, mock SCD41/radar | `false` | BME680 IAQ fallback + NTCs |
| `ventosync_radar_only.yaml` | `NO_SCD41`, `NO_BME680` | LD2450, mock SCD41/BME680 | `false` | Radar presence + NTCs, no climate sensor |
| `ventosync_NTConly.yaml` | `NO_SCD41`, `NO_BME680`, `NO_RADAR` | mocks only | `false` | **NTCs fitted** (air temperature before/after fan & ceramic block), no I2C sensors |
| `ventosync_nosensor.yaml` | `NO_SCD41`, `NO_BME680`, `NO_RADAR` | mocks only | `true` | **No sensors at all** — not even NTCs |

(Flags are written without the `-DVENTOSYNC_` prefix, e.g. `NO_SCD41` = `-DVENTOSYNC_NO_SCD41`.)

- `NTConly` and `nosensor` compile the **same code**; they differ only in the substitutions
  (`hide_ntc_sensors`, `firmware_variant`). In `nosensor` the NTC package is still compiled but its
  entities are hidden (`internal: true`) because no thermistors are connected.
- CI additionally generates `ventosync_nosensor_mqtt.yaml` (MQTT instead of the native API) — the file is
  not in the repository.
- Missing sensors are replaced by `mock_*.yaml` packages that return clean `NaN`/`false` values.
  Never add real sensor YAML without the corresponding mock for the fallback variants.
- Adding a variant or flag: update this table, `build.yaml` (matrix) and `upload_all.sh`.

---

## Commands

```bash
# Activate Python virtual environment (VS Code integrated terminal / WSL)
source venv/bin/activate

# Validate configuration (syntax check, no compilation) — needs a secrets.yaml
esphome config ventosync.yaml
esphome config ventosync_nosensor.yaml

# Compile only (generates .bin, no upload) — NOTE: bumps version.json locally, do not commit that bump
esphome compile ventosync.yaml

# Compile + OTA upload to a specific device
esphome run ventosync.yaml --device <IP> --no-logs

# Upload a pre-compiled binary
esphome upload ventosync.yaml --device <IP> --no-logs

# Bulk upload to all network devices
./upload_all.sh

# Initial USB flash via UART/USB-C
esphome run ventosync.yaml --device /dev/ttyACM0

# Native C++ unit tests — run from the repository root (identical to CI in build.yaml)
g++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer \
    -o simple_test tests/simple_test_runner.cpp \
    components/ventilation_logic/ventilation_logic.cpp \
    components/ventilation_group/ventilation_state_machine.cpp \
    -Icomponents && ./simple_test && rm -f simple_test
```

Always validate before uploading. CI pins ESPHome to `2026.8.0` — use the same version locally.
Pure logic belongs in `components/ventilation_logic/` (no ESPHome dependencies) so it can be unit-tested;
`components/helpers/` is only compiled by ESPHome and has no native tests.

---

## Operating Modes

| Mode | German UI / HA value | C++ (`VentilationMode`) | Behaviour |
|---|---|---|---|
| Smart automatic | `Smart-Automatik` | `auto_mode_active` flag on top of `MODE_ECO_RECOVERY` / `MODE_VENTILATION` | Sensor-driven PID demand (CO2, humidity/enthalpy), summer bypass |
| Heat recovery | `Wärmerückgewinnung` | `MODE_ECO_RECOVERY` | Alternating push-pull cycles (70 s at level 1 → 50 s at level 10) |
| Cross-ventilation | `Durchlüften` | `MODE_VENTILATION` | Continuous one-directional ventilation with optional timer |
| Boost ventilation | `Stoßlüftung` | `MODE_STOSSLUEFTUNG` | 2 h cycle: 15 min active / 105 min pause |
| Off | `Aus` | `MODE_OFF` | Fan stopped, standby |

The German strings are the values of the HA select **and** the HA fan preset modes.
LED behaviour per mode: `documentation/en/en_operating-modes.md`.

---

## ESP-NOW Protocol & Cluster Synchronization

- **Protocol version:** `v9` (`PACKET_MAGIC = 0x42`, `PROTOCOL_VERSION = 9` in `ventilation_group.h`;
  v8 added `room_co2` and the room-wide Smart Climate Control thresholds, v9 added `room_humidity`).
- **Changing `VentilationPacket`:** bump `PROTOCOL_VERSION`, keep the `static_assert(sizeof ≤ 250)`,
  update the version above (and in the `network_sync.h` entry below), and note in the CHANGELOG that **all devices of a
  room must be flashed** (mixed versions reject each other's packets).
- **Safe deserialization:** only via `std::memcpy` into a stack-local struct in
  `espnow_handler::validate_and_parse_packet()` — **never** cast `uint8_t*` to a struct pointer.
- **Discovery:** broadcast `ROOM_DISC` on boot → matching Floor + Room ID → unicast pairing.
- **Peer cache:** LRU, capped at 10 peers (`VentilationController::peers`).
- **Master/Slave authority:** device ID 1 is Master. Slaves mirror mode and — in Smart-Automatik — the
  Master's **fan level**. Consequence: the Master decides for the whole room, so every demand must reach it.
- **Heartbeat:** `sync_interval_config` (default 60 s). **Peer timeout:** `PEER_TIMEOUT_MS = 900000` (15 min)
  for mode/level following and the dashboard.
- **Room-wide fusion** (`components/ventilation_logic/room_fusion.h`, `ventosync::room`):
  - Devices broadcast **only values from their own sensors**: `pid_demand` (NaN without sensors),
    `room_co2`, `room_humidity`. **Never re-broadcast a fused/adopted value** — two devices would latch
    each other at a high level (feedback loop, CHANGELOG 0.10.21).
  - Receivers fuse the maximum over all peers fresher than `PEER_DATA_MAX_AGE_MS` (5 min) — not only the
    last received packet.

---

## C++ Architecture & Coding Rules

### Namespaces

Use `ventosync::` for project-wide constants, guards, and helper structs. Sub-namespaces in use:
`ventosync::config`, `ventosync::espnow`, `ventosync::ha_fan`, `ventosync::health`, `ventosync::hrv`,
`ventosync::hvac`, `ventosync::hw`, `ventosync::room`, `ventosync::vacation`.

### Modular C++ Headers (`components/helpers/`)

Complex YAML lambda logic is extracted into focused header files:

- **`globals.h`** — Central `extern` registry and shared pointers for all ESPHome sensors and entities
- **`auto_mode.h`** — Dual-PID demand evaluation, CO2 priority hysteresis, summer bypass, room demand fusion, HVAC glue
- **`automation_helpers.h`** — Fan motor actuation, V-curve PWM duty calculation, soft ramps, thermal cutoff
- **`bme680_iaq_engine.h`** — BME680 IAQ index estimation, absolute humidity, and calibration logic
- **`climate.h`** — Phase-locked NTC stabilization filter, sensor mapping, and human-readable AQI formatting
- **`config_helpers.h`** — Dynamic runtime configuration (Room, Floor, Device ID, Phase) and NVS persistence
- **`espnow_helpers.h`** — Unified incoming packet validation, source tagging (`RxSource`), and dispatch routing
- **`ha_fan_helpers.h`** — HA Fan platform bridge, bidirectional state sync, and loopback suppression
- **`health_helpers.h`** — System watchdog, loop freeze detection, and stack/heap monitoring
- **`hrv_efficiency.h`** — Real-time sensible and latent heat recovery calculation (DIN EN 13141-8)
- **`led_feedback.h`** — Original VentoMaxx panel LED control (PCA9685/MCP23017), dimming, diagnostic blinks
- **`network_sync.h`** — ESP-NOW v9 mesh communication, packet handlers, peer caching, and room sync
- **`system_boot_helpers.h`** — Low-level GPIO configuration, RF-switch antenna path activation, boot discovery
- **`system_lifecycle.h`** — Multi-stage boot orchestration, filter operating hours tracking, reboot hooks
- **`user_input.h`** — Button debouncing, click/long-press handlers, timed boost countdowns, Child Lock
- **`vacation_helpers.h`** — Vacation mode scheduling and low-intensity interval ventilation

### Custom Components (`components/`)

- **`ventilation_group`** (`VentilationController`, `VentilationStateMachine`): multi-device coordination,
  peer tracking, 5 s soft ramps (`RAMP_DURATION_MS`), push-pull timing.
- **`ventilation_logic`**: hardware-agnostic, unit-tested logic.
  - `ventilation_logic.h/.cpp` (`VentilationLogic`): static math — fan curve, PWM, cycle timing, efficiency.
  - `hvac_coordinator.h` (`ventosync::hvac::Coordinator`): Smart Climate Control state machine (AC debounce,
    CO2 emergency, mold guard), applied by `auto_mode.h` as a modifier to Smart-Automatik.
  - `room_fusion.h` (`ventosync::room`): room-wide max of CO2 / humidity / peer demand with 5-min freshness.
- **`wrg_dashboard`** (`WrgDashboard`): async web server hosting the local SPA (`/ui`, `/state`, `/set`).

### Type Safety & Best Practices

- Use `static_cast<>` everywhere; no C-style casts
- Prefer `constexpr` over `const` for compile-time constants
- Add `static_assert` for critical constants (packet sizes, enum types, timing bounds)
- Use `std::clamp` for all float/integer bounds enforcement — never allow silent overflow
- Use `const std::vector<uint8_t>&` for packet receive parameters to avoid heap copies
- `millis()` comparisons: always `now - last < interval` (unsigned, wrap-safe) — never `last + interval > now`

### ESPHome-Specific Patterns

- Always call `->publish_state()` after direct state mutation; never mutate `->state` without publishing
- Use `make_call()` for fan and climate state changes, not direct state assignment
- `internal: true` on HA entities that depend on sensors absent in some hardware variants
- Template sensors reading C++ globals require an explicit `update_interval` (e.g., `1s` or `5s`)

### NVS / Flash wear

- Filter runtime: accumulate in RAM, write to NVS every **8 hours** max (was 30 min → 1440 writes/day)
- BME680 baseline: flash save gated by `save_interval_ms` (1 h minimum) and `save_delta_pct` (2 %) in
  `bme680_iaq_engine.h`
- Prefer native `restore_mode` for UI switches (no extra global variable)

---

## Known Pitfalls

- **`"Smart-Automatik"`** must match exactly in the YAML select/preset options and all C++ code
  (`globals.h`, `network_sync.h`, `user_input.h`, `dashboard_html.h`). A mismatch silently breaks mode
  switching (CHANGELOG 0.8.169).
- **PID output without sensor is `0.0`, not NaN.** The ESPHome PID writes 0 when its input is NaN, and
  `co2_pid_result` starts at `0.0`. To detect "no local sensor", check the sensor value
  (`effective_co2->state`), not the PID output.
- **Re-broadcasting fused values** creates latching feedback loops (see ESP-NOW section, CHANGELOG 0.10.21).
- **`static` locals in `inline` header functions** (e.g. `evaluate_auto_mode()`) are shared state for the
  whole firmware — they persist across mode switches and are not per-instance.
- **`effective_co2` may be a BME680 eCO2 estimate** (VOC-based) in the `bme680_only` variant; room-wide
  maxima include it as if it were an NDIR value.
- **Local builds bump `version.json`** via `version_bump.py` — never commit that local bump.

---

## PID Controller & Smart-Automatik Mode

- **CO2 PID:** `kp = 0.001`, `ki = 0.0000005` (slow integral — smooth level transitions every 20–30 min)
- **Humidity PID:** `kp = 0.05`, `ki = 0.00001`
- **Conflict resolution (hysteresis):** CO2 grabs exclusive priority at `co2_demand >= 0.01`, releases at
  `< 0.005`; while CO2 controls, the higher of both demands is used.
- **Room fusion:** effective demand = max(local sensor demand, freshest peer demands) — see ESP-NOW section.
- **Soft rate limiting:** at most ±1 level per 10-second evaluation cycle (±2 right after a mode change).
- **Dynamic limits:** `automatik_min_luefterstufe` (default 2) … `automatik_max_luefterstufe` (default 7).
- **Enthalpy guard (Magnus formula):** humidity demand is suppressed when outdoor absolute humidity (g/m³)
  exceeds indoor. Requires `sensor.outdoor_humidity` in Home Assistant.

Fan curve & PWM mapping (50 % = standstill, level 1 = 30 %/70 %, level 10 = 5 %/95 %):
`VentilationLogic::calculate_fan_speed_from_intensity()` / `calculate_fan_pwm()` and
`documentation/en/en_system-architecture.md`.

---

## CI / GitHub Actions

Triggered on push and pull request to `master`:

- **`build.yaml`**
  - *Run Unit Tests*: native `g++` build of `tests/simple_test_runner.cpp` with ASan/UBSan (command above).
  - *Build* matrix (6 variants): `ventosync-full`, `bme680-only`, `radar-only`, `nosensor`, `ntconly`,
    `nosensor-mqtt` (generated), ESPHome pinned to `2026.8.0`, secret-free OTA configs.
  - *Create Release* (push to `master` only): tag `v<version.json>`, `.ota.bin`, `.factory.bin`,
    `manifest-<variant>.json`; release notes = first section of `CHANGELOG.md`.
- **`lint.yaml`**: `esphome config` validation of the YAML (dummy secrets).
- **`codeql.yaml`**: CodeQL analysis of the C/C++ code.
- **`security.yaml`**: TruffleHog secret scanner.

Devices use NVS-stored Wi-Fi credentials; secrets are stripped from release binaries.

---

## Versioning & Release

- `version.json` is the single source of truth: it is included into the firmware
  (`packages/base/esp32c6_common.yaml`) and defines the release tag.
- **CI never bumps the version.** A release is created on every push to `master` with the version from
  `version.json`. If that tag already exists, the devices do not see an update.
- Therefore every change that should reach the devices must bump `version.json` **in the commit/PR**
  (convention: separate commit `chore: bump version to x.y.z`), matching the new top entry in `CHANGELOG.md`.
- `version_bump.py` only bumps during **local** builds (`esphome compile`, `upload_all.sh`), guarded by
  `.version_bump_lock`. Do not commit those local bumps.

---

## Files to Never Commit

- `secrets.yaml` — gitignored, use `secrets_example.yaml` as template
- `.version_bump_lock` — lockfile managed by `version_bump.py`
- Build artifacts: `build.log`, `.esphome/`, test binaries (`test_runner`, `simple_test`, `*.exe`,
  `tests/manual_test_runner`), `print_size`
- Backups: `*.bak`, `*.backup`

---

## CHANGELOG Convention

Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), Semantic Versioning.
Sections: `Added`, `Changed`, `Fixed`, `Removed`, `Security / Stability`.
Security items are tagged **K-n** (Kritisch/Critical), **H-n** (High), **M-n** (Medium).
The top `## [x.y.z] - YYYY-MM-DD` entry must match `version.json` (it becomes the release notes).

---

## Documentation Structure

- **English:** `documentation/en/*.md` (e.g., `documentation/en/en_home-assistant-entities.md`)
- **German:** `documentation/de/*.md` (e.g., `documentation/de/de_home-assistant-entities.md`)
- **Datasheets:** `documentation/datasheets/*.pdf`
- **Main READMEs:** `Readme.md` (EN) and `Readme_de.md` (DE) in the repository root
- **Component READMEs:** `components/*/Readme.md` — update when adding/removing files in a component

When modifying features, behaviour, or configuration entities, always update both README files, the
corresponding guides in `documentation/en/` and `documentation/de/`, and `CHANGELOG.md`.
