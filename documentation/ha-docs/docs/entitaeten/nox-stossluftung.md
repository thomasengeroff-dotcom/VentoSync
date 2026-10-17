---
title: NOx-Stoßlüftung
sidebar_label: NOx-Stoßlüftung
---

In Räumen mit Gasherd, Kamin oder anderen Verbrennungsquellen erhöht die NOx-Stoßlüftung die Lüftung des ganzen Raums für begrenzte Zeit, sobald der NOx-Index des **SGP41** ansteigt. Den Sensor haben nur die `sgp41`-Varianten; die anderen Geräte des Raums lüften aber mit.

## Übersicht

| Entität | Entitäts-ID | Typ | Bereich | Standard | Gültigkeit | Varianten | Kategorie |
| --- | --- | --- | --- | --- | --- | --- | --- |
| NOx-Stoßlüftung | `switch.…_nox_stossluftung` | Switch | an / aus | aus | Raum | alle | config |
| NOx-Stoßlüftung: Schwelle | `number.…_nox_stossluftung_schwelle` | Number | 20 – 100, Schritt 5 | 20 | Gerät | nur `sgp41` | config |

Den Schalter gibt es in **allen** Varianten, damit auch ein Master ohne SGP41 die raumweite Einstellung führen und der Stoßlüftung folgen kann. Beide Einstellungen werden gespeichert.

## Ablauf

| Phase | Bedingung |
| --- | --- |
| **Start** | NOx-Index ≥ `Schwelle` für **60 s** — nur in der `Smart-Automatik` und nur bei eingeschaltetem Schalter. |
| **Stoßlüftung** | Der Lüftungsbedarf des Raums wird auf 100 % gesetzt; die Automatik fährt mit ±1 Stufe pro 10 s bis `Smart-Automatik Max Lüfterstufe`. |
| **Ende** | Index **10 min** lang unter der halben Schwelle. |
| **Höchstdauer** | 60 min, dann Abbruch (Event `nox_abbruch_max_dauer`). |
| **Außenluft-Test** | Jede Zuluftphase des SGP41-Geräts muss den Index senken. Tut sie das nicht, kommt das NOx vermutlich von draußen (Straßenverkehr) — sofortiger Abbruch (Event `nox_abbruch_aussenluft`). |
| **Sperre** | Nach einem Abbruch startet keine neue Stoßlüftung, bis der Index unter die halbe Schwelle gefallen ist. |

Ein **starkes Ereignis** (Index ≥ 150) hebt zusätzlich die Obergrenze der Klima-Koordination auf. Jede NOx-Stoßlüftung darf über eine laufende Übersteuerung hinausgehen.

## Status in Home Assistant

| Wo | Anzeige |
| --- | --- |
| `sensor.…_automatik_status` | `NOx-Stoßlüftung: Index 85 → Stufe 7` bzw. `NOx-Stoßlüftung (anderes Gerät im Raum) → Stufe 7` |
| `sensor.…_automatik_grund` | `nox` |
| `event.…_luftung_ereignis` | `nox` (nach 30 s), `nox_abbruch_aussenluft`, `nox_abbruch_max_dauer` |
| `sensor.…_sgp41_nox_index` | aktueller NOx-Index (nur `sgp41`) |

## Schwelle wählen

Der Sensirion-NOx-Index liegt in sauberer Luft bei **1**. Die Standardschwelle 20 ist die empfindlichste Einstellung; wer Fehlauslösungen sieht, erhöht sie schrittweise (Verlauf von `SGP41 NOx Index` beim Kochen ansehen). Der Index-Algorithmus braucht nach dem Einschalten etwa 5 Minuten und rund 12 Stunden, bis er den Raum gelernt hat.

## Beispiel

```yaml
# Hinweis bei NOx von draußen
triggers:
  - trigger: state
    entity_id: event.ventosync_kuche_luftung_ereignis
conditions:
  - condition: state
    entity_id: event.ventosync_kuche_luftung_ereignis
    attribute: event_type
    state: nox_abbruch_aussenluft
actions:
  - action: notify.notify
    data:
      message: "NOx kommt von draußen – Stoßlüftung abgebrochen, Fenster geschlossen halten."
```

Weitere Beispiele: [Automationen](../beispiele/automationen.md#nox-von-draußen-melden).
