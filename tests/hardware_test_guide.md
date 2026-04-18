# Hardware-Testanleitung — Code Review Fixes (ventilation_group.h)

Diese Fixes können **nicht** im nativen Unit-Test-Runner verifiziert werden,
da sie ESPHome-Komponenten, ESP-IDF-APIs oder ESP-NOW-Hardware voraussetzen.
Alle Tests setzen ein laufendes VentoSync-Gerät mit aktivem Serial-Monitor voraus.

---

## Voraussetzungen

- ESPHome Serial-Monitor geöffnet (`esphome logs ventosync.yaml`)
- Home Assistant erreichbar und mit dem Gerät verbunden
- Mindestens **zwei** Geräte für Peer-Tests (HW-3, HW-5)

---

## HW-1 — Strict-Aliasing-Safe Paketempfang (`on_packet_received`) {#hw-1}

**Betroffener Fix:** K-1 — `memcpy` statt C-Style-Cast  
**Code:** `ventilation_group.h:496–500`

### Was getestet wird
Der Cast `(VentilationPacket *)data.data()` wurde durch `std::memcpy` ersetzt.
Das Verhalten ist semantisch identisch — dieser Test stellt sicher, dass die
neue Deserialisierung korrekt dekodierte Pakete liefert und keine Felder
korrumpiert.

### Testschritte

1. **Zwei Geräte starten** (Master ID 1, Slave ID 2)
2. Serial-Monitor auf dem Slave öffnen
3. Am Master: Modus oder Intensität ändern (z.B. Stufe auf 8 setzen)
4. Im Log des Slaves prüfen:

```
# Erwartete Log-Ausgaben auf dem Slave:
[I][vent]: on_packet_received() called
[I][vent_sync]: Received MSG_STATE from device X: mode=Y, intensity=Z
```

### Erwartetes Ergebnis
- Slave übernimmt **exakt** denselben Modus und dieselbe Intensität wie der Master
- Keine `[W]` oder `[E]` Ausgaben rund um `on_packet_received`
- Kein Neustart des Slaves

### Fehlerindikator
- Slave-Modus bleibt falsch (z.B. immer `MODE_OFF`) → `memcpy` schlägt fehl
- Slave-Intensität ist `0` oder `255` → Struct-Padding-Problem
- Watchdog-Reset innerhalb von 5s nach Empfang → Memory-Korruption

---

## HW-2 — HA-Synchronisation nach `update_hardware()` (`publish_state`) {#hw-2}

**Betroffener Fix:** K-3 — `main_fan->publish_state()` nach direkter State-Mutation  
**Code:** `ventilation_group.h:706–718`

### Was getestet wird
`main_fan->state` wird im Fenstersperre-Pfad direkt gesetzt, ohne `turn_on()`/
`turn_off()` aufzurufen. Ohne `publish_state()` weiß Home Assistant nichts davon.

### Testschritte — Fenstersperre aktivieren

1. Serial-Monitor öffnen
2. In Home Assistant: Fenstersperre-Sensor auf **Offen** setzen
   (Entity: `binary_sensor.ventosync_window_lock_room_X`)
3. Warten bis Lüfter stoppt (ca. 10s Debounce)
4. In Home Assistant: **Lüfter-Entity** prüfen  
   (Entity: `fan.ventosync_lueftung_X`)

### Erwartetes Ergebnis

| Schritt | HA-Status Lüfter | Physikalischer Lüfter |
|---------|-----------------|----------------------|
| Fenster öffnen | **Aus** (innerhalb ~15s) | Motor auf 50% PWM (Stopp) |
| Fenster schließen | **An** (innerhalb ~5s) | Motor läuft wieder |

### Fehlerindikator
- HA zeigt **An** obwohl Lüfter physisch steht → `publish_state()` fehlt/fehlerhaft
- HA zeigt **Aus** aber Lüfter **dreht** → falscher Pfad in `update_hardware()`

### Log-Erwartung
```
[D][vent]: Window guard active — fan stopped
[D][fan]: publish_state called, state=false
```

---

## HW-3 — Peer-Liste LRU-Eviction (MAX_PEERS = 10) {#hw-3}

**Betroffener Fix:** H-5 — Peer-Liste auf 10 Einträge begrenzt mit LRU-Eviction  
**Code:** `ventilation_group.h:537–565`

### Was getestet wird
Wenn mehr als 10 Peers einer Gruppe gesehen werden, wird der älteste (längste
inaktive) entfernt. Die Peer-Liste darf unbegrenzt **nie** wachsen.

### Testschritte (Simulationsansatz mit einer Einheit)

1. Im Code `MAX_PEERS` temporär auf `3` setzen und neu flashen
2. **Drei verschiedene Test-Geräte** sequenziell booten (IDs 1, 2, 3)
   — warten bis alle im Log als Peer erscheinen
3. Viertes Gerät (ID 4) booten
4. Log des ersten Geräts beobachten

### Erwartetes Ergebnis
```
# Wenn Gerät 4 als neuer Peer registriert wird:
[W][vent_group]: MAX_PEERS=3 reached. Evicting oldest peer (device_id=N).
[I][vent_group]: Registered new peer via ESP-NOW: device_id=4
```

