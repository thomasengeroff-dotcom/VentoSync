#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/core/component.h"
#include <mutex>
#include <string>
#include <vector>

namespace esphome {
namespace wrg_dashboard {

struct DashboardAction {
  std::string key;
  std::string s_value;
  float f_value;
  bool is_string;
};

class WrgDashboard : public Component, public AsyncWebHandler {
public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override {
    return setup_priority::WIFI - 1.0f;
  }

  // Web server
  void set_web_server_base(web_server_base::WebServerBase *b) { base_ = b; }

  // Dependencies
  void set_temperature(sensor::Sensor *s) { temperature_ = s; }
  void set_pressure(sensor::Sensor *s) { pressure_ = s; }
  void set_outdoor_humidity(sensor::Sensor *s) { outdoor_humidity_ = s; }
  void set_scd41_co2(sensor::Sensor *s) { scd41_co2_ = s; }
  void set_effective_co2(sensor::Sensor *s) { effective_co2_ = s; }
  void set_scd41_temperature(sensor::Sensor *s) { scd41_temperature_ = s; }
  void set_scd41_humidity(sensor::Sensor *s) { scd41_humidity_ = s; }
  void set_temp_zuluft(sensor::Sensor *s) { temp_zuluft_ = s; }
  void set_temp_abluft(sensor::Sensor *s) { temp_abluft_ = s; }
  void set_heat_recovery_efficiency(sensor::Sensor *s) {
    heat_recovery_efficiency_ = s;
  }
  void set_fan_rpm(sensor::Sensor *s) { fan_rpm_ = s; }
  void set_filter_operating_days(sensor::Sensor *s) {
    filter_operating_days_ = s;
  }

  void set_filter_change_alarm(binary_sensor::BinarySensor *b) {
    filter_change_alarm_ = b;
  }
  void set_radar_presence(binary_sensor::BinarySensor *b) {
    radar_presence_ = b;
  }

  void set_scd41_co2_bewertung(text_sensor::TextSensor *t) {
    scd41_co2_bewertung_ = t;
  }
  void set_device_id(text_sensor::TextSensor *t) { device_id_ = t; }
  void set_floor_id(text_sensor::TextSensor *t) { floor_id_ = t; }
  void set_room_id(text_sensor::TextSensor *t) { room_id_ = t; }
  void set_phase(text_sensor::TextSensor *t) { phase_ = t; }
  void set_direction_display(text_sensor::TextSensor *t) {
    direction_display_ = t;
  }


  void set_vent_timer(number::Number *n) { vent_timer_ = n; }
  void set_sync_interval_config(number::Number *n) {
    sync_interval_config_ = n;
  }
  void set_fan_intensity_display(number::Number *n) {
    fan_intensity_display_ = n;
  }
  void set_automatik_min_luefterstufe(number::Number *n) {
    automatik_min_luefterstufe_ = n;
  }
  void set_automatik_max_luefterstufe(number::Number *n) {
    automatik_max_luefterstufe_ = n;
  }
  void set_auto_co2_threshold(number::Number *n) { auto_co2_threshold_ = n; }
  void set_auto_humidity_threshold(number::Number *n) {
    auto_humidity_threshold_ = n;
  }
  void set_auto_presence_slider(number::Number *n) {
    auto_presence_slider_ = n;
  }

  void set_luefter_modus(select::Select *s) { luefter_modus_ = s; }
  void set_lueftung_fan(fan::Fan *f) { lueftung_fan_ = f; }

  // AsyncWebHandler overrides
  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;
  bool isRequestHandlerTrivial() const override { return false; }

protected:
  void handle_root_(AsyncWebServerRequest *request);
  void handle_state_(AsyncWebServerRequest *request);
  void handle_set_(AsyncWebServerRequest *request);
  void dispatch_set_(const std::string &key, const std::string &sval,
                     float fval, bool is_string);

  std::vector<DashboardAction> action_queue_;
  std::mutex action_mutex_;

  web_server_base::WebServerBase *base_{nullptr};

  // Internal references
  sensor::Sensor *temperature_{nullptr};
  sensor::Sensor *pressure_{nullptr};
  sensor::Sensor *outdoor_humidity_{nullptr};
  sensor::Sensor *scd41_co2_{nullptr};
  sensor::Sensor *effective_co2_{nullptr};
  sensor::Sensor *scd41_temperature_{nullptr};
  sensor::Sensor *scd41_humidity_{nullptr};
  sensor::Sensor *temp_zuluft_{nullptr};
  sensor::Sensor *temp_abluft_{nullptr};
  sensor::Sensor *heat_recovery_efficiency_{nullptr};
  sensor::Sensor *fan_rpm_{nullptr};
  sensor::Sensor *filter_operating_days_{nullptr};

  binary_sensor::BinarySensor *filter_change_alarm_{nullptr};
  binary_sensor::BinarySensor *radar_presence_{nullptr};

  text_sensor::TextSensor *scd41_co2_bewertung_{nullptr};
  text_sensor::TextSensor *device_id_{nullptr};
  text_sensor::TextSensor *floor_id_{nullptr};
  text_sensor::TextSensor *room_id_{nullptr};
  text_sensor::TextSensor *phase_{nullptr};
  text_sensor::TextSensor *direction_display_{nullptr};

  number::Number *vent_timer_{nullptr};
  number::Number *sync_interval_config_{nullptr};
  number::Number *fan_intensity_display_{nullptr};
  number::Number *automatik_min_luefterstufe_{nullptr};
  number::Number *automatik_max_luefterstufe_{nullptr};
  number::Number *auto_co2_threshold_{nullptr};
  number::Number *auto_humidity_threshold_{nullptr};
  number::Number *auto_presence_slider_{nullptr};

  select::Select *luefter_modus_{nullptr};
  fan::Fan *lueftung_fan_{nullptr};
};

} // namespace wrg_dashboard
} // namespace esphome