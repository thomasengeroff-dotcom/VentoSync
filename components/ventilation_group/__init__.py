import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan, switch, sensor, binary_sensor
from esphome.const import CONF_ID

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
CONF_WINDOW_SENSOR = 'window_sensor'

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
    cv.Optional(CONF_WINDOW_SENSOR): cv.use_id(binary_sensor.BinarySensor),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    cg.add_global(cg.RawStatement('#include "esphome/components/ventilation_group/ventilation_group.h"'))

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_floor_id(config[CONF_FLOOR_ID]))
    cg.add(var.set_room_id(config[CONF_ROOM_ID]))
    cg.add(var.set_device_id(config[CONF_DEVICE_ID]))
    cg.add(var.set_is_phase_a(config[CONF_IS_PHASE_A]))
    
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

    if (CONF_WINDOW_SENSOR in config):
        s = await cg.get_variable(config[CONF_WINDOW_SENSOR])
        cg.add(var.set_window_sensor(s))
