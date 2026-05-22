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

"""Script entrypoint for binding package preparation.

Public entrypoint lives in tools/scripts/, while implementation stays in
tools/packaging/build_binding_packages.py.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    impl = repo_root / "tools" / "packaging" / "build_binding_packages.py"
    cmd = [sys.executable, str(impl), "--repo-root", str(repo_root), *sys.argv[1:]]
    return subprocess.call(cmd, cwd=os.fspath(repo_root))


if __name__ == "__main__":
    raise SystemExit(main())
