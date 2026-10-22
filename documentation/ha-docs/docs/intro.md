---
title: VentoSync in Home Assistant
sidebar_label: Einführung
slug: /
---

VentoSync ist eine ESPHome-Firmware für dezentrale Lüftungsgeräte mit Wärmerückgewinnung (VentoMaxx V-WRG). Jedes Gerät meldet sich über die **ESPHome-Integration** in Home Assistant an und stellt dort Entitäten zum Steuern, Konfigurieren und Überwachen bereit. Die Geräte eines Raums stimmen sich untereinander per ESP-NOW ab — Home Assistant ist für den Betrieb **nicht** nötig, erweitert ihn aber um Automationen, Klima- und Fensterkopplung, Urlaubsmodus und Dashboards.

Diese Dokumentation beschreibt alle Home-Assistant-Entitäten der Firmware, die API-Actions und Events sowie die Eingänge, die die Firmware aus Home Assistant liest — mit Beispielen für Automationen und Dashboards.

:::info Firmware-Stand
Beschrieben ist die Firmware ab Version **0.10.39** (ESP-NOW-Protokoll v12, ESPHome 2026.9.0). Ältere Versionen haben einzelne Entitäten noch nicht (z. B. `Automatik-Status`, `Lüftung Ereignis`, `NOx-Stoßlüftung`). Änderungen je Version stehen im [CHANGELOG](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/CHANGELOG.md).
:::

## Aufbau dieser Dokumentation

| Bereich | Inhalt |
| --- | --- |
| [Konzepte](konzepte/entitaets-ids.md) | Wie Entitäts-IDs entstehen, was „raumweit“ bedeutet, welche Variante welche Entitäten hat. |
| [Entitäten](entitaeten/uebersicht.md) | Referenz aller Entitäten, nach Funktion gruppiert — mit Wertebereich, Standardwert, Gültigkeit und Nutzung. |
| [Integration](integration/actions.md) | API-Actions (`set_ac_active`, `set_window_open`), die Event-Entität und Eingänge aus Home Assistant. |
| [Beispiele](beispiele/automationen.md) | Fertige Automationen und Dashboard-Karten. |

## Schnellstart

1. **Gerät einbinden.** Nach dem ersten Flashen erkennt Home Assistant das Gerät automatisch (*Einstellungen → Geräte & Dienste → ESPHome*). Den API-Schlüssel aus der `secrets.yaml` beim Hinzufügen eingeben.
2. **Raum zuordnen.** Unter *Konfiguration* auf der Geräteseite `ID Stockwerk`, `ID Raum` und `ID Gerät` setzen. Geräte mit gleichem Stockwerk und Raum bilden eine Gruppe, Gerät 1 ist der Master. Details: [Raum, Master & raumweite Einstellungen](konzepte/raum-und-master.md).
3. **Betriebsmodus wählen.** Über die Fan-Entität `VentoSync HRV` oder den Select `Lüftermodus`, z. B. `Smart-Automatik`.
4. **Optionale Eingänge anlegen.** `sensor.outdoor_humidity`, `binary_sensor.sommerbetrieb` und `input_boolean.ventosync_vacation_mode` — siehe [Eingänge aus Home Assistant](integration/ha-eingaenge.md).
5. **Fenster und Klimaanlage koppeln** (optional) mit den Actions [`set_window_open` und `set_ac_active`](integration/actions.md).

## Die wichtigsten Entitäten

Für den Alltag genügen wenige Entitäten. Beispiel-IDs gelten für ein Gerät mit dem Namen „VentoSync Wohnzimmer“ (siehe [Entitäts-IDs](konzepte/entitaets-ids.md)).

| Entität | Wofür |
| --- | --- |
| `fan.ventosync_wohnzimmer_ventosync_hrv` | Ein/Aus, Stufe 1–10, Betriebsmodus als Preset — ideal für Dashboards und Sprachassistenten. |
| `select.ventosync_wohnzimmer_luftermodus` | Betriebsmodus direkt wählen (inkl. `Aus`). |
| `number.ventosync_wohnzimmer_lufter_intensitat` | Lüfterstufe 1–10. |
| `sensor.ventosync_wohnzimmer_automatik_status` | Klartext: was der Lüfter gerade tut und warum. |
| `event.ventosync_wohnzimmer_luftung_ereignis` | Trigger für Automationen bei Zustandswechseln. |
| `sensor.ventosync_wohnzimmer_effektiver_co2_wert` | CO2-Wert, nach dem die Automatik regelt. |
| `binary_sensor.ventosync_wohnzimmer_filterwechsel_alarm` | Filterwechsel fällig. |

:::tip Lokales Web-Dashboard
Jedes Gerät hat zusätzlich ein eigenes Web-Dashboard unter `http://<ip-adresse>/`, das ohne Home Assistant funktioniert. Siehe [Lokales Web-Dashboard](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/documentation/de/de_local-web-dashboard.md).
:::
