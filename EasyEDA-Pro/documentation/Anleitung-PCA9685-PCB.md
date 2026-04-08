# 📟 Anleitung: Control Panel PCB (PCA9685 & Buttons)

Diese Anleitung beschreibt das Design des PCBs für die Anbindung des VentoMaxx-Panels.
Wir verwenden einen **PCA9685** für **dimmbare LEDs** und direkte **GPIOs des ESP32-C6** für die **Taster**.

## 1. Konzept ändern

* **ALT**: MCP23017 (16 GPIOs für Taster & LEDs) -> Nur An/Aus für LEDs.
* **NEU**:
  * **PCA9685 (I2C PWM Driver)**: Steuert die LEDs und ermöglicht echtes **Dimmen** (Soft-Fade Effekte).
  * **MCP23017 (I2C I/O Expander)**: Fragt die Taster ab (GPA0-GPA2) und stellt freie Anschlüsse für mögliche Erweiterungen zur Verfügung (GPB0-GPB5 auf Pin Header). Dies spart direkte GPIO-Pins des ESP32.

## 2. Benötigte Komponenten (BOM)

| Bauteil | Wert | Bauform | Menge | Funktion |
| :--- | :--- | :--- | :--- | :--- |
| **PCA9685PW** | - | **TSSOP-28** | 1 | 16-Kanal PWM Driver (I2C). LCSC: `C2678753` |
| **Kondensator** | **10uF** | 0805 | 1 | Power Buffer (VDD) |
| **Kondensator** | **100nF** | 0603 | 1 | Bypass (direkt am Chip) |
| **Widerstand** | **10kΩ** | 0603 | 1 | Pullup für OE (Output Enable) |
| **Widerstand** | **4.7kΩ** | 0603 | 2 | Pullups für I2C (SDA/SCL) / schon erfolgt für BME680 |
| **Stecker** | **14-Pin FFC** | 1.0mm/0.5mm* | 1 | Zum Panel (*Pitch geprüft*) |
| **Widerstand** | **470Ω*** | 0603 | 9 | Vorwiderstände für LEDs (durch Messung ermittelt) |
| **Resistor Array** | **10kΩ** | 0603 (4-fach) | 1 | Pullups für Taster (LCSC: `C136853`) |
| **RC-Filter** | **100Ω/100nF** | 0603 | 3x | Optional: Entprellung für Taster |

## 3. Schaltplan Details

### A. PCA9685 (PWM für LEDs) - Pinout (TSSOP-28)

* **VDD (Pin 28)**: An **3.3V**.
* **VSS (Pin 14)**: An **GND**.
* **SDA (Pin 27)**: An **XIAO SDA (D4/GPIO22)**. (Korrigiert: Pin 27, nicht 26)
* **SCL (Pin 26)**: An **XIAO SCL (D5/GPIO23)**. (Korrigiert: Pin 26, nicht 25)
* **OE (Pin 23)**: An **XIAO D3 (GPIO21)**. **PullUp 10k** an 3.3V. (Active LOW).
* **EXTCLK (Pin 25)**: External Clock Input. Via **GND** deaktivieren (nutze internen Oszillator). (Korrigiert: Pin 25)
* **A0-A4 (Pin 1-5)**: Address Inputs. An **GND**.
* **A5 (Pin 24)**: Address Input. An **GND**. (Korrigiert: Pin 24)
  * Alle Adress-Pins an GND = Adresse **0x40**.

### B. LED Zuordnung (PCA9685 Outputs)

| Funktion | PCA9685 Kanal | TSSOP-28 Pin | Signal Name |
| :--- | :--- | :--- | :--- |
| **LED Power** | Channel 8 | Pin 15 | LED_PWRV |
| **LED Master** | Channel 7 | Pin 13 | LED_MSTV |
| **LED L1** | Channel 1 | Pin 7 | LED_L1V |
| **LED L2** | Channel 9 | Pin 16 | LED_L2V |
| **LED L3** | Channel 3 | Pin 9 | LED_L3V |
| **LED L4** | Channel 11 | Pin 18 | LED_L4V |
| **LED L5** | Channel 5 | Pin 11 | LED_L5V |
| **LED WRG** | Channel 13 | Pin 20 | LED_WRGV |
| **LED Vent** | Channel 15 | Pin 22 | LED_VENV |
| *Unbenutzt* | Ch 0, 2, 4, 6, 10, 12, 14 | - | - |

