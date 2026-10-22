---
title: Automationen
sidebar_label: Automationen
---

Fertige Automationen und Skripte für typische Aufgaben. Alle Beispiele verwenden das Gerät „VentoSync Wohnzimmer“ (Entitäts-Präfix `ventosync_wohnzimmer`, Node-Name `ventosync-a1b2c3`) — IDs bitte an die eigenen Geräte anpassen, siehe [Entitäts-IDs](../konzepte/entitaets-ids.md).

Die Beispiele sind im Format von `automations.yaml` bzw. `scripts.yaml` geschrieben. Im Automations-Editor: neue Automation → *⋮ → In YAML bearbeiten* → Inhalt ohne den führenden Bindestrich vor `alias:` einfügen.

:::tip Raumweite Einstellungen
Raumweite Einstellungen (Modus, Stufe, Grenzwerte …) nur an **einem** Gerät pro Raum setzen, am besten am Master — die anderen Geräte übernehmen sie. Siehe [Raum & Master](../konzepte/raum-und-master.md).
:::

## Kopplung mit Home Assistant

### Fensterkontakte koppeln

Pausiert die Lüftung des Raums, solange ein Fenster offen ist ([Fenstersperre](../entitaeten/fenstersperre.md)). Zuerst die Fensterkontakte zu einer Binärsensor-Gruppe zusammenfassen (*Helfer → Gruppe → Binärsensor-Gruppe*, hier `binary_sensor.fenster_wohnzimmer`).

```yaml
- alias: "VentoSync: Fensterstatus Wohnzimmer"
  mode: queued
  triggers:
    - trigger: state
      entity_id: binary_sensor.fenster_wohnzimmer
    - trigger: homeassistant
      event: start
    - trigger: time_pattern   # Status läuft im Gerät nach 15 min ab
      minutes: "/5"
  variables:
    window_open: "{{ is_state('binary_sensor.fenster_wohnzimmer', 'on') }}"
  actions:
    # Ein Gerät pro Raum genügt; für Redundanz alle Geräte des Raums.
    - action: esphome.ventosync_a1b2c3_set_window_open
      data:
        window_open: "{{ window_open }}"
      continue_on_error: true
    - action: esphome.ventosync_d4e5f6_set_window_open
      data:
        window_open: "{{ window_open }}"
      continue_on_error: true
```

### Klimaanlage koppeln

Drosselt die Smart-Automatik, solange die Klimaanlage des Raums läuft ([Klima-Koordination](../entitaeten/klima-koordination.md)). Vorher den Schalter `Klima-Koordination` einschalten.

```yaml
- alias: "VentoSync: Klimastatus Wohnzimmer"
  mode: queued
  triggers:
    - trigger: state
      entity_id: climate.wohnzimmer_klima
    - trigger: homeassistant
      event: start
    - trigger: time_pattern
      minutes: "/5"
  variables:
    # Betriebsmodus (state) verwenden, NICHT hvac_action — der Kompressor taktet.
    ac_active: >
      {{ states('climate.wohnzimmer_klima') in ['cool', 'heat', 'heat_cool', 'dry', 'auto'] }}
  actions:
    - action: esphome.ventosync_a1b2c3_set_ac_active
      data:
        active: "{{ ac_active }}"
      continue_on_error: true
```

:::note Klimageräte ohne `hvac_mode`
Liefert eine Integration nur `hvac_action`, einen Template-Binärsensor mit `delay_off` (z. B. 10 min) dazwischenschalten und dessen Zustand senden. So glättet man das Takten des Kompressors.
:::

#### Variante: Klimastatus als Binärsensor

Statt die Bedingung in der Automation auszuwerten, kann ein Vorlagen-Binärsensor den Klimastatus des Raums bereitstellen (*Helfer → Vorlage → Binärsensor*, Geräteklasse *Läuft*). Der Sensor lässt sich auch im Dashboard anzeigen, und mehrere Klimageräte eines Raums lassen sich darin zusammenfassen.

```yaml
# Vorlagen-Binärsensor „Klima aktiv Wohnzimmer“ → binary_sensor.klima_aktiv_wohnzimmer
{{ states('climate.wohnzimmer_klima') in ['cool', 'heat', 'heat_cool', 'dry', 'auto'] }}
```

