# 🔧 Dynamic Device Configuration via Home Assistant

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_dynamic-configuration.md)

## Overview

Instead of hardcoding device addresses in YAML code, all ventilation units can be flashed with the **exact same firmware binary**. Individual configuration (Floor ID, Room ID, Device ID, Phase) is then configured conveniently via **Home Assistant** after connecting to Wi-Fi.

## ✨ Advantages

- ✅ **Single Firmware for All Devices** - No need for individual builds per device
- ✅ **Easy Setup** - Configure parameters directly in the Home Assistant UI
- ✅ **Persistently Stored** - Values are preserved across reboots in NVS flash
- ✅ **Change Anytime** - Adapt room assignments without re-flashing
- ✅ **Instant Effect** - Changes are applied in real time

---

## 📋 Configurable Parameters

### Available in Home Assistant

| Parameter | Description | Range | Default |
| :--- | :--- | :--- | :--- |
| **Floor ID** | Floor / Story number | 0-9 | 1 |
| **Room ID** | Room number | 0-10 | 1 |
| **Device ID** | Unique device identifier | 1-25 | 1 |
| **Device Phase (A/B)** | Initial ventilation cycle phase | A or B | A |
| **Own MAC Address** | Diagnostic MAC display | - | (automatic) |

**Note on ESP-NOW:** Communication uses an automated **hybrid discovery model**. New peers find each other via broadcast, while operational data sync runs resource-efficiently via **unicast**. **No manual MAC configuration is required!**

### Phase A vs. Phase B

- **Phase A:** Starts with **Supply Air** (Intake / IN)
- **Phase B:** Starts with **Exhaust Air** (Extract / OUT)

**Rule:** Units mounted on the **same exterior wall** should share the **same phase**.

---

## 🚀 Setup Workflow

### 1. Flash Firmware (Once)

All devices receive the identical firmware binary:

```bash
esphome run ventosync.yaml
```

### 2. Connect to Home Assistant

1. Connect device to Wi-Fi (via Captive Portal fallback AP or pre-configured secrets)
2. Home Assistant automatically discovers the ESPHome device
3. Click "Configure" to add the integration

### 3. Set Device Parameters

> 💡 **Without Home Assistant:** since 0.10.56 the local web dashboard (`http://<device-ip>/`) opens a **setup wizard** on a device that was never set up — floor / room, device ID with a check against the IDs already used in the room, phase with a suggestion, then the operating mode, the Smart-Automatik basics, Klima-Koordination, vacation preset and child lock (since 0.10.58). See [Local Web Dashboard → Setup Wizard](en_local-web-dashboard.md#-setup-wizard-01056).

In Home Assistant under the device card, configure the location:

#### Example: Living Room, Floor 1, Device 1

```text
Floor ID:       1
Room ID:        2  (e.g., Living Room)
Device ID:      1  (First unit in the room)
Device Phase:   Phase A (Starts with intake)
```

#### Example: Living Room, Floor 1, Device 2 (Paired Unit)

```text
Floor ID:       1
Room ID:        2  (Same room!)
Device ID:      2  (Second unit in the room)
Device Phase:   Phase A (Same wall = same phase!)
```

### 4. Automated ESP-NOW Discovery

**Zero manual configuration required!**

ESP-NOW uses a hybrid broadcast/unicast protocol:

- ✅ **Broadcast Discovery:** Broadcast mode (`FF:FF:FF:FF:FF:FF`) is **only** used to discover new peers on the same floor and in the same room.
- ✅ **Unicast Operation:** Once discovered, all ongoing operational sync (status updates, speed changes) runs targeted via **unicast** with hardware ACKs.
- ✅ **Master Authority (Device ID 1):** The unit with **Device ID 1** automatically acts as group master for cycle timing.
- ✅ **Decentralized Command Distribution:** Changes made on any physical unit or UI are instantly forwarded to all peers.
- ✅ **Group Synchronicity:** All units in a room remain 100% synchronized.

---

## 🏢 Multi-Floor Example Configuration

### Ground Floor (Floor 0)

| Room | Unit | Floor ID | Room ID | Device ID | Phase |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Kitchen | 1 | 0 | 1 | 1 | A |
| Kitchen | 2 | 0 | 1 | 2 | B |
| Living Room | 1 | 0 | 2 | 1 | A |
| Living Room | 2 | 0 | 2 | 2 | B |

### Upper Floor (Floor 1)

| Room | Unit | Floor ID | Room ID | Device ID | Phase |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Bedroom | 1 | 1 | 1 | 1 | A |
| Bedroom | 2 | 1 | 1 | 2 | B |
| Child Room | 1 | 1 | 2 | 1 | A |
| Bathroom | 1 | 1 | 3 | 1 | A |

---

## 🔄 ESP-NOW Pairing Rules

Devices only exchange sync packets with peers that share:

- ✅ **Identical Floor ID**
- ✅ **Identical Room ID**
- ✅ **Different Device ID**

---

## 🛠️ Technical Details

### Flash Persistence

Values are stored in non-volatile flash storage using `restore_value: true`:

```yaml
- platform: template
  name: "Stockwerk (Floor ID)"
  restore_value: true
```

### Real-Time Update

Changes made in Home Assistant take effect immediately on the `VentilationController`:

```yaml
set_action:
  then:
    - lambda: |-
        auto *v = (esphome::VentilationController*)id(ventilation_ctrl);
        v->set_floor_id((uint8_t)x);
```

---

## 🔍 Troubleshooting

| Problem | Solution |
| :--- | :--- |
| Units do not synchronize | Verify Floor ID and Room ID match, and Device IDs are unique |
| Inverted airflow direction | Toggle Phase setting from A to B or B to A |
| Settings reset after reboot | Ensure `restore_value: true` is configured in NVS globals |
