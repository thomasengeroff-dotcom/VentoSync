# 🏠 Home Assistant Konfiguration: Fenstersperre (Window Guard)

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_window-guard-ha-setup.md)


Die **Fenstersperre (Window Guard)** pausiert automatisch alle Lüftungsgeräte in einem Raum, sobald ein Fenster geöffnet wird, um Energieverschwendung und Wärmeverluste zu vermeiden.

---

## ✨ Leistungsmerkmale & Verhalten

- ⏱️ **Smart Pause (5s Verzögerung)**: Die Sperre greift erst nach 5 Sekunden durchgehender Fenster-Öffnung, um kurzes Lüften oder Nachschauen abzufedern. Alle VentoSync-Geräte im Raum stoppen sofort ihre Lüfter.
- 🔄 **Automatisches Fortsetzen**: Das System behält seinen aktuellen Betriebsmodus (z. B. Automatik oder Manuell) bei und nimmt den Betrieb nahtlos wieder auf, sobald alle Fenster geschlossen sind.
- 🔆 **Visuelles Feedback (35s Limit)**: Ein markantes Pulsieren der Master-LED (1s An, 2s Aus) signalisiert den Zustand "Pause durch Fenster". Zur Vermeidung von Lichtstörungen nachts stoppt das Pulsieren nach 35 Sekunden, während der Lüfter weiterhin sicher gestoppt bleibt.
- 📊 **HA Status-Entität**: Ein dedizierter Textsensor (`text_sensor.fenstersperre_aktiv`, `Ja` / `Nein`) bietet direkte Sichtbarkeit des Sperrstatus in Home Assistant.
- 🎛️ **Individueller Bypass-Schalter**: Über den Schalter **"Fenstersperre ignorieren"** (`switch.ignore_window_guard` / `switch.fenstersperre_ignorieren`) können einzelne Geräte bei Bedarf von der Raumsperre ausgenommen werden.

---

## 🛠️ Einrichtung in Home Assistant

Seit **0.10.22** **sendet** Home Assistant den Fensterstatus mit der API-Action **`set_window_open`** (Daten `window_open: true/false`) an die Lüftungsgeräte. Der Status wird per ESP-NOW (Protokoll v10) mit allen Geräten des Raums geteilt; Home Assistant muss daher **mindestens ein** Gerät des Raums erreichen — unabhängig von der am Gerät eingestellten Raum-ID. Ein gesendeter Status gilt **15 Minuten**; ein abgelaufener oder nie gesendeter Status gilt als **geschlossen** (die Lüftung kann nie dauerhaft stehen bleiben), deshalb sendet die Automation ihn alle 5 Minuten erneut.

### Schritt 1: Fensterkontakte gruppieren (optional)

Bei mehreren Fenstern die Kontakte in einem **Binary-Sensor-Gruppen**-Helfer bündeln (**Einstellungen** > **Geräte & Dienste** > **Helfer** > **Helfer erstellen** > **Gruppe** > **Binärsensor-Gruppe**, Mitglieder: alle Fensterkontakte des Raums, „beliebige Entität" = an). Eine vorhandene Gruppe wie `binary_sensor.ventosync_window_lock_room_1` kann unverändert weiterverwendet werden.

### Schritt 2: Automation

`<gerätename>` ist der ESPHome-Node-Name des Geräts (Bindestriche werden zu Unterstrichen, z. B. `ventosync-dg-buero` → `esphome.ventosync_dg_buero_set_window_open`).

```yaml
automation:
  - alias: "VentoSync: Fensterstatus Raum 1"
    mode: queued
    triggers:
      - trigger: state
        entity_id: binary_sensor.ventosync_window_lock_room_1
      - trigger: homeassistant
        event: start
      - trigger: time_pattern   # erneut senden: der Status läuft im Gerät nach 15 min ab
        minutes: "/5"
    variables:
      window_open: "{{ is_state('binary_sensor.ventosync_window_lock_room_1', 'on') }}"
    actions:
      # Mindestens ein Gerät des Raums; für Redundanz alle Geräte auflisten.
      - action: esphome.ventosync_raum1_a_set_window_open
        data:
          window_open: "{{ window_open }}"
        continue_on_error: true
      - action: esphome.ventosync_raum1_b_set_window_open
        data:
          window_open: "{{ window_open }}"
        continue_on_error: true
```

Der zuletzt gesendete Wert ist auf jedem Gerät als `binary_sensor.fenster_offen_ha_signal` („Fenster offen (HA-Signal)", Diagnose) sichtbar; die daraus resultierende raumweite Sperre als Textsensor `text_sensor.fenstersperre_aktiv` („Fenstersperre Aktiv": `Ja` / `Nein`).

> [!IMPORTANT]
> **Umstieg von ≤ 0.10.21:** Die Firmware importiert `binary_sensor.ventosync_window_lock_room_<room_id>` nicht mehr (Substitution `window_sensor_id` entfernt) — diese Entität wurde aus der Compile-Zeit-`room_id` (Standard `1`) gebildet, sodass zur Laufzeit einem anderen Raum zugeordnete Geräte auf Raum 1 hörten. Den Gruppen-Helfer behalten und die obige Automation anlegen. Alle Geräte eines Raums gemeinsam flashen (ESP-NOW-Protokoll v10).
