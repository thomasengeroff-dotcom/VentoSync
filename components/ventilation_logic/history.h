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
// File:        history.h
// Description: 24 h dashboard history: 5-minute averages of the room
//              climate, fan level, demand and heat recovery in a RAM ring
//              buffer, serialized for GET /history. No ESPHome
//              dependencies — unit-tested.
// Author:      Thomas Engeroff
// Created:     2026-10-21
// Modified:    2026-10-21
// ==========================================================================
#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

/// @file history.h
/// @brief Device-side history of the web dashboard (wrg_dashboard).
namespace ventosync {
namespace history {

/// One sample of every channel is added this often.
constexpr uint32_t SAMPLE_MS = 10000;
/// Samples are averaged into one bucket per interval (5 min).
constexpr uint32_t BUCKET_MS = 300000;
/// 24 h of buckets.
constexpr size_t CAPACITY = 288;
/// Stored for a channel without any valid sample in the bucket (JSON null).
constexpr int16_t NO_DATA = INT16_MIN;

static_assert(BUCKET_MS % SAMPLE_MS == 0, "a bucket must hold a whole number of samples");
static_assert(CAPACITY * BUCKET_MS == 24u * 3600u * 1000u, "the history covers exactly 24 h");

/// Recorded channels (index into Values / Bucket).
enum Channel : uint8_t {
  CO2,         ///< Room CO2 / eCO2 in ppm.
  ROOM_T,      ///< Room temperature in °C.
  ROOM_RH,     ///< Room relative humidity in %.
  LEVEL,       ///< Fan level 0–10 (0 = Aus).
  DEMAND,      ///< Local PID demand 0–1 (Smart-Automatik only).
  EFFICIENCY,  ///< Heat recovery efficiency in %.
  T_OUT,       ///< Outdoor air (NTC Zuluft) in °C.
  T_IN,        ///< Extract air (NTC Abluft) in °C.
  T_SUPPLY,    ///< Supply air after the exchanger (indoor NTC, intake phase mean) in °C.
  CHANNEL_COUNT
};

/// JSON key and fixed-point scale (stored value = round(value × scale)).
struct ChannelInfo {
  const char *key;
  int scale;
};

constexpr std::array<ChannelInfo, CHANNEL_COUNT> CHANNELS = {{
    {"co2", 1},
    {"t", 10},
    {"rh", 10},
    {"level", 10},
    {"demand", 1000},
    {"eff", 10},
    {"t_out", 10},
    {"t_in", 10},
    {"t_sup", 10},
}};

using Values = std::array<float, CHANNEL_COUNT>;  ///< One sample, NaN = no value.
using Bucket = std::array<int16_t, CHANNEL_COUNT>;

/**
 * @brief   Converts a value to the stored fixed-point format.
 * @return  NO_DATA for NaN / infinity, otherwise round(value × scale)
 *          clamped to the int16 range (NO_DATA itself is never produced).
 */
inline int16_t encode(float value, int scale) {
  if (!std::isfinite(value)) return NO_DATA;
  const float scaled = std::round(value * static_cast<float>(scale));
  if (scaled >= 32767.0f) return 32767;
  if (scaled <= -32767.0f) return -32767;
  return static_cast<int16_t>(scaled);
}

/**
 * @brief   Ring buffer of 5-minute averages, fed every SAMPLE_MS.
 *
 * @details One instance lives in WrgDashboard (RAM only — the history starts
 *          over after a reboot, no flash wear). A channel's bucket value is
 *          the mean of its valid samples; NaN samples are skipped, so a
 *          sensor that drops out for a minute still yields an average. A
 *          bucket is appended even if every channel is empty, so the time
 *          axis has no holes.
 */
class Recorder {
 public:
  /**
   * @brief   Adds one sample; closes the bucket once BUCKET_MS have passed.
   * @details The first call starts the first bucket. If the caller was
   *          stalled for more than one bucket, the missed buckets are
   *          appended as empty ones (time axis stays correct). Wrap-safe.
   * @param[in] values  Current value of every channel (NaN = none).
   * @param[in] now_ms  millis().
   * @return  true if at least one bucket was appended.
   */
  bool add(const Values &values, uint32_t now_ms) {
    if (!this->started_) {
      this->started_ = true;
      this->bucket_start_ms_ = now_ms;
    }
    for (size_t c = 0; c < CHANNEL_COUNT; c++) {
      if (std::isfinite(values[c])) {
        this->sum_[c] += values[c];
        this->n_[c]++;
      }
    }
    const uint32_t elapsed = now_ms - this->bucket_start_ms_;
    if (elapsed < BUCKET_MS) return false;

    Bucket b;
    for (size_t c = 0; c < CHANNEL_COUNT; c++) {
      b[c] = this->n_[c] > 0 ? encode(this->sum_[c] / static_cast<float>(this->n_[c]), CHANNELS[c].scale) : NO_DATA;
      this->sum_[c] = 0.0f;
      this->n_[c] = 0;
    }
    this->push_(b);

    // Missed buckets (stalled caller): empty, at most one full history
    const uint32_t missed = elapsed / BUCKET_MS - 1u;
    Bucket empty;
    empty.fill(NO_DATA);
    for (uint32_t i = 0; i < missed && i < CAPACITY; i++) this->push_(empty);
    // Keep the bucket grid (no drift from the sampling jitter)
    this->bucket_start_ms_ += (elapsed / BUCKET_MS) * BUCKET_MS;
    return true;
  }

