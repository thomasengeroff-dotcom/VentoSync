# 🛠️ Hardware Setup: Prototyp & PCB

Dieses Dokument spezifiziert die Hardware-Komponenten, die Verkabelung für den Prototypen (Breadboard) und die Design-Vorgaben für das finale PCB (EasyEDA).

> [!IMPORTANT]
> **Pin-Referenz**: Die Pin-Belegung in diesem Dokument wurde an den aktuellen Software-Stand (`esptest.yaml`) angepasst und weicht daher von ursprünglichen Entwürfen ab. **I2C liegt auf D4/D5.**

---

## 1. Übersicht & BOM (Bill of Materials)

### Zentrale Einheit & Power

* **MCU**: Seeed Studio XIAO ESP32C6
* **Netzteil (High Voltage)**: Recom RAC10-12SK/277 (230V AC -> 12V DC, 10W)
* **Logic Power (DC/DC)**: Recom R-78E5.0-0.5 oder Traco TSR 1-2450 (12V -> 5V)
* **Sicherung**: 1A Slow Blow (Träge)
* **Varistor**: S10K275 (Überspannungsschutz Eingang)
* **Pufferung**:
  * 470µF / 25V Elko (12V Rail)
  * 10µF / 50V Elko (Eingang DC/DC)
  * 10µF / 16V Elko (Ausgang DC/DC 5V)
  * 100nF Kerko (HF-Entstörung 5V Rail)

### Aktoren & Sensoren

* **Lüfter**: 120mm PWM Lüfter (12V) -> *Geplant: ebm-papst AxiRev*
  * *Interface*: BC547 NPN-Transistor + 1kΩ Basiswiderstand
* **BME680**: Bosch Umweltsensor (I2C) - Temperatur, Feuchte, Druck, IAQ
* **APDS-9960 / DEBO GESTURE**: Gesten-Sensor (I2C) für Wake-Up
* **Display**: 0.91" OLED (SSD1306, 128x32, I2C)
* **LEDs**: DEBO LED RING 7 (WS2812B)
  * *Schutz*: 470Ω Widerstand in der Datenleitung
  * *Puffer*: 100nF Kerko direkt am Ring
* **Touch**: DEBO TOUCH (Kapazitiv)
* **NTC-Sensorik**: 2x 10kΩ NTC (Beta 3435)
  * *Beschaltung*: 10kΩ 0.1% Referenzwiderstand + 100nF Filter + 3.3V Zener/TVS

---

## 2. Offizielle XIAO ESP32C6 Pin-Map

### Vollständige Pin-Übersicht (Seeed Studio)

| XIAO Pin | Funktion | Chip Pin | Alternate Functions | Beschreibung |
| :--- | :--- | :--- | :--- | :--- |
| **5V** | VBUS | - | - | Power Input/Output |
| **GND** | GND | - | - | Ground |
| **3V3** | 3V3_OUT | - | - | Power Output |
| **D0** | Analog | GPIO0 | LP_GPIO0 | GPIO, **ADC** |
| **D1** | Analog | GPIO1 | LP_GPIO1 | GPIO, **ADC** |
| **D2** | Analog | GPIO2 | LP_GPIO2 | GPIO, **ADC** |
| **D3** | Digital | GPIO21 | SDIO_DATA1 | GPIO |
| **D4** | SDA | GPIO22 | SDIO_DATA2 | GPIO, I2C Data |
| **D5** | SCL | GPIO23 | SDIO_DATA3 | GPIO, I2C Clock |
| **D6** | TX | GPIO16 | - | GPIO, UART Transmit |
| **D7** | RX | GPIO17 | - | GPIO, UART Receive |
| **D8** | SCK | GPIO19 | SDIO_CLK | GPIO, SPI Clock |
| **D9** | MISO | GPIO20 | SDIO_DATA0 | GPIO, SPI Data |
| **D10** | MOSI | GPIO18 | SDIO_CMD | GPIO, SPI Data |
| **MTDO** | - | GPIO7 | - | JTAG |
| **MTDI** | - | GPIO5 | - | JTAG, **ADC** |
| **MTCK** | - | GPIO6 | - | JTAG, **ADC** |
| **MTMS** | - | GPIO4 | - | JTAG, **ADC** |
| **EN** | - | CHIP_PU | - | Reset |
| **Boot** | - | GPIO9 | - | Enter Boot Mode |
| **RF Switch Port** | - | GPIO14 | - | Switch onboard/UFL antenna |
| **RF Switch Power** | - | GPIO3 | - | Power |
| **Light** | - | GPIO15 | - | User Light |

> ⚠️ **Wichtig**: Nur **D0, D1, D2** (GPIO0, GPIO1, GPIO2) sowie die JTAG-Pins (GPIO4, GPIO5, GPIO6, GPIO7) haben ADC-Funktionalität!

---

## 3. Projekt Pin-Belegung (Master)

Dies ist die verbindliche Belegung für Firmware und PCB.

