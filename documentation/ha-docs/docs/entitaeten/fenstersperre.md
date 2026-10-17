---
title: Fenstersperre
sidebar_label: Fenstersperre
---

Die Fenstersperre (Window Guard) hält alle Lüfter eines Raums an, solange dort ein Fenster offen ist — offene Fenster und Wärmerückgewinnung vertragen sich nicht. Nach dem Schließen läuft der Raum im vorherigen Modus weiter.

Den Fensterstatus sendet Home Assistant mit der Action [`set_window_open`](../integration/actions.md#set_window_open) an ein beliebiges Gerät des Raums; das Gerät teilt ihn per ESP-NOW mit den anderen.

## Übersicht

| Entität | Entitäts-ID | Typ | Gültigkeit | Kategorie |
| --- | --- | --- | --- | --- |
| Fenstersperre Aktiv | `sensor.…_fenstersperre_aktiv` | Text-Sensor (`Ja` / `Nein`) | Raum | – |
| Fenster offen (HA-Signal) | `binary_sensor.…_fenster_offen_ha_signal` | Binary Sensor (`window`) | Gerät | diagnostic |
| Fenstersperre ignorieren | `switch.…_fenstersperre_ignorieren` | Switch | Gerät | config |

## Verhalten

| Schritt | Verhalten |
| --- | --- |
| Fenster öffnet | Nach **5 s** durchgehend „offen“ greift die Sperre: Alle Geräte des Raums stoppen ihre Lüfter. |
| Während der Sperre | `Lüftermodus` bleibt unverändert. Die Master-LED pulsiert 35 s lang (1 s an, 2 s aus) und geht dann aus — der Lüfter bleibt gestoppt. |
| Fenster schließt | Die Sperre endet, der Raum läuft im vorherigen Modus weiter. |
| Kein Signal mehr | Ein Status gilt **15 min**. Ohne neue Meldung gilt das Fenster als **geschlossen** — die Lüftung bleibt nie dauerhaft stehen. |

Offen ist der Raum, wenn **irgendein** Gerät des Raums einen aktuellen Status „offen“ hat.

## Fenstersperre Aktiv

`sensor.…_fenstersperre_aktiv` zeigt die raumweite Sperre (`Ja` / `Nein`, aktualisiert alle 10 s). Der Wert zeigt die Sperre des Raums — auch auf einem Gerät, das sie mit `Fenstersperre ignorieren` übergeht.

## Fenster offen (HA-Signal)

`binary_sensor.…_fenster_offen_ha_signal` zeigt den Wert, den Home Assistant zuletzt an **dieses** Gerät gesendet hat. Die Entität behält den letzten Wert, auch wenn die Firmware ihn nach 15 min nicht mehr auswertet. Zur Fehlersuche: Steht hier `off`, obwohl das Fenster offen ist, erreicht die Automation das Gerät nicht.

## Fenstersperre ignorieren

`switch.…_fenstersperre_ignorieren` nimmt **dieses eine Gerät** von der Sperre aus — sein Lüfter läuft bei offenem Fenster weiter. Die übrigen Geräte des Raums stoppen weiterhin. Der Schalter wird gespeichert (Standard: aus).

Mit eingeschaltetem Schalter zeigt `Automatik-Status` auf diesem Gerät den regulären Grund statt `Pausiert: Fenster offen`.

## Nutzung

Fensterkontakte des Raums in einer Binärsensor-Gruppe bündeln und den Status per Automation senden — fertiges Beispiel: [Fensterkontakte koppeln](../beispiele/automationen.md#fensterkontakte-koppeln).

:::tip Mehrere Geräte pro Raum
Es genügt, den Status an ein Gerät des Raums zu senden. Wer ihn an **alle** Geräte sendet, hat Redundanz, falls eines gerade neu startet.
:::
