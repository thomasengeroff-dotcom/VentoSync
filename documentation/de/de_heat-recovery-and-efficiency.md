# 🧠 Wärmerückgewinnung, Sensor-Physik & Effizienzberechnung

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_heat-recovery-and-efficiency.md)


Dieses Dokument bietet tiefe technische Einblicke in den regenerativen Keramik-Wärmerückgewinnungszyklus, die phasen-synchrone NTC-Temperaturstabilisierung, die energiebasierte Effizienzberechnung (DIN EN 13141-8) sowie die kabellose ESP-NOW Push-Pull-Gruppensynchronisation.

---

### Betriebszyklus (50s bis 70s pro Phase)

```mermaid
graph LR
    A[Phase 1: ABLUFT 70s] -->|Keramik lädt sich auf| B[Phase 2: ZULUFT 70s]
    B -->|Keramik gibt Wärme ab| A
    
    style A fill:#ff6b6b
    style B fill:#4ecdc4
```

### Phase 1: Abluft (Rausblasen) - 70 Sekunden

```text
Innenraum (warm) → Keramikspeicher → Außen
    21°C              ↓ Wärme         5°C
                  speichern
```

**Was passiert:**

- 🔥 Warme Raumluft (21°C) strömt durch den Keramikspeicher
- 📈 Keramik erwärmt sich und speichert Energie
- 🌡️ **NTC Innen** misst am Ende die wahre Raumtemperatur
- 💨 Abgekühlte Luft (~10°C) wird nach außen geblasen

### Phase 2: Zuluft (Reinblasen) - 70 Sekunden

```text
Außen → Keramikspeicher → Innenraum (vorgewärmt)
 5°C     ↑ Wärme           ~16°C
        abgeben
```

**Was passiert:**

- ❄️ Kalte Außenluft (5°C) strömt durch den warmen Keramikspeicher
- 🔄 Keramik gibt gespeicherte Wärme ab
- 🌡️ **NTC Außen** misst Außentemperatur
- 🌡️ **NTC Innen** misst vorgewärmte Zuluft (~16°C)
- 🏠 Vorgewärmte Luft strömt in den Raum

### NTC Sensoren (Temperatur-Stabilisierung)

Die NTC Sensoren messen die Temperatur am Keramikspeicher innen und außen (`temp_zuluft` und `temp_abluft`). Da die Lüfterrichtung im Wärmerückgewinnungs-Modus zyklisch (z.B. alle 70 Sekunden) wechselt, benötigen die Sensoren aufgrund ihrer thermischen Masse eine gewisse Zeit, um sich an die neue Lufttemperatur anzupassen. Um die Messung möglichst genau zu machen, werden sehr kleine NTC Sensoren genutzt, mit möglichst geringer Masse und hoher Genauigkeit. Dadurch wird die Anpassung an die wechselnde Temperatur je nach Lüftungsrichtung möglichst schnell und präzise.
Um fehlerhafte Zwischenwerte in Home Assistant zu vermeiden und die wahren thermischen Grenzwerte exakt zu erfassen, nutzen beide Sensoren eine **intelligente, vereinheitlichte und phasengesteuerte Temperatur-Stabilisierung**:

- **Phase-Lock:** Das System verwirft Messwerte während der "falschen" Luftrichtung explizit (z.B. Innensensor während der Zuluft-Phase). Dies verhindert, dass der Puffer mit bereits erwärmter/gekühlter Luft verfälscht wird.
- **Thermische Wartezeit:** Nach einem Richtungswechsel (Push/Pull) wird die Messwertübertragung für **40% der Zyklusdauer (min. 15s)** pausiert, damit sich der NTC an den neuen Luftstrom anpassen kann.
- **Kombinierter Filter:** Alle Stufen (Phase-Lock, History-Invalidierung, Stabilitätscheck und saisonale Selektion) sind in einer einzigen, performanten C++ Funktion (`filter_ntc_combined`) vereint.
- **Dynamische Saison-Selektion:**
  - **Winter/Übergang:** Der Außensensor nimmt den Minimalwert (wahre kalte Außenluft), der Innensensor den Maximalwert (wahre warme Raumluft).
  - **Sommer-Kühlung:** Wenn die Außenluft heißer ist als die Innenluft, kehrt sich die Logik automatisch um (Außen nimmt Max, Innen nimmt Min).
- **Median-Fallback:** Sollte ein Referenzsensor temporär ausfallen, nutzt das System den Median der letzten 3 Werte als sicheren Kompromiss.
- **120s Failsafe-Timeout:** Ein großzügiger Watchdog hält die Sensoren in Home Assistant "online", auch wenn während langer Lüftungsphasen durch den Phase-Lock zeitweise keine Werte geliefert werden.

*Hinweis zur Redundanz:* `temp_zuluft` (Außen-NTC) liefert bei nach innen gerichtetem Luftstrom die tatsächliche Außentemperatur. `temp_abluft` (Innen-NTC) liefert bei nach außen gerichtetem Luftstrom die Raumtemperatur und dient als Redundanz zum präziseren Sensorboard. Die Smart-Automatik nimmt die Innentemperatur vom Sensorboard an H2 (SCD43 oder SHT45), danach vom Innen-NTC; der BMP390 auf der Hauptplatine misst die Platine bzw. das Netzteil (10–15 K über dem Raum) und wird nie als Raumtemperatur verwendet.

Konkret wird der folgende Sensor verwendet:

