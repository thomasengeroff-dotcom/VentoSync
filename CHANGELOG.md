# 📋 Changelog - ESPHome Wohnraumlüftung

## 2026-02-15 — Compilation Fixes, Integration Tests & Code Documentation

### 🐛 Bug Fixes

#### Main Project Compilation (`esp_wohnraumlueftung.yaml`)

- **`ventilation_logic` Component**: Added `ventilation_logic:` to YAML — component war registriert aber der Include fehlte, Build schlug fehl.
- **`automation_helpers.h` Access Corrections**:
  - `v->current_mode` → `v->state_machine.current_mode` (3 Stellen)
  - `v->ventilation_duration_ms` → `v->state_machine.ventilation_duration_ms` (1 Stelle)
- **`ventilation_group.h`**: Entfernte fehlerhafte `current_mode()` Getter-Methode (API-Konflikt).

#### Integration Test Compilation (`integration_test.yaml`)

- **Vollständig neu geschrieben** — alle `extern`-Deklarationen aus `automation_helpers.h` werden jetzt korrekt gemockt:
  - **9 Status-LEDs** (`status_led_l1`–`l5`, `power`, `master`, `mode_wrg`, `mode_vent`) → `monochromatic` + `template float` Outputs
  - **Fan PWM** (`fan_pwm_primary`, `fan_pwm_secondary`) → `ledc` Outputs (GPIO19/18)
  - **Speed Fan** → `lueftung_fan` (ID korrigiert von `mock_fan`)
  - **Selects** → `selected_mode` + `fan_mode_select` mit korrekten Optionen
  - **Numbers** → `vent_timer` + `fan_intensity_display`
  - **Switch** → `fan_direction` (ID korrigiert von `mock_direction`)
  - **Scripts** → `update_leds`, `fan_speed_update` mit `mode: restart`; `set_fan_speed_and_direction` mit `parameters: float, int`
- **Globals-Zugriff**: `id(x)->value() = wert` → `id(x) = wert` (5 Stellen)

### 📚 Code Documentation

Doxygen-Style `/// @brief` / `@param` / `@return` Kommentare zu **allen** C++/H-Dateien hinzugefügt:

| Datei | Kommentierte Elemente |
|-------|----------------------|
| `ventilation_state_machine.h` | File-doc, Enum-Werte, Struct-Felder, Klasse, Members, Methoden |
| `ventilation_state_machine.cpp` | Alle 8 Methoden |
| `ventilation_group.h` | File-doc, `MessageType`, `VentilationPacket`, `VentilationController` (Felder, Setter, Methoden) |
| `ventilation_logic.h` | File-doc, Klasse, alle 7 statischen Methoden |
| `ventilation_logic.cpp` | Alle 7 Methoden |
| `automation_helpers.h` | File-doc, alle `extern`-Gruppen (`@name`), alle 14 inline-Funktionen |

### 🧪 Unit Tests (SimpleTest Runner)

Neues natives C++ Test-Framework ohne ESPHome-Abhängigkeit aufgesetzt (`tests/simple_test_runner.cpp`):

- **Test-Infrastruktur**: Eigenes `TEST_ASSERT` Makro, kompiliert mit `g++` direkt auf dem Host
- **`run_tests.bat`**: Build + Run Skript für Windows

#### Tests (5 Testfälle, alle ✅ PASS)

| Test | Prüft |
| ---- | ----- |
| `test_iaq_classification` | IAQ-Werte → deutsche Klassifikation (Ausgezeichnet…Gesundheitsgefährdend) |
| `test_heat_recovery` | WRG-Effizienz-Berechnung + Division-by-Zero-Schutz |
| `test_fan_logic` | Slider-Off-Schwelle + Level-Cycling 1→10→1 |
| `test_stosslueftung_cycle` | 15 min ON → 105 min OFF → Direction-Flip |
| `test_phase_logic` | Phase A/B Wechsel mit 1 s Zyklusdauer |

### 📁 File Changes

#### New Files

- `tests/simple_test_runner.cpp` — 5 native Unit Tests für `VentilationLogic` + `VentilationStateMachine`
- `tests/run_tests.bat` — Build & Run Skript
- `tests/README.md` — Testdokumentation

#### Modified Files

- `esp_wohnraumlueftung.yaml` — `ventilation_logic:` Component hinzugefügt
- `automation_helpers.h` — Access-Path Korrekturen + Doxygen-Kommentare
- `components/ventilation_group/ventilation_group.h` — Getter entfernt + Doxygen-Kommentare
- `components/ventilation_group/ventilation_state_machine.h` — Doxygen-Kommentare
- `components/ventilation_group/ventilation_state_machine.cpp` — Doxygen-Kommentare
- `components/ventilation_logic/ventilation_logic.h` — Doxygen-Kommentare
- `components/ventilation_logic/ventilation_logic.cpp` — Doxygen-Kommentare

#### Rewritten Files

- `integration_test.yaml` — Komplett neu geschrieben mit vollständigen Mock-Komponenten

### ✅ Compilation Status

