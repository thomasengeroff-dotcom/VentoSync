# 🛠️ Hardware Setup: Prototyp & PCB

Dieses Dokument spezifiziert die Hardware-Komponenten, die Verkabelung für den Prototypen (Breadboard) und die Design-Vorgaben für das finale PCB (EasyEDA).

> [!IMPORTANT]
> **Pin-Referenz**: Die Pin-Belegung basiert auf der direkten Ansteuerung der **Buttons via ESP32 GPIOs** und der **LEDs via PCA9685 I2C PWM Driver**. Das Front-Panel wird über einen 14-Pin FFC Connector angeschlossen.

---

## Zentrale Einheit & Power

* **MCU**: Seeed Studio XIAO ESP32C6
* **Netzteil (High Voltage)**: XP Power ECE10US12 oder TRACO POWER TMPS 10-112 (230V AC -> 12V DC, 10W)
  * *Hinweis*: Der XP Power ECE10US12 ist eine Alternative zum TRACO POWER TMPS 10-112. Grundriss und Pin-Belegung sind identisch.
* **Logic Power (5V/MCU Supply)**: **Diodes Inc. AP63205WU-7** (12V -> 5V, 2A)
  * *Versorgung*: Speist den XIAO ESP32C6 (via 5V Pin).
* **Peripheral Power (3.3V Buck)**: **Diodes Inc. AP63203WU-7** (12V -> 3.3V, 2A)
  * *Versorgung*: Speist alle Sensoren (BME680), PCA9685 PWM Driver und Front-Panel LEDs.
  * *Hinweis*: Der interne 3.3V LDO des ESP32 wird **nicht** genutzt (Pin open).
* **Sicherung**: 1A Slow Blow (Träge)
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
* **Front-Panel**: 9 LEDs (via PCA9685) und 3 Buttons (direkt an ESP32) über 14-Pin FFC Connector.
* **LED Driver**: **PCA9685PW** (I2C, 0x40) - 16-Kanal PWM Driver für dimmbare LEDs.
* **SCD41**: ⚠️ *NICHT IN BOM* - CO2 Sensor (I2C, 0x62) über 4-Pin Header H2. --> kann extern angeschlossen werden oder jeder andere I2C Sensor verwendet werden.
* **NTC-Sensorik**: 2x 10kΩ NTC (Beta 3435)
  * *Beschaltung*: 10kΩ 0.1% Referenzwiderstand + 100nF Filter + 3.3V Zener/TVS

## 1. Detaillierte BOM (Bill of Materials)

Status: **Final (10.02.2026)** - ✅ Verified

