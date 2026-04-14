# 🌴 Home Assistant Konfiguration: Urlaubsmodus (Vacation Mode)

Um den neuen globalen Urlaubsmodus für dein VentoSync-System zu aktivieren, musst du in Home Assistant lediglich einen einfachen **Schalter-Helfer** (Input Boolean) anlegen. Dieser eine Schalter steuert den Urlaubsmodus global für all deine VentoSync-Geräte gleichzeitig.

**Standard-Entity-ID:** `input_boolean.ventosync_vacation_mode`

## So legst du den Helfer an

### Option A: Über die Benutzeroberfläche (Empfohlen)
1. Gehe zu **Einstellungen** > **Geräte & Dienste** > **Helfer**.
2. Klicke auf **Helfer erstellen** > **Schalter (Toggle)**.
3. **Name**: `VentoSync Vacation Mode` (oder "VentoSync Urlaubsmodus")
4. **Symbol**: `mdi:palm-tree` (Optional, aber passend)
5. **Entitäts-ID**: Nach dem Erstellen generiert Home Assistant die Entitäts-ID basierend auf dem Namen. Klicke auf das Zahnrad des neuen Helfers, um sicherzustellen, dass die ID exakt `input_boolean.ventosync_vacation_mode` lautet. Passe sie, wenn nötig, manuell an.

### Option B: Über die `configuration.yaml`
Füge folgenden Code in deine Home Assistant Konfiguration ein (falls du die YAML-Methode bevorzugst):

```yaml
input_boolean:
  ventosync_vacation_mode:
    name: "VentoSync Urlaubsmodus"
    icon: mdi:palm-tree
```

## Wie es funktioniert
- **Logik:** Wird dieser Schalter auf `an` (on) gesetzt, geht das System global in den Urlaubsmodus über.
- **VentoSync Reaktion:** All deine VentoSync Lüfter merken sich sofort ihren momentanen Modus sowie die eingestellte Lüfterstufe. Daraufhin wechseln sie in die stromsparende **Stoßlüftung** auf der niedrigsten **Stufe 1**, um den minimalen Luftaustausch bei Abwesenheit sicherzustellen.
- **Resume:** Kommst du aus dem Urlaub zurück und stellst den Schalter auf `aus` (off), stellt jedes Gerät nahtlos wieder exakt den vorherigen Modus samt Intensität her.
