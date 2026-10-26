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
// File:        wrg_dashboard.h
// Description: Web server component definitions for the dashboard.
// Author:      Thomas Engeroff
// Created:     2026-03-09
// Modified:    2026-10-26
// ==========================================================================

#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_TEXT
#include "esphome/components/text/text.h"
#endif
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/components/ventilation_group/ventilation_group.h"
#include "esphome/components/ventilation_logic/history.h"
#include "esphome/core/component.h"
#ifdef USE_ESP32
#include <esp_http_server.h>
#endif
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace esphome {
namespace wrg_dashboard {

struct DashboardAction {
  std::string key;
  std::string s_value;
  float f_value;
};

/// An entity exposed under a fixed JSON key in /state (and /set if writable).
/// The key is a string literal from the code generator (__init__.py).
template<typename T> struct KeyedEntity {
  const char *key;
  T *entity;
};

#ifdef USE_ESP32
/// One open GET /stream connection (Server-Sent Events). Created in the httpd
/// task, owned and deleted by loop(). httpd only marks it dead (fd = 0) via
/// destroy() when the socket closes, so the object outlives every httpd
/// reference to it.
struct StreamSession {
  httpd_handle_t hd{nullptr};
  std::atomic<int> fd{0};
  std::shared_ptr<const std::string> frame;  ///< Chunk being sent (main loop only).
  size_t sent{0};                            ///< Bytes of `frame` already sent.
  bool stalled{false};                       ///< Send buffer full since stalled_since_ms.
  uint32_t stalled_since_ms{0};
  bool closing{false};                       ///< Close requested, waiting for destroy().
  /// httpd free_ctx callback (httpd task): the socket is closing.
  static void destroy(void *ctx);
};
#endif

class WrgDashboard : public Component, public AsyncWebHandler {
public:
  /// Maximum number of /set actions waiting for loop(). The queue is drained
  /// every 100 ms, so a human using the UI never gets near it; a client
  /// flooding /set gets HTTP 503 instead of exhausting the heap.
  static constexpr size_t MAX_PENDING_ACTIONS = 16;
  /// Maximum length of a /set value in bytes (longest legit values: a mode
  /// name, the device name with max. 31 bytes).
  static constexpr size_t MAX_VALUE_LEN = 32;
  /// /state is rebuilt this often while a client polls.
  static constexpr uint32_t STATE_REBUILD_MS = 1000;
  /// Stop rebuilding /state this long after the last request (no client).
  static constexpr uint32_t STATE_IDLE_MS = 15000;
  /// A cached /state older than this is not served (503, the UI retries).
  static constexpr uint32_t STATE_MAX_AGE_MS = 5000;
  /// /stream pushes /state this often (and at once after a /set change).
  static constexpr uint32_t STREAM_PUSH_MS = 2000;
  /// Open /stream connections; more get HTTP 503 and the UI polls instead.
  static constexpr uint8_t MAX_STREAMS = 3;
  /// A /stream client that accepts no data for this long is disconnected.
  static constexpr uint32_t STREAM_STALL_MS = 20000;

  /**
   * @brief   Initializes the web server handler.
   */
  void setup() override;
  /**
   * @brief   Processes the action queue and rebuilds the cached JSON documents.
   */
  void loop() override;
  /**
   * @brief   Returns the setup priority.
   * @return  WIFI priority minus 0.5 — above web_server (WIFI - 1.0), so this
   *          handler is registered first and answers "/" instead of the
   *          native ESPHome page.
   */
  float get_setup_priority() const override {
    return setup_priority::WIFI - 0.5f;
  }

  // Web server
  void set_web_server_base(web_server_base::WebServerBase *b) { base_ = b; }

  // Sensors with a fallback chain (room climate: SCD41 → BME680)
  void set_scd41_co2(sensor::Sensor *s) { scd41_co2_ = s; }
  void set_effective_co2(sensor::Sensor *s) { effective_co2_ = s; }
  void set_scd41_temperature(sensor::Sensor *s) { scd41_temperature_ = s; }
  void set_scd41_humidity(sensor::Sensor *s) { scd41_humidity_ = s; }
  void set_bme680_temperature(sensor::Sensor *s) { bme680_temperature_ = s; }
  void set_bme680_humidity(sensor::Sensor *s) { bme680_humidity_ = s; }

  void set_ventilation_ctrl(esphome::VentilationController *c) { ventilation_ctrl_ = c; }
  void set_firmware_info(const char *version, const char *variant) {
    project_version_ = version;
    firmware_variant_ = variant;
  }
  /// The gzipped dashboard.html (generated by __init__.py).
  void set_html(const uint8_t *data, size_t size) {
    html_gz_ = data;
    html_gz_size_ = size;
  }

  // Pass-through entities: published in /state under `key`. A key is only
  // present when its entity is configured (e.g. voc_index on the sgp41
  // variants, bt_proxy in the Bluetooth proxy variant); the UI hides the
  // matching rows otherwise.
  void add_sensor(const char *key, sensor::Sensor *s) { sensors_.push_back({key, s}); }
  void add_binary_sensor(const char *key, binary_sensor::BinarySensor *b) { binary_sensors_.push_back({key, b}); }
  void add_text_sensor(const char *key, text_sensor::TextSensor *t) { text_sensors_.push_back({key, t}); }
  // Writable entities: also accepted by /set under `key` (the whitelist).
  void add_number(const char *key, number::Number *n) { numbers_.push_back({key, n}); }
  void add_select(const char *key, select::Select *s) { selects_.push_back({key, s}); }
#ifdef USE_SWITCH
  void add_switch(const char *key, switch_::Switch *s) { switches_.push_back({key, s}); }
#endif
#ifdef USE_BUTTON
  void add_button(const char *key, button::Button *b) { buttons_.push_back({key, b}); }
#endif
#ifdef USE_TEXT
  void add_text(const char *key, text::Text *t) { texts_.push_back({key, t}); }
#endif
  /// Factory default of a number key or a select's option index (setup wizard, listed in /meta).
  void add_default(const char *key, float value) { defaults_.push_back({key, value}); }

  /**
   * @brief   Checks if the request can be handled by this component.
   * @param[in] request  The incoming AsyncWebServerRequest.
   * @return  true for GET /, /ui, /state, /meta, /history, /stream and POST /set.
   */
  bool canHandle(AsyncWebServerRequest *request) const override;
  /**
   * @brief   Handles the incoming web request.
   * @param[in] request  The incoming AsyncWebServerRequest.
   */
  void handleRequest(AsyncWebServerRequest *request) override;
  /**
   * @brief   Indicates if the request handler is trivial.
   * @return  false.
   */
  bool isRequestHandlerTrivial() const override { return false; }

protected:
  void handle_root_(AsyncWebServerRequest *request);
  void handle_cached_json_(AsyncWebServerRequest *request, bool meta);
  void handle_set_(AsyncWebServerRequest *request);
  void handle_history_(AsyncWebServerRequest *request);
  bool is_writable_key_(const std::string &key) const;
  bool is_text_key_(const std::string &key) const;
  void dispatch_set_(const DashboardAction &action);
  std::string build_state_json_();
  std::string build_meta_json_();
  float room_co2_() const;
  float room_temperature_() const;
  float room_humidity_() const;
  sensor::Sensor *find_sensor_(const char *key) const;
  void sample_history_(uint32_t now);
#ifdef USE_ESP32
  void handle_stream_(AsyncWebServerRequest *request);
  void service_streams_(uint32_t now, const std::string *state_json);
  static void flush_stream_(StreamSession *s, uint32_t now);
#endif

  // The web server runs in its own task. Entities and the peer list are only
  // read in loop() (main task), which serializes /state and /meta into these
  // strings; the request handler just copies them under the mutex.
  std::mutex mutex_;
  std::vector<DashboardAction> action_queue_;  ///< Guarded by mutex_, max MAX_PENDING_ACTIONS.
  uint32_t dropped_actions_{0};                ///< /set requests rejected (queue full), guarded by mutex_.
  std::string state_json_;                     ///< Guarded by mutex_.
  uint32_t state_built_ms_{0};                 ///< Guarded by mutex_.
  std::string meta_json_;                      ///< Guarded by mutex_, built once.
  std::atomic<uint32_t> last_request_ms_{0};
  std::atomic<bool> client_waiting_{false};    ///< A request found no fresh /state.
  uint32_t last_queue_check_ms_{0};

  // 24 h history (GET /history): fed by loop() every SAMPLE_MS, read by the
  // request handler — both under mutex_.
  ventosync::history::Recorder history_;       ///< Guarded by mutex_.
  uint32_t last_sample_ms_{0};
  sensor::Sensor *efficiency_sensor_{nullptr};  ///< Resolved from sensors_ in setup().
  sensor::Sensor *t_out_sensor_{nullptr};
  sensor::Sensor *t_in_sensor_{nullptr};
  sensor::Sensor *t_supply_sensor_{nullptr};

#ifdef USE_ESP32
  // Server-Sent Events (GET /stream)
  std::vector<StreamSession *> pending_streams_;  ///< New sessions from the httpd task, guarded by mutex_.
  std::vector<StreamSession *> streams_;          ///< Main loop only.
  std::atomic<uint8_t> stream_count_{0};          ///< pending + active sessions.
  std::atomic<bool> stream_push_now_{false};      ///< A client connected: push without waiting.
  uint32_t last_push_ms_{0};
#endif

  web_server_base::WebServerBase *base_{nullptr};

  sensor::Sensor *scd41_co2_{nullptr};
  sensor::Sensor *effective_co2_{nullptr};
  sensor::Sensor *scd41_temperature_{nullptr};
  sensor::Sensor *scd41_humidity_{nullptr};
  sensor::Sensor *bme680_temperature_{nullptr};
  sensor::Sensor *bme680_humidity_{nullptr};
  esphome::VentilationController *ventilation_ctrl_{nullptr};
  const uint8_t *html_gz_{nullptr};
  size_t html_gz_size_{0};
  const char *project_version_{""};
  const char *firmware_variant_{""};
  const char *reset_reason_{""};
  std::string host_prefix_;  ///< "<name>-" when the MAC suffix is on: peer host = prefix + MAC[3..5].
  std::string host_;         ///< Own mDNS host name ("<name>.local").

  // Filled by the code generator before setup(); read-only afterwards.
  std::vector<KeyedEntity<sensor::Sensor>> sensors_;
  std::vector<KeyedEntity<binary_sensor::BinarySensor>> binary_sensors_;
  std::vector<KeyedEntity<text_sensor::TextSensor>> text_sensors_;
  std::vector<KeyedEntity<number::Number>> numbers_;
  std::vector<KeyedEntity<select::Select>> selects_;
#ifdef USE_SWITCH
  std::vector<KeyedEntity<switch_::Switch>> switches_;
#endif
#ifdef USE_BUTTON
  std::vector<KeyedEntity<button::Button>> buttons_;
#endif
#ifdef USE_TEXT
  std::vector<KeyedEntity<text::Text>> texts_;
#endif
  std::vector<std::pair<const char *, float>> defaults_;
};

} // namespace wrg_dashboard
} // namespace esphome
