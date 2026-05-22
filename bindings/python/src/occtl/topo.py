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

"""Topology graph wrapper — :mod:`occtl.topo`.

Wraps ``occtl_graph_t`` and the BRepGraph builders / iterators behind a
context-managed Python class. Every method here is hand-written on top of the
auto-generated wrappers in :mod:`occtl._generated.topo`.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Generator, Optional, Sequence

from ._errors import _check
from ._generated import topo as _topo_gen
from ._handles import _Handle
from ._ids import NodeId, RefId, RepId, RepUid, Uid
from ._generated._raw import ffi, lib
from .geom import Point3


@dataclass(frozen=True, slots=True)
class GraphRootResult:
    """A newly-owned graph plus the root node produced inside it."""

    graph: "Graph"
    root: NodeId


@dataclass(frozen=True, slots=True)
class Color:
    """RGBA color attached to a graph node."""

    r: float
    g: float
    b: float
    a: float = 1.0

    def _to_c(self):
        c = ffi.new("occtl_color_rgba_t*")
        c.r = float(self.r)
        c.g = float(self.g)
        c.b = float(self.b)
        c.a = float(self.a)
        return c[0]

    @classmethod
    def _from_c(cls, c) -> "Color":
        return cls(r=float(c.r), g=float(c.g), b=float(c.b), a=float(c.a))


@dataclass(frozen=True, slots=True)
class CheckIssue:
    """Validation issue reported by :meth:`Graph.check_issues`."""

    node_id: NodeId
    context_node_id: NodeId
    status_bit: int
    severity: int


@dataclass(frozen=True, slots=True)
class BBox:
    """Axis-aligned bounding box."""

    min: Point3
    max: Point3


@dataclass(frozen=True, slots=True)
class MassProperties:
    """Graph-cache mass-property summary."""

    linear_length: float
    surface_area: float
    volume: float
    mass: float
    centre_of_mass: Point3
    inertia: tuple[float, ...]

    @classmethod
    def _from_c(cls, c) -> "MassProperties":
        return cls(
            linear_length=float(c.linear_length),
            surface_area=float(c.surface_area),
            volume=float(c.volume),
            mass=float(c.mass),
            centre_of_mass=Point3._from_c(c.centre_of_mass),
            inertia=tuple(float(c.inertia[i]) for i in range(9)),
        )


def _node_id(node: NodeId):
    out = ffi.new("occtl_node_id_t*")
    out.bits = int(node.bits if hasattr(node, "bits") else node)
    return out[0]


def occtl_topo_closest_point_to_point(graph: "Graph", node: NodeId, point: Point3) -> Point3:
    """Return the closest point on ``node`` to ``point``."""
    return graph.closest_point_to_point(node, point)


class NodeIter(_Handle):
    """Opaque node-iterator handle (``occtl_node_iter_t``). Rarely held directly."""

    _c_typename = "occtl_node_iter_t *"

    @staticmethod
    def _free_fn(ptr) -> None:  # type: ignore[override]
        lib.occtl_node_iter_free(ptr)


class Batch(_Handle):
    """Opaque batch handle (``occtl_batch_t``) for grouped graph mutations."""

    _c_typename = "occtl_batch_t *"

    @staticmethod
    def _free_fn(ptr) -> None:  # type: ignore[override]
        lib.occtl_batch_free(ptr)


# ---------------------------------------------------------------------------
# Graph
# ---------------------------------------------------------------------------

class Graph(_Handle):
    """A BRepGraph topology document.

    ``with Graph() as g:`` is the idiomatic spelling; the handle is freed when
    the context exits or when the object is garbage-collected.
    """

    _c_typename = "occtl_graph_t *"

    @staticmethod
    def _free_fn(ptr) -> None:  # type: ignore[override]
        lib.occtl_graph_free(ptr)

    # ------------------------------------------------------------------ #
    # Construction
    # ------------------------------------------------------------------ #

    def __init__(self) -> None:
        slot = ffi.new("occtl_graph_t**")
        _check(lib.occtl_graph_create(slot))
        # Use _adopt's bookkeeping but we already allocated the slot.
        self._ptr = slot[0]
        self._closed = False

    @classmethod
    def create(cls) -> "Graph":
        """Create a new empty topology graph."""
        return cls()

    @classmethod
    def _from_ptr(cls, ptr) -> "Graph":
        """Adopt a pre-existing native ``occtl_graph_t*``.

        Test entry point — exposed so the smoke test can exercise the
        NULL-handle rejection path. Userland code constructs ``Graph()``.
        """
        return cls._adopt(ptr)

    @classmethod
    def from_pointer_unsafe(cls, ptr) -> "Graph":
        """Adopt a pre-existing native ``occtl_graph_t*``.

        Public alias used by cross-language parity tests and migration helpers.
        """
        return cls._from_ptr(ptr)

    # ------------------------------------------------------------------ #
    # Builders
    # ------------------------------------------------------------------ #

    def make_vertex(
        self,
        x: float = 0.0,
        y: float = 0.0,
        z: float = 0.0,
        tolerance: float = 1e-7,
    ) -> NodeId:
        """Create a vertex at (x, y, z) with the given tolerance."""
        info = ffi.new("occtl_topo_make_vertex_info_t*")
        lib.occtl_topo_make_vertex_info_init(info)
        info.point.x = float(x)
        info.point.y = float(y)
        info.point.z = float(z)
        info.tolerance = float(tolerance)
        out_id = ffi.new("occtl_node_id_t*")
        _check(lib.occtl_topo_make_vertex(self._as_ptr(), info, out_id))
        return NodeId(int(out_id.bits))

    def make_box(self, dx: float, dy: float, dz: float) -> NodeId:
        """Create an axis-aligned box solid in this graph."""
        from .prim import BoxInfo, make_box

        return make_box(self, BoxInfo(dx=float(dx), dy=float(dy), dz=float(dz)))

    def make_sphere(self, radius: float) -> NodeId:
        """Create a sphere solid in this graph."""
        from .prim import make_sphere

        return make_sphere(self, float(radius))

    def make_cylinder(self, radius: float, height: float) -> NodeId:
        """Create a cylinder solid in this graph."""
        from .prim import CylinderInfo, make_cylinder

        return make_cylinder(
            self,
            CylinderInfo(radius=float(radius), height=float(height)),
        )

    def make_cone(self, r1: float, r2: float, height: float) -> NodeId:
        """Create a cone or truncated-cone solid in this graph."""
        from .prim import ConeInfo, make_cone

        return make_cone(
            self,
            ConeInfo(r1=float(r1), r2=float(r2), height=float(height)),
        )

    def make_torus(self, r1: float, r2: float) -> NodeId:
        """Create a torus solid in this graph."""
        from .prim import TorusInfo, make_torus

        return make_torus(self, TorusInfo(r1=float(r1), r2=float(r2)))

    def curves_to_wire(self, curves: Sequence[RepId]) -> NodeId:
        """Build a wire from a sequence of curve RepIds."""
        c_array = ffi.new("occtl_rep_id_t[]", len(curves))
        for i, c in enumerate(curves):
            c_array[i].bits = int(c.bits if hasattr(c, "bits") else c)
        out_wire = ffi.new("occtl_node_id_t*")
        _check(lib.occtl_topo_curves_to_wire(self._as_ptr(), c_array, len(curves), out_wire))
        return NodeId(int(out_wire.bits))

    def make_wedge(self, dx: float, dy: float, dz: float, ltx: float) -> NodeId:
        """Create a right-angular wedge solid in this graph."""
        from .prim import WedgeInfo, make_wedge

        return make_wedge(
            self,
            WedgeInfo(dx=float(dx), dy=float(dy), dz=float(dz), ltx=float(ltx)),
        )

    # ------------------------------------------------------------------ #
    # Cardinality
    # ------------------------------------------------------------------ #

    def _graph_count(self, fn) -> int:
        out = ffi.new("size_t *")
        _check(fn(self._as_ptr(), out))
        return int(out[0])

    @property
    def node_count(self) -> int:
        return self._graph_count(lib.occtl_graph_node_count)

    @property
    def vertex_count(self) -> int:
        return self._graph_count(lib.occtl_graph_vertex_count)

    @property
    def edge_count(self) -> int:
        return self._graph_count(lib.occtl_graph_edge_count)

    @property
    def face_count(self) -> int:
        return self._graph_count(lib.occtl_graph_face_count)

    @property
    def wire_count(self) -> int:
        return self._graph_count(lib.occtl_graph_wire_count)

    @property
    def shell_count(self) -> int:
        return self._graph_count(lib.occtl_graph_shell_count)

    @property
    def solid_count(self) -> int:
        return self._graph_count(lib.occtl_graph_solid_count)

    @property
    def compound_count(self) -> int:
        return self._graph_count(lib.occtl_graph_compound_count)

    @property
    def compsolid_count(self) -> int:
        return self._graph_count(lib.occtl_graph_compsolid_count)

    @property
    def coedge_count(self) -> int:
        return self._graph_count(lib.occtl_graph_coedge_count)

    @property
    def product_count(self) -> int:
        return self._graph_count(lib.occtl_graph_product_count)

    @property
    def occurrence_count(self) -> int:
        return self._graph_count(lib.occtl_graph_occurrence_count)

    # ------------------------------------------------------------------ #
    # Iterators
    # ------------------------------------------------------------------ #

    def _node_iter(self, create_fn) -> Generator[NodeId, None, None]:
        """Internal generator wrapping any ``occtl_graph_*_iter_create``."""
        slot = ffi.new("occtl_node_iter_t**")
        _check(create_fn(self._as_ptr(), slot))
        iter_ptr = slot[0]
        id_slot = ffi.new("occtl_node_id_t*")
        try:
            while True:
                status_int = int(lib.occtl_node_iter_next(iter_ptr, id_slot))
                if status_int == 4:  # OCCTL_NOT_FOUND
                    return
                _check(status_int)
                yield NodeId(int(id_slot.bits))
        finally:
            try:
                lib.occtl_node_iter_free(iter_ptr)
            except Exception:  # pragma: no cover
                pass

    def vertex_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_vertex_iter_create)

    def edge_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_edge_iter_create)

    def face_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_face_iter_create)

    def wire_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_wire_iter_create)

    def shell_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_shell_iter_create)

    def solid_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_solid_iter_create)

    def compound_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_compound_iter_create)

    def compsolid_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_compsolid_iter_create)

    def coedge_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_coedge_iter_create)

    def product_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_product_iter_create)

    def occurrence_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_occurrence_iter_create)

    def root_product_iter(self) -> Generator[NodeId, None, None]:
        return self._node_iter(lib.occtl_graph_root_product_iter_create)

    def vertices(self) -> tuple[NodeId, ...]:
        """Return all vertex IDs as a stable Python tuple."""
        return tuple(self.vertex_iter())

    def edges(self) -> tuple[NodeId, ...]:
        """Return all edge IDs as a stable Python tuple."""
        return tuple(self.edge_iter())

    def faces(self) -> tuple[NodeId, ...]:
        """Return all face IDs as a stable Python tuple."""
        return tuple(self.face_iter())

    def solids(self) -> tuple[NodeId, ...]:
        """Return all solid IDs as a stable Python tuple."""
        return tuple(self.solid_iter())

    # ------------------------------------------------------------------ #
    # Validation
    # ------------------------------------------------------------------ #

    def check_issues(self) -> tuple[CheckIssue, ...]:
        """Run OCCT-Light graph validation and return structured issues."""
        out_count = ffi.new("size_t*")
        _check(lib.occtl_topo_check(self._as_ptr(), ffi.NULL, 0, out_count))
        count = int(out_count[0])
        if count == 0:
            return ()
        issues = ffi.new("occtl_topo_check_issue_t[]", count)
        _check(lib.occtl_topo_check(self._as_ptr(), issues, count, out_count))
        return tuple(
            CheckIssue(
                node_id=NodeId(int(issues[i].node_id.bits)),
                context_node_id=NodeId(int(issues[i].context_node_id.bits)),
                status_bit=int(issues[i].status_bit),
                severity=int(issues[i].severity),
            )
            for i in range(int(out_count[0]))
        )

    def is_valid(self) -> bool:
        """Return ``True`` when graph validation reports no issues."""
        return not self.check_issues()

    # ------------------------------------------------------------------ #
    # Metadata
    # ------------------------------------------------------------------ #

    def set_name(self, node: NodeId, name: str) -> None:
        """Set the UTF-8 display name for ``node``."""
        data = name.encode("utf-8")
        nid = ffi.new("occtl_node_id_t*")
        nid.bits = int(node.bits)
        _check(lib.occtl_graph_name_set(self._as_ptr(), nid[0], data, len(data)))

    def name(self, node: NodeId) -> str:
        """Return the display name for ``node`` or ``\"\"`` when unset."""
        nid = ffi.new("occtl_node_id_t*")
        nid.bits = int(node.bits)
        required = ffi.new("size_t*")
        _check(
            lib.occtl_graph_name_get(
                self._as_ptr(), nid[0], ffi.NULL, 0, required,
            ),
        )
        buf = ffi.new("char[]", required[0])
        _check(
            lib.occtl_graph_name_get(
                self._as_ptr(), nid[0], buf, required[0], required,
            ),
        )
        return ffi.string(buf).decode("utf-8", errors="replace")

    def set_color(
        self,
        node: NodeId,
        color: Color | tuple[float, float, float] | tuple[float, float, float, float],
    ) -> None:
        """Set the RGBA color for ``node``."""
        if not isinstance(color, Color):
            color = Color(*color)
        nid = ffi.new("occtl_node_id_t*")
        nid.bits = int(node.bits)
        _check(lib.occtl_graph_color_set(self._as_ptr(), nid[0], color._to_c()))

    def color(self, node: NodeId) -> Color:
        """Return the RGBA color for ``node`` or opaque white when unset."""
        nid = ffi.new("occtl_node_id_t*")
        nid.bits = int(node.bits)
        out = ffi.new("occtl_color_rgba_t*")
        _check(lib.occtl_graph_color_get(self._as_ptr(), nid[0], out))
        return Color._from_c(out[0])

    def unset_color(self, node: NodeId) -> None:
        """Remove any explicit color attached to ``node``."""
        nid = ffi.new("occtl_node_id_t*")
        nid.bits = int(node.bits)
        _check(lib.occtl_graph_color_unset(self._as_ptr(), nid[0]))

    # ------------------------------------------------------------------ #
    # Vertex queries
    # ------------------------------------------------------------------ #

    def vertex_point(self, vertex: NodeId):
        """Return the (x, y, z) of a vertex as a :class:`geom.Point3`."""
        from .geom import Point3

        slot = ffi.new("occtl_point3_t*")
        nid = ffi.new("occtl_node_id_t*")
        nid.bits = int(vertex.bits if hasattr(vertex, "bits") else vertex)
        _check(lib.occtl_topo_vertex_point(self._as_ptr(), nid[0], slot))
        return Point3._from_c(slot[0])

    def vertex_tolerance(self, vertex: NodeId) -> float:
        slot = ffi.new("double*")
        nid = ffi.new("occtl_node_id_t*")
        nid.bits = int(vertex.bits)
        _check(lib.occtl_topo_vertex_tolerance(self._as_ptr(), nid[0], slot))
        return float(slot[0])

    # ------------------------------------------------------------------ #
    # Measurement shortcuts
    # ------------------------------------------------------------------ #

    def bounding_box(self, node: NodeId):
        out = ffi.new("occtl_select_bbox_t*")
        _check(lib.occtl_graph_bbox_get(self._as_ptr(), _node_id(node), out))
        return BBox(min=Point3._from_c(out.min), max=Point3._from_c(out.max))

    def volume(self, node: NodeId) -> float:
        return self._measure(node, lib.OCCTL_SELECT_MEASURE_VOLUME)

    def area(self, node: NodeId) -> float:
        return self._measure(node, lib.OCCTL_SELECT_MEASURE_FACE_AREA)

    def edge_length(self, edge: NodeId) -> float:
        return self._measure(edge, lib.OCCTL_SELECT_MEASURE_EDGE_LENGTH)

    def wire_length(self, wire: NodeId) -> float:
        return self._measure(wire, lib.OCCTL_SELECT_MEASURE_WIRE_LENGTH)

    def mass_properties(self, node: NodeId):
        out = ffi.new("occtl_graph_mass_properties_t*")
        _check(lib.occtl_graph_mass_properties_get(self._as_ptr(), _node_id(node), out))
        return MassProperties._from_c(out[0])

    def closest_point_to_point(self, node: NodeId, point: Point3):
        out = ffi.new("occtl_point3_t*")
        _check(lib.occtl_topo_closest_point_to_point(self._as_ptr(), _node_id(node), point._to_c(), out))
        return Point3._from_c(out[0])

    def _measure(self, node: NodeId, kind: int) -> float:
        out = ffi.new("double*")
        _check(lib.occtl_graph_measure_get(self._as_ptr(), _node_id(node), kind, out))
        return float(out[0])

    # ------------------------------------------------------------------ #
    # Identity round-trip
    # ------------------------------------------------------------------ #

    def uid_of(self, node: NodeId) -> Uid:
        slot = ffi.new("occtl_uid_t*")
        nid = ffi.new("occtl_node_id_t*")
        nid.bits = int(node.bits)
        _check(lib.occtl_graph_uid_from_node_id(self._as_ptr(), nid[0], slot))
        return Uid(int(slot.bits))

    def node_id_of(self, uid: Uid) -> NodeId:
        slot = ffi.new("occtl_node_id_t*")
        u = ffi.new("occtl_uid_t*")
        u.bits = int(uid.bits)
        _check(lib.occtl_graph_node_id_from_uid(self._as_ptr(), u[0], slot))
        return NodeId(int(slot.bits))

    # ------------------------------------------------------------------ #
    # Modeling graph-result helpers
    # ------------------------------------------------------------------ #

    def transformed(self, root: NodeId, transform) -> GraphRootResult:
        """Return a transformed copy of ``root`` in a newly-owned graph."""
        out_graph = ffi.new("occtl_graph_t**")
        out_root = ffi.new("occtl_node_id_t*")
        c_transform = transform._to_c() if hasattr(transform, "_to_c") else transform
        nid = ffi.new("occtl_node_id_t*")
        nid.bits = int(root.bits)
        _check(lib.occtl_topo_transformed(self._as_ptr(), nid[0], c_transform, out_graph, out_root))
        return GraphRootResult(Graph._adopt(out_graph[0]), NodeId(int(out_root.bits)))

    def translated(self, root: NodeId, delta) -> GraphRootResult:
        """Return a translated copy of ``root`` in a newly-owned graph."""
        from .geom import Vector3, translation

        if not isinstance(delta, Vector3):
            delta = Vector3(*delta)
        return self.transformed(root, translation(delta))

    def rotated(self, root: NodeId, axis, angle: float) -> GraphRootResult:
        """Return a rotated copy of ``root`` in a newly-owned graph."""
        from .geom import rotation

        return self.transformed(root, rotation(axis, float(angle)))

    def scaled(self, root: NodeId, center, factor: float) -> GraphRootResult:
        """Return a uniformly scaled copy of ``root`` in a newly-owned graph."""
        from .geom import Point3, scale

        if not isinstance(center, Point3):
            center = Point3(*center)
        return self.transformed(root, scale(center, float(factor)))

    def _blend_edges(
        self,
        root: NodeId,
        edges: Sequence[NodeId],
        *,
        radius: float,
        chamfer_mode: int,
        chamfer_dist1: float = 0.0,
        chamfer_dist2: float = 0.0,
    ) -> GraphRootResult:
        opts = ffi.new("occtl_topo_edge_blend_options_t*")
        lib.occtl_topo_edge_blend_options_init(opts)
        opts.root.bits = int(root.bits)
        edge_buf = ffi.new("occtl_node_id_t[]", len(edges))
        for i, edge in enumerate(edges):
            edge_buf[i].bits = int(edge.bits if hasattr(edge, "bits") else edge)
        opts.edges = edge_buf
        opts.edge_count = len(edges)
        opts.radius = float(radius)
        opts.chamfer_mode = int(chamfer_mode)
        opts.chamfer_dist1 = float(chamfer_dist1)
        opts.chamfer_dist2 = float(chamfer_dist2)
        out_graph = ffi.new("occtl_graph_t**")
        out_root = ffi.new("occtl_node_id_t*")
        _check(lib.occtl_topo_blend_edges(self._as_ptr(), opts, out_graph, out_root))
        return GraphRootResult(Graph._adopt(out_graph[0]), NodeId(int(out_root.bits)))

    def fillet_edges(self, root: NodeId, edges: Sequence[NodeId], radius: float) -> GraphRootResult:
        """Fillet selected edges of ``root`` and return the resulting graph/root."""
        return self._blend_edges(root, edges, radius=float(radius), chamfer_mode=0)

    def chamfer_edges(
        self,
        root: NodeId,
        edges: Sequence[NodeId],
        distance1: float,
        distance2: Optional[float] = None,
    ) -> GraphRootResult:
        """Chamfer selected edges of ``root`` and return the resulting graph/root."""
        d2 = distance1 if distance2 is None else distance2
        return self._blend_edges(
            root,
            edges,
            radius=0.0,
            chamfer_mode=1,
            chamfer_dist1=float(distance1),
            chamfer_dist2=float(d2),
        )


    def history_modified(self, input_uid: Uid) -> list[Uid]:
        """Return UIDs modified from ``input_uid`` in this graph's recorded history."""
        return [Uid(int(u.bits)) for u in _topo_gen.occtl_graph_history_modified(self._as_ptr(), input_uid)]

    def history_generated(self, input_uid: Uid) -> list[Uid]:
        """Return UIDs generated from ``input_uid`` in this graph's recorded history."""
        return [Uid(int(u.bits)) for u in _topo_gen.occtl_graph_history_generated(self._as_ptr(), input_uid)]

    def history_deleted_all(self) -> list[Uid]:
        """Return all UIDs deleted in this graph's recorded history."""
        return [Uid(int(u.bits)) for u in _topo_gen.occtl_graph_history_deleted_all(self._as_ptr())]

    def rep_uid_from_rep_id(self, rep_id: RepId) -> RepUid:
        """Return the persistent representation UID for ``rep_id``."""
        return _topo_gen.occtl_graph_rep_uid_from_rep_id(self._as_ptr(), rep_id)

    def rep_id_from_rep_uid(self, rep_uid: RepUid) -> RepId:
        """Resolve a persistent representation UID to the current ``RepId``."""
        return _topo_gen.occtl_graph_rep_id_from_rep_uid(self._as_ptr(), rep_uid)

    def fuse(self, objects: Sequence[NodeId], tools: Sequence[NodeId], options=None):
        """Boolean union shortcut returning the result NodeId."""
        from . import bool_

        return bool_.fuse(
            self,
            objects,
            tools,
            bool_.BoolOptions() if options is None else options,
        )

    def cut(self, objects: Sequence[NodeId], tools: Sequence[NodeId], options=None):
        """Boolean difference shortcut returning the result NodeId."""
        from . import bool_

        return bool_.cut(
            self,
            objects,
            tools,
            bool_.BoolOptions() if options is None else options,
        )

    def common(self, objects: Sequence[NodeId], tools: Sequence[NodeId], options=None):
        """Boolean intersection shortcut returning the result NodeId."""
        from . import bool_

        return bool_.common(
            self,
            objects,
            tools,
            bool_.BoolOptions() if options is None else options,
        )

    def section(self, objects: Sequence[NodeId], tools: Sequence[NodeId], options=None):
        """Boolean section shortcut returning the result NodeId."""
        from . import bool_

        return bool_.section(
            self,
            objects,
            tools,
            bool_.BoolOptions() if options is None else options,
        )

    def split(self, objects: Sequence[NodeId], tools: Sequence[NodeId], options=None):
        """Boolean split shortcut returning the result NodeId."""
        from . import bool_

        return bool_.split(
            self,
            objects,
            tools,
            bool_.BoolOptions() if options is None else options,
        )


# ---------------------------------------------------------------------------
# Symbol coverage re-exports.
# ---------------------------------------------------------------------------

for _name in dir(_topo_gen):
    if _name.startswith("occtl_"):
        globals()[_name] = getattr(_topo_gen, _name)

del _name
