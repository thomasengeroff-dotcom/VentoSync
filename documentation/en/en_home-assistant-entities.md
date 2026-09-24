# Home Assistant Entities: VentoSync

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_home-assistant-entities.md)


This file lists the Home Assistant entities provided by the VentoSync firmware (`ventosync.yaml` and its packages), grouped by topic.

> [!IMPORTANT]
> **Entity IDs:** Home Assistant builds the entity ID from the **device name + entity name** (umlauts become plain letters: `ü` → `u`). The IDs below are shown **without the device prefix** — on a device named "Wohnzimmer" the mode select is `select.wohnzimmer_luftermodus`, not `select.luftermodus`. The German names in quotes are the entity names as shown in HA; the YAML ID is only needed when editing the firmware.
>
> Entities depending on missing hardware are hidden in the reduced variants (`internal: true`): climate sensors without SCD43/BME680, NTC and efficiency sensors in `nosensor`.

## 1. Control & Operating Mode

* **`fan.ventosync_hrv`** ("VentoSync HRV", YAML ID `ventosync_hrv_fan`)
  * *Type:* Fan (10 speed steps, presets = operating modes)
  * *Documentation:* Standard HA fan entity for dashboards and voice assistants: on/off (room-wide, *on* restores the last active mode), 10 speed steps (10 % … 100 % = level 1 … 10), preset = operating mode.
* **`select.luftermodus`** ("Lüftermodus", YAML ID `luefter_modus`)
  * *Type:* Select
  * *Documentation:* Operating mode: `Smart-Automatik`, `Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung`, `Aus` — room-wide. Details: [Operating Modes](en_operating-modes.md).
* **`number.lufter_intensitat`** ("Lüfter Intensität", YAML ID `fan_intensity_display`)
  * *Type:* Number (1–10)
  * *Documentation:* Manual fan level. In Smart-Automatik it is set by the automatic.
* **`sensor.lufter_drehzahl`** ("Lüfter Drehzahl", YAML ID `fan_rpm`)
  * *Type:* Sensor (RPM, negative while extracting)
  * *Documentation:* Tacho RPM of a 4-pin fan, otherwise estimated as speed fraction × 4200 RPM.
* **`sensor.lufter_pwm`** ("Lüfter PWM", YAML ID `fan_pwm_percent`)
  * *Type:* Sensor (%)
  * *Documentation:* Current PWM duty cycle (50 % = standstill, < 50 % extract, > 50 % supply).
* **`text_sensor.lufter_richtung`** ("Lüfter Richtung", YAML ID `direction_display`)
  * *Type:* Text Sensor
  * *Documentation:* Current airflow direction: `Zuluft (Rein)`, `Abluft (Raus)` or `Stillstand`.
* **`number.durchluften_dauer_min`** ("Durchlüften Dauer (min)", YAML ID `vent_timer`)
  * *Type:* Number (0–120 min, step 5, default 30), room-wide
  * *Documentation:* Duration of the manual `Durchlüften` mode; afterwards the room returns to `Wärmerückgewinnung` (0 = continuous operation). Not used by `Stoßlüftung` (fixed 15/105 min cycle) or by the automatic summer bypass.

## 2. Smart-Automatik (Sensor-driven Regulation)

* **`number.smart_automatik_co2_grenzwert`** ("Smart-Automatik: CO2 Grenzwert", YAML ID `auto_co2_threshold`)
  * *Type:* Number (400–2000 ppm, step 50, default 1000), room-wide
  * *Documentation:* CO2 setpoint of the PID controller. CO2 regulation is always active in Smart-Automatik and has priority over humidity.
* **`number.smart_automatik_feuchte_grenzwert`** ("Smart-Automatik: Feuchte Grenzwert", YAML ID `auto_humidity_threshold`)
  * *Type:* Number (40–100 %, step 5, default 60), room-wide
  * *Documentation:* Relative humidity above which the ventilation increases (suppressed when the outdoor air is more humid, enthalpy guard).
