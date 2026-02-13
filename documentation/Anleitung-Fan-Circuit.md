# 🔌 Anleitung: Hybrid Fan Interface (4-Pin PWM + 3-Pin Dual-GND)

Diese Anleitung beschreibt die Schaltung für eine **universelle Lüftersteuerung**, die sowohl moderne 4-Pin PWM-Lüfter als auch spezielle 3-Pin Dual-GND Lüfter (ebm-papst VarioPro 4412 FGM PR) unterstützt.

## 🛠️ Funktionsprinzip

Die Schaltung kombiniert zwei Steuerungsmethoden:

### A. 4-Pin PWM Modus (Standard PC-Lüfter)

- **High-Side Switch** schaltet +12V via P-MOSFET
- Lüfter bekommt dauerhaft 12V an VCC (Jumper 1-2)
- Geschwindigkeitsregelung via PWM-Signal an Pin 4 (GPIO16)
- Lüfter regelt sich selbst

### B. 3-Pin Dual-GND Modus (VarioPro)

Die Steuerung der Lüfter erfolgt über die GND-Leitungen. Dies wird im folgenden Blog detailliert beschrieben: <https://www.mikrocontroller.net/topic/412458>

- **Dual Low-Side MOSFETs** schalten GND1 und GND2 getrennt
- Lüfter bekommt variable Spannung (Jumper 2-3)
- Richtungssteuerung durch Umschalten der aktiven GND-Leitung:
  - **GND1 aktiv** (GPIO16 PWM) = Richtung A (z.B. Zuluft)
  - **GND2 aktiv** (GPIO2 PWM) = Richtung B (z.B. Abluft)
- Spannungsbereich: 7V-12V (58%-100% Duty Cycle)
- **PWM-Frequenz**: 28kHz (geräuschloser Betrieb)

---

## 1. Benötigte Komponenten (BOM)

### Basis-Komponenten (für beide Modi)

| Bauteil | Symbol | Wert/Typ | Bauform | Funktion |
| :--- | :--- | :--- | :--- | :--- |
| **Q1** | PMOS | **AO3401** (oder AO3401A) | SOT-23 | High-Side Switch für 4-Pin Modus (Max 3-4A). |
| **Q3** | NPN | **S8050** (oder 2N2222, BC847) | SOT-23 | Pegelwandler 3.3V → 12V Gate für Q1. |
| **D1** | Diode | **B5819WS** (Schottky) | SOD-323 | Freilaufdiode (Schutz gegen Induktion). |
| **R3** | Resistor | **1kΩ** | 0603 | Basis-Widerstand für Q3. |
| **R8** | Resistor | **2.2kΩ** | 0805 | Pullup für Gate Q1 (hält MOSFET OFF). |
| **JP1** | Header | **1x3 Pin Header** (2.54mm) | THT | Jumper zur Moduswahl. |

### Zusätzliche Komponenten für 3-Pin Dual-GND Modus

| Bauteil | Symbol | Wert/Typ | Bauform | Funktion |
| :--- | :--- | :--- | :--- | :--- |
| **Q4** | NMOS | **PMV16XNR** (TECH PUBLIC) | SOT-23 | Low-Side Switch für GND2 (Max 6.8A, LCSC C28646372). |
| **Q5** | NMOS | **PMV16XNR** (TECH PUBLIC) | SOT-23 | Low-Side Switch für GND1 (Max 6.8A, LCSC C28646372). |
| **D2** | Diode | **B5819WS** (Schottky) | SOD-323 | Schutzdiode für Fan Pin 1 (GND/GND1). |
| **D3** | Diode | **B5819WS** (Schottky) | SOD-323 | Freilaufdiode für Pin 4 (PWM/GND2). |
| **R18** | Resistor | **10kΩ** | 0402 | Pull-down für Gate Q4 (LCSC C71617). |
| **R19** | Resistor | **10kΩ** | 0402 | Pull-down für Gate Q5 (LCSC C71617). |

### Optional (LC-Filter für 3-Pin Glättung)

