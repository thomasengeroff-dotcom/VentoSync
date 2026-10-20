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
// File:        dashboard_html.h
// Description: HTML/CSS strings for the WRG web dashboard.
// Author:      Thomas Engeroff
// Created:     2026-03-09
// Modified:    2026-10-20
// ==========================================================================
#pragma once

const char DASHBOARD_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de" class="dark">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Wohnraumlüftung Dashboard</title>
  <script src="https://cdn.tailwindcss.com"></script>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script>
    tailwind.config = {
      darkMode: 'class',
      theme: {
        extend: {
          colors: {
            bg: '#121212',
            card: '#1e1e1e',
            accent: '#03dac6',
            accentHover: '#01b4a0',
            danger: '#cf6679',
            warn: '#f2b84b',
          }
        }
      }
    }
  </script>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      height: 16px;
      width: 16px;
      border-radius: 50%;
      background: #03dac6;
      cursor: pointer;
      margin-top: -6px;
    }
    input[type=range]::-webkit-slider-runnable-track {
      width: 100%;
      height: 4px;
      cursor: pointer;
      background: #555;
      border-radius: 2px;
    }
  </style>
</head>
<body class="bg-bg text-gray-200 min-h-screen p-4 sm:p-6 lg:p-8 flex flex-col items-center">
  <div class="w-full max-w-5xl space-y-6">

    <header class="text-center space-y-2">
      <h1 class="text-3xl font-bold tracking-tight text-white mb-6">
        WRG Lüftung Dashboard
        <span id="hdr_version" class="text-sm font-normal text-gray-500 ml-2"></span>
      </h1>
    </header>

    <!-- Automatik-Status: what the ventilation does and why, plus the active room inputs -->
    <div id="status_card" class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 border-l-4">
      <div class="text-xs uppercase tracking-wider text-gray-500 mb-1">Automatik-Status</div>
      <div id="val_auto_status" class="text-xl font-semibold text-white">--</div>
      <div id="status_chips" class="flex flex-wrap gap-2 mt-3 empty:hidden"></div>
    </div>

    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">

      <!-- Device & system info -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-3">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Gerät &amp; System</h2>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Geräte-ID:</span> <span class="font-medium text-lg"><span id="val_device_id">--</span> <span id="master_badge" class="hidden text-xs font-bold text-accent bg-accent/10 px-2 py-0.5 rounded-full">MASTER</span></span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Floor / Raum:</span> <span class="font-medium text-lg"><span id="val_floor_id">--</span> / <span id="val_room_id">--</span></span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Geräte-Phase (A/B):</span> <span class="font-medium text-lg" id="val_phase">--</span></div>
        <div class="text-xs text-gray-500">IDs und Phase ändern: <a href="/" class="text-accent hover:underline">ESPHome-Seite</a></div>
        <div class="border-t border-gray-700 pt-3 space-y-1 text-sm">
          <div class="flex justify-between"><span class="text-gray-400">Firmware:</span> <span id="sys_version">--</span></div>
          <div class="flex justify-between"><span class="text-gray-400">Variante:</span> <span id="sys_variant">--</span></div>
          <div class="flex justify-between"><span class="text-gray-400">Laufzeit:</span> <span id="sys_uptime">--</span></div>
          <div class="flex justify-between"><span class="text-gray-400">Letzter Neustart:</span> <span id="sys_reset">--</span></div>
          <div class="flex justify-between"><span class="text-gray-400">WLAN-Signal:</span> <span id="sys_rssi">--</span></div>
          <div class="flex justify-between"><span class="text-gray-400">Freier RAM (min.):</span> <span id="sys_heap">--</span></div>
        </div>
      </div>

      <!-- Status & Sensors -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Aktuelle Sensordaten</h2>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Board-Temperatur:</span> <span class="font-medium text-lg"><span id="val_temperature">--</span> °C</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Luftdruck:</span> <span class="font-medium text-lg"><span id="val_pressure">--</span> hPa</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Feuchtigkeit (Außen):</span> <span class="font-medium text-lg"><span id="val_outdoor_humidity">--</span> %</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Temperatur Zuluft:</span> <span class="font-medium text-lg"><span id="val_temp_zuluft">--</span> °C</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Temperatur Abluft:</span> <span class="font-medium text-lg"><span id="val_temp_abluft">--</span> °C</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Effizienz WRG:</span> <span class="font-medium text-accent text-lg"><span id="val_heat_recovery_efficiency">--</span> %</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Lüfter RPM:</span> <span class="font-medium text-lg" id="val_fan_rpm">--</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Luftrichtung:</span> <span class="font-medium text-accent text-lg" id="val_direction_display">--</span></div>
      </div>

      <!-- Air Quality & Maintenance combined column -->
      <div class="flex flex-col gap-6">
        <!-- Air Quality -->
        <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4 flex-1">
          <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Luftqualität</h2>
          <div class="flex justify-between items-center"><span class="text-gray-400 text-sm" id="lbl_room_co2">CO2:</span> <span class="font-medium text-lg"><span id="val_room_co2">--</span> ppm</span></div>
          <!-- SGP41 board: only shown when /state reports voc_index / nox_index (sensor_SGP41_SHT45.yaml) -->
          <div id="voc_index_row" class="hidden flex justify-between items-center"><span class="text-gray-400 text-sm">VOC-Index:</span> <span class="font-medium text-lg" id="val_voc_index">--</span></div>
          <div id="nox_index_row" class="hidden flex justify-between items-center"><span class="text-gray-400 text-sm">NOx-Index:</span> <span class="font-medium text-lg" id="val_nox_index">--</span></div>
          <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Bewertung:</span> <span class="font-medium text-lg" id="val_room_co2_bewertung">--</span></div>
          <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Temperatur:</span> <span class="font-medium text-lg"><span id="val_room_temperature">--</span> °C</span></div>
          <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Luftfeuchtigkeit:</span> <span class="font-medium text-lg"><span id="val_room_humidity">--</span> %</span></div>
        </div>

        <!-- Maintenance -->
        <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4">
          <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Wartung</h2>
          <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Filter Betriebstage:</span> <span class="font-medium text-lg" id="val_filter_operating_days">--</span></div>
          <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Filterwechsel Alarm:</span> <span class="font-medium text-lg" id="val_filter_change_alarm">--</span></div>
          <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Radar Präsenz:</span> <span class="font-medium text-lg" id="val_radar_presence">--</span></div>
          <button id="btn_filter_reset" onclick="filterReset()" class="hidden w-full text-sm font-medium bg-gray-800 hover:bg-gray-700 border border-gray-600 rounded-lg p-2 transition-colors">Filterwechsel bestätigen (Zähler zurücksetzen)</button>
        </div>
      </div>

      <!-- Controls: rows are rendered from SETTINGS (below) and shown only when /state has the key -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4 md:col-span-2 lg:col-span-3">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Einstellungen</h2>
        <p class="text-xs text-gray-500 -mt-2"><span class="text-accent/80 font-semibold">RAUMWEIT</span> = gilt für alle Geräte des Raums und wird per ESP-NOW verteilt.</p>

        <div class="grid grid-cols-1 lg:grid-cols-2 gap-6 mt-2">
          <div class="space-y-6">
            <div>
              <span class="text-sm text-gray-400 font-medium block mb-2 border-b border-gray-700 pb-1">Steuerung</span>
              <div class="space-y-4">
                <div class="flex flex-col space-y-2">
                  <span class="text-sm text-gray-400 font-medium">Lüfter Modus <span class="text-[10px] font-semibold text-accent/80">RAUMWEIT</span></span>
                  <select id="luefter_modus" data-sel="luefter_modus" onchange="sendSet('luefter_modus', this.value, this)" class="bg-gray-800 border border-gray-700 text-white text-sm rounded-lg focus:ring-accent focus:border-accent block w-full p-2.5 transition-colors cursor-pointer"></select>
                </div>

                <div class="flex flex-col space-y-2">
                  <div class="flex justify-between"><span class="text-sm text-gray-400 font-medium">Lüfter Intensität</span><span id="label_fan_intensity" class="font-bold text-accent">--</span></div>
                  <input type="range" id="fan_intensity_display" min="1" max="10" step="1" onchange="sendSet('fan_intensity_display', this.value, this)" oninput="document.getElementById('label_fan_intensity').innerText = this.value" class="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer mt-2">
                  <span id="override_hint" class="text-xs text-gray-500"></span>
                </div>
                <div id="grp_control" class="space-y-3"></div>
              </div>
            </div>
            <div data-group><span class="text-sm text-gray-400 font-medium block mb-2 border-b border-gray-700 pb-1">Klima-Koordination</span><div id="grp_hvac" class="space-y-3"></div></div>
            <div data-group><span class="text-sm text-gray-400 font-medium block mb-2 border-b border-gray-700 pb-1">Urlaubsmodus</span><div id="grp_vac" class="space-y-3"></div></div>
          </div>
          <div class="space-y-6">
            <div data-group><span class="text-sm text-gray-400 font-medium block mb-2 border-b border-gray-700 pb-1">Smart-Automatik</span><div id="grp_auto" class="space-y-3"></div></div>
            <div data-group><span class="text-sm text-gray-400 font-medium block mb-2 border-b border-gray-700 pb-1">NOx-Stoßlüftung</span><div id="grp_nox" class="space-y-3"></div></div>
            <div data-group><span class="text-sm text-gray-400 font-medium block mb-2 border-b border-gray-700 pb-1">Netzwerk</span><div id="grp_net" class="space-y-3"></div></div>
          </div>
        </div>
      </div>

      <!-- Verbundene Geräte (ESP-NOW) -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4 md:col-span-2 lg:col-span-3 hidden" id="peers_card">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Verbundene Geräte (ESP-NOW)</h2>
        <div id="peers_container" class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mt-2"></div>
      </div>

      <!-- Chart -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col md:col-span-2 lg:col-span-3">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-4">Graphen & Verlauf</h2>
        <div class="relative w-full h-[400px]">
          <canvas id="historyChart"></canvas>
        </div>
      </div>

    </div>
  </div>

  <script>
    function sanitizeHTML(str) {
        if (typeof str !== 'string') return str;
        return str.replace(/[<>&"]/g, function(c) {
            return {'<': '&lt;', '>': '&gt;', '&': '&amp;', '"': '&quot;'}[c];
        });
    }

    // Value formatting helpers for the device cards
    function fmtNum(v, digits, unit) {
        return (v !== undefined && v !== null && !isNaN(v)) ? Number(v).toFixed(digits) + unit : "--";
    }
    function fmtTxt(v) {
        return (v !== undefined && v !== null && v !== "") ? sanitizeHTML(String(v)) : "--";
    }
    function fmtUptime(s) {
        if (s === undefined || s === null) return "--";
        const d = Math.floor(s / 86400), h = Math.floor(s % 86400 / 3600), m = Math.floor(s % 3600 / 60);
        return (d > 0 ? d + " d " : "") + h + " h " + m + " min";
    }
    function chip(text, tone) {
        const cls = {accent: 'text-accent bg-accent/10 border-accent/30', warn: 'text-warn bg-warn/10 border-warn/30',
                     danger: 'text-danger bg-danger/10 border-danger/30', gray: 'text-gray-300 bg-gray-700/40 border-gray-600'}[tone];
        return `<span class="text-xs font-medium px-2.5 py-1 rounded-full border ${cls}">${sanitizeHTML(text)}</span>`;
    }
    // Air quality block shown on every device card. Values come from the local
    // sensors or, for peers, from the ESP-NOW packet (SCD43 or BME680 eCO2).
    function airQualityBlock(co2, rating, temp, hum) {
        return `<div class="text-[10px] uppercase tracking-wider text-gray-500 mt-3 mb-1">Luftqualität</div>
            <div class="grid grid-cols-2 gap-x-4 gap-y-1 text-xs text-gray-500 bg-black/20 p-2 rounded">
                <div class="flex justify-between"><span>CO2:</span> <strong class="text-gray-400">${fmtNum(co2, 0, " ppm")}</strong></div>
                <div class="flex justify-between"><span>Bewertung:</span> <strong class="text-gray-400">${fmtTxt(rating)}</strong></div>
                <div class="flex justify-between"><span>Temperatur:</span> <strong class="text-gray-400">${fmtNum(temp, 1, " °C")}</strong></div>
                <div class="flex justify-between"><span>Feuchte:</span> <strong class="text-gray-400">${fmtNum(hum, 1, " %")}</strong></div>
            </div>`;
    }

    // Settings rows: [type (n = number, t = toggle, s = select), /state key, label, unit, room-wide].
    // Limits and select options come from /meta (the YAML entity definitions).
    const SETTINGS = {
      grp_control: [['n', 'vent_timer', 'Durchlüften-Dauer', 'min (0 = unbegrenzt)', 1],
                    ['t', 'child_lock', 'Kindersicherung (Tasten sperren)', '', 0],
                    ['t', 'bt_proxy', 'Bluetooth Proxy (experimentell)', '', 0]],
      grp_auto: [['n', 'automatik_min_luefterstufe', 'Min. Stufe', '', 1],
                 ['n', 'automatik_max_luefterstufe', 'Max. Stufe', '', 1],
                 ['n', 'auto_co2_threshold', 'CO2-Grenzwert', 'ppm', 1],
                 ['n', 'auto_humidity_threshold', 'Feuchte-Grenzwert', '%', 1],
                 ['n', 'auto_presence_slider', 'Anwesenheit-Anpassung (manuelle Modi)', 'Stufen', 1],
                 ['n', 'auto_override_duration', 'Übersteuerung Dauer', 'min', 1],
                 ['n', 'auto_summer_cooling_threshold', 'Sommerkühlung Schwelle', '°C', 0]],
      grp_hvac: [['t', 'hvac_enabled', 'Klima-Koordination aktiv', '', 1],
                 ['n', 'hvac_co2_threshold', 'CO2-Grenzwert (Klima an)', 'ppm', 1],
                 ['n', 'hvac_max_fan_level', 'Max. Stufe (Klima an)', '', 1],
                 ['n', 'hvac_emergency_co2', 'CO2-Notfallgrenze', 'ppm', 1]],
      grp_nox: [['t', 'nox_boost', 'NOx-Stoßlüftung aktiv', '', 1],
                ['n', 'nox_boost_threshold', 'Schwelle (dieses Gerät)', 'Index', 0]],
      grp_vac: [['s', 'vacation_mode', 'Betriebsmodus', '', 0],
                ['n', 'vacation_intensity', 'Intensität', '', 0]],
      grp_net: [['n', 'sync_interval_config', 'ESP-NOW Sync-Intervall', 'min', 1]],
    };
    const INPUT_CLS = 'bg-gray-700 border border-gray-600 text-white text-sm rounded focus:ring-accent focus:border-accent block p-1.5';
    function renderSettings() {
      for (const [grp, rows] of Object.entries(SETTINGS)) {
        document.getElementById(grp).innerHTML = rows.map(([t, k, label, unit, rw]) => {
          const badge = rw ? ' <span class="text-[10px] font-semibold text-accent/80">RAUMWEIT</span>' : '';
          let ctl;
          if (t === 'n') {
            ctl = `<span class="flex items-center gap-2 shrink-0"><input type="number" id="${k}" data-num="${k}" onchange="sendSet('${k}', this.value, this)" class="${INPUT_CLS} w-20 text-center"><span class="text-xs text-gray-500">${unit}</span></span>`;
          } else if (t === 't') {
            ctl = `<input type="checkbox" id="${k}" data-sw="${k}" onchange="sendSet('${k}', this.checked ? 1 : 0, this)" class="w-5 h-5 accent-accent cursor-pointer shrink-0">`;
          } else {
            ctl = `<select id="${k}" data-sel="${k}" onchange="sendSet('${k}', this.value, this)" class="${INPUT_CLS} w-44 cursor-pointer"></select>`;
          }
          return `<label data-row="${k}" class="hidden flex justify-between items-center gap-3 bg-gray-800/50 p-3 rounded-lg border border-gray-700/50"><span class="text-sm text-gray-300">${label}${badge}</span>${ctl}</label>`;
        }).join('');
      }
    }
    renderSettings();

    // Limits of the writable settings (/meta, built once by the device)
    let META = null;
    async function loadMeta() {
      try {
        const res = await fetch('/meta');
        if (!res.ok) { setTimeout(loadMeta, 1000); return; }
        META = await res.json();
        for (const [k, [min, max, step]] of Object.entries(META.numbers || {})) {
          const el = document.getElementById(k);
          if (el) { el.min = min; el.max = max; el.step = step; }
        }
        for (const [k, opts] of Object.entries(META.selects || {})) {
          const el = document.getElementById(k);
          if (el) el.innerHTML = opts.map(o => `<option value="${sanitizeHTML(o)}">${sanitizeHTML(o)}</option>`).join('');
        }
        if ((META.buttons || []).includes('filter_reset')) {
          document.getElementById('btn_filter_reset').classList.remove('hidden');
        }
        updateData();
      } catch (e) {
        console.error('Error fetching meta:', e);
        setTimeout(loadMeta, 2000);
      }
    }

    // Chart Setup
    const maxHistoryPoints = 150; // approx 5 minutes at 2s interval
    const chartData = {
      labels: ["", "", "", "", ""],
      datasets: [
        {
          label: 'Lüfter RPM',
          borderColor: '#03dac6',
          backgroundColor: '#03dac6',
          data: [null, null, null, null, null],
          yAxisID: 'y',
          tension: 0.3,
          borderWidth: 2,
          pointRadius: 0
        },
        {
          label: 'Raumtemp °C',
          borderColor: '#cf6679',
          backgroundColor: '#cf6679',
          data: [null, null, null, null, null],
          yAxisID: 'y1',
          tension: 0.3,
          borderWidth: 2,
          pointRadius: 0
        },
        {
          label: 'CO2 ppm',
          borderColor: '#bb86fc',
          backgroundColor: '#bb86fc',
          data: [null, null, null, null, null],
          yAxisID: 'y2',
          tension: 0.3,
          borderWidth: 2,
          pointRadius: 0
        },
        {
          label: 'Luftfeuchte %',
          borderColor: '#03a9f4',
          backgroundColor: '#03a9f4',
          data: [null, null, null, null, null],
          yAxisID: 'y1',
          tension: 0.3,
          borderWidth: 2,
          pointRadius: 0
        }
      ]
    };

    const ctx = document.getElementById('historyChart').getContext('2d');
    const historyChart = new Chart(ctx, {
      type: 'line',
      data: chartData,
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        interaction: { mode: 'index', intersect: false },
        plugins: {
          legend: {
            labels: { color: '#e5e7eb', usePointStyle: true, boxWidth: 8 },
            position: 'top'
          },
          tooltip: {
            backgroundColor: 'rgba(30, 30, 30, 0.9)',
            titleColor: '#fff',
            bodyColor: '#e5e7eb',
            borderColor: '#374151',
            borderWidth: 1,
            padding: 10
          }
        },
        scales: {
          x: {
            ticks: { color: '#9ca3af', maxRotation: 0, autoSkipPadding: 15 },
            grid: { color: '#374151', drawBorder: false }
          },
          y: {
            type: 'linear',
            display: true,
            position: 'left',
            title: { display: true, text: 'RPM', color: '#03dac6', font: {size: 11} },
            ticks: { color: '#9ca3af' },
            grid: { color: '#374151', drawBorder: false }
          },
          y1: {
            type: 'linear',
            display: true,
            position: 'right',
            title: { display: true, text: '°C / %', color: '#cf6679', font: {size: 11} },
            ticks: { color: '#9ca3af' },
            grid: { drawOnChartArea: false }
          },
          y2: {
            type: 'linear',
            display: true,
            position: 'right',
            title: { display: true, text: 'CO2 ppm', color: '#bb86fc', font: {size: 11} },
            ticks: { color: '#9ca3af' },
            grid: { drawOnChartArea: false }
          }
        }
      }
    });

    // Border colour of the status card per automatik_grund key (auto_status.h)
    const REASON_TONE = {co2_notfall: 'danger', schimmelschutz: 'danger', nox: 'danger',
                         fenster: 'warn', urlaub: 'warn', uebersteuerung: 'warn', keine_daten: 'warn',
                         co2: 'accent', feuchte: 'accent'};
    const TONE_BORDER = {danger: '#cf6679', warn: '#f2b84b', accent: '#03dac6'};
    const MODE_SHORT = ["Automatik", "WRG", "Durchlüften", "Stoßlüftung", "Aus"]; // UI mode index 0-4

    function renderStatus(data) {
      document.getElementById('val_auto_status').textContent = data.auto_status || '--';
      document.getElementById('status_card').style.borderLeftColor = TONE_BORDER[REASON_TONE[data.auto_reason]] || '#4b5563';

      const chips = [];
      if (data.vacation_active) chips.push(chip('Urlaubsmodus', 'warn'));
      if (data.window_guard) chips.push(chip(data.window_paused ? 'Fenster offen – pausiert' : 'Fenster offen (Sperre ignoriert)', 'warn'));
      if (data.auto_reason === 'nox') chips.push(chip('NOx-Stoßlüftung', 'danger'));
      if (data.luefter_modus === 'Smart-Automatik' && Number(data.auto_override_min) > 0)
        chips.push(chip(`Übersteuert – noch ${data.auto_override_min} min`, 'warn'));
      if (data.room_presence) chips.push(chip('Anwesenheit im Raum', 'accent'));
      if (data.hvac_enabled && data.hvac_status) chips.push(chip('Klima: ' + data.hvac_status, 'accent'));
      if (data.child_lock) chips.push(chip('Kindersicherung aktiv', 'gray'));
      if (data.filter_change_alarm) chips.push(chip('Filterwechsel fällig', 'danger'));
      document.getElementById('status_chips').innerHTML = chips.join('');
    }

    function renderSystem(data) {
      const sys = data.sys || {};
      document.getElementById('hdr_version').textContent = sys.version ? 'v' + sys.version : '';
      document.getElementById('sys_version').textContent = sys.version || '--';
      document.getElementById('sys_variant').textContent = sys.variant || '--';
      document.getElementById('sys_uptime').textContent = fmtUptime(sys.uptime_s);
      document.getElementById('sys_reset').textContent = sys.reset_reason || '--';
      document.getElementById('sys_rssi').textContent = (sys.rssi !== undefined) ? sys.rssi + ' dBm' : '--';
      document.getElementById('sys_heap').textContent = (sys.heap_free !== undefined)
        ? `${Math.round(sys.heap_free / 1024)} KB (${Math.round(sys.heap_min / 1024)} KB)` : '--';
      document.getElementById('master_badge').classList.toggle('hidden', !data.is_master);
    }

    // Shows each settings row whose key /state reports and syncs its value
    // (not while the user edits it); hides groups without visible rows.
    function renderSettingValues(data) {
      document.querySelectorAll('[data-row]').forEach(row => {
        const k = row.dataset.row;
        row.classList.toggle('hidden', data[k] === undefined);
        const el = document.getElementById(k);
        if (data[k] === undefined || document.activeElement === el) return;
        if (el.type === 'checkbox') el.checked = !!data[k];
        else if (data[k] !== null) el.value = data[k];
      });
      document.querySelectorAll('[data-group]').forEach(g => {
        g.classList.toggle('hidden', g.querySelector('[data-row]:not(.hidden)') === null);
      });
    }

    function peerCard(title, local, modeLabel, speed, phaseBadge, rpm, boardT, pid, flags, dotCls, dotTitle, air) {
      return `<div class="${local ? 'bg-gray-700/50 border-gray-600' : 'bg-gray-800/80 border-gray-700 hover:border-gray-600 transition-colors'} rounded-lg p-4 border">
            <div class="font-bold ${local ? 'text-gray-300' : 'text-white'} mb-3 pb-2 border-b border-gray-600 flex justify-between items-center">
              <span>${title}</span>
              <span class="w-2 h-2 rounded-full ${dotCls}" title="${dotTitle}"></span>
            </div>
            <div class="flex justify-between text-sm mb-2">
                <span class="text-gray-400">Modus: <strong class="text-gray-200">${sanitizeHTML(modeLabel)}</strong></span>
                <span class="flex items-center gap-2">Stufe: <strong class="text-gray-200">${sanitizeHTML(String(speed))}</strong> ${phaseBadge}</span>
            </div>
            ${flags ? `<div class="flex flex-wrap gap-1 mb-1">${flags}</div>` : ''}
            <div class="grid grid-cols-2 gap-x-4 gap-y-1 text-xs text-gray-500 bg-black/20 p-2 rounded mt-3">
                <div class="flex justify-between"><span>Lüfter RPM:</span> <strong class="text-gray-400">${rpm}</strong></div>
                <div class="flex justify-between"><span>Board-temp:</span> <strong class="text-gray-400">${boardT}</strong></div>
                <div class="flex justify-between"><span>PID:</span> <strong class="text-accent/80">${pid}</strong></div>
            </div>
            ${air}
        </div>`;
    }
    const BADGE_IN = "<span class='text-accent font-bold px-2 py-0.5 bg-accent/10 rounded-full text-xs'>IN</span>";
    const BADGE_OUT = "<span class='text-danger font-bold px-2 py-0.5 bg-danger/10 rounded-full text-xs'>OUT</span>";
    const BADGE_NONE = "<span class='text-gray-400 font-bold px-2 py-0.5 bg-gray-700/50 rounded-full text-xs'>--</span>";
    const MASTER_TAG = " <span class='text-[10px] font-bold text-accent bg-accent/10 px-1.5 py-0.5 rounded-full'>MASTER</span>";
    const miniFlag = t => `<span class="text-[10px] uppercase tracking-wide text-gray-300 bg-gray-700/60 px-1.5 py-0.5 rounded">${t}</span>`;
    const pct = v => (v !== undefined && v !== null && !isNaN(v)) ? Math.round(v * 100) + "%" : "--";

    function renderPeers(data) {
      document.getElementById('peers_card').classList.remove('hidden');
      const dir = data.direction_display || "";
      const localBadge = dir.includes("Zuluft") ? BADGE_IN : (dir.includes("Abluft") ? BADGE_OUT : BADGE_NONE);
      const localMode = data.luefter_modus === 'Wärmerückgewinnung' ? 'WRG' : (data.luefter_modus || '--');
      const localFlags = [data.radar_presence ? miniFlag('Präsenz') : ''].join('');

      let html = peerCard(`Gerät ${sanitizeHTML(String(data.device_id || "--"))}${data.is_master ? MASTER_TAG : ''} (lokal)`, true,
          localMode, data.fan_intensity_display || "--", localBadge, fmtNum(data.fan_rpm, 0, ""),
          fmtNum(data.temperature, 1, " °C"), pct(data.pid_demand), localFlags, 'bg-gray-400', 'Dieses Gerät',
          airQualityBlock(data.room_co2, data.room_co2_bewertung, data.room_temperature, data.room_humidity));

      (data.peers || []).forEach(peer => {
        const mode = MODE_SHORT[peer.mode_index] || "Unbekannt";
        // Direction only means something while the fan runs
        const badge = peer.mode_index === 4 ? BADGE_NONE : (peer.phase ? BADGE_IN : BADGE_OUT);
        const flags = [peer.presence ? miniFlag('Präsenz') : '', peer.window ? miniFlag('Fenster offen') : '',
                       peer.ac ? miniFlag('Klima an') : '', peer.nox ? miniFlag('NOx') : ''].join('');
        const seen = `zuletzt vor ${peer.age_s} s`;
        html += peerCard(`Gerät ${sanitizeHTML(String(peer.device_id))}${peer.master ? MASTER_TAG : ''} <span class="text-xs font-normal text-gray-500">${seen}</span>`, false,
            mode, peer.speed, badge, fmtNum(peer.rpm, 0, ""), fmtNum(peer.board_t, 1, " °C"), pct(peer.pid_demand), flags,
            peer.fresh ? 'bg-accent' : 'bg-warn', peer.fresh ? 'Aktuell' : 'Veraltet: ' + seen,
            airQualityBlock(peer.co2, peer.co2_rating, peer.room_t, peer.humidity));
      });
      document.getElementById('peers_container').innerHTML = html;
    }

    let retryPending = false;
    async function updateData() {
      try {
        const res = await fetch('/state');
        if (res.status === 503 && !retryPending) {
          // The device builds /state only while a client polls — ready within ~100 ms
          retryPending = true;
          setTimeout(() => { retryPending = false; updateData(); }, 300);
          return;
        }
        if (!res.ok) return;
        const data = await res.json();

        // Update direct text elements
        const ids = ["device_id", "floor_id", "room_id", "phase",
                     "temperature", "pressure", "outdoor_humidity", "temp_zuluft", "temp_abluft",
                     "heat_recovery_efficiency", "fan_rpm", "room_co2",
                     "room_temperature", "room_humidity", "filter_operating_days"];

        ids.forEach(id => {
          const el = document.getElementById("val_" + id);
          if (el && data[id] !== null && data[id] !== undefined) {
            let num = parseFloat(data[id]);
            let strVal = typeof data[id] === 'string' ? data[id] : String(data[id]);
            el.textContent = isNaN(num) ? strVal : (Number.isInteger(num) ? num : num.toFixed(1));
          }
        });

        document.getElementById("val_direction_display").textContent = data.direction_display || "--";

        // SGP41 board: label the VOC-based CO2 equivalent, show VOC / NOx index
        if (data.voc_index !== undefined) {
          document.getElementById("lbl_room_co2").textContent = "CO2-Äquivalent (VOC):";
          document.getElementById("voc_index_row").classList.remove("hidden");
          document.getElementById("val_voc_index").textContent =
            (data.voc_index === null) ? "--" : Math.round(data.voc_index);
        }
        if (data.nox_index !== undefined) {
          document.getElementById("nox_index_row").classList.remove("hidden");
          document.getElementById("val_nox_index").textContent =
            (data.nox_index === null) ? "--" : Math.round(data.nox_index);
        }

        document.getElementById("val_room_co2_bewertung").textContent = data.room_co2_bewertung || "--";

        // Binary sensors
        const alarmEl = document.getElementById("val_filter_change_alarm");
        alarmEl.innerText = data.filter_change_alarm ? "ALARM" : "OK";
        alarmEl.className = data.filter_change_alarm ? "font-bold text-danger text-lg" : "font-medium text-accent text-lg";

        const radarEl = document.getElementById("val_radar_presence");
        radarEl.innerText = data.radar_presence ? "Ja" : "Nein";
        radarEl.className = data.radar_presence ? "font-medium text-accent text-lg" : "font-medium text-gray-400 text-lg";

        // Mode select and intensity slider (not while the user edits them)
        const modeEl = document.getElementById("luefter_modus");
        if (document.activeElement !== modeEl && data.luefter_modus) modeEl.value = data.luefter_modus;
        const fanEl = document.getElementById("fan_intensity_display");
        if (document.activeElement !== fanEl && data.fan_intensity_display !== null) fanEl.value = data.fan_intensity_display;
        document.getElementById("label_fan_intensity").innerText = fanEl.value;

        // Smart-Automatik level override (room-wide, started by the slider or the Level button)
        const overrideHint = document.getElementById("override_hint");
        if (data.luefter_modus === 'Smart-Automatik') {
          const om = Number(data.auto_override_min);
          overrideHint.innerText = (om > 0)
            ? `Automatik übersteuert – noch ${om} min (Moduswechsel beendet)`
            : 'Stufe ändern übersteuert die Automatik (raumweit, zeitlich begrenzt)';
          overrideHint.className = (om > 0) ? 'text-xs text-accent' : 'text-xs text-gray-500';
        } else {
          overrideHint.innerText = '';
        }

        renderStatus(data);
        renderSystem(data);
        renderSettingValues(data);
        renderPeers(data);

        // Update Chart
        const now = new Date();
        const timeStr = now.getHours().toString().padStart(2, '0') + ':' +
                        now.getMinutes().toString().padStart(2, '0') + ':' +
                        now.getSeconds().toString().padStart(2, '0');

        chartData.labels.push(timeStr);
        chartData.datasets[0].data.push(data.fan_rpm === null ? null : parseFloat(data.fan_rpm));
        chartData.datasets[1].data.push(data.room_temperature === null ? null : parseFloat(data.room_temperature));
        chartData.datasets[2].data.push(data.room_co2 === null ? null : parseFloat(data.room_co2));
        chartData.datasets[3].data.push(data.room_humidity === null ? null : parseFloat(data.room_humidity));

        if (chartData.labels.length > maxHistoryPoints) {
            chartData.labels.shift();
            chartData.datasets.forEach(dataset => dataset.data.shift());
        }
        historyChart.update();

      } catch (e) {
        console.error('Error fetching state:', e);
      }
    }

    // Briefly outlines the control: accent = applied, danger = rejected
    function flash(el, ok) {
      if (!el) return;
      el.style.outline = '2px solid ' + (ok ? '#03dac6' : '#cf6679');
      setTimeout(() => { el.style.outline = ''; }, ok ? 800 : 2500);
    }

    // POST + X-VentoSync header: the device rejects /set without it (CSRF protection)
    async function sendSet(id, val, el) {
      const lim = META && META.numbers && META.numbers[id];
      if (lim && (isNaN(parseFloat(val)) || val < lim[0] || val > lim[1])) {
        console.warn(`Not sent: ${id} = ${val} outside ${lim[0]}–${lim[1]}`);
        flash(el, false);
        return;
      }
      if (id === 'fan_intensity_display') document.getElementById("label_fan_intensity").innerText = val;

      try {
        const res = await fetch('/set', {method: 'POST', headers: {'X-VentoSync': '1'},
                                         body: new URLSearchParams({id: id, val: String(val)})});
        if (!res.ok) console.warn('Setting not applied (HTTP ' + res.status + '):', id);
        flash(el, res.ok);
      } catch (e) {
        console.error('Error setting value:', e);
        flash(el, false);
      }
      setTimeout(updateData, 500); // refresh after delay
    }

    function filterReset() {
      if (confirm('Filter gewechselt? Der Betriebsstunden-Zähler wird auf 0 gesetzt.')) {
        sendSet('filter_reset', 1, document.getElementById('btn_filter_reset'));
      }
    }

    setInterval(updateData, 2000);
    loadMeta(); // fills limits / options, then the first updateData()
  </script>
</body>
</html>
)=====";
