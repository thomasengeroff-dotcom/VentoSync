# 📊 VentoSync Lokales Web-Dashboard

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_local-web-dashboard.md)


Das **VentoSync Lokale Web-Dashboard** ist eine eigenständige, browserbasierte Steuer- und Diagnose-Oberfläche, die direkt vom ESP32-C6 Mikrocontroller bereitgestellt wird. Sie ermöglicht die volle Steuerung, Live-Messung und Konfiguration ganz ohne Home Assistant oder externe Server.

---

## ✨ Hauptfunktionen

* **Modernes responsives Design**: High-End Dark-Mode UI basierend auf **Tailwind CSS**, optimiert für Smartphones, Tablets und Desktop-PCs.
* **Automatik-Status-Banner** *(0.10.48)*: Die oberste Kachel zeigt, was die Lüftung gerade tut und warum — derselbe Text wie der HA-Sensor `Automatik-Status` (z. B. „CO2 erhöht: 1150 ppm (Soll 1000) → Stufe 5“), mit farbigem Rand je Grund (`automatik_grund`: rot = CO2-Notfall / Schimmelschutz / NOx, gelb = Fenster / Urlaub / Übersteuerung / keine Daten, türkis = CO2- / Feuchte-Bedarf). Darunter zeigen Chips die aktiven Raum-Eingänge: Urlaubsmodus, Fenster offen (pausiert oder Sperre ignoriert), NOx-Stoßlüftung, Stufen-Übersteuerung mit Restzeit, Anwesenheit im Raum, Status der Klima-Koordination (nur bei eingeschaltetem Schalter), Kindersicherung, Filterwechsel fällig.
* **Graphen & Verlauf** *(0.10.49)*: drei **Chart.js**-Diagramme, umschaltbar zwischen **Live** (letzte 10 Minuten, alle 2 s, nur im Browser), **6 h** und **24 h**:
  * *Luftqualität & Regelung*: CO2, Lüfterstufe (0 = Aus) und PID-Bedarf (lokaler Sensorbedarf, nur in der Smart-Automatik) — zeigt, wie die Regelung arbeitet (Hysterese, Stufensprünge).
  * *Raumklima*: Raumtemperatur, relative und **absolute Feuchte** (g/m³, Magnus-Formel wie beim Enthalpie-Schutz).
  * *Wärmerückgewinnung*: Außenluft (NTC Zuluft), Zuluft nach WRG (Innen-NTC in der Zuluft-Phase, *0.10.51*), Abluft (NTC Abluft) und WRG-Wirkungsgrad.

  Die Ansichten 6 h / 24 h kommen aus dem **Gerät** (`GET /history`): 5-Minuten-Mittelwerte der letzten 24 h in einem RAM-Ringpuffer (288 Punkte, ~5,2 KB). Sie überstehen also ein Neuladen der Seite und sind in jedem Browser gleich. Der Verlauf liegt nur im RAM (kein Flash-Verschleiß) und beginnt nach einem Neustart neu. Unter den Diagrammen zeigt eine **Statistik** Min / Mittel / Max über den gespeicherten Verlauf (bis 24 h, aus den 5-Minuten-Mitteln) für CO2, Temperatur, relative und absolute Feuchte, Lüfterstufe, Bedarf, Wirkungsgrad, Außenluft, Zuluft nach WRG und Abluft.