| Bauteil | Symbol | Wert/Typ | Bauform | Funktion |
| :--- | :--- | :--- | :--- | :--- |
| **L1** | Inductor | **470µH** (>500mA) | SMD/THT | LC-Filter Spule (Glättung für 3-Pin). |
| **C27** | Capacitor | **100µF** / KEMET Tantal | SMD 7343 | LC-Filter Kondensator (LCSC C7230). |

---

## 2. Schaltplan (Schematic)

### A. High-Side Circuit (für 4-Pin PWM Modus)

```
                    +12V
                     │
                     ├──── R8 (2.2k) ─┐
                     │                │
                     │            Gate Q1 (PMOS AO3401)
                     │                │
                     │                │
FAN_PWM ──── R3 (1k) ─┴─ Base Q3 (NPN S8050)
                         │
                    Emitter Q3
                         │
                        GND

Source Q1 ──── +12V
Drain Q1  ──── PWM_12V_OUT ──── D1 (B5819WS Kathode)
                                 │
                                GND (D1 Anode)

PWM_12V_OUT ──── L1 (470µH) ──── DC_VAR_12V ──── C27 (100µF) ─── GND
```

### B. Dual Low-Side Circuit (für 3-Pin Dual-GND Modus)

```
Fan +12V ──── Constant +12V Supply

Fan GND1 ──── Drain Q5 (NMOS PMV16XNR SOT-23)
              │
          Source Q5 ──── GND
              │
          D2 (B5819WS): Kathode → Fan GND1, Anode → GND

GPIO16 ───────┬──── Gate Q5
              │
          R19 (10k)
              │
             GND

Fan GND2 ──── Drain Q4 (NMOS PMV16XNR SOT-23)
              │
          Source Q4 ──── GND
              │
          D3 (B5819WS): Kathode → Fan GND2, Anode → GND

GPIO2 ────────┬──── Gate Q4
              │
          R18 (10k)
              │
             GND
```

### C. Jumper Konfiguration (JP1)

```
Pin 1 (Oben)   ──── +12V (Constant)
Pin 2 (Mitte)  ──── Fan VCC
Pin 3 (Unten)  ──── DC_VAR_12V (PWM Output)
```

### D. Mode Select Jumper Block (JP2 & JP3)

Um Pin 1 und Pin 4 korrekt zwischen den Modi umzuschalten, sind zwei weitere Jumper (oder ein 2x3 Block) nötig:

**JP2 (Pin 1 Select):**

- **1-2 (4-Pin Mode):** Fan Pin 1 <--> GND
- **2-3 (3-Pin Mode):** Fan Pin 1 <--> Drain Q5 (GND1)

**JP3 (Pin 4 Select):**

- **1-2 (4-Pin Mode):** Fan Pin 4 <--> GPIO16 (PWM Signal)
- **2-3 (3-Pin Mode):** Fan Pin 4 <--> Drain Q4 (GND2)

**Jumper-Positionen:**

- **1-2**: 4-Pin PWM Modus (Constant 12V to fan)
- **2-3**: 3-Pin Dual-GND Modus (Variable voltage to fan)

### E. Lüfterstecker (4-Pin Fan Header)

| Pin | Name | 4-Pin Modus (Jumper auf 1-2) | 3-Pin Dual-GND Modus (Jumper auf 2-3) |
| :--- | :--- | :--- | :--- |
| **1** | GND/GND1 | **GND** (via JP2 1-2) | **Fan GND1** (via JP2 2-3 to Q5) |
| **2** | VCC | JP1 Pin 2 (12V) | JP1 Pin 2 (Variable) |
| **3** | Tacho | GPIO17 (Pullup) | GPIO17 (Pullup) |
| **4** | PWM/GND2 | **PWM** (via JP3 1-2 from GPIO16) | **Fan GND2** (via JP3 2-3 to Q4) |

> ⚠️ **Wichtig**: Für korrekte Funktion müssen **ALLE Jumper (JP1, JP2, JP3)** passend gesetzt werden!

---

## 3. Funktionsweise der Modi

### Modus 1: 4-Pin PWM (Jumper 1-2) - ebm-papst AxiRev

