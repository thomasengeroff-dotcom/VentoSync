---
title: Einrichtung, Netzwerk & Diagnose
sidebar_label: Einrichtung & Diagnose
---

Entitäten für die Erstinbetriebnahme (Raumzuordnung), das ESP-NOW-Netz der Geräte und die Systemdiagnose. Im Alltag werden sie selten gebraucht.

## Raumzuordnung

| Entität | Entitäts-ID | Typ | Bereich | Standard | Kategorie |
| --- | --- | --- | --- | --- | --- |
| ID Stockwerk | `number.…_id_stockwerk` | Number | 0 – 9 | 1 | config |
| ID Raum | `number.…_id_raum` | Number | 0 – 10 | 1 | config |
| ID Gerät | `number.…_id_gerat` | Number | 1 – 25 | 1 | config |
| Geräte-Phase (A\|B) | `select.…_gerate_phase_a_b` | Select | `Phase A (Startet mit Zuluft)`, `Phase B (Startet mit Abluft)` | Phase A | – |
| Auf Standardwerte zurücksetzen | `button.…_auf_standardwerte_zurucksetzen` | Button | — | — | config |

Alle Werte gelten **pro Gerät** und werden gespeichert. Die Standardwerte kommen aus den Substitutionen `floor_id`, `room_id`, `device_id` und `is_phase_a` der Varianten-YAML.

- **Stockwerk + Raum** bestimmen die Raumgruppe. Geräte mit gleichen Werten finden sich automatisch per ESP-NOW.
- **Gerät 1** ist der Master des Raums. Jede Geräte-ID darf pro Raum nur einmal vorkommen.
- **Phase:** Bei einem Paar bekommt ein Gerät Phase A, das andere Phase B — so bläst immer eines herein, während das andere absaugt.
- **Auf Standardwerte zurücksetzen** setzt Stockwerk, Raum, Gerät und Phase auf die Standardwerte zurück.

Typische Belegung für zwei Räume mit je einem Paar:

| Gerät | ID Stockwerk | ID Raum | ID Gerät | Phase |
| --- | :---: | :---: | :---: | --- |
| Wohnzimmer links | 1 | 1 | 1 (Master) | A |
| Wohnzimmer rechts | 1 | 1 | 2 | B |
| Schlafzimmer links | 1 | 2 | 1 (Master) | A |
| Schlafzimmer rechts | 1 | 2 | 2 | B |

:::caution IDs per Automation ändern
Eine Änderung der IDs nimmt das Gerät aus seiner bisherigen Raumgruppe. Sie gehören in die Einrichtung, nicht in Automationen.
:::

### Anzeige der aktiven Zuordnung

| Entität | Entitäts-ID | Typ | Kategorie | Beschreibung |
| --- | --- | --- | --- | --- |
| Geräte-Konfiguration | `sensor.…_gerate_konfiguration` | Text-Sensor | diagnostic | z. B. `Floor: 1 \| Room: 1 \| Device: 1 \| Phase: A` |
| ID Stockwerk | `sensor.…_id_stockwerk` | Sensor | diagnostic | aktives Stockwerk |
| ID Raum | `sensor.…_id_raum` | Sensor | diagnostic | aktiver Raum |
| ID Gerät | `sensor.…_id_gerat` | Sensor | diagnostic | aktive Geräte-ID |

Die Sensoren werden jede Minute aktualisiert. Sie tragen denselben Namen wie die Einstellungen, liegen aber in der Domain `sensor` und eignen sich für Vorlagen, z. B. um den Master eines Raums zu finden:

```jinja
{{ states('sensor.ventosync_wohnzimmer_id_gerat') | int == 1 }}
```

## ESP-NOW

