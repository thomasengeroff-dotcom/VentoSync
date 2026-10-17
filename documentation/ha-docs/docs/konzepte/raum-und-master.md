---
title: Raum, Master & raumweite Einstellungen
sidebar_label: Raum & Master
---

VentoSync-Geräte arbeiten paarweise oder in Gruppen: Ein Gerät bläst Luft herein, während das andere Luft absaugt, und nach jedem Zyklus wechseln beide die Richtung. Damit das funktioniert, teilen die Geräte eines Raums ihren Zustand per **ESP-NOW** — direkt von Gerät zu Gerät, ohne Router und ohne Home Assistant.

Für Home Assistant bedeutet das: **Viele Entitäten gelten für den ganzen Raum**, auch wenn jedes Gerät sie als eigene Entität anbietet.

## Raumgruppe

| Begriff | Bedeutung |
| --- | --- |
| **Raumgruppe** | Alle Geräte mit gleicher `ID Stockwerk` **und** gleicher `ID Raum`. |
| **Master** | Das Gerät mit `ID Gerät` = 1. Es entscheidet für den Raum. |
| **Slave** | Alle übrigen Geräte der Gruppe. Sie übernehmen Modus und — in der Smart-Automatik — die Lüfterstufe des Masters. |
| **Phase** | `Phase A` startet mit Zuluft, `Phase B` mit Abluft. Ein Paar braucht je ein Gerät pro Phase. |
| **Peer** | Ein anderes Gerät der eigenen Raumgruppe, das gerade erreichbar ist (höchstens 10). |

Ein Peer gilt nach 15 Minuten ohne Lebenszeichen als nicht mehr erreichbar. Ist kein Master erreichbar, regelt ein Gerät selbstständig weiter.

:::info Raum zur Laufzeit
Die Raumzuordnung wird zur Laufzeit über die Entitäten `ID Stockwerk`, `ID Raum` und `ID Gerät` eingestellt und im Gerät gespeichert — nicht beim Kompilieren. Deshalb verwendet die Firmware keine raumbezogenen Entitäts-IDs aus Home Assistant; Raum-Informationen wie Fenster- oder Klimastatus schickt Home Assistant per [Action](../integration/actions.md) an ein beliebiges Gerät des Raums.
:::

## Raumweit oder pro Gerät

Jede Einstellung in der [Entitäten-Referenz](../entitaeten/uebersicht.md) ist mit ihrer **Gültigkeit** gekennzeichnet:

| Gültigkeit | Verhalten |
| --- | --- |
| **Raum** | Eine Änderung an **einem** Gerät wird sofort an alle Geräte des Raums übertragen; alle Geräte zeigen danach denselben Wert. Der Master wiederholt raumweite Einstellungen mit jedem Heartbeat (`Sync Intervall`). |
| **Gerät** | Gilt nur für dieses eine Gerät. |
| **Raum (Zustand)** | Ein laufender Zustand, den alle Geräte teilen, z. B. die Restzeit einer Übersteuerung. |

### Raumweite Einstellungen

`Lüftermodus`, `Lüfter Intensität` (und die Fan-Entität), `Durchlüften Dauer (min)`, alle `Smart-Automatik`-Grenzwerte und -Stufen, `Smart-Automatik: Übersteuerung Dauer`, `Radar Lüfter-Anpassung`, `Klima-Koordination` mit ihren drei Reglern, `NOx-Stoßlüftung`, `Maximale LED Helligkeit` und `Sync Intervall`.

### Einstellungen pro Gerät

`Smart-Automatik: Sommerkühlung Schwelle`, `NOx-Stoßlüftung: Schwelle`, `Fenstersperre ignorieren`, `Kindersicherung`, `ESP-NOW Peerprüfung`, `Bluetooth Proxy`, `Urlaubsmodus Betriebsmodus` / `Urlaubsmodus Intensität` (wirksam ist der Wert am Master), die IDs und die Phase sowie Filterzähler, Neustart und BME680-Basiswert.

:::tip Raumweite Werte am Master setzen
Für Automationen und Skripte genügt es, eine raumweite Einstellung an **einem** Gerät pro Raum zu setzen. Nimm dafür den **Master**: Er wiederholt seine Werte regelmäßig, sodass ein Wert, der ihn nicht erreicht hat (z. B. weil er gerade neu startete), beim nächsten Heartbeat wieder überschrieben würde.
:::

## Raumweite Eingänge aus Home Assistant

Klima- und Fensterstatus, die Home Assistant per Action an ein Gerät sendet, teilt dieses Gerät mit dem Raum. Jedes Gerät sendet dabei **nur den Wert, den es selbst bekommen hat** — nie das Ergebnis aus dem ganzen Raum. Der Raum wertet aus:

| Eingang | Raumweite Auswertung |
| --- | --- |
| Klima aktiv (`set_ac_active`) | aktiv, wenn **ein** Gerät des Raums einen aktuellen „aktiv“-Status hat |
| Fenster offen (`set_window_open`) | offen, wenn **ein** Gerät des Raums einen aktuellen „offen“-Status hat |
| Radar-Anwesenheit | anwesend, wenn **ein** Radar im Raum Anwesenheit meldet (30 s nachgehalten) |
| CO2 / Luftfeuchte | **Maximum** der aktuellen Messwerte aller Geräte |
| NOx-Ereignis | aktiv, wenn **ein** Gerät mit SGP41 eine NOx-Stoßlüftung auslöst |

Die Sensor-Entitäten eines Geräts zeigen immer die **eigenen** Messwerte. Den raumweiten Wert, nach dem die Automatik gerade regelt, zeigt [`Automatik-Status`](../entitaeten/automatik-status.md).

## Ohne Home Assistant

Alle Regelfunktionen laufen auf den Geräten selbst. Fällt Home Assistant aus,

- regeln die Geräte mit den zuletzt eingestellten Werten weiter (alle Einstellungen sind im Gerät gespeichert),
- laufen Klima- und Fenstersignale nach 15 Minuten ab (Klima gilt dann als aus, Fenster als geschlossen),
- gilt `Sommerbetrieb` als aus (kein Sommer-Bypass) und die Außenfeuchte als unbekannt,
- bleibt ein laufender Urlaubsmodus aktiv, bis Home Assistant wieder ein echtes „aus“ meldet.