- Die Peer-Liste bleibt bei **≤ 3 Einträgen**
- Das evakuierte Gerät ist das mit dem **ältesten `last_seen_ms`-Timestamp**
- Heap-Nutzung bleibt stabil (kein kontinuierlicher Anstieg über Stunden)

### Langzeit-Test (24h)
```bash
# Heap-Monitoring im Serial-Log aktivieren:
# heap_caps_get_free_size(MALLOC_CAP_DEFAULT) sollte stabil bleiben
# Erwartung: Variation < 1 KB über 24h im Normalbetrieb
```

### Fehlerindikator
- Log zeigt > 10 Peer-Einträge → Eviction-Logik defekt
- `E (MALLOC)` oder Watchdog-Reset nach vielen Peer-Wechseln → Heap-Erschöpfung

---

## HW-4 — Timer-Clamp in `build_packet()` {#hw-4}

**Betroffener Fix:** H-4 — `sync_interval_ms` und `ventilation_duration_ms`
werden vor `uint16_t`-Cast geclampt  
**Code:** `ventilation_group.h:772–778`

### Was getestet wird
Wenn `sync_interval_ms` oder `ventilation_duration_ms` einen Wert > 24h annimmt
(z.B. durch NVS-Korruption oder UI-Eingabefehler), darf der Cast nicht still
auf `0` überrollen.

### Testschritte

1. In HA: **Sync-Intervall-Slider** auf den **Maximalwert** setzen (1440 min)
2. Serial-Monitor beobachten — `build_packet()` wird beim nächsten Heartbeat aufgerufen

**Log-Erwartung (Normalfall, kein Clamp nötig):**
```
[D][vent]: build_packet: sync_interval=1440min, vent_duration=0min
```

3. Zum Test eines Clamp-Eingriffs: `sync_interval_ms` über direkten NVS-Write
   auf `0xFFFFFFFF` setzen (requires debugging session) — **oder** den
   `MAX_TIMER_MS`-Wert temporär auf `3600000` (1h) senken und Slider auf Max:

**Log-Erwartung (mit aktivem Clamp):**
```
[W][vent]: sync_interval_ms clamped from XXXXXX to MAX (86400000ms)
```

### Erwartetes Ergebnis
- Empfangendes Gerät dekodiert den Wert korrekt als **1440** (oder geclampt)
- Kein `0` oder `65535` im dekodiertem Wert beim Empfänger
- Kein Neustart

### Fehlerindikator
- Peer zeigt nach Sync-Empfang `sync_interval = 0 min` → Cast-Overflow aktiv
- Cycler bricht sofort ab → `ventilation_duration` ist auf 0 überrollt

---

## HW-5 — Watchdog-Fehlerbehandlung beim Soft-Reset (`esp_task_wdt_add`) {#hw-5}

**Betroffener Fix:** M-1 — `esp_task_wdt_add()` Rückgabewert geprüft  
**Code:** `ventilation_group.h:297`

### Was getestet wird
Nach einem **OTA-Update** oder manuellem **Soft-Reset** (`esp_restart()`) ist der
Task bereits beim WDT registriert. `esp_task_wdt_add()` gibt dann
`ESP_ERR_INVALID_ARG` zurück. Ohne Fix: stiller Fehler. Mit Fix: Debug-Log.

### Testschritte

1. Gerät normal booten — WDT-Registrierung prüfen:
```
# Erwartete Ausgabe beim Erstboot:
[D][vent]: Task WDT registered successfully
```

2. **Soft-Reset auslösen** (z.B. über HA-Button "System Neustart" oder
   `esp_restart()` via Konsole)
3. Serial direkt nach dem Neustart beobachten

### Erwartetes Ergebnis nach Soft-Reset
```
# Option A: Bereits registriert (erwartet nach Soft-Reset):
[D][vent]: Task WDT already registered (harmless after soft-reset)

# Option B: Echter Fehler:
[W][vent]: Task WDT registration failed: error=0xXX
```

**Kein Szenario darf** zu einem sofortigen Watchdog-Reset führen.

### Fehlerindikator
- Gerät bootet nach Soft-Reset in **Reboot-Loop** → WDT wurde doppelt
  registriert und löst sofort aus
- Log enthält `panic` oder `Task watchdog got triggered` direkt beim Boot

---

## Testmatrix — Schnellübersicht

| Test | Geräteanzahl | Dauer | Aufwand |
|------|-------------|-------|---------|
| HW-1 Paketempfang | 2 | 5 min | ⭐⭐ |
| HW-2 HA-Sync Fenstersperre | 1 | 10 min | ⭐ |
| HW-3 Peer LRU (MAX=3) | 4 | 20 min | ⭐⭐⭐ |
| HW-4 Timer-Clamp | 1 | 5 min | ⭐⭐ |
| HW-5 WDT Soft-Reset | 1 | 5 min | ⭐ |

**Empfohlene Reihenfolge:** HW-2 → HW-5 → HW-1 → HW-4 → HW-3

> [!TIP]
> HW-2 und HW-5 lassen sich auf einem einzelnen Gerät ohne weitere
> Hardware-Voraussetzungen in < 15 Minuten durchführen und decken die
> kritischsten Fixes ab.

> [!NOTE]
> HW-3 (Peer LRU) ist der einzige Test, der eine temporäre Codeänderung
> (`MAX_PEERS = 3`) erfordert. Nach dem Test wieder auf `10` zurücksetzen
> und neu flashen.
