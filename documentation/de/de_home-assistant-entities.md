# Home Assistant Entitäten: VentoSync

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_home-assistant-entities.md)


Diese Datei listet alle Home Assistant Entitäten auf, die durch die Konfiguration `ventosync.yaml` (und ihre inkludierten Packages) bereitgestellt werden. Sie ist konzeptionell nach Themen gruppiert, um eine Basis für saubere Benennung und Integration in Dashboards zu bieten.

## 1. Steuerung & Betriebsmodus (Lüftung)

Diese Entitäten dienen der primären Steuerung des Lüfters und des Betriebsmodus.

* **`select.luefter_modus`** ("Lüftermodus")
  * *Typ:* Select
  * *Dokumentation:* Auswahl des Betriebsmodus: `Smart-Automatik`, `Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung`, `Aus`. Details: [Betriebsmodi](de_operating-modes.md).
* **`number.fan_intensity_display`** ("Lüfter Intensität")
  * *Typ:* Number (Slider)
  * *Dokumentation:* Manuelle Einstellung der Lüfterstufe (1 bis 10). Wird im Automatik-Modus durch den PID-Regler gesteuert.
* **`sensor.fan_pwm_percent`** ("Lüfter PWM")
  * *Typ:* Sensor
  * *Dokumentation:* Zeigt den aktuell berechneten PWM-Wert des Lüfters in Prozent an (0-100%). 50% entspricht dem Motor-Stopp.
* **`sensor.fan_rpm`** ("Lüfter Drehzahl")
  * *Typ:* Sensor
  * *Dokumentation:* Zeigt die Drehzahl des Lüfters in RPM an. Falls kein physischer Tacho-Sensor (4-Pin Lüfter) angeschlossen ist, wird ein berechneter Wert basierend auf der Leistungsstufe ausgegeben (100% = 4200 RPM).
* **`text_sensor.direction_display`** ("Aktuelle Luftrichtung")
  * *Typ:* Text Sensor
  * *Dokumentation:* Zeigt die momentane Richtung des Luftstroms an: "Zuluft (Rein)", "Abluft (Raus)" oder "Stillstand".

## 2. Automatik & Regelung (Sensoren-getrieben)

Mit diesen Entitäten wird das Verhalten des "Smart-Automatik" Modus konfiguriert.

* **`number.auto_co2_threshold`** ("Automatik: CO2 Grenzwert")
  * *Typ:* Number (Slider)
  * *Dokumentation:* Setzt den Ziel-CO2-Wert (Setpoint) in ppm für den PID-Regler. Die CO2-Regelung ist im Automatik-Modus immer aktiv und hat Priorität vor der Feuchtigkeitsregelung.
* **`number.auto_humidity_threshold`** ("Automatik: Feuchte Grenzwert")
  * *Typ:* Number (Slider)
  * *Dokumentation:* Ein optionaler Feuchtigkeitsgrenzwert (%), ab dem die Lüftung ebenfalls hochregelt.
* **`number.automatik_co2_min_luefterstufe`** ("Automatik Min Lüfterstufe")
  * *Typ:* Number (Slider)
  * *Dokumentation:* Grundlüftung (Schimmelschutz) im Automatikmodus, unter die der Lüfter nicht fällt (Stufe 1-10).
* **`number.automatik_co2_max_luefterstufe`** ("Automatik Max Lüfterstufe")
  * *Typ:* Number (Slider)
  * *Dokumentation:* Obergrenze (z. B. als Lärmschutz) für den Automatikmodus (Stufe 1-10).
* **`number.radar_lufter_anpassung`** ("Radar Lüfter-Anpassung", YAML-ID `auto_presence_slider`)
  * *Typ:* Number (Slider, -5 … +5, `0` = aus), raumweit
  * *Dokumentation:* Stufen, die in den **manuellen** Modi zur Grundstufe addiert werden, solange irgendwo im Raum Radar-Anwesenheit erkannt wird (LD2450 eines beliebigen Geräts, per ESP-NOW geteilt). Ohne Anwesenheit keine Anpassung, in der Smart-Automatik nie.

### Klima-Koordination (Smart Climate Control)

Modifikator für die `Smart-Automatik`, solange die Raumklimaanlage aktiv ist. Vollständige Beschreibung: [📄 Intelligente Klimaanlagen-Koordination](de_smart-climate-control.md).

