# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.9.13] - 2026-05-17

### Added

- **Unified ESP-NOW Receive Dispatcher**: Neue Funktion `dispatch_espnow_packet()`
  in `espnow_helpers.h` ersetzt die dreifache Code-Duplikation in den
  `on_broadcast`/`on_receive`/`on_unknown_peer` Callbacks. Enthält:
  - `RxSource`-Enum für typsichere Quell-Identifikation im Log
  - Vollständige Source-MAC-Adresse im Debug-Log
  - Minimum-Paketgröße-Validierung (2 Bytes) als erste Verteidigungslinie
- **Post-Boot UI Init Script**: Neues Script `post_boot_ui_init` trennt
  LED-Self-Test und UI-Initialisierung von der ESP-NOW Discovery
  (Single Responsibility).
- **Peer Presence Watchdog**: Neue Funktion `peer_presence_watchdog()`
  konsolidiert die doppelte Peer-Prüfung (60s in `esp_now.yaml` +
  5min in `logic_automation.yaml`) zu einem einzigen, konfigurierbaren
  Watchdog.

### Changed

- **ESP-NOW Receive Callbacks**: Alle drei Callbacks sind nun Einzeiler,
  die `dispatch_espnow_packet()` mit dem jeweiligen `RxSource`-Enum aufrufen.
- **Heap-Allokation optimiert**: Die `std::vector<uint8_t>`-Kopie findet
  nun zentral in `dispatch_espnow_packet()` statt (1× statt 3×), mit
  dokumentiertem Pfad zu einer zukünftigen Zero-Copy-Implementierung.

### Removed

- **Doppelter Peer-Watchdog**: Das redundante 5min-Intervall
  `periodic_peer_rediscovery()` aus `logic_automation.yaml` entfernt.
  Der 60s-Watchdog in `esp_now.yaml` übernimmt diese Aufgabe.

## [0.9.12] - 2026-05-16

### Added

- **Erweiterte Boot-Diagnose**: Einführung von `log_boot_diagnostics()`. Die Funktion loggt beim Systemstart detaillierte Informationen wie den Reset-Grund (ESP-IDF), verfügbaren Heap und den Watchdog-Restart-Counter.

- **System-Health Monitoring**: Implementierung von `check_brain_health()` zur Erkennung von "Brain-Freezes" (stalling main loop). Der Restart-Counter wird nun konsistent vor dem Reboot im Flash persistiert.

- **Automatisierungs-Guard**: Neue Funktion `guard_config_sync()` zur sauberen Kapselung der Boot-Race-Condition-Prüfung zwischen Home Assistant und dem lokalen Controller.

- **Config-Helper-Modul**: Neuer Header `config_helpers.h` mit zentralisierten Funktionen für Gerätekonfiguration:
  - `update_config_id()` – Unified Handler für Floor/Room/Device-ID-Änderungen mit automatischer Peer-Cache-Invalidierung bei Topologie-Änderungen.
  - `update_phase_config()` – Typsichere Phasen-Umschaltung (A/B) ohne Magic Strings im Lambda.
  - `build_config_summary()` – Ausgelagerte Diagnostic-Summary für den HA Text-Sensor.
  - `get_phase_short()` – Kurzform-Phasen-Identifier für Dashboard-Sensoren.
  - `force_discovery_with_diagnostics()` – Strukturierte ESP-NOW-Diagnoseausgabe für den manuellen Discovery-Button.

- **Verbessertes Logging**:
  - `wifi_just_connected()` loggt WiFi Connect/Disconnect Ereignisse nun explizit im Serial-Log.
  - `process_pending_broadcast()` loggt nun die exakte Anzahl der erreichten Peers bei Status-Updates.
  - `invalidate_peer_cache_and_rediscover()` loggt Peer-Cache-Resets bei Topologie-Änderungen.

- **System-Boot-Helper-Modul**: Neuer Header `system_boot_helpers.h` mit zentralisierten Boot-Funktionen:
  - `init_external_antenna()` – XIAO ESP32-C6 RF-Switch-Konfiguration für externe U.FL-Antenne mit benannten GPIO-Konstanten (`PIN_WIFI_ENABLE`, `PIN_WIFI_ANT_SELECT`).
  - `init_core_system()` – Konsolidierte Core-Initialisierung (Controller-Sync + Window-Guard-Override).
  - `boot_discovery_broadcast(phase)` – Parametrisierte Discovery-Broadcasts mit Phase-Logging (ersetzt 3× duplizierten Code).
  - `boot_finish_discovery()` – Abschluss der Discovery-Sequenz mit Peer-Status-Request und Peer-Count-Logging.

### Changed

- **C++ Modularisierung**: Weitere Auslagerung komplexer YAML-Lambdas in spezialisierte Helper-Dateien (`vacation_helpers.h`, `health_helpers.h`, `espnow_helpers.h`, `config_helpers.h`).

- **Namespacing & Clean Code**: Einführung von Namespaces (`ventosync::vacation`, `ventosync::health`, `ventosync::config`) für Konstanten zur Eliminierung von Magic Numbers. Phase-Strings sind nun als `constexpr`-Konstanten (`PHASE_A_LABEL`, `PHASE_B_LABEL`) zentralisiert.

- **Deduplizierung Config-IDs**: Die drei nahezu identischen `set_action`-Lambdas für Floor/Room/Device-ID wurden zu einem einzigen `update_config_id()`-Handler mit `ConfigField`-Enum konsolidiert. Die bewusste Entscheidung, bei Device-ID-Änderungen den Peer-Cache *nicht* zu invalidieren (Intra-Room-Identität vs. Topologie), ist nun explizit im Code dokumentiert.

- **C-Style Casts eliminiert**: Alle `(esphome::VentilationController*)` und `(uint8_t)x` Casts in `device_config.yaml` durch `static_cast<>` in den Helper-Funktionen ersetzt.

- **Struktur-Optimierung**: Einrichtung eines `include/` Symlinks für übersichtlichere YAML-Konfigurationen und Umbenennung von `fan_control.h` in `automation_helpers.h` zur besseren fachlichen Einordnung.

- **Boot-Sequenz vereinfacht**: Die `on_boot`-Blöcke in `ventosync.yaml` sind von ~30 Inline-C++-Zeilen auf 5 Einzeiler-Lambdas reduziert. Hardware-Delays bleiben bewusst in YAML (kooperatives non-blocking Yielding).
- **GPIO Magic Numbers eliminiert**: Die rohen GPIO-Nummern `3` und `14` für den Antennen-Switch sind durch `constexpr`-Konstanten im Namespace `ventosync::hw` ersetzt und mit Hardware-Dokumentation versehen.

- **Globals modularisiert**: Die monolithische `globals:`-Sektion (25 Einträge) aus
  `ventosync.yaml` wurde in vier thematische Package-Dateien aufgeteilt:
  - `globals_ventilation.yaml` – Systemzustand, Betriebsmodus, Lüfterstufe, Vacation-Memory (8 Globals)
  - `globals_automation.yaml` – Smart-Automatik-Schwellwerte, PID-Ausgänge, Filterwartung (10 Globals)
  - `globals_ui.yaml` – LED-Helligkeit, UI-State, Kindersicherung, Bounce-Richtung (4 Globals)
  - `globals_network.yaml` – ESP-NOW Peers, Peer-Check, Watchdog-Counter (3 Globals)
- **Globals-Migration**: `watchdog_restarts_count` (vorher `logic_automation.yaml`) und
  `intensity_bounce_up` (vorher `logic_automation.yaml`) in die thematisch passenden
  Globals-Dateien verschoben. Duplikate in der Quelldatei entfernt.

### Fixed

- **Implizite Peer-Cache-Logik dokumentiert**: Im Original-Code invalidierte `floor_id` und `room_id` den Peer-Cache, `device_id` jedoch nicht – ohne erkennbare Dokumentation dieser Designentscheidung. Das Verhalten ist nun im `update_config_id()`-Handler explizit kommentiert und über den `ConfigField`-Enum-Switch nachvollziehbar.


## [0.9.11] - 2026-05-16

### Added
- **Advanced BME680 IAQ Engine**: Migration der BME680-Logik auf eine dedizierte C++ Engine (`BME680IAQEngine`). Diese bietet präzises Baseline-Tracking, einen Health-Watchdog und intelligentes Flash-Wear-Leveling zur Schonung des Speichers.
- **Dynamische Thermokompensation**: Einführung eines temperaturabhängigen Offsets für den BME680 zur Kompensation der Eigenwärme basierend auf der Umgebungstemperatur.
- **Heater-Profil Optimierung**: Umstellung des BME680 Heaters auf 300°C / 150ms gemäß Bosch-Empfehlungen für Wohnraum-IAQ-Anwendungen.

### Changed
- **Code-Struktur**: Bereinigung der globalen Variablen und Korrektur der Include-Pfade in den Sensor-Paketen.
- **Dokumentation**: Ausführliche Beschreibung der neuen BME680-Architektur in den Readme-Dateien (DE/EN) hinzugefügt.

## [0.9.10] - 2026-05-16

### Added
- **Unified NTC Filter**: Implementierung der C++ Funktion `filter_ntc_combined()`, die Phase-Lock, thermische Wartezeit und saisonale Min/Max-Selektion in einer Pipeline vereint. Dies garantiert 100% Datenintegrität durch Ausschluss von Fehlmessungen während der falschen Luftrichtung.
- **Robustness Features**: Erhöhung des NTC-Timeouts auf 120s (konfigurierbar via `ntc_timeout`), um "Unavailable"-Zustände in Home Assistant bei langen Lüftungszyklen zu verhindern.
- **Median Fallback**: Einführung eines Median-basierten Fallbacks für NTC-Sensoren zur Auflösung von zirkulären Abhängigkeiten beim Systemstart.

### Changed
- **Dokumentations-Parität**: Vollständige Synchronisierung der englischen und deutschen Readme-Dateien bezüglich der neuen Filter-Architektur und technischer Exzellenz-Optimierungen.
- **Hardware-Abstraktion**: Umstellung auf `11db` ADC Attenuation (ESP-IDF v5+) und Optimierung der Sensor-Deklarationsreihenfolge im YAML.

## [0.9.9] - 2026-05-16

### Fixed
- **Volumenstrom-Schätzung**: Korrektur der bisherigen linearen Heuristik (4,5 m³/h pro Stufe). Diese wurde durch eine präzise, nicht-lineare Lookup-Tabelle ersetzt, welche die 10 benutzerdefinierten Lüfterstufen auf die realen Herstellerdaten des Ventomaxx v-wrg-1 (17–43 m³/h) abbildet. Dies stellt die physikalische Korrektheit der berechneten Wattstunden (Wh) sicher.

### Added
- **Technische Dokumentation (v0.9.9)**: Ergänzung der Readme-Dateien um wichtige technische Details: NVS-Flash-Schutz (8h-Speicherintervall), LED-Selbsttest beim Booten und die NTC-Fallback-Logik (NaN bei Sensorausfall).

### Changed
- **Energie-Integration**: Verfeinerung der HRV-Integrator-Logik. Die Berechnung der zurückgewonnenen thermischen Energie nutzt nun die realen Volumenstromwerte der jeweiligen Lüfterstufe für ein exaktes energetisches Abbild.

## [0.9.8] - 2026-05-15

### Fixed
- **WRG Effizienz Mapping**: Korrektur der Sensor-Zuweisung in `sensor_hrv_efficiency.yaml`. Die Zuluft-Temperatur (`t_supply`) wird nun korrekt vom ungefilterten Innensensor (`temp_abluft_raw`) bezogen, da der Haupt-Innensensor während der Zuluft-Phase thermisch stabilisiert ("eingefroren") bleibt. Dies behebt den Fehler, bei dem die Effizienz dauerhaft `0.0 %` anzeigte.
- **Release-Workflow**: Die Sperrdatei `.version_bump_lock` wird nun vor jedem Build automatisch gelöscht, um sicherzustellen, dass die Versionsnummer bei manuellen Release-Aufrufen zuverlässig hochgezählt wird.

### Added
- **Volumenstrom-Schätzung**: Implementierung einer heuristischen Volumenstrom-Berechnung (ca. 4,5 m³/h pro Lüfterstufe). Dies ermöglicht dem HRV-Integrator die Berechnung der tatsächlich zurückgewonnenen thermischen Energie in **Wattstunden (Wh)**.

## [0.9.4] - 2026-05-15

### Added
- **Energiebasierte WRG-Effizienz**: Die Berechnung der Wärmerückgewinnung nutzt nun eine numerische Trapez-Integration über den gesamten Luftzyklus (gemäß DIN EN 13141-8). Dies löst das Problem mathematisch instabiler "Live"-Werte bei geringen Temperaturdifferenzen in der Übergangszeit und liefert ein echtes physikalisches Abbild der zurückgewonnenen Energie.

### Fixed
- **NTC Sensor-Dokumentation**: Korrektur der vertauschten Sensor-Beschreibungen in den Readme-Dateien (Indoor/Outdoor Mapping) zur Vermeidung von Missverständnissen bei der Verkabelung.
- **C++ Namespace Konflikt**: Behebung des Fehlers `ventosync was not declared in this scope` beim Firmware-Build. Die globale Instanziierung des HRV-Rechners über ESPHomes `globals` wurde auf ein natives und threadsicheres C++ Singleton-Pattern (`get_calculator()`) umgestellt.

## [0.9.1] - 2026-05-14

### Fixed
- **UI Latenz & Synchronisation**: Behebung drastischer Verzögerungen bei der Aktualisierung von Template-Sensoren im Home Assistant Dashboard (z.B. "Lüfter Richtung" und "WRG Effizienz").
- **Event-Driven Template Sensors**: Fehlende `update_interval` Angaben führten dazu, dass Template-Sensoren, die auf C++ Variablen (`globals`) basieren, auf den langsamen 60-Sekunden Standard-Polling-Zyklus von ESPHome zurückfielen. Entsprechende Sensoren erzwingen nun via `update_interval: 1s` oder `5s` sofortige HA-Statusupdates.
- **WRG-Effizienz Schwebung**: Ein statisches 60-Sekunden-Intervall führte dazu, dass die Effizienz-Berechnung fast nie das korrekte 42-Sekunden-Zuluft-Fenster traf. Dies wurde in einen Event-gesteuerten Modus mit 5-Sekunden-Takt (`update_interval: 5s`) umgeschrieben. Die Effizienzkurve wird nun live und hochauflösend gerendert.

### Changed
- **NTC Sensor Mapping**: Konsequenter Tausch der NTC-Rollen zur physischen Realität: `temp_abluft` repräsentiert die Referenz-Innentemperatur (gemessen während der Abluft-Phase), während `temp_zuluft` die Außentemperatur repräsentiert.
- **Median-Filter Tuning**: Anpassung der Window-Send-Rate in `sensor_NTC.yaml` (`send_every: 3` -> `1`), um die trägen thermischen Messwerte mit der sehr kurzen 70-Sekunden Richtungsphase zu synchronisieren und das "Verschlucken" von gültigen Fenstern zu beenden.
- **CO2 Bewertung**: Die Text-Klassifizierung ("Gut", "Schlecht") reagiert nun ohne 60-Sekunden Verzögerung latenzfrei auf Änderungen des darunterliegenden CO2-Wertes.

## [0.8.259] - 2026-05-13
### Fixed
- **UI Responsiveness**: Resolved an issue where the "Lüfter Intensität" slider and "ESP-NOW Peerprüfung" switch in Home Assistant would take up to 60 seconds to update or revert to their previous state.
- Added explicit `publish_state()` calls to the `set_action` and `turn_on/off_action` blocks for template entities configured with `optimistic: false`.

## [0.8.257] - 2026-05-08

### Added
- **Automated Versioning**: Implemented a "Single Source of Truth" system where `esp32c6_common.yaml` dynamically pulls its version from `version.json` via YAML `!include`.

### Changed
- **Refactored `version_bump.py`**: Simplified the build script by removing manual YAML mutation logic, now relying on ESPHome's native JSON parsing capabilities.
- **Improved Maintainability**: Reduced the number of manual version update points from three to one (`version.json`).

## [0.8.256] - 2026-05-08

### Changed
- **Project Structure Optimization**: Moved `ventosync_base.yaml` to `packages/base/` for a cleaner root directory and improved modularity.
- **Path Resolution**: Updated all internal `!include` and `external_components` paths to ensure correct resolution from the new nested directory structure.

### Fixed
- **Pre-compile Script Path**: Corrected root-relative path to `version_bump.py` in `esp32c6_common.yaml` after the move.

### Added
- **Documentation Parity**: Updated `Readme.md`, `Readme_de.md`, and architectural reviews to reflect the new file locations and ensure consistent documentation.

## [0.8.255] - 2026-05-07

### Added
- **ESP-NOW Staggering**: Added random jitter (up to 5s) to periodic heartbeat broadcasts to prevent RF collisions in multi-device environments.

### Changed
- **Peer Reliability**: Increased peer stale timeout from 5 to 15 minutes (`PEER_TIMEOUT_MS = 900000`) to improve dashboard persistence during intermittent packet loss.
- **Queue Optimization**: Doubled ESP-NOW receive queue size (`RX_QUEUE_MAX_DEPTH`) from 16 to 32 packets to handle concurrent bursts from multiple nodes.
- **Sync Interval**: Reduced default `sync_interval_config` from 180 to 1 minute to ensure high-frequency status updates for the web dashboard.

### Fixed
- **Dashboard Synchronization**: Improved heartbeat authority logic to ensure peers remain visible even if cycle synchronization is ignored by Slaves.

## [0.8.253] - 2026-05-07
### Changed
- **NVS Verschleiß-Schutz**: Das Speicherintervall für die Filter-Betriebsstunden wurde von 30 Minuten auf 8 Stunden erhöht. Dies reduziert die Schreibzugriffe auf den Flash-Speicher drastisch (max. 3x pro Tag statt 48x).
- **LED Self-Test**: Beim Start (Boot) werden die LEDs jetzt explizit auf 100% Helligkeit gezwungen, unabhängig von der Nutzereinstellung. Direkt nach dem 3-sekündigen Test stellt das System automatisch die vom Nutzer konfigurierte Helligkeit wieder her (`update_leds_logic` wurde um einen `force` Parameter erweitert, um den LED-Cache sicher zu überschreiben).
- **Code Qualität**: Die verbliebenen deutschen Kommentare in `system_lifecycle.h` wurden ins Englische übersetzt, um einen einheitlichen und sauberen Quellcode zu gewährleisten.

## [0.8.252] - 2026-05-07
### Changed
- **NTC Sensor Fallback**: Verbesserung der `filter_ntc_stable` und Wärmerückgewinnung-Logik. Nicht angeschlossene NTC-Sensoren (die ~87°C melden) werden nun frühzeitig herausgefiltert und als `NaN` (Unknown) deklariert.
- **WRG-Effizienz Anzeige**: Die Berechnung der Wärmerückgewinnungs-Effizienz wird sofort abgebrochen (und liefert `NaN`), wenn relevante NTC-Sensoren fehlen. Dadurch zeigt das System saubere Sensor-Zustände in Home Assistant, anstatt ungültige Messwerte auszugeben.

## [0.8.251] - 2026-05-06
### Added
- **OTA Dokumentation**: Ausführlicher Abschnitt in `Readme.md` und `Readme_de.md` zur Ersteinrichtung über das Captive Portal ("VentoSync Hotspot"), inklusive Passwort-Hinweis (`ventosync`) und Screenshots.
- **WLAN-Stabilität**: Klärung des "Secret-Free" Prinzips in der Dokumentation, um Nutzer bei der Migration von lokalen zu GitHub-Releases zu unterstützen.

### Changed
- **Update-Intervall**: Das Intervall für die Firmware-Überprüfung (`update_interval`) in `esp32c6_common.yaml` wurde von 10 Minuten auf 30 Minuten erhöht, um unnötigen Netzwerk-Traffic zu reduzieren.

## [0.8.250] - 2026-05-06
### Fixed
- **WLAN Verbindungsabbruch nach OTA**: Ein Fehler in der CI-Pipeline (`build.yaml`) wurde behoben. Das `sed`-Kommando zum Entfernen lokaler Netzwerkdaten schnitt zu viel ab und machte die YAML-Struktur für `wifi:` ungültig. Dadurch verlor der ESP nach dem OTA-Update seine Netzwerkeinstellungen.
- **Workflow Release Notes**: Der "unexpected EOF"-Absturz im GitHub-Actions-Workflow wurde behoben, indem die Changelog-Extraktion nun `release_notes.md` nutzt anstatt den Text als Bash-Variable zu übergeben (Escaping-Probleme mit Backticks gelöst).

### Changed
- **SMB Deployment entfernt**: Der automatische Netzwerk-Kopiervorgang (`shutil.copy2`) auf Windows-SMB-Pfade (`\\192.168.178.45\...`) wurde aus `version_bump.py` entfernt, da er unter Linux/WSL zu Fehlern führte.
- **OTA Update-Intervall**: Das OTA-Abfrage-Intervall in der `esp32c6_common.yaml` wurde auf `10min` (statt `15min`) verkürzt.

## [0.8.249] - 2026-05-05
### Fixed
- **OTA-Update-Fehler behoben**: Die `http_request`-Komponente verwendet nun einen deutlich größeren Transmit-Puffer (`buffer_size_tx: 20248`). Dies behebt das Problem, dass OTA-Updates über GitHub-Releases (HTTPS/TLS) aufgrund von Puffer-Überläufen beim Handshake oder Redirect stillschweigend fehlschlugen.
- **Update-Entität Stabilisierung**: Durch die Speicher-Optimierung wird die Update-Entität in Home Assistant nun zuverlässig aktualisiert, sobald ein neues Release auf GitHub verfügbar ist.

## [0.8.247] - 2026-05-05
### Changed
- **OTA-Update-Optimierung**: Das Abfrage-Intervall der `http_request` Update-Plattform wurde auf **15 Minuten** optimiert. Dies beschleunigt die Benachrichtigung über neue Firmware-Releases in Home Assistant erheblich.
- **Fehlerbehebung CI**: Der `IndentationError` im GitHub-Workflow wurde durch Umstellung auf eine einzeilige Python-Generierung behoben.
- **Workflow-Vollendung**: Der automatisierte Release-Prozess (Changelog-Generierung & Git-Push) ist nun vollständig einsatzbereit.


## [0.8.234] - 2026-05-04
### Added
- **Multi-Varianten CI Build**: GitHub Action baut nun alle 4 Firmware-Varianten (`full`, `nosensor`, `radar_only`, `bme680_only`) automatisch als Matrix. Bei GitHub Releases werden OTA-Binaries und `manifest.json` als Assets hochgeladen.
- **WiFi Provisioning**: `captive_portal` und `improv_serial` hinzugefügt für die Ersteinrichtung neuer Geräte ohne serielle Verbindung.
- **OTA ohne Secrets**: Neue `wifi_ota.yaml` für CI-Builds — der ESP nutzt NVS-gespeicherte WiFi-Credentials vom initialen Flash, keine Secrets im Repository nötig.

