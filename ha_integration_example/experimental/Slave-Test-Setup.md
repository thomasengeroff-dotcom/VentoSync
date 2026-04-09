# 🧪 Slave Test Setup (Simulation)

Dieses Dokument beschreibt die `espslavetest.yaml` Konfiguration. Sie dient dazu, einen **zweiten Lüfter (Slave)** zu simulieren, ohne dass die komplette Hardware (Platine, Lüfter, Sensoren) angeschlossen sein muss.

Das ist ideal, um die **ESP-NOW Synchronisation** und die **Luftqualitäts-Ampel** zu testen.

## 🎯 Zweck

* **Paar-Betrieb testen**: Überprüfung, ob Master und Slave synchron schalten (z.B. Pendellüftung: Master rein, Slave raus).
* **Reichweitentest**: Testen der ESP-NOW Verbindung an verschiedenen Orten im Haus mit minimalem Aufwand.
* **Ampel-Funktion**: Visualisierung der vom Master gemessenen Luftqualität (IAQ) auf einem externen Gerät.

## ⚙️ Funktionsweise

Der `espslavetest` verhält sich softwareseitig wie ein vollwertiges Lüftungsgerät, jedoch sind alle Aktoren (Lüfter, Relais) **simuliert**.

1. **Rolle**: Das Gerät ist als **Device ID 2** konfiguriert und läuft auf **Phase B** (`is_phase_a: false`). Es ist somit der direkte Gegenspieler zum Master (Device ID 1, Phase A).
2. **ESP-NOW**: Es empfängt Befehle vom Master:
    * **Modus-Wechsel**: Schaltet virtuell zwischen Wärmerückgewinnung und Durchlüften um.
    * **Lüfterstufe**: Passt die (simulierte) Drehzahl an.
    * **IAQ-Daten**: Empfängt die Luftqualitäts-Bewertung (Gut/Mittel/Schlecht) zur Steuerung der echten LEDs.
3. **Mock-Hardware**:
    * Der Lüfter ist ein `template fan`, der Änderungen nur in die Logs schreibt.
    * Die Status-LEDs (für das Panel) sind `internal` Templates und existieren nur im Code, damit die Logik (`automation_helpers.h`) fehlerfrei läuft.

## 🔌 Hardware Setup (Minimal)

Du benötigst nur einen **Seeed Studio XIAO ESP32-C6** (oder baugleich).

Optional kannst du 3 LEDs anschließen, um die **Luftqualität (Traffic Light)** anzuzeigen:

| LED Farbe | GPIO Pin | Bedeutung |
| :--- | :--- | :--- |
| 🔴 **Rot** | **GPIO0** (D0) | Luftqualität Schlecht (IAQ > 200) |
| 🟡 **Gelb** | **GPIO1** (D1) | Luftqualität Mittel (IAQ 51-200) |
| 🟢 **Grün** | **GPIO2** (D2) | Luftqualität Gut (IAQ <= 50) |

*Hinweis: Vorwiderstände für die LEDs nicht vergessen (z.B. 220Ω - 1kΩ).*

## 🚀 Inbetriebnahme

1. Verbinde den Test-ESP mit dem PC.
2. Flashe die Firmware:

    ```bash
    py -3.13 -m esphome run espslavetest.yaml
    ```

3. Beobachte die Logs. Wenn der Master läuft, solltest du `[Slave] Fan ...` Nachrichten und ESP-NOW Pakete sehen.
4. Ändere am Master die Lüfterstufe oder den Modus -> Der Slave sollte im Log sofort nachziehen (invertiert bei Pendellüftung).
