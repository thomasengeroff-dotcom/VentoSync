# Funktionsvergleich: ESPHome-Steuerung vs. VentoMaxx Original

Dieser Vergleich basiert auf der Analyse der VentoMaxx Bedienungsanleitung (`MA-BA_Endmontage_V1_V3-WRG_DE_2502.pdf`) und dem aktuellen Funktionsumfang der ESPHome-Firmware (Stand: März 2026).

## 🏆 Zusammenfassung

Die **ESPHome-Steuerung** bietet **100% der Original-Funktionalität** (inkl. aller Sicherheitsmechanismen wie Tacho-Überwachung und Frostschutz) und erweitert diese massiv um:

- **Smart-Home-Integration** (WiFi 6, Home Assistant Native API, OTA-Updates)
- **Komfort & Automatik** (Feuchtigkeitsmanagement, Fenstersperre, Silent-Mode, Sanftanlauf)
- **Präzisions-Sensorik** (SCD41 CO2, BMP390 Luftdruck, HLK-LD2450 Radar, NTC-Temperaturen, BME680 Fallback)
- **Intelligente Regelalgorithmen** (PID-gesteuerte CO2/Feuchte-Regelung, adaptive Zykluszeiten)
- **Prädiktive Wartung** (Filterwechsel-Alarm basierend auf Betriebsstunden + Kalenderzeit)
- **Kabelloses Mesh-Netzwerk** (ESP-NOW Discovery für autonomen / routerunabhängigen Gruppenbetrieb)
- **Hardware-Exzellenz** (Custom PCB, Traco-Power Safety, hocheffiziente DC/DC Wandler)

---

## ⚙️ Betriebsmodi & Logik

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart WRG (Dieses Projekt) | Vorteil ESPHome |
| :--- | :--- | :--- | :--- |
| **Wärmerückgewinnung (WRG)** | Statischer Zyklus: **70 Sek.** (fix) | **Dynamischer Zyklus**: 50s – 90s (je nach Stufe/Temp) | ✅ **Höhere Effizienz** durch angepasste Zykluszeiten |
| **Querlüftung (Sommer)** | Nur im Paar-Verbund möglich | ✅ Ja (via App/Panel/Automatik) | ⚖️ Gleichwertig |
| **Auto-Sommerbetrieb** | ❌ Nein (manuell umschalten) | ✅ **Automatische Querlüftung** wenn Außentemp. < Innentemp. (NTC + ESP-NOW Gruppendaten) | ✅ **Kein manuelles Umschalten nötig** |
| **Stoßlüftung** | 15 Min. Intensiv, dann Pause | ✅ Ja (Konfigurierbar: Zeit/Stufe) | ✅ **Flexibler** |
| **Lüfterregelung** | 5 feste Stufen | **10 Stufen + stufenlose PID-Regelung** (Stufen 1 - 6 sind feiner abgestimmt als die höheren Stufen, um die Lautstärke in den unteren Stufen zu optimieren) | ✅ **Feingranular** |
| **Automatik (CO2)** | ❌ Nein (nur optionale VOC-Schätzung) | ✅ **SCD41 (echtes CO2)**: Stufenlose PID-Regelung mit Deadband-Hysterese, konfigurierbarem Min/Max-Level | ✅ **Präzise & leise** |
| **Automatik (Feuchte)** | Schwellwerte fest: 55%, 65%, 75% r.F. | ✅ **PID-Regler** mit konfigurierbarem Grenzwert (40-100%), Outdoor-Feuchte-Vergleich, Deadband-Hysterese (±2%) | ✅ **Anpassbar** + Outdoor-Check |
| **Anwesenheit** | ❌ Nein | ✅ **mmWave-Radar (HLK-LD2450)**: 4 Profile (Keine Anpassung, Intensiv, Normal, Gering): je Raum kann Lüftungsintensität automatisch verringert oder erhöht werden bei Anwesenheit | ✅ **Bedarfsgerecht** |
| **Fenstersperre** | ❌ Nein | ✅ **Window Guard**: Lüftung wird nach 5s Delay-Stopp bei offenem Fenster angehalten (raumweit via ESP-NOW) und setzt bei geschlossenen Fenstern automatisch im vorigen Modus fort | ✅ **Energieersparnis** |
| **Sanftanlauf** | ✅ fest implementiert | ✅ **Slew-Rate Limiter**: Sanfte Übergänge (~5%/s) bei Drehzahländerungen | ⚖️ Gleichwertig |
| **Phasen-Kontinuität** | ✅ fest implementiert | ✅ **Intelligente Skalierung**: Nahtlose Fortführung bei Intensitätswechseln | ⚖️ Gleichwertig |
| **Nachtmodus** | ❌ Nein (manuell ausschalten) | 📋 Geplant: Zeitgesteuerte Drosselung/Dimming & Lichtsensor-Support | ✅ **Komfort** (in Planung) |

