#!/bin/bash
# OTA Upload auf alle Lüftungsgeräte (Wohnraumlüftung)

# Damit das Skript sofort abbricht, falls ein Upload hart fehlschlägt:
set -e

echo ""
echo "=================================================="
echo "🚀 Checking yaml config ventosync.yaml..."
echo "=================================================="
esphome config ventosync.yaml

echo ""
echo "=================================================="
echo "🚀 Compiling ventosync.yaml..."
echo "=================================================="
esphome compile ventosync.yaml

#echo "=================================================="
#echo "🚀 Starte Upload auf Gerät WRG 12V Test (192.168.178.225)..."
#echo "=================================================="
#esphome upload ventosync.yaml --device 192.168.178.225

echo ""
echo "=================================================="
echo "🚀 Starte Upload auf Gerät WRG Büro (192.168.178.206)..."
echo "=================================================="
esphome upload ventosync.yaml --device 192.168.178.206

echo ""
echo "=================================================="
echo "🚀 Starte Upload auf Gerät WRG DG Wohnraum (192.168.178.244)..."
echo "=================================================="
esphome upload ventosync.yaml --device 192.168.178.244

echo ""
echo "=================================================="
echo "🚀 Starte Upload auf Gerät ESP Test (192.168.178.220)..."
echo "=================================================="
esphome upload ventosync.yaml --device 192.168.178.220

echo ""
echo "✅ Firmware erfolgreich kompiliert und auf alle Geräte hochgeladen!"
