#pragma once

const char DASHBOARD_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Wohnraumlüftung Dashboard</title>
  <style>
    :root {
      --bg-color: #121212;
      --card-bg: #1e1e1e;
      --text-color: #ededed;
      --accent: #03dac6;
      --accent-variant: #3700b3;
      --danger: #cf6679;
    }
    body {
      background-color: var(--bg-color);
      color: var(--text-color);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      margin: 0; padding: 20px;
      display: flex; flex-direction: column; align-items: center;
    }
    .container {
      max-width: 900px;
      width: 100%;
    }
    h1 { text-align: center; }
    .grid {
      display: display: grid;
      grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      gap: 20px;
      margin-top: 20px;
    }
    .card {
      background: var(--card-bg);
      border-radius: 8px;
      padding: 20px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }
    .card h2 { margin-top: 0; font-size: 1.2rem; border-bottom: 1px solid #333; padding-bottom: 10px; }
    .item {
      display: flex; justify-content: space-between; align-items: center;
      margin: 15px 0;
    }
    .value { font-weight: bold; font-size: 1.1rem; }
    select, input[type=range], input[type=number] {
      background: #333; border: 1px solid #555; color: white; padding: 8px; border-radius: 4px;
    }
    .btn {
      background: var(--accent); color: #000; padding: 8px 12px; border: none; border-radius: 4px; cursor: pointer; font-weight: bold;
    }
    .btn:hover { background: #01b4a0; }
  </style>
</head>
<body>
  <div class="container">
    <h1>WRG Lüftung Dashboard</h1>
    
    <div class="grid">
      <!-- Status & Sensors -->
      <div class="card">
        <h2>Aktuelle Sensordaten</h2>
        <div class="item"><span>Raumtemperatur:</span> <span class="value" id="val_temperature">-- °C</span></div>
        <div class="item"><span>Luftdruck:</span> <span class="value" id="val_pressure">-- hPa</span></div>
        <div class="item"><span>Feuchtigkeit (Außen):</span> <span class="value" id="val_outdoor_humidity">-- %</span></div>
        <div class="item"><span>Temperatur Zuluft:</span> <span class="value" id="val_temp_zuluft">-- °C</span></div>
        <div class="item"><span>Temperatur Abluft:</span> <span class="value" id="val_temp_abluft">-- °C</span></div>
        <div class="item"><span>Effizienz WRG:</span> <span class="value" id="val_heat_recovery_efficiency">-- %</span></div>
        <div class="item"><span>Lüfter RPM:</span> <span class="value" id="val_fan_rpm">--</span></div>
      </div>

      <!-- Air Quality -->
      <div class="card">
        <h2>Luftqualität (SCD41)</h2>
        <div class="item"><span>CO2:</span> <span class="value" id="val_scd41_co2">-- ppm</span></div>
        <div class="item"><span>Bewertung:</span> <span class="value" id="val_scd41_co2_bewertung">--</span></div>
        <div class="item"><span>Temperatur:</span> <span class="value" id="val_scd41_temperature">-- °C</span></div>
        <div class="item"><span>Luftfeuchtigkeit:</span> <span class="value" id="val_scd41_humidity">-- %</span></div>
      </div>

      <!-- Maintenance -->
      <div class="card">
        <h2>Wartung</h2>
        <div class="item"><span>Filter Betriebstage:</span> <span class="value" id="val_filter_operating_days">--</span></div>
        <div class="item"><span>Filterwechsel Alarm:</span> <span class="value" id="val_filter_change_alarm">--</span></div>
        <div class="item"><span>Radar Präsenz:</span> <span class="value" id="val_radar_presence">--</span></div>
      </div>

      <!-- Controls -->
      <div class="card">
        <h2>Einstellungen</h2>
        
        <div class="item">
          <span>Lüfter Modus</span>
          <select id="luefter_modus" onchange="sendSet('luefter_modus', this.value)">
            <option value="Wärmerückgewinnung">Wärmerückgewinnung</option>
            <option value="Stoßlüftung">Stoßlüftung</option>
            <option value="Durchlüften">Durchlüften</option>
            <option value="Automatik">Automatik</option>
            <option value="Aus">Aus</option>
          </select>
        </div>

        <div class="item">
          <span>Lüfter Intensität</span>
          <input type="range" id="fan_intensity_display" min="1" max="10" step="1" onchange="sendSet('fan_intensity_display', this.value)">
          <span id="label_fan_intensity">--</span>
        </div>

        <div class="item">
          <span>Lüfter Speed (%)</span>
          <input type="range" id="test_speed_slider" min="0" max="100" step="1" onchange="sendSet('test_speed_slider', this.value)">
          <span id="label_speed">--</span>
        </div>

        <div class="item">
          <span>Autom. Min Stufe</span>
          <input type="number" id="automatik_min_luefterstufe" style="width: 60px" onchange="sendSet('automatik_min_luefterstufe', this.value)">
        </div>

        <div class="item">
          <span>Autom. Max Stufe</span>
          <input type="number" id="automatik_max_luefterstufe" style="width: 60px" onchange="sendSet('automatik_max_luefterstufe', this.value)">
        </div>

        <div class="item">
          <span>CO2 Schwellwert</span>
          <input type="number" id="auto_co2_threshold" style="width: 80px" onchange="sendSet('auto_co2_threshold', this.value)">
        </div>
        
        <div class="item">
          <span>Feuchte Schwellwert</span>
          <input type="number" id="auto_humidity_threshold" style="width: 80px" onchange="sendSet('auto_humidity_threshold', this.value)">
        </div>

        <div class="item">
          <span>Anwesenheit Anpassung</span>
          <input type="number" id="auto_presence_slider" style="width: 60px" onchange="sendSet('auto_presence_slider', this.value)">
        </div>

      </div>
    </div>
  </div>

  <script>
    async function updateData() {
      try {
        const res = await fetch('/state');
        if (!res.ok) return;
        const data = await res.json();
        
        // Update direct text elements
        const ids = ["temperature", "pressure", "outdoor_humidity", "temp_zuluft", "temp_abluft", 
                     "heat_recovery_efficiency", "fan_rpm", "scd41_co2", "scd41_co2_bewertung", 
                     "scd41_temperature", "scd41_humidity", "filter_operating_days"];
        
        ids.forEach(id => {
          const el = document.getElementById("val_" + id);
          if (el && data[id] !== null) {
            let num = parseFloat(data[id]);
            el.innerText = isNaN(num) ? data[id] : (Number.isInteger(num) ? num : num.toFixed(1));
          }
        });

        // Binary sensors
        document.getElementById("val_filter_change_alarm").innerText = data.filter_change_alarm ? "ALARM" : "OK";
        document.getElementById("val_filter_change_alarm").style.color = data.filter_change_alarm ? "red" : "lime";
        document.getElementById("val_radar_presence").innerText = data.radar_presence ? "Ja" : "Nein";

        // Inputs / Selects (only update if not currently focused)
        if (document.activeElement.id !== "luefter_modus" && data.luefter_modus) {
          document.getElementById("luefter_modus").value = data.luefter_modus;
        }
        
        const uiControls = ['fan_intensity_display', 'test_speed_slider', 'automatik_min_luefterstufe', 'automatik_max_luefterstufe', 'auto_co2_threshold', 'auto_humidity_threshold', 'auto_presence_slider'];
        uiControls.forEach(ctrl => {
           if (document.activeElement.id !== ctrl && data[ctrl] !== null) {
              document.getElementById(ctrl).value = data[ctrl];
           }
        });

        document.getElementById("label_fan_intensity").innerText = document.getElementById("fan_intensity_display").value;
        document.getElementById("label_speed").innerText = document.getElementById("test_speed_slider").value;
        
      } catch (e) {
        console.error('Error fetching state:', e);
      }
    }

    async function sendSet(id, val) {
      if (id === 'test_speed_slider') document.getElementById("label_speed").innerText = val;
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

