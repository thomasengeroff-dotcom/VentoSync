---
title: Smart-Automatik
sidebar_label: Smart-Automatik
---

Im Modus `Smart-Automatik` wählt die Firmware die Lüfterstufe selbst. Zwei PID-Regler ermitteln den Lüftungsbedarf aus CO2 und relativer Luftfeuchte; alle 10 Sekunden wird daraus eine Stufe zwischen Mindest- und Höchststufe berechnet und um höchstens ±1 Stufe pro Zyklus angepasst. Die folgenden Entitäten stellen die Regelung ein.

Hintergründe zur Regelung: [Smart-Automatik-Logik](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/documentation/de/de_smart-automatic-logic.md).

## Übersicht

| Entität | Entitäts-ID | Bereich | Standard | Gültigkeit | Kategorie |
| --- | --- | --- | --- | --- | --- |
| Smart-Automatik: CO2 Grenzwert | `number.…_smart_automatik_co2_grenzwert` | 400 – 2000 ppm, Schritt 50 | 1000 | Raum | – |
| Smart-Automatik: Feuchte Grenzwert | `number.…_smart_automatik_feuchte_grenzwert` | 40 – 100 %, Schritt 5 | 60 | Raum | – |
| Smart-Automatik Min Lüfterstufe | `number.…_smart_automatik_min_lufterstufe` | 1 – 3 | 2 | Raum | – |
| Smart-Automatik Max Lüfterstufe | `number.…_smart_automatik_max_lufterstufe` | 5 – 10 | 7 | Raum | – |
| Smart-Automatik: Übersteuerung Dauer | `number.…_smart_automatik_ubersteuerung_dauer` | 30 – 300 min, Schritt 5 | 60 | Raum | config |
| Automatik-Übersteuerung Restzeit | `sensor.…_automatik_ubersteuerung_restzeit` | min | – | Raum (Zustand) | – |
| Smart-Automatik: Sommerkühlung Schwelle | `number.…_smart_automatik_sommerkuhlung_schwelle` | 18 – 30 °C | 22 | Raum | config |
| Radar Lüfter-Anpassung | `number.…_radar_lufter_anpassung` | −5 … +5 | 0 | Raum | config |

