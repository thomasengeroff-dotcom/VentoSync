# Home Assistant Entitäten: VentoSync

Diese Datei listet alle Home Assistant Entitäten auf, die durch die Konfiguration `ventosync.yaml` (und ihre inkludierten Packages) bereitgestellt werden. Sie ist konzeptionell nach Themen gruppiert, um eine Basis für saubere Benennung und Integration in Dashboards zu bieten.

## 1. Steuerung & Betriebsmodus (Lüftung)

Diese Entitäten dienen der primären Steuerung des Lüfters und des Betriebsmodus.

* **`select.lueftungsmodus`** ("Lüftungsmodus")
  * *Typ:* Select
  * *Dokumentation:* Ermöglicht die Auswahl des primären Betriebsmodus: `Wärmerückgewinnung`, `Stoßlüftung`, `Durchlüften`, `Automatik`, `Aus`.
* **`number.fan_intensity_display`** ("Lüfter Intensität")
  * *Typ:* Number (Slider)
  * *Dokumentation:* Manuelle Einstellung der Lüfterstufe (1 bis 10). Wird im Automatik-Modus überschrieben.
* **`number.fan_intensity_display`** ("Lüfter Intensität")
  * *Typ:* Number (Template)
  * *Dokumentation:* Zeigt den aktuell berechneten PWM-Wert des Lüfters an (bzw. lässt bei manueller test_speed_slider Steuerung Prozent zu).
* **`sensor.fan_rpm`** ("Lüfter Drehzahl")
  * *Typ:* Sensor
  * *Dokumentation:* Zeigt die Drehzahl des Lüfters in RPM an. Falls kein physischer Tacho-Sensor (4-Pin Lüfter) angeschlossen ist, wird ein berechneter Wert basierend auf der Leistungsstufe ausgegeben (100% = 4200 RPM).
* **`text_sensor.direction_display`** ("Aktuelle Luftrichtung")
  * *Typ:* Text Sensor
  * *Dokumentation:* Zeigt die momentane Richtung des Luftstroms an: "Zuluft (Rein)", "Abluft (Raus)" oder "Stillstand".
* **`sensor.ventilation_timer_remaining`** ("Verbleibende Zeit").

## 2. Automatik & Regelung (Sensoren-getrieben)

Mit diesen Entitäten wird das Verhalten des "Automatik" Modus konfiguriert.

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
* **`number.auto_presence_slider`** ("Anwesenheit Lüfter-Anpassung")
  * *Typ:* Number (Slider)
  * *Dokumentation:* Ermöglicht die Einstellung (von -5 bis +5), wie viele Stufen die Lüftung erhöht/gesenkt werden soll, wenn der Radar-Sensor eine Person im Raum erkennt.

## 3. Zeiten & Intervalle

* **`number.vent_timer`** ("Durchlüften Dauer (min)")
  * *Typ:* Number
  * *Dokumentation:* Stellt ein, wie lange (in Minuten) der Timer für die Stoß- bzw. Sommer-Querlüftung läuft, bevor das Gerät in den vorherigen Modus zurückkehrt (0 = Dauerbetrieb).
* **`number.sync_interval_config`** ("Sync Intervall")
  * *Typ:* Number
  * *Dokumentation:* Definiert den Rhythmus zum Abgleich der Gerätegruppen über WLAN / ESP-NOW.

## 4. Sensordaten & Klima

Diese Entitäten repräsentieren die von den Hardware-Sensoren gelesenen Werte.

* **`sensor.scd41_temperature`** ("SCD41 Temperatur")
  * *Typ:* Sensor (SCD4x)
  * *Dokumentation:* Innentemperatur am Bedienteil.
* **`sensor.scd41_humidity`** ("SCD41 Luftfeuchtigkeit")
  * *Typ:* Sensor (SCD4x)
  * *Dokumentation:* Innenluftfeuchtigkeit am Bedienteil.
* **`sensor.scd41_co2`** ("SCD41 CO2")
  * *Typ:* Sensor (SCD4x)
  * *Dokumentation:* Echte CO2 Messung (ppm).
* **`text_sensor.scd41_co2_bewertung`** ("SCD41 CO2 Bewertung")
  * *Typ:* Text Sensor
  * *Dokumentation:* Gibt die CO2-Qualität in Textform an (z. B. "Ausgezeichnet", "Mangelhaft").
* **`sensor.temp_zuluft`** ("Temperatur Zuluft (Innen)")
  * *Typ:* Sensor (NTC)
  * *Dokumentation:* Überwacht die Zulufttemperatur im Rohr.
* **`sensor.temp_abluft`** ("Temperatur Abluft (Außen)")
  * *Typ:* Sensor (NTC)
  * *Dokumentation:* Überwacht die Ablufttemperatur Richtung Außenwand.
* **`sensor.heat_recovery_efficiency`** ("Wärmerückgewinnung Effizienz")
  * *Typ:* Sensor (Template)
  * *Dokumentation:* Berechnet kontinuierlich den rechnerischen Wirkungsgrad (%) basierend auf Innen-, Außen- und Rohr-Temperatur differentials.
* **`sensor.temperature`** ("BMP390 Temperatur") / **`sensor.pressure`** ("BMP390 Luftdruck")
  * *Typ:* Sensor
  * *Dokumentation:* Optionaler Sensor für Luftdruck (wird zur Autokalibrierung des CO2-Sensors auf bestimmte Höhenlagen genutzt).

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
* **`button.system_reboot`** ("System Reboot")
  * *Typ:* Button
  * *Dokumentation:* Löste einen Neustart des ESP32 aus.

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
* **`switch.config_phase`** ("Geräte-Phase (A/B)")
  * *Typ:* Switch
  * *Dokumentation:* Ändert den initialen Lüfter-Rhythmus für Push-Pull-Paare (Phase A fängt pustend an, Phase B saugend).
* **`button.device_config_summary`** ("Geräte-Konfiguration")
  * *Typ:* Button
  * *Dokumentation:* Speichert und übernimmt die oben konfigurierten Gruppen-Ids (Device Builder Helper).
* **`button.auf_standardwerte_zuruecksetzen`** ("Auf Standardwerte zurücksetzen")
  * *Typ:* Button
  * *Dokumentation:* Setzt EEPROM/VLAN Werte der Raumzuordnungen zurück.

*(Zusätzlich stellt das Projekt noch Standard-Sensoren für WiFi, Uptime und IP-Informationen als Diagnose-Entitäten bereit).*
