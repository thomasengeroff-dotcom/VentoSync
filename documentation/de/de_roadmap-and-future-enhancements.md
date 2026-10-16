# 🗺️ Roadmap & Zukünftige Erweiterungen

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_roadmap-and-future-enhancements.md)


Dieses Dokument beschreibt geplante Funktionen, Architekturkonzepte und zukünftige Hardware- und Software-Erweiterungen für **VentoSync**.

---

## 📑 Übersicht der geplanten Erweiterungen

- [🎛️ Intuitive Gruppensteuerung & Single-Tile UI](#️-intuitive-gruppensteuerung--single-tile-ui)
- [🌙 Intelligenter Nacht- & Flüster-Schlafmodus](#-intelligenter-nacht--flüster-schlafmodus)
- [🏠 Abwesenheits- & Außer-Haus-Logik](#-abwesenheits--außer-haus-logik)
- [❄️ Frostschutz-Automatik](#️-frostschutz-automatik)
- [📅 Autarker Wochenzeitplan](#-autarker-wochenzeitplan)
- [🔔 Erweiterte Alarm- & Filterüberwachung](#-erweiterte-alarm--filterüberwachung)
- [🔄 Closed-Loop Drehzahlüberwachung](#-closed-loop-drehzahlüberwachung)
- [🌬️ Aerodynamische Rückwärts-Kompensation](#️-aerodynamische-rückwärts-kompensation)
- [🧠 KI-gestützte vorausschauende Lüftungssteuerung](#-ki-gestützte-vorausschauende-lüftungssteuerung)
- [🔌 Erweiterungen & Gebäudeautomations-Gateways](#-erweiterungen--gebäudeautomations-gateways)
- [❄️🔥 Intelligente Klimaanlagen-Koordination (Smart Climate Control)](#️-intelligente-klimaanlagen-koordination-smart-climate-control)

---

## 🎛️ Intuitive Gruppensteuerung & Single-Tile UI

* **Group-Controller Konzept**: Über ESP-NOW können mehrere synchronisierte Geräte in einem Raum als eine einzige visuelle Einheit im Home Assistant Dashboard (z. B. via Mushroom Cards) gesteuert werden.
* **Vorteile**: Reduziert den WLAN-Traffic drastisch, erhöht die Stabilität und vereinfacht die Bedienung für die ganze Familie (hoher WAF).
* *Ausführliche Konzepte und YAML-Beispiele findest du im Ordner [ha_integration_example/](../../ha_integration_example/).*

---

## 🌙 Intelligenter Nacht- & Flüster-Schlafmodus

* **Lichtsensor-Integration**: Automatische Aktivierung eines "Whisper-Quiet" Flüsterprofils bei Dunkelheit über einen Hardware-Dämmerungssensor (LDR oder BH1750 I2C-Sensor).
* **Flüster-Schlafmodus via mmWave**: Nutzung der Mikrobewegungserkennung des mmWave-Radars (Atmungserkennung im Bett), um im Schlafzimmer auf die unhörbarste Minimalstufe zu drosseln und Richtungswechselzyklen zu verlängern (Minimierung mechanischer Umschaltgeräusche).
* **CO2-Sicherheitsnetz**: Garantiert frische Luft, ohne die Nachtruhe zu stören.

---

## 🏠 Abwesenheits- & Außer-Haus-Logik

* **HA-Geofencing & Alarmanlagen-Kopplung**: Erkennt Home Assistant die Abwesenheit aller Bewohner (GPS-Geofencing oder Scharfschaltung der Alarmanlage), wechseln alle Geräte auf eine hygienische Grundlüftungsstufe für maximale Energieersparnis.
* **Kurzzeit-Abwesenheitsreduzierung**: Automatische Absenkung auf ein hygienisches Minimum bei leerem Raum (erkannt über den On-Board-Radarsensor).

---

## ❄️ Frostschutz-Automatik

* **Regenerator-Schutz bei Minusgraden**: Bei extremen Außentemperaturen kann Feuchtigkeit im Keramikspeicher gefrieren. Durch kontinuierliche Überwachung des äußeren NTC-Temperatursensors passt VentoSync die Zykluszeiten an oder deaktiviert kurzzeitig die Zuluft, um den Wärmetauscher aufzutauen.

---

## 📅 Autarker Wochenzeitplan

* **Autonome ESP32-Zeitschaltuhr**: Native Ausführung von Zeitplänen direkt auf dem Mikrocontroller (via SNTP-Zeitsynchronisation), um Komfortfunktionen auch bei Ausfall der zentralen Smart-Home-Zentrale oder des WLANs sicherzustellen.
* **Kollisionsschutz**: Intelligente Priorisierung stellt sicher, dass On-Device-Pläne nahtlos mit Home Assistant Automatisierungen harmonieren.

---

## 🔔 Erweiterte Alarm- & Filterüberwachung

* **Multi-Kanal-Benachrichtigung**: Optische Blinkcodes auf der Master-LED kombiniert mit Home Assistant Push-Nachrichten bei kritischen Zuständen (z. B. extreme Luftfeuchtigkeit >75% rH, Frostgefahr, Sensorausfall oder filterbetriebsstundenbasierte Reinigungserinnerungen).

---

## 🔄 Closed-Loop Drehzahlüberwachung

* **Echtzeit-Tachomessung**: Kontinuierliches Monitoring der tatsächlichen Drehzahl über den GPIO20 Pulse-Counter bei 4-PIN PWM-Lüftern (z. B. AxiRev).
* **Konstanter Volumenstrom**: Automatischer Ausgleich von Gegendruck bei starkem Wind oder zunehmender Filterverschmutzung.

---

## 🌬️ Aerodynamische Rückwärts-Kompensation

* **Asymmetrische Lüftereffizienz**: Standard-Axiallüfter (wie der ebm-papst 4412 F/2 GLL) sind aerodynamisch stark auf eine Strömungsrichtung optimiert. Bei Drehrichtungsumkehr für die Abluftphase bricht das Fördervolumen durch das asymmetrische Schaufelprofil und die Statorstege signifikant ein (oft 30–50 % Verlust).
* **Softwarebasierter Volumenstrom-Ausgleich**: Einführung eines konfigurierbaren `REVERSE_COMPENSATION_FACTOR` in der PWM-Berechnung. Der Lüfter wird in der ineffizienten Richtung automatisch mit einem berechneten Drehzahl-Offset (höheres PWM-Delta) angesteuert, um absolut identische Fördervolumina ($V_{Zuluft} \approx V_{Abluft}$) zu garantieren. Dies ist essenziell für die maximale Rückwärmezahl des Keramik-Wärmetauschers und verhindert unerwünschte Druckdifferenzen im Raum.

---

## 🧠 KI-gestützte vorausschauende Lüftungssteuerung

* **Proaktive Regelung**: Machine-Learning-Modelle und Wetterprognosen zur vorausschauenden Lüftung (z. B. vor heranziehenden Hitzewellen oder feuchtem Wetter).
* **Personenzählung**: Abschätzung der Belegungsdichte über mmWave-Mehrziel-Tracking zur bedarfsgerechten Anpassung des Luftvolumenstroms (CFM/m³).
* *Details siehe [📄 KI-gestützte Lüftungssteuerung](de_ai-powered-ventilation-control.md).*

---

## 🔌 Erweiterungen & Gebäudeautomations-Gateways

* **Mixed-Air VOC+CO2-Regelung**: Kombination von BME680 VOC/Gas-Messwerten mit Sensirion SCD4x CO2-Daten für eine ganzheitliche Luftqualitätsregelung.
* **Gebäudeleittechnik-Brücke (Modbus / KNX)**: Schnittstellen zur Einbindung von VentoSync in professionelle Gebäudeautomationssysteme zur gewerkeübergreifenden Abstimmung mit Heizung und Klimatechnik.

---

## ❄️🔥 Intelligente Klimaanlagen-Koordination (Smart Climate Control)

> ✅ **Implementiert in Version 0.10.13.** Zur Referenz hier belassen; das Konzept wurde bei der Umsetzung geprüft und verfeinert.

* **Intelligente Klima-Koordination**: Wenn eine Raumklimaanlage aktiv ist, drosselt VentoSync die Lüftung automatisch auf das Minimum, das für gesunde Raumluft (CO2-basiert) erforderlich ist — und verhindert so Energieverschwendung durch den Import heißer Außenluft. Wirkt ausschließlich als Modifikator der `Smart-Automatik`; manuelle Modi bleiben unberührt.
* **Dedizierter Aktivierungsschalter**: Der Schalter `smart_climate_control` („Klima-Koordination") ermöglicht die geräteweise Aktivierung des HVAC-Koordinations-Features.
* **Reiner CO2-Luftqualitäts-Schutz**: Wechselt von Dual-PID (CO2 + Feuchte) auf eine gelockerte CO2-only-Regelschleife (Zielwert: 1200 ppm / DIN EN 13779 IDA 3) mit harter Lüfterstufen-Obergrenze (Standard: Stufe 3) und erzwungener Wärmerückgewinnung (kein Sommer-Bypass).
* **Gesundheitsschutz**: Ein CO2-Notfall-Override (Standard 1500 ppm, Hysterese zurück auf 1200 ppm) und ein Schimmelschutz (≥ 70 % rH, solange die Außenluft trockener ist) heben die Einschränkungen automatisch auf. Ohne CO2-Messwert wird nie gedrosselt.
* **Robuste Klima-Erkennung**: 120 s Freigabeverzögerung und Fail-Safe „Klima inaktiv" bei getrenntem Home Assistant; das HA-Mapping nutzt den gewählten `hvac_mode`, nicht die taktende Kompressor-`hvac_action`.
* *Die vollständige technische Spezifikation, den Zustandsautomaten und die Konfigurations-Entitäten findest du in [📄 Intelligente Klimaanlagen-Koordination](de_smart-climate-control.md).*
