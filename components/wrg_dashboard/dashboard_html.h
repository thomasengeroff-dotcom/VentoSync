// ==========================================================================
// WRG Wohnraumlüftung – ESPHome Custom Component
// https://github.com/thomasengeroff-dotcom/ESPHome-Wohnraumlueftung
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
// Modified:    2026-03-22
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
      <h1 class="text-3xl font-bold tracking-tight text-white mb-8">
        WRG Lüftung Dashboard 
        <span class="text-sm font-normal text-gray-500 ml-2">(v2.0)</span>
      </h1>
    </header>
    
    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
      
      <!-- General Settings / Device Info -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Grundeinstellungen</h2>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Geräte-ID:</span> <span class="font-medium text-lg" id="val_device_id">--</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Aktuelle Floor ID:</span> <span class="font-medium text-lg" id="val_floor_id">--</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Raum (Room ID):</span> <span class="font-medium text-lg" id="val_room_id">--</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Geräte-Phase (A/B):</span> <span class="font-medium text-lg" id="val_phase">--</span></div>
      </div>

      <!-- Status & Sensors -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Aktuelle Sensordaten</h2>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Board-Temperatur:</span> <span class="font-medium text-lg" id="val_temperature">-- °C</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Luftdruck:</span> <span class="font-medium text-lg" id="val_pressure">-- hPa</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Feuchtigkeit (Außen):</span> <span class="font-medium text-lg" id="val_outdoor_humidity">-- %</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Temperatur Zuluft:</span> <span class="font-medium text-lg" id="val_temp_zuluft">-- °C</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Temperatur Abluft:</span> <span class="font-medium text-lg" id="val_temp_abluft">-- °C</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Effizienz WRG:</span> <span class="font-medium text-accent text-lg" id="val_heat_recovery_efficiency">-- %</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Lüfter RPM:</span> <span class="font-medium text-lg" id="val_fan_rpm">--</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Luftrichtung:</span> <span class="font-medium text-accent text-lg" id="val_direction_display">--</span></div>
      </div>

      <!-- Air Quality -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Luftqualität (SCD41)</h2>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">CO2:</span> <span class="font-medium text-lg" id="val_scd41_co2">-- ppm</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Bewertung:</span> <span class="font-medium text-lg" id="val_scd41_co2_bewertung">--</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Temperatur:</span> <span class="font-medium text-lg" id="val_scd41_temperature">-- °C</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Luftfeuchtigkeit:</span> <span class="font-medium text-lg" id="val_scd41_humidity">-- %</span></div>
      </div>

      <!-- Maintenance -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Wartung</h2>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Filter Betriebstage:</span> <span class="font-medium text-lg" id="val_filter_operating_days">--</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Filterwechsel Alarm:</span> <span class="font-medium text-lg" id="val_filter_change_alarm">--</span></div>
        <div class="flex justify-between items-center"><span class="text-gray-400 text-sm">Radar Präsenz:</span> <span class="font-medium text-lg" id="val_radar_presence">--</span></div>
      </div>

      <!-- Controls -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4 md:col-span-2 lg:col-span-3">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Einstellungen</h2>
        
        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mt-2">
          <div class="flex flex-col space-y-2">
            <span class="text-sm text-gray-400 font-medium">Lüfter Modus</span>
            <select id="luefter_modus" onchange="sendSet('luefter_modus', this.value)" class="bg-gray-800 border border-gray-700 text-white text-sm rounded-lg focus:ring-accent focus:border-accent block w-full p-2.5 transition-colors">
              <option value="Automatik">Automatik</option>
              <option value="Wärmerückgewinnung">Wärmerückgewinnung</option>
              <option value="Durchlüften">Durchlüften</option>
              <option value="Stoßlüftung">Stoßlüftung</option>
              <option value="Aus">Aus</option>
            </select>
          </div>

          <div class="flex flex-col space-y-2">
            <div class="flex justify-between"><span class="text-sm text-gray-400 font-medium">Lüfter Intensität</span><span id="label_fan_intensity" class="font-bold text-accent">--</span></div>
            <input type="range" id="fan_intensity_display" min="1" max="10" step="1" onchange="sendSet('fan_intensity_display', this.value)" class="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer mt-2">
          </div>

          <div class="flex justify-between items-center bg-gray-800/50 p-3 rounded-lg border border-gray-700/50">
            <span class="text-sm text-gray-300">Autom. Min Stufe</span>
            <input type="number" id="automatik_min_luefterstufe" onchange="sendSet('automatik_min_luefterstufe', this.value)" class="bg-gray-700 border border-gray-600 text-white text-sm rounded focus:ring-accent focus:border-accent block w-20 p-1.5 text-center">
          </div>

          <div class="flex justify-between items-center bg-gray-800/50 p-3 rounded-lg border border-gray-700/50">
            <span class="text-sm text-gray-300">Autom. Max Stufe</span>
            <input type="number" id="automatik_max_luefterstufe" onchange="sendSet('automatik_max_luefterstufe', this.value)" class="bg-gray-700 border border-gray-600 text-white text-sm rounded focus:ring-accent focus:border-accent block w-20 p-1.5 text-center">
          </div>

          <div class="flex justify-between items-center bg-gray-800/50 p-3 rounded-lg border border-gray-700/50">
            <span class="text-sm text-gray-300">CO2 Schwellwert</span>
            <input type="number" id="auto_co2_threshold" onchange="sendSet('auto_co2_threshold', this.value)" class="bg-gray-700 border border-gray-600 text-white text-sm rounded focus:ring-accent focus:border-accent block w-20 p-1.5 text-center">
          </div>
          
          <div class="flex justify-between items-center bg-gray-800/50 p-3 rounded-lg border border-gray-700/50">
            <span class="text-sm text-gray-300">Feuchte Schwellwert</span>
            <input type="number" id="auto_humidity_threshold" onchange="sendSet('auto_humidity_threshold', this.value)" class="bg-gray-700 border border-gray-600 text-white text-sm rounded focus:ring-accent focus:border-accent block w-20 p-1.5 text-center">
          </div>

          <div class="flex justify-between items-center bg-gray-800/50 p-3 rounded-lg border border-gray-700/50 lg:col-span-2">
            <span class="text-sm text-gray-300">Anwesenheit Anpassung</span>
            <input type="number" id="auto_presence_slider" onchange="sendSet('auto_presence_slider', this.value)" class="bg-gray-700 border border-gray-600 text-white text-sm rounded focus:ring-accent focus:border-accent block w-20 p-1.5 text-center">
          </div>
        </div>
      </div>

      <!-- Verbundene Geräte (ESP-NOW) -->
      <div class="bg-card rounded-xl p-5 shadow-lg border border-gray-800 flex flex-col space-y-4 md:col-span-2 lg:col-span-3 hidden" id="peers_card">
        <h2 class="text-xl font-semibold text-white border-b border-gray-700 pb-2 mb-2">Verbundene Geräte (ESP-NOW)</h2>
        <div id="peers_container" class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4 mt-2"></div>
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

    async function updateData() {
      try {
        const res = await fetch('/state');
        if (!res.ok) return;
        const data = await res.json();
        
        // Update direct text elements
        const ids = ["device_id", "floor_id", "room_id", "phase", 
                     "temperature", "pressure", "outdoor_humidity", "temp_zuluft", "temp_abluft", 
                     "heat_recovery_efficiency", "fan_rpm", "direction_display", "scd41_co2", "scd41_co2_bewertung", 
                     "scd41_temperature", "scd41_humidity", "filter_operating_days"];
        
        ids.forEach(id => {
          const el = document.getElementById("val_" + id);
          if (el && data[id] !== null) {
            let num = parseFloat(data[id]);
            let strVal = typeof data[id] === 'string' ? data[id] : String(data[id]);
            el.textContent = isNaN(num) ? sanitizeHTML(strVal) : (Number.isInteger(num) ? num : num.toFixed(1));
          }
        });

        // Binary sensors
        const alarmEl = document.getElementById("val_filter_change_alarm");
        if (alarmEl) {
          alarmEl.innerText = data.filter_change_alarm ? "ALARM" : "OK";
          alarmEl.className = data.filter_change_alarm ? "font-bold text-danger text-lg" : "font-medium text-accent text-lg";
        }
        
        const radarEl = document.getElementById("val_radar_presence");
        if (radarEl) {
          radarEl.innerText = data.radar_presence ? "Ja" : "Nein";
          radarEl.className = data.radar_presence ? "font-medium text-accent text-lg" : "font-medium text-gray-400 text-lg";
        }

        // Inputs / Selects (only update if not currently focused)
        if (document.activeElement.id !== "luefter_modus" && data.luefter_modus) {
          document.getElementById("luefter_modus").value = data.luefter_modus;
        }
        
        const uiControls = ['fan_intensity_display', 'automatik_min_luefterstufe', 'automatik_max_luefterstufe', 'auto_co2_threshold', 'auto_humidity_threshold', 'auto_presence_slider'];
        uiControls.forEach(ctrl => {
           if (document.activeElement.id !== ctrl && data[ctrl] !== null) {
              document.getElementById(ctrl).value = data[ctrl];
           }
        });

        document.getElementById("label_fan_intensity").innerText = document.getElementById("fan_intensity_display").value;
        
        // Render ESP-NOW Peers
        document.getElementById('peers_card').classList.remove('hidden');
        const container = document.getElementById('peers_container');
        
        let localPhaseBadge = "<span class='text-gray-400 font-bold px-2 py-0.5 bg-gray-700/50 rounded-full text-xs'>--</span>";
        if (data.direction_display && data.direction_display.includes("Zuluft")) {
            localPhaseBadge = "<span class='text-accent font-bold px-2 py-0.5 bg-accent/10 rounded-full text-xs'>IN</span>";
        } else if (data.direction_display && data.direction_display.includes("Abluft")) {
            localPhaseBadge = "<span class='text-danger font-bold px-2 py-0.5 bg-danger/10 rounded-full text-xs'>OUT</span>";
        }
        
        const localRPM = (data.fan_rpm !== null && data.fan_rpm !== undefined && !isNaN(data.fan_rpm)) ? Number(data.fan_rpm).toFixed(0) : "--";
        const localBoardT = (data.temperature !== null && data.temperature !== undefined && !isNaN(data.temperature)) ? Number(data.temperature).toFixed(1) + " °C" : "--";
        const localRoomT = (data.room_temp !== null && data.room_temp !== undefined && !isNaN(data.room_temp)) ? Number(data.room_temp).toFixed(1) + " °C" : "--";
        const localPID = (data.pid_demand !== null && data.pid_demand !== undefined && !isNaN(data.pid_demand)) ? (Math.round(data.pid_demand * 100) + "%") : "--";
        const localMode = data.luefter_modus === 'Wärmerückgewinnung' ? 'WRG' : (data.luefter_modus || '--');

        let html = `<div class="bg-gray-700/50 rounded-lg p-4 border border-gray-600">
            <div class="font-bold text-gray-300 mb-3 pb-2 border-b border-gray-600 flex justify-between items-center">
              <span>Gerät ${sanitizeHTML(String(data.device_id || "--"))} (lokales Gerät)</span>
              <span class="w-2 h-2 rounded-full bg-gray-400"></span>
            </div>
            <div class="flex justify-between text-sm mb-2">
                <span class="text-gray-400">Modus: <strong class="text-gray-200">${sanitizeHTML(localMode)}</strong></span>
                <span class="flex items-center gap-2">Stufe: <strong class="text-gray-200">${sanitizeHTML(String(data.fan_intensity_display || "--"))}</strong> ${localPhaseBadge}</span>
            </div>
            <div class="grid grid-cols-2 gap-x-4 gap-y-1 text-xs text-gray-500 bg-black/20 p-2 rounded mt-3">
                <div class="flex justify-between"><span>Lüfter RPM:</span> <strong class="text-gray-400">${localRPM}</strong></div>
                <div class="flex justify-between"><span>Board-temp:</span> <strong class="text-gray-400">${localBoardT}</strong></div>
                <div class="flex justify-between"><span>Raum-temp:</span> <strong class="text-gray-400">${localRoomT}</strong></div>
                <div class="flex justify-between"><span>PID:</span> <strong class="text-gray-400">${localPID}</strong></div>
            </div>
        </div>`;

        if (data.peers && data.peers.length > 0) {
          data.peers.forEach(peer => {
            const modeNames = ["Aus", "WRG", "Durchlüften", "Stoßlüftung"];
            const mode = peer.mode >= 0 && peer.mode <= 3 ? modeNames[peer.mode] : "Unbekannt";
            const phase = peer.phase ? "<span class='text-accent font-bold px-2 py-0.5 bg-accent/10 rounded-full text-xs'>IN</span>" : "<span class='text-danger font-bold px-2 py-0.5 bg-danger/10 rounded-full text-xs'>OUT</span>";
            
            const rpm = (peer.rpm !== undefined && peer.rpm !== null && !isNaN(peer.rpm)) ? Number(peer.rpm).toFixed(0) : "--";
            const boardT = (peer.board_t !== undefined && peer.board_t !== null && !isNaN(peer.board_t)) ? Number(peer.board_t).toFixed(1) + " °C" : "--";
            const roomT = (peer.room_t !== undefined && peer.room_t !== null && !isNaN(peer.room_t)) ? Number(peer.room_t).toFixed(1) + " °C" : "--";
            const pid = (peer.pid_demand !== undefined && peer.pid_demand !== null && !isNaN(peer.pid_demand)) ? (Math.round(peer.pid_demand * 100) + "%") : "--";
            
            html += `<div class="bg-gray-800/80 rounded-lg p-4 border border-gray-700 hover:border-gray-600 transition-colors">
                <div class="font-bold text-white mb-3 pb-2 border-b border-gray-700 flex justify-between items-center">
                  <span>Gerät ${sanitizeHTML(String(peer.device_id))}</span>
                  <span class="w-2 h-2 rounded-full bg-accent"></span>
                </div>
                <div class="flex justify-between text-sm mb-2">
                    <span class="text-gray-400">Modus: <strong class="text-gray-200">${sanitizeHTML(mode)}</strong></span>
                    <span class="flex items-center gap-2">Stufe: <strong class="text-gray-200">${sanitizeHTML(String(peer.speed))}</strong> ${phase}</span>
                </div>
                <div class="grid grid-cols-2 gap-x-4 gap-y-1 text-xs text-gray-500 bg-black/20 p-2 rounded mt-3">
                    <div class="flex justify-between"><span>Lüfter RPM:</span> <strong class="text-gray-400">${rpm}</strong></div>
                    <div class="flex justify-between"><span>Board-temp:</span> <strong class="text-gray-400">${boardT}</strong></div>
                    <div class="flex justify-between"><span>Raum-temp:</span> <strong class="text-gray-400">${roomT}</strong></div>
                    <div class="flex justify-between"><span>PID:</span> <strong class="text-accent/80">${pid}</strong></div>
                </div>
            </div>`;
          });
        }
        container.innerHTML = html;
        
        // Update Chart
        const now = new Date();
        const timeStr = now.getHours().toString().padStart(2, '0') + ':' + 
                        now.getMinutes().toString().padStart(2, '0') + ':' + 
                        now.getSeconds().toString().padStart(2, '0');

        chartData.labels.push(timeStr);
        chartData.datasets[0].data.push(data.fan_rpm === null ? null : parseFloat(data.fan_rpm));
        chartData.datasets[1].data.push(data.scd41_temperature === null ? null : parseFloat(data.scd41_temperature));
        chartData.datasets[2].data.push(data.scd41_co2 === null ? null : parseFloat(data.scd41_co2));
        chartData.datasets[3].data.push(data.scd41_humidity === null ? null : parseFloat(data.scd41_humidity));

        if (chartData.labels.length > maxHistoryPoints) {
            chartData.labels.shift();
            chartData.datasets.forEach(dataset => dataset.data.shift());
        }
        historyChart.update();

      } catch (e) {
        console.error('Error fetching state:', e);
      }
    }

    async function sendSet(id, val) {
      const allowedKeys = ['luefter_modus', 'fan_intensity_display', 'automatik_min_luefterstufe', 
                          'automatik_max_luefterstufe', 'auto_co2_threshold', 'auto_humidity_threshold',
                          'auto_presence_slider', 'vent_timer', 'sync_interval_config'];

      if (!allowedKeys.includes(id)) {
          console.error('Client validation failed: Invalid parameter =', id);
          return;
      }
      if (id === 'fan_intensity_display' && (val < 1 || val > 10)) {
          console.error('Client validation failed: Invalid fan intensity =', val);
          return;
      }

      if (id === 'fan_intensity_display') document.getElementById("label_fan_intensity").innerText = val;

      try {
        await fetch(`/set?id=${encodeURIComponent(id)}&val=${encodeURIComponent(val)}`);
      } catch (e) {
        console.error('Error setting value:', e);
      }
      setTimeout(updateData, 500); // refresh after delay
    }

    setInterval(updateData, 2000);
    updateData(); // initial call
  </script>
</body>
</html>
)=====";
