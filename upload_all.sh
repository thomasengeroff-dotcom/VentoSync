#!/bin/bash
# OTA Upload auf alle Lüftungsgeräte (Wohnraumlüftung)

# Damit das Skript sofort abbricht, falls ein Upload hart fehlschlägt:
set -e

echo "=================================================="
echo "🚀 Starte Upload auf Gerät WRG 12V Test (192.168.178.225)..."
echo "=================================================="
esphome upload ventosync.yaml --device 192.168.178.225

echo ""
echo "=================================================="
echo "🚀 Starte Upload auf Gerät WRG Büro (192.168.178.237)..."
echo "=================================================="
esphome upload ventosync.yaml --device 192.168.178.237

echo ""
echo "=================================================="
echo "🚀 Starte Upload auf Gerät WRG DG Wohnraum (192.168.178.244)..."
echo "=================================================="
esphome upload ventosync.yaml --device 192.168.178.244

echo ""
echo "✅ Alle Firmware-Uploads wurden erfolgreich abgeschlossen!"
