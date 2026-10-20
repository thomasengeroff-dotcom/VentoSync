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
// Modified:    2026-10-20
// ==========================================================================

#include "wrg_dashboard.h"
#include "dashboard_html.h"
#include "esphome/components/ventilation_group/ventilation_group.h"
#include "esphome/components/ventilation_logic/room_fusion.h"
#include "esphome/components/ventilation_logic/ventilation_logic.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif
#ifdef USE_ESP32
#include <esp_heap_caps.h>
#include <esp_system.h>
#endif
#include <ArduinoJson.h>
#include <cmath>
#include <cstdlib>

namespace esphome {
namespace wrg_dashboard {

static const char *const TAG = "wrg_dashboard";

/// Header the dashboard sends with every /set. A cross-site page cannot set
/// it without a CORS preflight, which this server never grants — so a
/// forged form or <img> request from another site is rejected.
static const char *const CSRF_HEADER = "X-VentoSync";

#ifdef USE_ESP32
/// Short German label of the last reset cause (Systeminfo row).
static const char *reset_reason_label() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "Einschalten";
    case ESP_RST_EXT:       return "Reset-Pin";
    case ESP_RST_SW:        return "Software-Neustart";
    case ESP_RST_PANIC:     return "Absturz";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:       return "Watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep Sleep";
    case ESP_RST_BROWNOUT:  return "Unterspannung";
    case ESP_RST_USB:       return "USB";
    default:                return "Unbekannt";
  }
}
#endif

/**
 * @brief   Same-origin check, like ESPHome's web_server: a browser request
 *          with an Origin header must come from the page served by this host.
 *          Requests without Origin (curl, scripts) are allowed.
 */
static bool origin_allowed(AsyncWebServerRequest *request) {
  const auto origin = request->get_header("Origin");
  if (!origin.has_value() || origin->empty()) {
    return true;
  }
  const size_t scheme_sep = origin->find("://");
  const auto host = request->get_header("Host");
  return scheme_sep != std::string::npos && host.has_value() && !host->empty() &&
         origin->compare(scheme_sep + 3, std::string::npos, *host) == 0;
}

bool WrgDashboard::canHandle(AsyncWebServerRequest *request) const {
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  auto url = request->url_to(url_buf);
  if (url == "/set") {
    return true;  // any method: handle_set_() answers 405 for non-POST
  }
  return request->method() == HTTP_GET && (url == "/ui" || url == "/state" || url == "/meta");
}

void WrgDashboard::setup() {
#ifdef USE_ESP32
  this->reset_reason_ = reset_reason_label();
#endif
  if (this->base_ != nullptr) {
    this->base_->add_handler(this);
    ESP_LOGI(TAG, "Webserver Dashboard Handler registered");
  } else {
    ESP_LOGE(TAG, "WebServerBase not provided!");
  }
}

void WrgDashboard::loop() {
  const uint32_t now = millis();
  if (now - this->last_queue_check_ms_ < 100) {
    return;
  }
  this->last_queue_check_ms_ = now;

  std::vector<DashboardAction> todo;
  uint32_t dropped = 0;
  bool need_meta = false;
  uint32_t built_ms = 0;
  {
    std::lock_guard<std::mutex> lock(this->mutex_);
    dropped = this->dropped_actions_;
    this->dropped_actions_ = 0;
    todo = std::move(this->action_queue_);
    this->action_queue_.clear(); // moved-from: valid but unspecified
    need_meta = this->meta_json_.empty();
    built_ms = this->state_built_ms_;
  }

  // One summary line per drain instead of one log line per rejected request
  if (dropped > 0) {
    ESP_LOGW(TAG, "Dashboard /set flood: %u request(s) rejected (queue full, max %u)",
             static_cast<unsigned>(dropped), static_cast<unsigned>(MAX_PENDING_ACTIONS));
  }

  for (const auto &action : todo) {
    this->dispatch_set_(action);
  }

  if (need_meta) {
    std::string meta = this->build_meta_json_();
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->meta_json_ = std::move(meta);
  }

  // Serialize /state only while a client polls; a change from /set is
  // reflected at once (the UI re-reads 500 ms after a change).
  const bool polled = now - this->last_request_ms_.load() < STATE_IDLE_MS;
  const bool due = now - built_ms >= STATE_REBUILD_MS || !todo.empty() || this->client_waiting_.load();
  if (polled && due) {
    std::string json = this->build_state_json_();
    this->client_waiting_.store(false);
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->state_json_ = std::move(json);
    this->state_built_ms_ = now;
  }
}