> ✅ **Bestätigung für Common Cathode (Gemeinsames GND)**:
>
> * **Verkabelung**: Kathoden der LEDs an **GND** (via Pin 13/14). Anoden an den PCA9685.
> * **Spannung**: Die LEDs benötigen 3.3V (High-Pegel vom PCA9685).
> * **Vorwiderstände**: **JA**, diese sind **zwingend erforderlich** (z.B. 470Ω - 1kΩ), da der PCA9685 eine Spannungsquelle ist (Totem-Pole) und den Strom nicht selbst begrenzt.
> * **Chip-Modus**: Der PCA9685 startet laut Datasheet (NXP) standardmäßig im **Totem-Pole Modus** (MODE2 Register = 0x04). Das passt perfekt für Common Cathode.

**Achtung bei 3.3V Logic:**
Schließe VDD des PCA9685 an 3.3V an. Der Chip treibt die LEDs direkt aus VDD. Daher ist die Entkopplung (siehe unten) extrem wichtig.

### C. Taster (Bedieneinheit via MCP23017)

Die Taster-Pins vom FFC gehen auf den **MCP23017 I2C Expander** (Adresse `0x20`).
**WICHTIG:** Die Taster sind mit externen 10kΩ Pull-Up-Widerständen beschaltet und reagieren somit "Active Low" (Gedrückt = GND).

| Funktion | FFC Pin | MCP23017 Pin | Port/Bit | Pullup |
| :--- | :--- | :--- | :--- | :--- |
| **BTN Power** | Pin 2 | **Pin 21** | **GPA0** | **10k an 3.3V** |
| **BTN Mode** | Pin 4 | **Pin 22** | **GPA1** | **10k an 3.3V** |
| **BTN Level** | Pin 3 | **Pin 23** | **GPA2** | **10k an 3.3V** |
| **GND** | PIN 1 | - | - | - |
| **GND** | PIN 14 | - | - | - |

**Freie GPB-Ports (Expansion Header):**
Die Pins **GPB0 bis GPB5** (Pins 1-5 und 25-28) sind auf einen separaten Pin Header herausgeführt, um spätere Erweiterungen (z.B. externe Hardware-Taster) leicht anbinden zu können.

**Gesamte Pin-Belegung (XIAO ESP32-C6):**

**Gesamte Pin-Belegung (XIAO ESP32-C6):**

| Pin | Funktion |
| :--- | :--- |
| **D0 (GPIO0)** | NTC Sensor Innen (ADC) |
| **D1 (GPIO1)** | NTC Sensor Außen (ADC) |
| **D2 (GPIO2)** | MCP23017_RESET (Output) |
| **D3 (GPIO21)** | PCA9685_OE (Output Enable) |
| **D4 (GPIO22)** | I2C SDA |
| **D5 (GPIO23)** | I2C SCL |
| **D6 (GPIO16)** | Radar RX (Geplant) |
| **D7 (GPIO17)** | Radar TX (Geplant) |
| **D8 (GPIO19)** | Lüfter PWM |
| **D9 (GPIO20)** | Lüfter Tacho |
| **D10 (GPIO18)** | *Unbelegt* |

> ✅ **Lösung**: Durch die Auslagerung der Taster auf den MCP23017 bleiben die ESP32-Pins D6/D7 für das MR24HPC1-Radar frei. Die analogen Temperatursensoren belegen weiterhin sicher D0 und D1. Das Design ist somit **konfliktfrei**.

## 4. PCB Layout Empfehlungen

1. **Entkopplung**: 100nF Kondensator direkt an VDD des PCA9685.
2. **I2C**: Saubere Leiterbahnführung für SDA/SCL.
3. **Taster**: Leitungen zu den Buttons ggf. mit 100nF gegen GND entprellen (Hardware Debounce), entlastet die Software.

---

## Software Konfiguration (ESPHome)

In der `yaml` wird der Block für den alten **PCA9685** hinzugefügt (I2C `0x40`) und **der MCP23017 bleibt** unter der Adresse `0x20` aktiv.
Die `binary_sensor` (Taster) verwenden als Plattform weiterhin `gpio`, beziehen sich nun jedoch auf den Hub des `mcp23017` statt interner Pins.

### Beispiel Ausschnitt

```yaml
i2c:
  sda: GPIO22
  scl: GPIO23

mcp23017:
  - id: mcp23017_hub
    address: 0x20

pca9685:
  - id: 'pwm_hub'
    frequency: 1000Hz # Gute Frequenz für LEDs (flimmerfrei)

binary_sensor:
  - platform: gpio
    pin:
      mcp23xxx: mcp23017_hub
      number: 0 # GPA0
      mode:
        input: true
      inverted: true
    name: "Button Power"
    # ...
```
