# 🔧 Dynamische Geräte-Konfiguration via Home Assistant

## Übersicht

Statt die Geräte-IDs fest im YAML-Code zu definieren, können alle Lüftungsgeräte mit der **gleichen Firmware** geflasht werden. Die individuelle Konfiguration (Stockwerk, Raum, Geräte-ID, Phase) erfolgt dann bequem über **Home Assistant** nach dem ersten Einbinden ins WLAN.

## ✨ Vorteile

- ✅ **Eine Firmware für alle Geräte** - Keine individuellen Builds mehr nötig
- ✅ **Einfache Konfiguration** - Einstellung über Home Assistant UI
- ✅ **Persistent gespeichert** - Werte bleiben nach Neustart erhalten
- ✅ **Jederzeit änderbar** - Flexibel anpassbar ohne Neu-Flashen
- ✅ **Sofort wirksam** - Änderungen werden direkt übernommen

## 📋 Konfigurierbare Parameter

### In Home Assistant verfügbar

| Parameter | Beschreibung | Bereich | Standard |
| :--- | :--- | :--- | :--- |
| **Stockwerk (Floor ID)** | Stockwerk-Nummer | 0-99 | 1 |
| **Raum (Room ID)** | Raum-Nummer | 0-99 | 1 |
| **Geräte-ID (Device ID)** | Eindeutige Geräte-ID | 1-99 | 1 |
| **Geräte-Phase (A/B)** | Startphase des Geräts | A oder B | A |
| **Eigene MAC Adresse** | Anzeige der eigenen MAC | - | (automatisch) |

**Hinweis zu ESP-NOW:** Die Kommunikation erfolgt im **Broadcast-Modus** (FF:FF:FF:FF:FF:FF). Alle Geräte empfangen alle Nachrichten, aber die Filterung erfolgt automatisch im Code basierend auf Floor/Room/Device ID. Es ist **keine manuelle MAC-Konfiguration** erforderlich!

### Phase A vs. Phase B

**Phase A:** Startet mit **Zuluft** (Rein)  
**Phase B:** Startet mit **Abluft** (Raus)

**Regel:** Geräte an der **gleichen Wand** sollten die **gleiche Phase** haben.

## 🚀 Workflow: Geräte einrichten

### 1. Firmware flashen (einmalig)

Alle Geräte erhalten die **gleiche** Firmware:

```bash
esphome run ventosync.yaml
```

### 2. In Home Assistant einbinden

1. Gerät mit WLAN verbinden (Fallback-AP oder vorkonfiguriert)
2. In Home Assistant wird das Gerät automatisch erkannt
3. Integration hinzufügen

### 3. Geräte-Konfiguration setzen

In Home Assistant unter dem Gerät findest du folgende Einstellungen:

#### Beispiel: Wohnzimmer, Stockwerk 1, Gerät 1

```text
Stockwerk (Floor ID):     1
Raum (Room ID):          2  (z.B. Wohnzimmer)
Geräte-ID (Device ID):   1  (Erstes Gerät im Raum)
Geräte-Phase:            Phase A (Startet mit Zuluft)
```

#### Beispiel: Wohnzimmer, Stockwerk 1, Gerät 2 (Paar-Betrieb)

```text
Stockwerk (Floor ID):     1
Raum (Room ID):          2  (Gleicher Raum!)
Geräte-ID (Device ID):   2  (Zweites Gerät im Raum)
Geräte-Phase:            Phase A (Gleiche Wand = gleiche Phase!)
```

### 4. ESP-NOW Kommunikation (automatisch)

**Keine manuelle Konfiguration erforderlich!**

Die ESP-NOW Kommunikation funktioniert automatisch im **Broadcast-Modus**:

✅ **Alle Geräte senden an:** `FF:FF:FF:FF:FF:FF` (Broadcast)  
✅ **Alle Geräte empfangen:** Alle ESP-NOW Nachrichten  
✅ **Filterung erfolgt automatisch:** Im VentilationController basierend auf:

- Floor ID (muss übereinstimmen)
- Room ID (muss übereinstimmen)
- Device ID (muss unterschiedlich sein)

**Beispiel:**

```text
Gerät 1: Floor 1, Room 2, Device 1
Gerät 2: Floor 1, Room 2, Device 2
Gerät 3: Floor 1, Room 3, Device 1

→ Gerät 1 & 2 kommunizieren miteinander (gleicher Raum)
→ Gerät 3 wird ignoriert (anderer Raum)
```

**Vorteile:**

- 🚀 Plug & Play - Keine MAC-Adressen eintragen
- 🔄 Flexibel - Geräte können jederzeit hinzugefügt werden
- 🛡️ Sicher - Filterung durch Floor/Room/Device ID

### 5. Speichern & Fertig

Die Werte werden **automatisch im Flash** gespeichert und bleiben nach Neustarts erhalten.

## 🏢 Beispiel-Konfiguration: Mehrgeschossiges Haus

### Erdgeschoss (Floor 0)

| Raum | Gerät | Floor ID | Room ID | Device ID | Phase |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Küche | 1 | 0 | 1 | 1 | A |
| Küche | 2 | 0 | 1 | 2 | B |
| Wohnzimmer | 1 | 0 | 2 | 1 | A |
| Wohnzimmer | 2 | 0 | 2 | 2 | B |

