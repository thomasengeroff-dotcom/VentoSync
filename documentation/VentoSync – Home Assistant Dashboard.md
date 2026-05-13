# VentoSync – Home Assistant Dashboard

## Übersicht

Dieses Dokument beschreibt das modulare Lovelace-Dashboard für **VentoSync**-Geräte in Home Assistant. Das Dashboard nutzt ein wiederverwendbares **Decluttering-Template**, das für beliebig viele VentoSync-Geräte instanziiert werden kann – pro Gerät sind nur 3 Zeilen YAML nötig.

![Dashboard Vorschau](docs/images/dashboard-preview.png)

---

## Inhaltsverzeichnis

- [Voraussetzungen](#voraussetzungen)
- [Installation](#installation)
- [Dashboard-Architektur](#dashboard-architektur)
- [Template-Aufbau](#template-aufbau)
  - [1. Hero Section](#1-hero-section)
  - [2. Status Chips](#2-status-chips)
  - [3. Detail Cards](#3-detail-cards)
  - [4. Mini Graph](#4-mini-graph)
  - [5. Steuerung](#5-steuerung)
  - [6. Switches](#6-switches)
- [Entity-Referenz](#entity-referenz)
- [Geräte hinzufügen](#geräte-hinzufügen)
- [Anpassungen](#anpassungen)
- [Fehlerbehebung](#fehlerbehebung)

---

## Voraussetzungen

### HACS Custom Cards

Folgende Custom Cards müssen über [HACS](https://hacs.xyz/) (Frontend) installiert sein:

| Card | Repository | Zweck |
|------|-----------|-------|
| **Mushroom** | [piitaya/lovelace-mushroom](https://github.com/piitaya/lovelace-mushroom) | Template-, Chips-, Select-, Number-Cards |
| **Mini Graph Card** | [kalkih/mini-graph-card](https://github.com/kalkih/mini-graph-card) | Verlaufsgraphen |
| **Card Mod** | [thomasloven/lovelace-card-mod](https://github.com/thomasloven/lovelace-card-mod) | CSS-Anpassungen & Animationen |
| **Stack In Card** | [custom-cards/stack-in-card](https://github.com/custom-cards/stack-in-card) | Nahtlose Kartenverschachtelung |
| **Decluttering Card** | [jcwillox/lovelace-decluttering-card](https://github.com/jcwillox/lovelace-decluttering-card) | Wiederverwendbare Templates |

### Systemvoraussetzungen

- Home Assistant **2024.1** oder neuer
- Mindestens ein konfiguriertes VentoSync-Gerät via ESPHome
- Einheitliches Entity-Namensschema (automatisch durch ESPHome `name:`)

---

## Installation

### 1. HACS Cards installieren
HACS → Frontend → + Explore & Download Repositories → Suche nach: mushroom, mini-graph-card, card-mod, stack-in-card, decluttering-card → Jeweils installieren

### 2. Browser-Cache leeren

Nach jeder HACS-Installation **zwingend** den Browser-Cache leeren:

- **Windows/Linux:** `Strg + Shift + R`
- **macOS:** `Cmd + Shift + R`

### 3. Dashboard konfigurieren

1. Dashboard öffnen → Oben rechts `⋮` → **Raw-Konfigurationseditor**
2. Das `decluttering_templates:`-Block **vor** `views:` einfügen
3. In der gewünschten View die Geräte per `custom:decluttering-card` aufrufen
4. Speichern

---

## Dashboard-Architektur

```text
┌─────────────────────────────────────────────┐
│                HERO SECTION                 │
│    Animiertes Fan-Icon + Modus + Metrics    │
│  ┌───────────────────────────────────────┐  │
│  │ Chips: Richtung │ Fenster │ Gehäuse   │  │
│  └───────────────────────────────────────┘  │
├─────────────────────────────────────────────┤
│          DETAIL CARDS (4 Spalten)           │
│ Drehzahl │ PWM Duty │ Gehäuse │ Fensterspr. │
├─────────────────────────────────────────────┤
│                 MINI GRAPH                  │
│    Drehzahl (RPM) + PWM (%) + Temp (°C)     │
├─────────────────────────────────────────────┤
│                 STEUERUNG                   │
│          Betriebsmodus (Select)             │
│          Lüfter Stufe (Slider)              │
│          LED Helligkeit (Slider)            │
├─────────────────────────────────────────────┤
│            SWITCHES (2 Spalten)             │
│         Fenstersperre │ Kindersicherung     │
└─────────────────────────────────────────────┘
```

### Wiederverwendbarkeit

```yaml
decluttering_templates:
  ventosync:             # ← Template (einmal definiert)
    card: ...            # ← Enthält [[device]] Platzhalter

# Anwendung im Dashboard:
views:
  cards:
    - type: custom:decluttering-card
      template: ventosync
      variables:
        - device: ventosync_dg_buero   # ← Instanz 1
    - type: custom:decluttering-card
      template: ventosync
      variables:
        - device: ventosync_eg_wozi     # ← Instanz 2
```

---

## Template-Aufbau

### 1. Hero Section

Das zentrale Element des Dashboards. Zeigt den aktuellen Betriebsmodus mit einem animierten Fan-Icon.

**Features:**

- **Richtungsbewusste Spin-Animation:** Das Icon dreht sich vorwärts (Zuluft) oder rückwärts (Abluft)
- **RPM-skalierte Geschwindigkeit:** Die Animationsgeschwindigkeit passt sich der tatsächlichen Drehzahl an
  - `> 1500 RPM` → 0.8s (schnell)
  - `> 800 RPM` → 1.5s (mittel)
  - `≤ 800 RPM` → 3s (langsam)
  - `0 RPM` → keine Animation
- **Dynamische Farbcodierung:**
  - 🔵 Blau = Zuluft (positive RPM)
  - 🟠 Orange = Abluft (negative RPM)
  - ⚫ Disabled = Stillstand

**Kompakte Sekundärzeile:**
Zuluft (Rein) · 1680 RPM · 22% PWM · Stufe 5


### 2. Status Chips

Drei farbcodierte Chips unterhalb des Hero-Icons:

| Chip | Entity-Suffix | Darstellung |
|------|--------------|-------------|
| **Richtung** | `_lufter_richtung` | ↓ Zuluft (blau) / ↑ Abluft (orange) / ⇅ Wärmetausch (teal) / ⏸ Pause (grau) |
| **Fenstersperre** | `_fenstersperre_aktiv` | 🟢 Fenster zu / 🔴 Fenster offen |
| **Gehäuse-Temp** | `_bmp390_temperatur` | Farbcodiert: teal (< 40°C) / orange (40–50°C) / rot (> 50°C) |

### 3. Detail Cards

Vier Mushroom Template Cards in einer horizontalen Reihe:

| Card | Entity-Suffix | Besonderheit |
|------|--------------|-------------|
| **Drehzahl** | `_lufter_drehzahl` | Zeigt `↓ 1680 RPM` (blau) oder `↑ 1680 RPM` (orange) je nach Richtung. Pulse-Animation wenn aktiv. |
| **PWM Duty** | `_lufter_pwm` | Farbcodiert: amber (< 50%) / deep-orange (50–80%) / rot (> 80%) |
| **Gehäuse** | `_bmp390_temperatur` | Farbcodiert: blau (< 40°C) / deep-orange (40–50°C) / rot (> 50°C) |
| **Fenstersperre** | `_fenstersperre_aktiv` | Dynamisches Icon: 🪟 offen (rot) / 🪟 geschlossen (grün) |

### 4. Mini Graph

Verlaufsgraph mit drei numerischen Metriken:

| Metrik | Farbe | Y-Achse |
|--------|-------|---------|
| Drehzahl (RPM) | `#7c4dff` (lila) | Sekundär (rechts) |
| PWM (%) | `#ff6d00` (orange) | Primär (links) |
| Temp. Gehäuse (°C) | `#00b0ff` (blau) | Primär (links) |

**Konfiguration:**
- `hours_to_show: 0.2` (ca. 12 Minuten – ideal für den ~70s Reversierzyklus)
- `points_per_hour: 3600` (1 Punkt pro Sekunde)
- `fill: fade` (halbtransparente Füllung unter den Linien)

### 5. Steuerung

Drei Controls in einer nahtlosen `stack-in-card`:

| Control | Typ | Entity-Suffix |
|---------|-----|--------------|
| **Betriebsmodus** | Mushroom Select (Dropdown) | `_luftermodus` |
| **Lüfter Stufe** | Mushroom Number (Slider) | `_lufter_intensitat` |
| **LED Helligkeit** | Mushroom Number (Slider) | `_maximale_led_helligkeit` |

### 6. Switches

Zwei Toggle-Cards nebeneinander mit visuellem Feedback:

| Switch | Entity-Suffix | Aktiv-Zustand |
|--------|--------------|--------------|
| **Fenstersperre ignorieren** | `_fenstersperre_ignorieren` | 🟠 Orange Border + "Wird ignoriert" |
| **Kindersicherung** | `_kindersicherung` | 🔴 Rote Border + "Bedienfeld gesperrt" |

Beide Cards reagieren auf **Tap** zum Togglen und zeigen über eine farbige `border-left`-Linie den aktiven Override-Zustand an.

---

## Entity-Referenz

Alle Entities folgen dem Schema `{domain}.{device}_{suffix}`:

| Domain | Suffix | Typ | Beschreibung |
|--------|--------|-----|-------------|
| `select` | `_luftermodus` | Select | Betriebsmodus (Wärmerückgewinnung, Zuluft, Abluft, etc.) |
| `number` | `_lufter_intensitat` | Number | Lüfterstufe (manuell) |
| `number` | `_maximale_led_helligkeit` | Number | Maximale LED-Helligkeit am Bedienfeld |
| `sensor` | `_lufter_drehzahl` | Sensor | Aktuelle Drehzahl in RPM (negativ = Abluft) |
| `sensor` | `_lufter_pwm` | Sensor | Aktueller PWM Duty Cycle in % |
| `sensor` | `_lufter_richtung` | Sensor | Aktuelle Richtung (z.B. "Zuluft (Rein)", "Abluft (Raus)") |
| `sensor` | `_bmp390_temperatur` | Sensor | Gehäusetemperatur (BMP390) in °C |
| `sensor` | `_fenstersperre_aktiv` | Sensor | Fenstersperre Status ("Ja" / "Nein") |
| `switch` | `_fenstersperre_ignorieren` | Switch | Override: Fenstersperre ignorieren |
| `switch` | `_kindersicherung` | Switch | Kindersicherung (Bedienfeld sperren) |

> **Hinweis:** Die Entity-IDs werden automatisch durch ESPHome generiert, basierend auf dem `name:`-Feld in der ESPHome-Konfiguration. Alle VentoSync-Geräte müssen das gleiche Suffix-Schema verwenden.

---

## Geräte hinzufügen

### Neues Gerät in 3 Zeilen

```yaml
- type: custom:decluttering-card
  template: ventosync
  variables:
    - device: ventosync_STOCKWERK_RAUM
```

### Beispiel: Komplettes Haus

```yaml
views:
  - title: Lüftung
    icon: mdi:hvac
    path: lueftung
    cards:
      # Dachgeschoss
      - type: custom:decluttering-card
        template: ventosync
        variables:
          - device: ventosync_dg_buero
```