| YAML | Status | Build-Zeit |
|------|--------|-----------|
| `esp_wohnraumlueftung.yaml` | ✅ SUCCESS | ~70 s |
| `integration_test.yaml` | ✅ SUCCESS | ~355 s |

---

## 2026-02-14 - Hardware Pinout & Safety Update

### ✨ Hardware Changes

#### Pinout Update (Verified)

- **Layout korrigiert**: Pin-Mapping an finales Board-Layout angepasst (basierend auf Hardware Verification Report)
  - **Buttons**: Power (D6/GPIO16), Mode (D9/GPIO20), Level (D2/GPIO2).
  - **Fan PWM**: Primary (D8/GPIO19), Secondary (D10/GPIO18).
  - **NTCs**: ADC Swapped (Out=D0, In=D1).
- **Logger**: Umzug auf `USB_SERIAL_JTAG` (GPIO16/17 jetzt für Buttons/Tacho frei).

#### Circuit Safety (Critical)

- **Q1 MOSFET**: **DMP3098L** (Vgs ±20V) ist jetzt Pflicht-Bauteil.
- **AO3401 Warnung**: Entfernt, da Bauteil durch DMP3098L ersetzt wurde (keine Vgs-Probleme mehr).
- **R_GS**: Widerstand ersatzlos gestrichen (nicht mehr nötig).

### 📚 Documentation Updates

- `Anleitung-Fan-Circuit.md`:
  - Q1 auf DMP3098L aktualisiert.
  - Vgs Warnung entfernt.
  - Schaltplan und Stückliste bereinigt.
- `Hardware-Setup-Readme.md`:
  - Neue Pin-Tabelle (D0-D10) eingepflegt.
- `PCB-Best-Practices.md`:
  - Neue Regel für **Teardrops** hinzugefügt (Mechanische Stabilität).
  - Tacho-Filter Hinweis ergänzt.

### 📁 File Changes

- `esp_wohnraumlueftung.yaml`: GPIO-Assignment und Logger-Config angepasst.
- `esp32c6_common.yaml`: Logger auf `USB_SERIAL_JTAG` umgestellt.

---

## 2026-02-10 - Hybrid Fan Interface & Documentation Update

### ✨ Hardware Changes

#### Hybrid Fan Control Interface

- **Dual-Mode Support**: Board unterstützt jetzt sowohl **4-Pin PWM** (AxiRev) als auch **3-Pin Dual-GND** (VarioPro) Lüfter
- **Neue Komponenten**:
  - **Q4, Q5** (PMV16XNR SOT-23): Dual Low-Side MOSFETs für 3-Pin GND1/GND2 Kontrolle
  - **D1** (B5819WS): Freilaufdiode an PWM_12V_OUT (Q1 Drain) — kritisch für Buck-Converter
  - **D2** (B5819WS): Schutzdiode an Fan Pin 1 (GND/GND1)
  - **D3** (B5819WS): Schutzdiode an Fan Pin 4 (PWM/GND2)
  - **R18, R19** (10kΩ 0402): Pull-down Widerstände für Q4/Q5 Gates
  - **JP1** (1x3 2.54mm): VCC Moduswahl (Constant/Variable 12V)
  - **JP2, JP3** (1x3 1.27mm): Pin 1/Pin 4 Moduswahl
- **GPIO2**: Neue Funktion als **Fan PWM Secondary** (Low-Side GND2 via Q4)

### 🐛 Bug Fixes

- **D1 Freilaufdiode** an PWM_12V_OUT gefehlt — ohne diese wird Q1 im 3-Pin Modus durch induktive Spannungsspitzen zerstört
- **R8**: Dokumentation korrigiert (2.2kΩ / 0805, nicht 10kΩ / 0603)
- **R3**: Beschreibung korrigiert ("für Q3" statt "für Q2")
- **D1 (SS14)** in Schaltplan → korrigiert zu **D1 (B5819WS)**
- **Dioden-Designatoren**: D1/D3 → D2/D3 im Low-Side Circuit

### 📚 Documentation Updates

- `Anleitung-Fan-Circuit.md` — BOM und Schaltplan an aktuelle Hardware angepasst
- `Hardware-Setup-Readme.md` — Universal Fan Interface komplett überarbeitet (S8050, R8=2.2kΩ, JP1/JP2/JP3)
- `Technical-Details.md` — PWM-Konfiguration aktualisiert (GPIO16/GPIO2, 28kHz, Dual-Mode)
- `Readme.md` — Pin-Tabelle, Mermaid-Diagramm, Projektstruktur erweitert
- `esp_wohnraumlueftung.yaml` — PWM-Kommentare mit vollständigen Schaltungsdetails

### 📁 File Changes

#### Modified Files

- `esp_wohnraumlueftung.yaml` — PWM-Kommentare erweitert
- `documentation/Anleitung-Fan-Circuit.md` — BOM/Schematic Korrekturen
- `documentation/Hardware-Setup-Readme.md` — Fan Interface Rewrite
- `documentation/Technical-Details.md` — PWM-Sektion aktualisiert
- `Readme.md` — GPIO-Tabelle, Diagramm, Struktur

---

