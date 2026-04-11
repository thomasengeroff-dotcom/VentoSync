# 🏠 Home Assistant Konfiguration: Fenstersperre (Window Guard)

Für die Einbindung deiner Fenstersensoren in den VentoSync Window Guard für einen bestimmten Raum (z.B. **Raum 1**) musst du in Home Assistant eine **Binärer Sensor-Gruppe** anlegen. Diese Gruppe bündelt die Sensoren zu einer einzigen Entität, auf die die Firmware (ab v0.8.128) automatisch zugreift.

**Standard-Entity-ID für Raum 1:** `binary_sensor.ventosync_window_lock_room_1`

## So legst du die Gruppe an

### Option A: Über die Benutzeroberfläche (Empfohlen)
1. Gehe zu **Einstellungen** > **Geräte & Dienste** > **Helfer**.
2. Klicke auf **Helfer erstellen** > **Gruppe** > **Binärer Sensor-Gruppe**.
3. **Name**: `VentoSync Window Lock Room 1`
4. **Mitglieder**: Füge alle deine Fensterkontakte hinzu (z.B. `binary_sensor.fenster_dg_wohnraum_contact`).
5. **Status aller Entitäten**: Aktiviere **"Status einer Entität"** (Standard – d.h. wenn *irgendein* Fenster offen ist, ist die Gruppe `on`).
6. **Entitäts-ID**: Ändere diese manuell auf `ventosync_window_lock_room_1`.

### Option B: Über die `configuration.yaml`
Füge folgenden Code in deine Home Assistant Konfiguration ein:

```yaml
binary_sensor:
  - platform: group
    name: "VentoSync Window Lock Room 1"
    unique_id: ventosync_window_lock_room_1
    device_class: window
    entities:
      - binary_sensor.fenster_dg_wohnraum_contact
      - binary_sensor.fenster_dg_flur_contact
      - binary_sensor.fenster_dg_gaube_contact
```

## Wie es funktioniert
- **Logik:** Sobald einer der Sensoren den Status `offen` (on) meldet, schaltet die Gruppe auf `on`.
- **VentoSync Reaktion:** Alle Lüfter in Raum 1 empfangen diesen Status, stoppen den Motor und lassen die Master-LED im "Window Lock Pulse" (1s an, 2s aus) leuchten.
- **Resume:** Sobald alle Fenster geschlossen sind, geht die Gruppe auf `off` und die Lüftung setzt automatisch dort fort, wo sie pausiert wurde.

> [!TIP]
> Du kannst die Gruppe in Home Assistant auch so konfigurieren, dass sie erst mit einer kurzen Verzögerung (z.B. 5 Sekunden) reagiert, um ein kurzes "Auf-und-Zu" beim Lüften abzufedern.
