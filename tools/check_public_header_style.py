#!/usr/bin/env python3
# Copyright (c) 2026 Capgemini Engineering Research and Development.
#
# This file is part of OCCT-Light software library.
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License version 3 as published
# by the Free Software Foundation, with an option to use any later version.
# Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
# for complete text of the license and disclaimer of any warranty.
#
# Alternatively, this file may be used under the terms of a commercial
# license or contractual agreement.
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Check public C/C++ header comment style.

Public headers allow:
* SPDX comments;
* Doxygen comments (/**, ///, //!);
* include-guard end comments: #endif // GUARD.

Free comments and decorative separators in include/occtl and
include/occtl-hpp are rejected so generated bindings can rely on structured
documentation rather than section labels.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys


SEPARATOR_RE = re.compile(r"(/\*\s*-{2,}|/\*\s*=+|^\s*//\s*-{2,}|^\s*//\s*=+|[-=]{6,}|\|[- ]+\|)")


def is_allowed_comment(line: str) -> bool:
    stripped = line.lstrip()
    if "SPDX-License-Identifier" in stripped:
        return True
    if stripped.startswith("#endif //"):
        return True
    if stripped.startswith("///") or stripped.startswith("//!"):
        return True
    if stripped.startswith("/**") or stripped.startswith("/*!") or stripped.startswith("*/"):
        return True
    if stripped.startswith("*"):
        return True
    return False


def _strip_leading_license_block(lines: list[str]) -> list[str]:
    """Ignore the standard leading license header comment block if present."""
    idx = 0
    while idx < len(lines) and not lines[idx].strip():
        idx += 1

    if idx >= len(lines):
        return lines

    stripped = lines[idx].lstrip()
    if not (stripped.startswith("//") or stripped.startswith("/*")):
        return lines

    end = idx
    while end < len(lines):
        s = lines[end].lstrip()
        if s.startswith("//") or s.startswith("/*") or s.startswith("*") or s.startswith("*/") or not s.strip():
            end += 1
            continue
        break

    block = lines[idx:end]
    block_text = "\n".join(block)
    has_spdx = "SPDX-License-Identifier" in block_text
    has_occtl = "This file is part of OCCT-Light software library" in block_text
    has_copyright = "Copyright (c)" in block_text
    if has_spdx and has_occtl and has_copyright:
        return lines[end:]
    return lines


def scan_file(path: pathlib.Path) -> list[str]:
    errors: list[str] = []
    all_lines = path.read_text(encoding="utf-8").splitlines()
    lines = _strip_leading_license_block(all_lines)
    leading_offset = len(all_lines) - len(lines)
    for idx, line in enumerate(lines, start=1):
        line_no = idx + leading_offset
        if SEPARATOR_RE.search(line):
            errors.append(f"{path}:{line_no}: decorative separator/table comment is not allowed")
            continue

        stripped = line.lstrip()
        if stripped.startswith("//") and not is_allowed_comment(line):
            errors.append(f"{path}:{line_no}: free // comment is not allowed")
        elif stripped.startswith("/*") and not is_allowed_comment(line):
            errors.append(f"{path}:{line_no}: free /* comment is not allowed")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dirs", nargs="+", type=pathlib.Path)
    args = parser.parse_args()

    errors: list[str] = []
    for root in args.dirs:
        for path in sorted(root.rglob("*")):
            if path.suffix in {".h", ".hpp"}:
                errors.extend(scan_file(path))

    if errors:
        print("OCCT-Light public-header style check failed:", file=sys.stderr)
        for err in errors:
            print(f"  {err}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