```yaml
- alias: "VentoSync: Klimastatus Wohnzimmer"
  mode: queued
  triggers:
    - trigger: state
      entity_id: binary_sensor.klima_aktiv_wohnzimmer
    - trigger: homeassistant
      event: start
    - trigger: time_pattern
      minutes: "/5"
  variables:
    ac_active: "{{ is_state('binary_sensor.klima_aktiv_wohnzimmer', 'on') }}"
  actions:
    - action: esphome.ventosync_a1b2c3_set_ac_active
      data:
        active: "{{ ac_active }}"
      continue_on_error: true
    - action: esphome.ventosync_d4e5f6_set_ac_active
      data:
        active: "{{ ac_active }}"
      continue_on_error: true
```

### Urlaubsmodus automatisch

Startet den [Urlaubsmodus](../entitaeten/urlaubsmodus.md), wenn alle Bewohner länger als 24 Stunden weg sind, und beendet ihn bei der Rückkehr.

```yaml
- alias: "VentoSync: Urlaubsmodus nach Abwesenheit"
  mode: single
  triggers:
    - trigger: state
      entity_id: zone.home
      to: "0"
      for:
        hours: 24
      id: weg
    - trigger: numeric_state
      entity_id: zone.home
      above: 0
      id: da
  actions:
    - choose:
        - conditions:
            - condition: trigger
              id: weg
          sequence:
            - action: input_boolean.turn_on
              target:
                entity_id: input_boolean.ventosync_vacation_mode
        - conditions:
            - condition: trigger
              id: da
          sequence:
            - action: input_boolean.turn_off
              target:
                entity_id: input_boolean.ventosync_vacation_mode
```

## Steuerung

### Nachtmodus

Abends leiser und dunkler, morgens zurück. Höchststufe und LED-Helligkeit gelten raumweit.

```yaml
- alias: "VentoSync: Nachtmodus Schlafzimmer"
  mode: single
  triggers:
    - trigger: time
      at: "22:00:00"
      id: nacht
    - trigger: time
      at: "06:30:00"
      id: tag
  actions:
    - action: number.set_value
      target:
        entity_id: number.ventosync_schlafzimmer_smart_automatik_max_lufterstufe
      data:
        value: "{{ 5 if trigger.id == 'nacht' else 7 }}"
    - action: number.set_value
      target:
        entity_id: number.ventosync_schlafzimmer_maximale_led_helligkeit
      data:
        value: "{{ 5 if trigger.id == 'nacht' else 80 }}"
```

### Leiser, wenn niemand da ist

Senkt in der Smart-Automatik die Höchststufe, solange das Radar niemanden erkennt (Radar wirkt in der Smart-Automatik nicht von selbst).

```yaml
- alias: "VentoSync: Höchststufe nach Anwesenheit"
  mode: restart
  triggers:
    - trigger: state
      entity_id: binary_sensor.ventosync_wohnzimmer_radar_anwesenheit
      to: "off"
      for:
        minutes: 15
    - trigger: state
      entity_id: binary_sensor.ventosync_wohnzimmer_radar_anwesenheit
      to: "on"
  actions:
    - action: number.set_value
      target:
        entity_id: number.ventosync_wohnzimmer_smart_automatik_max_lufterstufe
      data:
        value: "{{ 7 if trigger.to_state.state == 'on' else 5 }}"
```

### Bad: kräftig lüften nach dem Duschen

