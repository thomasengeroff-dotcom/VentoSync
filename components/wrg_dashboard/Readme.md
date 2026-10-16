# 📊 WRG Dashboard Component

This component implements a standalone, high-performance local web dashboard for the VentoSync system. It provides real-time observability, telemetry graphing, peer mesh visualization, and device control directly from the ESP32-C6 without requiring Home Assistant or internet connectivity for its backend API.

## 📄 File Overview

| File | Description |
| :--- | :--- |
| **`wrg_dashboard.h` / `.cpp`** | **Web Server & REST Controller (`WrgDashboard`)**: Implements the asynchronous HTTP request handler, serving `/ui` (Frontend), `/state` (JSON Telemetry & ESP-NOW Mesh Peers), and `/set` (thread-safe mutation queue). |
| **`dashboard_html.h`** | **Frontend SPA Bundle**: Contains the embedded, single-page application (HTML, Tailwind CSS styling, Chart.js telemetry visualization, and dynamic DOM rendering) stored as a flash-resident C-string. |
| **`__init__.py`** | **ESPHome Integration**: Registers `WrgDashboard` and dynamically binds all configured sensors, text sensors, binary switches, numbers, and the `VentilationController`. |

## ⚙️ Key Mechanisms & Endpoints

- **HTTP Endpoints**:
  - `GET /ui`: Serves the responsive web management dashboard.
  - `GET /state`: Streams real-time system metrics (temperatures, air quality, humidity, fan RPM, filter days, remaining Smart-Automatik level override `auto_override_min`, SGP41 `voc_index` / `nox_index` (only when `voc_index_id` / `nox_index_id` are set, i.e. the sgp41 variants), and connected ESP-NOW peer tables) in JSON format.
  - `GET /set?key=<param>&val=<value>`: Dispatches parameter updates (e.g., mode, intensity, timer, CO2/humidity setpoints) via a mutex-guarded action queue processed safely in the ESPHome `loop()`.
- **Live Telemetry & Graphing**: Integrates **Chart.js** to display historical trend graphs for Fan RPM, Temperature, CO2, and Humidity in real time.
- **ESP-NOW Mesh Observability**: Continuously monitors the status, RSSI signal strength, active mode, and fan intensity of all peer units in the same room.
- **Standalone Operation**: Full control and monitoring are available via any standard web browser using the ESP32's local IP address or mDNS hostname.
