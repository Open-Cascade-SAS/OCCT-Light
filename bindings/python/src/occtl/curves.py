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

"""3D parametric curves — :mod:`occtl.curves`.

The generated wrappers in :mod:`occtl._generated.curves` provide one Python
function per ``occtl_curve_*`` ABI entry point. This module re-exports them
under the ``occtl.curves`` namespace so the symbol-coverage check has a
single place to look. Idiomatic ``Curve`` / ``Curve2d`` handle wrappers live
in :mod:`occtl.topo`.
"""

from __future__ import annotations

from ._generated import curves as _curves_gen

for _name in dir(_curves_gen):
    if _name.startswith("occtl_"):
        globals()[_name] = getattr(_curves_gen, _name)

del _name
