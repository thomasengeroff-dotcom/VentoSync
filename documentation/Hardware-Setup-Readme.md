# 🛠️ Hardware Setup: Prototyp & PCB

Dieses Dokument spezifiziert die Hardware-Komponenten, die Verkabelung für den Prototypen (Breadboard) und die Design-Vorgaben für das finale PCB (EasyEDA).

> [!IMPORTANT]
> **Pin-Referenz**: Die Pin-Belegung basiert auf der Nutzung des **Original VentoMaxx Bedienpanels** (14-Pin FFC), welches über einen **MCP23017 I/O Expander** via I²C angesteuert wird.

---

## 1. Übersicht & BOM (Bill of Materials)

### Zentrale Einheit & Power

* **MCU**: Seeed Studio XIAO ESP32C6
* **Netzteil (High Voltage)**: Recom RAC10-12SK/277 (230V AC -> 12V DC, 10W)
* **Logic Power (DC/DC)**: Recom R-78E5.0-0.5 oder Traco TSR 1-2450 (12V -> 5V)
* **Sicherung**: 1A Slow Blow (Träge)
* **Varistor**: S10K275 (Überspannungsschutz Eingang)
* **Pufferung**:
  * 470µF / 25V Elko (12V Rail)
* **MCU**: Seeed Studio XIAO ESP32C6
* **Netzteil (High Voltage)**: Recom RAC10-12SK/277 (230V AC -> 12V DC, 10W)
* **Logic Power (DC/DC)**: Recom R-78E5.0-0.5 oder Traco TSR 1-2450 (12V -> 5V)
* **Sicherung**: 1A Slow Blow (Träge)
* **Varistor**: S10K275 (Überspannungsschutz Eingang)
* **Pufferung**:
  * 470µF / 25V Elko (12V Rail)
  * 10µF / 50V Elko (Eingang DC/DC)
  * 10µF / 16V Elko (Ausgang DC/DC 5V)
  * 100nF Kerko (HF-Entstörung 5V Rail)

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
* **BME680**: Bosch Umweltsensor (I2C) - Temperatur, Feuchte, Druck, IAQ
* **VentoMaxx Bedienpanel**: Original Panel mit 9 LEDs und 3 Tastern (14-Pin FFC Anschluss).
* **I/O Expander**: **MCP23017** (I2C) - Zur Ansteuerung des Panels.
* **NTC-Sensorik**: 2x 10kΩ NTC (Beta 3435)
  * *Beschaltung*: 10kΩ 0.1% Referenzwiderstand + 100nF Filter + 3.3V Zener/TVS

---

## 2. Pin-Mapping

### A. XIAO ESP32C6 (Master)

| XIAO Pin | GPIO | Funktion | Anschluss-Details |
| :--- | :--- | :--- | :--- |
| **D0** | GPIO0 | **NTC Innen** | Analog In (10k Spannungsteiler) |
| **D1** | GPIO1 | **NTC Außen** | Analog In (10k Spannungsteiler) |
| **D2** | GPIO2 | *Reserve* | (Frei / Debug LED) |
| **D3** | GPIO21 | **MCP23017 INTB** | Interrupt von Port B (Taster). Pin 19. |
| **D4** | GPIO22 | **I2C SDA** | BME680, MCP23017 (mit 4.7kΩ Pullup) |
| **D5** | GPIO23 | **I2C SCL** | BME680, MCP23017 (mit 4.7kΩ Pullup) |
| **D6** | GPIO16 | **Fan PWM** | An Basis BC547 |
| **D7** | GPIO17 | **Fan Tacho** | Von Lüfter Pin 3 (mit Pullup & Zener) |
| **D8-D10** | - | *Reserve* | SPI / Frei |

### C. Universal-Lüfter Interface (3-Pin & 4-Pin Support)

Damit du sowohl moderne 4-Pin PWM-Lüfter als auch ältere/spezielle 3-Pin Lüfter (wie VarioPro) steuern kannst, brauchst du eine **Umschaltung**.

**Logik:**

* **4-Pin Mode**: 12V ist dauerhaft an. PWM-Signal geht an Pin 4.
* **3-Pin Mode**: 12V wird "gehackt" (High-Side PWM). Pin 4 ist inaktiv.

