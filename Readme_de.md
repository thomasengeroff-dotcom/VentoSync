# 🌬️ VentoSync — Intelligente WRG-Wohnraumlüftungssteuerung auf Basis von ESPHome für VentoMaxx V-WRG Serie (ESP32-C6)

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](Readme.md)

## ⚖️ Disclaimer

> ⚠️ **VentoSync ist ein unabhängiges Community-Projekt und steht in keiner Verbindung zur Ventomaxx GmbH.**

## 🚀 Zusammenfassung & Überblick

Dieses Open-Source-Projekt bietet eine professionelle, dezentrale Lüftungssteuerung basierend auf ESPHome. Es ersetzt die Steuerung der VentoMaxx V-WRG Serie mittels einer eigens dafür entwickelten Platine (PCB) und steuert damit den reversierbaren 12V Lüfter zur Wärmerückgewinnung, überwacht optional die Luftqualität (CO2, Feuchte und Temperatur) mittels eines hochwertigen Sensirion SCD43 Sensors, berechnet die effektive Wärmerückgewinnung und nutzt das **originale VentoMaxx Bedienpanel** für eine nahtlose Integration, intuitive Steuerung. Darüber hinaus kann optional ein Radar-Sensor zur Anwesenheitserkennung integriert werden, der unsichtbar hinter der Blende des Lüftungsgerätes montiert werden kann.
Die Kommunikation zwischen den einzelnen Lüftungsgeräten erfolgt über das ESP-NOW Protokoll, sodass kein WLAN oder eine zentrale Steuereinheit erforderlich sind (die Kommunikation über die Stromleitungen, welche Ventomaxx nutzt, wird nicht verwendet).

<p align="center">
  <img src="EasyEDA-Pro/PCB%20mounting/Ventomaxx-WRG-mit-VentoSync-PCB.jpg" width="48%" alt="VentoSync Platine im VentoMaxx V-WRG Gehäuse" />
  <img src="EasyEDA-Pro/PCB%20mounting/Ventomaxx-WRG-mit-VentoSync-PCB_Radarsensor.jpg" width="48%" alt="VentoSync Platine mit Radar-Sensor im VentoMaxx V-WRG Gehäuse" />
</p>
<p align="center">
  <em><strong>Drop-in Hardware-Upgrade:</strong> Maßgefertigte VentoSync ESP32-C6 Platine montiert im originalen VentoMaxx V-WRG Lüftergehäuse — Standard-Installation mit externer Antenne (links) und mit optional integriertem mmWave-Radar-Sensor zur unsichtbaren Raumpräsenzerkennung (rechts).</em>
</p>

> 💡 **Kompatibilität:** Die Steuerung funktioniert prinzipiell für jede dezentrale Wohnraumlüftung mit einem reversierbaren 12V Lüfter (3-PIN oder 4-PIN PWM). Sie wurde jedoch **speziell als Ersatz für die VentoMaxx V-WRG Serie** entwickelt. Die Hardware (PCB-Layout/Größe und Bedienpanel) ist damit explizit für die VentoMaxx V-WRG Serie optimiert und muss für andere Hersteller ggf. angepasst werden. Das PCB ist so konzipiert, dass es exakt in das Gehäuse der VentoMaxx V-WRG Serie passt und die vorhandenen Befestigungspunkte nutzt.
Achtung: Diese Lösung ist nicht kompatibel mit der VentoMaxx ZR-WRG Serie, da diese eine zentrale Steuereinheit nutzt!

