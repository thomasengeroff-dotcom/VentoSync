
  > **⚙️ Voraussetzung für das Feuchte-Management: `sensor.outdoor_humidity` in Home Assistant**
  >
  > Das Feuchte-Management benötigt eine Außenluftfeuchtigkeit aus Home Assistant. Der ESPHome-Code erwartet die Entity-ID `sensor.outdoor_humidity` (konfiguriert in `sensors_climate.yaml`). Es gibt zwei Wege, diesen Sensor bereitzustellen:
  >
  > **Option A: Wetterdienst-Integration** (kein zusätzlicher Sensor nötig)
  >
  > Installiere eine Wetter-Integration (z.B. [OpenWeatherMap](https://www.home-assistant.io/integrations/openweathermap/), [Met.no](https://www.home-assistant.io/integrations/met/)) und erstelle in `configuration.yaml` einen Template-Sensor:
  >
  > ```yaml
  > template:
  >   - sensor:
  >       - name: "Outdoor Humidity"
  >         unique_id: outdoor_humidity
  >         unit_of_measurement: "%"
  >         state: "{{ state_attr('weather.home', 'humidity') }}"
  >         device_class: humidity
  > ```
  >
  > **Option B: Physischer Außensensor** (z.B. ESP32 + BME280/SHT31 auf Balkon/Terrasse)
  >
  > Wenn der Außensensor bereits als HA-Entity existiert (z.B. `sensor.balkon_sht31_humidity`), erstelle einen Alias:
  >
  > ```yaml
  > template:
  >   - sensor:
  >       - name: "Outdoor Humidity"
  >         unique_id: outdoor_humidity
  >         unit_of_measurement: "%"
  >         state: "{{ states('sensor.balkon_sht31_humidity') }}"
  >         device_class: humidity
  > ```
  >
  > **Alternativ:** Ändere direkt die `entity_id` in `sensors_climate.yaml` auf deinen Sensor:
  >
  > ```yaml
  > entity_id: sensor.balkon_sht31_humidity  # statt sensor.outdoor_humidity
  > ```
  >
  > 💡 **Ohne `sensor.outdoor_humidity`** funktioniert die Entfeuchtung trotzdem — der Outdoor-Check wird dann übersprungen und der PID regelt rein nach dem Innenfeuchte-Grenzwert.