### Fixed
- **Sanftanlauf (Slew-Rate) Intervall**: Das Aufrufintervall für `update_fan_logic()` wurde von 10s auf **1s** korrigiert. Das bisherige 10s-Intervall lag über der 5s-Recovery-Schwelle, wodurch jeder einzelne Tick die "Long pause detected"-Warnung auslöste und die Slew-Rate effektiv auf 1% pro Tick statt der vorgesehenen 10%/Sekunde reduziert wurde.

## [0.8.231] - 2026-05-03
### Changed
- **Härtung der State-Machine**:
    - **Stoßlüftung Sanftanlauf**: Implementierung von Sanftanlauf (Soft-Start) und Sanftstopp (Soft-Stop) für die Übergänge zwischen Pausen- und Aktivphasen in der Stoßlüftung zur Schonung der Lüftermotoren.
    - **Sync-Stabilität**: Behebung eines potenziellen Überlaufs in der Zeit-Synchronisierungslogik (`sync_time`) durch Nutzung von 64-Bit-Arithmetik für die Offset-Berechnung.
    - **Code-Bereinigung**: Entfernung von redundantem Code in der Zyklus-Synchronisierung (`set_cycle_duration`).

## [0.8.230] - 2026-05-03
### Changed
- **Präzisierung der PID-Regelung**: Die Dokumentation wurde korrigiert, um klarzustellen, dass die PID-Regelung (CO2/Feuchte) die Lüfterstufe in 10 diskreten Schritten ansteuert, statt stufenlos. Betrifft `Readme.md`, `Readme_de.md` und den VentoMaxx-Vergleich.
- **Härtung der Ventilation Group Komponente**: 
    - **Window Guard**: Korrektur der Inkonsistenz zwischen Code und Dokumentation. Die Fenstersperre greift nach **5 Sekunden** (wie im Code implementiert), Logs und Kommentare wurden entsprechend angepasst.
    - **Heartbeat-Intervall**: Das periodische Senden des Status (Heartbeat) berücksichtigt nun korrekt den via UI/YAML konfigurierbaren `sync_interval_ms` Wert, statt hartkodiert auf 60 Sekunden festzuliegen.
    - **Timing-Konsistenz**: Optimierung der Paketverarbeitung (`on_packet_received`) und Paketerstellung (`build_packet`) durch Nutzung eines einheitlichen Zeitstempels (`now`) pro Funktionsaufruf zur Vermeidung minimaler Zeitdrifts.
    - **Code-Qualität**: Refactoring der Peer-Verwaltung zur Vermeidung von Code-Duplizierung (DRY) durch Einführung eines zentralen Helpers für die Peer-Status-Aktualisierung.

## [0.8.226] - 2026-04-28
### Added
- **Kindersicherung (Child Protection Mode)**: Implementierung einer Sperrfunktion für die physische Bedienoberfläche zur Vermeidung ungewollter Interaktionen.
    - **Home Assistant Integration**: Neue Entität `switch.kindersicherung` (Kategorie Konfiguration) zum raumweiten oder gerätespezifischen Sperren der Tasten.
    - **Physischer Override**: Lokales Deaktivieren/Aktivieren direkt am Gerät durch Halten der **Modus**-Taste für 5 Sekunden möglich.
    - **Visuelles Feedback**: 
        - 3-faches Aufblinken aller LEDs bei blockiertem Tastendruck signalisiert aktive Sperre.
        - 2-faches Aufblinken aller LEDs bestätigt die erfolgreiche Umschaltung via Tastenkombination.
    - **HA-Priorität**: Änderungen über Home Assistant sind jederzeit möglich und werden nicht durch die Kindersicherung blockiert.
    - **Persistenz**: Der Status der Sperre bleibt über Neustarts hinweg erhalten (NVS).

## [0.8.210] - 2026-04-26
### Added
- **Konfigurierbarer Urlaubsmodus**: Einführung neuer Home Assistant Entitäten zur individuellen Konfiguration des Urlaubsmodus.
    - `select.urlaubsmodus_betriebsmodus`: Auswahl des Zielmodus (z.B. Stoßlüftung, Wärmerückgewinnung, Aus).
    - `number.urlaubsmodus_intensitat`: Einstellung der Lüfterstufe (1-10) für die Dauer der Abwesenheit.
    - Diese Einstellungen erlauben es, das Systemverhalten bei längerer Abwesenheit präzise an saisonale Anforderungen oder persönliche Vorlieben anzupassen.
- **Rechtlicher Haftungsausschluss (Legal Disclaimer)**: Ergänzung der Dokumentation um einen Haftungsausschluss zur Klarstellung der Unabhängigkeit von der VentoMaxx GmbH und der Eigenverantwortung des Nutzers bei Installation und Betrieb.

### Changed
- **Zentralisierung der Betriebsmodi (C++ Constants)**: Refactoring der Betriebsmodus-Namen in eine zentrale Konstante in `globals.h`.
    - Alle C++ Komponenten und YAML-Lambdas greifen nun auf eine "Single Source of Truth" (`MODE_NAMES[]`) zu.
    - Dies verbessert die Wartbarkeit signifikant, da Modus-Bezeichnungen nun an einer einzigen Stelle im Code gepflegt werden können.
- **Dokumentations-Update**: Aktualisierung der `Readme.md` und `Readme_de.md` bezüglich der neuen Urlaubsmodus-Funktionen und der Roadmap.

## [0.8.200] - 2026-04-25
### Added
- **Vollständiges Dokumentations-Audit (Doxygen)**: Umfassende Dokumentation des gesamten C++-Codes im `components/`-Verzeichnis. Alle Funktionen und Methoden in `.h` und `.cpp` Dateien verfügen nun über standardisierte Doxygen-Kommentare (`@brief`, `@param`, `@return`), um die Wartbarkeit und die Onboarding-Experience für neue Entwickler zu verbessern.
    - Betrifft: `VentilationController`, `VentilationStateMachine`, `VentilationLogic`, `WrgDashboard` sowie alle Hilfs-Header in `components/helpers/`.
- **Hybrid-Offline Dokumentation**: Aktualisierung der README-Dateien zur Klarstellung des Asset-Status. Das lokale Web-Dashboard nutzt für maximale Flexibilität Tailwind CSS und Chart.js über CDNs, während die gesamte physikalische Steuerungslogik (PID, Sensor-Fusion, ESP-NOW) weiterhin zu 100% lokal und offline-fähig auf dem ESP32-C6 läuft.

## [0.8.197] - 2026-04-24
### Fixed
- **ESP-NOW Peer-Tracking & Dashboard-Anzeige**: Behebung eines Fehlers, bei dem Peers im Web-Dashboard überschrieben wurden oder gar nicht erschienen, wenn mehrere Geräte die gleiche `device_id` (z.B. Standardwert 1) verwendeten.
    - Umstellung des Peer-Trackings im `VentilationController` von `device_id` auf die eindeutige **MAC-Adresse** (`src_mac`).
    - Härtung der Loopback-Erkennung: Eigene Pakete werden nun zuverlässig anhand der lokalen MAC-Adresse gefiltert (`is_local_mac`), statt sich auf die konfigurierbare `device_id` zu verlassen.
    - Dies stellt sicher, dass das Dashboard auch bei ID-Kollisionen alle physischen Geräte korrekt auflistet und die gegenseitige Steuerung (Mode/Fan-Sync) robust funktioniert.

## [0.8.187] - 2026-04-23
### Added
- **Tiered NVS Storage für Filter-Betriebsstunden**: Die Filter-Laufzeit wird nun im RAM minutengenau akkumuliert, aber nur alle 30 Minuten dauerhaft im Flash (NVS) gespeichert. Dies verhindert den Verschleiß des Flash-Speichers durch zu häufige Schreibvorgänge (NVS Wear-Out) bei gleichzeitiger Erhaltung der Persistenz über Reboots hinweg.

### Changed
- **Nomenklatur der Luftdruck-Sensoren**: Umbenennung der BME680-Entitäten zur fachlichen Korrektheit. `bme680_pressure` wurde zu `bme680_pressure_absolute` (statischer Druck) und `bme680_sea_level_pressure` zu `bme680_pressure_relative` (auf Meereshöhe korrigiert).
- **Härtung der Einstellungen-Synchronisation**: Optimierung der Zuweisungslogik für via ESP-NOW empfangene Parameter. Die Nutzung des ESPHome-eigenen Polling-Mechanismus garantiert nun, dass Konfigurationsänderungen (z.B. CO2-Grenzwerte) nur bei tatsächlicher Änderung in den Flash geschrieben werden, was die Systemstabilität erhöht.

### Fixed
- **BME680 NVS Wear-Out (Kritisch)**: Behebung eines kritischen Architekturfehlers, bei dem der Gas-Basiswert-Zähler (`burn_in_counter`) jede Minute einen Flash-Schreibvorgang auslöste. Die neue RAM-basierte Zählung mit 30-minütigem Sync-Intervall reduziert die Schreiblast von 1440 auf max. 48 Vorgänge pro Tag.
- **Filter-Persistenz**: Korrektur eines Fehlers, bei dem die akkumulierten Filter-Betriebsstunden nach einem Neustart des Geräts verloren gingen.
- **Dokumentations-Update**: Aktualisierung der `Entities_Documentation.md` zur Erläuterung der "Tiered Storage" Strategie und der neuen Sensor-IDs.

## [0.8.177] - 2026-04-22
### Added
- **Weitere Hardware-Variante**: `ventosync_bme680_only.yaml` für Geräte mit BME680, aber ohne SCD41/LD2450.
- **Granulare Sensor-Mocks**: Aufteilung der Mocks in `mock_scd41.yaml` und `mock_bme680.yaml` für maximale Flexibilität bei der Sensor-Kombination.
- **Automatisierte Bereitstellung**: Das `upload_all.sh` Skript wurde vollständig auf die neue modulare Struktur umgestellt und weist jedem Gerät (Büro, Flur, Wohnraum) automatisch die passende Firmware-Variante zu.

### Changed
- **Verfeinerte Build-Flags**: Umstellung auf granulare Flags (`-DVENTOSYNC_NO_SCD41`, `-DVENTOSYNC_NO_BME680`, `-DVENTOSYNC_NO_RADAR`) für präzise C++ Typ-Steuerung in `globals.h`.
- **Standardisierte Datei-Header**: Alle Haupt-Konfigurationsdateien verfügen nun über den vollständigen GPLv3-Standardheader.

## [0.8.173] - 2026-04-21
### Added
- **Hardware-Varianten (Wrapper)**: Neue Konfigurationsdateien für unterschiedliche Hardware-Ausstattungen (`ventosync_nosensor.yaml` und `ventosync_radar_only.yaml`).
- **Sensor-Mocks**: Neue Pakete `mock_climate.yaml` und `mock_radar.yaml` ermöglichen das Kompilieren der Firmware auch ohne SCD41, BME680 oder LD2450. Diese Mocks verhindern Spamming im Log und geben saubere `NAN`/`false`-Werte an die internen C++-Regler weiter.
- **C++ Preprocessor Flags**: Einführung von `-DVENTOSYNC_NO_CLIMATE` und `-DVENTOSYNC_NO_RADAR` Build-Flags für saubere, konfliktfreie C++-Deklarationen in `globals.h`.

### Changed
- **Base-Konfiguration (`packages/base/ventosync_base.yaml`)**: Die `ventosync.yaml` wurde vollständig entschlackt. Der gesamte gemeinsame Code (Hardware-Pins, UI, Netzwerk, Lüfter) liegt nun zentral in der Base-Datei. Die Hauptdateien fungieren nur noch als schlanke Wrapper.
- **Home Assistant Entitäten aufgeräumt**: Fehlende Sensoren (via Mocks) und davon abgeleitete Berechnungen (z.B. "Effektiver CO2 Wert", "WRG Effizienz") werden durch `internal: true` dynamisch vor Home Assistant versteckt. Es gibt keine "Unknown"-Sensoren mehr im Dashboard.

## [0.8.171] - 2026-04-21
### Added
- **Standardisierte Datei-Header**: Alle YAML-Konfigurationsdateien in `packages/` und die Hauptdatei `ventosync.yaml` verfügen nun über einen einheitlichen, professionellen Header inklusive GPLv3-Lizenz, Dateibeschreibung und Metadaten.
- **Neue Sub-Pakete**: Einführung von `ui_lights.yaml`, `ui_diagnostics.yaml` und `homeassistant.yaml` zur besseren Kapselung von UI-Elementen und externen Integrationen.

### Changed
- **Struktur-Refactoring (Modularisierung)**: Komplette Reorganisation des `packages/`-Verzeichnisses in eine hierarchische Unterordner-Struktur (`base/`, `actuators/`, `io/`, `ui/`, `integration/`, `sensors/`). Dies verbessert die Übersichtlichkeit und Wartbarkeit erheblich.
- **Logik-Entkopplung**:
    - Hochperformante Steuerungslogik (Lüfter-Skripte, Urlaubsmodus) wurde in `actuators/logic_automation.yaml` zentralisiert.
    - Sicherheitshritische thermische Abschaltung wurde in ein eigenes Modul `actuators/logic_safety.yaml` ausgelagert.
    - Externe Home Assistant Datenpunkte wurden konsequent in `integration/homeassistant.yaml` isoliert, um das System unabhängiger von der Zentrale betreiben zu können.
- **Dokumentations-Update**: Die `packages/Readme.md` wurde vollständig überarbeitet und spiegelt nun die neue Verzeichnisstruktur und die funktionalen Zuständigkeiten der Module wider.

### Fixed
- **ID-Konflikt gelöst**: Behebung eines Fehlers mit doppelt definierten IDs (`pid_humidity_output`), der durch die Extraktion der PID-Logik entstanden war.

## [0.8.169] - 2026-04-18
### Fixed
- **Smart-Automatik Modus springt zurück zu Wärmerückgewinnung (BUGFIX)**
  Beim Wechsel in den Modus "Smart-Automatik" wurde die HA-Select-Entity sofort
  auf "Wärmerückgewinnung" zurückgesetzt. Ursache: String-Mismatch zwischen den
  C++-Konstanten (`"Automatik"`) und den YAML-Select-Optionen (`"Smart-Automatik"`).
  ESPHome lehnt ungültige `publish_state()`-Werte still ab und behält den letzten
  gültigen Wert bei. Betraf zwei Stellen:
  - `system_lifecycle.h`: `mode_names[0]` von `"Automatik"` → `"Smart-Automatik"`
  - `network_sync.h`: `mode_str` bei `auto_mode_active` von `"Automatik"` → `"Smart-Automatik"`
  Der Bug entstand beim Umbenennen "Automatik" → "Smart-Automatik" in einer früheren
  Session: die YAML-Optionen wurden damals aktualisiert, die C++-Strings nicht.
  
- **Smart-Automatik Modus-Sync über ESP-NOW wird vom Slave ignoriert (BUGFIX)**
  Ein Wechsel auf Smart-Automatik durch den Master wurde von den Slaves nicht übernommen, 
  wenn diese bereits in "Wärmerückgewinnung" waren. Ursache: Beide Modi basieren auf 
  dem gleichen Enum (`MODE_ECO_RECOVERY`). Die alte ESP-NOW Logik verglich nur dieses
  Enum und hielt den Sync für unnötig, wodurch der Wechsel ignoriert wurde.
  Fix: Die Sync-Bedingung in `ventilation_group.h` prüft nun zusätzlich den via ESP-NOW 
  übertragenen HA-Modus-Index (`pkt->current_mode_index`). `network_sync.h` übernimmt
  diesen Index 1:1, um das UI und das globale `auto_mode_active`-Flag präzise zu steuern.

- **Lokale UI Auswahl von Smart-Automatik wird ignoriert (BUGFIX)**
  Die C++ Funktion `set_operating_mode_select` in `user_input.h` erwartete beim
  Klick im Dashboard noch immer den String `"Automatik"`. Da das ESPHome-Select 
  aber `"Smart-Automatik"` liefert, wurde die Eingabe mit einer Log-Warnung verworfen 
  und der Modus fiel zurück.
  Fix: String auf `"Smart-Automatik"` aktualisiert.

- **`esphome logs` Disconnect-Loop nach ESPHome 2026.4.0 Update (HOTFIX)**
  Der Log-Monitor crashte wiederholt mit `AttributeError: 'NumberInfo' object has
  no attribute 'accuracy_decimals'`. Ursache: `aioesphomeapi==44.15.0` übergibt bei
  bestimmten State-Updates ein `NumberInfo`-Objekt an `_format_sensor()`, das
  `accuracy_decimals` (in `NumberInfo` nicht vorhanden) erwartet. Die Firmware
  und OTA-Funktion waren nicht betroffen — nur der Python-Log-Client.
  Hotfix: `hasattr(info, "accuracy_decimals")`-Guard in
  `aioesphomeapi/state_log_formatter.py`. Muss nach einem `pip upgrade` ggf.
  erneut angewendet werden.

## [0.8.163] - 2026-04-18
### Security / Stability
- **K-2 — Stoßlüftungs-Konstanten mit Compile-Zeit-Verifikation abgesichert**
  `STOSS_ACTIVE_MS` und `STOSS_PAUSE_MS` verwendeten `UL`-Suffix statt portablem `u`. Da `unsigned long` auf 32-Bit-Plattformen (ESP32-C6 RISC-V) nur 32 Bit breit ist, wäre ein zukünftiger Wert >4,29 Milliarden still übergelaufen. Jetzt: `u`-Suffix + `static_assert` verifiziert die erwarteten Werte und den Gesamtzyklus zur Compile-Zeit.
- **H-2 — `VentilationMode` Enum um expliziten `uint8_t` Underlying Type ergänzt**
  Der Enum ohne Typ-Angabe ließ dem Compiler freie Wahl (potenziell `int` = 4 Byte). Da `VentilationPacket::current_mode` als `uint8_t` serialisiert und über ESP-NOW übertragen wird, ist eine implizite Narrowing-Conversion die Folge. Mit `enum VentilationMode : uint8_t` ist die Paket-Kompatibilität nun vom Typ-System garantiert. `static_assert(sizeof(VentilationMode) == sizeof(uint8_t))` verifiziert dies zur Compile-Zeit.

### Changed
- **H-1 — Parameternamen-Konflikt `now_ms` vs. `now` zwischen Header und Implementation behoben**
  `get_target_state()` und `get_remaining_duration()` verwendeten `now_ms` im Header aber `now` im .cpp — inkonsistent mit `get_cycle_pos(uint32_t now)`. Alle Deklarationen auf konsistentes `now` normalisiert.
- **H-3 — Totes Feld `needs_update` aus `HardwareState` entfernt**
  Das Feld wurde in `get_target_state()` stets auf `false` gesetzt und nirgends ausgewertet. Entfernt in Header und Implementation.
- **M-1 — `DEFAULT_CYCLE_DURATION_MS` als benannte Konstante mit `static_assert` eingeführt**
  Der Default-Wert 70000 war bisher ein Magic Number. Jetzt als `static constexpr` Klassenmember mit `static_assert(DEFAULT_CYCLE_DURATION_MS >= 2 * RAMP_DURATION_MS)` gegen die Rahmenbedingung des K-2-Fixes aus dem .cpp abgesichert: Wenn jemand den Default unter 10000ms senkt, schlägt die Kompilierung fehl.
- **M-2 — Serialisierungs-Dokumentation für `VentilationMode` Enum-Werte**
  Explizite Warnung im Doxygen-Kommentar, dass die Enum-Werte nicht umbenannt werden dürfen, da sie über ESP-NOW serialisiert werden. Trailing-Comma für zukünftige Erweiterbarkeit.

### Not fixed (audited, no action required)
- **K-1 — Default `cycle_duration_ms = 70000`**
  Der Default-Wert ist eine bewusste Engineering-Entscheidung: Das System startet in einem sicheren, bekannten Zustand. Das Boot-Race-Window (~100-2000ms mit dem Default statt dem konfigurierten Wert) ist folgenlos, da der Slew-Rate-Limiter den Motor ohnehin sanft hochfährt. Die K-4-Guards im .cpp (Division-by-Zero bei 0) fangen den Fall ab, falls jemand den Default auf 0 ändert.

## [0.8.162] - 2026-04-18
### Security / Stability
- **K-1 — int64→int32 Cast-Overflow in `set_cycle_duration()` behoben (KRITISCH)**
  `time_offset_ms = (int32_t)target_offset` wurde ohne Bereichsprüfung ausgeführt. Wenn `target_offset` nach der Normalisierung außerhalb des int32_t-Bereichs lag, führte dies zu falschen Richtungswechsel-Zeitpunkten. Jetzt: `std::clamp` gegen `INT32_MAX` vor dem Cast.
- **K-2 — Division-by-Zero und Ramp-Overlap in `get_target_state()` behoben (KRITISCH)**
  Bei extrem kurzen Halbzyklen (`cycle_duration_ms < 2 × RAMP_DURATION_MS`) überlappten sich Ramp-Up und Ramp-Down — der `ramp_factor` wurde erst auf Anlauf und direkt danach auf Auslauf gesetzt. Neuer Guard springt die Rampenlogik komplett, wenn der Halbzyklus zu kurz für zwei separate 5s-Rampen ist. Tote Variable `full` entfernt (S-1).
- **K-3 — Tote Variable `changed` in `set_mode()` entfernt (KRITISCH)**
  `bool changed = (current_mode != mode)` wurde nach dem Early-Return nie wieder gelesen. Durch direkte inline-Vergleichung ersetzt: `if (current_mode == mode && duration == ventilation_duration_ms) return;`. Die Timer-Reset-Logik für `ventilation_start_time` war bereits korrekt.
- **K-4 — Division-by-Zero in `get_cycle_pos()` behoben (KRITISCH)**
  `raw_pos % (int64_t)period` crashte wenn `cycle_duration_ms == 0` (period = 0 → Modulo durch Null). Guard `if (cycle_duration_ms == 0) return 0;` + Overflow-Prüfung für `period = cycle_duration_ms * 2` eingefügt.

### Changed
- **H-2 — Zero-Guard für `sync_time()` eingefügt**
  `time_offset_ms %= (int32_t)period` ohne vorherige Prüfung auf `cycle_duration_ms == 0` crashte mit Division-by-Zero. Guard hinzugefügt, C-Style-Cast durch `static_cast` ersetzt.
- **H-3 — Input-Validierung in `set_cycle_duration()` gehärtet**
  `ms == 0` Guard gegen ungültige Eingaben und Overflow-Guard für `ms * 2` eingefügt. `new_period` und `old_half` als `const` deklariert, Code-Reihenfolge optimiert.

### Not fixed (audited, no action required)
- **H-1 — Future-Start-Time Guard in `update()`**
  Der vorgeschlagene Guard `if (ventilation_start_time <= now)` würde die korrekte `millis()`-Wraparound-Behandlung nach 49,7 Tagen brechen. Beispiel: `start_time = 4.294.000.000`, `now = 500.000` (5s nach Overflow) — der Guard würde den Timer fälschlicherweise zurücksetzen, obwohl `now - start_time` korrekt ~5s ergibt. uint32_t-Subtraktion ist inhärent überlaufsicher; der Finding erübrigt sich.

