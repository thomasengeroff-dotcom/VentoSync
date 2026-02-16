# Brainstorming: KI-gestützte Lüftungssteuerung

## Grundidee

Eine intelligente, dezentrale Wohnraumlüftung, die nicht nur auf aktuelle Sensorwerte reagiert, sondern proaktiv handelt. Das System lernt aus historischen Daten (Innenraum) und externen Prognosen (Wetter), um die Effizienz zu maximieren und den Wohnkomfort zu steigern (z.B. Hitzeschutz im Sommer, Feuchteschutz im Winter).

## Kernkonzepte

### 1. Datenerfassung & Speicherung (Local Intelligence)

Das Gerät agiert als "Black Box" Datenlogger für seinen spezifischen Raum.

* **Interne Sensoren:**
  * CO2 (SCD41)
  * Temperatur & Luftfeuchtigkeit Innen (SCD41/BME680)
  * Temperatur & Luftfeuchtigkeit Außen (NTCs + Berechnung)
  * Luftdruck (BME680)
  * VOC / Luftgüte (BME680)
* **Aktor-Status:**
  * Aktuelle Lüfterstufe (RPM)
  * Laufzeit & Modus (Wärmerückgewinnung vs. Durchzug)
* **Speicherlösung:**
  * **In-Memory DB:** Ringspeicher im RAM für kurzfristige Trends (z.B. letzte 1-2 Stunden für schnelle Regelung).
  * **Persistente Speicherung:** Lokaler Flash (LittleFS) oder SD-Karte für Langzeitdaten (Tage/Wochen), um Muster zu erkennen (z.B. "CO2 steigt immer morgens um 7:00").

### 2. Externe Datenquellen (Context Awareness)

Das System ist nicht isoliert, sondern kennt die Umweltbedingungen.

* **Integration:** Über Home Assistant (HA) oder direkte API-Calls (falls WiFi permanent an).
* **Datenpunkte:**
  * **Wettervorhersage:** "Morgen wird es 35°C heiße" -> Heute Nacht maximal auskühlen (Night Cooling).
  * **Luftqualität Außen:** "Hohe Pollenbelastung oder Feinstaub" -> Lüftung reduzieren oder filtern.
  * **Anwesenheit:** HA meldet "Niemand zuhause" -> Lüftung auf Minimum/Standby.

### 3. KI / Smart Logic (Proactive Control)

Statt starrer `IF/THEN` Regeln nutzt das System adaptive Algorithmen.

#### Szenario: Sommerlicher Hitzeschutz (Smart Cooling)

* **Problem:** Normale Lüftung würde warme Außenluft reinholen, wenn nur CO2 betrachtet wird.
* **KI-Ansatz:**
  * System erkennt: "Außentemperatur > Innentemperatur" UND "Wetterbericht sagt Hitzewelle voraus".
  * **Aktion:** Lüftung tagsüber auf absolutes Minimum (nur Feuchteschutz/CO2-Notfall).
  * **Proaktiv:** Sobald Außentemperatur < Innentemperatur (nachts), boostet das System automatisch ("Free Cooling"), um das Gebäude als Kältespeicher aufzuladen.

#### Szenario: Feuchte-Management (Dew Point Logic)

* **Problem:** Lüftung im Sommerkeller holt Feuchtigkeit rein (Kondensatschimmel).
* **KI-Ansatz:** Berechnung des Taupunkts innen vs. außen.
* **Aktion:** Lüftung stoppt, wenn absolute Luftfeuchte außen > innen.

## Lösungsansätze & Architektur

### Ansatz A: Edge AI (Alles auf dem ESP)

* **Hardware:**
  * ESP32-S3 (mit PSRAM für größere Modelle/Datenbanken) oder ESP32-C6 (RISC-V, effizient).
  * SD-Karten-Slot für massives Logging.
* **Software:**
  * **TensorFlow Lite for Microcontrollers:** Trainiertes Modell (z.B. am PC) läuft auf dem ESP.
  * **Edge Impulse:** Plattform zum Trainieren von Modellen anhand der Sensordaten.
* **Vorteil:** Autark, funktioniert auch ohne WLAN/Server kurzzeitig.
* **Nachteil:** Begrenzte Rechenleistung, komplexes Training.

### Ansatz B: Hybrid Intelligence (ESP + Home Assistant)

* **Hardware:**
  * Bleibt schlank (ESP32 Classic/C3/C6).
* **Software:**
  * ESP sammelt Daten und puffert sie.
  * Home Assistant (mit Add-ons wie InfluxDB + AI-Integrationen) übernimmt die schwere "Denkarbeit" und Analyse.
  * HA sendet optimierte "Fahrpläne" oder Steuerbefehle an den ESP.
* **Vorteil:** Massive Rechenpower verfügbar, einfache Integration von Wetterdaten.
* **Nachteil:** Abhängig von HA-Verfügbarkeit.

### Ansatz C: Regel-basierte "KI" (Fuzzy Logic)

* Kein neuronales Netz, sondern unscharfe Logik.
* **Beispiel:** Statt `IF Temp > 25`, eher Gewichtung: `WÄRME_LAST` (Hoch) + `CO2_LAST` (Mittel) = `LÜFTUNG` (Niedrig, aber nicht aus).
* Kann lokal auf dem ESP effizient berechnet werden.

## Hardware-Implikationen

1. **Speicher:** Wechsel auf ESP32 mit **PSRAM** empfohlen, wenn Datenbank im RAM laufen soll.
2. **RTC (Real Time Clock):** Für exaktes Logging und Zeitpläne ohne WLAN.
3. **SD-Karte:** Optional für lokales Blackbox-Logging.
4. **Rechenleistung:** ESP32-S3 wäre ideal für On-Device AI Aufgaben.

## Nächste Schritte (Machbarkeit)

1. **Daten sammeln:** Aktuelle Sensordaten über Wochen aufzeichnen (InfluxDB via HA).
2. **Muster analysieren:** Gibt es wiederkehrende Muster, die eine KI lernen könnte?
3. **Prototyping:**
    * Einfache `Predictive Cooling` Logik in ESPHome implementieren (basiert auf Wettervorhersage von HA).
    * Testen von `TinyML` Modellen für Anomalieerkennung (z.B. "Lüfterlager defekt" durch Vibrationsanalyse via IMU).
