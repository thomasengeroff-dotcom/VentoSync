# 🏠 Home Assistant Configuration: Window Guard

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_window-guard-ha-setup.md)


The **Window Guard** feature automatically pauses all ventilation units in a room when windows are opened to prevent heat loss and energy waste.

---

## ✨ Features & Behavior

- ⏱️ **Smart Pause (5s Delay)**: The guard engages after 5 seconds of continuous "open" state to prevent accidental triggers when briefly checking a window. All VentoSync units in the room immediately stop their fans to prevent energy waste.
- 🔄 **Automatic Resume**: The system preserves its current operating mode (e.g., Automatic, Heat Recovery, etc.) and resumes operation seamlessly as soon as all windows are closed.
- 🔆 **Visual Feedback (35s Limit)**: A distinct pulsing pattern on the Master LED (1s ON, 2s OFF) indicates the "Paused by Window" state. To avoid light pollution at night, the pulsing starts after 5 seconds and stops after 35 seconds while the fan remains safely stopped.
- 📊 **HA Status Entity**: A dedicated text sensor (`text_sensor.fenstersperre_aktiv`, `Ja` / `Nein`) provides real-time visibility of the lock status in Home Assistant.
- 🎛️ **Per-Device Bypass Switch**: Includes an **"Ignore Window Guard" switch** (`switch.fenstersperre_ignorieren` / `switch.fenstersperre_ignorieren`) to bypass the lock for specific individual units if needed.

---

## 🛠️ Home Assistant Setup

Since **0.10.22** Home Assistant **pushes** the window state to the ventilation units with the API action **`set_window_open`** (data `window_open: true/false`). The state is shared with all devices of the room over ESP-NOW (protocol v11), so HA has to reach **at least one** device of the room — independent of the room ID configured on the device. A pushed state is trusted for **15 minutes**; an expired or never pushed state reads as **closed** (the ventilation can never stay stopped forever), so the automation re-sends it every 5 minutes.

### Step 1: Group the window contacts (optional)

With several windows, bundle them in a **Binary Sensor Group** helper (**Settings** > **Devices & Services** > **Helpers** > **Create Helper** > **Group** > **Binary Sensor Group**, members: all window contacts of the room, "any entity" = on). An existing group such as `binary_sensor.ventosync_window_lock_room_1` can be reused as is.

### Step 2: Automation

`<device_name>` is the ESPHome node name of the unit (hyphens become underscores, e.g. `ventosync-dg-buero` → `esphome.ventosync_dg_buero_set_window_open`).

```yaml
automation:
  - alias: "VentoSync: window state room 1"
    mode: queued
    triggers:
      - trigger: state
        entity_id: binary_sensor.ventosync_window_lock_room_1
      - trigger: homeassistant
        event: start
      - trigger: time_pattern   # re-send: the device state expires after 15 min
        minutes: "/5"
    variables:
      window_open: "{{ is_state('binary_sensor.ventosync_window_lock_room_1', 'on') }}"
    actions:
      # At least one device of the room; list every device for redundancy.
      - action: esphome.ventosync_room1_a_set_window_open
        data:
          window_open: "{{ window_open }}"
        continue_on_error: true
      - action: esphome.ventosync_room1_b_set_window_open
        data:
          window_open: "{{ window_open }}"
        continue_on_error: true
```

The last pushed value is visible on each device as `binary_sensor.fenster_offen_ha_signal` ("Fenster offen (HA-Signal)", diagnostic); the resulting room-wide lock as the text sensor `text_sensor.fenstersperre_aktiv` ("Fenstersperre Aktiv": `Ja` / `Nein`).

> [!IMPORTANT]
> **Migration from ≤ 0.10.21:** the firmware no longer imports `binary_sensor.ventosync_window_lock_room_<room_id>` (substitution `window_sensor_id` removed) — that entity was derived from the compile-time `room_id` (default `1`), so devices assigned to another room at runtime listened to room 1. Keep your group helper and add the automation above. Flash all devices of a room together (ESP-NOW protocol v10).