## [0.8.161] - 2026-04-18
### Security / Stability
- **K-1 — Strict-Aliasing-Verletzung in `on_packet_received()` behoben (KRITISCH)**
  Der C-Style-Cast `(VentilationPacket *)data.data()` auf `uint8_t*`-Rohdaten ist nach C++17 [basic.lval]/11 Undefined Behavior. Mit `-O2` darf der Compiler annehmen, dass `uint8_t*` und `VentilationPacket*` niemals denselben Speicher adressieren und Lese-/Schreibzugriffe rund um den Cast neu ordnen — mit potenziell silenten Datenfehlern. Ersetzt durch `std::memcpy` auf eine Stack-lokale `VentilationPacket`-Instanz. Für trivially-copyable Typen ist dieser Copy bei `-O2` zero-cost (NRVO).
- **K-3 — Direktes State-Mutation-Antipattern in `update_hardware()` behoben (KRITISCH)**
  `main_fan->state = true/false` umging ESPHome's Event-Loop vollständig: Home Assistant erhielt kein Update, wenn die Fenstersperre den Lüfter abschaltete oder freigab. Ergänzt um `main_fan->publish_state()` (kein Argument, kein `perform()`) direkt nach der State-Zuweisung: HA wird informiert, ohne dass ESPHome den PWM-Ausgang berührt — die eigentliche Hardware-Steuerung obliegt unverändert `update_fan_logic()`.
- **H-5 — Peer-Liste auf maximal 10 Einträge begrenzt (KRITISCH für Langzeitbetrieb)**
  `std::vector<PeerState> peers` wuchs bei jedem unbekannten `device_id` unbegrenzt. Ohne Limit könnte ein Gerät mit wechselnden IDs (z.B. nach mehreren Firmware-Updates oder in Testszenarien) den Heap fragmentieren und einen OOM-Reset provozieren. Implementiert: LRU-Eviction — bei vollem Slot (>= 10) wird der am längsten inaktive Peer anhand von `last_seen_ms` ermittelt und entfernt, bevor der neue eingetragen wird.

### Changed
- **H-1 — `on_packet_received()` Parameter auf `const`-Referenz umgestellt**
  `bool on_packet_received(std::vector<uint8_t> data)` erzeugte bei jedem eingehenden Paket eine unnötige Heap-Allokation und -Kopie. Geändert auf `const std::vector<uint8_t> &data` — zero-copy, kombiniert mit dem K-1-Fix (memcpy auf Stack-Objekt).
- **H-2 — Doppelte Schrittnummerierung im Loop-Kommentar korrigiert**
  Im `loop()`-Body waren zwei unabhängige Codeblöcke beide als `// 5.` kommentiert. Korrigiert auf `// 5. Cleanup old peers` und `// 6. Watchdog Feed`.
- **H-3 — Redundanten `millis()` Re-Fetch in `loop()` entfernt**
  `now = millis()` wurde mitten im Loop ein zweites Mal aufgerufen, mit dem irreführenden Kommentar, es sei nötig, damit `now >= last_seen_ms`. uint32_t-Subtraktion ist von Haus aus überlaufsicher; zwei verschiedene Zeitstempel in derselben Loop-Iteration sind hingegen eine potenzielle Inkonsistenz-Quelle. `now` ist jetzt `const` und wird einmalig am Funktionsanfang gesetzt. Der stale-peer check wurde von `(now >= it->last_seen_ms && now - it->last_seen_ms > 300000)` auf die korrekte `(now - it->last_seen_ms > 300000)` vereinfacht.
- **H-4 — Timer-Konvertierung in `build_packet()` mit Clamp gesichert**
  `(uint16_t)(sync_interval_ms / 60000)` wurde ohne vorherige Bereichsprüfung gecasted. Bei einem durch einen Bug oder NVS-Fehler korrumpierten Wert > 65535 min würde der Cast still auf 0 umbrechen. Jetzt: `std::min(value, MAX_TIMER_MS)` vor dem `static_cast<uint16_t>`, plus `static_assert`, der zur Compile-Zeit sicherstellt, dass 1440 min in uint16_t passt.
- **M-1 — `esp_task_wdt_add()` mit Fehlerbehandlung versehen**
  Rückgabewert wurde bisher ignoriert. Nach einem Soft-Reset ist der Task bereits registriert (`ESP_ERR_INVALID_ARG`) — das ist kein Fehler, aber war bisher nicht unterscheidbar. Jetzt wird `ESP_ERR_INVALID_ARG` als erwartet eingestuft (Debug-Log), echte Fehler werden als Warning geloggt.
- **M-2 — `PROTOCOL_VERSION` von `static const` auf `static constexpr` umgestellt**
  `static const` für eine Compile-Time-Integer-Konstante erzeugt eine Variable mit Speicheradresse und Linker-Symbol. `static constexpr` ist die semantisch korrekte Wahl: kein Speicher, kein Linker-Symbol, kann in `static_assert` und Template-Parametern verwendet werden.

## [0.8.160] - 2026-04-18
### Fixed
- **NVS-Verlust der Fenstersperre ("Fenstersperre ignorieren") behoben**
  Der UI-Switch hat seinen Status nach System-Updates (OTA) oder Stromverlust regelmäßig vergessen, weil ESPHome Änderungen über Lambda-Zuweisungen nicht von selbst in den Flash-Speicher wegschreibt. 
### Refactored
- **Nativer NVS Restore-Mode für UI Switches (Wohnraumlüftung)**
  Die überflüssige globale Variable (`ignore_window_guard_val`) wurde zur Speichereffizienz komplett gestrichen. Der Switch nutzt jetzt seinen nativen `restore_mode` via `optimistic: true` Evaluierung und synchronisiert diesen beim Boot-Vorgang naturgemäß in den C++ Core-Controller (Priorität `-10.0`).

## [0.8.156] - 2026-04-18
### Security / Stability
- **K-1 — UI Slider Overflow Limits (KRITISCH)**
  Ein fehlerhafter Float-Rundungsguard für die manuelle Timer- bzw. Sync-Intervall Steuerung in der UI wurde korrigiert, um eine uint32_t Multiplikationskorruption zu vermeiden. Die Werte kappen jetzt verlässlich Hardware-Sicherheitsgrenzen zwischen 1m und 1440m um physikalisch kritische Auslastungen zu unterbinden.
- **K-2, H-3 — Absolute Timestamps aus Netzwerk-Sync entfernt (KRITISCH)**
  Die Race-Condition durch Überlaufe beim Sync-Timeout (49-Tage ESP32 `millis()` Overflow Limit) wurde restrukturiert und auf Delta-Berechnungen umgebaut. Dies verhindert den "Brainfreeze" des Master-Syncs, bei dem ein ESP nach rund 49 Tagen Laufzeit nie wieder die Steuerung von anderen Peers übernimmt.

### Changed
- **H-1 — Slider Stufen Rundung korrigiert**
  Vermeidung von Off-By-One Level Truncations durch direkte Float-auf-Integer Casts bei Touch-Device Slidern.
- **H-2 — Modus Array-Crash Loop korrigiert**
  Ein defekter NVS Schreibvorgang des Cycle-Modes konnte in eine negative modulo-Schleife fallen und die State-Machine zum Absturz bringen.
- **H-4 — De-Coupling für Emergency Power-Cuts**
  Der Long-Press (Langes Halten) Power-Down Befehl funktionierte u.U. nicht wenn HA-Hardware Definitionen fehlten. Die Notabschaltung erzwingt nun Hardware-PWM Stops vor Software Callbacks.
- **M-1 — Cloud UI API String Sanitizer**
  Whitespaces via Automatisierungs-Systeme und Templates konnten API-Steuerbefehle korrumpieren, dies wird nun trim-normalisiert.

## [0.8.155] - 2026-04-18
### Security / Stability
- **K-1 — Durchlüften Timer Overflow behoben (KRITISCH)**
  Ein reiner Float-Overflow-Bug in der Dauerberechnung wurde geschlossen. Der `vent_timer` (Float) wird nun vor der Konvertierung in Millisekunden (uint32_t) strikt geclampt, um Implementation-Defined Behavior und unvorhersehbare Auto-Fallback Dauern bei NVS-Fehlern zu vermeiden.
- **K-2 — Sicherung der ESPHome State-Konsistenz (KRITISCH)**
  Direktes manuelles Setzen der internen Objektstatusrepräsentation (`lueftung_fan->state = false`) im "Aus"-Modus umging die Event-Loop Architektur. Der Controller bedient die internen State-Ebenen nun sicher über generierte `make_call()` Anweisungen.
- **K-3 — Modus Operations-Safety bei NVS-Corruption (KRITISCH)**
  Ein korrupter Parameterwert für den `current_mode_index` konnte Systemzustände aus dem Tritt bringen. Harte Bounds-Checks am Eintritt der Orchestration fangen invalide Enums ab und forcieren "Wärmerückgewinnung" (Modus 1) als sicheren Standard.

### Changed
- **H-1 — Filter-Betriebsstunden: Langzeit-Rundung behoben**
  Das direkte Akkumulieren von asynchron tickenden Millisekunden in einer Float-Repräsentation führte systembedingt nach ~40 Tagen zu einem heimlichen Einfrieren des Filter-Trackers durch Limitierungen der 23-Bit Gleitkomma-Genauigkeit. Eine separierte Integer-Engine bereinigt das.
- **H-2 — NVS Cast-Safeguards**
  Das Umleiten der 8-Bit Identifier Werte werfen nun im Bootloader bei ungültigen Bounds dedizierte Logs und sichern das Discovery System vor Speicherglitches des Flash Roms.
- **H-3 — Erweiterte System-Reset Diagnostik**
  Das Reset-Logging erfasst ab sofort neben regulären Task-Watchdogs auch ESP32-generierte Brownouts (Unterspannung), Panther-Panics (Stackoverflow) oder blockierte Interrupt-Service-Routinen in der Analytik-Zähler-Engine für besseres Debugging.
- **H-4 — Ausfallsichere LED Startroutinen**
  Poka-Yoke-Überprüfungen im Startup der visuellen LEDs deaktivieren den "Knight-Rider" Bootsequence Mode nicht mehr komplett, sobald eine singuläre Indicator-LED einen defekten Bus meldet.

## [0.8.154] - 2026-04-18
### Security / Stability
- **K-1 — Sanftanlauf-Limiter Bootstrap behoben (KRITISCH)**
  Die Initialisierung des Variablen `current_smoothed_speed` beim ersten Aufruf wurde auf `0.0` korrigiert. Damit entfällt ein vorher existierender, ungebremster 100%-Anlauf des Motors bei Systemstart. Der Slew-Rate-Limiter erzwingt nun konsistent einen Material-schonenden Hochlauf aus dem Stillstand.
- **K-2 — Sicherung der internen PWM State Updates (KRITISCH)**
  Die unsaubere Kapselung der PWM-Wertverfolgung für die virtuellen RPM-Statistiken ist behoben. Ein fehlender PWM-Treiber (z.B. im Software-Testbetrieb) friert nicht mehr versehentlich die `last_fan_pwm_level` für abhängige Routinen wie `calculate_virtual_fan_rpm()` ein. 
- **K-3 — Freeze-Recovery nach Watchdog oder OTA-Updates**
  Wenn das Betriebssystem außergewöhnlich lange Pausen (>5s) einlegen musste (z.B. Firmware Update Download), versuchte der Slew-Rate Limiter anschließend den Motor sprunghaft in Position zu bringen. Ein neuer 5000ms Trigger resetet nun sanft den Puffer auf halbe Fahrtrichtung, um Mechanik-Sprünge zu verhindern.
- **H-1 — Heimliche Clamp-Verletzungen der Präsenz-Kompensation**
  Wenn das Radar (+2 Stufen) den Basiswert (Stufe 9) ins physikalisch unmögliche verschob, schlug blind der Limit-Clamp auf Stufe 10 an und der Nutzer sah keine Aktion. Das Ranging ist nun gefixt und im Log nachvollziehbar dokumentiert.

### Changed
- **H-3, H-4 — Richtungszuweisungen & History Redundanz**
  Der Target-Speed Logger erhält die Windrichtung nun synchron aus der State-Machine Arrays und nicht mehr aus der langsameren UI-Switch-Repräsentation (`fan_direction->state`). Redundante Pufferlöschoperationen entfallen, da sie von der Klimakontrolle (`filter_ntc_stable`) abgefangen werden, wie es konzeptionell gedacht war.
- **M-1, M-2, M-3 — Type Safety & Log-Polishing**
  Fehlende Bounds-Checks bei Floats auf Float-Ramping Arrays (0-99 iterations) wurden geschlossen (`std::clamp`) und ein Bug im State-Logger ("Ramping complete" wurde asynchron geworfen) bereinigt.

## [0.8.152] - 2026-04-17
### Security / Stability
- **K-1 — WRG-Effizienz Ausgabe-Validierung (KRITISCH)**
  Ein fehlender Clamp in der Effizienzberechnung (`calculate_heat_recovery_efficiency`) konnte potenziell Werte jenseits von 1.0 (z.B. >100%) oder mathematische Fehler (NaN/Inf) bei Gleichheit der Temperaturen direkt an Home Assistant melden. Eingefügte Plausibilitätskorridore ([0.0, 1.1]) und ein harter Clamp auf `1.0` garantieren sichere Anzeige-Metriken.
- **K-2 — NTC-Filter: Sliding-Window Invalidierung (KRITISCH)**
  Die Temperatur-Stabilisierungs-History der NTC Sensoren wurde bei einem Wechsel der Luftrichtung nicht invalidiert. Dadurch wurden alte thermische Werte in den Puffer verschleppt und der Filter hat fälschlicherweise "stabil" gemeldet. Das Sliding-Window unterbindet nun thermische Pollution durch ein `history.clear()` nach jedem Wechsel.
- **H-1 — NTC-Filter: Out-of-Bounds Protection**
  Der von außen übergebene `sensor_idx` für die History greift nun nicht mehr blind auf das Array zu, sondern wird vorher strikt mittels Bounds-Check auf das Sensor-Limit `(0, 1)` geprüft.

### Changed
- **H-2 — WRG-Effizienz: Boot-Safety Guard**
  Die Heuristik zum Messen der Effizienz ("erst 30s nach Flip") triggerte versehentlich sofort nach dem ESP-Kaltstart, weil `last_direction_change_time = 0` falsch evaluiert wurde. Ein dediziertes 60s Boot-Flag sorgt jetzt dafür, dass während der Anlaufphase keine instabilen Messungen durchrutschen.
- **H-3 — NTC-Filter: Safety-Clamp bei kurzen Zyklen gefixt**
  Fehlerhafte Mindestwartezeiten, die den Stabilisierungsfilter im Worst-Case komplett deaktivierten, wenn der WRG-Zyklus extrem kurz eingestellt war, wurden überarbeitet. Kurze Zyklen profitieren nun von einem offiziellen Bypass inkl. Warning Log.
- **M-1 — CO2-Klassifizierung validiert Daten**
  Der ungenutzte Wrapper `get_co2_classification()` validiert Werte vor der Weitergabe nun gegen `NaN`, tieferliegendes Negative und astronomische Werte (über `10000 ppm`), um komische Strings in Home Assistant abzufangen.

## [0.8.151] - 2026-04-17
### Security / Stability
- **K-1 — Division-by-Zero in Hysterese-Berechnung behoben (KRITISCH)**
  Ein Fehler in der Hysterese-Kalkulation des Automatikmodus konnte potenziell Inf/NaN Werte provozieren, wenn `current_level` außerhalb der Bandbreite zwischen `min_l` und `max_l` lag. Es wurde ein defensiver `std::clamp` implementiert, um den Wert vorher zu limitieren und Endlos-Ramping zu unterbinden.
- **K-2 — Numerische Instabilität bei absoluter Feuchtekalkulation behoben (KRITISCH)**
  Die fehlenden Bounds-Checks auf absurde Sensor-Temperaturen produzierten in der `calculate_absolute_humidity()` Funktion potenziell negative absolute Feuchtigkeitswerte durch Division durch negative Nenner oder astronomische Exponenten (z.B. via NaN Propagation). Temperaturgrenzen und Ergebnisvalidierung verhindern nun ein Fehlverhalten des PID-Managers.
- **H-2 — Peer-Demand Plausibilitätsprüfungen eingefügt**
  Fehlerhafte PID-Sensordaten eines Peers werden nicht mehr blind übernommen, da sie den lokalen Lüfter unnötigerweise auf Maximum rampten konnten. Peer-Werte werden nun sauber auf die Grenzen `0.0` bis `1.0` gekappt (`std::clamp`) und asynchrone Sprünge werfen nun klare Warnungen (Log).
- **H-3 — Asymmetrische NTC-Sensorik-Fallbacks gefixt**
  Beim Kreuzabgleichen der Injektions- und Extraktionskanäle kam es im `Ventilation`-Modus zu falsch zugeordneten Sensoren (`local_in`/`local_out`), wodurch das System temporär blind für Innenwerte werden konnte. Der Sensor-Mapping-Prozess nutzt nun stabile Fallbacks und logische Symmetrie durch das `read_sensor()` Lambda.

### Changed
- **H-1, K-3 — Klarstellung Master Peer Authority Logiken**
  Dokumentations-Refactoring zur Klärung, weshalb der Master im Auto Mode über die lokale `v->peers` Variable iteriert (Staleness <= PEER_TIMEOUT_MS) und Begradigung der fehlerhaften Schrittnummerierungen in der Automode-Evaluation.

## [0.8.149] - 2026-04-17
### Security / Stability
- **K-1 — Null-Pointer-Dereferenzierungen in LED-Steuerung (KRITISCH)**
  Die ungeschützten Pointer-Aufrufe in `led_feedback.h` (`status_led_...->turn_off()`) wurden durch die neuen `led_guard`-Helferfunktionen abgelöst. Ein fehlender Eintrag im YAML crasht nun nicht mehr den ESP32, da jeder Zugriff sicher via `if (led != nullptr)` geschützt ist. Fehlen essenzielle Zeiger wie der der Power-LED, führt dies nun zu einem ausfallsicheren Early-Exit (M-3).
- **K-2 — Variables Shadowing in Master-LED behoben**
  Eine gefährliche Re-Deklaration von `max_b` im inneren Scope der Master-Funktion logisch korrigiert, um asynchrones Helligkeits-Clipping künftig auszuschließen.

### Changed
- **H-1, H-2 — Stabileres State-Tracking & Sentinel Boot-Flags**
  Static Initializer wurden durch dedizierte Bool-Flags (`first_call`, `initialized`) ersetzt. Dies stoppt False-Positive WLAN-Fehler während der Boot-Phase auf dem Master und erzwingt garantiert ein sicheres erstes Initial-Rendering aller Status-LEDs aus jedem beliebigen Boot-Zustand.
- **H-3, M-2 — Refactoring zu Lookup-Table ("Fill Bar") + I2C-Overhead halbiert**
  Ein überlanges, fehleranfälliges 60-Zeilen `switch`-Statement wurde ins effiziente `LedLevelConfig` Array ausgelagert. Unnötige "immer alle aus, dann 3 wieder an" Befehle entfallen — dies entlastet den I2C-Bus drastisch (<5 Transaktionen statt z.T. >10).
  - **Neues "Fill Bar"-Design**: Entgegen der vorherigen, teils asymmetrischen Ansteuerung agiert die 5-Stufige Level-Matrix (für 10-Intensitäten) nun als gewohnter Füllbalken (jede LED macht exakt zwei Schritte: 20% → 100%, bevor die nächste greift).
- **M-1 — Deterministischere "Sync-Loss" Erkennung**
  Die Heuristik zum Triggern des Master-Peer-Fehlers (2x Blinken) wurde geschärft. Es wird nicht mehr ein abgeleiteter Demand geprüft, sondern direkt die Timestamps der einzelnen verbuchten Peers auslesbar gemacht.

## [0.8.147] - 2026-04-17
### Security / Stability
- **K-1 — Thread-Safety: ACK-Callback greift nicht mehr direkt auf `peer_cache` zu (KRITISCH)**
  Die WiFi-Task-Send-Callbacks von `send_sync_to_all_peers()` mutierten `peer_cache` direkt aus dem WiFi-Task-Kontext — ein Data Race, der auf dem Single-Core ESP32-C6 via Task-Preemption zu Heap-Korruption und Watchdog-Resets führen kann. Die Callbacks schreiben nun ausschließlich ein `PeerEvent` (MAC + Ergebnis) in eine neue, Mutex-geschützte `peer_event_queue`. Die Mutation von `peer_cache` (`fail_count++`, `remove_stale_peer()`, usw.) erfolgt sicher im Main-Loop-Kontext durch die neue Funktion `process_peer_events()`.
  - Neue Typen in `globals.h`: `PeerEvent` (Struct mit MAC-Array + `Type::SEND_OK`/`SEND_FAIL`), `peer_event_queue` (`std::queue<PeerEvent>`), `peer_event_mutex` (`std::mutex`).
  - Neue Funktion `process_peer_events()` in `network_sync.h`, aufgerufen aus `process_queued_packets()`.

- **K-2 — Thread-Safety: `is_local_mac()` MAC-Initialisierung via `std::call_once` gesichert**
  Das nicht-atomare `static bool cached`-Flag-Muster hatte ein theoretisches Race Window zwischen zwei Tasks. Ersetzt durch `std::once_flag` + `std::call_once()` — nach C++11-Standard garantiert-einmalig und thread-sicher, ohne YAML-Änderung.

- **N-2 — Design: Doppelter `reinterpret_cast` auf eine einzige sichere Parse-Funktion konsolidiert**
  `validate_packet()` castete via `reinterpret_cast` fuer Feld-Checks; `process_espnow_packet_local()` machte einen zweiten, unabhaengigen Cast. Zwei lose gekoppelte Cast-Stellen bedeuten: ein Struct-Refactor kann eine Stelle vergessen.
  Ersetzt durch `validate_and_parse_packet()` -> gibt `std::optional<VentilationPacket>` zurueck.
  Einziger Cast-Punkt: `std::memcpy(&pkt, data.data(), sizeof(pkt))` — formal Strict-Aliasing-sicher (im Gegensatz zu `reinterpret_cast`). Alle Checks (Magic, Version, Groesse, Wertebereich) haben nun eigene `ESP_LOGW`-Ausgaben fuer bessere Diagnosierbarkeit.

### Changed
- **H-1 — `register_peer_dynamic()`: `peer_cache` als einzige Source of Truth**
  Direkter Append auf `espnow_peers->value()` entfernt. Die NVS-Repraesentation wird nun ausschliesslich ueber `rebuild_peers_string()` abgeleitet — Divergenz zwischen binaeren Cache und persistiertem String strukturell unmoglich.

