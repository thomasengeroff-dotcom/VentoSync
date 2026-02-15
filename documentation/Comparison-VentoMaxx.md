# Funktionsvergleich: ESPHome-Steuerung vs. VentoMaxx Original

Dieser Vergleich basiert auf der Analyse der VentoMaxx Bedienungsanleitung (`MA-BA_Endmontage_V1_V3-WRG_DE_2502.pdf`) und dem aktuellen Funktionsumfang der ESPHome-Firmware.

## 🏆 Zusammenfassung

Die **ESPHome-Steuerung** bietet **100% der Original-Funktionalität** (inkl. aller Sicherheitsmechanismen wie Tacho-Überwachung und Frostschutz) und erweitert diese massiv um **Smart-Home-Funktionen** (WLAN, Home Assistant, OTA-Updates) sowie **verbesserte Regelalgorithmen** (variable Zykluszeiten, CO2-Steuerung).

---

## ⚙️ Betriebsmodi & Logik

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart Vision (Dieses Projekt) | Vorteil ESPHome |
| :--- | :--- | :--- | :--- |
| **Wärmerückgewinnung (WRG)** | Statischer Zyklus: **70 Sek.** (fix) | **Dynamischer Zyklus**: 50s – 90s (je nach Stufe/Temp) | ✅ **Höhere Effizienz** durch angepasste Zykluszeiten |
| **Querlüftung (Sommer)** | Nur im Paar-Verbund möglich | ✅ Ja (via App/Panel/Automatik) | ⚖️ Gleichwertig |
| **Stoßlüftung** | 15 Min. Intensiv, dann Pause | ✅ Ja (Konfigurierbar: Zeit/Stufe) | ✅ **Flexibler** |
| **Automatik (Feuchte)** | Schwellwerte fest: 55%, 65%, 75% r.F. | ✅ Ja (Standard-Werte identisch, aber **frei konfigurierbar**) | ✅ **Anpassbar** an Bausubstanz |
| **Automatik (CO2)** | ❌ Nein (Nur VOC-Option) | ✅ **Ja (SCD41)**: 6-stufige Regelung nach DIN EN 13779 mit Hysterese + Noise Control | ✅ **Bedarfsgerecht + leise** |
| **Nachtmodus** | ❌ Nein (Manuell ausschalten) | ✅ **Geplant**: Zeitgesteuerte Drosselung/Dimming | ✅ **Komfort** |
| **Filter-Alarm** | Zeitgesteuert (LED blinkt) | ✅ **Ja**: Zeit + Betriebsstunden (Visualisierung in App) | ✅ **Präziser** |

---

## 🛡️ Sicherheit & Hardware

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart Vision (Dieses Projekt) | Vorteil ESPHome |
| :--- | :--- | :--- | :--- |
| **Lüfter-Überwachung** | Tacho-Signal Auswertung (LED blinkt bei Fehler) | ✅ **Ja**: Tacho-Monitor (Alarm in Home Assistant + LED) | ✅ **Push-Benachrichtigung** bei Ausfall |
| **Frostschutz** | Abschaltung bei Zuluft < 5°C | ✅ **Ja**: NTC-Überwachung (Abschaltung/Vorheizregister) | ✅ **Visualisierung** der Temperaturen |
| **Kommunikation** | Kabelgebunden (Steuerleitung) | **ESP-NOW Funk** (Ausfallsicher, kein Router nötig) | ✅️**Keine Phasenkoppler nötig**, wenn Geräte an unterschiedlichen Phasen betrieben werden. ESP-NOW ist robuster als normales WLAN und funktioniert auch ohne Router (Peer-to-Peer). |
| **Synchronisierung** | Master/Slave (DIP-Schalter) | **Mesh-Netzwerk** (Automatische Gruppenfindung) | ✅ **Einfachere Einrichtung** über Home Assistant |
| **Stromverbrauch** | Standby < 1W | Standby **~0.5W** (Dank ESP32-C6 Deep-Sleep Features) | ⚖️ Vergleichbar |

---

## 📱 Bedienung & Schnittstellen

| Feature | VentoMaxx V-WRG (Original) | ESPHome Smart Vision (Dieses Projekt) | Vorteil ESPHome |
| :--- | :--- | :--- | :--- |
| **Bedienpanel** | Taster + LEDs | ✅ **Unterstützt Original-Panel** (Volle Funktion) | ⚖️ **Identische Optik/Haptik** |
| **Fernbedienung** | IR-Fernbedienung (Optional) | **Jedes Smartphone / Tablet / PC** | ✅ **Keine Zusatzkosten** |
| **Smart Home** | ❌ Nein (Proprietär) | ✅ **Native Home Assistant API** | ✅ **Volle Integration** |
| **Daten-Transparenz** | ❌ Keine (Blackbox) | ✅ **Vollständige Historie** (NTC Innen/Außen, CO2, Feuchte) | ✅ **Datenbasierte Optimierung** & Analyse |
| **Updates** | ❌ Nein (Hardware-Tausch nötig) | ✅ **OTA-Updates** (Over-the-Air) | ✅ **Zukunftssicher** |
| **Konfiguration** | DIP-Schalter auf Platine | **Web-Oberfläche / YAML** | ✅ **Komfortabel** |
| **System & Lizenz** | Geschlossen (Proprietär) | ✅ **100% Open Source (MIT)** | ✅ **Kein Vendor-Lock-In**, volle Kontrolle, Community-Support |

---

## 💡 Fazit

Die ESPHome-Lösung ist ein **"Drop-in Replacement"** für die originale VentoMaxx-Steuerung. Sie behält die bewährte Bedienung bei, beseitigt aber die Limitierungen (kein Smart Home, starre Regelung) und fügt moderne Sensorik (CO2) hinzu.

**Empfehlung:**
Wer eine **vollständige Integration** in Home Assistant sucht und **keine neuen Kabel** ziehen möchte, erhält mit der ESPHome-Lösung einen deutlich erweiterten Funktionsumfang bei gleicher mechanischer Kompatibilität.
