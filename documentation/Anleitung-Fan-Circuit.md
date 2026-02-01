# 🔌 Anleitung: Universal Fan Interface (3-Pin & 4-Pin Support)

Diese Anleitung beschreibt die Schaltung für eine **universelle Lüftersteuerung**, die sowohl moderne 4-Pin PWM-Lüfter (ebm-papst AxiRev) als auch klassische 3-Pin Lüfter (Spannungssteuerung) unterstützt.

## 🛠️ Funktionsprinzip

Wir bauen einen **High-Side Switch**. Das bedeutet, wir schalten die **+12V Versorgungsleitung** des Lüfters mittels eines P-Channel Mosfets ein und aus (PWM).

* **3-Pin Modus**: Der Lüfter bekommt ein gepulstes 12V Signal (oder geglättetes DC) als Stromversorgung.
* **4-Pin Modus**: Der Lüfter bekommt dauerhaft 12V und regelt sich selbst über das separate PWM-Signal (Pin 4).

Die Umschaltung erfolgt über einen **Jumper (Steckbrücke)** auf der Platine.

---

## 1. Benötigte Komponenten (BOM)

| Bauteil | Symbol | Wert/Typ | Bauform | Funktion |
| :--- | :--- | :--- | :--- | :--- |
| **Q1** | PMOS | **AO3401** (oder AO3401A) | SOT-23 | Schaltet die 12V (Max 3-4A). P-Channel. |
| **Q2** | NPN | **S8050** (oder 2N2222, BC847) | SOT-23 | Pegelwandler 3.3V -> 12V Gate. |
| **D1** | Diode | **SS14** (Schottky) | SMA | Freilaufdiode (Schutz gegen Induktion). |
| **R1** | Resistor | **1kΩ** | 0603 | Basis-Widerstand für Q2. |
| **R2** | Resistor | **10kΩ** | 0603 | Pullup für Gate Q1 (hält Mosfet OFF). |
| **JP1** | Header | **1x3 Pin Header** (2.54mm) | THT | Jumper zur Moduswahl. |
| **L1** (Optional) | Inductor | **470µH** (>500mA) | SMD/THT | LC-Filter Spule (Glättung für 3-Pin). |
| **C1** (Optional) | Capacitor | **100µF** / 25V | Elko | LC-Filter Kondensator. |

---

## 2. Schaltplan (Schematic) in EasyEDA Pro

### A. Pegelwandler (Level Shifter)

Das ESP32 Signal (3.3V) kann den 12V P-Mosfet nicht direkt steuern. Wir nutzen einen NPN Transistor.

1. **XIAO Pin D6 (GPIO16)** -> Widerstand **R1 (1kΩ)**.
2. Andere Seite von R1 -> **Basis** von **Q2 (NPN)**.
3. **Emitter** von Q2 -> **GND**.
4. **Collector** von Q2 -> Verbunden mit **Gate** von **Q1 (PMOS)**.

### B. High-Side Switch (PMOS)

1. **Gate (G)** von Q1 -> Verbunden mit **Collector Q2**.
2. Zusätzlich: **Widerstand R2 (10kΩ)** zwischen **Gate (G)** und **+12V Source (S)**. (Pullup).
3. **Source (S)** von Q1 -> An **+12V Netzteil**.
4. **Drain (D)** von Q1 -> Das ist unser **PWM_12V_OUT** (zum Lüfter/Filter).

### C. Schutzdiode (Freilauf)

* **Kathode (Strich)** an **PWM_12V_OUT** (Drain Q1).
* **Anode** an **GND**.
* *Funktion*: Leitet Spannungsspitzen ab, wenn der Lüfter abschaltet.

### D. LC-Filter (Empfohlen für 3-Pin "Silent")

Um Spulenfiepen bei 3-Pin Lüftern zu verhindern, glätten wir das PWM Signal zu einer sauberen Gleichspannung.

1. **PWM_12V_OUT** -> Spule **L1 (470µH)** -> Knoten **DC_VAR_12V**.
2. Knoten **DC_VAR_12V** -> Kondensator **C1 (100µF, +)** -> **GND (-)**.

*Wenn du kein LC-Filter willst, verbinde PWM_12V_OUT direkt mit DC_VAR_12V.*

### E. Jumper Konfiguration (Der "Universelle" Teil)

Wir nutzen einen **3-Pin Header (JP1)** um zu wählen, was an **Pin 2 des Lüftersteckers** ankommt.

* **Pin 1 (Oben)**: Verbunden mit **+12V Dauerstrom** (vom Netzteil).
* **Pin 2 (Mitte)**: Geht zum **Lüfterstecker Pin 2 (+VCC)**.
* **Pin 3 (Unten)**: Verbunden mit **DC_VAR_12V** (der Ausgang unserer Mosfet/Filter Schaltung).

**Logik:**

* Jumper auf **1-2**: Lüfter bekommt dauerhaft 12V. (**4-Pin Betrieb**)
* Jumper auf **2-3**: Lüfter bekommt geregelte Spannung. (**3-Pin Betrieb**)

### F. Lüfterstecker (4-Pin Fan Header)

Verbinde den 4-Pin PC-Lüfteranschluss (Molex KK 254 kompatibel) wie folgt:

| Pin | Name | Anschluss an... |
| :--- | :--- | :--- |
| **1** | GND | **GND** |
| **2** | VCC | **JP1 Pin 2 (Mitte)** |
| **3** | Tacho | An **XIAO D7 (GPIO17)** (Pullup nicht vergessen!) |
| **4** | PWM | An **XIAO D6 (GPIO16)** (Kein Pullup nötig, Push-Pull) |

> ⚠️ **Wichtig**: PWM (Pin 4) wird direkt vom GPIO16 gesteuert.
>
> * Im **4-Pin Modus** (Jumper 1-2) nutzt der Lüfter dieses Signal zur Regelung.
> * Im **3-Pin Modus** (Jumper 2-3) ignoriert der Lüfter Pin 4 (da Pin 4 bei 3-Pin Steckern gar nicht belegt ist), und die Regelung erfolgt über die Spannung an Pin 2.

---

## 3. Layout Guide (PCB Design)

1. **Leiterbahnbreite**:
    * Die Strecke **12V Input -> Mosfet -> Jumper -> Lüfterstecker** führt den vollen Motorstrom.
    * Nutze hier **mindestens 0.5mm (20mil)**, besser **0.8mm**.
2. **Abstand**:
    * Halte den Hochstrom-Pfad fern vom Tacho-Signal (Pin 3), um Störungen zu vermeiden.
3. **Kühlung**:
    * Der AO3401 Mosfet ist klein (SOT-23). Er schafft ca. 3A. Wenn du größere Flächen an Drain/Source anbindest, dient das Kupfer als Kühlkörper.
4. **Platzierung der Diode**:
    * Die SS14 Diode sollte so nah wie möglich am Lüfterstecker oder am Mosfet platziert werden.

---

## 4. Zusammenfassung für EasyEDA

1. Platziere **Q1 (PMOS)** und **Q2 (NPN)**.
2. Verbinde **NPN Basis** via 1k an GPIO16.
3. Verbinde **PMOS Gate** an NPN Collector + 10k Pullup an 12V.
4. Platziere **Jumper JP1**. Pin 1 an 12V, Pin 3 an PMOS Drain, Pin 2 an Fan.
5. Route GPIO16 **ZUSÄTZLICH** direkt an Fan Pin 4 (für 4-Pin PWM).

Damit hast du ein Board, das **jeden** PC-Lüftertyp steuern kann! 🚀