- **H-2 — `rebuild_peers_string()`: Lokale Buffer-Strategie**
  Baut den String in einem lokalen `std::string` auf und weist ihn via `std::move()` zu. Erlaubt `reserve()` fuer eine einzige Allokation und eliminiert Unklarheit ueber Mutation waehrend Iteration.

- **H-3 — `handle_discovery_payload()`: Hardcoded Offset `10` durch Prefix-Konstanten ersetzt**
  `sscanf` verwendete den Magic-Offset `10` (Laenge von `"ROOM_DISC:"`/`"ROOM_CONF:"`). Ersetzt durch `constexpr std::string_view DISC_PREFIX_DISC` / `DISC_PREFIX_CONF` mit dynamischem Offset aus `.size()`.

- **H-4 — `process_queued_packets()`: Kommentar zu Lock-freiem Fast-Path praezisiert**
  Frueherer Kommentar "benign race" ohne Begruendung. Ersetzt durch akkurate Erklaerung: 32-bit-aligned read ist natuerlich atomar auf RISC-V; der echte Schutz gegen Korruption liegt im `lock_guard` darunter.

### Not fixed (audited, no action required)
- **K-3**: `handle_espnow_receive()` Deduplication-Statics — nur aus einem Task aufgerufen, kein echter Race.
- **K-4**: Buffer-Overread in `validate_packet()` — Cast erfolgte erst nach dem strikten Size-Check. Unabhaengig durch N-2 aufgeraeumt.
- **H-5**: Null-Checks in `handle_state_sync()` — korrekt durch implizite `if (ptr)` gesichert.
- **N-1**: Integer Overflow in Timer-Berechnung — `uint16_t` wird durch C++ Integral Promotion zu `int` (32-bit) hochgestuft; `1440 x 60 x 1000 = 86.400.000 << INT32_MAX`.

## [0.8.135] - 2026-04-15
### Added
- **Globaler Urlaubsmodus (Vacation Mode)**: Einführung eines hausweiten Energiesparmodus, der über einen Home Assistant Helfer (`input_boolean.ventosync_vacation_mode`) gesteuert wird.
  - **Automatisches Save/Restore**: Das System speichert beim Aktivieren den aktuellen Modus und die Intensität und stellt diese beim Deaktivieren nahtlos wieder her.
  - **Energiespar-Logik**: Automatische Umstellung aller Geräte auf „Stoßlüftung“ (Stufe 1) während der Abwesenheit.
  - **Detaillierte Dokumentation**: Neue Schritt-für-Schritt-Anleitungen (`documentation/Vacation-Mode-HA-Setup-DE.md`) für die Einrichtung des Helfers in Home Assistant.

## [0.8.134] - 2026-04-14
### Changed
- **Smart-Automatik Rebranding**: Projektweite Umstellung des primären Automatik-Modus von „Standard Automatic“ auf **„Smart automatic“** (Englisch) bzw. „Smart-Automatik“ (Deutsch) in Dokumentation, UI und Home Assistant Entitäten.
- **UI & Entitäten-Update**: Umbenennung der Steuerungs-Entitäten in Home Assistant für ein einheitliches Markenbild und bessere Benutzerführung.
- **Dokumentations-Refining**: Aktualisierung der README mit korrekter Hersteller-Nennung (**Bosch**) für den BME680-Sensor und Präzisierung der Automatik-Beschreibungen.

## [0.8.133] - 2026-04-12
### Added
- **"Fenstersperre ignorieren" Bypass**: Einführung eines gerätespezifischen Overrides, um die raumweite Fenstersperre für einzelne Einheiten zu deaktivieren. Inklusive NVS-Persistenz.
- **Detaillierte Logik-Dokumentation**: Neues Dokument `documentation/Automatic-Mode-Logic.md` zur präzisen Beschreibung der Entscheidungsmatrix und PID-Steuerung des Automatik-Modus.
- **Erweiterte Roadmap**: Detaillierung fortgeschrittener mmwave-Radar-Strategien (Silent Sleep, Personenzählung, Abwesenheits-Absenkung).

### Changed
- **Globales Rebranding**: Projektweite Umstellung des Namens von „WRG Wohnraumlüftung“ auf **„VentoSync HRV“** in allen Headers, Log-Ausgaben und Beschreibungen.
- **Standardisierte Datei-Header**: Alle YAML- und C++-Dateien erhielten einen einheitlichen Header mit Lizenz- (GPL v3), Autor- und Versionsinformationen.
- **LED-Feedback Optimierung**: Das "Window Guard" Blinkmuster wird nun unterdrückt, wenn der lokale Bypass aktiv ist. Die Grundhelligkeit der Master-LED bei aktiven Effekten wurde auf 30% reduziert, um Blendung bei Nacht zu vermeiden.

## [0.8.131] - 2026-04-11
### Added
- **Status-Entität für Fenstersperre**: Neuer Binär-Sensor in Home Assistant (`binary_sensor.fenstersperre_aktiv`), der den gefilterten Zustand der Sperre anzeigt.

### Changed
- **Fenstersperre Aktivierungs-Verzögerung**: Die Sperre greift nun erst nach **10 Sekunden** durchgehender Fenster-Öffnung, um kurzes Lüften/Nachschauen abzufedern.
- **LED-Feedback Timeout**: Das Pulsieren der Master-LED ist nun auf **5 Minuten** begrenzt. Bei dauerhaft offenem Fenster kehrt die LED in den Normalzustand zurück, während der Lüfter zum Schutz vor Wärmeverlust weiterhin gestoppt bleibt.
- **Dokumentations-Refactoring**: Die detaillierte Anleitung zur Home Assistant Konfiguration wurde in separate Dokumentationsdateien ausgelagert, um die Übersichtlichkeit der READMEs zu erhöhen.

### Fixed
- **Fenstersperre (Window Guard) Motor-Stopp**: Korrektur der Motorsteuerung, die den "AUS"-Befehl der Fenstersperre im Hardware-Loop ignorierte. Der Lüfter bremst nun bei geöffnetem Fenster wie vorgesehen sanft bis zum Stillstand ab.

## [0.8.128] - 2026-04-11
### Added
- **Fenstersperre (Window Guard)**: Implementierung einer raumübergreifenden Schutzfunktion. Wenn ein Fenster im Raum geöffnet wird (erkannt über einen Home Assistant Gruppen-Sensor), pausieren alle VentoSync-Geräte im Raum sofort ihre Lüfter.
- **Smart-Resume Logik**: Das System behält seinen aktuellen Modus (z. B. Automatik) bei und nimmt den Betrieb nahtlos wieder auf, sobald alle Fenster geschlossen sind.
- **Visuelles Feedback für Fenstersperre**: Die Master-LED pulsiert in einem 1s/2s Rhythmus, um anzuzeigen, dass das System aufgrund eines offenen Fensters pausiert.
- **Konfigurierbare Sensor-ID**: Über die YAML-Substitution `window_sensor_id` lässt sich die HA-Entity pro Raum flexibel zuweisen (Standard: `binary_sensor.ventosync_window_lock_room_${room_id}`).

### Changed
- **Architektur-Härtung (Dependency Injection)**: Umstellung der Sensor-Integration in der `VentilationController`-Komponente auf Dependency Injection. Dies löst Linker-Konflikte und verbessert die Stabilität bei der Kompilierung komplexer Raum-Konfigurationen.

## [0.8.121] - 2026-04-11
### Fixed
- **Mesh-Stabilität (Stale Peer Fix)**: Korrektur eines Race-Conditions in der Peer-Verwaltung. Durch das erneute Abrufen von `millis()` vor dem Bereinigungs-Loop und zusätzliche Plausibilitätsprüfungen (`now >= last_seen_ms`) wird verhindert, dass Peers fälschlicherweise unmittelbar nach Empfang aufgrund eines `uint32_t` Underflows gelöscht werden.
- **Netzwerk-Synchronisierung der PID-Sollwerte**: Änderungen an den CO2- und Feuchte-Grenzwerten werden nun sofort an die lokalen PID-Controller auf allen Zielgeräten im Raum übertragen. Zuvor wurden nur die UI-Slider aktualisiert, während die Regellogik auf alten Werten verharrte.
- **Boot-Initialisierung der PID-Regler**: Die beim Systemstart aus dem NVS wiederhergestellten Grenzwerte werden nun explizit als Sollwerte (Setpoint) an die PID-Controller übergeben.

## [0.8.120] - 2026-04-10
### Added
- **Saisonale Sperre für Sommerkühlung**: Integration der Home Assistant Entität `binary_sensor.sommerbetrieb`. Die Sommerkühlung (Bypass) aktiviert sich nun nur noch, wenn HA die warme Jahreszeit bestätigt UND die Außentemperatur über 18°C liegt. Dies verhindert zuverlässig das Einblasen von Kaltluft im Winter.
- **Wissenschaftlich korrekte Entfeuchtung (Absolute Feuchte)**: Implementierung der Magnus-Formel zur Berechnung der absoluten Luftfeuchtigkeit (g/m³). Der Feuchte-PID berücksichtigt nun, ob die Außenluft absolut trockener ist als die Innenluft, anstatt nur die relativen Werte zu vergleichen.
- **Konfigurierbare Kühlschwelle**: Die Innentemperatur-Schwelle für die Sommerkühlung (zuvor hartkodiert 22°C) ist nun über die neue HA-Entität "Automatik: Sommerkühlung Schwelle" (18-30°C) einstellbar.
- **Sensorbewertung-Timeout (Staleness)**: Der `effective_co2` Sensor veröffentlicht nun nach 5 Minuten ohne gültige Rohdaten (SCD41/BME680 beide NaN) einen `NaN` Wert, um den PID-Regler sicher zu deaktivieren und den letzten Zustand zu halten.

### Changed
- **PID-Regler Wartung**: Automatischer Reset der Integral-Speicher (I-Anteil) beim Umschalten zwischen CO2- und Feuchte-Priorität sowie beim Wechsel von Manuell zu Automatik. Dies verhindert "Überschwinger" (Integral Windup) nach langen Standzeiten.
- **Boot-Modus Glitch-Fix**: Korrektur der Rampen-Initialisierung beim Systemstart. `last_committed_mode` startet nun im Smart-Automatik-Zustand (`MODE_ECO_RECOVERY`), was den ersten Rampen-Schritt sauber auf 1 begrenzt.

### Removed
- **Legacy Dead-Code**: Vollständige Entfernung der veralteten `get_co2_fan_level()` Funktion (Berechnung der diskreten Stufen) zugunsten der stufenlosen PID-Regelung. Alle Unit-Tests wurden entsprechend migriert.

## [0.8.115] - 2026-04-09
### Added
- **Detailliertes Debug-Logging für WRG-Effizienz**: Implementierung von `ESP_LOGD` Statements in `ventilation_logic.cpp` und `climate.h`, um die Berechnung der Wärmerückgewinnung (NaN-Handling, ΔT Schwellenwerte, Stabilisierungsphasen) im Betrieb präzise zu überwachen.
- **Unit-Test Kompatibilität**: Logging-Erweiterungen in der C++ Logik wurden mit `#ifdef ESPHOME` Makros abgesichert, um die Testbarkeit in Nicht-ESPHome Umgebungen (Unit-Tests) beizubehalten.

### Changed
- **Dashboard-Abstraktion (Sensor-Fallback)**: Umstellung der internen API und des Web-Dashboards auf generische Identifikatoren (z.B. `room_temperature` statt `scd41_temperature`). Dies sorgt für eine transparente Darstellung, egal ob die Daten vom SCD41 oder dem BME680-Fallback kommen.
- **Modulare Optimierung der Custom Components**: Umfassendes Refactoring der `__init__.py` Generatoren für `ventilation_group`, `ventilation_logic` und `wrg_dashboard`.
    - Bereinigung ungenutzter Imports.
    - Verwendung spezifischer Datentypen (`uint8_t`) für IDs zur Platzeinsparung und Validierung.
    - Automatisierung der Header-Includes zur Reduzierung von manuellem YAML-Aufwand.
- **Stabilitäts-Fixes**: Korrektur der `http_request` Konfiguration in der Basis-Firmware zur Sicherstellung der Firmware-Update-Funktionalität.
- **Dokumentations-Update (README)**: Umfassende Korrektur und Synchronisierung der Projektdokumentation.
    - Korrektur der Lizenzangaben (GPL v3), Protokollversion (v7) und Entity-Namen.
    - Präzisierung der Fehler-Blinkcodes (Hysterese-Zeiten und Abhängigkeiten).
    - Vervollständigung der Projektstruktur und Behebung von Darstellungsfehlern (Emojis/Bilder).

### Fixed
- **ESPHome Version-Kompatibilität**: Behebung von Kompilierfehlern durch Rückfall auf die bewährte `cg.add_global(cg.RawStatement(...))` Methode für Header-Includes, da `cg.add_include` in der verwendeten ESPHome-Version nicht zur Verfügung steht.


## [0.8.101] - 2026-04-09
### Fixed
- **Dashboard Sensor-Fallback**: Implementierung eines transparenten Fallbacks für Raumtemperatur und Luftfeuchtigkeit. Wenn der SCD41 nicht angeschlossen ist, nutzt das Dashboard nun automatisch die Werte des BME680.
- **UI-Verbesserung**: Umbenennung der Kachel "Luftqualität (SCD41)" in "Luftqualität", da die Daten nun dynamisch von beiden Sensortypen bezogen werden können.

## [0.8.100] - 2026-04-08
### Added
- **Synchronisierte Entitäten-Dokumentation**: Vollständiger Audit und Abgleich von `Entities_Documentation.md` mit dem aktuellen YAML-Code (v0.8.x).
  - Hinzufügen von BME680 Umweltsensoren (IAQ, Taupunkt, Luftdruck absolut, Trend, Status).
  - Hinzufügen von Diagnose-Entitäten (ESP-NOW Peers, MAC-Adresse, interne IDs).
  - Hinzufügen von Hardware-Entitäten (Lüfter PWM %, LED-Helligkeit).
  - Korrektur der Entitätstypen (z.B. Phasen-Konfiguration als Select statt Switch).

### Changed
- **ESP-NOW Architektur-Dokumentation**: Überarbeitung der `Dynamic-Configuration.md` zur präzisen Beschreibung des Hybrid-Kommunikationsmodells (Broadcast für Discovery, Unicast für Betrieb) und der Master-Rolle von ID 1.
- **Konfigurations-Parameter**: Aktualisierung der dokumentierten Wertebereiche für Floor/Room/Device IDs passend zu den Firmware-Limits in `packages/device_config.yaml`.

### Fixed
- **Dokumentations-Konsistenz**: Entfernung redundanter Einträge (z.B. doppelte Lüfterintensität) und Bereinigung veralteter IDs in der technischen Dokumentation.

## [0.8.95] - 2026-04-08
### Added
- **Umfassende Doxygen-Dokumentation**: Der gesamte C++ Helper-Code und die kritischen YAML-Lambdas wurden vollständig dokumentiert. Fokus auf die Erklärung der architektonischen Hintergründe ("WARUM") statt nur der Funktion.
- **Modulare Dokumentations-Architektur**: Erstellung dedizierter `Readme.md` Dateien für alle Kernbereiche (`packages/`, `components/helpers/`, `components/ventilation_group/`, `components/ventilation_logic/`, `components/wrg_dashboard/`) zur Verbesserung der Onboarding-Experience und Wartbarkeit.
- **Troubleshooting & Guide**: Erweiterung der Haupt-Dokumentation um einen Troubleshooting-Guide für häufige Hardware- und Synchronisationsprobleme.
- **NAN-Prüfung für Benutzereingaben**: Implementierung von `std::isnan()` Prüfungen in `user_input.h` für alle float-basierten Handler (Timer, Sync-Intervall, Intensität), um undefiniertes Verhalten beim Integer-Casting zu verhindern.

### Changed
- **Refactoring der Boot-Logik (Phase 1)**: Auslagerung der System-Initialisierung (Watchdog-Erkennung, Komponenten-Sync, PID-Start) von `ventosync.yaml` in eine dedizierte C++ Funktion `run_system_boot_initialization()` in `system_lifecycle.h`. Dies vereinfacht die YAML-Konfiguration erheblich.
- **Inhaltsverzeichnis & Struktur**: Überarbeitung der Lesbarkeit und Navigation in der Haupt-Dokumentation (`Readme.md` / `Readme_de.md`).

### Fixed
- **Code-Stabilisierung**: Behebung von Syntaxfehlern und Fragmenten in `network_sync.h`, die durch Refactoring-Versuche entstanden waren.

## [0.8.91] - 2026-04-08
### Added
- **Externe Antennen-Unterstützung (Seed Studio XIAO ESP32C6)**: Implementierung einer High-Priority Boot-Frequenz (900), die den hardwareseitigen RF-Switch auf den U.FL-Anschluss umschaltet. Dies löst Empfangsprobleme bei Verbau in tiefen Wänden/Lüftungsrohren.
- **WiFi-Sendeleistung**: Erhöhung der WiFi-Ausgangsleistung auf **20dB** (physikalisches Limit des C6) für verbesserte Durchdringung von Mauerwerk.

### Changed
- **Optimierte Button-Logik (Click/Hold Separation)**: Vollständige Trennung von kurzem Klick und langem Halten für den Intensitäts-Button.
  - Implementierung eines **750ms Initial-Delays** im Hold-Handler, um "Geister-Änderungen" bei kurzen Klicks zu vermeiden.
  - Erhöhung der Scroll-Geschwindigkeit beim Halten auf **200ms** pro Stufe für ein flüssigeres Bedienerlebnis.

## [0.8.88] - 2026-04-07
### Fixed
- **Master-LED Bug auf Slave-Geräten**: Behebung des Fehlers, bei dem die Master-LED (ID 3) nach dem Booten dauerhaft leuchtete.
  - Initialisierung der `device_id` auf `0` statt `1`, um eine voreilige Master-Erkennung vor dem Laden der NvS-Daten zu verhindern.
  - Einführung eines globalen `led_state` Namespace zur konsistenten Zustandsverfolgung.
  - Synchronisation des LED-Selbsttests mit der Steuerlogik, damit die LED nach Ablauf des Tests korrekt abgeschaltet wird.
- **Debug-Logging**: Hinzufügen von `[led]` Diagnose-Ausgaben zur besseren Nachverfolgbarkeit von Zustandsänderungen.

## [0.8.83] - 2026-04-07
### Fixed
- **ESP-NOW Kommunikations-Stabilisierung (Framework Integration)**: 
  - Umstellung von manuellen ESP-IDF API-Aufrufen (`esp_now_add_peer`, `esp_now_del_peer`, `esp_wifi_get_channel`) auf das native ESPHome-Framework (`global_esp_now`). 
  - Dies stellt sicher, dass alle Radio-Operationen (Peer-Registration, Löschen und Kanalabfragen) synchron vom ESPHome-Treiber koordiniert werden – ein entscheidender Faktor für die Funk-Stabilität des ESP32-C6 (Single-Radio STA/ESP-NOW Time-Slicing).
- **Netzwerk-Resilienz (Timeout-Handling)**: Erhöhung der tolerierten Sende-Fehlversuche (`MAX_PEER_SEND_FAILURES`) von **3 auf 10**. Dies verhindert den voreiligen Verlust von Peers bei kurzzeitigen Funkstörungen oder WiFi-Powersave-Intervallen.
- **Build-Fix (Variable Scope)**: Behebung eines Kompilierungsfehlers in `network_sync.h`, bei dem eine gelöschte Variable in einem Log-Statement referenziert wurde.
- **Web-Server Sicherheit**: Vorübergehende Deaktivierung der HTTP-Authentifizierung (Port 80) zur Vereinfachung des Debugging-Zugriffs während der Instabilitäts-Phase.

## [0.8.81] - 2026-04-06
### Added
- **Sanftanlauf (Slew-Rate-Limiter)**: Implementierung einer sanften Geschwindigkeitsanpassung (verdoppelt auf ca. **10% pro Sekunde**) für den Lüfter bei Intensitätsänderungen. Dies verhindert abrupte elektrische Lastsprünge und sorgt für eine angenehmere Akustik.
- **Kontinuierliche Hardware-Synchronisation**: Hardware-Updates werden nun während einer laufenden Geschwindigkeitsanpassung (Slew) forciert, anstatt auf den nächsten Richtungswechsel zu warten.

### Fixed
- **Nahtlose Intensitätsanpassung (Cycle Phase Continuity)**: Behebung des Fehlers, bei dem der Lüfter bei Änderung der Intensität abrupt stoppte oder den Zyklus zurücksetzte. Die relative Position im Lüftungszyklus wird nun bei Änderung der Zyklusdauer proportional skaliert.
- **Build-Fix (Forward Declarations)**: Behebung von Kompilierungsfehlern in `ventilation_group.h` durch Hinzufügen notwendiger `extern` Deklarationen für `current_smoothed_speed` und `get_current_target_speed()`.

## [0.8.77] - 2026-04-06
### Fixed
- **Build-Fix (Scope Error)**: Behebung eines Kompilierungsfehlers in `ventilation_group.h`, bei dem versucht wurde, auf die globale Variable `fan_pwm_primary` zuzugreifen, bevor diese im Scope bekannt war.
- **Code-Bereinigung**: Redundante Logik in `update_hardware()` entfernt, da der Motor-Stopp (50% PWM) bereits implizit durch die nachfolgende `update_fan_logic()` korrekt berechnet wird.

## [0.8.74] - 2026-04-06
### Fixed
- **Motor-Stopp Logik**: Konsistente Implementierung des Hardware-Stopps bei 50% PWM für bidirektionale VarioPro-Lüfter.
  - Entfernung von `zero_means_zero`, um zu verhindern, dass ESPHome beim Ausschalten fälschlicherweise 0% PWM (Vollgas Rückwärts) ansteuert.
  - Direkte PWM-Ansteuerung auf 0.5f in allen "Aus"-Pfaden unter Umgehung der fehleranfälligen `turn_off()` Vererbung.
- **UI & System-Crash Protektion**: 
  - Lückenlose Implementierung von Null-Pointer-Checks für `ventilation_ctrl` in allen Button-Handlern und HA-Select-Callbacks.
  - Explizites Error-Handling für unbekannte Modus-Strings in `set_operating_mode_select` zur Vermeidung von Fehlsteuerungen.
- **Synchronisations-Resilienz**: Neuer **Sync-Watchdog** (30s), der ein Gerät nach dem Aufwachen zur aktiven Meldung zwingt, falls kein Master-Status empfangen wurde (verhindert "Pantomimen-Zustand" im Netzwerk).

