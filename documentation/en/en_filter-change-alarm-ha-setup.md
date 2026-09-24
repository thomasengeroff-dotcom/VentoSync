# 🧹 Filter Change Alarm & Home Assistant Setup

[![Language: DE](https://img.shields.io/badge/Language-DE-red.svg)](../de/de_filter-change-alarm-ha-setup.md)


VentoSync features intelligent predictive filter monitoring to ensure optimal air throughput, motor longevity, and indoor air hygiene.

---

## ⚙️ Filter Monitoring Logic

The system automatically tracks the operating hours of the fan and triggers an alarm when:

- **Operating Hours > 365 days** (8760h runtime), or
- **Calendar Time > 3 years** since the last filter change.

**Available Entities:**

| Entity | Type | Description |
| --- | --- | --- |
| `binary_sensor.filterwechsel_alarm` | Binary Sensor | `ON` = filter change recommended |
| `sensor.filter_betriebstage` | Sensor | Fan runtime in days since last change |
| `button.filterwechsel_reset` | Button | Press after filter change → resets counter |

**Example: Push notification via HA Automation**

Add the following automation to your Home Assistant `automations.yaml`:

```yaml
automation:
  - alias: "Filter Change Notification"
    trigger:
      - platform: state
        entity_id: binary_sensor.ventosync_filterwechsel_alarm
        to: "on"
    action:
      - service: notify.mobile_app_<your_device>
        data:
          title: "🧹 Filter change recommended"
          message: >-
            The ventilation system has reached {{ states("sensor.esptest_filter_betriebstage") }} operating days
            since the last filter change. Please check and change filter.
          data:
            tag: "filter_change"
            importance: high
```

> 💡 **After the filter change:** Press the button `Filter changed (Reset)` in Home Assistant to reset the operating hours and the calendar timer.
