---
title: API-Actions
sidebar_label: Actions
---

Über API-Actions schickt Home Assistant Informationen an ein VentoSync-Gerät, die die Firmware nicht selbst messen kann: ob im Raum die Klimaanlage läuft und ob ein Fenster offen ist. Die ESPHome-Integration legt dafür pro Gerät zwei Actions an.

| Action | Daten | Zweck |
| --- | --- | --- |
| [`esphome.<node>_set_ac_active`](#set_ac_active) | `active: bool` | Klimaanlage des Raums aktiv → [Klima-Koordination](../entitaeten/klima-koordination.md) |
| [`esphome.<node>_set_window_open`](#set_window_open) | `window_open: bool` | Fenster des Raums offen → [Fenstersperre](../entitaeten/fenstersperre.md) |

`<node>` ist der ESPHome-Node-Name des Geräts mit Unterstrichen statt Bindestrichen. Er endet auf die letzten sechs Stellen der MAC-Adresse: `ventosync-a1b2c3` → `esphome.ventosync_a1b2c3_set_window_open`. Ein Umbenennen des Geräts in Home Assistant ändert ihn nicht. Die verfügbaren Actions zeigt *Entwicklerwerkzeuge → Aktionen* (Suche nach `esphome.ventosync_`); die MAC-Adresse steht auf der Geräteseite unter *Geräteinformationen* — siehe [Node-Name für Actions](../konzepte/entitaets-ids.md#node-name-für-actions).

## Gemeinsame Regeln

Beide Actions verhalten sich gleich:

| Regel | Details |
| --- | --- |
| **Ein Gerät pro Raum genügt** | Das Gerät teilt den Status per ESP-NOW mit allen Geräten seiner Raumgruppe. Wer an **alle** Geräte sendet, hat Redundanz. |
| **Raum wird zur Laufzeit bestimmt** | Maßgeblich ist die am Gerät eingestellte `ID Stockwerk` / `ID Raum`, nicht der Gerätename. |
| **Ablauf nach 15 Minuten** | Ein Status gilt 15 min. Danach gilt die Klimaanlage als aus, das Fenster als geschlossen. Die Automation muss den Status daher **alle 5 Minuten** erneut senden. |
| **API getrennt** | Solange die Verbindung zu Home Assistant getrennt ist, wertet das Gerät den eigenen Status nicht aus. Ein aktueller Status eines anderen Geräts zählt weiter. |
| **Kein Echo** | Jedes Gerät teilt nur den Status, den es selbst von Home Assistant bekommen hat — Geräte können sich nicht gegenseitig festhalten. |

:::tip Fehler tolerieren
Mit `continue_on_error: true` an jeder Action bricht die Automation nicht ab, wenn ein Gerät gerade nicht erreichbar ist (z. B. während eines Updates).
:::

---

## set_ac_active

```text
esphome.<node>_set_ac_active
```

| Feld | Typ | Pflicht | Beschreibung |
| --- | --- | :---: | --- |
| `active` | bool | ✅ | `true` = Klimaanlage arbeitet in einem konditionierenden Modus (`cool`, `heat`, `heat_cool`, `dry`, `auto`). `false` = aus oder `fan_only`. |

Wirkung nur bei eingeschaltetem Schalter `Klima-Koordination` und im Modus `Smart-Automatik`. Rückmeldung: [`binary_sensor.…_klima_aktiv_ha_signal`](../entitaeten/klima-koordination.md#klima-aktiv-ha-signal) und `sensor.…_klima_koordination_status`.

```yaml
action: esphome.ventosync_a1b2c3_set_ac_active
data:
  active: true
```

Vollständige Automation mit Wiederholung: [Klimaanlage koppeln](../beispiele/automationen.md#klimaanlage-koppeln).

---

## set_window_open

```text
esphome.<node>_set_window_open
```

| Feld | Typ | Pflicht | Beschreibung |
| --- | --- | :---: | --- |
| `window_open` | bool | ✅ | `true` = mindestens ein Fenster des Raums ist offen. |

Nach 5 s „offen“ stoppen alle Lüfter des Raums (außer Geräte mit `Fenstersperre ignorieren`). Rückmeldung: [`binary_sensor.…_fenster_offen_ha_signal`](../entitaeten/fenstersperre.md#fenster-offen-ha-signal) und `sensor.…_fenstersperre_aktiv`.

```yaml
action: esphome.ventosync_a1b2c3_set_window_open
data:
  window_open: "{{ is_state('binary_sensor.fenster_wohnzimmer', 'on') }}"
```

Vollständige Automation: [Fensterkontakte koppeln](../beispiele/automationen.md#fensterkontakte-koppeln).

---

## Standard-Actions für VentoSync-Entitäten

Alle anderen Steuerungen laufen über die normalen Home-Assistant-Actions der jeweiligen Domain:

| Domain | Action | Beispiel |
| --- | --- | --- |
| `fan` | `fan.turn_on`, `fan.turn_off`, `fan.set_percentage`, `fan.set_preset_mode` | [Fan-Entität](../entitaeten/fan.md#actions) |
| `select` | `select.select_option` | Betriebsmodus, Urlaubsmodus, Phase |
| `number` | `number.set_value` | Grenzwerte, Stufen, Dauer |
| `switch` | `switch.turn_on`, `switch.turn_off`, `switch.toggle` | Klima-Koordination, NOx, Kindersicherung |
| `button` | `button.press` | Filter-Reset, Neustart |
| `update` | `update.install` | Firmware-Update |

---

## Vom Gerät ausgelöste Benachrichtigungen

In einer Richtung ruft die Firmware selbst eine Home-Assistant-Action auf: Die **Übertemperatur-Überwachung** des Netzteils (gemessen mit `BMP390 Temperatur` auf der Platine) sendet Meldungen über `notify.notify`.

| Temperatur | Meldung | Reaktion des Geräts |
| --- | --- | --- |
| ≥ 50 °C | `<Gerätename>: Achtung: Traco-Netzteil Derating-Grenze erreicht: 52.3°C` | Warn-Blinken der Master-LED |
| ≥ 60 °C | Titel `WRG NOT-AUS (Überhitzung)`, Text `… KRITISCHE TEMPERATUR (61.0°C)! Not-Abschaltung eingeleitet!` | Not-Abschaltung: Lüfter stoppt, Gerät geht für 60 min in den Tiefschlaf |
| < 48 °C | `<Gerätename>: Entwarnung: Temperatur wieder im sicheren Bereich (47.5°C)` | Warnung aufgehoben |

:::caution Freigabe in der ESPHome-Integration
Damit diese Meldungen ankommen, muss das Gerät Home-Assistant-Aktionen ausführen dürfen: *Einstellungen → Geräte & Dienste → ESPHome → Gerät → Konfigurieren →* „Erlaube dem Gerät, Home Assistant-Aktionen auszuführen“. Ohne Freigabe schützt sich das Gerät trotzdem, nur die Benachrichtigung fehlt. `notify.notify` muss in Home Assistant auf einen Benachrichtigungsdienst zeigen (z. B. die Companion-App).
:::