| Nr. | Menge | Wert / Bauteil | Designator | Footprint | Hersteller Part | Notiz |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| 1 | 1 | 470µF / 25V | C1 | Radial BD10 | EEUFR1E471B | Main 12V Buffer |
| 2 | 7 | 100nF / 50V | C2, C3, C4, C11, C14, C17, C20 | 0805 | CL21B104KBCNNNC | Bypass / HF / Input Caps |
| 3 | 1 | 470µF / 16V | C6 | Radial BD8 | ERS1CM471F12OT | 5V Output Buffer |
| 4 | 4 | 22µF / 6.3V | C12, C13, C18, C19 | 0603 | CL10A226MQ8NRNC | AP63203/AP63205 Output |
| 5 | 1 | **100µF / 25V** | **C15** | **SMD 7343** | TAJE107M025RNJ | Fan Filter (Tantal, Low ESR) ✅ |
| 6 | 2 | 10µF / 25V | C16, C21 | 1210 | CL32B106KAJNNNE | AP63203/AP63205 Input |
| 7 | 3 | Diode | D1, D2, D3 | SOD-323 | B5819WS | Schottky (Fan Protect) |
| 8 | 1 | Fuse Holder | FH1 | TH | 65800001009 | 5x20mm Fuse Holder |
| 9 | 1 | Header 4P | H2 | 1.27mm | PZ127V-11-04-0720 | SCD41 / I2C Ext |
| 10 | 1 | Header 3P | JP1 | 2.54mm | HX PH254... | Fan Voltage Select |
| 11 | 2 | Header 3P | JP2, JP3 | 1.27mm | 1271WV-3P | Fan Mode Select |
| 12 | 1 | 470µH | L1 | SMD | ASPI-0804T-471M-T | Fan Filter Inductor |
| 13 | 2 | 3.9µH | L2, L3 | SMD 4x4 | ANR4030T3R9M | Buck Inductors (3.3V & 5V) |
| 14 | 1 | PMOS | Q1 | SOT-23 | AO3401 | High-Side Switch |
| 15 | 1 | NPN | Q2 | SOT-23 | S8050 | High-Side Driver |
| 16 | 2 | NMOS | Q3, Q4 | SOT-23 | PMV16XNR | Low-Side Dual Switch |
| 17 | 2 | 4.7kΩ | R1, R2 | 0603 | 0603WAF4701T5E | I2C Pullups |
| 18 | 1 | 1kΩ | R3 | 0603 | RC0603FR-071KL | Base Resistor Q2 |
| 19 | 3 | 10kΩ | R4, R5, R6 | 0603 | 10kΩ Resistors | NTC Pullup, Pullups |
| 20 | 1 | 2.2kΩ | R7 | 0805 | 0805W8F2201T5E | Gate Pullup Q1 |
| 21 | 1 | 470Ω | R8 | 0603 | 0603WAF4700T5E | LED Resistor |
| 22 | 2 | 10kΩ | R9, R10 | 0402 | CRCW040210K0FKED | Gate Pulldown Q3/Q4 |
| 23 | 2 | 4x 470Ω | RN1, RN2 | 0603 Array | 4D03WGJ0471T5E | LED Series Resistors |
| 24 | 1 | 4x 10kΩ | RN3 | 0603 Array | 4D03WGJ0103T5E | Button Pullups |
| 25 | 2 | 47kΩ | R_B1, R_B2 | 0805 | 0805W8F4702T5E | Bleeder Resistors AC |
| 26 | 1 | Terminal | U1 | 5.08mm | DB128L-5.08-2P | AC Input |
| 27 | 1 | Power Module | U2 | Module | TMPS 10-112 | 230V->12V AC/DC |
| 28 | 10 | ESD Diode | U3-U5, U9-U14, U21 | SOT-23 | PESD5V0S2BT | Dataline Protection |
| 29 | 2 | Header 7P | U6, U7 | 2.54mm | B254N02... | XIAO Sockets |
| 30 | 1 | FPC Conn | U8 | SMD | FPC 0.5-14P LTH2.0 | Front Panel Connector |
| 31 | 1 | LED Driver | U15 | TSSOP-28 | PCA9685PW | 16-Ch PWM |
| 32 | 1 | 10kΩ | U16 | 0402 | FRC0402F1002TS | Resistor (Note: U-Desig??) |
| 33 | 2 | Terminal | U17, U18 | 2.54mm | KF128-2.54-2P | NTC Connectors |
| 34 | 2 | Terminal | U19, U20 | 2.54mm | KF128-2.54-4P | I2C/Fan Connectors |
| 35 | 1 | 3.3V Buck | **U25** | SOT-26 | **AP63203WU-7** | 12V->3.3V DC/DC |
| 36 | 1 | 5V Buck | **U26** | SOT-26 | **AP63205WU-7** | 12V->5V DC/DC |
| 37 | 1 | Varistor | V1 | TH | S10K275 equiv | AC Protection (220pF cap equiv) |

> [!TIP]
> **Check-Ergebnis**:
>
> 1. **Sicherheit**: Das kritische C11 Problem ist gelöst. **C15 (100µF / 25V)** ist absolut sicher für die 12V Lüfter-Spannung. (Upgrade auf Tantal C7230).
> 2. **Stabilität**: **C2 (100nF)** wurde parallel zu C16/C21 ergänzt. Das ist sehr gut für das HF-Verhalten des Buck-Converters.
> 3. **Hinweis zu C16/C21**: Du nutzt `CL32B106KAJNNNE` (1210, 10µF, 25V).
>     * Das ist elektrisch sicher und physikalisch robust (1210 Bauform).
>     * Da du **C2** (HF) und C1 (470µF Bulk) hast, ist das Design **valide**.
>
> **Freigabe: BOM ist valide.**

---

## 2. Pin-Mapping

### A. XIAO ESP32C6 (Master)

| XIAO Pin | GPIO | Funktion | Anschluss-Details |
| :--- | :--- | :--- | :--- |
| **D0** | GPIO0 | **NTC Innen** | Analog In (10k Spannungsteiler) |
| **D1** | GPIO1 | **NTC Außen** | Analog In (10k Spannungsteiler) |
| **D2** | GPIO2 | **Fan PWM Secondary** | Low-Side GND2 via Q4 (3-Pin Dual-GND Mode) |
| **D3** | GPIO21 | **PCA9685 OE** | Output Enable (Active Low, mit 10kΩ Pullup) |
| **D4** | GPIO22 | **I2C SDA** | PCA9685, BME680⚠️, SCD41⚠️ (mit 4.7kΩ Pullup) |
| **D5** | GPIO23 | **I2C SCL** | PCA9685, BME680⚠️, SCD41⚠️ (mit 4.7kΩ Pullup) |
| **D6** | GPIO16 | **Fan PWM** | An Basis S8050 (NPN) |
| **D7** | GPIO17 | **Fan Tacho** | Von Lüfter Pin 3 (mit Pullup & TVS) |
| **D8** | GPIO19 | **Button Power** | Front-Panel (mit 10kΩ Pullup) |
| **D9** | GPIO20 | **Button Mode** | Front-Panel (mit 10kΩ Pullup) |
| **D10** | GPIO18 | **Button Level** | Front-Panel (mit 10kΩ Pullup) |

### C. Universal-Lüfter Interface (3-Pin & 4-Pin Support)

