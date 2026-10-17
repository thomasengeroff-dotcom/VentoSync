---
title: Betriebsmodus & Lüfter
sidebar_label: Betriebsmodus & Lüfter
---

Diese Entitäten wählen den Betriebsmodus und die Lüfterstufe und zeigen, was der Lüfter gerade tut. Modus und Stufe sind **raumweit**: Eine Änderung an einem Gerät übernehmen alle Geräte der Raumgruppe.

## Übersicht

| Entität | Entitäts-ID | Typ | Gültigkeit |
| --- | --- | --- | --- |
| [Lüftermodus](#lüftermodus) | `select.…_luftermodus` | Select | Raum |
| [Lüfter Intensität](#lüfter-intensität) | `number.…_lufter_intensitat` | Number | Raum |
| [Durchlüften Dauer (min)](#durchlüften-dauer-min) | `number.…_durchluften_dauer_min` | Number | Raum |
| [Lüfter Drehzahl](#lüfter-drehzahl) | `sensor.…_lufter_drehzahl` | Sensor | Gerät |
| [Lüfter PWM](#lüfter-pwm) | `sensor.…_lufter_pwm` | Sensor | Gerät |
| [Lüfter Richtung](#lüfter-richtung) | `sensor.…_lufter_richtung` | Text-Sensor | Gerät |

Die Fan-Entität `VentoSync HRV`, die Modus und Stufe zusammenfasst, hat eine [eigene Seite](fan.md).

---

## Lüftermodus

Wählt den Betriebsmodus des Raums.

| | |
| --- | --- |
| **Entitäts-ID** | `select.…_luftermodus` |
| **Firmware-ID** | `luefter_modus` |
| **Optionen** | `Smart-Automatik`, `Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung`, `Aus` |
| **Standard** | `Smart-Automatik` (beim allerersten Start) |
| **Gültigkeit** | Raum |
| **Gespeichert** | ja — nach einem Neustart läuft der zuletzt gewählte Modus weiter |

### Betriebsmodi

| Option | Verhalten |
| --- | --- |
| `Smart-Automatik` | Sensorgeführte Regelung nach CO2 und Luftfeuchte (PID), mit Sommer-Bypass, Klima-Koordination und NOx-Stoßlüftung. Stufe zwischen `Smart-Automatik Min Lüfterstufe` und `Smart-Automatik Max Lüfterstufe`. Siehe [Smart-Automatik](smart-automatik.md). |
| `Wärmerückgewinnung` | Pendelbetrieb: Die Geräte wechseln zyklisch zwischen Zu- und Abluft (70 s bei Stufe 1 bis 50 s bei Stufe 10); der Keramikspeicher hält die Wärme im Raum. Manuelle Stufe. |
| `Durchlüften` | Dauerhafter Luftstrom in eine Richtung (Phase A rein, Phase B raus) — Querlüftung ohne Wärmerückgewinnung, z. B. zur Nachtkühlung. Endet nach `Durchlüften Dauer (min)` und wechselt dann in `Wärmerückgewinnung`. |
| `Stoßlüftung` | 2-Stunden-Zyklus: 15 min Lüften in eine Richtung, 105 min Pause; die Richtung wechselt bei jedem zweiten Durchgang. Läuft auf der manuellen Stufe. |
| `Aus` | Lüfter steht. WLAN, Sensoren, Home-Assistant-Verbindung und ESP-NOW bleiben aktiv. Auch `Aus` gilt raumweit. |

:::warning Exakte Schreibweise
Die Optionen müssen in Automationen **exakt** so geschrieben werden — mit Bindestrich in `Smart-Automatik` und Umlauten in `Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung`. Eine abweichende Schreibweise wird ignoriert.
:::

### Nutzung

```yaml
action: select.select_option
target:
  entity_id: select.ventosync_wohnzimmer_luftermodus
data:
  option: Durchlüften
```

```yaml
# Bedingung: Raum läuft in der Smart-Automatik
condition: state
entity_id: select.ventosync_wohnzimmer_luftermodus
state: Smart-Automatik
```

:::note Modi in Ausnahmesituationen
Fenstersperre und Urlaubsmodus ändern den Lüftermodus nicht sichtbar bzw. setzen ihn danach zurück:

- **Fenster offen:** Der Lüfter steht, `Lüftermodus` behält seinen Wert. Nach dem Schließen geht es im selben Modus weiter.
- **Urlaubsmodus:** Der Master schaltet den Raum in `Urlaubsmodus Betriebsmodus` und stellt danach den vorherigen Modus wieder her.
- **Sommer-Bypass:** In der Smart-Automatik lüftet das Gerät zeitweise wie `Durchlüften`; `Lüftermodus` bleibt `Smart-Automatik`.
:::

---

## Lüfter Intensität

Lüfterstufe des Raums.

| | |
| --- | --- |
| **Entitäts-ID** | `number.…_lufter_intensitat` |
| **Firmware-ID** | `fan_intensity_display` |
| **Bereich** | 1 – 10, Schritt 1 (Schieberegler) |
| **Gültigkeit** | Raum |
| **Gespeichert** | ja |

| Modus | Bedeutung des Werts | Wirkung beim Setzen |
| --- | --- | --- |
| `Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung` | manuelle Stufe | neue Stufe für den Raum |
| `Smart-Automatik` | von der Automatik gewählte Stufe | startet die raumweite [Übersteuerung](smart-automatik.md#übersteuerung) für `Smart-Automatik: Übersteuerung Dauer` |

Begrenzt die Klima-Koordination den Raum gerade (z. B. auf Stufe 3), zeigt die Entität nach dem Setzen die **tatsächlich angewendete** Stufe.

In den manuellen Modi addiert die Firmware bei erkannter Anwesenheit den Wert von `Radar Lüfter-Anpassung` zur Stufe; `Lüfter Intensität` zeigt weiterhin die Grundstufe.

```yaml
action: number.set_value
target:
  entity_id: number.ventosync_wohnzimmer_lufter_intensitat
data:
  value: 4
```

---

## Durchlüften Dauer (min)

Laufzeit des manuellen Modus `Durchlüften`.

| | |
| --- | --- |
| **Entitäts-ID** | `number.…_durchluften_dauer_min` |
| **Firmware-ID** | `vent_timer` |
| **Bereich** | 0 – 120 min, Schritt 5 |
| **Standard** | 30 |
| **Gültigkeit** | Raum |
| **Gespeichert** | ja |

- `0` = Dauerbetrieb ohne Zeitbegrenzung.
- Nach Ablauf wechselt der Raum in `Wärmerückgewinnung`.
- Gilt **nicht** für `Stoßlüftung` (fester 15/105-min-Zyklus) und nicht für den automatischen Sommer-Bypass der Smart-Automatik.
- Eine Änderung während `Durchlüften` läuft, gilt sofort; gezählt wird ab dem Start der Durchlüftung (30 → 60 min verlängert die laufende Durchlüftung um 30 min).

```yaml
# 45 Minuten querlüften
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

---

## Lüfter Drehzahl

| | |
| --- | --- |
| **Entitäts-ID** | `sensor.…_lufter_drehzahl` |
| **Firmware-ID** | `fan_rpm` |
| **Einheit** | `RPM` (`state_class: measurement`) |
| **Aktualisierung** | alle 2 s |
| **Gültigkeit** | Gerät |

Drehzahl des Lüfters. **Negative Werte** bedeuten Abluft (Luft nach draußen), positive Zuluft. Bei einem 4-Pin-Lüfter mit Tachosignal ist der Wert gemessen, sonst geschätzt (Drehzahlanteil × 4200 RPM).

---

## Lüfter PWM

| | |
| --- | --- |
| **Entitäts-ID** | `sensor.…_lufter_pwm` |
| **Firmware-ID** | `fan_pwm_percent` |
| **Einheit** | `%` (`state_class: measurement`) |
| **Aktualisierung** | alle 2 s |
| **Gültigkeit** | Gerät |

Aktuelles PWM-Tastverhältnis des Lüftermotors: **50 % = Stillstand**, unter 50 % Abluft, über 50 % Zuluft. Stufe 1 entspricht 30 % / 70 %, Stufe 10 entspricht 5 % / 95 %. Stufenwechsel laufen über eine 5-Sekunden-Rampe.

---

## Lüfter Richtung

| | |
| --- | --- |
| **Entitäts-ID** | `sensor.…_lufter_richtung` |
| **Firmware-ID** | `direction_display` |
| **Werte** | `Zuluft (Rein)`, `Abluft (Raus)`, `Stillstand` |
| **Gültigkeit** | Gerät |

Aktuelle Luftrichtung, abgeleitet aus dem PWM-Wert. Der Wert wird nur bei einer Änderung gesendet; im Pendelbetrieb wechselt er also etwa jede Minute.

:::tip Recorder entlasten
`Lüfter Drehzahl`, `Lüfter PWM` und `Lüfter Richtung` ändern sich im Pendelbetrieb ständig. Wer keine Verlaufsdiagramme dieser Werte braucht, schließt sie vom Recorder aus:

```yaml
recorder:
  exclude:
    entity_globs:
      - sensor.ventosync_*_lufter_drehzahl
      - sensor.ventosync_*_lufter_pwm
      - sensor.ventosync_*_lufter_richtung
```
:::
