# Erweiterung: Anwesenheitserkennung (Radar + PIR)

Dieses Dokument beschreibt die geplante Erweiterung der Wohnraumlüftungssteuerung um zwei Bewegungssensoren:

1. **HLK-LD2450** (24GHz mmWave Radar) für präzise Anwesenheitserkennung.
    * *Einbau:* Hinter der Kunststoffblende, verbunden per Kabel (JST-Stecker auf PCB).
2. **AM312** (Mini-PIR) als **Optionale Ergänzung**.
    * *Hinweis:* Da der PIR eine freie Sicht benötigt ("Guckloch"), kann er nicht unsichtbar im bestehenden Gehäuse verbaut werden. Er wird nur bei Bedarf extern ergänzt.
3. **Zukunftsfähigkeit (PCB):** Vorsehen von zusätzlichen Headern für I2C und UART.

Da der ESP32-C6 keine freien Pins mehr hat, werden die unkritischen Taster auf einen **MCP23017 I/O-Expander** ausgelagert, um die notwendigen High-Speed-Pins (UART) für das Radar freizumachen.

## 1. Hardware-Anpassungen (PCB & Verkabelung)

### A. MCP23017 I/O Expander Integration

Der MCP23017 wird an den bestehenden I2C-Bus angeschlossen.

* **Anschluss:**
  * **SDA:** An `GPIO22` (parallel zu BME680/SCD41)
  * **SCL:** An `GPIO23` (parallel zu BME680/SCD41)
  * **VCC:** 3.3V
  * **GND:** GND
  * **Adresse:** `0x20` (Standard, alle Adress-Pins A0-A2 auf GND)

#### Anschluss-Details (QFN-28 Gehäuse)

* **RESET:** Muss auf **3.3V (HIGH)** gezogen werden. (Z.B. über 10kOhm Pullup oder direkt). Wenn er LOW ist, resettet der Chip.
* **EP (Exposed Pad) / VSS:** Die Metallfläche unten am Chip **MUSS auf GND** verbunden werden (Wärme & Stabilität).
* **INTA / INTB:** (Interrupt A/B). Diese Ausgänge melden, wenn sich ein Eingang ändert.
  * *Da wir keine freien Pins am ESP32 mehr haben:* **Offen lassen (NC)**. ESPHome pollt den Chip regelmäßig über I2C.
* **NC (Not Connected):** Diese Pins haben keine Funktion. Einfach **frei lassen** (nicht anschließen).

### B. Pin-Umzug (Rewiring)

Die Taster wandern auf den Expander, um native GPIOs für UART freizugeben.

| Komponente | Alter Pin (ESP32-C6) | Neuer Pin / Hinweise | Grund |
| :--- | :--- | :--- | :--- |
| **Button Power** | GPIO16 | **MCP GPA0** (Pin 21) | Native UART TX (GPIO16) wird für Radar benötigt. |
| **Button Mode** | GPIO20 | **MCP GPA1** (Pin 22) | Pin frei für spätere Erweiterung (PIR). |
| **Button Level** | GPIO2 | **MCP GPA2** (Pin 23) | Pin wird frei für **Fan RPM** (siehe unten). |
| **Fan RPM** | GPIO17 | **GPIO2** | Native UART RX (GPIO17) wird für Radar benötigt. Pulse Counter läuft auch auf GPIO2. |

*Hinweis: Die Taster schalten gegen GND. Interne Pullups des MCP23017 aktivieren.*

### C. Anschluss der neuen Sensoren

#### 1. HLK-LD2450 (Radar)

Benötigt UART (Serial). **Achtung: 5V Versorgung empfohlen! Logic Level ist 3.3V kompatibel.**
Wir nutzen jetzt die **Hardware-UART Pins** des ESP32-C6 (UART0).