| Entität | Entitäts-ID | Typ | Werte | Gültigkeit | Kategorie |
| --- | --- | --- | --- | --- | --- |
| Sync Intervall | `number.…_sync_intervall` | Number | 1 – 360 min, Standard 1 | Raum | – |
| ESP-NOW Peers | `sensor.…_esp_now_peers` | Text-Sensor | MAC-Adressen, kommagetrennt, oder `Keine Peers` | Gerät | diagnostic |
| ESP-NOW Peerprüfung | `switch.…_esp_now_peerprufung` | Switch | an / aus, Standard an | Gerät | – |
| Force ESPNOW Discovery | `button.…_force_espnow_discovery` | Button | — | Gerät | diagnostic |

- **Sync Intervall** — Abstand der Heartbeats, mit denen der Master den Raum abgleicht. Kürzer = schnellere Anzeige der Peer-Werte im Web-Dashboard, länger = weniger Funkverkehr. Änderungen am Modus oder an Einstellungen werden unabhängig davon sofort übertragen.
- **ESP-NOW Peers** — die erreichbaren Geräte des eigenen Raums. Leer bleibt die Liste bei einem Einzelgerät oder wenn Stockwerk/Raum nicht übereinstimmen.
- **ESP-NOW Peerprüfung** — bei `an` blinkt die Master-LED, wenn ein bekanntes Gerät des Raums länger als 3 Minuten nichts gesendet hat. Bei Einzelgeräten ausschalten.
- **Force ESPNOW Discovery** — sucht sofort nach Geräten des Raums, z. B. nach dem Ändern von Stockwerk oder Raum. Automatisch geschieht das beim Start.

## System

| Entität | Entitäts-ID | Typ | Kategorie | Beschreibung |
| --- | --- | --- | --- | --- |
| WLAN Signal | `sensor.…_wlan_signal` | dBm | diagnostic | Signalstärke, alle 60 s. |
| WLAN Kanal | `sensor.…_wlan_kanal` | Sensor | diagnostic | Aktueller WLAN-Kanal. ESP-NOW funkt auf demselben Kanal. |
| WLAN SSID | `sensor.…_wlan_ssid` | Text | – | Verbundenes WLAN. |
| IP-Adresse | `sensor.…_ip_adresse` | Text | – | IP des Geräts, z. B. für das Web-Dashboard `http://<ip>/`. |
| Laufzeit | `sensor.…_laufzeit` | h | diagnostic | Zeit seit dem letzten Start. |
| Freier Speicher (RAM) | `sensor.…_freier_speicher_ram` | B | diagnostic | Freier Heap. |
| Watchdog Restarts | `sensor.…_watchdog_restarts` | Anzahl | diagnostic | Vom Watchdog ausgelöste Neustarts. |
| Internal ESP32-C6 Temperature | `sensor.…_internal_esp32_c6_temperature` | °C | diagnostic | Chiptemperatur. |
| ESPHome Version | `sensor.…_esphome_version` | Text | – | ESPHome-Version der Firmware. |
| Projektversion | `sensor.…_projektversion` | Text | diagnostic | VentoSync-Version, z. B. `0.10.39`. |
| Firmware Update | `update.…_firmware_update` | Update | – | Neue VentoSync-Version aus GitHub Releases (Prüfung alle 30 min). |

### Firmware Update

Die Update-Entität prüft das Release-Manifest der eigenen Variante auf GitHub. Eine neue Version erscheint in Home Assistant unter *Einstellungen → Updates* und lässt sich dort installieren.

```yaml
action: update.install
target:
  entity_id: update.ventosync_wohnzimmer_firmware_update
```

:::warning ESP-NOW-Protokoll
Ändert eine neue Version das ESP-NOW-Protokoll, müssen **alle Geräte eines Raums** aktualisiert werden — Geräte mit unterschiedlichem Protokoll ignorieren einander. Das CHANGELOG weist darauf hin. Am einfachsten installiert man Updates für alle Geräte gleichzeitig.
:::

```yaml
# Alle VentoSync-Updates auf einmal installieren
action: update.install
target:
  entity_id: >
    {{ states.update
       | selectattr('entity_id', 'search', 'ventosync_.*_firmware_update')
       | selectattr('state', 'eq', 'on')
       | map(attribute='entity_id') | list }}
```
