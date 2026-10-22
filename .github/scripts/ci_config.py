#!/usr/bin/env python3
# ==========================================================================
# VentoSync HRV – CI configuration helper
# https://github.com/thomasengeroff-dotcom/VentoSync
#
# Copyright (c) 2026 Thomas Engeroff
# SPDX-License-Identifier: GPL-3.0-or-later
#
# File:        .github/scripts/ci_config.py
# Description: Single source for the firmware variant list (.github/variants.json)
#              and consistency checks between YAML and C++ defaults.
# Created:     2026-10-18
# Modified:    2026-10-22
# ==========================================================================
"""CI configuration helper (dependency-free, Python >= 3.8).

Subcommands (run from the repository root):

  matrix [--pr]   Print the build matrix as JSON (``include`` list). ``--pr``
                  limits it to the variants with ``"pr": true``. With
                  ``GITHUB_OUTPUT`` set, also writes ``matrix=<json>`` there.
  list            Print the YAML file of every variant (one per line).
  generate        Create the generated variant files (e.g. the MQTT variant).
  check           Consistency checks, exit code 1 on any error:
                    - every ventosync*.yaml in the root is listed (and vice versa),
                    - name == firmware_variant substitution of the variant,
                    - every variant has a row in the CLAUDE.md variant table,
                    - generated variant files are not tracked by git,
                    - default_* / hvac_default_* substitutions in
                      packages/base/ventosync_core.yaml equal the C++
                      constants (defaults.h, hvac_coordinator.h).

``upload_all.sh`` is intentionally not generated: it only lists the devices
the maintainer updates manually.
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
VARIANTS_FILE = ROOT / ".github" / "variants.json"
CORE_YAML = ROOT / "packages" / "base" / "ventosync_core.yaml"
COMMON_YAML = ROOT / "packages" / "base" / "esp32c6_common.yaml"
CLAUDE_MD = ROOT / "CLAUDE.md"

# YAML substitution -> (C++ header, constant name)
DEFAULTS = {
    "default_auto_min_fan_level": ("components/ventilation_logic/defaults.h", "AUTO_MIN_FAN_LEVEL"),
    "default_auto_max_fan_level": ("components/ventilation_logic/defaults.h", "AUTO_MAX_FAN_LEVEL"),
    "default_auto_co2_threshold": ("components/ventilation_logic/defaults.h", "AUTO_CO2_THRESHOLD_PPM"),
    "default_auto_humidity_threshold": ("components/ventilation_logic/defaults.h", "AUTO_HUMIDITY_THRESHOLD_PCT"),
    "default_led_max_brightness": ("components/ventilation_logic/defaults.h", "LED_MAX_BRIGHTNESS_PCT"),
    "default_vacation_mode_index": ("components/ventilation_logic/defaults.h", "VACATION_MODE_INDEX"),
    "default_vacation_level": ("components/ventilation_logic/defaults.h", "VACATION_LEVEL"),
    "default_summer_cooling_threshold": ("components/ventilation_logic/summer_bypass.h", "DEFAULT_THRESHOLD_C"),
    "default_auto_override_duration": ("components/ventilation_logic/level_override.h", "DEFAULT_DURATION_MIN"),
    "hvac_default_co2_threshold": ("components/ventilation_logic/hvac_coordinator.h", "DEFAULT_CO2_THRESHOLD_PPM"),
    "hvac_default_emergency_co2": ("components/ventilation_logic/hvac_coordinator.h", "DEFAULT_EMERGENCY_CO2_PPM"),
    "hvac_default_max_fan_level": ("components/ventilation_logic/hvac_coordinator.h", "DEFAULT_MAX_FAN_LEVEL"),
}


def load_variants() -> list[dict]:
    return json.loads(VARIANTS_FILE.read_text(encoding="utf-8"))["variants"]


def substitution(text: str, key: str) -> str | None:
    """Value of a top-level-ish ``key: "value"`` line (plain regex — the YAML
    uses ESPHome tags a plain YAML parser would reject)."""
    m = re.search(rf'^\s*{re.escape(key)}:\s*"?([^"#\n]+?)"?\s*(?:#.*)?$', text, re.MULTILINE)
    return m.group(1).strip() if m else None


def cpp_constant(path: Path, name: str) -> str | None:
    m = re.search(rf"constexpr\s+\w+\s+{re.escape(name)}\s*=\s*([-0-9.]+)f?\s*;", path.read_text(encoding="utf-8"))
    return m.group(1) if m else None


def variant_table_files(markdown: str) -> set[str]:
    """YAML files in the first column of the CLAUDE.md variant table (the
    table whose header starts with ``| Variant YAML``) — prose mentions of a
    file elsewhere in CLAUDE.md do not count."""
    files: set[str] = set()
    in_table = False
    for line in markdown.splitlines():
        if line.startswith("| Variant YAML"):
            in_table = True
            continue
        if in_table:
            if not line.startswith("|"):
                break
            m = re.match(r"\|\s*`([^`]+\.yaml)`", line)
            if m:
                files.add(m.group(1))
    return files


def git_tracked(path: str) -> bool:
    """True if git tracks `path` (False when git is unavailable)."""
    try:
        res = subprocess.run(["git", "ls-files", "--error-unmatch", path], cwd=ROOT,
                             capture_output=True, check=False)
    except OSError:
        return False
    return res.returncode == 0


def same_number(a: str, b: str) -> bool:
    try:
        return float(a) == float(b)
    except ValueError:
        return False


def cmd_matrix(pr_only: bool) -> int:
    include = []
    for v in load_variants():
        if pr_only and not v.get("pr", False):
            continue
        entry = {"yaml": v["yaml"], "name": v["name"]}
        if "generate" in v:
            entry["generated"] = True
        include.append(entry)
    data = json.dumps(include, separators=(",", ":"))
    print(data)
    out = os.environ.get("GITHUB_OUTPUT")
    if out:
        with open(out, "a", encoding="utf-8") as f:
            f.write(f"matrix={data}\n")
    return 0


def cmd_list() -> int:
    for v in load_variants():
        print(v["yaml"])
    return 0


def cmd_generate() -> int:
    for v in load_variants():
        gen = v.get("generate")
        if not gen:
            continue
        src = (ROOT / gen["from"]).read_text(encoding="utf-8")
        if not src.endswith("\n"):
            src += "\n"
        (ROOT / v["yaml"]).write_text(src + gen["append"] + "\n", encoding="utf-8")
        print(f"generated {v['yaml']} from {gen['from']}")
    return 0


def cmd_check() -> int:
    errors: list[str] = []
    variants = load_variants()

    names = [v["name"] for v in variants]
    files = [v["yaml"] for v in variants]
    for dup in {n for n in names if names.count(n) > 1}:
        errors.append(f"variants.json: duplicate name {dup}")
    if not any(v.get("pr") for v in variants):
        errors.append("variants.json: no variant is built on pull requests")

    generated = {v["yaml"] for v in variants if "generate" in v}
    in_repo = {p.name for p in ROOT.glob("ventosync*.yaml")} - generated
    for f in sorted(in_repo - set(files)):
        errors.append(f"{f} exists but is missing in .github/variants.json")
    for f in sorted(set(files) - in_repo - generated):
        errors.append(f"variants.json lists {f}, which does not exist")

    default_variant = substitution(COMMON_YAML.read_text(encoding="utf-8"), "firmware_variant")
    table = variant_table_files(CLAUDE_MD.read_text(encoding="utf-8"))
    if not table:
        errors.append("CLAUDE.md: variant table ('| Variant YAML ...') not found")
    for v in variants:
        if "generate" in v:
            if not (ROOT / v["generate"]["from"]).exists():
                errors.append(f"{v['yaml']}: generate.from {v['generate']['from']} does not exist")
            if git_tracked(v["yaml"]):
                errors.append(f"{v['yaml']} is generated by ci_config.py but tracked by git — remove it")
            continue
        path = ROOT / v["yaml"]
        if not path.exists():
            continue
        fw = substitution(path.read_text(encoding="utf-8"), "firmware_variant") or default_variant
        if fw != v["name"]:
            errors.append(f"{v['yaml']}: firmware_variant '{fw}' != variants.json name '{v['name']}'")
        if v["yaml"] not in table:
            errors.append(f"{v['yaml']} is missing in the CLAUDE.md variant table")

    core = CORE_YAML.read_text(encoding="utf-8")
    for key, (header, const) in DEFAULTS.items():
        yaml_val = substitution(core, key)
        cpp_val = cpp_constant(ROOT / header, const)
        if yaml_val is None:
            errors.append(f"{CORE_YAML.relative_to(ROOT)}: substitution {key} not found")
        elif cpp_val is None:
            errors.append(f"{header}: constant {const} not found")
        elif not same_number(yaml_val, cpp_val):
            errors.append(f"default mismatch: {key} = {yaml_val} (YAML) vs {const} = {cpp_val} ({header})")

    for e in errors:
        print(f"::error::{e}")
    if not errors:
        print(f"OK: {len(variants)} variants, {len(DEFAULTS)} defaults consistent")
    return 1 if errors else 0


def main(argv: list[str]) -> int:
    if not argv:
        print(__doc__)
        return 2
    cmd, args = argv[0], argv[1:]
    if cmd == "matrix":
        return cmd_matrix("--pr" in args)
    if cmd == "list":
        return cmd_list()
    if cmd == "generate":
        return cmd_generate()
    if cmd == "check":
        return cmd_check()
    print(f"unknown command: {cmd}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