| HLK-LD2450 Pin | Anschluss an ESP32-C6 | Funktion |
| :--- | :--- | :--- |
| **VCC** | **5V** (VBUS/5V Pin) | **WICHTIG:** 5V für stabile Funktion |
| **GND** | GND | Masse |
| **TX** | **GPIO17** (RX0) | Sendet Daten an ESP (ehemals Fan RPM) |
| **RX** | **GPIO16** (TX0) | Empfängt Konfig vom ESP (ehemals Button Power) |

*\*Kreuzung beachten: Sensor TX an ESP RX, Sensor RX an ESP TX*

#### 2. AM312 (PIR) - *OPTIONAL / EXTERN*

Digitaler Output. Kann nicht hinter geschlossener Blende montiert werden.

| AM312 Pin | Anschluss an ESP32-C6 | Funktion |
| :--- | :--- | :--- |
| **VCC** | 3.3V (oder 5V) | Spannungsversorgung |
| **GND** | GND | Masse |
| **OUT** | **GPIO20** | Digital Signal (High = Bewegung) |

### D. Zusätzliche Erweiterungs-Header (Future-Proofing)

Um für künftige Projekte oder externe Sensoren gerüstet zu sein, sollen auf dem PCB folgende Anschlüsse als Stiftleisten (JST oder Pin Header) herausgeführt werden:

1. **Zusätzlicher I2C Port (External I2C):**
    * Pins: SDA (GPIO22), SCL (GPIO23), 3.3V, GND
    * Zweck: Anschluss weiterer Sensoren (z.B. Display, weitere Umweltsensoren).
2. **Zusätzlicher UART Port (External UART):**
    * Pins: **GPIO20** (Shared mit PIR Option) und/oder freie MCP-Pins.
    * *Hinweis:* Da wir GPIO16/17 für das Radar nutzen, sind die dedizierten Hardware-UARTs belegt.
    * *Lösung:* Header für GPIO20 vorsehen. Falls PIR nicht genutzt wird, ist hier ein Pin frei für Software-Serial RX oder TX.

### E. Überlegung: BMP388 / BMP390 Drucksensor (Onboard)

Als weitere Option soll direkt auf dem PCB ein Footprint für einen hochpräzisen Drucksensor (z.B. BMP390) vorgesehen werden.

* **Anschluss:** I2C (parallel zu BME680/SCD41).
* **Adresse:** `0x76` (SDO auf GND) oder `0x77` (SDO auf VCC - Konflikt mit BME680 prüfen!). **Wichtig:** BME680 hat meist 0x77. BMP390 muss auf **0x76** konfiguriert werden.
* **Anschluss:** I2C (parallel zu BME680/SCD41).
* **Adresse:** `0x76` (SDO auf GND) oder `0x77` (SDO auf VCC - Konflikt mit BME680 prüfen!). **Wichtig:** BME680 hat meist 0x77. BMP390 muss auf **0x76** konfiguriert werden.

#### Anschluss-Details (LGA-10 Package) für Adresse 0x76

Bei einem nackten Chip (LGA-10) auf dem PCB:

| Pin Nr. | Name | Funktion | Anschluss |
| :--- | :--- | :--- | :--- |
| **1** | **VDDIO** | Interface Voltage | 3.3V |
| **2** | **SCK** | I2C Clock | SCL (GPIO23) |
| **3** | **VSS** | Ground | GND |
| **4** | **SDI** | I2C Data | SDA (GPIO22) |
| **5** | **SDO** | Address Select | **GND** (für Adr. 0x76) |
| **6** | **CSB** | Chip Select | **3.3V (VDDIO)** (High = I2C Mode) |
| **7** | **INT** | Interrupt | NC (oder an GPIO falls benötigt) |
| **8** | **VDD** | Analog Voltage | 3.3V |
| **9** | **VSS** | Ground | GND |
| **10** | **VSS** | Ground | GND |

*Hinweis: Pullup-Widerstände für SCL/SDA sind für den I2C-Bus generell nötig (4.7k oder 10k).*

**Mögliche Anwendungsfälle:**

