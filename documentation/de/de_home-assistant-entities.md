# Home Assistant Entitäten: VentoSync

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_home-assistant-entities.md)


Diese Datei listet die Home-Assistant-Entitäten der VentoSync-Firmware (`ventosync.yaml` und ihre Packages), nach Themen gruppiert.

> [!IMPORTANT]
> **Entitäts-IDs:** Home Assistant bildet die Entitäts-ID aus **Gerätename + Entitätsname** (Umlaute werden zu einfachen Buchstaben: `ü` → `u`). Die IDs unten sind **ohne Geräte-Präfix** angegeben — bei einem Gerät „Wohnzimmer“ heißt der Modus-Select `select.wohnzimmer_luftermodus`, nicht `select.luftermodus`. Die Namen in Anführungszeichen sind die in HA angezeigten Entitätsnamen; die YAML-ID wird nur beim Bearbeiten der Firmware benötigt.
>
> Entitäten, die fehlende Hardware voraussetzen, sind in den reduzierten Varianten ausgeblendet (`internal: true`): Klimasensoren ohne SCD43/BME680, NTC- und Effizienzsensoren in `nosensor`.

## 1. Steuerung & Betriebsmodus

* **`fan.ventosync_hrv`** („VentoSync HRV“, YAML-ID `ventosync_hrv_fan`)
  * *Typ:* Fan (10 Geschwindigkeitsstufen, Presets = Betriebsmodi)
  * *Dokumentation:* Standard-Fan-Entität für Dashboards und Sprachassistenten: Ein/Aus (raumweit, *Ein* stellt den zuletzt aktiven Modus wieder her), 10 Stufen (10 % … 100 % = Stufe 1 … 10), Preset = Betriebsmodus.
* **`select.luftermodus`** („Lüftermodus“, YAML-ID `luefter_modus`)
  * *Typ:* Select
  * *Dokumentation:* Betriebsmodus: `Smart-Automatik`, `Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung`, `Aus` — raumweit. Details: [Betriebsmodi](de_operating-modes.md).
* **`number.lufter_intensitat`** („Lüfter Intensität“, YAML-ID `fan_intensity_display`)
  * *Typ:* Number (1–10)
  * *Dokumentation:* Manuelle Lüfterstufe. In der Smart-Automatik wird sie von der Automatik gesetzt.
* **`sensor.lufter_drehzahl`** („Lüfter Drehzahl“, YAML-ID `fan_rpm`)
  * *Typ:* Sensor (RPM, negativ bei Abluft)
  * *Dokumentation:* Tacho-Drehzahl eines 4-Pin-Lüfters, sonst geschätzt als Geschwindigkeitsanteil × 4200 RPM.
* **`sensor.lufter_pwm`** („Lüfter PWM“, YAML-ID `fan_pwm_percent`)
  * *Typ:* Sensor (%)
  * *Dokumentation:* Aktuelles PWM-Tastverhältnis (50 % = Stillstand, < 50 % Abluft, > 50 % Zuluft).
* **`text_sensor.lufter_richtung`** („Lüfter Richtung“, YAML-ID `direction_display`)
  * *Typ:* Textsensor
  * *Dokumentation:* Aktuelle Luftrichtung: `Zuluft (Rein)`, `Abluft (Raus)` oder `Stillstand`.
* **`number.durchluften_dauer_min`** („Durchlüften Dauer (min)“, YAML-ID `vent_timer`)
  * *Typ:* Number (0–120 min, Schrittweite 5, Standard 30), raumweit
  * *Dokumentation:* Dauer des manuellen Modus `Durchlüften`; danach kehrt der Raum in die `Wärmerückgewinnung` zurück (0 = Dauerbetrieb). Wird von `Stoßlüftung` (fester 15/105-min-Zyklus) und vom automatischen Sommer-Bypass nicht verwendet.

## 2. Smart-Automatik (Sensor-gesteuerte Regelung)

* **`number.smart_automatik_co2_grenzwert`** („Smart-Automatik: CO2 Grenzwert“, YAML-ID `auto_co2_threshold`)
  * *Typ:* Number (400–2000 ppm, Schrittweite 50, Standard 1000), raumweit
  * *Dokumentation:* CO2-Sollwert des PID-Reglers. Die CO2-Regelung ist in der Smart-Automatik immer aktiv und hat Vorrang vor der Feuchte.
* **`number.smart_automatik_feuchte_grenzwert`** („Smart-Automatik: Feuchte Grenzwert“, YAML-ID `auto_humidity_threshold`)
  * *Typ:* Number (40–100 %, Schrittweite 5, Standard 60), raumweit
  * *Dokumentation:* Relative Feuchte, ab der die Lüftung hochregelt (unterdrückt, wenn die Außenluft feuchter ist — Enthalpie-Schutz).
