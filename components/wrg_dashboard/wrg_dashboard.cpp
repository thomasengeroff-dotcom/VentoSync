// ==========================================================================
// VentoSync HRV – ESPHome Custom Component
// https://github.com/thomasengeroff-dotcom/VentoSync
//
// Copyright (c) 2026 Thomas Engeroff
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
// File:        wrg_dashboard.cpp
// Description: Web server component for the room ventilation dashboard.
// Author:      Thomas Engeroff
// Created:     2026-03-09
// Modified:    2026-03-23
// ==========================================================================

#include "wrg_dashboard.h"
#include "dashboard_html.h"
#include "esphome/components/ventilation_group/ventilation_group.h"
#include "esphome/core/log.h"
#include <ArduinoJson.h>
#include <unordered_set>

namespace esphome {
namespace wrg_dashboard {

static const char *const TAG = "wrg_dashboard";

bool WrgDashboard::canHandle(AsyncWebServerRequest *request) const {
  if (request->method() == HTTP_GET) {
    if (request->url() == "/ui" || request->url() == "/state" ||
        request->url() == "/set") {
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
  static uint32_t last_check = 0;
  uint32_t now = millis();
  if (now - last_check < 100) {
    return;
  }
  last_check = now;

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
    this->dispatch_set_(action.key, action.s_value, action.f_value,
                        action.is_string);
  }
}

void WrgDashboard::dispatch_set_(const std::string &key,
                                 const std::string &sval, float fval,
                                 bool is_string) {
  auto doSelect = [&](select::Select *sel) {
    if (!sel)
      return;
    auto call = sel->make_call();
    call.set_option(sval);
    call.perform();
  };

  auto doNumber = [&](number::Number *n) {
    if (!n)
      return;
    auto call = n->make_call();
    call.set_value(fval);
    call.perform();
  };

  if (key == "luefter_modus") {
    doSelect(this->luefter_modus_);
  } else if (key == "fan_intensity_display") {
    doNumber(this->fan_intensity_display_);
  } else if (key == "automatik_min_luefterstufe") {
    doNumber(this->automatik_min_luefterstufe_);
  } else if (key == "automatik_max_luefterstufe") {
    doNumber(this->automatik_max_luefterstufe_);
  } else if (key == "auto_co2_threshold") {
    doNumber(this->auto_co2_threshold_);
  } else if (key == "auto_humidity_threshold") {
    doNumber(this->auto_humidity_threshold_);
  } else if (key == "auto_presence_slider") {
    doNumber(this->auto_presence_slider_);
  } else if (key == "vent_timer") {
    doNumber(this->vent_timer_);
  } else if (key == "sync_interval_config") {
    doNumber(this->sync_interval_config_);
  } else {
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
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/html", DASHBOARD_HTML);
  response->addHeader("Cache-Control",
                      "no-store, no-cache, must-revalidate, max-age=0");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "0");
  request->send(response);
}

void WrgDashboard::handle_state_(AsyncWebServerRequest *request) {
  JsonDocument doc;
  auto get_f = [](sensor::Sensor *s) -> float {
    return (s && s->has_state()) ? s->state : (float)NAN;
  };
  auto get_b = [](binary_sensor::BinarySensor *b) -> bool {
    return (b && b->has_state()) ? b->state : false;
  };
  auto get_t = [](text_sensor::TextSensor *t) -> const char * {
    return (t && t->has_state()) ? t->state.c_str() : "";
  };
  auto get_n = [](number::Number *n) -> float {
    return (n && n->has_state()) ? n->state : (float)NAN;
  };
  auto get_s = [](select::Select *s) -> std::string {
    if (s && s->has_state()) return s->current_option();
    return "";
  };

  doc["temperature"] = get_f(this->temperature_);
  doc["pressure"] = get_f(this->pressure_);
  doc["outdoor_humidity"] = get_f(this->outdoor_humidity_);
  doc["room_co2"] =
      get_f(this->effective_co2_ ? this->effective_co2_ : this->scd41_co2_);
  // Temperature: SCD41 → BME680 fallback
  float display_temp = get_f(this->scd41_temperature_);
  if (std::isnan(display_temp)) {
    display_temp = get_f(this->bme680_temperature_);
  }
  doc["room_temperature"] = display_temp;

  // Humidity: SCD41 → BME680 fallback
  float display_hum = get_f(this->scd41_humidity_);
  if (std::isnan(display_hum)) {
    display_hum = get_f(this->bme680_humidity_);
  }
  doc["room_humidity"] = display_hum;

  // Room temp for peer card and chart (same fallback chain)
  float local_room_t = display_temp;
  if (!std::isnan(local_room_t)) {
    doc["room_temp"] = local_room_t;
  }
  doc["temp_zuluft"] = get_f(this->temp_zuluft_);
  doc["temp_abluft"] = get_f(this->temp_abluft_);
  doc["heat_recovery_efficiency"] = get_f(this->heat_recovery_efficiency_);
  doc["fan_rpm"] = get_f(this->fan_rpm_);
  doc["pid_demand"] = (this->ventilation_ctrl_ != nullptr) ? this->ventilation_ctrl_->local_pid_demand : (float)NAN;
  doc["filter_operating_days"] = get_f(this->filter_operating_days_);

  doc["device_id"] = get_t(this->device_id_);
  doc["floor_id"] = get_t(this->floor_id_);
  doc["room_id"] = get_t(this->room_id_);
  doc["phase"] = get_t(this->phase_);
  doc["direction_display"] = get_t(this->direction_display_);

  doc["filter_change_alarm"] = get_b(this->filter_change_alarm_);
  doc["radar_presence"] = get_b(this->radar_presence_);

  doc["room_co2_bewertung"] = get_t(this->scd41_co2_bewertung_);
  doc["luefter_modus"] = get_s(this->luefter_modus_);

  doc["vent_timer"] = get_n(this->vent_timer_);
  doc["sync_interval_config"] = get_n(this->sync_interval_config_);
  doc["fan_intensity_display"] = get_n(this->fan_intensity_display_);
  doc["automatik_min_luefterstufe"] = get_n(this->automatik_min_luefterstufe_);
  doc["automatik_max_luefterstufe"] = get_n(this->automatik_max_luefterstufe_);
  doc["auto_co2_threshold"] = get_n(this->auto_co2_threshold_);
  doc["auto_humidity_threshold"] = get_n(this->auto_humidity_threshold_);
  doc["auto_presence_slider"] = get_n(this->auto_presence_slider_);

  JsonArray peers_array = doc["peers"].to<JsonArray>();

  // Peer devices from VentilationController
  // Thread Safety Note: ESPHome's AsyncWebServer runs in an external FreeRTOS task. 
  // While std::vector concurrent reads/writes are technically not thread-safe if the
  // vector reallocates, our peer list is extremely small (max ~4 items) and early-bound.
  // We accept this minimal risk to avoid mutex locks in the critical ESP-NOW fast-path.
  if (this->ventilation_ctrl_ != nullptr) {
    uint32_t now = millis();
    for (const auto &peer : this->ventilation_ctrl_->peers) {
      if (now - peer.last_seen_ms <
          900000) { // Only show peers seen in the last 15 mins
        JsonObject p = peers_array.add<JsonObject>();
        p["device_id"] = peer.device_id;
        p["mode"] = peer.current_mode;
        p["speed"] = peer.fan_intensity;
        p["phase"] = peer.phase_state;
        if (!std::isnan(peer.t_in))
          p["t_in"] = peer.t_in;
        if (!std::isnan(peer.t_out))
          p["t_out"] = peer.t_out;
        if (!std::isnan(peer.pid_demand))
          p["pid_demand"] = peer.pid_demand;
        if (!std::isnan(peer.fan_rpm))
          p["rpm"] = peer.fan_rpm;
        if (!std::isnan(peer.board_temp))
          p["board_t"] = peer.board_temp;
        if (!std::isnan(peer.room_temp))
          p["room_t"] = peer.room_temp;
      }
    }
  }

  std::string response;
  serializeJson(doc, response);
  request->send(200, "application/json", response.c_str());
}

void WrgDashboard::handle_set_(AsyncWebServerRequest *request) {
  static const std::unordered_set<std::string> ALLOWED_KEYS = {
      "luefter_modus", "fan_intensity_display", "automatik_min_luefterstufe",
      "automatik_max_luefterstufe", "auto_co2_threshold", "auto_humidity_threshold",
      "auto_presence_slider", "vent_timer", "sync_interval_config"
  };

  if (!request->hasParam("id") || !request->hasParam("val")) {
    request->send(400, "text/plain", "Missing parameters");
    return;
  }

  std::string id(request->getParam("id")->value().c_str());
  std::string val(request->getParam("val")->value().c_str());

  if (ALLOWED_KEYS.find(id) == ALLOWED_KEYS.end()) {
    ESP_LOGW(TAG, "Blocked invalid parameter from dashboard: %s", id.c_str());
    request->send(400, "text/plain", "Invalid parameter");
    return;
  }

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
}

} // namespace wrg_dashboard
} // namespace esphome
