import hashlib
import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan, switch, sensor, binary_sensor
from esphome.const import CONF_ID

_LOGGER = logging.getLogger(__name__)

# Namespace & Classes
VentilationController = cg.esphome_ns.class_('VentilationController', cg.Component)
GlobalsComponent = cg.esphome_ns.class_('globals::GlobalsComponent', cg.Component)

CONF_FLOOR_ID = 'floor_id'
CONF_ROOM_ID = 'room_id'
CONF_DEVICE_ID = 'device_id'
CONF_IS_PHASE_A = 'is_phase_a'
CONF_MAIN_FAN = 'main_fan'
CONF_DIRECTION_SWITCH = 'direction_switch'
CONF_FAN_RPM_SENSOR = 'fan_rpm_sensor'
CONF_BOARD_TEMP_SENSOR = 'board_temp_sensor'
CONF_SCD41_TEMP_SENSOR = 'scd41_temp_sensor'
CONF_BME680_TEMP_SENSOR = 'bme680_temp_sensor'
CONF_MODE_INDEX_GLOBAL = 'mode_index_global'
CONF_AUTO_MIN_GLOBAL = 'automatik_min_fan_level_global'
CONF_AUTO_MAX_GLOBAL = 'automatik_max_fan_level_global'
CONF_AUTO_CO2_GLOBAL = 'auto_co2_threshold_global'
CONF_AUTO_HUM_GLOBAL = 'auto_humidity_threshold_global'
CONF_AUTO_PRES_GLOBAL = 'auto_presence_global'
CONF_LED_BRIGHT_GLOBAL = 'max_led_brightness_global'
CONF_CO2_SENSOR = 'co2_sensor'
CONF_SCD41_HUM_SENSOR = 'scd41_humidity_sensor'
CONF_BME680_HUM_SENSOR = 'bme680_humidity_sensor'
CONF_HVAC_CO2_GLOBAL = 'hvac_co2_threshold_global'
CONF_HVAC_EMERGENCY_GLOBAL = 'hvac_emergency_co2_global'
CONF_HVAC_MAX_GLOBAL = 'hvac_max_fan_level_global'
CONF_VACATION_STATE_GLOBAL = 'vacation_state_global'
CONF_PRE_VAC_MODE_GLOBAL = 'pre_vacation_mode_index_global'
CONF_PRE_VAC_INTENSITY_GLOBAL = 'pre_vacation_intensity_global'
CONF_VAC_PRESET_MODE_GLOBAL = 'vacation_preset_mode_global'
CONF_VAC_PRESET_LEVEL_GLOBAL = 'vacation_preset_level_global'
CONF_HVAC_ENABLED_GLOBAL = 'hvac_enabled_global'
CONF_NOX_BOOST_ENABLED_GLOBAL = 'nox_boost_enabled_global'
CONF_OVERRIDE_DURATION_GLOBAL = 'auto_override_duration_global'
CONF_ESPNOW_KEY = 'espnow_key'

# Public example passphrase from secrets_example.yaml — warn when it is used.
EXAMPLE_ESPNOW_KEY = 'CHANGE-ME-ventosync-room-key'


