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

"""Generator helpers around opaque ``occtl_*_iter_t`` handles.

The C iterator contract is:

1. ``occtl_xxx_iter_create(graph, out_iter)`` produces a handle.
2. ``occtl_xxx_iter_next(iter, out_id)`` returns ``OCCTL_OK`` while there are
   more elements, ``OCCTL_NOT_FOUND`` once exhausted. ``OCCTL_NOT_FOUND`` is
   **not** an error and the last-error TLS is **not** read on that branch.
3. ``occtl_xxx_iter_free(iter)`` is called by the wrapper, in a ``finally``
   block, on either exhaustion or early ``Generator.close()``.

This module's :func:`node_iter` factory wraps the pattern uniformly for any
iterator that yields ``occtl_node_id_t``. The handle is owned by the generator
frame; closing the generator (or letting GC reclaim it after iteration) frees
the native iterator.
"""

from __future__ import annotations

from typing import Callable, Generator

from ._errors import Status, _check
from ._ids import NodeId, RefId, RepId


def node_iter(
    create_fn: Callable[[], object],
    next_fn: Callable[[object, object], int],
    free_fn: Callable[[object], None],
    new_iter_handle: Callable[[], object],
    new_id_slot: Callable[[], object],
) -> Generator[NodeId, None, None]:
    """Yield ``NodeId`` values from a node-iterator creation call.

    ``create_fn``  — returns the native iterator handle (already error-checked).
    ``next_fn``    — invoked as ``status = next_fn(iter_ptr, id_slot_ptr)``.
    ``free_fn``    — releases the iterator (NULL-tolerant).
    ``new_iter_handle`` / ``new_id_slot`` — allocate cffi storage for the
    iterator pointer and the out-id slot respectively.

    **Lifetime**: the yielded ``NodeId`` values are transient — they are valid
    only until the next mutating call on the parent ``Graph`` (e.g.
    ``compact``, ``add_*``, ``remove_*``). Copy the integer before calling
    another OCCT-Light entry point.
    """
    iter_ptr = create_fn()
    id_slot = new_id_slot()
    try:
        while True:
            status_int = next_fn(iter_ptr, id_slot)
            if status_int == Status.NOT_FOUND:
                return
            _check(status_int)
            yield NodeId(int(id_slot.bits))
    finally:
        try:
            free_fn(iter_ptr)
        except Exception:  # pragma: no cover
            pass


def ref_iter(
    create_fn,
    next_fn,
    free_fn,
    new_iter_handle,
    new_id_slot,
) -> Generator[RefId, None, None]:
    """Yield ``RefId`` values; same shape as :func:`node_iter`."""
    iter_ptr = create_fn()
    id_slot = new_id_slot()
    try:
        while True:
            status_int = next_fn(iter_ptr, id_slot)
            if status_int == Status.NOT_FOUND:
                return
            _check(status_int)
            yield RefId(int(id_slot.bits))
    finally:
        try:
            free_fn(iter_ptr)
        except Exception:  # pragma: no cover
            pass


def rep_iter(
    create_fn,
    next_fn,
    free_fn,
    new_iter_handle,
    new_id_slot,
) -> Generator[RepId, None, None]:
    """Yield ``RepId`` values; same shape as :func:`node_iter`."""
    iter_ptr = create_fn()
    id_slot = new_id_slot()
    try:
        while True:
            status_int = next_fn(iter_ptr, id_slot)
            if status_int == Status.NOT_FOUND:
                return
            _check(status_int)
            yield RepId(int(id_slot.bits))
    finally:
        try:
            free_fn(iter_ptr)
        except Exception:  # pragma: no cover
            pass
