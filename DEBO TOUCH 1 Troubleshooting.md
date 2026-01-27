# DEBO TOUCH 1 Troubleshooting

## Problem

Touch button on GPIO21 generates NO events in logs despite proper configuration.

## Already Applied Fixes

1. ✅ Changed `INPUT_PULLDOWN` → `pullup: true`
2. ✅ Added debounce filters (50ms)
3. ✅ Fixed action from `fan_auto_cycle` → `switch_display`
4. ✅ Added debug logging (on_press, on_release, on_state)

## Current Result

**No logs appear** - button is not being detected at all.

## Possible Root Causes

### 1. Wrong GPIO Pin

- Configuration says GPIO21 (Pin D3 on XIAO ESP32C6)
- Sensor might be connected to a different pin
- **Action**: Verify physical connection

### 2. Inverted Logic

- Some capacitive touch sensors are "active LOW"
- Need `inverted: true` in configuration
- **Action**: Try inverting the logic

### 3. Hardware Issue

- Sensor not receiving power
- Defective sensor
- Loose connection
- **Action**: Check wiring and power

### 4. Wrong Pull Mode

- Current: `pullup: true`
- Some sensors need `pulldown: true` instead
- **Action**: Try both modes

## Next Diagnostic Steps

### Step 1: Verify GPIO Pin

Check XIAO ESP32C6 pinout:

- D0 = GPIO0
- D1 = GPIO1
- D2 = GPIO2
- **D3 = GPIO21** ← Currently configured
- D4 = GPIO3
- D5 = GPIO4
- D6 = GPIO5

### Step 2: Test with Inverted Logic

```yaml
binary_sensor:
  - platform: gpio
    pin:
      number: GPIO21
      mode:
        input: true
        pullup: true
      inverted: true  # ← Add this
```

### Step 3: Try Different Pull Modes

Test matrix:

- pullup: true, inverted: false ← **Currently testing**
- pullup: true, inverted: true
- pulldown: true, inverted: false
- pulldown: true, inverted: true

### Step 4: GPIO Pin Scanner

Create a test configuration that monitors multiple pins and logs their states to identify which pin the sensor is actually on.

## Questions for User

1. Can you physically verify the DEBO TOUCH 1 is connected?
2. Which GPIO pin is it connected to? (Check the board labels)
3. Does the sensor have an LED that lights when touched?
4. Is the sensor getting 3.3V power?
