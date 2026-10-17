---
title: Entitäten-Übersicht
sidebar_label: Übersicht
---

Alle Entitäten, die die VentoSync-Firmware in Home Assistant anlegt. Die IDs sind ohne Gerätepräfix angegeben (`…` = z. B. `ventosync_wohnzimmer`, siehe [Entitäts-IDs](../konzepte/entitaets-ids.md)).

**Gültigkeit:** *Raum* = wird mit allen Geräten der Raumgruppe synchronisiert, *Gerät* = gilt nur für dieses Gerät ([Raum & Master](../konzepte/raum-und-master.md)).
**Varianten:** *alle*, sonst der benötigte Sensor ([Hardware-Varianten](../konzepte/varianten.md)).

## Steuerung

| Name | Entitäts-ID | Domain | Kategorie | Gültigkeit | Varianten | Seite |
| --- | --- | --- | --- | --- | --- | --- |
| VentoSync HRV | `fan.…_ventosync_hrv` | fan | – | Raum | alle | [Fan](fan.md) |
| Lüftermodus | `select.…_luftermodus` | select | – | Raum | alle | [Betriebsmodus](betriebsmodus.md#lüftermodus) |
| Lüfter Intensität | `number.…_lufter_intensitat` | number | – | Raum | alle | [Betriebsmodus](betriebsmodus.md#lüfter-intensität) |
| Durchlüften Dauer (min) | `number.…_durchluften_dauer_min` | number | – | Raum | alle | [Betriebsmodus](betriebsmodus.md#durchlüften-dauer-min) |
| Lüfter Drehzahl | `sensor.…_lufter_drehzahl` | sensor | – | Gerät | alle | [Betriebsmodus](betriebsmodus.md#lüfter-drehzahl) |
| Lüfter PWM | `sensor.…_lufter_pwm` | sensor | – | Gerät | alle | [Betriebsmodus](betriebsmodus.md#lüfter-pwm) |
| Lüfter Richtung | `sensor.…_lufter_richtung` | sensor (Text) | – | Gerät | alle | [Betriebsmodus](betriebsmodus.md#lüfter-richtung) |

## Smart-Automatik

| Name | Entitäts-ID | Domain | Kategorie | Gültigkeit | Varianten | Seite |
| --- | --- | --- | --- | --- | --- | --- |
| Smart-Automatik: CO2 Grenzwert | `number.…_smart_automatik_co2_grenzwert` | number | – | Raum | alle | [Smart-Automatik](smart-automatik.md#co2-grenzwert) |
| Smart-Automatik: Feuchte Grenzwert | `number.…_smart_automatik_feuchte_grenzwert` | number | – | Raum | alle | [Smart-Automatik](smart-automatik.md#feuchte-grenzwert) |
| Smart-Automatik Min Lüfterstufe | `number.…_smart_automatik_min_lufterstufe` | number | – | Raum | alle | [Smart-Automatik](smart-automatik.md#min-lüfterstufe--max-lüfterstufe) |
| Smart-Automatik Max Lüfterstufe | `number.…_smart_automatik_max_lufterstufe` | number | – | Raum | alle | [Smart-Automatik](smart-automatik.md#min-lüfterstufe--max-lüfterstufe) |
| Smart-Automatik: Übersteuerung Dauer | `number.…_smart_automatik_ubersteuerung_dauer` | number | config | Raum | alle | [Smart-Automatik](smart-automatik.md#übersteuerung-dauer) |
| Automatik-Übersteuerung Restzeit | `sensor.…_automatik_ubersteuerung_restzeit` | sensor | – | Raum | alle | [Smart-Automatik](smart-automatik.md#automatik-übersteuerung-restzeit) |
| Smart-Automatik: Sommerkühlung Schwelle | `number.…_smart_automatik_sommerkuhlung_schwelle` | number | config | Gerät | alle | [Smart-Automatik](smart-automatik.md#sommerkühlung-schwelle) |
| Radar Lüfter-Anpassung | `number.…_radar_lufter_anpassung` | number | config | Raum | alle | [Smart-Automatik](smart-automatik.md#radar-lüfter-anpassung) |
| Automatik-Status | `sensor.…_automatik_status` | sensor (Text) | – | Gerät | alle | [Automatik-Status](automatik-status.md#automatik-status) |
| Automatik-Grund | `sensor.…_automatik_grund` | sensor (Text) | – | Gerät | alle | [Automatik-Status](automatik-status.md#automatik-grund) |
| Lüftung Ereignis | `event.…_luftung_ereignis` | event | – | Gerät | alle | [Automatik-Status](automatik-status.md#lüftung-ereignis) |

## Klima-Koordination, Fenstersperre & NOx

| Name | Entitäts-ID | Domain | Kategorie | Gültigkeit | Varianten | Seite |
| --- | --- | --- | --- | --- | --- | --- |
| Klima-Koordination | `switch.…_klima_koordination` | switch | config | Raum | alle | [Klima-Koordination](klima-koordination.md) |
| Klima-Koordination: CO2 Grenzwert | `number.…_klima_koordination_co2_grenzwert` | number | config | Raum | alle | [Klima-Koordination](klima-koordination.md) |
| Klima-Koordination: Max Lüfterstufe | `number.…_klima_koordination_max_lufterstufe` | number | config | Raum | alle | [Klima-Koordination](klima-koordination.md) |
| Klima-Koordination: CO2 Notfallgrenze | `number.…_klima_koordination_co2_notfallgrenze` | number | config | Raum | alle | [Klima-Koordination](klima-koordination.md) |
| Klima-Koordination Status | `sensor.…_klima_koordination_status` | sensor (Text) | diagnostic | Raum | alle | [Klima-Koordination](klima-koordination.md#klima-koordination-status) |
| Klima aktiv (HA-Signal) | `binary_sensor.…_klima_aktiv_ha_signal` | binary_sensor | diagnostic | Gerät | alle | [Klima-Koordination](klima-koordination.md#klima-aktiv-ha-signal) |
| Fenstersperre Aktiv | `sensor.…_fenstersperre_aktiv` | sensor (Text) | – | Raum | alle | [Fenstersperre](fenstersperre.md#fenstersperre-aktiv) |
| Fenster offen (HA-Signal) | `binary_sensor.…_fenster_offen_ha_signal` | binary_sensor | diagnostic | Gerät | alle | [Fenstersperre](fenstersperre.md#fenster-offen-ha-signal) |
| Fenstersperre ignorieren | `switch.…_fenstersperre_ignorieren` | switch | config | Gerät | alle | [Fenstersperre](fenstersperre.md#fenstersperre-ignorieren) |
| NOx-Stoßlüftung | `switch.…_nox_stossluftung` | switch | config | Raum | alle | [NOx-Stoßlüftung](nox-stossluftung.md) |
| NOx-Stoßlüftung: Schwelle | `number.…_nox_stossluftung_schwelle` | number | config | Gerät | SGP41 | [NOx-Stoßlüftung](nox-stossluftung.md) |

## Sensoren

| Name | Entitäts-ID | Domain | Kategorie | Varianten | Seite |
| --- | --- | --- | --- | --- | --- |
| Effektiver CO2 Wert | `sensor.…_effektiver_co2_wert` | sensor | – | mit Klimasensor | [Sensoren](sensoren.md#effektiver-co2-wert) |
| CO2 Bewertung | `sensor.…_co2_bewertung` | sensor (Text) | – | mit Klimasensor | [Sensoren](sensoren.md#co2-bewertung) |
| WRG Effizienz | `sensor.…_wrg_effizienz` | sensor | – | NTC | [Sensoren](sensoren.md#wärmerückgewinnung) |
| WRG Effizienz (Zyklus) | `sensor.…_wrg_effizienz_zyklus` | sensor | – | NTC | [Sensoren](sensoren.md#wärmerückgewinnung) |
| WRG Energie (Zyklus) | `sensor.…_wrg_energie_zyklus` | sensor | – | NTC | [Sensoren](sensoren.md#wärmerückgewinnung) |
| WRG Referenz-Messpunkt | `sensor.…_wrg_referenz_messpunkt` | sensor (Text) | – | NTC | [Sensoren](sensoren.md#wärmerückgewinnung) |
| Temp. Abluft (Innen) | `sensor.…_temp_abluft_innen` | sensor | – | NTC | [Sensoren](sensoren.md#ntc-temperaturfühler) |
| Temp. Zuluft (Außen) | `sensor.…_temp_zuluft_aussen` | sensor | – | NTC | [Sensoren](sensoren.md#ntc-temperaturfühler) |
| Temp. Abluft (Innen) RAW | `sensor.…_temp_abluft_innen_raw` | sensor | diagnostic | NTC | [Sensoren](sensoren.md#ntc-temperaturfühler) |
| Temp. Zuluft (Außen) RAW | `sensor.…_temp_zuluft_aussen_raw` | sensor | diagnostic | NTC | [Sensoren](sensoren.md#ntc-temperaturfühler) |
| SCD41 CO2 | `sensor.…_scd41_co2` | sensor | – | SCD43 | [Sensoren](sensoren.md#scd43-co2-temperatur-feuchte) |
| SCD41 Temperatur | `sensor.…_scd41_temperatur` | sensor | – | SCD43 | [Sensoren](sensoren.md#scd43-co2-temperatur-feuchte) |
| SCD41 Luftfeuchtigkeit | `sensor.…_scd41_luftfeuchtigkeit` | sensor | – | SCD43 | [Sensoren](sensoren.md#scd43-co2-temperatur-feuchte) |
| SHT45 Temperatur | `sensor.…_sht45_temperatur` | sensor | – | SGP41 | [Sensoren](sensoren.md#sgp41--sht45-voc-nox-temperatur-feuchte) |
| SHT45 Luftfeuchtigkeit | `sensor.…_sht45_luftfeuchtigkeit` | sensor | – | SGP41 | [Sensoren](sensoren.md#sgp41--sht45-voc-nox-temperatur-feuchte) |
| SGP41 VOC Index | `sensor.…_sgp41_voc_index` | sensor | – | SGP41 | [Sensoren](sensoren.md#sgp41--sht45-voc-nox-temperatur-feuchte) |
| SGP41 NOx Index | `sensor.…_sgp41_nox_index` | sensor | – | SGP41 | [Sensoren](sensoren.md#sgp41--sht45-voc-nox-temperatur-feuchte) |
| SGP41 VOC CO2-Äquivalent | `sensor.…_sgp41_voc_co2_aquivalent` | sensor | – | SGP41 | [Sensoren](sensoren.md#sgp41--sht45-voc-nox-temperatur-feuchte) |
| BME680 Temperatur | `sensor.…_bme680_temperatur` | sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 Luftfeuchtigkeit | `sensor.…_bme680_luftfeuchtigkeit` | sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 Luftdruck (Relativ) | `sensor.…_bme680_luftdruck_relativ` | sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 Taupunkt | `sensor.…_bme680_taupunkt` | sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 Absolute Feuchtigkeit | `sensor.…_bme680_absolute_feuchtigkeit` | sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 Gaswiderstand | `sensor.…_bme680_gaswiderstand` | sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 Gas-Basiswert | `sensor.…_bme680_gas_basiswert` | sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 Luftqualität (CO2eq) | `sensor.…_bme680_luftqualitat_co2eq` | sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 IAQ Trend | `sensor.…_bme680_iaq_trend` | sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 IAQ Bewertung | `sensor.…_bme680_iaq_bewertung` | sensor (Text) | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 IAQ Trendrichtung | `sensor.…_bme680_iaq_trendrichtung` | sensor (Text) | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 Sensor-Status | `sensor.…_bme680_sensor_status` | sensor (Text) | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BME680 Sensor Health | `binary_sensor.…_bme680_sensor_health` | binary_sensor | – | BME680 | [Sensoren](sensoren.md#bme680-umgebung--luftqualität) |
| BMP390 Luftdruck | `sensor.…_bmp390_luftdruck` | sensor | – | alle | [Sensoren](sensoren.md#bmp390-luftdruck) |
| BMP390 Temperatur | `sensor.…_bmp390_temperatur` | sensor | – | alle | [Sensoren](sensoren.md#bmp390-luftdruck) |
| Radar Anwesenheit | `binary_sensor.…_radar_anwesenheit` | binary_sensor | – | Radar | [Radar](radar.md) |
| Radar Bewegung | `binary_sensor.…_radar_bewegung` | binary_sensor | – | Radar | [Radar](radar.md) |
| Radar Stillstand | `binary_sensor.…_radar_stillstand` | binary_sensor | – | Radar | [Radar](radar.md) |
| Radar Anzahl Ziele | `sensor.…_radar_anzahl_ziele` | sensor | – | Radar | [Radar](radar.md) |

Alle Sensoren gelten pro Gerät.

## Urlaubsmodus, Wartung & Bedienfeld

| Name | Entitäts-ID | Domain | Kategorie | Gültigkeit | Varianten | Seite |
| --- | --- | --- | --- | --- | --- | --- |
| Urlaubsmodus Betriebsmodus | `select.…_urlaubsmodus_betriebsmodus` | select | config | Gerät | alle | [Urlaubsmodus](urlaubsmodus.md) |
| Urlaubsmodus Intensität | `number.…_urlaubsmodus_intensitat` | number | config | Gerät | alle | [Urlaubsmodus](urlaubsmodus.md) |
| Filterwechsel Alarm | `binary_sensor.…_filterwechsel_alarm` | binary_sensor | – | Gerät | alle | [Wartung](wartung.md#filterwechsel-alarm) |
| Filter Betriebstage | `sensor.…_filter_betriebstage` | sensor | diagnostic | Gerät | alle | [Wartung](wartung.md#filter-betriebstage) |
| Filterwechsel (Reset) | `button.…_filterwechsel_reset` | button | config | Gerät | alle | [Wartung](wartung.md#filterwechsel-reset) |
| Maximale LED Helligkeit | `number.…_maximale_led_helligkeit` | number | – | Raum | alle | [Wartung](wartung.md#maximale-led-helligkeit) |
| Kindersicherung | `switch.…_kindersicherung` | switch | config | Gerät | alle | [Wartung](wartung.md#kindersicherung) |
| ESP Neustart erzwingen | `button.…_esp_neustart_erzwingen` | button | config | Gerät | alle | [Wartung](wartung.md#esp-neustart-erzwingen) |
| BME680 Basiswert zurücksetzen | `button.…_bme680_basiswert_zurucksetzen` | button | config | Gerät | BME680 | [Wartung](wartung.md#bme680-basiswert-zurücksetzen) |
| Bluetooth Proxy | `switch.…_bluetooth_proxy` | switch | config | Gerät | `nosensor_btproxy` | [Wartung](wartung.md#bluetooth-proxy) |

## Einrichtung & Diagnose

| Name | Entitäts-ID | Domain | Kategorie | Gültigkeit | Seite |
| --- | --- | --- | --- | --- | --- |
| ID Stockwerk | `number.…_id_stockwerk` | number | config | Gerät | [Einrichtung](einrichtung-diagnose.md#raumzuordnung) |
| ID Raum | `number.…_id_raum` | number | config | Gerät | [Einrichtung](einrichtung-diagnose.md#raumzuordnung) |
| ID Gerät | `number.…_id_gerat` | number | config | Gerät | [Einrichtung](einrichtung-diagnose.md#raumzuordnung) |
| Geräte-Phase (A\|B) | `select.…_gerate_phase_a_b` | select | – | Gerät | [Einrichtung](einrichtung-diagnose.md#raumzuordnung) |
| Auf Standardwerte zurücksetzen | `button.…_auf_standardwerte_zurucksetzen` | button | config | Gerät | [Einrichtung](einrichtung-diagnose.md#raumzuordnung) |
| Geräte-Konfiguration | `sensor.…_gerate_konfiguration` | sensor (Text) | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#anzeige-der-aktiven-zuordnung) |
| ID Stockwerk / ID Raum / ID Gerät | `sensor.…_id_stockwerk` / `_id_raum` / `_id_gerat` | sensor | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#anzeige-der-aktiven-zuordnung) |
| Sync Intervall | `number.…_sync_intervall` | number | – | Raum | [Einrichtung](einrichtung-diagnose.md#esp-now) |
| ESP-NOW Peers | `sensor.…_esp_now_peers` | sensor (Text) | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#esp-now) |
| ESP-NOW Peerprüfung | `switch.…_esp_now_peerprufung` | switch | – | Gerät | [Einrichtung](einrichtung-diagnose.md#esp-now) |
| Force ESPNOW Discovery | `button.…_force_espnow_discovery` | button | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#esp-now) |
| WLAN Signal | `sensor.…_wlan_signal` | sensor | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| WLAN Kanal | `sensor.…_wlan_kanal` | sensor | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| WLAN SSID | `sensor.…_wlan_ssid` | sensor (Text) | – | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| IP-Adresse | `sensor.…_ip_adresse` | sensor (Text) | – | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| Laufzeit | `sensor.…_laufzeit` | sensor | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| Freier Speicher (RAM) | `sensor.…_freier_speicher_ram` | sensor | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| Watchdog Restarts | `sensor.…_watchdog_restarts` | sensor | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| Internal ESP32-C6 Temperature | `sensor.…_internal_esp32_c6_temperature` | sensor | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| ESPHome Version | `sensor.…_esphome_version` | sensor (Text) | – | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| Projektversion | `sensor.…_projektversion` | sensor (Text) | diagnostic | Gerät | [Einrichtung](einrichtung-diagnose.md#system) |
| Firmware Update | `update.…_firmware_update` | update | – | Gerät | [Einrichtung](einrichtung-diagnose.md#firmware-update) |

## Nicht als Entität vorhanden

Die Firmware verwendet einige Entitäten nur intern; sie erscheinen nicht in Home Assistant: die Tasten und LEDs des Bedienfelds, die PID-Regler, Platzhalter fehlender Sensoren sowie die aus Home Assistant **gelesenen** Eingänge `sensor.outdoor_humidity`, `binary_sensor.sommerbetrieb` und `input_boolean.ventosync_vacation_mode` ([Eingänge aus Home Assistant](../integration/ha-eingaenge.md)). Klima- und Fensterstatus kommen über [API-Actions](../integration/actions.md).
