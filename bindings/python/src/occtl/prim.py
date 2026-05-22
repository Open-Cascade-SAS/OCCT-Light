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

"""Primitive solids — :mod:`occtl.prim`.

The typed options and common builders in this module are generated from
``build/abi.json`` by :mod:`bindings.python.tools.generate_facade`.  Raw
``occtl_prim_*`` wrappers are still re-exported as escape hatches for API
coverage that has not yet been promoted into the typed facade.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Sequence

from ._generated import prim as _prim_gen
from ._ids import NodeId
from ._generated._raw import ffi, lib
from ._generated._typed.prim import *  # noqa: F403
from ._generated._typed.prim import _IDIOMATIC_OVERRIDES
from ._errors import _check


@dataclass(frozen=True, slots=True)
class OffsetOptions:
    """Options shared by offset-shape and thick-solid construction."""

    tolerance: float = 1.0e-3
    mode: int = 0
    join: int = 0
    intersection: bool = False
    self_intersection: bool = False
    remove_internal_edges: bool = False


def _write_offset_options(c, options: OffsetOptions) -> None:
    c.tolerance = float(options.tolerance)
    c.mode = int(options.mode)
    c.join = int(options.join)
    c.intersection = 1 if options.intersection else 0
    c.self_intersection = 1 if options.self_intersection else 0
    c.remove_internal_edges = 1 if options.remove_internal_edges else 0


def make_offset_shape(graph, shape: NodeId, offset: float, options: OffsetOptions | None = None) -> NodeId:
    """Build an offset copy of ``shape`` inside ``graph``."""
    opts = OffsetOptions() if options is None else options
    c = ffi.new("occtl_prim_offset_shape_info_t*")
    lib.occtl_prim_offset_shape_info_init(c)
    c.shape.bits = int(shape.bits)
    c.offset = float(offset)
    _write_offset_options(c, opts)
    out = ffi.new("occtl_node_id_t*")
    _check(lib.occtl_prim_make_offset_shape(graph._as_ptr(), c, out))
    return NodeId(int(out.bits))


def make_thick_solid(
    graph,
    solid: NodeId,
    closing_faces: Sequence[NodeId] = (),
    offset: float = 0.0,
    options: OffsetOptions | None = None,
) -> NodeId:
    """Hollow ``solid`` into a thick-walled solid inside ``graph``."""
    opts = OffsetOptions() if options is None else options
    c = ffi.new("occtl_prim_thick_solid_info_t*")
    lib.occtl_prim_thick_solid_info_init(c)
    c.solid.bits = int(solid.bits)
    c.offset = float(offset)
    _write_offset_options(c, opts)
    face_array = None
    if closing_faces:
        face_array = ffi.new("occtl_node_id_t[]", len(closing_faces))
        for index, face in enumerate(closing_faces):
            face_array[index].bits = int(face.bits)
        c.closing_faces = face_array
        c.closing_face_count = len(closing_faces)
    out = ffi.new("occtl_node_id_t*")
    _check(lib.occtl_prim_make_thick_solid(graph._as_ptr(), c, out))
    return NodeId(int(out.bits))


_SENTINEL = object()
for _name in dir(_prim_gen):
    if not _name.startswith("occtl_"):
        continue
    if _name in _IDIOMATIC_OVERRIDES:
        continue
    if globals().get(_name, _SENTINEL) is _SENTINEL:
        globals()[_name] = getattr(_prim_gen, _name)
    else:
        import warnings
        warnings.warn(
            f"occtl.prim: skipping {_name} — already defined in prim namespace",
            stacklevel=2,
        )
del _name, _SENTINEL