def espnow_auth_key_hex(passphrase: str) -> str:
    """32-byte HMAC key for the ESP-NOW frame authentication (protocol v13).

    Derived at compile time, so the firmware never contains the passphrase
    itself. Domain-separated from other uses of the same secret.
    """
    return hashlib.sha256(('VentoSync ESP-NOW auth v1:' + passphrase).encode('utf-8')).hexdigest()

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(VentilationController),
    cv.Required(CONF_FLOOR_ID): cv.uint8_t,
    cv.Required(CONF_ROOM_ID): cv.uint8_t,
    cv.Required(CONF_DEVICE_ID): cv.uint8_t,
    cv.Required(CONF_IS_PHASE_A): cv.boolean,
    cv.Required(CONF_MAIN_FAN): cv.use_id(fan.Fan),
    cv.Optional(CONF_DIRECTION_SWITCH): cv.use_id(switch.Switch), 
    cv.Optional(CONF_FAN_RPM_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_BOARD_TEMP_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_SCD41_TEMP_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_BME680_TEMP_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_MODE_INDEX_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_AUTO_MIN_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_AUTO_MAX_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_AUTO_CO2_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_AUTO_HUM_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_AUTO_PRES_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_LED_BRIGHT_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_CO2_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_SCD41_HUM_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_BME680_HUM_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_HVAC_CO2_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_HVAC_EMERGENCY_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_HVAC_MAX_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_VACATION_STATE_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_PRE_VAC_MODE_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_PRE_VAC_INTENSITY_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_VAC_PRESET_MODE_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_VAC_PRESET_LEVEL_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_HVAC_ENABLED_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_NOX_BOOST_ENABLED_GLOBAL): cv.use_id(GlobalsComponent),
    cv.Optional(CONF_OVERRIDE_DURATION_GLOBAL): cv.use_id(GlobalsComponent),
    # Room key for authenticated ESP-NOW frames — must be identical on all
    # devices of a room (secrets.yaml: espnow_key).
    cv.Required(CONF_ESPNOW_KEY): cv.All(cv.string_strict, cv.Length(min=16, max=128)),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    cg.add_global(cg.RawStatement('#include "esphome/components/ventilation_group/ventilation_group.h"'))

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_floor_id(config[CONF_FLOOR_ID]))
    cg.add(var.set_room_id(config[CONF_ROOM_ID]))
    cg.add(var.set_device_id(config[CONF_DEVICE_ID]))
    cg.add(var.set_is_phase_a(config[CONF_IS_PHASE_A]))

    if config[CONF_ESPNOW_KEY] == EXAMPLE_ESPNOW_KEY:
        _LOGGER.warning(
            "espnow_key is the public example value from secrets_example.yaml — "
            "anyone can forge ESP-NOW frames for this device. Set your own key."
        )
    cg.add(var.set_espnow_auth_key_hex(espnow_auth_key_hex(config[CONF_ESPNOW_KEY])))
    
    fan_ = await cg.get_variable(config[CONF_MAIN_FAN])
    cg.add(var.set_main_fan(fan_))
    
    if CONF_DIRECTION_SWITCH in config:
        sw = await cg.get_variable(config[CONF_DIRECTION_SWITCH])
        cg.add(var.set_direction_switch(sw))

    if CONF_FAN_RPM_SENSOR in config:
        s = await cg.get_variable(config[CONF_FAN_RPM_SENSOR])
        cg.add(var.set_fan_rpm_sensor(s))
    if CONF_BOARD_TEMP_SENSOR in config:
        s = await cg.get_variable(config[CONF_BOARD_TEMP_SENSOR])
        cg.add(var.set_board_temp_sensor(s))
    if CONF_SCD41_TEMP_SENSOR in config:
        s = await cg.get_variable(config[CONF_SCD41_TEMP_SENSOR])
        cg.add(var.set_scd41_temp_sensor(s))
    if CONF_BME680_TEMP_SENSOR in config:
        s = await cg.get_variable(config[CONF_BME680_TEMP_SENSOR])
        cg.add(var.set_bme680_temp_sensor(s))

    if CONF_MODE_INDEX_GLOBAL in config:
        g = await cg.get_variable(config[CONF_MODE_INDEX_GLOBAL])
        cg.add(var.set_mode_index_global(g))
    if CONF_AUTO_MIN_GLOBAL in config:
        g = await cg.get_variable(config[CONF_AUTO_MIN_GLOBAL])
        cg.add(var.set_automatik_min_fan_level_global(g))
    if CONF_AUTO_MAX_GLOBAL in config:
        g = await cg.get_variable(config[CONF_AUTO_MAX_GLOBAL])
        cg.add(var.set_automatik_max_fan_level_global(g))
    if CONF_AUTO_CO2_GLOBAL in config:
        g = await cg.get_variable(config[CONF_AUTO_CO2_GLOBAL])
        cg.add(var.set_auto_co2_threshold_global(g))
    if CONF_AUTO_HUM_GLOBAL in config:
        g = await cg.get_variable(config[CONF_AUTO_HUM_GLOBAL])
        cg.add(var.set_auto_humidity_threshold_global(g))
    if CONF_AUTO_PRES_GLOBAL in config:
        g = await cg.get_variable(config[CONF_AUTO_PRES_GLOBAL])
        cg.add(var.set_auto_presence_global(g))
    if (CONF_LED_BRIGHT_GLOBAL in config):
        g = await cg.get_variable(config[CONF_LED_BRIGHT_GLOBAL])
        cg.add(var.set_max_led_brightness_global(g))


    if CONF_CO2_SENSOR in config:
        s = await cg.get_variable(config[CONF_CO2_SENSOR])
        cg.add(var.set_co2_sensor(s))
    if CONF_SCD41_HUM_SENSOR in config:
        s = await cg.get_variable(config[CONF_SCD41_HUM_SENSOR])
        cg.add(var.set_scd41_humidity_sensor(s))
    if CONF_BME680_HUM_SENSOR in config:
        s = await cg.get_variable(config[CONF_BME680_HUM_SENSOR])
        cg.add(var.set_bme680_humidity_sensor(s))
    if CONF_HVAC_CO2_GLOBAL in config:
        g = await cg.get_variable(config[CONF_HVAC_CO2_GLOBAL])
        cg.add(var.set_hvac_co2_threshold_global(g))
    if CONF_HVAC_EMERGENCY_GLOBAL in config:
        g = await cg.get_variable(config[CONF_HVAC_EMERGENCY_GLOBAL])
        cg.add(var.set_hvac_emergency_co2_global(g))
    if CONF_HVAC_MAX_GLOBAL in config:
        g = await cg.get_variable(config[CONF_HVAC_MAX_GLOBAL])
        cg.add(var.set_hvac_max_fan_level_global(g))
    if CONF_VACATION_STATE_GLOBAL in config:
        g = await cg.get_variable(config[CONF_VACATION_STATE_GLOBAL])
        cg.add(var.set_vacation_state_global(g))
    if CONF_PRE_VAC_MODE_GLOBAL in config:
        g = await cg.get_variable(config[CONF_PRE_VAC_MODE_GLOBAL])
        cg.add(var.set_pre_vacation_mode_index_global(g))
    if CONF_PRE_VAC_INTENSITY_GLOBAL in config:
        g = await cg.get_variable(config[CONF_PRE_VAC_INTENSITY_GLOBAL])
        cg.add(var.set_pre_vacation_intensity_global(g))
    if CONF_VAC_PRESET_MODE_GLOBAL in config:
        g = await cg.get_variable(config[CONF_VAC_PRESET_MODE_GLOBAL])
        cg.add(var.set_vacation_preset_mode_global(g))
    if CONF_VAC_PRESET_LEVEL_GLOBAL in config:
        g = await cg.get_variable(config[CONF_VAC_PRESET_LEVEL_GLOBAL])
        cg.add(var.set_vacation_preset_level_global(g))
    if CONF_HVAC_ENABLED_GLOBAL in config:
        g = await cg.get_variable(config[CONF_HVAC_ENABLED_GLOBAL])
        cg.add(var.set_hvac_enabled_global(g))
    if CONF_NOX_BOOST_ENABLED_GLOBAL in config:
        g = await cg.get_variable(config[CONF_NOX_BOOST_ENABLED_GLOBAL])
        cg.add(var.set_nox_boost_enabled_global(g))
    if CONF_OVERRIDE_DURATION_GLOBAL in config:
        g = await cg.get_variable(config[CONF_OVERRIDE_DURATION_GLOBAL])
        cg.add(var.set_auto_override_duration_global(g))