* **`switch.klima_koordination`** ("Klima-Koordination", YAML-ID `smart_climate_control`)
  * *Typ:* Switch (Config, persistent), raumweit
  * *Dokumentation:* Aktiviert die HVAC-Koordination für den ganzen Raum (per ESP-NOW mit allen Peers abgeglichen). Solange Home Assistant die Klimaanlage als aktiv meldet (API-Action `set_ac_active`, an ein beliebiges Gerät des Raums), wechselt die Automatik auf eine reine CO2-Regelung mit gelockertem Sollwert, Stufenbegrenzung und erzwungener Wärmerückgewinnung. Standard: aus.
* **`number.klima_koordination_co2_grenzwert`** ("Klima-Koordination: CO2 Grenzwert", YAML-ID `hvac_co2_threshold`)
  * *Typ:* Number (Slider, 800–1500 ppm), raumweit
  * *Dokumentation:* Gelockerter CO2-Sollwert bei aktiver Klimaanlage (Standard 1200 ppm). Zugleich Freigabeschwelle des CO2-Notfalls. Wie die beiden anderen HVAC-Slider ein **raumweiter** Wert: Eine Änderung an einem Gerät wird per ESP-NOW an alle Peers des Raums synchronisiert.
* **`number.klima_koordination_max_lufterstufe`** ("Klima-Koordination: Max Lüfterstufe", YAML-ID `hvac_max_fan_level`)
  * *Typ:* Number (Slider, 1–5)
  * *Dokumentation:* Harte Lüfterstufen-Obergrenze bei aktiver Klimaanlage (Standard 3). Das Minimum ist immer Stufe 1.
* **`number.klima_koordination_co2_notfallgrenze`** ("Klima-Koordination: CO2 Notfallgrenze", YAML-ID `hvac_emergency_co2`)
  * *Typ:* Number (Slider, 1200–2000 ppm)
  * *Dokumentation:* CO2-Wert, ab dem die normale Automatikregelung unabhängig vom Klima-Status greift (Standard 1500 ppm, mindestens 100 ppm über dem gelockerten Sollwert).
* **`text_sensor.klima_koordination_status`** ("Klima-Koordination Status", YAML-ID `hvac_status`)
  * *Typ:* Textsensor (Diagnose)
  * *Dokumentation:* Aktueller Koordinator-Zustand: `Deaktiviert`, `Inaktiv (kein Smart-Automatik)`, `Bereit (Klima aus)`, `Aktiv (gedrosselt)`, `Notfall (CO2)`, `Notfall (Feuchte)`, `Ausgesetzt (kein CO2-Wert im Raum)`. Geräte ohne CO2-Sensor nutzen den per ESP-NOW geteilten CO2-Wert eines Peers.
* **`binary_sensor.klima_aktiv_ha_signal`** ("Klima aktiv (HA-Signal)", YAML-ID `hvac_ac_active`)
  * *Typ:* Binary-Sensor (Diagnose)
  * *Dokumentation:* Zuletzt von Home Assistant per API-Action `set_ac_active` an dieses Gerät gesendeter Klima-Status (läuft ohne neue Meldung nach 15 min ab). Der Koordinator berücksichtigt auch den Klima-Status, den ein Peer des Raums erhalten hat.

### Fenstersperre

Raumweiter Lüftungsstopp, solange ein Fenster offen ist. Einrichtung: [📄 Fenstersperre einrichten](de_window-guard-ha-setup.md).

* **`binary_sensor.fenster_offen_ha_signal`** ("Fenster offen (HA-Signal)", YAML-ID `window_locked`)
  * *Typ:* Binary-Sensor (Diagnose)
  * *Dokumentation:* Zuletzt von Home Assistant per API-Action `set_window_open` an dieses Gerät gesendeter Fensterstatus (läuft ohne neue Meldung nach 15 min ab → gilt als geschlossen). Die Sperre berücksichtigt auch den Fensterstatus, den ein Peer des Raums erhalten hat.
