# 🏗️ Code-Architektur, Modularität & Wartbarkeit

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_code-architecture-and-maintainability.md)


Dieses Dokument bietet einen tiefen technischen Einblick in die mehrstufige Software-Architektur von VentoSync, die modulare YAML-Paketstruktur, das Design der nativen C++ Hilfsbibliotheken, Performance-Optimierungen und den System-Boot-Ablauf.

---

## 🏛️ Architektur-Übersicht

Die Firmware folgt einem **mehrstufigen modularen Architekturansatz**, der Wartbarkeit und Erweiterbarkeit maximiert:

#### **1. YAML Modularisierung (Packages)**

Die ehemals gewaltige Hauptdatei wurde drastisch verschlankt, um die Lesbarkeit und Pflege zu vereinfachen. Das Projekt nutzt intensiv die ESPHome `packages:` Funktion, um in sich geschlossene Logikbausteine in separate YAML-Dateien auszulagern. Seit Version 0.8.171 ist das `packages/`-Verzeichnis streng hierarchisch gegliedert:

- **`base/`**: Enthält die grundlegende ESP32-C6 Gerätekonfiguration.
- **`io/`**: Kapselt die physische Hardware. Beinhaltet I2C-Busse, Port-Expander, Basis-Pinbelegungen und die zentrale Lüfterkonfiguration.
- **`sensors/`**: Beinhaltet die gesamte Mess-Peripherie (SCD43, SGP41 + SHT45, Radar, NTCs, BMP390).
  - 🧩 **Sensor-Mocks**: Fehlt ein Sensor (z.B. SCD43), springen automatisch Mocks (`mock_scd41.yaml`) ein. Diese verhindern Compile-Fehler, unterdrücken Log-Spamming und blenden nicht vorhandene Sensoren dank `internal: true` nahtlos aus Home Assistant aus.
- **`actuators/`**: Das "Gehirn" der Anlage. Hier sitzen hochperformante Automatisierungen, PID-Klimaregler und die sicherheitskritische thermische Abschaltung (`logic_safety.yaml`).
- **`integration/`**: Isoliert alle externen Home Assistant Datenpunkte (`homeassistant.yaml`), um das System autark lauffähig zu halten.
- **`ui/`**: Enthält Web GUI, Diagnose-Entitäten und Status-LEDs.

Die Hauptdateien (`ventosync.yaml` etc.) fungieren nun lediglich als schlanke "Wrapper", die die `packages/base/ventosync_base.yaml` importieren und je nach Variante spezifische Sensoren oder Mocks dazuladen.

#### **2. `automation_helpers.h` - Zentrale Helper-Bibliothek**

Alle komplexen Lambda-Funktionen wurden aus dem YAML Code verbannt und in wiederverwendbare native C++ Helper-Funktionen ausgelagert:

**Vorteile:**

- ✅ **Bessere Lesbarkeit**: YAML bleibt übersichtlich, Logik ist in C++ dokumentiert
- ✅ **Wiederverwendbarkeit**: Funktionen können an mehreren Stellen genutzt werden
- ✅ **Typsicherheit**: Compiler-Checks zur Compile-Zeit statt Runtime-Fehler
- ✅ **IDE-Support**: Syntax-Highlighting, Auto-Completion und Refactoring-Tools
- ✅ **Einfachere Wartung**: Änderungen an einem Ort statt in mehreren YAML-Lambdas

**Enthaltene Funktionen:**

- `handle_espnow_receive()` - ESP-NOW Paket-Verarbeitung und State-Synchronisation
- `handle_button_*_click()` - Taster-Event-Handler (Power, Mode, Level)
- `set_*_handler()` - UI-Element Callbacks (Timer, Cycle Duration, Fan Intensity)
- `update_leds_logic()` - LED-Status-Aktualisierung basierend auf System-State
- `cycle_operating_mode()` - Betriebsmodus-Wechsel-Logik
- `calculate_heat_recovery_efficiency()` - Wärmerückgewinnungs-Berechnung

**Beispiel:**

```yaml
# Vorher: Komplexe Lambda direkt im YAML
binary_sensor:
  - platform: gpio
    on_press:
      - lambda: |-
          id(current_mode_index) = (id(current_mode_index) + 1) % 5;
          cycle_operating_mode(id(current_mode_index));
          id(update_leds).execute();

# Nachher: Sauberer Aufruf der Helper-Funktion
binary_sensor:
  - platform: gpio
    on_press:
      - lambda: handle_button_mode_click();

```

#### **3. 🚀 Performance & Technische Exzellenz**

Um eine 24/7-Zuverlässigkeit und Premium-Performance auf dem ESP32-C6 zu gewährleisten, implementiert die Firmware mehrere High-End C++ und Architektur-Optimierungen:

- **C++ Pro Performance & Thread Safety**:
  - ✅ **Thread Safety**: Ablösung von manuellen LwIP Semaphoren durch C++ Standard-Library `<mutex>` und `std::lock_guard` für 100% Exception-sicheres HTTP-Event Queueing.
  - ✅ **Memory Management**: Nutzung von Move Semantics (`std::move`) und strikte Const-Correctness zur Minimierung von RAM-Fragmentierung und CPU-Overhead.
  - ✅ **DRY Architecture**: Dedizierte, anonyme Lambda Helper-Funktionen für Web-JSON Building zur Eliminierung redundanter Logik.
  - ✅ **Footprint Reduction**: Optimierter RAM-Verbrauch durch Entfernung veralteter Web-UI Cache-Konzepte.

- **🛡️ Systemstabilität & Zuverlässigkeit**:
  - ✅ **NaN-Sichere PID-Steuerung**: Härtung der Bedarfsberechnung gegen ungültige Sensordaten. Das System hält den letzten gültigen Status bei Sensorausfällen und verhindert so unkontrolliertes Schalten.
  - ✅ **Einheitliche Steuerungsautorität**: Zentralisierung der Intensitätsberechnung (`evaluate_auto_mode`), um Race-Conditions zwischen unabhängigen Update-Intervallen zu eliminieren.
  - ✅ **Smart Group Sync**: Automatische Übertragung von Modi und Konfigurationen an alle Geräte im Raum via ESP-NOW mit integrierter Loop-Prevention.
  - ✅ **Konfigurations-Sicherheit**: Validierung von Min/Max-Lüfterstufen (Swap-Guard) zur Vermeidung von invertierter Skalierung bei Fehlkonfigurationen.
  - ✅ **NVS-Verschleißschutz**: Schreibzugriffe auf den internen Flash-Speicher werden minimiert, indem nicht-kritische Daten wie die Filter-Betriebsstunden gepuffert und nur alle 8 Stunden (3x pro Tag) festgeschrieben werden.
  - ✅ **LED-Selbsttest**: Beim Systemstart führt das Gerät einen 3-sekündigen Hardware-Check durch, bei dem alle LEDs auf 100% Helligkeit gezwungen werden, um die visuelle Rückmeldung zu verifizieren.
  - ✅ **Kombinierter NTC-Filter**: Ersetzung fragmentierter YAML-Lambdas durch einen zentralen C++ Filter (`filter_ntc_combined`). Dies vereint Phase-Lock, thermische Wartezeit und saisonale Selektion in einer kohärenten Pipeline für 100% Datenintegrität.
  - ✅ **Robustes Failsafe**: Implementierung konfigurierbarer Plausibilitätsbereiche und eines erweiterten 120s-Timeouts zur Vermeidung von "Unavailable"-Zuständen in Home Assistant während langer Lüftungsphasen.

- **Vollständiger Boot-Flow nach allen Fixes**:

  ```text
  Boot (t=0)
    │
    ├─ on_boot (priority -10)
    │   ├─ delay 2s
    │   ├─ sync_config_to_controller()
    │   ├─ cycle_operating_mode()
    │   ├─ load_peers_from_runtime_cache()  ← NVS laden
    │   ├─ delay 1s
    │   ├─ send_discovery_broadcast()       ← Peers suchen
    │   ├─ delay 3s
    │   └─ request_peer_status()            ← State sync
    │
    ├─ interval 60s (wiederholt)
    │   └─ if peer_cache.empty() → send_discovery_broadcast()
    │
    └─ Normalbetrieb
  ```

- **Protocol v4 & Stability (März 2026)**:
  - ✅ **ESP-NOW v4 Upgrade**: Einführung von Magic Header (`0x42`) und Protokoll-Versionierung zur Vermeidung von Inkompatibilitäten.
  - ✅ **Echtzeit-Settings-Sync**: Vollständiges Mirroring aller Benutzer-Konfigurationen (CO2-Grenzwerte, Fan-Levels, Timer) via Unicast.
  - ✅ **Millis-Refactoring**: 64-Bit Arithmetik zur Vermeidung des 49-Tage Rollover Bugs in der `VentilationStateMachine`.
  - ✅ **NTC-Performance**: Optimierung der Filter-Wartezeit (40% des Zyklus) für schnellere Wertlieferung bei gleicher Stabilität.
  - ✅ **Sommer-Kühlung**: Präzisierung der Hysterese-Regelung (+1.5°C Aktivierung / -0.5°C Deaktivierung).
  - ✅ **Modularisierung & Internationalisierung**: Saubere Trennung von C++ Kern und YAML-Zuschnitt (sensor-spezifische Pakete) zur Behebung von Linker-Errors und Verbesserung der Kompilierbarkeit. Umstellung sämtlicher Code-Kommentare auf Englisch zur internationalen Wartbarkeit.