## [0.8.73] - 2026-04-06
### Fixed
- **Boot-Up LED Flash**: Behebung eines Fehlers, bei dem alle LEDs beim Start kurzzeitig mit voller Helligkeit aufblitzten. Die PCA9685 Output-Enable (OE) Leitung wird nun erst nach der vollständigen Initialisierung der PWM-Register im C++ Boot-Prozess freigeschaltet.
- **I2C-Bus Stabilität**: 
  - Erhöhung der Frequenz auf **400kHz** für schnellere Transaktionen.
  - Reduzierung des Timeouts auf **10ms**, um Blockaden des Haupt-Loops bei Sensorstörungen zu minimieren.
  - Deaktivierung des I2C-Scans im Normalbetrieb zur Reduzierung von Boot-Latenzen.
- **Sensor-Diagnose**: Der WLAN-Kanal-Sensor zeigt nun korrekt `NaN` (Nicht verfügbar) an, wenn keine aktive Verbindung besteht, anstatt den letzten (potenziell falschen) Wert anzuzeigen.

### Added
- **Hardware-Dokumentation**: Zusätzliche Kommentare für GPIO-Zuweisungen (D2/Reset, D3/OE) und deren elektrisches Verhalten (Pull-Ups/Bootregeln) direkt in der YAML zur besseren Wartbarkeit.

## [0.8.72] - 2026-04-05
### Fixed
- **ESP-NOW Funkkollisionen (Simultaneous Transmit Clash)**: Behebung drastischer Sendeaussetzer (`err=1 / ESP_NOW_SEND_FAIL`), die durch exakt gleichzeitige Broadcasts von Geräten im selben Raum entstanden sind.
  - Bei deterministischen Richtungsänderungen (Timer-Flip) sendet nun **ausschließlich der Master (Device ID 1)** ein Status-Paket, um ungleiche Latenzen und RF-Kollisionen der Slaves zu verhindern.
  - Der 60-Sekunden Dashboard-Heartbeat erhielt einen Geräte-ID basierten Zeitversatz (Jitter: `device_id * 1500 ms`), wodurch synchrone Dauer-Kollisionen nach einem gleichzeitigen Boot der Module ausgemerzt wurden.
- **Hardware-Watchdog Konfiguration**: Entfernung des ungültigen `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1` Flags, da der ESP32-C6 ein reiner Single-Core Prozessor (RISC-V) ist, was zu internen Fehlkonfigurationen im ESP-IDF führen konnte.
- **Variablen-Referenzierung**: Tippfehler im Diagnose-Sensor "Watchdog Restarts" behoben (`watchdog_restart_count` -> `watchdog_restarts_count`).

### Added
- **Unabhängige Zeitsynchronisation**: Integration nativer SNTP-Zeitserver (`pool.ntp.org`) als ausfallsicherer Fallback, falls die Home Assistant API beim Start vorübergehend nicht als Zeitquelle zur Verfügung steht. Dies ist kritisch für die Filterwechsel-Logik.
- **Netzwerk-Resilienz**: Erhöhung des `reboot_timeout` bei WLAN-Verlust auf 5 bis 15 Minuten, um das System bei kurzen Router-Wartungen vom ungewollten Reboot abzuhalten.

## [0.8.37] - 2026-04-04
### Fixed
- **ESP-NOW Discovery Handshake**: Behebung eines Fehlers, bei dem Geräte nach dem Booten keine Peers registrierten. Durch die Implementierung einer bilateralen Registrierung in den `MSG_STATUS_REQUEST` und `MSG_STATUS_RESPONSE` Handlern finden sich Master und Slaves nun zuverlässig und dauerhaft.
- **Wi-Fi Stabilität (Channel 9)**: Fixierung des WLAN-Kanals auf **Kanal 9** zur Vermeidung von "Deafness"-Effekten durch Hintergrund-Scans auf dem ESP32-C6. Korrektur der YAML-Struktur (Umstellung auf `networks`-Liste) für eine valide ESPHome-Konfiguration.
- **Log-Remediation (HTTP Error)**: Deaktivierung des periodischen Firmware-Update-Checks (`update_interval: never`), um die Fehlermeldung "Failed to fetch manifest" bei Nichterreichbarkeit des Home Assistant Servers zu eliminieren. Manuelle Updates bleiben weiterhin möglich.
- **I2C-Bus Optimierung**: Erhöhung der I2C-Frequenz auf **400kHz** und des Timeouts auf **50ms**. Dies behebt die `pca9685 set Warning flag` Meldungen, die durch Funkinterferenzen (Wi-Fi/ESP-NOW) auf dem ESP32-C6 verursacht wurden.
- **Code-Qualität**: Korrektur eines Syntaxfehlers (fehlendes `ESP_LOGI`) in `network_sync.h`, der durch einen Refactoring-Fehler entstanden war.
- **Noise Reduction**: Entfernung redundanter "Registered via ESPHome API" Log-Einträge zur besseren Übersichtlichkeit der Konsole.


## [0.8.32] - 2026-04-04
### Fixed
- **Sync-Echo-Unterdrückung**: Einführung von `notify`-Flags in `VentilationController`, um zirkuläre ESP-NOW-Sync-Schleifen zu verhindern. Statusänderungen, die von Peers empfangen werden, lösen nun keine automatische Gegen-Synchronisation mehr aus.
- **Auto-Modus-Stabilität**: Im "Smart-Automatik"-Modus übernimmt der Master (Device ID 1) nun die finale Entscheidungsgewalt über die diskrete Lüfterstufe (1-10) basierend auf dem aggregierten CO2/Feuchte-Bedarf aller Peers. Dies verhindert "Jumping"-Effekte bei Grenzwertüberschreitungen auf einzelnen Geräten.
- **Phasen-Jitter**: Erhöhung des Synchronisations-Schwellenwerts in der State Machine auf 500ms, um häufige Richtungswechsel (Phase-Flips) durch minimalen Takt-Drift zu vermeiden.

## [0.8.30] - 2026-04-03
### Changed
- **v0.8.31**: Stabilisierung der Synchronisation & Smart-Automatik. Fix von Sync-Schleifen.
- **v0.8.30**: Progressive quadratische RPM-Kennlinie für Lüfterstufen 1-10.
- **RPM-Kennlinie**: Optimierte quadratische Verteilung der Lüfterstufen (v0.8.30) zur feineren Steuerung in niedrigen Bereichen (1-4) bei gleichbleibender Maximallast.
- **Dokumentation**: Aktualisierung der RPM-Tabellen in Readme.md und Readme_de.md.

## [0.8.27] - 2026-04-03
### Optimized
- **NTC Stabilization Filter**: Refactoring des `filter_ntc_stable` in `climate.h`. Umstellung auf `constexpr` Konstanten und Optimierung der Sliding-Window-Logik für präzisere Wertermittlung während der thermischen Einschwingphase.
- **Code Modernization**: Nutzung von `std::minmax_element` zur effizienten Abweichungsberechnung innerhalb des Filter-Fensters.

## [0.8.26] - 2026-04-03
### Added
- **Optimized ESP-NOW Sync Strategy**: Bei aktiven Peers im Cache erfolgt die Statusabfrage (`request_peer_status`) nun via Unicast. Dies nutzt die Hardware-Bestätigungen (ACKs) des ESP32 für eine robustere Erkennung und reduziert die Broadcast-Last im WLAN.
- **Unicast-Discovery-Antworten**: Die Bestätigung von Discovery-Anfragen (`send_discovery_confirmation`) wurde auf Unicast umgestellt, um Kollisionen zu vermeiden und die Zuverlässigkeit beim Pairing zu erhöhen.

### Changed
- **Logging-Efficiency**: Das Log-Level für `request_peer_status` wurde von DEBUG auf INFO angehoben, um die Synchronisationsphasen nach dem Boot besser zu dokumentieren. Umgekehrt wurde das redundante Loop-Prevention-Logging in `sync_settings_to_peers` auf DEBUG gesenkt, um das Haupt-Log übersichtlicher zu halten.
- **Protocol-Robustness**: Erweiterung des `handle_espnow_receive` Handlers zur direkten Verarbeitung der Absender-MAC-Adresse (`src_mac`) für zukünftige bidirektionale Diagnose-Features.

### Fixed
- **ESP-NOW Reliability**: Behebung eines Fehlers, bei dem Statusabfragen unter bestimmten Bedingungen nicht an alle bekannten Peers verteilt wurden.

## [0.8.22] - 2026-04-03
### Added
- **Master Device Architecture**: Device ID=1 ist ab sofort der authoritative Master innerhalb einer Raumgruppe. Nur der Master gibt Zyklus-Timing und Richtungs-Synchronisation vor. Einstellungsänderungen (Button / Home Assistant) werden weiterhin von allen Geräten akzeptiert (`MSG_STATE`).
- **Master LED Indicator**: Die Master-LED (PCA9685 Channel 7) leuchtet dauerhaft gedimmt auf dem Master-Gerät (ID=1), sofern kein Fehlerzustand aktiv ist. Non-Master-Geräte behalten das bisherige Verhalten (LED nur bei Fehler).
- **Zentrale Helper-Funktionen**: `is_master()` und `is_from_master()` ersetzen alle hardcodierten `device_id == 1` Prüfungen für bessere Wartbarkeit.

### Fixed
- **Boot Button Glitch**: MCP23017 I/O Expander konnte während der Boot-Initialisierung den Level-Button als gedrückt interpretieren (GPIOs floaten LOW → `inverted: true` = ON). Ein 10-Sekunden Boot-Guard in `handle_intensity_bounce()` und `handle_button_level_click()` verhindert dies jetzt.
- **Zyklus-Timing Drift**: `sync_time()` wird nun ausschließlich vom Master (ID=1) übernommen. Zuvor konnte jeder `MSG_SYNC` Heartbeat die Zyklusposition überschreiben, was bei mehreren Non-Master-Geräten zu kurzzeitigem Timing-Jitter führen konnte.
- **Reboot-Sync Logging**: `MSG_STATUS_RESPONSE` protokolliert jetzt, ob der Sync vom Master oder als Fallback von einem anderen Peer erfolgte.

## [0.8.21] - 2026-04-03
### Added
- **ESP-NOW Hardware-ACK Recovery**: Implementierung eines Fehler-Trackings für Unicast-Übertragungen.
- **Peer-Management**: Automatisches Entfernen von "Stale Peers" nach 3 aufeinanderfolgenden Übertragungsfehlern (`MAX_PEER_SEND_FAILURES`).
- **Selbstheilung**: Intelligente Re-Discovery mit 30s Throttle bei Peer-Verlust zur Wiederherstellung der Gruppen-Integrität.
- **Diagnose-Logging**: Neue Log-Kategorien `espnow_ack` und `espnow_recovery` zur Überwachung der Hardware-Bestätigungen und Recovery-Aktionen.

### Optimized
- **Binary Peer Storage**: Komplette Umstellung des Peer-Managements von rechenintensivem String-Parsing auf einen binären `peer_cache` (`std::vector<PeerEntry>`). Dies ermöglicht O(1) MAC-Lookups bei jedem Sendevorgang und reduziert die CPU-Last im Main-Loop erheblich. Die NVS-basierte Persistenz des `espnow_peers` Strings bleibt für Reboots erhalten und wird im Hintergrund synchronisiert.
- **Performance**: Reduzierung der Latenz bei Unicast-Kommunikation durch Wegfall der String-Interpreten während des Sende-Loops.

## [0.8.15] - 2026-04-03
### Refactored
- **Architektur-Bereinigung**: Vollständige Entfernung des Umbrella-Headers `automation_helpers.h`. Alle Komponenten inkludieren nun nur noch die spezifisch benötigten Header (`globals.h`, `fan_control.h`, etc.), was die Abhängigkeiten minimiert und die Kompilierungszeiten optimiert.
- **Relative Inkludierung**: Normalisierung der Inklusionspfade innerhalb der Helper-Bibliothek, um Redefinitionen durch unterschiedliche Pfad-Mappings im ESPHome-Buildsystem zu verhindern.

### Changed
- **Logging-Optimierung (Noise Reduction)**:
    - Die Sensoren für "Lüfter Richtung" und "BME680 IAQ Bewertung" nutzen nun einen Filter, der nur bei tatsächlicher Statusänderung ein Log-Ereignis auslöst. Dies eliminiert das periodische Log-Spamming alle 2 bzw. 30 Sekunden im Leerlauf.
    - Erhöhung des Update-Intervalls für Zustands-Textsensoren von 2s auf 10s zur Entlastung der CPU.
- **Visuelles Feedback**: Hinzufügen eines "Ramping complete" Logs, um dem Nutzer eine Bestätigung zu geben, wenn der Lüfter nach einer Richtungsänderung oder Modus-Anpassung seinen stabilen Zielzustand erreicht hat.

## [0.8.10] - 2026-04-03
### Changed
- **Fan-Control Refactoring**: `calculate_automatic_pid_demand()` entfernt (Toter Code), da die Demand-Berechnung nun zentral in `auto_mode.h` erfolgt.
- **Tacho-Konsistenz**: `calculate_virtual_fan_rpm()` liefert nun auch bei physischen Tacho-Werten ein korrektes Vorzeichen (negativ für Abluft) für eine konsistente Anzeige im Dashboard.
- **Optimierung der Fan-Logik**: Race-Condition im Logging behoben durch Caching der Basisgeschwindigkeit. 
- **Robustheit**: Manueller Demand wird nun direkt in `calculate_manual_demand()` geclampt (1.0 - 10.0), um Überlauf durch Präsenzerkennung zu verhindern.
- **Wärmerückgewinnung**: Refactoring der Effizienzberechnung in `climate.h` für bessere Stabilität und sauberere Sensorhandhabung.

## [0.8.5] - 2026-04-03
### Changed
- **Auto-Modus Optimierung (Akustik)**: Einführung von Soft-Ramping (max. ±1 Stufe pro 10s) für einen akustisch unauffälligen Betrieb bei Demand-Schwankungen.
- **Hysterese-Verfeinerung**: Implementierung eines 25% Hysterese-Bandes an den Stufengrenzen zur Vermeidung von schnellem Hin- und Herschalten ("Ping-Pong-Effekt").
- **Race Condition Protection**: Ein 2-Sekunden Rate-Limiter in `evaluate_auto_mode()` verhindert Mehrfachausführungen durch konkurrierende Trigger (Timer vs. Netzwerk).
- **Zentralisierung des Hysterese-Status**: `co2_is_controlling` wurde zu einem Klassen-Member verschoben, um die Konsistenz des Regelkreises bei re-entranten Aufrufen zu sichern.
- **Ramping-Boost**: Erlaubt einen einmaligen Sprung von ±2 Stufen direkt nach einem Moduswechsel (z.B. Sommerkühlung), für eine schnellere Reaktion bei stabilen Zielzuständen.

## [0.8.3] - 2026-04-03
### Changed
- **Status-LED Helligkeit**: Der minimale Dimmbereich der Status-LEDs wurde von 10% auf 5% gesenkt, um eine noch dezentere Anzeige in sehr dunklen Räumen zu ermöglichen.

## [0.8.2] - 2026-04-02
### Changed
- **ESP-NOW Unicast-Refactoring**: Umstellung der Zustandssynchronisation von BROADCAST auf gezielten UNICAST. Dies reduziert die Netzwerklast und verhindert, dass Steuerungsbefehle (Modus, Intensität) fälschlicherweise von Geräten in benachbarten Räumen auf demselben Kanal verarbeitet werden.
- **Zielgerichtete Status-Antworten**: `handle_status_request()` antwortet nun direkt per UNICAST an den anfragenden Peer, anstatt eine Antwort an das gesamte Netzwerk zu senden.
- **Bereinigung der Sync-Logik**: Entfernung redundanter BROADCAST-Pakete für das periodische Heartbeat-Interval (`MSG_SYNC`) in der `logic_automation.yaml`.

## [0.8.1] - 2026-04-02
### Changed
- **Internationalization (Documentation)**: Full translation and normalization of the project's documentation into English.
- **Filename Normalization**: Renamed English documentation files to descriptive English filenames (e.g., `Fan-Interface-Guide_en.md`, `VentoMaxx-Comparison_en.md`) for better clarity.
- **Link Updating**: Comprehensive update of the main `Readme_en.md` and internal documentation to ensure all links point correctly to the new English versions.

## [0.8.0] - 2026-04-02
### Changed
- **Prio-Steuerung (CO2 vor Feuchte)**: Grundlegende Umstellung der Automatik-Logik. CO2-Werte haben nun immer Priorität vor der Luftfeuchtigkeit. Nur wenn der CO2-Bedarf gedeckt ist, greift die Feuchtigkeits-Regelung.
- **Hysterese-Härtung**: Implementierung einer intelligenten Hysterese beim Wechsel zwischen CO2- und Feuchtigkeits-PID, um "Ping-Pong"-Effekte und unnötige Drehzahländerungen zu vermeiden.
- **Vereinfachung**: Entfernung des separaten "Automatik CO2" Schalters. Die adaptive Regelung ist nun integraler und permanenter Bestandteil des Automatik-Modus.
- **Protokoll-Update**: Bereinigung des ESP-NOW Datenpakets (`VentilationPacket`). Dies ist ein Breaking Change, der ein Update aller Geräte in der Gruppe erfordert.

## [0.7.42] - 2026-04-02
### Fixed
- **NaN-Härtung**: Absicherung aller PID-Anforderungsberechnungen in `auto_mode.h` und `fan_control.h` gegen `NaN`-Werte. Bei Sensorausfällen (z.B. SCD41/BME680 Glitches) hält das System nun den letzten gültigen Zustand ("Hold-Last-State"), anstatt auf die minimalste Stufe abzufallen. Dies eliminiert das "Yo-Yo"-Verhalten des Lüfters.
- **Steuerungs-Autorität**: Konsolidierung der Intensitätsberechnung auf `evaluate_auto_mode()` als einzige "Source of Truth". Das unabhängige 10s-Intervall `fan_speed_update` nutzt nun den bereits berechneten Level, was Oszillationen durch konkurrierende Berechnungswege verhindert.
- **ESP-NOW Synchronisation**: Übertragung des `auto_mode_active` Status an Peers via `current_mode_index`. Peers im selben Raum synchronisieren nun zuverlässig den Automatik-Modus und ignorieren externe Intensitäts-Overrides, wenn sie selbst im PID-Modus sind.
- **Broadcast-Optimierung**: Umstellung auf direkte Zuweisung der Lüfterintensität im Auto-Modus. Dies verhindert rekursive Broadcast-Kaskaden (Ping-Pong-Effekt) bei jeder PID-Anpassung und schont die Netzwerk-Bandbreite.
- **Konfigurations-Sicherheit**: Implementierung eines Swap-Guards für die `Min/Max Lüfterstufe`. Bei Fehlkonfiguration (Min > Max) korrigiert das System die Werte intern, um eine invertierte Kennlinie zu vermeiden.

## [0.7.39] - 2026-04-01
### Changed
- **UI & Entitäten-Bereinigung**: Umfassende Umbenennung von Entitäten für bessere Klarheit in Home Assistant (z.B. "Lüfter Richtung", "WRG Effizienz").
- **Kategorisierung**: Umzug von Wartungs-Buttons (BME680 Reset) in die Kategorie "Konfiguration".
- **Präfix-Entfernung**: Entfernung des Gerätenamens aus allen Diagnose-Entitäten für eine sauberere Ansicht.
- **ESP-NOW Peers**: Formatierung der Anzeige mit Leerzeichen nach Kommas für bessere Zeilenumbrüche.
- **Bereinigung**: Entfernung des redundanten "System Reboot" Buttons.

## [0.7.37] - 2026-04-01
### Fixed
- **Boot-Log Optimierung**: Hinzufügen einer `on_boot` Synchronisierung für die PID-Regler. Dies verhindert, dass der CO2-Regler beim Systemstart unnötig im Modus `COOL` initialisiert wird und Log-Einträge erzeugt, wenn die Automatik deaktiviert ist.

## [0.7.36] - 2026-04-01
### Fixed
- **Peer-Diagnose Härtung**: Der Peer-Synchronisationsfehler (2x Blinken) respektiert nun den "Peerprüfung" Schalter im Dashboard. 
- **Timeout-Optimierung**: Reduzierung des Peer-Timeouts von 5 auf 3 Minuten für eine schnellere Fehlererkennung bei gleichzeitig minimierten Fehlalarmen durch eine 3-minütige Einschalt-Hysterese.

## [0.7.31] - 2026-04-01
### Added
- **ESP-NOW Protokoll v7**: Erweiterung des Kommunikations-Pakets um Echtzeit-Daten für Lüfter-Drehzahl (RPM), Board-Temperatur (BMP390), Raum-Temperatur (SCD41/BME680) und PID-Anforderung.
- **Dashboard-Modernisierung**: Komplette Überarbeitung der Peer-Ansicht ("Verbundene Geräte") mit neuem Diagnose-Layout.
- **Intelligente Temperatur-Logik**: Implementierung eines automatischen Fallbacks für die Raumtemperatur (SCD41 bevorzugt, BME680 als Ersatz).

### Changed
- **UI-Standardisierung**: Umbenennung des Modus "Wärmerückgewinnung" in das kompaktere "WRG" in der gesamten Weboberfläche.
- **Peer-Visualisierung**: Ersetzung der NTC-Werte ("In/Out") durch ein hochauflösendes Diagnose-Gitter (RPM, PID, Board- & Raum-Temp) für alle vernetzten Geräte.

### Fixed
- **System-Stabilität**: Härtung der Sensor-Abfragen gegen fehlende Hardware (Nullpointer-Schutz und NaN-Handling) in der `ventilation_group`.
- **JS-Robustheit**: Absicherung der Dashboard-Anzeige gegen fehlerhafte oder fehlende Sensor-Daten durch verbesserte Validierung.

## [0.7.30] - 2026-04-01
### Fixed
- **Log-Bereinigung**: Deaktivierung des sekündlichen Debug-Spams ("0.00 pulses/min") der `pulse_counter`-Komponente für Lüfter ohne physischen Tacho-Anschluss.
- **RPM-Diagnose**: Der Sensor für die **virtuelle Drehzahl** (berechnet aus PWM & Richtung) loggt nun ohne Delta-Filter, um in der Konsole eine konsistente Echtzeit-Rückmeldung der Lüfteraktivität zu geben.

## [0.7.28] - 2026-04-01
### Fixed
- **Log-Spam & I2C-Optimierung**: Die LED-Ansteuerung (`update_leds_logic`) wurde komplett auf eine **statusbewusste (stateful) Logik** umgestellt. Hardware-Befehle werden nun nur noch gesendet, wenn sich der Zustand (Modus, Helligkeit, UI-Aktivität) tatsächlich ändert. Dies stoppt das wiederholte Auftreten von "Setting"-Logs bei Sensor-Updates.
- **C++ Struktur**: Behebung eines Kompilierfehlers durch korrekte Anordnung der Funktions-Deklarationen in `led_feedback.h`.
- **UI-Priorität**: Sicherstellung, dass die Fehleranzeige der Master-LED (`check_master_led_error`) immer ausgeführt wird, unabhängig vom Status der restlichen Lüftungs-Logik.

