# 📘 VentoSync – Bedienungsanleitung am Lüftungsgerät

[![Language: EN](https://img.shields.io/badge/Language-EN-blue.svg)](../en/en_device-user-guide.md)

Diese Anleitung beschreibt **ausschließlich die Bedienung direkt am Lüftungsgerät**: die drei Tasten, die
neun Leuchtanzeigen und die Blinkcodes. Einrichtung, Home Assistant und das Web-Dashboard sind in den
übrigen Dokumenten beschrieben (siehe [Weiterführende Dokumente](#-weiterführende-dokumente)).

<p align="center">
  <img src="../../images/Ventomax%20V-WRG-1/PXL_20260128_232625674.jpg" alt="VentoSync Bedienpanel am Lüftungsgerät" width="500" />
</p>

---

## 📑 Inhalt

1. [Das Wichtigste in Kürze](#-das-wichtigste-in-kürze)
2. [Bedienfeld im Überblick](#-bedienfeld-im-überblick)
3. [Die Tasten](#-die-tasten)
4. [Die Betriebsmodi](#-die-betriebsmodi)
5. [Die Lüfterstufe (1–10)](#-die-lüfterstufe-110)
6. [Leuchtanzeigen (LEDs) lesen](#-leuchtanzeigen-leds-lesen)
7. [Fehler- und Warncodes der Master-LED](#-fehler--und-warncodes-der-master-led)
8. [Kindersicherung](#-kindersicherung)
9. [Mehrere Geräte in einem Raum](#-mehrere-geräte-in-einem-raum)
10. [Einschalten, Neustart und Stromausfall](#-einschalten-neustart-und-stromausfall)
11. [Sonderzustände, die das Gerät von selbst einnimmt](#-sonderzustände-die-das-gerät-von-selbst-einnimmt)
12. [Fehlerbehebung](#-fehlerbehebung)
13. [Sicherheitshinweise](#️-sicherheitshinweise)

---

## ⚡ Das Wichtigste in Kürze

| Ich möchte … | So geht's |
| :--- | :--- |
| die Lüftung im Raum **aus- oder einschalten** | **Power-Taste** kurz drücken |
| den **Betriebsmodus** wechseln | **Modus-Taste (M)** kurz drücken – jeder Druck schaltet einen Modus weiter |
| **stärker oder schwächer** lüften | **Stufen-Taste (+)** kurz drücken (eine Stufe weiter) oder gedrückt halten (Stufen laufen durch) |
| die **Tasten sperren / entsperren** (Kindersicherung) | **Modus-Taste** ca. **5 Sekunden** gedrückt halten |
| das Gerät **neu starten** | **Power-Taste** länger als **10 Sekunden** gedrückt halten |
| den **aktuellen Zustand sehen**, obwohl die LEDs dunkel sind | Eine Taste drücken – oder einfach abwarten, bis sich im Raum etwas ändert |

> [!IMPORTANT]
> **Alle Einstellungen gelten für den ganzen Raum.** Sind mehrere VentoSync-Geräte im selben Raum
> eingerichtet, übernehmen alle Geräte die Änderung, egal an welchem Gerät du drückst. Zu- und Abluft
> bleiben so immer im Gleichgewicht.

---

## 🖐️ Bedienfeld im Überblick

Das originale VentoMaxx-Bedienfeld bleibt erhalten:

- **3 Tasten:** Power (I/O), Modus (M), Stufe (+)
- **9 grüne LEDs:**

| LED | Anzahl | Aufgabe |
| :--- | :---: | :--- |
| **Power-LED** | 1 | Betriebsbereitschaft. Leuchtet hell nach einer Bedienung, danach dauerhaft schwach (10 %). |
| **Master-LED** (mittig) | 1 | Leuchtet schwach am **Master-Gerät** des Raums (Geräte-Nr. 1); zeigt an **allen** Geräten Fehler und Warnungen per Blinkcode. |
| **Modus-LED links** („WRG“) | 1 | Zeigt zusammen mit der rechten Modus-LED den Betriebsmodus. |
| **Modus-LED rechts** („VEN“) | 1 | Zeigt zusammen mit der linken Modus-LED den Betriebsmodus. |
| **Stufen-LEDs** | 5 | Balkenanzeige für die Lüfterstufe 1–10. |

> [!NOTE]
> **Dunkles Bedienfeld ist normal.** 30 Sekunden nach der letzten Bedienung schalten sich Modus- und
> Stufen-LEDs aus und die Power-LED wird auf 10 % gedimmt („Unauffälligkeitsmodus“), damit das Gerät
> nachts nicht stört. Das Gerät läuft dabei unverändert weiter.

---

## 🔘 Die Tasten

### Power (I/O) – Raum ein/aus

| Bedienung | Wirkung |
| :--- | :--- |
| **Kurz drücken** (bis 10 s) | Ist der Raum **an**: schaltet in den Modus **Aus** (Lüfter stehen). <br>Ist der Raum **aus**: schaltet zurück in den **zuletzt genutzten Modus** (ab Werk: Smart-Automatik). |
| **Sehr lange drücken** (über 10 s) | **Neustart** des Geräts. Modus und Einstellungen bleiben erhalten. Hilfreich, wenn das Gerät nicht mehr reagiert oder dauerhaft einen Verbindungsfehler meldet. |

> Im Modus **Aus** bleiben Sensoren, WLAN und die Verbindung zu den anderen Geräten aktiv – das Gerät misst
> weiter, nur die Lüfter stehen. Einen Schlaf- oder Energiesparzustand gibt es nicht.

### Modus (M) – Betriebsmodus wählen

| Bedienung | Wirkung |
| :--- | :--- |
| **Kurz drücken** | Schaltet zum nächsten Modus:<br>**Smart-Automatik → Wärmerückgewinnung → Durchlüften → Stoßlüftung → Aus → Smart-Automatik …** |
| **ca. 5 s gedrückt halten** | Schaltet die **Kindersicherung** ein oder aus (siehe [Kindersicherung](#-kindersicherung)). Der Modus wird dabei **nicht** gewechselt. |

### Stufe (+) – Lüfterstärke

| Bedienung | Wirkung |
| :--- | :--- |
| **Kurz drücken** | Eine Stufe höher (1 → 2 → … → 10 → 1). |
| **Gedrückt halten** | Nach ca. 0,75 s laufen die Stufen automatisch durch (etwa 2 Stufen pro Sekunde): hoch bis 10, dann wieder abwärts bis 1 usw. **Loslassen**, wenn die gewünschte Stufe erreicht ist. |

Gut zu wissen:

- Im Modus **Aus** reagiert die Stufen-Taste nicht – erst einschalten.
- In den ersten **10 Sekunden nach dem Einschalten / Neustart** wird die Stufen-Taste ignoriert.
- In der **Smart-Automatik** regelt das Gerät die Stufe selbst. Eine manuell gewählte Stufe wird von der
  Automatik in den nächsten Regelzyklen (alle 10 s, je eine Stufe) wieder angepasst. Wer eine feste Stufe
  möchte, wählt **Wärmerückgewinnung**.

> [!TIP]
> Jeder Tastendruck wirkt **sofort**, auch wenn das Bedienfeld gerade dunkel ist – es gibt keinen „Aufweck-Druck“.
> Wer nur den Zustand sehen will, ohne etwas zu verändern, drückt am besten nichts und schaut in Home
> Assistant oder im Web-Dashboard nach.

---

## 🔄 Die Betriebsmodi

| # | Modus | Modus-LED links (WRG) | Modus-LED rechts (VEN) | Was passiert |
| :-: | :--- | :---: | :---: | :--- |
| 1 | **Smart-Automatik** *(Standard)* | 🟢 **pulsiert langsam** | ⚫ | Das Gerät lüftet selbstständig mit Wärmerückgewinnung und regelt die Stufe nach CO2 und Luftfeuchte. |
| 2 | **Wärmerückgewinnung** | 🟢 an | ⚫ | Wechselbetrieb mit Wärmerückgewinnung auf der von dir gewählten, festen Stufe. |
| 3 | **Durchlüften** | 🟢 an | 🟢 an | Dauerhafter Luftstrom in eine Richtung (Querlüftung, z. B. zur Abkühlung im Sommer). Endet ab Werk nach **30 Minuten**. |
| 4 | **Stoßlüftung** | ⚫ | 🟢 an | 15 Minuten kräftig in eine Richtung lüften, dann 105 Minuten Pause – im Wechsel. |
| 5 | **Aus** | ⚫ | ⚫ | Lüfter stehen. Nur die Power-LED leuchtet. |

### 1. Smart-Automatik (empfohlen)

- Lüftet im Wechselbetrieb mit Wärmerückgewinnung auf einer niedrigen Grundstufe (ab Werk Stufe 2).
- Steigen **CO2** oder **Luftfeuchte** an, erhöht das Gerät die Stufe langsam und sanft (höchstens eine Stufe
  alle 10 Sekunden; ab Werk bis maximal Stufe 7) und senkt sie wieder, sobald die Luft besser ist.
- Ist es im Sommer draußen deutlich kühler als drinnen, kann die Automatik selbstständig auf **Durchlüften**
  umschalten. Die linke Modus-LED **pulsiert dabei weiter** – das Gerät bleibt in der Smart-Automatik.
- Auch Geräte ohne eigene Sensoren regeln mit, weil sie den Bedarf der anderen Geräte im Raum übernehmen.

### 2. Wärmerückgewinnung

- Die Luftrichtung wechselt regelmäßig: je Richtung 70 s (Stufe 1) bis 50 s (Stufe 10). Der Keramikspeicher
  hält dabei die Wärme im Raum.
- Bei jedem Richtungswechsel läuft der Lüfter sanft aus und wieder an (je ca. 5 s) – eine kurze Stille ist normal.
- Die Stufe bleibt fest, bis du sie änderst.

### 3. Durchlüften

- Kein Richtungswechsel: Ein Gerät zieht Außenluft herein, das Partnergerät bläst Raumluft hinaus.
- **Zeitbegrenzung:** Ab Werk endet das Durchlüften nach **30 Minuten**; danach schaltet der Raum automatisch
  in die **Wärmerückgewinnung** (die rechte Modus-LED geht aus). Die Dauer (0–120 min, 0 = unbegrenzt)
  wird in Home Assistant oder im Web-Dashboard eingestellt.
- Keine Wärmerückgewinnung – im Winter nur kurz verwenden.

### 4. Stoßlüftung

- Ablauf alle 2 Stunden: **15 Minuten Lüften** in eine Richtung auf der eingestellten Stufe, danach
  **105 Minuten Pause** (Lüfter stehen). Jeder zweite Durchgang lüftet in die Gegenrichtung.
- **Stillstand ist hier normal:** Die meiste Zeit stehen die Lüfter, obwohl die Modus-LED leuchtet.
- Die Stärke des Durchgangs bestimmt die **Stufen-Taste**. Für eine kräftige Stoßlüftung eine hohe Stufe wählen.
- Läuft unbegrenzt, bis ein anderer Modus gewählt wird.

### 5. Aus

- Die Lüfter **aller Geräte des Raums** stehen. Sensoren und Funkverbindung bleiben aktiv.
- Einschalten mit der **Power-Taste** (zurück in den letzten Modus) oder der **Modus-Taste** (startet mit
  der Smart-Automatik).

---

## 📶 Die Lüfterstufe (1–10)

Die 10 Stufen werden über die **5 Stufen-LEDs** angezeigt. Jede LED steht für zwei Stufen:
**schwach leuchtend = ungerade Stufe**, **hell = gerade Stufe**.

| Stufe | LED 1 | LED 2 | LED 3 | LED 4 | LED 5 |
| :-: | :-: | :-: | :-: | :-: | :-: |
| 1 | ◐ | ○ | ○ | ○ | ○ |
| 2 | ● | ○ | ○ | ○ | ○ |
| 3 | ● | ◐ | ○ | ○ | ○ |
| 4 | ● | ● | ○ | ○ | ○ |
| 5 | ● | ● | ◐ | ○ | ○ |
| 6 | ● | ● | ● | ○ | ○ |
| 7 | ● | ● | ● | ◐ | ○ |
| 8 | ● | ● | ● | ● | ○ |
| 9 | ● | ● | ● | ● | ◐ |
| 10 | ● | ● | ● | ● | ● |

● hell · ◐ schwach (ca. 20 %) · ○ aus

Gut zu wissen:

- Die LEDs zeigen sofort die **Zielstufe**. Der Lüfter fährt sanft innerhalb weniger Sekunden dorthin.
- In der Smart-Automatik zeigen die LEDs die **von der Automatik gewählte** Stufe.
- Ist in Home Assistant eine **Anwesenheits-Anpassung** eingestellt, läuft der Lüfter bei erkannter
  Anwesenheit etwas stärker oder schwächer; die LEDs zeigen weiterhin die eingestellte Grundstufe.
- Im Modus **Aus** sind alle Stufen-LEDs dunkel.

---

## 🔆 Leuchtanzeigen (LEDs) lesen

### Normale Anzeige

| Situation | Power | Master | Modus-LEDs | Stufen-LEDs |
| :--- | :---: | :---: | :---: | :---: |
| Nach einer Bedienung (30 s lang) | hell | Master-Gerät: schwach an · andere: aus | Modus (siehe oben) | Stufe |
| Ruhezustand (nach 30 s) | schwach (10 %) | Master-Gerät: schwach an · andere: aus | aus | aus |
| Modus **Aus** | hell, dann schwach | wie oben | aus | aus |
| Beim Einschalten / Neustart (ca. 3 s) | alle LEDs leuchten hell (LED-Selbsttest) | | | |

- Die **maximale LED-Helligkeit** (5–100 %, ab Werk 80 %) wird in Home Assistant oder im Web-Dashboard
  eingestellt und gilt für den ganzen Raum.
- Ändert jemand an einem anderen Gerät des Raums Modus oder Stufe, **leuchten alle Geräte des Raums
  gemeinsam auf** und zeigen den neuen Zustand 30 Sekunden lang.

### Kurze Blinksignale aller LEDs

| Signal | Bedeutung |
| :--- | :--- |
| Alle LEDs blinken **2× ruhig** | Kindersicherung wurde **ein- oder ausgeschaltet** (Bestätigung). |
| Alle LEDs blinken **3× schnell** | Taste ist **gesperrt** – die Kindersicherung ist aktiv, der Tastendruck wurde ignoriert. |

---

## 🚨 Fehler- und Warncodes der Master-LED

Störungen und Sonderzustände zeigt die **mittlere Master-LED** über ein sich wiederholendes Blinkmuster.
Die Codes erscheinen auf **jedem** Gerät (nicht nur am Master) und unabhängig davon, ob das Bedienfeld
gerade gedimmt ist.

| Muster | Bedeutung | Ursache | Was tun? |
| :--- | :--- | :--- | :--- |
| **2× lang** (je 0,5 s), dann 5 s Pause | **Verbindung zu Partnergerät verloren** | Ein Gerät desselben Raums hat sich seit über 3 Minuten nicht gemeldet (z. B. stromlos, defekt, außer Reichweite). Nur aktiv, wenn die „ESP-NOW Peerprüfung“ in Home Assistant eingeschaltet ist (ab Werk an); frühestens 3 Minuten nach dem Start. | Prüfen, ob alle Geräte des Raums Strom haben (Power-LED leuchtet?). Ggf. das betroffene Gerät neu starten (Power > 10 s). Wird ein Gerät dauerhaft nicht mehr betrieben, die Peerprüfung in Home Assistant abschalten. |
| **3× lang** (je 0,5 s), dann 5 s Pause | **WLAN-Verbindung verloren** | Das Gerät ist seit über 30 Sekunden weder mit dem WLAN noch mit Home Assistant verbunden. | Router / WLAN prüfen. Die Lüftung läuft **normal weiter**, auch die Abstimmung der Geräte im Raum funktioniert ohne WLAN. Kommt das Gerät gar nicht mehr ins WLAN, öffnet es einen eigenen Hotspot zur Neueinrichtung (siehe [Fehlerbehebung](#-fehlerbehebung)). |
| **4× sehr schnell** (je 0,1 s), dann 2 s Pause, **volle Helligkeit** | **Hitzewarnung** | Die Temperatur im Gerät liegt bei **50–60 °C** (Netzteil-Grenzbereich). | Ursache für die Wärme suchen (direkte Sonne auf das Innenteil, Heizung, verdeckte Lüftungsschlitze). Die Warnung erlischt automatisch unter 48 °C. **Ab 60 °C schaltet das Gerät zum Schutz ab** (siehe unten). |
| **Langsames Pulsieren** (1 s an, 2 s aus) | **Fenster offen – Lüftung pausiert** | Home Assistant meldet ein offenes Fenster im Raum; nach 5 s stoppen alle Lüfter des Raums. Das Pulsieren erscheint nur ca. **30 Sekunden** lang (danach aus, um nachts nicht zu stören) – die Pause bleibt aber bestehen. | Fenster schließen – die Lüftung läuft automatisch im eingestellten Modus weiter. |
| **Dauerhaft schwach an** | **Kein Fehler** – dieses Gerät ist der **Master** des Raums | – | – |
| **Aus** | **Kein Fehler** (Nicht-Master-Gerät) | – | – |

**Rangfolge:** Liegen mehrere Zustände gleichzeitig vor, zeigt die LED nur den wichtigsten:
Verbindung zu Partnergerät (2×) → WLAN (3×) → Hitze (4×) → Fenster offen.

### Not-Abschaltung bei Überhitzung (über 60 °C)

Erreicht die Temperatur im Gerät **60 °C**, wird der Lüfter gestoppt und das Gerät schaltet sich für
**60 Minuten komplett ab**. In dieser Zeit sind **alle LEDs dunkel** und die Tasten reagieren nicht. Danach
startet das Gerät von selbst neu (LED-Selbsttest). Ist es dann noch immer zu heiß, wiederholt sich der Vorgang.

> [!WARNING]
> Ein Gerät, das wiederholt komplett dunkel wird, kann überhitzen. Ursache beseitigen oder eine
> Elektrofachkraft hinzuziehen. **Gehäuse nicht unter Spannung öffnen.**

---

## 🔒 Kindersicherung

Die Kindersicherung sperrt **alle drei Tasten** am Gerät. Home Assistant und das Web-Dashboard bleiben
voll bedienbar.

| Aktion | Bedienung | Rückmeldung |
| :--- | :--- | :--- |
| **Einschalten** | **Modus-Taste** ca. **5 Sekunden** gedrückt halten | alle LEDs blinken **2×** |
| **Ausschalten** | **Modus-Taste** erneut ca. **5 Sekunden** gedrückt halten | alle LEDs blinken **2×** |
| Taste drücken, während gesperrt | – | alle LEDs blinken **3× schnell**, nichts passiert |

- Die Modus-Taste bleibt auch bei aktiver Sperre für das 5-Sekunden-Halten nutzbar – nur so lässt sich die
  Sperre am Gerät wieder lösen.
- Ob die Sperre gerade an oder aus ist, erkennst du am Gerät nur durch einen kurzen Tastendruck
  (3× Blinken = gesperrt). Alternativ: Schalter „Kindersicherung“ in Home Assistant.
- Die Sperre bleibt nach Neustart und Stromausfall erhalten.
- Die Kindersicherung gilt **je Gerät**, nicht für den ganzen Raum.
- **Ausnahme:** Der Neustart per Power-Taste (> 10 s) funktioniert auch bei aktiver Sperre.

---

## 🔗 Mehrere Geräte in einem Raum

Lüftungsgeräte arbeiten paarweise (eines bläst hinein, das andere hinaus). VentoSync-Geräte desselben Raums
stimmen sich per Funk direkt ab – auch ohne WLAN-Router.

- **Bedienung an einem beliebigen Gerät genügt:** Modus, Stufe und Aus/Ein werden auf alle Geräte des Raums übertragen.
- Alle Geräte des Raums **leuchten gemeinsam auf**, wenn sich etwas ändert.
- Das Gerät mit der **Nummer 1** ist der **Master** (Master-LED schwach an). Er gibt in der Smart-Automatik die
  Stufe vor und den Takt der Stoßlüftung.
- Fällt ein Gerät aus, laufen die übrigen weiter; bei eingeschalteter Peerprüfung meldet die Master-LED **2×**.

---

## 🔌 Einschalten, Neustart und Stromausfall

1. Nach dem Einschalten (oder einem Neustart) leuchten **alle LEDs ca. 3 Sekunden hell** (Selbsttest).
2. Danach zeigt das Bedienfeld den aktuellen Modus und die Stufe und dimmt nach 30 s ab.
3. In den ersten 10 Sekunden wird die Stufen-Taste ignoriert.
4. Modus, Stufe und Kindersicherung bleiben über Neustart und Stromausfall erhalten. Ein Gerät, das neu
   startet, übernimmt den Zustand der anderen Geräte im Raum.

---

## 🌙 Sonderzustände, die das Gerät von selbst einnimmt

Manche Änderungen kommen nicht von den Tasten, sondern von Home Assistant oder der Automatik.
So erkennst du sie am Gerät:

| Zustand | Erkennbar am Gerät | Hinweis |
| :--- | :--- | :--- |
| **Fenster offen** (Fenstersperre) | Lüfter stehen, Master-LED pulsiert ca. 30 s lang | Läuft nach dem Schließen automatisch weiter. Einzelne Geräte können in Home Assistant von der Sperre ausgenommen sein. |
| **Durchlüften-Timer abgelaufen** | Rechte Modus-LED aus, linke an → Wärmerückgewinnung | Normal, siehe [Durchlüften](#3-durchlüften). |
| **Sommerkühlung** in der Smart-Automatik | Konstanter Luftstrom, linke Modus-LED pulsiert weiter | Normal, die Automatik kehrt selbst zur Wärmerückgewinnung zurück. |
| **Urlaubsmodus** (über Home Assistant) | Ab Werk Stoßlüftung auf Stufe 1 (rechte Modus-LED, Stufen-LED 1 schwach) | Wird über Home Assistant beendet; danach stellt das Gerät den vorherigen Modus wieder her. |
| **Klimaanlage aktiv** (Klima-Koordination) | Smart-Automatik lüftet höchstens auf Stufe 3 | Bei sehr hohem CO2 oder hoher Feuchte regelt das Gerät trotzdem hoch. |
| **Überhitzung** | Master-LED 4× schnell bzw. Gerät komplett dunkel | Siehe [Not-Abschaltung](#not-abschaltung-bei-überhitzung-über-60-c). |

> Der **Filterwechsel-Hinweis** (nach ca. 1 Jahr Laufzeit bzw. 3 Jahren) wird **nicht** am Gerät angezeigt,
> sondern in Home Assistant gemeldet.

---

## 🛠️ Fehlerbehebung

| Problem | Mögliche Ursache | Lösung |
| :--- | :--- | :--- |
| Bedienfeld ist dunkel, nur Power-LED schwach | Normaler Ruhezustand nach 30 s | Kein Fehler. |
| **Alle** LEDs dunkel, auch die Power-LED | Kein Strom, oder Not-Abschaltung wegen Überhitzung (bis 60 min) | Sicherung prüfen. Bei Überhitzung abwarten und Ursache beseitigen. |
| Lüfter steht, Modus-LED leuchtet | Stoßlüftung in der Pause, Fenster offen, oder kurzer Richtungswechsel (Wärmerückgewinnung) | Normal. |
| Lüfter steht, beide Modus-LEDs aus | Modus **Aus** | Power-Taste kurz drücken. |
| Stufen-Taste reagiert nicht | Modus **Aus**, Gerät gerade gestartet (< 10 s) oder Kindersicherung (3× Blinken) | Einschalten / kurz warten / Sperre lösen. |
| Stufe springt in der Smart-Automatik zurück | Die Automatik regelt die Stufe selbst | Für eine feste Stufe **Wärmerückgewinnung** wählen. |
| Durchlüften endet „von selbst“ | 30-Minuten-Timer abgelaufen | Normal; Dauer in Home Assistant / Web-Dashboard ändern (0 = unbegrenzt). |
| Alle LEDs blinken 3× bei jedem Tastendruck | Kindersicherung aktiv | Modus-Taste 5 s halten. |
| Ein anderes Gerät im Raum ändert sich mit | Raumweite Abstimmung | Normal und gewollt. |
| Master-LED blinkt 3× dauerhaft | Kein WLAN | Router prüfen. Falls das WLAN geändert wurde: mit dem Handy das WLAN **„VentoSync Hotspot“** (Passwort `ventosync`) suchen, verbinden und im sich öffnenden Fenster (sonst `192.168.4.1`) das Heim-WLAN neu eintragen. |
| Master-LED blinkt 2× dauerhaft | Partnergerät meldet sich nicht | Partnergerät prüfen / neu starten. |
| Gerät reagiert gar nicht mehr | Software hängt | Power-Taste > 10 s halten (Neustart). Hilft das nicht: Sicherung kurz aus- und wieder einschalten. |

---

## ⚠️ Sicherheitshinweise

> [!CAUTION]
> Im Gerät befindet sich ein Netzteil mit **230 V Netzspannung**. Gehäuse nur im spannungsfreien Zustand
> öffnen (Leitungsschutzschalter aus). Arbeiten am Netzanschluss nur durch eine **Elektrofachkraft**.

- Lüftungsöffnungen nicht abdecken.
- Ein Gerät, das sich wiederholt wegen Überhitzung abschaltet (Master-LED 4× oder komplett dunkel), nicht
  dauerhaft weiter betreiben, ohne die Ursache zu klären.
- VentoSync ist eine unabhängige Open-Source-Steuerung und steht in keiner Verbindung zur VentoMaxx GmbH.

---

## 📚 Weiterführende Dokumente

- [Bedienungsanleitung Lüftungsgerät (technische Details)](de_control-panel-operation.md)
- [Betriebsmodi & Programmlogik](de_operating-modes.md)
- [Kindersicherung](de_child-lock-implementation.md)
- [Fenstersperre einrichten](de_window-guard-ha-setup.md)
- [Urlaubsmodus einrichten](de_vacation-mode-ha-setup.md)
- [Lokales Web-Dashboard](de_local-web-dashboard.md)
- [Home-Assistant-Entitäten](de_home-assistant-entities.md)
