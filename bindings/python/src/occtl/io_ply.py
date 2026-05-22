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

"""PLY export — :mod:`occtl.io_ply`.

Wraps :c:`occtl_io_ply_write` for Stanford PLY mesh streams.  OCCT 8.0.0's
PLY provider is export-only, so no read wrapper is exposed.
"""

from __future__ import annotations

from ._generated import io_ply as _io_ply_gen

_SENTINEL = object()
for _name in dir(_io_ply_gen):
    if _name.startswith("occtl_"):
        if globals().get(_name, _SENTINEL) is _SENTINEL:
            globals()[_name] = getattr(_io_ply_gen, _name)
        else:
            import warnings
            warnings.warn(
                f"occtl.io_ply: skipping {_name} — already defined in io_ply namespace",
                stacklevel=2,
            )
del _name, _SENTINEL
