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

"""Healing — :mod:`occtl.heal`.

Wraps :c:`occtl_heal_shape` and its options. Applies OCCT's
ShapeHealing toolkit to a graph, fixing tolerances, orientations,
and other topological defects.
"""

from __future__ import annotations

from ._generated import heal as _heal_gen

_SENTINEL = object()
for _name in dir(_heal_gen):
    if _name.startswith("occtl_"):
        if globals().get(_name, _SENTINEL) is _SENTINEL:
            globals()[_name] = getattr(_heal_gen, _name)
        else:
            import warnings
            warnings.warn(
                f"occtl.heal: skipping {_name} — already defined in heal namespace",
                stacklevel=2,
            )
del _name, _SENTINEL
