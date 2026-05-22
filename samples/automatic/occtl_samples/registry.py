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

"""Sample model registry used by ``samples/python/run_samples.py``.

The registry is intentionally import-light: it does not import ``occtl`` until a
sample is built, so static tests can verify sample coverage without a native
library on the loader path.
"""

from __future__ import annotations

from dataclasses import dataclass
import math
from typing import Callable


@dataclass(slots=True)
class SampleBuild:
    """An owning graph and root node produced by a sample builder."""

    graph: object
    root: object


@dataclass(frozen=True, slots=True)
class SampleSpec:
    """Registered Python sample."""

    name: str
    category: str
    description: str
    build: Callable[[], SampleBuild]


def _placement(x: float, y: float, z: float):
    return _placement_with_dirs(x, y, z, (0.0, 0.0, 1.0), (1.0, 0.0, 0.0))


def _placement_with_dirs(
    x: float,
    y: float,
    z: float,
    x_dir: tuple[float, float, float],
    x_ref: tuple[float, float, float],
):
    from occtl.geom import Axis2Placement, Direction3, Point3

    return Axis2Placement(
        location=Point3(float(x), float(y), float(z)),
        x_dir=Direction3(*x_dir),
        x_dir_ref=Direction3(*x_ref),
    )


def _placement_axis_x(x: float, y: float, z: float):
    return _placement_with_dirs(x, y, z, (1.0, 0.0, 0.0), (0.0, 1.0, 0.0))


def _placement_axis_y(x: float, y: float, z: float):
    return _placement_with_dirs(x, y, z, (0.0, 1.0, 0.0), (1.0, 0.0, 0.0))


def _single(kind: str, **kwargs: float) -> Callable[[], SampleBuild]:
    def build() -> SampleBuild:
        from occtl import Graph, prim

        graph = Graph()
        if kind == "box":
            root = prim.make_box(
                graph,
                prim.BoxInfo(
                    dx=kwargs["dx"],
                    dy=kwargs["dy"],
                    dz=kwargs["dz"],
                    placement=_placement(kwargs.get("x", 0.0), kwargs.get("y", 0.0), kwargs.get("z", 0.0)),
                ),
            )
        elif kind == "sphere":
            root = prim.make_sphere(
                graph,
                prim.SphereInfo(
                    radius=kwargs["radius"],
                    angle1=kwargs.get("angle1", -math.pi / 2.0),
                    angle2=kwargs.get("angle2", math.pi / 2.0),
                    angle=kwargs.get("angle", math.tau),
                    placement=_placement(kwargs.get("x", 0.0), kwargs.get("y", 0.0), kwargs.get("z", 0.0)),
                ),
            )
        elif kind == "cylinder":
            root = prim.make_cylinder(
                graph,
                prim.CylinderInfo(
                    radius=kwargs["radius"],
                    height=kwargs["height"],
                    angle=kwargs.get("angle", math.tau),
                    placement=_placement(kwargs.get("x", 0.0), kwargs.get("y", 0.0), kwargs.get("z", 0.0)),
                ),
            )
        elif kind == "cone":
            root = prim.make_cone(
                graph,
                prim.ConeInfo(
                    r1=kwargs["r1"],
                    r2=kwargs["r2"],
                    height=kwargs["height"],
                    angle=kwargs.get("angle", math.tau),
                    placement=_placement(kwargs.get("x", 0.0), kwargs.get("y", 0.0), kwargs.get("z", 0.0)),
                ),
            )
        elif kind == "torus":
            root = prim.make_torus(
                graph,
                prim.TorusInfo(
                    r1=kwargs["r1"],
                    r2=kwargs["r2"],
                    angle1=kwargs.get("angle1", 0.0),
                    angle2=kwargs.get("angle2", math.tau),
                    angle=kwargs.get("angle", math.tau),
                    placement=_placement(kwargs.get("x", 0.0), kwargs.get("y", 0.0), kwargs.get("z", 0.0)),
                ),
            )
        elif kind == "wedge":
            root = prim.make_wedge(
                graph,
                prim.WedgeInfo(
                    dx=kwargs["dx"],
                    dy=kwargs["dy"],
                    dz=kwargs["dz"],
                    ltx=kwargs["ltx"],
                    placement=_placement(kwargs.get("x", 0.0), kwargs.get("y", 0.0), kwargs.get("z", 0.0)),
                ),
            )
        else:
            graph.close()
            raise ValueError(f"unknown sample primitive kind: {kind}")
        graph.set_name(root, kwargs.get("label", kind))
        graph.set_color(root, (kwargs.get("r", 0.62), kwargs.get("g", 0.66), kwargs.get("b", 0.70), 1.0))
        return SampleBuild(graph=graph, root=root)

    return build


