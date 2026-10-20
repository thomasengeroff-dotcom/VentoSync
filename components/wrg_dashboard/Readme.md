# 📊 WRG Dashboard Component

This component implements a standalone, high-performance local web dashboard for the VentoSync system. It provides real-time observability, telemetry graphing, peer mesh visualization, and device control directly from the ESP32-C6 without requiring Home Assistant or internet connectivity for its backend API.

## 📄 File Overview

| File | Description |
| :--- | :--- |
| **`wrg_dashboard.h` / `.cpp`** | **Web Server & REST Controller (`WrgDashboard`)**: Implements the asynchronous HTTP request handler, serving `/ui` (Frontend), `/state` (JSON telemetry, room status, system info & ESP-NOW peers), `/meta` (limits of the writable settings) and `/set` (thread-safe mutation queue). |
| **`dashboard_html.h`** | **Frontend SPA Bundle**: Contains the embedded, single-page application (HTML, Tailwind CSS styling, Chart.js telemetry visualization, and dynamic DOM rendering) stored as a flash-resident C-string. |
| **`__init__.py`** | **ESPHome Integration**: Registers `WrgDashboard`. The tables `SENSORS`, `BINARY_SENSORS`, `TEXT_SENSORS`, `NUMBERS`, `SELECTS`, `SWITCHES` and `BUTTONS` map each config option to its JSON key (`add_<type>(key, entity)`); a key exists in `/state` only when its option is configured. Adding a setting: one table entry + the option in `packages/base/ventosync_core.yaml` + a row in `SETTINGS` (`dashboard_html.h`). |

## ⚙️ Key Mechanisms & Endpoints

- **Threading**: the web server runs in its own task. Entities and the `VentilationController` peer list are only read in `loop()` (main task), which serializes `/state` (every second while a client polled within 15 s) and `/meta` (once) into strings guarded by a mutex; the request handler only copies them. A missing or > 5 s old `/state` answers `503` and the UI retries after 300 ms.
- **HTTP Endpoints**:
  - `GET /ui`: Serves the responsive web management dashboard.
  - `GET /state`: Sensor values, Automatik-Status (`auto_status`, `auto_reason`), room status (`room_presence`, `window_guard`, `window_paused`, `vacation_active`, `is_master`), all writable settings under their keys, system info (`sys`: version, variant, uptime, reset reason, RSSI, heap) and the ESP-NOW peers (`mode_index`, `age_s`, `fresh`, room flags, telemetry, air quality). Optional keys only when configured: `voc_index` / `nox_index` / `nox_boost_threshold` (sgp41 variants), `bt_proxy` (Bluetooth proxy variant).
  - `GET /meta`: `numbers` → `[min, max, step]`, `selects` → options, `buttons` → keys. Built once from the entity traits so the UI never duplicates YAML ranges.
  - `POST /set` (form fields `id`, `val`): Queues a change for `loop()`. CSRF protection: POST only (`405`), header `X-VentoSync` required (`403`). Only keys of configured numbers / selects / switches / buttons; values > 32 characters or non-numeric values for non-selects → `400`; at most 16 pending actions (`MAX_PENDING_ACTIONS`), further requests → `503 Busy`.
  - All endpoints except `/ui` reject a foreign `Origin` header with `403` (same-origin rule of ESPHome's web server). No login unless `web_server: auth:` is enabled (it covers these endpoints).
- **Live Telemetry & Graphing**: Integrates **Chart.js** to display trend graphs for Fan RPM, Temperature, CO2, and Humidity (browser-side, ~5 minutes).
- **ESP-NOW Mesh Observability**: mode, level, direction, freshness, room flags and telemetry of all peer units in the same room.
- **Standalone Operation**: Full control and monitoring are available via any standard web browser using the ESP32's local IP address or mDNS hostname.
