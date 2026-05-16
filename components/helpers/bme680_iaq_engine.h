// bme680_iaq_engine.h
// ============================================================
// VentoSync – BME680 IAQ Engine
// Thread-safe, self-calibrating air quality processor
// ============================================================
#pragma once

#include <cmath>
#include <mutex>
#include <string>
#include "esphome/core/log.h"

namespace ventosync {

static const char *const TAG_IAQ = "bme680_iaq";

struct IAQResult {
  float co2eq_ppm;      // 400–5000
  float baseline_ohm;   // Current learned baseline
  float trend_ppm_min;  // Positive = worsening
  std::string level;    // Human-readable classification
  std::string trend_dir;
  bool healthy;         // false if sensor is failing
  bool burn_in_done;    // true after 48h
};

class BME680IAQEngine {
 public:
  // ── Configuration ──────────────────────────────────────
  struct Config {
    float hum_ideal       = 40.0f;
    float hum_weight      = 0.20f;
    float gas_weight      = 0.80f;
    float ppm_min         = 400.0f;
    float ppm_max         = 5000.0f;
    float gas_log_range   = 50.0f;   // baseline / gas_log_range = "dirty" floor
    float alpha_up        = 0.002f;  // Baseline learning rate (gas > baseline)
    float alpha_down      = 0.0001f; // Baseline learning rate (gas < baseline)
    float alpha_warmup    = 0.05f;   // Fast learning during warm-up
    int   warmup_samples  = 20;
    int   burn_in_minutes = 2880;    // 48 h
    int   max_consecutive_fails = 10;
    float save_delta_pct  = 0.02f;   // Save to flash when baseline changes >2%
    uint32_t save_interval_ms = 3600000; // Minimum 1h between flash writes
  };

  BME680IAQEngine() : cfg_() {}
  explicit BME680IAQEngine(const Config &cfg) : cfg_(cfg) {}

  // ── Restore from flash (call once at boot) ────────────
  void restore(float saved_baseline, int saved_burn_in_minutes) {
    std::lock_guard<std::mutex> lock(mtx_);
    burn_in_counter_ = saved_burn_in_minutes;
    if (saved_baseline > 1000.0f) {
      baseline_ = saved_baseline;
      warmed_up_ = true;
      ESP_LOGI(TAG_IAQ, "Restored baseline=%.0f Ω, burn-in=%d min",
               baseline_, burn_in_counter_);
    }
    initialized_ = true;
  }