def _fused_bottle() -> SampleBuild:
    from occtl import Graph, bool_, prim

    graph = Graph()
    body = prim.make_cylinder(graph, prim.CylinderInfo(radius=1.2, height=3.0, placement=_placement(0.0, 0.0, 0.0)))
    shoulder = prim.make_sphere(
        graph,
        prim.SphereInfo(radius=1.15, angle1=0.0, angle2=math.pi / 2.0, placement=_placement(0.0, 0.0, 2.65)),
    )
    neck = prim.make_cylinder(graph, prim.CylinderInfo(radius=0.38, height=1.25, placement=_placement(0.0, 0.0, 3.0)))
    cap = prim.make_cylinder(graph, prim.CylinderInfo(radius=0.50, height=0.38, placement=_placement(0.0, 0.0, 4.05)))
    result = graph.fuse([body, shoulder], [neck, cap], bool_.BoolOptions(build_history=False, simplify_result=True))
    graph.set_name(result.root, "bottle")
    graph.set_color(result.root, (0.30, 0.55, 0.72, 1.0))
    return SampleBuild(graph=graph, root=result.root)


def _fused_pawn() -> SampleBuild:
    from occtl import Graph, bool_, prim

    graph = Graph()
    base = prim.make_cylinder(graph, prim.CylinderInfo(radius=0.9, height=0.25, placement=_placement(0.0, 0.0, 0.0)))
    stem = prim.make_cylinder(graph, prim.CylinderInfo(radius=0.35, height=1.2, placement=_placement(0.0, 0.0, 0.2)))
    head = prim.make_sphere(graph, prim.SphereInfo(radius=0.48, placement=_placement(0.0, 0.0, 1.45)))
    result = graph.fuse([base, stem], [head], bool_.BoolOptions(build_history=False, simplify_result=True))
    graph.set_name(result.root, "pawn")
    graph.set_color(result.root, (0.78, 0.62, 0.35, 1.0))
    return SampleBuild(graph=graph, root=result.root)


def _box_cut_tunnel() -> SampleBuild:
    from occtl import Graph, bool_, prim

    graph = Graph()
    block = prim.make_box(graph, prim.BoxInfo(dx=8.0, dy=5.0, dz=3.0, placement=_placement(-4.0, -2.5, -1.5)))
    cutters = [
        prim.make_cylinder(graph, prim.CylinderInfo(radius=0.78, height=10.0, placement=_placement_axis_x(-5.0, 0.0, 0.0))),
        prim.make_cylinder(graph, prim.CylinderInfo(radius=0.48, height=7.0, placement=_placement_axis_y(0.0, -3.5, 0.75))),
        prim.make_cylinder(graph, prim.CylinderInfo(radius=0.38, height=4.6, placement=_placement(-2.35, 1.35, -2.3))),
        prim.make_cylinder(graph, prim.CylinderInfo(radius=0.38, height=4.6, placement=_placement(2.35, 1.35, -2.3))),
    ]
    result = graph.cut([block], cutters, bool_.BoolOptions(build_history=False, simplify_result=True))
    graph.set_name(result.root, "box_cut_tunnel")
    graph.set_color(result.root, (0.58, 0.64, 0.52, 1.0))
    return SampleBuild(graph=graph, root=result.root)


def _stacked_tower() -> SampleBuild:
    from occtl import Graph, bool_, prim

    graph = Graph()
    a = prim.make_box(graph, prim.BoxInfo(dx=1.8, dy=1.8, dz=0.45, placement=_placement(-0.9, -0.9, 0.0)))
    b = prim.make_cylinder(graph, prim.CylinderInfo(radius=0.75, height=1.4, placement=_placement(0.0, 0.0, 0.35)))
    c = prim.make_cone(graph, prim.ConeInfo(r1=0.72, r2=0.2, height=0.9, placement=_placement(0.0, 0.0, 1.65)))
    result = graph.fuse([a, b], [c], bool_.BoolOptions(build_history=False, simplify_result=True))
    graph.set_name(result.root, "stacked_tower")
    graph.set_color(result.root, (0.45, 0.48, 0.68, 1.0))
    return SampleBuild(graph=graph, root=result.root)