* **`number.smart_automatik_min_lufterstufe`** („Smart-Automatik Min Lüfterstufe“, YAML-ID `automatik_min_luefterstufe`)
  * *Typ:* Number (1–3, Standard 2), raumweit
  * *Dokumentation:* Grundlüftung (Schimmelschutz) — die Automatik unterschreitet diese Stufe nie.
* **`number.smart_automatik_max_lufterstufe`** („Smart-Automatik Max Lüfterstufe“, YAML-ID `automatik_max_luefterstufe`)
  * *Typ:* Number (5–10, Standard 7), raumweit
  * *Dokumentation:* Obergrenze (z. B. Lärmschutz) für die Automatik.
* **`number.smart_automatik_sommerkuhlung_schwelle`** („Smart-Automatik: Sommerkühlung Schwelle“, YAML-ID `auto_summer_cooling_threshold`)
  * *Typ:* Number (18–30 °C, Standard 22), Konfiguration, pro Gerät
  * *Dokumentation:* Innentemperatur, ab der der Sommer-Bypass (freie Kühlung per `Durchlüften`) starten darf. Details: [Smart-Automatik Logik](de_smart-automatic-logic.md).
* **`number.radar_lufter_anpassung`** („Radar Lüfter-Anpassung“, YAML-ID `auto_presence_slider`)
  * *Typ:* Number (-5 … +5, `0` = aus), Konfiguration, raumweit
  * *Dokumentation:* Stufen, die in den **manuellen** Modi zur Grundstufe addiert werden, solange irgendwo im Raum Radar-Anwesenheit erkannt wird (LD2450 eines beliebigen Geräts, per ESP-NOW geteilt). Ohne Anwesenheit keine Anpassung, in der Smart-Automatik nie.

**Eingänge aus Home Assistant** (müssen in HA existieren; die Firmware liest sie, legt sie aber nicht an): `sensor.outdoor_humidity` (relative Außenfeuchte für den Enthalpie-Schutz) und `binary_sensor.sommerbetrieb` (Sommerbetrieb-Schalter für den Sommer-Bypass).

### Klima-Koordination (Smart Climate Control)

Modifikator für die `Smart-Automatik`, solange die Raumklimaanlage aktiv ist. Vollständige Beschreibung: [📄 Intelligente Klimaanlagen-Koordination](de_smart-climate-control.md).

* **`switch.klima_koordination`** („Klima-Koordination“, YAML-ID `smart_climate_control`)
  * *Typ:* Switch (Konfiguration, persistent), raumweit
  * *Dokumentation:* Aktiviert die HVAC-Koordination für den ganzen Raum. Solange Home Assistant die Klimaanlage als aktiv meldet (API-Action `set_ac_active`, an ein beliebiges Gerät des Raums), wechselt die Automatik auf eine reine CO2-Regelung mit gelockertem Sollwert, Stufenbegrenzung und erzwungener Wärmerückgewinnung. Standard: aus.
* **`number.klima_koordination_co2_grenzwert`** („Klima-Koordination: CO2 Grenzwert“, YAML-ID `hvac_co2_threshold`)
  * *Typ:* Number (800–1500 ppm, Standard 1200), Konfiguration, raumweit
  * *Dokumentation:* Gelockerter CO2-Sollwert bei aktiver Klimaanlage; zugleich Freigabeschwelle des CO2-Notfalls.
* **`number.klima_koordination_max_lufterstufe`** („Klima-Koordination: Max Lüfterstufe“, YAML-ID `hvac_max_fan_level`)
  * *Typ:* Number (1–5, Standard 3), Konfiguration, raumweit
  * *Dokumentation:* Harte Stufen-Obergrenze bei aktiver Klimaanlage.
* **`number.klima_koordination_co2_notfallgrenze`** („Klima-Koordination: CO2 Notfallgrenze“, YAML-ID `hvac_emergency_co2`)
  * *Typ:* Number (1200–2000 ppm, Standard 1500), Konfiguration, raumweit
  * *Dokumentation:* CO2-Wert, ab dem die normale Automatik unabhängig vom Klima-Status greift (mindestens 100 ppm über dem gelockerten Sollwert).
* **`text_sensor.klima_koordination_status`** („Klima-Koordination Status“, YAML-ID `hvac_status`)
  * *Typ:* Textsensor (Diagnose)
  * *Dokumentation:* `Deaktiviert`, `Inaktiv (kein Smart-Automatik)`, `Bereit (Klima aus)`, `Aktiv (gedrosselt)`, `Notfall (CO2)`, `Notfall (Feuchte)`, `Ausgesetzt (kein CO2-Wert im Raum)`.