## 2026-01-28 - Major Documentation & Optimization Update

### ✨ New Features

#### APDS9960 Sensor Optimization

- **Performance Improvements:**
  - Update interval: 100ms → 500ms (80% less I²C traffic)
  - Proximity gain: 4x → 2x (better close-range detection)
  - LED drive: 100mA → 50mA (50% power savings)
  - Added delta filter: 5 (noise reduction)
  - Added throttle filter: 500ms (prevents I²C bus flooding)

- **Adaptive Display Brightness:**
  - Display brightness now adjusts automatically based on ambient light
  - Bright room (>100): Full brightness (255)
  - Dim room (≤100): Half brightness (128)

#### Configuration Improvements

- Added configurable proximity threshold via substitutions
- Improved logging (DEBUG level for reduced spam)
- Better filter configuration for stable readings
- Future-ready gesture detection (commented, ready to enable)

### 🐛 Bug Fixes

#### ESPHome YAML Syntax Errors

Fixed "Unresolved tag: !lambda" errors in 5 locations:

- Line 186: IAQ traffic light ESP-NOW send
- Line 274: Fan speed control from slider
- Line 381: Fan ramp-up calculation
- Line 398: Fan ramp-down calculation
- Line 472: Ventilation sync broadcast

**Root Cause:** ESPHome requires lambda expressions to use YAML block scalar format (`|-`) instead of quoted strings.

**Before (Incorrect):**

```yaml
data: !lambda "return get_iaq_traffic_light_data(x);"
```

**After (Correct):**

```yaml
data: !lambda |-
  return get_iaq_traffic_light_data(x);
```

### 📚 Documentation Updates

#### README.md Enhancements

Added comprehensive documentation sections:

1. **Project Structure**
   - Complete file tree with descriptions
   - Component organization

2. **Technical Details & Optimizations**
   - APDS9960 sensor optimization details
   - ESPHome YAML syntax guide
   - I²C bus configuration
   - BME680 BSEC2 configuration
   - ESP-NOW communication
   - Fan control logic

3. **Troubleshooting Section**
   - ESPHome YAML errors
   - I²C bus problems
   - APDS9960 issues
   - BME680/BSEC2 problems
   - ESP-NOW synchronization
   - Compilation errors
   - Performance issues
   - Debug commands

4. **UI Features**
   - Added adaptive brightness feature
   - Added optimized sensor description

#### VS Code Configuration

Added ESPHome YAML tag support to `settings.json`:

```json
{
  "yaml.customTags": [
    "!lambda scalar",
    "!secret scalar",
    "!include scalar",
    "!include_dir_named scalar",
    "!include_dir_list sequence",
    "!include_dir_merge_list sequence",
    "!include_dir_merge_named mapping"
  ]
}
```

### 📁 File Changes

#### Modified Files

- `esp_wohnraumlueftung.yaml` - Fixed 5 lambda syntax errors
- `apds9960_config.yaml` - Replaced with optimized version
- `Readme.md` - Added 200+ lines of documentation
- `settings.json` - Added YAML custom tags

#### New Files

- `apds9960_config_optimized.yaml` - Optimized sensor configuration (now active)
- `apds9960_config.yaml.backup` - Backup of original configuration

### 🎯 Performance Improvements

| Metric            | Before | After | Improvement   |
|-------------------|--------|-------|---------------|
| I²C Bus Traffic   | 100%   | 20%   | 80% reduction |
| APDS9960 Power    | 100mA  | 50mA  | 50% savings   |
| Update Rate       | 10/sec | 2/sec | More stable   |
| Log Spam          | High   | Low   | DEBUG level   |

### 🔧 Technical Changes

#### APDS9960 Configuration

```yaml
# Old
apds_update_interval: 100ms
# No gain configuration
# No filters

# New
apds_update_interval: 500ms
proximity_gain: 2x
led_drive: 50mA
filters:
  - delta: 5
  - throttle: 500ms
```

#### Lambda Expressions

All lambda expressions now use proper YAML block format:

```yaml
# Pattern used throughout
action: !lambda |-
  return function_call(x);
```

### 📖 Documentation Metrics

- **README.md:** 171 lines → 543 lines (+372 lines)
- **New sections:** 7
- **Code examples:** 20+
- **Troubleshooting entries:** 15+

### ✅ Testing & Validation

- [x] YAML syntax validated
- [x] Lambda expressions corrected
- [x] APDS9960 configuration optimized
- [x] Documentation complete
- [x] Markdown linting passed
- [ ] Hardware testing pending (user to flash)

### 🚀 Next Steps

1. Flash updated configuration to device
2. Test adaptive brightness in different lighting conditions
3. Monitor I²C bus stability
4. Verify power consumption improvements
5. Consider enabling gesture controls for fan speed

### 📝 Notes

- Original APDS9960 config backed up to `apds9960_config.yaml.backup`
- All changes are backward compatible
- No breaking changes to existing functionality
- Gesture detection code included but commented out (ready to enable)

---

**Author:** Antigravity AI Assistant  
**Date:** 2026-01-28  
**Version:** 1.1.0
