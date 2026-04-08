# 🛠️ Hardware Setup: Prototyp & PCB

Dieses Dokument spezifiziert die Hardware-Komponenten, die Verkabelung für den Prototypen (Breadboard) und die Design-Vorgaben für das finale PCB (EasyEDA).

> [!IMPORTANT]
> **Pin-Referenz**: Die Pin-Belegung basiert auf der **Ansteuerung der Buttons via MCP23017 I2C Expander** und der **LEDs via PCA9685 I2C PWM Driver**. Das Front-Panel wird über einen 14-Pin FFC Connector angeschlossen.

---

## Zentrale Einheit & Power

* **MCU**: Seeed Studio XIAO ESP32C6
* **Netzteil (High Voltage)**: XP Power ECE10US12 oder TRACO POWER TMPS 10-112 (230V AC -> 12V DC, 10W)
  * *Hinweis*: Der XP Power ECE10US12 ist eine Alternative zum TRACO POWER TMPS 10-112. Grundriss und Pin-Belegung sind identisch.
  * **Empfehlung**: Das **TRACO POWER TMPS 10-112 ist zu bevorzugen**. Es besitzt neben der Standard-IT-Zulassung (EN/UL 62368-1) zusätzlich die **Haushaltsgerätenorm EN 60335-1**. Zudem bietet es eine etwas höhere Spitzenlast (1080 mA vs. 1000 mA) und eine minimal bessere Effizienz (84% vs. 83%), was es für den 24/7-Einsatz in der Wohnraumlüftung robuster und normgerechter macht.
* **Logic Power (5V/MCU Supply)**: **Diodes Inc. AP63205WU-7** (12V -> 5V, 2A)
  * *Versorgung*: Speist den XIAO ESP32C6 (via 5V Pin).
* **Peripheral Power (3.3V Buck)**: **Diodes Inc. AP63203WU-7** (12V -> 3.3V, 2A)
  * *Versorgung*: Speist alle Sensoren (BME680), PCA9685 PWM Driver und Front-Panel LEDs.
  * *Hinweis*: Der interne 3.3V LDO des ESP32 wird **nicht** genutzt (Pin open).
* **Sicherung**: 1,6A Slow Blow (Träge), z.B.: <https://www.mouser.de/ProductDetail/Littelfuse/021501.6HXP?qs=qI%252BDxnNls1%252B5BKXaCJ2cdg%3D%3D>
* **Varistor**: S10K275 (Überspannungsschutz Eingang)
* **Pufferung**:
  * 470µF / 25V Elko (12V Rail) - **C1**
  * 10µF / 25V MLCC (Eingang 5V DC/DC) - **C16**
  * **470µF / 16V Elko** (Ausgang 5V DC/DC) - **C6** - *Wichtig für ESP32 WiFi Peaks!*
    * 100nF Kerko (HF-Entstörung 5V Rail) - **C2**
  * **AP63203 Beschaltung** (siehe unten bei PCB Design)

### Aktoren & Sensoren

* **Lüfter (Option 1 - Aktuell)**: **ebm-papst VarioPro® 4412 FGM PR** (3-Pin)
  * *4412*: Baugröße 119x119x25mm, 12V DC.
  * *F*: Flache Bauweise (25mm).
  * *G*: Gleitlager (Sintec).
  * *M*: Mittlere Drehzahl/Leistung.
  * *PR*: Programmierbar (VarioPro), hier genutzt als 3-Pin Anschluss (Spannungsregelung).
  * *Reversierbar*: Ja (via Programmierung)
  * *Interface*: **Universal-Interface im 3-Pin Mode** (High-Side PWM + Filter).

* **Lüfter (Option 2 - Upgrade)**: **ebm-papst AxiRev** (4-Pin PWM)
  * *Besonderheit*: **Volle Reversierbarkeit** bei fast identischer Effizienz in beide Richtungen.
  * *Größe*: 126mm (Benötigt ggf. mechanische Anpassung).
  * *Interface*: **Universal-Interface im 4-Pin Mode** (Direktes PWM, Tacho, 12V Dauerplus).
  * *Vorteil*: Extrem leise Umschaltung und optimierte Strömungskennlinie für Pendellüfter.
