#!/bin/bash
# OTA Upload auf alle Lüftungsgeräte (Wohnraumlüftung)

# Damit das Skript sofort abbricht, falls ein Upload hart fehlschlägt:
set -e

echo ""
echo "=================================================="
echo "🚀 Checking yaml configs..."
echo "=================================================="
esphome config ventosync_bme680_only.yaml
esphome config ventosync_nosensor.yaml
esphome config ventosync_radar_only.yaml

echo ""
echo "=================================================="
echo "🚀 Compiling firmwares..."
echo "=================================================="
# esphome compile ventosync.yaml
# esphome compile ventosync_bme680_only.yaml
# esphome compile ventosync_nosensor.yaml
# esphome compile ventosync_radar_only.yaml

echo "=================================================="
echo "🚀 Starte Upload auf Gerät WRG 12V Test (192.168.178.225)..."
echo "device has BME680 and no SCD41 and no LD2450"
echo "=================================================="
esphome upload ventosync_bme680_only.yaml --device 192.168.178.225

echo ""
echo "=================================================="
echo "🚀 Starte Upload auf Gerät WRG Büro (192.168.178.206)..."
echo "device has no sensors"
echo "=================================================="
esphome upload ventosync_nosensor.yaml --device 192.168.178.206

echo ""
echo "=================================================="
echo "🚀 Starte Upload auf Gerät WRG DG Wohnraum (192.168.178.244)..."
echo "device has LD2450 and no SCD41 and no BME680"
echo "=================================================="
esphome upload ventosync_radar_only.yaml --device 192.168.178.244

echo ""
echo "=================================================="
echo "🚀 Starte Upload auf Gerät WRG DG Flur (192.168.178.220)..."
echo "device has no sensors"
echo "=================================================="
esphome upload ventosync_nosensor.yaml --device 192.168.178.220

echo ""
echo "✅ Firmware erfolgreich kompiliert und auf alle Geräte hochgeladen!"