**Schaltplan-Erweiterung (High-Side Switch):**

1. **P-Channel MOSFET** (z.B. AO3401) in die 12V-Leitung zum Lüfter einfügen (Source an 12V, Drain an Lüfter Pin 2).
2. **Treiber**: NPN-Transistor (BC547) steuert das Gate des MOSFETs (Gate Pullup 10k auf 12V, NPN Collector an Gate).
3. **DIP-Switch / Jumper** (2-Wege Umschalter) für das PWM-Signal (XIAO D6):
    * **Position A (4-Pin)**: Leitet D6 direkt an Lüfter **Pin 4**. (In dieser Stellung muss der MOSFET dauerhaft leitend geschaltet werden, z.B. Gate manuell auf GND brücken oder zweiten Jumper nutzen).
    * **Position B (3-Pin)**: Leitet D6 an die Basis des NPN-Treibers -> MOSFET moduliert die 12V Versorgungsspannung.

> ⚠️ **Tacho-Signal & Geräusche (3-Pin Mode)**:
>
> 1. **RPM-Schwankung**: Ja, das betrifft nur das *Auslesen* der Drehzahl (Tacho), da dem Sensor der Strom fehlt. Das ist für die Regelung unkritisch, nur die Anzeige "zappelt".
> 2. **Spulenfiepen (Whine)**: "Gehackte" 12V (PWM) erzeugen bei Lüftern oft hörbares Fiepen.
>     * **Lösung für "maximale Sauberkeit"**: Baue einen **LC-Filter** (Spule + Kondensator) hinter den MOSFET. Das glättet das PWM-Signal zu einer sauberen Gleichspannung (Buck-Converter Prinzip).
>     * **Regelung**: Die Drehzahl ist weiterhin **voll regelbar**! (50% PWM ≈ 6V, 100% PWM = 12V).
>     * *Werte*: z.B. 100µH - 330µH Spule + 100µF Elko. Damit läuft der 3-Pin Lüfter lautlos und analog.

### B. MCP23017 Expander (Adresse 0x20)

Der Expander steuert das 14-Pin FFC Kabel des Panels.

| MCP Pin | Funktion | Panel Komponente |
| :--- | :--- | :--- |
| **GPA0** | OUTPUT | LED Power (On/Off) |
| **GPA1** | OUTPUT | LED Master (Status) |
| **GPA2** | OUTPUT | LED Intensität 1 |
| **GPA3** | OUTPUT | LED Intensität 2 |
| **GPA4** | OUTPUT | LED Intensität 3 |
| **GPA5** | OUTPUT | LED Intensität 4 |
| **GPA6** | OUTPUT | LED Intensität 5 |
| **GPA7** | OUTPUT | LED Modus: Wärmerückgewinnung |
| **GPB0** | OUTPUT | LED Modus: Durchlüften |
| **GPB5** | INPUT_PULLUP | Taster: Power |
| **GPB6** | INPUT_PULLUP | Taster: Modus |
| **GPB7** | INPUT_PULLUP | Taster: Intensität |

---

## 3. Verkabelung & Bedienpanel (FFC 14-Pin)

Das VentoMaxx Panel wird über ein 14-Pin Flachbandkabel angeschlossen. Die Belegung muss auf dem PCB auf den I/O Expander geroutet werden.

**Logik-Annahme**:

* **LEDs**: Common Cathode (oder Anode) -> *Muss geprüft werden!* (Planung geht von Standard-Logik aus).
* **Taster**: Schalten gegen GND (Common Line).

---

## 4. PCB Design Hinweise

* **MCP23017**: Reset-Pin auf VCC (High) legen. Adress-Pins (A0, A1, A2) auf GND (Addr 0x20).
* **FFC Connector**: Passenden 14-Pin FPC/FFC Connector (Pitch messen! Meist 1.0mm oder 0.5mm) einplanen.
* **ESD Schutz**: Alle Leitungen zum FFC Connector sollten mit TVS-Dioden geschützt werden, da das Panel extern bedienbar ist.
