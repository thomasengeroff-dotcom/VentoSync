# 📊 WRG Dashboard Component

This component implements a high-performance local web interface for the VentoSync system, providing real-time observability without relying on Home Assistant connectivity.

## 📄 File Overview

| File | Description |
| :--- | :--- |
| **`wrg_dashboard.h` / `.cpp`** | **Web Server**: Implements an asynchronous web server that serves the UI and provides `/json` and `/status` endpoints for real-time sensor data streaming. |
| **`dashboard_html.h`** | **Frontend Assets**: Contains the minified HTML, CSS, and Javascript for the "VentoSync Dashboard" as a C-string. Designed for low memory footprint and high responsiveness. |
| **`__init__.py`** | **ESPHome Integration**: Registers the dashboard component and handles the configuration of the web server port and authentication (if enabled). |

## ⚙️ Key Mechanisms

- **Local Servicing**: All assets are stored on the ESP32-C6 flash, allowing the dashboard to function entirely offline.
- **REST API**: Provides a lightweight JSON API used by both the internal frontend and external integration tools.
- **Multi-Client Support**: Optimized for simultaneous connections across multiple browser instances.
