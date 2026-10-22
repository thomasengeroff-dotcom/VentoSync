# 📊 VentoSync Local Web Dashboard

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_local-web-dashboard.md)


The **VentoSync Local Web Dashboard** is a standalone, browser-based management and monitoring interface served directly from the ESP32-C6 microcontroller. It allows full control, live monitoring, and configuration without requiring Home Assistant or an external smart home hub.

---

## ✨ Core Features

* **Modern Responsive Interface**: High-end dark mode design built with **Tailwind CSS**, optimized for smartphones, tablets, and desktop computers.
* **Automatik-Status banner** *(0.10.48)*: the top card shows what the ventilation does and why — the same text as the HA sensor `Automatik-Status` (e.g. "CO2 erhöht: 1150 ppm (Soll 1000) → Stufe 5"), with a coloured edge per reason (`automatik_grund`: red = CO2 emergency / mold guard / NOx, amber = window / vacation / override / no data, teal = CO2 / humidity demand). Below it, chips list the active room inputs: vacation mode, window open (paused or guard ignored), NOx boost, level override with remaining time, presence anywhere in the room, Smart Climate Control state (only while the switch is on), child lock, filter change due.
* **Charts & history** *(0.10.49)*: three **Chart.js** charts, switchable between **Live** (last 10 minutes, every 2 s, kept in the browser), **6 h** and **24 h**:
  * *Luftqualität & Regelung*: CO2, fan level (0 = Aus) and PID demand (local sensor demand, Smart-Automatik only) — shows how the control works (hysteresis, level steps).
  * *Raumklima*: room temperature, relative and **absolute humidity** (g/m³, Magnus formula as in the enthalpy guard).
  * *Wärmerückgewinnung*: outdoor air (NTC Zuluft), supply air after heat recovery (indoor NTC during the supply phase, *0.10.51*), extract air (NTC Abluft) and the heat recovery efficiency.

  The 6 h / 24 h views come from the **device** (`GET /history`): 5-minute averages of the last 24 h in a RAM ring buffer (288 points, ~5.2 KB), so they survive a page reload and are the same in every browser. The history lives in RAM only (no flash wear) and starts over after a reboot. Below the charts a **statistics table** shows min / mean / max over the stored history (up to 24 h, from the 5-minute averages) for CO2, temperature, relative and absolute humidity, fan level, demand, efficiency, outdoor air, supply air after heat recovery and extract air.
* **Connection indicator** *(0.10.49)*: below the title — teal "Live-Verbindung (Stream)", amber "Verbunden (Abfrage alle 2 s)" (fallback), red "Keine Verbindung". When no data arrived for 6 s a red banner says how old the shown values are ("letzte Daten vor 34 s") and the cards are dimmed, so outdated values are never mistaken for current ones. Every request has a timeout (4 s, `/history` 8 s).
* **Feedback for changes** *(0.10.49)*: a change is confirmed only when the device reports the new value (**"Gespeichert: …"**); a value the device did not take within 5 s shows **"Nicht übernommen: … (aktuell …)"**, a rejected request the reason (invalid value, security check, device busy, no connection), a number outside the device's range "Wert außerhalb min–max". Messages appear bottom right; the control is outlined teal / red as before.
* **Device & system info**: Device ID (with a **MASTER** badge on device 1), floor / room, phase, firmware version and variant, mDNS host name (*0.10.49*), uptime, cause of the last restart (power-on, software restart, crash, watchdog, brown-out …), Wi-Fi signal, free RAM (and the minimum since boot). IDs and phase are set in the settings group *Gerätekonfiguration* (*0.10.55*).
* **Settings** — every setting the device has, grouped like in HA; a **RAUMWEIT** tag marks room-wide settings (shared over ESP-NOW):
  * *Steuerung*: mode, fan level, Durchlüften duration (0 = continuous), child lock, Bluetooth proxy (only `ventosync_nosensor_btproxy`).
  * *Smart-Automatik*: min / max level, CO2 and humidity thresholds, presence offset, override duration, summer cooling threshold.
  * *Klima-Koordination*: switch, CO2 threshold, max level, CO2 emergency limit.
  * *NOx-Stoßlüftung*: room-wide switch; the threshold only on the SGP41 variants.
  * *Urlaubsmodus*: operating mode and level, both room-wide (*0.10.52*).
  * *Netzwerk*: ESP-NOW sync interval, ESP-NOW peer check (Master LED blinks without peers, room-wide).
  * *Gerätekonfiguration* *(0.10.55)*, collapsed by default since 0.10.58 (only needed when setting up or moving a device — click the heading to open it): floor ID, room ID, device ID, phase A/B and "Fenstersperre ignorieren" (all per device), plus the buttons **"ESP-NOW Discovery starten"** and **"Gerät neu starten"** (with confirmation). Changing floor or room starts a new peer discovery, like the HA entities.

  Number fields take their limits from the device (`/meta`), so a value outside the HA slider range is not sent; the control briefly outlines teal (applied) or red (rejected).
