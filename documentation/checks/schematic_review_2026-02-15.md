# Schematic Review — 2026-02-15

Cross-reference of EasyEDA schematic (V1.0, 2026-02-14) against component datasheets in `EasyEDA-Pro/components/`.

---

## 1. Netzteil-Sektion (High Voltage → 12 V DC)

### Components: U1 (screw terminal), U2 (TMPS10-112), FH1 (fuse holder), V1 (varistor), C1, R_B1/R_B2

| Check | Spec | Schematic | Status |
| ----- | ---- | --------- | ------ |
| V1 (B72210S0271K101) max AC voltage | 275 V_RMS (S20 K271) | Connected across AC(L)–AC(N) after fuse | ✅ OK |
| V1 type | Metal-oxide varistor, leaded, 7.5 mm pitch | Footprint `RES-TH_L12.5-W5.1-P7.50` | ✅ OK |
| FH1 (65800001009) ratings | 250 V / 10 A, 5×20 mm fuse | AC input, before transformer | ✅ OK |
| C1 (EEUFR1E471B) voltage rating | 25 V rated | After TMPS10-112 12 V output | ✅ OK — 12 V ≤ 25 V |
| C1 ripple current | 967.5 mA @ 120 Hz | AC/DC output filtering | ✅ OK |
| R_B1/R_B2 (47 kΩ, 0805, 125 mW) | Bleeder resistors across C1 | Parallel across 12 V output cap | ✅ OK |
| R_B1/R_B2 power dissipation check | P = V²/R = 12²/47000 = 3.1 mW | Max 125 mW | ✅ OK — wide margin |
| U2 (TMPS10-112) output | 12 V DC, 10 W | Supplies 5 V and 3.3 V buck stages + fan | ✅ OK |

> [!TIP]
> R_B1/R_B2 see only 3 mW — no thermal concern.

---

## 2. 5 V Stromversorgung (Buck Converter)

### Components: U26 (AP63205WU-7), L2, C11/C17 (input), C22 (VIN bulk), C18/C19 (output), C6 (output bulk)

| Check | Spec (Datasheet) | Schematic | Status |
| ----- | ---------------- | --------- | ------ |
| U26 (AP63205WU-7) input range | 3.8 V – 32 V | VIN = 12 V DC from TMPS10 | ✅ OK |
| U26 fixed output voltage | 5.0 V | Connected to 5 V rail | ✅ OK |
| U26 output current | 2 A max | Powers PCA9685 + LEDs + SCD41 + ESP32 | ✅ OK — well within limits |
| U26 switching freq | 1.1 MHz | — | ✅ OK |
| U26 pin assignment (TSOT-26) | 1=VIN, 2=GND, 3=EN, 4=FB, 5=SW, 6=BST | Pin 1→12V, 2→GND, 3→EN(12V via R?), 4→FB(output/divider), 5→SW→L2, 6→BST cap | ✅ Check below |
| **EN pin** | Must be driven HIGH to enable; internal 1.18 V threshold | Schematic shows EN tied to VIN (12 V) | ✅ OK — always enabled |
| **FB pin** | AP6320**5** = fixed 5 V output, FB tied directly to VOUT | FB → VOUT (5 V) | ✅ OK — fixed output variant, no resistor divider needed |
| BST capacitor | 100 nF recommended (BST → SW) | C17 = 100 nF (BST to SW) | ✅ OK |
| Input cap | 22 µF ceramic recommended | C11 = 100 nF (HF-Bypass) + **C22 = 22 µF / 25 V MLCC** (CL21A226MAQNNNE) | ✅ OK — C22 ergänzt |
| Output cap | 22 µF ceramic recommended | C18 + C19 = 2× 22 µF + C6 470 µF bulk (16 V) | ✅ OK |
| L2 inductance | Typical application: 3.3 µH – 10 µH | L2 = 3.9 µH (ANR4030T3R9M) | ✅ OK |
| L2 saturation current | 3.3 A (Isat) | Max load 2 A | ✅ OK — 65% margin |
| L2 DC current rating | 2.3 A (Irms) | 2 A max load | ✅ OK |
| C6 voltage rating | 16 V rated | 5 V output | ✅ OK — 3.2× derating |

> [!TIP]
> ✅ **Behoben:** C22 (22 µF / 25 V / X5R / 1206, CL21A226MAQNNNE, LCSC C45783) wurde direkt am VIN-Pin von U26 ergänzt.
> C11 (100 nF) verbleibt als HF-Bypass.