bool WrgDashboard::is_writable_key_(const std::string &key) const {
  for (const auto &e : this->numbers_) if (key == e.key) return true;
  for (const auto &e : this->selects_) if (key == e.key) return true;
#ifdef USE_SWITCH
  for (const auto &e : this->switches_) if (key == e.key) return true;
#endif
#ifdef USE_BUTTON
  for (const auto &e : this->buttons_) if (key == e.key) return true;
#endif
  return false;
}

void WrgDashboard::dispatch_set_(const DashboardAction &action) {
  for (const auto &e : this->numbers_) {
    if (action.key == e.key) {
      auto call = e.entity->make_call();
      call.set_value(action.f_value);  // NumberCall rejects values outside min/max
      call.perform();
      return;
    }
  }
  for (const auto &e : this->selects_) {
    if (action.key == e.key) {
      auto call = e.entity->make_call();
      call.set_option(action.s_value);
      call.perform();
      return;
    }
  }
#ifdef USE_SWITCH
  for (const auto &e : this->switches_) {
    if (action.key == e.key) {
      if (action.f_value > 0.5f) {
        e.entity->turn_on();
      } else {
        e.entity->turn_off();
      }
      return;
    }
  }
#endif
#ifdef USE_BUTTON
  for (const auto &e : this->buttons_) {
    if (action.key == e.key) {
      ESP_LOGI(TAG, "Dashboard pressed button: %s", e.key);
      e.entity->press();
      return;
    }
  }
#endif
  ESP_LOGW(TAG, "Unknown action trigger received: %s", action.key.c_str());
}

/**
 * @brief   Routes the request to the appropriate handler based on the URL.
 * @param[in] request  The incoming AsyncWebServerRequest.
 */
void WrgDashboard::handleRequest(AsyncWebServerRequest *request) {
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  auto url = request->url_to(url_buf);
  if (url == "/ui") {
    this->handle_root_(request);
    return;
  }
  if (!origin_allowed(request)) {
    request->send(403, "text/plain", "Cross-origin request rejected");
    return;
  }
  if (url == "/state") {
    this->handle_cached_json_(request, false);
  } else if (url == "/meta") {
    this->handle_cached_json_(request, true);
  } else if (url == "/set") {
    this->handle_set_(request);
  }
}

/**
 * @brief   Serves the main dashboard HTML page.
 * @param[in] request  The incoming AsyncWebServerRequest.
 */
void WrgDashboard::handle_root_(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/html", DASHBOARD_HTML);
  response->addHeader("Cache-Control",
                      "no-store, no-cache, must-revalidate, max-age=0");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "0");
  request->send(response);
}

/**
 * @brief   Serves the /state or /meta JSON built by loop().
 * @details A missing or outdated /state (no client polled for a while)
 *          answers 503; loop() rebuilds it within ~100 ms and the UI retries.
 */
void WrgDashboard::handle_cached_json_(AsyncWebServerRequest *request, bool meta) {
  const uint32_t now = millis();
  std::string body;
  {
    std::lock_guard<std::mutex> lock(this->mutex_);
    if (meta) {
      body = this->meta_json_;
    } else {
      this->last_request_ms_.store(now);
      if (!this->state_json_.empty() && now - this->state_built_ms_ < STATE_MAX_AGE_MS) {
        body = this->state_json_;
      }
    }
  }
  if (body.empty()) {
    if (!meta) {
      this->client_waiting_.store(true);
    }
    request->send(503, "text/plain", "Not ready");
    return;
  }
  request->send(200, "application/json", body.c_str());
}

/**
 * @brief   Serializes the current system state (runs in loop()).
 * @details Sensor values, room status, system info, writable settings and
 *          the ESP-NOW peer list. NaN values serialize as null.
 */
