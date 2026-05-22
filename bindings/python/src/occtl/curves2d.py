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

"""2D parametric curves (PCurves) — :mod:`occtl.curves2d`.

See :mod:`occtl.curves` for the equivalent 3D namespace.
"""

from __future__ import annotations

from ._generated import curves2d as _curves2d_gen

for _name in dir(_curves2d_gen):
    if _name.startswith("occtl_"):
        globals()[_name] = getattr(_curves2d_gen, _name)

del _name
