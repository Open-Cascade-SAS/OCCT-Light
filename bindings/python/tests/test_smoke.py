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

"""Tier-1 smoke test for the occtl Python binding.

The contract for this test mirrors the §7 plan: ABI handshake, build a
vertex, iterate the graph, exercise the error path.
"""

from __future__ import annotations

import pytest


def test_abi_handshake(occtl_module):
    """Runtime ABI must match the value baked into the generated _abi.py."""
    assert occtl_module.runtime_abi_version() == occtl_module.ABI_VERSION


def test_occt_version_non_empty(occtl_module):
    s = occtl_module.runtime_occt_version()
    assert isinstance(s, str)
    assert s != ""


def test_build_vertex_and_iterate(graph, occtl_module):
    """Build a vertex, iterate, confirm round-trip identity + vertex_count."""
    v = graph.make_vertex(x=1.0, y=2.0, z=3.0)
    assert isinstance(v, occtl_module.NodeId)
    assert v.is_valid

    ids = list(graph.vertex_iter())
    assert v in ids
    assert len(ids) == 1
    assert graph.vertex_count == 1

    p = graph.vertex_point(v)
    assert p.x == pytest.approx(1.0)
    assert p.y == pytest.approx(2.0)
    assert p.z == pytest.approx(3.0)


def test_error_path_carries_message(occtl_module):
    """A NULL-pointer adopt should raise InvalidHandleError with a message."""
    with pytest.raises(occtl_module.InvalidHandleError) as exc_info:
        occtl_module.Graph.from_pointer_unsafe(None)
    err = exc_info.value
    assert err.status == occtl_module.Status.INVALID_HANDLE
    # The handle layer composes its own diagnostic; either path is acceptable
    # so long as the message is non-empty.
    assert err.message != ""


def test_graph_create_alias(occtl_module):
    g = occtl_module.Graph.create()
    try:
        assert not g.closed
    finally:
        g.close()


def test_invalid_argument_path(graph, occtl_module):
    """occtl_topo_vertex_point with an invalid NodeId should fail cleanly."""
    bogus = occtl_module.NodeId(0xDEADBEEFDEADBEEF)
    with pytest.raises(occtl_module.Error) as exc_info:
        graph.vertex_point(bogus)
    err = exc_info.value
    # The error category is one of NOT_FOUND / INVALID_HANDLE / WRONG_KIND /
    # OUT_OF_RANGE depending on the C implementation's argument check order.
    assert err.status in (
        occtl_module.Status.NOT_FOUND,
        occtl_module.Status.INVALID_HANDLE,
        occtl_module.Status.WRONG_KIND,
        occtl_module.Status.OUT_OF_RANGE,
        occtl_module.Status.INVALID_ARGUMENT,
    )


def test_graph_context_manager(occtl_module):
    """Context manager closes the handle."""
    with occtl_module.Graph() as g:
        assert not g.closed
        g.make_vertex(x=0.0, y=0.0, z=0.0)
    assert g.closed


def test_modeling_measure_and_de_helpers(occtl_module):
    with occtl_module.Graph() as g:
        box = g.make_box(2.0, 3.0, 4.0)
        assert box.is_valid
        assert g.is_valid()
        assert g.check_issues() == ()
        sphere = g.make_sphere(1.0)
        cylinder = g.make_cylinder(0.5, 2.0)
        cone = g.make_cone(1.0, 0.25, 2.0)
        torus = g.make_torus(2.0, 0.25)
        wedge = g.make_wedge(2.0, 3.0, 4.0, 1.0)
        assert all(node.is_valid for node in (sphere, cylinder, cone, torus, wedge))
        assert len(g.solids()) >= 6

        g.set_name(box, "box")
        assert g.name(box) == "box"
        g.set_color(box, (0.25, 0.5, 0.75, 1.0))
        color = g.color(box)
        assert color.r == pytest.approx(0.25)
        assert color.g == pytest.approx(0.5)
        assert color.b == pytest.approx(0.75)

        props = g.mass_properties(box)
        assert props.volume == pytest.approx(24.0)
        assert props.surface_area == pytest.approx(52.0)
        assert g.volume(box) == pytest.approx(24.0)
        bbox = g.bounding_box(box)
        spans = sorted((
            bbox.max.x - bbox.min.x,
            bbox.max.y - bbox.min.y,
            bbox.max.z - bbox.min.z,
        ))
        assert spans == pytest.approx([2.0, 3.0, 4.0], abs=1.0e-6)

        moved = g.translated(box, (1.0, 2.0, 3.0))
        try:
            assert moved.root.is_valid
            assert moved.graph.solid_count >= 1
        finally:
            moved.graph.close()

    formats = occtl_module.de.supported_formats()
    assert "brep" in formats
    assert ".brep" in occtl_module.de.format_extensions("brep")
    assert occtl_module.de.format_id_for_path("part.BREP") == "brep"
    brep_info = occtl_module.de.format_info("brep")
    assert brep_info.can_read_file
    assert brep_info.can_write_file
    assert not brep_info.can_read_memory
    assert not brep_info.can_write_memory


def test_brep_helpers_roundtrip(tmp_path, occtl_module):
    path = tmp_path / "box.brep"
    with occtl_module.Graph() as g:
        box = g.make_box(1.0, 2.0, 3.0)
        occtl_module.io_brep.write(g, box, path)
        assert path.exists()

    result = occtl_module.io_brep.read(path)
    try:
        assert result.root.is_valid
        assert result.graph.is_valid()
        assert result.graph.solid_count == 1
    finally:
        result.graph.close()


def test_offset_and_thick_solid_helpers(occtl_module):
    with occtl_module.Graph() as g:
        box = g.make_box(2.0, 2.0, 1.0)
        offset = occtl_module.prim.make_offset_shape(
            g,
            box,
            0.05,
            occtl_module.prim.OffsetOptions(intersection=True, remove_internal_edges=True),
        )
        tray = occtl_module.prim.make_thick_solid(
            g,
            box,
            g.faces()[:1],
            -0.05,
            occtl_module.prim.OffsetOptions(intersection=True),
        )

        assert offset.is_valid
        assert tray.is_valid
        assert g.is_valid()


def test_node_id_distinct_types(occtl_module):
    """NodeId, Uid, RefId, RepId are distinct *types* (mypy / pyright catch
    mixing them); at runtime they're NamedTuples so tuple-equality applies.
    """
    n = occtl_module.NodeId(1)
    u = occtl_module.Uid(1)
    assert type(n) is not type(u)
    assert isinstance(n, occtl_module.NodeId)
    assert not isinstance(n, occtl_module.Uid)
