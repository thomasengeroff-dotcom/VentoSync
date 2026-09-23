# ❄️🔥 Intelligente Klimaanlagen-Koordination (Smart Climate Control)

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_smart-climate-control.md)

> **Status:** Implementiert seit Version **0.10.13** (raumweite Grenzwerte und geteilter CO2-Wert seit **0.10.14**, ESP-NOW-Protokoll v8; Klima-Status per Home-Assistant-Action und raumweiter Schalter seit **0.10.22**, Protokoll v10). Die Entscheidungslogik liegt in
> [`components/ventilation_logic/hvac_coordinator.h`](../../components/ventilation_logic/hvac_coordinator.h)
> (rein, unit-getestet), die Integration in die Smart-Automatik in
> [`components/helpers/auto_mode.h`](../../components/helpers/auto_mode.h) und die Home-Assistant-Entitäten in
> [`packages/ui/ui_controls.yaml`](../../packages/ui/ui_controls.yaml) sowie
> [`packages/integration/homeassistant.yaml`](../../packages/integration/homeassistant.yaml).

## Problemstellung

Dezentrale Wohnraumlüftungen mit Wärmerückgewinnung tauschen ständig Innen- gegen Außenluft aus. Wenn eine **Raumklimaanlage (Split-Klimagerät oder mobiles Klimagerät)** einen Raum aktiv konditioniert, ist intensive Lüftung kontraproduktiv: Sie importiert heiße, feuchte Außenluft (bzw. bläst im Winter teuer erwärmte Raumluft hinaus), belastet den Kompressor stärker und verschwendet Energie. Andererseits verschlechtert das vollständige Abschalten der Lüftung die Raumluftqualität (CO2-Anstieg, VOC-Anreicherung) und beseitigt den für den Feuchteschutz nötigen Grundluftwechsel.

**Ziel:** Solange die Klimaanlage im Raum aktiv ist, drosselt VentoSync die Lüftung auf das **Minimum, das für gesunde Raumluft erforderlich ist** — und nicht mehr. Gesundheit hat immer Vorrang vor Energieeffizienz.

---

## Funktionskonzept

### Aktivierung & Modusumschaltung

VentoSync bietet einen **dedizierten Schalter** in Home Assistant, mit dem Smart Climate Control für den **ganzen Raum** aktiviert oder deaktiviert wird:

