#!/usr/bin/env bash
# VentoSync HRV – builds components/wrg_dashboard/dashboard.css
# Copyright (c) 2026 Thomas Engeroff — GPL v3 (see LICENSE)
# Created: 2026-10-22, Modified: 2026-10-22
#
# Usage: build_css.sh          regenerate dashboard.css (commit the result)
#        build_css.sh --check  fail if dashboard.css is outdated (CI, lint.yaml)
#
# Run it after changing classes in dashboard.html. Uses the pinned Tailwind v3
# standalone CLI (no Node.js), downloaded once into ~/.cache/ventosync and
# verified against its SHA-256. The firmware build itself never needs it:
# __init__.py only inlines the committed dashboard.css into the page.
set -euo pipefail

TAILWIND_VERSION="3.4.17"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$DIR/../dashboard.css"

case "$(uname -s)-$(uname -m)" in
  Linux-x86_64)               PLATFORM=linux-x64;   SHA256=7d24f7fa191d2193b78cd5f5a42a6093e14409521908529f42d80b11fde1f1d4 ;;
  Linux-aarch64|Linux-arm64)  PLATFORM=linux-arm64; SHA256=69b1378b8133192d7d2feb12a116fa12d035594f58db3eff215879e4ad8cf39b ;;
  Darwin-arm64)               PLATFORM=macos-arm64; SHA256=a1d0c7985759accca0bf12e51ac1dcbf0f6cf2fffb62e6e0f62d091c477a10a3 ;;
  Darwin-x86_64)              PLATFORM=macos-x64;   SHA256=6cbdad74be776c087ffa5e9a057512c54898f9fe8828d3362212dfe32fc933a3 ;;
  *) echo "Unsupported platform: $(uname -s)-$(uname -m)" >&2; exit 1 ;;
esac

CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/ventosync"
BIN="$CACHE/tailwindcss-v$TAILWIND_VERSION-$PLATFORM"
if [[ ! -x "$BIN" ]]; then
  mkdir -p "$CACHE"
  curl -sSfL -o "$BIN.tmp" \
    "https://github.com/tailwindlabs/tailwindcss/releases/download/v$TAILWIND_VERSION/tailwindcss-$PLATFORM"
  if command -v sha256sum >/dev/null; then
    actual=$(sha256sum "$BIN.tmp" | cut -d' ' -f1)
  else
    actual=$(shasum -a 256 "$BIN.tmp" | cut -d' ' -f1)
  fi
  if [[ "$actual" != "$SHA256" ]]; then
    rm -f "$BIN.tmp"
    echo "SHA-256 mismatch for tailwindcss-$PLATFORM" >&2
    exit 1
  fi
  chmod +x "$BIN.tmp"
  mv "$BIN.tmp" "$BIN"
fi

if [[ "${1:-}" == "--check" ]]; then
  tmp=$(mktemp)
  trap 'rm -f "$tmp"' EXIT
  "$BIN" -c "$DIR/tailwind.config.js" -i "$DIR/input.css" -o "$tmp" --minify 2>/dev/null
  if ! cmp -s "$tmp" "$OUT"; then
    echo "components/wrg_dashboard/dashboard.css is outdated — run components/wrg_dashboard/tailwind/build_css.sh and commit the result." >&2
    exit 1
  fi
  echo "dashboard.css is up to date"
else
  "$BIN" -c "$DIR/tailwind.config.js" -i "$DIR/input.css" -o "$OUT" --minify
fi
