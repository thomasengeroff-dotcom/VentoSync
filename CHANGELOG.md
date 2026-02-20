# Changelog

Alle erheblichen Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.1.0/).

## [Unreleased]

### Added

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
- **NTC Temperatursensoren (Analog)**:
  - Integration von NTC-Sensoren via ADC (GPIO4 am Slave / GPIO0/1 am Master).
  - Optimiertes Sampling: 1000ms Intervall mit Median- (Window 5) und Delta-Filter (0.2°C) für stabile Messwerte.
  - Korrekte `UPSTREAM`/`DOWNSTREAM` Konfiguration je nach Hardware-Setup.
  - Deprecation Fix: `attenuation` von 11db auf 12db aktualisiert.
- **AI-Lüftungssteuerung (Konzept)**:
  - Initiales Konzeptdokument (`documentation/KI-gestützte-Lüftungssteuerung.md`) erstellt.
  - Ansätze für lokale Datenaufzeichnung, externe Wetterdaten und proaktive Regelung (Sommer-Hitzeschutz, Entfeuchtung).

### Changed

- **Lüfterkompatibilität**: Vollständige Entfernung der obsoleten "3-Pin Dual-GND" (VarioPro) Lüftersteuerung aus dem ESPHome-Code und der Dokumentation. Das System unterstützt nun ausschließlich "4-Pin PWM" Lüfter (AxiRev) sowie "3-Pin PWM" Lüfter (ohne Tacho-Signal).
- **GPIO Pin-Mapping**: Korrektur der Hardware-Zuweisungen (physische D-Pins zu internen GPIO-Pins) für das Seeed Studio XIAO ESP32C6 Board in `esp_wohnraumlueftung.yaml` (betrifft I2C, Fan PWM, Tacho, NTCs).
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
  - **Pull-Up Resistors**: **4.7kΩ** (0402, z.B. FRC0402F4701TS) als Standardwert bestätigt.
- **Cleanup**: `esp_wohnraumlueftung.yaml` bereinigt und Inline-Logik durch einfache Funktionsaufrufe ersetzt.
- **Readme**: Update mit Verweis auf KI-Lüftungskonzept.
- **Dokumentation**: NTC-Sensor-Spezifikationen in `Readme.md` tabellarisch dargestellt und Datenblatt-Link ergänzt.

### Fixed

- **Kompilierung**: `RestoringGlobalsComponent` Typ-Konflikt in `automation_helpers.h` behoben (`co2_auto_enabled`, `co2_min/max_fan_level`).
- **Validierung**: `switch.template` Fehler bei `co2_auto_switch` korrigiert (redundante `component.update` Calls entfernt).

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
- **`ventilation_group.h`**: Fehlerhafte `current_mode()` Getter-Methode entfernt (API-Konflikt).
- **Integration Test**: Globals-Zugriff korrigiert (`id(x)->value() = wert` → `id(x) = wert`, 5 Stellen).

### Removed

- Fehlerhafte `current_mode()` Getter-Methode aus `ventilation_group.h`.

## [0.3.0] - 2026-02-14

### Changed

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

- 5 Lambda-Syntax-Fehler („Unresolved tag: !lambda") in `esp_wohnraumlueftung.yaml` — ESPHome erfordert YAML Block-Scalar-Format (`|-`) statt Quoted Strings.

### Changed

- Logging auf DEBUG-Level reduziert (weniger Spam).

[Unreleased]: https://github.com/thomasengeroff-dotcom/ESPHome-Wohnraumlueftung/compare/v0.4.0...HEAD
[0.4.0]: https://github.com/thomasengeroff-dotcom/ESPHome-Wohnraumlueftung/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/thomasengeroff-dotcom/ESPHome-Wohnraumlueftung/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/thomasengeroff-dotcom/ESPHome-Wohnraumlueftung/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/thomasengeroff-dotcom/ESPHome-Wohnraumlueftung/releases/tag/v0.1.0