| XIAO Pin | GPIO | Funktion | Anschluss-Details |
| :--- | :--- | :--- | :--- |
| **D0** | GPIO0 | **NTC Innen** | Analog In (Spannungsteiler 10k + Filter) - *ADC-fähig* |
| **D1** | GPIO1 | **NTC Außen** | Analog In (Spannungsteiler 10k + Filter) - *ADC-fähig* |
| **D2** | GPIO2 | **LED Data** | Zu WS2812 DIN (via 470Ω) |
| **D3** | GPIO21 | **Touch Button 1 (Rechts)** | Mode/Power Control (DEBO TOUCH) |
| **D4** | GPIO22 | **I2C SDA** | BME680, OLED, APDS-9960 (mit 4.7kΩ Pullup) |
| **D5** | GPIO23 | **I2C SCL** | BME680, OLED, APDS-9960 (mit 4.7kΩ Pullup) |
| **D6** | GPIO16 | **Fan PWM** | An Basis BC547 (via 1kΩ) |
| **D7** | GPIO17 | **Fan Tacho** | Von Lüfter Pin 3 (mit 10k Pullup auf 3V3 & 3.3V Zener-Diode) |
| **D8** | GPIO19 | *Reserve* | (Frei) |
| **D9** | GPIO20 | *Reserve* | (Frei) |
| **D10** | GPIO18 | **Touch Button 2 (Links)** | Fan Intensity Control (DEBO TOUCH) |

---

## 4. Verkabelung & Prototyping (Breadboard)

### A. Spannungsversorgung

1. **12V Netzteil** versorgt die **12V Schiene** (für den Lüfter).
2. **DC/DC Wandler** (TSR 1-2450) wandelt 12V auf **5V** für den LED-Ring und den XIAO (an 5V Pin).
3. **XIAO 3V3 Pin** versorgt alle Sensoren (BME680, OLED, APDS-9960, Touch).
    * ⚠️ **ACHTUNG**: Sensoren, insbesondere der APDS-9960, **nicht** an 5V anschließen!
    * **Pufferung**: Stecke **2x 10µF Elko** und **1x 100nF Kerko** (parallel) direkt an die 5V/GND Pins des XIAO auf das Breadboard.

### B. I2C Bus (Sensoren & Display)

* **SDA**: Alle SDA-Pins an **XIAO D4**.
* **SCL**: Alle SCL-Pins an **XIAO D5**.
* **Power**: VCC an 3V3, GND an GND.
* **Pullups**: Breadboards brauchen oft 4.7kΩ Pullups von SDA/SCL zu 3V3 für stabile Signale.

### C. Lüfter Interface (12V / 3.3V Logic)

* **PWM**: XIAO D6 -> 1kΩ Widerstand -> Basis BC547. Emitter an GND, Collector an Lüfter PWM (Blau).
* **Tacho**: Lüfter Tacho (Grün) an XIAO D7.
  * **Wichtig**: 10kΩ Pullup von D7 nach **3.3V** (damit das Signal sauber ist).
  * **Schutz**: Zener-Diode 3.3V von D7 nach GND (Kathode an D7), um den Pin zu schützen.

### D. LED Ring (5V Power, 3.3V Logic)

* **VCC**: 5V (vom DC/DC).
* **GND**: GND.
* **DIN**: XIAO D2 -> 470Ω Widerstand -> DIN (verhindert Reflexionen und schützt erste LED).
* **Stabilität**: 100nF Kerko direkt an den Power-Pins des Rings.

---

## 5. PCB Design Spezifikation (EasyEDA)

Vorgaben für den Schaltplan (Schematic) und das Layout.

### 4.1 Power Sektion (HV & LV)

* **Eingang**: Schraubklemme 2-Pin (AC_L, AC_N).
* **Schutzbeschaltung**: Sicherung + Varistor.
* **AC/DC**: RAC10-12SK footprint.
* **DC/DC**: TSR 1-2450 footprint.
* **Filterung**: Elkos wie oben in der Stückliste definiert platzieren.

### 4.2 Interfaces

* **Lüfter Stecker**: 4-Pin PWM Header (Standard PC Fan Pinout: GND, 12V, Tacho, PWM).
* **NTC Stecker**: 2-Pin JST oder Schraubklemmen für externe Fühler.
  * Hardware-Filter (RC-Glied) direkt am ADC-Eingang vorsehen (10k Referenz + 100nF).

### 4.3 Externer UI-Bus (Front Panel Connector)

Verbindung zum abgesetzten Bedienelement (im Gehäusedeckel).

* **Stecker-Typ**: 2x4 Pin Wannenstecker (IDC) oder 8-Pin Header.
* **Pinout (Vorschlag)**:
    1. **+5V** (LED Power - High Current Trace!)
    2. **GND**
    3. **+3V3** (Sensor Power)
    4. **D2** (LED Data - geschützt mit TVS)
    5. **D3** (Touch 1 - geschützt mit TVS)
    6. **D4** (SDA - geschützt mit SRV05-4 Array)
    7. **D5** (SCL - geschützt mit SRV05-4 Array)
    8. **D6** (Touch 2 / Reserve)
* **ESD-Schutz**: Da der Nutzer das Panel berührt, müssen alle Datenleitungen (D2..D6) mit TVS-Dioden oder ESD-Arrays (z.B. SRV05-4) gegen statische Entladung geschützt werden.
* **Puffer**: 100µF Elko auf der 5V Leitung direkt am Stecker auf dem Mainboard vorsehen, um Spannungsabfälle durch das Kabel abzufangen.
