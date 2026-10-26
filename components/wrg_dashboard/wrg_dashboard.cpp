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
// Modified:    2026-10-26
// ==========================================================================

#include "wrg_dashboard.h"
#include "esphome/components/ventilation_group/ventilation_group.h"
#include "esphome/components/ventilation_logic/room_fusion.h"
#include "esphome/components/ventilation_logic/ventilation_logic.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif
#ifdef USE_ESP32
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <sys/socket.h>
#include <cerrno>
#endif
#include <ArduinoJson.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace esphome {
namespace wrg_dashboard {

static const char *const TAG = "wrg_dashboard";

/// Header the dashboard sends with every /set. A cross-site page cannot set
/// it without a CORS preflight, which this server never grants — so a
/// forged form or <img> request from another site is rejected.
static const char *const CSRF_HEADER = "X-VentoSync";

/// Current value of a sensor, NaN without one.
static float sensor_value(sensor::Sensor *s) { return (s != nullptr && s->has_state()) ? s->state : NAN; }

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

#ifdef USE_ESP32
/**
 * @brief   Send function of a /stream session (httpd send override): never
 *          blocks the main loop, a full TCP buffer is retried next loop.
 * @return  Bytes sent, HTTPD_SOCK_ERR_TIMEOUT (buffer full) or
 *          HTTPD_SOCK_ERR_FAIL.
 */
static int stream_send(httpd_handle_t /*hd*/, int sockfd, const char *buf, size_t buf_len, int flags) {
  if (buf == nullptr) return HTTPD_SOCK_ERR_INVALID;
  const int ret = send(sockfd, buf, buf_len, flags | MSG_DONTWAIT);
  if (ret < 0) {
    return (errno == EAGAIN || errno == EWOULDBLOCK) ? HTTPD_SOCK_ERR_TIMEOUT : HTTPD_SOCK_ERR_FAIL;
  }
  return ret;
}

/// One SSE event as an HTTP chunk: "<hex len>\r\nevent: …\ndata: …\n\n\r\n".
/// `data` must not contain newlines (serialized JSON never does).
static std::string sse_chunk(const char *event, const std::string &data) {
  const size_t body_len = 7 + std::strlen(event) + 7 + data.size() + 2;  // "event: " … "\ndata: " … "\n\n"
  char head[12];
  std::snprintf(head, sizeof(head), "%X\r\n", static_cast<unsigned>(body_len));
  std::string out;
  out.reserve(std::strlen(head) + body_len + 2);
  out += head;
  out += "event: ";
  out += event;
  out += "\ndata: ";
  out += data;
  out += "\n\n\r\n";
  return out;
}

void StreamSession::destroy(void *ctx) {
  // httpd task: only mark the session dead; loop() deletes it
  static_cast<StreamSession *>(ctx)->fd.store(0);
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
  return request->method() == HTTP_GET &&
         (url == "/" || url == "/ui" || url == "/state" || url == "/meta" || url == "/history"
#ifdef USE_ESP32
          || url == "/stream"
#endif
         );
}

sensor::Sensor *WrgDashboard::find_sensor_(const char *key) const {
  for (const auto &e : this->sensors_) {
    if (std::strcmp(e.key, key) == 0) return e.entity;
  }
  return nullptr;
}

void WrgDashboard::setup() {
#ifdef USE_ESP32
  this->reset_reason_ = reset_reason_label();
#endif
  this->efficiency_sensor_ = this->find_sensor_("heat_recovery_efficiency");
  this->t_out_sensor_ = this->find_sensor_("temp_zuluft");
  this->t_in_sensor_ = this->find_sensor_("temp_abluft");
  this->t_supply_sensor_ = this->find_sensor_("temp_zuluft_wrg");

  // mDNS names: with name_add_mac_suffix every device is "<name>-<MAC[3..5]>",
  // so a peer's host follows from the MAC of its ESP-NOW packets.
  const std::string name = App.get_name().str();
  this->host_ = name + ".local";
  if (App.is_name_add_mac_suffix_enabled() && name.size() > 6) {
    this->host_prefix_ = name.substr(0, name.size() - 6);
  }

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

  if (now - this->last_sample_ms_ >= ventosync::history::SAMPLE_MS) {
    this->last_sample_ms_ = now;
    this->sample_history_(now);
  }

  // Serialize /state only while a client polls or streams; a change from
  // /set is reflected at once (pushed, or re-read by the UI after 500 ms).
  bool streaming = false;
#ifdef USE_ESP32
  streaming = this->stream_count_.load() > 0;
#endif
  const bool polled = streaming || now - this->last_request_ms_.load() < STATE_IDLE_MS;
  const bool due = now - built_ms >= STATE_REBUILD_MS || !todo.empty() || this->client_waiting_.load();
  std::string json;
  const bool built = polled && due;
  if (built) {
    json = this->build_state_json_();
    this->client_waiting_.store(false);
  }
#ifdef USE_ESP32
  if (streaming) {
    const bool push = built && (now - this->last_push_ms_ >= STREAM_PUSH_MS || !todo.empty() ||
                                this->stream_push_now_.exchange(false));
    this->service_streams_(now, push ? &json : nullptr);
  }
#endif
  if (built) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->state_json_ = std::move(json);
    this->state_built_ms_ = now;
  }
}

