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

"""Boolean operations — :mod:`occtl.bool_`.

Wraps :c:`occtl_bool_fuse`/`_cut`/`_common`/`_section`/`_split`. Every op
takes a :class:`Graph` (mutated in place: the result merges into the
caller's graph) plus two sequences of :class:`NodeId` (objects and
tools) and returns the new topology-root :class:`NodeId`. History is
stored on the graph and queried through :class:`occtl.topo.Graph`.

Use a trailing underscore on the module name because ``bool`` is a Python
keyword — ``from occtl import bool_`` then ``bool_.fuse(...)``.

Regenerate the raw FFI binding when ``occtl_bool.h`` changes::

    python3 tools/abi_dump.py --output build/abi.json
    python3 bindings/python/tools/generate_facade.py
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Sequence, TYPE_CHECKING

from ._errors import _check
from ._generated import bool_ as _bool_gen
from ._ids import NodeId
from ._generated._raw import ffi, lib

if TYPE_CHECKING:
    from .topo import Graph


@dataclass(frozen=True, slots=True)
class BoolOptions:
    """Tunable parameters shared by every boolean operation.

    :attr fuzzy_value: Additional tolerance applied to every input. 0 keeps the OCCT default.
    :attr run_parallel: Enable OCCT's parallel BOP execution.
    :attr simplify_result: Run ``SimplifyResult`` after the operation.
    :attr simplify_angular_tolerance: Angular criteria for simplification.
    :attr build_history: Collect Modified/Generated/Deleted history on the
        supplied graph.
    """

    fuzzy_value: float = 0.0
    run_parallel: bool = False
    simplify_result: bool = False
    simplify_angular_tolerance: float = 1.0e-2
    build_history: bool = True


def _to_c_options(options: BoolOptions):
    c = ffi.new("occtl_bool_options_t*")
    lib.occtl_bool_options_init(c)
    c.fuzzy_value = float(options.fuzzy_value)
    c.run_parallel = 1 if options.run_parallel else 0
    c.simplify_result = 1 if options.simplify_result else 0
    c.simplify_angular_tolerance = float(options.simplify_angular_tolerance)
    c.build_history = 1 if options.build_history else 0
    return c


def _to_c_id_array(ids: Sequence[NodeId]):
    if not ids:
        return ffi.NULL, 0
    arr = ffi.new(f"occtl_node_id_t[{len(ids)}]")
    for i, node in enumerate(ids):
        arr[i].bits = int(node.bits)
    return arr, len(ids)


def _run(fn, graph: "Graph", objects: Sequence[NodeId], tools: Sequence[NodeId],
         options: BoolOptions) -> NodeId:
    opts = _to_c_options(options)
    obj_arr, n_obj = _to_c_id_array(objects)
    tool_arr, n_tool = _to_c_id_array(tools)

    out_root = ffi.new("occtl_node_id_t*")
    _check(fn(graph._as_ptr(),
              obj_arr, n_obj,
              tool_arr, n_tool,
              opts,
              out_root))

    return NodeId(int(out_root.bits))


def fuse(graph: "Graph", objects: Sequence[NodeId], tools: Sequence[NodeId],
         options: BoolOptions = BoolOptions()) -> NodeId:
    """Boolean Fuse (union) of the object and tool groups.

    Both groups must hold entities of equal dimension. Returns the new
    topology root NodeId. When ``options.build_history`` is true, history
    is recorded on ``graph`` and queried through graph history methods.
    """
    return _run(lib.occtl_bool_fuse, graph, objects, tools, options)


def cut(graph: "Graph", objects: Sequence[NodeId], tools: Sequence[NodeId],
        options: BoolOptions = BoolOptions()) -> NodeId:
    """Boolean Cut: objects minus tools."""
    return _run(lib.occtl_bool_cut, graph, objects, tools, options)


def common(graph: "Graph", objects: Sequence[NodeId], tools: Sequence[NodeId],
           options: BoolOptions = BoolOptions()) -> NodeId:
    """Boolean Common: intersection of the two argument groups."""
    return _run(lib.occtl_bool_common, graph, objects, tools, options)


def section(graph: "Graph", objects: Sequence[NodeId], tools: Sequence[NodeId],
            options: BoolOptions = BoolOptions()) -> NodeId:
    """Boolean Section: intersection edges/vertices of all arguments."""
    return _run(lib.occtl_bool_section, graph, objects, tools, options)


def split(graph: "Graph", objects: Sequence[NodeId], tools: Sequence[NodeId],
          options: BoolOptions = BoolOptions()) -> NodeId:
    """Boolean Split: partition each object using the tools as cutters."""
    return _run(lib.occtl_bool_split, graph, objects, tools, options)


# Re-export every generated wrapper. Keep hand-written names authoritative.
_SENTINEL = object()
for _name in dir(_bool_gen):
    if _name.startswith("occtl_"):
        if globals().get(_name, _SENTINEL) is _SENTINEL:
            globals()[_name] = getattr(_bool_gen, _name)
        else:
            import warnings
            warnings.warn(
                f"occtl.bool_: not exporting {_name} — already defined in bool_ namespace",
                stacklevel=2,
            )
del _name, _SENTINEL
