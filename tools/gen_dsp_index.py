#!/usr/bin/env python3
"""
gen_dsp_index.py — scan jsfx-primitives/ and jsfx-algorithms/ and rebuild
docs/api_reference/00-index.md with the full compatibility matrix.

Run from the repo root:
    python3 tools/gen_dsp_index.py

Output:
    docs/api_reference/00-index.md (overwritten)

Idempotent — safe to run multiple times. New primitives default to △
("needs verification") in the matrix until human-confirmed.
"""
from pathlib import Path
import re
import sys
from typing import Dict, List, Optional, Tuple

REPO = Path(__file__).resolve().parent.parent
PRIMITIVES_DIR = REPO / "docs" / "api_reference" / "jsfx-primitives"
ALGORITHMS_DIR = REPO / "docs" / "api_reference" / "jsfx-algorithms"
OUT = REPO / "docs" / "api_reference" / "00-index.md"

CATEGORY_ORDER = [
    "utilities",
    "filters",
    "saturation",
    "dynamics",
    "delays",
    "reverb",
    "modulation",
    "pitch",
]

SIGNAL_CHAIN = "utilities → filters → saturation → dynamics → delays → reverb → modulation → pitch"

PRIMITIVE_PATTERN = re.compile(r"^# (.+?) \((.+?)\)$", re.MULTILINE)
COMPAT_BEFORE = re.compile(r"\*\*Before\*\*:\s*(.+)")
COMPAT_AFTER = re.compile(r"\*\*After\*\*:\s*(.+)")
COMPAT_CONFLICTS = re.compile(r"\*\*Conflicts\*\*:\s*(.+)")


def scan_dir(root: Path) -> Dict[str, Dict]:
    """Scan a directory of .md files and extract metadata."""
    results = {}
    for md_path in sorted(root.rglob("*.md")):
        rel = md_path.relative_to(root).with_suffix("")
        # Target key: e.g. "saturation/tanh-soft-clip"
        target_key = str(rel).replace("\\", "/")
        content = md_path.read_text(encoding="utf-8")

        # Extract name and category from the H1
        match = PRIMITIVE_PATTERN.search(content)
        if not match:
            continue
        name = match.group(1).strip()
        category = match.group(2).strip()

        # Estimate lines of JSFX code (inside ```jsfx blocks)
        in_code = False
        jsfx_lines = 0
        for line in content.split("\n"):
            if line.startswith("```jsfx"):
                in_code = True
            elif in_code and line.startswith("```"):
                in_code = False
            elif in_code:
                jsfx_lines += 1

        # Extract compatibility
        before = COMPAT_BEFORE.search(content)
        after = COMPAT_AFTER.search(content)
        conflicts = COMPAT_CONFLICTS.search(content)

        results[target_key] = {
            "name": name,
            "category": category,
            "lines": jsfx_lines,
            "before": before.group(1).strip() if before else "",
            "after": after.group(1).strip() if after else "",
            "conflicts": conflicts.group(1).strip() if conflicts else "None",
        }
    return results


def build_matrix(entries: Dict[str, Dict]) -> Dict[Tuple[str, str], str]:
    """Build a compatibility matrix from compatibility declarations."""
    keys = list(entries.keys())
    matrix = {}
    for key_a in keys:
        for key_b in keys:
            cell = "△"
            compat_a = entries[key_a]
            compat_b = entries[key_b]
            # Self-reference
            if key_a == key_b:
                cell = "—"
            # Check if key_a lists key_b's name/category as compatible
            elif compat_b["name"].lower() in compat_a["after"].lower():
                cell = "✓"
            elif compat_b["category"].lower() in compat_a["after"].lower():
                cell = "✓✓"
            # Check if conflicts are declared
            elif compat_b["name"].lower() in compat_a["conflicts"].lower():
                cell = "✗"
            matrix[(key_a, key_b)] = cell
    return matrix


def generate(entries: Dict, matrix: Dict) -> str:
    """Generate the 00-index.md content."""
    lines = []
    lines.append("# DSP Primitives Index")
    lines.append("")
    lines.append("## Signal Chain Order")
    lines.append(f"**{SIGNAL_CHAIN}**")
    lines.append("")

    # Catalog table
    lines.append("## Primitive Catalog")
    lines.append("")
    lines.append("| Target Key | Category | Lines | Description |")
    lines.append("|---|---|---|---|")
    for key in sorted(entries.keys(), key=lambda k: (CATEGORY_ORDER.index(entries[k]["category"]) if entries[k]["category"] in CATEGORY_ORDER else 99, k)):
        e = entries[key]
        lines.append(f"| `{key}` | {e['category']} | {e['lines']} | {e['name']} |")
    lines.append("")

    # Compatibility matrix
    keys = list(entries.keys())
    lines.append("## Compatibility Matrix")
    lines.append("")
    header = "| | " + " | ".join(f"`{k.split('/')[-1][:12]}`" for k in keys) + " |"
    sep = "|---|" + "|".join("---" for _ in keys) + "|"
    lines.append(header)
    lines.append(sep)
    for key_a in keys:
        row = [f"`{key_a.split('/')[-1][:12]}`"]
        for key_b in keys:
            cell = matrix.get((key_a, key_b), "△")
            row.append(cell)
        lines.append("| " + " | ".join(row) + " |")
    lines.append("")

    # Legend
    lines.append("**Legend:** — = self | ✓ = compatible | ✓✓ = strongly compatible (category-level) | ✗ = conflicts | △ = needs verification")
    lines.append("")

    # Mandatory rules
    lines.append("## Mandatory Composition Rules")
    lines.append("")
    lines.append("1. **DC blocking AFTER any saturation primitive with drive > 2.0**")
    lines.append("2. **Denormal prevention ALWAYS include `denorm = 1e-25;` + `+=/-=` pattern**")
    lines.append("3. **Category order: " + SIGNAL_CHAIN + "**")
    lines.append("4. **EQ before saturation to shape which frequencies distort**")
    lines.append("5. **Gain stage between primitives: output level ≈ input level**")
    lines.append("")

    # Regeneration note
    lines.append("---")
    lines.append("Generated by `tools/gen_dsp_index.py`. Regenerate after adding or modifying primitives.")
    return "\n".join(lines)


def main() -> int:
    entries = {}
    entries.update(scan_dir(PRIMITIVES_DIR))
    entries.update(scan_dir(ALGORITHMS_DIR))

    if not entries:
        print("ERROR: no primitives or algorithms found.", file=sys.stderr)
        return 1

    matrix = build_matrix(entries)
    content = generate(entries, matrix)
    OUT.write_text(content, encoding="utf-8")
    print(f"Wrote {OUT} ({len(entries)} entries, {len(matrix)} matrix cells)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