[![Build Status](https://github.com/thomasengeroff-dotcom/VentoSync/actions/workflows/build.yaml/badge.svg)](https://github.com/thomasengeroff-dotcom/VentoSync/actions/workflows/build.yaml)
[![GitHub Release](https://img.shields.io/github/v/release/thomasengeroff-dotcom/VentoSync?color=blue&logo=github)](https://github.com/thomasengeroff-dotcom/VentoSync/releases)
[![ESPHome](https://img.shields.io/badge/ESPHome-Compatible-blue?logo=esphome)](https://esphome.io/)
[![Home Assistant](https://img.shields.io/badge/Home%20Assistant-Integration-green?logo=home-assistant)](https://www.home-assistant.io/)
[![MQTT](https://img.shields.io/badge/MQTT-Optional-blue?logo=mqtt&logoColor=white)](documentation/en/en_mqtt-integration.md)
[![Platform](https://img.shields.io/badge/Platform-ESP32--C6-red?logo=espressif)](https://esphome.io/components/esp32.html)
![Sensor: SCD43](https://img.shields.io/badge/Sensor-SCD43-lightgrey)
![Sensor: BMP390](https://img.shields.io/badge/Sensor-BMP390-lightgrey)
![Sensor: BME680](https://img.shields.io/badge/Sensor-BME680-lightgrey)
![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)

---

## 📑 Inhaltsverzeichnis

- [⚖️ Disclaimer](#⚖️-disclaimer)
- [🚀 Zusammenfassung & Überblick](#🚀-zusammenfassung--überblick)
  - [Motivation](#motivation)
  - [🛠️ Maßgefertigte Leiterplatte (PCB)](#🛠️-maßgefertigte-leiterplatte-pcb)
  - [🔄 Vergleich mit VentoMaxx V-WRG](#🔄-vergleich-mit-ventomaxx-v-wrg)
- [✨ Leistungsmerkmale](#✨-leistungsmerkmale)
  - [⚙️ Intelligente Betriebsmodi](#⚙️-intelligente-betriebsmodi)
  - [🛡️ Präzisions-Sensorik & Monitoring](#🛡️-präzisions-sensorik--monitoring)
  - [⚡ Extrem niedriger Stromverbrauch](#⚡-extrem-niedriger-stromverbrauch)
  - [🖥️ Native Bedienung am Gerät](#️-native-bedienung-am-gerät)
  - [🏠 Home Assistant Integration](#🏠-home-assistant-integration)
  - [📊 VentoSync Dashboard - Lokales Web-Dashboard](#📊-ventosync-dashboard---lokales-web-dashboard)
- [📡 ESP-NOW: Kabellose Autonomie](#📡-esp-now-kabellose-autonomie)
- [🗺️ Roadmap & Zukünftige Erweiterungen](#🗺️-roadmap--zukünftige-erweiterungen)
- [🎛️ Eigene Platine - PCB](#️-eigene-platine---pcb)
  - [Passgenaues SCD43 Sensor Board](#passgenaues-scd43-sensor-board)
  - [🌿 Modulares Sensor-Ökosystem: SGP41 & SHT4x](#-modulares-sensor-ökosystem-die-passende-messgröße-pro-raum)
- [🛠️ Einrichtung & Installation](#🛠️-einrichtung--installation)
  - [🧰 PCB-Montage & Lüfter-Verdrahtung](#-pcb-montage--lüfter-verdrahtung)
  - [💻 Entwicklungsumgebung (Linux `venv` & ESPHome-CLI)](#-entwicklungsumgebung-linux-venv--esphome-cli)
  - [⚙️ Konfiguration & Kompilierung](#️-konfiguration--kompilierung)
  - [⚡ Erstmaliges Flashen & Inbetriebnahme](#-erstmaliges-flashen--inbetriebnahme)
  - [🔄 OTA-Updates & Home Assistant Integration](#-ota-updates--home-assistant-integration)
  - [🌡️ Kalibrierung der NTC-Sensoren](#️-kalibrierung-der-ntc-sensoren)
- [🎮 Bedienung & Steuerung](#🎮-bedienung--steuerung)
  - [🖐️ Bedienpanel (VentoMaxx Style)](#🖐️-bedienpanel-ventomaxx-style)
  - [🔄 Detaillierte Betriebsmodi (Programme)](#🔄-detaillierte-betriebsmodi-programme)
  - [📱 Steuerung über Home Assistant](#📱-steuerung-über-home-assistant)
- [🧠 Wärmerückgewinnung - So funktioniert's](#🧠-wärmerückgewinnung---so-funktionierts)
- [🔧 Technische Details & Optimierungen](#🔧-technische-details--optimierungen)
- [📁 Projektstruktur](#📁-projektstruktur)
- [🏗️ Code-Architektur & Wartbarkeit](#🏗️-code-architektur--wartbarkeit)
- [🚀 Automatisierte Release & Versionierung](#🚀-automatisierte-release--versionierung)
- [🙏 Danksagungen / Credits](#-danksagungen--credits)
- [⚠️ Sicherheitshinweise](#⚠️-sicherheitshinweise)
- [⚖️ Rechtlicher Haftungsausschluss](#⚖️-rechtlicher-haftungsausschluss)
- [📜 Lizenz](#📜-lizenz)

---

## Motivation

Ich habe vor vielen Jahren im Rahmen der Haussanierung die dezentrale Wohnraumlüftung V-WRG von Ventomaxx installiert (10 Geräte) und war damit auch sehr zufrieden. Allerdings hat mich die proprietäre Steuerung und die fehlende Integration in mein Smart Home System immer gestört. Daher habe ich mich entschlossen, eine eigene Platine (PCB) inkl. der Steuerungssoftware auf Basis von ESPHome zu entwickeln, da es keine fertige Lösung gab. Diese Lösung ist Open Source und soll anderen Nutzern helfen, die in der gleichen Situation wie ich sind.
Für die Steuerung der Lüftung auf Basis von CO2 nutze ich einen extrem hochwertigen und präzisen CO2-Sensor (Sensirion SCD43), der direkt in die Platine (per kleines Zusatz-PCB) integriert ist (Hinweis: Aktuell dient der BME680 als Fallback, da das SCD43-PCB noch in Fertigung ist). Dieser Sensor misst die echte CO2-Konzentration in der Luft und steuert die Lüftungsintensität entsprechend der Voreinstellungen (mittels einer modernen PID-Regelung). Sämtliche Code-Kommentare und die interne Dokumentation wurden zur besseren internationalen Wartbarkeit auf Englisch umgestellt, während das User-Interface weiterhin auf Deutsch bleibt.
Da die Lüftungsgeräte in den verschiedenen Räumen meistens eine sehr zentrale Position haben, nutze ich diese auch direkt zur Anwesenheitserkennung mittels Radar-Sensor, der unsichtbar hinter der Blende des Lüftungsgerätes versteckt montiert werden kann. Der Anwesenheitssensor wird für die Steuerung der Lüftungsintensität im Smart-Automatik Modus genutzt und kann darüber hinaus in Home Assistant für jegliche weitere Automatisierungen genutzt werden.
Der Funktionsumfang dieser Eigenentwicklung geht nach meinen Recherechen über alles hinaus, was aktuell am Markt der Lüftungsgeräte zu finden ist!

---

## 🛠️ Maßgefertigte Leiterplatte (PCB)

Das Herzstück des Projekts ist eine eigens entwickelte Platine, die exakt in das vorhandene Gehäuse der VentoMaxx-Geräte passt.

![Custom PCB](EasyEDA-Pro/PCB%20Prototype%20Images/pcb4.jpg)

> [!TIP]
> Wenn du Interesse an einer Platine für deine eigenen Geräte hast, kannst du mich gerne unter **<thomas@engeroff.net>** kontaktieren.
> Bitte beachte, dass ich noch nicht entschieden habe, ob ich die PCB-Produktionsdaten als Open Source zur Verfügung stellen werde.

![PCB in Gehäuse mit Antenne](EasyEDA-Pro/PCB%20mounting/PCB-FAN-ANT-in-Gehäuse.jpg)

> [!CAUTION]
> **LEBENSGEFAHR (230V Netzspannung):** Der Einbau des PCB in das Lüftungsgerät erfordert Arbeiten an der **230V Netzspannung**. Installation und elektrischer Anschluss dürfen **ausnahmslos nur von einer qualifizierten Elektrofachkraft** unter Beachtung der geltenden Sicherheitsvorschriften durchgeführt werden!

---

## 🔄 Vergleich mit VentoMaxx V-WRG

Diese Lösung ist ein **Drop-in Replacement** für die [VentoMaxx V-WRG / WRG PLUS](https://www.ventomaxx.de/dezentrale-lueftung-produktuebersicht/aktive-luefter-mit-waermerueckgewinnung/) Steuerung — mechanisch kompatibel, funktional massiv erweitert:

| | VentoMaxx (Original) | ESPHome Smart WRG |
| :--- | :---: | :---: |
| Betriebsmodi | 3 | **5+** (inkl. Automatiken) |
| Sensorik | 0-1 (opt. VOC) | **6** (CO2, Temp, Feuchte, Druck, Radar, Tacho) |
| Lüfterregelung | 3 feste Stufen | **10 Stufen (diskrete PID-Regelung)** |
| Smart Home | ❌ | ✅ Home Assistant (nativ) |
| Wartungsalarm | Timer-LED | ✅ Prädiktiv + Push |
| Synchronisation | Stromleitung | ✅ Kabellos (**ESP-NOW Protocol**) & Echtzeit-Sync |
| Updates | nur per Servicetechniker (muss eingeschickt werden) | ✅ Over-the-Air (OTA) |
| Versionierung | Manuell | ✅ Vollautomatisch (Patch-Level) |
| Erweiterbarkeit | ❌ | ✅ System kann mit zusätzlichen Sensoren und Aktoren oder individuellen Funktionen erweitert werden |
| Lizenz | Proprietär | ✅ Open Source (GPL v3) |

 **Den vollständigen Feature-für-Feature Vergleich mit allen technischen Details findest du in [📄 Comparison-VentoMaxx.md](documentation/de/de_ventomaxx-comparison.md).**

---

## ✨ Leistungsmerkmale

### ⚙️ Intelligente Betriebsmodi

Alle Geräte in einem Raum finden sich beim Start oder Raumwechsel vollautomatisch über eine **dynamische ESP-NOW Discovery** und kommunizieren anschließend effizient via Unicast.

- 🤖 **Smart-Automatik**: Vollautomatische Steuerung für maximalen Komfort und Effizienz. Standardbetrieb in Wärmerückgewinnung (Push-Pull) mit dynamischer PID-Regelung für CO2 und Luftfeuchtigkeit unter Einbezug aktueller Außenluftbedingungen. Im Sommer wird Querlüftung zur passiven nächtlichen Kühlung automatisch aktiviert, wenn es außen kühler ist als innen. Die CO2-Regelung wertet dabei den **höchsten CO2-Wert aller Geräte im Raum aus (Raumweite Sensor-Fusion)**. Dadurch regeln auch Geräte ohne eigenen CO2-Sensor ihre Lüftungsintensität korrekt hoch, wenn ein anderes Gerät im Raum hohe CO2-Werte erkennt. *→ [Vollständige Details und Zeitbeispiele in 📄 Betriebsmodi & Programmlogik](documentation/de/de_operating-modes.md)*
- 🔄 **Effiziente Wärmerückgewinnung**: Zyklischer, bidirektionaler Betrieb (Push-Pull) zur Maximierung der Energieeffizienz. Während die automatische CO2- und Feuchteregelung inaktiv ist, kann die Anwesenheitserkennung die Lüfterstufe bei Bedarf dynamisch anpassen.
- 💨 **Querlüftung (Sommerbetrieb)**: Konstanter Luftstrom ohne Richtungswechsel (Phase-A-Geräte saugen an, Phase-B-Geräte blasen gleichzeitig ab für einen spürbaren Durchzug zur passiven Nachtkühlung). Flexibel konfigurierbar via Timer oder als Dauerbetrieb.
- 🚀 **Stoßlüftung**: Intensivlüftung für schnellen Luftaustausch. Das Gerät lüftet für 15 Minuten mit der **manuell gewählten Intensität** und pausiert anschließend für 105 Minuten, um Feuchtigkeit effektiv abzuführen und den Keramikspeicher zu regenerieren. Danach wiederholt sich der Zyklus.
- 🌡️ **Aus (Monitoring-Modus)**: Der Lüfter wird gestoppt (0 RPM), aber alle Sensoren (CO2, Temp, Radar) und das Web-Dashboard bleiben für lückenlose Messdaten in Home Assistant aktiv. *(Hinweis: Der extrem stromsparende Light-Sleep mit deaktiviertem WLAN wird per langem Tastendruck >5s auf den Power-Button aktiviert).*

### 🛡️ Präzisions-Sensorik & Monitoring

- 🌡️ **Klimadatenerfassung**: Hochpräzise Messung von Temperatur und relativer Luftfeuchtigkeit mittels [Sensirion SCD43](https://sensirion.com/de/produkte/katalog/SCD43).
  - ✅ **Photoacoustic sensing** für präzise CO2-Messung (400-5000 ppm), Integrierte Temperatur- und Feuchtigkeitsmessung (SCD43), Dokumentation: `EasyEDA-Pro/components/SCD43-Sensirion.pdf`
  - ✅ **BME680 Advanced IAQ Engine**: Der BME680 nutzt nun eine dedizierte C++ Engine für robustes Baseline-Tracking, dynamische Temperaturkompensation und intelligentes Flash-Wear-Leveling. Dies liefert hochwertige VOC/IAQ-Daten ohne den Overhead der BSEC-Bibliothek.
  - ⚠️ **Hinweis:** Da das SCD43-PCB noch in Fertigung ist, dient der **BME680** aktuell als Fallback (IAQ-Index). Der Code erkennt automatisch, ob der SCD43 vorhanden ist.
  - 💨 **Echte CO2-Messung**: Der SCD43 nutzt **photoacoustic sensing** zur direkten CO2-Messung (400-5000 ppm) statt berechneter Äquivalente - ideal für bedarfsgerechte Lüftungssteuerung.
  - 🏔️ **Luftdruckmessung & Hardware-Schutz via BMP390**: Der hochpräzise Barometer-Sensor [Bosch BMP390](https://www.bosch-sensortec.com/en/products/environmental-sensors/pressure-sensors/pressure-sensors-bmp390.html) liefert nicht nur lokale Wetterdaten und barometrische Kompensation für den SCD43, sondern fungiert auch als **Sicherheitswächter für das Traco-Netzteil**:
    - **Automatisches Derating-Management**: Überwachung der Innentemperatur im Gehäuse des Lüftungsgerätes zur Einhaltung der Traco-Spezifikationen.
    - **Not-Abschaltung**: Bei kritischen Temperaturen (>60°C) startet ein Sicherheits-Protokoll (Lüfterstopp und 60min Deep Sleep), um die Hardware vor Überhitzung zu schützen und eine entsprechende Warnung an Home Assistant zu senden.

- **💨 Fortgeschrittene Luftqualitäts-Logik**:
  - **Enthalpie-Schutz / Absolute Feuchtigkeits-Sperre**: Um einen Feuchtigkeitseintrag von außen zuverlässig zu verhindern, arbeitet die feuchtegeführte Lüftung in **zwei aufeinander abgestimmten Stufen**:
    1. **Stufe 1 (Sollwert-Trigger)**: Der interne Feuchte-PID-Regler (`pid_humidity`) überwacht die Raumfeuchte gegen den konfigurierbaren Sollwert (Standard: 60 % rH via `auto_humidity_threshold`). Liegt die Raumfeuchte unter diesem Schwellwert, beträgt der Bedarf `0,0` (0 %) — das System lüftet also **nicht** grundlos, nur weil es draußen trocken ist.
    2. **Stufe 2 (Enthalpie-Schutz / Veto-Filter)**: Steigt die Raumfeuchte über den Sollwert und der PID-Regler fordert Lüftung an, prüft das System anhand der **absoluten Luftfeuchtigkeit** in g/m³ ([Magnus-Formel](https://de.wikipedia.org/wiki/Clausius-Clapeyron-Gleichung)), ob die Außenluft tatsächlich trockener ist. Ist die Außenluft feuchter (z. B. an schwülen Sommertagen oder bei Regen), wird der Feuchte-Bedarf auf **null** erzwungen — kein Feuchtigkeitseintrag.

    | Szenario | Innen | Außen | Absolute Feuchtigkeit | Ergebnis |
    | --- | --- | --- | --- | --- |
    | ☀️ **Normaler Sommertag** | 23°C / 55% rH | 20°C / 45% rH | Innen: 11,3 g/m³ **>** Außen: 7,8 g/m³ | ✅ Lüften hilft → Feuchtigkeits-Demand aktiv |
    | 🌧️ **Regentag / Schwüle** | 23°C / 55% rH | 18°C / 90% rH | Innen: 11,3 g/m³ **<** Außen: 13,8 g/m³ | 🛑 Außenluft feuchter → Feuchtigkeits-Demand = 0 |
    | ❄️ **Winternacht** | 21°C / 45% rH | −5°C / 80% rH | Innen: 8,3 g/m³ **>** Außen: 2,6 g/m³ | ✅ Kalte Luft ist sehr trocken → Lüften hilft |

    > [!TIP]
    > Dieses 2-Stufen-Prinzip hebt VentoSync von den meisten kommerziellen WRG-Geräten ab, die blind auf Basis der relativen Luftfeuchtigkeit lüften und dadurch die Raumfeuchtigkeit bei Regen oder Schwüle sogar **erhöhen** können.

    Falls beide Temperatursensoren ausfallen, greift ein Fallback, der die relative Feuchtigkeit direkt vergleicht. Details im [📄 Smart-Automatik Modus (Auto-Logik)](documentation/de/de_smart-automatic-logic.md).
- 📊 **Echte VentoMaxx V-Kennlinie**: Basierend auf den physikalischen Parametern der Original-Hardware (50% PWM = Stopp-Zone), wurde die Kennlinie jedoch in den niedrigeren Stufen (Stufe 1-6) feiner abgestimmt, um akustisch noch dezenter zu bleiben.
- 🪟 **Fenstersperre (Window Guard)**: Automatischer raumweiter Lüftungsstopp bei offenen Fenstern mit 5s Verzögerung, automatischem Fortsetzen und Master-LED-Feedback.
  > 👉 *Einrichtungsanleitung & Details: [📄 Fenstersperre Setup Guide](documentation/de/de_window-guard-ha-setup.md).*
- ❄️🔥 **Klima-Koordination (Smart Climate Control)**: Solange die Raumklimaanlage aktiv ist, drosselt die `Smart-Automatik` auf eine reine CO2-Regelung (gelockertes Ziel 1200 ppm, Lüfter-Obergrenze Stufe 3, erzwungene Wärmerückgewinnung), damit die Lüftung keine heiße Außenluft importiert. CO2-Notfall (1500 ppm) und Schimmelschutz (70 % rH) stellen die volle Regelung automatisch wieder her; die Freigabe ist entprellt (120 s).
  > 👉 *Konzept, Zustandsautomat & HA-Template-Sensor: [📄 Intelligente Klimaanlagen-Koordination](documentation/de/de_smart-climate-control.md).*

- 🌟 **Erweiterte Komfort- & Schutzfunktionen**:
  - 📈 **Phasen-Kontinuität & Sanftanlauf**: Proportionalskalierung bei Stufenwechseln und sanfte Geschwindigkeitsübergänge (~5%/s) für minimalen Verschleiß und leisen Betrieb.
  - 🔄 **Echtzeit-Diagnose**: Klartext-Richtungsanzeige (*Zuluft*, *Abluft*, *Stillstand*) und virtuelle Drehzahlberechnung (4200 RPM @ 100%).
  - 🌴 **Urlaubsmodus**: Automatischer Energiesparbetrieb mit konfigurierbarem Modus/Stufe bei längerer Abwesenheit.
  - 🔒 **Kindersicherung**: Sperrung der Gerätetasten über Home Assistant oder per Tastenkombination (5s Modus/Stufe halten) mit LED-Feedback.
  > 👉 *Ausführliche Dokumentation, Parameter & Funktionsweise siehe [📄 Komfort- und Sicherheitsfunktionen](documentation/de/de_comfort-and-safety-features.md).*

### ⚡ Extrem niedriger Stromverbrauch

Das VentoMaxx System mit dieser ESPHome Steuerung arbeitet überragend effizient. Durch die Nutzung eines hochwertigen Traco-Netzteils und der präzisen PWM-Steuerung des ebm-papst Motors liegt die reine Wirkleistung (gemessen an 230V) in einem Bereich, der viele kommerzielle Anlagen deutlich unterbietet:

- **Stufe 1 (Grundlüftung):** ~2,7 - 2,9 Watt *(ca. 7,36 € / Jahr)*
- **Stufe 5 (Erhöhte Last):** ~3,2 - 3,7 Watt *(ca. 9,10 € / Jahr)*
- **Stufe 10 (Maximalleistung):** ~5,0 - 6,0 Watt *(ca. 15,75 € / Jahr)*

Selbst bei ganzjährigem 24/7-Dauerbetrieb auf der *absoluten Maximalstufe (10)* belaufen sich die nominellen Stromkosten (bei 0,30 €/kWh) auf lediglich rund 15 Euro im Jahr. Im meist genutzten Automatik-Modus (Werte pendeln nachts oder bei Abwesenheit auf Stufe 1 bis 3) liegen die realen Betriebskosten bei extrem sparsamen **ca. 7 bis 8,50 Euro pro Jahr** für die gesamte Einheit.

> **Hinweis**: Es handelt sich hierbei um keine 100% akkurate Labormessung. Ich habe diese Werte mittels eines Shelly 1PM mini ermittelt.

*Besonders bemerkenswert: In diese Messwerte ist der durchgängige Betrieb aller verbauten Komponenten eingeflossen – inklusive der ESP32-Steuerung (WLAN/ESP-NOW), der Klima- und CO2-Sensoren sowie dem kontinuierlich messenden mmWave-Radar-Anwesenheitssensor!*

### 🖥️ Native Bedienung am Gerät

Das originale Bedienpanel des VentoMaxx V-WRG-1 (9 LEDs, 3 Taster) bleibt vollständig erhalten und wird um 10 Lüftungsstufen, synchrone Gruppen-Aktivierung in Echtzeit sowie automatisches Dimmen aufgewertet.

![Bedienung am Lüftungsgerät](images/Ventomax%20V-WRG-1/PXL_20260128_232625674.jpg)

> 👉 *Für eine schnelle Tastenübersicht siehe [Bedienpanel am Gerät](#🖐️-bedienpanel-ventomaxx-style) weiter unten oder die ausführliche [📄 Bedienungsanleitung Lüftungsgerät](documentation/de/de_control-panel-operation.md).*

### 🏠 Home Assistant Integration

**Vollständige Home Assistant Integration**: Native Unterstützung der **ESPHome Native API** für hochperformantes Echtzeit-Monitoring und Steuerung. Im Gegensatz zum herkömmlichen MQTT nutzt die Native API hochoptimierte Protocol Buffers für minimale Latenz und geringsten Ressourcenverbrauch.

- **Sofortige Synchronisierung**: Zustandsänderungen werden sofort übertragen, mit bis zu 10-mal kleineren Nachrichtengrößen als bei MQTT.
- **Zero-Configuration**: Automatische Erkennung in Home Assistant – keine manuelle Einrichtung von Entitäten oder ein MQTT-Broker erforderlich.
- **Verschlüsselt & Sicher**: Ende-zu-Ende verschlüsselte Kommunikation mit Home Assistant über Pre-Shared Keys (ESPHome Native API Verschlüsselung auf Basis des Noise-Protokolls).

**Hybride Integrations-Philosophie**: Während der **Hauptfokus** von VentoSync auf einer tiefen und nahtlosen Integration in **Home Assistant** liegt, bietet das Projekt auch eine leistungsstarke Alternative. Durch das integrierte **lokale Web-Dashboard** kann das System als **voll funktionsfähige Standalone-Lösung** genutzt werden. Dies ermöglicht es Anwendern, den vollen Funktionsumfang – von der automatisierten Lüftung bis zur Sensordiagnose – zu nutzen, ohne jemals eine Home Assistant-Instanz einrichten oder warten zu müssen.

> 🔌 **MQTT für externe Systeme**: VentoSync unterstützt optional die MQTT-Veröffentlichung zur Integration mit Node-RED, openHAB, ioBroker und anderen MQTT-basierten Plattformen — ohne die native Home Assistant-Integration zu beeinflussen. Siehe [📄 MQTT-Integrations-Leitfaden](documentation/de/de_mqtt-integration.md) für die Einrichtung.

### 📊 VentoSync Dashboard - Lokales Web-Dashboard

Für VentoSync ist kein Home Assistant oder Smart-Home-Server zwingend erforderlich: Jedes Lüftungsgerät stellt eine eigene, direkt im Webbrowser (auf Smartphone, Tablet oder PC) aufrufbare Benutzeroberfläche bereit — so kannst du Raumluftwerte live überwachen, Betriebsmodi steuern und Einstellungen vornehmen, ganz ohne App- oder Software-Installation.

<p align="center">
  <img src="documentation/screenshots/wrg-dashboard1.png" alt="WRG Dashboard Einstellungen" width="48%" />
  &nbsp;
  <img src="documentation/screenshots/wrg-dashboard2.png" alt="WRG Dashboard Verbundene Geräte & Echtzeitdaten" width="48%" />
</p>

> 👉 *Vollständige Funktionsübersicht, ESP-NOW Live-Visualisierung und Standard-ESPHome-Interface siehe [📄 Lokales Web-Dashboard Guide](documentation/de/de_local-web-dashboard.md).*

## 📡 ESP-NOW: Kabellose Autonomie

Die VentoSync-Geräte kommunizieren direkt untereinander über [ESP-NOW](https://esphome.io/components/espnow.html) — ein schnelles, verbindungsloses 2,4-GHz-Funkprotokoll von Espressif.

Ich habe mich hier bewusst **gegen die fehleranfällige Kommunikation über die Stromleitung (Powerline / PLC)** wie beim originalen VentoMaxx-System entschieden: Die Datenübertragung über das 230V-Stromnetz leidet im Alltag oft unter Phasenproblemen und Störsignalen, während herkömmliches WLAN von einem funktionierenden Router abhängt. **ESP-NOW** bietet den idealen, modernen Mittelweg — eine extrem zuverlässige, blitzschnelle und direkte Funkverbindung, die vollkommen autonom und unabhängig vom normalen WLAN-Netzwerk arbeitet und keinerlei Steuerleitungen erfordert.

<p align="center">
  <img src="EasyEDA-Pro/PCB%20mounting/PCB-ANT-in-Gehäuse.jpg" alt="Externe Antenne im Gehäuse" width="500" />
</p>

> 👉 *Ausführliche Details zu Protokoll v4, dynamischer Raum-Discovery, Unicast-Architektur und Antennen-Optimierung siehe [📄 ESP-NOW Kommunikation Guide](documentation/de/de_esp-now-communication.md).*

---

## 🗺️ Roadmap & Zukünftige Erweiterungen

VentoSync wird aktiv gepflegt und kontinuierlich weiterentwickelt, mit Fokus auf tiefere Sensorfusion, akustische Optimierung und moderne Gebäudeautomation.

> 👉 *Detaillierte Beschreibungen und Konzepte aller geplanten Features und Roadmap-Meilensteine siehe [📄 Roadmap & Zukünftige Erweiterungen](documentation/de/de_roadmap-and-future-enhancements.md).*

## 🎛️ Eigene Platine - PCB

Eine eigens entwickelte Platine (PCB) wurde entworfen, um alle Kernkomponenten (XIAO ESP32-C6, Traco Power DC/DC-Wandler, Pegelwandler) in einer kompakten und robusten Einheit zu vereinen. Die Platinen werden von JLCPCB gefertigt und befinden sich aktuell in der finalen Validierungsphase.

**Wichtige Design-Prinzipien:**

- **Hohe Zuverlässigkeit & Langlebigkeit**: Komponenten wurden gezielt für eine prognostizierte Lebensdauer von >10 Jahren im 24/7-Dauerbetrieb ausgewählt.
- **Sicherheit an erster Stelle**: Trotz des geringen Stromverbrauchs folgt das Layout strengen Sicherheitsstandards, um Brandschutz und Spannungsstabilität zu gewährleisten.
- **Zukunftssichere Erweiterbarkeit**: Die Platine verfügt über dedizierte Erweiterungs-Header für zukünftige Upgrades:
  - **H4 (UART)**: Hochgeschwindigkeits-Serienverbindung (wird aktuell für das mmWave-Radar genutzt).
  - **H3 (I²C)**: Für zusätzliche Umgebungssensoren oder OLED-Displays.
  - **H1 (GPIO)**: 6 freie GPIOs inklusive 3,3V/GND für eigene DIY-Erweiterungen.

![PCB Prototype](EasyEDA-Pro/PCB%20Prototype%20Images/3D_PCB_ESPHome-WRG_ESP32_PWM_2026-08-28.png)

### Passgenaues SCD43 Sensor Board

Um die höchstmögliche Genauigkeit zu erzielen, wurde eine separate Platine speziell für den **Sensirion SCD43** entwickelt. Im Gegensatz zu generischen Breakout-Boards implementiert dieses Design die Referenzspezifikationen des Herstellers zur Entkopplung und hat genau die Dimensionen, so dass der Sensor an der exakten Zuluftöffnung positioniert werden kann:

- **Thermische Entkopplung**: Ein spezieller Frässchlitz und kupferfreie Zonen "entkoppeln" den Sensor thermisch von der Wärmekapazität der Hauptplatine.
- **Präzisionsfilterung**: Korrekte Entkopplungskondensatoren sind in unmittelbarer Nähe des Sensors platziert.
- **Perfekte Passform**: Entwickelt mit einem 1,25-mm-Pitch-Anschluss, der perfekt mit der Zuluftöffnung des VentoMaxx-Gehäuses ausgerichtet ist.

<img src="EasyEDA-Pro/PCB%20Prototype%20Images/3D_PCB%20SCD43%20Gas%20Sensor_2026-08-28.png" alt="SCD43 Prototyp" width="25%" />

### 🌿 Modulares Sensor-Ökosystem: Die passende Messgröße pro Raum

Neben dem SCD43-Board befinden sich aktuell **zwei weitere Sensorplatinen mit dem Sensirion SGP41** in Entwicklung — speziell konzipiert für Räume, in denen CO₂ die tatsächliche Luftqualität nicht ideal beschreibt:

- **Wohn- und Schlafbereich (Personenbelegung)**: CO₂ ist ein direktes Maß für die Anwesenheit von Menschen. Im Wohn- und Schlafbereich ist der Mensch die Quelle — dort regelt der **SCD43** exakt richtig.
- **Küche (Bratfette, Gerüche & Verbrennungsgase)**: Beim Kochen entstehen Belastungen durch Bratfette, starke Gerüche, Lösemittel aus Reinigern und bei einer Gasflamme zusätzlich Stickoxide (NOx). Der CO₂-Wert bleibt dabei oft niedrig, während die Luft längst schlecht ist — eine reine CO₂-Regelung würde hier nicht reagieren.
- **Badezimmer & Feuchträume**: Erfordert schnelles Feuchtemanagement kombiniert mit VOC-Geruchserkennung.

Alle Sensorplatinen nutzen die identische **vierpolige I²C-Schnittstelle (1,25 mm Pitch, Header H2)**. Dies ermöglicht ein modulares Konzept: **Ein Steuergerät — je Raum die perfekt passende Sensorplatine**:

- **Sensirion SGP41**: Liefert den **VOC- und NOx-Index** — zwei getrennte Kenngrößen für Gerüche und Verbrennungsgase.
- **Sensirion SHT4x (z.B. SHT45)**: Ergänzt präzise Temperatur- und Feuchtemessung zur Berechnung der absoluten Feuchte (Enthalpie), entscheidend in Küche und Bad.

| Sensorplatine | Bestückung | Messgrößen | Idealer Einsatzbereich |
| :--- | :--- | :--- | :--- |
| **SCD43 Board** | Sensirion SCD43 | CO₂, Temperatur, rel. Feuchte | Wohnzimmer, Schlafzimmer, Büro |
| **SGP41 + SHT4x Board** | Sensirion SGP41 + SHT45 | VOC-Index, NOx-Index, Temperatur, Feuchte (absolute Feuchte) | Küche, Badezimmer |
| **SGP41 Board** | Sensirion SGP41 | VOC-Index, NOx-Index | Küche (wenn Klimadaten bereits vorliegen) |

<p align="center">
  <img src="EasyEDA-Pro/PCB%20Prototype%20Images/3D_PCB%20SGP41%20SHT4x%20VOC%20Sensor_2026-08-28.png" width="25%" alt="3D PCB SGP41 + SHT4x Sensor Board" />
  &nbsp;&nbsp;&nbsp;&nbsp;
  <img src="EasyEDA-Pro/PCB%20Prototype%20Images/3D_PCB%20SGP41%20VOC%20Sensor_2026-08-28.png" width="25%" alt="3D PCB SGP41 VOC Sensor Board" />
</p>
<p align="center"><em>Links: SGP41 + SHT4x Kombi-Board (VOC, NOx, Temp, Feuchte) &nbsp;|&nbsp; Rechts: SGP41 Board (VOC, NOx)</em></p>

> 👉 *Detaillierte Komponentenspezifikationen, die vollständige Stückliste (BOM), Lüfter-Verdrahtung, GPIO-Pinbelegung des XIAO ESP32-C6 und schematische Blockschaltbilder siehe [📄 Hardware, BOM & Verdrahtung Guide](documentation/de/de_hardware-and-wiring.md).*

---

## 🛠️ Einrichtung & Installation

### 🧰 PCB-Montage & Lüfter-Verdrahtung

Das maßgefertigte VentoSync-PCB wird als Drop-in-Replacement direkt in das Originalgehäuse der **VentoMaxx V-WRG** Serie eingesetzt und nutzt die vorhandenen Montagepunkte sowie die Originalblende.

> [!CAUTION]
> **NETZSPANNUNG (230V):** Arbeiten im Lüftungsgerät erfordern den Umgang mit gefährlicher 230V-Netzspannung. Schalte vor dem Öffnen des Gehäuses oder dem Berühren von Kabeln unbedingt den Leitungsschutzschalter am Sicherungskasten aus und prüfe die Spannungsfreiheit. Elektrische Arbeiten dürfen ausschließlich von einer qualifizierten Elektrofachkraft durchgeführt werden.

#### Gehäuse-Einbau & Kabelführung

- **Passform**: Die Platine wird passgenau in die Führungsschienen des Originalgehäuses geschoben.
- **Antennen-Ausrichtung**: Richte die externe bzw. PCB-Antenne frei in den Raum aus, um maximale Reichweite für WLAN und ESP-NOW zu erzielen.
- **Sensorkabel verlegen**: Das 14-polige FFC-Flachbandkabel zum Front-Bedienpanel führen und das optionale SCD43-Sensorboard an Header **H2** anstecken (optimal im Zuluftkanal positioniert).

![PCB und Lüfter im Gehäuse montiert](EasyEDA-Pro/PCB%20mounting/PCB-FAN-ANT-in-Gehäuse.jpg)
*VentoSync PCB im VentoMaxx-Gehäuse montiert mit angeschlossenem Lüfter und verlegter Antenne.*

#### Lüfter-Kabelanschluss (Original 3-Pin / 4-Pin)

Schließe das Lüfterkabel an die dedizierte **FAN**-Buchse auf dem PCB an. Das PCB unterstützt sowohl den originalen 3-Pin EBM-Papst Lüfter (4412 F/2 GLL VarioPro) als auch moderne 4-Pin PWM-Lüfter (z. B. AxiRev mit Tachosignal).

![Lüfter Anschluss](EasyEDA-Pro/PCB%20mounting/PCB-Anschluss-FAN2.jpg)
*Lüfter-Anschlussbelegung mit Originalkabel.*

| Pin / Ader | Original Kabelfarbe | Funktion | Beschreibung |
| :--- | :--- | :--- | :--- |
| **GND** | Grün | Masse (0V) | Lüfter-Masse (GND) |
| **+12V** | Braun | 12V DC Versorgung | Geschaltete 12V-Spannungsversorgung vom Traco-Netzteil |
| **PWM** | Weiß | PWM Steuersignal | Geschwindigkeits- & Richtungssignal vom ESP32-C6 (GPIO19) |
| **TACH** | - | Tacho-Impuls | *(Optional bei 4-Pin-Lüftern)* RPM-Drehzahlrückmeldung (GPIO20) |

---

### 💻 Entwicklungsumgebung (Linux `venv` & ESPHome-CLI)

Für eine stabile Entwicklungsumgebung wird dringend empfohlen, ESPHome in einer **virtuellen Python-Umgebung** (`venv`) zu installieren. Dies vermeidet Konflikte mit systemweiten Paketen und ist die einzige offiziell unterstützte manuelle Installationsmethode unter Linux.

```bash
# 1. Virtuelle Umgebung erstellen
python3 -m venv venv

# 2. Umgebung aktivieren
source venv/bin/activate

# 3. ESPHome installieren
pip install --upgrade esphome
```

*(Hinweis: Denke immer daran, `source venv/bin/activate` auszuführen, bevor du den Befehl `esphome` in einer neuen Terminal-Sitzung verwendest.)*

#### 🔄 Umgebung aktualisieren

Um deine Entwicklungsumgebung auf dem neuesten Stand zu halten, verwende die folgenden Befehle:

**ESPHome aktualisieren (innerhalb der venv):**

```bash
# Sicherstellen, dass venv aktiv ist
source venv/bin/activate
# Auf die neueste Version aktualisieren
pip install --upgrade esphome
```

**Vollständiges System- & Python-Update (Linux):**

```bash
# Paketliste aktualisieren und alle Systempakete upgraden
sudo apt update && sudo apt upgrade -y
```

**pip & setuptools aktualisieren (innerhalb der venv):**

```bash
pip install --upgrade pip setuptools
```

### ⚙️ Konfiguration & Kompilierung

VentoSync nutzt eine modulare Hardware-Architektur. Wähle je nach verbauter Hardware die passende Konfigurationsdatei:

- **`ventosync.yaml`**: Vollversion (SCD43, BME680, LD2450)
- **`ventosync_bme680_only.yaml`**: Variante mit BME680 (ohne SCD43/LD2450)
- **`ventosync_radar_only.yaml`**: Variante mit Radar (ohne Klima-Sensoren)
- **`ventosync_nosensor.yaml`**: Basis-Lüftungssteuerung ohne Sensoren

Nutze das Skript `upload_all.sh` für automatisches Kompilieren und Flashen lokal auf deine Geräte:

```bash
# Kompiliert und flasht alle im Skript definierten Geräte
./upload_all.sh
```

Alternativ manuell für ein einzelnes Gerät per ESPHome CLI:

```bash
# 1. Konfiguration validieren (prüft auf YAML-Fehler)
esphome config ventosync_nosensor.yaml

# 2. Kompilieren & via OTA flashen (lädt automatisch auf die angegebene IP hoch)
esphome run ventosync_nosensor.yaml --device <IP-Adresse> --no-logs

# 3. Nur kompilieren (generiert die Binärdatei ohne Upload)
esphome compile ventosync_nosensor.yaml

# 4. Nur flashen (nützlich, wenn die Binärdatei bereits kompiliert wurde)
esphome upload ventosync_nosensor.yaml --device <IP-Adresse> --no-logs
```

### ⚡ Erstmaliges Flashen & Inbetriebnahme

1. **Firmware vorbereiten**: Kompiliere die Firmware mit deinen eigenen WLAN-Zugangsdaten (via `secrets.yaml`).
2. **Initiales Flashen**: Flashe den ESP (XIAO) initial per USB mit der VentoSync Firmware über das ESPHome Dashboard oder per ESPHome CLI-Befehl:

   ```bash
   esphome run ventosync.yaml --device /dev/ttyACM0
   ```

3. **Initiale Einrichtung (Captive Portal)**:
   Die kompilierten Firmware-Binaries auf GitHub sind "secret-free" und enthalten keine fest einkompilierten WLAN-Zugangsdaten. Wenn du ein OTA-Update mit diesen offiziellen Release-Dateien durchführst oder dein Gerät die WLAN-Verbindung verliert, befolge diese Schritte, um die WLAN-Verbindung wiederherzustellen:
   1. Suche mit deinem Smartphone oder PC nach dem WLAN **"VentoSync Hotspot"**.
   2. Verbinde dich mit dem Passwort: `ventosync`
   3. Es sollte sich automatisch ein Fenster (Captive Portal) öffnen. (Falls nicht, rufe im Browser `192.168.4.1` auf).
   4. Wähle dein Heim-WLAN aus der Liste aus und gib dein Passwort ein.
   **Fertig!** ESPHome hat nun deine Zugangsdaten dauerhaft im NVS-Flash gesichert. **Alle zukünftigen OTA-Updates werden diese Zugangsdaten automatisch nutzen und sich sofort verbinden.**
4. **Netzwerk-Konfiguration**: Hinterlege die IP-Adresse im Router als **feste IP (Static IP)**, um eine dauerhaft stabile Erreichbarkeit zu garantieren.

### 🔄 OTA-Updates & Home Assistant Integration

1. **Home Assistant Integration**: Füge das Gerät in Home Assistant unter der ESPHome-Integration hinzu (es wird i. d. R. sofort automatisch erkannt).
2. **Einstellungen anpassen**: Passe nach der Integration die folgenden Basis-Einstellungen in der Home Assistant UI oder dem lokalen Dashboard an:
   - **Device ID** (Eindeutige Nummer des Geräts)
   - **Room ID** (Geräte mit gleicher Room ID synchronisieren sich)
   - **Floor ID**
3. **Alternative - Web Dashboard**: Alternativ können (auch ohne Home Assistant) alle Settings über das lokale Web-Dashboard konfiguriert werden unter `http://<device-ip>` oder `http://<device-ip>/ui`.
4. **Spaß haben**: Genieße dein smartes, energieeffizientes Lüftungssystem!
5. **OTA-Updates**:
   Beispiel eines OTA Updates in Home Assistant:
   ![OTA Update in Home Assistant](documentation/screenshots/OTA-Update.png)

### 🌡️ Kalibrierung der NTC-Sensoren

Die Konfiguration ist optimiert für den NTC-Thermistor **[ENTC-10K9777-02](https://www.reichelt.de/de/de/shop/produkt/thermistor_ntc_-40_bis_125_c-350474)** (10kΩ, B-Wert 3435). Falls du andere Sensoren verwendest, müssen die Werte für `b_constant` und `reference_resistance` im YAML-Code entsprechend angepasst werden.

---

## 🎮 Bedienung & Steuerung

Die Steuerung erfolgt intuitiv über das integrierte Bedienpanel oder vollautomatisch via Home Assistant.

### 🖐️ Bedienpanel (VentoMaxx Style)

Das Panel verfügt über 3 Taster und 9 Status-LEDs.

#### Tastenbelegung

| Taste | Funktion | Bedienung |
| :--- | :--- | :--- |
| **Power (I/O)** | System Ein/Aus | • Kurz drücken: Schaltet Lüftung EIN/AUS (AUS: stoppt Lüfter, bleibt online im Monitoring-Modus; EIN: stellt Modus wieder her & beendet Light-Sleep)<br>• Lang (>5s): Versetzt das Gerät in den Light-Sleep-Modus (Lüfter/LEDs/WLAN aus)<br>• Sehr lang (>10s): Versetzt das Gerät in den Light-Sleep-Modus und startet den ESP32 neu (Reboot) |
| **Modus (M)** | Betriebsmodus | • Kurz drücken: Zykliert durch Automatik → WRG → Durchlüften → Stoßlüftung → Aus |
| **Stufe (+)** | Lüfterstärke | • Kurz drücken: Zykliert durch 10 Geschwindigkeitsstufen (angezeigt über 5 LEDs).<br>• **Gedrückt halten**: Automatisches Auf- und Ab-Durchlaufen der Stufen (1 Stufe pro Sekunde) bis zum Loslassen. |

#### Status-LEDs (Feedback)

| LED | Anzahl | Position | Verhalten |
| :--- | :---: | :--- | :--- |
| **Power** | 🟢 1x | LED Panel | Leuchtet hell im Betrieb. Dimmt nach 60s @ `ui_active_timeout` (Standard: 60s) auf 20% Helligkeit ab (statt ganz auszugehen). |
| **Master** | 🟢 1x | LED Panel | Leuchtet bei aktivem UI (Normalbetrieb). Signalisiert Störungen durch Blink-Muster: **2x**: Raum-Synchronisierung fehlgeschlagen | **3x**: WLAN-Verlust | **4x**: Hitze-Warnung (50-60°C). Bei über 60°C schaltet das Gerät automatisch ab. |
| **Modus L** (`LED_WRG`) | 🟢 1x | Links | **Pulsiert** im Smart-Automatik Modus. Dauerhaft an bei WRG oder Durchlüften. |
| **Modus R** (`LED_VEN`) | 🟢 1x | Rechts | Dauerhaft an bei Stoßlüftung oder Durchlüften. |
| **Intensität** | 🟢 5x | LED Panel | Zeigt aktuelle Lüfterstufe 1–10 (halbe/volle Helligkeit für 10 Stufen über 5 LEDs). Nur bei aktivem UI sichtbar. |

**Modus-LED Zuordnung (bei aktivem UI):**

| Modus | `LED_WRG` (links) | `LED_VEN` (rechts) |
| :--- | :---: | :---: |
| **Automatik (Standard)** | 🟢 (pulsiert langsam) | ⚫ |
| Wärmerückgewinnung (Eco) | 🟢 | ⚫ |
| Stoßlüftung | ⚫ | 🟢 |
| Durchlüften (Sommer) | 🟢 | 🟢 |
| Aus / System OFF | ⚫ | ⚫ |

> 💡 **60 Sekunden Auto-Dimming:** Alle Status-LEDs (Modus, Intensität, Master) erlöschen 60 Sekunden (konfigurierbar) nach dem letzten Tastendruck sanft. Die **Power-LED** bleibt dabei auf 20% gedimmt an. Bei jedem Tastendruck werden alle LEDs wieder aktiviert. Ausnahme: Die **Master-LED signalisiert Fehlerzustände weiter**, auch nach dem Timeout.

---

### 🔄 Detaillierte Betriebsmodi (Programme)

Über die **Modus-Taste (M)** zykliert das Gerät durch die Programme. Beim **Einschalten** ist **Modus 1 (Smart-Automatik)** aktiv.

> 💡 **Tipp:** Die Reihenfolge beim Tastendruck ist: **Automatik → WRG → Durchlüften → Stoßlüftung → Aus → Automatik...**

---

#### 1. 🤖 Smart-Automatik *(Standard / Empfohlen)* — `LED_WRG` 🟢 (pulsiert langsam)

**Dieser Modus ist der Standard beim Einschalten** und übernimmt vollautomatisch alle Steuerungsaufgaben. Die Lüftungsanlage regelt sich eigenständig basierend auf Umgebungsdaten und erfordert nach initialer HA-Konfiguration keinerlei manuelle Eingriffe ("Set and Forget").

**Aktive Smart-Features:**

| Feature | Sensor(en) | Schwellenwert |
| :--- | :--- | :--- |
| ✅ **CO2-Regelung (PID)** | SCD43 (`sensor.scd41_co2`) | `number.auto_co2_threshold` |
| ✅ **Feuchte-Management (PID)** | SCD43 (`sensor.scd41_humidity`) + HA `outdoor_humidity` | Über Außenfeuchte |
| ✅ **Sommer-Kühlfunktion** | NTC-Sensoren + ESP-NOW Gruppentemperatur | 22°C Innentemperatur |

**Logik im Detail:**

- **Grundbetrieb:** Wärmerückgewinnung (`MODE_ECO_RECOVERY`) auf Mindestlüfterstufe (`co2_min_fan_level`, Standard: 2). Die Wechselintervalle (Zyklusdauer) passen sich dabei dynamisch der aktuellen Lüfterstufe an (sanfte 70 Sekunden auf Stufe 1 bis schnelle 50 Sekunden auf Stufe 10) inkl. synchronisiertem NTC-Zeitfenster.

- **🎛️ Intelligente PID-Regelung — CO2 & Feuchtigkeit:** Anstatt den Lüfter einfach auf volle Leistung zu schalten, wenn Grenzwerte überschritten werden, nutzt VentoSync einen **PID-Regler**, um die Lüfterstärke präzise, stufenweise und vor allem leise zu regulieren.

  > **Was ist ein PID-Regler?**
  > Stell dir vor, du fährst Auto: Bist du nur knapp über dem Tempolimit, nimmst du kaum Gas raus. Bist du weit drüber, bremst du stärker. Und wenn du schon längere Zeit knapp drüber bist, drückst du etwas mehr auf die Bremse. VentoSync funktioniert mit CO2 und Luftfeuchte genauso — kein abruptes Schalten, sondern sanftes, kontinuierliches Nachregeln.

  Der Regler hat **zwei aktive Anteile**:

  - **P (Proportional)**: Reagiert *sofort* auf die Abweichung. Bei 100 ppm über dem Grenzwert: moderate Anforderung. Bei 500 ppm: spürbar höher.
  - **I (Integral)**: Das „Gedächtnis" des Reglers. Bleibt eine Abweichung *über längere Zeit* bestehen (z. B. weil Personen im Raum atmen), erhöht dieser Anteil die Anforderung langsam und stetig — bis sich die Luftqualität verbessert. Sinkt der CO2-Wert, baut sich der Integral-Anteil wieder ab.

  > [!NOTE]
  > Der Regler ist bewusst sehr langsam eingestellt (I-Anteil: `0.0000005`). Erst anhaltend erhöhte CO2-Werte über viele Minuten führen zu einer höheren Lüfterstufe.

  **Praxisbeispiel** — CO2-Grenzwert `800 ppm`, Lüfterbereich Stufe 2–7:

  | Zeit | CO2-Wert | Was passiert |
  | --- | --- | --- |
  | 0 min | 820 ppm | 20 ppm über Grenzwert → kleiner Bedarf → **Lüfter bleibt auf Stufe 2** (Minimum) |
  | 15 min | 870 ppm | 70 ppm drüber, Integral baut sich auf → **Lüfter bleibt auf Stufe 2** |
  | 30 min | 920 ppm | 120 ppm drüber, Integral akkumuliert → **Lüfter schaltet auf Stufe 3** |
  | 50 min | 960 ppm | CO2 steigt weiter, Integral ebenfalls → **Lüfter schaltet auf Stufe 4** |
  | 70 min | 900 ppm | CO2 fällt, Integral baut ab → **Lüfter kehrt auf Stufe 3 zurück** |
  | 90 min | 790 ppm | Unter Grenzwert, Bedarf → null → **Lüfter kehrt auf Stufe 2 zurück** |

  **Die wichtigsten Verhaltensregeln:**
  - Der Lüfter ändert sich um **maximal ±1 Stufe pro 10-Sekunden-Zyklus** — keine abrupten Sprünge.
  - Der Lüfter unterschreitet nie die konfigurierte **Mindeststufe** (Standard: Stufe 2) und überschreitet nie die **Maximalstufe** (Standard: Stufe 7).
  - CO2 ist das **primäre Regelsignal**, aber das System verwendet immer den **höheren** Bedarf aus CO2 und Feuchtigkeit — so wird keines der beiden Luftqualitäts-Kriterien jemals vernachlässigt.
  - Hat ein Gerät keine eigenen Sensoren, übernimmt es automatisch den höchsten Bedarf aus der Raumgruppe (via ESP-NOW).
  - Beim Wechsel *in* den Smart-Automatik-Modus werden alle Bedarfswerte auf null zurückgesetzt — der Lüfter **startet immer sanft von der Mindeststufe**, niemals direkt auf einer hohen Stufe.

- **💧 Feuchtigkeitsmanagement:** Der Feuchtigkeits-PID-Regler (`pid_humidity`) läuft kontinuierlich parallel zum CO2-Regler. Entfeuchtung wird aktiviert, wenn das Feuchtigkeitslimit überschritten wird (Standard: 60%) **und** die Außenluft tatsächlich trockener ist als die Innenluft (absolute Feuchtigkeitsprüfung via Magnus-Formel — nicht nur relative Feuchtigkeit). Ist die Außenluft feuchter (z. B. Regentag), wird der Feuchtigkeits-Bedarf auf null gesetzt. Siehe [Enthalpie-Schutz ↑](#präzisions-sensorik--monitoring) für Beispiele mit Vergleichstabelle.

- **Sommer-Kühlung:** Bei Innentemperatur > 22°C und kühlerem Außenbereich (mindestens 1,5°C kühler) wechselt das System automatisch in `Durchlüften` (Querlüftung). Phase-A- und Phase-B-Geräte im Raum blasen dabei gleichzeitig in entgegengesetzte Richtungen — echte Querlüftung. Sobald es außen wieder wärmer wird (Hysterese), kehrt das System zu WRG zurück.

- **Anwesenheit (Manuelle Modi):** In den Modi WRG, Durchlüften und Stoßlüftung wird die Lüfterstärke bei erkannter Präsenz dynamisch angepasst (Slider `-5` bis `+5`). Dies erlaubt einen bedarfsgerechten „Präsenz-Boost" ohne die Automatik-Regelung zu beeinflussen.

- **🌱 Energiespar-Modus (Light Sleep):** Aktivierbar durch langen Druck auf den Power-Button (>5s). Im Light Sleep werden Lüfter und WLAN deaktiviert und der LED-Treiber (PCA9685) komplett stromlos geschaltet. Ein einfacher Druck auf den Power-Button weckt das Gerät sofort wieder auf und synchronisiert es mit der Gruppe.

- **Gruppenlogik:** PID-Demand und Temperaturen werden sekündlich via ESP-NOW Unicast geteilt — alle entdeckten Geräte im Raum laufen synchron (die Lüfter skalieren identisch auf den höchsten Bedarf im Raum).

> **⚙️ Voraussetzung für das Feuchte-Management: `sensor.outdoor_humidity` in Home Assistant**
>
> Der ESPHome-Code erwartet die Entity-ID `sensor.outdoor_humidity` (in `sensors_climate.yaml`). Es gibt zwei Wege:
> **Option A (Wetterdienst):** Erstelle einen Template-Sensor basierend auf deiner Wetter-Integration (z.B. OpenWeatherMap).
> **Option B (Lokaler Sensor):** Erstelle einen Template-Sensor (Alias) oder passe die Entity-ID in der YAML an.
> *Ohne diesen Sensor funktioniert die Entfeuchtung trotzdem, der Outdoor-Check wird dann einfach übersprungen.*
Details siehe [Feuchte-Management-HA-Sensor.md](documentation/de/de_humidity-management.md)

---

#### 2. ❄️ Wärmerückgewinnung (Eco Recovery) — `LED_WRG` 🟢

- **HA Entität:** `select.luefter_modus` → `Wärmerückgewinnung`
- **Funktion:** Manueller WRG-Betrieb ohne die Smart-Automatik-Features. Die Luftrichtung wechselt zyklisch, Wärmeverlust wird um bis zu 85% reduziert.
- **Zykluszeiten:** Passen sich der Lüfterstufe an: Stufe 1: **70 Sek.**, Stufe 2: **65 Sek.**, … Stufe 5: **50 Sek.**
- **Synchronisation:** Phase A bläst hinein, Phase B hinaus — Geräte im Gegentakt, Haus druckneutral.

---

#### 3. 💨 Stoßlüftung — `LED_VEN` 🟢

- **HA Entität:** `select.luefter_modus` → `Stoßlüftung`
- **Funktion:** Intensivlüftung für schnellen Luftaustausch (z. B. nach dem Duschen oder Kochen).
- **Ablauf:** 15 Minuten intensiv lüften, 105 Minuten Pause, dann erneuter 15-Minuten-Zyklus (2 Std. Rhythmus). Wechselnde Startrichtung schützt den Keramikspeicher.

---

#### 4. 🌬️ Querlüftung / Durchlüften (Sommer) — `LED_WRG` 🟢 + `LED_VEN` 🟢

- **HA Entität:** `select.luefter_modus` → `Durchlüften` + `number.lueftungsdauer` (Timer, 0 = Endlos)
- **Funktion:** Konstanter Luftstrom ohne Richtungswechsel. Hälfte der Gruppe saugt an, andere Hälfte bläst ab → kühler Luftzug durch den Wohnraum.
- **Hinweis:** Im Automatik-Modus wird die Querlüftung **automatisch** bei hoher Innentemperatur aktiviert.

---

#### 5. ⭕ Aus (Monitoring-Modus) — beide LEDs ⚫

- **HA Entität:** `select.luefter_modus` → `Aus`
- **Funktion:** Lüfter und PWM-Ausgänge werden gestoppt (0 RPM). Alle Umweltsensoren (CO2, Temp, Radar) und das Web-Dashboard bleiben für unterbrechungsfreies Logging in Home Assistant aktiv. *(Hinweis: Für den stromsparenden Light-Sleep mit abgeschaltetem WLAN die Power-Taste >5s gedrückt halten).*

---

### 📱 Steuerung über Home Assistant

Alle Funktionen sind vollständig in Home Assistant integriert. Änderungen am Panel werden sofort synchronisiert.

#### Verfügbare Steuerungen

- **Lüfter**: Slider 0-10% bis 100% (entspricht intern den 10 Stufen des Bedienpanels)
- **Modus**: Auswahl (Smart-Automatik / Eco Recovery / Ventilation / Off)
- **Timer**: Konfiguration für "Durchlüften" (Standard: 30 Min)
- **LED-Helligkeit**: `number.max_led_brightness` (0-100%, Standard: 80%) zur Begrenzung der maximalen Panel-Helligkeit.
- **CO2-Grenzwert**: `number.auto_co2_threshold` (Im Automatik-Modus immer aktiv)
- **Klima-Koordination** *(Konfiguration)*:
  - `switch.klima_koordination` — HVAC-Koordination für dieses Gerät aktivieren (Standard: aus)
  - `number.klima_koordination_co2_grenzwert` — Gelockerter CO2-Sollwert bei aktiver Klimaanlage, 800–1500 ppm (Standard: `1200`)
  - `number.klima_koordination_max_lufterstufe` — Lüfter-Obergrenze bei aktiver Klimaanlage, 1–5 (Standard: `3`)
  - `number.klima_koordination_co2_notfallgrenze` — CO2-Notfallgrenze, 1200–2000 ppm (Standard: `1500`)
  - `text_sensor.klima_koordination_status` — Aktueller Koordinator-Zustand (Diagnose)
- **Diagnose**: Anzeige von RPM, Temperatur, Feuchte und **CO2-Gehalt (ppm)**

👉 **Tipp:** Eine detaillierte Übersicht aller verfügbaren Home Assistant Entitäten inklusive ihrer technischen Namen (`ID`) und Funktion findest du im Dokument **[Entities_Documentation.md](documentation/de/de_home-assistant-entities.md)**.

#### 📊 Lüftergeschwindigkeit pro Stufe (VentoMaxx V-Kennlinie)

Der original VentoMaxx Lüfter (**ebm-papst 4412 F/2 GLL**) wird über ein **einzelnes PWM-Signal** gesteuert. Die Kennlinie folgt einer V-Form (gemessen via Oszilloskop), wobei 50% PWM den Stillstand markiert:

| Stufe | Leistung | PWM Richtung A (Abluft) | PWM Richtung B (Zuluft) | RPM (ca.) |
| :---: | :---: | :---: | :---: | :---: |
| **OFF** | 0 % | 50.0 % | 50.0 % | 0 |
| **1** | 10 % | 30.0 % | 70.0 % | 420 |
| **2** | 16 % | 27.2 % | 72.8 % | 672 |
| **3** | 23 % | 24.4 % | 75.6 % | 966 |
| **4** | 31 % | 21.7 % | 78.3 % | 1302 |
| **5** | 40 % | 18.9 % | 81.1 % | 1680 |
| **6** | 50 % | 16.1 % | 83.9 % | 2100 |
| **7** | 61 % | 13.3 % | 86.7 % | 2562 |
| **8** | 73 % | 10.6 % | 89.4 % | 3066 |
| **9** | 86 % | 7.8 % | 92.2 % | 3612 |
| **10** | 100 % | 5.0 % | 95.0 % | 4200 |

Das Drehzahlband ist so optimiert, dass es in den niedrigen Stufen (Stufe 1-6) eine feinere Abstufung ermöglicht, um akustisch noch dezenter zu bleiben, während in den höheren Stufen die Leistung schneller ansteigt.
> ⚙️ **Mindestdrehzahl:** Stufe 1 entspricht 10 % Drehzahl (PWM nie auf 50 % = Stopp). Im Automatik-Modus (PID) wird die Drehzahl in **10 Stufen** zwischen `co2_min_fan_level` und `co2_max_fan_level` geregelt.
> 🔄 **Software-Fan-Ramping:** Bei jedem Richtungswechsel (WRG/Stoßlüftung) führt das System eine **5-sekündige sanfte Abbrems- und Anlauframpe** durch. Dies schont den Motor und minimiert Umschaltgeräusche. Die Intensitäts-LEDs zeigen währenddessen bereits den Zielwert an.

#### Automatische Funktionen

- **Unauffälligkeitsmodus (Stealth Mode)**: Die LEDs werden bei Nichtbedienung automatisch abgedunkelt/ausgeschaltet — das verhindert insbesondere störendes Licht in Schlaf- und Wohnräumen bei Nacht.
- **Filterwechsel-Alarm**: Intelligente vorausschauende Wartung auf Basis aktiver Lüfterlaufzeit (**>365 Betriebstage / 8.760h**) und kalendarischer Alterung (**>3 Jahre**), um Hardware und Lufthygiene zu schützen. Inklusive Ein-Klick-Reset nach dem Filtertausch.

> 👉 *Ausführliche Entitäten-Übersicht, Automations-Beispiele und Push-Benachrichtigungen in Home Assistant siehe [📄 Filterwechsel-Alarm Setup Guide](documentation/de/de_filter-change-alarm-ha-setup.md).*

---

## 🧠 Wärmerückgewinnung - So funktioniert's

### Grundprinzip & Übersicht

VentoSync nutzt einen hocheffizienten **Keramik-Wärmespeicher** (regenerativer Rekuperator), um wertvolle Heizenergie beim Lüften im Raum zu halten:

- **Zyklischer Push-Pull-Betrieb**: Der Lüfter wechselt zyklisch in adaptiven **50s bis 70s Phasen** zwischen Abluft (Wärmespeicherung im Keramikkern) und Zuluft (Erwärmung der frischen Außenluft).
- **Synchronisierter Paarbetrieb**: Geräte im selben Raum synchronisieren sich über **ESP-NOW Unicast**. Während ein Gerät frische Luft zuführt, führt das Partnergerät verbrauchte Luft ab – für einen kontinuierlichen, zugfreien Luftaustausch ohne Druckschwankungen.
- **Phasen-synchrone NTC-Temperaturstabilisierung**: Innen- und Außen-NTC-Thermistoren nutzen eine optimierte C++ Filter-Pipeline (`filter_ntc_combined`) mit thermischer Einschwingzeit und saisonaler Min/Max-Selektion für verlässliche Temperaturwerte.
- **Energiebasierte Effizienzberechnung (DIN EN 13141-8)**: Anstelle ungenauer Momentanwerte berechnet das System den thermodynamischen Wirkungsgrad ($\eta_{WRG}$ bis zu ~85%) über eine **numerische Trapez-Integration** über den gesamten Zyklus und ermittelt die tatsächlich **rückgewonnene Wärmeenergie in Wattstunden (Wh)** anhand kalibrierter Volumenstrom-Kennlinien.
- **Fortschrittliche IAQ-Engine (BME680)**: Native C++ Auswertung mit 300°C/150ms Heizprofil, dynamischer Temperaturkompensation und Flash-Wear-Leveling.

> 👉 *Ausführliche mathematische Integrationsmodelle, Formeln, NTC-Filteralgorithmen, Details zur BME680-IAQ-Engine und Synchronisationsdiagramme siehe [📄 Wärmerückgewinnung & Effizienz Guide](documentation/de/de_heat-recovery-and-efficiency.md).*

---

## 🔧 Technische Details & Optimierungen

Detaillierte technische Informationen zu Sensor-Optimierungen, ESPHome YAML Syntax, I²C-Konfiguration und weiteren technischen Aspekten findest du in der separaten Dokumentation:

📄 **[Technical-Details.md](EasyEDA-Pro/documentation/Technical-Details.md)** / **[Smart-Automatik Modus (Auto-Logik)](documentation/de/de_smart-automatic-logic.md)**

Diese Dokumentation enthält:

- ESPHome YAML Syntax Best Practices
- I²C Bus Konfiguration
- SCD43 CO2-Sensor Konfiguration
- ESP-NOW Kommunikation
- Lüftersteuerung (PWM)

---

## 📁 Projektstruktur

```text
VentoSync/
├── .github/workflows/         # CI/CD (GitHub Actions) für automatisierte Builds & Releases
├── components/                # Eigene ESPHome C++ Komponenten & Hilfsbibliotheken
│   ├── helpers/               # Modulare C++ Header (PID-Regelung, ESP-NOW Sync, IAQ-Engine, NTC-Filter)
│   ├── ventilation_group/     # Zentrale Gruppen-Statemachine & Multi-Device-Koordination
│   ├── ventilation_logic/     # IAQ-Klassifizierung, Komfort-Logik & mathematische Helfer
│   └── wrg_dashboard/         # Integriertes Web-UI-Dashboard (Tailwind CSS & Chart.js)
├── documentation/             # Detaillierte technische Anleitungen, Datenblätter & Setups
│   ├── de/                    # Deutsche Dokumentation
│   ├── en/                    # Englische Dokumentation
│   ├── datasheets/            # PDF-Datenblätter
│   └── screenshots/           # UI & Dashboard Screenshots
├── EasyEDA-Pro/               # PCB-Hardware-Dateien (Schaltpläne, Gerber, BOM, Fotos)
├── ESPHome-VentoMaxx-Analyser/# Hardware-Analyse & PWM-Oszilloskop-Messtools
├── ha_integration_example/    # Home Assistant Dashboard-Vorlagen & Master-Node-Configs
├── json/                      # Deployment-Manifeste & GitHub-Release-Vorlagen
├── packages/                  # Modulare YAML-Konfigurationspakete
│   ├── actuators/             # PID-Regler, Automationen, Schutz- & Urlaubslogik
│   ├── base/                  # ESP32-C6 Basiskonfiguration, Geräteeinstellungen & WLAN/OTA
│   ├── communication/         # ESP-NOW Unicast- & Broadcast-Protokolle
│   ├── globals/               # Strukturierte globale Variablen (Automation, Netzwerk, UI, Lüfter)
│   ├── integration/           # Home Assistant Entitäten & Datenanbindung
│   ├── io/                    # Lüfter-PWM, Taster, PCA9685/MCP23017 Pinbelegungen
│   ├── sensors/               # Treiber & Mocks für SCD43, BME680, Radar, BMP390, NTCs
│   └── ui/                    # Bedienpanel-Steuerung & Diagnose-Entitäten
├── tests/                     # C++ Unit-Tests & native Test-Suite
├── ventosync.yaml             # Hauptkonfiguration (Vollversion mit allen Sensoren)
├── ventosync_bme680_only.yaml # Hardware-Variante: BME680 Fallback (ohne SCD43/Radar)
├── ventosync_radar_only.yaml  # Hardware-Variante: Nur Radar-Anwesenheitssensor
├── ventosync_nosensor.yaml    # Hardware-Variante: Basis-Lüftersteuerung ohne Sensoren
├── ventosync_NTConly.yaml     # Hardware-Variante: Basis-Lüftersteuerung nur mit NTCs
├── upload_all.sh              # Batch-Kompilierungs- & OTA-Upload-Skript für alle Geräte
└── version.json               # Aktuelle semantische Firmware-Version & Release-Metadaten
```

---

## 🏗️ Code-Architektur & Wartbarkeit

### Mehrstufige modulare Architektur

Um dauerhafte 24/7-Stabilität, langfristige Wartbarkeit und saubere Codequalität zu gewährleisten, setzt VentoSync auf eine strikt entkoppelte Schichtenarchitektur:

- **Strikte YAML-Modularisierung (`packages/`)**: Aufteilung in 8 thematische Domänenpakete (`base`, `communication`, `globals`, `io`, `sensors`, `actuators`, `integration`, `ui`). Sensor-Mocks (`mock_*.yaml`) sorgen für saubere Fallbacks ohne Kompilierfehler oder Log-Spamming bei verschiedenen Hardware-Varianten.
- **Nativer C++ Hilfsbibliotheks-Kern (`components/helpers/`)**: Komplexe Lambdas wurden vollständig aus dem YAML-Code in typensichere C++ Header ausgelagert (PID-Regelung, ESP-NOW Status-Synchronisation, IAQ-Engines, Taster-/LED-Handler) – für maximale Ausführungsgeschwindigkeit und native Testbarkeit.
- **Laufzeit- & Performance-Optimierungen**: Thread-sichere Eventverarbeitung mit `std::lock_guard`, Move-Semantik, NaN-sicherer PID-Regler, Flash-Schonung (8h NVS-Pufferung) und kombinierte NTC-Filterung (`filter_ntc_combined`).
- **Deterministischer Boot-Ablauf**: Mehrstufige Initialisierungssequenz (`on_boot` Priorität -10) mit Peer-Wiederherstellung aus dem Cache, verzögertem Discovery-Broadcast und LED-Hardware-Selbsttest.

> 👉 *Ausführliche Paket-Strukturen, C++ Architekturdetails, Vorher-Nachher-Codebeispiele, Stabilitätsmaßnahmen und das Boot-Ablaufdiagramm siehe [📄 Code-Architektur & Wartbarkeit Guide](documentation/de/de_code-architecture-and-maintainability.md).*

---

## 🚀 Automatisierte Release & Versionierung

Um eine zuverlässige Software-Pflege und vollständige Nachvollziehbarkeit jeder Änderung sicherzustellen, nutzt das Projekt einen automatisierten Release-Workflow:

- **KI-gestützte Changelogs**: Jedem Release geht eine automatisierte Analyse der Code-Änderungen voraus. Ein KI-Assistent generiert detaillierte Einträge für die `CHANGELOG.md` und aktualisiert die Firmware-Beschreibung in `version.json`.
- **Automatischer Version-Bump**: Die Versionierung folgt einem strengen Muster, bei dem die Patch-Version (z. B. `0.8.251` → `0.8.252`) während des Build-Prozesses automatisch erhöht wird.
- **Git Integration**: Erfolgreiche Builds werden automatisch committed und in das Repository gepusht, wodurch sichergestellt wird, dass das GitHub-Manifest und die Binär-Releases immer synchron mit dem lokalen Entwicklungsstand sind.
- **Kontinuierliche Transparenz**: Die aktuelle Version ist als Sensor in Home Assistant verfügbar und wird zur einfachen Überprüfung auf dem lokalen Web-Dashboard angezeigt.

---

## 🙏 Danksagungen / Credits

Ein besonderer Dank gilt **[patrickcollins12](https://github.com/patrickcollins12)** für sein hervorragendes Projekt **[ESPHome Fan Controller](https://github.com/patrickcollins12/esphome-fan-controller)**. Seine Implementierung und Erläuterungen zur Nutzung des [ESPHome PID Climate](https://esphome.io/components/climate/pid/) Moduls für geräuschlose (lautlose) stufenlose PWM-Lüftersteuerungen dienten als maßgebliche Inspiration und Grundlage für die CO2- und Feuchtigkeitsautomatik in diesem Projekt.

---

## ⚠️ Sicherheitshinweise

> [!CAUTION]
> **230V-Netzspannungsgefahr:** Während die VentoSync Steuerlogik und der Lüfterkreis im sicheren Kleinspannungsbereich (12V / 3,3V DC) arbeiten, ist das interne Netzteil direkt an die **230V-Netzspannung** angeschlossen.
> 
> Schalte vor dem Öffnen des Gehäuses unbedingt den Leitungsschutzschalter am Sicherungskasten spannungsfrei. Installation und 230V-Netzanschluss **MÜSSEN zwingend durch eine qualifizierte Elektrofachkraft** gemäß den geltenden VDE- und Sicherheitsvorschriften ausgeführt werden!
> 
> *Bitte beachte die spezifischen Installations- und Sicherheitsrichtlinien im Abschnitt [PCB-Montage](#-pcb-montage--lüfter-verdrahtung) sowie im [Hardware- & Verdrahtungs-Guide](documentation/de/de_hardware-and-wiring.md).*

---

## ⚖️ Rechtlicher Haftungsausschluss

Dieses Projekt ist eine unabhängige Open-Source-Entwicklung. Es steht in **keiner** Verbindung zu der **VentoMaxx GmbH** und wird von dieser weder unterstützt noch empfohlen. Die Verwendung des Markennamens „VentoMaxx“ dient ausschließlich Identifikations- und Kompatibilitätszwecken.

Obwohl alle Anstrengungen unternommen wurden, um die Sicherheit und Funktionalität dieser Firmware und des zugehörigen PCB-Designs zu gewährleisten, übernimmt der Endbenutzer die volle Verantwortung für die Installation, Verkabelung und Nutzung. Modifikationen an Ihrem Lüftungsgerät können zum Erlöschen der Garantie führen und sollten nur von qualifiziertem Fachpersonal durchgeführt werden.

---

## 📜 Lizenz

Dieses Projekt steht unter der [GNU General Public License v3.0 (GPLv3)](LICENSE).
Feel free to fork & improve!

---

**Made with ❤️ and ESPHome**
