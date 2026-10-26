---
title: Sensoren & Raumklima
sidebar_label: Sensoren
---

Messwerte der verbauten Sensoren und daraus berechnete Größen. Alle Sensoren gelten **pro Gerät** — sie zeigen die eigenen Messwerte, nicht den Raum. Welche Sensoren eine Variante hat, steht unter [Hardware-Varianten](../konzepte/varianten.md).

:::info Ausgefallene Sensoren
Fällt ein Sensor aus oder fehlt die Sensorplatine, liefern SCD43, SHT45 und SGP41 nach 2 Minuten mit Lesefehlern **keinen Wert mehr** (in Home Assistant `unknown`) statt den letzten Wert einzufrieren. So regelt die Automatik nie nach veralteten Messwerten.
:::

## Berechnete Werte

| Entität | Entitäts-ID | Einheit | Aktualisierung | Beschreibung |
| --- | --- | --- | --- | --- |
| Effektiver CO2 Wert | `sensor.…_effektiver_co2_wert` | ppm (`carbon_dioxide`) | 30 s | CO2-Wert, nach dem dieses Gerät regelt. |
| CO2 Bewertung | `sensor.…_co2_bewertung` | Text | mit dem CO2-Wert | Einordnung des effektiven CO2-Werts. |

### Effektiver CO2 Wert

Die Quelle hängt von der Variante ab:

1. **SCD43** (NDIR, echte CO2-Messung), sonst
2. **SGP41**-CO2-Äquivalent aus dem VOC-Index (`sgp41`-Varianten), sonst
3. der letzte gültige Wert, höchstens 5 Minuten lang, danach kein Wert.

Für Dashboards und Luftqualitäts-Automationen ist das die richtige CO2-Entität, weil sie in allen Varianten mit Klimasensor existiert. In Varianten ohne Klimasensor (`radar_only`, `NTConly`, `nosensor`) ist sie ausgeblendet.

### CO2 Bewertung

| Wert | CO2 |
| --- | --- |
| `Ausgezeichnet` | ≤ 600 ppm |
| `Gut` | ≤ 800 ppm |
| `Mäßig` | ≤ 1000 ppm |
| `Erhöht` | ≤ 1200 ppm |
| `Schlecht` | ≤ 1400 ppm |
| `Inakzeptabel` | > 1400 ppm |
| `Unbekannt` | kein Wert |
| `Sensor-Fehler` | > 10 000 ppm |

## Wärmerückgewinnung

Berechnet aus den NTC-Fühlern (nach DIN EN 13141-8). In `nosensor`-Varianten ausgeblendet.

| Entität | Entitäts-ID | Einheit | Beschreibung |
| --- | --- | --- | --- |
| WRG Effizienz | `sensor.…_wrg_effizienz` | % | Laufende Schätzung des Wärmebereitstellungsgrads (alle 10 s, nur während des Pendelbetriebs). |
| WRG Effizienz (Zyklus) | `sensor.…_wrg_effizienz_zyklus` | % | Energiebasierter Wirkungsgrad des letzten abgeschlossenen Zu-/Abluft-Zyklus. |
| WRG Energie (Zyklus) | `sensor.…_wrg_energie_zyklus` | Wh | Im letzten Zyklus zurückgewonnene Energie. |
| Temp. Zuluft (nach WRG) | `sensor.…_temp_zuluft_nach_wrg` | °C | Temperatur der Luft, die nach dem Keramikspeicher in den Raum strömt: Mittelwert des Innen-NTC über die letzte Zuluft-Phase (am Ende jeder Zuluft-Phase, nur im WRG-Betrieb, sonst *unbekannt*). |
| WRG Referenz-Messpunkt | `sensor.…_wrg_referenz_messpunkt` | Text | Quelle der Referenztemperaturen. |

Details zur Berechnung: [Wärmerückgewinnung & Effizienz](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/documentation/de/de_heat-recovery-and-efficiency.md).

## NTC-Temperaturfühler

Zwei Thermistoren im Luftkanal, vor und hinter dem Keramikspeicher.

