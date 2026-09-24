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

In der Wärmerückgewinnung wechselt der Lüfter nach einer festen Zeit **pro Richtung** die Drehrichtung; diese hängt von der Stufe ab (70 s bei Stufe 1 → 50 s bei Stufe 10; z. B. Stufe 2 = 68 s, Stufe 6 = 59 s). Bei einem Stufenwechsel ändert sich diese Dauer.

Um ein abruptes Zurücksetzen oder ein vorzeitiges Umschalten der Drehrichtung zu verhindern, behält VentoSync die **relative Position innerhalb der aktuellen Richtung** bei und skaliert sie auf die neue Dauer:

$$\text{Neue verbleibende Zeit} = \text{Neue Dauer pro Richtung} \times \left(1 - \frac{\text{Verstrichene Zeit}}{\text{Alte Dauer pro Richtung}}\right)$$

* **Vorteil**: Der Lüfter setzt seine aktuelle Wärmespeicherungs- bzw. Wärmeabgabephase nahtlos fort, ohne den thermischen Regenerator aus dem Takt zu bringen. Der Master hält die Phasen aller Geräte des Raums synchron.

---

## 🌊 Sanftanlauf (Slew-Rate Limiter)

Zur Schonung der Motorelektronik und zur akustischen Optimierung werden alle Drehzahländerungen über einen Software-Slew-Rate-Limiter gefiltert.

* **Rampen-Geschwindigkeit**: **10 % der vollen Geschwindigkeit pro Sekunde** (≈ 2,8 % PWM pro Sekunde) — von Stufe 1 auf Stufe 10 in etwa 9 s.
* **Sanfte Richtungsumkehr**: Beim Richtungswechsel (Wärmerückgewinnung) sowie zu Beginn und am Ende jedes Stoßlüftungs-Durchgangs wird der Lüfter über eine sanfte 5-Sekunden-Brems- und Anlauframpe geführt.
* **Vorteil**: Verhindert Stromspitzen auf der 12V-Schiene und eliminiert störende Lastwechselgeräusche im Wohnraum.

---

## ⚙️ Virtuelle Drehzahlberechnung & Tachometer

Nicht alle verbauten Lüfter verfügen über ein physisches Tachosignal (z. B. der 3-PIN ebm-papst 4412 F/2 GLL).

* **Virtuelle Berechnung**: Ohne Tachosignal schätzt VentoSync die Drehzahl als *Geschwindigkeitsanteil × 4200 RPM* (der Geschwindigkeitsanteil folgt der nichtlinearen Stufenkurve: Stufe 1 = 10 %, Stufe 6 = 50 %, Stufe 10 = 100 %), einschließlich der 5-s-Rampen.
* **Physisches Tachosignal**: Bei einem 4-PIN-Lüfter mit Tachoausgang (z. B. AxiRev) werden die Impulse an GPIO20 (Hardware-Pulse-Counter) verwendet. Der Wert dient der Anzeige und Diagnose — die Drehzahl wird nicht geregelt (kein Closed-Loop).
* **Entität**: `sensor.lufter_drehzahl` („Lüfter Drehzahl“), negativ bei Abluft.

---

## 🔄 Klartext-Richtungsanzeige

Für eine einfache Diagnose und Überwachung der ESP-NOW-Gruppensynchronisation stellt VentoSync eine Klartext-Sensor-Entität in Home Assistant bereit:

* `text_sensor.lufter_richtung` („Lüfter Richtung“):
  * 🟢 **„Zuluft (Rein)“**
  * 🔵 **„Abluft (Raus)“**
  * ⚫ **„Stillstand“**

---

## 🌴 Urlaubsmodus (Vacation Mode)

Der Urlaubsmodus ist ein konfigurierbarer Energiesparmodus für längere Abwesenheiten und wird über einen Home-Assistant-Toggle-Helper geschaltet (Standard `input_boolean.ventosync_vacation_mode`, Substitution `vacation_sensor_id`).

### Funktionsweise
1. **Aktivierung (raumweit)**: Der **Master** (Geräte-ID 1) jedes Raums sichert seinen aktuellen Betriebsmodus und seine Lüfterstufe und schaltet den Raum in den konfigurierten Urlaubsmodus (Standard: *Stoßlüftung auf Stufe 1*). Die übrigen Geräte folgen dem Master per ESP-NOW — sie schalten nicht selbst um, sodass kein Gerät versehentlich den Urlaubszustand als „vorherigen“ Zustand sichert.
2. **Wiederherstellung**: Beim Deaktivieren stellt der Master den gesicherten Modus und die Stufe für den ganzen Raum wieder her.
3. **Ohne erreichbaren Master** schaltet und stellt ein Gerät den Urlaubsmodus selbst her. Der Urlaubszustand wird dauerhaft gespeichert (`vacation_state`); ein Neustart oder ein wiederholter Auslöser während des Urlaubs überschreibt den gesicherten Zustand nie.

> [!NOTE]
> Geräte, die sich beim Update auf 0.10.27 bereits im Urlaubsmodus befinden, kennen den laufenden Urlaub noch nicht: den Helper einmal aus- und wieder einschalten oder den Modus nach dem Urlaub manuell zurückstellen.

### Home Assistant Konfiguration
Die Parameter sind in den Geräteeinstellungen unter *Konfiguration* anpassbar (am Master einstellen — er schaltet den Raum):

| Entität | Typ | Standard | Beschreibung |
| :--- | :--- | :--- | :--- |
| `select.urlaubsmodus_betriebsmodus` | Select | `Stoßlüftung` | Zielmodus während des Urlaubs |
| `number.urlaubsmodus_intensitat` | Number | `1` | Lüfterstufe (1–10) während des Urlaubs |

> [!TIP]
> Eine vollständige Anleitung zur raumweiten Steuerung über einen zentralen Home Assistant Toggle Helper findest du im **[Home Assistant Urlaubsmodus Setup Guide](de_vacation-mode-ha-setup.md)**.

---

## 🔒 Kindersicherung (Child Protection Mode)

Die Kindersicherung verhindert versehentliche oder unerwünschte Änderungen über die physischen Tasten am Lüftungsgerät. Sie gilt pro Gerät.

### Steuerung & Bedienung

* **Via Home Assistant**:
  * Entität: `switch.kindersicherung` (in der *Konfiguration* des Geräts).
  * Die Steuerung über Home Assistant und das Web-Dashboard bleibt **vollständig uneingeschränkt möglich**.

* **Am Gerät selbst**:
  * **Aktivieren / Deaktivieren**: Die **Modus-Taste** etwa **5 Sekunden** gedrückt halten (bestätigt nach 4,5 s durchgehendem Halten).
  * **Quittierung**: Die 8 Panel-LEDs (Power, beide Modus-LEDs, 5 Stufen-LEDs) blinken **2-mal** zur Bestätigung.

* **Feedback bei gesperrtem Tastendruck**:
  * Wird bei aktiver Sperre eine Taste gedrückt, wird die Eingabe ignoriert und die LEDs blinken **3-mal** als optischer Hinweis.

### Technische Absicherung
* Der Zustand wird im Flash-Speicher gespeichert (`child_lock_active`, `restore_value: true`) und bleibt nach Stromausfall, Neustart und OTA-Update erhalten (der HA-Schalter nutzt `restore_mode: DISABLED` und überschreibt den gespeicherten Zustand beim Booten nicht mehr — behoben in 0.10.27).
* Ein Combo-Cooldown (500 ms) unterdrückt ein veraltetes Tasten-Event direkt nach dem Umschalten.
