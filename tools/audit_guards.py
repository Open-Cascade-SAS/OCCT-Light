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

"""Audit exported status-returning C shims for OcctL::Core::Guard."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys


FUNC_RE = re.compile(
    r"OCCTL_API\s+occtl_status_t\s+OCCTL_CALL\s+"
    r"(?P<name>occtl_[A-Za-z0-9_]+)\s*\(",
    re.MULTILINE,
)


def line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def scan_file(path: pathlib.Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    matches = list(FUNC_RE.finditer(text))
    errors: list[str] = []
    for idx, match in enumerate(matches):
        end = matches[idx + 1].start() if idx + 1 < len(matches) else len(text)
        body = text[match.start() : end]
        if "OcctL::Core::Guard" not in body:
            errors.append(f"{path}:{line_number(text, match.start())}: {match.group('name')} is not guarded")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("src", type=pathlib.Path)
    args = parser.parse_args()

    errors: list[str] = []
    for path in sorted(args.src.rglob("*.cxx")):
        errors.extend(scan_file(path))

    if errors:
        print("OCCT-Light Guard audit failed:", file=sys.stderr)
        for err in errors:
            print(f"  {err}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