def _cut_hole_grid(graph, root, rows: int, cols: int, sx: float, sy: float, radius: float, height: float):
    from occtl import bool_, prim

    cutters = []
    x0 = -0.5 * (cols - 1) * sx
    y0 = -0.5 * (rows - 1) * sy
    for row in range(rows):
        for col in range(cols):
            cutters.append(
                prim.make_cylinder(
                    graph,
                    prim.CylinderInfo(
                        radius=radius,
                        height=height,
                        placement=_placement(x0 + col * sx, y0 + row * sy, -0.5 * height),
                    ),
                ),
            )
    return graph.cut([root], cutters, bool_.BoolOptions(build_history=False, simplify_result=True)).root


def _fuse_many(graph, solids):
    from occtl import bool_

    root = solids[0]
    for solid in solids[1:]:
        root = graph.fuse([root], [solid], bool_.BoolOptions(build_history=False, simplify_result=True)).root
    return root


def _perforated_plate(
    name: str,
    rows: int,
    cols: int,
    radius: float,
    edge_treatment: str = "none",
    edge_size: float = 0.0,
) -> Callable[[], SampleBuild]:
    def build() -> SampleBuild:
        from occtl import prim

        graph = __import__("occtl").Graph()
        root = prim.make_box(
            graph,
            prim.BoxInfo(
                dx=cols * 0.8 + 1.2,
                dy=rows * 0.8 + 1.2,
                dz=0.35,
                placement=_placement(-0.5 * (cols * 0.8 + 1.2), -0.5 * (rows * 0.8 + 1.2), 0.0),
            ),
        )
        if edge_treatment == "chamfer":
            edges = list(graph.edges())[:24]
            result = graph.chamfer_edges(root, edges, edge_size)
            graph.close()
            graph, root = result.graph, result.root
        elif edge_treatment == "fillet":
            edges = list(graph.edges())[:24]
            result = graph.fillet_edges(root, edges, edge_size)
            graph.close()
            graph, root = result.graph, result.root
        root = _cut_hole_grid(graph, root, rows, cols, 0.8, 0.8, radius, 1.2)
        graph.set_name(root, name)
        graph.set_color(root, (0.42, 0.52, 0.58, 1.0))
        return SampleBuild(graph=graph, root=root)

    return build


def _bolt_flange(name: str, bolt_count: int, outer_radius: float, bolt_radius: float) -> Callable[[], SampleBuild]:
    def build() -> SampleBuild:
        from occtl import bool_, prim

        graph = __import__("occtl").Graph()
        root = prim.make_cylinder(graph, prim.CylinderInfo(radius=outer_radius, height=0.55, placement=_placement(0.0, 0.0, 0.0)))
        cutters = [
            prim.make_cylinder(graph, prim.CylinderInfo(radius=outer_radius * 0.28, height=1.4, placement=_placement(0.0, 0.0, -0.45)))
        ]
        for i in range(bolt_count):
            angle = math.tau * i / bolt_count
            cutters.append(
                prim.make_cylinder(
                    graph,
                    prim.CylinderInfo(
                        radius=bolt_radius,
                        height=1.4,
                        placement=_placement(math.cos(angle) * outer_radius * 0.62, math.sin(angle) * outer_radius * 0.62, -0.45),
                    ),
                ),
            )
        root = graph.cut([root], cutters, bool_.BoolOptions(build_history=False, simplify_result=True)).root
        graph.set_name(root, name)
        graph.set_color(root, (0.56, 0.49, 0.40, 1.0))
        return SampleBuild(graph=graph, root=root)

    return build


