#include "wrg_dashboard.h"
#include "dashboard_html.h"
#include "esphome/core/log.h"
#include <ArduinoJson.h>

namespace esphome {
namespace wrg_dashboard {

static const char *const TAG = "wrg_dashboard";

bool WrgDashboard::canHandle(AsyncWebServerRequest *request) const {
  if (request->method() == HTTP_GET) {
    if (request->url() == "/ui" || request->url() == "/state" || request->url() == "/set") {
      return true;
    }
  }
  return false;
}

void WrgDashboard::setup() {
  if (this->base_ != nullptr) {
    this->base_->add_handler(this);
    ESP_LOGI(TAG, "Webserver Dashboard Handler registered");
  } else {
    ESP_LOGE(TAG, "WebServerBase not provided!");
  }
}

void WrgDashboard::loop() {
  std::vector<DashboardAction> todo;

  {
    std::lock_guard<std::mutex> lock(this->action_mutex_);
    if (this->action_queue_.empty()) {
      return;
    }
    // Efficiently move the vector out of the queue and clear the lock
    todo = std::move(this->action_queue_);
  }

  for (const auto &action : todo) {
    this->dispatch_set_(action.key, action.s_value, action.f_value, action.is_string);
  }
}

void WrgDashboard::dispatch_set_(const std::string &key, const std::string &sval, float fval, bool is_string) {
  auto doSelect = [&](select::Select *sel) {
    if (!sel) return;
    auto call = sel->make_call();
    call.set_option(sval);
    call.perform();
  };

  auto doNumber = [&](number::Number *n) {
    if (!n) return;
    auto call = n->make_call();
    call.set_value(fval);
    call.perform();
  };

  if (key == "luefter_modus") { doSelect(this->luefter_modus_); }
  else if (key == "fan_intensity_display") { doNumber(this->fan_intensity_display_); }
  else if (key == "test_speed_slider") { doNumber(this->test_speed_slider_); }
  else if (key == "automatik_min_luefterstufe") { doNumber(this->automatik_min_luefterstufe_); }
  else if (key == "automatik_max_luefterstufe") { doNumber(this->automatik_max_luefterstufe_); }
  else if (key == "auto_co2_threshold") { doNumber(this->auto_co2_threshold_); }
  else if (key == "auto_humidity_threshold") { doNumber(this->auto_humidity_threshold_); }
  else if (key == "auto_presence_slider") { doNumber(this->auto_presence_slider_); }
  else if (key == "vent_timer") { doNumber(this->vent_timer_); }
  else if (key == "sync_interval_config") { doNumber(this->sync_interval_config_); }
  else {
    ESP_LOGW(TAG, "Unknown action trigger received: %s", key.c_str());
  }
}

void WrgDashboard::handleRequest(AsyncWebServerRequest *request) {
  if (request->url() == "/ui") {
    this->handle_root_(request);
  } else if (request->url() == "/state") {
    this->handle_state_(request);
  } else if (request->url() == "/set") {
    this->handle_set_(request);
  }
}

void WrgDashboard::handle_root_(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", DASHBOARD_HTML);
  response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "0");
  request->send(response);
}

void WrgDashboard::handle_state_(AsyncWebServerRequest *request) {
  JsonDocument doc;
  auto get_f = [](sensor::Sensor *s) -> float { return (s && s->has_state()) ? s->state : (float)NAN; };
  auto get_b = [](binary_sensor::BinarySensor *b) -> bool { return (b && b->has_state()) ? b->state : false; };
  auto get_t = [](text_sensor::TextSensor *t) -> const char* { return (t && t->has_state()) ? t->state.c_str() : ""; };
  auto get_n = [](number::Number *n) -> float { return (n && n->has_state()) ? n->state : (float)NAN; };
  auto get_s = [](select::Select *s) -> const char* { return (s && s->has_state()) ? s->current_option() : ""; };
  
  doc["temperature"] = get_f(this->temperature_);
  doc["pressure"] = get_f(this->pressure_);
  doc["outdoor_humidity"] = get_f(this->outdoor_humidity_);
  doc["scd41_co2"] = get_f(this->scd41_co2_);
  doc["scd41_temperature"] = get_f(this->scd41_temperature_);
  doc["scd41_humidity"] = get_f(this->scd41_humidity_);
  doc["temp_zuluft"] = get_f(this->temp_zuluft_);
  doc["temp_abluft"] = get_f(this->temp_abluft_);
  doc["heat_recovery_efficiency"] = get_f(this->heat_recovery_efficiency_);
  doc["fan_rpm"] = get_f(this->fan_rpm_);
  doc["filter_operating_days"] = get_f(this->filter_operating_days_);

  doc["filter_change_alarm"] = get_b(this->filter_change_alarm_);
  doc["radar_presence"] = get_b(this->radar_presence_);

  doc["scd41_co2_bewertung"] = get_t(this->scd41_co2_bewertung_);
  doc["luefter_modus"] = get_s(this->luefter_modus_);

  doc["test_speed_slider"] = get_n(this->test_speed_slider_);
  doc["vent_timer"] = get_n(this->vent_timer_);
  doc["sync_interval_config"] = get_n(this->sync_interval_config_);
  doc["fan_intensity_display"] = get_n(this->fan_intensity_display_);
  doc["automatik_min_luefterstufe"] = get_n(this->automatik_min_luefterstufe_);
  doc["automatik_max_luefterstufe"] = get_n(this->automatik_max_luefterstufe_);
  doc["auto_co2_threshold"] = get_n(this->auto_co2_threshold_);
  doc["auto_humidity_threshold"] = get_n(this->auto_humidity_threshold_);
  doc["auto_presence_slider"] = get_n(this->auto_presence_slider_);

  std::string response;
  serializeJson(doc, response);
  request->send(200, "application/json", response.c_str());
}

void WrgDashboard::handle_set_(AsyncWebServerRequest *request) {
  if (request->hasParam("id") && request->hasParam("val")) {
    std::string id(request->getParam("id")->value().c_str());
    std::string val(request->getParam("val")->value().c_str());
    
    DashboardAction a;
    a.key = id;
    a.s_value = val;
    if (a.key == "luefter_modus") {
        a.is_string = true;
    } else {
        a.f_value = atof(val.c_str());
        a.is_string = false;
    }

    {
      std::lock_guard<std::mutex> lock(this->action_mutex_);
      this->action_queue_.push_back(std::move(a));
    }
    request->send(200, "text/plain", "OK");
  } else {
    request->send(400, "text/plain", "Bad Request");
  }
}

}  // namespace wrg_dashboard
}  // namespace esphome