* **BME680**: ⚠️ *NICHT IN BOM* - Bosch Umweltsensor (I2C) - Temperatur, Feuchte, Druck, IAQ --> kann extern angeschlossen werden oder jeder andere I2C Sensor verwendet werden.
* **Front-Panel**: 9 LEDs (via PCA9685) und 3 Buttons (via MCP23017) über 14-Pin FFC Connector.
* **LED Driver**: **PCA9685PW** (I2C, 0x40) - 16-Kanal PWM Driver für dimmbare LEDs.
* **SCD41**: ⚠️ *NICHT IN BOM* - CO2 Sensor (I2C, 0x62) über 4-Pin Header H2. --> kann extern angeschlossen werden oder jeder andere I2C Sensor verwendet werden.
* **NTC-Sensorik**: 2x 10kΩ NTC (Beta 3435)
  * *Beschaltung*: 10kΩ 0.1% Referenzwiderstand + 100nF Filter + 3.3V Zener/TVS

## 1. Detaillierte BOM (Bill of Materials)

Status: **Final (22.02.2026)** - ✅ Verified (Passend zur Schematic: `EasyEDA-Pro\SCH_ESPHome VentoSync PWM_2026-02-22.pdf`)

| Nr. | Menge | Wert / Bauteil | Designator | Footprint | Hersteller Part | Notiz |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| 1 | 1 | 470µF / 25V | C1 | Radial TH | EEUFR1E471B | Main 12V Buffer |
| 2 | 6 | 100nF | C2, C3, C11, C14, C17, C20 | 0805 | CL21B104KBCNNNC | Bypass / HF Caps |
| 3 | 3 | 100nF | C4, C24, C25 | 0402 | CL05B104KO5NNNC | Bypass Caps |
| 4 | 1 | 470µF / 16V | C6 | Radial TH | ERS1CM471F12OT | 5V Output Buffer |
| 5 | 4 | 22µF / 6.3V | C12, C13, C18, C19 | 0603 | CL10A226MQ8NRNC | AP63203/AP63205 Output |
| 6 | 2 | 10µF | C16, C21 | 1206 | CL31A106KBHNNNE | AP63203/AP63205 Input |
| 7 | 2 | 22µF / 25V | C22, C23 | 0805 | CL21A226MAQNNNE | Buck VIN Input Caps |
| 8 | 1 | Header 4P | CN1 | 2.54mm TH | 470533000 | Connector |
| 9 | 3 | ESD Diode | D1, D2, D3 | SOT-23-6 | USBLC6-2SC6 | Dataline Protection |
| 10 | 2 | Diode | D4, D5 | SMB | RS3A | Fast Recovery Diode |
| 11 | 1 | Fuse Holder | FH1 | TH | 65800001009 | 5x20mm 10A 250V |
| 12 | 1 | Header 2x4P | H1 | 2.54mm SMD | PZ254-2-04-S | Connector |
| 13 | 1 | Header 4P | H2 | 1.27mm TH | PZ127V-11-04-0720 | Connector |
| 14 | 2 | Header 4P | H4, H5 | 2.54mm TH | PZ254V-11-04P | UART/Ext Connector (z.B. MR24HPC1) |
| 15 | 2 | 3.9µH | L2, L3 | SMD 4x4 | ANR4030T3R9M | Buck Inductors |
| 16 | 1 | NMOS | QPWM1 | SOT-23 | PMV16XNR | Low-Side Switch PWM |
| 17 | 3 | 4.7kΩ | R1, R2, R20 | 0402 | FRC0402F4701TS | Pullups |
| 18 | 2 | 10kΩ | R4, R5 | 0603 | HoAR0603-1/10W-10KR-0.1%-TCR25 | Resistors |
| 19 | 1 | 470Ω | R8 | 0603 | 0603WAF4700T5E | Resistor |
| 20 | 1 | 1kΩ | R12 | 0603 | RC0603FR-071KL | Resistor |
| 21 | 1 | 10kΩ | R13 | 0402 | CRCW040210K0FKED | Pullup/Pulldown |
| 22 | 2 | 1.2kΩ | R18, R19 | 0402 | FRC0402F1201TS | Resistors |
| 23 | 2 | 47kΩ | RB1, RB2 | 0805 | 0805W8F4702T5E | Bleeder Resistors AC |
| 24 | 2 | 4x 470Ω | RN1, RN2 | 0603 Array | 4D03WGJ0471T5E | LED Series Resistors |
| 25 | 2 | 4x 10kΩ | RN3, RN4 | 0603 Array | 4D03WGJ0103T5E | Pullups |
| 26 | 1 | Terminal | U1 | 5.08mm TH | DB128L-5.08-2P-BK-S | AC Input 230V |
| 27 | 1 | Power Module | U2 | Module | TMPS10-112 | 230V->12V AC/DC |
| 28 | 8 | ESD Diode | U3, U4, U9-U14 | SOT-23-3 | PESD5V0S2BT | Dataline Protection |
| 29 | 2 | Header 7P | U6, U7 | 2.54mm TH | B254N02-0B7P51-H85C32 | XIAO Sockets |
| 30 | 1 | FPC Conn | U8 | SMD 0.5mm | FPC 0.5-14P LTH2.0 | Front Panel Connector |
| 31 | 1 | LED Driver | U15 | TSSOP-28 | PCA9685PW,118 | 16-Ch PWM I2C |
| 32 | 2 | Terminal | U17, U18 | 2.54mm | KF128-2.54-2P | NTC/Misc Connectors |
| 33 | 1 | 3.3V Buck | U25 | TSOT-26 | AP63203WU-7 | 12V->3.3V DC/DC |
| 34 | 1 | 5V Buck | U26 | TSOT-23-6 | AP63205WU-7 | 12V->5V DC/DC |
| 35 | 3 | LED Green | U27, U31, U32 | 0402 | YLED0402G | Status LEDs |
| 36 | 1 | I/O Expander | U29 | QFN-28 | MCP23017T-E/ML | 16-Bit I2C I/O (VentoMaxx Panel) |
| 37 | 1 | BMP390 | U30 | LGA-10 | BMP390 | Barometrischer Drucksensor |
| 38 | 2 | PTC Fuse | U33, U34 | 1206 | BSMD1206-100-30V | 1A 30V Resettable Fuse |
| 39 | 1 | Varistor | V1 | TH | B72210S0271K101 | AC Protection (220pF@1kHz cap equiv) |

