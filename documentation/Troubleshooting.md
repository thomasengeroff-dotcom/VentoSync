# 🔍 Troubleshooting

Dieser Leitfaden hilft bei der Diagnose und Behebung häufiger Probleme mit dem ESPHome Wohnraumlüftungs-Projekt.

---

## ESPHome YAML Fehler

### "Unresolved tag: !lambda"

**Problem:** Lambda-Ausdrücke verwenden falsche YAML-Syntax.

**Lösung:**

```yaml
# ❌ Falsch
data: !lambda "return x;"

# ✅ Richtig
data: !lambda |-
  return x;
```

**VS Code Fix:** Füge ESPHome-Tags zu `settings.json` hinzu (siehe Hauptdokumentation, Abschnitt "ESPHome YAML Syntax").

### "Invalid YAML" oder "mapping values are not allowed here"

**Ursachen:**

- Fehlende Einrückung (YAML verwendet 2 Leerzeichen)
- Tab-Zeichen statt Leerzeichen
- Fehlende Doppelpunkte nach Keys

**Lösung:** Prüfe Einrückungen und verwende nur Leerzeichen.

---

## I²C Bus Probleme

### "I2C device not found at address 0x..."

**Diagnose:**

```yaml
i2c:
  scan: true  # Aktiviert I²C-Scan beim Boot
```

**Lösungen:**

1. Prüfe Verkabelung (SDA/SCL vertauscht?)
2. Prüfe Pull-Up-Widerstände (4.7kΩ an SDA/SCL)
3. Reduziere I²C-Frequenz: `frequency: 100kHz`
4. Erhöhe Timeout: `timeout: 100ms`

### "Software timeout" oder "I2C bus busy"

**Ursache:** Zu viele Geräte oder zu schnelle Updates.

**Lösungen:**

1. Erhöhe `update_interval` bei Sensoren
2. Füge `throttle` Filter hinzu
3. Erhöhe I²C Timeout: `timeout: 50ms`
4. Reduziere Frequenz: `frequency: 100kHz`

---

## APDS9960 Probleme

### Display aktiviert nicht bei Annäherung

**Lösungen:**

1. Prüfe Schwellwert: `prox_threshold: "38"` (anpassen 0-255)
2. Teste Sensor: Logge Proximity-Werte
3. Prüfe `proximity_gain` (2x oder 4x)
4. Stelle sicher, dass Sensor nicht verdeckt ist

### Zu viele False-Positives

**Lösungen:**

1. Erhöhe `delta` Filter: `delta: 10`
2. Reduziere `proximity_gain: 1x`
3. Erhöhe Schwellwert: `prox_threshold: "50"`
4. Füge Debounce hinzu

---

## BME680 / BSEC2 Probleme

### "BSEC library not found"

**Lösung:** ESPHome lädt BSEC2 automatisch. Bei Problemen:

```yaml
external_components:
  - source: github://esphome/esphome@dev
    components: [ bme68x_bsec2_i2c ]
```

### IAQ-Werte bleiben bei 25 oder 50

**Ursache:** Sensor noch in Kalibrierung (bis zu 28 Tage).

**Lösung:** Geduld! Nach `operating_age: 28d` stabilisieren sich die Werte.

### "IAQ Accuracy: 0"

**Bedeutung:** Sensor kalibriert noch.

- 0 = Unkalibriert
- 1 = Niedrige Genauigkeit
- 2 = Mittlere Genauigkeit
- 3 = Hohe Genauigkeit (Ziel)

---

## ESP-NOW Probleme

### Geräte synchronisieren nicht

**Lösungen:**

1. Prüfe Floor/Room/Device ID Konfiguration
2. Stelle sicher, beide Geräte sind im gleichen WiFi-Kanal
3. Prüfe Reichweite (max ~100m Freifeld)
4. Aktiviere Debug-Logging:

```yaml
logger:
  level: DEBUG
```

---

## Kompilierungsfehler

### "undefined reference to..."

**Ursache:** Fehlende Funktionen in `.h` Dateien.

**Lösung:** Prüfe `automation_helpers.h` auf fehlende Funktionen:

- `get_iaq_traffic_light_data()`
- `get_iaq_classification()`
- `calculate_ramp_up()`
- `calculate_ramp_down()`
- `is_fan_slider_off()`
- `calculate_heat_recovery_efficiency()`

### "error: 'VentilationController' was not declared"

**Lösung:** Prüfe externe Komponente:

```yaml
external_components:
  - source:
      type: local
      path: components
```

Stelle sicher, dass `components/ventilation_group/` existiert.

---

## Performance-Probleme

### ESP32 stürzt ab oder bootet neu

**Ursachen:**

- Zu viele gleichzeitige I²C-Zugriffe
- Stromversorgung instabil
- Watchdog-Timeout

**Lösungen:**

1. Erhöhe `update_interval` bei allen Sensoren
2. Prüfe 5V-Versorgung (min. 500mA)
3. Füge Kondensator (100µF) an 5V hinzu
4. Deaktiviere WiFi-Power-Save:

```yaml
wifi:
  power_save_mode: none
```

### Langsame Reaktionszeit

**Lösungen:**

1. Reduziere `throttle` Filter
2. Reduziere `update_interval`
3. Optimiere Lambda-Code (vermeide `delay()`)

---

## Hilfreiche Debug-Befehle

### I²C-Scan anzeigen

```yaml
i2c:
  scan: true

logger:
  level: DEBUG
```

### Alle Sensor-Werte loggen

```yaml
sensor:
  - platform: apds9960
    on_value:
      then:
        - logger.log:
            format: "Proximity: %.0f"
            args: [ 'x' ]
```

### ESP-NOW Pakete debuggen

```yaml
espnow:
  on_receive:
    then:
      - logger.log: "ESP-NOW packet received"
```

---

## 🔗 Weitere Hilfe

- [ESPHome Dokumentation](https://esphome.io/)
- [ESPHome Discord](https://discord.gg/KhAMKrd)
- [GitHub Issues](https://github.com/esphome/issues/issues)

---

**Zurück zur [Hauptdokumentation](../Readme.md)**
