---
title: Hardware-Varianten & verfügbare Entitäten
sidebar_label: Hardware-Varianten
---

VentoSync gibt es als mehrere Firmware-Varianten, je nachdem, welche Sensoren im Gerät stecken. Fehlende Sensoren werden in der Firmware durch Platzhalter ersetzt, die keine Werte liefern; ihre Entitäten erscheinen **nicht** in Home Assistant. Steuer- und Konfigurationsentitäten sind dagegen in jeder Variante gleich, damit sich alle Geräte eines Raums gleich bedienen lassen.

## Variantenübersicht

| Variante (YAML) | Platine | SCD43 | BME680 | SGP41 + SHT45 | Radar | NTC & WRG | `Effektiver CO2 Wert` |
| --- | --- | :---: | :---: | :---: | :---: | :---: | :---: |
| `ventosync.yaml` | v1.0 | ✅ | ✅ | – | ✅ | ✅ | ✅ |
| `ventosync_bme680_only.yaml` | v1.0 | – | ✅ | – | – | ✅ | ✅ |
| `ventosync_scd43_only.yaml` | v1.0 | ✅ | – | – | – | ✅ | ✅ |
| `ventosync_sgp41.yaml` | v1.0 | – | – | ✅ | – | ✅ | ✅ |
| `ventosync_radar_only.yaml` | v1.0 | – | – | – | ✅ | ✅ | – |
| `ventosync_NTConly.yaml` | v1.0 | – | – | – | – | ✅ | – |
| `ventosync_nosensor.yaml` | v1.0 | – | – | – | – | – | – |
| `ventosync_nosensor_btproxy.yaml` | v1.0 | – | – | – | – | – | – |
| `ventosync_v2_bme680_only.yaml` | v2.0 | – | ✅ | – | – | ✅ | ✅ |
| `ventosync_v2_scd43_only.yaml` | v2.0 | ✅ | – | – | – | ✅ | ✅ |
| `ventosync_v2_sgp41.yaml` | v2.0 | – | – | ✅ | – | ✅ | ✅ |
| `ventosync_v2_nosensor.yaml` | v2.0 | – | – | – | – | – | – |

Der Luftdrucksensor **BMP390** sitzt auf der Platine und ist in allen Varianten vorhanden.

## Was „–“ für die Entitäten bedeutet

| Spalte | Entitäten, die ohne den Sensor fehlen |
| --- | --- |
| SCD43 | `SCD41 CO2`, `SCD41 Temperatur`, `SCD41 Luftfeuchtigkeit` |
| BME680 | alle `BME680 …`-Sensoren und der Button `BME680 Basiswert zurücksetzen` |
| SGP41 + SHT45 | `SHT45 …`, `SGP41 …` und `NOx-Stoßlüftung: Schwelle` |
| Radar | `Radar Anwesenheit`, `Radar Bewegung`, `Radar Stillstand`, `Radar Anzahl Ziele` |
| NTC & WRG | `Temp. Abluft (Innen)`, `Temp. Zuluft (Außen)` (+ `RAW`), `WRG Effizienz`, `WRG Effizienz (Zyklus)`, `WRG Energie (Zyklus)`, `WRG Referenz-Messpunkt` |
| `Effektiver CO2 Wert` | `Effektiver CO2 Wert`, `CO2 Bewertung` |

Nur in `ventosync_nosensor_btproxy.yaml` gibt es zusätzlich den Schalter [`Bluetooth Proxy`](../entitaeten/wartung.md#bluetooth-proxy).

:::info Smart-Automatik ohne eigene Sensoren
Auch Geräte ohne Klimasensor bieten den Modus `Smart-Automatik` und alle Grenzwerte an. Sie übernehmen dann die Lüfterstufe des Masters bzw. regeln nach den Messwerten, die andere Geräte des Raums teilen. Ein Raum braucht also nur **ein** Gerät mit CO2- oder Feuchtesensor.
:::

:::note SGP41: CO2-Äquivalent statt CO2
In den `sgp41`-Varianten ist `Effektiver CO2 Wert` kein gemessener CO2-Wert, sondern ein aus dem VOC-Index berechnetes Äquivalent (400–2200 ppm). Der VOC-Index bezieht sich auf die letzten 24 Stunden: Eine gleichbleibende Belastung sinkt mit der Zeit wieder auf Index 100 (≈ 600 ppm). Gleiches gilt sinngemäß für den BME680-Schätzwert.
:::

## MQTT-Variante

Für Systeme neben Home Assistant (Node-RED, ioBroker, openHAB …) kann das Paket `packages/integration/mqtt.yaml` eingebunden werden. Es veröffentlicht **dieselben Entitäten** zusätzlich unter `ventosync/<floor_id>/<room_id>/<device_id>/…` auf einem MQTT-Broker (Werte der Substitutionen beim Kompilieren, nicht der zur Laufzeit eingestellten IDs). MQTT-Discovery ist abgeschaltet — in Home Assistant bleibt die ESPHome-Integration der einzige Weg, damit keine doppelten Entitäten entstehen. Details: [MQTT-Integration](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/documentation/de/de_mqtt-integration.md).
