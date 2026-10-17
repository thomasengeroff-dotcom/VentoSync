// @ts-check
// Sidebar der VentoSync-Home-Assistant-Dokumentation.

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  docs: [
    'intro',
    {
      type: 'category',
      label: 'Konzepte',
      collapsed: false,
      items: ['konzepte/entitaets-ids', 'konzepte/raum-und-master', 'konzepte/varianten'],
    },
    {
      type: 'category',
      label: 'Entitäten',
      collapsed: false,
      link: { type: 'doc', id: 'entitaeten/uebersicht' },
      items: [
        'entitaeten/fan',
        'entitaeten/betriebsmodus',
        'entitaeten/smart-automatik',
        'entitaeten/automatik-status',
        'entitaeten/klima-koordination',
        'entitaeten/fenstersperre',
        'entitaeten/nox-stossluftung',
        'entitaeten/sensoren',
        'entitaeten/radar',
        'entitaeten/urlaubsmodus',
        'entitaeten/wartung',
        'entitaeten/einrichtung-diagnose',
      ],
    },
    {
      type: 'category',
      label: 'Integration',
      collapsed: false,
      items: ['integration/actions', 'integration/events', 'integration/ha-eingaenge'],
    },
    {
      type: 'category',
      label: 'Beispiele',
      collapsed: false,
      items: ['beispiele/automationen', 'beispiele/dashboards'],
    },
  ],
};

module.exports = sidebars;