def _offset_perforated_plate(name: str, rows: int, cols: int, radius: float, offset: float) -> Callable[[], SampleBuild]:
    def build() -> SampleBuild:
        from occtl import prim

        graph = __import__("occtl").Graph()
        root = prim.make_box(
            graph,
            prim.BoxInfo(
                dx=cols * 0.75 + 1.2,
                dy=rows * 0.75 + 1.2,
                dz=0.32,
                placement=_placement(-0.5 * (cols * 0.75 + 1.2), -0.5 * (rows * 0.75 + 1.2), 0.0),
            ),
        )
        root = prim.make_offset_shape(
            graph,
            root,
            offset,
            prim.OffsetOptions(intersection=True, remove_internal_edges=True),
        )
        root = _cut_hole_grid(graph, root, rows, cols, 0.75, 0.75, radius, 1.4)
        graph.set_name(root, name)
        graph.set_color(root, (0.50, 0.48, 0.56, 1.0))
        return SampleBuild(graph=graph, root=root)

    return build


def _thick_drilled_tray(name: str, rows: int, cols: int, radius: float, wall: float) -> Callable[[], SampleBuild]:
    def build() -> SampleBuild:
        from occtl import bool_, prim

        graph = __import__("occtl").Graph()
        root = prim.make_box(
            graph,
            prim.BoxInfo(
                dx=cols * 0.7 + 1.0,
                dy=rows * 0.7 + 1.0,
                dz=1.25,
                placement=_placement(-0.5 * (cols * 0.7 + 1.0), -0.5 * (rows * 0.7 + 1.0), 0.0),
            ),
        )
        root = prim.make_thick_solid(graph, root, list(graph.faces())[:1], -wall, prim.OffsetOptions(intersection=True))
        cutters = []
        x0 = -0.5 * (cols - 1) * 0.7
        y0 = -0.5 * (rows - 1) * 0.7
        for row in range(rows):
            for col in range(cols):
                cutters.append(
                    prim.make_cylinder(
                        graph,
                        prim.CylinderInfo(
                            radius=radius,
                            height=2.1,
                            placement=_placement(x0 + col * 0.7, y0 + row * 0.7, -0.45),
                        ),
                    ),
                )
        root = graph.cut([root], cutters, bool_.BoolOptions(build_history=False, simplify_result=True)).root
        graph.set_name(root, name)
        graph.set_color(root, (0.46, 0.44, 0.40, 1.0))
        return SampleBuild(graph=graph, root=root)

    return build


def _cross_drilled_manifold(name: str, x_ports: int, y_ports: int, deck_holes: int) -> Callable[[], SampleBuild]:
    def build() -> SampleBuild:
        from occtl import bool_, prim

        graph = __import__("occtl").Graph()
        root = prim.make_box(graph, prim.BoxInfo(dx=12.0, dy=7.5, dz=4.0, placement=_placement(-6.0, -3.75, -2.0)))
        cutters = []
        for index in range(x_ports):
            y = -2.7 + index * (5.4 / max(x_ports - 1, 1))
            radius = 0.34 if index % 2 == 0 else 0.27
            cutters.append(prim.make_cylinder(graph, prim.CylinderInfo(radius=radius, height=14.0, placement=_placement_axis_x(-7.0, y, -0.58))))
            cutters.append(prim.make_cylinder(graph, prim.CylinderInfo(radius=radius * 0.72, height=14.0, placement=_placement_axis_x(-7.0, y, 1.0))))
        for index in range(y_ports):
            x = -4.8 + index * (9.6 / max(y_ports - 1, 1))
            cutters.append(prim.make_cylinder(graph, prim.CylinderInfo(radius=0.24, height=9.5, placement=_placement_axis_y(x, -4.75, 0.2))))
        x0 = -0.5 * (deck_holes - 1) * 0.82
        for index in range(deck_holes):
            cutters.append(prim.make_cylinder(graph, prim.CylinderInfo(radius=0.18, height=5.2, placement=_placement(x0 + index * 0.82, 2.55, -2.6))))
        root = graph.cut([root], cutters, bool_.BoolOptions(build_history=False, simplify_result=True)).root
        result = graph.chamfer_edges(root, list(graph.edges())[: min(48, len(graph.edges()))], 0.045)
        graph.close()
        graph, root = result.graph, result.root
        graph.set_name(root, name)
        graph.set_color(root, (0.47, 0.50, 0.45, 1.0))
        return SampleBuild(graph=graph, root=root)

    return build


def _spec(name: str, category: str, description: str, build: Callable[[], SampleBuild]) -> SampleSpec:
    return SampleSpec(name=name, category=category, description=description, build=build)