> [!TIP]
> **Check-Ergebnis**:
>
> 1. **Integration MR24HPC1**: Der neue UART-Header (H4/H5) ermöglicht den direkten Anschluss des MR24HPC1 Radar-Sensors für Anwesenheitserkennung auf dem Board.
> 2. **BMP390 & MCP23017**: Update auf LGA-10 (U30) und QFN-28 (U29) Packages reflektiert das neue kompakte PCB-Design.
>
> **Freigabe: BOM ist valide für Rev 2026-02-22.**

---

## 2. Pin-Mapping

### A. XIAO ESP32C6 (Master)

| XIAO Pin | GPIO | Funktion | Type | Status | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **D0** | GPIO0 | **NTC_OUT_SIG** | Analog | ✅ OK | NTC Außen (Abluft) als ADC1_CH0. |
| **D1** | GPIO1 | **NTC_IN_SIG** | Analog | ✅ OK | NTC Innen (Zuluft) als ADC1_CH1. |
| **D2** | GPIO2 | **MCP23017_RESET** | Output | ✅ OK | Reset-Signal für den I/O Expander. |
| **D3** | GPIO21 | **PCA9685_OE** | Output | ✅ OK | Output Enable für LED Driver (Active Low). |
| **D4** | GPIO22 | **I2C SDA** | I2C | ✅ OK | Native Hardware I2C SDA. |
| **D5** | GPIO23 | **I2C SCL** | I2C | ✅ OK | Native Hardware I2C SCL. |
| **D6** | GPIO16 | **RADAR_RX** | UART RX | ⚠️ Check | UART RX für MR24HPC1 (Logger muss auf USB!). |
| **D7** | GPIO17 | **RADAR_TX** | UART TX | ⚠️ Check | UART TX für MR24HPC1 (Logger muss auf USB!). |
| **D8** | GPIO19 | **FAN_PWM** | Output | ✅ OK | PWM Signal für den Hauptlüfter. |
| **D9** | GPIO20 | **FAN_TACHO** | Input | ✅ OK | Tacho Signal vom Lüfter (Pullup über 3.3V). |
| **D10** | GPIO18 | **Unbelegt (NC)** | - | ✅ OK | Reserviert / Nicht verbunden. |

