# 📡 ESP-NOW Funk-Mesh & Raum-Synchronisation

[![Language: EN](https://img.shields.io/badge/Language-EN-red.svg)](../en/en_esp-now-communication.md)


VentoSync nutzt **ESP-NOW** (über die [ESPHome ESP-NOW Komponente](https://esphome.io/components/espnow.html)) für die direkte, ultra-latenzarme Kommunikation von Gerät zu Gerät. ESP-NOW arbeitet direkt auf der MAC-Schicht im 2,4-GHz-Band, ohne Umweg über einen zentralen WLAN-Router oder Steuerkabel.

<p align="center">
  <img src="../../EasyEDA-Pro/PCB%20mounting/PCB-ANT-in-Gehäuse.jpg" alt="Externe Antenne im Gehäuse" width="500" />
</p>

---

## 💡 Architekturentscheidung: Warum ESP-NOW statt Powerline (PLC)?

Kommerzielle Systeme wie das originale VentoMaxx setzen auf **Powerline Communication (PLC)** über die 230V-Stromleitung, um Lüfterpaare zu synchronisieren. Im Wohnalltag führt PLC jedoch häufig zu Problemen:
* Hohe Anfälligkeit für Störungen durch Phasenverschiebungen bei unterschiedlichen Stromkreisen.
* Starke Beeinträchtigung durch Schaltnetzteile, LED-Treiber und Wechselrichter moderner Photovoltaikanlagen.
* Erfordert oft teure Phasenkoppler im Sicherungskasten.

Ein reiner Betrieb über herkömmliches WLAN hingegen erzeugt eine dauerhafte Abhängigkeit vom WLAN-Router und dessen Verfügbarkeit.

**ESP-NOW** ist der moderne, optimale Mittelweg: Es baut ein extrem robustes, schnelles und direktes 2,4-GHz-Funknetzwerk zwischen den Lüftern auf — ohne Leitungen, ohne Phasenprobleme und mit voller lokaler Autonomie.

---

## ⚡ Zentrale Vorteile

* 🌐 **Vollständige WLAN-Unabhängigkeit**: Die Geräte benötigen keinen aktiven WLAN-Router für den synchronen Lüftungsbetrieb. Fällt das Heim-WLAN aus, arbeitet die Raum-Lüftungsgruppe ungestört weiter.
* 🛡️ **Netzwerk-Immunität**: Direkte Punkt-zu-Punkt-Kommunikation ist immun gegen Überlastungen oder Störungen im gewöhnlichen Heimnetzwerk.
* ⚡ **Nahezu verzögerungsfreie Übertragung**: Da nach der Discovery keine TCP/IP-Handshakes erforderlich sind, werden Richtungswechsel und Taktzyklen synchron auf die Millisekunde genau übertragen.
* 🔌 **Keine Steuerleitungen**: Kein Verlegen von Datenkabeln zwischen Wänden oder Räumen erforderlich.
* 📡 **Dynamische Discovery & NVS-Persistenz**: Geräte im selben Raum finden sich automatisch und speichern die MAC-Adressen der Partnergeräte dauerhaft im Flash-Speicher (NVS).
  > [!NOTE]
  > Durch die 254-Zeichen-Begrenzung in ESPHome Globals ist die persistente Peer-Liste für **bis zu ca. 14 Peers** pro Gerät ausgelegt — mehr als ausreichend für jeden Wohnraum.
* ⚙️ **Effiziente Unicast-Kommunikation**: Nach der ersten Erkennung erfolgen alle Sync-, Status- und PID-Datenübertragungen über gezielte Unicast-Pakete, was das 2,4-GHz-Frequenzband schont und die Stabilität maximiert.
* 🔄 **Echtzeit-Spiegelung von Einstellungen**: Änderungen an Parametern (CO2-Grenzwerte, Lüfterstufen, Betriebsmodi) an einem Gerät werden sofort drahtlos auf alle Peers der Raumgruppe übertragen.

---

## 🔍 Discovery- & Kopplungs-Ablauf

```mermaid
sequenceDiagram
    autonumber
    participant D1 as Gerät 1 (Neu / Start)
    participant D2 as Gerät 2 (Bestehender Peer)

    D1->>D2: Broadcast ROOM_DISC (Floor ID, Room ID)
    Note over D2: Prüfung: Stimmen Floor ID & Room ID überein?
    D2->>D1: Unicast ROOM_CONF (MAC, Floor ID, Room ID)
    Note over D1,D2: MAC-Adressen im Flash (NVS) speichern
    loop Normalbetrieb
        D1->>D2: Unicast Sync / Modus / PID-Bedarf
        D2->>D1: Unicast Status & Heartbeat
    end
```

1. **Broadcast (`ROOM_DISC`)**: Ein startendes Gerät sendet ein Suchpaket an alle (`FF:FF:FF:FF:FF:FF`).
2. **Matching**: Empfänger prüfen, ob Etagen- und Raum-ID mit den eigenen Parametern übereinstimmen.
3. **Handshake (`ROOM_CONF`)**: Bei Übereinstimmung wird der Sender gespeichert und eine Unicast-Bestätigung zurückgesendet.
4. **Persistenz**: Die Peer-Liste übersteht Neustarts und ermöglicht sofortige Synchronisation direkt nach dem Booten.

---

## 🔒 Protokoll-Architektur & Paketvalidierung (v12)

* **Magic Header**: Jedes Datenpaket beginnt mit einem festen Prüfbyte (`0x42`), um fremde oder fehlerhafte 2,4-GHz-Pakete sofort zu verwerfen.
* **Firmware-Versionsprüfung**: Strikte Validierung verhindert Fehlinterpretationen bei unterschiedlichen Firmware-Ständen.
* **Schleifen-Schutz**: Übernommene Werte werden ohne neuen Broadcast angewendet (`notify = false`), und Geräte senden nur ihre **eigenen** Eingangswerte, nie einen aus Peers fusionierten Wert — gespiegelte Einstellungen und raumweite Maxima können daher nicht zwischen Geräten hin- und herspringen oder sich gegenseitig festhalten. (Sequenznummern gibt es im Paket nicht.)
* **Stufen-Übersteuerung (v12)**: Jedes Paket trägt die Stufen-Übersteuerung der Smart-Automatik des Raums (`auto_override_level`, `auto_override_remaining_s`). `MSG_STATE` eines beliebigen Geräts startet, ändert oder beendet sie; der Master-Heartbeat startet bzw. synchronisiert sie nur. Firmware ≥ 0.10.34 verwirft v11-Pakete — **alle Geräte eines Raums flashen**.

---

## ⚠️ Sicherheit: keine Verschlüsselung, keine Authentifizierung

Der ESP-NOW-Verkehr zwischen VentoSync-Geräten ist **weder verschlüsselt noch authentifiziert**. Die Prüfungen oben (Magic Byte, Protokollversion, Paketgröße, Wertebereiche, Etagen-/Raum-ID) verwerfen nur fremde oder fehlerhafte Pakete — gegen einen gezielten Angreifer schützen sie nicht:

* **Mitlesbar**: Jeder in Funkreichweite (typisch ein Gebäude, mit Richtantenne mehr) kann die Pakete empfangen und Modus, Lüfterstufe, CO2, Luftfeuchte und Temperaturen lesen.
* **Fälschbar**: Das Paketformat ist öffentlich (dieses Repository). Ein ESP32 in der Nähe kann passende Pakete mit beliebiger Absender-MAC senden und den Modus des Raums umschalten (z. B. `Aus`), die Stufe oder die raumweiten Einstellungen ändern (Grenzen der Smart-Automatik, CO2-/Feuchte-Schwellen, Klima-Koordination). Etagen- und Raum-ID lassen sich aus den Paketen des Geräts selbst ablesen.
* **Nicht betroffen**: WLAN, die Home-Assistant-API und OTA-Updates sind getrennte Kanäle (siehe *OTA-Sicherheit* in der README); über ESP-NOW lässt sich keine Firmware austauschen. Die Übertemperatur-Abschaltung läuft lokal auf jedem Gerät. Die Fenstersperre wertet auch das Fenster-Flag der Peers aus, ein gefälschtes Paket kann die Lüfter also ebenfalls anhalten.
* **Warum keine ESP-NOW-Verschlüsselung?** ESPHome 2026.9.1 unterstützt sie nicht (Peers werden immer unverschlüsselt angelegt), und die eingebaute ESP-NOW-Verschlüsselung deckt nur Unicast ab (Discovery und Statusabfragen sind Broadcasts) und ist auf wenige verschlüsselte Peers begrenzt. Ein in die öffentlichen Release-Binaries kompilierter Schlüssel wäre zudem für jeden auslesbar. Eine Nachrichten-Authentifizierung mit einem Schlüssel pro Installation wird geprüft.

**Empfehlung:** ESP-NOW wie einen ungeschützten lokalen Bus behandeln. Für ein Einfamilienhaus ist das unkritisch; bei exponierten Installationen (Mehrparteienhaus mit Nachbarwohnungen in Funkreichweite, Büros) das Risiko abwägen — ein gefälschtes Paket kann die Lüftung ausschalten, bis jemand sie wieder einschaltet.

---

## 📶 Funk- & Antennen-Optimierung

Zur Gewährleistung maximaler Funkreichweite durch Betonwände und Decken:
* **Externe U.FL-Antenne**: Der ESP32-C6 nutzt eine externe Rundstrahlantenne über einen U.FL-Konnektor.
* **Antennen-Routing**: Die ESPHome-Firmware ist so konfiguriert, dass alle Funksignale über die externe Antenne statt über die interne PCB-Antenne geleitet werden, was die Signalstärke und Verbindungsqualität deutlich verbessert.