| Hersteller | Artikelnummer | Bezugsquelle | Genauigkeit | Datenblatt |
| :--- | :--- | :--- | :--- | :--- |
| **VARIOHM** | `ENTC-EI-10K9777-02` | [Reichelt Elektronik](https://www.reichelt.de/de/de/shop/produkt/thermistor_ntc_-40_bis_125_c-350474) | ± 0,2 °C | [PDF](../datasheets/ENTC_EI-10K9777-02E.pdf) |

### Effizienzberechnung (Energiebasiert)

Die wahre Wärmerückgewinnungseffizienz eines Keramikspeichers über einen vollständigen Zyklus ist energiebasiert, nicht basierend auf punktuellen Temperaturen (gemäß DIN EN 13141-8).

Am Ende der Zuluft-Phase berechnet das System die Effizienz mittels **numerischer Trapez-Integration** über die gesamte Phasendauer:

$$
\eta_{WRG} = \frac{\int (\text{T}_{Zuluft} - \text{T}_{Außen}) dt}{\int (\text{T}_{Raum} - \text{T}_{Außen}) dt}
$$

**Warum das mathematisch überlegen ist:**
Würde man die Effizienz als simplen Durchschnitt der momentanen Effizienzwerte berechnen, würde der Wert bei sehr kleinen Temperaturunterschieden ($\Delta T$) extrem ungenau und numerisch instabil werden (explodierende Werte) – etwa in der Übergangszeit. Durch die Integration der Temperaturdifferenzen über die Zeit bleibt die Berechnung physikalisch korrekt, stabil und liefert ein echtes Abbild der während des Zyklus zurückgewonnenen Wärmeenergie.

**Zurückgewonnene thermische Energie (Wh):**
Zusätzlich zur prozentualen Effizienz berechnet das System die tatsächlich zurückgewonnene thermische Energie in **Wattstunden (Wh)**. Dies erfolgt durch ein nicht-lineares Mapping der 10 Lüfterstufen auf die realen Volumenströme des Ventomaxx v-wrg-1 (von ca. 17 m³/h auf Stufe 1 bis zu 43 m³/h auf Stufe 10) und der Integration der tatsächlichen Temperaturdifferenz ($T_{Zuluft} - T_{Außen}$) über die Zeit. So lässt sich genau nachvollziehen, wie viel Heizenergie (oder Kühlenergie im Sommer) das System pro Zyklus "eingespart" hat.

#### Zulufttemperatur nach WRG

In der Zuluft-Phase sitzt der Innen-NTC in der Luft, die gerade den Keramikspeicher durchströmt hat und in den Raum
geblasen wird — der Phase-Lock verwirft diese Messwerte für die Ablufttemperatur, die Effizienzberechnung nutzt sie aber
als $T_{Zuluft}$. Seit 0.10.51 wird ihr **Mittelwert über jede Zuluft-Phase** als `sensor.temp_zuluft_nach_wrg`
(„Temp. Zuluft (nach WRG)“) veröffentlicht und im Web-Dashboard angezeigt (Karte, Diagramm *Wärmerückgewinnung*,
Statistik, dazu der Gewinn gegenüber der Außenluft in K). Die drei Temperaturen machen die Wärmerückgewinnung greifbar,
z. B. im Winter:

| Außenluft | Zuluft nach WRG | Abluft (Raum) | Wirkungsgrad |
|---|---|---|---|
| 2,0 °C | 18,4 °C | 21,0 °C | ≈ 86 % |

Der Mittelwert umfasst die ganze Phase — zu Beginn ist die Keramik noch warm, gegen Ende wird die Zuluft kühler — und
passt damit zum energiebasierten Wirkungsgrad ($\eta \approx (T_{Zuluft} - T_{Außen}) / (T_{Raum} - T_{Außen})$).
Er wird nur im WRG-Betrieb (auch Smart-Automatik) veröffentlicht; in *Durchlüften*, *Stoßlüftung* und *Aus* gibt es keine
Rückgewinnung, der Sensor ist dann *unbekannt*. Eine Phase, die länger als ein Halbzyklus ist (z. B. Rest eines
Moduswechsels), wird verworfen. Der Wert braucht alle drei NTC-Messwerte (Zuluft, Abluft, Außenluft) — ohne NTCs
(`nosensor`) bleibt die Entität ausgeblendet.

**Interpretation:**

- **> 70%:** Ausgezeichnete Wärmerückgewinnung
- **50-70%:** Gute Wärmerückgewinnung
- **< 50%:** Keramik zu kalt, Zyklus zu kurz oder Temperaturdifferenz zu gering

### Optimierung der Effizienz

| Parameter                 | Auswirkung                          | Empfehlung      |
| :------------------------ | :---------------------------------- | :-------------- |
| **Zyklusdauer**           | Längere Zyklen = bessere Speicherung| 70-90s optimal  |
| **Lüftergeschwindigkeit** | Langsamer = mehr Wärmeübertragung   | 60-80%          |
| **Keramikvolumen**        | Mehr Masse = mehr Speicher          | Größer ist besser|
| **Außentemperatur**       | Kälter = höhere Effizienz möglich   | -               |

### Synchronisation mehrerer Geräte

Bei Verwendung mehrerer Geräte im gleichen Raum:

**Paar-Betrieb (2 Geräte):**

```text
Gerät A: Phase A (Zuluft)  ←→  Gerät B: Phase B (Abluft)
         ↓ 70s wechseln ↓
Gerät A: Phase B (Abluft) ←→  Gerät B: Phase A (Zuluft)
```

**Vorteile:**

- ✅ Kontinuierlicher Luftaustausch
- ✅ Keine Druckschwankungen
- ✅ Optimale Wärmerückgewinnung
- ✅ Synchronisiert über ESP-NOW
