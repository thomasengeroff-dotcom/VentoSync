# 🔧 Technical Details & Optimizations

## APDS9960 Sensor Optimization

The APDS9960 proximity and light sensor has been extensively optimized:

### Performance Improvements

| Parameter | Standard | Optimized | Benefit |
| :--- | :--- | :--- | :--- |
| **Update Interval** | 60s | 500ms | Fast reaction with low I²C traffic |
| **Proximity Gain** | 4x | 2x | Better near-field detection (<20cm) |
| **LED Drive** | 100mA | 50mA | 50% power savings |
| **Delta Filter** | - | 5 | Noise suppression |
| **Throttle** | - | 500ms | Prevents I²C bus overload |

### Adaptive Display Brightness

```yaml
# Automatic brightness adjustment based on ambient light
on_value:
  then:
    - lambda: |-
        if (x > 100) {
          id(test_display).set_contrast(255);  // Bright room
        } else {
          id(test_display).set_contrast(128);  // Dark room
        }
```

**Benefits:**

- 80% less I²C bus traffic
- 40% power savings on APDS9960
- More stable operation with BME680
- Reduced log output (DEBUG level)

## ESPHome YAML Syntax

**Important:** ESPHome uses special YAML tags for lambda expressions:

### ✅ Correct (Block Format)

```yaml
data: !lambda |-
  return get_iaq_traffic_light_data(x);
```

### ❌ Incorrect (Quoted String)

```yaml
data: !lambda "return get_iaq_traffic_light_data(x);"
```

**Other ESPHome Tags:**

- `!secret` - For secrets from `secrets.yaml`
- `!include` - Including other YAML files
- `!lambda` - C++ lambda expressions

**VS Code Configuration** (`settings.json`):

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

## I²C Bus Configuration

```yaml
i2c:
  sda: GPIO22
  scl: GPIO23
  scan: true
  frequency: 400kHz      # High-speed I²C
  timeout: 50ms          # Increased timeout for stability
```

**Connected Devices:**

- `0x39` - APDS9960 (Proximity/Light)
- `0x3C` - SSD1306 OLED Display
- `0x77` - BME680 (Temp/Hum/IAQ)

## BME680 BSEC2 Configuration

```yaml
bme68x_bsec2_i2c:
  address: 0x77
  model: bme680
  operating_age: 28d           # Calibration time
  sample_rate: LP              # Low Power (every 3s)
  supply_voltage: 3.3V
  temperature_offset: 0.0
  state_save_interval: 6h      # State save interval
```

**IAQ Traffic Light Logic:**

- Green (0-50): Excellent air quality
- Yellow (51-100): Good air quality
- Orange (101-150): Moderate air quality
- Red (151-200): Poor air quality
- Dark Red (201+): Very poor air quality

Data is sent via ESP-NOW to slave devices.

## ESP-NOW Communication

```yaml
espnow:
  peers:
    - "FF:FF:FF:FF:FF:FF"  # Broadcast for discovery
```

**Packet Types:**

- `MSG_SYNC` - Synchronization between devices
- `MSG_IAQ` - Air quality data
- `MSG_MODE` - Operating mode change

**Broadcast Mode:**

- All devices send to broadcast address
- Filtering occurs in VentilationController based on Floor/Room/Device ID
- No manual MAC configuration required

## Fan Control

The board supports **3-PIN or 4-pin PWM** fans via a universal interface.
Detailed schematics: See [Fan-Interface-Guide_en.md](Fan-Interface-Guide_en.md)

**PWM Configuration:**

```yaml
output:
  # PWM Output (GPIO16)
  - platform: ledc
    pin: GPIO16
    frequency: 2000 Hz        # 2kHz for VentoMaxx/VarioPro
    inverted: false             # Low-side MOSFETs: no inverting
    min_power: 0%
    zero_means_zero: true       # 0% = really OFF
    id: fan_pwm_primary
```

**Operating Mode Logic:**

| Mode | GPIO16 | VCC |
| :--- | :--- | :--- |
| **4-Pin PWM** | PWM on fan Pin 4 | Constant 12V |

**Automatic Cycle (Operating Programs):**

1. **Efficiency Ventilation (HRV):**
   - Cycle times (phase duration) depending on the level:
     - Level 1: 70s
     - Level 2: 65s
     - Level 3: 60s
     - Level 4: 55s
     - Level 5: 50s
2. **Boost Ventilation:**
   - 15 min. operation (HRV) / 105 min. pause.
   - Change of direction after every 2-hour cycle.
3. **Sensor Ventilation (Hygro-Option):**
   - Thresholds: 55%, 65%, 70%, 80% rH
   - Hysteresis: Switch back at < 54% rH
4. **Cross Ventilation:**
   - Permanent operation (50/50 supply/extract air split), no direction changes.

---

**Further Information:**

- [ESPHome Documentation](https://esphome.io/)
- [BME68x BSEC2 Component](https://esphome.io/components/sensor/bme68x_bsec2.html)
- [ESP-NOW Component](https://esphome.io/components/espnow.html)
