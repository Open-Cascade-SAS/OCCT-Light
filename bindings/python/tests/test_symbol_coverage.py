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

"""Tier-3 symbol coverage — every ``OCCTL_API`` has a Python wrapper.

Reads ``build/abi.json`` and confirms each ``OCCTL_API`` function name is
exposed by **some** Python module under :mod:`occtl` (either at module level
or as a class method). Fails the test if any C function is missing — this
prevents the hand-written idiomatic layer from drifting behind the C ABI.
"""

from __future__ import annotations

import importlib
import inspect
import json
import os
import pkgutil
from pathlib import Path
from typing import Iterable, Set

import pytest


_HERE = Path(__file__).resolve().parent
_REPO_ROOT = _HERE.parents[2]
_ABI_JSON = _REPO_ROOT / "build" / "abi.json"


def _collect_module_attrs(module) -> Set[str]:
    out: Set[str] = set()
    for name in dir(module):
        if name.startswith("_"):
            continue
        out.add(name)
    return out


def _collect_class_methods(module) -> Set[str]:
    out: Set[str] = set()
    for _, obj in inspect.getmembers(module, predicate=inspect.isclass):
        if obj.__module__ != module.__name__:
            continue
        for mname, _ in inspect.getmembers(obj):
            if mname.startswith("_"):
                continue
            out.add(mname)
    return out


def _all_binding_symbols() -> Set[str]:
    """Walk every submodule of ``occtl`` and collect all exposed names."""
    try:
        occtl = importlib.import_module("occtl")
    except Exception as exc:  # pragma: no cover
        pytest.fail(f"occtl import failed: {exc}")

    names: Set[str] = set()
    pkg_path = Path(occtl.__file__).parent
    for module_info in pkgutil.walk_packages([str(pkg_path)], prefix="occtl."):
        # Private subpackages still count — the generated `_generated.*`
        # modules are the canonical home of the auto-wrappers.
        try:
            m = importlib.import_module(module_info.name)
        except Exception:
            continue
        names |= _collect_module_attrs(m)
        names |= _collect_class_methods(m)

    # Also include the top-level ``occtl`` namespace.
    names |= _collect_module_attrs(occtl)
    names |= _collect_class_methods(occtl)
    return names


def _candidate_features_paths() -> list[Path]:
    explicit = os.environ.get("OCCTL_FEATURES_PATH")
    if explicit:
        return [Path(explicit)]
    library_path = os.environ.get("OCCTL_LIBRARY_PATH")
    if not library_path:
        return []
    lib_path = Path(library_path)
    out = [lib_path / "OCCTLFeatures.json"]
    if lib_path.parent != lib_path:
        out.append(lib_path.parent / "OCCTLFeatures.json")
    return out


def _enabled_headers_from_manifest() -> set[str] | None:
    feature_to_headers: dict[str, tuple[str, ...]] = {
        "core": ("occtl.h", "occtl_core.h"),
        "geom": ("occtl_geom.h", "occtl_curves.h", "occtl_curves2d.h", "occtl_curves_common.h", "occtl_surfaces.h"),
        "topo": ("occtl_topo.h", "occtl_topo_types.h", "occtl_topo_algo.h", "occtl_topo_build.h", "occtl_topo_relation.h"),
        "prim": ("occtl_prim.h", "occtl_prim_solid.h", "occtl_prim_sketch.h", "occtl_prim_sweep.h", "occtl_prim_feature.h"),
        "text": ("occtl_text.h",),
        "bool": ("occtl_bool.h",),
        "mesh": ("occtl_mesh.h",),
        "heal": ("occtl_heal.h",),
        "io_brep": ("occtl_io_brep.h",),
        "io_step": ("occtl_io_step.h",),
        "io_iges": ("occtl_io_iges.h",),
        "io_stl": ("occtl_io_stl.h",),
        "io_obj": ("occtl_io_obj.h",),
        "io_gltf": ("occtl_io_gltf.h",),
        "io_vrml": ("occtl_io_vrml.h",),
        "io_ply": ("occtl_io_ply.h",),
        "de": ("occtl_de.h",),
        "viz": ("occtl_viz.h",),
    }
    for path in _candidate_features_paths():
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
            features = payload.get("binding_features")
            if not isinstance(features, list):
                continue
            enabled_headers: set[str] = {"occtl.h", "occtl_core.h"}
            for feature in features:
                key = str(feature).strip()
                for header in feature_to_headers.get(key, ()):
                    enabled_headers.add(header)
            if enabled_headers:
                return enabled_headers
        except Exception:
            continue
    return None


def test_every_occtl_api_function_has_wrapper():
    if not _ABI_JSON.is_file():
        pytest.fail(
            f"{_ABI_JSON} missing; run `python3 tools/abi_dump.py "
            "--output build/abi.json` first."
        )
    abi = json.loads(_ABI_JSON.read_text(encoding="utf-8"))
    enabled_headers = _enabled_headers_from_manifest()
    c_functions: Set[str] = {
        f["name"]
        for f in abi["functions"]
        if enabled_headers is None or f.get("header", "") in enabled_headers
    }

    bound = _all_binding_symbols()

    missing = sorted(c_functions - bound)
    assert not missing, (
        f"{len(missing)} C ABI functions have no Python wrapper.\n"
        f"First 20 missing: {missing[:20]}"
    )


def test_abi_version_constant_matches_abi_json():
    """The generated ABI_VERSION must match what abi_dump.py reported."""
    if not _ABI_JSON.is_file():
        pytest.fail(f"{_ABI_JSON} missing.")
    abi = json.loads(_ABI_JSON.read_text(encoding="utf-8"))
    from occtl._generated._abi import ABI_VERSION

    assert ABI_VERSION == int(abi["abi_version"] or 0)
