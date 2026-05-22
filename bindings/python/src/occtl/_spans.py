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

"""Zero-copy NumPy views over §10.2 native spans.

The C ABI's §10.2 span pattern hands back a raw pointer + count tied to the
parent handle's lifetime. The idiomatic Python shape is a ``numpy.ndarray``
view over the native memory:

- No copy on the hot path.
- ``view.copy()`` is the documented escape hatch when the user needs to
  outlive the parent.
- The returned array keeps a strong reference to the parent handle wrapper
  so GC cannot reclaim the parent (and free the native pointer) while a
  view is live. See ``_attach_lifetime`` for the mechanism.
"""

from __future__ import annotations

import weakref
from typing import Optional, Tuple

import numpy as _np


def span_view(
    ffi,
    ptr,
    count: int,
    dtype: str,
    parent: object,
    shape: Optional[Tuple[int, ...]] = None,
) -> "_np.ndarray":
    """Return a NumPy ndarray view over ``ptr[0:count]``.

    Parameters
    ----------
    ffi:
        The cffi instance owning ``ptr``.
    ptr:
        cffi pointer to the first element. Treated as opaque.
    count:
        Element count (not byte count).
    dtype:
        NumPy dtype name (``"float64"``, ``"int32"``, etc.).
    parent:
        The handle wrapper whose lifetime bounds ``ptr``. A strong reference
        is attached to the returned array so the parent stays alive at least
        as long as the view. **The raw data behind the view is valid only**
        **until the next mutating call on the parent handle** (e.g. a
        ``compact()`` or ``remove_*()`` call on the owning ``Graph``).  Call
        ``view.copy()`` to snapshot the data if you need to outlive the
        parent.
    shape:
        Optional shape override; defaults to ``(count,)``. Pass ``(count, 3)``
        for xyz-packed point arrays.
    """
    if count <= 0 or not ptr:
        empty = _np.empty(0, dtype=dtype)
        if shape is not None:
            empty = empty.reshape((0,) + tuple(shape[1:]))
        return empty

    itemsize = _np.dtype(dtype).itemsize
    nbytes = int(count) * int(itemsize)
    buf = ffi.buffer(ptr, nbytes)
    array = _np.frombuffer(buf, dtype=dtype, count=int(count))
    if shape is not None:
        array = array.reshape(*shape)

    _attach_lifetime(array, parent)
    return array


def _attach_lifetime(array, parent) -> None:
    """Pin ``parent`` so it outlives the returned ``array``.

    ``numpy.frombuffer`` sets the array's ``.base`` to the cffi buffer object
    (read-only afterwards), so we cannot reassign it to point at ``parent``.
    Instead we record ``parent`` in a module-level table keyed by ``id(array)``
    and arm a weakref callback that drops the entry when the array is GC'd.

    Slices and reshapes work because their ``.base`` chain eventually reaches
    the array returned by ``frombuffer``; as long as that array is reachable,
    the table entry — and thus the strong reference to ``parent`` — survives.
    """
    key = id(array)

    def _drop(_ref, _k=key):
        _PIN_TABLE.pop(_k, None)

    _PIN_TABLE[key] = (parent, weakref.ref(array, _drop))


_PIN_TABLE: "dict[int, tuple[object, weakref.ref]]" = {}