* **`binary_sensor.klima_aktiv_ha_signal`** („Klima aktiv (HA-Signal)“, YAML-ID `hvac_ac_active`)
  * *Typ:* Binary-Sensor (Diagnose)
  * *Dokumentation:* Zuletzt von HA an dieses Gerät gesendeter Klima-Status (läuft ohne neue Meldung nach 15 min ab). Der Koordinator berücksichtigt auch den Klima-Status, den ein Peer des Raums erhalten hat.

### Fenstersperre

Raumweiter Lüftungsstopp, solange ein Fenster offen ist. Einrichtung: [📄 Fenstersperre einrichten](de_window-guard-ha-setup.md).

* **`binary_sensor.fenster_offen_ha_signal`** („Fenster offen (HA-Signal)“, YAML-ID `window_locked`)
  * *Typ:* Binary-Sensor (Diagnose)
  * *Dokumentation:* Zuletzt von HA per API-Action `set_window_open` gesendeter Fensterstatus (läuft nach 15 min ab → gilt als geschlossen). Fensterstatus der Peers wird ebenfalls berücksichtigt.
* **`text_sensor.fenstersperre_aktiv`** („Fenstersperre Aktiv“, YAML-ID `window_guard_status`)
  * *Typ:* Textsensor (`Ja` / `Nein`)
  * *Dokumentation:* Resultierende raumweite Sperre (greift nach 5 s „offen“).
* **`switch.fenstersperre_ignorieren`** („Fenstersperre ignorieren“, YAML-ID `ignore_window_guard_switch`)
  * *Typ:* Switch (Konfiguration, persistent, pro Gerät)
  * *Dokumentation:* Nimmt dieses Gerät von der Fenstersperre aus.

## 3. Sensordaten & Klima

### Kombinierte & berechnete Werte
* **`sensor.effektiver_co2_wert`** („Effektiver CO2 Wert“, YAML-ID `effective_co2`) — CO2 für die Regelung: SCD43 (NDIR), BME680-CO2-Schätzung als Rückfall.
* **`text_sensor.co2_bewertung`** („CO2 Bewertung“, YAML-ID `effective_co2_bewertung`) — qualitative Einstufung des CO2-Werts.
* **`sensor.wrg_effizienz`** („WRG Effizienz“, YAML-ID `heat_recovery_efficiency`) — gemessener WRG-Wirkungsgrad (%), siehe [Wärmerückgewinnung & Effizienz](de_heat-recovery-and-efficiency.md).
* **`sensor.wrg_effizienz_zyklus`** („WRG Effizienz (Zyklus)“) / **`sensor.wrg_energie_zyklus`** („WRG Energie (Zyklus)“) — Wirkungsgrad und rückgewonnene Energie (Wh) des letzten Push-Pull-Zyklus.
* **`text_sensor.wrg_referenz_messpunkt`** („WRG Referenz-Messpunkt“, YAML-ID `wrg_reference_sensor`) — welcher Sensor aktuell die Raumtemperatur-Referenz liefert.

### Hardware-Sensoren
* **SCD43** (Präzisions-CO2): `sensor.scd41_co2`, `sensor.scd41_temperatur`, `sensor.scd41_luftfeuchtigkeit`
* **BME680** (Umwelt & IAQ): `sensor.bme680_temperatur`, `sensor.bme680_luftdruck_relativ`, `sensor.bme680_taupunkt`, `sensor.bme680_absolute_feuchtigkeit`, `sensor.bme680_gas_basiswert`, `sensor.bme680_luftqualitat_co2eq`, `sensor.bme680_iaq_trend`, `binary_sensor.bme680_sensor_health`, `text_sensor.bme680_iaq_bewertung`, `text_sensor.bme680_iaq_trendrichtung`, `text_sensor.bme680_sensor_status`
* **BMP390** (Präzisions-Luftdruck): `sensor.bmp390_luftdruck`, `sensor.bmp390_temperatur`
* **NTC-Thermistoren** (Luft im Rohr): `sensor.temp_abluft_innen` („Temp. Abluft (Innen)“), `sensor.temp_zuluft_aussen` („Temp. Zuluft (Außen)“), dazu die Diagnosewerte `…_raw`

## 4. Radar / Anwesenheit (HLK-LD2450)

* **`binary_sensor.radar_anwesenheit`** („Radar Anwesenheit“) — Anwesenheit erkannt
* `binary_sensor.radar_bewegung` („Radar Bewegung“), `binary_sensor.radar_stillstand` („Radar Stillstand“), `sensor.radar_anzahl_ziele` („Radar Anzahl Ziele“)

## 5. Wartung & Hardware

