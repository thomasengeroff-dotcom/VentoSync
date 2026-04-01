# VentoMaxx PWM Analyser (ESPHome)

Dieses Tool dient dazu, das PWM-Signal der **originalen VentoMaxx-Steuerung** abzugreifen, zu analysieren und in Home Assistant zu visualisieren. Dies ist ein entscheidender Schritt, um die exakte Kennlinie des Lüfters (ebm-papst 4412 F/2 GLL) zu verstehen und die neue ESPHome-Steuerung perfekt darauf zu kalibrieren.

## Zweck des Analysers

- **Signal-Capturing**: Aufzeichnung des Duty Cycle der Originalsteuerung.
- **Logik-Validierung**: Überprüfung, ob die Steuerung nach dem VarioPro-Standard (50% STOP) oder einer herstellerspezifischen Kurve arbeitet.
- **Invertierungs-Check**: Feststellen, ob das Signal "High-Active" oder "Low-Active" (via MOSFET) ausgegeben wird.
- **Frequenz-Messung**: Ermittlung der PWM-Frequenz (Standard: 2000 Hz).

## Hardware-Voraussetzungen

- **Microcontroller**: [Seeed XIAO ESP32-C6](https://www.seeedstudio.com/XIAO-ESP32C6-p-5884.html) (identisch zur Haupt-Platine).
- **Spannungsteiler**: Da der VentoMaxx-PWM-Ausgang mit **12V** arbeitet, der ESP32-C6 aber nur maximal **3.6V** verträgt, ist ein Spannungsteiler zwingend erforderlich!

### Empfohlener Schaltplan (22k / 10k)

```text
Original PWM Signal (12V) ---- [ 22 kOhm (R1) ] ----+---- GPIO0 (D0 am XIAO)
                                                    |
                                             [ 10 kOhm (R2) ]
                                                    |
GND (VentoMaxx) ------------------------------------+---- GND (XIAO)
```

*Berechnung: 12V * (10 / (22+10)) = 3.75V (Theoretisch). In der Praxis liegt das Signal oft bei ~11.26V, was ca. **3.52V** am Pin ergibt – sicher für den ESP32-C6.*

## Software & Konfiguration

Die Konfiguration erfolgt über die Datei [esp_ventomaxx_analyser.yaml](file:///c:/Users/thomas.engeroff/Documents/ESPHome-Projekte/VentoSync/ESPHome-VentoMaxx-Analyser/esp_ventomaxx_analyser.yaml).

### Funktionen im YAML

- **PWM Signal (direkt)**: Rohmessung des Duty Cycle am Pin.
- **PWM Fan sieht (invertiert)**: Berechneter Wert unter der Annahme einer MOSFET-Invertierung.
- **Richtung+Speed Vektor**: Visualisierung von -100% (Richtung A) bis +100% (Richtung B).
- **Status-Text**: Klartext-Anzeige (z.B. "Richtung A: 40%").

## Ergebnisse der Analyse

Die detaillierten Messergebnisse und die daraus abgeleiteten Anpassungen für die Haupt-Steuerung sind im Dokument [PWM_ANALYSIS.md](file:///c:/Users/thomas.engeroff/Documents/ESPHome-Projekte/VentoSync/ESPHome-VentoMaxx-Analyser/PWM_ANALYSIS.md) dokumentiert.

> [!IMPORTANT]
> Schließe den Analyser niemals ohne Spannungsteiler an die 12V der Originalsteuerung an, da dies den ESP32-C6 sofort zerstören würde!
