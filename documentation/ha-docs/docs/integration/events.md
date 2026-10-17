---
title: Events
sidebar_label: Events
---

Jedes VentoSync-Gerät hat eine [Event-Entität](https://www.home-assistant.io/integrations/event/) `Lüftung Ereignis`. Sie meldet, wenn sich der Grund für das Verhalten der Lüftung dauerhaft ändert oder eine NOx-Stoßlüftung abbricht — ideal als Trigger für Benachrichtigungen und Folgeaktionen.

| | |
| --- | --- |
| **Entitäts-ID** | `event.…_luftung_ereignis` |
| **Zustand** | Zeitstempel des letzten Ereignisses (ISO 8601) |
| **Attribut `event_type`** | Typ des letzten Ereignisses |
| **Attribut `event_types`** | Liste aller möglichen Typen |

## Event-Typen

| `event_type` | Ausgelöst, wenn … | Entprellung |
| --- | --- | --- |
| `aus` | der Raum in den Modus `Aus` geht | 30 s |
| `urlaub` | der Urlaubsmodus beginnt | 30 s |
| `fenster` | die Fenstersperre den Lüfter dieses Geräts stoppt | 30 s |
| `manuell` | ein manueller Modus gewählt wird | 30 s |
| `nox` | eine NOx-Stoßlüftung im Raum läuft | 30 s |
| `co2_notfall` | der CO2-Notfall der Klima-Koordination greift | 30 s |
| `schimmelschutz` | der Schimmelschutz der Klima-Koordination greift | 30 s |
| `uebersteuerung` | eine Übersteuerung beginnt | 30 s |
| `keine_daten` | kein Gerät des Raums Messwerte liefert | 30 s |
| `co2` | die Automatik wegen CO2 lüftet | 30 s |
| `feuchte` | die Automatik wegen Feuchte lüftet | 30 s |
| `grundlueftung` | die Automatik keinen Bedarf mehr sieht | 30 s |
| `nox_abbruch_aussenluft` | eine NOx-Stoßlüftung abbricht, weil das NOx von draußen kommt | sofort |
| `nox_abbruch_max_dauer` | eine NOx-Stoßlüftung nach 60 min Höchstdauer endet | sofort |

Die ersten zwölf Typen entsprechen den Schlüsseln von [`Automatik-Grund`](../entitaeten/automatik-status.md#automatik-grund). Ein Ereignis kommt erst, wenn der neue Grund **30 s ohne Unterbrechung** besteht; kurzes Hin und Her erzeugt keine Ereignisse. Beim Start eines Geräts wird kein Ereignis ausgelöst.

:::info Ein Ereignis pro Gerät
Jedes Gerät eines Raums löst seine Ereignisse selbst aus — ein Raum mit zwei Geräten meldet `co2` also zweimal. Für raumbezogene Automationen nur die Event-Entität **eines** Geräts verwenden (z. B. des Masters).
:::

## Trigger

Event-Entitäten werden mit einem **State-Trigger** abgefragt, der Typ steht im Attribut `event_type`:

```yaml
triggers:
  - trigger: state
    entity_id: event.ventosync_wohnzimmer_luftung_ereignis
    not_from: unavailable   # kein Trigger beim Neustart von HA oder Gerät
conditions:
  - condition: state
    entity_id: event.ventosync_wohnzimmer_luftung_ereignis
    attribute: event_type
    state: co2_notfall
```

Mehrere Typen und Zugriff auf den Typ in den Actions:

```yaml
triggers:
  - trigger: state
    entity_id: event.ventosync_wohnzimmer_luftung_ereignis
    not_from: unavailable
conditions:
  - condition: template
    value_template: >
      {{ trigger.to_state.attributes.event_type in ['nox_abbruch_aussenluft', 'nox_abbruch_max_dauer'] }}
actions:
  - action: notify.notify
    data:
      message: "Lüftung Wohnzimmer: {{ trigger.to_state.attributes.event_type }}"
```

:::tip Event oder Zustand?
- **Event** (`Lüftung Ereignis`): „Benachrichtige mich, **wenn** …“ — entprellt, ein Trigger pro Wechsel.
- **Zustand** (`Automatik-Grund`): „Tu etwas, **solange** …“ — als Bedingung oder in Vorlagen.
:::

## Logbuch und Verlauf

Home Assistant zeigt die Ereignisse im Logbuch und auf der Entitätsseite. Mit einer Vorlage lässt sich ein lesbarer Text daraus machen:

```jinja
{% set texte = {
  'aus': 'Lüftung aus', 'urlaub': 'Urlaubsmodus', 'fenster': 'Fenster offen – pausiert',
  'manuell': 'Manueller Betrieb', 'nox': 'NOx-Stoßlüftung', 'co2_notfall': 'CO2-Notfall',
  'schimmelschutz': 'Schimmelschutz', 'uebersteuerung': 'Übersteuerung',
  'keine_daten': 'Keine Sensordaten', 'co2': 'CO2 erhöht', 'feuchte': 'Feuchte erhöht',
  'grundlueftung': 'Grundlüftung', 'nox_abbruch_aussenluft': 'NOx von draußen – abgebrochen',
  'nox_abbruch_max_dauer': 'NOx-Stoßlüftung beendet (60 min)' } %}
{{ texte.get(state_attr('event.ventosync_wohnzimmer_luftung_ereignis', 'event_type'), 'unbekannt') }}
```
