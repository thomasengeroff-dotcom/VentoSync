---
title: Radar & Anwesenheit
sidebar_label: Radar
---

Der mmWave-Radarsensor **HLK-LD2450** erkennt Personen im Raum — auch wenn sie still sitzen. Verbaut ist er in den Varianten `ventosync` und `radar_only`.

## Übersicht

| Entität | Entitäts-ID | Typ | Beschreibung |
| --- | --- | --- | --- |
| Radar Anwesenheit | `binary_sensor.…_radar_anwesenheit` | Binary Sensor | `on`, sobald mindestens ein Ziel erkannt wird. |
| Radar Bewegung | `binary_sensor.…_radar_bewegung` | Binary Sensor | `on` bei einem sich bewegenden Ziel. |
| Radar Stillstand | `binary_sensor.…_radar_stillstand` | Binary Sensor | `on` bei einem ruhenden Ziel (z. B. sitzende Person). |
| Radar Anzahl Ziele | `sensor.…_radar_anzahl_ziele` | Sensor | Anzahl erkannter Ziele (0–3). |

Alle vier Entitäten gelten **pro Gerät**.

## Wirkung auf die Lüftung

`Radar Anwesenheit` wird mit dem Raum geteilt: Erkennt **irgendein** Radar des Raums eine Person, gilt der ganze Raum 30 Sekunden lang als belegt. Solange das so ist, addieren die Geräte in den **manuellen** Modi den Wert von [`Radar Lüfter-Anpassung`](smart-automatik.md#radar-lüfter-anpassung) zur Lüfterstufe. In der Smart-Automatik hat die Anwesenheit keinen Einfluss — dort regeln CO2 und Feuchte.

## Nutzung über die Lüftung hinaus

Die Radar-Entitäten sind normale Präsenzsensoren und lassen sich für Licht, Heizung oder Alarmanlagen verwenden:

```yaml
# Licht aus, wenn der Raum 10 Minuten leer ist
triggers:
  - trigger: state
    entity_id: binary_sensor.ventosync_wohnzimmer_radar_anwesenheit
    to: "off"
    for:
      minutes: 10
actions:
  - action: light.turn_off
    target:
      area_id: wohnzimmer
```

:::tip Anwesenheit für die Automatik nutzen
Wer die Lüftung in der Smart-Automatik nach Anwesenheit anpassen möchte, kann per Automation die Höchststufe ändern — z. B. bei leerem Raum `Smart-Automatik Max Lüfterstufe` auf 5 senken. Beispiel: [Anwesenheit](../beispiele/automationen.md#leiser-wenn-niemand-da-ist).
:::