## [0.7.27] - 2026-04-01
### Added
- **System-Härtung (Watchdog)**: Implementierung eines gestuften Sicherheits-Konzepts gegen "Zombie-Zustände" (ESP ist online, aber interne Tasks hängen).
  - ESP-IDF Task Watchdog (TWDT) auf **15s** verkürzt mit Idle-Task Monitoring auf beiden Cores.
  - Aktivierung der Kernel-Flags `CONFIG_ESP_TASK_WDT_INIT` und `CONFIG_FREERTOS_WDT_EN` für maximale Ausfallsicherheit.
  - Neuer Diagnose-Sensor `Watchdog Restarts` (persistent via NVS) zur Überwachung unerwarteter Neustarts in Home Assistant.
- **Automatisierte Selbstheilung (Health Check)**: Einführung eines 10-Minuten-Timers, der die Aktivität des Haupt-Loops überwacht und bei einem "Brain Freeze" (Logik-Hängern) einen automatischen Neustart erzwingt.

### Fixed
- **Blockierender Code**: Entfernung eines `delay()`-Aufrufs in der `network_sync.h`, der den Systemstart verzögern konnte.
- **Endlosschleife (Rekursion)**: Behebung einer zirkulären Abhängigkeit zwischen Fan-Logik und Hardware-Aktualisierung in `fan_control.h`, die zu System-Hängern führen konnte.
- **Framework-Kompatibilität**: Umstellung des Reset-Befehls auf `esp_restart()` für volle Kompatibilität mit dem ESP-IDF Framework.

## [0.7.21] - 2026-03-31
### Fixed
- **System-Stabilität**: Einführung einer "Stateful"-Logik in der `led_feedback.h`, um I2C-Bus-Flutungen durch redundante LED-Befehle zu verhindern.
- **WLAN & Erreichbarkeit**: 
  - Reduzierung des I2C-Timeouts auf **20ms** zur Vermeidung von Loop-Blocking.
  - Verkürzung des `reboot_timeout` auf **1min** für schnellere Selbstheilung bei Verbindungsverlust.
  - Umstellung auf `power_save_mode: LIGHT` zur Verbesserung der Stabilität auf dem ESP32-C6.
- **Sicherheit**: Mathematische Absicherung (`NaN`-Check) der Effizienzberechnung in der `sensors_climate.yaml`.

### Changed
- **I2C-Bus Optimierung**: Anpassung der Frequenz auf 100kHz und Timeout auf 100ms für eine noch robustere Kommunikation mit den Sensoren.
- **Dokumentation**: Übersetzung technischer Kommentare in der `hardware_io.yaml` ins Englische.

### Fixed
- **Wärmerückgewinnung (WRG) Effizienz**: 
  - Entfernung des ungenauen **BMP390** (Gehäuse-Sensor) als Referenzquelle zur Vermeidung von Verfälschungen durch Elektronik-Abwärme.
  - Senkung der Berechnungsschwelle von **1,0 K auf 0,3 K** zur Verbesserung der Messgenauigkeit bei geringen Temperaturunterschieden (Übergangszeit).

### Added
- **Diagnose-Sensor**: Neuer Text-Sensor `WRG Referenz-Messpunkt` zur transparenten Anzeige der verwendeten Temperaturquelle (SCD41 vs. BME680).

### Added
- **Priorisierte Diagnose-Blinkcodes**: Die Master-LED signalisiert nun verschiedene Fehlerzustände über ein mehrstufiges Blink-System (Prioritäts-Ladder):
  1. **2 Pulse (Höchste Prio)**: ESP-NOW Peer-Synchronisierungsfehler.
  2. **3 Pulse (Mittlere Prio)**: WLAN-Verbindungsverlust.
  3. **4 Pulse (Niedrige Prio)**: Thermische Warnung (Traco PSU Schutz, aktiv zwischen 50°C und 60°C).
- **Netzwerk-Härtung (ESP32-C6)**: 
  - `power_save_mode: NONE` erzwingt permanent aktives WLAN zur Vermeidung von Stack-Lockups.
  - `reboot_timeout: 15min` als Watchdog für nicht-reaktive Netzwerkzustände.

### Changed
- **Codestyle-Bereinigung**: Alle verbleibenden Kommentare in den YAML-Konfigurationsdateien wurden zur besseren internationalen Wartbarkeit auf Englisch umgestellt.
- **C++ Refactoring**: Optimierung der `check_master_led_error` Logik in `led_feedback.h` zur sauberen Abbildung der neuen Fehler-Prioritäten.

### Added
- **Hardware-Sicherheitssystem**: Implementierung einer mehrstufigen Temperaturüberwachung (BMP390) zum Schutz des Traco-Netzteils.
  - Warnstufe (50°C): Warn-Log und HA-Benachrichtigung.
  - Kritische Stufe (60°C): Error-Log, kritische Benachrichtigung, automatischer Lüfterstopp (50% PWM) und 60-minütiger Deep Sleep zur Abkühlung.
  - Entwarnung (<48°C): Info-Log und Entwarnungs-Benachrichtigung.
- **System-Diagnose**: Integration einer Debug-Komponente zur Überwachung des freien Speichers (Heap RAM) in Home Assistant.
- **Geräte-Identifikation**: Home Assistant Benachrichtigungen enthalten nun automatisch den `${friendly_name}` für eine einfachere Zuordnung bei mehreren Geräten.

### Changed
- **I2C-Bus Härtung**: Optimierung der I2C-Parameter zur Vermeidung von "Zombie-Zuständen" (Bus-Lockups). Frequenz auf 50kHz (Standard) und Timeout auf 13ms gesetzt für schnellere Recovery.
- **Architektur-Refactoring (Modularisierung)**: Die Sensorkonfiguration wurde für bessere Wartbarkeit in dedizierte Pakete aufgeteilt:
  - `sensor_BMP390.yaml`: BMP390 Druck & Temperatur + Sicherheitslogik.
  - `sensor_SCD41.yaml`: SCD41 CO2, Temp & Feuchte + Kalibrierung.
  - `sensor_NTC.yaml`: Analoge NTC-Fühler (Zuluft/Abluft) + ADC-Konfiguration.
  - `hardware_fan.yaml`: Konsolidierung von PWM-Output, RPM-Tacho und Lüfter-Komponente.

## [0.7.6] - 2026-03-30
### Fixed
- Fixed Slave synchronization when Master ID is 2 (e.g., during device transition).
- Heartbeats (`MSG_SYNC`) are now accepted from ID 1 and ID 2 to ensure state consensus.
- Forced UI sync for manual state changes (`MSG_STATE`) even if internal state matches.
- Added detailed debug logging for why packets are ignored in the room group.

## [0.7.0] - 2026-03-30
### Fixed
- **Manueller State-Override (HA/UI)**: Es wurde ein Fehler behoben, bei dem manuelle Änderungen in Home Assistant (Modus-Wechsel, Intensitäts-Slider) nicht sofort an alle Peers übertragen wurden. Dies passierte, da das neue 60-Sekunden-Heartbeat-System (`MSG_SYNC`) fälschlicherweise als "ausreichend" für die Synchronisation angesehen wurde. Wir haben nun explizite `sync_settings_to_peers()`-Aufrufe in jeden UI-Interaktionspfad implementiert. Jede manuelle Änderung triggert nun sofort ein `MSG_STATE`-Paket, das alle Peers zur sofortigen Übernahme zwingt.

## [0.6.97] - 2026-03-30

### Fixed

- **Dashboard Verbundene Geräte (ESP-NOW)**: Fehler behoben, bei dem die Nachbargeräte aus dem Dashboard verschwunden sind. Grund war, dass der konfigurierbare `sync_interval_config` (z. B. 3 Stunden) dafür gesorgt hat, dass Geräte in bestimmten Modi (z. B. Durchlüften oder Aus) nicht mehr regelmäßig gesendet haben und deshalb nach dem internen 5-Minuten-Timeout fälschlicherweise als "Offline" markiert und gelöscht wurden. Es wurde nun ein konstanter UI-Heartbeat (alle 60 Sekunden) implementiert.
- **WLAN Kanal-Hopping & I2C Aussetzer**: Ist die WLAN-Verbindung zum Router abgerissen, hat das Gerät intensiv das komplette 2.4GHz Band gescannt. Dies hat den Single-Core SoC überlastet, die I2C-Sensorkommunikation (SCD41) abreißen lassen und parallel den ESP-NOW Funkverkehr zu den anderen Räumen blockiert. Durch `fast_connect: true` fokussiert sich das Gerät nun stabil auf den konfigurierten Kanal.
- **Webserver-Crash & Tote Sockets (Error 23)**: Ein Router-Neustart führte dazu, dass im Hintergrund Web-Browser (SSE / EventSource) "unsichtbar" die Verbindung verloren haben. Der ESP-IDF Webserver lief mit maximal 10 Sockets durch Fehler 23 (`httpd_accept_conn: error in accept (23) ENFILE`) über. Durch das Erhöhen auf `CONFIG_LWIP_MAX_SOCKETS: 16` gibt es nun den essenziellen Pufferraum.
- **Kompletter Auto-Recovery nach Router-Ausfall**: Bleibt der Router / das WLAN für mehr als 5 Minuten verschwunden, macht das Gerät nun automatisch einen sauberen Neustart (`reboot_timeout: 5min`). Das entfernt zuverlässig gestrandete Zombie-Sockets und startet den System-Bus fehlerfrei neu durch, um sich nahtlos zurück ins Netzwerk zu integrieren.

## [0.6.96] - 2026-03-30
- (Interne Anpassungen)

## [0.6.95] - 2026-03-30
- (Interne Anpassungen)

## [0.6.94] - 2026-03-30

### Fixed

- **Aggressives Master-Override (ESP-NOW)**: Der "Master Device Authority Enforcer" in `network_sync.h` wurde entfernt. Dieser überschrieb zuvor fehlerhaft die gesamte Gruppe, wenn ein "Master"-Gerät neu startete und sein Recovery-Paket mit alten Konfigurationseinstellungen flashte. Geräte übernehmen nun Modus-Zustände aus dem Netzwerk nur noch bei dedizierten Sync-Events, echten State-Änderungen oder Status-Requests.
- **Boot-Loop und Light Sleep Wakeup**: Beim Aufwecken aus dem energiersparenden "Aus"-Zustand (Hardware-Button) flutet das Gerät das ESP-NOW-Netzwerk nicht mehr sofort mit seinem alten, lokal gespeicherten Zustand. Es fährt stattdessen die Lüfter lokal hoch, stummgeschaltet (`pending_broadcast = false`), und wartet auf den `MSG_STATUS_RESPONSE` des Master-Geräts, um sich nahtlos in die Gruppe einzufügen.
- **WiFi Connect Discovery**: Die Recovery-Erkennung (`request_peer_status`) triggert nun intelligent bei **jedem** erfolgreichen WLAN-Reconnect und nicht nur einmalig beim Hardware-Boot (wie bisheriger `static done` Guard).

### Changed

- **Web-Dashboard Sicherheit (XSS)**: Einführung strikter HTML-Sanitisierung (`sanitizeHTML`) und Umstellung von `innerHTML`/`innerText` Zuweisungen auf saubere Typkonvertierungen im Web-Dashboard (`dashboard_html.h`), um serverseitige XSS-Angriffsvektoren über den `/state` Endpoint sicher auszuschließen.
- **Backend-Whitelist Validierung**: Der lokale `/set` Endpoint in `wrg_dashboard.cpp` schützt die REST-Schnittstelle nun mit einem Whitelist-Filter (`std::unordered_set`) und verhindert die Verarbeitung von manipulierten GET-Parametern.
- **C++ Bounds Checking & Memory Safety**: Die `calculate_ramp_up` und `calculate_ramp_down` Funktionen in der Lüftungslogik erhielten saubere In-/Out-Bounding Checks sowie `memset` Zero-Init für das `VentilationPacket` Struktur-Objekt, um potentielle Integer-Overflows oder Memory-Padding Leaks abzufangen.

## [0.6.79] - 2026-03-29

### Refactored

- **C++ Modularisierung**: Die große Datei `automation_helpers.h` (~1750 Zeilen) wurde drastisch aufgeräumt und fachlich in kleinere, gekapselte Module im neuen Ordner `components/helpers/` unterteilt (z. B. `fan_control.h`, `auto_mode.h`, `network_sync.h`, `led_feedback.h`).
- **Abhängigkeiten optimiert**: `automation_helpers.h` fungiert nun als sauberer *Umbrella-Header*, wodurch keine YAML-Änderungen an den Lambdas erforderlich sind. Zirkuläre Abhängigkeiten wurden durch automatisch generierte *Forward Declarations* in `globals.h` behoben.

## [0.6.74] - 2026-03-29

### Added

- **LED-Helligkeit Synchronisierung**: Unterstützung für die raumweite Synchronisierung der "Maximalen LED Helligkeit". Änderungen an einem Gerät werden nun sofort via ESP-NOW an alle Peer-Geräte übertragen.
- **Protokoll-Update (v5)**: Erhöhung der `PROTOCOL_VERSION` auf 5 zur Unterstützung der Helligkeits-Synchronisierung. (Erfordert Neu-Flashen aller Geräte).

### Fixed

- **Lüfterstufen-Limit (Automatik)**: Implementierung einer strikten Bedarfs-Begrenzung (PID Clamping) und Hardware-Clamping (Stufe 10), um ein Überschreiten der konfigurierten "Max-Lüfterstufe" bei extremen CO2-Werten zu verhindern.
- **Log-Spam Reduzierung**: Reduzierung der "Hardware Refresh" und "Ramping speed" Log-Frequenz auf 1s-Intervalle. Kritische Zustandsänderungen (z. B. Richtungswechsel) werden weiterhin sofort geloggt.

## [0.6.70] - 2026-03-28

### Fixed

- **C++ Kompilierfehler**: Behebung eines Typs-Konflikts in `automation_helpers.h`. Die Variable `fan_intensity_level` wurde auf `RestoringGlobalsComponent<int>` umgestellt, um mit der YAML-Einstellung `restore_value: true` übereinzustimmen.
- **Filter-Laufzeit Logik**: Sicherstellung der Funktionsfähigkeit der Filter-Wartungsanzeige durch Verschiebung der SNTP-Zeitsynchronisation (`sntp_time`) in das Basis-Paket `packages/esp32c6_common.yaml`.

### Removed

- **Diagnose-Display**: Vollständige Entfernung des optionalen OLED-Pakets `display_diagnostics.yaml` sowie aller zugehörigen Code-Referenzen und Dokumentationseinträge.

## [0.6.67] - 2026-03-27

### Changed

- **Internationalisierung**: Übersetzung aller Kommentare und UI-Texte (Namen, Optionen) in allen Paket-Dateien (`packages/*.yaml`) von Deutsch nach Englisch zur besseren Wartbarkeit.
- **System**: Erhöhung der Projektversion auf 0.6.67.

## [0.6.66] - 2026-03-27

### Changed

- **Modularisierung**: Weitere Auslagerung von Sensor-Komponenten in dedizierte Pakete.
  - Neuerstellung von `packages/sensor_LD2450.yaml` (Radar-Präsenzerkennung).
  - Umbenennung von `packages/BME680.yaml` in `packages/sensor_BME680.yaml` zur Vereinheitlichung.
  - Bereinigung der `packages/sensors_climate.yaml` (Entfernung von Radar-Entitäten).
  - Update der Hauptkonfiguration `ventosync.yaml` mit den neuen Pfaden.

## [0.6.65] - 2026-03-27

### Changed

- **OLED Diagnostics Display**: Umfassende Überarbeitung der UI für bessere Ablesbarkeit.
  - Erhöhung der Schriftgrößen (Small: 12pt, Medium: 16pt, Large: 22pt).
  - Neue dedizierte Seite für **BME680 IAQ/eCO2**, inklusive Trend-Anzeige und Sensor-Status.
  - Optimierung der **Wärmetauscher-Seite** zur Anzeige der neuen, stabilisierten WRG-Effizienz.
  - Layout-Anpassungen für klarere Datenvisualisierung auf dem SH1106 Display.

## [0.6.64] - 2026-03-27

### Added

- **BME680 Standalone Package**: Vollständige Modularisierung der BME680-Logik in `packages/BME680.yaml` zur besseren Wartbarkeit.
- **Advanced IAQ & eCO2 Logic**: Implementierung einer selbstlernenden Gas-Baseline mit **NVS-Speicherung** (überlebt Reboots) und einem präzisen eCO2-Modell (400–5000 ppm).
- **Erweiterte Umwelt-Sensorik**: Neue Berechnungen für Taupunkt, absolute Luftfeuchtigkeit und höhenkorrigierten Luftdruck (250m).
- **IAQ Trend- & Status-Analyse**: Echtzeit-Tracking der Luftqualitätsänderung (ppm/min) und Überwachung der 48-stündigen Burn-in Phase mit Statusanzeige.
- **Diagnose-Tools**: Neuer Reset-Button zum manuellen Zurücksetzen der gelernten BME680-Baseline.

### Fixed

- **Wärmerückgewinnung Effizienz**: Grundlegende Korrektur der WRG-Berechnung. Die Effizienz wird nun ausschließlich in der stabilen **Zuluft-Phase** (Intake) ermittelt, inklusive einer 30-sekündigen thermischen Stabilisierungszeit nach jedem Richtungswechsel.
- **Präzisions-Upgrade**: Die Raumtemperatur für die WRG-Berechnung wird nun mit Priorität vom **SCD41** oder **BME680** bezogen, um Verfälschungen durch Eigenwärme (BMP390) zu minimieren.

### Changed

- **Plattform-Migration & Performance**: Umstellung von `bme68x_bsec2` auf die Standard `bme680` Plattform. Dies führt zu einer massiven Reduzierung der Kompilierungszeit durch Entfernung der proprietären Bosch BSEC2-Bibliothek.
- **Transparente Fallback-Logik**: `effective_co2` nutzt nun das verbesserte eCO2-Modell des BME680 als redundanten Indikator zum SCD41.

### Removed

- **BSEC2-Abhängigkeiten**: Vollständige Entfernung aller BSEC-spezifischen Komponenten und Hub-Konfigurationen zur Systemverschlankung.

## [0.6.60] - 2026-03-27

### Added

- **Gruppenweite UI-Synchronisierung**: Wenn ein Gerät in einer Lüftungsgruppe den Modus oder die Intensität ändert (via Taster, Dashboard oder Home Assistant), wachen nun automatisch die Displays (LEDs) aller Peer-Geräte im Raum auf.
  - Aktiviert `ui_active` für 30 Sekunden auf allen synchronisierten Geräten.
  - Sorgt für unmittelbares visuelles Feedback im gesamten Raum, auch wenn die Teilnehmer vorher gedimmt waren.
- **Umfassende Roadmap-Erweiterung**: Integration zahlreicher Profi-Features in die Planung (inspiriert von Ambientika Smart):
  - Außer-Haus-Modus (Schimmelschutz via Sicherheits-Entfeuchtung).
  - Überwachungs-Modus (Sensor-Only ohne Sleep).
  - Frostschutz-Automatik für den Wärmetauscher.
  - Zeitgesteuerter Einrichtbetrieb (Manueller Timer).
  - Autarker Wochenzeitplan (direkt auf dem ESP32).

### Fixed

- **ESP-NOW Hybrid-Synchronisation**: Optimierung der Sende-Logik für Statusänderungen. Kritische Änderungen (Modus/Stufe) werden nun sofort via Broadcast (für maximale Robustheit) und zusätzlich via Unicast (für Zuverlässigkeit) verteilt.
- **Boot-Initialisierungs-Race-Condition**: Behebung eines Problems, bei dem Geräte-IDs beim Start fälschlicherweise als "0" initialisiert wurden, bevor die NVS-Wiederherstellung abgeschlossen war.
  - `on_boot` Priorität auf `-100.0` (DATA) gesenkt.
  - 2s Sicherheits-Delay vor der ersten Konfigurations-Synchronisation hinzugefügt.
  - Zyklischer Integrity-Check (10s) erkennt und korrigiert unvollständige Initialisierungen automatisch.

### Changed

- **Versionierung**: Projektversion auf 0.6.60 angehoben.

## [0.6.50] - 2026-03-26

### Fixed

- **Kritische ESP-NOW Synchronisation**: Grundlegender Fix der `send()` API (direkte Nutzung von Pointer und Größe statt `std::vector`-Objekt), was die Kommunikation und Status-Synchronisation zwischen den Geräten wiederhergestellt hat.
- **ESP-NOW Protokoll-Sicherheit (K2)**: Einführung eines `protocol_version` Feldes im `VentilationPacket` und Anhebung auf **Protokoll-Version 4**. Dies verhindert Speicher-Korruption bei inkompatiblem Firmware-Mischbetrieb während Rollouts.
- **ESP-NOW Loop-Prävention (K1/W6)**: Unterbindung einer potentiellen Endlosschleife (Ping-Pong-Effekt) beim Konfigurations-Sync durch gezielte Guards in `evaluate_auto_mode()`.
- **Systemstabilität nach 49,7 Tagen (W3)**: Behebung eines Fehlers bei der Peer-Timeout-Erkennung nach einem `millis()` Overflows durch Einführung eines dedizierten `has_peer_pid_demand` Flags.
- **Summer-Cooling Hysterese**: Implementierung eines 1.5°C/0.5°C Totbands, um hektisches Umschalten (Flapping) zwischen WRG und Sommer-Kühlung an der Temperaturschwelle zu verhindern.
- **Verbesserte Fehlerbehandlung**: Korrektur von Format-Strings (%lld -> %d), Namespace-Zuweisungen (`esphome::HardwareState`) und Hinzufügen von Cast-Guards zur Vermeidung von Undefined Behaviour beim Paketempfang.
- **Power-Off Logik**: Fix eines Fehlers, bei dem das System nach Power-Down via Long-Click nicht vollständig in den "Aus"-Zustand wechselte.

### Changed

- **C++17 Modernisierung**: Umstellung mutabler statischer Variablen auf `inline` zur Gewährleistung der Linkage-Sicherheit in einer Single-Translation-Unit.
- **Architektur & Modulardesign**: Einführung von `extern` Deklarationen in `automation_helpers.h`, was eine sauberere Trennung von generiertem Code und Logik ermöglicht.
- **NTC Filter-Diagnose (W1)**: Neue Warnung im Log, falls die gewählte Zyklusdauer physikalisch zu kurz für eine stabile NTC-Messung ist.
- **Refactoring Discovery**: Umbenennung von `load_peers_from_flash()` in `load_peers_from_runtime_cache()`, um das tatsächliche Verhalten (flüchtige Speicherung) korrekt im Code zu dokumentieren.
- **YAML Performance**: Unnötige String-Kopien (via `c_str()`) in UI-Actions entfernt.

## [0.6.37] - 2026-03-26

### Added

