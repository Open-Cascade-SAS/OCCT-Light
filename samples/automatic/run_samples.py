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

"""Generate registered OCCT-Light Python samples."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys
from typing import Iterable
import zlib

_HERE = Path(__file__).resolve().parent
_REPO_ROOT = _HERE.parents[1]
if str(_REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(_REPO_ROOT))

from samples.python.occtl_samples.registry import SAMPLE_SPECS, SampleSpec


def _selected(names: list[str], limit: int | None) -> Iterable[SampleSpec]:
    specs = SAMPLE_SPECS
    if names:
        wanted = set(names)
        specs = tuple(spec for spec in specs if spec.name in wanted)
        missing = sorted(wanted - {spec.name for spec in specs})
        if missing:
            raise SystemExit(f"unknown sample name(s): {', '.join(missing)}")
    if limit is not None:
        specs = specs[:limit]
    return specs


def _put_pixel(pixels: bytearray, width: int, height: int, x: int, y: int, color: tuple[int, int, int, int]) -> None:
    if x < 0 or y < 0 or x >= width or y >= height:
        return
    index = (y * width + x) * 4
    pixels[index:index + 4] = bytes(color)


def _draw_line(
    pixels: bytearray,
    width: int,
    height: int,
    start: tuple[int, int],
    end: tuple[int, int],
    color: tuple[int, int, int, int],
) -> None:
    x0, y0 = start
    x1, y1 = end
    dx = abs(x1 - x0)
    sx = 1 if x0 < x1 else -1
    dy = -abs(y1 - y0)
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    while True:
        _put_pixel(pixels, width, height, x0, y0, color)
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy


def _draw_line_width(
    pixels: bytearray,
    width: int,
    height: int,
    start: tuple[int, int],
    end: tuple[int, int],
    color: tuple[int, int, int, int],
    line_width: int = 2,
) -> None:
    radius = max(0, line_width // 2)
    for oy in range(-radius, radius + 1):
        for ox in range(-radius, radius + 1):
            _draw_line(pixels, width, height, (start[0] + ox, start[1] + oy), (end[0] + ox, end[1] + oy), color)


def _write_png(path: Path, width: int, height: int, pixels: bytes) -> None:
    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    raw = b"".join(b"\x00" + pixels[y * width * 4:(y + 1) * width * 4] for y in range(height))
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw, 9))
        + chunk(b"IEND", b""),
    )


def _write_fallback_preview(graph, root, path: Path) -> None:
    width = 960
    height = 720
    pixels = bytearray([245, 247, 250, 255] * width * height)
    bbox = graph.bounding_box(root)
    dx = max(bbox.max.x - bbox.min.x, 1.0e-9)
    dy = max(bbox.max.y - bbox.min.y, 1.0e-9)
    dz = max(bbox.max.z - bbox.min.z, 1.0e-9)
    scale = 420.0 / max(dx + dy, dz * 1.6)
    cx = width // 2
    cy = height // 2 + 90

    def project(x: float, y: float, z: float) -> tuple[int, int]:
        px = (x - bbox.min.x - dx / 2.0) - (y - bbox.min.y - dy / 2.0)
        py = ((x - bbox.min.x - dx / 2.0) + (y - bbox.min.y - dy / 2.0)) * 0.45 - (z - bbox.min.z - dz / 2.0)
        return int(cx + px * scale), int(cy + py * scale)

    corners = [
        project(x, y, z)
        for x in (bbox.min.x, bbox.max.x)
        for y in (bbox.min.y, bbox.max.y)
        for z in (bbox.min.z, bbox.max.z)
    ]
    edges = ((0, 1), (0, 2), (0, 4), (3, 1), (3, 2), (3, 7), (5, 1), (5, 4), (5, 7), (6, 2), (6, 4), (6, 7))
    for edge in edges:
        _draw_line(pixels, width, height, corners[edge[0]], corners[edge[1]], (45, 65, 85, 255))
    _write_png(path, width, height, bytes(pixels))


def _node_id_value(node):
    from occtl._raw import ffi

    slot = ffi.new("occtl_node_id_t*")
    slot.bits = int(node.bits if hasattr(node, "bits") else node)
    return slot[0]


def _root_node_ids(graph, root, target_kind: int) -> list[object]:
    from occtl._errors import _check
    from occtl._raw import ffi, lib

    config = ffi.new("occtl_child_explorer_config_t*")
    lib.occtl_child_explorer_config_init(config)
    config.target_kind = target_kind
    out_iter = ffi.new("occtl_explorer_iter_t**")
    _check(lib.occtl_topo_child_explorer_create(graph._as_ptr(), _node_id_value(root), config, out_iter))
    iterator = out_iter[0]
    nodes = []
    try:
        while True:
            out_node = ffi.new("occtl_node_id_t*")
            out_transform = ffi.new("occtl_transform_t*")
            out_orientation = ffi.new("occtl_orientation_t*")
            status = lib.occtl_explorer_iter_next(iterator, out_node, out_transform, out_orientation)
            if status == lib.OCCTL_NOT_FOUND:
                break
            _check(status)
            nodes.append(out_node[0])
    finally:
        lib.occtl_explorer_iter_free(iterator)
    return nodes


def _write_mesh_preview(graph, root, path: Path) -> None:
    from occtl._errors import _check
    from occtl._raw import ffi, lib

    width = 960
    height = 720
    pixels = bytearray([42, 48, 54, 255] * width * height)
    bbox = graph.bounding_box(root)
    dx = max(bbox.max.x - bbox.min.x, 1.0e-9)
    dy = max(bbox.max.y - bbox.min.y, 1.0e-9)
    dz = max(bbox.max.z - bbox.min.z, 1.0e-9)
    scale = 500.0 / max(dx + dy, dz * 1.8)
    cx = width // 2
    cy = height // 2 + 80

    def project(point: tuple[float, float, float]) -> tuple[int, int]:
        x, y, z = point
        px = (x - bbox.min.x - dx / 2.0) - (y - bbox.min.y - dy / 2.0)
        py = ((x - bbox.min.x - dx / 2.0) + (y - bbox.min.y - dy / 2.0)) * 0.45 - (z - bbox.min.z - dz / 2.0)
        return int(cx + px * scale), int(cy + py * scale)

    options = ffi.new("occtl_mesh_options_t*")
    lib.occtl_mesh_options_init(options)
    options.deflection = max(max(dx, dy, dz) * 0.0015, 0.003)
    options.angle = 0.20
    _check(lib.occtl_mesh_generate(graph._as_ptr(), ffi.NULL, 0, options))

    drawn = 0
    for face in _root_node_ids(graph, root, lib.OCCTL_KIND_FACE):
        view = ffi.new("occtl_triangulation_view_t*")
        status = lib.occtl_mesh_face_triangulation(graph._as_ptr(), face, view)
        if status != lib.OCCTL_OK or int(view.node_count) < 3 or int(view.triangle_count) < 1:
            continue
        points = [
            (float(view.nodes[i * 3]), float(view.nodes[i * 3 + 1]), float(view.nodes[i * 3 + 2]))
            for i in range(int(view.node_count))
        ]
        for index in range(int(view.triangle_count)):
            a = int(view.triangles[index * 3])
            b = int(view.triangles[index * 3 + 1])
            c = int(view.triangles[index * 3 + 2])
            for start, end in ((a, b), (b, c), (c, a)):
                _draw_line_width(pixels, width, height, project(points[start]), project(points[end]), (180, 198, 212, 255), 1)
                drawn += 1

    for edge in _root_node_ids(graph, root, lib.OCCTL_KIND_EDGE):
        view = ffi.new("occtl_polygon3d_view_t*")
        status = lib.occtl_mesh_edge_polygon3d(graph._as_ptr(), edge, view)
        if status != lib.OCCTL_OK or int(view.node_count) < 2 or view.nodes == ffi.NULL:
            continue
        points = [
            (float(view.nodes[i * 3]), float(view.nodes[i * 3 + 1]), float(view.nodes[i * 3 + 2]))
            for i in range(int(view.node_count))
        ]
        for start, end in zip(points, points[1:]):
            _draw_line_width(pixels, width, height, project(start), project(end), (234, 220, 70, 255), 2)
            drawn += 1

    if drawn == 0:
        raise RuntimeError("mesh preview found no drawable root topology")
    _write_png(path, width, height, bytes(pixels))


def _write_screenshot(graph, root, path: Path) -> tuple[str | None, str | None]:
    try:
        _write_mesh_preview(graph, root, path)
        return None, "mesh"
    except Exception as exc:  # noqa: BLE001
        error = str(exc)
        try:
            _write_fallback_preview(graph, root, path)
            return error, "fallback"
        except Exception as fallback_exc:  # noqa: BLE001
            return f"{error}; fallback preview failed: {fallback_exc}", None


def run(output: Path, names: list[str], limit: int | None, screenshots: bool) -> list[dict[str, object]]:
    from occtl import io_brep

    models_dir = output / "models"
    screenshots_dir = output / "screenshots"
    reports_dir = output / "reports"
    models_dir.mkdir(parents=True, exist_ok=True)
    screenshots_dir.mkdir(parents=True, exist_ok=True)
    reports_dir.mkdir(parents=True, exist_ok=True)

    rows: list[dict[str, object]] = []
    for spec in _selected(names, limit):
        build = spec.build()
        graph = build.graph
        root = build.root
        try:
            issues = graph.check_issues()
            model_path = models_dir / f"{spec.name}.brep"
            io_brep.write(graph, root, model_path)
            screenshot_error = None
            screenshot_mode = None
            screenshot_path = screenshots_dir / f"{spec.name}.png"
            if screenshots:
                screenshot_error, screenshot_mode = _write_screenshot(graph, root, screenshot_path)
            row = {
                "name": spec.name,
                "category": spec.category,
                "description": spec.description,
                "valid": len(issues) == 0,
                "issue_count": len(issues),
                "model": str(model_path),
                "screenshot": str(screenshot_path) if screenshots and screenshot_path.exists() else None,
                "screenshot_mode": screenshot_mode,
                "screenshot_error": screenshot_error,
                "nodes": graph.nb_nodes,
                "solids": graph.nb_solids,
                "faces": graph.nb_faces,
                "edges": graph.nb_edges,
                "vertices": graph.nb_vertices,
            }
            rows.append(row)
            print(f"{spec.name}: {'valid' if row['valid'] else 'invalid'}")
        finally:
            graph.close()

    manifest = {"sample_count": len(rows), "samples": rows}
    (reports_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("build/samples/python"))
    parser.add_argument("--name", action="append", default=[], help="Run one registered sample by name; repeatable.")
    parser.add_argument("--limit", type=int, default=None, help="Run only the first N selected samples.")
    parser.add_argument("--reports-only", action="store_true", help="Generate CAD and validation reports only.")
    args = parser.parse_args()
    rows = run(args.output, args.name, args.limit, screenshots=not args.reports_only)
    invalid = [row["name"] for row in rows if not row["valid"]]
    if invalid:
        print("invalid samples: " + ", ".join(str(name) for name in invalid), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
