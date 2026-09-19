# 🖥️ Bedienung am Lüftungsgerät (VentoMaxx V-WRG-1)

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_control-panel-operation.md)


Um ein intuitives und nahtloses Bedienerlebnis zu gewährleisten, nutzt VentoSync das originale Bedienpanel des VentoMaxx V-WRG-1 (14-Pin FFC-Schnittstelle über MCP23017 und PCA9685) und erweitert dessen Funktionalität deutlich.

<p align="center">
  <img src="../../images/Ventomax%20V-WRG-1/PXL_20260128_232625674.jpg" alt="VentoSync Bedienpanel im Gehäuse" width="500" />
</p>

---

## 🔘 Tastenbelegung & Funktionen

Das Bedienfeld verfügt über 3 Taster:

| Taste | Funktion | Bedienung / Aktion |
| :--- | :--- | :--- |
| **Power (I/O)** | System Ein / Aus / Sleep | • **Kurzer Druck**: Schaltet die Lüftung EIN / AUS (AUS stoppt den Lüfter per 50% PWM; Gerät bleibt im Sensor-Monitoring-Modus online; EIN stellt den vorherigen Modus wieder her).<br>• **Langer Druck (> 5s)**: Versetzt das Gerät in den **Light Sleep Modus** (schaltet Lüfter, LEDs und WLAN-Funk ab, um maximal Strom zu sparen).<br>• **Sehr langer Druck (> 10s)**: Startet den ESP32-Mikrocontroller neu (Reboot). |
| **Modus (M)** | Betriebsmodus | • **Kurzer Druck**: Zykliert durch die Betriebsmodi: **Automatik → Wärmerückgewinnung → Durchlüften → Stoßlüftung → Aus → Automatik...** |
| **Stufe (+)** | Lüfterintensität | • **Kurzer Druck**: Zykliert durch 10 Geschwindigkeitsstufen.<br>• **Gedrückt halten**: Automatisches, flüssiges Hoch- und Runterschalten durch alle 10 Stufen (1 Stufe/s), bis die Taste losgelassen wird. |

---

## 🔆 Status-LEDs & Visuelles Feedback

Das Panel besitzt 9 grüne LEDs für die Systemzustandsanzeige:

| LED | Anzahl | Position | Verhalten |
| :--- | :---: | :--- | :--- |
| **Power** | 🟢 1x | Oben | Leuchtet im Normalbetrieb hell. Dimmt nach 30s Inaktivität auf 20% Resthelligkeit ab. |
| **Master** | 🟢 1x | Mitte | Leuchtet gedimmt auf dem Master-Gerät (Geräte-ID = 1). Signalisiert Störungen und Sonderzustände über Blinkmuster (siehe unten). |
| **Modus L** (`LED_WRG`) | 🟢 1x | Links | **Pulsiert langsam** im Smart-Automatik-Modus. Leuchtet dauerhaft bei Wärmerückgewinnung und Durchlüften. |
| **Modus R** (`LED_VEN`) | 🟢 1x | Rechts | Leuchtet dauerhaft bei Stoßlüftung und Durchlüften. |
| **Intensität (1–5)** | 🟢 5x | Balken | Zeigt die aktuelle Lüfterstufe 1–10 über halbe (50%) und volle (100%) LED-Helligkeit an. |

---

### 📊 Modus-LED Matrix

| Modus | `LED_WRG` (links) | `LED_VEN` (rechts) |
| :--- | :---: | :---: |
| **Smart Automatik (Standard)** | 🟢 (pulsiert langsam) | ⚫ |
| **Wärmerückgewinnung (Eco)** | 🟢 (dauerhaft an) | ⚫ |
| **Stoßlüftung (Boost)** | ⚫ | 🟢 (dauerhaft an) |
| **Durchlüften (Sommer-Querlüftung)** | 🟢 (dauerhaft an) | 🟢 (dauerhaft an) |
| **Aus / Standby** | ⚫ | ⚫ |

---

### 📶 Intensitäts-Balkenanzeige (10 Stufen über 5 LEDs)

Jede LED deckt genau 2 Lüftungsstufen ab (50% Helligkeit bei ungerader Stufe, 100% bei gerader Stufe):

* **Stufe 1**:  ◖ ◯ ◯ ◯ ◯  *(LED 1 @ 50%)*
* **Stufe 2**:  ⬤ ◯ ◯ ◯ ◯  *(LED 1 @ 100%)*
* **Stufe 3**:  ⬤ ◖ ◯ ◯ ◯  *(LED 2 startet @ 50%)*
* **Stufe 4**:  ⬤ ⬤ ◯ ◯ ◯  *(LED 2 @ 100%)*
* **Stufe 5**:  ⬤ ⬤ ◖ ◯ ◯  *(LED 3 startet @ 50%)*
* **Stufe 6**:  ⬤ ⬤ ⬤ ◯ ◯  *(LED 3 @ 100%)*
* **Stufe 7**:  ⬤ ⬤ ⬤ ◖ ◯  *(LED 4 startet @ 50%)*
* **Stufe 8**:  ⬤ ⬤ ⬤ ⬤ ◯  *(LED 4 @ 100%)*
* **Stufe 9**:  ⬤ ⬤ ⬤ ⬤ ◖  *(LED 5 startet @ 50%)*
* **Stufe 10**: ⬤ ⬤ ⬤ ⬤ ⬤  *(LED 5 @ 100%)*

---

## 🚨 Diagnose-Blinkcodes (Master-LED)

Tritt eine Störung oder ein Sonderzustand auf, signalisiert die mittlere **Master-LED** dies über ein wiederkehrendes Pulsmuster:

| Muster | Bedeutung / Störung | Beschreibung & Verhalten |
| :--- | :--- | :--- |
| **2x Blinken** | Sync-Fehler | Synchronisierungs-Verlust zwischen Partnergeräten im Raum (> 3 min keine ESP-NOW-Pakete). *(Nur aktiv, wenn Peer-Überwachung aktiv ist.)* |
| **3x Blinken** | WLAN-Verlust | Verbindung zum WLAN-Router unterbrochen (> 30s dauerhaft, um Roaming-Drops zu unterdrücken). |
| **4x Blinken** | Hitzewarnung | Gehäuse-Innentemperatur kritisch (50–60°C). Automatische Sicherheitsabschaltung greift ab > 60°C. |
| **Langsamer Puls** *(1s An, 2s Aus)* | Fenstersperre aktiv | Fenster im Raum geöffnet, Lüfter pausiert. Puls stoppt nach 35s automatisch zur Vermeidung nächtlicher Lichtstörung. |

---

## ✨ Gruppen-Synchronisation & Auto-Dimming

* **Echtzeit Wake-Up Effekt**: Ändert ein Gerät im Raum den Modus oder die Stufe, wachen die LEDs aller Partner-Geräte (Peers) im Raum sofort auf und zeigen den neuen Status für 30 Sekunden synchron an.
* **30 Sekunden Auto-Dimming**: Alle Status-LEDs blenden 30 Sekunden nach der letzten Bedienung sanft aus (`ui_timeout_script` in `packages/ui/ui_controls.yaml`). Die **Power-LED** bleibt auf 20% Resthelligkeit gedimmt, um Betriebsbereitschaft diskret anzuzeigen.
