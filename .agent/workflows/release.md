---
description: Analysiere Änderungen, generiere ausführlichen Changelog und starte ESPHome Build für Büro
---

# Workflow: Release Build

Wenn dieser Workflow aufgerufen wird, führst du als KI-Assistent vollautomatisch die Vorbereitung und den Start des lokalen Firmware-Builds durch.

## Ablauf:

1. **Änderungen analysieren:**
   Führe per Terminal die Befehle `git log -n 3 --oneline` sowie `git diff HEAD` aus. Analysiere, woran der User zuletzt gearbeitet hat.
2. **CHANGELOG.md analysieren (Ressourcenschonend):**
   Nutze das Terminal (`run_command`), um mit `tail -n 80 CHANGELOG.md` ausschließlich die letzten 80 Zeilen der Changelog-Datei zu lesen. **Lies auf keinen Fall die komplette Datei ein**, um Tokens zu sparen. Verstehe anhand des Ausschnitts das gewünschte Format.
3. **Dateien aktualisieren:**
   *   **version.json**: Erstelle einen sehr kurzen, prägnanten englischen Satz (ca. 5-12 Wörter). Überschreibe damit den Wert des `"description"`-Feldes in `version.json`. **WICHTIG: Erhöhe NICHT den Wert für "version". Der Version-Bump geschieht automatisch beim Build!**
   *   **CHANGELOG.md**: Schreibe einen **ausführlichen, detaillierten** Changelog-Eintrag zu deinen analysierten Code-Änderungen (so detailliert wie bisher auch). Leite für den Titel des Eintrags (z.B. `## [0.8.252] - YYYY-MM-DD`) die *nächste* Versionsnummer ab, indem du die Patch-Version aus der `version.json` im Kopf um 1 erhöhst. Füge diesen Eintrag oben an der passenden Stelle in der `CHANGELOG.md` ein.
4. **Kompilierung & Git Push:**
   Generiere eine kurze, passende englische Commit-Nachricht für die Code-Änderungen (z.B. `feat: ...` oder `fix: ...`).
   Starte den Build-Prozess und verknüpfe ihn per `&&` mit dem automatischen Git Push, damit alles nahtlos im Hintergrund durchlaufen kann, falls der Build erfolgreich ist.
   // turbo
   Führe den folgenden Befehl im Terminal aus (ersetze `DEINE_MESSAGE` durch deine generierte Nachricht):
   `esphome run ventosync_nosensor.yaml --device 192.168.178.206 --no-logs && git add . && git commit -m "DEINE_MESSAGE" && git push`
   Setze `SafeToAutoRun` auf `true`.

Zeige dem User abschließend kurz die generierte Description, den neuen CHANGELOG-Eintrag sowie die Commit-Nachricht, und melde, dass der Build inkl. anschließendem Push im Hintergrund gestartet wurde.