std::string WrgDashboard::build_state_json_() {
  JsonDocument doc;
  auto get_f = [](sensor::Sensor *s) -> float {
    return (s && s->has_state()) ? s->state : NAN;
  };
  const uint32_t now = millis();
  auto *v = this->ventilation_ctrl_;

  // Room climate: effective CO2 (may be a VOC/eCO2 estimate) and SCD41 → BME680 fallback
  doc["room_co2"] = get_f(this->effective_co2_ ? this->effective_co2_ : this->scd41_co2_);
  float room_t = get_f(this->scd41_temperature_);
  if (std::isnan(room_t)) room_t = get_f(this->bme680_temperature_);
  doc["room_temperature"] = room_t;
  float room_rh = get_f(this->scd41_humidity_);
  if (std::isnan(room_rh)) room_rh = get_f(this->bme680_humidity_);
  doc["room_humidity"] = room_rh;

  for (const auto &e : this->sensors_) doc[e.key] = get_f(e.entity);
  for (const auto &e : this->binary_sensors_) doc[e.key] = e.entity->has_state() && e.entity->state;
  for (const auto &e : this->text_sensors_) doc[e.key] = e.entity->has_state() ? e.entity->state.c_str() : "";
  for (const auto &e : this->numbers_) doc[e.key] = e.entity->has_state() ? e.entity->state : NAN;
  for (const auto &e : this->selects_) doc[e.key] = e.entity->current_option().c_str();
#ifdef USE_SWITCH
  for (const auto &e : this->switches_) doc[e.key] = e.entity->state;
#endif

  // Room status (same sources as the automatic itself)
  if (v != nullptr) {
    const uint32_t fusion_age = ventosync::room::fusion_max_age_ms(v->sync_interval_ms, PEER_TIMEOUT_MS);
    doc["pid_demand"] = v->local_pid_demand;
    doc["is_master"] = v->device_id == MASTER_DEVICE_ID;
    doc["room_presence"] = v->presence_local || ventosync::room::any_fresh_peer_presence(v->peers, now, fusion_age);
    doc["window_guard"] = v->is_window_guard_active();
    doc["window_paused"] = v->is_window_guard_pausing();
    doc["vacation_active"] = v->is_vacation_active();

    JsonArray peers_array = doc["peers"].to<JsonArray>();
    for (const auto &peer : v->peers) {
      const uint32_t age = now - peer.last_seen_ms;
      if (static_cast<int32_t>(age) > static_cast<int32_t>(PEER_TIMEOUT_MS)) {
        continue;  // dropped by the controller on its next cleanup
      }
      JsonObject p = peers_array.add<JsonObject>();
      p["device_id"] = peer.device_id;
      p["master"] = peer.device_id == MASTER_DEVICE_ID;
      p["mode_index"] = peer.current_mode_index;
      p["speed"] = peer.fan_intensity;
      p["phase"] = peer.phase_state;
      // last_seen_ms can be a few ms ahead of now (packet stamped after the loop's timestamp)
      p["age_s"] = static_cast<int32_t>(age) < 0 ? 0u : age / 1000u;
      p["fresh"] = static_cast<int32_t>(age) < static_cast<int32_t>(fusion_age);
      if (!std::isnan(peer.pid_demand)) p["pid_demand"] = peer.pid_demand;
      if (!std::isnan(peer.fan_rpm)) p["rpm"] = peer.fan_rpm;
      if (!std::isnan(peer.board_temp)) p["board_t"] = peer.board_temp;
      if (!std::isnan(peer.room_temp)) p["room_t"] = peer.room_temp;
      // Air quality shared over ESP-NOW (SCD43 or BME680, whichever the peer has).
      // The rating uses the same classification as the local card so both agree.
      if (!std::isnan(peer.room_co2) && peer.room_co2 > 0.0f) {
        p["co2"] = peer.room_co2;
        p["co2_rating"] = VentilationLogic::get_co2_classification(peer.room_co2);
      }
      if (!std::isnan(peer.room_humidity)) p["humidity"] = peer.room_humidity;
      // The peer's own inputs (room flags) — shown as icons on its card
      p["presence"] = peer.presence;
      p["window"] = peer.window_open;
      p["ac"] = peer.hvac_ac_active;
      p["nox"] = peer.nox_active;
    }
  }

  JsonObject sys = doc["sys"].to<JsonObject>();
  sys["version"] = this->project_version_;
  sys["variant"] = this->firmware_variant_;
  sys["uptime_s"] = static_cast<uint32_t>(millis_64() / 1000ULL);
  sys["reset_reason"] = this->reset_reason_;
#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr && wifi::global_wifi_component->is_connected()) {
    sys["rssi"] = wifi::global_wifi_component->wifi_rssi();
  }