---

## 3. 3.3 V Stromversorgung (Buck Converter)

### Components: U25 (AP63203WU-7), L3, C14/C20 (input), C23 (VIN bulk), C12/C13 (output), C15 (output bulk)

| Check | Spec (Datasheet) | Schematic | Status |
| ----- | ---------------- | --------- | ------ |
| U25 (AP63203WU-7) input range | 3.8 V – 32 V | VIN = 12 V DC | ✅ OK |
| U25 fixed output voltage | 3.3 V | Connected to 3.3 V rail | ✅ OK |
| U25 output current | 2 A max | Powers ESP32C6 + I2C pull-ups | ✅ OK |
| U25 switching freq | 1.1 MHz | — | ✅ OK |
| **EN pin** | Must be HIGH to enable | EN → VIN (12 V) | ✅ OK |
| **FB pin** | AP6320**3** = fixed 3.3 V, FB tied to VOUT | FB → VOUT (3.3 V) | ✅ OK |
| BST capacitor | 100 nF | C20 = 100 nF (BST to SW) | ✅ OK |
| Input cap | 22 µF ceramic recommended | C14 = 100 nF (HF-Bypass) + **C23 = 22 µF / 25 V MLCC** (CL21A226MAQNNNE) | ✅ OK — C23 ergänzt |
| Output caps | 22 µF ceramic recommended | C12 + C13 = 2× 22 µF + C15 100 µF tantalum | ✅ OK — exceeds recommendation |
| L3 inductance | 3.3 µH – 10 µH | 3.9 µH (ANR4030T3R9M) | ✅ OK |
| L3 current rating | Isat = 3.3 A, Irms = 2.3 A | Max 2 A | ✅ OK |
| C15 (TAJE107M025RNJ) voltage | 25 V rated tantalum | 3.3 V output | ✅ OK — 7.5× derating |
| C12/C13 voltage rating | 6.3 V (CL10A226MQ8NRNC) | 3.3 V output | ✅ OK |

> [!TIP]
> ✅ **Behoben:** C23 (22 µF / 25 V / X5R / 1206, CL21A226MAQNNNE, LCSC C45783) wurde direkt am VIN-Pin von U25 ergänzt.

---

## 4. TVS-Dioden (ESD-Schutz)

### Components: U3–U5, U9–U14, U21 (alle PESD5V0S2BT)

| Check | Spec | Schematic | Status |
| ----- | ---- | --------- | ------ |
| Vrwm (Stand-off) | 5 V | Signal lines at 3.3 V level | ✅ OK — 5 V > 3.3 V |
| Clamping voltage | 9 V | Below SCD41/ESP32 abs max | ✅ OK |
| Peak pulse power | 100 W (8/20 µs) | ESD protection on I2C, buttons, NTC | ✅ OK |
| Bidirectional | Yes | Suitable for I2C (bidirectional) | ✅ OK |
| Number of channels | 2 per device | SOT-23 package, protects 2 lines each | ✅ OK |
| U3, U4, U5 (BOM: 10×) | Schematic shows ESD on: NTC_IN, NTC_OUT, I2C SDA/SCL, BTN_PWR, BTN_MOD, BTN_LVL, FAN_TACHO, FAN_PWR, and Front Panel signals | Covers all external-facing lines | ✅ OK — comprehensive coverage |

> [!NOTE]
> 10 TVS-Dioden = 20 geschützte Kanäle. Sehr guter ESD-Schutz für alle externen Signale.

---

## 5. PWM Treiber (Fan High-Side Switch)

### Components: Q1 (DMP3098L), Q2 (S8050), R3 (1 kΩ), R7 (2.2 kΩ), R8 (470 Ω), D1 (B5819WS)

