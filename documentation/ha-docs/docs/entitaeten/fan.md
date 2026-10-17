---
title: Fan-Entität
sidebar_label: Fan (VentoSync HRV)
---

Die Fan-Entität `VentoSync HRV` bildet das Lüftungsgerät als Standard-Ventilator von Home Assistant ab: Ein/Aus, 10 Geschwindigkeitsstufen und die Betriebsmodi als Presets. Sie ist die empfohlene Entität für Dashboards, Sprachassistenten und einfache Automationen, weil alle Home-Assistant-Karten und Assistenten Ventilatoren ohne Anpassung verstehen.

| | |
| --- | --- |
| **Entitäts-ID** | `fan.…_ventosync_hrv` |
| **Firmware-ID** | `ventosync_hrv_fan` (`packages/integration/ha_fan_entity.yaml`) |
| **Gültigkeit** | Raum — Modus und Stufe gelten für die ganze Raumgruppe |
| **Varianten** | alle |

## Eigenschaften

| Eigenschaft | Typ | Werte | Beschreibung |
| --- | --- | --- | --- |
| `state` | string | `on`, `off` | `off` = Betriebsmodus `Aus`, sonst `on`. |
| `percentage` | int | `0`, `10` … `100` | Lüfterstufe × 10 (Stufe 3 = 30 %). |
| `percentage_step` | float | `10` | 10 Stufen (`speed_count: 10`). |
| `preset_mode` | string | siehe unten | Aktueller Betriebsmodus. |
| `preset_modes` | list | `Smart-Automatik`, `Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung` | Wählbare Modi. `Aus` ist kein Preset, sondern `state: off`. |

Richtung und Oszillation werden **nicht** unterstützt — die Luftrichtung wechselt die Firmware selbst (siehe [`Lüfter Richtung`](betriebsmodus.md#lüfter-richtung)).

:::info Abgleich mit dem Gerät
Die Fan-Entität spiegelt den Gerätezustand alle 3 Sekunden. Änderungen am Bedienfeld, im Web-Dashboard, über `Lüftermodus` oder von einem anderen Gerät des Raums erscheinen also spätestens nach 3 Sekunden. Die Firmware unterdrückt dabei Rückkopplungen: Der Abgleich selbst löst keine neue Änderung aus.
:::

## Unterstützte Funktionen

| Funktion | Unterstützt |
| --- | :---: |
| `TURN_ON` / `TURN_OFF` | ✅ |
| `SET_SPEED` | ✅ |
| `PRESET_MODE` | ✅ |
| `DIRECTION` | – |
| `OSCILLATE` | – |

## Actions

### Einschalten

`fan.turn_on` schaltet den Raum in den **zuletzt aktiven Modus** zurück (wie die Power-Taste am Gerät). War noch nie ein Modus aktiv, startet `Smart-Automatik`.

```yaml
action: fan.turn_on
target:
  entity_id: fan.ventosync_wohnzimmer_ventosync_hrv
```

### Ausschalten

`fan.turn_off` setzt den Betriebsmodus `Aus` für den ganzen Raum. WLAN, Sensoren und Home-Assistant-Verbindung bleiben aktiv — es gibt keinen Schlafzustand.

```yaml
action: fan.turn_off
target:
  entity_id: fan.ventosync_wohnzimmer_ventosync_hrv
```

### Stufe setzen

`fan.set_percentage` setzt die Lüfterstufe (`percentage / 10`; Home Assistant rundet auf die nächste ganze Stufe **auf**, 55 % → Stufe 6). `percentage: 0` schaltet aus.

```yaml
action: fan.set_percentage
target:
  entity_id: fan.ventosync_wohnzimmer_ventosync_hrv
data:
  percentage: 60   # Stufe 6
```

| Aktueller Modus | Wirkung |
| --- | --- |
| `Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung` | Neue manuelle Stufe für den Raum. |
| `Smart-Automatik` | Startet die raumweite **Übersteuerung**: Die Stufe gilt für `Smart-Automatik: Übersteuerung Dauer` (Standard 60 min), danach regelt die Automatik wieder. Siehe [Übersteuerung](smart-automatik.md#übersteuerung). |

`fan.increase_speed` / `fan.decrease_speed` funktionieren ebenso (Schrittweite 10 %).

### Betriebsmodus wählen

```yaml
action: fan.set_preset_mode
target:
  entity_id: fan.ventosync_wohnzimmer_ventosync_hrv
data:
  preset_mode: Wärmerückgewinnung
```

Der Preset-Name muss **exakt** einem der vier Modi entsprechen, inklusive Bindestrich und Umlaut (`Smart-Automatik`, `Wärmerückgewinnung`). Ein Moduswechsel beendet eine laufende Übersteuerung.

:::tip Modus und Stufe zusammen setzen
Erst den Modus, dann die Stufe setzen — in zwei Actions. Andersherum würde der Moduswechsel eine gerade gestartete Übersteuerung in der Smart-Automatik wieder beenden.

```yaml
- action: fan.set_preset_mode
  target:
    entity_id: fan.ventosync_wohnzimmer_ventosync_hrv
  data:
    preset_mode: Stoßlüftung
- action: fan.set_percentage
  target:
    entity_id: fan.ventosync_wohnzimmer_ventosync_hrv
  data:
    percentage: 80
```
:::

## Fan-Entität oder Select?

| Aufgabe | Empfohlene Entität |
| --- | --- |
| Dashboard-Kachel, Sprachsteuerung, Ein/Aus | `fan.…_ventosync_hrv` |
| Modus `Aus` in einer Auswahlliste anbieten | [`select.…_luftermodus`](betriebsmodus.md#lüftermodus) |
| Stufe als Zahl (1–10) statt Prozent | [`number.…_lufter_intensitat`](betriebsmodus.md#lüfter-intensität) |
| Bedingung „Modus ist X“ in Automationen | beides möglich: `preset_mode` oder Zustand des Selects |

Alle drei Entitäten steuern denselben Zustand; welche du verwendest, ist Geschmackssache.

## Sprachassistenten

Über Assist, Alexa oder Google Home (via Home Assistant) funktionieren u. a.:

- „Schalte VentoSync Wohnzimmer aus“ → `turn_off` (Modus `Aus`)
- „Stelle VentoSync Wohnzimmer auf 50 Prozent“ → Stufe 5
- „Schalte VentoSync Wohnzimmer ein“ → letzter aktiver Modus

Für eingängigere Befehle der Entität in Home Assistant einen Alias geben, z. B. „Lüftung Wohnzimmer“.