1. **Hardware**:
   - JP1 auf Position 1-2 → Fan VCC bekommt konstant 12V
   - GPIO16 liefert 28kHz PWM direkt an Fan Pin 4
   - Q4, Q5 (Low-Side MOSFETs) sind **nicht aktiv**

2. **Software**:
   - "Fan Control Mode" = "4-Pin PWM"
   - `fan_pwm_primary` (GPIO16) steuert Geschwindigkeit **und Richtung**
   - `fan_pwm_secondary` (GPIO2) bleibt auf 0%

3. **Lüfterverhalten (AxiRev Spezifikation)**:
   - **Richtungssteuerung via PWM Duty Cycle**:
     - **0-49% Duty**: Richtung A (z.B. Zuluft) - Geschwindigkeit steigt mit PWM
     - **51-100% Duty**: Richtung B (z.B. Abluft) - Geschwindigkeit steigt mit PWM
     - **50% Duty**: Stillstand (Neutral)
   - **Besonderheit**: Volle Reversierbarkeit bei fast identischer Effizienz in beide Richtungen
   - **Automatisches Alternieren**: Alle 60-70 Sekunden (konfigurierbar)

### Modus 2: 3-Pin Dual-GND (Jumper 2-3)

1. **Hardware**:
   - JP1 auf Position 2-3 → Fan VCC bekommt variable Spannung
   - Fan +12V an konstant 12V
   - Fan GND1 an Q5 Drain (GPIO16 PWM)
   - Fan GND2 an Q4 Drain (GPIO2 PWM)

2. **Software**:
   - "Fan Control Mode" = "3-Pin Dual-GND"
   - **Richtung A**: GPIO16 = PWM (58%-100%), GPIO2 = 0%
   - **Richtung B**: GPIO16 = 0%, GPIO2 = PWM (58%-100%)
   - **Safety Interlock**: 10ms Delay beim Umschalten

3. **Lüfterverhalten**:
   - Geschwindigkeit via Spannung (7V-12V)
   - Richtungsumkehr durch GND-Umschaltung
   - Automatisches Alternieren alle 70 Sekunden (konfigurierbar)

---

## 4. PCB Layout Guide

### Leiterbahnbreiten

1. **Hochstrom-Pfade** (12V, GND1, GND2):
   - Mindestens **0.8mm** (für 3A Motorstrom)
   - Besser **1.0mm** für Sicherheit

2. **Signal-Pfade** (GPIO16, GPIO2, Tacho):
   - **0.3mm** ausreichend

### MOSFET Kühlung

- **AO3401 (SOT-23)**: Kleine Kupferfläche an Drain/Source als Kühlkörper
- **PMV16XNR (SOT-23) - Q4, Q5**: Sehr niedriger RDS(on) (~20mΩ bei 4.5V Gate)
  - Bei 3A Last: P = I² × R = 9 × 0.020 = **0.18W**
  - SOT-23 kann bis ~0.5W ohne Kühlung
  - Empfehlung: Große Kupferfläche an Drain (Pin 3) für bessere Wärmeableitung
  - Optional: Via-Stitching unter dem MOSFET zur Bodenplatte

### Dioden Platzierung

- **D1 (B5819WS)**: Freilaufdiode an **PWM_12V_OUT** (Q1 Drain), so nah wie möglich an Q1
- **D2 (B5819WS)**: Schutzdiode an **Fan Pin 1** (GND/GND1), direkt am Fan Connector
- **D3 (B5819WS)**: Schutzdiode an **Fan Pin 4** (PWM/GND2), direkt am Fan Connector
- Kathode jeweils an geschütztem Pin, Anode an Board GND
- SOD-323 ist sehr kompakt (1.25mm × 0.85mm) - ideal für enge Platinen

---

## 5. Software-Konfiguration (ESPHome)

### GPIO-Belegung

| GPIO | Funktion | 4-Pin Modus | 3-Pin Dual-GND Modus |
| :--- | :--- | :--- | :--- |
| **GPIO16** | PWM Primary | Fan PWM Signal | GND1 Control (Richtung A) |
| **GPIO2** | PWM Secondary | Nicht verwendet | GND2 Control (Richtung B) |
| **GPIO17** | Tacho Input | RPM Sensor | RPM Sensor |