1. **Filterüberwachung (indirekt):** Durch Messung des absoluten Luftdrucks im Ansaug- oder Abluftstrom *könnten* Rückschlüsse auf den Druckabfall (und damit Verschmutzung) gezogen werden (benötigt Referenz).
2. **Druckausgleich:** Erkennung von Unter-/Überdruck im Raum im Vergleich zu draußen (benötigt zweiten Sensor außen).
3. **Wetterstation:** Die BMP3xx-Serie ist extrem präzise (±8 Pa relative Genauigkeit) und eignet sich hervorragend als Referenz-Barometer.
4. **Lüftungssteuerung:** Erkennung, ob Fenster/Türen geöffnet wurden (plötzliche Druckänderung) -> Lüftung temporär stoppen.

### F. Erweiterte Luftqualität: SCD41 + SEN54/55 (PM & NOx)

Während der **SCD41** bereits für die CO2-Regelung vorgesehen ist, bietet die Ergänzung um einen Partikelsensor einen umfassenden Schutz vor Außenluftbelastung.
**Problem**: Das ist nur über ein separates Gerät möglich, da der Sensor außen angebracht werden muss und die Abmaße des SEN54/55 nicht in die Lüftungsbox passen. Es gibt aber eine Alternative: Der **Sensirion SPS30** ist kleiner und könnte passen.

* **SCD41 (CO2):** Misst verbrauchte Innenluft.
  * *Regelung:* "Wir brauchen mehr Sauerstoff".
  * *Status:* Bereits eingeplant/vorhanden.
* **Sensirion SEN54 oder SEN55 (PM1.0/2.5/4.0/10 + NOx):** Misst belastete Außenluft (Staub, Rauch, Abgase).
  * *Nutzen:* Verhindert, dass bei schlechter Außenluft (z.B. Kaminrauch des Nachbarn, Berufsverkehr) Schadstoffe ins Haus gesaugt werden.
  * *Regelung:* "Draußen ist Dreck, Lüftung reduzieren oder stoppen".
  * *Anschluss:* I2C (parallel zum Bus). Benötigt **5V** Spannungsversorgung!
  * *Hinweis:* Der SEN55 misst zusätzlich Stickoxide (NOx), was an stark befahrenen Straßen relevant ist.

### G. Differenzdruck-Sensor: SDP810 / SDP3x

Für die **echte Filterüberwachung** und Volumenstromregelung.

* **Empfehlung:** Sensirion **SDP810** (Robust) oder **SDP3x** (Winzige SMD-Variante).
* **Warum?**
  * Misst den Druckabfall über dem Filter (Druck VOR Filter vs. Druck NACH Filter).
  * Das ist der "Gold-Standard" gegenüber der reinen Zeitmessung oder dem absoluten Druck (BMP390).
* **Use-Case:**
    1. **Smarter Filterwechsel:** Meldung genau dann, wenn der Filter tatsächlich voll ist.
    2. **Konstant-Volumenstrom:** Der Lüfter regelt automatisch nach, wenn der Filter sich zusetzt, um die Luftmenge konstant zu halten.
* **Anschluss:** I2C.

### H. Helligkeitssensor: BH1750 / VEM7700

Für den Schlafzimmer-Betrieb (LED-Dimmung).

* **Empfehlung:** **BH1750** (Günstig, gut) oder **VEM7700** (Präziser, Low-Light).
* **Warum?**
  * Die Status-LEDs der Lüftung können nachts stören.
* **Use-Case:**
    1. **Auto-Dimming:** LEDs werden gedimmt oder ganz abgeschaltet, wenn der Raum dunkel ist.
    2. **Sleep-Mode:** Automatische Erkennung "Nachtruhe" -> Lüfter fährt auf flüsterleise Stufe runter.
* **Anschluss:** I2C.
  * *Montage:* Benötigt Lichtöffnung im Gehäuse oder Lichtleiter.

### I. I2C Adress-Map (Konfliktprüfung)

Damit alle Geräte am Bus funktionieren, müssen die Adressen eindeutig sein.

