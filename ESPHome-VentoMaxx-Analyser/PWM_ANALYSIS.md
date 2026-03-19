# VentoMaxx PWM Analyse (Original-Steuerung)

Dieses Dokument analysiert die gemessenen PWM-Spannungswerte der originalen VentoMaxx-Steuerung (ebm-papst 4412 F/2 GLL).

## Messdaten (Multimeter - Durchschnittsspannung)

Angenommene Peak-Spannung: **12.0 V**

| Modus | Spannung | Duty Cycle (berechnet) | Beobachtung |
|-------|----------|------------------------|-------------|
| **Richtung A (Max)** | 6.97 V | ~58 % | Hohe Drehzahl |
| **Richtung A (Min)** | 9.20 V | ~77 % | Niedrige Drehzahl |
| **Richtung B (Min)** | 9.95 V | ~83 % | Niedrige Drehzahl |
| **Richtung B (Max)** | 11.26 V | ~94 % | Hohe Drehzahl |
| **Stopp / Wechsel** | -0.15 V | ~0 % | Hardware-Aus |

## Interpretation der Logik

### 1. Der "Neutral-Punkt"

Anders als in der Standard-VarioPro-Spezifikation (50% = Stopp) liegt der Wendepunkt bei Ventomaxx anscheinend bei ca. **80% Duty Cycle**.

* Werte **über 80%** steuern Richtung B.
* Werte **unter 80%** steuern Richtung A.

### 2. Drehzahl-Kennlinie

* **In Richtung B:** Proportional. Mehr Duty Cycle = Mehr Drehzahl (83% -> 94%).
* **In Richtung A:** Umgekehrt Proportional. Weniger Duty Cycle = Mehr Drehzahl (77% -> 58%).

### 3. Das "Stopp"-Signal

Der Messwert von ~0V während des Wechsels ist kritisch. Das bedeutet, die Steuerung nutzt den **0% Duty Cycle (Dauersignal auf GND)** als expliziten Befehl zum Richtungsstopp oder -wechsel.

## Fazit der Messung

**Das passt nicht zu den Messwerten!** Wenn wir unseren Lüfter mit 50% PWM ansteuern (was wir für "Stopp" halten), interpretiert der Lüfter das laut diesen Werten wahrscheinlich bereits als "Vollgas Richtung A" (da es weit unter dem 80%-Wendepunkt liegt).

## Plan für die Software-Anpassung (Mapping)

Sobald die Analysator-Werte (Duty Cycle in %) von der originalen Steuerung vorliegen, werden wir die `set_fan_logic()` in der `automation_helpers.h` wie folgt anpassen, um das Originalverhalten ohne Hardware-Änderung zu kopieren:

1. **Pivot-Punkt (Neutral):** Wir verschieben den softwareseitigen Stopp-Punkt von 50 % auf den vom Analysator ermittelten Wendepunkt (vermutlich ca. 80-85 %).
2. **Skalierung (Gain):** Wir passen die Formel so an, dass die Stufen 1 bis 10 genau den Duty-Cycle-Bereichen der Originalsteuerung entsprechen (z. B. 83% bis 94% für Richtung B).
3. **Frequenz-Abgleich:** Falls der Analysator eine andere Frequenz als 2000 Hz anzeigt, passen wir den `ledc`-Output in der `hardware_io.yaml` an.
4. **Invertierungs-Validierung:** Der Analysator zeigt uns, ob das Originalsignal "High-Active" oder "Low-Active" ist. Damit stellen wir sicher, dass unser MOSFET-Kompensations-Flag (`inverted: true`) korrekt gesetzt ist.

**Abschluss-Fazit:** Der Xiao ESP32-C6 wird durch diese Software-Remapping-Logik zum perfekten Ersatz für die VentoMaxx-Steuerung, nutzt aber gleichzeitig deine neuen Smart-Features (CO2-Automatik, Radar-Präsenz, Dashboard).

## Oszilloskop Messung

## 1. Basisdaten & Signalqualität

