> **⚙️ Prerequisite for Humidity Management: `sensor.outdoor_humidity` in Home Assistant**
>
> Humidity management requires the outdoor air humidity from Home Assistant. The ESPHome code expects the entity ID `sensor.outdoor_humidity` (configured in `sensors_climate.yaml`). There are two ways to provide this sensor:
>
> **Option A: Weather Service Integration** (no additional sensor needed)
>
> Install a weather integration (e.g., [OpenWeatherMap](https://www.home-assistant.io/integrations/openweathermap/), [Met.no](https://www.home-assistant.io/integrations/met/)) and create a template sensor in `configuration.yaml`:
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
> **Option B: Physical Outdoor Sensor** (e.g., ESP32 + BME280/SHT31 on balcony/terrace)
>
> If the outdoor sensor already exists as an HA entity (e.g., `sensor.balcony_sht31_humidity`), create an alias:
>
> ```yaml
> template:
>   - sensor:
>       - name: "Outdoor Humidity"
>         unique_id: outdoor_humidity
>         unit_of_measurement: "%"
>         state: "{{ states('sensor.balcony_sht31_humidity') }}"
>         device_class: humidity
> ```
>
> **Alternatively:** Change the `entity_id` directly in `sensors_climate.yaml` to your sensor:
>
> ```yaml
> entity_id: sensor.balcony_sht31_humidity  # instead of sensor.outdoor_humidity
> ```
>
> 💡 **Without `sensor.outdoor_humidity`**, dehumidification still works — the outdoor check is then skipped and the PID regulates purely according to the indoor humidity limit.
