# 🌴 Home Assistant Configuration: Vacation Mode

To enable the new global Vacation Mode for your VentoSync system, you need to create a simple **Toggle Helper** (Input Boolean) in Home Assistant. This single switch will globally activate Vacation Mode across all your VentoSync devices simultaneously.

**Default Entity ID:** `input_boolean.ventosync_vacation_mode`

## How to set it up

### Option A: Via the User Interface (Recommended)
1. Go to **Settings** > **Devices & Services** > **Helpers**.
2. Click **Create Helper** > **Toggle**.
3. **Name**: `VentoSync Vacation Mode` 
4. **Icon**: `mdi:palm-tree` (Optional, but looks nice)
5. **Entity ID**: Once created, Home Assistant should automatically generate the entity ID `input_boolean.ventosync_vacation_mode` if you used exactly that name. You can verify or edit this by clicking on the newly created helper, then selecting the gear icon (Settings) and checking the Entity ID.

### Option B: Via `configuration.yaml`
Add the following code to your Home Assistant configuration:

```yaml
input_boolean:
  ventosync_vacation_mode:
    name: "VentoSync Vacation Mode"
    icon: mdi:palm-tree
```

## How it works
- **Logic:** Switching this toggle to `on` triggers the Vacation Mode globally.
- **VentoSync Reaction:** All your VentoSync fans instantly save their current operating mode and fan intensity, and then safely drop into **Boost Ventilation (Stoßlüftung)** on the lowest intensity **Level 1** for maximum energy saving while keeping a baseline air exchange.
- **Resume:** When you return from your vacation and switch the toggle back to `off`, every unit seamlessly restores its previously active operating mode and speed level.