def _complex_specs() -> tuple[SampleSpec, ...]:
    specs: list[SampleSpec] = []

    plate_configs = (
        (8, 10, 0.17, "chamfer", 0.030),
        (8, 12, 0.16, "chamfer", 0.030),
        (8, 14, 0.15, "none", 0.0),
        (8, 16, 0.13, "none", 0.0),
        (9, 10, 0.16, "fillet", 0.025),
        (9, 12, 0.15, "none", 0.0),
        (9, 14, 0.13, "chamfer", 0.025),
        (9, 16, 0.11, "none", 0.0),
        (10, 10, 0.15, "none", 0.0),
        (10, 12, 0.145, "none", 0.0),
        (10, 14, 0.125, "chamfer", 0.025),
        (10, 16, 0.11, "none", 0.0),
        (11, 10, 0.145, "fillet", 0.020),
        (11, 12, 0.13, "none", 0.0),
        (11, 14, 0.115, "none", 0.0),
        (11, 16, 0.10, "chamfer", 0.020),
        (12, 10, 0.135, "none", 0.0),
        (12, 12, 0.13, "none", 0.0),
        (12, 14, 0.11, "none", 0.0),
        (12, 16, 0.10, "none", 0.0),
        (13, 10, 0.125, "chamfer", 0.020),
        (13, 12, 0.115, "none", 0.0),
        (13, 14, 0.105, "none", 0.0),
        (14, 10, 0.115, "none", 0.0),
        (14, 12, 0.105, "none", 0.0),
        (15, 10, 0.105, "none", 0.0),
        (15, 12, 0.095, "none", 0.0),
        (16, 10, 0.100, "chamfer", 0.018),
        (16, 12, 0.090, "none", 0.0),
        (16, 14, 0.080, "none", 0.0),
        (18, 10, 0.090, "none", 0.0),
        (18, 12, 0.080, "none", 0.0),
        (18, 14, 0.070, "none", 0.0),
        (20, 10, 0.080, "none", 0.0),
        (20, 12, 0.070, "none", 0.0),
        (20, 14, 0.065, "none", 0.0),
        (20, 16, 0.060, "none", 0.0),
        (18, 16, 0.066, "chamfer", 0.016),
        (20, 18, 0.054, "none", 0.0),
        (22, 14, 0.060, "fillet", 0.014),
        (22, 16, 0.056, "none", 0.0),
        (22, 18, 0.050, "none", 0.0),
        (24, 14, 0.058, "chamfer", 0.014),
        (24, 16, 0.052, "none", 0.0),
        (24, 18, 0.048, "none", 0.0),
        (26, 16, 0.048, "none", 0.0),
    )
    for rows, cols, radius, treatment, edge_size in plate_configs:
        holes = rows * cols
        suffix = f"_{treatment}" if treatment != "none" else ""
        name = f"perforated_plate_{rows:02d}x{cols:02d}_{holes:03d}_holes{suffix}"
        description = f"Dense boolean-cut plate with {holes} holes"
        if treatment == "chamfer":
            description = f"Chamfered dense boolean-cut plate with {holes} holes"
        elif treatment == "fillet":
            description = f"Filleted dense boolean-cut plate with {holes} holes"
        specs.append(
            _spec(
                name,
                "complex",
                description,
                _perforated_plate(name, rows, cols, radius, treatment, edge_size),
            ),
        )

    for bolt_count, outer_radius, bolt_radius in ((32, 3.8, 0.105),):
        name = f"bolt_flange_{bolt_count:03d}_holes"
        specs.append(
            _spec(
                name,
                "complex",
                f"Large boolean-cut flange with center bore and {bolt_count} bolt holes",
                _bolt_flange(name, bolt_count, outer_radius, bolt_radius),
            ),
        )

    for rows, cols, radius, offset in (
        (10, 16, 0.11, 0.045),
        (12, 16, 0.10, 0.040),
        (14, 16, 0.09, 0.035),
        (16, 16, 0.08, 0.030),
        (18, 18, 0.070, 0.028),
        (20, 18, 0.064, 0.026),
        (22, 18, 0.058, 0.024),
    ):
        holes = rows * cols
        name = f"offset_plate_{rows:02d}x{cols:02d}_{holes:03d}_holes"
        specs.append(
            _spec(
                name,
                "complex",
                f"Offset solid plate followed by {holes} boolean-drilled holes",
                _offset_perforated_plate(name, rows, cols, radius, offset),
            ),
        )

    for rows, cols, radius, wall in (
        (8, 12, 0.12, 0.055),
        (10, 12, 0.105, 0.050),
        (12, 12, 0.095, 0.045),
        (12, 14, 0.085, 0.040),
        (14, 16, 0.074, 0.036),
        (16, 18, 0.066, 0.032),
    ):
        holes = rows * cols
        name = f"thick_tray_{rows:02d}x{cols:02d}_{holes:03d}_holes"
        specs.append(
            _spec(
                name,
                "complex",
                f"Thick-solid hollow tray with {holes} boolean-drilled holes",
                _thick_drilled_tray(name, rows, cols, radius, wall),
            ),
        )

    for index, (x_ports, y_ports, deck_holes) in enumerate(((7, 8, 12),)):
        name = f"cross_drilled_manifold_{index:02d}_{x_ports + y_ports + deck_holes:03d}_ports"
        specs.append(
            _spec(
                name,
                "complex",
                "Large chamfered manifold with intersecting side tunnels and deck holes",
                _cross_drilled_manifold(name, x_ports, y_ports, deck_holes),
            ),
        )

    return tuple(specs)


