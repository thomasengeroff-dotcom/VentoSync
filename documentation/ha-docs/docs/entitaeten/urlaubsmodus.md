---
title: Urlaubsmodus
sidebar_label: Urlaubsmodus
---

Der Urlaubsmodus schaltet alle Räume während einer Abwesenheit auf eine sparsame Mindestlüftung und stellt danach den vorherigen Zustand wieder her. Gesteuert wird er von **einem** Schalter-Helfer in Home Assistant, der für alle VentoSync-Geräte gilt.

## Übersicht

| Entität | Entitäts-ID | Typ | Werte | Standard | Gültigkeit | Kategorie |
| --- | --- | --- | --- | --- | --- | --- |
| Urlaubsmodus Betriebsmodus | `select.…_urlaubsmodus_betriebsmodus` | Select | Optionen wie `Lüftermodus` | `Stoßlüftung` | Gerät (wirksam am Master) | config |
| Urlaubsmodus Intensität | `number.…_urlaubsmodus_intensitat` | Number | 1 – 10 | 1 | Gerät (wirksam am Master) | config |
| *Eingang:* `input_boolean.ventosync_vacation_mode` | — | Helfer in HA | on / off | — | alle Geräte | — |

Beide Einstellungen werden gespeichert, aber **nicht** raumweit synchronisiert. Am Master (Gerät 1) einstellen — er wendet den Urlaubsmodus für den Raum an.

## Ablauf

| Schritt | Verhalten |
| --- | --- |
| Helfer → `on` | Jedes Gerät sichert seinen aktuellen Modus und seine Stufe. Der Master jedes Raums schaltet den Raum auf `Urlaubsmodus Betriebsmodus` / `Urlaubsmodus Intensität`, die anderen Geräte folgen ihm. |
| Während des Urlaubs | `Automatik-Status` zeigt `Urlaubsmodus → Stufe 1`, `Automatik-Grund` ist `urlaub`. Ein Neustart (Gerät oder Home Assistant) ändert nichts. |
| Helfer → `off` | Der Master stellt den gesicherten Modus und die Stufe für den Raum wieder her. |
| Helfer `unavailable` | Beendet den Urlaub **nicht** (z. B. während eines HA-Neustarts) — nur ein echtes `off`. |

Ist kein Master erreichbar, übernimmt ein anderes Gerät des Raums. Weil der Master seine Sicherung laufend an den Raum überträgt, stellt auch ein einspringendes Gerät den richtigen Zustand von vor dem Urlaub wieder her.

## Einrichtung

```yaml
# configuration.yaml
input_boolean:
  ventosync_vacation_mode:
    name: "VentoSync Urlaubsmodus"
    icon: mdi:palm-tree
```

Oder per UI: *Einstellungen → Geräte & Dienste → Helfer → Helfer erstellen → Schalter*, Entitäts-ID `input_boolean.ventosync_vacation_mode`. Eine andere ID lässt sich in der Firmware mit der Substitution `vacation_sensor_id` einstellen.

:::tip Mit dem Alarm- oder Abwesenheitsstatus koppeln
Den Helfer z. B. mit dem Abwesenheitsmodus der Alarmanlage oder einem Kalender verknüpfen — Beispiel: [Urlaubsmodus automatisch](../beispiele/automationen.md#urlaubsmodus-automatisch).
:::
