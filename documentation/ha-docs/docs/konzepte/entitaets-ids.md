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
Alle Beispiele verwenden ein Gerät, das in Home Assistant **„VentoSync Wohnzimmer“** heißt (Präfix `ventosync_wohnzimmer`), mit dem ESPHome-Node-Namen `ventosync-a1b2c3`.

Die Referenztabellen zeigen die IDs **ohne** Gerätepräfix (z. B. `select.…_luftermodus`). Die tatsächliche ID steht auf der Entitätsseite in Home Assistant (*Einstellungen → Entitäten*).
:::

## Name nach dem Einbinden

Alle Geräte laufen mit derselben Firmware. Damit sie sich unterscheiden, hängt die Firmware die letzten sechs Stellen der MAC-Adresse an den Namen an (`name_add_mac_suffix`). Ein frisch eingebundenes Gerät heißt deshalb z. B. **„VentoSync - Smart HRV a1b2c3“**, seine Entitäten beginnen mit `ventosync_smart_hrv_a1b2c3_`.

Sprechende IDs entstehen, indem man das Gerät in Home Assistant umbenennt (*Geräteseite → ✏️*, z. B. „VentoSync Wohnzimmer“) und die Frage, ob die Entitäts-IDs mit umbenannt werden sollen, bestätigt. Am besten direkt nach dem Einbinden, bevor Automationen und Dashboards die IDs verwenden.

:::warning Umbenennen
Wird das Gerät oder eine Entität in Home Assistant umbenannt, bietet Home Assistant an, die Entitäts-IDs mit umzubenennen. Automationen und Dashboards, die die alte ID verwenden, müssen dann angepasst werden. Die Namen der [API-Actions](#node-name-für-actions) ändern sich dabei **nicht**.
:::

## Bereich in der Entitäts-ID

Neuere Home-Assistant-Versionen (beobachtet ab 2026.6) stellen bei **neu angelegten** Entitäten den Namen des Bereichs voran, dem das Gerät zugeordnet ist:

```text
<domain>.<bereich>_<gerätename>_<entitätsname>
sensor.wohnzimmer_ventosync_wohnzimmer_automatik_status
```

Das betrifft auch Entitäten, die erst durch ein Firmware-Update dazukommen. Ein Gerät kann dann IDs beider Formen haben:

| Entität | angelegt | Entitäts-ID |
| --- | --- | --- |
| `Lüftermodus` | vor dem HA-Update | `select.ventosync_wohnzimmer_luftermodus` |
| `Automatik-Status` | nach dem HA-Update | `sensor.wohnzimmer_ventosync_wohnzimmer_automatik_status` |

Die Beispiele dieser Dokumentation verwenden die Form **ohne** Bereich. Vor dem Übernehmen eines Beispiels die tatsächliche ID auf der Entitätsseite nachsehen. Wer einheitliche IDs möchte, ändert die betroffenen Entitäts-IDs in den Entitätseinstellungen von Hand.

:::caution Dashboard-Vorlagen
Vorlagen, die IDs aus einem Gerätepräfix zusammensetzen (z. B. `sensor.[[device]]_automatik_status` in einer `decluttering-card`), finden Entitäten mit Bereichspräfix nicht.
:::

## Text-Sensoren erscheinen als `sensor`

ESPHome-`text_sensor`-Entitäten legt Home Assistant in der Domain **`sensor`** an (Zustand = Text, ohne Einheit). Die Firmware-Entität `Automatik-Status` ist also `sensor.…_automatik_status`, nicht `text_sensor.…`.

## Node-Name für Actions

API-Actions verwenden **nicht** den Gerätenamen aus Home Assistant, sondern den **ESPHome-Node-Namen**. Er lautet `ventosync-` plus die letzten sechs Stellen der MAC-Adresse (Substitution `device_name` und `name_add_mac_suffix`). Bindestriche werden zu Unterstrichen:

```text
esphome.<node_name>_<action>
ventosync-a1b2c3  →  esphome.ventosync_a1b2c3_set_window_open
```

Ein Umbenennen des Geräts in Home Assistant ändert die Action-Namen nicht. Welche Action zu welchem Gerät gehört, zeigt die MAC-Adresse unter *Geräteinformationen* auf der Geräteseite; alle Actions listet *Entwicklerwerkzeuge → Aktionen* (Suche nach `esphome.ventosync_`).

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
