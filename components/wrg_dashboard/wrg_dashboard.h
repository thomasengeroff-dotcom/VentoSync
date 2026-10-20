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
// Modified:    2026-10-20
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
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/components/ventilation_group/ventilation_group.h"
#include "esphome/core/component.h"
#include <atomic>
#include <mutex>
#include <string>
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

class WrgDashboard : public Component, public AsyncWebHandler {
public:
  /// Maximum number of /set actions waiting for loop(). The queue is drained
  /// every 100 ms, so a human using the UI never gets near it; a client
  /// flooding /set gets HTTP 503 instead of exhausting the heap.
  static constexpr size_t MAX_PENDING_ACTIONS = 16;
  /// Maximum length of a /set value (longest legit value: a mode name).
  static constexpr size_t MAX_VALUE_LEN = 32;
  /// /state is rebuilt this often while a client polls.
  static constexpr uint32_t STATE_REBUILD_MS = 1000;
  /// Stop rebuilding /state this long after the last request (no client).
  static constexpr uint32_t STATE_IDLE_MS = 15000;
  /// A cached /state older than this is not served (503, the UI retries).
  static constexpr uint32_t STATE_MAX_AGE_MS = 5000;

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
   * @return  WIFI priority minus 1.0.
   */
  float get_setup_priority() const override {
    return setup_priority::WIFI - 1.0f;
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

  /**
   * @brief   Checks if the request can be handled by this component.
   * @param[in] request  The incoming AsyncWebServerRequest.
   * @return  true for GET /ui, /state, /meta and POST /set.
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
  bool is_writable_key_(const std::string &key) const;
  void dispatch_set_(const DashboardAction &action);
  std::string build_state_json_();
  std::string build_meta_json_();

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

  web_server_base::WebServerBase *base_{nullptr};

  sensor::Sensor *scd41_co2_{nullptr};
  sensor::Sensor *effective_co2_{nullptr};
  sensor::Sensor *scd41_temperature_{nullptr};
  sensor::Sensor *scd41_humidity_{nullptr};
  sensor::Sensor *bme680_temperature_{nullptr};
  sensor::Sensor *bme680_humidity_{nullptr};
  esphome::VentilationController *ventilation_ctrl_{nullptr};
  const char *project_version_{""};
  const char *firmware_variant_{""};
  const char *reset_reason_{""};

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
};

} // namespace wrg_dashboard
} // namespace esphome