* **Maintenance**: filter operating days, filter alarm, local radar presence, and a **"Filterwechsel bestätigen"** button (with confirmation) that resets the filter counter like the HA button `Filterwechsel (Reset)`.
* **Level slider in Smart-Automatik**: changing the level starts the room-wide **level override** (like the Level button, duration `Smart-Automatik: Übersteuerung Dauer`, default 60 min). A hint below the slider shows the remaining time (`/state` key `auto_override_min`); a mode change ends the override. See [Smart Automatic Logic](en_smart-automatic-logic.md#7-level-override-level-button).
* **SGP41 + SHT45 board** (`ventosync_sgp41` / `ventosync_v2_sgp41` only): the *Luftqualität* tile labels the CO2 value **"CO2-Äquivalent (VOC)"** and adds the **VOC index** and **NOx index** rows (`/state` keys `voc_index`, `nox_index`, only sent by these variants). Other variants keep the plain "CO2" label. The peer cards still say "CO2" — the ESP-NOW packet does not carry the sensor type.
* **Standalone Operation**: Full autonomy without Home Assistant. Accessible at `http://<your-device-ip>/` (or `http://ventosync-a1b2c3.local/`); the **"Visit"** link on the device page in Home Assistant opens it too. `/ui` still works (old bookmarks).

> [!NOTE]
> Since 0.10.55 the dashboard replaces the native ESPHome page at `/`. ESPHome's `web_server` component stays in the firmware (`local: false`, `log: false`) only because Home Assistant builds the "Visit" link from the web port it reports; `wrg_dashboard` sets up before it, so its handler answers `/`. The native entity table and log viewer are therefore no longer reachable — logs via `esphome logs <variant>.yaml --device <ip>` or the ESPHome add-on.

---

## 🧭 Setup Wizard *(0.10.56)*

On a device that was never set up, the dashboard opens a **setup wizard** by itself — the on-site setup without Home Assistant. It appears while the device's (internal) flag *Einrichtung abgeschlossen* is off **and** floor, room and device ID still have their factory values; *Gerätekonfiguration → Einrichtungs-Assistent* opens it at any time. "×" (or Esc) postpones it until the next visit, **"Bereits eingerichtet"** hides it on this device for good. *Auf Standardwerte zurücksetzen* (HA button) clears the flag again.

1. **Room** — floor ID and room ID. Devices with the same pair form a room (same mode and cycle, shared sensor values over ESP-NOW). 0 is not offered: with floor or room 0 a device sends no discovery broadcast.
2. **Device in the room** — the device searches the room for 8 s (discovery broadcast + status request; after a floor / room change the peer list of the old room is dropped first) and lists the devices found with ID, MASTER tag and phase.
   * **Device ID**: checked live against the IDs in use — a taken ID is shown in red with the free IDs, and *Weiter* stays disabled. Suggestion: the current ID if it is free, otherwise the lowest free one. Hints for "this device becomes the Master" and "no Master (device 1) found in the room".
   * **Phase**: suggestion = the phase fewer devices of the room have, so as much air flows in as out (a pair: one A, one B). Peers report their configured phase since 0.10.56 (room flag bit 7); older firmware is shown as A.
3. **Betrieb & Smart-Automatik** — grouped into *Betriebsmodus*, *Lüfterstufen der Smart-Automatik* (min / max level, override duration) and *Grenzwerte der Smart-Automatik* (CO2 and humidity threshold, summer cooling threshold).
4. **Weitere Funktionen** *(0.10.58)* — *Klima-Koordination* (switch), *NOx-Stoßlüftung* (switch, only when an SGP41 + SHT45 board is fitted, i.e. `/state` has `nox_index`), *Urlaubsmodus* (operating mode and level of the vacation preset) and *Bedienung am Gerät* (child lock).
5. **Done** — summary in the same groups (plus *Raum & Gerät*: floor / room, ID, Master, phase, devices in the room); the flag is set.

Every value has an explanation, its factory default where one exists (*Standardwerte eintragen* fills in the defaults of the current step; the mode becomes the recommended Smart-Automatik) and a tag *(0.10.58)*:

* **RAUMWEIT** — applies to all devices of the room and comes from the Master (device 1). On a slave the values shown are the room's (taken over from the Master's status response right after the device ID was set); changing them is only needed to adjust the Master's values, and a change applies to the whole room, like everywhere else.
* **PRÜFEN** (orange) — applies to this device only and should be checked: floor / room, device ID, phase and the child lock.