  /// Number of stored buckets (≤ CAPACITY).
  size_t size() const { return this->count_; }

  /// Bucket i, 0 = oldest.
  const Bucket &at(size_t i) const {
    return this->ring_[(this->head_ + CAPACITY - this->count_ + i) % CAPACITY];
  }

  /// Time since the newest bucket ended (the running bucket's age).
  uint32_t age_ms(uint32_t now_ms) const { return this->started_ ? now_ms - this->bucket_start_ms_ : 0u; }

  /**
   * @brief   Appends the history as JSON to `out` (GET /history).
   * @details {"interval_s":300,"age_s":42,"scale":{"co2":1,…},
   *          "series":{"co2":[612,…],…}} — oldest bucket first, values in
   *          fixed point (divide by scale), null = no data. The newest
   *          bucket ended age_s seconds ago; bucket i ended
   *          (size − 1 − i) × interval_s seconds before that.
   */
  void append_json(std::string &out, uint32_t now_ms) const {
    char num[16];
    out.reserve(out.size() + 160 + this->count_ * CHANNEL_COUNT * 5);
    std::snprintf(num, sizeof(num), "%u", static_cast<unsigned>(BUCKET_MS / 1000u));
    out += "{\"interval_s\":";
    out += num;
    std::snprintf(num, sizeof(num), "%u", static_cast<unsigned>(this->age_ms(now_ms) / 1000u));
    out += ",\"age_s\":";
    out += num;
    out += ",\"scale\":{";
    for (size_t c = 0; c < CHANNEL_COUNT; c++) {
      std::snprintf(num, sizeof(num), "%d", CHANNELS[c].scale);
      if (c > 0) out += ',';
      out += '"';
      out += CHANNELS[c].key;
      out += "\":";
      out += num;
    }
    out += "},\"series\":{";
    for (size_t c = 0; c < CHANNEL_COUNT; c++) {
      if (c > 0) out += ',';
      out += '"';
      out += CHANNELS[c].key;
      out += "\":[";
      for (size_t i = 0; i < this->count_; i++) {
        if (i > 0) out += ',';
        const int16_t v = this->at(i)[c];
        if (v == NO_DATA) {
          out += "null";
        } else {
          std::snprintf(num, sizeof(num), "%d", static_cast<int>(v));
          out += num;
        }
      }
      out += ']';
    }
    out += "}}";
  }

 private:
  void push_(const Bucket &b) {
    this->ring_[this->head_] = b;
    this->head_ = (this->head_ + 1) % CAPACITY;
    if (this->count_ < CAPACITY) this->count_++;
  }

  std::array<Bucket, CAPACITY> ring_{};
  size_t head_ = 0;   ///< Next write position.
  size_t count_ = 0;
  std::array<float, CHANNEL_COUNT> sum_{};
  std::array<uint16_t, CHANNEL_COUNT> n_{};
  bool started_ = false;
  uint32_t bucket_start_ms_ = 0;  ///< Start of the running bucket = end of the newest one.
};

}  // namespace history
}  // namespace ventosync
