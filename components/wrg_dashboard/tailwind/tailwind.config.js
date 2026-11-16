// VentoSync HRV – Tailwind configuration of the dashboard (components/wrg_dashboard)
// Copyright (c) 2026 Thomas Engeroff — GPL v3 (see LICENSE)
// Created: 2026-10-22, Modified: 2026-11-16
//
// Input of build_css.sh, which writes ../dashboard.css (only the classes that
// dashboard.html uses). Classes built in JavaScript must appear as complete
// strings in dashboard.html, or Tailwind does not find them.
//
// Colours and fonts are tokens declared at :root in input.css (space-separated
// RGB channels, so Tailwind opacity modifiers like bg-accent/10 keep working).
const token = name => `rgb(var(--c-${name}) / <alpha-value>)`;

module.exports = {
  content: {relative: true, files: ['../dashboard.html']},
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        paper: token('paper'),
        'paper-2': token('paper-2'),
        'paper-3': token('paper-3'),
        ink: token('ink'),
        'ink-2': token('ink-2'),
        muted: token('muted'),
        rule: token('rule'),
        'rule-2': token('rule-2'),
        accent: token('accent'),
        'accent-ink': token('accent-ink'),
        sky: token('sky'),
        danger: token('danger'),
        warn: token('warn'),
      },
      fontFamily: {
        sans: ['var(--font-sans)'],
        mono: ['var(--font-mono)'],
      },
    }
  }
};