Alle Einstellungen werden im Gerät gespeichert. Raumweite Einstellungen nur an einem Gerät (am besten dem Master) ändern — siehe [Raum & Master](../konzepte/raum-und-master.md#raumweit-oder-pro-gerät).

---

## CO2 Grenzwert

`number.…_smart_automatik_co2_grenzwert` — Sollwert des CO2-Reglers. Liegt der CO2-Wert des Raums darüber, lüftet die Automatik stärker. Die CO2-Regelung ist in der Smart-Automatik immer aktiv und hat Vorrang vor der Feuchte: Sobald CO2 Bedarf meldet, übernimmt der CO2-Regler; die Feuchte kann die Stufe dann nur noch anheben, nicht absenken.

Geregelt wird nach dem **höchsten** aktuellen CO2-Wert aller Geräte des Raums (bei Geräten mit SGP41 nach deren CO2-Äquivalent).

| Wert | Einordnung (DIN EN 13779) |
| --- | --- |
| ≤ 800 ppm | IDA 1 — ausgezeichnet |
| ≤ 1000 ppm | IDA 2 — gut, Pettenkofer-Grenze (**Standard**) |
| ≤ 1200 ppm | IDA 3 — akzeptabel |
| ≤ 1500 ppm | IDA 4 — tolerierbar, nicht für Dauerbetrieb |

## Feuchte Grenzwert

`number.…_smart_automatik_feuchte_grenzwert` — relative Luftfeuchte, oberhalb derer die Automatik zur Entfeuchtung stärker lüftet.

:::info Enthalpie-Schutz
Entfeuchtet wird nur, wenn die Außenluft **absolut** trockener ist als die Raumluft (Magnus-Formel, g/m³). Dafür braucht die Firmware die Außenfeuchte aus Home Assistant — [`sensor.outdoor_humidity`](../integration/ha-eingaenge.md#sensoroutdoor_humidity). Ohne diesen Sensor regelt der Feuchte-Regler allein nach dem Innenwert.
:::

## Min Lüfterstufe & Max Lüfterstufe

- **`Smart-Automatik Min Lüfterstufe`** — Grundlüftung (Feuchteschutz). Die Automatik geht nie darunter, auch ohne Bedarf.
- **`Smart-Automatik Max Lüfterstufe`** — Obergrenze der Automatik, z. B. als Geräuschbegrenzung im Schlafzimmer.

Die Höchststufe begrenzt **nicht** die [Übersteuerung](#übersteuerung) — wer manuell eine höhere Stufe wählt, bekommt sie. Die Klima-Koordination kann die Höchststufe zusätzlich absenken (`Klima-Koordination: Max Lüfterstufe`).

```yaml
# Nachts leiser: Höchststufe 4, morgens wieder 7
- action: number.set_value
  target:
    entity_id: number.ventosync_wohnzimmer_smart_automatik_max_lufterstufe
  data:
    value: 4
```

---

## Übersteuerung

Wählt jemand in der Smart-Automatik eine Stufe von Hand, hält der ganze Raum diese Stufe für eine einstellbare Zeit, statt dass die Automatik sofort zurückregelt. Auslöser:

- die Stufen-Taste am Gerät,
- `number.…_lufter_intensitat` in Home Assistant,
- die Geschwindigkeit der Fan-Entität (`fan.set_percentage`),
- der Stufen-Regler im Web-Dashboard.

### Übersteuerung Dauer

`number.…_smart_automatik_ubersteuerung_dauer` — wie lange eine Übersteuerung gilt. Eine Änderung gilt ab der **nächsten** Übersteuerung; eine laufende behält ihre Restzeit.

### Automatik-Übersteuerung Restzeit

| | |
| --- | --- |
| **Entitäts-ID** | `sensor.…_automatik_ubersteuerung_restzeit` |
| **Firmware-ID** | `auto_override_remaining` |
| **Einheit** | `min` (aufgerundet; aktualisiert alle 30 s und bei jeder Änderung) |
| **Werte** | `0` = keine Übersteuerung, die Automatik regelt |

### Verhalten

| Situation | Ergebnis |
| --- | --- |
| Restzeit läuft ab | Die Automatik regelt von der Übersteuerungsstufe aus mit der normalen Rampe weiter. |
| Moduswechsel (auch erneutes Wählen von `Smart-Automatik`) | Übersteuerung endet sofort. |
| Klima-Koordination drosselt den Raum | Die Übersteuerung wird auf `Klima-Koordination: Max Lüfterstufe` begrenzt. |
| CO2-Notfall, Schimmelschutz oder NOx-Stoßlüftung | Die Automatik darf höher lüften als die Übersteuerung. |
| Gerät startet neu | Ein neu gestartetes Gerät übernimmt die laufende Übersteuerung des Raums, beendet sie aber nicht. |

```yaml
# Übersteuerung vorzeitig beenden: Smart-Automatik erneut wählen
action: select.select_option
target:
  entity_id: select.ventosync_wohnzimmer_luftermodus
data:
  option: Smart-Automatik
```

```yaml
# Bedingung: keine Übersteuerung aktiv
condition: numeric_state
entity_id: sensor.ventosync_wohnzimmer_automatik_ubersteuerung_restzeit
below: 1
```

---

## Sommerkühlung Schwelle

`number.…_smart_automatik_sommerkuhlung_schwelle` — Raumtemperatur, ab der die Automatik den **Sommer-Bypass** starten darf: Statt Wärme zurückzugewinnen, lüftet das Gerät in eine Richtung (wie `Durchlüften`) und holt kühlere Außenluft herein. Die Schwelle gilt raumweit (seit 0.10.54): Eine Änderung an einem Gerät übernehmen alle Geräte des Raums.

Der Bypass startet, wenn **alle** Bedingungen erfüllt sind:

1. Raumtemperatur > Schwelle,
2. Außentemperatur mindestens 1,5 °C unter der Raumtemperatur,
3. [`binary_sensor.sommerbetrieb`](../integration/ha-eingaenge.md#binary_sensorsommerbetrieb) in Home Assistant ist `on`,
4. die Klima-Koordination drosselt den Raum nicht.

Er endet, wenn `Sommerbetrieb` ausgeht, außen nicht mehr mindestens 0,5 °C kühler ist oder der Raum unter Schwelle − 0,5 °C abkühlt. Die Temperaturen stammen von den NTC-Fühlern im Gerät bzw. von anderen Geräten des Raums.

Die Schwelle gilt **pro Gerät**. Während des Bypasses hängt `Automatik-Status` den Zusatz `· Sommer-Kühlung` an.

---

## Radar Lüfter-Anpassung

`number.…_radar_lufter_anpassung` — Stufen, die in den **manuellen** Modi (`Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung`) zur eingestellten Stufe addiert werden, solange ein Radar im Raum Anwesenheit erkennt. `0` = aus.

- `+2`: mehr Frischluft, wenn jemand im Raum ist.
- `−2`: leiser, wenn jemand im Raum ist (z. B. Arbeitszimmer).

Die Anwesenheit gilt raumweit (jedes LD2450-Radar des Raums zählt) und wird 30 s nachgehalten. In der Smart-Automatik wird der Wert **nicht** angewendet; die Panel-LEDs zeigen weiterhin die Grundstufe. Siehe auch [Radar](radar.md).