/**
 * @brief   Adds one sample of every history channel (runs in loop()).
 */
void WrgDashboard::sample_history_(uint32_t now) {
  namespace h = ventosync::history;
  h::Values v;
  v.fill(NAN);
  v[h::CO2] = this->room_co2_();
  v[h::ROOM_T] = this->room_temperature_();
  v[h::ROOM_RH] = this->room_humidity_();
  if (const auto *c = this->ventilation_ctrl_) {
    v[h::LEVEL] = c->state_machine.current_mode == MODE_OFF ? 0.0f : static_cast<float>(c->current_fan_intensity);
    v[h::DEMAND] = c->local_pid_demand;  // NaN outside Smart-Automatik / without own sensor
  }
  v[h::EFFICIENCY] = sensor_value(this->efficiency_sensor_);
  v[h::T_OUT] = sensor_value(this->t_out_sensor_);
  v[h::T_IN] = sensor_value(this->t_in_sensor_);
  v[h::T_SUPPLY] = sensor_value(this->t_supply_sensor_);
  std::lock_guard<std::mutex> lock(this->mutex_);
  this->history_.add(v, now);
}

#ifdef USE_ESP32
/**
 * @brief   Adopts new /stream sessions, removes closed ones and sends
 *          `state_json` (if given) as an SSE "state" event to every client.
 * @details A client still busy with the previous event skips this one — the
 *          next event carries the complete state anyway.
 */
void WrgDashboard::service_streams_(uint32_t now, const std::string *state_json) {
  {
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->streams_.insert(this->streams_.end(), this->pending_streams_.begin(), this->pending_streams_.end());
    this->pending_streams_.clear();
  }
  std::shared_ptr<const std::string> frame;
  if (state_json != nullptr) {
    frame = std::make_shared<const std::string>(sse_chunk("state", *state_json));
    this->last_push_ms_ = now;
  }
  for (size_t i = 0; i < this->streams_.size();) {
    StreamSession *s = this->streams_[i];
    if (s->fd.load() == 0) {  // socket closed: destroy() ran, httpd holds no reference any more
      delete s;  // NOLINT(cppcoreguidelines-owning-memory)
      this->streams_[i] = this->streams_.back();
      this->streams_.pop_back();
      this->stream_count_--;
      continue;
    }
    if (!s->closing) {
      if (frame && !s->frame) {
        s->frame = frame;
        s->sent = 0;
      }
      flush_stream_(s, now);
    }
    i++;
  }
}

/**
 * @brief   Sends the rest of a session's pending event (non-blocking).
 * @details A dead or stuck client (no progress for STREAM_STALL_MS) is
 *          closed via httpd; the session is deleted once destroy() ran.
 */
void WrgDashboard::flush_stream_(StreamSession *s, uint32_t now) {
  if (!s->frame) return;
  const int fd = s->fd.load();
  if (fd == 0) return;
  const std::string &f = *s->frame;
  const int n = httpd_socket_send(s->hd, fd, f.data() + s->sent, f.size() - s->sent, 0);
  if (n > 0) {
    s->stalled = false;
    s->sent += static_cast<size_t>(n);
    if (s->sent >= f.size()) {
      s->frame.reset();
      s->sent = 0;
    }
    return;
  }
  if (n == HTTPD_SOCK_ERR_TIMEOUT || n == 0) {  // TCP send buffer full
    if (!s->stalled) {
      s->stalled = true;
      s->stalled_since_ms = now;
      return;
    }
    if (now - s->stalled_since_ms < STREAM_STALL_MS) return;
    ESP_LOGW(TAG, "Closing stuck /stream client (fd %d)", fd);
  } else {
    ESP_LOGD(TAG, "/stream send failed (fd %d), closing", fd);
  }
  s->closing = true;
  s->frame.reset();
  httpd_sess_trigger_close(s->hd, fd);
}
#endif