* **`binary_sensor.filterwechsel_alarm`** („Filterwechsel Alarm“, YAML-ID `filter_change_alarm`) — aktiv nach 8.760 Lüfter-Betriebsstunden oder 3 Jahren seit dem letzten Filterwechsel. Einrichtung: [Filterwechsel-Alarm](de_filter-change-alarm-ha-setup.md).
* **`sensor.filter_betriebstage`** („Filter Betriebstage“, YAML-ID `filter_operating_days_sensor`) — Betriebstage seit dem letzten Filterwechsel (Diagnose).
* **`button.filterwechsel_reset`** („Filterwechsel (Reset)“, YAML-ID `filter_reset_btn`) — setzt den Zähler nach dem Filtertausch zurück (Konfiguration).
* **`button.bme680_basiswert_zurucksetzen`** („BME680 Basiswert zurücksetzen“) — setzt den Gas-Basiswert des BME680 zurück (neue Einlaufphase).
* **`button.esp_neustart_erzwingen`** („ESP Neustart erzwingen“, YAML-ID `force_restart_button`) — startet den ESP32 neu (Konfiguration).
* **`number.maximale_led_helligkeit`** („Maximale LED Helligkeit“, YAML-ID `led_max_brightness_config`) — maximale Panel-LED-Helligkeit, 5–100 % (Standard 80 %), raumweit.
* **`switch.kindersicherung`** („Kindersicherung“, YAML-ID `child_lock_switch`) — sperrt die Tasten dieses Geräts (Konfiguration, persistent). Siehe [Komfort & Sicherheit](de_comfort-and-safety-features.md#-kindersicherung-child-protection-mode).

Die Panel-LEDs selbst steuert die Firmware; sie sind **keine** Light-Entitäten in HA.

## 6. Urlaubsmodus

* **`select.urlaubsmodus_betriebsmodus`** („Urlaubsmodus Betriebsmodus“) — Modus während des Urlaubs (Standard `Stoßlüftung`), Konfiguration.
* **`number.urlaubsmodus_intensitat`** („Urlaubsmodus Intensität“) — Stufe während des Urlaubs (1–10, Standard 1), Konfiguration.
* Geschaltet über den HA-Helper `input_boolean.ventosync_vacation_mode` (Substitution `vacation_sensor_id`); der Master schaltet den Raum. Einrichtung: [Urlaubsmodus](de_vacation-mode-ha-setup.md).

## 7. Einrichtung, Netzwerk & Diagnose

(Hauptsächlich für die Erstkonfiguration nach dem Einbau der Hardware.)

* **`number.id_stockwerk`**, **`number.id_raum`**, **`number.id_gerat`** („ID Stockwerk“ / „ID Raum“ / „ID Gerät“, Konfiguration) — Stockwerk-, Raum- und Geräte-ID. Geräte mit gleichem Stockwerk + Raum bilden eine Raumgruppe; Geräte-ID 1 ist der Master.
* **`select.gerate_phase_a_b`** („Geräte-Phase (A|B)“, YAML-ID `config_phase`) — Phase A beginnt mit Zuluft, Phase B mit Abluft (Push-Pull-Paare brauchen je eines).
* **`button.auf_standardwerte_zurucksetzen`** („Auf Standardwerte zurücksetzen“, Konfiguration) — setzt Stockwerk-, Raum- und Geräte-ID zurück.
* **`button.force_espnow_discovery`** („Force ESPNOW Discovery“, Diagnose) — sucht nach Peers des Raums.
* **`number.sync_intervall`** („Sync Intervall“, YAML-ID `sync_interval_config`) — Heartbeat-Intervall des Masters über ESP-NOW (1–360 min, Standard 1).
* **`text_sensor.gerate_konfiguration`** („Geräte-Konfiguration“), **`sensor.id_stockwerk`**, **`sensor.id_raum`**, **`sensor.id_gerat`** — aktive IDs und Phase (Diagnose).
* **`text_sensor.esp_now_peers`** („ESP-NOW Peers“) und **`switch.esp_now_peerprufung`** („ESP-NOW Peerprüfung“) — bekannte Peers des Raums.
* System: `sensor.wlan_signal`, `sensor.wlan_kanal`, `sensor.laufzeit`, `sensor.freier_speicher_ram`, `sensor.watchdog_restarts`, `sensor.internal_esp32_c6_temperature`, `text_sensor.ip_adresse`, `text_sensor.wlan_ssid`, `text_sensor.esphome_version`, `text_sensor.projektversion`, `update.firmware_update`.

## 8. Flash-Speicher & Lebensdauer (NVS)

Um den Flash-Speicher des ESP32 vor vorzeitigem Verschleiß zu schützen, werden hochfrequente Daten im RAM gepuffert:

* **Filter-Betriebsstunden:** höchstens alle **8 Stunden** in den NVS geschrieben (nach einem plötzlichen Stromausfall gehen höchstens die letzten 8 h Laufzeit verloren).
* **BME680-Basiswert:** höchstens einmal pro Stunde und nur bei einer Änderung ≥ 2 %.
* **Einstellungen** (Modus, Slider, Schalter): werden bei Änderung gespeichert.
