# 🔌 Anleitung: Universal Fan Interface (4-Pin Screw Terminal)

Diese Anleitung beschreibt die Schaltung für eine **universelle Lüftersteuerung**, die standardmäßig über ein **4-Pin Schraubterminal** realisiert wird. Diese Schnittstelle unterstützt sowohl Standard 4-Pin PWM-Lüfter als auch spezielle 3-Pin Ventomaxx-Lüfter (ebm-papst VarioPro) durch einfache Anpassung der Verkabelung.

> ℹ️ **Hinweis zu 3-Pin PWM Lüftern:**
> Neben den klassischen 4-Pin PWM Lüftern gibt es auch spezielle Lüfter, die **kein Tacho-Signal** besitzen und daher nur über **3 Pins** verfügen (GND, 12V, PWM). Dies ist auch der Fall für den von Ventomaxx verbauten EBM-Papst Lüfter.
Diese können problemlos ohne physikalische Änderung an der Schaltung betrieben werden, indem der Tacho-Pin (Pin 3 am Terminal) einfach unbelegt bleibt. Beachte jedoch, dass ohne Tacho-Signal keine direkte Überwachung der Drehzahl (RPM) oder Blockadeerkennung durch die Software möglich ist.

## 🛠️ Funktionsprinzip: Open-Drain PWM

Die Steuerung basiert auf einem simplen **Open-Drain PWM-Treiber**:

- **4-Pin PWM-Lüfter**: Der PWM-Eingang (Pin 4) wird vom MOSFET auf GND gezogen. Der interne Pull-Up im Lüfter sorgt für das High-Signal.

> ✅ **Verifiziert**: Diese Logik wurde mit `espslaveNTC.yaml` und 2kHz PWM erfolgreich getestet.

---

## 1. Pinbelegung (PCB & Terminal)

Wir verwenden ein standardisiertes **4-Pin Schraubterminal** auf der Platine.

| Terminal Pin | Funktion | Beschreibung | ESP32 GPIO |
| :--- | :--- | :--- | :--- |
| **1** | **GND** | Gemeinsame Masse | - |
| **2** | **+12V** | Konstante 12V Versorgungsspannung | - |
| **3** | **TACHO** | Drehzahlsignal (Input) | GPIO17 (mit Pull-Up) |
| **4** | **PWM** | PWM Steuersignal (Open-Drain Output) | **GPIO6** (via MOSFET) |

---

## 2. Verkabelung der Lüfter

Durch unterschiedliches Anschließen der Adern am Schraubterminal werden beide Lüftertypen unterstützt.

### A. Standard 4-Pin PWM Lüfter (PC-Lüfter, ebm-papst AxiRev)

Die Farben können variieren (oft Schwarz/Gelb/Grün/Blau), aber die Pin-Reihenfolge am Stecker ist genormt.

| Lüfter Funktion | Terminal Pin |
| :--- | :--- |
| **GND** (Pin 1) | -> **1 (GND)** |
| **+12V** (Pin 2) | -> **2 (+12V)** |
| **Tacho** (Pin 3) | -> **3 (TACHO)** |
| **PWM** (Pin 4) | -> **4 (PWM)** |

### B. Ventomaxx / ebm-papst VarioPro (3-Pin)

Dieser Lüfter hat typischerweise offene Adern oder einen speziellen Stecker.
**WICHTIG**: Die Aderfarben beachten! (Rot = +12V, Blau = GND, Weiß = Signal/PWM - *Beispielhaft, bitte Datenblatt prüfen!*).
Laut Nutzer-Validierung:

- Ader 1: +12V
- Ader 2: GND
- Ader 3 (Mitte): Steuersignal (PWM)

| Lüfter Ader | Funktion | Terminal Pin |
| :--- | :--- | :--- |
| **Ader 2** | GND / Masse | -> **1 (GND)** |
| **Ader 1** | +12V DC | -> **2 (+12V)** |
| **-** | (Nicht belegt) | **3 (TACHO)** (leer lassen) |
| **Ader 3** | Steuersignal | -> **4 (PWM)** |

