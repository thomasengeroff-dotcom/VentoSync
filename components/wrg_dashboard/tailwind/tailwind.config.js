// VentoSync HRV – Tailwind configuration of the dashboard (components/wrg_dashboard)
// Copyright (c) 2026 Thomas Engeroff — GPL v3 (see LICENSE)
// Created: 2026-10-22, Modified: 2026-10-22
//
// Input of build_css.sh, which writes ../dashboard.css (only the classes that
// dashboard.html uses). Classes built in JavaScript must appear as complete
// strings in dashboard.html, or Tailwind does not find them.
module.exports = {
  content: {relative: true, files: ['../dashboard.html']},
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        bg: '#121212',
        card: '#1e1e1e',
        accent: '#03dac6',
        accentHover: '#01b4a0',
        danger: '#cf6679',
        warn: '#f2b84b',
      }
    }
  }
};
