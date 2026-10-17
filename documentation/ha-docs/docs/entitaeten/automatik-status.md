---
title: Automatik-Status & Ereignisse
sidebar_label: Automatik-Status
---

Drei Entitäten beantworten die Frage **„Was macht die Lüftung gerade — und warum?“**. Sie gibt es in allen Varianten und in allen Betriebsmodi (nicht nur in der Smart-Automatik).

| Entität | Entitäts-ID | Typ | Zweck |
| --- | --- | --- | --- |
| Automatik-Status | `sensor.…_automatik_status` | Text-Sensor | Klartext für Dashboards |
| Automatik-Grund | `sensor.…_automatik_grund` | Text-Sensor | stabiler Schlüssel für Bedingungen |
| Lüftung Ereignis | `event.…_luftung_ereignis` | Event | Trigger für Automationen |

---

## Automatik-Status

| | |
| --- | --- |
| **Entitäts-ID** | `sensor.…_automatik_status` |
| **Firmware-ID** | `automatik_status` |
| **Aktualisierung** | alle 10 s, gesendet nur bei Änderung |

Text aus Grund und angewendeter Lüfterstufe, zum Beispiel:

| Beispiel | Situation |
| --- | --- |
| `CO2 erhöht: 1150 ppm (Soll 1000) → Stufe 5` | CO2-Regelung |
| `Feuchte erhöht: 68 % (Soll 60 %) → Stufe 4` | Entfeuchtung |
| `Grundlüftung: kein Bedarf → Stufe 2` | kein Bedarf, Mindeststufe |
| `Übersteuerung: noch 35 min → Stufe 4` | manuell gewählte Stufe in der Smart-Automatik |
| `CO2-Notfall: 1600 ppm → Stufe 7` | Notfall trotz Klimaanlage |
| `Schimmelschutz: 78 % → Stufe 6` | Feuchte-Notfall trotz Klimaanlage |
| `NOx-Stoßlüftung: Index 85 → Stufe 7` | NOx-Ereignis an diesem Gerät |
| `NOx-Stoßlüftung (anderes Gerät im Raum) → Stufe 7` | NOx-Ereignis an einem anderen Gerät |
| `Keine Sensordaten: Stufe 3 gehalten` | kein Gerät des Raums liefert Messwerte |
| `Pausiert: Fenster offen` | Fenstersperre stoppt den Lüfter |
| `Manuell: Wärmerückgewinnung → Stufe 5` | manueller Modus |
| `Urlaubsmodus → Stufe 1` | Urlaubsmodus aktiv |
| `Aus` | Modus `Aus` |

Zusätze am Ende des Texts:

- ` (Klimaanlage: max. Stufe 3)` — die Klima-Koordination begrenzt die Stufe,
- ` · Sommer-Kühlung` — der Sommer-Bypass ist aktiv,
- `, stark` (bei NOx) — starkes NOx-Ereignis (Index ≥ 150), hebt die Klima-Obergrenze auf.

CO2- und Feuchtewerte im Text sind die **raumweiten Höchstwerte**, nach denen tatsächlich geregelt wird — sie können vom eigenen Sensor des Geräts abweichen.

:::warning Nicht für Bedingungen verwenden
Der Text enthält Messwerte und ändert sich laufend. Für Automationen [`Automatik-Grund`](#automatik-grund) oder [`Lüftung Ereignis`](#lüftung-ereignis) verwenden.
:::

---

## Automatik-Grund

| | |
| --- | --- |
| **Entitäts-ID** | `sensor.…_automatik_grund` |
| **Firmware-ID** | `automatik_grund` |
| **Aktualisierung** | sofort bei Änderung (ohne Entprellung) |

Der Grund als stabiler Schlüssel. Bei mehreren gleichzeitigen Gründen gewinnt der in der Tabelle weiter oben stehende.

| Priorität | Schlüssel | Bedeutung |
| :---: | --- | --- |
| 1 | `aus` | Betriebsmodus `Aus` |
| 2 | `urlaub` | Urlaubsmodus aktiv |
| 3 | `fenster` | Fenstersperre stoppt den Lüfter (nicht, wenn `Fenstersperre ignorieren` an ist) |
| 4 | `manuell` | manueller Modus (`Wärmerückgewinnung`, `Durchlüften`, `Stoßlüftung`) |
| 5 | `nox` | NOx-Stoßlüftung im Raum |
| 6 | `co2_notfall` | CO2-Notfall der Klima-Koordination |
| 7 | `schimmelschutz` | Feuchte-Notfall der Klima-Koordination |
| 8 | `uebersteuerung` | Übersteuerung läuft |
| 9 | `keine_daten` | keine Messwerte im Raum, Stufe wird gehalten |
| 10 | `co2` | CO2 über Sollwert |
| 11 | `feuchte` | Luftfeuchte über Sollwert |
| 12 | `grundlueftung` | kein Bedarf, Mindeststufe |

```yaml
# Bedingung: Die Automatik regelt gerade wegen CO2 oder Feuchte
condition: state
entity_id: sensor.ventosync_wohnzimmer_automatik_grund
state:
  - co2
  - feuchte
```

---

## Lüftung Ereignis

| | |
| --- | --- |
| **Entitäts-ID** | `event.…_luftung_ereignis` |
| **Firmware-ID** | `lueftung_ereignis` |
| **Event-Typen** | die 12 Schlüssel von `Automatik-Grund` sowie `nox_abbruch_aussenluft`, `nox_abbruch_max_dauer` |

Eine [Event-Entität](https://www.home-assistant.io/integrations/event/): Ihr Zustand ist der Zeitstempel des letzten Ereignisses, das Attribut `event_type` dessen Typ.

| Event-Typ | Wann |
| --- | --- |
| Grund-Schlüssel (`co2`, `fenster`, …) | wenn ein **neuer** Grund mindestens **30 s** ununterbrochen besteht — kurzes Hin und Her löst nichts aus. Beim Gerätestart wird kein Ereignis ausgelöst. |
| `nox_abbruch_aussenluft` | sofort, wenn eine NOx-Stoßlüftung abbricht, weil die Zuluft den NOx-Index nicht senkt (NOx kommt von draußen). |
| `nox_abbruch_max_dauer` | sofort, wenn eine NOx-Stoßlüftung nach 60 min Höchstdauer endet. |

Trigger-Muster, Attribute und weitere Beispiele: [Events](../integration/events.md).