---

## 🛡️ Sensorik & Monitoring

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart WRG (Dieses Projekt) | Vorteil ESPHome |
| :--- | :--- | :--- | :--- |
| **CO2-Messung** | ❌ Nein (nur optionales VOC-Modul) | ✅ **SCD41**: Photoacoustic sensing, 400-5000 ppm | ✅ **Echte CO2-Werte** statt Schätzung |
| **Temperatur** | ❌ Keine Anzeige | ✅ **NTC-Sensoren** (Zuluft/Abluft) + HA Wetterdaten | ✅ **Visualisierung & Automatik** |
| **Luftfeuchtigkeit** | Rudimentär (feste Schwellen) | ✅ **SCD41** (Innen) + **Außenfeuchte** (HA Wetterdienst/Sensor) | ✅ **PID-geregelt + Outdoor-Check** |
| **Luftdruck** | ❌ Nein | ✅ **BMP390**: Wettertrend, Sturmwarnung, **Barometrische Kompensation** für SCD41 | ✅ **Korrekturen in Echtzeit** |
| **BME680 Fallback** | ❌ Nein | ✅ **Zusatz-Sensor**: Automatisches Backup bei SCD41 Ausfall | ✅ **Fehlertoleranz** |
| **Anwesenheit** | ❌ Nein | ✅ **HLK-LD2450 mmWave-Radar**: Präsenz, Moving/Still Targets, Target Count | ✅ **Raumgenaue Erfassung** |
| **Drehzahl** | Tacho-Signal (LED blinkt bei Fehler) | ✅ **Pulse Counter**: Virtueller Tacho (Fallback) oder Echtzeit-RPM + Fehleralarm | ✅ **Quantitative Überwachung** |
| **Richtungsanzeige** | ❌ Nein | ✅ **Klartext-Sensor**: "Zuluft", "Abluft", "Stillstand" in HA | ✅ **Sofortige Klarheit** |

---

## 🔧 Sicherheit & Wartung

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart WRG (Dieses Projekt) | Vorteil ESPHome |
| :--- | :--- | :--- | :--- |
| **Lüfter-Überwachung** | Tacho-Signal (LED blinkt bei Fehler) | ✅ **Tacho-Monitor**: Alarm in Home Assistant + LED | ✅ **Push-Benachrichtigung** bei Ausfall |
| **Frostschutz** | Abschaltung bei Zuluft < 5°C | ✅ **NTC-Überwachung** (Abschaltung/Vorheizregister) | ✅ **Visualisierung** der Temperaturen |
| **Filterwechsel** | Timer-basiert (LED blinkt) | ✅ **Prädiktiver Alarm**: Betriebsstunden (>365d) + Kalenderzeit (>3 Jahre) + HA Push-Benachrichtigung + Reset-Button | ✅ **Präziser & proaktiv** |
| **Fehlererkennung** | LED blinkt bei Tachofehler | ✅ **Master LED Strobe**: WiFi-Ausfall, ESP-NOW Timeout (3 Min), Hitze-Warnung | ✅ **Umfassender Schutz** |
| **Thermal Guard** | ❌ Nein | ✅ **Hardware-Self-Protection**: Stop & Deep-Sleep bei >60°C Gehäuse-Temp | ✅ **Brandschutz / HW-Schutz** |
| **Stromversorgung** | Ventomaxx Netzteil | ✅ **Traco Power**: EN 60335-1 zertifiziert (Haushalt), hocheffizienter Standby | ✅ **Industriequalität** |
| **Kommunikation** | Kabelgebunden (über Stromnetz), eventuell Phasenkoppler nötig | **ESP-NOW Funk** (Ausfallsicher, kein Router nötig, 2.4GHz Unicast) | ✅ **Keine Phasenkoppler nötig** |
| **Versionierung** | Manuell / Keine | ✅ **Patch-Level-Auto-Update**: Vollautomatisches Tracking der Firmware-Version | ✅ **Zukunftssicher & Transparent & Open-Source** |
| **Synchronisierung** | Master/Slave (DIP-Schalter) | **Mesh-Netzwerk** (Auto-Discovery, PID-Demand-Sync, Settings-Mirroring), Device mit ID 1 ist Master | ✅ **Echtzeit + Drahtlos** |
| **Stromverbrauch** | Standby < 1W | Standby **~0.5W** / Stufe 1: **2.7W** / Stufe 10: **5.0W** | ✅ **Extrem Sparsam** trotz erhöhtem Funktionsumfang |