Das Board unterstützt sowohl 4-Pin PWM (AxiRev) als auch 3-Pin Dual-GND (VarioPro) Lüfter. Die Umschaltung erfolgt über 3 Jumper-Blöcke.

**Detaillierte Schaltpläne und BOM**: Siehe [Anleitung-Fan-Circuit.md](Anleitung-Fan-Circuit.md)

**High-Side Circuit (4-Pin Mode):**

1. **Level Shifter Q3** (NPN **S8050**): GPIO16 → R3 (1kΩ) → Q3 Basis. Q3 Emitter → GND.
2. **PMOS Q1** (AO3401): Q3 Collector → Q1 Gate + **R8 (2.2kΩ)** Pullup auf 12V. Q1 Source → 12V, Drain → PWM_12V_OUT.
3. **D1 (B5819WS)**: Freilaufdiode Kathode → PWM_12V_OUT, Anode → GND.
4. **LC-Filter**: PWM_12V_OUT → L1 (470µH) → DC_VAR_12V → **C15 (100µF)** → GND.

**Dual Low-Side Circuit (3-Pin Dual-GND Mode):**

1. **Q4** (NMOS **PMV16XNR** SOT-23): Fan GND1 → Drain, Source → GND. Gate ← GPIO16 + R19 (10kΩ Pull-down).
2. **Q3** (NMOS **PMV16XNR** SOT-23): Fan GND2 → Drain, Source → GND. Gate ← GPIO2 + R18 (10kΩ Pull-down).
3. **D2 (B5819WS)**: Schutzdiode Kathode → Fan Pin 1, Anode → GND.
4. **D3 (B5819WS)**: Schutzdiode Kathode → Fan Pin 4, Anode → GND.

**Jumper-Konfiguration:**

| Jumper | Position 1-2 (4-Pin) | Position 2-3 (3-Pin) |
| :--- | :--- | :--- |
| **JP1** | Fan VCC ← Constant 12V | Fan VCC ← Variable (DC_VAR_12V) |
| **JP2** | Fan Pin 1 ← GND | Fan Pin 1 ← Q4 Drain (GND1) |
| **JP3** | Fan Pin 4 ← GPIO16 (PWM) | Fan Pin 4 ← Q3 Drain (GND2) |

> ⚠️ **Tacho-Signal & Geräusche (3-Pin Mode)**:
>
> 1. **RPM-Schwankung**: Betrifft nur das *Auslesen* der Drehzahl (Tacho). Für die Regelung unkritisch.
> 2. **Spulenfiepen**: Der LC-Filter (L1 + **C15**) glättet das PWM-Signal zu einer sauberen Gleichspannung (Buck-Converter Prinzip) → lautloser Betrieb.

### B. PCA9685 PWM Driver (Adresse 0x40)

Der PCA9685 steuert die 9 LEDs des Front-Panels über PWM (dimmbar).

| PCA9685 Kanal | Funktion | Panel Komponente | Widerstand |
| :--- | :--- | :--- | :--- |
| **Channel 0-5** | *NC* | *Not Connected* | - |
| **Channel 6** | PWM OUTPUT | LED Power (On/Off) | 470Ω |
| **Channel 7** | PWM OUTPUT | LED Master (Status) | 470Ω |
| **Channel 8** | PWM OUTPUT | LED Intensität 1 | 470Ω |
| **Channel 9** | PWM OUTPUT | LED Intensität 2 | 470Ω |
| **Channel 10** | PWM OUTPUT | LED Intensität 3 | 470Ω |
| **Channel 11** | PWM OUTPUT | LED Intensität 4 | 470Ω |
| **Channel 12** | PWM OUTPUT | LED Intensität 5 | 470Ω |
| **Channel 13** | PWM OUTPUT | LED Modus: WRG | 470Ω |
| **Channel 14** | *NC* | *Not Connected* | - |
| **Channel 15** | PWM OUTPUT | LED Modus: Durchlüften | 470Ω |

### C. Front-Panel Buttons (Direkt an ESP32)

| ESP32 Pin | GPIO | Funktion | Beschaltung |
| :--- | :--- | :--- | :--- |
| **D8** | GPIO19 | Button Power | 10kΩ Pullup (RN3), Active Low |
| **D9** | GPIO20 | Button Mode | 10kΩ Pullup (RN3), Active Low |
| **D10** | GPIO18 | Button Level | 10kΩ Pullup (RN3), Active Low |

---

## 3. Verkabelung & Bedienpanel (FFC 14-Pin)

Das Front-Panel wird über ein 14-Pin Flachbandkabel (0.5mm Pitch) angeschlossen.

**Anschluss-Logik**:

* **LEDs**: Angesteuert via PCA9685 PWM Outputs (Channels 6-15) mit 470Ω/10kΩ Vorwiderständen.
* **Buttons**: Direkt an ESP32 GPIOs (D8, D9, D10) mit 10kΩ Pullups, schalten gegen GND.
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
  * 1x **10µF** / 25V MLCC (1210, **C16 / C21**) - Nahe an VIN/GND Pins!
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