- **Dynamische ESP-NOW Discovery**: Umstellung von statischem Broadcast auf ein intelligentes Peer-to-Peer Discovery-System. Geräte finden sich nun beim Booten oder bei Raumwechseln automatisch über einen Discovery-Handshake (`ROOM_DISC` / `ROOM_CONF`).
- **NVS-basierte Peer-Persistenz**: Entdeckte Peers werden dauerhaft im Flash-Speicher gespeichert, was einen sofortigen Verbindungsaufbau nach einem Neustart ermöglicht.
- **Effiziente Unicast-Synchronisation**: Nach der initialen Kopplung erfolgt der gesamte Datenaustausch (PID-Demand, Status-Sync) per gezieltem Unicast statt Broadcast. Dies reduziert den Netzwerk-Overhead massiv.
- **Diagnose-Entität "ESP-NOW Peers"**: Neuer Text-Sensor in Home Assistant zur Echtzeit-Anzeige aller aktuell verbundenen MAC-Adressen der Lüftungsgruppe.
- **Sensor-Sichtbarkeit in HA**: Einführung eines `delta: 0.1` Filters für RAW-Temperatursensoren, um sicherzustellen, dass Werte unmittelbar in Home Assistant veröffentlicht werden.

### Fixed

- **Wiederherstellung NTC-Filter**: Die versehentlich entfernte Funktion `filter_ntc_stable` wurde wiederhergestellt, wodurch die primären Temperatursensoren wieder zuverlässige Werte liefern (statt "unknown").
- **ESPHome Kompilierfehler**: Korrektur von Typ-Konflikten in `automation_helpers.h` (Nutzung der statischen Variablen aus `main.cpp` statt fehlerhafter `extern` Deklarationen).
- **YAML Validierung**: Behebung von Fehlern bei den Global-Variablen (Umbenennung `max_length` -> `max_restore_data_length` und Korrektur auf den Maximalwert 254).

## [0.6.14] - 2026-03-22

### Added

- **GitHub Actions CI**: Offizielle `esphome/build-action` als `.github/workflows/build.yaml` hinterlegt, um die Firmware-Kompilierung bei jedem Git Push automatisch zu prüfen.
- **Echte VentoMaxx V-Kennlinie (Stopp-Modus)**: Der Lüfter wird nun exakt nach Oszilloskop-Messungen der Original-Steuerung angesteuert (50% = Stopp, V-Kurve für Richtungswechsel).
- **Text-Sensor "Aktuelle Luftrichtung"**: Neue Entität in HA und Dashboard zur Anzeige der Strömungsrichtung ("Zuluft (Rein)", "Abluft (Raus)", "Stillstand").
- **Virtuelle Drehzahlberechnung (Fallback)**: Automatische Berechnung der RPM (4200 RPM @ 100%), falls kein physisches Tacho-Signal (4-Pin) erkannt wird. Berücksichtigt weiche Anlauf-/Auslauframpen und zeigt die Luftrichtung (Abluft = negative Werte) an.
- **Dynamische Präsenz-Kompensation (Manuelle Modi)**: Der Radar-Sensor wirkt nun als flexibler Boost/Dämpfer (+/- Stufen) in den Modi WRG, Durchlüften und Stoßlüftung.
- **Power-Button Toggle-Funktion**: Ein kurzes Drücken des Power-Buttons wechselt nun zwischen Ein- und Ausschalten (vorher nur Einschalten möglich).
- **Light Sleep & Hardware-Energiesparen**: Automatische Abschaltung von WLAN und PCA9685 LED-Treiber im Modus "Aus" zur signifikanten Reduzierung des Stromverbrauchs (Light Sleep Modus).

### Changed

- **Dashboard UI (Lokales Gerät)**: Das eigene Gerät wird in der ESP-NOW Peer-Liste des Dashboards ab sofort permanent oben links, grau abgesetzt und als "(lokales Gerät)" markiert dargestellt.
- **Dokumentation Stromverbrauch**: Die `Readme.md` wurde mit echten gemessenen Leistungsdaten aktualisiert (Stufe 1: ~2,8W, Stufe 5: ~3,5W, Stufe 10: ~5,5W inkl. gesamter Sensorik).
- **Dashboard-Optimierung**: Veralteter "Test Speed Slider" entfernt und durch die dynamische Anzeige der Luftrichtung ersetzt.
- **PWM-Logik-Invertierung**: MOSFET-Ansteuerung in `hardware_io.yaml` auf `inverted: true` gesetzt, um konsistente Spannungspegel zu gewährleisten.
- **Radar-Logik Refactoring**: Präsenz-Kompensation aus dem Automatik-Modus entfernt (dort nun rein PID-basiert für CO2/Feuchte) und stattdessen als dynamischen Boost in manuelle Modi integriert.
- **Reaktionszeit-Optimierung**: Automatik- und Lüfter-Intervalle von 60s auf **10s** verkürzt für unmittelbarere Reaktion auf Präsenz und Sommer/Winter-Umschaltung.
- **Bereinigung Automatisierungs-Logik**: Veraltete und konkurrierende CO2-Prüfintervalle (`apply_co2_auto_control`) vollständig entfernt.

- **Software-gesteuertes Fan Ramping (WRG)**: Einführung einer 5-sekündigen sanften Anlauf- und Auslauframpe bei Richtungswechseln im WRG- und Stoßlüftungs-Modus. Dies schont die Hardware und reduziert Geräusche während der Umschaltphasen. Die Intensitäts-LEDs bleiben dabei entkoppelt auf dem Zielwert.
- **Optimierte LED-Logik & Power-Dimming**:
  - Die **Power-LED** dimmt bei Inaktivität (nach 60s) nun auf 20% Helligkeit herunter, anstatt vollständig zu löschen, um den Betriebszustand dezent anzuzeigen.
  - Fehler im **Boot-Selbsttest** behoben (3s LED-Lauflicht funktioniert nun zuverlässig).
  - Modus-LEDs reagieren nun unmittelbar auf Änderungen über das Home Assistant Web-Interface.
- **Dokumentations-Restrukturierung (`Readme.md`)**: "Implementierte Erweiterungen"-Sektion aufgelöst und alle Features (ESP-NOW Dashboard, Adaptive Automatik, Radar, Feuchte-Management) thematisch fließend in die jeweiligen Hauptkapitel integriert für eine logischere Leserführung.
- **Bedienkonzept & Stoßlüftung**: Dokumentation der Stoßlüftung korrigiert (Lüfter läuft auf der aktuell eingestellten manuellen Stufe, nicht zwingend auf 100%). Modus-Zyklus-Reihenfolge in der Readme exakt an das physische Taster-Verhalten (Automatik → WRG → Durchlüften → Stoßlüftung → Aus) angeglichen.
- **Feuchte-Management HA Setup**: Die detaillierte Anleitung zur Bereitstellung des externen `sensor.outdoor_humidity` für das Feuchte-Management in ein separates Dokument (`documentation/Feuchte-Management-HA-Sensor.md`) ausgelagert.
- **VentoMaxx 3-Pin Lüfter**: Hinweise zum fehlenden Tacho-Signal bei originalen VentoMaxx EBM-PAPST Lüftern in der `Readme.md` und `documentation/Anleitung-Fan-Circuit.md` präzisiert.
- **Sensor-Lock im manuellen Betrieb**: `apply_co2_auto_control` in `automation_helpers.h` ist nun durch `auto_mode_active->value()` flankiert. Sensordaten und PID-Regler verändern die PWM-Leistung nur noch im bestätigten "Smart-Automatik" Modus und funken manuellen Modi (WRG, Querlüftung, Stoßlüftung) nicht mehr dazwischen.
- **Modern Web-Dashboard (Tailwind CSS) & UX**: Vollständige Neugestaltung des asynchronen Web-Dashboards (`wrg_dashboard`) mit Tailwind CSS. Modernes Dark-Mode Design, voll-responsives Layout und verbesserte Performance durch optimiertes CSS-Delivery via CDN.
- **Dashboard Sektion: Grundeinstellungen**: Integration einer neuen Sektion zur Anzeige von Geräte-ID, Floor ID, Room ID und Geräte-Phase (A/B) direkt im Dashboard für eine einfachere Vor-Ort-Identifikation und Konfiguration.
- **C++ Automatik Optimierungen & Thread Safety (`wrg_dashboard.cpp`)**: Einführung von `std::mutex` und `std::lock_guard` für ein 100% exception-sicheres Queueing von Web-API Actions. Komplettes Neuschreiben der `loop()` und internen JSON Parsing-Logik mit DRY C++ Lambdas. Strikte "const correctness" (`const std::string&`) angewendet und ungenutzte Cache-Referenzen (`DashboardSnapshot`) für minimalen RAM-Footprint entfernt.
- **C++ Code-Style & Best Practices**: Umfangreiches Refactoring der Logik in `automation_helpers.h`. Konstanten (`const`), C++ STL-Algorithmen (`std::max`, `std::clamp`) und C++17 Bindings ersetzen alte Magic-Numbers und C-Macros.
- **Dynamische Zyklusdauer**: Der statische UI-Slider für die `Zyklusdauer` wurde vollständig entfernt. Das System berechnet die Zyklusdauer im C++-Code nun dynamisch basierend auf der Lüfterintensität (linear skalierend von 70s bei Stufe 1 bis zu rasanten 50s bei Stufe 10) in `update_fan_logic()`.
- **Dynamische NTC-Temperatur-Stabilisierung (`filter_ntc_stable`)**:
  - Statischer Wartezeitraum für thermische Trägheit der NTCs wurde nach dem Richtungswechsel skalierend gestaltet: Wartet dynamisch **60 % des Zyklus (z. B. 30s bei Stufe 10)** ab.
  - Das `delta: 0.2` Flag in der YAML Konfiguration blockierte stabile konstante Temperaturen vom Weiterleiten und verklemmte das Window: **Filter wurde entfernt**, das fortlaufend gleitende Sensor-Zeitfenster wächst nun im C++ dynamisch mit.
- **Zusammengefasste Radar Anwesenheits-Kompensation (-5 bis +5)**:
  - Drei getrennte Variablen für das Anwesenheitsverhalten zusammengelegt zu einem einzigen gleitenden Range-Slider (`auto_presence_slider` von -5 bis +5) für Home Assistant.
  - `VentilationPacket` Struktur aktualisiert: `int8_t auto_presence_val` ersetzt die drei Vorgängervariablen. **ESP-NOW Protokoll-Version auf v3 angehoben.** (Erfordert Neu-Flashen aller Raumnodes).
- **Einheitliche Min/Max Parameter (`automatik_min_fan_level`)**: Zuvor fälschlicherweise auf CO2-Auto begrenzte Bezeichner wurden generisch umbenannt (`co2_min_fan_level` -> `automatik_min_fan_level`), da sie gleichermaßen die Begrenzungslinien des Feuchtigkeitsreglers darstellen.

### Added

- **ESP-NOW Peer Visualisierung auf dem Dashboard**: Anzeige der verbundenen ESP-NOW Geräte der Lüftungsgruppe im lokalen Web-Dashboard (`/ui`). Zeigt dynamisch Node-ID, Modus, Phase (Zuluft/Abluft), Lüfterstufe und Temperaturen für alle aktiven Peers in Echtzeit an.
- **Lokales Web-Dashboard URL (`/ui`)**: Das benutzerdefinierte Dashboard (`wrg_dashboard`) wurde von `/` auf `/ui` verschoben, um Konflikte mit dem Standard-ESPHome-UI (`web_server`) zu verhindern.
- **Lokales Web-Dashboard (`wrg_dashboard`)**: Neues Feature für den Zugriff auf alle Sensorwerte, Statusinformationen und Einstellungen (Lüfterstufe, Modus, CO2-Grenzwerte) direkt über einen integrierten, asynchronen Webserver auf dem ESP32. Die responsive Weboberfläche bietet animierte Kacheln, Echtzeit-Updates und interaktive Graphen – völlig unabhängig von Cloud-Diensten oder zwingendem Home Assistant Einsatz.
- **Entitäten-Dokumentation**: Neues Referenz-Dokument `documentation/Entities_Documentation.md` mit einer vollständigen Übersicht und thematischen Gruppierung aller bereitgestellten Home Assistant Entitäten inklusive technischer IDs hinzugefügt.

- **ebm-papst VarioPro: Korrektes Single-PWM-Mapping** (`set_fan_logic()` in `automation_helpers.h`):
  - Der Lüfter 4412 F/2 GLL nutzt **ein einzelnes PWM-Signal** für Drehzahl und Richtung: `50 % = Stopp`, `< 50 % = Richtung A (Abluft)`, `> 50 % = Richtung B (Zuluft)`.
  - Vorher wurde 0–100 % direkt ausgegeben (Lüfter drehte immer in Richtung B, kein Stopp möglich).
  - Neu: `direction 0 → pwm = 0.5 - (speed × 0.5)`, `direction 1 → pwm = 0.5 + (speed × 0.5)`.
  - Soft-Stop-Zone: Unterhalb von 5 % Drehzahl wird immer exakt 50 % PWM ausgegeben (sicheres Stoppen ohne Kriechen).
- **Mindestdrehzahl Stufe 1 (10 %)** (`update_fan_logic()` in `automation_helpers.h`):
  - Das alte lineare Mapping `(intensity - 1) / 9` lieferte für Stufe 1 `speed = 0.0`, was in der Soft-Stop-Zone lag und den Lüfter stoppte.
  - Neu: `speed = 0.10 + ((intensity - 1) / 9) × 0.90` — Stufe 1 entspricht nun 10 % Drehzahl, Stufe 10 = 100 %.
- **Richtungssteuerung dynamisch** (`update_fan_logic()`): Richtung wird jetzt aus dem `fan_direction`-Switch gelesen (vorher hardcoded `direction = 0`).
- **Smart-Automatik als Standard-Startmodus**: Der intelligente Automatik-Modus (Modus-Index 3) ist jetzt der Standard-Startmodus beim ersten Start oder nach Factory Reset.
  - `current_mode_index` Default geändert: `0` (WRG) → `3` (Automatik).
  - `co2_auto_enabled` Default geändert: `false` → `true` (CO2-Regelung standardmäßig aktiv).
  - Alle Smart-Features (CO2-PID, Feuchte-PID, Radar-Anwesenheit, Sommer-Kühlung) sind damit out-of-the-box aktiv.
- **LED_WRG Puls-Effekt für Automatik-Modus**: Im Smart-Automatik Modus pulsiert `LED_WRG` (links) langsam (2s Zyklus, 15%-100%), um sich optisch vom manuellen Wärmerückgewinnung-Modus (Dauerleuchten) zu unterscheiden.
  - Neuer ESPHome Effekt `"Automatik Pulse"` auf `status_led_mode_wrg` in `ui_controls.yaml` definiert.
  - `update_leds_logic()` in `automation_helpers.h` prüft `auto_mode_active` und setzt den Effekt entsprechend.
- **Erweiterte Power-Button Logik (Haltedauer)**: Der physische Power-Button reagiert nun unterschiedlich basierend auf der Druckdauer (native ESPHome `on_click` Längenmessung):
  - *Kurzer Druck (0.05s - 4.9s)*: Schaltet das System EIN (falls es aus war).
  - *Langer Druck (5.0s - 9.9s)*: Schaltet das System AUS (Lüfter und PWM-Signale werden gestoppt).
  - *Sehr langer Druck (ab 10.0s)*: Schaltet das System AUS und löst einen Neustart (Reboot) des ESP32 aus.
- **30s LED Auto-Dimming**: Alle LEDs (Modus, Intensität, Power, Master) werden 30 Sekunden nach dem letzten Tastendruck sanft auf 0% gedimmt, um ein störendes Dauerleuchten zu vermeiden.
  - Das Drücken eines beliebigen Buttons am Bedienfeld weckt die UI auf und startet den Timer neu.
  - *Ausnahme:* Die Master-LED dimmt nicht herunter, wenn sie durch einen Fehlerzustand (wie Verbindungsverlust) blinkt.
- **HLK-LD2450 Radar-Anwesenheitserkennung**: Integration des mmWave-Radarsensors über UART (TX: GPIO16, RX: GPIO17, 256000 Baud).
  - Sensoren in Home Assistant: Radar Moving Target, Radar Presence, Radar Still Target, Radar Total Target Count.
  - `has_state()` Safety-Guard: System funktioniert fehlerfrei auch ohne angeschlossenen Radar-Sensor.
  - Bedarfsgesteuerte Lüftungsanpassung bei erkannter Anwesenheit mit 4 konfigurierbaren Profilen.
- **Radar Anwesenheits-Profile**: Dropdown `Anwesenheitsverhalten` in Home Assistant mit 4 Optionen:
  - *Keine Anpassung* (Default) — kein Einfluss auf Lüfterleistung.
  - *Intensiv (z. B. für Büro)* — Lüfterstufe +3 bei Anwesenheit.
  - *Normal (z. B. für Wohnraum)* — Lüfterstufe +1 bei Anwesenheit.
  - *Gering (z. B. für Schlafzimmer)* — Lüfterstufe -1 bei Anwesenheit.
- **Master LED Fehleranzeige**: `status_led_master` blinkt via Strobe-Effekt (500ms on/off) bei:
  - WiFi-Verbindungsverlust (via `esphome::wifi::global_wifi_component->is_connected()`).
  - Keine ESP-NOW Nachrichten von Peer-Geräten innerhalb von 5 Minuten (`last_peer_pid_demand_time`).
  - LED schaltet sich bei normalem Betrieb automatisch ab.
  - `check_master_led_error()` Inline-Funktion in `automation_helpers.h`, aufgerufen alle 5 Sekunden.
- **Prädiktiver Filterwechsel-Alarm**: Automatisches Wartungs-Management für rechtzeitige Filterwechsel.
  - Persistentes Tracking der Lüfter-Betriebsstunden (`filter_operating_hours`, `restore_value`) und Kalenderzeit seit letztem Wechsel (`filter_last_change_ts`).
  - Alarm-Bedingungen: Betriebsstunden > 8760h (365 Tage) ODER Kalenderzeit > 3 Jahre (94608000s).
  - `binary_sensor.filterwechsel_alarm` (device_class: problem) — wird `ON` bei Filterwechselbedarf.
  - `sensor.filter_betriebstage` — zeigt Laufzeit in Tagen seit letztem Filterwechsel.
  - `button.filter_gewechselt_reset` — setzt nach Filterwechsel alle Zähler zurück.
  - 60s Tracking-Intervall in `logic_automation.yaml` (zählt nur bei aktivem Lüfter).
  - HA Automation-Beispiel für Push-Benachrichtigungen in `Readme.md` dokumentiert.
- **OLED Diagnose-Display** (`display_diagnostics.yaml`): Integration eines SH1106 1.3" OLED (128x64, I²C 0x3C) mit 3 automatisch wechselnden Diagnoseseiten (alle 5s):
  - *System Info*: Aktueller Betriebsmodus und Lüfterstufe (1-10).
  - *Klima Daten*: Innen-/Außentemperatur (NTC) und CO2-Wert (ppm, SCD41) — mit `isnan`/`has_state()` Guards für fehlende Sensoren.
  - *Netzwerk*: IP-Adresse, WiFi-Signalstärke (dBm) und Uptime (Tage).
  - Google Fonts (Roboto) in 3 Größen (10/14/20px) für optimale Lesbarkeit.
- **Globale Konfigurations-Synchronisation via ESP-NOW**: Alle in Home Assistant (oder über das Panel) vorgenommenen Einstellungen (CO2 Automatik, Lüfter Min/Max-Level, CO2/Feuchte/Präsenz-Grenzwerte, Timer, Zykluszeiten) werden nun automatisch und nahtlos über das ESP-NOW Netzwerk an die gesamte Lüftungsgruppe übertragen.
  - Verhindert asynchrones Verhalten von Geräten im selben Raum. Wird ein Timer oder ein Grenzwert auf Gerät A geändert, übernimmt Gerät B diesen sofort.
  - Sende-Optimierung: Konfigurationen werden nur bei tatsächlicher Änderung übermittelt.
- **Stufenlose PID-Regelung (Lautlos)**: Der Lüfter wird nun über interne PID-Regler (`pid_co2` / `pid_humidity`) komplett stufenlos und extrem präzise (`0.0 - 1.0` Duty Cycle) gesteuert.
  - Verhindert jegliche hörbaren Drehzahlsprünge beim Hoch-/Runterregeln (Ablösung der alten starren 10-Stufen-Logik).
  - CO2 und Humbidity Limits nutzen Deadbands, um Mikro-Schwankungen zu ignorieren.
- **Globale PID-Synchronisation via ESP-NOW**: Der errechnete Leistungsbedarf (PID Demand) wird sekündlich über das kabellose ESP-NOW Netzwerk mit allen Geräten der Raumgruppe geteilt.
  - Verhindert, dass Lüfter im selben Raum "gegeneinander" kämpfen (z. B. ein Lüfter misst viel CO2 am Bett, der andere am offenen Flur wenig). Beide fahren nun absolut synchron und harmonisch auf identischer Maximalstufe hoch.
  - Hinweis: Durch die Erweiterung der `VentilationPacket`-Größe müssen alle Geräte der Lüftungsgruppe zeitgleich geflasht werden!
- **Adaptive CO2-Regelung**: Automatische Lüfteranpassung basierend auf SCD41 CO2-Werten (ppm).
  - 6-stufige Schwellwerte nach DIN EN 13779 / Umweltbundesamt (600/800/1000/1200/1400 ppm).
  - 100 ppm Hysterese gegen Pendelverhalten.
  - Konfigurierbarer Min-Level (Feuchteschutz / DIN 1946-6) — Standard: 2.
  - Konfigurierbarer Max-Level (Noise Control) — Standard: 7.
  - Nur aktiv wenn SCD41 angeschlossen (`isnan` Guard).
  - `VentilationLogic::get_co2_fan_level()` / `get_co2_classification()` (pure C++, unit-testbar).
  - `CO2 Automatik` Switch + `CO2 Min/Max Lüfterstufe` Slider + `CO2 Bewertung` Text-Sensor in Home Assistant.
  - 30s Regelintervall via `interval` Automation.
  - Dokumentation: `documentation/CO2-Automatik.md`.
  - **Unit Tests**: CO2-Logic Test Cases (`test_co2_logic`) hinzugefügt (Klassifikation, Schwellwerte, Hysterese, Min/Max-Clamping).
- **Sommer-Kühlfunktion (Querlüftung) via ESP-NOW**:
  - Erweiterung der "Smart-Automatik" um eine bedarfsgesteuerte passiv-kühlende Querlüftung an Sommermonaten.
  - Dezentrales Thermometer-Netzwerk: Die echten physikalischen Temperaturdaten der NTC-Sensoren (`temp_zuluft`/`temp_abluft`) werden ausgelesen, wenn die Lüfter im Querlüften einen konstanten Luftstrom haben.
  - Das `VentilationPacket` verteilt die präzisen Innen- und Außentemperaturen im Sekundentakt an die gesamte Lüftungsgruppe via ESP-NOW.
  - Automatisches Rückschalt-Sicherheitsnetz: Die Automatik erkennt gruppenweit sofort einen Temperaturanstieg der Außenluft und schaltet nahtlos alle Lüfter in den effizienten `Eco Recovery` (WRG) Modus zurück.
