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

"""Text-to-faces — :mod:`occtl.text`.

Only two public functions ship today:

- ``occtl_text_faces_info_init``  — initialise the options struct.
- ``occtl_text_make_faces``       — build a planar wire+face set from a string.

Hand-written helpers will land here as the API matures; for now we re-export
the generated wrappers so the symbol-coverage check is satisfied.
"""

from __future__ import annotations

from ._generated import text as _text_gen

for _name in dir(_text_gen):
    if _name.startswith("occtl_"):
        globals()[_name] = getattr(_text_gen, _name)

del _name
