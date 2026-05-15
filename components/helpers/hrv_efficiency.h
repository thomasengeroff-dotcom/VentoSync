// ============================================================================
// VentoSync HRV – Heat Recovery Efficiency Calculator
// 
// Computes true energy-based HRV efficiency for ceramic regenerator systems
// using trapezoidal numerical integration over complete intake/exhaust cycles.
//
// Physical basis: η = ∫(T_supply - T_outside)dt / ∫(T_extract - T_outside)dt
// Reference: DIN EN 13141-8 (Ventilation for buildings)
// ============================================================================

#pragma once

#include <cmath>
#include <mutex>
#include <algorithm>

namespace ventosync {
namespace hrv {

/// Result of a completed HRV cycle efficiency calculation
struct CycleResult {
  float efficiency_pct{NAN};       ///< Energy-based efficiency [%]
  float energy_recovered_wh{0.0f}; ///< Approximate recovered energy [Wh]
  float avg_supply_temp{NAN};      ///< Average supply air temp during cycle [°C]
  float cycle_duration_s{0.0f};    ///< Actual cycle duration [s]
  bool  valid{false};              ///< Whether the result is trustworthy
};

/// Accumulator for trapezoidal integration over one phase
struct PhaseIntegrator {
  float numerator_integral{0.0f};   ///< ∫(T_supply - T_outside) dt
  float denominator_integral{0.0f}; ///< ∫(T_extract - T_outside) dt
  float prev_numerator{NAN};        ///< Previous (T_supply - T_outside) sample
  float prev_denominator{NAN};      ///< Previous (T_extract - T_outside) sample
  float prev_supply_temp{NAN};      ///< Previous T_supply for averaging
  float supply_temp_sum{0.0f};      ///< Running sum for average supply temp
  uint32_t prev_timestamp_ms{0};    ///< Timestamp of previous sample [ms]
  uint32_t start_timestamp_ms{0};   ///< Phase start timestamp [ms]
  uint32_t sample_count{0};         ///< Number of samples integrated

  void reset() {
    numerator_integral = 0.0f;
    denominator_integral = 0.0f;
    prev_numerator = NAN;
    prev_denominator = NAN;
    prev_supply_temp = NAN;
    supply_temp_sum = 0.0f;
    prev_timestamp_ms = 0;
    start_timestamp_ms = 0;
    sample_count = 0;
  }
};

class HrvEfficiencyCalculator {
 public:
  /// Configuration constants
  static constexpr float MIN_DELTA_T_KELVIN = 2.0f;      ///< Minimum ΔT for meaningful calculation
  static constexpr float MAX_VALID_EFFICIENCY = 105.0f;   ///< Upper plausibility bound [%]
  static constexpr float MIN_VALID_EFFICIENCY = -10.0f;   ///< Lower bound (slight negative = bypass cooling)
  static constexpr uint32_t MIN_SAMPLES_FOR_VALID = 3;    ///< Minimum samples per phase
  static constexpr float AIR_DENSITY_KG_M3 = 1.2f;       ///< Approximate air density at ~20°C
  static constexpr float AIR_CP_J_KGK = 1005.0f;         ///< Specific heat capacity of air

  /**
   * @brief Feed a new temperature sample into the integrator.
   * 
   * Call this at every NTC update interval (e.g., every 5s) during the
   * active supply-air (Zuluft) phase.
   *
   * @param t_supply   Current supply air temperature (after ceramic) [°C]
   * @param t_extract  Current extract air temperature (room air) [°C]  
   * @param t_outside  Current outside air temperature (before ceramic) [°C]
   * @param now_ms     Current timestamp in milliseconds (millis())
   */
  void add_sample(float t_supply, float t_extract, float t_outside, uint32_t now_ms) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (std::isnan(t_supply) || std::isnan(t_extract) || std::isnan(t_outside)) {
      return;  // Skip invalid readings
    }

    float num = t_supply - t_outside;    // Recovered temperature delta
    float den = t_extract - t_outside;   // Maximum possible delta

    // First sample of this phase: just store, no integration yet
    if (std::isnan(phase_.prev_numerator)) {
      phase_.prev_numerator = num;
      phase_.prev_denominator = den;
      phase_.prev_supply_temp = t_supply;
      phase_.prev_timestamp_ms = now_ms;
      phase_.start_timestamp_ms = now_ms;
      phase_.supply_temp_sum = t_supply;
      phase_.sample_count = 1;
      return;
    }