- **NTC Temperatursensoren (Analog)**:
  - Integration von NTC-Sensoren via ADC (GPIO4 am Slave / GPIO0/1 am Master).
  - Optimiertes Sampling: 1000ms Intervall mit Median- (Window 5) und Delta-Filter (0.2°C) für stabile Messwerte.
  - C++ `filter_ntc_stable` Logik: Pausiert nach jedem Richtungswechsel für 45s (thermal adaptation) und sendet Messwerte erst bei <= 0.3°C Schwankung über 30s Window an Home Assistant, um springende Werte zu verhindern.
  - Korrekte `UPSTREAM`/`DOWNSTREAM` Konfiguration je nach Hardware-Setup.
  - Deprecation Fix: `attenuation` von 11db auf 12db aktualisiert.
- **AI-Lüftungssteuerung (Konzept)**:
  - Initiales Konzeptdokument (`documentation/KI-gestützte-Lüftungssteuerung.md`) erstellt.
  - Ansätze für lokale Datenaufzeichnung, externe Wetterdaten und proaktive Regelung (Sommer-Hitzeschutz, Entfeuchtung).

### Changed

- **Entitäten Harmonisierung & Umbenennung**: Diverse Entitäten in YAML und C++ Code wurden für ein einheitliches Namensschema überarbeitet (z. B. Wechsel von `co2_min_fan_level` zu `automatik_min_luefterstufe`, `co2_sensor` zu `scd41_co2`, Modus-Auswahl zu `luefter_modus`). Das Dokument `Entities_Documentation.md` wurde hierfür als Quellreferenz verfasst.
- **Dynamische Zyklusdauer (WRG)**: Der Wechselintervall im Wärmerückgewinnungsmodus ist nicht mehr statisch, sondern wird nun im C++ Code (`automation_helpers.h`) dynamisch aus der Lüfterstufe abgeleitet. Stufe 1 läuft mit 70 Sekunden, Stufe 10 mit 50 Sekunden. Dies optimiert automatisch die thermische Effizienz des Keramikspeichers unter verschiedenen Lasten.
- **ESP-NOW Payload Optimerung**: Die Variable `cycle_duration_sec` wurde aus dem `VentilationPacket` der ESP-NOW Synchronisationslogik gestrichen. Die Master-Geräte übermitteln stattdessen nur die Basis-Parameter, der lokale C++ Code berechnet die Zyklusdauer nun selbst deterministisch, was die Paketgröße reduziert und Asynchronitäten vermeidet.
- **Radar Präsenz-Steuerung**: Die drei separaten Slider und Entitäten für "Intensiv", "Normal" und "Gering" wurden durch einen einzelnen übersichtlichen UI-Slider `Anwesenheit Lüfter-Anpassung` (-5 bis +5) in Home Assistant ersetzt, was die Ansicht stark vereinfacht.
- **Readme.md Restrukturierung**: Implementierte Features (CO2-Regelung, Radar-Anwesenheit, Wartungs-Management, Master LED Fehleranzeige) aus dem Roadmap-Bereich in eine eigene Sektion "✅ Implementierte Erweiterungen" verschoben. Roadmap enthält nun ausschließlich geplante Features.
  - Inhaltsverzeichnis erweitert.
  - Projektstruktur aktualisiert (u. a. `display_diagnostics.yaml` ergänzt).
  - Korrektur: "5 Lüfterstufen" → "10 Geschwindigkeitsstufen".
  - Typo-Fix: "Drehlzahlsprünge" → "Drehzahlsprünge".
  - HA Filterwechsel-Automation YAML-Beispiel für Push-Benachrichtigungen hinzugefügt.
  - **Feuchte-Management** als bereits implementiert erkannt und von Roadmap → Implementierte Erweiterungen verschoben: PID-Regler (`pid_humidity`) mit Deadband-Hysterese (±2%), konfigurierbarem Grenzwert (40-100%, HA Slider), Outdoor-Feuchte-Vergleich und ESP-NOW Synchronisation.
- **Projektstruktur & Modularisierung (ESPHome Best-Practices)**: Das gesamte Projekt wurde extrem verschlankt und nach offiziellen ESPHome Best-Practices in saubere Verzeichnisse restrukturiert:
  - `packages/`: Die fünf in sich geschlossenen Logikbausteine (`hardware_io.yaml`, `sensors_climate.yaml`, `ui_controls.yaml`, `logic_automation.yaml`, `display_diagnostics.yaml` sowie die Basis-Konfigs) wurden hierhin verschoben.
  - `components/`: Die `automation_helpers.h` befindet sich nun korrekt im Komponenten-Ordner neben den C++-Klassen (`ventilation_logic`, `ventilation_group`).
  - `experimental/`: Entwicklungs-Boards, Test-Knoten und Demo-Setups (`espslavetest.yaml`, `integration_test.yaml`, `espslaveNTC.yaml`) isoliert, um das Hauptverzeichnis sauber zu halten.
  - `assets/`: Ablageort für lokale Ressourcen (z. B. Fonts).
  - Keine "Mischung" von C++ und YAML im Root-Ordner mehr.
- **BOM (Bill of Materials) Update**: Die Komponentenlisten (CSV) wurden auf den neuesten Stand gebracht und mit aktuellen Daten von JLCPCB/LCSC (Preise, Warenbestände, erweiterte Bauteile wie z. B. PCA9685PW) abgeglichen und aktualisiert.
- **Auto-Modus Basis-Level**: Die Automatik (CO2/Feuchte) nutzt nun strikt den eingestellten Wert des "CO2 Min Lüfterstufe" Sliders aus Home Assistant als absolutes Grundrauschen (Moisture Protection), auf den sie bei fallenden Werten zurückfällt.
- **C++ Refactoring**: `evaluate_auto_mode()` und `update_fan_logic()` in `automation_helpers.h` komplett neu geschrieben, um float-basierte PID-Werte und Peer-Network-Demands (`last_peer_pid_demand`) zusammenzuführen, aufzulösen und auf das tatsächliche physische PWM-Signal sowie die 1-10 Indikator-LEDs zu mappen.
- **Hardware Pinout Update**: Pinbelegung für den Seeed Studio XIAO ESP32C6 an den neuesten Schaltplan (2026-02-22) angepasst:
  - Taster (Power, Mode, Level) werden nun über den **MCP23017** I/O Expander (`GPA0`-`GPA2`) ausgelesen, um direkte GPIOs am ESP32 freizumachen.
  - **PCA9685** LED-Mapping im Code korrigiert (z. B. `out_led_l1` auf `channel: 1`), um exakt der Schaltplan-Verdrahtung zu entsprechen. Alle ungenutzten Kanäle als `NC` dokumentiert.
  - UART-Pins (`D6`/`D7`) explizit für den **MR24HPC1** Radar-Sensor (RX/TX) vorgesehen.
  - Lüfter PWM und Tacho auf `D8` und `D9` verschoben.
- **Dokumentations-Update**: `Hardware-Setup-Readme.md` und `Readme.md` (inkl. Mermaid-Diagramm) umfassend überarbeitet, um die neuen Pin-Zuweisungen (sowohl ESP32 als auch I/O-Expander) fehlerfrei abzubilden. Alle alten Interrupt (INTB) Hinweise des MCP23017 entfernt.
- **BME680 Entfernung**: BME680-spezifischer IAQ- und BSEC-Code (`get_iaq_classification`, `get_iaq_traffic_light_data`) aus C++-Komponenten (`ventilation_logic`, `automation_helpers.h`) und Unit-Tests restlos entfernt, da der Sensor durch SCD41 / BMP390 ersetzt wurde.
- **SCD41 Konfiguration**: Zusätzliche Sensor-Parameter für SCD41 in `ventosync.yaml` ergänzt (`temperature_offset`, `altitude_compensation`, `automatic_self_calibration`).
- **Lüfterkompatibilität**: Vollständige Entfernung der obsoleten "3-Pin Dual-GND" (VarioPro) Lüftersteuerung aus dem ESPHome-Code und der Dokumentation. Das System unterstützt nun ausschließlich "4-Pin PWM" Lüfter (AxiRev) sowie "3-Pin PWM" Lüfter (ohne Tacho-Signal).
- **GPIO Pin-Mapping**: Korrektur der Hardware-Zuweisungen (physische D-Pins zu internen GPIO-Pins) für das Seeed Studio XIAO ESP32C6 Board in `ventosync.yaml` (betrifft I2C, Fan PWM, Tacho, NTCs).
- **Fan Logic Update**: C++ Helper (`automation_helpers.h`) vereinfacht durch Entfernung der Dual-GND Logik und alter Variablen (`fan_mode_select`, `fan_direction`).
- **Dokumentations-Update**: Umfangreiche Aktualisierung von `Readme.md`, `Hardware-Setup-Readme.md`, `Anleitung-Fan-Circuit.md` und `Technical-Details.md` passend zu den neuen Lüfter-Spezifikationen und Pin-Belegungen.
- **Refactoring**: komplette Fan-Logik (`set_fan_speed_and_direction`, `fan_speed_update`) aus YAML-Lambdas in C++ Helper-Funktionen (`set_fan_logic`, `update_fan_logic`) in `automation_helpers.h` ausgelagert.

### Verified

- **PCA9685 Support**: Bestätigt auf ESP32-C6 (Revision v0.0 / DevKitC-1) mit Framework `esp-idf` (v5.x).
  - `switch` und `light` (Monochromatic) Entities erfolgreich kompiliert und getestet.
- **Hardware PCA9685**:
  - `PCA9685PW` (TSSOP-28) vs `PCA9685BS` (HVQFN-28): Identischer Chip, nur Package-Unterschied.
  - **OE Pin**: Benötigt Pull-Up (4.7kΩ - 10kΩ) gegen Floating-States beim Boot (Active-Low). Ansteuerung via GPIO21 (als Switch inverted) möglich.
- **I2C Bus Hardware**:
  - **ESD Protection**: **UMW USBLC6-2SC6** (SOT-23-6) als Schutzdiode validiert (<1pF, optimiert für I2C/Data). Empfohlen: 1x pro Connector direkt am Eingang.
  - **Pull-Up Resistors**: **4.7kΩ** (0402, z. B. FRC0402F4701TS) als Standardwert bestätigt.
- **Cleanup**: `ventosync.yaml` bereinigt und Inline-Logik durch einfache Funktionsaufrufe ersetzt.
- **Readme**: Update mit Verweis auf KI-Lüftungskonzept.
- **Dokumentation**: NTC-Sensor-Spezifikationen in `Readme.md` tabellarisch dargestellt und Datenblatt-Link ergänzt.
- **ESPHome Build (März 2026)**: Erfolgreicher Kompiliertest aller neuen Features (Radar, Master LED, Filterwechsel-Alarm). RAM: 12.5%, Flash: 69.1%.

### Fixed

- **Start-Modus Bugfix (UI vs. Backend)**: Ein Bug wurde behoben, bei dem das System nach einem Neustart nicht im korrekten "Smart-Automatik" Modus gestartet ist. Obwohl das Backend den Modus "Automatik" vorbereitete, hat das Home-Assistant-Dropdown (`luefter_modus`) durch seine fehlerhafte `initial_option` ("Wärmerückgewinnung") beim Booten ein Event ausgelöst und die Automatik ungewollt überschrieben. Dies wurde in `ui_controls.yaml` korrigiert, sodass beide Systeme nun synchron auf "Automatik" starten.
- **BMP390 Konfiguration**: I2C-Adresse für BMP390 in `ventosync.yaml` fest auf `0x76` korrigiert und ungültige `bmp3xx_i2c` Top-Level-Konfiguration entfernt.
- **Kompilierung**: `RestoringGlobalsComponent` Typ-Konflikt in `automation_helpers.h` behoben (`co2_auto_enabled`, `co2_min/max_fan_level`).
- **Validierung**: `switch.template` Fehler bei `co2_auto_switch` korrigiert (redundante `component.update` Calls entfernt).
- **WiFi-Check auf ESP-IDF**: `WiFi.isConnected()` (Arduino-only) durch `esphome::wifi::global_wifi_component->is_connected()` ersetzt, um Kompatibilität mit ESP-IDF Framework sicherzustellen.
- **Display Typ-Fehler**: `current_option()` Rückgabetyp in `display_diagnostics.yaml` korrigiert (gibt `std::string` zurück, nicht `const char*`).

## [0.6.1] - 2026-03-21

### Added

- **Automatisierte Versionierung**: Implementierung eines `version_bump.py` Skripts, das bei jedem Compile die Patch-Version (0.6.x) automatisch erhöht und als C++ Makro (`PROJECT_VERSION`) zur Verfügung stellt.
- **Projekt-Version Sensor**: Neue Entität in Home Assistant zur Anzeige der aktuellen Firmware-Version.
- **Kontinuierliche Intensitäts-Anpassung**: Durch Gedrückthalten der Intensitäts-Taste werden die Stufen nun automatisch zyklisch durchlaufen (1→10→1) mit einer Geschwindigkeit von einer Stufe pro Sekunde.
- **Optimierter Automatik-Pulse**: Das Pulsieren der LED im Automatik-Modus wurde verlangsamt (3.5s Fade) und vertieft (bis auf 5% Helligkeit), um es deutlich vom statischen WRG-Modus unterscheidbar zu machen.
- **Globale LED-Helligkeitssteuerung**: Über einen neuen Schieberegler in Home Assistant kann die maximale Helligkeit aller LEDs (Status, Modus, Stufen) begrenzt werden (Standard: 80%).
- **Performance-Optimierungen**: Systemweite Entlastung der CPU durch reduziertes Log-Level (INFO), intelligente Delta-Filter für Sensoren (RPM/PWM) und optimierte Taktung der Dashboard-Prozesse.

### Changed

- **Echtzeit-Synchronisations-Trigger**: Das System sendet nun **sofort** einen ESP-NOW Broadcast, sobald ein Richtungswechsel (Zyklus-Ende) oder ein physischer Richtungs-Toggle eintritt. Dies garantiert, dass alle Geräte im Raum ohne Latenz synchron bleiben.

### Fixed

- **Home Assistant Langzeitstatistiken**: `state_class: measurement` zu den Sensoren für Drehzahl hinzugefügt, um die Datenaufzeichnung in der HA-Datenbank zu reparieren.

## [0.6.0] - 2026-03-20

### Changed

- **Echtzeit-Synchronisations-Trigger**: Das System sendet nun **sofort** einen ESP-NOW Broadcast, sobald ein Richtungswechsel (Zyklus-Ende) oder ein Phasenwechsel (Stoßlüftung) eintritt. Dies garantiert, dass alle Geräte im Raum exakt zeitgleich umschalten.
- **ESP-NOW Broadcast Refactoring**: Umstellung von `espnow.send` auf die native `espnow.broadcast` Action in der YAML-Konfiguration für saubereren Code und bessere Kompatibilität.
- **Unmittelbare Hardware-Reaktion**: Settermethoden für Phase (A/B) und Intensität triggern nun sofort einen Hardware-Refresh, anstatt auf den nächsten 1s-Loop zu warten.

### Fixed

- **Home Assistant Langzeitstatistiken**: `state_class: measurement` zu den Sensoren für Drehzahl, PWM-Ansteuerung und WRG-Effizienz hinzugefügt. Behebt die Warnung "Entität hat keine Zustandsklasse" und stellt Grafiken in HA wieder her.

## [0.5.0] - 2026-03-20

### Changed

- **WLAN-Stabilität (ESP32-C6)**: `power_save_mode` in `esp32c6_common.yaml` von `HIGH` auf `NONE` geändert. Behebt häufige Verbindungsabbrüche zur API und verbessert die Zuverlässigkeit von ESP-NOW.
- **Logger-Optimierung**: Standard-Log-Level auf `DEBUG` angehoben, um Synchronisationsvorgänge besser nachvollziehen zu können.

### Fixed

- **ESP-NOW Analyse**: Identifikation des "Interface does not match" Fehlers im Light-Sleep-Modus (WLAN-Deaktivierung im "AUS"-Zustand).

## [0.4.0] - 2026-02-15

### Added

- **Unit Tests**: Natives C++ Test-Framework (`tests/simple_test_runner.cpp`) mit 5 Testfällen — IAQ-Klassifikation, WRG-Effizienz, Fan-Logik, Stoßlüftungs-Zyklus, Phasen-Logik. Alle PASS.
- **Test-Infrastruktur**: `run_tests.bat` Build & Run Skript, `tests/README.md` Dokumentation.
- **Integration Test**: `integration_test.yaml` komplett neu geschrieben mit vollständigen Mock-Komponenten für alle `extern`-Deklarationen.
- **Doxygen-Kommentare**: `/// @brief` / `@param` / `@return` zu allen C++/H-Dateien hinzugefügt (`ventilation_state_machine.h/.cpp`, `ventilation_group.h`, `ventilation_logic.h/.cpp`, `automation_helpers.h`).
- **C22, C23** (BOM V2): 22 µF / 25 V MLCC (CL21A226MAQNNNE, LCSC C45783) als VIN-Eingangskondensatoren für AP63203 (U25) und AP63205 (U26) Buck-Converter ergänzt.

### Changed

- **L1** (BOM V2): Von ASPI-0804T-471M (470 µH / 500 mA) auf SLH1207S221MTT (220 µH / 1.16 A / Isat 2.36 A, LCSC C182174) aufgerüstet — geschirmte SMD-Spule, verkraftet 920 mA Anlaufstrom des VarioPro bei Richtungswechsel alle 70 s.
- Schematic Review (`schematic_review_2026-02-15.md`): Alle 3 Findings als behoben markiert.
- Dokumentation (`Hardware-Setup-Readme.md`, `Anleitung-Fan-Circuit.md`, `bom_cross_reference.md`) an BOM V2 angepasst.

### Fixed

- **`ventilation_logic` Component**: `ventilation_logic:` Include in YAML fehlte — Build schlug fehl.
- **`automation_helpers.h`**: Zugriffspfade korrigiert (`v->current_mode` → `v->state_machine.current_mode`, 3 Stellen; `v->ventilation_duration_ms` → `v->state_machine.ventilation_duration_ms`, 1 Stelle).
- [x] `ventilation_group.h` anpassen: Heartbeat-Zuständigkeit erweitern
- [x] `network_sync.h` anpassen: Erzwungene UI-Synchronisation bei MSG_STATE
- [x] Version auf 0.7.5 anheben
- [x] Verifikation durch Kompilierungt (`id(x)->value() = wert` → `id(x) = wert`, 5 Stellen).

### Removed

- Manueller Slider `number.zyklusdauer_eine_richtung` in Home Assistant entfernt, da diese nun vollautomatisch und lüfterdrehzahlabhängig berechnet wird.
- Fehlerhafte `current_mode()` Getter-Methode aus `ventilation_group.h`.

## [0.3.0] - 2026-02-14

### Changed

- **v0.8.31**: Synchronization and Auto-Mode Stabilization. Fixed sync loops and improved group authority logic.
- **v0.8.30**: Progressive quadratic RPM mapping for fan levels 1-10.
- **Pinout**: Pin-Mapping an finales Board-Layout angepasst (basierend auf Hardware Verification Report).
  - Buttons: Power (D6/GPIO16), Mode (D9/GPIO20), Level (D2/GPIO2).
  - Fan PWM: Primary (D8/GPIO19), Secondary (D10/GPIO18).
  - NTCs: ADC Swapped (Out=D0, In=D1).
- **Logger**: Umzug auf `USB_SERIAL_JTAG` (GPIO16/17 jetzt für Buttons/Tacho frei).
- **Q1 MOSFET**: DMP3098L (Vgs ±20V) ist jetzt Pflicht-Bauteil (ersetzt AO3401).
- `Anleitung-Fan-Circuit.md`: Q1 auf DMP3098L aktualisiert, Vgs Warnung entfernt.
- `Hardware-Setup-Readme.md`: Neue Pin-Tabelle (D0-D10) eingepflegt.
- `PCB-Best-Practices.md`: Neue Regel für Teardrops (Mechanische Stabilität), Tacho-Filter Hinweis ergänzt.

### Removed

- AO3401 Warnung (Bauteil durch DMP3098L ersetzt).
- R_GS Widerstand (nicht mehr nötig mit DMP3098L).

## [0.2.0] - 2026-02-10

### Added

- **Universal Fan Interface**: Board unterstützt 3-Pin oder 4-Pin PWM Lüfter.
- **Neue Komponenten**: Q4/Q5 (PMV16XNR), D1–D3 (B5819WS), R18/R19 (10kΩ), JP1 (VCC Moduswahl), JP2/JP3 (Pin-Moduswahl).
- GPIO2 als Fan PWM Secondary (Low-Side GND2 via Q4).
- `Anleitung-Fan-Circuit.md`, `Technical-Details.md`: Neue Dokumentation zum Hybrid Fan Interface.

### Fixed

- D1 Freilaufdiode an PWM_12V_OUT fehlte — ohne diese wird Q1 durch induktive Spannungsspitzen zerstört.
- R8: Dokumentation korrigiert (2.2kΩ / 0805, nicht 10kΩ / 0603).
- R3: Beschreibung korrigiert („für Q3" statt „für Q2").
- D1 (SS14) in Schaltplan → korrigiert zu D1 (B5819WS).
- Dioden-Designatoren: D1/D3 → D2/D3 im Low-Side Circuit.

### Changed

- `Hardware-Setup-Readme.md`: Universal Fan Interface komplett überarbeitet.
- `Readme.md`: Pin-Tabelle, Mermaid-Diagramm, Projektstruktur erweitert.

## [0.1.0] - 2026-01-28

### Added

- **APDS9960 Sensor Optimierung**: Update interval 100ms → 500ms, Proximity gain 4x → 2x, LED drive 100mA → 50mA, Delta-Filter (5), Throttle-Filter (500ms).
- **Adaptive Display-Helligkeit**: Automatische Anpassung basierend auf Umgebungslicht (>100: volle Helligkeit, ≤100: halbe Helligkeit).
- Konfigurierbarer Proximity-Threshold via Substitutions.
- `apds9960_config_optimized.yaml` — Optimierte Sensorkonfiguration.
- README.md um 372 Zeilen erweitert: Projektstruktur, Technische Details, Troubleshooting (15+ Einträge), 20+ Code-Beispiele.
- VS-Code YAML-Tag-Support (`!lambda`, `!secret`, `!include`) in `settings.json`.

### Fixed

- 5 Lambda-Syntax-Fehler („Unresolved tag: !lambda") in `ventosync.yaml` — ESPHome erfordert YAML Block-Scalar-Format (`|-`) statt Quoted Strings.

### Changed

- Logging auf DEBUG-Level reduziert (weniger Spam).

[0.4.0]: https://github.com/thomasengeroff-dotcom/VentoSync/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/thomasengeroff-dotcom/VentoSync/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/thomasengeroff-dotcom/VentoSync/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/thomasengeroff-dotcom/VentoSync/releases/tag/v0.1.0