Only values that differ from the device's current ones are sent, through the same `/set` path as the settings (with the usual feedback messages).

<p align="center">
  <img src="../screenshots/setup-wizard-0-welcome.png" alt="Setup wizard: welcome" width="32%" />
  <img src="../screenshots/setup-wizard-1-room.png" alt="Setup wizard step 1: room" width="32%" />
  <img src="../screenshots/setup-wizard-2-device.png" alt="Setup wizard step 2: device ID and phase" width="32%" />
</p>
<p align="center">
  <img src="../screenshots/setup-wizard-3-automatik.png" alt="Setup wizard step 3: mode and Smart-Automatik" width="32%" />
  <img src="../screenshots/setup-wizard-4-functions.png" alt="Setup wizard step 4: further functions" width="32%" />
  <img src="../screenshots/setup-wizard-5-summary.png" alt="Setup wizard: summary" width="32%" />
</p>

*Top: welcome with the legend of the tags, room, device in the room (three devices found, ID 4 and phase A suggested). Bottom: Betrieb & Smart-Automatik, Weitere Funktionen (SGP41 variant, hence with NOx-Stoßlüftung), summary.*

---

## 📸 Interface Preview

### VentoSync Dashboard (Tailwind CSS UI)

<p align="center">
  <img src="../screenshots/wrg-dashboard1.png" alt="WRG Dashboard Settings & Overview" width="48%" />
  &nbsp;
  <img src="../screenshots/wrg-dashboard2.png" alt="WRG Dashboard Connected Devices & Live Sensor Data" width="48%" />
</p>

*Left: Automatik-Status with room hints, device & system, sensor data, air quality, maintenance and all settings (RAUMWEIT tagged, Gerätekonfiguration collapsed).*  
*Right: devices in the room (ESP-NOW, with a link to their dashboard), 24 h history and statistics.*

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
* **"Dashboard dieses Geräts öffnen ↗"** *(0.10.49)*: link to the peer's own dashboard via its mDNS name (`http://ventosync-a1b2c3.local/`). Every device is called `ventosync-<last 6 hex digits of its MAC>` (`name_add_mac_suffix`), and the dashboard knows each peer's MAC from its ESP-NOW packets — no protocol change. The link needs a client that resolves `.local` names (macOS, iOS, Windows 10+, Linux with Avahi, recent Android); otherwise open the peer by its IP.

