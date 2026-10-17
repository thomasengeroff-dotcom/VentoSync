// @ts-check
// VentoSync – Home-Assistant-Dokumentation (Docusaurus 3).
// Aufbau und Stil angelehnt an developers.home-assistant.io.
// Lokal starten:  npm install && npm start   (siehe README.md)

const { themes: prismThemes } = require('prism-react-renderer');

const REPO = 'https://github.com/thomasengeroff-dotcom/VentoSync';

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'VentoSync',
  tagline: 'Home-Assistant-Entitäten, Actions und Events der VentoSync-Firmware',
  favicon: 'img/favicon.svg',

  // GitHub Pages: https://<user>.github.io/VentoSync/
  url: 'https://thomasengeroff-dotcom.github.io',
  baseUrl: '/VentoSync/',
  organizationName: 'thomasengeroff-dotcom',
  projectName: 'VentoSync',

  onBrokenLinks: 'throw',
  onBrokenAnchors: 'warn',

  i18n: {
    defaultLocale: 'de',
    locales: ['de'],
  },

  markdown: {
    // .md = CommonMark (Platzhalter wie <gerät> brechen kein JSX), .mdx = MDX.
    format: 'detect',
    hooks: {
      onBrokenMarkdownLinks: 'throw',
    },
  },

  presets: [
    [
      'classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          routeBasePath: '/',
          sidebarPath: require.resolve('./sidebars.js'),
          editUrl: `${REPO}/edit/master/documentation/ha-docs/`,
          showLastUpdateTime: false,
        },
        blog: false,
        theme: {
          customCss: require.resolve('./src/css/custom.css'),
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      colorMode: {
        respectPrefersColorScheme: true,
      },
      navbar: {
        title: 'VentoSync Docs',
        logo: {
          alt: 'VentoSync',
          src: 'img/favicon.svg',
        },
        items: [
          { type: 'docSidebar', sidebarId: 'docs', position: 'left', label: 'Dokumentation' },
          { to: '/entitaeten/uebersicht', label: 'Entitäten', position: 'left' },
          { to: '/integration/actions', label: 'Actions', position: 'left' },
          { to: '/beispiele/automationen', label: 'Beispiele', position: 'left' },
          { href: REPO, label: 'GitHub', position: 'right' },
        ],
      },
      footer: {
        style: 'dark',
        links: [
          {
            title: 'Dokumentation',
            items: [
              { label: 'Einführung', to: '/' },
              { label: 'Entitäten', to: '/entitaeten/uebersicht' },
              { label: 'Automationen', to: '/beispiele/automationen' },
            ],
          },
          {
            title: 'Projekt',
            items: [
              { label: 'GitHub', href: REPO },
              { label: 'Changelog', href: `${REPO}/blob/master/CHANGELOG.md` },
              { label: 'Technische Guides', href: `${REPO}/tree/master/documentation/de` },
            ],
          },
        ],
        copyright: `VentoSync – GPL v3. Nicht verbunden mit Home Assistant oder VentoMaxx.`,
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
        additionalLanguages: ['bash', 'json'],
      },
      tableOfContents: {
        minHeadingLevel: 2,
        maxHeadingLevel: 3,
      },
    }),
};

module.exports = config;
