---
title: Klima-Koordination
sidebar_label: Klima-Koordination
---

Die Klima-Koordination (Smart Climate Control) passt die Smart-Automatik an, solange im Raum eine Klimaanlage läuft: Die Lüftung tauscht dann nur so viel Luft aus, wie für die Luftqualität nötig ist, statt die gekühlte oder geheizte Luft nach draußen zu befördern.

Den Klimastatus erfährt das Gerät **nicht** über eine importierte Entität, sondern Home Assistant sendet ihn mit der Action [`set_ac_active`](../integration/actions.md#set_ac_active) an ein beliebiges Gerät des Raums.

Hintergründe: [Smart Climate Control](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/documentation/de/de_smart-climate-control.md).

## Übersicht

| Entität | Entitäts-ID | Typ | Bereich | Standard | Gültigkeit | Kategorie |
| --- | --- | --- | --- | --- | --- | --- |
| Klima-Koordination | `switch.…_klima_koordination` | Switch | an / aus | aus | Raum | config |
| Klima-Koordination: CO2 Grenzwert | `number.…_klima_koordination_co2_grenzwert` | Number | 800 – 1500 ppm, Schritt 50 | 1200 | Raum | config |
| Klima-Koordination: Max Lüfterstufe | `number.…_klima_koordination_max_lufterstufe` | Number | 1 – 5 | 3 | Raum | config |
| Klima-Koordination: CO2 Notfallgrenze | `number.…_klima_koordination_co2_notfallgrenze` | Number | 1200 – 2000 ppm, Schritt 50 | 1500 | Raum | config |
| Klima-Koordination Status | `sensor.…_klima_koordination_status` | Text-Sensor | siehe unten | – | Raum | diagnostic |
| Klima aktiv (HA-Signal) | `binary_sensor.…_klima_aktiv_ha_signal` | Binary Sensor (`running`) | on / off | – | Gerät | diagnostic |

Die Startwerte der drei Regler stammen aus den Substitutionen `hvac_default_co2_threshold`, `hvac_default_max_fan_level` und `hvac_default_emergency_co2`. Alle Einstellungen werden gespeichert.

## Wirkung

Die Klima-Koordination greift nur, wenn **alle** Bedingungen erfüllt sind:

1. Schalter `Klima-Koordination` ist an,
2. Betriebsmodus ist `Smart-Automatik`,
3. mindestens ein Gerät des Raums hat einen aktuellen Status „Klima aktiv“ (nicht älter als 15 min).

Dann gilt:

| Parameter | Normale Smart-Automatik | Bei aktiver Klimaanlage |
| --- | --- | --- |
| CO2-Sollwert | `Smart-Automatik: CO2 Grenzwert` | `Klima-Koordination: CO2 Grenzwert` |
| Stufenbereich | Min … Max Lüfterstufe | 1 … `Klima-Koordination: Max Lüfterstufe` |
| Feuchte-Regelung | aktiv | aus (die Klimaanlage entfeuchtet) |
| Sommer-Bypass | erlaubt | gesperrt, Wärmerückgewinnung erzwungen |

Schaltet die Klimaanlage aus, hebt die Firmware die Einschränkungen erst nach **120 s** durchgehendem „aus“ auf — das glättet kurze Unterbrechungen.

## Schutzfunktionen

| Schutz | Eintritt | Freigabe | Wirkung |
| --- | --- | --- | --- |
| **CO2-Notfall** | CO2 im Raum ≥ `CO2 Notfallgrenze` | CO2 ≤ `Klima-Koordination: CO2 Grenzwert` | normale Grenzen und Sollwerte, Wärmerückgewinnung bleibt erzwungen |
| **Schimmelschutz** | Feuchte ≥ 70 % **und** Außenluft absolut trockener | Feuchte ≤ 65 % oder Außenluft feuchter | wie CO2-Notfall |

Die Firmware hält die Notfallgrenze immer mindestens 100 ppm über dem gelockerten CO2-Grenzwert.

## Klima-Koordination Status

| Wert | Bedeutung |
| --- | --- |
| `Deaktiviert` | Schalter aus. |
| `Inaktiv (kein Smart-Automatik)` | Schalter an, aber ein manueller Modus ist gewählt. |
| `Bereit (Klima aus)` | Bereit, Klimaanlage aus — normale Smart-Automatik. |
| `Aktiv (gedrosselt)` | Klimaanlage aktiv, Lüftung gedrosselt. |
| `Notfall (CO2)` | Klimaanlage aktiv, CO2-Notfall. |
| `Notfall (Feuchte)` | Klimaanlage aktiv, Schimmelschutz. |
| `Ausgesetzt (kein CO2-Wert im Raum)` | Klimaanlage aktiv, aber kein Gerät des Raums liefert CO2 — keine Drosselung. |

## Klima aktiv (HA-Signal)

Zeigt den Status, den Home Assistant zuletzt an **dieses** Gerät gesendet hat. Die Firmware wertet ihn nur 15 Minuten lang aus und ignoriert ihn, solange die API-Verbindung getrennt ist; die Entität selbst behält dabei den zuletzt gesendeten Wert. Ein Gerät, an das Home Assistant nichts sendet, zeigt hier `off`, folgt aber trotzdem dem Raum, wenn ein anderes Gerät „aktiv“ meldet.

## Nutzung

1. Schalter `Klima-Koordination` an einem Gerät des Raums einschalten (gilt raumweit).
2. In Home Assistant eine Automation anlegen, die den Status der Klimaanlage bei jeder Änderung **und alle 5 Minuten** sendet — fertiges Beispiel: [Klimaanlage koppeln](../beispiele/automationen.md#klimaanlage-koppeln).

:::caution `hvac_mode` statt `hvac_action`
„Klima aktiv“ sollte aus dem gewählten Betriebsmodus der `climate`-Entität (`state`: `cool`, `heat`, `heat_cool`, `dry`, `auto`) abgeleitet werden, **nicht** aus dem Attribut `hvac_action`. Der Kompressor wechselt im Betrieb ständig zwischen `cooling` und `idle`; die Lüftung würde mitpendeln. `fan_only` zählt als inaktiv.
:::