| Entität | Entitäts-ID | Einheit | Kategorie | Beschreibung |
| --- | --- | --- | --- | --- |
| Temp. Abluft (Innen) | `sensor.…_temp_abluft_innen` | °C | – | Raumseitige Lufttemperatur (gefiltert). |
| Temp. Zuluft (Außen) | `sensor.…_temp_zuluft_aussen` | °C | – | Außenseitige Lufttemperatur (gefiltert). |
| Temp. Abluft (Innen) RAW | `sensor.…_temp_abluft_innen_raw` | °C | diagnostic | Ungefilterter Rohwert. |
| Temp. Zuluft (Außen) RAW | `sensor.…_temp_zuluft_aussen_raw` | °C | diagnostic | Ungefilterter Rohwert. |

:::caution Phasengekoppelt
Ein NTC misst nur dann die „richtige“ Luft, wenn sein eigener Luftstrom durch ihn strömt: der Innenfühler bei Abluft, der Außenfühler bei Zuluft. Die gefilterten Werte werden deshalb nur in der passenden Phase aktualisiert. Bei `Durchlüften` (Luft nur in eine Richtung) bleibt einer der beiden Werte stehen — für Automationen ist er dann nicht aussagekräftig.
:::

## SCD43 (CO2, Temperatur, Feuchte)

Varianten `ventosync`, `scd43_only`, `v2_scd43_only`. Die Entitäten heißen aus historischen Gründen `SCD41 …`.

| Entität | Entitäts-ID | Einheit | Beschreibung |
| --- | --- | --- | --- |
| SCD41 CO2 | `sensor.…_scd41_co2` | ppm | NDIR-CO2, Median über 5 Messungen, luftdruckkompensiert (BMP390), automatische Selbstkalibrierung. |
| SCD41 Temperatur | `sensor.…_scd41_temperatur` | °C | Raumtemperatur (4 °C Eigenerwärmung abgezogen). |
| SCD41 Luftfeuchtigkeit | `sensor.…_scd41_luftfeuchtigkeit` | % | Relative Raumluftfeuchte, Eingang der Feuchte-Regelung. |

## SGP41 + SHT45 (VOC, NOx, Temperatur, Feuchte)

Varianten `sgp41`, `v2_sgp41`.

| Entität | Entitäts-ID | Einheit | Beschreibung |
| --- | --- | --- | --- |
| SHT45 Temperatur | `sensor.…_sht45_temperatur` | °C | Raumtemperatur für die Regelung. |
| SHT45 Luftfeuchtigkeit | `sensor.…_sht45_luftfeuchtigkeit` | % | Raumfeuchte für die Regelung. |
| SGP41 VOC Index | `sensor.…_sgp41_voc_index` | Index 1–500 | 100 = Durchschnitt der letzten 24 h; höher = mehr flüchtige organische Stoffe. |
| SGP41 NOx Index | `sensor.…_sgp41_nox_index` | Index 1–500 | 1 = saubere Luft; steuert die [NOx-Stoßlüftung](nox-stossluftung.md). |
| SGP41 VOC CO2-Äquivalent | `sensor.…_sgp41_voc_co2_aquivalent` | ppm (`carbon_dioxide`) | VOC-Index umgerechnet: 400 + 4 × (Index − 50), begrenzt auf 400–2200 ppm. Speist `Effektiver CO2 Wert`. |

Der Index-Algorithmus braucht nach dem Start etwa 1 Minute (VOC) bzw. 5 Minuten (NOx) und rund 12 Stunden, um den Raum zu lernen.

## BMP390 (Luftdruck)

Auf der Platine, alle Varianten.

| Entität | Entitäts-ID | Einheit | Beschreibung |
| --- | --- | --- | --- |
| BMP390 Luftdruck | `sensor.…_bmp390_luftdruck` | hPa | Absoluter Luftdruck; kompensiert die SCD43-Messung. |
| BMP390 Temperatur | `sensor.…_bmp390_temperatur` | °C | Temperatur auf der Platine (Gehäusetemperatur, nicht Raumtemperatur — liegt 10–15 K über dem Raum und wird von der Regelung nie als Raumtemperatur verwendet). Überwacht das Netzteil: Warnung ab 50 °C, Not-Abschaltung ab 60 °C — siehe [Benachrichtigungen](../integration/actions.md#vom-gerät-ausgelöste-benachrichtigungen). |

Aktualisierung alle 60 s.
