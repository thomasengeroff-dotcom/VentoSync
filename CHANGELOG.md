# Changelog

Alle erheblichen Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.1.0/).

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
- **Boot-Modus Glitch-Fix**: Korrektur der Rampen-Initialisierung beim Systemstart. `last_committed_mode` startet nun im Standard-Automatik-Zustand (`MODE_ECO_RECOVERY`), was den ersten Rampen-Schritt sauber auf 1 begrenzt.

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
  - Erweiterung der "Standard-Automatik" um eine bedarfsgesteuerte passiv-kühlende Querlüftung an Sommermonaten.
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
