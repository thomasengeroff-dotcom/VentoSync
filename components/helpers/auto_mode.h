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
// File:        auto_mode.h
// Description: Logic for the Ventilation Auto Mode Controller.
//              Final refactored version addressing all safety and maintainability concerns.
// Author:      Thomas Engeroff
// Created:     2026-03-29
// Modified:    2026-09-23
// ==========================================================================
#pragma once
#include "globals.h"

/**
 * @brief   Calculates absolute humidity (g/m³) from relative humidity and temperature.
 *
 * @details Uses the Magnus formula to derive the saturation vapor pressure, then
 *          converts to absolute humidity. This is critical for determining whether
 *          ventilation will actually reduce indoor moisture — comparing relative
 *          humidity alone is misleading because cold air at high %rH holds far
 *          less water than warm air at moderate %rH.
 *
 * @param[in] rh_percent  Relative humidity (0–100%).
 * @param[in] temp_c      Temperature in °C.
 *
 * @return  Absolute humidity in g/m³.
 */
inline float calculate_absolute_humidity(float rh_percent, float temp_c) {
    if (std::isnan(rh_percent) || std::isnan(temp_c)) return NAN;
    
    // Physikalische Plausibilitätsgrenzen
    if (temp_c <= -243.0f || temp_c > 100.0f) {
        ESP_LOGW("auto_mode", "Temperature out of physical range: %.1f°C", temp_c);
        return NAN;
    }
    if (rh_percent < 0.0f || rh_percent > 100.0f) {
        ESP_LOGW("auto_mode", "RH out of range: %.1f%%", rh_percent);
        return NAN;
    }
    
    // Magnus formula: e_s(T) = 6.112 * exp(17.67 * T / (T + 243.5)) [hPa]
    const float e_s = 6.112f * std::exp(17.67f * temp_c / (temp_c + 243.5f));
    const float denominator = 273.15f + temp_c;
    
    if (std::abs(denominator) < 0.001f) return NAN;
    
    // Absolute humidity: AH = 216.7 * (rH/100 * e_s) / (273.15 + T) [g/m³]
    const float result = 216.7f * (rh_percent / 100.0f * e_s) / denominator;
    
    // Physikalisch unmögliche Ausgabe abfangen
    if (result < 0.0f || result > 100.0f) {
        ESP_LOGW("auto_mode", "AH result out of range: %.3f g/m³", result);
        return NAN;
    }
    
    return result;
}

