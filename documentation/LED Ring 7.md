# LED Ring 7 Documentation

Complete documentation for the DEBO LED RING 7 (WS2812) implementation in ESPHome.

---

## Hardware Specifications

**Product**: DEBO LED RING 7  
**LEDs**: 7x WS2812 RGB LEDs  
**Chipset**: WS2812  
**RGB Order**: GRB  
**Pin**: GPIO2 (on ESP32-C6)  
**Power**: 5V  
**Protocol**: Single-wire addressable RGB

---

## ESPHome Configuration

### Light Platform

```yaml
light:
  - platform: esp32_rmt_led_strip
    rgb_order: GRB
    chipset: ws2812
    pin: GPIO2
    num_leds: 7
    name: "DEBO LED RING 7"
    id: led_ring_physical
    internal: false
    restore_mode: RESTORE_DEFAULT_OFF
```

**Key Settings:**

- `esp32_rmt_led_strip`: Uses ESP32's RMT peripheral for reliable timing
- `restore_mode: RESTORE_DEFAULT_OFF`: LEDs start OFF after boot/restart
- `internal: false`: Exposed to Home Assistant for manual control

### Custom Effect: Random Chase

Located in: [`led_effects.h`](file:///c:/Users/thomas.engeroff/Documents/ESPHome-Projekte/ESPHome-SlaveTest/led_effects.h)

```yaml
effects:
  - addressable_lambda:
      name: "Random Chase"
      update_interval: 50ms
      lambda: random_chase_effect(it);
```

**Animation Behavior:**

1. **Fill Phase (2 seconds)**: LEDs progressively light up in a random color
   - Each LED takes ~285ms to activate (2000ms / 7 LEDs)
   - All LEDs display the same color during this phase
2. **Wait Phase (3 seconds)**: All LEDs turn OFF
3. **Repeat**: Cycle restarts with a new random color

**Total Cycle Time**: 5 seconds (2s fill + 3s wait)

---

## Switch Controls

### LED Animation Switch

```yaml
switch:
  - platform: template
    name: "LED Animation Switch"
    id: led_animation_switch
    optimistic: true
    turn_on_action:
      - logger.log: "LED Animation: ON"
      - light.turn_on:
          id: led_ring_physical
          brightness: 100%
      - delay: 100ms
      - light.turn_on:
          id: led_ring_physical
          effect: "Random Chase"
    turn_off_action:
      - logger.log: "LED Animation: OFF"
      - light.turn_off: led_ring_physical
```

**How It Works:**

1. First turns on the light at 100% brightness
2. Small delay (100ms) ensures light is initialized
3. Applies the "Random Chase" effect
4. OFF action stops the effect and turns off LEDs

---

## Implementation Details

### Color System

**Important**: ESPHome's `addressable_lambda` effects require `Color` objects (RGB), **not** `ESPHSVColor`.

#### HSV to RGB Conversion

Due to ESPHome limitations, manual HSV-to-RGB conversion is performed:

```cpp
// Convert HSV (0-360, 0-1, 0-1) to RGB (0-255)
float h = current_hue / 360.0f;
float s = 1.0f;  // Full saturation
float v = 1.0f;  // Full brightness

// HSV to RGB algorithm
int hi = (int)(h * 6);
float f = h * 6 - hi;
float p = v * (1 - s);
float q = v * (1 - f * s);
float t = v * (1 - (1 - f) * s);

// Convert based on hue sector
float r, g, b;
switch (hi % 6) {
  case 0: r = v; g = t; b = p; break;
  case 1: r = q; g = v; b = p; break;
  case 2: r = p; g = v; b = t; break;
  case 3: r = p; g = q; b = v; break;
  case 4: r = t; g = p; b = v; break;
  case 5: r = v; g = p; b = q; break;
}

// Scale to 0-255 and apply
uint8_t red = (uint8_t)(r * 255);
uint8_t green = (uint8_t)(g * 255);
uint8_t blue = (uint8_t)(b * 255);
it[i] = Color(red, green, blue);
```

### State Machine

The animation uses a simple state machine:

```
┌──────────┐
│  State 0 │  Fill Phase (2s)
│  (FILL)  │  - Progressive LED activation
└────┬─────┘  - Single random color
     │
     ▼
┌──────────┐
│  State 1 │  Wait Phase (3s)
│  (WAIT)  │  - All LEDs OFF
└────┬─────┘
     │
     └──────► Back to FILL with new random color
```

### Watchdog Timer

Prevents stuck states due to timing issues:

```cpp
if (elapsed > 6000) {  // 6s = 5s cycle + 1s margin
  state = 0;           // Reset to FILL
  state_start_time = now;
}
```

---

## Troubleshooting

### Issue: LEDs Not Lighting

**Symptoms:**

- Animation logs show function is running
- LEDs remain dark
- Manual control works fine

**Root Cause**: Using `ESPHSVColor` instead of `Color` objects

**Solution**: Ensure all LED assignments use `Color(r, g, b)`:

```cpp
// ❌ WRONG - Doesn't work with addressable_lambda
it[i] = esphome::light::ESPHSVColor(hue, 1.0f, 1.0f);

// ✅ CORRECT - Works properly
it[i] = Color(red, green, blue);
```

### Issue: Incorrect Colors

**Check:**

1. RGB order setting: Should be `GRB` for WS2812
2. HSV conversion: Verify hue is 0-360, saturation/value are 0-1
3. RGB scaling: Values should be 0-255

### Issue: Flickering or Unstable

**Possible Causes:**

- Insufficient power supply
- Long wires causing signal degradation
- Update interval too fast

**Solutions:**

- Use dedicated 5V power supply for LEDs
- Keep data wire short (<1m) or use level shifter
- Increase `update_interval` from 50ms to 100ms if needed

### Issue: Effect Not Starting

**Check:**

1. Light must be turned ON before applying effect
2. Brightness must be > 0%
3. Verify switch sequence:

   ```yaml
   - light.turn_on:
       id: led_ring_physical
       brightness: 100%
   - delay: 100ms
   - light.turn_on:
       id: led_ring_physical
       effect: "Random Chase"
   ```

---

## Log Messages

During normal operation, you'll see these log messages:

```
[I][led_anim]: Fill complete → Wait (Hue: 180)
[I][led_anim]: Wait complete → Fill (New Hue: 270)
[I][led_anim]: Fill complete → Wait (Hue: 45)
```

These indicate state transitions and confirm the animation is running correctly.

---

## References

**ESPHome Documentation:**

- [Addressable Light](https://esphome.io/components/light/index.html#addressable-light)
- [ESP32 RMT LED Strip](https://esphome.io/components/light/esp32_rmt_led_strip.html)
- [Light Effects](https://esphome.io/components/light/index.html#light-effects)

**Similar Implementation:**

- [ESPHome Analog Clock](https://github.com/markusressel/ESPHome-Analog-Clock) - Reference for Color object usage

---

## Files

- **Effect Logic**: [`led_effects.h`](file:///c:/Users/thomas.engeroff/Documents/ESPHome-Projekte/ESPHome-SlaveTest/led_effects.h)
- **Configuration**: [`esptest.yaml`](file:///c:/Users/thomas.engeroff/Documents/ESPHome-Projekte/ESPHome-SlaveTest/esptest.yaml) (lines 356-374)

---

*Last Updated: 2026-01-25*
