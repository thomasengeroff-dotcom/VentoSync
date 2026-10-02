# 📶 Bluetooth-Proxy (experimentell)

[![Language: EN](https://img.shields.io/badge/Language-EN-blue.svg)](../en/en_bluetooth-proxy.md)

VentoSync kann als **Bluetooth-Proxy für Home Assistant** arbeiten
([ESPHome `bluetooth_proxy`](https://esphome.io/components/bluetooth_proxy/)). Das Lüftungsgerät empfängt dann
Bluetooth-Low-Energy-Advertisements (BLE) im Raum – Thermometer, Pflanzensensoren, Präsenz-Tags, smarte Schlösser –
und leitet sie per WLAN an Home Assistant weiter. Da in fast jedem Raum ein Lüftungsgerät in der Außenwand sitzt,
ist es ein naheliegender Standort für die Proxy-Abdeckung.

> ⚠️ **Status: experimentell, für Testzwecke.** Die Funktion gibt es nur in der eigenen Variante
> `ventosync_nosensor_btproxy.yaml` (PCB v1.0, ohne Sensoren). Sie braucht eine **eigene Partitionstabelle** und
> damit einen **einmaligen USB-Flash**. Bitte vorher [Firmwaregröße](#-das-problem-der-firmwaregröße) lesen.

---

## 📑 Inhalt

- [Funktionsweise](#-funktionsweise)
- [Ein- und Ausschalten](#-ein--und-ausschalten)
- [Das Problem der Firmwaregröße](#-das-problem-der-firmwaregröße)
- [Installation](#️-installation)
- [Funk-Koexistenz mit WLAN und ESP-NOW](#-funk-koexistenz-mit-wlan-und-esp-now)
- [Einschränkungen & FAQ](#-einschränkungen--faq)

---

## 🔍 Funktionsweise

| Komponente | Konfiguration | Zweck |
|---|---|---|
| `esp32_ble` | `enable_on_boot: false` | Bluetooth-Stack (Bluedroid + BT-Controller), **nach dem Booten aus** |
| `esp32_ble_tracker` | aktiver Scan | Empfängt BLE-Advertisements |
| `bluetooth_proxy` | `active: true`, 3 Verbindungs-Slots | Leitet Advertisements an HA weiter und erlaubt aktive GATT-Verbindungen (z. B. Schlösser) |
| `switch` „Bluetooth Proxy“ | `restore_mode: RESTORE_DEFAULT_OFF` | Schaltet den Stack ein (`ble.enable`) und aus (`ble.disable`) |

Alles steckt in einem Paket: [`packages/integration/bluetooth_proxy.yaml`](../../packages/integration/bluetooth_proxy.yaml).
Home Assistant erkennt den Proxy automatisch über die Bluetooth-Integration, sobald das Gerät über die
ESPHome-Integration eingebunden ist – auf HA-Seite ist keine weitere Konfiguration nötig.

Der Proxy ist unabhängig von der Lüftungslogik: Kein ESP-NOW-Paket, keine raumweite Einstellung und kein Regelkreis
ändert sich, wenn er eingeschaltet ist.

---

## 🔘 Ein- und Ausschalten

Der Proxy ist einkompiliert, aber **standardmäßig deaktiviert**. Er wird pro Gerät eingeschaltet:

- **Home Assistant:** Konfigurations-Entität **`switch.<gerät>_bluetooth_proxy`** („Bluetooth Proxy“).
- **Lokales Web-Dashboard** (`http://<geräte-ip>/ui`): Schalter **„Bluetooth Proxy (experimentell)“** in der
  Gruppe *Einstellungen → Steuerung*. Der Schalter erscheint nur in dieser Variante.

Verhalten:

- `ble.disable` baut Bluedroid und den BT-Controller vollständig ab (`esp_bluedroid_deinit()`,
  `esp_bt_controller_deinit()`), der **Heap wird freigegeben**. Ein ausgeschalteter Proxy kostet Flash, aber
  keinen RAM und keine Funkzeit.
- Der Schaltzustand wird im NVS gespeichert und nach einem Neustart wiederhergestellt; dabei läuft die passende
  Aktion, ein eingeschalteter Proxy startet also von selbst wieder.
- Die Einstellung gilt **pro Gerät** und wird bewusst **nicht** raumweit synchronisiert: Ein Proxy pro Raum, auf dem
  Gerät mit dem besten Standort, reicht in der Regel.

---

## 📦 Das Problem der Firmwaregröße

### Warum der Proxy nicht in die Standard-Firmware passt

Beide Platinen nutzen Module mit 4 MB Flash (Seeed XIAO ESP32-C6 auf PCB v1.0, ESP32-C6-MINI-1U-**H4** auf
PCB v2.0; die 8-MB-Version des U.FL-Moduls, ESP32-C6-MINI-1U-H8, ist nicht lieferbar – siehe
[Ausblick 8 MB](#-ausblick-8-mb-modul-esp32-c6-mini-1-h8)). ESPHome teilt 4 MB in zwei OTA-App-Partitionen zu je
**1.835.008 Bytes (1,75 MB)** plus eine NVS-Partition mit 448 KB. Zwei App-Partitionen sind für OTA nötig: Das
neue Image wird in die inaktive geschrieben, während die alte weiterläuft.

Allein der Bluetooth-Stack bringt rund **610 KB** mit:

| Bibliothek | Größe | Inhalt |
|---|---:|---|
| `libbt.a` | 291 KB | Bluedroid-Host-Stack (GAP, GATT-Client/-Server, SMP, L2CAP) |
| `libble_app.a` | 236 KB | BT-Controller (vorkompiliertes Espressif-Binary, nicht verkleinerbar) |
| `libesp_timer`, `libsrc`, `libcoexist`, … | ~85 KB | ESPHome-BLE-Komponenten, Koexistenz-Arbiter, Timer |

ESPHome unterstützt nur Bluedroid; der kleinere NimBLE-Stack ist keine Option.

### Messwerte (ESPHome 2026.9.0, echte Builds)

| Build | Image-Größe | Standard-Partition 1,75 MB |
|---|---:|---|
| `ventosync.yaml` (Full), ohne BLE | 1.509.678 B | 82 % |
| `ventosync_nosensor.yaml`, ohne BLE | 1.483.940 B | 81 % |
| Full + Bluetooth-Proxy | 2.121.936 B | ❌ 286 KB zu groß |
| nosensor + Bluetooth-Proxy | 2.095.520 B | ❌ 254 KB zu groß |
| nosensor + Proxy + Größenoptimierungen (siehe unten) | 1.992.832 B | ❌ 154 KB zu groß |
| Full + Proxy + Optimierungen, ohne HTTPS-Updater und ESPHome-Web-UI | 1.828.472 B | 99,6 % – nicht wartbar |

Die Sensortreiber sind klein (~26 KB); den Großteil des Images macht die Plattform selbst aus (WLAN, Native API,
TLS, ESP-NOW, Web-Dashboard). Sensoren wegzulassen löst das Problem daher nicht.

### Gewählte Lösung: eigene Partitionstabelle

`ventosync_nosensor_btproxy.yaml` verwendet
[`packages/board/partitions_4mb_btproxy.csv`](../../packages/board/partitions_4mb_btproxy.csv):

| Partition | ESPHome-Standard | Bluetooth-Proxy-Variante |
|---|---:|---:|
| `otadata` | 8 KB | 8 KB |
| `phy_init` | 4 KB | 4 KB |
| `app0` / `app1` | 2 × 1.792 KB | **2 × 1.984 KB** (0x1F0000) |
| `nvs` | 448 KB | **64 KB** |

64 KB NVS (16 Seiten) reichen für die VentoSync-Einstellungen (WLAN, Konfiguration, wiederhergestellte Entitäten,
Filterstunden) problemlos. Zusätzlich setzt das Paket drei verlustfreie Größenoptimierungen:

- `CONFIG_BT_LE_50_FEATURE_SUPPORT: n` – ESPHome nutzt nur die Legacy-API (BLE 4.2) für Scan und Verbindungen,
  der Code für BLE-5.0-Extended-Advertising ist Ballast.
- `assertion_level: SILENT` – Assertions brechen weiterhin ab, aber ohne Datei-/Zeilen-Strings.
- `logger: level: INFO` – DEBUG-Logstrings werden nicht einkompiliert.

Ergebnis (komplette Variante inkl. Schalter und Dashboard-Toggle): **1.994.218 B von 2.031.616 B → ca. 37 KB Reserve (98,2 %).**

### Konsequenzen

- ⚠️ **Wenig Reserve.** Ein neues ESPHome-Release oder ein zusätzliches Paket kann die Partition sprengen. Die CI
  baut diese Variante deshalb bei **jedem Pull Request** – der Build schlägt mit `All app partitions are too small`
  fehl, sobald sie nicht mehr passt.
- ⚠️ **Nur mit der nosensor-Variante.** Sensorpakete (SCD43, BME680, Radar) sind für diese Variante nicht
  vorgesehen; die Sensorwerte des Raums liefern weiterhin die anderen Geräte per ESP-NOW.
- ⚠️ **Kein OTA-Wechsel zu dieser Variante.** Die Partitionstabelle wird nur per Seriell-/USB-Flash geschrieben.
  Ein OTA-Upload dieses Images auf ein Gerät mit Standard-Layout wird abgelehnt (1,99-MB-Image, 1,75-MB-Slot).
  Innerhalb der Variante funktioniert OTA (ESPHome und GitHub-Release-Updates) wie gewohnt.
- ⚠️ **Beim Wechsel geht das NVS verloren.** Die NVS-Partition verschiebt sich, WLAN-Zugangsdaten und die
  Laufzeitkonfiguration (Etage / Raum / Geräte-ID, Phase, Einstellungen, Filterstunden) beginnen von vorn.


### 🔭 Ausblick: 8-MB-Modul (ESP32-C6-MINI-1-H8)

Alles oben Beschriebene folgt aus den **aktuellen 4-MB-Modulen**. Neben der nicht lieferbaren U.FL-Version
(MINI-1U-H8) gibt es das **ESP32-C6-MINI-1-H8** mit **8 MB Flash** (PCB-Antenne auf dem Modul statt U.FL-Buchse).
Mit 8 MB entfällt das Problem:

| | 4 MB (heute) | 8 MB (MINI-1-H8) |
|---|---:|---:|
| App-Partition (ESPHome-Standardlayout) | 1.835.008 B (1,75 MB) | **3.932.160 B (3,75 MB)** |
| NVS | 448 KB | 448 KB |
| Full-Variante + Bluetooth-Proxy, **ohne** Größenoptimierungen (2.121.936 B) | ❌ passt nicht | ✅ ca. 54 % |

Mit einer solchen Platine ließe sich der Proxy **in allen Varianten** anbieten – einkompiliert, standardmäßig aus
und wie oben beschrieben in Home Assistant bzw. im Web-Dashboard schaltbar –, mit dem Standard-Partitionslayout,
ohne Größenoptimierungen und ohne Beschränkung auf nosensor.

Mit den **aktuellen ESP-Modulen** (XIAO ESP32-C6 und ESP32-C6-MINI-1U-H4) ist das **nicht möglich**. Eine 8-MB-Platine
bräuchte:

- eine **PCB-Revision** – das MINI-1 ist länger als das MINI-1U (Antenne auf dem Modul) und braucht eine
  Antennen-Sperrfläche; eine externe U.FL-Antenne ist nicht mehr möglich, der Empfang am Einbauort muss also
  geprüft werden,
- ein eigenes Boardpaket mit `esp32: flash_size: 8MB` und eigene `firmware_variant`-Werte, damit 4-MB-Geräten nie
  8-MB-Firmware angeboten wird.

Noch nicht umgesetzt – das ist eine Hardware-Entscheidung für eine künftige PCB-Revision.

---

## 🛠️ Installation

1. **Aktuelle Konfiguration notieren** (Etage, Raum, Geräte-ID, Phase, Einstellungen) – sie geht verloren.
2. **Per USB flashen** (USB-C am XIAO):

   ```bash
   esphome run ventosync_nosensor_btproxy.yaml --device /dev/ttyACM0
   ```

   oder `ventosync-nosensor-btproxy.factory.bin` aus dem GitHub-Release mit einem Web-Flasher aufspielen
   (das Factory-Image enthält Bootloader, Partitionstabelle und Anwendung).
3. **WLAN einrichten** (Improv Serial oder Fallback-Hotspot) und Etage / Raum / Geräte-ID neu setzen
   (siehe [Dynamische Konfiguration](de_dynamic-configuration.md)).
4. **Gerät in Home Assistant einbinden** und **„Bluetooth Proxy“** einschalten. Der Proxy erscheint unter
   *Einstellungen → Geräte & Dienste → Bluetooth*.
5. Eine Zeit lang **`Freier Speicher (RAM)`** und die ESP-NOW-Peers im Dashboard **beobachten**.

**Zurück** zu einer Standardvariante geht per OTA (`esphome run ventosync_nosensor.yaml --device <IP>`): Das kleinere
Standard-Image passt in die größere App-Partition, die Konfiguration bleibt erhalten. Das Gerät behält dann das
Partitionslayout der Proxy-Variante (64 KB NVS), was unschädlich ist. Nur ein USB-Flash stellt das
ESPHome-Standardlayout wieder her (und löscht dabei erneut das NVS).

---

## 📡 Funk-Koexistenz mit WLAN und ESP-NOW

Der ESP32-C6 hat **ein** 2,4-GHz-Funkmodul für WLAN, ESP-NOW und Bluetooth. Der Koexistenz-Arbiter von ESP-IDF teilt
es im Zeitmultiplex auf. Bei aktivem WLAN setzt ESPHome 2026.9 das BLE-Scanfenster gleich dem Scanintervall
(Dauerscan) – der Arbiter gibt WLAN weiterhin Vorrang, BLE nutzt die verbleibende Funkzeit.

Was das für VentoSync bedeutet:

- **ESP-NOW-Pakete können sich verzögern oder verloren gehen**, während das Funkmodul auf BLE lauscht. Die
  Raum-Synchronisation verträgt das: Heartbeat alle 60 s, raumweites Fusionsfenster ≥ 5 min, Peer-Timeout 15 min.
  Moduswechsel werden sofort gesendet und vom Master-Heartbeat erneut bestätigt.
- **Empfehlung:** Den Proxy auf **einem** Gerät pro Raum einschalten und die ESP-NOW-Peer-Liste im Dashboard im
  Auge behalten. Fallen Peers aus, lässt sich im Varianten-YAML ein kürzeres Scanfenster setzen, z. B.:

  ```yaml
  esp32_ble_tracker:
    scan_parameters:
      interval: 320ms
      window: 120ms
  ```

  Ein kürzeres Fenster verpasst mehr Advertisements; ESPHome warnt oberhalb von 600 ms.
- Aktive Verbindungen (`active: true`) belegen das Funkmodul länger; sie entstehen nur, wenn Home Assistant sie
  anfordert.

---

## ❓ Einschränkungen & FAQ

**Warum nicht einfach in jede Variante einbauen und ausgeschaltet lassen?**
Das Ausschalten gibt nur RAM und Funkzeit frei – die ~610 KB Code stecken immer im Image. Auf 4-MB-Modulen passt das
nicht neben das Standard-Partitionslayout, und das Layout aller installierten Geräte zu ändern hieße, jedes Gerät
per USB neu zu flashen. Mit einem 8-MB-Modul wäre es möglich – siehe
[Ausblick 8 MB](#-ausblick-8-mb-modul-esp32-c6-mini-1-h8).

**Gibt es eine Variante für PCB v2.0 oder mit Sensoren?**
Noch nicht. Das Paket ist platinenunabhängig (`bluetooth_proxy: !include
packages/integration/bluetooth_proxy.yaml`), aber jedes Sensorpaket verkleinert die ~37 KB Reserve. Vor dem Einsatz
einen Build testen.

**Wie viel RAM braucht er?**
Etwa +33 KB statischen RAM (immer), dazu den Bluedroid-Heap, solange der Proxy eingeschaltet ist. `ble.disable` gibt
diesen Heap wieder frei.

**Alternative ohne Kompromisse?**
Ein separates, günstiges ESP32-Board mit der Standard-Bluetooth-Proxy-Firmware von ESPHome. VentoSync behält dann
seine volle Flash-Reserve.

---

*Siehe auch: [Home-Assistant-Entitäten](de_home-assistant-entities.md) · [Lokales Web-Dashboard](de_local-web-dashboard.md) ·
[ESP-NOW-Kommunikation](de_esp-now-communication.md) · [Dynamische Konfiguration](de_dynamic-configuration.md)*