* **Verbindungsanzeige** *(0.10.49)*: unter dem Titel — türkis „Live-Verbindung (Stream)“, gelb „Verbunden (Abfrage alle 2 s)“ (Ausweichweg), rot „Keine Verbindung“. Kommen 6 s lang keine Daten, nennt ein roter Hinweis das Alter der angezeigten Werte („letzte Daten vor 34 s“) und die Kacheln werden abgeblendet — veraltete Werte sehen nie wie aktuelle aus. Jede Anfrage hat ein Timeout (4 s, `/history` 8 s).
* **Rückmeldung bei Änderungen** *(0.10.49)*: Eine Änderung gilt erst als bestätigt, wenn das Gerät den neuen Wert meldet (**„Gespeichert: …“**). Übernimmt das Gerät den Wert nicht innerhalb von 5 s, erscheint **„Nicht übernommen: … (aktuell …)“**; bei einer abgelehnten Anfrage der Grund (ungültiger Wert, Sicherheitsprüfung, Gerät ausgelastet, keine Verbindung), bei einer Zahl außerhalb des Gerätebereichs „Wert außerhalb min–max“. Die Meldungen erscheinen unten rechts; das Bedienelement wird wie bisher türkis / rot umrandet.
* **Gerät & System**: Geräte-ID (mit **MASTER**-Kennzeichnung bei Gerät 1), Floor / Raum, Phase, Firmware-Version und -Variante, mDNS-Hostname (*0.10.49*), Laufzeit, Ursache des letzten Neustarts (Einschalten, Software-Neustart, Absturz, Watchdog, Unterspannung …), WLAN-Signal, freier RAM (und das Minimum seit dem Start). IDs und Phase werden in der Einstellungsgruppe *Gerätekonfiguration* eingestellt (*0.10.55*).
* **Einstellungen** — alle Einstellungen des Geräts, gruppiert wie in HA; **RAUMWEIT** kennzeichnet raumweite Einstellungen (per ESP-NOW verteilt):
  * *Steuerung*: Modus, Lüfterstufe, Durchlüften-Dauer (0 = unbegrenzt), Kindersicherung, Bluetooth Proxy (nur `ventosync_nosensor_btproxy`).
  * *Smart-Automatik*: Min-/Max-Stufe, CO2- und Feuchte-Grenzwert, Anwesenheits-Anpassung, Übersteuerungsdauer, Sommerkühlung-Schwelle.
  * *Klima-Koordination*: Schalter, CO2-Grenzwert, Max-Stufe, CO2-Notfallgrenze.
  * *NOx-Stoßlüftung*: raumweiter Schalter; die Schwelle nur bei den SGP41-Varianten.
  * *Urlaubsmodus*: Betriebsmodus und Stufe, beide raumweit (*0.10.52*).
  * *Netzwerk*: ESP-NOW-Sync-Intervall, ESP-NOW-Peerprüfung (Master-LED blinkt ohne Peers, raumweit).
  * *Gerätekonfiguration* *(0.10.55)*: Stockwerk-ID, Raum-ID, Geräte-ID, Phase A/B und „Fenstersperre ignorieren“ (alle pro Gerät) sowie die Buttons **„ESP-NOW Discovery starten“** und **„Gerät neu starten“** (mit Rückfrage). Eine Änderung von Stockwerk oder Raum startet wie bei den HA-Entitäten eine neue Peer-Suche.

  Zahlenfelder übernehmen ihre Grenzen vom Gerät (`/meta`); ein Wert außerhalb des HA-Slider-Bereichs wird nicht gesendet. Das Bedienelement wird kurz türkis (übernommen) oder rot (abgelehnt) umrandet.
