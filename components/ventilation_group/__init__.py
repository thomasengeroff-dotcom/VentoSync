import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan, switch, sensor
from esphome.const import CONF_ID, CONF_TEMPERATURE, CONF_HUMIDITY

# Namespace esphome
VentilationController = cg.esphome_ns.class_('VentilationController', cg.Component)

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

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(VentilationController),
    cv.Required(CONF_FLOOR_ID): cv.int_,
    cv.Required(CONF_ROOM_ID): cv.int_,
    cv.Required(CONF_DEVICE_ID): cv.int_,
    cv.Required(CONF_IS_PHASE_A): cv.boolean,
    cv.Required(CONF_MAIN_FAN): cv.use_id(fan.Fan),
    cv.Optional(CONF_DIRECTION_SWITCH): cv.use_id(switch.Switch), 
    cv.Optional(CONF_FAN_RPM_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_BOARD_TEMP_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_SCD41_TEMP_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_BME680_TEMP_SENSOR): cv.use_id(sensor.Sensor),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    # Add include
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