> [!NOTE]
> The air quality values are shared over ESP-NOW (protocol v11), so each card shows the readings of **that** device's own sensor — whether it is an SCD43 or the BME680 eCO2 fallback. A device without a climate sensor shows `--`; the rating uses the same classification as the local "Luftqualität" tile, so both always agree.

---

## 🔒 Access & Security

* **No login by default** — `/`, `/ui`, `/state`, `/stream`, `/history`, `/meta` and `/set` are open to every client in the network. Keep the devices in a trusted network (or a separate VLAN).
* **Optional login** — enable `auth:` under `web_server:` in `packages/base/esp32c6_common.yaml` (credentials `webuser` / `webpw` in `secrets.yaml`) in your own build. The dashboard registers through `web_server_base::add_handler()`, so the same login protects all endpoints.
* **No cross-site changes** *(0.10.48)* — `/set` only accepts `POST` with the header `X-VentoSync` (`405` / `403` otherwise) from the device's own page: requests carrying a foreign `Origin` are rejected with `403`, on `/state`, `/stream`, `/history` and `/meta` too (same rule as ESPHome's own web server). A web page you visit in the same network can therefore no longer switch the ventilation via a hidden link or form. Scripts without a browser (`curl`) keep working: `curl -X POST -H 'X-VentoSync: 1' -d 'id=luefter_modus&val=Aus' http://<ip>/set`.
* **`/set` limits** — only the keys of the settings shown in the dashboard are accepted, values are limited to 32 characters (`400 Value too long`), numbers must be numeric and inside the entity's range, and at most 16 changes wait for the main loop (`503 Busy` when full; the device logs one summary warning per drain).
* **`/state` is built in the main loop** (once per second while a page polls or streams) and only copied by the web server task, so the server never reads the peer list while ESP-NOW changes it. The first request after a pause answers `503` for ~100 ms; the page retries automatically.
* **Server-Sent Events instead of polling** *(0.10.49)* — the page opens `GET /stream` (`text/event-stream`), and the device pushes the `/state` JSON as event `state` every 2 s and right after a change; no request per update. Up to 3 streams at a time (more get `503`), a client that takes no data for 20 s is disconnected. Whenever the stream delivers nothing for 5 s (refused, reconnecting, older firmware) the page polls `/state` every 2 s instead and retries the stream after 30 s.
* **`GET /history`** — `{"interval_s":300,"age_s":42,"scale":{"co2":1,"t":10,…},"series":{"co2":[612,…],…}}`: oldest bucket first, values in fixed point (divide by `scale`), `null` = no data; the newest bucket ended `age_s` seconds ago. Channels: `co2`, `t`, `rh`, `level`, `demand` (0–1), `eff`, `t_out`, `t_in`, `t_sup` (supply air after heat recovery, *0.10.51*).

---

## 🌐 Hybrid-Offline Operation

> [!IMPORTANT]
> **Only the charts need internet**: All ventilation logic, PID control and sensor telemetry run 100 % locally on the ESP32-C6. Since *0.10.57* the dashboard's styling (**Tailwind CSS**) is built into the firmware too, so the page is complete without internet access — e.g. in the fallback hotspot. Only **Chart.js** is still loaded from a CDN (`cdn.jsdelivr.net`, ~70 KB compressed, too large for the flash of the Bluetooth proxy variant); without internet the page shows a note instead of the three charts.

The page itself (`components/wrg_dashboard/dashboard.html`) is gzip-compressed at build time (*0.10.49*) and served with `Content-Encoding: gzip`. Since *0.10.57* the build first inlines the generated stylesheet `dashboard.css` (only the Tailwind classes the page uses, ~4 KB compressed), ~93 KB → ~26 KB in flash. After changing classes in `dashboard.html`, regenerate it with `components/wrg_dashboard/tailwind/build_css.sh` (pinned Tailwind standalone CLI, no Node.js) and commit the result; CI (`lint.yaml`) fails while it is outdated.