* **Wartung**: Filter-Betriebstage, Filteralarm, lokale Radar-Präsenz und ein Button **„Filterwechsel bestätigen“** (mit Rückfrage), der den Filterzähler wie der HA-Button `Filterwechsel (Reset)` zurücksetzt.
* **Stufen-Slider in der Smart-Automatik**: Eine Stufenänderung startet die raumweite **Stufen-Übersteuerung** (wie die Stufen-Taste, Dauer `Smart-Automatik: Übersteuerung Dauer`, Standard 60 min). Ein Hinweis unter dem Slider zeigt die Restzeit (`/state`-Schlüssel `auto_override_min`); ein Moduswechsel beendet die Übersteuerung. Siehe [Smart-Automatik Logik](de_smart-automatic-logic.md#7-stufen-übersteuerung-stufen-taste).
* **SGP41 + SHT45 Sensorboard** (nur `ventosync_sgp41` / `ventosync_v2_sgp41`): Die Kachel *Luftqualität* beschriftet den CO2-Wert als **„CO2-Äquivalent (VOC)“** und zeigt zusätzlich **VOC-Index** und **NOx-Index** (`/state`-Schlüssel `voc_index`, `nox_index`, nur von diesen Varianten gesendet). Andere Varianten behalten die Beschriftung „CO2“. Die Peer-Karten zeigen weiterhin „CO2“ — das ESP-NOW-Paket enthält den Sensortyp nicht.
* **Autarker Standalone-Betrieb**: Vollständig ohne Home Assistant nutzbar. Erreichbar unter `http://<deine-IP-Adresse>/` (oder `http://ventosync-a1b2c3.local/`); auch der Link **„Besuchen“** auf der Geräteseite in Home Assistant öffnet es. `/ui` funktioniert weiterhin (alte Lesezeichen).

> [!NOTE]
> Seit 0.10.55 ersetzt das Dashboard die native ESPHome-Seite unter `/`. ESPHomes `web_server`-Komponente bleibt nur in der Firmware (`local: false`, `log: false`), weil Home Assistant den „Besuchen“-Link aus dem gemeldeten Web-Port baut; `wrg_dashboard` startet vor ihr, daher beantwortet sein Handler `/`. Die native Entitätstabelle und die Log-Anzeige sind damit nicht mehr erreichbar — Logs über `esphome logs <variante>.yaml --device <ip>` oder das ESPHome-Add-on.

---

## 📸 Dashboard-Vorschau

### VentoSync Dashboard (Tailwind CSS UI)

<p align="center">
  <img src="../screenshots/wrg-dashboard1.png" alt="WRG Dashboard Einstellungen & Übersicht" width="48%" />
  &nbsp;
  <img src="../screenshots/wrg-dashboard2.png" alt="WRG Dashboard Verbundene Geräte & Live-Sensordaten" width="48%" />
</p>

*Links: Hauptsteuerung, Lüfterstufen-Slider, Betriebsmodi und Konfigurationsparameter.*  
*Rechts: Verbundene ESP-NOW-Geräte und Echtzeit-Sensorkacheln.*

---

## 📡 ESP-NOW Visualisierung

Das Web-Dashboard scannt und visualisiert alle aktiven Lüftungsgeräte im selben Raum in Echtzeit. Die Kachel **"Verbundene Geräte (ESP-NOW)"** zeigt:
* **Geräte-ID / Node-ID**, mit **MASTER**-Kennzeichnung bei Gerät 1 und dem Alter des letzten Pakets („zuletzt vor 12 s“)
* **Status-Punkt**: türkis, solange die Daten des Peers frisch genug für die Raum-Fusion sind (≥ 5 min, zwei Heartbeats), gelb, wenn sie älter sind (die Karte bleibt bis zum 15-Minuten-Peer-Timeout)
* **Aktueller Betriebsmodus** (Automatik, WRG, Durchlüften, Stoßlüftung, Aus) — seit 0.10.48 erscheint ein Peer in der Smart-Automatik als „Automatik“ statt mit dem Modus, den er gerade fährt
* **Lüfterstufe** (1–10)
* **Aktuelle Luftrichtung** (IN / OUT; `--` im Modus Aus)
* **Raum-Eingänge dieses Geräts**: Präsenz, Fenster offen, Klima an, NOx (seine eigenen ESP-NOW-Raum-Flags)
* **Telemetrie** (Lüfter-RPM, Board-Temperatur, PID-Bedarf)
* **Luftqualität je Gerät** (CO2 in ppm, Bewertung, Temperatur, relative Luftfeuchtigkeit)
* **„Dashboard dieses Geräts öffnen ↗“** *(0.10.49)*: Link zum Dashboard des Peers über seinen mDNS-Namen (`http://ventosync-a1b2c3.local/`). Jedes Gerät heißt `ventosync-<letzte 6 Hex-Stellen seiner MAC>` (`name_add_mac_suffix`), und das Dashboard kennt die MAC jedes Peers aus seinen ESP-NOW-Paketen — keine Protokolländerung. Der Link braucht einen Client, der `.local`-Namen auflöst (macOS, iOS, Windows 10+, Linux mit Avahi, aktuelles Android); sonst den Peer über seine IP öffnen.

> [!NOTE]
> Die Luftqualitätswerte werden per ESP-NOW (Protokoll v11) geteilt. Jede Karte zeigt also die Messwerte des **jeweiligen** Geräts — egal ob SCD43 oder der BME680-eCO2-Fallback. Geräte ohne Klimasensor zeigen `--`; die Bewertung nutzt dieselbe Klassifizierung wie die lokale Kachel „Luftqualität", sodass beide immer übereinstimmen.

---

## 🔒 Zugriff & Sicherheit

* **Standardmäßig ohne Login** — `/`, `/ui`, `/state`, `/stream`, `/history`, `/meta` und `/set` für jeden Client im Netzwerk offen. Betreibe die Geräte in einem vertrauenswürdigen Netz (oder einem eigenen VLAN).
* **Optionaler Login** — im eigenen Build `auth:` unter `web_server:` in `packages/base/esp32c6_common.yaml` aktivieren (Zugangsdaten `webuser` / `webpw` in `secrets.yaml`). Das Dashboard registriert sich über `web_server_base::add_handler()`, daher schützt derselbe Login alle Endpunkte.
* **Keine Änderungen von fremden Seiten** *(0.10.48)* — `/set` nimmt nur `POST` mit dem Header `X-VentoSync` an (sonst `405` / `403`), und zwar nur von der Seite des Geräts selbst: Anfragen mit fremdem `Origin` werden mit `403` abgelehnt, auch bei `/state`, `/stream`, `/history` und `/meta` (dieselbe Regel wie beim ESPHome-Webserver). Eine Webseite, die du im selben Netz öffnest, kann die Lüftung also nicht mehr über einen versteckten Link oder ein Formular umschalten. Skripte ohne Browser (`curl`) funktionieren weiter: `curl -X POST -H 'X-VentoSync: 1' -d 'id=luefter_modus&val=Aus' http://<ip>/set`.
* **Grenzen für `/set`** — nur die Schlüssel der im Dashboard gezeigten Einstellungen werden angenommen, Werte sind auf 32 Zeichen begrenzt (`400 Value too long`), Zahlen müssen numerisch sein und im Bereich der Entität liegen, und höchstens 16 Änderungen warten auf die Hauptschleife (`503 Busy`, wenn voll; das Gerät loggt pro Abarbeitung eine zusammenfassende Warnung).
* **`/state` entsteht in der Hauptschleife** (einmal pro Sekunde, solange eine Seite abfragt oder streamt) und wird vom Webserver-Task nur kopiert — der Webserver liest die Peer-Liste also nie, während ESP-NOW sie ändert. Die erste Anfrage nach einer Pause antwortet ~100 ms lang mit `503`; die Seite wiederholt automatisch.
* **Server-Sent Events statt Abfrage** *(0.10.49)* — die Seite öffnet `GET /stream` (`text/event-stream`), das Gerät schickt das `/state`-JSON alle 2 s und sofort nach einer Änderung als Ereignis `state`; keine Anfrage pro Aktualisierung. Höchstens 3 Streams gleichzeitig (weitere erhalten `503`), ein Client, der 20 s lang keine Daten annimmt, wird getrennt. Liefert der Stream 5 s lang nichts (abgelehnt, Neuverbindung, ältere Firmware), fragt die Seite stattdessen alle 2 s `/state` ab und versucht den Stream nach 30 s erneut.
* **`GET /history`** — `{"interval_s":300,"age_s":42,"scale":{"co2":1,"t":10,…},"series":{"co2":[612,…],…}}`: ältester Wert zuerst, Festkomma (durch `scale` teilen), `null` = keine Daten; der neueste Wert endete vor `age_s` Sekunden. Kanäle: `co2`, `t`, `rh`, `level`, `demand` (0–1), `eff`, `t_out`, `t_in`, `t_sup` (Zuluft nach WRG, *0.10.51*).

---

## 🌐 Hybrider Offline-Betrieb

> [!IMPORTANT]
> **CDN Asset-Laden**: Während alle Steuerungslogiken, PID-Regelungen und Sensorauswertungen zu 100 % lokal auf dem ESP32-C6 laufen (voll funktionsfähig auch ohne Internetverbindung), lädt das Web-Dashboard die Bibliotheken **Tailwind CSS** und **Chart.js** über ein externes CDN (`https://cdn.tailwindcss.com`...).
>
> Für die grafische Darstellung und die Diagramme im Webbrowser ist daher eine aktive Internetverbindung erforderlich. Ohne sie bleibt die Seite bedienbar (Werte, Einstellungen, Peers; ungestylt) und zeigt statt der Diagramme einen Hinweis.

Die Seite selbst (`components/wrg_dashboard/dashboard.html`) wird beim Build gzip-komprimiert (*0.10.49*, ~50 KB → ~14 KB im Flash) und mit `Content-Encoding: gzip` ausgeliefert.