| Gerät | Adresse (Hex) | Status/Notiz |
| :--- | :--- | :--- |
| **MCP23017** | `0x20` | **OK.** Standard (A0/A1/A2 = GND). |
| **BH1750** (Lux) | `0x23` | *Optional.* Standard (ADDR = GND). |
| **SDP810** (Diff.Druck) | `0x25` | *Optional.* Standard. |
| **PCA9685** | `0x40` | **OK.** Standard (A0-A5 = GND). |
| **SCD41** (CO2) | `0x62` | **OK.** Fixe Adresse. |
| **SEN54 / SEN55** | `0x69` | *Optional.* Fixe Adresse. |
| **BMP390** (Druck) | **`0x76`** | **WICHTIG:** SDO auf GND legen! |
| **BME680** (IAQ) | `0x77` | **OK.** Standard (SDO auf VCC oder offen). |

**Fazit:** Keine Konflikte, wenn beim **BMP390 Pin SDO auf GND** gelegt wird. Der BME680 darf dann auf 0x77 bleiben.

---

## 2. Software-Anpassungen (ESPHome YAML)

### A. MCP23017 Konfiguration (Update)

Falls noch nicht vorhanden, `pca9685` ist für PWM, `mcp23017` ist neu für Inputs.

```yaml
i2c:
  sda: GPIO22
  scl: GPIO23

mcp23017:
  - id: mcp_buttons
    address: 0x20
```

### B. Taster auf MCP23017 umziehen

Die `binary_sensor` Einträge für die Buttons müssen angepasst werden.

```yaml
binary_sensor:
  - platform: gpio
    name: "Button Power"
    id: button_power
    pin:
      mcp23xxx: mcp_buttons
      number: 0 # GPA0
      mode:
        input: true
        pullup: true # MCP hat interne Pullups
      inverted: true # Schaltet gegen GND
    # ... on_click logic bleibt gleich ...

  - platform: gpio
    name: "Button Mode"
    id: button_mode
    pin:
      mcp23xxx: mcp_buttons
      number: 1 # GPA1
      mode:
        input: true
        pullup: true
      inverted: true
    # ... on_click logic bleibt gleich ...

  - platform: gpio
    name: "Button Level"
    id: button_level
    pin:
      mcp23xxx: mcp_buttons
      number: 2 # GPA2
      mode:
        input: true
        pullup: true
      inverted: true
    # ... on_click logic bleibt gleich ...
```

### C. HLK-LD2450 Konfiguration

UART und Komponente hinzufügen.

```yaml
uart:
  id: uart_radar
  tx_pin: GPIO16 # ESP TX -> Sensor RX
  rx_pin: GPIO17 # ESP RX -> Sensor TX
  baud_rate: 256000
  parity: NONE
  stop_bits: 1

ld2450:
  uart_id: uart_radar
  id: radar_sensor
```

### D. Fan RPM Anpassung (Wichtig!)

Da GPIO17 jetzt für UART genutzt wird, muss der Lüfter-Tacho auf GPIO2 umziehen.

```yaml
sensor:
  - platform: pulse_counter
    pin: GPIO2 # Moved from GPIO17
    name: "Lüfter Drehzahl"
    # ... rest configuration check ...
```

### E. AM312 Konfiguration

```yaml
binary_sensor:
  - platform: gpio
    pin: GPIO20
    name: "PIR Bewegung"
    device_class: motion
    filters:
      - delayed_off: 5s # Kurzes Entprellen
```

## 3. Platzierungsempfehlung

* **Radar (HLK-LD2450):**
  * Montage **hinter** der Kunststofffront der Lüftungsanlage (unsichtbar).
  * Verbunden über ein Kabel mit dem PCB (Steckverbinder vorsehen).
  * Ausrichtung: Leicht geneigt (5-10°) nach unten.
* **PIR (AM312) - Optional:**
  * Benötigt "Sichtkontakt" zum Raum (Linse muss herausschauen).
  * Wird nur bei explizitem Bedarf nachgerüstet (z.B. seitlich am Gehäuse).
