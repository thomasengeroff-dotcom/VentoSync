#!/usr/bin/env python3
# ==========================================================================
# VentoSync HRV – CI change detection
# https://github.com/thomasengeroff-dotcom/VentoSync
#
# Copyright (c) 2026 Thomas Engeroff
# SPDX-License-Identifier: GPL-3.0-or-later
#
# File:        .github/scripts/detect_changes.py
# Description: Decides whether a CI run touches the firmware at all, so that
#              docs-only changes skip unit tests, firmware builds, the YAML
#              validation and CodeQL.
# Created:     2026-10-16
# Modified:    2026-10-16
# ==========================================================================
"""CI change detection.

Writes ``firmware=true|false`` to ``$GITHUB_OUTPUT``. ``false`` only when every
changed file matches ``DOC_ONLY_PATTERNS`` (README, documentation, images, PCB
files, HA examples). Anything unknown counts as a firmware change, and so does
every situation the diff cannot be determined for (workflow_dispatch, schedule,
new branch, rewritten history) — the safe default is always "build".

Event inputs come from the environment (set by the workflow):

  EVENT_NAME   github.event_name
  BASE_SHA     pull_request: github.event.pull_request.base.sha
  HEAD_SHA     pull_request: github.event.pull_request.head.sha, push: github.sha
  BEFORE_SHA   push: github.event.before

``--release`` (build.yaml only): on a push, also build when the release tag
``v<version.json>`` does not exist yet — a release that failed on an earlier
push is then completed by the next one, even if that one only changes docs.

Local check of the current branch against master:

    EVENT_NAME=pull_request BASE_SHA=origin/master HEAD_SHA=HEAD \
        python3 .github/scripts/detect_changes.py
"""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import subprocess
import sys

# Files that never influence the firmware, the tests or the CI itself.
DOC_ONLY_PATTERNS = (
    "*.md",
    "LICENSE",
    "documentation/*",
    "images/*",
    "EasyEDA-Pro/*",
    "ha_integration_example/*",
    "ESPHome-VentoMaxx-Analyser/*",
    "*.png",
    "*.jpg",
    "*.jpeg",
    "*.webp",
    "*.gif",
    "*.pdf",
)

NULL_SHA = "0" * 40


def git(*args: str) -> str:
    return subprocess.run(
        ["git", *args], check=True, capture_output=True, text=True
    ).stdout


def changed_files(event: str) -> list[str] | None:
    """Changed paths of this event, or None if they cannot be determined."""
    if event == "pull_request":
        base, head = os.environ.get("BASE_SHA", ""), os.environ.get("HEAD_SHA", "")
        if not base or not head:
            return None
        rng = f"{base}...{head}"  # merge base → PR head: only the PR's changes
    elif event == "push":
        before, head = os.environ.get("BEFORE_SHA", ""), os.environ.get("HEAD_SHA", "")
        if not before or before == NULL_SHA or not head:
            return None
        rng = f"{before}..{head}"
    else:
        return None
    try:
        return [line for line in git("diff", "--name-only", rng).splitlines() if line]
    except subprocess.CalledProcessError as err:
        print(f"::warning::git diff {rng} failed ({err.stderr.strip().splitlines()[0]}) — assuming firmware changes.")
        return None


def is_doc_only(path: str) -> bool:
    return any(fnmatch.fnmatch(path, pattern) for pattern in DOC_ONLY_PATTERNS)


def release_tag_missing() -> bool:
    with open("version.json", encoding="utf-8") as fh:
        tag = "v" + json.load(fh)["project_version"]
    result = subprocess.run(
        ["git", "ls-remote", "--exit-code", "--tags", "origin", f"refs/tags/{tag}"],
        capture_output=True,
    )
    if result.returncode == 2:  # no matching ref
        print(f"Release tag {tag} does not exist yet — release pending.")
        return True
    if result.returncode != 0:
        print("::warning::git ls-remote failed — assuming release pending.")
        return True
    return False


def decide(event: str, release: bool) -> tuple[bool, str]:
    files = changed_files(event)
    if files is None:
        return True, f"{event}: no comparable diff — building everything"
    firmware = [f for f in files if not is_doc_only(f)]
    for f in files:
        print(f"  {'firmware' if f in firmware else 'docs    '}  {f}")
    if firmware:
        return True, f"{len(firmware)} of {len(files)} changed files affect the firmware"
    if release and event == "push" and release_tag_missing():
        return True, "docs only, but the release of version.json is still pending"
    return False, f"docs only ({len(files)} files) — firmware jobs skipped"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--release", action="store_true",
                        help="push: also build while the release tag is missing")
    args = parser.parse_args()

    firmware, reason = decide(os.environ.get("EVENT_NAME", ""), args.release)
    print(f"firmware={str(firmware).lower()}: {reason}")

    if out := os.environ.get("GITHUB_OUTPUT"):
        with open(out, "a", encoding="utf-8") as fh:
            fh.write(f"firmware={str(firmware).lower()}\n")
    if summary := os.environ.get("GITHUB_STEP_SUMMARY"):
        with open(summary, "a", encoding="utf-8") as fh:
            fh.write(f"**Firmware jobs:** {'run' if firmware else 'skipped'} — {reason}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
