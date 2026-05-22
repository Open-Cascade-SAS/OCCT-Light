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

"""
Print shell export lines for Go cgo builds from OCCTLFeatures.json.

Usage:
  python3 tools/occtl_features_env.py /path/to/OCCTLFeatures.json
"""

from __future__ import annotations

import json
import pathlib
import shlex
import sys

FEATURE_TO_MACRO = {
    "geom": "OCCTL_HAS_GEOM",
    "topo": "OCCTL_HAS_TOPO",
    "prim": "OCCTL_HAS_PRIM",
    "text": "OCCTL_HAS_TEXT",
    "bool": "OCCTL_HAS_BOOL",
    "mesh": "OCCTL_HAS_MESH",
    "heal": "OCCTL_HAS_HEAL",
    "io_brep": "OCCTL_HAS_IO_BREP",
    "io_step": "OCCTL_HAS_IO_STEP",
    "io_iges": "OCCTL_HAS_IO_IGES",
    "io_stl": "OCCTL_HAS_IO_STL",
    "io_obj": "OCCTL_HAS_IO_OBJ",
    "io_gltf": "OCCTL_HAS_IO_GLTF",
    "io_vrml": "OCCTL_HAS_IO_VRML",
    "io_ply": "OCCTL_HAS_IO_PLY",
    "de": "OCCTL_HAS_DE",
    "viz": "OCCTL_HAS_VIZ",
}

FEATURE_TO_GO_TAG = {
    "core": "occtl_core",
    "geom": "occtl_geom",
    "topo": "occtl_topo",
    "prim": "occtl_prim",
    "text": "occtl_text",
    "bool": "occtl_bool",
    "mesh": "occtl_mesh",
    "heal": "occtl_heal",
    "io_brep": "occtl_io_brep",
    "io_step": "occtl_io_step",
    "io_iges": "occtl_io_iges",
    "io_stl": "occtl_io_stl",
    "io_obj": "occtl_io_obj",
    "io_gltf": "occtl_io_gltf",
    "io_vrml": "occtl_io_vrml",
    "io_ply": "occtl_io_ply",
    "de": "occtl_de",
    "viz": "occtl_viz",
}


def die(msg: str) -> None:
    print(f"error: {msg}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    if len(sys.argv) != 2:
        die("expected one argument: /path/to/OCCTLFeatures.json")

    manifest_path = pathlib.Path(sys.argv[1]).resolve()
    if not manifest_path.is_file():
        die(f"manifest not found: {manifest_path}")

    data = json.loads(manifest_path.read_text(encoding="utf-8"))
    features = list(data.get("binding_features") or [])
    library_name = data.get("library_name")
    if not isinstance(library_name, str) or not library_name:
        die("library_name missing in manifest")

    # Manifest is typically build/<preset>/OCCTLFeatures.json; lib dir is sibling.
    lib_dir = manifest_path.parent / "lib"
    if not lib_dir.is_dir():
        die(f"lib directory not found beside manifest: {lib_dir}")

    repo_root = manifest_path.parents[2] if len(manifest_path.parents) >= 3 and manifest_path.parents[1].name == "build" else None
    include_dir = (repo_root / "include") if repo_root else None
    if include_dir is None or not include_dir.is_dir():
        die("could not infer include directory from manifest path")

    lib_dir = choose_library_dir(library_name, manifest_path, lib_dir, repo_root)

    macros = [f"-D{FEATURE_TO_MACRO[f]}" for f in features if f in FEATURE_TO_MACRO]
    go_tags = [FEATURE_TO_GO_TAG[f] for f in features if f in FEATURE_TO_GO_TAG]
    cflags = [f"-I{include_dir}", *macros]
    ldflags = [f"-L{lib_dir}", f"-Wl,-rpath,{lib_dir}", f"-l{library_name}"]

    cflags_s = " ".join(shlex.quote(x) for x in cflags)
    ldflags_s = " ".join(shlex.quote(x) for x in ldflags)

    print("# Evaluate these in your shell before running `go test` / `go build`.")
    print("# Or run directly: python3 tools/go_with_features.py /path/to/OCCTLFeatures.json -- go test ./...")
    print(f"export CGO_CFLAGS={shlex.quote(cflags_s)}")
    print(f"export CGO_LDFLAGS={shlex.quote(ldflags_s)}")
    if go_tags:
        print(f"export GOFLAGS={shlex.quote('-tags=' + ','.join(go_tags))}")


def choose_library_dir(
    library_name: str,
    manifest_path: pathlib.Path,
    default_lib_dir: pathlib.Path,
    repo_root: pathlib.Path,
) -> pathlib.Path:
    if has_shared_library(default_lib_dir, library_name):
        return default_lib_dir

    build_dir = repo_root / "build"
    if build_dir.is_dir():
        preset = manifest_path.parent.name
        preferred = [
            build_dir / f"{preset}-shared" / "lib",
            build_dir / "full-with-viz-shared" / "lib",
            build_dir / "full-shared" / "lib",
        ]
        for candidate in preferred:
            if has_shared_library(candidate, library_name):
                return candidate

        for candidate in sorted(build_dir.glob("*/lib")):
            if has_shared_library(candidate, library_name):
                return candidate

    return default_lib_dir


def has_shared_library(lib_dir: pathlib.Path, library_name: str) -> bool:
    names = [
        f"lib{library_name}.dylib",
        f"lib{library_name}.so",
        f"{library_name}.dll",
    ]
    return any((lib_dir / name).exists() for name in names)


if __name__ == "__main__":
    main()