### Obergeschoss (Floor 1)

| Raum | Gerät | Floor ID | Room ID | Device ID | Phase |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Schlafzimmer | 1 | 1 | 1 | 1 | A |
| Schlafzimmer | 2 | 1 | 1 | 2 | B |
| Kinderzimmer | 1 | 1 | 2 | 1 | A |
| Bad | 1 | 1 | 3 | 1 | A |

## 🔄 ESP-NOW Synchronisation

Geräte kommunizieren nur mit anderen Geräten, die:

- ✅ **Gleiche Floor ID** haben
- ✅ **Gleiche Room ID** haben
- ❌ **Unterschiedliche Device ID** haben (ignoriert sich selbst)

### Beispiel: Paar-Betrieb

```text
Gerät A: Floor 1, Room 2, Device 1, Phase A
Gerät B: Floor 1, Room 2, Device 2, Phase A
```

Diese beiden Geräte synchronisieren sich automatisch:

- Gerät A: Zuluft → Gerät B: Abluft
- Nach 70s wechseln beide die Richtung
- Kontinuierlicher Luftaustausch mit Wärmerückgewinnung

## 🛠️ Technische Details

### Persistenz

Die Werte werden mit `restore_value: true` im **Flash-Speicher** gespeichert:

```yaml
- platform: template
  name: "Stockwerk (Floor ID)"
  restore_value: true  # ← Persistent!
```

### Echtzeit-Update

Änderungen werden sofort an den VentilationController übergeben:

```yaml
set_action:
  then:
    - lambda: |-
        auto *v = (esphome::VentilationController*)id(ventilation_ctrl);
        v->set_floor_id((uint8_t)x);
```

### Logging

Alle Änderungen werden geloggt:

```text
[config] Floor ID updated to: 2
[config] Room ID updated to: 3
[config] Device ID updated to: 1
[config] Phase updated to: B
```

## 📱 Home Assistant UI

Die Einstellungen erscheinen als **Number-Inputs** und **Select-Dropdown**:

```text
┌─────────────────────────────────────┐
│ ESPTest                             │
├─────────────────────────────────────┤
│ Stockwerk (Floor ID)        [  1  ] │
│ Raum (Room ID)              [  2  ] │
│ Geräte-ID (Device ID)       [  1  ] │
│ Geräte-Phase (A/B)          [ A ▼ ] │
├─────────────────────────────────────┤
│ Lüftungsmodus               [Wärme▼]│
│ Durchlüften Dauer (min)     [ 30  ] │
└─────────────────────────────────────┘
```

## ⚠️ Wichtige Hinweise

### Beim ersten Einrichten

1. **Stockwerk & Raum** zuerst setzen
2. **Geräte-ID** eindeutig vergeben (1, 2, 3, ...)
3. **Phase** entsprechend der Wandposition wählen
4. **Warten** bis Gerät neu synchronisiert (max. 3 Stunden oder manuell Neustart)

### Phase-Auswahl

- Geräte an **gegenüberliegenden Wänden** → **Unterschiedliche Phasen**
- Geräte an **gleicher Wand** → **Gleiche Phase**
- Bei Unsicherheit: **Alle auf Phase A** starten und beobachten

### Änderungen während Betrieb

- Änderungen werden **sofort** wirksam
- ESP-NOW Synchronisation erfolgt beim nächsten **Sync-Intervall** (Standard: 3h)
- Für sofortige Synchronisation: **Gerät neu starten**

## 🔍 Troubleshooting

### Geräte synchronisieren nicht

**Problem:** Zwei Geräte im gleichen Raum arbeiten nicht zusammen.

**Lösung:**

1. Prüfe Floor ID und Room ID (müssen identisch sein!)
2. Prüfe Device ID (müssen unterschiedlich sein!)
3. Warte auf nächstes Sync-Intervall oder starte beide Geräte neu

### Falsche Luftrichtung

**Problem:** Gerät bläst in die falsche Richtung.

**Lösung:**

1. Ändere Phase von A → B oder B → A
2. Beobachte Verhalten
3. Bei Paar-Betrieb: Beide Geräte sollten gleiche Phase haben

### Werte werden nicht gespeichert

**Problem:** Nach Neustart sind alte Werte wieder da.

**Lösung:**

1. Prüfe ob `restore_value: true` in YAML gesetzt ist
2. Flash-Speicher könnte voll sein → Gerät komplett neu flashen
3. Prüfe Logs auf Fehler

## 📚 Weiterführende Informationen

- **ESP-NOW Kommunikation:** Siehe `components/ventilation_group/ventilation_group.h`
- **Packet-Struktur:** `VentilationPacket` in Header-Datei
- **Synchronisations-Logik:** `on_packet_received()` Funktion

---

**Vorteil dieser Lösung:** Du kannst eine **Master-Firmware** erstellen und auf alle Geräte flashen. Die individuelle Konfiguration erfolgt dann bequem über die Home Assistant UI - kein Code-Anpassen mehr nötig! 🎉