  // ── Main update (call every sensor cycle, e.g. 30 s) ──
  //    Returns true if flash save is recommended
  bool update(float gas_ohm, float humidity_pct, IAQResult &out) {
    std::lock_guard<std::mutex> lock(mtx_);
    bool should_save = false;

    // ── Health tracking ──
    if (std::isnan(gas_ohm) || gas_ohm < 1000.0f) {
      fail_count_++;
      out.healthy = (fail_count_ < cfg_.max_consecutive_fails);
      if (!out.healthy) {
        ESP_LOGW(TAG_IAQ, "Sensor unhealthy: %d consecutive failures", fail_count_);
      }
      out.co2eq_ppm   = NAN;
      out.baseline_ohm = baseline_;
      out.trend_ppm_min = NAN;
      out.level       = "Unbekannt";
      out.trend_dir   = "Unbekannt";
      out.burn_in_done = (burn_in_counter_ >= cfg_.burn_in_minutes);
      return false;
    }

    if (fail_count_ >= cfg_.max_consecutive_fails) {
      ESP_LOGI(TAG_IAQ, "Sensor recovered after %d failures", fail_count_);
    }
    fail_count_ = 0;
    out.healthy = true;
    sample_count_++;
    burn_in_counter_++;

    // ── Baseline learning ──
    if (!warmed_up_) {
      baseline_ = (baseline_ == 0.0f)
                    ? gas_ohm
                    : ((1.0f - cfg_.alpha_warmup) * baseline_ + cfg_.alpha_warmup * gas_ohm);
      if (sample_count_ > cfg_.warmup_samples) warmed_up_ = true;
    } else {
      float alpha = (gas_ohm > baseline_) ? cfg_.alpha_up : cfg_.alpha_down;
      baseline_ = (1.0f - alpha) * baseline_ + alpha * gas_ohm;
    }

    out.baseline_ohm = baseline_;
    out.burn_in_done = (burn_in_counter_ >= cfg_.burn_in_minutes);

    // ── IAQ calculation ──
    if (baseline_ < 1000.0f || std::isnan(humidity_pct)) {
      out.co2eq_ppm = NAN;
      out.level = "Initialisierung";
      out.trend_ppm_min = NAN;
      out.trend_dir = "Unbekannt";
      return false;
    }

    float log_clean = log10f(baseline_);
    float log_dirty = log10f(baseline_ / cfg_.gas_log_range);
    float log_val   = log10f(fmaxf(gas_ohm, 1.0f));

    float g_score = (log_clean - log_val) / (log_clean - log_dirty);
    g_score = std::clamp(g_score, 0.0f, 1.0f);

    float h_score = (humidity_pct >= cfg_.hum_ideal)
                      ? (humidity_pct - cfg_.hum_ideal) / (100.0f - cfg_.hum_ideal)
                      : (cfg_.hum_ideal - humidity_pct) / cfg_.hum_ideal;
    h_score = std::clamp(h_score, 0.0f, 1.0f);

    float combined = cfg_.gas_weight * g_score + cfg_.hum_weight * h_score;
    float ppm = cfg_.ppm_min + combined * (cfg_.ppm_max - cfg_.ppm_min);
    out.co2eq_ppm = ppm;

    // ── Trend ──
    if (prev_ppm_ != 0.0f) {
      float raw_trend = ppm - prev_ppm_;
      // Exponential moving average for smooth trend
      trend_ema_ = 0.3f * raw_trend + 0.7f * trend_ema_;
      out.trend_ppm_min = trend_ema_;
    } else {
      out.trend_ppm_min = 0.0f;
    }
    prev_ppm_ = ppm;

    // ── Classifications ──
    out.level = classify_level(ppm);
    out.trend_dir = classify_trend(out.trend_ppm_min);

    // ── Flash save decision ──
    uint32_t now = millis();
    if (now - last_save_ms_ > cfg_.save_interval_ms) {
      float delta = (saved_baseline_snapshot_ > 0.0f)
                      ? fabsf(baseline_ - saved_baseline_snapshot_) / saved_baseline_snapshot_
                      : 1.0f;
      if (delta > cfg_.save_delta_pct) {
        last_save_ms_ = now;
        saved_baseline_snapshot_ = baseline_;
        should_save = true;
      }
    }

    return should_save;
  }

  // ── Getters (thread-safe) ──────────────────────────────
  float get_baseline() {
    std::lock_guard<std::mutex> lock(mtx_);
    return baseline_;
  }

  int get_burn_in_minutes() {
    std::lock_guard<std::mutex> lock(mtx_);
    return burn_in_counter_;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(mtx_);
    baseline_ = 0.0f;
    warmed_up_ = false;
    sample_count_ = 0;
    burn_in_counter_ = 0;
    prev_ppm_ = 0.0f;
    trend_ema_ = 0.0f;
    fail_count_ = 0;
    ESP_LOGW(TAG_IAQ, "Engine reset – 48h burn-in required");
  }

  // ── Status string for UI ──────────────────────────────
  std::string get_status() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (baseline_ < 1000.0f) return "Initialisierung";
    if (burn_in_counter_ < 60) return "Aufwaermphase";
    if (burn_in_counter_ < cfg_.burn_in_minutes) {
      char buf[48];
      snprintf(buf, sizeof(buf), "Burn-in (%dh / 48h)", burn_in_counter_ / 60);
      return std::string(buf);
    }
    return "Bereit";
  }

 private:
  static std::string classify_level(float ppm) {
    if (ppm <= 700.0f)  return "Ausgezeichnet";
    if (ppm <= 1000.0f) return "Gut";
    if (ppm <= 1500.0f) return "Maessig";
    if (ppm <= 2000.0f) return "Schlecht";
    return "Sehr schlecht";
  }

  static std::string classify_trend(float trend) {
    if (trend < -50.0f) return "Stark verbessernd";
    if (trend < -10.0f) return "Verbessernd";
    if (trend <= 10.0f) return "Stabil";
    if (trend <= 50.0f) return "Verschlechternd";
    return "Stark verschlechternd";
  }

  Config cfg_;
  std::mutex mtx_;

  float baseline_             = 0.0f;
  float saved_baseline_snapshot_ = 0.0f;
  bool  warmed_up_            = false;
  bool  initialized_          = false;
  int   sample_count_         = 0;
  int   burn_in_counter_      = 0;
  int   fail_count_           = 0;
  float prev_ppm_             = 0.0f;
  float trend_ema_            = 0.0f;
  uint32_t last_save_ms_      = 0;
};

}  // namespace ventosync
