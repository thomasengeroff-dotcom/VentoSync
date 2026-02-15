# 🤖 CO2-gesteuerte adaptive Lüfterregelung

## Übersicht

Die CO2-Automatik passt die Lüfterintensität automatisch an den aktuellen CO2-Gehalt der Raumluft an.
Wenn der CO2-Wert steigt, erhöht sich die Lüfterdrehzahl schrittweise — sinkt der Wert, wird dezent zurückregeregelt.
Eine einstellbare **Mindest-Lüfterstufe** stellt sicher, dass auch bei Abwesenheit (niedriges CO2)
eine Grundlüftung zum Feuchteschutz aktiv bleibt (nach DIN 1946-6).

**Voraussetzung:** SCD41 CO2-Sensor muss am I2C-Bus (Header H2) angeschlossen sein.
Ohne angeschlossenen Sensor bleibt die Funktion automatisch inaktiv.

---

## Schwellwerte (DIN EN 13779 / Umweltbundesamt)

Die Grenzwerte orientieren sich an der DIN EN 13779 und den Empfehlungen des
Umweltbundesamts für Innenraumluftqualität:

| CO2 (ppm) | Bewertung       | Lüfterstufe | Beschreibung                         |
| ---------:| :-------------- | :---------: | :----------------------------------- |
|    ≤ 600  | Ausgezeichnet   |      1      | Minimale Drehzahl, kaum hörbar       |
|    ≤ 800  | Gut             |      2      | Ruhiger Betrieb                      |
|   ≤ 1000  | Mäßig           |      3      | Pettenkofer-Grenze (Normallüftung)   |
|   ≤ 1200  | Erhöht          |      5      | Spürbare Lüftung                     |
|   ≤ 1400  | Schlecht        |      7      | Intensive Lüftung                    |
|   > 1400  | Inakzeptabel    |      9      | Maximale Lüftung (Notbetrieb)        |

> **Hinweis:** Außenluft hat typischerweise 400–420 ppm CO2.
> Der Pettenkofer-Grenzwert (1000 ppm) gilt seit über 150 Jahren als Maßstab für akzeptable Raumluft.

---

## Hysterese (Pendelschutz)

Um ein ständiges Auf- und Abregeln bei CO2-Werten nahe den Schwellwerten zu verhindern,
verwendet die Regelung eine **100 ppm Hysterese**:

- **Hochregelung:** Bei exakt dem Schwellwert (z.B. 1000 ppm → Stufe 5)
- **Rückregelung:** Erst 100 ppm darunter (z.B. 900 ppm → erlaubt Rückfall unter Stufe 5)

```text
                CO2 (ppm)
                   ▲
    1400 ──────────┤─── Level 9 ────────── ▲ up
    1300 ──────────┤                       ▼ down (Hysterese)
    1200 ──────────┤─── Level 7 ────────── ▲ up
    1100 ──────────┤                       ▼ down
    1000 ──────────┤─── Level 5 ────────── ▲ up
     900 ──────────┤                       ▼ down
     800 ──────────┤─── Level 3 ────────── ▲ up
     700 ──────────┤                       ▼ down
     600 ──────────┤─── Level 2 ────────── ▲ up
     500 ──────────┤                       ▼ down
                   └──────────────────────
```

---

## Noise Control (Max-Level)

Der Slider **"CO2 Max Lüfterstufe"** in Home Assistant begrenzt die maximale Lüfterstufe,
die die CO2-Automatik verwenden darf.

| Einstellung | Bedeutung                                             |
| :---------: | :---------------------------------------------------- |
|     10      | Volle Leistung erlaubt (kein Limit)                   |
|      7      | **Standard** — guter Kompromiss aus Lüftung und Lärm  |
|      5      | Moderate Lüftung, sehr leise                          |
|      3      | Minimale Lüftung, flüsterleise                        |

> **Tipp:** Im **Schlafzimmer** empfiehlt sich ein Max-Level von **5** oder niedriger.
> Im **Wohnzimmer** oder **Büro** ist Level **7–8** ein guter Kompromiss.

---

## Grundlüftung (Min-Level) — Feuchteschutz

Der Slider **"CO2 Min Lüfterstufe"** garantiert eine **Mindestlüftung**, auch wenn der
CO2-Wert niedrig ist (z.B. leerer Raum, Nacht). Dies ist wichtig für:

- **Feuchteschutz** — Schimmelprävention auch bei Abwesenheit (DIN 1946-6: „Lüftung zum Feuchteschutz“)
- **VOC-Abbau** — Ausdünstungen von Möbeln, Baumaterialien und Farben
- **Lufthygiene** — Grundlegender Luftaustausch gegen abgestandene Luft

