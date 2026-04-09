import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import web_server_base
from esphome.const import CONF_ID

DEPENDENCIES = ["web_server_base", "network"]
AUTO_LOAD = ["web_server_base"]

CONF_WEB_SERVER_BASE_ID = "web_server_base_id"

from esphome.components import sensor, binary_sensor, text_sensor, number, select, fan

wrg_dashboard_ns = cg.esphome_ns.namespace("wrg_dashboard")
WrgDashboard = wrg_dashboard_ns.class_("WrgDashboard", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WrgDashboard),
        cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(web_server_base.WebServerBase),

        cv.Optional("temperature_id"):              cv.use_id(sensor.Sensor),
        cv.Optional("pressure_id"):                 cv.use_id(sensor.Sensor),
        cv.Optional("outdoor_humidity_id"):         cv.use_id(sensor.Sensor),
        cv.Optional("scd41_co2_id"):                cv.use_id(sensor.Sensor),
        cv.Optional("effective_co2_id"):            cv.use_id(sensor.Sensor),
        cv.Optional("scd41_temperature_id"):        cv.use_id(sensor.Sensor),
        cv.Optional("scd41_humidity_id"):           cv.use_id(sensor.Sensor),
        cv.Optional("bme680_temperature_id"):        cv.use_id(sensor.Sensor),
        cv.Optional("bme680_humidity_id"):           cv.use_id(sensor.Sensor),
        cv.Optional("temp_zuluft_id"):              cv.use_id(sensor.Sensor),
        cv.Optional("temp_abluft_id"):              cv.use_id(sensor.Sensor),
        cv.Optional("heat_recovery_efficiency_id"): cv.use_id(sensor.Sensor),
        cv.Optional("fan_rpm_id"):                  cv.use_id(sensor.Sensor),
        cv.Optional("filter_operating_days_id"):    cv.use_id(sensor.Sensor),

        cv.Optional("filter_change_alarm_id"):      cv.use_id(binary_sensor.BinarySensor),
        cv.Optional("radar_presence_id"):           cv.use_id(binary_sensor.BinarySensor),

        cv.Optional("scd41_co2_bewertung_id"):      cv.use_id(text_sensor.TextSensor),
        cv.Optional("device_id_id"):                cv.use_id(text_sensor.TextSensor),
        cv.Optional("floor_id_id"):                 cv.use_id(text_sensor.TextSensor),
        cv.Optional("room_id_id"):                  cv.use_id(text_sensor.TextSensor),
        cv.Optional("phase_id"):                    cv.use_id(text_sensor.TextSensor),
        cv.Optional("direction_display_id"):        cv.use_id(text_sensor.TextSensor),

        cv.Optional("vent_timer_id"):               cv.use_id(number.Number),
        cv.Optional("sync_interval_config_id"):     cv.use_id(number.Number),
        cv.Optional("fan_intensity_display_id"):    cv.use_id(number.Number),
        cv.Optional("automatik_min_luefterstufe_id"): cv.use_id(number.Number),
        cv.Optional("automatik_max_luefterstufe_id"): cv.use_id(number.Number),
        cv.Optional("auto_co2_threshold_id"):       cv.use_id(number.Number),
        cv.Optional("auto_humidity_threshold_id"):  cv.use_id(number.Number),
        cv.Optional("auto_presence_slider_id"):     cv.use_id(number.Number),

        cv.Optional("luefter_modus_id"):            cv.use_id(select.Select),

        cv.Optional("lueftung_fan_id"):             cv.use_id(fan.Fan),

        cv.Optional("ventilation_ctrl_id"):         cv.use_id(cg.Component),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    cg.add_global(cg.RawStatement('#include "esphome/components/wrg_dashboard/wrg_dashboard.h"'))
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    web_server_base_var = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    cg.add(var.set_web_server_base(web_server_base_var))

    for key in config.keys():
        if key.endswith("_id") and key not in [CONF_ID, CONF_WEB_SERVER_BASE_ID]:
            sens = await cg.get_variable(config[key])
            func_name = f"set_{key[:-3]}"
            cg.add(getattr(var, func_name)(sens))
