# ⚡ Breadboard Power-Checkliste

Diese Checkliste hilft dir, das Problem mit der instabilen Spannungsversorgung zu finden. Das Symptom "ESP läuft an USB, aber nicht am Traco" deutet meist auf **Verkabelungs-Widerstände** oder **fehlende Pufferung** für die WiFi-Lastspitzen hin.

### 1. Masse-Verbindung (Ground)

Der häufigste Fehler.

- [x] **Common Ground**: Sind **alle** GND-Pins verbunden?
  - GND vom 12V Eingang
  - GND vom Traco TSR 1-2450
  - GND vom ESP32 (XIAO)
  - GND vom LED-Ring
  - *Test*: Messung Durchgangsprüfer (Piepser) zwischen Minus-Pol Netzteil und dem Metallgehäuse des USB-Ports am XIAO (das ist auch Masse). Es muss piepen.

### 2. Kondensatoren (Pufferung)

Der ESP32 zieht beim WiFi-Start kurzzeitig ~400mA Stromspitzen. Ohne Puffer bricht die Spannung ein.

- [x] **Eingangs-Kondensator (12V Seite)**: Ist vor dem Traco ein Elko (z.B. 10µF - 470µF)?
  - *Warum?* Stabilisiert den Eingang des Wandlers.
- [x] **Ausgangs-Kondensator (5V Seite)**: Ist **direkt** am Ausgang des Traco ein Elko (mind. 10µF)?
- [x] **Lokaler Puffer am ESP (WICHTIG)**: Hast du einen Kondensator (10µF - 100µF) **direkt** an den 5V/GND Pins des XIAO gesteckt?
  - *Problem*: Auf Breadboards haben die langen Steckbrücken Widerstand. Wenn der ESP Strom zieht, fällt Spannung über dem Kabel ab. Ein Kondensator direkt am ESP puffert das weg.
- [x] **Keramik-Kondensator (HF)**: Ein 100nF Kerko parallel zum Elko filtert hochfrequente Störungen.

### 3. Verkabelung & Kontakte

Breadboards leiern aus und Jumper-Kabel haben oft schlechten Kontakt.

- [ ] **Kabel-Test**: Wackel mal vorsichtig an den Kabeln zur Stromversorgung. Startet der ESP neu? -> Wackelkontakt.
- [ ] **5V Pin statt 3V3**: Geht der Ausgang des Traco (5V) sicher in den "5V" Pin des XIAO und **nicht** in den "3V3" Pin?
  - *Achtung*: Wenn du 5V in den 3V3 Pin speist, wird der Chip heiß und instabil (oder stirbt).
- [ ] **Kabel-Querschnitt**: Nutzt du für die Stromleitungen (12V und 5V) vernünftige Kabel und nicht die ganz dünnen, billigen Jumper-Kabel? Versuche ggf. zwei Kabel parallel für 5V und GND.

### 4. Last-Test (Ausschlussverfahren)

Zieht vielleicht ein anderes Bauteil die Spannung runter?

- [ ] **Alles abstecken**: Ziehe den LED-Ring und die Sensoren ab (oder deren VCC Leitung).
- [ ] **Nur ESP + Power**: Versorge nur den ESP über den Traco. Läuft er stabil?
  - *Wenn JA*: Stecke nach und nach Komponenten wieder an. Der LED-Ring (WS2812) zieht beim Start kurz viel Strom, wenn er nicht softwareseitig begrenzt ist.
  - *Wenn NEIN*: Miss die Spannung (siehe unten).

### 5. Messungen (Multimeter)

Miss die Spannung **während** der ESP versucht zu booten.

- [ ] **Messpunkt 1 (Traco Out)**: Miss direkt an den Beinchen des Traco Wandlers.
  - *Soll*: Konstant 5.0V.
  - *Ist*: 4,95V bis 5,00V
- [ ] **Messpunkt 2 (XIAO In)**: Miss direkt an den Pins 5V und GND des XIAO Moduls.
  - *Soll*: 5.0V (bzw. mind. 4.5V).
  - *Ist*: 4,23V bis 4,85V
  - *Differenz*: Wenn du bei 1. 5V misst und bei 2. nur 4.2V, verlierst du 0.8V über das Kabel/Breadboard -> **Dickere Kabel / Kondensator am ESP nötig.**

---

### Schnelle Lösungsmethoden

1. **Elko direkt an den ESP**: Steck einen 100µF Elko direkt in die Breadboard-Reihen vom ESP (5V und GND). Das hilft in 90% der Fälle.
2. **LED-Ring Power trennen**: Teste erst ohne LEDs.
