# 📊 VentoSync Local Web Dashboard

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_local-web-dashboard.md)


The **VentoSync Local Web Dashboard** is a standalone, browser-based management and monitoring interface served directly from the ESP32-C6 microcontroller. It allows full control, live monitoring, and configuration without requiring Home Assistant or an external smart home hub.

---

## ✨ Core Features

* **Modern Responsive Interface**: High-end dark mode design built with **Tailwind CSS**, optimized for smartphones, tablets, and desktop computers.
* **Automatik-Status banner** *(0.10.48)*: the top card shows what the ventilation does and why — the same text as the HA sensor `Automatik-Status` (e.g. "CO2 erhöht: 1150 ppm (Soll 1000) → Stufe 5"), with a coloured edge per reason (`automatik_grund`: red = CO2 emergency / mold guard / NOx, amber = window / vacation / override / no data, teal = CO2 / humidity demand). Below it, chips list the active room inputs: vacation mode, window open (paused or guard ignored), NOx boost, level override with remaining time, presence anywhere in the room, Smart Climate Control state (only while the switch is on), child lock, filter change due.
* **Live Chart**: **Chart.js** graph of fan RPM, room temperature, CO2 and relative humidity for the last ~5 minutes (kept in the browser only — reloading the page starts empty).
* **Device & system info**: Device ID (with a **MASTER** badge on device 1), floor / room, phase, firmware version and variant, uptime, cause of the last restart (power-on, software restart, crash, watchdog, brown-out …), Wi-Fi signal, free RAM (and the minimum since boot). IDs and phase are only displayed — set them once on the native ESPHome page (`/`, link in the card).
* **Settings** — every setting the device has, grouped like in HA; a **RAUMWEIT** tag marks room-wide settings (shared over ESP-NOW):
  * *Steuerung*: mode, fan level, Durchlüften duration (0 = continuous), child lock, Bluetooth proxy (only `ventosync_nosensor_btproxy`).
  * *Smart-Automatik*: min / max level, CO2 and humidity thresholds, presence offset, override duration, summer cooling threshold.
  * *Klima-Koordination*: switch, CO2 threshold, max level, CO2 emergency limit.
  * *NOx-Stoßlüftung*: room-wide switch; the threshold only on the SGP41 variants.
  * *Urlaubsmodus*: operating mode and level.
  * *Netzwerk*: ESP-NOW sync interval.

  Number fields take their limits from the device (`/meta`), so a value outside the HA slider range is not sent; the control briefly outlines teal (applied) or red (rejected).
