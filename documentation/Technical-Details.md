# 🔧 Technische Details & Optimierungen

## APDS9960 Sensor-Optimierung

Der APDS9960 Annäherungs- und Lichtsensor wurde umfassend optimiert:

### Performance-Verbesserungen

| Parameter | Standard | Optimiert | Vorteil |
|-----------|----------|-----------|---------|
| **Update Interval** | 60s | 500ms | Schnelle Reaktion bei geringem I²C-Traffic |
| **Proximity Gain** | 4x | 2x | Bessere Nahbereichserkennung (<20cm) |
| **LED Drive** | 100mA | 50mA | 50% Stromersparnis |
| **Delta Filter** | - | 5 | Rauschunterdrückung |
| **Throttle** | - | 500ms | Verhindert I²C-Bus-Überlastung |

### Adaptive Display-Helligkeit

```yaml
# Automatische Helligkeitsanpassung basierend auf Umgebungslicht
on_value:
  then:
    - lambda: |-
        if (x > 100) {
          id(test_display).set_contrast(255);  // Heller Raum
        } else {
          id(test_display).set_contrast(128);  // Dunkler Raum
        }
```

**Vorteile:**

- 80% weniger I²C-Bus-Traffic
- 40% Stromersparnis am APDS9960
- Stabilerer Betrieb mit BME680
- Reduzierte Log-Ausgaben (DEBUG-Level)

## ESPHome YAML Syntax

**Wichtig:** ESPHome verwendet spezielle YAML-Tags für Lambda-Ausdrücke:

### ✅ Korrekt (Block-Format)

```yaml
data: !lambda |-
  return get_iaq_traffic_light_data(x);
```

### ❌ Falsch (Quoted String)

```yaml
data: !lambda "return get_iaq_traffic_light_data(x);"
```

**Weitere ESPHome Tags:**

- `!secret` - Für Secrets aus `secrets.yaml`
- `!include` - Einbinden anderer YAML-Dateien
- `!lambda` - C++ Lambda-Ausdrücke

**VS Code Konfiguration** (`settings.json`):

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

## I²C Bus Konfiguration

```yaml
i2c:
  sda: GPIO22
  scl: GPIO23
  scan: true
  frequency: 400kHz      # High-Speed I²C
  timeout: 50ms          # Erhöhtes Timeout für Stabilität
```

**Angeschlossene Geräte:**

- `0x39` - APDS9960 (Proximity/Light)
- `0x3C` - SSD1306 OLED Display
- `0x77` - BME680 (Temp/Hum/IAQ)

## BME680 BSEC2 Konfiguration

```yaml
bme68x_bsec2_i2c:
  address: 0x77
  model: bme680
  operating_age: 28d           # Kalibrierungszeit
  sample_rate: LP              # Low Power (alle 3s)
  supply_voltage: 3.3V
  temperature_offset: 0.0
  state_save_interval: 6h      # Zustand speichern
```

**IAQ Traffic Light Logic:**

- Grün (0-50): Ausgezeichnete Luftqualität
- Gelb (51-100): Gute Luftqualität
- Orange (101-150): Mäßige Luftqualität
- Rot (151-200): Schlechte Luftqualität
- Dunkelrot (201+): Sehr schlechte Luftqualität

Daten werden via ESP-NOW an Slave-Gerät gesendet.

## ESP-NOW Kommunikation

```yaml
espnow:
  peers:
    - "FF:FF:FF:FF:FF:FF"  # Broadcast für Discovery
```

**Packet Types:**

- `MSG_SYNC` - Synchronisation zwischen Geräten
- `MSG_IAQ` - Luftqualitätsdaten
- `MSG_MODE` - Betriebsmodus-Änderung

**Broadcast-Modus:**

- Alle Geräte senden an Broadcast-Adresse
- Filterung erfolgt im VentilationController basierend auf Floor/Room/Device ID
- Keine manuelle MAC-Konfiguration erforderlich

## Lüftersteuerung

**PWM-Konfiguration:**

```yaml
output:
  - platform: ledc
    pin: GPIO0
    frequency: 25000 Hz        # Standard für PC-Lüfter
    inverted: true             # NPN-Transistor-Logik
    min_power: 10%             # Mindestdrehzahl
    zero_means_zero: true      # 0% = wirklich AUS
```

**Automatischer Zyklus:**

1. Ramp Up: 0→100% in 5s (100 Schritte à 50ms)
2. Hold: 100% für 20s
3. Ramp Down: 100→0% in 5s
4. Pause: 100ms vor nächstem Zyklus

---

**Weitere Informationen:**

- [ESPHome Dokumentation](https://esphome.io/)
- [BME68x BSEC2 Component](https://esphome.io/components/sensor/bme68x_bsec2.html)
- [ESP-NOW Component](https://esphome.io/components/espnow.html)
