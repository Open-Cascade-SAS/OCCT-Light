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

"""BRep I/O — :mod:`occtl.io_brep`.

Wraps :c:`occtl_io_brep_read` / :c:`occtl_io_brep_write` for the
Open CASCADE BRep format (``.brep``, ``.rle``).
"""

from __future__ import annotations

from pathlib import Path

from ._errors import _check
from ._generated import io_brep as _io_brep_gen
from ._ids import NodeId
from ._generated._raw import ffi, lib
from .topo import Graph, GraphRootResult


def _node_id(value: NodeId):
    c = ffi.new("occtl_node_id_t*")
    c.bits = int(value.bits if hasattr(value, "bits") else value)
    return c[0]


def write(graph: Graph, root: NodeId, path: str | Path, *, write_triangulation: bool = True) -> None:
    """Write ``root`` from ``graph`` to an OCCT BRep file."""
    opts = ffi.new("occtl_io_brep_write_options_t*")
    lib.occtl_io_brep_write_options_init(opts)
    opts.write_triangulation = 1 if write_triangulation else 0
    _check(
        lib.occtl_io_brep_write(
            graph._as_ptr(),
            _node_id(root),
            str(path).encode("utf-8"),
            opts,
        ),
    )


def read(path: str | Path) -> GraphRootResult:
    """Read an OCCT BRep file into a newly-owned graph."""
    out_graph = ffi.new("occtl_graph_t**")
    out_root = ffi.new("occtl_node_id_t*")
    _check(lib.occtl_io_brep_read(str(path).encode("utf-8"), out_graph, out_root))
    return GraphRootResult(Graph._adopt(out_graph[0]), NodeId(int(out_root.bits)))


_SENTINEL = object()
for _name in dir(_io_brep_gen):
    if _name.startswith("occtl_"):
        if globals().get(_name, _SENTINEL) is _SENTINEL:
            globals()[_name] = getattr(_io_brep_gen, _name)
        else:
            import warnings
            warnings.warn(
                f"occtl.io_brep: skipping {_name} — already defined in io_brep namespace",
                stacklevel=2,
            )
del _name, _SENTINEL