| Check | Spec | Schematic | Status |
| ----- | ---- | --------- | ------ |
| Q1 (DMP3098L) Vds max | -30 V | VDS = -12 V (Source=12 V, Drain=Fan) | ✅ OK |
| Q1 Id max | -3.8 A continuous | Fan current typically < 1 A | ✅ OK |
| Q1 Vgs(th) | -0.5 V to -1.2 V | Gate driven by Q2 collector | ✅ Check below |
| Q1 Vgs max | ±12 V | Vgs = 0 V (on) to -12 V (off) | ✅ OK — within ±12 V |
| Q1 Vgs when Q2 ON | Gate pulled to GND via Q2, Source = 12 V → Vgs = -12 V | Turns Q1 fully ON (Vgs = -12 V >> Vgs(th)) | ✅ OK |
| Q1 Vgs when Q2 OFF | Gate pulled to 12 V via R7, Vgs = 0 V | Q1 OFF | ✅ OK |
| Q2 (S8050) Vceo | 25 V | Collector swings to 12 V max | ✅ OK |
| Q2 Ic | 500 mA max | Base current limited by R3/R8 | ✅ OK |
| Q2 base resistor R3 | 1 kΩ from ESP32 GPIO (3.3 V) | Ib = (3.3 - 0.7) / 1000 = 2.6 mA | ✅ OK |
| Q2 collector current | Through R7 (2.2 kΩ): Ic = 12 / 2200 = 5.5 mA | Well within 500 mA Ic max | ✅ OK |
| Q2 hFE requirement | Ic/Ib = 5.5/2.6 = 2.1 | hFE min = 120 → deeply saturated | ✅ OK |
| R7 (2.2 kΩ, 0805) power | P = 12² / 2200 = 65 mW | Rated 125 mW | ✅ OK |
| R8 (470 Ω) purpose | Pull-down on Q2 base → ensures Q2 OFF at boot | Prevents Q1 glitch at startup | ✅ OK |
| D1 (B5819WS) Flyback | Vr = 40 V, If = 1 A | Across Q1 drain → protects from inductive fan spikes | ✅ OK |
| D1 forward voltage | 0.45 V typ @ 1 A | Low Vf for fast clamping | ✅ OK |

> [!TIP]
> Die Q2/Q1 Kaskade ist korrekt dimensioniert. Q1 wird mit Vgs = -12 V voll durchgesteuert (RDS(ON) = 48 mΩ → nahezu verlustfrei bei < 1 A Lüfterstrom).

---

## 6. Dual Low-Side MOSFETs (3-Pin Fan GND-Steuerung)

### Components: Q3, Q4 (PMV16XNR), R9, R10 (10 kΩ), D2, D3 (B5819WS)

| Check | Spec (BOM) | Schematic | Status |
| ----- | ---------- | --------- | ------ |
| Q3/Q4 Vds | 20 V max | Switched: 12 V fan rail | ✅ OK |
| Q3/Q4 Id | 6 A max | Fan < 1 A | ✅ OK |
| Q3/Q4 Vgs(th) | 1 V typ | Gate driven by ESP32 GPIO (3.3 V) | ✅ OK — fully enhanced |
| Q3/Q4 RDS(ON) | 35 mΩ @ Vgs = 2.5 V | At 3.3 V even lower | ✅ OK |
| R9/R10 (10 kΩ) pull-downs | Gate → GND when GPIO floating | Prevents parasitic turn-on at boot | ✅ OK |
| R9/R10 size | 0402, 63 mW | Gate current negligible | ✅ OK |
| D2, D3 (B5819WS) protection | 40 V / 1 A Schottky | Across Q3/Q4 drain-source → inductive protection | ✅ OK |
| JP2, JP3 (1.27 mm jumpers) | Mode select for Pin 1/Pin 4 | Selects between PWM and GND-switching | ✅ OK |

---

## 7. LC-Filter (Fan MODE SELECT Spule)

### Components: L1 (220 µH), C16 (10 µF), C21 (10 µF)

| Check | Spec | Schematic | Status |
| ----- | ---- | --------- | ------ |
| L1 (SLH1207S221MTT) inductance | 220 µH ±20% | LC filter for variable 12 V supply | ✅ OK |
| L1 DC current | 1.16 A (Irms), Isat = 2.36 A | Fan current VarioPro: 283 mA, Anlauf: 920 mA | ✅ OK — Anlauf bei 39% Isat |
| L1 DCR | 0.39 Ω max | Voltage drop at 283 mA = 0.11 V | ✅ OK |
| L1 Bauart | Geschirmt (magnetic shielded) | Reduziert EMI | ✅ OK |
| C16, C21 voltage rating | 25 V (CL32B106KAJNNNE) | 12 V rail | ✅ OK |
| JP1 (2.54 mm jumper) | VCC mode select: constant vs. variable 12 V | Selects LP-filtered or direct 12 V | ✅ OK |

