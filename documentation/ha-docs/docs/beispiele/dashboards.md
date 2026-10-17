---
title: Dashboard-Karten
sidebar_label: Dashboards
---

Beispiele für Lovelace-Karten mit VentoSync-Entitäten. Die Karten auf dieser Seite nutzen nur **Standardkarten** von Home Assistant — ohne HACS. Für ein fertiges, animiertes Dashboard mit Mushroom-Karten siehe [Home-Assistant-Dashboard](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/documentation/de/de_ha-lovelace-dashboard.md) und die Vorlage [`ha_integration_example/ha_dashboard_example.yaml`](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/ha_integration_example/ha_dashboard_example.yaml).

Einfügen: Dashboard bearbeiten → *Karte hinzufügen* → *Manuell* → YAML einfügen.

:::note Kartenfunktionen
Die `features` der Kachelkarte gibt es je nach Home-Assistant-Version in unterschiedlichem Umfang. Fehlt eine Funktion, zeigt der Karteneditor die verfügbaren an.
:::

## Bedienkachel

Ein/Aus, Betriebsmodus und Stufe in einer Kachel — über die Fan-Entität.

```yaml
type: tile
entity: fan.ventosync_wohnzimmer_ventosync_hrv
name: Lüftung Wohnzimmer
icon: mdi:hvac
features:
  - type: fan-preset-modes
    style: icons
    preset_modes:
      - Smart-Automatik
      - Wärmerückgewinnung
      - Durchlüften
      - Stoßlüftung
  - type: fan-speed
```

Alternativ mit dem Select (enthält auch `Aus`):

```yaml
type: tile
entity: select.ventosync_wohnzimmer_luftermodus
name: Betriebsmodus
features:
  - type: select-options
```

## Status auf einen Blick

Eine Markdown-Karte mit dem Klartext-Status, Messwerten und Hinweisen.

```yaml
type: markdown
title: Lüftung Wohnzimmer
content: |
  **{{ states('sensor.ventosync_wohnzimmer_automatik_status') }}**

  CO2: {{ states('sensor.ventosync_wohnzimmer_effektiver_co2_wert') }} ppm
  ({{ states('sensor.ventosync_wohnzimmer_co2_bewertung') }}) ·
  Feuchte: {{ states('sensor.ventosync_wohnzimmer_scd41_luftfeuchtigkeit') }} % ·
  Richtung: {{ states('sensor.ventosync_wohnzimmer_lufter_richtung') }}

  {% if states('sensor.ventosync_wohnzimmer_automatik_ubersteuerung_restzeit') | int(0) > 0 %}
  ⏱️ Übersteuerung noch {{ states('sensor.ventosync_wohnzimmer_automatik_ubersteuerung_restzeit') }} min
  {% endif %}
  {% if is_state('sensor.ventosync_wohnzimmer_fenstersperre_aktiv', 'Ja') %}
  🪟 Pausiert – Fenster offen
  {% endif %}
```

## Hinweiskarten

Erscheinen nur bei Bedarf.

```yaml
type: vertical-stack
cards:
  - type: conditional
    conditions:
      - condition: state
        entity: binary_sensor.ventosync_wohnzimmer_filterwechsel_alarm
        state: "on"
    card:
      type: tile
      entity: button.ventosync_wohnzimmer_filterwechsel_reset
      name: Filterwechsel fällig – nach dem Wechsel hier zurücksetzen
      icon: mdi:air-filter
      color: orange
      tap_action:
        action: perform-action
        perform_action: button.press
        target:
          entity_id: button.ventosync_wohnzimmer_filterwechsel_reset
        confirmation:
          text: Filter gewechselt? Zähler wird zurückgesetzt.
  - type: conditional
    conditions:
      - condition: state
        entity: sensor.ventosync_wohnzimmer_fenstersperre_aktiv
        state: "Ja"
    card:
      type: tile
      entity: sensor.ventosync_wohnzimmer_fenstersperre_aktiv
      name: Lüftung pausiert – Fenster offen
      icon: mdi:window-open-variant
      color: blue
```

## Einstellungen der Smart-Automatik

```yaml
type: entities
title: Smart-Automatik
show_header_toggle: false
entities:
  - entity: number.ventosync_wohnzimmer_smart_automatik_co2_grenzwert
    name: CO2-Sollwert
  - entity: number.ventosync_wohnzimmer_smart_automatik_feuchte_grenzwert
    name: Feuchte-Sollwert
  - entity: number.ventosync_wohnzimmer_smart_automatik_min_lufterstufe
    name: Mindeststufe
  - entity: number.ventosync_wohnzimmer_smart_automatik_max_lufterstufe
    name: Höchststufe
  - entity: number.ventosync_wohnzimmer_smart_automatik_ubersteuerung_dauer
    name: Dauer der Übersteuerung
  - type: divider
  - entity: switch.ventosync_wohnzimmer_klima_koordination
  - entity: sensor.ventosync_wohnzimmer_klima_koordination_status
    name: Status
  - entity: switch.ventosync_wohnzimmer_nox_stossluftung
```

## Verlauf

```yaml
type: history-graph
title: Raumluft Wohnzimmer
hours_to_show: 24
entities:
  - entity: sensor.ventosync_wohnzimmer_effektiver_co2_wert
    name: CO2
  - entity: number.ventosync_wohnzimmer_lufter_intensitat
    name: Lüfterstufe
```

```yaml
type: statistics-graph
title: Wärmerückgewinnung
chart_type: line
period: day
stat_types:
  - mean
entities:
  - entity: sensor.ventosync_wohnzimmer_wrg_effizienz
    name: WRG-Effizienz
```

## Mehrere Räume

Eine Übersicht mit einer Zeile pro Raum. Da Modus und Stufe raumweit gelten, reicht je Raum ein Gerät.

```yaml
type: grid
columns: 2
square: false
cards:
  - type: tile
    entity: fan.ventosync_wohnzimmer_ventosync_hrv
    name: Wohnzimmer
    state_content:
      - preset_mode
      - percentage
  - type: tile
    entity: sensor.ventosync_wohnzimmer_automatik_status
    name: Status
  - type: tile
    entity: fan.ventosync_schlafzimmer_ventosync_hrv
    name: Schlafzimmer
    state_content:
      - preset_mode
      - percentage
  - type: tile
    entity: sensor.ventosync_schlafzimmer_automatik_status
    name: Status
```

## Urlaubsmodus

```yaml
type: tile
entity: input_boolean.ventosync_vacation_mode
name: Urlaubsmodus (alle Räume)
icon: mdi:palm-tree
features:
  - type: toggle
```

## Eigene Ansicht pro Gerät

Für Einrichtung und Fehlersuche ist die **Geräteseite** am praktischsten (*Einstellungen → Geräte & Dienste → ESPHome → Gerät*): Dort stehen alle Entitäten nach Steuerelementen, Sensoren, Konfiguration und Diagnose sortiert. Eine Kachel darauf verlinkt:

```yaml
type: button
name: VentoSync Wohnzimmer – Gerät
icon: mdi:cog
tap_action:
  action: navigate
  navigation_path: /config/devices/device/<geräte-id>
```

Die `<geräte-id>` steht in der Adresszeile des Browsers, wenn die Geräteseite geöffnet ist.
