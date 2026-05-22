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

"""Runtime, version, and error utilities — :mod:`occtl.core`.

The runtime is initialised automatically by ``occtl/__init__.py``; users only
call these functions if they need explicit control over shutdown or want
human-readable diagnostics.
"""

from __future__ import annotations

from typing import Tuple

from ._errors import Status, _check
from ._generated import core as _core_gen
from ._generated._raw import ffi, lib


def status_to_string(status: int | Status) -> str:
    """Return the canonical name for a status code (UTF-8 string)."""
    return _core_gen.occtl_status_to_string(int(status))


def runtime_abi_version() -> int:
    """Return the runtime ABI version (compare against :data:`occtl.ABI_VERSION`)."""
    return int(lib.occtl_runtime_abi_version())


def runtime_occt_version() -> str:
    """Return the OCCT version OCCT-Light was built against, e.g. ``"7.9.0"``."""
    return _core_gen.occtl_runtime_occt_version()


def runtime_version() -> Tuple[int, int, int]:
    """Return the (major, minor, patch) SemVer of the OCCT-Light runtime."""
    major = ffi.new("uint32_t*")
    minor = ffi.new("uint32_t*")
    patch = ffi.new("uint32_t*")
    lib.occtl_runtime_version(major, minor, patch)
    return (int(major[0]), int(minor[0]), int(patch[0]))


def runtime_shutdown() -> None:
    """Release process-wide runtime state. Optional, but harmless at exit."""
    lib.occtl_runtime_shutdown()


def error_clear() -> None:
    """Clear the thread-local last-error slot."""
    lib.occtl_error_clear()


# Re-export every generated wrapper under the ``core`` namespace so the
# symbol-coverage check can find them. Keep hand-written names authoritative to avoid silently
# overwriting a hand-written wrapper; the coverage test will catch a missing
# export.
_SENTINEL = object()
for _name in dir(_core_gen):
    if _name.startswith("occtl_"):
        if globals().get(_name, _SENTINEL) is _SENTINEL:
            globals()[_name] = getattr(_core_gen, _name)
        else:
            import warnings
            warnings.warn(
                f"occtl.core: not exporting {_name} — already defined in core namespace",
                stacklevel=2,
            )
del _name, _SENTINEL
