# 🌴 Home Assistant Konfiguration: Urlaubsmodus (Vacation Mode)

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_vacation-mode-ha-setup.md)


Um den neuen globalen Urlaubsmodus für dein VentoSync-System zu aktivieren, musst du in Home Assistant lediglich einen einfachen **Schalter-Helfer** (Input Boolean) anlegen. Dieser eine Schalter steuert den Urlaubsmodus global für all deine VentoSync-Geräte gleichzeitig.

**Standard-Entity-ID:** `input_boolean.ventosync_vacation_mode`

## So legst du den Helfer an

### Option A: Über die Benutzeroberfläche (Empfohlen)
1. Gehe zu **Einstellungen** > **Geräte & Dienste** > **Helfer**.
2. Klicke auf **Helfer erstellen** > **Schalter (Toggle)**.
3. **Name**: `VentoSync Vacation Mode` (oder "VentoSync Urlaubsmodus")
4. **Symbol**: `mdi:palm-tree` (Optional, aber passend)
5. **Entitäts-ID**: Nach dem Erstellen generiert Home Assistant die Entitäts-ID basierend auf dem Namen. Klicke auf das Zahnrad des neuen Helfers, um sicherzustellen, dass die ID exakt `input_boolean.ventosync_vacation_mode` lautet. Passe sie, wenn nötig, manuell an.

### Option B: Über die `configuration.yaml`
Füge folgenden Code in deine Home Assistant Konfiguration ein (falls du die YAML-Methode bevorzugst):

```yaml
input_boolean:
  ventosync_vacation_mode:
    name: "VentoSync Urlaubsmodus"
    icon: mdi:palm-tree
```

## Wie es funktioniert
- **Logik:** Wird dieser Schalter auf `an` (on) gesetzt, geht das System global in den Urlaubsmodus über.
- **VentoSync Reaktion:** **Jedes** Gerät sichert beim Umschalten seinen momentanen Modus und seine Lüfterstufe, angewendet wird der Urlaubsmodus aber nur vom **Master** (Geräte-ID 1) des Raums — standardmäßig die stromsparende **Stoßlüftung** auf der niedrigsten **Stufe 1**, um den minimalen Luftaustausch bei Abwesenheit sicherzustellen. Die übrigen Geräte folgen dem Master per ESP-NOW. Ein Gerät schaltet nur selbst, wenn kein Master seines Raums erreichbar ist — dafür ist die eigene Sicherung da.
- **Resume:** Kommst du aus dem Urlaub zurück und stellst den Schalter auf `aus` (off), stellt der Master den vorherigen Modus samt Intensität für den ganzen Raum wieder her.
- **Geteilte Sicherung:** Der Master überträgt seine Sicherung laufend per ESP-NOW an die übrigen Geräte des Raums (Protokoll v11). Nötig ist das, weil das MSG_STATE des Masters ein Gerät bereits in den Urlaubsmodus schalten kann, **bevor** dessen eigene Meldung aus Home Assistant eintrifft — beide treffen im Millisekundenabstand ein. Ohne die geteilte Sicherung würde ein solches Gerät den Urlaubszustand selbst sichern und ihn am Urlaubsende „wiederherstellen“. Fällt der Master während des Urlaubs aus, stellt das einspringende Gerät dadurch den tatsächlichen Zustand von vor dem Urlaub wieder her.
- **Robust:** Der Urlaubszustand wird im Gerät gespeichert; ein Neustart oder HA-Neustart während des Urlaubs überschreibt den gesicherten Zustand nicht. Ein zwischenzeitlich nicht erreichbarer Helper (z. B. während eines HA-Neustarts) beendet den Urlaub nicht — nur ein echtes `off` tut das. Die Urlaubs-Einstellungen (`select.urlaubsmodus_betriebsmodus`, `number.urlaubsmodus_intensitat`) am Master einstellen; sie werden **nicht** raumweit synchronisiert.
