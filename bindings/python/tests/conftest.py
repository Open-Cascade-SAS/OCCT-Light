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

"""Pytest fixtures for the occtl Python binding tests.

The smoke and parity tests need the native ``libocctl-<feature-set>`` shared library on
the loader path. Build the project with ``OCCTL_SHARED_LIBS=ON`` (presets
``minimal``, ``cad``, ``full``, …) and either:

- Let the binding discover it automatically (the wheel install bundles the
  library, and a source checkout falls back to ``build/<preset>/lib/``).
- Set ``OCCTL_LIBRARY_PATH=/abs/path/to/lib_directory`` for unusual layouts.

If the library is missing, tests fail immediately with a clear setup error.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

import pytest


_HERE = Path(__file__).resolve().parent
_REPO_ROOT = _HERE.parents[2]  # bindings/python/tests -> bindings/python -> bindings -> repo


def _try_load() -> "Exception | None":
    """Return ``None`` if the binding loads, or the captured exception."""
    try:
        import occtl  # noqa: F401
    except Exception as exc:  # noqa: BLE001
        return exc
    return None


@pytest.fixture(scope="session")
def occtl_module():
    """Yield the loaded ``occtl`` package, or fail if the native lib is missing."""
    err = _try_load()
    if err is not None:
        pytest.fail(
            f"occtl native library not loadable: {err}. "
            "Build with -DOCCTL_SHARED_LIBS=ON and set OCCTL_LIBRARY_PATH if needed."
        )
    import occtl
    return occtl


@pytest.fixture
def graph(occtl_module):
    """A fresh empty topology graph; closed at test exit."""
    g = occtl_module.Graph()
    try:
        yield g
    finally:
        g.close()