SAMPLE_SPECS: tuple[SampleSpec, ...] = (
    *(
        _spec(f"box_{i:02d}", "box", "Rectangular box primitive", _single("box", dx=1.0 + i * 0.15, dy=0.8 + i * 0.07, dz=0.5 + i * 0.05, r=0.45, g=0.55, b=0.68, label=f"box_{i:02d}"))
        for i in range(10)
    ),
    *(
        _spec(f"sphere_{i:02d}", "sphere", "Sphere and spherical segment primitive", _single("sphere", radius=0.6 + i * 0.08, angle=math.tau if i % 2 == 0 else math.pi * 1.5, r=0.70, g=0.45, b=0.50, label=f"sphere_{i:02d}"))
        for i in range(8)
    ),
    *(
        _spec(f"cylinder_{i:02d}", "cylinder", "Cylinder and partial-cylinder primitive", _single("cylinder", radius=0.35 + i * 0.06, height=0.9 + i * 0.18, angle=math.tau if i % 3 else math.pi * 1.5, r=0.35, g=0.62, b=0.58, label=f"cylinder_{i:02d}"))
        for i in range(8)
    ),
    *(
        _spec(f"cone_{i:02d}", "cone", "Cone and truncated-cone primitive", _single("cone", r1=0.65 + i * 0.05, r2=0.15 + (i % 4) * 0.08, height=1.0 + i * 0.12, r=0.74, g=0.55, b=0.34, label=f"cone_{i:02d}"))
        for i in range(8)
    ),
    *(
        _spec(f"torus_{i:02d}", "torus", "Torus primitive", _single("torus", r1=1.0 + i * 0.12, r2=0.16 + i * 0.02, angle=math.tau if i % 2 == 0 else math.pi * 1.6, r=0.52, g=0.42, b=0.68, label=f"torus_{i:02d}"))
        for i in range(6)
    ),
    *(
        _spec(f"wedge_{i:02d}", "wedge", "Wedge primitive", _single("wedge", dx=1.4 + i * 0.12, dy=0.9 + i * 0.05, dz=0.8 + i * 0.07, ltx=0.25 + i * 0.12, r=0.50, g=0.62, b=0.40, label=f"wedge_{i:02d}"))
        for i in range(6)
    ),
    _spec("bottle_00", "compound", "Fused bottle-like solid", _fused_bottle),
    _spec("pawn_00", "compound", "Fused pawn-like solid", _fused_pawn),
    _spec("box_cut_tunnel_00", "boolean", "Box with cylindrical cut tunnel", _box_cut_tunnel),
    _spec("stacked_tower_00", "compound", "Stacked fused tower", _stacked_tower),
    *_complex_specs(),
)


def by_name(name: str) -> SampleSpec:
    """Return a registered sample by name."""
    for spec in SAMPLE_SPECS:
        if spec.name == name:
            return spec
    raise KeyError(name)
