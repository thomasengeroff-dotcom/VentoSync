---
title: Wartung & Bedienfeld
sidebar_label: Wartung & Bedienfeld
---

Filterüberwachung, Bedienfeld-Einstellungen, Neustart und der optionale Bluetooth-Proxy.

## Übersicht

| Entität | Entitäts-ID | Typ | Gültigkeit | Kategorie |
| --- | --- | --- | --- | --- |
| [Filterwechsel Alarm](#filterwechsel-alarm) | `binary_sensor.…_filterwechsel_alarm` | Binary Sensor (`problem`) | Gerät | – |
| [Filter Betriebstage](#filter-betriebstage) | `sensor.…_filter_betriebstage` | Sensor (Tage) | Gerät | diagnostic |
| [Filterwechsel (Reset)](#filterwechsel-reset) | `button.…_filterwechsel_reset` | Button | Gerät | config |
| [Maximale LED Helligkeit](#maximale-led-helligkeit) | `number.…_maximale_led_helligkeit` | Number | Raum | – |
| [Kindersicherung](#kindersicherung) | `switch.…_kindersicherung` | Switch | Gerät | config |
| [ESP Neustart erzwingen](#esp-neustart-erzwingen) | `button.…_esp_neustart_erzwingen` | Button | Gerät | config |
| [BME680 Basiswert zurücksetzen](#bme680-basiswert-zurücksetzen) | `button.…_bme680_basiswert_zurucksetzen` | Button | Gerät | config |
| [Bluetooth Proxy](#bluetooth-proxy) | `switch.…_bluetooth_proxy` | Switch | Gerät | config |

---

## Filterwechsel Alarm

`binary_sensor.…_filterwechsel_alarm` wird `on` (= Problem), wenn

- der Lüfter seit dem letzten Filterwechsel **8760 Betriebsstunden** (365 Tage Laufzeit) gelaufen ist, **oder**
- seit dem letzten Filterwechsel **3 Jahre** vergangen sind.

Gezählt wird nur die Zeit, in der der Lüfter läuft. Die Betriebsstunden werden höchstens alle 8 Stunden in den Flash geschrieben (Schonung des Speichers) — nach einem Stromausfall fehlen also höchstens die letzten 8 Stunden.

## Filter Betriebstage

`sensor.…_filter_betriebstage` — Lüfter-Laufzeit seit dem letzten Filterwechsel in Tagen (Aktualisierung alle 5 min). Der Alarm kommt bei 365.

## Filterwechsel (Reset)

`button.…_filterwechsel_reset` — nach dem Filterwechsel drücken: setzt Betriebsstunden und Kalenderzeit zurück und schaltet den Alarm aus. Jedes Gerät hat eigene Filter und eigene Zähler.

```yaml
action: button.press
target:
  entity_id: button.ventosync_wohnzimmer_filterwechsel_reset
```

Benachrichtigungs-Beispiel: [Filterwechsel melden](../beispiele/automationen.md#filterwechsel-melden).

---

## Maximale LED Helligkeit

| | |
| --- | --- |
| **Entitäts-ID** | `number.…_maximale_led_helligkeit` |
| **Bereich** | 5 – 100 %, Schritt 5 |
| **Standard** | 80 |
| **Gültigkeit** | Raum |

Höchste Helligkeit der Bedienfeld-LEDs. Die LEDs dimmen 30 s nach der letzten Bedienung selbsttätig ab. Die LEDs selbst sind **keine** Licht-Entitäten in Home Assistant — die Firmware steuert sie.

```yaml
# Nachts dunkler (gilt für den ganzen Raum)
action: number.set_value
target:
  entity_id: number.ventosync_schlafzimmer_maximale_led_helligkeit
data:
  value: 10
```

## Kindersicherung

`switch.…_kindersicherung` sperrt die Tasten am Bedienfeld **dieses** Geräts. Home Assistant, Web-Dashboard und die anderen Geräte des Raums steuern weiter. Der Zustand bleibt über einen Neustart erhalten. Siehe [Kindersicherung](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/documentation/de/de_child-lock-implementation.md).

## ESP Neustart erzwingen

`button.…_esp_neustart_erzwingen` startet den ESP32 neu. Einstellungen, Modus und Stufe sind gespeichert und werden nach dem Start mit dem Raum abgeglichen.

## BME680 Basiswert zurücksetzen

`button.…_bme680_basiswert_zurucksetzen` (nur Varianten mit BME680) verwirft den gelernten Gas-Basiswert und startet die Einbrennphase neu (bis zu 48 h). Sinnvoll nach einem Sensortausch oder wenn das CO2-Äquivalent dauerhaft falsch wirkt.

---

## Bluetooth Proxy

:::caution Experimentell
Nur in der Variante `ventosync_nosensor_btproxy.yaml`. Der Wechsel auf diese Variante braucht einmalig einen USB-Flash (andere Partitionstabelle).
:::

`switch.…_bluetooth_proxy` schaltet den [Bluetooth-Proxy](https://esphome.io/components/bluetooth_proxy/) von Home Assistant auf diesem Gerät ein oder aus (Standard: aus, gespeichert). Aus gibt Speicher und Funkzeit frei. Details: [Bluetooth Proxy](https://github.com/thomasengeroff-dotcom/VentoSync/blob/master/documentation/de/de_bluetooth-proxy.md).