### C. Universal-Lüfter Interface (3-Pin & 4-Pin Support)

Das Board unterstützt sowohl 4-Pin PWM (AxiRev) als auch 3-Pin PWM (VarioPro) Lüfter.

**Detaillierte Schaltpläne und BOM**: Siehe [Anleitung-Fan-Circuit.md](Anleitung-Fan-Circuit.md)

**High-Side Circuit (4-Pin Mode):**

1. **Level Shifter Q3** (NPN **S8050**): GPIO16 → R3 (1kΩ) → Q3 Basis. Q3 Emitter → GND.
2. **PMOS Q1** (AO3401): Q3 Collector → Q1 Gate + **R8 (2.2kΩ)** Pullup auf 12V. Q1 Source → 12V, Drain → PWM_12V_OUT.
3. **D1 (B5819WS)**: Freilaufdiode Kathode → PWM_12V_OUT, Anode → GND.
4. **LC-Filter**: PWM_12V_OUT → L1 (220µH) → DC_VAR_12V → **C15 (100µF)** → GND.

### B. PCA9685 PWM Driver (Adresse 0x40)

Der PCA9685 steuert die 9 LEDs des Front-Panels über PWM (dimmbar).

| PCA9685 Kanal | Funktion | Panel Komponente | Widerstand |
| :--- | :--- | :--- | :--- |
| **Channel 0** | *NC* | *Not Connected* | - |
| **Channel 1** | PWM OUTPUT | LED Intensität 1 | 470Ω |
| **Channel 2** | *NC* | *Not Connected* | - |
| **Channel 3** | PWM OUTPUT | LED Intensität 3 | 470Ω |
| **Channel 4** | *NC* | *Not Connected* | - |
| **Channel 5** | PWM OUTPUT | LED Intensität 5 | 470Ω |
| **Channel 6** | *NC* | *Not Connected* | - |
| **Channel 7** | PWM OUTPUT | LED Master (Status) | 470Ω |
| **Channel 8** | PWM OUTPUT | LED Power (On/Off) | 470Ω |
| **Channel 9** | PWM OUTPUT | LED Intensität 2 | 470Ω |
| **Channel 10** | *NC* | *Not Connected* | - |
| **Channel 11** | PWM OUTPUT | LED Intensität 4 | 470Ω |
| **Channel 12** | *NC* | *Not Connected* | - |
| **Channel 13** | PWM OUTPUT | LED Modus: WRG | 470Ω |
| **Channel 14** | *NC* | *Not Connected* | - |
| **Channel 15** | PWM OUTPUT | LED Modus: Durchlüften | 470Ω |

### C. Front-Panel Buttons (via MCP23017)

Die Taster des Bedienpanels werden nun komplett über den **MCP23017** I/O-Expander abgefragt (I2C), wodurch ESP32-Pins für Lüfter und Radar freigeworden sind.

| I/O Expander | Funktion | Beschaltung |
| :--- | :--- | :--- |
| **GPA0** | Button Power | 10kΩ Pullup (RN3), Active Low |
| **GPA1** | Button Mode | 10kΩ Pullup (RN3), Active Low |
| **GPA2** | Button Level | 10kΩ Pullup (RN3), Active Low |
| **GPB0-GPB3** | Unbelegt (NC) | Auf Pin-Header herausgeführt für Erweiterungen (z.B. externe Buttons) |

