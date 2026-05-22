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

"""Options-struct construction helpers.

Every ``occtl_*_info_t`` struct has the same header layout per the §8 ABI
contract:

- ``uint32_t struct_version`` — must be set to ``OCCTL_*_VERSION_N`` matching
  the version the binding was compiled against.
- ``const void* p_next``      — reserved for extension chains; always ``NULL``
  in v1.

The binding's responsibility is to fill those two fields automatically. The
user supplies the data fields via a Python dict or keyword arguments; this
module supplies the small reusable helpers.

The actual struct types and their version constants live in the generated
``_generated._raw`` cffi cdef and ``_generated._abi`` constants module. Per-module helpers in
``occtl/topo.py`` / ``occtl/prim.py`` / ``occtl/text.py`` use these primitives
to build their domain-specific builders.
"""

from __future__ import annotations

from typing import Any, Callable, Mapping, Optional


def fill_info(
    info_struct,
    version_value: int,
    init_fn: Optional[Callable[[Any], None]] = None,
    overrides: Optional[Mapping[str, Any]] = None,
) -> Any:
    """Populate ``info_struct`` with version + user overrides.

    If ``init_fn`` is provided, call it first to let the library set defaults;
    then stamp ``struct_version`` (the library's init function does this
    already, but the explicit second write is harmless and makes the contract
    auditable). Finally apply any user ``overrides`` as attribute assignments.

    Returns the same struct for chaining.
    """
    if init_fn is not None:
        init_fn(info_struct)
    # Even if init_fn ran, re-stamp the version. The binding always vouches
    # for the version, never the user; this also covers the case where the
    # generated cdef differs from the runtime library (caught downstream by
    # the OCCTL_VERSION_MISMATCH return value).
    info_struct.struct_version = int(version_value)
    info_struct.p_next = info_struct.p_next  # explicit no-op for readers
    if overrides:
        for key, value in overrides.items():
            if key in ("struct_version", "p_next"):
                # Reserved for the binding; refuse to let user overrides
                # leak through.
                continue
            _assign(info_struct, key, value)
    return info_struct


def _assign(struct, name: str, value: Any) -> None:
    """Assign ``value`` to ``struct.<name>``, copying nested dicts when needed.

    For nested struct fields (``placement``, ``point``, …) the user may pass
    a mapping; the helper copies field-by-field. Sequences become field-by-
    field copies as well (e.g. for fixed-size double arrays).
    """
    target = getattr(struct, name)
    if isinstance(value, Mapping) and _is_cdata(target):
        for k, v in value.items():
            _assign(target, k, v)
        return
    setattr(struct, name, value)


def _is_cdata(obj: Any) -> bool:
    """Best-effort 'is this a cffi cdata object' check."""
    return hasattr(obj, "__class__") and "cdata" in type(obj).__name__.lower()
