---
title: Entitäts-IDs & Benennung
sidebar_label: Entitäts-IDs
---

Alle Entitätsnamen der Firmware sind **deutsch** und für jedes VentoSync-Gerät gleich. Home Assistant setzt die Entitäts-ID beim ersten Einbinden aus dem **Gerätenamen** und dem **Entitätsnamen** zusammen.

## Aufbau einer Entitäts-ID

```text
<domain>.<gerätename>_<entitätsname>
```

| Teil | Herkunft | Beispiel |
| --- | --- | --- |
| `domain` | Entitätstyp in Home Assistant | `select` |
| `gerätename` | `friendly_name` des Geräts (Substitution in der Varianten-YAML) bzw. der in Home Assistant vergebene Gerätename | `VentoSync Wohnzimmer` → `ventosync_wohnzimmer` |
| `entitätsname` | `name:` der Entität in der Firmware | `Lüftermodus` → `luftermodus` |

Ergebnis: `select.ventosync_wohnzimmer_luftermodus`.

Home Assistant wandelt dabei in Kleinbuchstaben, ersetzt Leer- und Sonderzeichen durch `_` und schreibt Umlaute um:

| Zeichen | wird zu | Beispiel |
| --- | --- | --- |
| `ä`, `ö`, `ü` | `a`, `o`, `u` | `Lüfter Intensität` → `lufter_intensitat` |
| `ß` | `ss` | `NOx-Stoßlüftung` → `nox_stossluftung` |
| `-`, `:`, `(`, `)`, `.`, `\|` | `_` (mehrfache zusammengefasst) | `Geräte-Phase (A\|B)` → `gerate_phase_a_b` |

:::note Beispiel-IDs in dieser Dokumentation
Alle Beispiele verwenden ein Gerät mit dem Namen **„VentoSync Wohnzimmer“** (Präfix `ventosync_wohnzimmer`) und dem ESPHome-Node-Namen `ventosync-wohnzimmer`. Ohne eigene Substitution heißt ein Gerät „VentoSync - Smart HRV“; das Präfix ist dann `ventosync_smart_hrv`.

Die Referenztabellen zeigen die IDs **ohne** Gerätepräfix (z. B. `select.…_luftermodus`). Die tatsächliche ID steht auf der Entitätsseite in Home Assistant (*Einstellungen → Entitäten*).
:::

:::warning Umbenennen
Wird das Gerät oder eine Entität in Home Assistant umbenannt, bietet Home Assistant an, die Entitäts-IDs mit umzubenennen. Automationen und Dashboards, die die alte ID verwenden, müssen dann angepasst werden.
:::

## Text-Sensoren erscheinen als `sensor`

ESPHome-`text_sensor`-Entitäten legt Home Assistant in der Domain **`sensor`** an (Zustand = Text, ohne Einheit). Die Firmware-Entität `Automatik-Status` ist also `sensor.…_automatik_status`, nicht `text_sensor.…`.

## Node-Name für Actions

API-Actions verwenden **nicht** den Gerätenamen, sondern den **ESPHome-Node-Namen** (`esphome: name:` bzw. Substitution `device_name`). Bindestriche werden zu Unterstrichen:

```text
esphome.<node_name>_<action>
ventosync-wohnzimmer  →  esphome.ventosync_wohnzimmer_set_window_open
```

Details: [API-Actions](../integration/actions.md).

## Entitätskategorien

Die Firmware ordnet Einstellungen und Diagnosewerte den Home-Assistant-Kategorien zu. Sie stehen auf der Geräteseite in eigenen Abschnitten und werden in automatisch erzeugten Dashboards ausgeblendet.

| Kategorie | Abschnitt auf der Geräteseite | Typische Entitäten |
| --- | --- | --- |
| *(keine)* | Steuerelemente / Sensoren | Betriebsmodus, Lüfterstufe, Messwerte, Status |
| `config` | Konfiguration | Grenzwerte, Klima-Koordination, Kindersicherung, IDs |
| `diagnostic` | Diagnose | WLAN-Signal, Laufzeit, HA-Signale, ESP-NOW-Peers |

## Interne Entitäten

Ein Teil der Firmware-Entitäten ist `internal: true` und erscheint nie in Home Assistant: Taster des Bedienfelds, Panel-LEDs, PID-Regler, Platzhalter („Mock“) fehlender Sensoren sowie die aus Home Assistant importierten Eingänge (`Außenluftfeuchtigkeit`, `Sommerbetrieb`, `Urlaubsmodus Sensor`). Welche Messwerte eine Variante ausblendet, steht unter [Hardware-Varianten](varianten.md).