Die oszilloskopischen Messungen am PWM-Ausgang der Steuerplatine belegen eine einwandfreie Funktion der Hardware-Stufe.

* **Frequenz (f):** Eine Signalperiode (T) dauert exakt $500\ \mu\text{s}$ (5 Divisionen à $100\ \mu\text{s}$). Dies entspricht exakt den spezifizierten **2.000 Hz** ($2\text{ kHz}$) des Herstellers.
* **Logikpegel:** Die Spannung pegelt sich im High-Zustand sicher bei knapp **5V** ein. Dies resultiert aus dem internen Pull-up-Widerstand des Lüfters.
* **Invertierende Schaltlogik:** Da ein Low-Side-MOSFET als Open-Drain/Open-Collector agiert, invertiert sich das Tastverhältnis physikalisch. Ein $100\ \%$-PWM-Signal des Controllers zieht die Leitung auf $0\text{V}$ (entspricht $0\ \%$-PWM am Lüfter). Dies muss in der Software ausgeglichen werden.

---

## 2. Messreihen & Auswertung (V-Kennlinie)

Der verwendete Lüfter besitzt eine V-förmige Steuerkurve für den Richtungswechsel. Die Messreihe bestätigt, dass alle relevanten Betriebspunkte exakt und stabil angefahren werden können.

*Hinweis: Ein hohes Signal (High) auf dem Oszilloskop bedeutet, dass der MOSFET sperrt und der Lüfter das Signal über seinen Pull-up-Widerstand hochzieht.*

| Betriebszustand | Datei / Messung | Signal (Lüfter sieht) | Verhalten gem. Datenblatt |
| :--- | :--- | :--- | :--- |
| **Stillstand (Pause)** | `S5-Pause.jpg` | **50 % PWM** | Aktive Bremsung, Rotor steht |
| **Zuluft: Stufe 1** (Minimum) | `S1a.jpg` | **30 % PWM** | Niedrige Vorwärtsdrehzahl |
| **Zuluft: Stufe 5** (Maximum) | `S5a.jpg` | **5 % PWM** | Maximale Vorwärtsdrehzahl ($4.200\text{ U/min}$) |
| **Abluft: Stufe 1** (Minimum) | `S2b.jpg` | **70 % PWM** | Niedrige Rückwärtsdrehzahl |
| **Abluft: Stufe 5** (Maximum) | `S5b.jpg` | **95 % PWM** | Maximale Rückwärtsdrehzahl ($4.200\text{ U/min}$) |

---

## 3. Hardware-Fazit

Das Platinendesign der PWM-Stufe funktioniert **makellos**.
Der PCA9685 I2C-Treiber gibt das 2-kHz-Signal absolut stabil aus, der N-Channel MOSFET (PMV16XNR) schaltet ausreichend schnell und präzise, und die Pegel bewegen sich im spezifizierten, sicheren Rahmen. Die hardwareseitigen Voraussetzungen für eine softwareseitige, bidirektionale Drehzahlregelung in ESPHome sind damit vollständig erfüllt.

## Software Anpassung (Mapping)

Ich habe die Steuerung nun exakt an die Oszilloskop-Messungen der V-Kennlinie angepasst.

**Was wurde geändert?**

* **set_fan_logic:** Die Funktion mappt nun die Stufen 1 bis 10 linear auf die gemessenen Werte:
  * **Zuluft:** Von 30% (Stufe 1) runter auf 5% (Stufe 10).
  * **Abluft:** Von 70% (Stufe 1) hoch auf 95% (Stufe 10).
  * **Stillstand:** Exakt 50%.
* **Richtungen:** Die Richtungen im Code wurden so getauscht, dass sie zur Mess-Tabelle passen (Zuluft < 50%, Abluft > 50%).
* **Hardware-Check:** Die Frequenz in `hardware_io.yaml` steht bereits auf den korrekten **2000 Hz**.

Das Projekt kann nun neu kompiliert und getestet werden. Der Lüfter sollte nun hochpräzise reagieren und die Richtungen korrekt wechseln.
