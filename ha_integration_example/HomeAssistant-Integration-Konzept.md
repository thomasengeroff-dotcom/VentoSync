# Konzept: Intuitive Integration in Home Assistant als Raumgruppe

Dieses Konzept beschreibt, wie mehrere dezentrale Lüftungsgeräte (z.B. zwei Geräte im "Wohnzimmer", die im Gegentakt arbeiten) in Home Assistant (HA) als **eine einzige, intuitive Einheit** abgebildet werden können.

## 1. Die Architektur (Das "Group-Controller" Muster via ESP-NOW)

Anstatt jedes Gerät einzeln in Home Assistant einzubinden und dort komplexe Gruppen oder Automatisierungen zu bauen, verlagern wir die Logik in das ESPHome-Netzwerk. Da die Geräte in eurem Projekt bereits **ESP-NOW** für die Kommunikation nutzen (z.B. für Temperaturabgleich und Takt-Synchronisation), ist das der effizienteste und stabilste Weg.

### So funktioniert es

1. **Der Lead-Node:** Ein Gerät im Raum (oder ein dedizierter Mikrocontroller) fungiert als Ansprechpartner ("Lead") für Home Assistant für diesen speziellen Raum. Alternativ können auch alle Geräte einer Gruppe exakt die gleichen virtuellen Entitäten bereitstellen – HA fasst diese bei geschickter Namensgebung zusammen, aber ein definierter Master macht das Debugging meist einfacher.
2. **Virtuelle Entitäten:** Auf diesem Lead-Node definieren wir in ESPHome `template`-Entitäten (Lüfter, Sensoren, Select-Menüs). Diese Entitäten steuern nicht direkt die lokale Hardware, sondern repräsentieren den aggregierten Zustand und die Settings der *gesamten Gruppe*.
3. **Synchronisation nach unten (Befehle):** Wenn man in Home Assistant am Lead-Node die Lüfterstufe auf "3" stellt, sendet der ESPHome-Code via ESP-NOW einen Broadcast: `CMD_SET_SPEED: 3`. Alle Geräte in der ESP-NOW Gruppe (inklusive dem Lead-Node selbst) empfangen das Kommando und passen ihre reale Hardware an.
4. **Aggregation nach oben (Sensoren):** Alle verbauten Geräte (Node A, Node B, etc.) senden periodisch ihre lokalen Sensordaten (CO2, Temp, Feuchte) per ESP-NOW. Der Lead-Node empfängt diese, bildet z.B. einen Mittelwert für den CO2-Gehalt im Raum oder nutzt gezielt den **Maximalwert** (Worst-Case). Diesen aggregierten Wert meldet er als *einen* "Raum-Sensor" an Home Assistant.

### Echte Hardware-Entitäten in HA verstecken (Wartungsmodus)

Die realen Hardware-Lüfterkomponenten (`fan:`) und Rohsensoren der Einzelgeräte sollten in ESPHome idealerweise auf `internal: true` gesetzt werden. Dadurch "sieht" Home Assistant diese gar nicht erst und das Dashboard bleibt extrem übersichtlich.
Gibt es Bedarf, diese für Wartungszwecke (z.B. Filterwechsel, RSSI Signalstärke) dennoch in HA zu haben, macht man sie nicht `internal`, verschiebt sie im HA-Dashboard aber auf eine separate Admin-/Settings-Unterseite.

### Übersichtliche Vorteile

* **"WAF / SAF" (Wife/Spouse Acceptance Factor):** Die Bedienung ist simpel. "Das Wohnzimmer" taucht als einzelner Lüfter und ein CO2 Wert auf. Keine Verwirrung durch "Lüfter A" und "Lüfter B".
* **Weniger WLAN-Traffic & Ausfallsicherheit:** Home Assistant redet nur noch effektiv mit einer Entität pro Raum. Fällt das Heim-WLAN aus, redet die Gruppe per ESP-NOW autark weiter und kann über angeschlossene Hardware-Taster gesteuert werden (die dann ebenfalls ESP-NOW Befehle an den Rest der Gruppe senden).

## 2. Visuelle Darstellung im HA Dashboard

Um die Bedienung so intuitiv und einladend wie möglich zu machen, empfehlen wir den Einsatz der **Mushroom Cards** (verfügbar als Custom Integration über den Home Assistant Community Store / HACS).

Mit diesen Karten lässt sich eine Raum-Steuerung aufbauen:

* **Eine Mushroom Fan Card** als visueller Mittelpunkt.
  * Das Propeller-Icon animiert sich anhand der echten Gruppengeschwindigkeit.
  * Es hat einen eingebauten Schieberegler (Slider) für die Intensität.
  * *Extra-Konzept:* Man kann einen kleinen "Badge"-Indikator an das Icon hängen, der bei schlechter Raumluft rot aufleuchtet.
* **Mushroom Chips Cards** darunter.
  * Kleine ovale Badges für die wichtigsten Sensorwerte (Gemittelte Temperatur, max. CO2) und ein Dropdown für den Gesamtmodus ("Auto", "Manuell", "Sommer").

*Siehe `ha_dashboard_example.yaml` für ein Code-Beispiel zur Visualisierung.*

## 3. Umsetzung im ESPHome Code

Die Umsetzung erfordert die Nutzung von Template-Komponenten (`platform: template`) im ESPHome YAML-Code eures Projekts. Diese bilden die Brücke zwischen der HA-Schnittstelle und eurer C++ ESP-NOW Logik (die ihr laut `automation_helpers.h` und `ventilation_group.h` bereits sehr weit entwickelt habt).

*Siehe `esphome_master_node.yaml` für ein konkretes Code-Beispiel für den Mikrocontroller.*
