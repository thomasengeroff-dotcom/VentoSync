# 🏛️ VentoSync Architektur-Review

[![Language: EN](https://img.shields.io/badge/Language-EN-blue.svg)](../en/en_architecture-review.md)

**Datum:** April 2026  
**Zielprojekt:** VentoSync HRV (ESPHome)

## 1. Management Summary
Das VentoSync-Projekt zeichnet sich durch eine hochgradig modulare, professionelle ESPHome-Architektur aus. Es nutzt fortschrittliche ESPHome-Funktionen wie lokale `external_components`, umfassendes YAML-Packaging und strukturierte C++ Header-Includes. Das Projekt geht weit über typische Smart-Home-Konfigurationen hinaus, indem es komplexe PID-Regelungen, maßgeschneiderte ESP-NOW Kommunikationsprotokolle und eine saubere Hardware-Abstraktion implementiert.

## 2. Einhaltung der ESPHome Best Practices

### ✅ Modulare Konfiguration (`packages`)
**Status: Exzellent**  
Das Projekt nutzt das `packages:`-Feature von ESPHome vorbildlich (z.B. ruft `ventosync.yaml` die Datei `packages/base/ventosync_base.yaml` auf, welche wiederum dedizierte Pakete wie `packages/io/hardware_fan.yaml`, `packages/sensors/sensor_SCD41.yaml` etc. einbindet).

### ✅ Maßgeschneiderte C++ Integration (`external_components` & `includes`)
**Status: Exzellent**  
Die Integration individueller C++ Logik ist optimal gelöst:
* **External Components**: Lokale `external_components` mit `source: type: local` verweisen sauber auf das `components`-Verzeichnis. Dies ist der von ESPHome empfohlene Weg, um Kernkomponenten zu erweitern oder tief integrierte Logik (wie die `ventilation_group`) hinzuzufügen.
* **Header-Trennung**: Die C++ Hilfsfunktionen sind sauber in logische Domänen aufgeteilt (`globals.h`, `user_input.h`, `network_sync.h`, `led_feedback.h`) und werden über `esphome: includes:` eingebunden. Dies verhindert, dass `lambda:`-Abschnitte im YAML überladen und schwer wartbar werden.

### ✅ Nicht-blockierende Ausführung
**Status: Sehr gut (Kürzlich optimiert)**  
ESPHome basiert auf einer asynchronen Ereignisschleife; blockierende Aufrufe (wie standardmäßiges C++ `delay()`) stoppen das gesamte System und verhindern Sensoraktualisierungen, WLAN-Verarbeitung und I2C-Kommunikation. Das Refactoring der `flash_all_leds()`-Logik von blockierendem C++ `delay()` zu nicht-blockierenden YAML-Skripten mit ESPHomes nativem `delay:` zeigt ein tiefes Verständnis des Nebenläufigkeitsmodells. Netzwerkoperationen (ESP-NOW) und Hardware-Rendering (PCA9685) laufen flüssig ohne Einfrieren der Hauptschleife.

### ✅ Zustandspersistenz (`globals`)
**Status: Exzellent**  
Das Projekt nutzt ESPHomes `RestoringGlobalsComponent` (via `restore_value: true` im NVS) für kritische Statusvariablen wie `child_lock_active`, `current_mode_index` und `filter_operating_hours`. Dadurch behält das Gerät seinen Zustand nach Stromausfällen oder OTA-Updates zuverlässig bei. Der gestufte Speicheransatz für hochfrequente Variablen (wie Filterbetriebsstunden) schont das Flash-Speicher-Wear-Leveling.

## 3. Architektonische Stärken

* **ESP-NOW Unicast-Effizienz**: Der Verzicht auf Standard-WLAN für die lokale Cluster-Synchronisation zugunsten von ESP-NOW ist eine hervorragende Designentscheidung. Dynamisches Discovery per Broadcast gefolgt von gezieltem Unicast garantiert minimale Latenz bei der Phasensynchronisation und Ausfallsicherheit bei Router-Problemen.
* **Hardware Abstraction Layer (HAL)**: Die Verwendung globaler Zeiger in `globals.h` (`extern esphome::globals::...`) fungiert als Brücke zwischen YAML-Entitäten und C++ Logik. Dies schafft eine saubere Schnittstelle, sodass UI- und Sensor-YAMLs angepasst werden können, ohne C++ Code umschreiben zu müssen.
* **Graceful Degradation / Hardwarevarianten**: Die Bereitstellung von `ventosync_nosensor.yaml` und Mock-Konfigurationen ermöglicht den fehlerfreien Betrieb und das Kompilieren auch dann, wenn optionale Sensoren (wie SCD43) nicht verbaut sind.

## 4. Empfehlungen & Optimierungspotenziale

* **Header Guards (`#pragma once`)**: Sicherstellen, dass alle benutzerdefinierten `.h`-Dateien unter `components/helpers/` ein `#pragma once` am Dateianfang besitzen, um Mehrfachinkludierungen zu verhindern.
* **Mutex-Kontexte**: Die ESP-NOW Implementierung verwendet `std::mutex` für die Empfangs- und Event-Queues. Da ESPHome kooperatives Multitasking nutzt, treten echte Threads primär bei FreeRTOS ISR/Interrupt-Kontexten auf. Das Queuing von Events (`peer_event_queue`) zur Abarbeitung in der Hauptschleife verhindert Race-Conditions und unzulässige Flash-Schreibvorgänge in Callback-Kontexten.
* **Lokales Hosting von Tailwind / Chart.js**: Das Web-Dashboard nutzt aktuell CDNs für Tailwind CSS und Chart.js. Dies minimiert die Firmwaregröße, erfordert jedoch Internetzugriff beim Laden der Oberfläche. Ein vollständig offlinefähiges Web-Dashboard-Asset-Paket wäre ein möglicher nächster Schritt.

## 5. Fazit
VentoSync ist ein Paradebeispiel dafür, wie ESPHome von einfachen Sensorknoten zu komplexen, verteilten Embedded-Systemen skaliert werden kann. Die Architektur ist robust, hochgradig modular und entspricht den Best Practices für große ESPHome-Codebasen.
