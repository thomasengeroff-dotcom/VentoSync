# 📶 Bluetooth Proxy (experimental)

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_bluetooth-proxy.md)

VentoSync can act as a **Home Assistant Bluetooth proxy**
([ESPHome `bluetooth_proxy`](https://esphome.io/components/bluetooth_proxy/)). The ventilation unit then
receives Bluetooth Low Energy (BLE) advertisements in its room — thermometers, plant sensors, presence tags,
smart locks — and forwards them to Home Assistant over Wi-Fi. A ventilation unit is mounted in an outer wall in
almost every room, which makes it a natural place for proxy coverage.

> ⚠️ **Status: experimental, for testing.** The feature only exists in the dedicated variant
> `ventosync_nosensor_btproxy.yaml` (PCB v1.0, no sensors). It needs its **own partition table** and therefore
> a **one-time USB flash**. Read [Firmware size](#-the-firmware-size-problem) before using it.

---

## 📑 Contents

- [How it works](#-how-it-works)
- [Switching it on and off](#-switching-it-on-and-off)
- [The firmware size problem](#-the-firmware-size-problem)
- [Installation](#-installation)
- [Radio coexistence with Wi-Fi and ESP-NOW](#-radio-coexistence-with-wi-fi-and-esp-now)
- [Limitations & FAQ](#-limitations--faq)

---

## 🔍 How it works

| Component | Configuration | Purpose |
|---|---|---|
| `esp32_ble` | `enable_on_boot: false` | Bluetooth stack (Bluedroid + BT controller), **off after boot** |
| `esp32_ble_tracker` | active scan | Receives BLE advertisements |
| `bluetooth_proxy` | `active: true`, 3 connection slots | Forwards advertisements to HA and allows active GATT connections (e.g. locks) |
| `switch` "Bluetooth Proxy" | `restore_mode: RESTORE_DEFAULT_OFF` | Turns the stack on (`ble.enable`) and off (`ble.disable`) |

Everything lives in one package: [`packages/integration/bluetooth_proxy.yaml`](../../packages/integration/bluetooth_proxy.yaml).
Home Assistant discovers the proxy automatically through its Bluetooth integration once the device is adopted
via the ESPHome integration — there is no extra configuration on the HA side.

The proxy is independent of the ventilation logic: no ESP-NOW packet, no room-wide setting and no control loop
changes when it is enabled.

---

## 🔘 Switching it on and off

The proxy is compiled in but **disabled by default**. It is turned on per device:

- **Home Assistant:** configuration entity **`switch.<device>_bluetooth_proxy`** ("Bluetooth Proxy").
- **Local web dashboard** (`http://<device-ip>/ui`): toggle **"Bluetooth Proxy (experimentell)"** in the
  *Einstellungen → Steuerung* group. The toggle only appears in this variant.

Behaviour:

- `ble.disable` tears down Bluedroid and the BT controller completely (`esp_bluedroid_deinit()`,
  `esp_bt_controller_deinit()`), so their **heap is returned**. A disabled proxy costs flash, but no RAM and
  no airtime.
- The switch state is stored in NVS and restored after a reboot; the restore runs the matching action, so an
  enabled proxy starts again on its own.
- The setting is **per device** and deliberately **not** synchronised room-wide: one proxy per room, on the unit
  with the best position, is usually enough.

---

## 📦 The firmware size problem

### Why the proxy does not fit into the standard firmware

Both boards use 4 MB flash modules (Seeed XIAO ESP32-C6 on PCB v1.0, ESP32-C6-MINI-1U-**H4** on PCB v2.0; the
8 MB version of the U.FL module, ESP32-C6-MINI-1U-H8, is not available — see the
[8 MB outlook](#-outlook-8-mb-module-esp32-c6-mini-1-h8)). ESPHome splits 4 MB into two OTA app partitions of **1,835,008 bytes
(1.75 MB)** each plus a 448 KB NVS partition. Two app partitions are required for OTA: the new image is written
to the inactive one while the old one keeps running.

The Bluetooth stack alone adds about **610 KB**:

| Library | Size | Content |
|---|---:|---|
| `libbt.a` | 291 KB | Bluedroid host stack (GAP, GATT client/server, SMP, L2CAP) |
| `libble_app.a` | 236 KB | BT controller (pre-built Espressif binary, cannot be trimmed) |
| `libesp_timer`, `libsrc`, `libcoexist`, … | ~85 KB | ESPHome BLE components, coexistence arbiter, timers |

ESPHome only supports Bluedroid; the smaller NimBLE stack is not an option.

### Measurements (ESPHome 2026.9.0, real builds)

| Build | Image size | 1.75 MB default partition |
|---|---:|---|
| `ventosync.yaml` (full), no BLE | 1,509,678 B | 82 % |
| `ventosync_nosensor.yaml`, no BLE | 1,483,940 B | 81 % |
| full + Bluetooth proxy | 2,121,936 B | ❌ 286 KB too large |
| nosensor + Bluetooth proxy | 2,095,520 B | ❌ 254 KB too large |
| nosensor + proxy + size optimisations (below) | 1,992,832 B | ❌ 154 KB too large |
| full + proxy + optimisations, without HTTPS updater and ESPHome web UI | 1,828,472 B | 99.6 % — not maintainable |

The sensor drivers are small (~26 KB); most of the image is the platform itself (Wi-Fi, native API, TLS,
ESP-NOW, web dashboard). Leaving out sensors therefore does not solve the problem.

### The chosen solution: a dedicated partition table

`ventosync_nosensor_btproxy.yaml` uses
[`packages/board/partitions_4mb_btproxy.csv`](../../packages/board/partitions_4mb_btproxy.csv):

| Partition | ESPHome default | Bluetooth proxy variant |
|---|---:|---:|
| `otadata` | 8 KB | 8 KB |
| `phy_init` | 4 KB | 4 KB |
| `app0` / `app1` | 2 × 1,792 KB | **2 × 1,984 KB** (0x1F0000) |
| `nvs` | 448 KB | **64 KB** |

64 KB NVS (16 pages) is plenty for VentoSync's preferences (Wi-Fi, configuration, restored entities, filter
hours). In addition the package applies three loss-free size optimisations:

- `CONFIG_BT_LE_50_FEATURE_SUPPORT: n` — ESPHome only uses the legacy (BLE 4.2) scan/connect API, the BLE 5.0
  extended advertising code is dead weight.
- `assertion_level: SILENT` — assertions still abort, but without file/line strings.
- `logger: level: INFO` — DEBUG log strings are not compiled in.

Result (complete variant incl. switch and dashboard toggle): **1,994,218 B of 2,031,616 B → ~37 KB headroom (98.2 %).**

### Consequences

- ⚠️ **Little headroom.** A new ESPHome release or an extra package can exceed the partition. CI therefore
  compiles this variant on **every pull request** — the build fails with `All app partitions are too small`
  as soon as it no longer fits.
- ⚠️ **Only with the nosensor variant.** Sensor packages (SCD43, BME680, radar) are not planned for this
  variant; ESP-NOW still delivers the room's sensor values from the other units.
- ⚠️ **No OTA switch to this variant.** The partition table is only written by a serial/USB flash. An OTA
  upload of this image to a device with the default layout is rejected (1.99 MB image, 1.75 MB slot).
  Within the variant, OTA (ESPHome and GitHub release updates) works as usual.
- ⚠️ **NVS is lost when switching.** The NVS partition moves, so Wi-Fi credentials and the runtime
  configuration (floor / room / device ID, phase, settings, filter hours) start from scratch.


### 🔭 Outlook: 8 MB module (ESP32-C6-MINI-1-H8)

All of the above is a consequence of the **current 4 MB modules**. Besides the unavailable U.FL version
(MINI-1U-H8) there is the **ESP32-C6-MINI-1-H8** with **8 MB flash** (on-module PCB antenna instead of the U.FL
connector). With 8 MB the problem disappears:

| | 4 MB (today) | 8 MB (MINI-1-H8) |
|---|---:|---:|
| App partition (ESPHome default layout) | 1,835,008 B (1.75 MB) | **3,932,160 B (3.75 MB)** |
| NVS | 448 KB | 448 KB |
| Full variant + Bluetooth proxy, **without** size optimisations (2,121,936 B) | ❌ does not fit | ✅ ~54 % |

With such a board the proxy could be offered **in all variants** — compiled in, off by default and switched in
Home Assistant / the web dashboard, as described above — with the standard partition layout, without the size
optimisations and without the nosensor restriction.

This is **not possible with the current ESP modules** (XIAO ESP32-C6 and ESP32-C6-MINI-1U-H4). An 8 MB board would
need:

- a **PCB revision** — the MINI-1 is longer than the MINI-1U (on-module antenna) and needs an antenna keep-out
  area; an external U.FL antenna can no longer be used, so reception at the mounting position must be checked,
- its own board package with `esp32: flash_size: 8MB` and its own `firmware_variant` values, so that 4 MB devices
  are never offered 8 MB firmware.

Not implemented yet — this is a hardware decision for a future PCB revision.

---

## 🛠️ Installation

1. **Note the current configuration** of the device (floor, room, device ID, phase, settings) — it is lost.
2. **Flash over USB** (USB-C on the XIAO):

   ```bash
   esphome run ventosync_nosensor_btproxy.yaml --device /dev/ttyACM0
   ```

   or flash `ventosync-nosensor-btproxy.factory.bin` from the GitHub release with a web flasher
   (the factory image contains bootloader, partition table and application).
3. **Provision Wi-Fi** (Improv serial or the fallback hotspot) and set floor / room / device ID again
   (see [Dynamic Configuration](en_dynamic-configuration.md)).
4. **Adopt the device in Home Assistant** and turn on **"Bluetooth Proxy"**. The proxy appears under
   *Settings → Devices & services → Bluetooth*.
5. **Watch** `Freier Speicher (RAM)` and the ESP-NOW peers in the dashboard for a while.

**Going back** to a standard variant works over OTA (`esphome run ventosync_nosensor.yaml --device <IP>`): the
smaller standard image fits into the larger app partition and the configuration is kept. The device then keeps
the Bluetooth proxy partition layout (64 KB NVS), which is harmless. Only a USB flash restores ESPHome's default
layout (and erases the NVS again).

---

## 📡 Radio coexistence with Wi-Fi and ESP-NOW

The ESP32-C6 has **one** 2.4 GHz radio for Wi-Fi, ESP-NOW and Bluetooth. ESP-IDF's coexistence arbiter shares it
by time-slicing. With Wi-Fi active, ESPHome 2026.9 sets the BLE scan window equal to the scan interval
(continuous scanning) — the arbiter still gives Wi-Fi priority, BLE uses the remaining airtime.

What this means for VentoSync:

- **ESP-NOW packets can be delayed or lost** while the radio listens for BLE. The room sync tolerates this:
  heartbeat every 60 s, room-wide fusion window ≥ 5 min, peer timeout 15 min.
  Mode changes are sent immediately and re-asserted by the Master heartbeat.
- **Recommendation:** enable the proxy on **one** unit per room and keep an eye on the ESP-NOW peer list in the
  dashboard. If peers drop out, a shorter scan window can be set in the variant YAML, e.g.:

  ```yaml
  esp32_ble_tracker:
    scan_parameters:
      interval: 320ms
      window: 120ms
  ```

  A shorter window misses more advertisements; ESPHome warns above 600 ms.
- Active connections (`active: true`) occupy the radio for longer; they are only established when Home
  Assistant requests them.

---

## ❓ Limitations & FAQ

**Why not just include it in every variant and keep it disabled?**
Disabling only frees RAM and airtime — the ~610 KB of code are always in the image. On 4 MB modules that does
not fit next to the standard partition layout, and changing the layout of every installed device would require
a USB flash of all units. With an 8 MB module it would be possible — see the
[8 MB outlook](#-outlook-8-mb-module-esp32-c6-mini-1-h8).

**Is there a variant for PCB v2.0 or with sensors?**
Not yet. The package is board-independent (`bluetooth_proxy: !include
packages/integration/bluetooth_proxy.yaml`), but every sensor package reduces the ~37 KB headroom. Test a build
before relying on it.

**How much RAM does it use?**
About +33 KB static RAM (always), plus the Bluedroid heap while the proxy is on. `ble.disable` returns that heap.

**Alternative without any trade-offs?**
A separate, inexpensive ESP32 board running the stock ESPHome Bluetooth proxy firmware. VentoSync then keeps
its full flash headroom.

---

*Related: [Home Assistant Entities](en_home-assistant-entities.md) · [Local Web Dashboard](en_local-web-dashboard.md) ·
[ESP-NOW Communication](en_esp-now-communication.md) · [Dynamic Configuration](en_dynamic-configuration.md)*