* **`text_sensor.fenstersperre_aktiv`** ("Fenstersperre Aktiv", YAML-ID `window_guard_status`)
  * *Typ:* Textsensor (`Ja` / `Nein`)
  * *Dokumentation:* Resultierende raumweite Sperre (greift nach 5 s „offen").
* **`switch.fenstersperre_ignorieren`** ("Fenstersperre ignorieren", YAML-ID `ignore_window_guard_switch`)
  * *Typ:* Switch (Config, persistent, pro Gerät)
  * *Dokumentation:* Nimmt dieses Gerät von der Fenstersperre aus.

## 3. Zeiten & Intervalle

* **`number.vent_timer`** ("Durchlüften Dauer (min)")
  * *Typ:* Number (0–120 min, Schrittweite 5, Standard 30), raumweit
  * *Dokumentation:* Dauer des manuellen Modus `Durchlüften`; danach kehrt der Raum in die `Wärmerückgewinnung` zurück (0 = Dauerbetrieb). Wird von `Stoßlüftung` (fester 15/105-min-Zyklus) und vom automatischen Sommer-Bypass nicht verwendet.
* **`number.sync_interval_config`** ("Sync Intervall")
  * *Typ:* Number
  * *Dokumentation:* Definiert den Rhythmus zum Abgleich der Gerätegruppen über WLAN / ESP-NOW (in Minuten).
* **`number.led_max_brightness_config`** ("Maximale LED Helligkeit")
  * *Typ:* Number (Slider)
  * *Dokumentation:* Stellt die maximale Helligkeit der Status-LEDs am Gerät ein (5-100%).

## 4. Sensordaten & Klima

Diese Entitäten repräsentieren die von den Hardware-Sensoren gelesenen Werte sowie berechnete Klimadaten.

### Kombinierte & Berechnete Werte
* **`sensor.effective_co2`** ("Effektiver CO2 Wert")
  * *Typ:* Sensor
  * *Dokumentation:* Der primäre CO2-Wert für die Regelung. Nutzt bevorzugt den SCD43 (echtes CO2), fällt bei dessen Fehlen aber automatisch auf den BME680 (CO2eq) zurück.
* **`text_sensor.effective_co2_bewertung`** ("CO2 Bewertung")
  * *Typ:* Text Sensor
  * *Dokumentation:* Qualitative Einstufung des aktuellen CO2-Werts (z.B. "Ausgezeichnet", "Mangelhaft").
* **`sensor.heat_recovery_efficiency`** ("Wärmerückgewinnung Effizienz")
  * *Typ:* Sensor (Template)
  * *Dokumentation:* Berechnet den energetischen Wirkungsgrad (%) basierend auf den Temperaturdifferenzen zwischen Innen, Außen und Zuluft.
* **`text_sensor.wrg_reference_sensor`** ("WRG Referenz-Messpunkt")
  * *Typ:* Text Sensor
  * *Dokumentation:* Gibt an, welcher Sensor aktuell als Referenz für die Raumtemperatur dient (SCD43 oder BME680).

### Primäre Sensoren (Hardware)
* **SCD43 (Präzisions-CO2):**
  * `sensor.scd41_co2` (CO2 ppm)
  * `sensor.scd41_temperature` (Temp °C)
  * `sensor.scd41_humidity` (Feuchte %)
* **BME680 (Umwelt & IAQ):**
  * `sensor.iaq_co2eq` (Luftqualität CO2-Äquivalent)
  * `sensor.bme680_temperature` (Temp °C)
  * `sensor.bme680_humidity` (Feuchte %)
  * `sensor.bme680_pressure_relative` (Relativer Luftdruck hPa)
  * `sensor.bme680_dewpoint` (Taupunkt °C)
  * `sensor.iaq_trend` (IAQ Trend ppm/min)
  * `text_sensor.iaq_level` (IAQ Bewertung)
* **BMP390 (Präzisions-Luftdruck):**
  * `sensor.pressure` (Luftdruck hPa)
  * `sensor.bmp390_temperature` (Temp °C)
* **Rohr-Sensoren (NTC):**
  * `sensor.temp_zuluft` ("Temperatur Zuluft") - Misst die einströmende Luft im Rohr.
  * `sensor.temp_abluft` ("Temperatur Abluft") - Misst die ausgeblasene Luft vor der Außenwand.

## 5. Radar / Anwesenheit (HLK-LD2450)

* **`binary_sensor.radar_presence`** ("Radar Presence")
  * *Typ:* Binary Sensor
  * *Dokumentation:* Allgemeine Bewegungserkennung (Aktiv/Inaktiv).
* **Weitere Targets:**
  * `binary_sensor.radar_moving_target`
  * `binary_sensor.radar_still_target`
  * `sensor.radar_total_target_count`

## 6. Wartung & Hardware

* **`binary_sensor.filter_change_alarm`** ("Filterwechsel Alarm")
  * *Typ:* Binary Sensor
  * *Dokumentation:* Wird aktiv, sobald die errechneten Betriebstage den Wartungsintervall überschreiten.
* **`sensor.filter_operating_days_sensor`** ("Filter Betriebstage")
  * *Typ:* Sensor (Template)
  * *Dokumentation:* Zähler der aktiven Betriebstage seit dem letzten Filterwechsel.
* **`button.filter_reset_btn`** ("Filter gewechselt (Reset)")
  * *Typ:* Button
  * *Dokumentation:* Setzt den Betriebstage-Zähler nach einem physischen Filtertausch zurück auf 0.
* **`button.bme680_basiswert_zuruecksetzen`** ("BME680 Basiswert zurücksetzen")
  * *Typ:* Button
  * *Dokumentation:* Setzt den internen Basiswert für die Gas-Messung des BME680 zurück (erfordert neue 48h Einlaufphase).
* **`button.force_restart_button`** ("ESP Neustart erzwingen")
  * *Typ:* Button
  * *Dokumentation:* Löst einen sofortigen Hardware-Neustart des ESP32 aus.

## 7. Status LEDs & Visuelles Feedback

Die physischen LEDs auf dem Controller-Interface.

* `light.status_led_power` ("Status LED Power")
* `light.status_led_master` ("Status LED Master" - mit "Error Blink" Warnungen)
* `light.status_led_mode_wrg` ("Status LED Mode WRG" - mit "Automatik Pulse")
* `light.status_led_mode_vent` ("Status LED Mode Vent")
* `light.status_led_l1` bis `light.status_led_l5` ("Status LED L1" - "L5" für die Anzeige der Intensität)

## 8. Setup & Device Configuration

(Achtung: Diese Entitäten sind primär für die Erstkonfiguration nach dem Einsetzen der Hardware gedacht)

* **`number.config_floor_id`**, **`number.config_room_id`**, **`number.config_device_id`**
  * *Typ:* Number
  * *Dokumentation:* Adresskonfiguration des Gerätes für die Gruppensynchronisation via ESP-NOW. Geräte mit dem gleichen Floor+Room bilden eine Multi-Device Gruppe.
* **`select.config_phase`** ("Geräte-Phase (A|B)")
  * *Typ:* Select
  * *Dokumentation:* Ändert den initialen Lüfter-Rhythmus für Push-Pull-Paare (Phase A fängt Zuluft-orientiert an, Phase B Abluft-orientiert).
* **`text_sensor.device_config_summary`** ("Geräte-Konfiguration")
  * *Typ:* Text Sensor
  * *Dokumentation:* Zeigt eine zusammenfassende Status-Zeile der aktuellen Ids und Phasen an.
* **`button.auf_standardwerte_zuruecksetzen`** ("Auf Standardwerte zurücksetzen")
  * *Typ:* Button
  * *Dokumentation:* Setzt Floor-, Room- und Device-ID auf Werkseinstellungen zurück.
* **`button.force_espnow_discovery`** ("Force ESPNOW Discovery")
  * *Typ:* Button
  * *Dokumentation:* Triggert manuell die Suche nach anderen Peers im gleichen Raum.

### Diagnose & System-Status
* **`text_sensor.own_mac_address`** ("Eigene MAC Adresse"): Zur eindeutigen Identifizierung der Hardware.
* **`text_sensor.espnow_peers_display`** ("ESP-NOW Peers"): Liste der aktuell synchronisierten MAC-Adressen in der Gruppe.
* **`sensor.current_floor_id`**, **`sensor.current_room_id`**, **`sensor.current_device_id`**: Diagnose-Sensoren, die die aktuell im Controller aktiven IDs spiegeln.

## 9. Flash-Speicher & Lebensdauer (NVS)

Um den Flash-Speicher des ESP32 vor vorzeitigem Verschleiß (Wear-Out) zu schützen, nutzt die Firmware für hochfrequente Daten eine **Tiered-Storage-Strategie**:

* **Echtzeit (RAM):** Daten werden in Echtzeit verarbeitet und sind im Dashboard aktuell (z.B. Betriebsstunden, BME680 Burn-In Counter).
* **Sync-Intervall (NVS):** Der Schreibvorgang in den permanenten Speicher erfolgt nur alle **30 Minuten**.
* **Konfiguration:** Manuelle Einstellungsänderungen (Modus, Zielwerte) werden weiterhin **sofort** gespeichert.

> [!NOTE]
> Nach einem plötzlichen Stromausfall können maximal die Daten der letzten 29 Minuten verloren gehen. Dies ist ein bewusster Design-Kompromiss zur Maximierung der Hardware-Lebensdauer auf >10 Jahre.