| HA-Entität | YAML-ID | Typ | Standard | Zweck |
| :--- | :--- | :---: | :---: | :--- |
| `switch.klima_koordination` („Klima-Koordination") | `smart_climate_control` | **Switch**, raumweit | Aus | Aktivierung/Deaktivierung der HVAC-Koordination. Das Schalten an einem Gerät schaltet alle Geräte des Raums (ESP-NOW). Wird im Flash gespeichert. |

Bei **deaktiviertem** Schalter ignoriert die Lüftung den Klima-Status vollständig und arbeitet normal.
Bei **aktiviertem** Schalter reagiert VentoSync auf die Klimaanlagen-Entität des Raums und wendet das unten beschriebene eingeschränkte Profil **ausschließlich im Betriebsmodus `Smart-Automatik`** an. Manuelle Modi (Wärmerückgewinnung, Durchlüften, Stoßlüftung, Aus) werden nie verändert — das Feature ist ein *Modifikator* der Automatik, kein eigener Betriebsmodus.

### Klimaanlagen-Status (aus Home Assistant)

Home Assistant **sendet** den Klima-Status mit der API-Action **`set_ac_active`** an die Lüftungsgeräte (definiert in `packages/integration/homeassistant.yaml`):

| HA-Action | Daten | Bedeutung |
| :--- | :--- | :--- |
| `esphome.<gerätename>_set_ac_active` | `active: true` / `false` | `true` = Klimaanlage ist in einem konditionierenden Modus eingeschaltet. |

* **Raumweit:** Der gesendete Status wird per ESP-NOW mit allen Geräten des Raums geteilt (Flag in Protokoll v10). Home Assistant muss deshalb **mindestens ein** Gerät des Raums erreichen; die Action an alle Geräte zu senden erhöht die Redundanz. Jedes Gerät sendet nur den Status, den HA an *dieses* Gerät geschickt hat (nie das raumweite Ergebnis) — das Flag kann sich daher nicht gegenseitig festhalten.
* **Unabhängig von der Raum-ID:** Anders als eine importierte Entität hängt die Action nicht von der Compile-Zeit-Substitution `room_id` ab — die zur Laufzeit eingestellte Raumzuordnung (`config_room_id`) bestimmt, welche Geräte den Status teilen.
* **Ablauf:** Ein gesendeter Status gilt **15 Minuten** (`AC_STATE_MAX_AGE_MS`). Die HA-Automation sendet ihn regelmäßig erneut (empfohlen alle 5 min, siehe [Home-Assistant-Einrichtung](#️-home-assistant-einrichtung)); das deckt auch Geräteneustarts und HA-Neustarts ab.
* **Diagnose:** Der zuletzt gesendete Wert erscheint als `binary_sensor.klima_aktiv_ha_signal` („Klima aktiv (HA-Signal)").

> [!IMPORTANT]
> **Den Betriebsmodus der Klimaanlage verwenden, nicht die Kompressor-Aktion.** Eine `climate`-Entität liefert zwei verschiedene Signale:
> `state` (der gewählte *hvac_mode*: `cool`, `heat`, `heat_cool`, `dry`, `fan_only`, `off`) und das Attribut `hvac_action`
> (was das Gerät *gerade jetzt* tut: `cooling`, `heating`, `drying`, `idle`, `off`). Sobald der Sollwert erreicht ist, wechselt
> der Kompressor alle paar Minuten zwischen `cooling` und `idle`. Würde „Klima aktiv" auf `hvac_action` abgebildet, würde die
> Lüftung ständig zwischen gedrosseltem und normalem Profil pendeln. Die empfohlene Zuordnung lautet deshalb:
>
> - **Klima aktiv** = `state` ∈ {`cool`, `heat`, `heat_cool`, `dry`, `auto`}
> - **Klima inaktiv** = `state` ∈ {`off`, `fan_only`, `unavailable`, `unknown`}
>
> `fan_only` zählt als inaktiv: Das Gerät wälzt nur Luft um, es gibt keine thermische Last zu schützen.

**Fail-Safe-Verhalten:** Hat Home Assistant noch nie einen Status gesendet, ist der letzte älter als 15 Minuten oder ist die API-Verbindung getrennt, gilt der lokale Klima-Status als **inaktiv** (ein aktueller Status eines Peers im Raum zählt weiterhin). Die Lüftung wird nie blind gedrosselt.

---

## Regelungsstrategie: Nur-Luftqualitäts-Profil

Wenn Smart Climate Control **aktiviert** ist, der Betriebsmodus **Smart-Automatik** lautet und die Klimaanlage **aktiv** ist, wechselt die Automatik in das eingeschränkte **„Nur Luftqualität"**-Profil:

### Leitprinzip

> **Nur so viel lüften, wie für die Gesundheit nötig ist — nicht für Komfort oder Entfeuchtung.**

Die standardmäßige Dual-PID-Regelung (CO2 + Feuchte) wird durch eine **reine CO2-Regelschleife** mit verschärften Grenzen ersetzt. Da Schalter und Klima-Status raumweit gelten, wendet jedes Gerät des Raums dasselbe Profil an — auch der Bedarf, den ein Gerät mit seinen Peers teilt, ist während der Drosselung reiner CO2-Bedarf:

| Parameter | Normale Smart-Automatik | HVAC-Koordination (gedrosselt) | Begründung |
| :--- | :---: | :---: | :--- |
| **CO2-Sollwert (PID)** | `auto_co2_threshold` (Standard 1000 ppm) | `hvac_co2_threshold` (Standard **1200 ppm**) | Gelockerte Zielgröße; 1200 ppm sind laut DIN EN 13779 Kategorie IDA 3 noch „mäßig / akzeptabel" und benötigen deutlich weniger Luftwechsel. |
| **Max. Lüfterstufe** | `automatik_max_fan_level` (Standard 7) | `hvac_max_fan_level` (Standard **3**) | Harte Obergrenze gegen Energieverschwendung; Stufe 3 erzeugt minimale Geräusche und thermische Last. |
| **Min. Lüfterstufe** | `automatik_min_fan_level` (Standard 2) | **1** (fest) | Grundlüftung (Feuchteschutz nach DIN 1946-6). Bewusst unterhalb des normalen Nutzer-Minimums: Im Kühl-/Entfeuchtungsbetrieb entfeuchtet die Klimaanlage wesentlich effektiver als die Lüftung. |
| **Feuchte-PID** | Aktiv (Entfeuchtung) | **Ignoriert** | Lüftungsbasierte Entfeuchtung würde feuchte Außenluft importieren. Der Schimmelschutz (unten) bleibt als Sicherheitsnetz. |
| **Sommerkühlung (Bypass)** | Aktiv (Querlüftung) | **Unterdrückt** | Querlüftung importiert Luft, die die Klimaanlage anschließend erneut konditionieren muss. |
| **Betriebsmodus innerhalb der Smart-Automatik** | Dynamisch (WRG / Durchlüften) | **Erzwungen: Wärmerückgewinnung** | Minimiert den thermischen Austausch mit der Außenluft, solange der Raum klimatisiert wird. |

Alles andere bleibt unverändert: Der CO2-PID (`kp = 0.001`, `ki = 0.0000005`), die diskrete Stufenzuordnung mit ihrem ±25-%-Hystereseband und die sanfte Rampe von **±1 Stufe pro 10-Sekunden-Zyklus** sind dieselben Codepfade wie im normalen Automatikbetrieb — nur Sollwert und Stufenfenster werden ausgetauscht.

### CO2-Schwellenwert-Begründung

| CO2-Wert (ppm) | DIN EN 13779 Kategorie | Gesundheitsbewertung | Rolle in VentoSync |
| :---: | :---: | :--- | :--- |
| ≤ 800 | IDA 1 (Hoch) | Ausgezeichnet | Bei aktivem Klimabetrieb unnötig |
| ≤ 1000 | IDA 2 (Mittel) | Gut — Pettenkofer-Grenzwert | Standard-Sollwert der Smart-Automatik |
| ≤ 1200 | IDA 3 (Mäßig) | Akzeptabel | **Standard-HVAC-Sollwert** (`hvac_co2_threshold`) |
| ≤ 1500 | IDA 4 (Niedrig) | Tolerierbar, nicht für Dauerbetrieb | Obere Sicherheitsgrenze |
| > 1500 | — | Schlecht | **Notfall-Override** (`hvac_emergency_co2`) — normale Automatikregelung greift wieder |

---

## Gesundheitsschutz (Guards)

Zwei Schutzmechanismen heben die Einschränkungen auf, während die Klimaanlage weiterläuft. Beide arbeiten mit Hysterese, damit der Lüfter nicht pendelt.

### 1. CO2-Notfall-Override

| Ereignis | Bedingung | Wirkung |
| :--- | :--- | :--- |
| **Eintritt** | CO2 ≥ `hvac_emergency_co2` (Standard 1500 ppm) | Stufenbegrenzung aufgehoben (normale `automatik_min/max`), CO2-Sollwert zurück auf `auto_co2_threshold`, Feuchte-PID wieder aktiv. Wärmerückgewinnung bleibt erzwungen (kein Sommer-Bypass bei laufender Klimaanlage). |
| **Freigabe** | CO2 ≤ `hvac_co2_threshold` (Standard 1200 ppm) | Gedrosseltes Profil wird wieder aktiv. |

Die Firmware hält die Notfallgrenze **mindestens 100 ppm oberhalb** des gelockerten Sollwerts, sodass ein falsch eingestellter Slider die Hysterese nie kollabieren lassen kann.

### 2. Schimmelschutz (Feuchte)

Das ursprüngliche Konzept hat die Feuchteregelung komplett deaktiviert. Das ist nur sicher, solange die Klimaanlage tatsächlich entfeuchtet (Kühl-/Entfeuchtungsmodus). Läuft die Klimaanlage im **Heizbetrieb**, entzieht sie keine Feuchte, und auch im Sommer kann ein Bad oder eine Küche die Schimmelschwelle überschreiten. Deshalb:

| Ereignis | Bedingung | Wirkung |
| :--- | :--- | :--- |
| **Eintritt** | Raumfeuchte ≥ **70 % rH** **und** Lüften kann den Raum trocknen (absolute Außenfeuchte < Innenfeuchte, Magnus-Formel — derselbe Enthalpie-Guard wie beim Feuchte-PID) | Wie beim CO2-Notfall: Einschränkungen aufgehoben, Dual-PID aktiv, Wärmerückgewinnung erzwungen. |
| **Freigabe** | Raumfeuchte ≤ **65 % rH** **oder** Außenluft wird feuchter als Innenluft | Gedrosseltes Profil wird wieder aktiv. |

Ist die Außenluft schwüler als der Raum, würde Lüften Feuchte *eintragen* — der Schutz bleibt stumm und überlässt die Entfeuchtung der Klimaanlage.

### 3. Fehlender CO2-Messwert (raumweit)

Die Gesundheitsgarantie dieses Features beruht auf einer CO2-Messung (SCD43 oder BME680-eCO2-Fallback über `effective_co2`). Die Messung gilt **raumweit**: Jedes Gerät teilt seinen eigenen effektiven CO2-Wert im ESP-NOW-Paket, und der Koordinator wertet den **höchsten** CO2-Wert aus lokalem Sensor und allen Peers aus, deren Wert höchstens 5 Minuten alt ist (`ventosync::room::PEER_DATA_MAX_AGE_MS`; bei längerem Sync-Intervall zwei Heartbeats). Ein Gerät ohne eigenen Sensor (Varianten `radar_only` / `nosensor` / `NTConly`) koordiniert damit auf Basis der Raummessung. Die Log-Zeile kennzeichnet Werte von einem Peer mit „via Peer".

Der **Schimmelschutz** arbeitet genauso: Er nutzt die höchste relative Feuchte aus lokalem SCD41 und allen aktuellen Peers (`room_humidity`) sowie die am selben Ort gemessene Temperatur für den Vergleich der absoluten Feuchte. Damit greift er auch in Räumen, deren Master keinen Feuchtesensor hat.

Nur wenn **kein Gerät im Raum** einen CO2-Wert liefert, meldet der Koordinator **„Ausgesetzt (kein CO2-Wert im Raum)"** und drosselt **nicht**. Die Wärmerückgewinnung bleibt bei aktiver Klimaanlage weiterhin erzwungen.

---

## Übergangsverhalten

* **Klimaanlage schaltet ein:** Das eingeschränkte Profil greift sofort auf dem Gerät, das die HA-Action erhalten hat, und auf den übrigen Geräten des Raums mit ihrem nächsten ESP-NOW-Heartbeat (Standard alle 60 s). Der Lüfter fährt um höchstens 1 Stufe pro 10-Sekunden-Zyklus herunter, z. B. Stufe 6 → 3 in ca. 30 s; das Stufenfenster wird immer durchgesetzt, auch wenn der Lüfter oberhalb der Obergrenze lief.
* **Klimaanlage schaltet aus:** Die Firmware wartet auf **120 s durchgehendes „aus"**, bevor die Einschränkungen aufgehoben werden (`AC_RELEASE_DELAY_MS`). Das fängt kurze Home-Assistant-Reconnects und kurzzeitiges Umschalten ab. Bei Split-Geräten, deren Integration nur `hvac_action` liefert, den gesendeten Status aus einem Template-Binary-Sensor mit `delay_off` ableiten (siehe unten), um Kompressor-Taktung weiter zu glätten.
* **Freigabe:** Grenzen und Sollwert kehren zu den Nutzerwerten zurück; das CO2-PID-Integral wird bei jedem Sollwertwechsel und das Feuchte-PID-Integral bei Wiederaktivierung zurückgesetzt, sodass kein Windup aus der gedrosselten Phase übernommen wird. Der Lüfter fährt mit ±1 Stufe pro 10 s wieder hoch.
* **Sollwert-Autorität:** Der HA-Slider `auto_co2_threshold` und der ESP-NOW-Konfigurationsabgleich schreiben beide das CO2-PID-Ziel. Der Koordinator setzt das korrekte Ziel in jedem Zyklus erneut, sodass eine Slider-Änderung während des Klimabetriebs den gelockerten Sollwert nicht unbemerkt überschreiben kann.

---

## Entscheidungs-Zustandsautomat

```mermaid
stateDiagram-v2
    [*] --> Deaktiviert : Schalter aus
    [*] --> Bereit : Schalter an

    Deaktiviert --> Bereit : Schalter an
    Bereit --> Deaktiviert : Schalter aus

    Bereit --> Aktiv : Klima an
    Aktiv --> Bereit : Klima 120 s aus

    state Aktiv {
        [*] --> Gedrosselt
        Gedrosselt --> Notfall_CO2 : CO2 >= hvac_emergency_co2
        Notfall_CO2 --> Gedrosselt : CO2 <= hvac_co2_threshold
        Gedrosselt --> Notfall_Feuchte : rH >= 70 % und Aussenluft trockener
        Notfall_Feuchte --> Gedrosselt : rH <= 65 % oder Aussenluft feuchter
        Gedrosselt --> Ausgesetzt : kein CO2 im Raum (lokal + Peers)
        Ausgesetzt --> Gedrosselt : CO2-Wert gueltig
    }

    note right of Gedrosselt
        Nur CO2-PID, Sollwert hvac_co2_threshold,
        Stufen 1..hvac_max_fan_level, WRG erzwungen
    end note
    note right of Notfall_CO2
        Normale Smart-Automatik-Grenzen und -Sollwert,
        Dual-PID, WRG weiterhin erzwungen
    end note
```

Der Zustand wird als Diagnose-Textsensor **„Klima-Koordination Status"** (`hvac_status`) mit folgenden Werten veröffentlicht:

| Wert | Bedeutung |
| :--- | :--- |
| `Deaktiviert` | Schalter ist aus. |
| `Inaktiv (kein Smart-Automatik)` | Schalter an, aber ein manueller Betriebsmodus ist gewählt. |
| `Bereit (Klima aus)` | Scharf, Klimaanlage inaktiv — normale Smart-Automatik. |
| `Aktiv (gedrosselt)` | Klimaanlage aktiv — Nur-Luftqualitäts-Profil angewendet. |
| `Notfall (CO2)` | Klimaanlage aktiv, CO2-Notfall-Override greift. |
| `Notfall (Feuchte)` | Klimaanlage aktiv, Schimmelschutz greift. |
| `Ausgesetzt (kein CO2-Wert im Raum)` | Klimaanlage aktiv, aber kein CO2-Wert von irgendeinem Gerät im Raum — keine Drosselung. |

---

## Konfigurations-Entitäten

| HA-Entität (deutscher UI-Name) | YAML-ID | Typ | Standard | Bereich | Zweck |
| :--- | :--- | :---: | :---: | :---: | :--- |
| `Klima-Koordination` | `smart_climate_control` | Switch, **raumweit** | Aus | — | Aktivierung/Deaktivierung der HVAC-Koordination für den Raum. |
| `Klima-Koordination: CO2 Grenzwert` | `hvac_co2_threshold` | Number (Slider), **raumweit** | 1200 ppm (`hvac_default_co2_threshold`) | 800–1500 ppm | Gelockerter CO2-Sollwert bei aktiver Klimaanlage. Zugleich Freigabeschwelle des CO2-Notfalls. |
| `Klima-Koordination: Max Lüfterstufe` | `hvac_max_fan_level` | Number (Slider), **raumweit** | 3 (`hvac_default_max_fan_level`) | 1–5 | Maximale Lüfterstufe bei aktiver Klimaanlage. |
| `Klima-Koordination: CO2 Notfallgrenze` | `hvac_emergency_co2` | Number (Slider), **raumweit** | 1500 ppm (`hvac_default_emergency_co2`) | 1200–2000 ppm | CO2-Wert, ab dem die normale Automatikregelung unabhängig vom Klima-Status greift (wird ≥ Sollwert + 100 ppm gehalten). |
| `Klima-Koordination Status` | `hvac_status` | Textsensor (Diagnose) | — | — | Aktueller Koordinator-Zustand (siehe Tabelle oben). |
| `Klima aktiv (HA-Signal)` | `hvac_ac_active` | Binary-Sensor (Diagnose) | — | — | Zuletzt von Home Assistant an dieses Gerät gesendeter Klima-Status (Action `set_ac_active`). |

Alle Slider und der Schalter sind `entity_category: config`, werden im NVS gespeichert und greifen beim nächsten Auswertezyklus. Der Schalter und die drei Slider sind **raumweite Einstellungen**: Eine Änderung an einem beliebigen Gerät wird sofort per ESP-NOW an alle Peers des Raums übertragen (`sync_settings_to_peers()`), und der Master-Heartbeat setzt sie erneut, genau wie die Smart-Automatik Min/Max-Stufen. Ihre Startwerte stammen aus den Substitutionen `hvac_default_co2_threshold`, `hvac_default_emergency_co2` und `hvac_default_max_fan_level` in `ventosync_base.yaml` (Schalter: aus). Werte von Peers werden nur innerhalb der obigen Slider-Bereiche übernommen; der Koordinator begrenzt sie zusätzlich (Konstanten `CO2_THRESHOLD_*`, `EMERGENCY_CO2_*`, `MAX_FAN_LEVEL_CONFIG_*` in `hvac_coordinator.h`). Feste Konstanten (`MIN_FAN_LEVEL = 1`, Schimmelschutz 70 % / 65 %, Freigabeverzögerung 120 s, Notfallabstand 100 ppm) sind in `hvac_coordinator.h` definiert.

---

## 🛠️ Home-Assistant-Einrichtung

Pro Raum eine Automation anlegen, die den Klima-Status an die VentoSync-Geräte dieses Raums sendet. `<gerätename>` ist der ESPHome-Node-Name des Geräts (Bindestriche werden zu Unterstrichen, z. B. `ventosync-dg-buero` → `esphome.ventosync_dg_buero_set_ac_active`).

```yaml
automation:
  - alias: "VentoSync: Klima-Status Raum 1"
    mode: queued
    triggers:
      - trigger: state
        entity_id: climate.schlafzimmer_klima
      - trigger: homeassistant
        event: start
      - trigger: time_pattern   # erneut senden: der Status läuft im Gerät nach 15 min ab
        minutes: "/5"
    variables:
      # Den gewählten hvac_mode verwenden, NICHT hvac_action (Kompressor-Taktung würde flattern).
      ac_active: "{{ states('climate.schlafzimmer_klima') in ['cool', 'heat', 'heat_cool', 'dry', 'auto'] }}"
    actions:
      # Mindestens ein Gerät des Raums; für Redundanz alle Geräte auflisten.
      - action: esphome.ventosync_schlafzimmer_1_set_ac_active
        data:
          active: "{{ ac_active }}"
        continue_on_error: true
      - action: esphome.ventosync_schlafzimmer_2_set_ac_active
        data:
          active: "{{ ac_active }}"
        continue_on_error: true
```

* **`input_boolean` / Schaltsteckdose** (mobiles Klimagerät): diese Entität als Trigger und `is_state('input_boolean.xyz', 'on')` als `ac_active` verwenden.
* **Zusätzliche Glättung** für Integrationen, die nur `hvac_action` liefern: einen Template-Binary-Sensor mit `delay_off: {minutes: 5}` anlegen und ihn statt der Climate-Entität als Trigger/Quelle nutzen.
* **Mehrere Klimageräte in einem Raum:** im `ac_active`-Template mit `or` verknüpfen (oder einen **Binary-Sensor-Gruppen**-Helfer „beliebige Entität an" verwenden — genau wie bei der [Fenstersperre](de_window-guard-ha-setup.md)).

> [!IMPORTANT]
> **Umstieg von ≤ 0.10.21:** Die Firmware importiert `binary_sensor.ventosync_hvac_active_room_<room_id>` nicht mehr (Substitution `hvac_ac_sensor_id` entfernt). Ein vorhandener Template-Binary-Sensor kann bleiben und einfach als Trigger/Quelle der obigen Automation dienen. Der Schalter „Klima-Koordination" steht nach dem Update auf **aus** — einmal pro Raum einschalten.

---

## Räume mit mehreren Geräten (ESP-NOW, Protokoll v10)

Das `VentilationPacket` trägt die Smart-Climate-Control-Felder `room_co2`, `hvac_co2_threshold`, `hvac_emergency_co2`, `hvac_max_fan_level` (seit v8) und `hvac_flags` (seit **v10**: Bit 0 = raumweiter Schalter, Bit 1 = eigener HA-Klima-Status des Senders). Alle Geräte eines Raums müssen dieselbe Firmware-Version fahren — gleichzeitiges OTA-Rollout, wie bei jedem Protokollbump. Fünf Mechanismen halten eine Raumgruppe konsistent:

1. **Geteilter CO2-Wert:** Jedes Gerät sendet seinen effektiven CO2-Wert; Geräte ohne Sensor werten den Koordinator mit dem Raumwert aus (siehe [Fehlender CO2-Messwert](#3-fehlender-co2-messwert-raumweit)).
2. **Raumweite Einstellungen:** Der Schalter und die drei Slider werden über den bestehenden Config-Sync-Pfad (`handle_config_sync()`) abgeglichen: Eine Änderung an einem Gerät geht als `MSG_STATE` an alle Peers, der `MSG_SYNC`-Heartbeat des Masters setzt die Werte auf Slaves erneut.
3. **Stufen-Autorität:** In der Smart-Automatik spiegeln Slaves die diskrete Lüfterstufe des Masters (Geräte-ID 1). Drosselt der Master auf Stufe 1–3, folgt jeder Slave innerhalb eines Auswertezyklus.
4. **Modus-Abgleich:** Das periodische Sync-Paket des Masters trägt den erzwungenen WRG-Modus; Slaves übernehmen ihn.
5. **Geteilter Klima-Status:** Ein Gerät wertet die Klimaanlage als aktiv, wenn HA „aktiv" an dieses Gerät **oder** an einen aktuellen Peer gesendet hat (Aktualität: 5 min, bei längerem Sync-Intervall zwei Heartbeats).

Jedes Gerät wertet den Koordinator dennoch lokal mit den raumweiten Eingangsdaten aus, sodass ein Slave bei ausgefallenem Master selbstständig drosselt. Slaves, die dem Master folgen, begrenzen die übernommene Stufe zusätzlich auf ihr eigenes Stufenfenster.

---

## Energie-Auswirkungsabschätzung

| Szenario | Ø Lüfterstufe | Geschätzte Lüfterleistung | Thermische Last auf Klimaanlage |
| :--- | :---: | :---: | :--- |
| **Normale Smart-Automatik** (Klima ignoriert) | 4–6 | 2–4 W | Hoch — ständige Außenluftzufuhr, Klima kompensiert |
| **HVAC-Koordination** (Klima aktiv) | 1–3 | 0,5–1,5 W | **Minimal** — Wärmerückgewinnung bei geringem Volumenstrom |
| **Aus** (keine Lüftung) | 0 | 0 W | Keine — aber CO2 steigt, ungesund |

> [!TIP]
> Grobe Abschätzung für ein 20 m² Schlafzimmer mit 2 Personen und einer 3,5-kW-Split-Klimaanlage: Die Drosselung von Stufe 5 auf Stufe 1–2 während des Klimabetriebs nimmt der Klimaanlage in der Größenordnung von **50–150 W** kontinuierlicher thermischer Kompensationslast ab, also grob **10–20 %** ihres Energiebedarfs in sommerlichen Spitzenstunden. Die tatsächliche Einsparung hängt von Außenbedingungen, Wärmetauscher-Wirkungsgrad und der belegungsabhängigen CO2-Last ab.

---

## Implementierungshinweise

1. **Sensor-Anforderungen:** Keine zusätzliche Hardware. Benötigt eine CO2-Quelle (`effective_co2`: SCD43 oder BME680-eCO2) und eine Home-Assistant-Automation, die den Klima-Status sendet (`set_ac_active`). Der Schimmelschutz nutzt zusätzlich die Raumfeuchte und `sensor.outdoor_humidity`.
2. **Dateien:** `components/ventilation_logic/hvac_coordinator.h` (reiner `ventosync::hvac::Coordinator`, Unit-Tests T-7a–T-7p in `tests/simple_test_runner.cpp`), `components/ventilation_logic/room_fusion.h` (raumweite CO2-/Feuchte-/Klima-Fusion), `VentilationLogic::calculate_auto_target_level()` (Stufenfenster), `components/helpers/auto_mode.h` (`evaluate_hvac_coordination()`, `hvac_on_ha_ac_state()`, `apply_co2_setpoint()`, WRG-Sperre in `evaluate_auto_mode()`), `components/helpers/network_sync.h` (raumweiter Abgleich), `components/helpers/globals.h` (Entitäts-Externs, `hvac_state`), `packages/ui/ui_controls.yaml`, `packages/integration/homeassistant.yaml` (API-Action).
3. **Heizbetrieb:** Dieselbe Logik gilt, wenn die Klimaanlage im Winter heizt — intensive Lüftung würde warme Raumluft hinausbefördern. Da Heizen nicht entfeuchtet, ist der Schimmelschutz in dieser Jahreszeit das Sicherheitsnetz.
4. **Flash-Verschleiß:** Nur Schalter und die drei Slider werden (bei Änderung) persistiert. Der Laufzeitzustand (auch der gesendete Klima-Status) liegt im RAM.
5. **Hardware-Varianten:** Die Entitäten existieren in allen Varianten. Auch die MQTT-Variante braucht für die Action `set_ac_active` die native API. Varianten ohne CO2-Quelle nutzen den von einem Peer geteilten CO2-Wert; nur ein Raum ganz ohne CO2-Quelle meldet `Ausgesetzt (kein CO2-Wert im Raum)` und drosselt nie.
