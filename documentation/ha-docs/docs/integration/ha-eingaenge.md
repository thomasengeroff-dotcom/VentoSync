---
title: Eingänge aus Home Assistant
sidebar_label: Eingänge aus HA
---

Drei Werte **liest** die Firmware aus Home Assistant. Sie müssen in Home Assistant unter genau dieser Entitäts-ID existieren; die Firmware legt sie nicht an. Fehlt ein Eingang, läuft die Lüftung trotzdem — die jeweilige Funktion fällt dann sicher aus.

| Entitäts-ID | Typ | Zweck | Ohne den Eingang |
| --- | --- | --- | --- |
| [`sensor.outdoor_humidity`](#sensoroutdoor_humidity) | Sensor (% rF) | Enthalpie-Schutz der Entfeuchtung, Schimmelschutz | Feuchte-Regelung nur nach dem Innenwert |
| [`binary_sensor.sommerbetrieb`](#binary_sensorsommerbetrieb) | Binary Sensor | Freigabe des Sommer-Bypasses | kein Sommer-Bypass |
| [`input_boolean.ventosync_vacation_mode`](#input_booleanventosync_vacation_mode) | Schalter-Helfer | Urlaubsmodus für alle Geräte | kein Urlaubsmodus |

Die Eingänge gelten für alle VentoSync-Geräte gemeinsam (eine Entität für das ganze Haus). Sie sind in der Firmware `internal` und erscheinen nicht als Geräte-Entitäten.

---

## sensor.outdoor_humidity

Relative Außenluftfeuchte in %. Mit der Außentemperatur der NTC-Fühler berechnet die Firmware daraus die absolute Außenfeuchte und entfeuchtet nur, wenn die Außenluft trockener ist als die Raumluft.

**Aus einer Wetter-Integration** (z. B. Met.no):

```yaml
# configuration.yaml
template:
  - sensor:
      - name: "Outdoor Humidity"
        unique_id: outdoor_humidity
        unit_of_measurement: "%"
        device_class: humidity
        state_class: measurement
        state: "{{ state_attr('weather.forecast_home', 'humidity') }}"
```

**Aus einem eigenen Außensensor:**

```yaml
template:
  - sensor:
      - name: "Outdoor Humidity"
        unique_id: outdoor_humidity
        unit_of_measurement: "%"
        device_class: humidity
        state_class: measurement
        state: "{{ states('sensor.balkon_luftfeuchtigkeit') }}"
        availability: "{{ has_value('sensor.balkon_luftfeuchtigkeit') }}"
```

:::note Entitäts-ID prüfen
Der Name „Outdoor Humidity“ ergibt die ID `sensor.outdoor_humidity`. Heißt der Sensor anders, die ID unter *Einstellungen → Entitäten* anpassen — oder in der Firmware `entity_id:` in `packages/integration/homeassistant.yaml` ändern.
:::

---

## binary_sensor.sommerbetrieb

Gibt den Sommer-Bypass der Smart-Automatik frei. Wann „Sommer“ ist, entscheidet Home Assistant — z. B. nach Jahreszeit und Außentemperatur. Ist der Sensor `off` oder nicht verfügbar, bleibt die Wärmerückgewinnung aktiv (sichere Seite).

```yaml
# configuration.yaml — Sommer: April bis Oktober und außen wärmer als 18 °C
template:
  - binary_sensor:
      - name: "Sommerbetrieb"
        unique_id: sommerbetrieb
        state: >
          {{ now().month in [4, 5, 6, 7, 8, 9, 10]
             and states('sensor.aussentemperatur') | float(0) > 18 }}
        delay_off:
          minutes: 30
```

`delay_off` verhindert, dass der Bypass bei Temperaturschwankungen um 18 °C ständig an- und ausgeht. Der Bypass selbst startet erst, wenn zusätzlich der Raum wärmer als `Smart-Automatik: Sommerkühlung Schwelle` und die Außenluft kühler als der Raum ist — siehe [Sommerkühlung](../entitaeten/smart-automatik.md#sommerkühlung-schwelle).

:::tip Manuell schalten
Wer den Sommerbetrieb selbst bestimmen möchte, legt statt der Vorlage einen Schalter-Helfer an und bildet ihn per Vorlage auf `binary_sensor.sommerbetrieb` ab:

```yaml
template:
  - binary_sensor:
      - name: "Sommerbetrieb"
        unique_id: sommerbetrieb
        state: "{{ is_state('input_boolean.sommerbetrieb_lueftung', 'on') }}"
```
:::

---

## input_boolean.ventosync_vacation_mode

Schaltet den [Urlaubsmodus](../entitaeten/urlaubsmodus.md) aller Geräte.

```yaml
# configuration.yaml
input_boolean:
  ventosync_vacation_mode:
    name: "VentoSync Urlaubsmodus"
    icon: mdi:palm-tree
```

| Zustand | Wirkung |
| --- | --- |
| `on` | Urlaubsmodus beginnt (jeder Raum über seinen Master). |
| `off` | Vorheriger Zustand wird wiederhergestellt. |
| `unavailable` / `unknown` | keine Änderung — ein HA-Neustart beendet den Urlaub nicht. |

Eine andere Entitäts-ID lässt sich beim Kompilieren mit der Substitution `vacation_sensor_id` setzen.

---

## Uhrzeit

Home Assistant ist als Zeitquelle der Geräte eingerichtet, zusätzlich SNTP aus dem Internet. Die Kalenderfrist des Filteralarms (3 Jahre) rechnet mit der SNTP-Zeit — ohne Internetzugang greift nur die Betriebsstunden-Grenze.