* **Maintenance**: filter operating days, filter alarm, local radar presence, and a **"Filterwechsel bestätigen"** button (with confirmation) that resets the filter counter like the HA button `Filterwechsel (Reset)`.
* **Level slider in Smart-Automatik**: changing the level starts the room-wide **level override** (like the Level button, duration `Smart-Automatik: Übersteuerung Dauer`, default 60 min). A hint below the slider shows the remaining time (`/state` key `auto_override_min`); a mode change ends the override. See [Smart Automatic Logic](en_smart-automatic-logic.md#7-level-override-level-button).
* **SGP41 + SHT45 board** (`ventosync_sgp41` / `ventosync_v2_sgp41` only): the *Luftqualität* tile labels the CO2 value **"CO2-Äquivalent (VOC)"** and adds the **VOC index** and **NOx index** rows (`/state` keys `voc_index`, `nox_index`, only sent by these variants). Other variants keep the plain "CO2" label. The peer cards still say "CO2" — the ESP-NOW packet does not carry the sensor type.
* **Standalone Operation**: Full autonomy without Home Assistant. Accessible via:
  * **Custom VentoSync UI**: `http://<your-device-ip>/ui` (or `http://esptest.local/ui`)
  * **Native ESPHome Interface**: `http://<your-device-ip>/`

---

## 📸 Interface Preview

### VentoSync Dashboard (Tailwind CSS UI)

<p align="center">
  <img src="../screenshots/wrg-dashboard1.png" alt="WRG Dashboard Settings & Overview" width="48%" />
  &nbsp;
  <img src="../screenshots/wrg-dashboard2.png" alt="WRG Dashboard Connected Devices & Live Sensor Data" width="48%" />
</p>

*Left: Main controls, fan speed slider, operating modes, and configuration parameters.*  
*Right: Connected ESP-NOW peers and real-time telemetry tiles.*

---

### Standard ESPHome Dashboard

For low-level diagnostics, full entity logs, and firmware management, the root URL (`/`) provides the standard ESPHome web server interface:

<p align="center">
  <img src="../screenshots/Control-Dashboard1.png" alt="Standard ESPHome Dashboard Entities" width="48%" />
  &nbsp;
  <img src="../screenshots/Control-Dashboard2.png" alt="Standard ESPHome Dashboard Logs & Details" width="48%" />
</p>

---

## 📡 ESP-NOW Peer Visualization

The dashboard dynamically scans and lists all active ESP-NOW ventilation units in the same room. The **"Connected Devices (ESP-NOW)"** tile displays:
* **Node / Device ID**, with a **MASTER** badge on device 1 and the age of the last packet ("zuletzt vor 12 s")
* **Status dot**: teal while the peer's data is fresh enough for the room fusion (≥ 5 min, two heartbeats), amber when it is older (the card stays until the 15-minute peer timeout)
* **Operating Mode** (Automatik, WRG, Durchlüften, Stoßlüftung, Aus) — since 0.10.48 a peer in Smart-Automatik is shown as "Automatik" instead of the mode it currently runs
* **Fan Speed Level** (1–10)
* **Current Airflow Direction** (IN / OUT; `--` in mode Aus)
* **Room inputs of that device**: Präsenz, Fenster offen, Klima an, NOx (its own ESP-NOW room flags)
* **Telemetry** (fan RPM, board temperature, PID demand)
* **Air quality per device** (CO2 in ppm, rating, temperature, relative humidity)

> [!NOTE]
> The air quality values are shared over ESP-NOW (protocol v11), so each card shows the readings of **that** device's own sensor — whether it is an SCD43 or the BME680 eCO2 fallback. A device without a climate sensor shows `--`; the rating uses the same classification as the local "Luftqualität" tile, so both always agree.

---

## 🔒 Access & Security

* **No login by default** — like the native ESPHome page, `/ui`, `/state`, `/meta` and `/set` are open to every client in the network. Keep the devices in a trusted network (or a separate VLAN).
* **Optional login** — enable `auth:` under `web_server:` in `packages/base/esp32c6_common.yaml` (credentials `webuser` / `webpw` in `secrets.yaml`) in your own build. The dashboard registers through `web_server_base::add_handler()`, so the same login protects all endpoints.
* **No cross-site changes** *(0.10.48)* — `/set` only accepts `POST` with the header `X-VentoSync` (`405` / `403` otherwise) from the device's own page: requests carrying a foreign `Origin` are rejected with `403`, on `/state` and `/meta` too (same rule as ESPHome's own web server). A web page you visit in the same network can therefore no longer switch the ventilation via a hidden link or form. Scripts without a browser (`curl`) keep working: `curl -X POST -H 'X-VentoSync: 1' -d 'id=luefter_modus&val=Aus' http://<ip>/set`.
* **`/set` limits** — only the keys of the settings shown in the dashboard are accepted, values are limited to 32 characters (`400 Value too long`), numbers must be numeric and inside the entity's range, and at most 16 changes wait for the main loop (`503 Busy` when full; the device logs one summary warning per drain).
* **`/state` is built in the main loop** (once per second while a page polls) and only copied by the web server task, so the server never reads the peer list while ESP-NOW changes it. The first request after a pause answers `503` for ~100 ms; the page retries automatically.

---

## 🌐 Hybrid-Offline Operation

> [!IMPORTANT]
> **CDN Asset Loading**: While all ventilation logic, PID control, and sensor telemetry run 100% locally on the ESP32-C6 (completely functional even without an active internet connection), the web dashboard loads **Tailwind CSS** and **Chart.js** via an external CDN (`https://cdn.tailwindcss.com`...).
>
> An internet connection is therefore required for the web browser to load the dashboard styling and live charts. Local font assets and scripts are kept off the flash memory to preserve storage footprint.
