# 🔧 Dynamische Geräte-Konfiguration via Home Assistant

[![Language: EN](https://img.shields.io/badge/Language-EN-blue.svg)](../en/en_dynamic-configuration.md)


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
| **Stockwerk (Floor ID)** | Stockwerk-Nummer | 1-9 (0 = nicht eingerichtet) | 1 |
| **Raum (Room ID)** | Raum-Nummer | 1-10 (0 = nicht eingerichtet) | 1 |
| **Geräte-ID (Device ID)** | Eindeutige Geräte-ID | 1-25 | 1 |
| **Geräte-Phase (A/B)** | Startphase des Geräts | A oder B | A |
| **Eigene MAC Adresse** | Anzeige der eigenen MAC | - | (automatisch) |

> ⚠️ **Stockwerk und Raum 0 nicht verwenden:** Die Eingabefelder erlauben zwar 0, ein Gerät mit Stockwerk **oder** Raum 0 sendet aber keine Suchrufe (Discovery-Broadcasts) und findet die anderen Geräte seines Raums nicht. Die Nummerierung beginnt deshalb bei 1 — z. B. Stockwerk 1 = Erdgeschoss, Stockwerk 2 = Obergeschoss.

**Hinweis zu ESP-NOW:** Die Kommunikation erfolgt automatisch über ein **hybrides Discovery-System**. Neue Geräte finden sich per Broadcast, der restliche Datenaustausch erfolgt ressourcenschonend und sicher per **Unicast**. Es ist **keine manuelle MAC-Konfiguration** erforderlich!

### Phase A vs. Phase B

**Phase A:** Startet mit **Zuluft** (Rein)  
**Phase B:** Startet mit **Abluft** (Raus)

Die Phase bezieht sich auf den Raum (hinein / hinaus), nicht auf die Wand, an der das Gerät sitzt.

**Regel:** Bei der Wärmerückgewinnung arbeiten die Geräte eines Raums im Gegentakt. Damit gleich viel Luft hinein- wie hinausströmt, sollten im Raum **gleich viele Geräte Phase A und Phase B** haben — ein **Paar braucht also ein Gerät mit Phase A und eines mit Phase B**, egal ob die beiden an derselben oder an gegenüberliegenden Wänden sitzen. Beim Durchlüften bläst Phase A dauerhaft hinein und Phase B hinaus; für eine Querlüftung setzt man das A-Gerät an die eine und das B-Gerät an die gegenüberliegende Seite der Wohnung.

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

> 💡 **Ohne Home Assistant:** Seit 0.10.56 öffnet das lokale Web-Dashboard (`http://<device-ip>/`) auf einem noch nicht eingerichteten Gerät einen **Einrichtungs-Assistenten** — Stockwerk/Raum, Geräte-ID mit Prüfung gegen die im Raum schon vergebenen IDs, Phase mit Vorschlag und die raumweiten Grundeinstellungen der Smart-Automatik. Siehe [Lokales Web-Dashboard → Einrichtungs-Assistent](de_local-web-dashboard.md#-einrichtungs-assistent-01056).

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
Geräte-Phase:            Phase B (Gegenstück zu Gerät 1 – ein Paar braucht A und B!)
```

### 4. ESP-NOW Kommunikation (automatisch)

**Keine manuelle Konfiguration erforderlich!**

Die ESP-NOW Kommunikation nutzt ein hybrides Modell aus **Broadcast** und **Unicast**:

✅ **Broadcast zur Entdeckung:** Der Broadcast-Modus (`FF:FF:FF:FF:FF:FF`) wird **nur** verwendet, um neue Peers im gleichen Stockwerk und Raum automatisch zu identifizieren.
✅ **Unicast für den Betrieb:** Sobald sich Peers gefunden haben, erfolgt die gesamte operative Kommunikation (Status-Updates, Einstellungs-Sync) gezielt per **Unicast**. Dies ist deutlich zuverlässiger (Hardware-ACK) und reduziert die Funk-Last.
✅ **Masterrolle (Device ID 1):** Das Gerät mit der **Device ID 1** übernimmt automatisch die Masterrolle für die zeitliche Synchronisation der Lüftungszyklen (Taktgeber).
✅ **Dezentrale Befehlsverteilung:** Befehle (z.B. intensitätsänderungen oder Modus-Wechsel) werden von **jedem** Gerät in der Gruppe entgegengenommen. Das auslösende Gerät verteilt die Änderung sofort per Unicast an alle anderen Peers der Gruppe.
✅ **Gruppen-Synchronität:** Da alle Geräte permanent ihren Status spiegeln und Änderungen sofort propagieren, bleibt eine Gruppe (gleiche Floor/Room ID) immer zu 100% synchron.

**Vorteile:**

- 🚀 **Plug & Play** - Es müssen keine MAC-Adressen eingetragen werden, Peers finden sich von selbst
- 🔄 **Zuverlässigkeit** - Unicast mit Empfangsbestätigung (Hardware-ACK)
- 🛡️ **Sicher** - Filterung nach Floor ID, Room ID und Device ID

### 5. Speichern & Fertig

Die Werte werden **automatisch im Flash** gespeichert und bleiben nach Neustarts erhalten.

## 🏢 Beispiel-Konfiguration: Mehrgeschossiges Haus

Stockwerk- und Raum-IDs beginnen bei **1** (mit 0 sendet ein Gerät keine Suchrufe, siehe oben).

### Erdgeschoss (Floor 1)

| Raum | Gerät | Floor ID | Room ID | Device ID | Phase |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Küche | 1 | 1 | 1 | 1 | A |
| Küche | 2 | 1 | 1 | 2 | B |
| Wohnzimmer | 1 | 1 | 2 | 1 | A |
| Wohnzimmer | 2 | 1 | 2 | 2 | B |

### Obergeschoss (Floor 2)

| Raum | Gerät | Floor ID | Room ID | Device ID | Phase |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Schlafzimmer | 1 | 2 | 1 | 1 | A |
| Schlafzimmer | 2 | 2 | 1 | 2 | B |
| Kinderzimmer | 1 | 2 | 2 | 1 | A |
| Bad | 1 | 2 | 3 | 1 | A |

Ein Einzelgerät (Kinderzimmer, Bad) hat kein Gegenstück; seine Phase ist dann beliebig.

## 🔄 ESP-NOW Synchronisation

Geräte kommunizieren nur mit anderen Geräten, die:

- ✅ **Gleiche Floor ID** haben
- ✅ **Gleiche Room ID** haben
- ✅ **Unterschiedliche Device ID** haben

### Beispiel: Paar-Betrieb

```text
Gerät A: Floor 1, Room 2, Device 1, Phase A
Gerät B: Floor 1, Room 2, Device 2, Phase B
```

Diese beiden Geräte synchronisieren sich automatisch:

- Gerät A: Zuluft → Gerät B: Abluft
- Nach 70 s (Stufe 1) bis 50 s (Stufe 10) wechseln beide die Richtung
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
3. **Phase** so wählen, dass im Raum gleich viele Geräte Phase A und Phase B haben (Paar: A + B)
4. **Warten** bis Gerät neu synchronisiert (max. 3 min oder manuell Neustart)

### Phase-Auswahl

- **Paar** (zwei Geräte im Raum) → ein Gerät **Phase A**, eines **Phase B** — unabhängig von der Wand
- **Mehrere Geräte** → möglichst gleich viele A und B (bei ungerader Anzahl eines mehr A oder B)
- **Einzelgerät** → Phase beliebig (Standard: A)
- Der Einrichtungs-Assistent im Web-Dashboard zählt die Phasen der Geräte im Raum und schlägt die passende vor

### Änderungen während Betrieb

- Änderungen werden **sofort** wirksam
- ESP-NOW Synchronisation erfolgt beim nächsten **Sync-Intervall** 
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
3. Bei Paar-Betrieb: Die beiden Geräte müssen gegenläufig blasen (eines hinein, eines hinaus) — also unterschiedliche Phasen haben. Bläst ein Gerät trotz richtiger Phase falsch herum, ist der Lüfter verkehrt herum eingebaut oder angeschlossen; dann seine Phase tauschen, sodass die Luftrichtungen wieder gegenläufig sind

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