Steigt die Feuchte im Bad schnell an, wird die Stufe hochgesetzt. In der Smart-Automatik startet das eine [Übersteuerung](../entitaeten/smart-automatik.md#übersteuerung), die nach `Smart-Automatik: Übersteuerung Dauer` von selbst endet.

```yaml
- alias: "VentoSync: Duschen erkannt"
  mode: single
  max_exceeded: silent
  triggers:
    - trigger: numeric_state
      entity_id: sensor.ventosync_bad_scd41_luftfeuchtigkeit
      above: 75
  conditions:
    - condition: state
      entity_id: select.ventosync_bad_luftermodus
      state: Smart-Automatik
  actions:
    - action: fan.set_percentage
      target:
        entity_id: fan.ventosync_bad_ventosync_hrv
      data:
        percentage: 90
```

### Querlüften auf Knopfdruck

Skript: 45 Minuten `Durchlüften`, danach automatisch zurück in die Wärmerückgewinnung.

```yaml
# scripts.yaml
ventosync_querlueften_wohnzimmer:
  alias: "Wohnzimmer 45 min querlüften"
  icon: mdi:weather-windy
  sequence:
    - action: number.set_value
      target:
        entity_id: number.ventosync_wohnzimmer_durchluften_dauer_min
      data:
        value: 45
    - action: select.select_option
      target:
        entity_id: select.ventosync_wohnzimmer_luftermodus
      data:
        option: Durchlüften
```

### Alle Räume aus- und wieder einschalten

Die Fan-Entität eines Geräts pro Raum genügt; `turn_on` stellt jeweils den letzten Modus wieder her.

```yaml
# scripts.yaml
ventosync_alle_aus:
  alias: "Lüftung überall aus"
  sequence:
    - action: fan.turn_off
      target:
        entity_id:
          - fan.ventosync_wohnzimmer_ventosync_hrv
          - fan.ventosync_schlafzimmer_ventosync_hrv
          - fan.ventosync_bad_ventosync_hrv

ventosync_alle_an:
  alias: "Lüftung überall an"
  sequence:
    - action: fan.turn_on
      target:
        entity_id:
          - fan.ventosync_wohnzimmer_ventosync_hrv
          - fan.ventosync_schlafzimmer_ventosync_hrv
          - fan.ventosync_bad_ventosync_hrv
```

## Benachrichtigungen

### Filterwechsel melden

```yaml
- alias: "VentoSync: Filterwechsel fällig"
  triggers:
    - trigger: state
      entity_id:
        - binary_sensor.ventosync_wohnzimmer_filterwechsel_alarm
        - binary_sensor.ventosync_schlafzimmer_filterwechsel_alarm
      to: "on"
  actions:
    - action: notify.mobile_app_mein_telefon
      data:
        title: "🧹 Filterwechsel empfohlen"
        message: >
          {{ trigger.to_state.name }}: Filter nach
          {{ states(trigger.entity_id | replace('binary_sensor.', 'sensor.') | replace('_filterwechsel_alarm', '_filter_betriebstage')) | int(0) }}
          Betriebstagen wechseln und danach „Filterwechsel (Reset)“ drücken.
        data:
          tag: "filterwechsel"
```

### NOx von draußen melden

Wird eine [NOx-Stoßlüftung](../entitaeten/nox-stossluftung.md) abgebrochen, weil das NOx von draußen kommt, ist Lüften gerade kontraproduktiv.

```yaml
- alias: "VentoSync: NOx von draußen"
  triggers:
    - trigger: state
      entity_id: event.ventosync_kuche_luftung_ereignis
      not_from: unavailable
  conditions:
    - condition: state
      entity_id: event.ventosync_kuche_luftung_ereignis
      attribute: event_type
      state: nox_abbruch_aussenluft
  actions:
    - action: notify.notify
      data:
        message: "Küche: NOx kommt von draußen – Fenster geschlossen halten."
```

### Schlechte Luft trotz Lüftung

Meldet, wenn der CO2-Wert 15 Minuten über 1400 ppm bleibt — etwa weil die Klima-Koordination drosselt oder die Höchststufe zu niedrig ist.

```yaml
- alias: "VentoSync: CO2 dauerhaft hoch"
  triggers:
    - trigger: numeric_state
      entity_id: sensor.ventosync_wohnzimmer_effektiver_co2_wert
      above: 1400
      for:
        minutes: 15
  actions:
    - action: notify.notify
      data:
        message: >
          Wohnzimmer: CO2 {{ states('sensor.ventosync_wohnzimmer_effektiver_co2_wert') }} ppm.
          {{ states('sensor.ventosync_wohnzimmer_automatik_status') }}
```

### Gerät nicht erreichbar

```yaml
- alias: "VentoSync: Gerät offline"
  triggers:
    - trigger: state
      entity_id: sensor.ventosync_wohnzimmer_laufzeit
      to: unavailable
      for:
        minutes: 10
  actions:
    - action: notify.notify
      data:
        message: "VentoSync Wohnzimmer ist seit 10 Minuten nicht erreichbar."
```
