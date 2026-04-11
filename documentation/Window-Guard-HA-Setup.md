# 🏠 Home Assistant Configuration: Window Guard

To integrate multiple window sensors into the VentoSync Window Guard for a specific room (e.g., **Room 1**), you need to create a **Binary Sensor Group** in Home Assistant. This group bundles the sensors into a single entity that the firmware monitors.

**Default Entity ID for Room 1:** `binary_sensor.ventosync_window_lock_room_1`

## How to set it up

### Option A: Via the User Interface (Recommended)
1. Go to **Settings** > **Devices & Services** > **Helpers**.
2. Click **Create Helper** > **Group** > **Binary Sensor Group**.
3. **Name**: `VentoSync Window Lock Room 1`
4. **Members**: Add all your window contacts (e.g., `binary_sensor.window_office_contact`).
5. **All Entities Status**: Set to **"Any entity"** (Default – if *any* window is open, the group is `on`).
6. **Entity ID**: Manually change this to `ventosync_window_lock_room_1`.

### Option B: Via `configuration.yaml`
Add the following code to your Home Assistant configuration:

```yaml
binary_sensor:
  - platform: group
    name: "VentoSync Window Lock Room 1"
    unique_id: ventosync_window_lock_room_1
    device_class: window
    entities:
      - binary_sensor.window_office_contact_1
      - binary_sensor.window_office_contact_2
```

## How it works
- **Logic:** As soon as one sensor reports `open` (on), the group switches to `on`.
- **VentoSync Reaction:** All fans in Room 1 receive this status, stop the motor, and pulse the Master LED (1s ON, 2s OFF).
- **Resume:** Once all windows are closed, the group switches to `off`, and ventilation resumes automatically.

> [!TIP]
> You can configure the group with a short delay (e.g., 5 seconds) in Home Assistant to prevent short flickers from triggering the guard.