    // Trapezoidal integration: area = (f(t₀) + f(t₁)) / 2 × Δt
    float dt_s = static_cast<float>(now_ms - phase_.prev_timestamp_ms) / 1000.0f;

    // Guard against time jumps or rollover
    if (dt_s <= 0.0f || dt_s > 300.0f) {
      // Reset on implausible time gap (>5 min between samples)
      phase_.prev_numerator = num;
      phase_.prev_denominator = den;
      phase_.prev_supply_temp = t_supply;
      phase_.prev_timestamp_ms = now_ms;
      return;
    }

    phase_.numerator_integral += (phase_.prev_numerator + num) * 0.5f * dt_s;
    phase_.denominator_integral += (phase_.prev_denominator + den) * 0.5f * dt_s;
    phase_.supply_temp_sum += t_supply;
    phase_.sample_count++;

    phase_.prev_numerator = num;
    phase_.prev_denominator = den;
    phase_.prev_supply_temp = t_supply;
    phase_.prev_timestamp_ms = now_ms;
  }

  /**
   * @brief Finalize the current phase and compute cycle efficiency.
   *
   * Call this when the fan direction reverses (end of Zuluft phase).
   * After calling, the integrator is reset for the next cycle.
   *
   * @param volume_flow_m3h  Current volume flow rate [m³/h] (for energy calc, 0 to skip)
   * @return CycleResult with the computed efficiency
   */
  CycleResult finalize_cycle(float volume_flow_m3h = 0.0f) {
    std::lock_guard<std::mutex> lock(mutex_);

    CycleResult result;

    if (phase_.sample_count < MIN_SAMPLES_FOR_VALID) {
      phase_.reset();
      return result;  // Not enough data
    }

    float duration_s = static_cast<float>(phase_.prev_timestamp_ms - phase_.start_timestamp_ms) / 1000.0f;
    result.cycle_duration_s = duration_s;
    result.avg_supply_temp = phase_.supply_temp_sum / static_cast<float>(phase_.sample_count);

    // Check denominator: is there enough temperature difference?
    if (std::fabs(phase_.denominator_integral) < (MIN_DELTA_T_KELVIN * duration_s)) {
      // ΔT too small over the cycle → efficiency undefined (already near equilibrium)
      phase_.reset();
      return result;
    }

    float eta = (phase_.numerator_integral / phase_.denominator_integral) * 100.0f;

    // Plausibility check
    if (eta >= MIN_VALID_EFFICIENCY && eta <= MAX_VALID_EFFICIENCY) {
      result.efficiency_pct = std::clamp(eta, 0.0f, 100.0f);
      result.valid = true;

      // Optional: estimate recovered thermal energy
      if (volume_flow_m3h > 0.0f && duration_s > 0.0f) {
        // Q = ρ · V̇ · cₚ · ∫ΔT·dt  →  [J] → [Wh]
        float v_flow_m3s = volume_flow_m3h / 3600.0f;
        float energy_j = AIR_DENSITY_KG_M3 * v_flow_m3s * AIR_CP_J_KGK * phase_.numerator_integral;
        result.energy_recovered_wh = energy_j / 3600.0f;
      }
    }

    phase_.reset();
    return result;
  }

  /**
   * @brief Get a live (in-progress) efficiency estimate during the current phase.
   * 
   * Useful for dashboard display. Less accurate than finalize_cycle().
   * @return Current running efficiency [%], or NAN if insufficient data.
   */
  float get_live_efficiency() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (phase_.sample_count < 2) return NAN;

    float duration_s = static_cast<float>(phase_.prev_timestamp_ms - phase_.start_timestamp_ms) / 1000.0f;
    if (duration_s < 1.0f) return NAN;

    if (std::fabs(phase_.denominator_integral) < (MIN_DELTA_T_KELVIN * duration_s * 0.5f)) {
      return NAN;
    }

    float eta = (phase_.numerator_integral / phase_.denominator_integral) * 100.0f;
    if (eta < MIN_VALID_EFFICIENCY || eta > MAX_VALID_EFFICIENCY) return NAN;

    return std::clamp(eta, 0.0f, 100.0f);
  }

  /// Reset the integrator (e.g., on mode change or error)
  void reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    phase_.reset();
  }

  /// Get number of samples in current phase
  uint32_t get_sample_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return phase_.sample_count;
  }

 private:
  mutable std::mutex mutex_;
  PhaseIntegrator phase_;
};

/// Global singleton instance accessor for YAML
inline HrvEfficiencyCalculator& get_calculator() {
  static HrvEfficiencyCalculator instance;
  return instance;
}

}  // namespace hrv
}  // namespace ventosync