* **`number.smart_automatik_min_lufterstufe`** ("Smart-Automatik Min Lüfterstufe", YAML ID `automatik_min_luefterstufe`)
  * *Type:* Number (1–3, default 2), room-wide
  * *Documentation:* Basic ventilation (mold protection) — the automatic never goes below this level.
* **`number.smart_automatik_max_lufterstufe`** ("Smart-Automatik Max Lüfterstufe", YAML ID `automatik_max_luefterstufe`)
  * *Type:* Number (5–10, default 7), room-wide
  * *Documentation:* Upper limit (e.g. noise protection) for the automatic.
* **`number.smart_automatik_sommerkuhlung_schwelle`** ("Smart-Automatik: Sommerkühlung Schwelle", YAML ID `auto_summer_cooling_threshold`)
  * *Type:* Number (18–30 °C, default 22), config, per device
  * *Documentation:* Indoor temperature above which the summer bypass (free cooling via `Durchlüften`) may start. Details: [Smart-Automatik Logic](en_smart-automatic-logic.md).
* **`number.radar_lufter_anpassung`** ("Radar Lüfter-Anpassung", YAML ID `auto_presence_slider`)
  * *Type:* Number (-5 … +5, `0` = off), config, room-wide
  * *Documentation:* Levels added to the base level in the **manual** modes while radar presence is detected anywhere in the room (any device's LD2450, shared via ESP-NOW). Not applied without presence and never in Smart-Automatik.

**Inputs from Home Assistant** (must exist in HA; read by the firmware, not created by it): `sensor.outdoor_humidity` (outdoor relative humidity for the enthalpy guard) and `binary_sensor.sommerbetrieb` (summer mode switch for the summer bypass).

### Smart Climate Control (HVAC Coordination)

Modifier for `Smart-Automatik` while the room air conditioner is active. Full description: [📄 Smart Climate Control — HVAC Coordination](en_smart-climate-control.md).

* **`switch.klima_koordination`** ("Klima-Koordination", YAML ID `smart_climate_control`)
  * *Type:* Switch (config, persisted), room-wide
  * *Documentation:* Enables the HVAC coordination for the whole room. While Home Assistant reports the AC as active (API action `set_ac_active`, pushed to any device of the room), the automatic switches to a CO2-only loop with a relaxed setpoint, a fan level cap and enforced heat recovery. Default: off.
* **`number.klima_koordination_co2_grenzwert`** ("Klima-Koordination: CO2 Grenzwert", YAML ID `hvac_co2_threshold`)
  * *Type:* Number (800–1500 ppm, default 1200), config, room-wide
  * *Documentation:* Relaxed CO2 setpoint while the AC is active; also the release threshold of the CO2 emergency.
* **`number.klima_koordination_max_lufterstufe`** ("Klima-Koordination: Max Lüfterstufe", YAML ID `hvac_max_fan_level`)
  * *Type:* Number (1–5, default 3), config, room-wide
  * *Documentation:* Hard fan level cap while the AC is active.
* **`number.klima_koordination_co2_notfallgrenze`** ("Klima-Koordination: CO2 Notfallgrenze", YAML ID `hvac_emergency_co2`)
  * *Type:* Number (1200–2000 ppm, default 1500), config, room-wide
  * *Documentation:* CO2 level at which the normal automatic regulation resumes regardless of the AC state (kept at least 100 ppm above the relaxed setpoint).
* **`text_sensor.klima_koordination_status`** ("Klima-Koordination Status", YAML ID `hvac_status`)
  * *Type:* Text Sensor (diagnostic)
  * *Documentation:* `Deaktiviert`, `Inaktiv (kein Smart-Automatik)`, `Bereit (Klima aus)`, `Aktiv (gedrosselt)`, `Notfall (CO2)`, `Notfall (Feuchte)`, `Ausgesetzt (kein CO2-Wert im Raum)`.
* **`binary_sensor.klima_aktiv_ha_signal`** ("Klima aktiv (HA-Signal)", YAML ID `hvac_ac_active`)
  * *Type:* Binary Sensor (diagnostic)
  * *Documentation:* AC state last pushed by HA to this device (expires after 15 min without a new push). The coordinator also honours the AC state a peer of the room received.

### Window Guard

Room-wide ventilation pause while a window is open. Setup: [📄 Window Guard Setup](en_window-guard-ha-setup.md).

* **`binary_sensor.fenster_offen_ha_signal`** ("Fenster offen (HA-Signal)", YAML ID `window_locked`)
  * *Type:* Binary Sensor (diagnostic)
  * *Documentation:* Window state last pushed by HA via the API action `set_window_open` (expires after 15 min → treated as closed). Peers' window states are honoured as well.
* **`text_sensor.fenstersperre_aktiv`** ("Fenstersperre Aktiv", YAML ID `window_guard_status`)
  * *Type:* Text Sensor (`Ja` / `Nein`)
  * *Documentation:* Resulting room-wide lock (engaged after 5 s of "open").
* **`switch.fenstersperre_ignorieren`** ("Fenstersperre ignorieren", YAML ID `ignore_window_guard_switch`)
  * *Type:* Switch (config, persisted, per device)
  * *Documentation:* Excludes this device from the window lock.

## 3. Sensor Data & Climate

### Combined & Calculated Values
* **`sensor.effektiver_co2_wert`** ("Effektiver CO2 Wert", YAML ID `effective_co2`) — CO2 used by the regulation: SCD43 (NDIR), BME680 CO2 estimate as fallback.
* **`text_sensor.co2_bewertung`** ("CO2 Bewertung", YAML ID `effective_co2_bewertung`) — qualitative rating of the CO2 value.
* **`sensor.wrg_effizienz`** ("WRG Effizienz", YAML ID `heat_recovery_efficiency`) — measured heat recovery efficiency (%), see [Heat Recovery & Efficiency](en_heat-recovery-and-efficiency.md).
* **`sensor.wrg_effizienz_zyklus`** ("WRG Effizienz (Zyklus)") / **`sensor.wrg_energie_zyklus`** ("WRG Energie (Zyklus)") — efficiency and recovered energy (Wh) of the last push-pull cycle.
* **`text_sensor.wrg_referenz_messpunkt`** ("WRG Referenz-Messpunkt", YAML ID `wrg_reference_sensor`) — which sensor currently provides the room temperature reference.

### Hardware Sensors
* **SCD43** (precision CO2): `sensor.scd41_co2`, `sensor.scd41_temperatur`, `sensor.scd41_luftfeuchtigkeit`
* **BME680** (environment & IAQ): `sensor.bme680_temperatur`, `sensor.bme680_luftdruck_relativ`, `sensor.bme680_taupunkt`, `sensor.bme680_absolute_feuchtigkeit`, `sensor.bme680_gas_basiswert`, `sensor.bme680_luftqualitat_co2eq`, `sensor.bme680_iaq_trend`, `binary_sensor.bme680_sensor_health`, `text_sensor.bme680_iaq_bewertung`, `text_sensor.bme680_iaq_trendrichtung`, `text_sensor.bme680_sensor_status`
* **BMP390** (precision pressure): `sensor.bmp390_luftdruck`, `sensor.bmp390_temperatur`
* **NTC thermistors** (air in the tube): `sensor.temp_abluft_innen` ("Temp. Abluft (Innen)"), `sensor.temp_zuluft_aussen` ("Temp. Zuluft (Außen)"), plus the diagnostic `…_raw` values

## 4. Radar / Presence (HLK-LD2450)

* **`binary_sensor.radar_anwesenheit`** ("Radar Anwesenheit") — presence detected
* `binary_sensor.radar_bewegung` ("Radar Bewegung"), `binary_sensor.radar_stillstand` ("Radar Stillstand"), `sensor.radar_anzahl_ziele` ("Radar Anzahl Ziele")

## 5. Maintenance & Hardware

* **`binary_sensor.filterwechsel_alarm`** ("Filterwechsel Alarm", YAML ID `filter_change_alarm`) — on after 8,760 fan operating hours or 3 years since the last filter change. Setup: [Filter Change Alarm](en_filter-change-alarm-ha-setup.md).
* **`sensor.filter_betriebstage`** ("Filter Betriebstage", YAML ID `filter_operating_days_sensor`) — operating days since the last filter change (diagnostic).
* **`button.filterwechsel_reset`** ("Filterwechsel (Reset)", YAML ID `filter_reset_btn`) — resets the counter after a filter change (config).
* **`button.bme680_basiswert_zurucksetzen`** ("BME680 Basiswert zurücksetzen") — resets the BME680 gas baseline (new burn-in phase).
* **`button.esp_neustart_erzwingen`** ("ESP Neustart erzwingen", YAML ID `force_restart_button`) — restarts the ESP32 (config).
* **`number.maximale_led_helligkeit`** ("Maximale LED Helligkeit", YAML ID `led_max_brightness_config`) — maximum panel LED brightness, 5–100 % (default 80 %), room-wide.
* **`switch.kindersicherung`** ("Kindersicherung", YAML ID `child_lock_switch`) — locks the panel buttons of this device (config, persisted). See [Comfort & Safety](en_comfort-and-safety-features.md#-child-protection-mode).

The panel LEDs themselves are driven by the firmware and are **not** exposed as light entities.

## 6. Vacation Mode

* **`select.urlaubsmodus_betriebsmodus`** ("Urlaubsmodus Betriebsmodus") — mode during vacation (default `Stoßlüftung`), config.
* **`number.urlaubsmodus_intensitat`** ("Urlaubsmodus Intensität") — level during vacation (1–10, default 1), config.
* Switched by the HA helper `input_boolean.ventosync_vacation_mode` (substitution `vacation_sensor_id`); the Master applies it for the room. Setup: [Vacation Mode](en_vacation-mode-ha-setup.md).

## 7. Setup, Network & Diagnostics

(Mainly needed for the initial configuration after installing the hardware.)

* **`number.id_stockwerk`**, **`number.id_raum`**, **`number.id_gerat`** ("ID Stockwerk" / "ID Raum" / "ID Gerät", config) — floor, room and device ID. Devices with the same floor + room form a room group; device ID 1 is the Master.
* **`select.gerate_phase_a_b`** ("Geräte-Phase (A|B)", YAML ID `config_phase`) — Phase A starts with supply air, Phase B with extract air (push-pull pairs need one of each).
* **`button.auf_standardwerte_zurucksetzen`** ("Auf Standardwerte zurücksetzen", config) — resets floor, room and device ID.
* **`button.force_espnow_discovery`** ("Force ESPNOW Discovery", diagnostic) — searches for peers of the room.
* **`number.sync_intervall`** ("Sync Intervall", YAML ID `sync_interval_config`) — Master heartbeat interval over ESP-NOW (1–360 min, default 1).
* **`text_sensor.gerate_konfiguration`** ("Geräte-Konfiguration"), **`sensor.id_stockwerk`**, **`sensor.id_raum`**, **`sensor.id_gerat`** — active IDs and phase (diagnostic).
* **`text_sensor.esp_now_peers`** ("ESP-NOW Peers") and **`switch.esp_now_peerprufung`** ("ESP-NOW Peerprüfung") — known peers of the room.
* System: `sensor.wlan_signal`, `sensor.wlan_kanal`, `sensor.laufzeit`, `sensor.freier_speicher_ram`, `sensor.watchdog_restarts`, `sensor.internal_esp32_c6_temperature`, `text_sensor.ip_adresse`, `text_sensor.wlan_ssid`, `text_sensor.esphome_version`, `text_sensor.projektversion`, `update.firmware_update`.

## 8. Flash Memory & Lifetime (NVS)

To protect the ESP32 flash from premature wear, high-frequency data is buffered in RAM:

* **Filter operating hours:** written to NVS at most every **8 hours** (after a sudden power loss at most the last 8 h of runtime are lost).
* **BME680 baseline:** saved at most once per hour and only after a change of ≥ 2 %.
* **Settings** (mode, sliders, switches): stored when they change.
