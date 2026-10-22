# VentoSync — Claude Code Context

## Project Overview

VentoSync is an advanced ESPHome-based smart heat recovery ventilation (HRV) controller
for the VentoMaxx V-WRG Series. It runs on a custom PCB with a **Seeed XIAO ESP32-C6**
and replaces the proprietary VentoMaxx control unit entirely.

**Key facts:**

- Platform: ESP32-C6 (RISC-V), ESPHome `2026.10.0`
- Hardware: Custom PCB with Traco power supply, MCP23017 GPIO expander, PCA9685 LED driver
- Sensors: Sensirion **SCD43** (CO2 / temperature / humidity) or **SGP41 + SHT45** board (VOC / NOx / temperature / humidity), BME680 (IAQ fallback), BMP390 (pressure),
  2× NTC thermistors, HLK-LD2450 (mmWave radar).
  **Naming:** the SCD43 is driven by the ESPHome `scd4x` platform; all IDs, files and globals are
  historically named `scd41_*` (`sensor_SCD41.yaml`, `mock_scd41.yaml`, `VENTOSYNC_NO_SCD41`). Keep that
  naming — do not rename to `scd43_*`.
- Communication: ESP-NOW (protocol v15, HMAC-authenticated) for multi-device sync (no Wi-Fi router required between units),
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
4. **Every PR into `master` →** bump `version.json` once per PR to the version of the new top CHANGELOG
   entry, or label the PR `no-release` (docs / tooling only). Enforced by the *Version Guard* check
   (see [Versioning & Release](#versioning--release)).
5. **`VentilationPacket` changed →** follow the [ESP-NOW rules](#esp-now-protocol--cluster-synchronization)
   (protocol version bump, size assert, update this file).
6. **Never commit** build artifacts, backups or local files (see [Files to Never Commit](#files-to-never-commit)).
7. Update the `Modified:` date in the header of every source file you touch.

---

## Repository Structure

```text
VentoSync/
├── .github/                   # CI/CD workflows (build, lint, codeql, security, version-guard), scripts/, variants.json
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

Every top-level YAML includes a **board base** plus sensor packages or their mocks:

- `packages/base/ventosync_base.yaml` → **PCB v1.0** (Seeed XIAO ESP32-C6, MCP23017 buttons, XIAO RF switch)
- `packages/base/ventosync_base_v2.yaml` → **PCB v2.0** (ESP32-C6-MINI-1U-H4, buttons on native GPIOs)

Both are thin wrappers: `ventosync_core.yaml` (board-independent logic, always contains the NTC and HRV
efficiency packages) + a board package `packages/board/pcb_v1.yaml` / `pcb_v2.yaml` (shared
`hardware_io.yaml` + `logic_buttons.yaml`; v1 adds `hardware_io_mcp23017.yaml`, re-maps the buttons to the
MCP23017 via `!extend` and runs `init_external_antenna()` at boot priority 900). Never call
`init_external_antenna()` on PCB v2 — it drives GPIO3 (BTN_MOD) and GPIO14 as outputs. Preprocessor flags control conditional
compilation in `globals.h`; substitutions control which entities are visible in HA.

| Variant YAML | Build flags | Sensor packages | `hide_ntc_sensors` | Hardware |
|---|---|---|---|---|
| `ventosync.yaml` | *(none)* | SCD41, BME680, LD2450 | `false` | Full: SCD43 + BME680 + radar + NTCs |
| `ventosync_bme680_only.yaml` | `NO_SCD41`, `NO_RADAR` | BME680, mock SCD41/radar | `false` | BME680 IAQ fallback + NTCs |
| `ventosync_scd43_only.yaml` | `NO_BME680`, `NO_RADAR` | SCD41, mock BME680/radar | `false` | SCD43 board at H2 as the only climate sensor + NTCs |
| `ventosync_radar_only.yaml` | `NO_SCD41`, `NO_BME680` | LD2450, mock SCD41/BME680 | `false` | Radar presence + NTCs, no climate sensor |
| `ventosync_NTConly.yaml` | `NO_SCD41`, `NO_BME680`, `NO_RADAR` | mocks only | `false` | **NTCs fitted** (air temperature before/after fan & ceramic block), no I2C sensors |
| `ventosync_nosensor.yaml` | `NO_SCD41`, `NO_BME680`, `NO_RADAR` | mocks only | `true` | **No sensors at all** — not even NTCs |
| `ventosync_v2_nosensor.yaml` | `NO_SCD41`, `NO_BME680`, `NO_RADAR` | mocks only | `true` | **PCB v2.0** — as `nosensor` |
| `ventosync_v2_bme680_only.yaml` | `NO_SCD41`, `NO_RADAR` | BME680, mock SCD41/radar | `false` | **PCB v2.0** — as `bme680_only` |
| `ventosync_sgp41.yaml` | `NO_SCD41`, `NO_BME680`, `NO_RADAR` | SGP41 + SHT45 (extends mock SCD41/BME680), mock radar | `false` | SGP41/SHT45 VOC board at H2 instead of the SCD43 board |
| `ventosync_v2_scd43_only.yaml` | `NO_BME680`, `NO_RADAR` | SCD41, mock BME680/radar | `false` | **PCB v2.0** — as `scd43_only` |
| `ventosync_v2_sgp41.yaml` | `NO_SCD41`, `NO_BME680`, `NO_RADAR` | SGP41 + SHT45 (extends mock SCD41/BME680), mock radar | `false` | **PCB v2.0** — as `sgp41` |
| `ventosync_nosensor_btproxy.yaml` | `NO_SCD41`, `NO_BME680`, `NO_RADAR` | mocks + `integration/bluetooth_proxy.yaml` | `true` | **Experimental** — `nosensor` + HA Bluetooth proxy, own partition table |

(Flags are written without the `-DVENTOSYNC_` prefix, e.g. `NO_SCD41` = `-DVENTOSYNC_NO_SCD41`.)

- `NTConly` and `nosensor` compile the **same code**; they differ only in the substitutions
  (`hide_ntc_sensors`, `firmware_variant`). In `nosensor` the NTC package is still compiled but its
  entities are hidden (`internal: true`) because no thermistors are connected.
- CI additionally generates `ventosync_nosensor_mqtt.yaml` (MQTT instead of the native API) — the file is
  not in the repository.
- Missing sensors are replaced by `mock_*.yaml` packages that return clean `NaN`/`false` values.
  Never add real sensor YAML without the corresponding mock for the fallback variants.
- **Bluetooth proxy** (`packages/integration/bluetooth_proxy.yaml`, only in `ventosync_nosensor_btproxy.yaml`): the BLE stack
  (~610 KB) does not fit ESPHome's default 4 MB layout, so the package sets `esp32: partitions:
  packages/board/partitions_4mb_btproxy.csv` (2 × 0x1F0000 app, 64 KB NVS), BLE 5.0 off, `assertion_level: SILENT`,
  logger INFO. Image 1,992,152 / 2,031,616 B (~39 KB headroom, 0.10.54 / ESPHome 2026.10.0) — CI compiles it only after the merge (PRs build
  `ventosync-full` and `ventosync-v2-sgp41`), so check the size locally (`esphome compile ventosync_nosensor_btproxy.yaml`) when a change adds code. Switching a device to it
  needs a **USB flash** (partition table, NVS lost); back to a standard variant works over OTA. BLE is off after boot,
  switch `bt_proxy_switch` ("Bluetooth Proxy", per device, also in the web dashboard). Do not add it to other variants
  without a size check. Details: `documentation/en/en_bluetooth-proxy.md`.
- **SGP41 + SHT45 board** (`packages/sensors/sensor_SGP41_SHT45.yaml`, in `ventosync_sgp41.yaml` / `ventosync_v2_sgp41.yaml`): takes over the
  SCD41 *roles* by `!extend`-ing the mocks — SHT45 → `scd41_temperature` / `scd41_humidity` (room climate),
  SGP41 VOC index → `iaq_co2eq` (`VentilationLogic::voc_index_to_co2eq()`: 400 + 4 × (index − 50), 400–2200 ppm) →
  `effective_co2`. The variant must therefore include `mock_scd41.yaml` and `mock_bme680.yaml`. NOx index is
  used for the NOx boost (see ESP-NOW section). The mirrors go through `sensor_value_or_nan()` (`climate.h`): the drivers keep their last value on
  read errors, so the mirrors turn NaN after 2 min of warnings or a failed component. The package also sets
  `wrg_dashboard: voc_index_id / nox_index_id` (dashboard label "CO2-Äquivalent (VOC)" + index rows). The SGP41 board needs 3.3 V at H2 (identical on PCB v1 and v2).
- **SCD43 stale guard** (`sensor_SCD41.yaml`, all variants with the real SCD43): `scd4x` also keeps its last values on read
  errors, and its sensors are read directly everywhere (PID, room fusion, dashboard), so a 10 s `interval` calls
  `invalidate_stale_sensors()` (`climate.h`): after 2 min of warnings or a failed component the three sensors are set to
  NaN via `internal_send_state_to_frontend()` — **not** `publish_state()`, the `median` filter skips NaN.
- PCB v2 variants use their own `firmware_variant` (`ventosync-v2-*`) and therefore their own OTA manifest —
  a v1 device is never offered v2 firmware and vice versa.
- Adding a variant or flag: add it to **`.github/variants.json`** (single source of the CI build matrix, the YAML lint
  and the generated MQTT variant; `"pr": true` = also compiled on pull requests) and to this table.
  `python3 .github/scripts/ci_config.py check` (runs in `lint.yaml`) fails if a `ventosync*.yaml` is not listed, its
  `firmware_variant` differs from the `name`, or the table misses it. `upload_all.sh` is **not** derived from it — it
  only lists the maintainer's manually updated test devices; do not extend it.

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
# (covers components/ventilation_logic/ and the pure headers in components/ventilation_group/)
g++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer \
    -o simple_test tests/simple_test_runner.cpp \
    components/ventilation_logic/ventilation_logic.cpp \
    components/ventilation_group/ventilation_state_machine.cpp \
    -Icomponents && ./simple_test && rm -f simple_test
```

Always validate before uploading. CI pins ESPHome to `2026.10.0` (needs Python 3.12–3.14) — use the same version
locally; `upload_all.sh` aborts on a mismatch (override `ALLOW_ESPHOME_MISMATCH=1`).
**ESPHome upgrade:** change `ESPHOME_VERSION` in `.github/workflows/build.yaml`, `.github/workflows/lint.yaml` and
`upload_all.sh`, plus the version in this file and both READMEs; validate all variants (see `lint.yaml`).
Pure logic belongs in `components/ventilation_logic/` (no ESPHome dependencies) so it can be unit-tested;
`components/helpers/` is only compiled by ESPHome and has no native tests.

---

## Operating Modes

| Mode | German UI / HA value | C++ (`VentilationMode`) | Behaviour |
|---|---|---|---|
| Smart automatic | `Smart-Automatik` | `auto_mode_active` flag on top of `MODE_ECO_RECOVERY` / `MODE_VENTILATION` | Sensor-driven PID demand (CO2, humidity/enthalpy), summer bypass |
| Heat recovery | `Wärmerückgewinnung` | `MODE_ECO_RECOVERY` | Alternating push-pull cycles (70 s at level 1 → 50 s at level 10) |
| Cross-ventilation | `Durchlüften` | `MODE_VENTILATION` | Continuous one-directional ventilation with optional timer |
| Boost ventilation | `Stoßlüftung` | `MODE_STOSSLUEFTUNG` | 2 h cycle: 15 min one-way burst (A in / B out) / 105 min pause, direction inverted every 2nd burst |
| Off | `Aus` | `MODE_OFF` | Fan stopped, room-wide like every mode; Wi-Fi/sensors stay on (no sleep state) |

The German strings are the values of the HA select **and** the HA fan preset modes.
LED behaviour per mode: `documentation/en/en_operating-modes.md`.

---

## ESP-NOW Protocol & Cluster Synchronization

- **Protocol version:** `v15` (`PACKET_MAGIC = 0x42`, `PROTOCOL_VERSION = 15` in `ventilation_packet.h`;
  v8 added `room_co2` and the room-wide Smart Climate Control thresholds, v9 added `room_humidity`,
  v10 added `room_flags`: bit 0 room-wide HVAC switch, bit 1 the sender's own HA AC state, bit 2 its own HA
  window state, bit 3 its own radar presence — bit 3 added in 0.10.23 without a bump, layout unchanged;
  bits 4–6 (0.10.38, no bump): bit 4 room-wide NOx-boost switch, bit 5 the sender's own NOx boost, bit 6 its own
  strong NOx event — an older Master never sets bit 4 and therefore keeps the room's NOx switch off;
  v11 added `vacation_pre_mode_index` / `vacation_pre_intensity`, the room leader's pre-vacation snapshot;
  v12 added `auto_override_level` / `auto_override_remaining_s` / `auto_override_duration_min`, the room's
  Smart-Automatik level override and its duration setting; v13 added the authentication trailer, see below;
  v14 added `vacation_preset_mode_index` / `vacation_preset_level`, the room-wide vacation preset;
  v15 added `summer_cooling_threshold`, the room-wide summer cooling threshold).
- **Changing `VentilationPacket`** (`components/ventilation_group/ventilation_packet.h`, with `PeerState`, the room
  flags and `update_peer_state()`): bump `PROTOCOL_VERSION`, keep the `static_assert(sizeof + trailer ≤ 250)`,
  extend the range checks in `ventosync::packet::parse()` and the unit tests T-8a/T-8b,
  update the version above (and in the `network_sync.h` entry below), and note in the CHANGELOG that **all devices of a
  room must be flashed** (mixed versions reject each other's packets).
- **Frame authentication (v13, `espnow_auth.h`, `ventosync::espnow_auth`):** every frame (VentilationPacket and
  the ROOM_DISC / ROOM_CONF strings, unicast and broadcast) ends with `epoch | seq | tag` (16 bytes,
  HMAC-SHA256 over sender MAC ‖ payload ‖ counter, truncated to 8 bytes). Send **only** via `espnow_send_frame()` /
  `espnow_seal_frame()` and parse only what `espnow_open_frame()` returned (`network_sync.h`) — never call
  `global_esp_now->send()` with an unsealed payload. Key: `espnow_key` secret → SHA-256 in `ventilation_group/__init__.py`
  (CI: repository secret `ESPNOW_KEY`, required for release builds). `epoch` is persisted once per boot (ESPHome
  preference `ventosync_espnow_tx_epoch`); the `ReplayGuard` (RAM, 16 senders) forgets a sender after
  `replay_expiry_ms()` = max(15 min, 2 × sync interval) without an accepted frame — never a fixed value below two
  heartbeats, or replays pass in every silent gap. HMAC on the device = mbedtls (`#if ESP_PLATFORM`), the portable
  SHA-256 is the native test implementation; `self_test()` (RFC 4231) gates ESP-NOW at runtime. The pre-queue
  duplicate guard in `handle_espnow_receive()` hashes the whole frame (counter makes genuine frames unique).
  Not encrypted — payloads stay readable; the release key is extractable (documented like the API key).
- **Safe deserialization:** only via `std::memcpy` into a stack-local struct in `ventosync::packet::parse()`
  (pure, unit-tested; `espnow_handler::validate_and_parse_packet()` only logs) — **never** cast `uint8_t*` to a
  struct pointer.
- **Discovery:** broadcast `ROOM_DISC` on boot → matching Floor + Room ID → unicast pairing.
- **Peer cache:** LRU, capped at `MAX_PEERS` = 10 peers (`VentilationController::peers` and the ESP-NOW `peer_cache`).
- **Master/Slave authority:** device ID 1 (`MASTER_DEVICE_ID`, check with `is_master()` / `is_from_master()`) is Master. Slaves mirror mode and — in Smart-Automatik — the
  Master's **fan level**. `Aus` is a room mode like the others: a device in `MODE_OFF` still follows the room
  (no "autonomy" exception since 0.10.26), so switching on is as room-wide as switching off.
  Consequence: the Master decides for the whole room, so every demand must reach it.
- **Stoßlüftung schedule:** `remaining_duration_ms` carries the remaining time of the 4 h super-cycle (two bursts);
  slaves re-align to the **Master** only (`stoss_misaligned` in `ventilation_group.h`), so a rebooted slave never
  restarts the room's schedule and push-pull pairs never blow in the same direction.
- **Heartbeat:** `sync_interval_config` (default 60 s). **Peer timeout:** `PEER_TIMEOUT_MS = 900000` (15 min)
  for mode/level following and the dashboard.
- **Room-wide settings** (sliders incl. the summer cooling threshold, HVAC switch, vacation preset): a change is sent as `MSG_STATE` (`sync_settings_to_peers()`),
  applied in `handle_config_sync()` only inside the HA slider ranges, and re-asserted by the Master heartbeat.
- **Level from a peer** (`handle_state_sync()`): decided by `ventosync::packet::level_to_adopt()` — by the packet's mode
  (a packet that switches the room out of Smart-Automatik arrives while the receiver still runs it) and compared with
  `fan_intensity_level`, never with `VentilationController::current_fan_intensity` (`on_packet_received()` has already
  updated that cache — a level that arrived first in a heartbeat would never reach the fan).
- **Room-wide fusion** (`components/ventilation_logic/room_fusion.h`, `ventosync::room`):
  - Devices broadcast **only their own inputs**: `pid_demand` (NaN without sensors), `room_co2`,
    `room_humidity`, the HA-pushed AC / window state (`ROOM_FLAG_AC_ACTIVE`, `ROOM_FLAG_WINDOW_OPEN`), the own
    radar presence held 30 s (`ROOM_FLAG_PRESENCE`), the own NOx boost (`ROOM_FLAG_NOX_ACTIVE` / `_STRONG`).
    **Never re-broadcast a fused/adopted value** —
    two devices would latch each other at a high level (feedback loop, CHANGELOG 0.10.21).
  - Receivers fuse the maximum (or OR) over all fresh peers — not only the last received packet. Freshness:
    `fusion_max_age_ms()` = max(5 min, two heartbeats), capped at `PEER_TIMEOUT_MS`.
  - **Vacation snapshot** (`leader_vacation_snapshot()`): the exception to "only own inputs" — while the room
    leader is in vacation mode it broadcasts the mode/level it saved *before* the vacation, so followers can
    restore the room state even if the leader is replaced or reboots without NVS. Followers adopt it only in
    `VACATION_FOLLOWING`, never re-broadcast it (they send `VACATION_SNAPSHOT_NONE`), and pick the lowest
    device ID among fresh peers, so exactly one snapshot wins.
  - **Vacation preset** (`vacation_preset_mode_index_val` / `vacation_preset_level_val`, HA "Urlaubsmodus
    Betriebsmodus" / "Urlaubsmodus Intensität", v14): the mode and level the leader applies — a room-wide *setting*
    like the sliders (`vacation_adopt_preset()` in `handle_config_sync()`), not part of the snapshot.
- **Level override** (`level_override.h`, `ventosync::auto_override`, `VentilationController::level_override`): the
  Level button in Smart-Automatik holds its level room-wide for `auto_override_duration_val` (30–300 min, default 60,
  room-wide setting synced like the min/max levels). Room-wide **state** like the mode, not a fused
  value: `MSG_STATE` from any device is adopted in full (incl. "none"); the Master heartbeat / boot status response
  only start or re-align it, never end it (a rebooted device must not cancel it). Countdown on each device's own clock,
  never restarted by an adoption. Ended by expiry or `cycle_operating_mode()`. Safety: HVAC cap clamps it, a health
  emergency (HVAC CO2 / mold guard or the override's `Co2Guard`) lets the automatic go higher (`resolve_level()`).
- **NOx boost** (`nox_boost.h`, `ventosync::nox`, only fed on the `sgp41` variants via `nox_state` in `globals.h`):
  Smart-Automatik only, room-wide switch `nox_boost_enabled_val` (room flag bit 4, synced like the HVAC switch),
  threshold per device (`nox_boost_threshold_val`, 20–100). Start ≥ threshold for 60 s, end 10 min below half of it,
  cap 60 min. **Outdoor-air test:** every intake window of the SGP41 device must lower the index, otherwise immediate
  abort + lockout (event `nox_abbruch_aussenluft`). While the room boosts, `evaluate_auto_mode()` sets the demand to
  1.0; a strong event (≥ 150) lifts the HVAC cap; any boost is a health emergency for the level override.
- **Automatik-Status** (`auto_status.h`, `update_auto_status()` in `auto_mode.h`): text sensor `automatik_status`
  (reason + level, every 10 s), `automatik_grund` (stable key) and event entity `lueftung_ereignis` (reason key after
  30 s debounce, NOx aborts). `evaluate_auto_mode()` stores a snapshot in `auto_status_state::last`; keep the
  `event_types` list in `ui_controls.yaml` in sync with `reason_key()`.
- **HA-pushed room inputs** (API actions in `packages/integration/homeassistant.yaml`): `set_ac_active`,
  `set_window_open`. Stored as `ventosync::room::HaPushedFlag` (expires after 15 min, false while the API is
  down), shared room-wide via `room_flags`; a local change triggers an immediate `MSG_SYNC`
  (`refresh_local_room_flags()`), so HA only has to reach one device per room.

---

## C++ Architecture & Coding Rules

### Namespaces

Use `ventosync::` for project-wide constants, guards, and helper structs. Sub-namespaces in use:
`ventosync::config`, `ventosync::espnow`, `ventosync::ha_fan`, `ventosync::health`, `ventosync::hrv`,
`ventosync::hvac`, `ventosync::hw`, `ventosync::room`, `ventosync::vacation`, `ventosync::auto_override`,
`ventosync::nox`, `ventosync::auto_status`, `ventosync::espnow_auth`, `ventosync::auto_level`, `ventosync::summer`,
`ventosync::packet`, `ventosync::defaults`, `ventosync::history`.

### Modular C++ Headers (`components/helpers/`)

Complex YAML lambda logic is extracted into focused header files:

- **`globals.h`** — Central `extern` registry and shared pointers for all ESPHome sensors and entities
- **`auto_mode.h`** — Dual-PID demand evaluation, CO2 priority hysteresis, summer bypass, room demand fusion, HVAC glue
  (incl. the HA API action handlers `hvac_on_ha_ac_state()` and `window_on_ha_state()`)
- **`automation_helpers.h`** — Fan motor actuation, V-curve PWM duty calculation, soft ramps, thermal cutoff
- **`bme680_iaq_engine.h`** — BME680 IAQ index estimation, absolute humidity, and calibration logic
- **`climate.h`** — Phase-locked NTC stabilization filter, sensor mapping, and human-readable AQI formatting
- **`config_helpers.h`** — Dynamic runtime configuration (Room, Floor, Device ID, Phase) and NVS persistence
- **`espnow_helpers.h`** — Unified incoming packet validation, source tagging (`RxSource`), and dispatch routing
- **`ha_fan_helpers.h`** — HA Fan platform bridge, bidirectional state sync, and loopback suppression
- **`health_helpers.h`** — System watchdog, loop freeze detection, and stack/heap monitoring
- **`hrv_efficiency.h`** — Real-time sensible and latent heat recovery calculation (DIN EN 13141-8)
- **`led_feedback.h`** — Original VentoMaxx panel LED control (PCA9685/MCP23017), dimming, diagnostic blinks
- **`network_sync.h`** — ESP-NOW v15 mesh communication (frame authentication glue), packet handlers, peer caching, and room sync
- **`system_boot_helpers.h`** — Low-level GPIO configuration, RF-switch antenna path activation, boot discovery
- **`system_lifecycle.h`** — Multi-stage boot orchestration, filter operating hours tracking, reboot hooks
- **`user_input.h`** — Button debouncing, click/long-press handlers, timed boost countdowns, Child Lock
- **`vacation_helpers.h`** — Vacation mode scheduling and low-intensity interval ventilation

### Custom Components (`components/`)

- **`ventilation_group`** (`VentilationController`, `VentilationStateMachine`): multi-device coordination,
  peer tracking, 5 s soft ramps (`RAMP_DURATION_MS`), push-pull timing.
- **`ventilation_logic`**: hardware-agnostic, unit-tested logic.
  - `ventilation_logic.h/.cpp` (`VentilationLogic`): static math — fan curve, PWM, cycle timing, efficiency,
    Smart-Automatik level mapping (`calculate_auto_target_level`, always inside the level window).
  - `hvac_coordinator.h` (`ventosync::hvac::Coordinator`): Smart Climate Control state machine (AC debounce and
    expiry, CO2 emergency, mold guard), applied by `auto_mode.h` as a modifier to Smart-Automatik. Also holds the
    configuration ranges (`CO2_THRESHOLD_*`, `EMERGENCY_CO2_*`, `MAX_FAN_LEVEL_CONFIG_*`) that must match the
    HA sliders in `ui_controls.yaml`.
  - `room_fusion.h` (`ventosync::room`): room-wide max of CO2 / humidity / peer demand, OR of the peers' AC /
    window / presence flags (freshness ≥ 5 min), `HaPushedFlag` for expiring HA-pushed inputs, `PresenceHold`.
  - `level_override.h` (`ventosync::auto_override`): Smart-Automatik level override (30–300 min) (`LevelOverride`
    countdown + packet adoption, `Co2Guard`, `resolve_level()`).
  - `nox_boost.h` (`ventosync::nox`): NOx boost `Detector` (trigger, hold, cap, outdoor-air test, lockout).
  - `auto_status.h` (`ventosync::auto_status`): Automatik-Status reason/text (`describe()`), keys, `ReasonTracker`.
  - `espnow_auth.h` (`ventosync::espnow_auth`): SHA-256 / HMAC-SHA256, frame trailer `seal()` / `verify()`, `ReplayGuard`.
  - `auto_level.h` (`ventosync::auto_level`): Smart-Automatik soft ramp (`ramp_level()`, ±1 / ±2 after a mode
    change) and `master_level()` (slaves follow a fresh Master).
  - `summer_bypass.h` (`ventosync::summer`): summer bypass `decide()` (thresholds, hysteresis, AC lock) and
    `RemeasureGuard`; owns the `SUMMER_*` constants.
  - `history.h` (`ventosync::history`): dashboard 24 h history `Recorder` (5-min averages, RAM ring buffer, `GET /history`).
  - `defaults.h` (`ventosync::defaults`): factory defaults of the Smart-Automatik settings (see *Factory defaults*).
- **`ventilation_group/ventilation_packet.h`** (pure, unit-tested): wire format (`VentilationPacket`, `MessageType`,
  `ROOM_FLAG_*`, `PROTOCOL_VERSION`), `PeerState`, `ventosync::packet::parse()` and `update_peer_state()`.
- **`wrg_dashboard`** (`WrgDashboard`): async web server hosting the local SPA (`/ui`, `/state`, `/stream`, `/history`,
  `/meta`, `POST /set`). The page is `dashboard.html` — gzipped into flash by `__init__.py` at build time, edit it directly.
  The server runs in its own task: read entities / `peers` only in `loop()` (it serializes `/state` and `/meta`),
  never in the request handler; the history ring buffer (`history.h`) is shared under the same mutex. `/stream`
  (Server-Sent Events) follows ESPHome's `/events` pattern: a `StreamSession` is tied to the socket, sent to only from
  `loop()` (non-blocking) and deleted only after httpd's `free_ctx` (`destroy()`) marked it dead. Entities are
  registered by JSON key (tables in `__init__.py`); `/set` accepts only those keys, via POST with the `X-VentoSync`
  header and same origin (CSRF).

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

### Factory defaults

First-boot values of the room-wide Smart-Automatik settings and the vacation preset exist on two sides: the `default_*` / `hvac_default_*`
substitutions in `packages/base/ventosync_core.yaml` (used by the globals and HA numbers) and the C++ constants in
`components/ventilation_logic/defaults.h` / `hvac_coordinator.h` (fallbacks, e.g. in `build_packet()`). Change both —
`ci_config.py check` fails on a mismatch. Never hard-code a default value in C++ or YAML.

### NVS / Flash wear

- Filter runtime: accumulate in RAM, write to NVS every **8 hours** max (was 30 min → 1440 writes/day)
- BME680 baseline: flash save gated by `save_interval_ms` (1 h minimum) and `save_delta_pct` (2 %) in
  `bme680_iaq_engine.h`
- Prefer native `restore_mode` for UI switches (no extra global variable)

---

## Known Pitfalls

- **`"Smart-Automatik"`** must match exactly in the YAML select/preset options and all C++ code
  (`globals.h`, `network_sync.h`, `user_input.h`, `dashboard.html`). A mismatch silently breaks mode
  switching (CHANGELOG 0.8.169).
- **PID output without sensor is `0.0`, not NaN.** The ESPHome PID writes 0 when its input is NaN, and
  `co2_pid_result` starts at `0.0`. To detect "no local sensor", check the sensor value
  (`effective_co2->state`), not the PID output.
- **Re-broadcasting fused values** creates latching feedback loops (see ESP-NOW section, CHANGELOG 0.10.21).
- **Template switch mirroring a global** (`lambda: return id(x);` + turn_on/off actions): set
  `restore_mode: DISABLED`, otherwise the boot-time restore runs the turn_off action and overwrites the global
  (e.g. `switch.kindersicherung` cleared the child lock on every reboot until 0.10.27)
  (and, for room-wide settings, broadcasts it).
- **Compile-time `${room_id}` ≠ runtime room:** the room is configured at runtime (`config_room_id`, NVS); never
  derive per-room HA entity IDs from the substitution — push room data via API actions instead (see
  `set_ac_active`, `set_window_open`).
- **Phase-locked NTCs in `MODE_VENTILATION`:** with one-way airflow only the NTC facing its own air stream
  publishes (intake: outdoor, exhaust: indoor); the other one freezes at its last value. Never use it —
  `get_effective_temperatures()` falls back to peers / `HeldReading` (≤ 30 min) and `guard_summer_bypass()`
  returns to heat recovery to re-measure.
- **No power-off / sleep state:** `system_on`, the Wi-Fi-off long press and `system_sleep` were removed in 0.10.26
  (rev. 1 PCB cannot wake from deep sleep). `api.reboot_timeout: 0s` keeps standalone devices (no HA client) from
  rebooting every 15 min. The Power button toggles `Aus` ↔ `last_active_mode_index`.
- **Vacation mode is led by the Master:** only device ID 1 (or a device without a reachable Master) snapshots,
  applies and restores (`vacation_is_room_leader()`); `vacation_state` (NVS) makes triggers idempotent.
  Followers take the leader's snapshot from the packet (`vacation_adopt_leader_snapshot()`, protocol v11), so a
  device that joins or is promoted mid-vacation still restores the room's pre-vacation mode and level.
- **`vent_timer` = 0 means continuous** (`ventilation_duration_ms = 0`); only values > 0 are clamped to 1–1440 min.
- **`static` locals in `inline` header functions** are shared state for the whole firmware — they persist across
  mode switches and are not per-instance. The Smart-Automatik state lives in `auto_mode_state` (`auto_mode.h`:
  held readings, summer re-measure guard, ramp, hold-off); prefer such a named namespace over new hidden `static`s.
- **`effective_co2` may be a VOC-based eCO2 estimate** (BME680 in the `bme680_only` variants, SGP41 VOC index in
  the `sgp41` variants); room-wide maxima include it as if it were an NDIR value. The SGP41 VOC index is relative to the
  last 24 h — a constant pollution level drifts back to index 100 (≈ 600 ppm).
- **Local builds bump `version.json`** via `version_bump.py` — never commit that local bump.
- **OTA is encrypted with the api key** (`ota: - platform: esphome` + bare `encryption:`, since 0.10.31): no OTA
  password, uploads without the key are rejected, `web_server: ota: false` closes the plaintext `/update` endpoint
  (ESPHome ≥ 2026.9.1 no longer warns about it). All devices and the repository secret `API_ENCRYPTION_KEY` must
  share one key; `build.yaml` refuses to build non-PR firmware without that secret. Never re-add `password:` —
  ESPHome rejects `password` together with `encryption`.
  The key is compiled into the **public** release binaries, so for release firmware it is not secret (extractable);
  the encryption only raises the bar. Keep that limitation documented (README "OTA Security").

---

## PID Controller & Smart-Automatik Mode

- **CO2 PID:** `kp = 0.001`, `ki = 0.0000005` (slow integral — smooth level transitions every 20–30 min)
- **Humidity PID:** `kp = 0.05`, `ki = 0.00001`
- **Anti-windup:** both PIDs run in COOL mode with `min_integral: -1`, `max_integral: 0` — never raise `max_integral`:
  hours below the setpoint would wind the integral up to "heat" and delay the demand by hours (issue #4).
- **Conflict resolution (hysteresis):** CO2 grabs exclusive priority at `co2_demand >= 0.01`, releases at
  `< 0.005`; while CO2 controls, the higher of both demands is used.
- **Room fusion:** effective demand = max(local sensor demand, freshest peer demands) — see ESP-NOW section.
- **Presence offset** (`auto_presence_val`, -5…+5, room-wide setting): applied only in the **manual** modes while
  radar presence is detected anywhere in the room (`room_presence_detected()` in `automation_helpers.h`), never
  in Smart-Automatik. Room-wide so push-pull pairs stay balanced.
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

- **Docs-only changes skip the firmware jobs:** `build.yaml` (job *Detect Changes*), `lint.yaml` and `codeql.yaml`
  run `.github/scripts/detect_changes.py`; when every changed file matches `DOC_ONLY_PATTERNS` (`*.md`,
  `documentation/`, `images/`, `EasyEDA-Pro/`, `ha_integration_example/`, `ESPHome-VentoMaxx-Analyser/`, image/PDF
  files, `LICENSE`), the jobs start but skip their steps, so every check still reports success (a skipped matrix
  job would leave required checks pending). Unknown files, `workflow_dispatch`, `schedule` and an undeterminable diff
  always build; a push also builds while the tag `v<version.json>` is missing (pending release). This is independent
  of the `no-release` label — a firmware PR labelled `no-release` is still compiled. Adding a docs-only
  directory: extend `DOC_ONLY_PATTERNS`.
- **`build.yaml`**
  - *Run Unit Tests*: native `g++` build of `tests/simple_test_runner.cpp` with ASan/UBSan (command above).
  - *Build* matrix from `.github/variants.json` (`ci_config.py matrix`, computed in the *Detect Changes* job):
    **pull requests build the `"pr": true` variants `ventosync-full`** (required check `Build ventosync-full`) **and
    `ventosync-v2-sgp41`** (PCB v2.0 board package + the only variant with the SGP41/SHT45 drivers); the size-critical
    Bluetooth proxy image is compiled only after the merge — compile it locally when a change adds code. Push to
    `master` / `workflow_dispatch` build all
    variants (incl. the generated `nosensor-mqtt`), ESPHome pinned to `2026.10.0`, secret-free OTA configs. The release
    job needs all builds. Check names stay `Build <name>` — renaming a variant changes a required check.
  - *Create Release* (push to `master`, or `workflow_dispatch` with `force_release: true`): tag
    `v<version.json>`, `.ota.bin`, `.factory.bin`, `manifest-<variant>.json`; release notes = first section
    of `CHANGELOG.md`. **Skipped if the tag already exists** (never silently overwrites a release).
- **`lint.yaml`**: `ci_config.py check` (variant list ↔ files / `firmware_variant` / CLAUDE.md table, YAML
  `default_*` substitutions ↔ C++ defaults; runs on every change) and `esphome config` validation of **all** variants
  of `variants.json` incl. the generated MQTT one (dummy secrets) — catches YAML errors of the variants that PRs no
  longer compile.
- **`codeql.yaml`**: CodeQL analysis of the C/C++ code (`build-mode: none`, `security-extended` +
  `security-and-quality`). Without the ESPHome headers CodeQL misparses functions / locals, so the quality rules
  `cpp/unused-static-variable` and `cpp/short-global-name` are excluded via `query-filters` (false positives only).
- **`security.yaml`**: TruffleHog secret scanner.
- **`version-guard.yaml`** (PRs only): runs `.github/scripts/check_version.py` — `version.json` must be
  greater than on `master`, match the top `CHANGELOG.md` entry, and its tag must not exist yet.
  Label `no-release` skips the check. Run locally: `python3 .github/scripts/check_version.py`.

Devices use NVS-stored Wi-Fi credentials; secrets are stripped from release binaries — except the api key
(`API_ENCRYPTION_KEY` repository secret), which also authenticates OTA uploads; push/`workflow_dispatch` builds fail
without it, PR builds fall back to the public dummy key.

---

## Versioning & Release

- `version.json` is the single source of truth: it is included into the firmware
  (`packages/base/esp32c6_common.yaml`) and defines the release tag `v<version>`.
- **One version bump per PR, not per commit.** Every merge into `master` publishes a release; CI never bumps
  the version itself.
- **Release PR** (firmware / behaviour change): bump `version.json` (convention: separate commit
  `chore: bump version to x.y.z`) and add the matching top entry `## [x.y.z] - YYYY-MM-DD` to `CHANGELOG.md`.
- **Non-release PR** (docs, CI, tooling only): label `no-release`, leave `version.json` and the CHANGELOG
  version headings untouched. After the merge the release job finds the existing tag and skips.
- Enforcement: *Version Guard* (PR check, should be **required** in the branch protection of `master` —
  direct pushes to `master` bypass it) and the release job's existing-tag skip.
- Rebuilding an existing release on purpose: *Actions → Build ESPHome Firmware → Run workflow* with
  `force_release: true`.
- Parallel PRs claiming the same version: the second one fails the guard after the first is merged
  (tag exists) → bump again.
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