---

## 📱 Bedienung & Schnittstellen

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart WRG (Dieses Projekt) | Vorteil ESPHome |
| :--- | :--- | :--- | :--- |
| **Bedienpanel** | Taster + LEDs | ✅ **Unterstützt Original-Panel** (volle Funktion, 10 Stufen) | ⚖️ **Identische Optik/Haptik** |
| **LED Feedback** | Statisch / Hell | ✅ **Auto-Dimming**: Sanftes Ausfaden nach 60s, konfigurierbare Helligkeit | ✅ **Schlafzimmertauglich** |
| **Fernbedienung** | ❌ Nein | **Jedes Smartphone / Tablet / PC** | ✅ **Keine Zusatzkosten** |
| **Smart Home** | ❌ Nein (proprietär) | ✅ **Native Home Assistant API** | ✅ **Volle Integration** |
| **Dashboard** | ❌ Nein | ✅ **Lokal (Async Webserver)**: High-End UI mit **Tailwind CSS** | ✅ **Premium UX** |
| **Daten-Visualisierung**| ❌ Nein | ✅ **Echtzeit-Graphen**: Chart.js für CO2, Temp, Feuchte, RPM | ✅ **Transparenz** |
| **Gruppensteuerung** | Master/Slave über Kabel | ✅ **ESP-NOW Group Controller**: Alle Geräte als eine Einheit im HA Dashboard | ✅ **Intuitiv + kabellos** |
| **Netzwerk-Visualisierung** | ❌ Nein | ✅ **ESP-NOW Live-Ansicht**: Status aller Peers im Dashboard | ✅ **Echtzeit-Monitoring** |
| **Updates** | ❌ Nein (Hardware-Tausch nötig) | ✅ **OTA-Updates** (Over-the-Air) | ✅ **Zukunftssicher** |
| **Konfiguration** | DIP-Schalter auf Platine | **Web-Oberfläche / YAML / HA Slider** | ✅ **Komfortabel** |
| **System & Lizenz** | Geschlossen (proprietär) | ✅ **100% Open Source (GPL v3)** | ✅ **Kein Vendor-Lock-In** |

---

## 💡 Fazit

Die ESPHome-Lösung ist ein **"Drop-in Replacement"** für die originale VentoMaxx-Steuerung. Sie behält die bewährte Bedienung über das Originalpanel bei, beseitigt aber die Limitierungen des Originals und erweitert den Funktionsumfang um ein Vielfaches:

| Kategorie | VentoMaxx (Original) | ESPHome Smart WRG |
| :--- | :---: | :---: |
| Betriebsmodi | 3 | **5+** (inkl. Automatiken) |
| Sensorik | 0-1 (opt. VOC) | **8+** (CO2, Temp, Feuchte, Druck, Radar, Tacho, Gas, etc.) |
| Lüfterstufen | 3 / 5 | **10 + stufenlos (PID)** |
| Smart Home | ❌ | ✅ Home Assistant (Native) |
| Wartungsalarm | Timer-LED | ✅ **Prädiktiv + Push-Benachrichtigung** |
| Kommunikation | Kabel | ✅ **Drahtlos (ESP-NOW)** |
| Sicherheit | Standard | ✅ **High-End (Thermal Guard, Traco)** |
| Open Source | ❌ | ✅ GPL v3 Lizenz |

**Empfehlung:** Wer eine **vollständige Smart-Home-Integration** sucht und **keine neuen Kabel** ziehen möchte, erhält mit der ESPHome-Lösung einen massiv erweiterten Funktionsumfang bei gleicher mechanischer Kompatibilität — inklusive Präzisionssensorik, prädiktiver Wartung und intelligenter Automatik.