| Einstellung | Empfehlung                                            |
| :---------: | :---------------------------------------------------- |
|      1      | Absolutes Minimum (nur für gut belüftete Räume)       |
|      2      | **Standard** — leise Grundlüftung für Feuchteschutz   |
|      3      | Für Räume mit erhöhter Feuchtebelastung (Bad, Küche)  |
|      4      | Für Neubauten mit hoher Restbaufeuchte                |

> **Hinweis:** Der Lüfter regelt im CO2-Modus **nie unter diese Stufe** — selbst wenn
> CO2 bei 400 ppm liegt. So bleibt die Bausubstanz geschützt.

---

## Aktivierung

### Home Assistant

1. **Switch:** `CO2 Automatik` → Ein/Aus
2. **Slider:** `CO2 Min Lüfterstufe` → 1–10 (Standard: 2, Feuchteschutz)
3. **Slider:** `CO2 Max Lüfterstufe` → 1–10 (Standard: 7, Noise Control)

Beim Einschalten prüft das System, ob der SCD41-Sensor angeschlossen ist.
Ist er nicht verfügbar (`NaN`), wird die Automatik **nicht** aktiviert und eine Warnung geloggt.

### Lokal (Bedienpanel)

Die CO2-Automatik kann bisher nur über Home Assistant gesteuert werden —
das Bedienpanel kennt diese Funktion nicht.

---

## Architektur

```text
┌─────────────┐    30s Interval     ┌──────────────────────┐
│ SCD41 Sensor │ ──── CO2 ppm ────▶ │ apply_co2_auto_control│
│ (I2C @ H2)  │                     │ (automation_helpers.h) │
└─────────────┘                     └──────────┬───────────┘
                                               │
                                    ┌──────────▼───────────┐
                                    │  VentilationLogic::   │
                                    │  get_co2_fan_level()  │
                                    │  (Hysterese + Clamp)  │
                                    └──────────┬───────────┘
                                               │
                                    ┌──────────▼───────────┐
                                    │  Fan PWM anpassen     │
                                    │  LEDs aktualisieren   │
                                    │  HA State publishen   │
                                    └──────────────────────┘
```

### Dateien

| Datei | Inhalt |
| :--- | :--- |
| `components/ventilation_logic/ventilation_logic.h/.cpp` | Pure C++ Logik: `get_co2_fan_level()`, `get_co2_classification()` |
| `automation_helpers.h` | Inline-Wrapper: `apply_co2_auto_control()` |
| `esp_wohnraumlueftung.yaml` | Globals, Switch, Slider, Text-Sensor, Interval (30s) |

---

## YAML-Konfiguration

Die folgenden Elemente werden automatisch in `esp_wohnraumlueftung.yaml` bereitgestellt:

```yaml
# Globals (persistent)
globals:
  - id: co2_auto_enabled
    type: bool
    initial_value: "false"
    restore_value: true

  - id: co2_min_fan_level
    type: int
    initial_value: "2"
    restore_value: true

  - id: co2_max_fan_level
    type: int
    initial_value: "7"
    restore_value: true

# Switch in Home Assistant
switch:
  - platform: template
    name: "CO2 Automatik"
    id: co2_auto_switch

# Min-Level Slider (Feuchteschutz)
number:
  - platform: template
    name: "CO2 Min Lüfterstufe"
    id: co2_min_level_slider
    min_value: 1
    max_value: 10
    initial_value: 2

# Max-Level Slider (Noise Control)
  - platform: template
    name: "CO2 Max Lüfterstufe"
    id: co2_max_level_slider
    min_value: 1
    max_value: 10
    initial_value: 7

# CO2 Bewertung als Text
text_sensor:
  - platform: template
    name: "CO2 Bewertung"
    id: co2_classification

# 30s Interval Automation
interval:
  - interval: 30s
    then:
      - lambda: apply_co2_auto_control();
```

---

## Fehlerbehebung

| Problem | Lösung |
| :--- | :--- |
| CO2 Automatik lässt sich nicht einschalten | SCD41 nicht angeschlossen → am H2 Header anschließen |
| Lüfter reagiert nicht auf CO2-Anstieg | `CO2 Automatik` Switch in HA prüfen |
| Lüfter dreht zu hoch | `CO2 Max Lüfterstufe` auf gewünschten Wert reduzieren |
| Lüfter dreht nie ab (auch bei gutem CO2) | `CO2 Min Lüfterstufe` prüfen — dies ist beabsichtigt (Feuchteschutz) |
| Log: "SCD41 not connected" | I2C-Verbindung prüfen (SDA/SCL, Kabelbruch) |
| CO2 Wert springt stark | Median-Filter im SCD41 Sensor bereits aktiv (window_size: 5) |
