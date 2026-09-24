# 🌟 Komfort- & Sicherheitsfunktionen

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_comfort-and-safety-features.md)


Dieses Dokument beschreibt die erweiterten Steuerungs-, Komfort- und Schutzfunktionen von **VentoSync**.

---

## 📑 Inhaltsverzeichnis

- [📈 Phasen-Kontinuität & Dynamische Zyklusanpassung](#-phasen-kontinuität--dynamische-zyklusanpassung)
- [🌊 Sanftanlauf (Slew-Rate Limiter)](#-sanftanlauf-slew-rate-limiter)
- [⚙️ Virtuelle Drehzahlberechnung & Tachometer](#️-virtuelle-drehzahlberechnung--tachometer)
- [🔄 Klartext-Richtungsanzeige](#-klartext-richtungsanzeige)
- [🌴 Urlaubsmodus (Vacation Mode)](#-urlaubsmodus-vacation-mode)
- [🔒 Kindersicherung (Child Protection Mode)](#-kindersicherung-child-protection-mode)

---

## 📈 Phasen-Kontinuität & Dynamische Zyklusanpassung

Bei einem Wechsel der Lüfterstufe (z. B. von Stufe 2 auf Stufe 6 im Automatik- oder Manuellbetrieb) ändert sich die Gesamtdauer des Wärmerückgewinnungszyklus (z. B. von 65s auf 50s). 

Um ein abruptes Zurücksetzen oder ein vorzeitiges Umschalten der Drehrichtung zu verhindern, verwendet VentoSync eine **proportionale Skalierung**:

$$\text{Neue verbleibende Zeit} = \text{Neue Gesamtdauer} \times \left(1 - \frac{\text{Verstrichene Zeit}}{\text{Alte Gesamtdauer}}\right)$$

* **Vorteil**: Der Lüfter setzt seinen aktuellen Wärmespeicherungs- bzw. Wärmeabgabezyklus nahtlos und kontinuierlich fort, ohne den thermischen Regenerator aus dem Takt zu bringen.

---

## 🌊 Sanftanlauf (Slew-Rate Limiter)

Zur Schonung der Motorelektronik und zur akustischen Optimierung werden alle Drehzahländerungen über einen Software-Slew-Rate-Limiter gefiltert.

* **Rampen-Geschwindigkeit**: ca. **5 % PWM pro Sekunde**
* **Sanfte Richtungsumkehr**: Beim Richtungswechsel (Wärmerückgewinnung) sowie zu Beginn und am Ende jedes Stoßlüftungs-Durchgangs wird der Lüfter über eine sanfte 5-Sekunden-Brems- und Anlauframpe geführt.
* **Vorteil**: Verhindert Stromspitzen auf der 12V-Schiene und eliminiert störende Lastwechselgeräusche im Wohnraum.

---

## ⚙️ Virtuelle Drehzahlberechnung & Tachometer

Nicht alle verbauten Lüfter verfügen über ein physisches Tachosignal (z. B. der 3-PIN ebm-papst 4412 F/2 GLL). 

* **Virtuelle Berechnung**: Für 3-PIN-Lüfter ohne Tachosignal berechnet VentoSync anhand der nichtlinearen Ventomaxx-Kennlinie die zu erwartende Drehzahl (bis zu 4200 RPM @ 100 %).
* **Physisches Tachosignal**: Bei Verwendung moderner 4-PIN-Lüfter (z. B. AxiRev) wird das Tachosignal über GPIO20 (Pulse Counter) in Echtzeit erfasst und für Closed-Loop-Überwachung bereitgestellt.

---

## 🔄 Klartext-Richtungsanzeige

Für eine einfache Diagnose und Überwachung der ESP-NOW-Gruppensynchronisation stellt VentoSync eine Klartext-Sensor-Entität in Home Assistant bereit:

* `sensor.luefter_richtung` / `sensor.fan_direction`:
  * 🟢 **„Zuluft (Rein)“** / `Supply Air (In)`
  * 🔵 **„Abluft (Raus)** / `Exhaust Air (Out)`
  * ⚫ **„Stillstand“** / `Standstill`

---

## 🌴 Urlaubsmodus (Vacation Mode)

Der Urlaubsmodus ist ein konfigurierbarer Energiesparmodus für längere Abwesenheiten.

### Funktionsweise
1. **Zustandsspeicherung**: Beim Aktivieren sichert VentoSync den vorherigen Betriebsmodus und die Lüfterstufe aller Geräte im Raum.
2. **Umschaltung**: Alle synchronisierten Geräte wechseln in den konfigurierten Urlaubsmodus (Standard: *Stoßlüftung auf Stufe 1*).
3. **Wiederherstellung**: Nach Deaktivierung des Urlaubsmodus kehren alle Geräte automatisch in ihren vorherigen Zustand zurück.

### Home Assistant Konfiguration
Die Parameter sind direkt in den Home Assistant Geräteeinstellungen unter *Konfiguration* anpassbar:

| Entität | Typ | Standard | Beschreibung |
| :--- | :--- | :--- | :--- |
| `select.urlaubsmodus_betriebsmodus` | Select | `Stoßlüftung` | Zielmodus während des Urlaubs |
| `number.urlaubsmodus_intensitat` | Number | `1` | Lüfterstufe (1–10) während des Urlaubs |

> [!TIP]
> Eine vollständige Anleitung zur raumweiten Steuerung über einen zentralen Home Assistant Toggle Helper findest du im **[Home Assistant Urlaubsmodus Setup Guide](de_vacation-mode-ha-setup.md)**.

---

## 🔒 Kindersicherung (Child Protection Mode)

Die Kindersicherung verhindert versehentliche oder unerwünschte Änderungen über die physischen Tasten am Lüftungsgerät.

### Steuerung & Bedienung

* **Via Home Assistant**:
  * Entität: `switch.kindersicherung` (in der *Konfiguration* des Geräts).
  * Das Steuern über Home Assistant bleibt bei aktiver Kindersicherung **vollständig uneingeschränkt möglich**.

* **Am Gerät selbst**:
  * **Aktivieren / Deaktivieren**: **Modus-** und **Stufen-Taste** für **5 Sekunden gleichzeitig gedrückt halten**.
  * **Quittierung**: Alle 9 LEDs blinken **2-mal** zur Bestätigung des neuen Status.

* **Feedback bei gesperrtem Tastendruck**:
  * Wird bei aktiver Sperre eine Taste gedrückt, wird die Eingabe ignoriert und alle LEDs blinken **3-mal** als optischer Hinweis.

### Technische Absicherung
* Der Zustand wird im Flash-Speicher (NVS) gespeichert (`restore_value: true`) und bleibt auch nach einem Stromausfall oder Neustart erhalten.
* Ein integrierter Combo-Cooldown (500 ms) verhindert unbeabsichtigte Tastenklicks direkt nach dem Entsperren.