bool WrgDashboard::is_writable_key_(const std::string &key) const {
  for (const auto &e : this->numbers_) if (key == e.key) return true;
  for (const auto &e : this->selects_) if (key == e.key) return true;
#ifdef USE_SWITCH
  for (const auto &e : this->switches_) if (key == e.key) return true;
#endif
#ifdef USE_BUTTON
  for (const auto &e : this->buttons_) if (key == e.key) return true;
#endif
  return this->is_text_key_(key);
}

bool WrgDashboard::is_text_key_(const std::string &key) const {
#ifdef USE_TEXT
  for (const auto &e : this->texts_) if (key == e.key) return true;
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
#ifdef USE_TEXT
  for (const auto &e : this->texts_) {
    if (action.key == e.key) {
      auto call = e.entity->make_call();
      call.set_value(action.s_value);  // TextCall rejects values outside min/max length
      call.perform();
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
  if (url == "/" || url == "/ui") {  // "/ui": old bookmarks and peer links of older firmware
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
  } else if (url == "/history") {
    this->handle_history_(request);
#ifdef USE_ESP32
  } else if (url == "/stream") {
    this->handle_stream_(request);
#endif
  } else if (url == "/set") {
    this->handle_set_(request);
  }
}

/**
 * @brief   Serves the 24 h history (GET /history, see history.h).
 * @details The ring buffer is only written by loop() under the mutex, so it
 *          is serialized (~13 KB) right here.
 */
void WrgDashboard::handle_history_(AsyncWebServerRequest *request) {
  std::string body;
  {
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->history_.append_json(body, millis());
  }
  request->send(200, "application/json", body.c_str());
}

#ifdef USE_ESP32
/**
 * @brief   Opens a Server-Sent Events stream (GET /stream, httpd task).
 * @details Same mechanism as ESPHome's /events: the response headers and a
 *          first chunk go out here, the socket stays open and loop() sends
 *          one "state" event (the /state JSON) every STREAM_PUSH_MS. The
 *          session is tied to the socket (sess_ctx / free_ctx), so httpd
 *          reports its closing via StreamSession::destroy().
 */
void WrgDashboard::handle_stream_(AsyncWebServerRequest *request) {
  if (this->stream_count_.load() >= MAX_STREAMS) {
    request->send(503, "text/plain", "Too many streams");  // the UI polls /state instead
    return;
  }
  httpd_req_t *req = *request;
  httpd_resp_set_status(req, HTTPD_200);
  httpd_resp_set_type(req, "text/event-stream");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
  httpd_resp_set_hdr(req, "Connection", "keep-alive");
  // "retry": reconnect delay of the browser's EventSource after a drop
  static const char HELLO[] = "retry: 3000\n\n";
  if (httpd_resp_send_chunk(req, HELLO, sizeof(HELLO) - 1) != ESP_OK) {
    return;
  }
  auto *s = new StreamSession();  // NOLINT(cppcoreguidelines-owning-memory) owned by loop()
  req->sess_ctx = s;
  req->free_ctx = StreamSession::destroy;
  s->hd = req->handle;
  s->fd.store(httpd_req_to_sockfd(req));
  httpd_sess_set_send_override(s->hd, s->fd.load(), stream_send);
  this->stream_count_++;
  {
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->pending_streams_.push_back(s);
  }
  this->client_waiting_.store(true);   // build /state now …
  this->stream_push_now_.store(true);  // … and push it without waiting for the interval
}
#endif

/**
 * @brief   Serves the dashboard page (dashboard.html, gzipped at build time).
 * @param[in] request  The incoming AsyncWebServerRequest.
 */
void WrgDashboard::handle_root_(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response =
      request->beginResponse(200, "text/html", this->html_gz_, this->html_gz_size_);
  response->addHeader("Content-Encoding", "gzip");
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
  const uint32_t now = millis();
  auto *v = this->ventilation_ctrl_;

  doc["room_co2"] = this->room_co2_();
  doc["room_temperature"] = this->room_temperature_();
  doc["room_humidity"] = this->room_humidity_();

  for (const auto &e : this->sensors_) doc[e.key] = sensor_value(e.entity);
  for (const auto &e : this->binary_sensors_) doc[e.key] = e.entity->has_state() && e.entity->state;
  for (const auto &e : this->text_sensors_) doc[e.key] = e.entity->has_state() ? e.entity->state.c_str() : "";
  for (const auto &e : this->numbers_) doc[e.key] = e.entity->has_state() ? e.entity->state : NAN;
  for (const auto &e : this->selects_) doc[e.key] = e.entity->current_option().c_str();
#ifdef USE_SWITCH
  for (const auto &e : this->switches_) doc[e.key] = e.entity->state;
#endif
#ifdef USE_TEXT
  for (const auto &e : this->texts_) doc[e.key] = e.entity->has_state() ? e.entity->state.c_str() : "";
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
      // Current direction from our room-synced cycle and the peer's configured
      // phase (absent while the fan stands or the direction is unknown)
      bool dir_in = false;
      if (v->state_machine.peer_direction_in(peer.current_mode, !peer.phase_b, now, dir_in)) {
        p["dir"] = dir_in ? "in" : "out";
      }
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
      p["phase_cfg"] = peer.phase_b ? "B" : "A";  // configured phase (firmware < 0.10.56: always "A")
      if (peer.name[0] != '\0') p["name"] = peer.name.data();  // its "Gerätename" (v16)
      // Link to the peer's dashboard (mDNS, see setup())
      if (!this->host_prefix_.empty()) {
        char host[16];
        std::snprintf(host, sizeof(host), "%02x%02x%02x.local", peer.mac[3], peer.mac[4], peer.mac[5]);
        p["host"] = this->host_prefix_ + host;
      }
    }
  }

  JsonObject sys = doc["sys"].to<JsonObject>();
  sys["version"] = this->project_version_;
  sys["variant"] = this->firmware_variant_;
  sys["uptime_s"] = static_cast<uint32_t>(millis_64() / 1000ULL);
  sys["reset_reason"] = this->reset_reason_;
  sys["host"] = this->host_;
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

// Room climate: effective CO2 (may be a VOC/eCO2 estimate), SCD41 → BME680 fallback
float WrgDashboard::room_co2_() const {
  return sensor_value(this->effective_co2_ != nullptr ? this->effective_co2_ : this->scd41_co2_);
}
float WrgDashboard::room_temperature_() const {
  const float t = sensor_value(this->scd41_temperature_);
  return std::isnan(t) ? sensor_value(this->bme680_temperature_) : t;
}
float WrgDashboard::room_humidity_() const {
  const float rh = sensor_value(this->scd41_humidity_);
  return std::isnan(rh) ? sensor_value(this->bme680_humidity_) : rh;
}

/**
 * @brief   Serializes the static limits of the writable settings (runs once).
 * @details numbers: [min, max, step] from the entity traits, selects: their
 *          options, texts: [min, max] length in bytes, buttons: their keys,
 *          defaults: the factory values of the `defaults:` option (setup
 *          wizard; a select's default is its option index) — so the UI never
 *          duplicates the YAML ranges and defaults.
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
  JsonObject texts = doc["texts"].to<JsonObject>();
#ifdef USE_TEXT
  for (const auto &e : this->texts_) {
    JsonArray r = texts[e.key].to<JsonArray>();
    r.add(e.entity->traits.get_min_length());
    r.add(e.entity->traits.get_max_length());
  }
#endif
  JsonArray buttons = doc["buttons"].to<JsonArray>();
#ifdef USE_BUTTON
  for (const auto &e : this->buttons_) buttons.add(e.key);
#endif
  JsonObject defaults = doc["defaults"].to<JsonObject>();
  for (const auto &d : this->defaults_) defaults[d.first] = d.second;
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
    a.f_value = NAN;  // only selects and texts take non-numeric values
    bool takes_text = this->is_text_key_(a.key);
    for (const auto &e : this->selects_) takes_text = takes_text || a.key == e.key;
    if (!takes_text) {
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