### Mode Selection in Home Assistant

1. **"Fan Control Mode"** Select:
   - Option 1: "4-Pin PWM"
   - Option 2: "3-Pin Dual-GND"

2. **"Lüfter Richtung"** Switch (nur 3-Pin Modus):
   - OFF = Richtung A (GND1 aktiv)
   - ON = Richtung B (GND2 aktiv)

3. **"Lüfter Intensität"** Number (1-10):
   - 4-Pin: 1 = 10%, 10 = 100%
   - 3-Pin: 1 = 7V (58%), 10 = 12V (100%)

---

## 6. Sicherheitshinweise

> [!CAUTION]
> **Niemals beide GNDs gleichzeitig aktivieren!**
>
> Die Software stellt sicher, dass nur EINE GND-Leitung PWM erhält:
>
> - 10ms Safety Delay beim Umschalten
> - Automatische Deaktivierung der anderen GND vor Aktivierung
>
> **Hardwareseitig**: Keine zusätzliche Schutzschaltung nötig, da die MOSFETs getrennt sind.

> [!IMPORTANT]
> **Hardware-Jumper und Software-Modus müssen übereinstimmen!**
>
> | Jumper Setting | Software Mode | Ergebnis |
|:---|:---|:---|
| **JP1=1-2, JP2=1-2, JP3=1-2** | 4-Pin PWM | ✅ Funktioniert (Constant VCC, PWM Signal) |
| **JP1=2-3, JP2=2-3, JP3=2-3** | 3-Pin Dual-GND | ✅ Funktioniert (Var VCC, Dual GND) |
| Falsche Kombination | Mismatch | ❌ Kurzschlussgefahr oder keine Funktion! |

---

## 7. Zusammenfassung für EasyEDA

### Schritt 1: Bestehende Schaltung beibehalten

1. Platziere **Q1 (PMOS AO3401)** und **Q3 (NPN S8050)**
2. Verbinde NPN Basis via **R3 (1kΩ)** an GPIO16
3. Verbinde PMOS Gate an NPN Collector + **R8 (2.2kΩ)** Pullup an 12V
4. Platziere **D1 (B5819WS)** als Freilaufdiode an PWM_12V_OUT (nah an Q1)
5. Platziere **Jumper JP1** (Pin 1 an 12V, Pin 3 an PMOS Drain, Pin 2 an Fan VCC)

### Schritt 2: Dual Low-Side MOSFETs hinzufügen

1. Platziere **Q4 (NMOS PMV16XNR SOT-23)** für GND2
2. Platziere **Q5 (NMOS PMV16XNR SOT-23)** für GND1
3. Verbinde Fan GND1 an Q5 Drain, Fan GND2 an Q4 Drain
4. Verbinde beide Sources an Board GND
5. Platziere **Freilaufdioden D2, D3** (Kathode an Drain, Anode an GND)
6. Verbinde GPIO16 via R19 (10kΩ 0402) an Gate Q5
7. Verbinde GPIO2 via R18 (10kΩ 0402) an Gate Q4
8. Platziere R18, R19 (10kΩ 0402) Pull-Down von jedem Gate zu GND

### Schritt 3: Mode Select Jumper & Fan Connector

1. Platziere **JP2 (1x3)** für Pin 1 Selektion:
   - Pin 1 an GND
   - Pin 2 an Fan Header Pin 1
   - Pin 3 an Drain Q5
2. Platziere **JP3 (1x3)** für Pin 4 Selektion:
   - Pin 1 an GPIO16
   - Pin 2 an Fan Header Pin 4
   - Pin 3 an Drain Q4

3. **Fan Connector Belegung**:
   - Pin 1: Geht zu JP2 Pin 2
   - Pin 2: Geht zu JP1 Pin 2
   - Pin 3: Geht zu GPIO17 (Tacho)
   - Pin 4: Geht zu JP3 Pin 2

Damit hast du ein Board, das **beide Lüftertypen** steuern kann! 🚀
