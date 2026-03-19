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