namespace auto_mode {

/**
 * @brief   Validates that all required components for auto mode are initialized.
 *
 * @details Ensures that sensors, controllers, and UI entities are non-null
 *          before the evaluation logic starts to prevent segmentation faults.
 *
 * @return  true if the system is safe to evaluate.
 */
inline bool is_system_ready() {
  if (auto_mode_active == nullptr || !auto_mode_active->value()) return false;
  if (system_on == nullptr || !system_on->value()) return false;
  if (ventilation_enabled == nullptr || !ventilation_enabled->value()) return false;
  
  return (ventilation_ctrl != nullptr && 
          automatik_min_fan_level != nullptr && 
          automatik_max_fan_level != nullptr &&
          fan_intensity_level != nullptr &&
          fan_intensity_display != nullptr &&
          fan_speed_update != nullptr &&
          update_leds != nullptr);
}

/**
 * @brief   Combines local and peer sensor data to determine room temperatures.
 *
 * @details This function implements "Sensor Fusion". If a local sensor is
 *          offline or NaN, it attempts to use data shared by peers over ESP-NOW.
 *          It also dynamically maps NTC sensors based on the active airflow
 *          direction to differentiate between Indoor and Outdoor ambient temps.
 *
 * @param[in]  now      Current system time.
 * @param[out] eff_in   Effective indoor temperature calculation result.
 * @param[out] eff_out  Effective outdoor temperature calculation result.
 * @param[in]  current_mode  Current operation mode for direction mapping.
 */
inline void get_effective_temperatures(uint32_t now, float &eff_in, float &eff_out, esphome::VentilationMode current_mode) {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return;

  float local_in = NAN;
  float local_out = NAN;

  // 1. Local Indoor Temp (SCD41 preferred)
  if (temperature != nullptr && !std::isnan(temperature->state)) {
    local_in = temperature->state;
  }

  // 2. Map NTC sensors based on current ventilation direction
  // Safe by-value return structure (no null pointer risk possible here in C++)
  const esphome::HardwareState hw_state = v->state_machine.get_target_state(now);
  const bool is_intake = hw_state.direction_in;
  const int internal_mode = static_cast<int>(current_mode);

  auto read_sensor = [](esphome::sensor::Sensor *s) -> float {
      if (s == nullptr) return NAN;
      float val = s->state;
      return (val > -50.0f && val < 100.0f) ? val : NAN;
  };

  if (internal_mode == esphome::MODE_VENTILATION) {
    if (is_intake) {
        local_out = read_sensor(temp_zuluft);
        if (std::isnan(local_in)) local_in = read_sensor(temp_abluft);
    } else {
        local_out = read_sensor(temp_zuluft);
        if (std::isnan(local_in)) local_in = read_sensor(temp_abluft);
    }
  } else if (internal_mode == esphome::MODE_ECO_RECOVERY) {
    local_out = read_sensor(temp_zuluft);
    if (std::isnan(local_in)) local_in = read_sensor(temp_abluft);
  }

  // Update controller state for networking
  v->local_t_in = local_in;
  v->local_t_out = local_out;

  // 3. Sensor Fusion (Peer fallback if local is missing)
  eff_in = local_in;
  eff_out = local_out;

  if (std::isnan(eff_in) && !std::isnan(v->last_peer_t_in) &&
      (now - v->last_peer_t_in_time < PEER_TIMEOUT_MS)) {
    eff_in = v->last_peer_t_in;
  }
  if (std::isnan(eff_out) && !std::isnan(v->last_peer_t_out) &&
      (now - v->last_peer_t_out_time < PEER_TIMEOUT_MS)) {
    eff_out = v->last_peer_t_out;
  }
}

/**
 * @brief   Logic for Summer Cooling / Bypass mode.
 *
 * @details Switches from Heat Recovery (MODE_ECO_RECOVERY) to Ventilation
 *          (MODE_VENTILATION) if:
 *          1. The HA "Sommerbetrieb" binary sensor confirms warm season
 *             (April-October AND outdoor temp > 18°C).
 *          2. The indoor temperature exceeds 22°C.
 *          3. The outdoor air is at least 1.5°C cooler.
 *          Includes a deactivation hysteresis to prevent mode oscillations.
 *          When HA is offline, sommerbetrieb defaults to false (safe: heat
 *          recovery stays active).
 */
inline esphome::VentilationMode determine_auto_operating_mode(float eff_in, float eff_out, esphome::VentilationMode current_mode,
                                                             bool lock_eco_mode = false) {
  // Smart Climate Control: while the room AC is active, cross-ventilation /
  // summer bypass is counter-productive (imports air the AC has to re-condition).
  // Heat recovery is enforced regardless of the temperature comparison.
  if (lock_eco_mode) {
    if (current_mode == esphome::MODE_VENTILATION) {
      ESP_LOGI("auto_mode", "Sommer-Kühlung DEAKTIVIERT (Klima-Koordination aktiv: WRG erzwungen)");
    }
    return esphome::MODE_ECO_RECOVERY;
  }

  // Summer cooling requires the HA "Sommerbetrieb" sensor to confirm warm season.
  // Default false when HA is offline → heat recovery stays active (safe fallback).
  const bool is_summer = (sommerbetrieb != nullptr && sommerbetrieb->has_state() && sommerbetrieb->state);

  // Runtime-configurable indoor threshold from HA entity
  const float threshold = (summer_cooling_threshold != nullptr)
      ? static_cast<float>(summer_cooling_threshold->value())
      : SUMMER_COOLING_THRESHOLD_INDOOR;
  const float threshold_hysteresis = threshold - 0.5f;

  if (std::isnan(eff_in) || std::isnan(eff_out)) {
    return (current_mode == esphome::MODE_VENTILATION || current_mode == esphome::MODE_ECO_RECOVERY) 
           ? current_mode : esphome::MODE_ECO_RECOVERY;
  }

  if (current_mode != esphome::MODE_VENTILATION) {
    // Activation: Only when summer AND indoor > threshold AND outdoor is sufficiently cooler
    if (is_summer && eff_in > threshold && eff_out < (eff_in - SUMMER_COOLING_MIN_DELTA)) {
      ESP_LOGI("auto_mode", "Sommer-Kühlung AKTIVIERT: Innen=%.1f°C, Außen=%.1f°C (Schwelle=%.1f°C)", eff_in, eff_out, threshold);
      return esphome::MODE_VENTILATION;
    }
    return esphome::MODE_ECO_RECOVERY;
  } else {
    // Deactivation: When NOT summer anymore OR hysteresis triggers
    if (!is_summer || eff_out >= (eff_in - SUMMER_COOLING_HYSTERESIS) || eff_in < threshold_hysteresis) {
      ESP_LOGI("auto_mode", "Sommer-Kühlung DEAKTIVIERT%s: Innen=%.1f°C, Außen=%.1f°C",
               !is_summer ? " (Sommerbetrieb OFF)" : " (Hysterese)", eff_in, eff_out);
      return esphome::MODE_ECO_RECOVERY;
    }
    return esphome::MODE_VENTILATION;
  }
}

/**
 * @brief   Freshness window for room-wide fusion of peer data.
 * @details At least 5 min, extended to two heartbeats when the user configured
 *          a longer ESP-NOW sync interval, capped at the peer timeout.
 */
inline uint32_t fusion_max_age_ms() {
  auto *v = ventilation_ctrl;
  const uint32_t sync_ms = (v != nullptr) ? v->sync_interval_ms : 60000u;
  return ventosync::room::fusion_max_age_ms(sync_ms, PEER_TIMEOUT_MS);
}

/**
 * @brief   Determines the highest CO2 value in the room from all sources.
 *
 * @details Combines the local effective CO2 sensor with the CO2 readings
 *          every peer shares over ESP-NOW (`room_co2`, always the sender's
 *          OWN sensor — never re-broadcast). Peer values older than
 *          `ventosync::room::PEER_DATA_MAX_AGE_MS` (5 min) are ignored.
 *          Used by the Smart Climate Control coordinator so the worst air
 *          quality anywhere in the room decides about throttling/emergency.
 *
 * @param[in] now  Current millis() timestamp for peer staleness checks.
 *
 * @return  Highest CO2 in ppm, or NAN if no source is available.
 */
inline float get_room_max_co2(uint32_t now) {
  const float local = (effective_co2 != nullptr) ? effective_co2->state : NAN;
  auto *v = ventilation_ctrl;
  if (v == nullptr) return (!std::isnan(local) && local > 0.0f) ? local : NAN;
  return ventosync::room::room_max_co2(local, v->peers, now, fusion_max_age_ms());
}

/**
 * @brief   Aggregates PID demands with CO2 priority and room-wide CO2 fusion.
 *
 * @details CO2 demand always takes priority over humidity to ensure air quality.
 *          Hysteresis (Grab at 0.01 / Release at 0.005) ensures stable behavior
 *          near setpoints. Room-wide fusion: the result is the maximum of
 *          the local demand and the highest LOCAL-SENSOR demand broadcast by
 *          any fresh peer (CO2 and humidity, full PID incl. integral term).
 *          Only the local-sensor demand is broadcast (`local_pid_demand`),
 *          so a fused value can never be echoed back and latch the room at
 *          a high level (feedback loop fixed in 0.10.21).
 *
 * @param[in] now         Current millis() timestamp.
 * @param[in] eff_in_temp Effective indoor temperature (for absolute humidity).
 * @param[in] eff_out_temp Effective outdoor temperature (for absolute humidity).
 * @param[in] suppress_humidity  Smart Climate Control: ignore the humidity PID
 *                               (the AC dehumidifies; ventilation would import
 *                               humid outdoor air). CO2-only regulation.
 */
inline float calculate_combined_demand(uint32_t now, float eff_in_temp, float eff_out_temp,
                                       bool suppress_humidity = false) {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return 0.0f;

  // Thresholds for hysteresis
  constexpr float CO2_GRAB_THRESHOLD   = 0.01f;  // CO2 takes exclusive control
  constexpr float CO2_RELEASE_THRESHOLD = 0.005f; // CO2 releases control to humidity

  float local_demand = NAN;

  // 1. CO2 Demand (always evaluated)
  float co2_demand = 0.0f;
  bool has_co2_data = false;
  if (co2_pid_result != nullptr) {
    const float co2_val = co2_pid_result->value();
    if (!std::isnan(co2_val)) {
      co2_demand = std::max(0.0f, co2_val);
      has_co2_data = true;
    }
  }

  // 2. Humidity Demand (always evaluated, but only used when CO2 is satisfied)
  //    Skipped entirely while Smart Climate Control throttles (CO2-only loop).
  float hum_demand = 0.0f;
  bool has_hum_data = false;
  if (!suppress_humidity && scd41_humidity != nullptr && outdoor_humidity != nullptr && humidity_pid_result != nullptr) {
    const float in_hum = scd41_humidity->state;
    const float out_hum = outdoor_humidity->state;
    const float hum_pid_val = humidity_pid_result->value();
    if (!std::isnan(in_hum) && !std::isnan(out_hum) && !std::isnan(hum_pid_val)) {
      has_hum_data = true;
      // Use absolute humidity (g/m³) for scientifically correct comparison.
      // Relative humidity alone is misleading: cold air at 90% rH holds far
      // less water than warm air at 50% rH.
      const float abs_in = calculate_absolute_humidity(in_hum, eff_in_temp);
      const float abs_out = calculate_absolute_humidity(out_hum, eff_out_temp);
      if (!std::isnan(abs_in) && !std::isnan(abs_out) && abs_out < abs_in) {
        hum_demand = std::max(0.0f, hum_pid_val);
      } else if (std::isnan(abs_in) || std::isnan(abs_out)) {
        // Temperature data unavailable — fall back to relative humidity comparison
        if (out_hum < in_hum) {
          hum_demand = std::max(0.0f, hum_pid_val);
        }
      }
    }
  }

  // 3. CO2 Priority Logic with Hysteresis
  if (v->co2_is_controlling) {
    // CO2 is currently in control — only release when demand drops below lower threshold
    if (co2_demand < CO2_RELEASE_THRESHOLD && has_co2_data) {
      v->co2_is_controlling = false;
      // Reset CO2 PID integral to prevent windup carryover after long high-CO2 periods
      if (pid_co2 != nullptr) pid_co2->reset_integral_term();
      ESP_LOGD("auto_mode", "CO2 demand released (%.3f < %.3f), humidity PID enabled (integral reset)", co2_demand, CO2_RELEASE_THRESHOLD);
    }
  } else {
    // CO2 is not in control — grab if demand exceeds upper threshold
    if (co2_demand >= CO2_GRAB_THRESHOLD) {
      v->co2_is_controlling = true;
      // Reset humidity PID integral so it starts fresh — its integral will
      // accumulate accurately from this point during CO2 control.
      if (pid_humidity != nullptr) pid_humidity->reset_integral_term();
      ESP_LOGD("auto_mode", "CO2 demand grabbed control (%.3f >= %.3f), humidity PID integral reset", co2_demand, CO2_GRAB_THRESHOLD);
    }
  }

  // 4. Select demand based on priority
  //    CO2 has priority for the hysteresis state machine (grab/release),
  //    but we always take the HIGHER of both demands to ensure humidity
  //    is never under-served while CO2 is controlling. Since max() can
  //    only increase demand, it cannot cause oscillation.
  if (v->co2_is_controlling) {
    // CO2 is the primary signal; humidity can boost above it if needed
    local_demand = has_hum_data ? std::max(co2_demand, hum_demand) : co2_demand;
  } else if (has_hum_data) {
    // CO2 is satisfied, use humidity demand
    local_demand = hum_demand;
  } else if (has_co2_data) {
    // Have CO2 data but no humidity data — use CO2 (even if near zero)
    local_demand = co2_demand;
  }
  // else: local_demand stays NAN → triggers "hold state" guard

  // 5. Broadcast ONLY the local-sensor demand (feedback-loop guard).
  //    Devices without their own sensors (nosensor / radar_only / NTConly)
  //    still get co2_pid_result == 0.0 from the PID (NaN input -> output 0),
  //    so gate on a real CO2 reading or valid humidity data and advertise
  //    NaN otherwise ("no data" instead of a misleading 0 %).
  const bool local_co2_valid = (effective_co2 != nullptr) && !std::isnan(effective_co2->state) &&
                               effective_co2->state > 0.0f;
  const bool has_local_sensor_data = local_co2_valid || has_hum_data;
  v->local_pid_demand = has_local_sensor_data ? local_demand : NAN;

  // 6. Room-wide demand fusion: adopt the highest local-sensor demand of any
  //    fresh peer (max age 5 min). Peers compute it with their full CO2 and
  //    humidity PIDs (integral term, hysteresis, enthalpy guard), so a Master
  //    without sensors regulates the room exactly like the sensor device.
  //    Smart Climate Control is consistent room-wide (switch + AC state are
  //    shared), so throttled peers broadcast a CO2-only demand as well.
  float effective_demand = local_demand;
  const float peer_demand = ventosync::room::room_max_peer_demand(v->peers, now, fusion_max_age_ms());
  if (!std::isnan(peer_demand)) {
    // Only meaningful when both sides measure — a device without sensors
    // naturally deviates from the room demand.
    if (has_local_sensor_data && !std::isnan(effective_demand) && peer_demand > effective_demand + 0.5f) {
      ESP_LOGW("auto_mode", "Large peer demand deviation: peer=%.2f local=%.2f — check peer sensor health",
               peer_demand, effective_demand);
    }
    if (std::isnan(effective_demand) || peer_demand > effective_demand + 0.05f) {
      ESP_LOGI("auto_mode", "Adopting higher peer demand: %.2f (Local: %.2f)", peer_demand, effective_demand);
    }
    effective_demand = ventosync::room::max_valid(effective_demand, peer_demand);
  }

  if (!std::isnan(effective_demand)) {
    return std::clamp(effective_demand, 0.0f, 1.0f);
  }

  return NAN;
}

// =========================================================
// SECTION: Smart Climate Control (HVAC Coordination) glue
// Decision logic lives in components/ventilation_logic/hvac_coordinator.h.
// =========================================================

/**
 * @brief   Determines whether ventilation can physically dry the room.
 *
 * @details Uses the room-wide wettest spot (highest rH of the local SCD41 and
 *          all fresh peers' `room_humidity`), so the mold guard also works on
 *          devices without a humidity sensor. Compares absolute humidity
 *          (g/m³) indoors vs. outdoors using the same Magnus-based conversion
 *          as the humidity PID path, with the temperature measured at the
 *          same spot. Falls back to a relative comparison when temperatures
 *          are unavailable.
 *
 * @param[in]  now           Current millis() for peer staleness checks.
 * @param[in]  eff_in_temp   Effective indoor temperature (local source).
 * @param[in]  eff_out_temp  Effective outdoor temperature.
 * @param[out] indoor_rh     Room-wide indoor relative humidity (NaN if unavailable).
 * @param[out] rh_from_peer  True if `indoor_rh` was reported by a peer.
 *
 * @return  true if outdoor air is drier than indoor air.
 */
inline bool ventilation_can_dry(uint32_t now, float eff_in_temp, float eff_out_temp,
                                float &indoor_rh, bool &rh_from_peer) {
  indoor_rh = NAN;
  rh_from_peer = false;
  const float local_rh = (scd41_humidity != nullptr) ? scd41_humidity->state : NAN;
  auto *v = ventilation_ctrl;
  ventosync::room::HumiditySource src;
  if (v != nullptr) {
    src = ventosync::room::room_max_humidity(local_rh, eff_in_temp, v->peers, now, fusion_max_age_ms());
  } else if (!std::isnan(local_rh)) {
    src.rh_percent = local_rh;
    src.temp_c = eff_in_temp;
  }
  if (std::isnan(src.rh_percent)) return false;
  indoor_rh = src.rh_percent;
  rh_from_peer = src.from_peer;

  if (outdoor_humidity == nullptr) return false;
  const float out_hum = outdoor_humidity->state;
  if (std::isnan(out_hum)) return false;

  const float in_temp = std::isnan(src.temp_c) ? eff_in_temp : src.temp_c;
  const float abs_in = calculate_absolute_humidity(src.rh_percent, in_temp);
  const float abs_out = calculate_absolute_humidity(out_hum, eff_out_temp);
  if (!std::isnan(abs_in) && !std::isnan(abs_out)) return abs_out < abs_in;
  return out_hum < src.rh_percent; // Temperature unavailable — relative fallback
}

/// @brief True while the Home Assistant API connection is up.
inline bool ha_api_connected() {
#ifdef USE_API
  return (esphome::api::global_api_server != nullptr) && esphome::api::global_api_server->is_connected();
#else
  return false;
#endif
}

/**
 * @brief   Fills the AC-state part of the coordinator inputs.
 *
 * @details Local: the state Home Assistant pushed to this device via the API
 *          action `set_ac_active` (with age for expiry). Peers: any fresh peer
 *          that reports its own HA AC state as active (ESP-NOW v10 flag).
 */
inline void fill_ac_inputs(ventosync::hvac::Inputs &in, uint32_t now) {
  in.ha_connected = ha_api_connected();
  in.ac_has_state = hvac_state::ac_has_state;
  in.ac_reported_active = hvac_state::ac_reported;
  in.ac_state_age_ms = hvac_state::ac_has_state ? (now - hvac_state::ac_update_ms) : UINT32_MAX;
  auto *v = ventilation_ctrl;
  in.peer_ac_active = (v != nullptr) &&
                      ventosync::room::any_fresh_peer_ac_active(v->peers, now, fusion_max_age_ms());
}

/**
 * @brief   Refreshes the AC flag this device broadcasts to its peers.
 *
 * @details Only the LOCAL HA state is broadcast (never the room-wide OR),
 *          so the flag cannot latch between devices. Called every 10 s
 *          regardless of the operating mode and on every HA push.
 */
inline void refresh_local_ac_broadcast(uint32_t now) {
  auto *v = ventilation_ctrl;
  if (v == nullptr) return;
  ventosync::hvac::Inputs in;
  fill_ac_inputs(in, now);
  v->hvac_local_ac_active = ventosync::hvac::local_ac_active(in);
}

/**
 * @brief   Collects all HVAC coordination inputs and runs the coordinator.
 *
 * @param[in] now       Current millis().
 * @param[in] eff_in    Effective indoor temperature (for the mold guard).
 * @param[in] eff_out   Effective outdoor temperature (for the mold guard).
 *
 * @return  Decision to apply in evaluate_auto_mode().
 */
inline ventosync::hvac::Decision evaluate_hvac_coordination(uint32_t now, float eff_in, float eff_out) {
  ventosync::hvac::Inputs in;
  in.now_ms = now;
  in.enabled = (hvac_enabled_val != nullptr) && hvac_enabled_val->value(); // room-wide switch
  fill_ac_inputs(in, now);

  // CO2: room-wide maximum (local sensor + all fresh peers, max age 5 min),
  // so the coordinator sees the worst-case air quality in the room.
  const float local_co2 = (effective_co2 != nullptr) ? effective_co2->state : NAN;
  in.co2_ppm = get_room_max_co2(now);
  const bool co2_from_peer = !std::isnan(in.co2_ppm) && (std::isnan(local_co2) || in.co2_ppm > local_co2);

  // Humidity (mold guard): room-wide wettest spot, local or peer.
  float indoor_rh = NAN;
  bool rh_from_peer = false;
  in.ventilation_can_dry = ventilation_can_dry(now, eff_in, eff_out, indoor_rh, rh_from_peer);
  in.indoor_rh_percent = indoor_rh;

  // Room-wide thresholds (globals mirrored by the sliders and synced over ESP-NOW)
  in.co2_threshold_ppm = (hvac_co2_threshold_val != nullptr)
      ? static_cast<float>(hvac_co2_threshold_val->value()) : ventosync::hvac::DEFAULT_CO2_THRESHOLD_PPM;
  in.emergency_co2_ppm = (hvac_emergency_co2_val != nullptr)
      ? static_cast<float>(hvac_emergency_co2_val->value()) : ventosync::hvac::DEFAULT_EMERGENCY_CO2_PPM;
  in.max_fan_level = (hvac_max_fan_level_val != nullptr)
      ? hvac_max_fan_level_val->value() : ventosync::hvac::DEFAULT_MAX_FAN_LEVEL;

  ventosync::hvac::Decision d = hvac_state::coordinator.evaluate(in);

  if (d.state != hvac_state::last_decision.state) {
    ESP_LOGI("hvac", "Klima-Koordination: %s -> %s (AC=%d, CO2=%.0f ppm%s, rH=%.0f%%%s)",
             ventosync::hvac::state_label(hvac_state::last_decision.state),
             ventosync::hvac::state_label(d.state),
             d.ac_active ? 1 : 0, in.co2_ppm, co2_from_peer ? " via Peer" : "",
             in.indoor_rh_percent, rh_from_peer ? " via Peer" : "");
  }
  hvac_state::last_decision = d;
  return d;
}

/**
 * @brief   Re-asserts the CO2 PID setpoint (idempotent).
 *
 * @details The HA slider, ESP-NOW config sync and Smart Climate Control all
 *          write the PID target. This helper makes the coordinator's choice
 *          win on every evaluation cycle and resets the integral term on a
 *          real setpoint change to avoid windup carry-over.
 *
 * @param[in] desired  Target CO2 concentration in ppm.
 */
inline void apply_co2_setpoint(float desired) {
  if (pid_co2 == nullptr || std::isnan(desired)) return;
  if (pid_co2->target_temperature == desired) return;
  ESP_LOGI("hvac", "CO2 PID setpoint %.0f -> %.0f ppm", pid_co2->target_temperature, desired);
  auto call = pid_co2->make_call();
  call.set_target_temperature(desired);
  call.perform();
  pid_co2->reset_integral_term();
}

} // namespace auto_mode

