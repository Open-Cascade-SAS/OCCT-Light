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

"""Verify that every public OCCTL_API function is exported by a built library."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


_COMMENT_RE = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
_DECL_RE = re.compile(r"\bOCCTL_API\b.*?\bOCCTL_CALL\b\s+(occtl_[A-Za-z0-9_]+)\s*\(", re.DOTALL)
_NM_SYMBOL_RE = re.compile(r"(?:^|\s)[A-Za-z]\s+_?(occtl_[A-Za-z0-9_]+)$")


def _public_symbols(headers: list[Path]) -> set[str]:
    symbols: set[str] = set()
    for header in headers:
        text = header.read_text(encoding="utf-8")
        text = _COMMENT_RE.sub("", text)
        symbols.update(_DECL_RE.findall(text))
    return symbols


def _run_nm(library: Path) -> str:
    attempts = [
        ["nm", "-gU", str(library)],
        ["nm", "-g", "--defined-only", str(library)],
        ["nm", "-g", str(library)],
    ]
    last_error = ""
    for cmd in attempts:
        proc = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if proc.returncode == 0:
            return proc.stdout
        last_error = proc.stderr.strip()
    raise RuntimeError(f"nm failed for {library}: {last_error}")


def _exported_symbols(library: Path) -> set[str]:
    symbols: set[str] = set()
    for line in _run_nm(library).splitlines():
        match = _NM_SYMBOL_RE.search(line.strip())
        if match:
            symbols.add(match.group(1))
    return symbols


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", required=True, type=Path, help="Built OCCT-Light library")
    parser.add_argument("--headers", required=True, nargs="+", type=Path, help="Public C headers")
    args = parser.parse_args()

    declared = _public_symbols(args.headers)
    exported = _exported_symbols(args.library)
    missing = sorted(declared - exported)
    extra = sorted(exported - declared)

    if missing:
        print("Missing exported OCCTL_API symbols:", file=sys.stderr)
        for symbol in missing:
            print(f"  {symbol}", file=sys.stderr)
        if extra:
            print(
                f"\nNote: {len(extra)} exported occtl_* symbols are not in the checked headers.",
                file=sys.stderr,
            )
        return 1

    print(f"Export check passed: {len(declared)} public functions exported by {args.library}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
