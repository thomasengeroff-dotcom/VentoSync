# 📊 VentoSync Lokales Web-Dashboard

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_local-web-dashboard.md)


Das **VentoSync Lokale Web-Dashboard** ist eine eigenständige, browserbasierte Steuer- und Diagnose-Oberfläche, die direkt vom ESP32-C6 Mikrocontroller bereitgestellt wird. Sie ermöglicht die volle Steuerung, Live-Messung und Konfiguration ganz ohne Home Assistant oder externe Server.

---

## ✨ Hauptfunktionen

* **Modernes responsives Design**: High-End Dark-Mode UI basierend auf **Tailwind CSS**, optimiert für Smartphones, Tablets und Desktop-PCs.
* **Echtzeit-Live-Graphen**: Integriertes **Chart.js** für flüssige Verlaufsdiagramme von CO2, relativer & absoluter Feuchte, Temperatur und Lüfter-RPM.
* **Schnelle Vor-Ort-Konfiguration**: Direkte Einstellung von Geräte-ID, Raum-ID, Etagen-ID und Lüfter-Phase (Phase A / Phase B).
* **Sensor-Diagnose**: Live-Kacheln aller Sensordaten inklusive Min/Max/Durchschnittswerten.
* **ESP-NOW Live-Peer-Ansicht**: Echtzeit-Visualisierung aller synchronisierten Geräte im selben Raum (Node-ID, Betriebsmodus, Lüfterstufe, Luftrichtung).
* **Stufen-Slider in der Smart-Automatik**: Eine Stufenänderung startet die raumweite **Stufen-Übersteuerung** (wie die Stufen-Taste, Dauer `Smart-Automatik: Übersteuerung Dauer`, Standard 60 min). Ein Hinweis unter dem Slider zeigt die Restzeit (`/state`-Schlüssel `auto_override_min`); ein Moduswechsel beendet die Übersteuerung. Siehe [Smart-Automatik Logik](de_smart-automatic-logic.md#7-stufen-übersteuerung-stufen-taste).
* **Bluetooth-Proxy-Schalter** *(experimentell)*: In `ventosync_nosensor_btproxy.yaml` zeigt die Gruppe *Steuerung* einen Schalter „Bluetooth Proxy“ (dieselbe Entität wie in HA). Andere Varianten zeigen ihn nicht. Siehe [Bluetooth-Proxy](de_bluetooth-proxy.md).
* **SGP41 + SHT45 Sensorboard** (nur `ventosync_sgp41` / `ventosync_v2_sgp41`): Die Kachel *Luftqualität* beschriftet den CO2-Wert als **„CO2-Äquivalent (VOC)“** und zeigt zusätzlich **VOC-Index** und **NOx-Index** (`/state`-Schlüssel `voc_index`, `nox_index`, nur von diesen Varianten gesendet). Andere Varianten behalten die Beschriftung „CO2“. Die Peer-Karten zeigen weiterhin „CO2“ — das ESP-NOW-Paket enthält den Sensortyp nicht.
* **Autarker Standalone-Betrieb**: Vollständig ohne Home Assistant nutzbar. Erreichbar unter:
  * **VentoSync Dashboard**: `http://<deine-IP-Adresse>/ui` (oder `http://esptest.local/ui`)
  * **Standard-ESPHome-UI**: `http://<deine-IP-Adresse>/`

---

## 📸 Dashboard-Vorschau

### VentoSync Dashboard (Tailwind CSS UI)

<p align="center">
  <img src="../screenshots/wrg-dashboard1.png" alt="WRG Dashboard Einstellungen & Übersicht" width="48%" />
  &nbsp;
  <img src="../screenshots/wrg-dashboard2.png" alt="WRG Dashboard Verbundene Geräte & Live-Sensordaten" width="48%" />
</p>

*Links: Hauptsteuerung, Lüfterstufen-Slider, Betriebsmodi und Konfigurationsparameter.*  
*Rechts: Verbundene ESP-NOW-Geräte und Echtzeit-Sensorkacheln.*

---

### Standard ESPHome Dashboard

Für Low-Level-Diagnose, Firmware-Updates und detaillierte Live-Logs stellt die Root-URL (`/`) das klassische ESPHome-Webinterface bereit:

<p align="center">
  <img src="../screenshots/Control-Dashboard1.png" alt="Standard ESPHome Dashboard Entitäten" width="48%" />
  &nbsp;
  <img src="../screenshots/Control-Dashboard2.png" alt="Standard ESPHome Dashboard Logs & Details" width="48%" />
</p>

---

## 📡 ESP-NOW Visualisierung

Das Web-Dashboard scannt und visualisiert alle aktiven Lüftungsgeräte im selben Raum in Echtzeit. Die Kachel **"Verbundene Geräte (ESP-NOW)"** zeigt:
* **Geräte-ID / Node-ID**
* **Aktueller Betriebsmodus** (Automatik, WRG, Stoßlüftung, Durchlüften, Aus)
* **Lüfterstufe** (1–10)
* **Aktuelle Luftrichtung / Phase** (Zuluft / Abluft / Stillstand)
* **Telemetrie** (Lüfter-RPM, Board-Temperatur, PID-Bedarf)
* **Luftqualität je Gerät** (CO2 in ppm, Bewertung, Temperatur, relative Luftfeuchtigkeit)

> [!NOTE]
> Die Luftqualitätswerte werden per ESP-NOW (Protokoll v11) geteilt. Jede Karte zeigt also die Messwerte des **jeweiligen** Geräts — egal ob SCD43 oder der BME680-eCO2-Fallback. Geräte ohne Klimasensor zeigen `--`; die Bewertung nutzt dieselbe Klassifizierung wie die lokale Kachel „Luftqualität", sodass beide immer übereinstimmen.

---

## 🔒 Zugriff & Sicherheit

* **Standardmäßig ohne Login** — wie die native ESPHome-Seite sind `/ui`, `/state` und `/set` für jeden Client im Netzwerk offen. Betreibe die Geräte in einem vertrauenswürdigen Netz (oder einem eigenen VLAN).
* **Optionaler Login** — im eigenen Build `auth:` unter `web_server:` in `packages/base/esp32c6_common.yaml` aktivieren (Zugangsdaten `webuser` / `webpw` in `secrets.yaml`). Das Dashboard registriert sich über `web_server_base::add_handler()`, daher schützt derselbe Login alle drei Endpunkte.
* **Grenzen für `/set`** — nur die freigegebenen Schlüssel werden angenommen, Werte sind auf 32 Zeichen begrenzt (`400 Value too long`), und höchstens 16 Änderungen warten auf die Hauptschleife (`503 Busy`, wenn voll; das Gerät loggt pro Abarbeitung eine zusammenfassende Warnung). Die Seite meldet eine abgelehnte Änderung in der Browser-Konsole.

---

## 🌐 Hybrider Offline-Betrieb

> [!IMPORTANT]
> **CDN Asset-Laden**: Während alle Steuerungslogiken, PID-Regelungen und Sensorauswertungen zu 100 % lokal auf dem ESP32-C6 laufen (voll funktionsfähig auch ohne Internetverbindung), lädt das Web-Dashboard die Bibliotheken **Tailwind CSS** und **Chart.js** über ein externes CDN (`https://cdn.tailwindcss.com`...).
>
> Für die grafische Darstellung und Diagramme im Webbrowser ist daher eine aktive Internetverbindung erforderlich. Lokale Assets werden nicht im Flash-Speicher abgelegt, um den Speicherbedarf minimal zu halten.
