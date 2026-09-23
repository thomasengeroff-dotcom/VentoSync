# 🔄 Betriebsmodi & Programmlogik

[![Language: EN](https://img.shields.io/badge/Language-EN-blue.svg)](../en/en_operating-modes.md)

VentoSync bietet 5 verschiedene Betriebsmodi, um Raumluftqualität, thermische Effizienz, akustischen Komfort und passive Kühlung optimal auszubalancieren. Das Gerät kann über die physische **Modus-Taste (M)** am Gerät, das lokale Web-Dashboard oder über Home Assistant gesteuert werden.

---

## 🔘 Umschalten der Modi & Tastenabfolge

Durch Drücken der **Modus-Taste (M)** werden die Programme in folgender Reihenfolge durchgeschaltet:

```text
Smart-Automatik ──► Wärmerückgewinnung (Eco) ──► Durchlüften (Sommer) ──► Stoßlüftung ──► Aus ──► Smart-Automatik...
```

Nach dem ersten Einschalten oder einem Microcontroller-Reset ist standardmäßig **Modus 1 (Smart-Automatik)** aktiv.

---

## 📊 Übersicht der Betriebsmodi

| # | Modus | Panel-LEDs (`WRG` / `VEN`) | Lüfterverhalten | Zykluszeit | HA-Entität / Auswahl |
| :-: | :--- | :---: | :--- | :--- | :--- |
| **1** | **🤖 Smart-Automatik** *(Standard)* | 🟢 *(pulsiert)* / ⚫ | Dynamischer PID (Stufen 1–10) basierend auf CO2 & Feuchte | 50s – 70s dynamisch | `select.luefter_modus` → `Smart-Automatik` |
| **2** | **❄️ Wärmerückgewinnung** *(Eco)* | 🟢 / ⚫ | Konstante manuelle Stufe (1–10) mit Pendellüftung / Wärmetausch | 50s – 70s dynamisch | `select.luefter_modus` → `Wärmerückgewinnung` |
| **3** | **🌬️ Durchlüften** *(Sommer)* | 🟢 / 🟢 | Konstanter Luftstrom ohne Richtungswechsel (Phase A rein, Phase B raus) | Dauerhaft / Timer | `select.luefter_modus` → `Durchlüften` |
| **4** | **💨 Stoßlüftung** | ⚫ / 🟢 | Intensive Lüftung (15 min Betrieb, 105 min Pause) | Dauerhaft (15 min) | `select.luefter_modus` → `Stoßlüftung` |
| **5** | **⭕ Aus** *(Monitoring)* | ⚫ / ⚫ | Lüfter gestoppt (0 RPM); alle Sensoren & Web-UI bleiben voll aktiv | — | `select.luefter_modus` → `Aus` |

---

## ⚙️ Detaillierte Modusbeschreibungen

### 1. 🤖 Smart-Automatik *(Standard / Empfohlen)* — `LED_WRG` 🟢 (pulsiert langsam)

**Dieser Modus ist der Standard nach dem Einschalten** und übernimmt alle Lüftungsaufgaben vollkommen autonom („Einstellen und vergessen“). Das System regelt sich kontinuierlich anhand der Umweltsensordaten von Innen- und Außenbereich.

#### Aktive Smart-Funktionen

| Funktion | Sensor(en) | Schwellwert / Regelmethode |
| :--- | :--- | :--- |
| ✅ **CO2-Regelung (PID)** | SCD43 (`sensor.scd41_co2`) | `number.auto_co2_threshold` (Sollwert, z.B. 1000 ppm) |
| ✅ **Feuchtemanagement (PID)** | SCD43 (`sensor.scd41_humidity`) + HA `sensor.outdoor_humidity` | Entfeuchtung via Enthalpie-Check (absolute Feuchte) |
| ✅ **Sommerkühlung** | NTC-Sensoren + ESP-NOW Gruppentemperatur | 22°C Raumtemperaturschwelle |
| ✅ **Gruppen-Unicast-Sync** | ESP-NOW | Synchronisiert Lüfterstufen und Bedarfsanforderungen aller Geräte im Raum |

#### Logik im Detail

- **Grundbetrieb:** Kontinuierliche Wärmerückgewinnung (`MODE_ECO_RECOVERY`) auf der konfigurierten Mindest-Lüfterstufe (`automatik_min_luefterstufe`, Standard: Stufe 2). Die Reversierintervalle passen sich dynamisch an die Lüfterdrehzahl an (70s bei Stufe 1 bis 50s bei Stufe 10).
- **🎛️ Intelligente PID-Regelung (CO2 & Feuchte):** Anstelle abrupter Schwellwertschaltungen verwendet VentoSync einen doppelten PID-Regelkreis:

  > **Was ist ein PID-Regler?**
  > Stell dir vor, du fährst Auto: Bist du nur knapp über dem Tempolimit, nimmst du kaum Gas raus. Bist du weit drüber, bremst du stärker. Und wenn du schon längere Zeit knapp drüber bist, drückst du etwas mehr auf die Bremse. VentoSync funktioniert mit CO2 und Luftfeuchte genauso — kein abruptes Schalten, sondern sanftes, kontinuierliches Nachregeln.

  - **P (Proportional):** Reagiert sofort auf Abweichungen oberhalb des Schwellwerts.
  - **I (Integral):** Summiert langanhaltende Abweichungen langsam auf (z.B. mehrere Personen im Raum) und steigert die Lüfterstufe sanft über die Zeit.
  - **Sanftes Tuning:** Der I-Gain ist extrem träge abgestimmt (`0.0000005`), um kurzzeitige Spitzen (z.B. Öffnen einer Mineralwasserflasche) zu ignorieren.

#### Praxisbeispiel (CO2-Zielwert: 800 ppm, Stufenbereich: 2–7)

| Verstrichene Zeit | CO2-Messwert | Aktion & Lüfterreaktion |
| :--- | :--- | :--- |
| **0 min** | 820 ppm | Geringe Abweichung (+20 ppm) → P-Bedarf minimal → **Lüfter bleibt auf Stufe 2 (Min)** |
| **15 min** | 870 ppm | Erhöht (+70 ppm), Integral baut sich langsam auf → **Lüfter bleibt auf Stufe 2** |
| **30 min** | 920 ppm | Anhaltende Abweichung (+120 ppm), Integral akkumuliert → **Lüfter regelt sanft auf Stufe 3** |
| **50 min** | 960 ppm | Kontinuierlicher Bedarf → **Lüfter regelt auf Stufe 4** |
| **70 min** | 900 ppm | Luftqualität verbessert sich, Integral baut ab → **Lüfter regelt zurück auf Stufe 3** |
| **90 min** | 790 ppm | Unter Schwellwert → Bedarf fällt auf Null → **Lüfter kehrt zu Stufe 2 (Min) zurück** |

#### Wichtige Verhaltensregeln

1. **Ramping-Begrenzung:** Die Lüfterdrehzahl ändert sich um **maximal ±1 Stufe pro 10-Sekunden-Zyklus**, um hörbare Drehzahlsprünge zu verhindern.
2. **Grenzen-Einhaltung:** Die Stufe unterschreitet nie `automatik_min_luefterstufe` (Stufe 2) und überschreitet nie `automatik_max_luefterstufe` (standardmäßig Stufe 7).
3. **Signal-Arbitrierung:** Das System wählt das **Maximum** aus CO2- und Feuchtebedarf, damit kein Luftqualitätsparameter vernachlässigt wird.
4. **Sanfter Einstieg:** Beim Umschalten *in* die Smart-Automatik werden die PID-Integrale zurückgesetzt, damit das Gerät stets mit der Minimalstufe startet und nur bei tatsächlichem Bedarf hochregelt.
5. **Absolutfeuchte-Schutz:** Eine Entfeuchtung erhöht die Drehzahl nur, wenn die Außenluft absolut trockener ist als die Innenluft (Magnus-Formel). Ist die Außenluft feuchter (z.B. bei Regen), wird der Feuchtebedarf auf 0 gesetzt.
6. **Gruppen-Fallback:** Hat ein Gerät keine eigenen Sensoren (Varianten `nosensor`, `radar_only`, `NTConly`), übernimmt es automatisch den höchsten Bedarf aus der Raumgruppe via ESP-NOW.

> [!TIP]
> Für die vollständigen technischen Hintergründe und die C++ Implementierung siehe **[📄 smart-automatic-logic.md](de_smart-automatic-logic.md)** und **[📄 humidity-management.md](de_humidity-management.md)**.

---

### 2. ❄️ Wärmerückgewinnung (Eco Recovery) — `LED_WRG` 🟢 (dauerhaft an)

- **HA-Entität:** `select.luefter_modus` → `Wärmerückgewinnung`
- **Funktion:** Manueller Wärmerückgewinnungsbetrieb ohne automatische PID-Skalierung. Die Drehrichtung wechselt periodisch und gewinnt bis zu 85% der Wärmeenergie zurück.
- **Zykluszeiten:** Passen sich dynamisch an die gewählte Lüfterstufe an:
  - Stufe 1: **70 Sekunden**
  - Stufe 5: **60 Sekunden**
  - Stufe 10: **50 Sekunden**
- **Synchronisierung:** Gerätepaare arbeiten im Push-Pull-Verfahren (Phase A fördert Frischluft hinein, während Phase B verbrauchte Luft absaugt), wodurch der Raumdruck ausgeglichen bleibt.
- **Präsenz-Boost:** Bei aktivierter Radar-Präsenzerkennung kann die Stufe bei Anwesenheit optional um `-5` bis `+5` Stufen angepasst werden.

---

### 3. 🌬️ Durchlüften (Sommerbetrieb) — `LED_WRG` 🟢 + `LED_VEN` 🟢 (dauerhaft an)

- **HA-Entität:** `select.luefter_modus` → `Durchlüften` + `number.vent_timer` (Timer, 0 = unbegrenzt)
- **Funktion:** Konstanter unidirektionaler Luftstrom ohne Richtungswechsel.
- **Betrieb:** Phase-A-Geräte ziehen kontinuierlich Außenluft ein, während Phase-B-Geräte Innenluft ausblasen. Dadurch entsteht ein Querlüftungseffekt zur passiven Nachtkühlung.
- **Automatischer Trigger:** Im Smart-Automatik Modus schaltet das System in Sommernächten automatisch auf Durchlüften, wenn die Raumtemperatur über 22°C liegt und die Außenluft um mindestens 1.5°C kühler ist.

---

### 4. 💨 Stoßlüftung — `LED_VEN` 🟢 (dauerhaft an)

- **HA-Entität:** `select.luefter_modus` → `Stoßlüftung`
- **Funktion:** Intensive Intervalllüftung für schnellen Luftaustausch (z.B. nach dem Kochen oder Duschen).
- **2-Stunden-Ablauf:**
  - **15 Minuten:** Lüftung mit hoher Intensität auf der konfigurierten Boost-Stufe.
  - **105 Minuten:** Pause (0 RPM), damit sich der Keramikkern regenerieren und Feuchtigkeit abbauen kann.
  - **Wiederholung:** Wiederholt sich automatisch alle 2 Stunden, bis der Modus beendet wird.
- **Wechselnde Startrichtung:** Jeder Stoßlüftungszyklus wechselt die Startrichtung, um die thermische Balance des Keramikkerns zu erhalten.

---

### 5. ⭕ Aus (Monitoring-Modus) — beide LEDs ⚫

- **HA-Entität:** `select.luefter_modus` → `Aus`
- **Funktion:** Lüftermotor und PWM-Ansteuerung sind komplett abgeschaltet (0 RPM).
- **Aktive Sensoren:** Umweltsensoren (SCD43 CO2/Temp/Feuchte, BMP390, BME680, Radar-Präsenz) sowie das lokale Web-Dashboard bleiben für lückenlose Messwerterfassung in Home Assistant aktiv.
- **Ultra-Low-Power Light Sleep:** Langes Drücken der physischen Power-Taste für **> 5s** versetzt das Gerät in den Deep-Light-Sleep (deaktiviert WLAN, LEDs und Radar; Leistungsaufnahme < 0.1W). Ein kurzer Tastendruck weckt das Gerät sofort wieder auf und verbindet es erneut mit dem Netzwerk.

---

## 🔗 Weiterführende Dokumentation

- **[📄 Bedienungsanleitung Lüftungsgerät](de_control-panel-operation.md)** — Tastenfunktionen, LED-Helligkeitsstufen und Blink-Fehlercodes.
- **[📄 Smart-Automatik Modus (Auto-Logik)](de_smart-automatic-logic.md)** — Architekturdetails und C++ Zustandsmaschine.
- **[📄 Feuchtemanagement & HA Sensor-Setup](de_humidity-management.md)** — Formeln für absolute Feuchte und Template-Sensoren.
- **[📄 ESP-NOW Kommunikation](de_esp-now-communication.md)** — Raumgruppen-Discovery und Unicast-Synchronisierung.
