#!/usr/bin/env python3
# ==========================================================================
# VentoSync HRV – Release version guard
# https://github.com/thomasengeroff-dotcom/VentoSync
#
# Copyright (c) 2026 Thomas Engeroff
# SPDX-License-Identifier: GPL-3.0-or-later
#
# File:        .github/scripts/check_version.py
# Description: Verifies that a pull request bumps version.json before it is
#              merged into master (every merge creates a release).
# Created:     2026-09-23
# Modified:    2026-09-23
# ==========================================================================
"""Release version guard.

Every push to ``master`` publishes a GitHub release tagged ``v<version.json>``.
If a PR is merged without a version bump, the release job would hit an existing
tag and the devices would never see an OTA update. This script fails a PR when:

  1. ``version.json`` is not strictly greater than on the base branch,
  2. the top ``## [x.y.z]`` entry of ``CHANGELOG.md`` does not match it, or
  3. the tag ``v<version>`` already exists on the remote.

Usage (CI and locally, from the repository root):

    python3 .github/scripts/check_version.py --base-ref origin/master
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

VERSION_RE = re.compile(r"^(\d+)\.(\d+)\.(\d+)$")
CHANGELOG_HEADING_RE = re.compile(r"^## \[(\d+\.\d+\.\d+)\]", re.MULTILINE)


def parse_version(text: str) -> tuple[int, int, int]:
    match = VERSION_RE.match(text.strip())
    if not match:
        raise ValueError(f"'{text}' is not a semantic version x.y.z")
    return tuple(int(part) for part in match.groups())  # type: ignore[return-value]


def read_version(raw_json: str, source: str) -> str:
    try:
        version = json.loads(raw_json)["project_version"]
    except (json.JSONDecodeError, KeyError) as exc:
        raise ValueError(f"{source}: cannot read 'project_version' ({exc})") from exc
    parse_version(version)  # validate format
    return version


def git(*args: str) -> str:
    return subprocess.run(["git", *args], check=True, capture_output=True, text=True).stdout


def top_changelog_version(changelog: str) -> str | None:
    match = CHANGELOG_HEADING_RE.search(changelog)
    return match.group(1) if match else None


def tag_exists(tag: str, remote: str) -> bool:
    return bool(git("ls-remote", "--tags", remote, f"refs/tags/{tag}").strip())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--base-ref", default="origin/master",
                        help="git ref of the PR base branch (default: origin/master)")
    parser.add_argument("--remote", default="origin", help="remote to look up existing tags")
    args = parser.parse_args()

    errors: list[str] = []

    head_version = read_version(Path("version.json").read_text(encoding="utf-8"), "version.json (PR)")
    base_version = read_version(git("show", f"{args.base_ref}:version.json"),
                                f"version.json ({args.base_ref})")
    print(f"Base version ({args.base_ref}): {base_version}")
    print(f"PR version:                  {head_version}")

    if parse_version(head_version) <= parse_version(base_version):
        errors.append(
            f"version.json must be bumped: {head_version} is not greater than {base_version} on "
            f"{args.base_ref}. Every merge into master publishes a release."
        )

    changelog_version = top_changelog_version(Path("CHANGELOG.md").read_text(encoding="utf-8"))
    if changelog_version != head_version:
        errors.append(
            f"Top CHANGELOG.md entry is [{changelog_version}], expected [{head_version}] "
            "(it becomes the release notes)."
        )

    tag = f"v{head_version}"
    if tag_exists(tag, args.remote):
        errors.append(f"Tag {tag} already exists — choose the next free version.")

    if errors:
        for err in errors:
            print(f"::error file=version.json::{err}")
        print("\nIf this PR must not create a release (docs / tooling only), "
              "add the label 'no-release'.")
        return 1

    print(f"OK: release {tag} will be created after the merge.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
