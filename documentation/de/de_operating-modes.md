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
| **1** | **🤖 Smart-Automatik** *(Standard)* | 🟢 *(pulsiert)* / ⚫ | Dynamischer PID (Stufen 1–10) basierend auf CO2 & Feuchte | 50s – 70s dynamisch | `select.luftermodus` → `Smart-Automatik` |
| **2** | **❄️ Wärmerückgewinnung** *(Eco)* | 🟢 / ⚫ | Konstante manuelle Stufe (1–10) mit Pendellüftung / Wärmetausch | 50s – 70s dynamisch | `select.luftermodus` → `Wärmerückgewinnung` |
| **3** | **🌬️ Durchlüften** *(Sommer)* | 🟢 / 🟢 | Konstanter Luftstrom ohne Richtungswechsel (Phase A rein, Phase B raus) | Dauerhaft / Timer | `select.luftermodus` → `Durchlüften` |
| **4** | **💨 Stoßlüftung** | ⚫ / 🟢 | Lüften in eine Richtung auf der manuellen Stufe (Phase A rein, Phase B raus), danach Pause; Richtung jeden zweiten Durchgang getauscht | 2-h-Zyklus (15 min Lüften / 105 min Pause) | `select.luftermodus` → `Stoßlüftung` |
| **5** | **⭕ Aus** *(Monitoring)* | ⚫ / ⚫ | Lüfter gestoppt (0 RPM), raumweit; alle Sensoren, WLAN & Web-UI bleiben voll aktiv | — | `select.luftermodus` → `Aus` |

---

## ⚙️ Detaillierte Modusbeschreibungen

### 1. 🤖 Smart-Automatik *(Standard / Empfohlen)* — `LED_WRG` 🟢 (pulsiert langsam)

**Dieser Modus ist der Standard nach dem Einschalten** und übernimmt alle Lüftungsaufgaben vollkommen autonom („Einstellen und vergessen“). Das System regelt sich kontinuierlich anhand der Umweltsensordaten von Innen- und Außenbereich.

#### Aktive Smart-Funktionen

| Funktion | Sensor(en) | Schwellwert / Regelmethode |
| :--- | :--- | :--- |
| ✅ **CO2-Regelung (PID)** | SCD43 (`sensor.scd41_co2`) | `number.smart_automatik_co2_grenzwert` (Sollwert, z.B. 1000 ppm) |
| ✅ **Feuchtemanagement (PID)** | SCD43 (`sensor.scd41_luftfeuchtigkeit`) + HA `sensor.outdoor_humidity` | Entfeuchtung via Enthalpie-Check (absolute Feuchte) |
| ✅ **Sommerkühlung** | NTC-Sensoren + ESP-NOW Gruppentemperatur + HA `binary_sensor.sommerbetrieb` | Raumtemperatur-Schwelle per Slider (Standard 22°C), außen ≥ 1,5°C kühler |
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
6. **Raumweite Bedarfsfusion:** Jedes Gerät sendet per ESP-NOW den Bedarf seiner **eigenen** CO2- und Feuchte-PIDs (ohne Sensoren → kein Wert). Jedes Gerät regelt auf das **Maximum** aus lokalem Bedarf und dem aktuellen (≤ 5 min alten) Bedarf aller Peers. Ein Gerät ohne eigene Sensoren (Varianten `nosensor`, `radar_only`, `NTConly` — z. B. ein Master ohne SCD41) folgt damit dem Sensorgerät mit vollem PID-Verhalten, für CO2 **und** Luftfeuchtigkeit. Übernommene Werte werden nie weitergesendet, ein hoher Bedarf kann sich daher nicht zwischen Geräten „festhalten“.

> [!TIP]
> Für die vollständigen technischen Hintergründe und die C++ Implementierung siehe **[📄 smart-automatic-logic.md](de_smart-automatic-logic.md)** und **[📄 humidity-management.md](de_humidity-management.md)**.

---

### 2. ❄️ Wärmerückgewinnung (Eco Recovery) — `LED_WRG` 🟢 (dauerhaft an)

