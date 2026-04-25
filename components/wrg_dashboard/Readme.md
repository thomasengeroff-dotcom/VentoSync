# 📊 WRG Dashboard Component

This component implements a high-performance local web interface for the VentoSync system, providing real-time observability without relying on Home Assistant connectivity.

## 📄 File Overview

| File | Description |
| :--- | :--- |
| **`wrg_dashboard.h` / `.cpp`** | **Web Server**: Implements an asynchronous web server that serves the UI and provides `/json` and `/status` endpoints for real-time sensor data streaming. |
| **`dashboard_html.h`** | **Frontend Assets**: Contains the minified HTML, CSS, and Javascript for the "VentoSync Dashboard" as a C-string. Designed for low memory footprint and high responsiveness. |
| **`__init__.py`** | **ESPHome Integration**: Registers the dashboard component and handles the configuration of the web server port and authentication (if enabled). |

## ⚙️ Key Mechanisms

- **Hybrid Servicing**: While the core logic and JSON API function entirely offline, the dashboard currently loads **Tailwind CSS** and **Chart.js** via an external CDN. Local assets (like Material Design Icons) are stored in the `assets/` folder but are currently not referenced in `dashboard_html.h` to minimize flash usage.
- **REST API**: Provides a lightweight JSON API used by both the internal frontend and external integration tools.
- **Multi-Client Support**: Optimized for simultaneous connections across multiple browser instances.