---

## 3. Verkabelung & Bedienpanel (FFC 14-Pin)

Das Front-Panel wird über ein 14-Pin Flachbandkabel (0.5mm Pitch) angeschlossen.

**Anschluss-Logik**:

* **LEDs**: Angesteuert via PCA9685 PWM Outputs (Channels 6-15) mit 470Ω/10kΩ Vorwiderständen.
* **Buttons**: Abgefragt über den MCP23017 I/O Expander (GPA0-GPA2) mit 10kΩ Pullups, schalten gegen GND.
* **Connector**: FPC 0.5-14P (U17) - SHOU HAN FPC 0.5-14P LTH2.0

---

## 4. PCB Design Hinweise

* **PCA9685**:
  * OE Pin (Pin 23) an GPIO21 (D3) mit 10kΩ Pullup auf 3.3V
  * 100nF Bypass-Kondensator direkt an VDD (Pin 28) - **C4**
  * 10µF Bulk-Kondensator auf 3.3V Rail
* **FFC Connector**: 14-Pin FPC/FFC Connector (0.5mm Pitch) - U17
* **ESD Schutz**: Alle Leitungen zum FFC Connector sind mit TVS-Dioden (PESD5V0S2BT) geschützt.

### AP63203 (12V -> 3.3V) & AP63205 (12V -> 5V) Implementierung

Der **AP63203WU-7** (**U25**) versorgt alle externen 3.3V Komponenten (Sensoren, MCP23017), während der **AP63205WU-7** (**U26**) den ESP32 mit 5V versorgt.

**Power-Architektur:**

* **ESP32**: Wird via **5V Pin** vom **AP63205 (5V)** versorgt.
* **Peripherie**: Wird via **AP63203 (3.3V)** versorgt (PCA9685, LEDs, Sensoren⚠️).
* **WICHTIG**:
  * Der 3.3V Pin des ESP32 bleibt **unbeschaltet** (NC).
  * **Ground (GND)** von 5V-Kreis und 3.3V-Kreis müssen verbunden sein!
  * I2C Pullups (R1, R2 = 4.7kΩ) müssen an die **3.3V (AP63203)** Rail angeschlossen werden.

**Komponenten-Werte (Schematic):**

* **U25 / U26**: Diodes Inc. AP63203WU-7 (3.3V) / AP63205WU-7 (5V)
* **C_IN (Eingang)**:
  * 1x **22µF** / 25V MLCC (1206, **C22 / C23**) - Direkt am VIN-Pin! (Review-Ergebnis)
  * 1x **10µF** / 25V MLCC (1210, **C16 / C21**) - Nahe an VIN/GND Pins.
  * 1x **100nF** MLCC (0805, **C2 / C14**) - HF-Entkopplung.
* **L1 / L2 (Spule)**:
  * **3.9µH** (L2, L3)
  * *Rating*: Sättigungsstrom (Isat) > 2.5A. DCR < 50mΩ.
* **C_OUT (Ausgang)**:
  * 2x **22µF** / 6.3V MLCC (0603, **C12, C13 / C18, C19**).
  * *Wichtig*: Keramik-Kondensatoren nutzen (Low ESR).
* **C_BOOT**:
  * 1x **100nF** MLCC (0805, **C17 / C11**) zwischen BST und SW Pin.

**Layout-Tipps:**

1. **Input-Cap**: Der 10µF Eingangskondensator muss so nah wie möglich an den PINs VIN und GND des ICs sitzen (kleinster Loop Area).
2. **SW Node**: Die Verbindung vom SW-Pin zur Spule kurz und breit halten (hohe HF-Emissionen).
3. **Power Path**: Breite Traces für VIN, GND und VOUT (mind. 20-30 mil, besser Flächen).
4. **Thermal**: GND-Pin gut an die Ground-Plane anbinden (Vias) zur Wärmeabfuhr.