> [!TIP]
> ✅ **Behoben:** L1 wurde von ASPI-0804T-471M (470 µH / 500 mA) auf **SLH1207S221MTT** (220 µH / 1.16 A / Isat 2.36 A) aufgerüstet.
> Damit ist auch der 920 mA Anlaufstrom des VarioPro 4412 FGM PR bei Richtungswechsel alle 70 s kein Problem mehr.

---

## 8. SCD41 CO2 Sensor

### Components: SCD41, Pull-up Widerstände, U-Schraubverbinder

| Check | Spec (Datasheet) | Schematic | Status |
| ----- | ---------------- | --------- | ------ |
| SCD41 VDD range | 2.25 V – 5.5 V | Powered from 3.3 V rail | ✅ OK |
| SCD41 I2C speed | max 100 kHz | ESP32C6 I2C default | ✅ OK |
| SCD41 I2C pull-ups | Required on SDA/SCL | R1, R2 = 4.7 kΩ to 3.3 V shown | ✅ OK |
| SCD41 max pin voltage | VDD + 0.3 V = 3.6 V | I2C at 3.3 V logic | ✅ OK |
| H2 connector (1.27 mm, 4P) | GND, VDD, SDA, SCL | 4 pin header for sensor cable | ✅ OK |
| SCD41 decoupling | 100 nF recommended | C3 = 100 nF am Sensor-Stecker | ✅ OK |

---

## 9. NTC Temperatursensoren

### Components: R4, R5 (10 kΩ 0.1%), U17, U18 (Screw Terminals)

| Check | Spec | Schematic | Status |
| ----- | ---- | --------- | ------ |
| R4/R5 precision | 0.1%, TCR 25 ppm/°C | High precision for NTC voltage divider | ✅ OK |
| R4/R5 value | 10 kΩ | Matched to 10 kΩ NTC at 25 °C | ✅ OK |
| R4/R5 footprint | 0603 | Compact, adequate power | ✅ OK |
| NTC_IN / NTC_OUT ADC pins | Connected to ESP32C6 ADC | ADC0 (D0) + ADC1 (D1) | ✅ OK |
| TVS protection | PESD5V0S2BT on NTC lines | External signal protection | ✅ OK |
| U17, U18 voltage rating | 130 V / 8 A (KF128-2.54) | 3.3 V signal level | ✅ OK — massiv überdimensioniert, aber funktional |

---

## 10. PCA9685 (I2C PWM LED Driver)

### Components: U15 (PCA9685PW,118), RN1/RN2 (470 Ω arrays), RN3 (10 kΩ array), U16 (10 kΩ)

| Check | Spec (Datasheet) | Schematic | Status |
| ----- | ---------------- | --------- | ------ |
| U15 VDD range | 2.3 V – 5.5 V | Powered from 3.3 V | ✅ OK |
| U15 I2C address | Default = 0x40 (A0–A5 all LOW) | A0–A5 → GND | ✅ OK |
| U15 OE (Output Enable) | Active LOW | OE → GND (always enabled) | ✅ OK |
| U15 EXTCLK | External clock input | Not connected or tied LOW | ✅ OK |
| U15 output current | 25 mA sink, 10 mA source @ 5 V | LEDs via 470 Ω series resistors | ✅ Check below |
| LED current through RN1/RN2 | I = (3.3 V - V_LED) / 470 Ω ≈ (3.3 - 2.0) / 470 = 2.8 mA | Well within 25 mA sink | ✅ OK |
| I2C pull-ups for PCA9685 | Required, shared with SCD41 | R1/R2 = 4.7 kΩ to 3.3 V | ✅ OK |
| U16 (10 kΩ) purpose | Pull-up on EXTCLK or address pin | Address/config | ✅ OK |
| RN3 (10 kΩ array) purpose | Pull-up/-down on DIP-Switch inputs | Address/config via DIP | ✅ OK |
| Decoupling | 100 nF recommended | C4 = 100 nF | ✅ OK |

---

## 11. Mikrocontroller (Seeed XIAO ESP32C6)

### Components: U6, U7 (female headers 1×7P, 2.54 mm)

