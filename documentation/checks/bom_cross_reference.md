# BOM Cross-Reference Report

**ESPHome Wohnraumlüftung v2**  
**Date**: 2026-02-06

---

## 📋 Components NOT in BOM

The following components are mentioned in `Hardware-Setup-Readme.md` but are **NOT present** in the actual PCB BOM CSV file:

### ⚠️ Missing Sensors

#### 1. **BME680** - Environmental Sensor

- **Mentioned in Readme**: Line 44 (now marked with ⚠️)
- **I2C Address**: 0x77
- **Function**: Temperature, Humidity, Pressure, IAQ (Air Quality)
- **Status**: ❌ **NOT IN BOM**
- **Impact**: Environmental sensing will not work without this component
- **Action**: Must be ordered separately if needed

#### 2. **SCD41** - CO2 Sensor

- **Mentioned in Readme**: Line 45 (now marked with ⚠️)
- **I2C Address**: 0x62
- **Connector**: H2 (4-Pin, 1.27mm pitch) - PZ127V-11-04-0720
- **Function**: CO2 measurement
- **Status**: ❌ **NOT IN BOM**
- **Impact**: CO2 monitoring will not work
- **Action**: Must be ordered separately if needed

---

### ⚠️ Component Substitutions

#### 3. **BC547** → **S8050** (NPN Transistor)

- **Readme Reference**: "An Basis BC547" (old documentation)
- **Actual BOM Component**: **S8050** (Q3, SOT-23)
- **Status**: ✅ **CORRECTED** in readme (now shows S8050)
- **Function**: Fan PWM driver (NPN transistor)
- **Note**: S8050 is functionally equivalent, higher current capability

#### 4. **Recom R-78E5.0-0.5** → **TSR 1-2450**

- **Readme Reference**: "Recom R-78E5.0-0.5 oder Traco TSR 1-2450"
- **Actual BOM Component**: **TSR 1-2450** (U2, Traco Power)
- **Status**: ✅ **CORRECTED** in readme
- **Function**: 12V → 5V DC/DC converter (500mA)
- **Note**: Both are pin-compatible SIP modules

---

### ⚠️ Design Changes

#### 5. **MCP23017** I/O Expander - NOT USED

- **Original Plan**: Control LEDs and read buttons via I2C expander
- **Actual Design**:
  - **LEDs**: Controlled via **PCA9685** PWM driver (dimmable)
  - **Buttons**: Direct connection to ESP32 GPIOs (D8, D9, D10)
- **Status**: ✅ **CORRECTED** in readme
- **I2C Address**: 0x20 (would have been, but not used)
- **Reason for Change**:
  - PCA9685 provides PWM dimming for LEDs
  - Direct GPIO for buttons is simpler and faster

---

## ✅ Components PRESENT in BOM

### Core Components (Verified)

- ✅ **ESP32-C6** (XIAO) - U11, U12 (headers)
- ✅ **TSR 1-2450** - 5V regulator (U2)
- ✅ **AP63203WU-7** - 3.3V buck converter (U37)
- ✅ **PCA9685PW** - LED PWM driver (U26)
- ✅ **AO3401** - P-MOSFET for fan control (Q1)
- ✅ **S8050** - NPN transistor for fan driver (Q3)
- ✅ **SS14** - Schottky diode for fan protection (D1)

### Passives (Verified)

- ✅ **C1** (470µF/25V) - Main 12V buffer
- ✅ **C2** (10µF/50V) - 5V reg input
- ✅ **C13** (470µF/16V) - 5V output buffer
- ✅ **C20** (10µF/25V) - AP63203 input
- ✅ **C23, C24** (22µF/6.3V) - AP63203 output
- ✅ **C25, C6, C7, C10, C22** (100nF) - Bypass caps
- ✅ **C27** (100µF/25V) - Fan filter cap
- ✅ **L1** (220µH) - Fan LC filter inductor (SLH1207S221MTT)
- ✅ **L2** (3.9µH) - AP63203 inductor
- ✅ **R1, R2** (4.7kΩ) - I2C pullups
- ✅ **R3** (1kΩ) - Gate resistor
- ✅ **R4, R5, U24** (10kΩ 0.1%) - NTC precision resistors
- ✅ **R6** (10kΩ) - Fan pullup
- ✅ **R17** (470Ω) - LED current limit
- ✅ **RN1, RN2** (470Ω x8) - LED resistor arrays
- ✅ **RN3** (10kΩ x8) - Button pullup array
- ✅ **U27** (10kΩ 0402) - PCA9685 OE pullup

### Protection & Connectors (Verified)

- ✅ **U5-U7, U18-U23, U32** (10x PESD5V0S2BT) - TVS diodes
- ✅ **F1** (1A Fuse) - Overcurrent protection
- ✅ **V1** (S10K275 Varistor) - Overvoltage protection
- ✅ **U17** (FPC 0.5-14P) - Front panel connector
- ✅ **H1** (3-Pin header) - Fan connector
- ✅ **H2** (4-Pin header) - SCD41 connector (sensor not in BOM!)
- ✅ **U28, U29** (2-Pin screw terminals) - NTC sensors
- ✅ **U30, U31** (4-Pin screw terminals) - External connections
- ✅ **U3** (2-Pin screw terminal 5.08mm) - Power input

---

## 📊 Summary

| Category | Count | Status |
| :--- | :--- | :--- |
| **Missing Sensors** | 2 | ⚠️ BME680, SCD41 |
| **Component Substitutions** | 2 | ✅ S8050, TSR 1-2450 |
| **Design Changes** | 1 | ✅ MCP23017 → PCA9685 + Direct GPIO |
| **Total BOM Components** | 34 | ✅ All present in CSV |

---

## 🎯 Recommendations

### If you need environmental sensing

1. **Order BME680** breakout board (I2C, 0x77)
   - Connect to I2C bus (SDA/SCL)
   - Powered by 3.3V (AP63203)
   - Update ESPHome YAML to enable BME680 component

### If you need CO2 monitoring

1. **Order SCD41** sensor
   - Connect to H2 header (4-pin, 1.27mm)
   - I2C address 0x62
   - Update ESPHome YAML to enable SCD4x component

### Current functionality WITHOUT sensors

- ✅ Fan control (PWM, Tacho)
- ✅ NTC temperature sensors (2x)
- ✅ Front panel (9 LEDs, 3 buttons)
- ✅ ESP-NOW communication
- ❌ Air quality (IAQ) - requires BME680
- ❌ CO2 measurement - requires SCD41

---

**Note**: The readme has been updated to mark missing components with ⚠️ symbols.