#endif
#ifdef USE_ESP32
  sys["heap_free"] = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  sys["heap_min"] = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
#endif

  std::string out;
  serializeJson(doc, out);
  return out;
}

/**
 * @brief   Serializes the static limits of the writable settings (runs once).
 * @details numbers: [min, max, step] from the entity traits, selects: their
 *          options, buttons: their keys — so the UI never duplicates the
 *          YAML ranges.
 */
std::string WrgDashboard::build_meta_json_() {
  JsonDocument doc;
  JsonObject numbers = doc["numbers"].to<JsonObject>();
  for (const auto &e : this->numbers_) {
    JsonArray r = numbers[e.key].to<JsonArray>();
    r.add(e.entity->traits.get_min_value());
    r.add(e.entity->traits.get_max_value());
    r.add(e.entity->traits.get_step());
  }
  JsonObject selects = doc["selects"].to<JsonObject>();
  for (const auto &e : this->selects_) {
    JsonArray opts = selects[e.key].to<JsonArray>();
    for (const char *opt : e.entity->traits.get_options()) opts.add(opt);
  }
  JsonArray buttons = doc["buttons"].to<JsonArray>();
#ifdef USE_BUTTON
  for (const auto &e : this->buttons_) buttons.add(e.key);
#endif
  std::string out;
  serializeJson(doc, out);
  return out;
}

/**
 * @brief   Handles POST /set (form fields id, val) to change a setting.
 * @details CSRF protection: POST only, X-VentoSync header required, same
 *          origin. Only keys of configured writable entities are accepted;
 *          the change is queued for loop().
 * @param[in] request  The incoming AsyncWebServerRequest.
 */
void WrgDashboard::handle_set_(AsyncWebServerRequest *request) {
  if (request->method() != HTTP_POST) {
    request->send(405, "text/plain", "POST required");
    return;
  }
  if (!request->hasHeader(CSRF_HEADER)) {
    request->send(403, "text/plain", "Missing X-VentoSync header");
    return;
  }
  if (!request->hasParam("id") || !request->hasParam("val")) {
    request->send(400, "text/plain", "Missing parameters");
    return;
  }

  std::string id(request->getParam("id")->value().c_str());
  std::string val(request->getParam("val")->value().c_str());

  if (val.size() > MAX_VALUE_LEN) {
    request->send(400, "text/plain", "Value too long");
    return;
  }

  if (!this->is_writable_key_(id)) {
    ESP_LOGW(TAG, "Blocked invalid parameter from dashboard: %s", id.c_str());
    request->send(400, "text/plain", "Invalid parameter");
    return;
  }

  DashboardAction a;
  a.key = std::move(id);
  a.s_value = val;
  char *end = nullptr;
  a.f_value = std::strtof(val.c_str(), &end);
  if (end == val.c_str() || !std::isfinite(a.f_value)) {
    a.f_value = NAN;  // only selects take non-numeric values
    bool is_select = false;
    for (const auto &e : this->selects_) is_select = is_select || a.key == e.key;
    if (!is_select) {
      request->send(400, "text/plain", "Invalid value");
      return;
    }
  }

  {
    std::lock_guard<std::mutex> lock(this->mutex_);
    if (this->action_queue_.size() >= MAX_PENDING_ACTIONS) {
      this->dropped_actions_++;
      request->send(503, "text/plain", "Busy");
      return;
    }
    this->action_queue_.push_back(std::move(a));
  }
  request->send(200, "text/plain", "OK");
}

} // namespace wrg_dashboard
} // namespace esphome
