#!/usr/bin/env python3
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

"""Parity runner: execute a binding_parity scenario and print canonical JSON.

Usage::

    python tests/parity/runner.py tests/binding_parity/build_box.json

The script reads a scenario file from ``tests/binding_parity/``, executes the
equivalent operations through :mod:`occtl`, and writes one JSON object to
stdout with the same schema every binding's runner emits. Phase 2 compares
the four bindings' outputs byte-for-byte.

Scenario shape::

    {
      "scenario": "build_box",
      "description": "...",
      "expected": {
        "face_count": 6,
        "edge_count": 12,
        "vertex_count": 8,
        "solid_count": 1
      },
      // optional:
      "params": { "dx": 10.0, "dy": 10.0, "dz": 5.0 }
    }

Output schema::

    {
      "scenario": "build_box",
      "binding": "python",
      "actual": { "face_count": 6, "edge_count": 12,
                  "vertex_count": 8, "solid_count": 1 },
      "matches": true
    }
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def _run_build_box(scenario: dict) -> dict:
    import occtl

    params = scenario.get("params") or {}
    dx = float(params.get("dx", 10.0))
    dy = float(params.get("dy", 10.0))
    dz = float(params.get("dz", 5.0))

    with occtl.Graph() as g:
        occtl.prim.make_box(g, occtl.prim.BoxInfo(dx=dx, dy=dy, dz=dz))
        return {
            "face_count": g.face_count,
            "edge_count": g.edge_count,
            "vertex_count": g.vertex_count,
            "solid_count": g.solid_count,
        }


def _run_fuse_two_boxes(scenario: dict) -> dict:
    import occtl

    with occtl.Graph() as g:
        box_a = occtl.prim.make_box(g, occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0))
        shifted = occtl.Axis2Placement(
            location=occtl.Point3(5.0, 0.0, 0.0),
            x_dir=occtl.Direction3(1.0, 0.0, 0.0),
            x_dir_ref=occtl.Direction3(0.0, 1.0, 0.0),
        )
        box_b = occtl.prim.make_box(
            g,
            occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0, placement=shifted),
        )

        # Capture box A's face UIDs before fuse so they survive the result
        # being mixed in graph-wide iteration.
        face_uids_before = [g.uid_of(f) for f in g.face_iter()]
        a_face_uids = face_uids_before[0:6]

        result = occtl.bool_.fuse(g, [box_a], [box_b])

        history_modified_nonempty = False
        for uid in a_face_uids:
            if g.history_modified(uid) or g.history_generated(uid):
                history_modified_nonempty = True
                break

        return {
            "root_kind": _kind_str(g, result),
            "history_modified_nonempty_on_first_face_of_box_a": history_modified_nonempty,
        }


def _kind_str(graph, node_id) -> str:
    """Map an occtl_node_kind_t into the short string used in scenarios."""
    from occtl._generated._raw import lib, ffi
    out = ffi.new("occtl_node_kind_t*")
    raw = ffi.new("occtl_node_id_t*", [node_id.bits])[0]
    lib.occtl_graph_node_kind(graph._as_ptr(), raw, out)
    int_to_str = {
        int(lib.OCCTL_KIND_SOLID):     "solid",
        int(lib.OCCTL_KIND_SHELL):     "shell",
        int(lib.OCCTL_KIND_FACE):      "face",
        int(lib.OCCTL_KIND_WIRE):      "wire",
        int(lib.OCCTL_KIND_EDGE):      "edge",
        int(lib.OCCTL_KIND_VERTEX):    "vertex",
        int(lib.OCCTL_KIND_COMPOUND):  "compound",
        int(lib.OCCTL_KIND_COMPSOLID): "compsolid",
    }
    return int_to_str.get(int(out[0]), "unknown")


def _run_cut_box_corner(scenario: dict) -> dict:
    import occtl

    with occtl.Graph() as g:
        box = occtl.prim.make_box(g, occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0))
        corner = occtl.Axis2Placement(
            location=occtl.Point3(7.5, 7.5, 7.5),
            x_dir=occtl.Direction3(1.0, 0.0, 0.0),
            x_dir_ref=occtl.Direction3(0.0, 1.0, 0.0),
        )
        occtl.prim.make_box(
            g,
            occtl.prim.BoxInfo(dx=5.0, dy=5.0, dz=5.0, placement=corner),
        )
        result = occtl.bool_.cut(g, [box], list(g.solid_iter())[1:])  # tool is the second solid
        return {
            "root_kind": _kind_str(g, result),
            "solid_count": g.solid_count,
        }


def _run_common_two_overlapping_boxes(scenario: dict) -> dict:
    import occtl

    with occtl.Graph() as g:
        a = occtl.prim.make_box(g, occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0))
        shifted = occtl.Axis2Placement(
            location=occtl.Point3(5.0, 5.0, 5.0),
            x_dir=occtl.Direction3(1.0, 0.0, 0.0),
            x_dir_ref=occtl.Direction3(0.0, 1.0, 0.0),
        )
        b = occtl.prim.make_box(
            g,
            occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0, placement=shifted),
        )
        result = occtl.bool_.common(g, [a], [b])
        return {
            "root_kind": _kind_str(g, result),
            "solid_count": g.solid_count,
        }


def _run_section_two_overlapping_boxes(scenario: dict) -> dict:
    import occtl

    with occtl.Graph() as g:
        a = occtl.prim.make_box(g, occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0))
        shifted = occtl.Axis2Placement(
            location=occtl.Point3(5.0, 0.0, 0.0),
            x_dir=occtl.Direction3(1.0, 0.0, 0.0),
            x_dir_ref=occtl.Direction3(0.0, 1.0, 0.0),
        )
        b = occtl.prim.make_box(
            g,
            occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0, placement=shifted),
        )
        edge_count_before = g.edge_count
        result = occtl.bool_.section(g, [a], [b])
        return {
            "root_kind": _kind_str(g, result),
            "edge_count_increased": g.edge_count > edge_count_before,
        }


def _run_split_box_by_box(scenario: dict) -> dict:
    import occtl

    with occtl.Graph() as g:
        a = occtl.prim.make_box(g, occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0))
        shifted = occtl.Axis2Placement(
            location=occtl.Point3(0.0, 0.0, 5.0),
            x_dir=occtl.Direction3(1.0, 0.0, 0.0),
            x_dir_ref=occtl.Direction3(0.0, 1.0, 0.0),
        )
        b = occtl.prim.make_box(
            g,
            occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0, placement=shifted),
        )
        result = occtl.bool_.split(g, [a], [b])
        return {
            "root_kind": _kind_str(g, result),
            "compound_count": g.compound_count,
            "solid_count": g.solid_count,
        }


def _run_history_modified_after_fuse(scenario: dict) -> dict:
    import occtl

    with occtl.Graph() as g:
        a = occtl.prim.make_box(g, occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0))
        shifted = occtl.Axis2Placement(
            location=occtl.Point3(5.0, 0.0, 0.0),
            x_dir=occtl.Direction3(1.0, 0.0, 0.0),
            x_dir_ref=occtl.Direction3(0.0, 1.0, 0.0),
        )
        b = occtl.prim.make_box(
            g,
            occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0, placement=shifted),
        )
        # Capture the face UIDs of each input box BEFORE the fuse. The
        # fuse adds new faces (the result solid's faces) so iterating
        # face_iter() afterwards mixes inputs and results.
        faces_before = [g.uid_of(f) for f in g.face_iter()]
        # Tag-by-position: first 6 belong to a, next 6 to b (box-shape
        # primitives emit 6 faces in deterministic order).
        a_face_uids = faces_before[0:6]
        b_face_uids = faces_before[6:12]

        occtl.bool_.fuse(g, [a], [b])

        def any_modified(uids):
            for uid in uids:
                if g.history_modified(uid) or g.history_generated(uid):
                    return True
            return False

        return {
            "box_a_modified_nonempty": any_modified(a_face_uids),
            "box_b_modified_nonempty": any_modified(b_face_uids),
        }


_HANDLERS = {
    "build_box": _run_build_box,
    "fuse_two_boxes": _run_fuse_two_boxes,
    "cut_box_corner": _run_cut_box_corner,
    "common_two_overlapping_boxes": _run_common_two_overlapping_boxes,
    "section_two_overlapping_boxes": _run_section_two_overlapping_boxes,
    "split_box_by_box": _run_split_box_by_box,
    "history_modified_after_fuse": _run_history_modified_after_fuse,
}


def main(argv: "list[str] | None" = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenario", type=Path)
    args = parser.parse_args(argv)

    scenario = json.loads(args.scenario.read_text(encoding="utf-8"))
    handler = _HANDLERS.get(scenario["scenario"])
    if handler is None:
        sys.stderr.write(
            f"runner: unknown scenario '{scenario['scenario']}'. "
            f"Known: {sorted(_HANDLERS)}\n"
        )
        return 2

    actual = handler(scenario)
    expected = scenario.get("expected") or {}
    output = {
        "scenario": scenario["scenario"],
        "binding": "python",
        "actual": actual,
        "matches": _matches(actual, expected),
    }
    sys.stdout.write(json.dumps(output, sort_keys=True) + "\n")
    return 0 if output["matches"] else 1


def _matches(actual: dict, expected: dict) -> bool:
    """Match an actual result against an expected dict.

    Each expected key drives a comparator:
      * key ``X_in`` ⇒ actual[X] must equal one of expected[X_in].
      * key ``X_at_least`` ⇒ actual[X] >= expected[X_at_least].
      * key ``X_greater_than`` ⇒ actual[X] > expected[X_greater_than].
      * otherwise ⇒ actual[X] must equal expected[X] exactly.
    """
    for key, want in expected.items():
        if key.endswith("_in"):
            base = key[:-len("_in")]
            if actual.get(base) not in want:
                return False
        elif key.endswith("_at_least"):
            base = key[:-len("_at_least")]
            if not (actual.get(base, 0) >= want):
                return False
        elif key.endswith("_greater_than"):
            base = key[:-len("_greater_than")]
            if not (actual.get(base, 0) > want):
                return False
        else:
            if actual.get(key) != want:
                return False
    return True


if __name__ == "__main__":
    raise SystemExit(main())