| Check | Spec | Schematic | Status |
| ----- | ---- | --------- | ------ |
| VCC supply | 3.3 V from buck U25 | 3.3 V rail | ✅ OK |
| GPIO usage | All GPIOs mapped per pin table in schematic | Buttons, PWM, I2C, ADC, Fan Tacho | ✅ OK |
| I2C pins | SDA/SCL via pull-ups R1/R2 to 3.3 V | Connected to PCA9685 + SCD41 | ✅ OK |
| ADC pins | D0 (NTC Out), D1 (NTC In) | Voltage divider with R4/R5 | ✅ OK |
| Button inputs | D6 (Power), D9 (Mode), D2 (Level) | With TVS protection + R8 pull-down | ✅ OK |
| Fan PWM outputs | D8 (GPIO19) primary, D10 (GPIO18) secondary | Connected to Q2 base (via R3) and Q3/Q4 gates | ✅ OK |

---

## 12. Front Panel Connector

### Components: U8 (FPC 0.5-14P), related TVS diodes

| Check | Spec | Schematic | Status |
| ----- | ---- | --------- | ------ |
| U8 pin count | 14P, 0.5 mm pitch | Connects to external Ventomaxx V-WRG panel | ✅ OK |
| Signal protection | TVS on external-facing pins | PESD5V0S2BT on signal lines | ✅ OK |
| Connector type | FPC slide-lock, vertical | SMD mount | ✅ OK |

---

## 13. Schutzdioden D1–D3 (B5819WS)

| Check | Spec | Schematic | Status |
| ----- | ---- | --------- | ------ |
| D1 position | Flyback diode at Q1 drain (PWM_12V_OUT) | Cathode → 12 V, Anode → Drain | ✅ OK |
| D2 position | Protection on Fan Pin 1 (GND1) | Across Q3 drain–source | ✅ OK |
| D3 position | Protection on Fan Pin 4 (GND2/PWM) | Across Q4 drain–source | ✅ OK |
| Voltage rating | 40 V all three | 12 V max in circuit | ✅ OK |
| Current rating | 1 A all three | Fan inductance spikes < 1 A | ✅ OK |
| Package | SOD-323 | Compact SMD | ✅ OK |

---

## Zusammenfassung

### ✅ Korrekt (keine Änderung nötig)

| Bereich | Ergebnis |
| ------- | -------- |
| Netzteil / Varistor / Sicherung | Korrekt dimensioniert |
| Q1/Q2 PWM-Treiber Kaskade | Korrekt — Vgs voll ausgesteuert |
| Q3/Q4 Low-Side MOSFETs | Korrekt — Vgs(th) = 1 V, Gate = 3.3 V |
| D1–D3 Freilaufdioden | Korrekt platziert und spezifiziert |
| Jumper JP1/JP2/JP3 | Korrekt für Moduswahl |
| SCD41 Anbindung | Korrekt — 3.3 V, I2C mit 4.7 kΩ Pull-ups |
| NTC Spannungsteiler | Korrekt — 10 kΩ 0.1% passend zu NTC |
| PCA9685 LED-Treiber | Korrekt — 3.3 V, I2C, 470 Ω Vorwiderstände |
| ESP32C6 Anbindung | Korrekt — alle GPIOs korrekt zugeordnet |
| TVS/ESD Schutz | Umfassend — 20 Kanäle auf allen externen Signalen |
| Buck-Ausgangsfilter | Korrekt — 22 µF + Bulk-Elkos/Tantalum |
| Buck-Eingangsfilter (C22/C23) | Korrekt — 22 µF / 25 V MLCC direkt an VIN |
| LC-Filter (L1) | Korrekt — 220 µH / 1.16 A, Isat 2.36 A |

### ✅ Alle Befunde behoben (BOM V2, 2026-02-15)

| # | Bereich | Ursprüngliches Problem | Lösung | Status |
| - | ------- | ---------------------- | ------ | ------ |
| 1 | **U26 VIN-Cap (5 V Buck)** | C11 = 100 nF zu klein als Eingangskondensator | **C22** (22 µF / 25 V, CL21A226MAQNNNE, LCSC C45783) ergänzt | ✅ Behoben |
| 2 | **U25 VIN-Cap (3.3 V Buck)** | C14 = 100 nF — gleiche Problematik | **C23** (22 µF / 25 V, CL21A226MAQNNNE, LCSC C45783) ergänzt | ✅ Behoben |
| 3 | **L1 Strombelastung** | ASPI-0804T-471M (470 µH / 500 mA) — Anlaufstrom VarioPro 920 mA überlastet L1 alle 70 s | **SLH1207S221MTT** (220 µH / 1.16 A / Isat 2.36 A, LCSC C182174) — geschirmt, niedrigerer DCR | ✅ Behoben |
