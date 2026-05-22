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

"""Regression test for the §10.2 span lifetime contract.

``occtl._spans.span_view`` returns a ``numpy.ndarray`` whose backing memory
is owned by a parent handle. The contract is that the parent stays alive at
least as long as any array returned (or any slice thereof) is reachable —
otherwise the native pointer is freed while NumPy still indexes into it.

These tests exercise the lifetime mechanism directly (a tiny in-process
buffer plus a fake ``ffi``-shaped stub), so they run even when the native
library is not built.
"""

from __future__ import annotations

import gc
import weakref

import numpy as np
import pytest

from occtl._spans import span_view


class _FakeFfi:
    """Minimal stub of cffi's ``FFI`` providing only ``buffer(ptr, nbytes)``.

    ``span_view`` only calls ``ffi.buffer(...)``; everything else is opaque.
    We return a memoryview over a real bytearray so ``np.frombuffer`` works.
    """

    @staticmethod
    def buffer(ptr, nbytes):
        return memoryview(ptr)[:nbytes]


class _Parent:
    """Stand-in for a handle wrapper that owns the underlying bytes."""

    def __init__(self, payload: bytearray):
        self.payload = payload


def _make_parent_and_view(count: int = 3, shape=None):
    payload = bytearray(np.arange(count, dtype=np.float64).tobytes())
    parent = _Parent(payload)
    arr = span_view(
        _FakeFfi(),
        payload,
        count=count,
        dtype="float64",
        parent=parent,
        shape=shape,
    )
    return parent, arr


def test_array_sees_expected_values():
    _parent, arr = _make_parent_and_view(count=3)
    assert arr.shape == (3,)
    assert arr.dtype == np.float64
    np.testing.assert_array_equal(arr, np.array([0.0, 1.0, 2.0]))


def test_parent_survives_when_only_array_holds_it():
    parent, arr = _make_parent_and_view(count=3)
    parent_ref = weakref.ref(parent)
    del parent
    gc.collect()
    assert parent_ref() is not None, (
        "parent was reclaimed while the array still held it — "
        "lifetime hook is broken"
    )
    np.testing.assert_array_equal(arr, np.array([0.0, 1.0, 2.0]))


def test_parent_survives_through_slice():
    parent, arr = _make_parent_and_view(count=4)
    parent_ref = weakref.ref(parent)
    sliced = arr[1:3]
    del parent
    del arr
    gc.collect()
    assert parent_ref() is not None, (
        "parent was reclaimed while a slice into the view was still live"
    )
    np.testing.assert_array_equal(sliced, np.array([1.0, 2.0]))


def test_parent_released_after_array_dropped():
    parent, arr = _make_parent_and_view(count=3)
    parent_ref = weakref.ref(parent)
    del parent
    del arr
    gc.collect()
    assert parent_ref() is None, (
        "parent was kept alive after the array (and any slice) was dropped — "
        "the pin table is leaking"
    )


def test_empty_span_returns_empty_array():
    parent = _Parent(bytearray(0))
    arr = span_view(_FakeFfi(), None, count=0, dtype="float64", parent=parent)
    assert arr.size == 0
    assert arr.dtype == np.float64


def test_empty_span_respects_shape_rank():
    parent = _Parent(bytearray(0))
    arr = span_view(
        _FakeFfi(), None, count=0, dtype="float64", parent=parent, shape=(0, 3)
    )
    assert arr.shape == (0, 3)


def test_reshape_packed_xyz():
    parent, arr = _make_parent_and_view(count=6, shape=(2, 3))
    assert arr.shape == (2, 3)
    np.testing.assert_array_equal(arr, np.array([[0.0, 1.0, 2.0], [3.0, 4.0, 5.0]]))
    # Parent still pinned through the reshape.
    parent_ref = weakref.ref(parent)
    del parent
    gc.collect()
    assert parent_ref() is not None