> ℹ️ **Hinweis**: Das Steuersignal des Ventomaxx kommt an den **PWM-Pin** des Terminals, da es vom selben MOSFET gesteuert wird wie der PWM-Eingang eines 4-Pin Lüfters.

---

## 3. Anleitung für EasyEDA Pro

### Schritt 1: Hinzufügen (Universal Circuit)

Wir benötigen nur einen einzigen MOSFET für den PWM-Kanal.

1. **Bauteil**: Platziere **1x N-Kanal MOSFET** (z.B. **PMV16XNR** oder 2N7002, SOT-23). Nennen wir ihn **Q_PWM**.
2. **Source**: Verbinde Source von Q_PWM direkt mit **GND**.
3. **Gate**:
    - Verbinde Gate von Q_PWM über einen **1kΩ Widerstand (R_Gate)** mit **GPIO6**.
    - Füge einen **10kΩ Pull-Down (R_PD)** direkt vom Gate zu GND hinzu (sichert "Aus"-Zustand beim Boot).
4. **Drain**: Verbinde Drain von Q_PWM direkt mit **Terminal Pin 4 (PWM)**.
5. **Schutz**: (Optional aber empfohlen) Füge eine kleine **Schottky-Diode (z.B. B5819WS)** zwischen Drain (Kathode) und GND (Anode) als ESD/Induktionsschutz hinzu, falls lange Kabel wirken.

### Schritt 2: Tacho & Power

1. **Terminal Pin 1**: Direkt an **GND Plane**.
2. **Terminal Pin 2**: Direkt an **+12V**.
3. **Terminal Pin 3 (Tacho)**:
    - Verbinde mit **GPIO17**.
    - Füge einen **4.7kΩ - 10kΩ Pull-Up Widerstand** zu +3.3V hinzu (wichtig für Open-Collector Tacho).
    - Füge einen **1kΩ Serienwiderstand** zum GPIO-Schutz hinzu.
    - Optional: Zener-Diode (3.3V) zum Schutz des GPIOs vor Überspannung.

### Zusammenfassung Schaltplan (Neu)

```
       +12V ─────────────────────────────── Terminal Pin 2
                                         ┌─ Terminal Pin 1
        GND ─────────────────────────────┴─ Source (Q_PWM)

GPIO6 ──[1k]──┬── Gate (Q_PWM)
              │
             [10k]
              │
             GND

Drain (Q_PWM) ───────────────────────────── Terminal Pin 4 (PWM Output)

                                         ┌─ +3.3V
                                         │
                                       [10k]
                                         │
GPIO17 ──[1k]────────────────────────────┼─ Terminal Pin 3 (Tacho Input)
```

---

## 4. ESPHome Konfiguration (Software)

Diese Konfiguration entspricht dem erfolgreich getesteten `espslaveNTC.yaml`.

```yaml
# PWM Output Konfiguration
output:
  - platform: ledc
    pin: GPIO6
    id: fan_pwm_output
    frequency: 2000Hz  # WICHTIG für Ventomaxx/VarioPro! (PC-Lüfter oft 25kHz, aber 2kHz meist auch ok)

# Tacho Sensor (Optional)
sensor:
  - platform: pulse_counter
    pin: GPIO17
    name: "Fan RPM"
    id: fan_rpm
    unit_of_measurement: 'RPM'
    filters:
      - multiply: 0.5  # Standard PC Fan: 2 Impulse pro Umdrehung

# Fan Komponente (Optional, zur Integration in HA)
fan:
  - platform: speed
    output: fan_pwm_output
    name: "Wohnraumlüftung"
```

> [!TIP]
> **Frequenz**: 2000 Hz ist für den Ventomaxx **zwingend**.
> Standard PC-Lüfter spezifizieren oft 25 kHz, laufen aber in der Regel auch mit 2 kHz problemlos. Sollte der PC-Lüfter Pfeifgeräusche machen, könnte man die Frequenz erhöhen – der Ventomaxx benötigt jedoch strikt ~2 kHz.