/**
 * @brief   Main entry point for the Smart-Automatik logic.
 *
 * @details Periodically evaluates climate state, syncs with peers, and updates 
 *          the local fan intensity. Implements the "Master Authority" rule:
 *          Slaves follow the Master's (ID=1) discrete intensity level to 
 *          ensure perfectly synchronized across the room.
 *
 * @param[in] force  If true, bypasses the 2s evaluation rate-limit.
 */
inline void evaluate_auto_mode(bool force) {
  // AC flag for peers must stay current in every operating mode.
  auto_mode::refresh_local_ac_broadcast(millis());

  if (!auto_mode::is_system_ready()) {
    // Not regulating: never advertise a frozen demand to the room.
    if (ventilation_ctrl != nullptr) ventilation_ctrl->local_pid_demand = NAN;
    return;
  }

  auto *v = ventilation_ctrl;
  // v is guaranteed non-null by is_system_ready()
  const uint32_t now = millis();

  // Rate-limiting: Prevent multiple calls (e.g., from timer + network handler) 
  // from bypassing ramping limits.
  static uint32_t last_eval_ms = 0;
  if (!force && now - last_eval_ms < 2000) return; // Allow max once per 2 seconds
  last_eval_ms = now;

  // 1. Snapshot common states to avoid race conditions during evaluation
  esphome::VentilationMode current_mode = v->state_machine.current_mode;
  int current_level = static_cast<int>(fan_intensity_level->value());

  // 2. Climate Sensor Fusion
  float eff_in = NAN, eff_out = NAN;
  auto_mode::get_effective_temperatures(now, eff_in, eff_out, current_mode);

  // 2b. Smart Climate Control (HVAC Coordination) — modifier on the auto logic
  const ventosync::hvac::Decision hvac = auto_mode::evaluate_hvac_coordination(now, eff_in, eff_out);

  // Re-assert the CO2 PID setpoint: relaxed target while throttled, otherwise
  // the user's normal threshold (also repairs slider / peer-sync overwrites).
  {
    const float normal_setpoint = (auto_co2_threshold_val != nullptr)
        ? static_cast<float>(auto_co2_threshold_val->value()) : 1000.0f;
    auto_mode::apply_co2_setpoint(hvac.relaxed_co2_setpoint ? hvac.co2_setpoint : normal_setpoint);
  }

  // Humidity PID kept running while suppressed — clear its integral on release
  if (hvac_state::prev_suppress_humidity && !hvac.suppress_humidity && pid_humidity != nullptr) {
    pid_humidity->reset_integral_term();
    ESP_LOGD("hvac", "Humidity PID re-enabled (integral reset)");
  }
  hvac_state::prev_suppress_humidity = hvac.suppress_humidity;

  // 3. Mode Management (Summer Cooling) — heat recovery enforced while AC active
  esphome::VentilationMode target_mode = auto_mode::determine_auto_operating_mode(eff_in, eff_out, current_mode,
                                                                                  hvac.lock_eco_mode);
  
  if (current_mode != target_mode) {
    v->set_mode(target_mode);
    current_mode = target_mode; // Update local snapshot for correct demand mapping
  }

  // 4. Presence-based demand adjustment (if LD2450 data available)
  
  // 5. Air Quality Power Management
  float demand = auto_mode::calculate_combined_demand(now, eff_in, eff_out, hvac.suppress_humidity);

  // Guard: After switching into Smart-Automatik the PID outputs are stale for
  // a few cycles (the PID controller's proportional term immediately overwrites
  // the reset-to-zero values set in system_lifecycle.h). Hold the fan at
  // min_level until the first genuine sensor cycle has completed (~15 s = 1.5
  // CO2 PID cycles at the SCD4x's ~5-30 s update rate).
  static uint32_t mode_switch_holdoff_until_ms = 0;
  if (force) {
    // evaluate_auto_mode(true) is called from system_lifecycle on mode switch
    mode_switch_holdoff_until_ms = now + 15000u;
    ESP_LOGD("auto_mode", "Mode-switch holdoff active for 15 s (until %u ms)", mode_switch_holdoff_until_ms);
  }
  if (mode_switch_holdoff_until_ms > 0) {
    if (now < mode_switch_holdoff_until_ms) {
      demand = 0.0f;
    } else {
      mode_switch_holdoff_until_ms = 0;
      ESP_LOGD("auto_mode", "Mode-switch holdoff expired, PID demand active");
    }
  }

  // FIXED: If demand is NAN (e.g., all sensors offline/unstable), we abort to hold the last state
  if (std::isnan(demand)) {
     ESP_LOGV("auto_mode", "Demand is NAN, skipping update to hold last state.");
     return;
  }
  
  // 6. Calculate target level with hysteresis and ramping
  int min_l = static_cast<int>(automatik_min_fan_level->value());
  int max_l = static_cast<int>(automatik_max_fan_level->value());
  if (hvac.restrict_levels) {
    // Smart Climate Control: hard cap while the AC is active. The minimum is
    // deliberately Level 1 (DIN 1946-6 base ventilation) — below the user's
    // normal moisture-protection minimum, because the AC handles dehumidification.
    min_l = hvac.min_level;
    max_l = hvac.max_level;
  }
  if (min_l > max_l) std::swap(min_l, max_l);
  
  int target_level = min_l;

  // AUTHORITY RULE: In Auto Mode, Slaves should follow the Master's discrete intensity.
  // This prevents the "jumping" behavior where nodes fight over level boundaries.
  bool slave_following_master = false;
  if (v->device_id != 1) { // I am a Slave
    // Check if we have a recent state from the Master
    // NOTE: v->peers is updated in process_espnow_packet_local() when
    // a MSG_STATE or MSG_STATUS_RESPONSE is received. It mirrors the
    // fan_intensity and device_id from the last received packet.
    // Maximum staleness: PEER_TIMEOUT_MS (defined in globals.h).
    for (const auto &peer : v->peers) {
      if (peer.device_id == 1 && (now - peer.last_seen_ms < PEER_TIMEOUT_MS)) {
        target_level = peer.fan_intensity;
        slave_following_master = true;
        break;
      }
    }
  }

  if (slave_following_master) {
    // Enforce the local window too. Room-wide settings and the room-wide AC
    // state make it identical to the Master's; it only differs transiently
    // (e.g. until the next heartbeat carries a changed AC state).
    target_level = std::clamp(target_level, min_l, max_l);
  } else {
    // I am the Master OR the Master is offline -> level from demand.
    // Pure + unit-tested; the result is always inside [min_l, max_l], so a
    // shrinking window (Smart Climate Control cap) is enforced via the ramp.
    target_level = VentilationLogic::calculate_auto_target_level(demand, current_level, min_l, max_l);
  }

  // Soft Ramping: Maximum ±1 level per evaluation cycle (10s)
  // Boost: Allow ±2 steps if mode just changed to reach target faster
  static bool first_eval = true;
  static esphome::VentilationMode last_committed_mode = esphome::MODE_ECO_RECOVERY;
  int max_ramp_step = 1;
  
  if (first_eval) {
    last_committed_mode = current_mode;
    first_eval = false;
  } else if (current_mode != last_committed_mode) {
    max_ramp_step = 2;
    last_committed_mode = current_mode;
  }

  if (target_level > current_level) {
    target_level = std::min(target_level, current_level + max_ramp_step);
  } else if (target_level < current_level) {
    target_level = std::max(target_level, current_level - max_ramp_step);
  }

  // 7. Commit State Updates
  if (current_level != target_level) {
    if (slave_following_master) {
       ESP_LOGD("auto_mode", "Slave following Master level: %d (Local demand: %.2f)", target_level, demand);
    }
    
    fan_intensity_level->value() = target_level;
    fan_intensity_display->publish_state(target_level);
    v->current_fan_intensity = target_level;
    
    fan_speed_update->execute();
    update_leds->execute();
    
    ESP_LOGI("auto_mode", "Automatic level %s %d -> %d (demand=%.2f)", 
             slave_following_master ? "synced" : "ramped", current_level, target_level, demand);
  }
}

/**
 * @brief   Home Assistant API action `set_ac_active` (Smart Climate Control).
 *
 * @details Home Assistant pushes the room's AC state to the device (see
 *          documentation/en/en_smart-climate-control.md). The state expires
 *          after `AC_STATE_MAX_AGE_MS` unless it is re-sent, and it is shared
 *          with the room over ESP-NOW, so HA only has to reach one device.
 *
 * @param[in] active  true = AC is conditioning (cool/heat/dry/auto).
 */
inline void hvac_on_ha_ac_state(bool active) {
  const uint32_t now = millis();
  const bool changed = !hvac_state::ac_has_state || hvac_state::ac_reported != active;
  hvac_state::ac_has_state = true;
  hvac_state::ac_reported = active;
  hvac_state::ac_update_ms = now;
  if (hvac_ac_active != nullptr) hvac_ac_active->publish_state(active);
  if (changed) {
    ESP_LOGI("hvac", "AC state from Home Assistant: %s", active ? "aktiv" : "inaktiv");
  }
  auto_mode::refresh_local_ac_broadcast(now);
  if (changed) evaluate_auto_mode();
}
