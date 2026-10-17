# VentoSync – Home-Assistant-Dokumentation

Docusaurus-Site im Stil von [developers.home-assistant.io](https://developers.home-assistant.io/), die alle
Home-Assistant-Entitäten, API-Actions, Events und Eingänge der VentoSync-Firmware beschreibt (Deutsch).

Die Seiten liegen als Markdown unter [`docs/`](docs/) und sind auch direkt auf GitHub lesbar
(Admonitions wie `:::tip` erscheinen dort als Klartext). Einstieg: [`docs/intro.md`](docs/intro.md).

## Lokal ansehen

Voraussetzung: Node.js ≥ 20.

```bash
cd documentation/ha-docs
npm install
npm start          # Entwicklungsserver mit Live-Reload auf http://localhost:3000/VentoSync/
npm run build      # statische Site nach build/ (prüft alle internen Links)
npm run serve      # gebaute Site lokal ausliefern
```

## Struktur

| Pfad | Inhalt |
| --- | --- |
| `docs/intro.md` | Einführung und Schnellstart |
| `docs/konzepte/` | Entitäts-IDs, Raum & Master, Hardware-Varianten |
| `docs/entitaeten/` | Referenz aller Entitäten (eine Seite pro Funktionsbereich + Übersicht) |
| `docs/integration/` | API-Actions, Events, Eingänge aus Home Assistant |
| `docs/beispiele/` | Automationen und Dashboard-Karten |
| `sidebars.js` | Navigation |
| `docusaurus.config.js` | Site-Konfiguration (`.md` als CommonMark, `.mdx` als MDX) |

## Pflege

- Neue oder geänderte Entitäten (`name:` in `packages/**.yaml`) in der passenden Seite unter `docs/entitaeten/`
  **und** in `docs/entitaeten/uebersicht.md` nachtragen.
- Entitäts-IDs ohne Gerätepräfix schreiben (`select.…_luftermodus`), Beispiele mit dem Gerät
  „VentoSync Wohnzimmer“ (`ventosync_wohnzimmer`).
- Bei jeder Seite `title` und `sidebar_label` im Frontmatter setzen; neue Seiten in `sidebars.js` eintragen.
- `npm run build` bricht bei kaputten internen Links ab — vor dem Commit einmal bauen.