- **HA-Entität:** `select.luftermodus` → `Wärmerückgewinnung`
- **Funktion:** Manueller Wärmerückgewinnungsbetrieb ohne automatische PID-Skalierung. Die Drehrichtung wechselt periodisch, der Keramikspeicher gewinnt die Wärme der Abluft zurück (Herstellerangabe bis zu 85 %; den tatsächlichen Wert misst die Firmware mit den NTC-Sensoren als `sensor.wrg_effizienz` („WRG Effizienz"), siehe [Wärmerückgewinnungs-Effizienz](de_heat-recovery-and-efficiency.md)).
- **Lüfterstufe:** Konstante manuelle Stufe (1–10), änderbar über die +/- Tasten, die HA-Fan-Entität oder das Dashboard. Beim Wechsel aus der Smart-Automatik bleibt die zuletzt von der Automatik gesetzte Stufe als Ausgangswert erhalten.
- **Richtungsintervall:** Die Dauer pro Luftrichtung hängt von der Lüfterstufe ab — `round(70 − (Stufe − 1) · 20/9)` Sekunden:

  | Stufe | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  | :--- | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
  | Sekunden pro Richtung | 70 | 68 | 66 | 63 | 61 | 59 | 57 | 54 | 52 | 50 |

  Ein vollständiger Push-Pull-Zyklus (hinein **und** hinaus) dauert doppelt so lang (140 s … 100 s). Jede Richtungsphase beginnt mit einem 5-s-Sanftanlauf und endet mit einem 5-s-Auslauf; mit voller Drehzahl läuft der Lüfter also das Intervall minus 10 s.
- **Synchronisierung:** Gerätepaare arbeiten im Push-Pull-Verfahren (Phase A fördert Frischluft hinein, während Phase B verbrauchte Luft absaugt), wodurch der Raumdruck ausgeglichen bleibt. Der Master (Geräte-ID 1) hält Richtungsphase und Lüfterstufe aller Geräte im Raum per ESP-NOW gleich.
- **Anwesenheits-Anpassung:** Mit dem Slider `number.radar_lufter_anpassung` („Radar Lüfter-Anpassung", -5 … +5, `0` = aus) wird die Stufe **verschoben, solange Anwesenheit erkannt wird** — z. B. `+2` für mehr Luft bei Anwesenheit, `-2` für leiseren Betrieb bei Anwesenheit. Ohne Anwesenheit wird nichts angewendet. Die Erkennung gilt **raumweit**: Sobald ein beliebiges Gerät des Raums mit LD2450-Radar (Variante `full` / `radar_only`) eine Person erkennt, wenden alle Geräte denselben Versatz an — Zu- und Abluft bleiben ausgeglichen. Die Anwesenheit wird nach der letzten Erkennung 30 s gehalten. Die Anpassung gilt in allen manuellen Modi (Wärmerückgewinnung, Durchlüften, Stoßlüftung), nie in der Smart-Automatik; die Panel-LEDs zeigen weiterhin die Grundstufe.

---

### 3. 🌬️ Durchlüften (Sommerbetrieb) — `LED_WRG` 🟢 + `LED_VEN` 🟢 (dauerhaft an)

- **HA-Entität:** `select.luftermodus` → `Durchlüften` + `number.durchluften_dauer_min` („Durchlüften Dauer (min)", 0–120 min in 5-min-Schritten, Standard 30, **0 = Dauerbetrieb**)
- **Funktion:** Konstanter unidirektionaler Luftstrom ohne Richtungswechsel (keine 5-s-Richtungsrampen).
- **Betrieb:** Phase-A-Geräte ziehen kontinuierlich Außenluft ein, während Phase-B-Geräte Innenluft ausblasen. Dadurch entsteht ein Querlüftungseffekt zur passiven Nachtkühlung.
- **Lüfterstufe:** Manuelle Stufe (1–10); die raumweite Anwesenheits-Anpassung wirkt (siehe Wärmerückgewinnung).
- **Timer:** Der Timer startet mit der Auswahl des Modus. Nach Ablauf kehrt der Raum in die **Wärmerückgewinnung** zurück — HA-Auswahl, Fan-Preset und Panel-LEDs wechseln entsprechend auf `Wärmerückgewinnung`. Mit `0` läuft der Modus, bis ein anderer Modus gewählt wird. Eine Timer-Änderung während des Betriebs gilt ab dem ursprünglichen Start.
- **Automatischer Trigger (nur Smart-Automatik):** Die Smart-Automatik schaltet selbstständig auf dauerhaftes Durchlüften (ohne Timer; das Panel zeigt weiter die pulsierende `LED_WRG`), wenn **alle** Bedingungen erfüllt sind:
  - der HA-Binärsensor `binary_sensor.sommerbetrieb` ist `on` (HA-Template: April–Oktober **und** außen > 18 °C; ohne HA gilt er als aus → kein Bypass),
  - Raumtemperatur > Slider „Smart-Automatik: Sommerkühlung Schwelle" (18–30 °C, Standard **22 °C**),
  - Außentemperatur mindestens **1,5 °C** unter der Raumtemperatur,
  - die Klima-Koordination meldet keine aktive Klimaanlage (bei laufender Klimaanlage wird Wärmerückgewinnung erzwungen).

  Zurück in die Wärmerückgewinnung geht es, wenn `sommerbetrieb` ausgeht, außen ≥ innen − 0,5 °C wird oder innen < Schwelle − 0,5 °C fällt. Eine Uhrzeit-Bedingung gibt es nicht — die „Nachtkühlung" ergibt sich aus der Temperaturbedingung.
- **Temperaturen während des Durchlüftens:** Bei konstantem Luftstrom kann jedes Gerät nur einen seiner beiden NTCs messen (ansaugende Geräte: außen, ausblasende Geräte: innen). Der fehlende Wert kommt von einem Peer des Raums, sonst wird die letzte eigene Messung bis zu 30 min verwendet. Ist gar keine Temperatur verfügbar (z. B. einzelnes Phase-B-Gerät), kehrt die Automatik für mindestens 5 min in die Wärmerückgewinnung zurück, misst beide Temperaturen neu und darf erst dann wieder in den Bypass wechseln.

---

### 4. 💨 Stoßlüftung — `LED_VEN` 🟢 (dauerhaft an)

- **HA-Entität:** `select.luftermodus` → `Stoßlüftung`
- **Funktion:** Intervalllüftung für schnellen Luftaustausch (z. B. nach dem Kochen oder Duschen). Läuft, bis ein anderer Modus gewählt wird.
- **2-Stunden-Ablauf:**
  - **15 Minuten Lüften:** Der Lüfter läuft in **eine Richtung** — wie bei `Durchlüften` blasen Geräte mit Phase A hinein, Geräte mit Phase B saugen ab. Während des Durchgangs gibt es keinen Wechselbetrieb: Die Luft verlässt den Raum direkt, dadurch wird mehr Luft ausgetauscht und Feuchte besser abgeführt als bei der Wärmerückgewinnung (während des Durchgangs keine Wärmerückgewinnung).
  - **105 Minuten Pause:** Lüfter steht (0 RPM), der Keramikkern regeneriert sich.
  - **Sanfter Anlauf / Auslauf:** 5-Sekunden-Rampe zu Beginn und am Ende jedes Durchgangs.
- **Stufe:** Der Durchgang läuft auf der **manuell eingestellten Lüfterstufe** (`number.lufter_intensitat`, 1–10) plus dem Radar-Versatz — eine eigene Stoßlüftungs-Stufe gibt es nicht. Für einen intensiven Durchgang eine hohe Stufe wählen; der Urlaubsmodus nutzt den Modus bewusst auf Stufe 1.
- **Wechselnde Richtung:** Jeder zweite Durchgang tauscht die Richtung (Phase A saugt ab, Phase B bläst hinein), damit Keramikkerne und beide Gebäudeseiten gleichmäßig belastet werden. Die Richtung wechselt nur in der Pause, nie unter Last.
- **Raum-Synchronisation:** Der Master (Geräte-ID 1) teilt mit jedem Heartbeat seine Position im 4-Stunden-Ablauf (zwei Durchgänge); alle Geräte des Raums pausieren und lüften gemeinsam, Zu-/Abluft-Paare laufen immer gegengleich — auch nach dem Neustart eines Geräts.
- **Hinweis Winter:** Ohne Wärmerückgewinnung saugt die Zuluftseite 15 von 120 Minuten Außenluft an. Wer das nicht möchte (z. B. im Urlaub im Winter), nutzt stattdessen `Wärmerückgewinnung` (Urlaub: `select.urlaubsmodus_betriebsmodus`).

---

### 5. ⭕ Aus (Monitoring-Modus) — beide Modus-LEDs ⚫

- **HA-Entität:** `select.luftermodus` → `Aus` (oder HA-Fan-Entität *aus*)
- **Funktion:** Der Lüftermotor steht (50 % PWM = Stillstand, 0 RPM). Nur die Power-LED leuchtet (nach 30 s gedimmt).
- **Raumweit:** Wie jeder andere Modus gilt `Aus` für den **ganzen Raum** — Ausschalten an einem beliebigen Gerät (HA, Web-Dashboard, Modus- oder Power-Taste) stoppt alle Geräte des Raums, Einschalten an einem beliebigen Gerät startet sie alle wieder.
- **Aktive Sensoren:** WLAN, Home-Assistant-API, Web-Dashboard, ESP-NOW und alle Sensoren (SCD43, BMP390, BME680, Radar, NTCs) bleiben für lückenlose Messwerterfassung aktiv; das Gerät teilt seine Sensordaten weiter mit dem Raum.
- **Power-Taste:** Ein Druck (< 10 s) schaltet zwischen `Aus` und dem **zuletzt aktiven Modus** (Standard `Smart-Automatik`) um — raumweit. Das *Einschalten* der HA-Fan-Entität stellt denselben Modus wieder her. Ein sehr langer Druck (> 10 s) startet das Gerät neu; der Modus bleibt erhalten.
- **Kein Schlafmodus:** Die Hardware (Platine Rev. 1) kann den ESP32 nicht per Taste aus dem Deep Sleep wecken, daher gibt es keinen Energiespar-Schlafzustand. Der frühere lange Druck (> 5 s, WLAN aus) wurde in 0.10.26 entfernt — er sparte kaum etwas (CPU, Sensoren und Radar liefen weiter) und endete nach 15 Minuten von selbst.

---

## 🔗 Weiterführende Dokumentation

- **[📄 Bedienungsanleitung Lüftungsgerät](de_control-panel-operation.md)** — Tastenfunktionen, LED-Helligkeitsstufen und Blink-Fehlercodes.
- **[📄 Smart-Automatik Modus (Auto-Logik)](de_smart-automatic-logic.md)** — Architekturdetails und C++ Zustandsmaschine.
- **[📄 Feuchtemanagement & HA Sensor-Setup](de_humidity-management.md)** — Formeln für absolute Feuchte und Template-Sensoren.
- **[📄 ESP-NOW Kommunikation](de_esp-now-communication.md)** — Raumgruppen-Discovery und Unicast-Synchronisierung.
