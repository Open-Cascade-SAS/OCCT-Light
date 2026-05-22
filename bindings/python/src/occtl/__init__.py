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

"""Public surface of the OCCT-Light Python binding.

Importing this package:

1. Loads the cffi-bound ``libocctl-<feature-set>`` shared library via :mod:`occtl._generated._raw`.
2. Performs the §4.1 ABI handshake against :data:`ABI_VERSION` and raises
   :class:`AbiMismatchError` on a mismatch.
3. Installs the thread-local error-snapshot callback so
   :func:`_errors._check` can translate every non-OK status into a typed
   exception.
4. Calls ``occtl_runtime_init(NULL)`` once, idempotently.

Module layout depends on the selected feature set:

    occtl.core       — runtime lifecycle helpers
    occtl.geom       — POD geometry values and helpers
    occtl.curves     — 3D parametric curves
    occtl.curves2d   — 2D parametric curves (PCurves)
    occtl.surfaces   — parametric surfaces
    occtl.topo       — Graph, builders, iterators
    occtl.prim       — primitive solids (box, sphere, cone, ...)
    occtl.text       — text-to-faces

Convenience re-exports at the package root cover the most-used names exposed
by the current feature set.
"""

from __future__ import annotations

import json
import os

from ._generated._abi import ABI_VERSION, AVAILABLE_MODULES as BUILD_AVAILABLE_MODULES, LIBRARY_VERSION
from ._errors import (
    AbiMismatchError,
    BufferTooSmallError,
    CancelledError,
    Error,
    FormatError,
    GenericError,
    GeometryInvalidError,
    InternalError,
    InvalidArgumentError,
    InvalidHandleError,
    IoError,
    NotDoneError,
    NotFoundError,
    OutOfMemoryError,
    OutOfRangeError,
    Status,
    TopologyInvalidError,
    UnsupportedError,
    VersionMismatchError,
    WrongKindError,
    install_snapshot_callback,
)
from ._ids import (
    NODE_ID_INVALID,
    REF_ID_INVALID,
    REF_UID_INVALID,
    REP_ID_INVALID,
    UID_INVALID,
    NodeId,
    RefId,
    RefUid,
    RepId,
    Uid,
)


# ---------------------------------------------------------------------------
# Load the native library and run the ABI handshake.
# ---------------------------------------------------------------------------

def _bootstrap() -> None:
    """Load the native library, verify ABI, init runtime, install hooks."""
    from ._generated import _raw  # noqa: F401  — side effect: dlopen

    ffi = _raw.ffi
    lib = _raw.lib

    actual_abi = int(lib.occtl_runtime_abi_version())
    if actual_abi != ABI_VERSION:
        raise AbiMismatchError(expected=ABI_VERSION, actual=actual_abi)

    # Install the error-snapshot callback.
    def _snapshot() -> tuple[str, int, int]:
        err_ptr = lib.occtl_error_last()
        if not err_ptr:
            return ("", 0, 0)
        msg_ptr = err_ptr.message
        if msg_ptr:
            message = ffi.string(msg_ptr).decode("utf-8", errors="replace")
        else:
            message = ""
        return (message, int(err_ptr.source.bits), int(err_ptr.extended))

    install_snapshot_callback(_snapshot)

    # Idempotent runtime init. occtl_runtime_init(NULL) returns OCCTL_OK on
    # first call and OCCTL_INVALID_ARGUMENT on subsequent calls; we ignore
    # the second-call error explicitly to keep import re-entrant.
    status = int(lib.occtl_runtime_init(ffi.NULL))
    if status not in (Status.OK, Status.INVALID_ARGUMENT):
        # A genuine failure — let the _check machinery raise.
        from ._errors import _check
        _check(status)


_bootstrap()


def _candidate_features_paths() -> tuple[str, ...]:
    explicit = os.environ.get("OCCTL_FEATURES_PATH")
    if explicit:
        return (explicit,)
    library_path = os.environ.get("OCCTL_LIBRARY_PATH")
    if not library_path:
        return ()
    parent = os.path.dirname(library_path)
    return (
        os.path.join(library_path, "OCCTLFeatures.json"),
        os.path.join(parent if parent else library_path, "OCCTLFeatures.json"),
    )


def _runtime_available_modules() -> frozenset[str]:
    for manifest in _candidate_features_paths():
        try:
            with open(manifest, "r", encoding="utf-8") as fp:
                payload = json.load(fp)
            features = payload.get("binding_features")
            if isinstance(features, list):
                enabled = {str(x).strip() for x in features if str(x).strip()}
                if enabled:
                    feature_to_modules: dict[str, tuple[str, ...]] = {
                        "core": ("core",),
                        "geom": ("geom", "curves", "curves2d", "surfaces"),
                        "topo": ("topo", "topo_algo"),
                        "prim": ("prim",),
                        "text": ("text",),
                        "bool": ("bool_",),
                        "mesh": ("mesh",),
                        "heal": ("heal",),
                        "de": ("de",),
                        "io_brep": ("io_brep",),
                        "io_step": ("io_step",),
                        "io_iges": ("io_iges",),
                        "io_stl": ("io_stl",),
                        "io_obj": ("io_obj",),
                        "io_gltf": ("io_gltf",),
                        "io_vrml": ("io_vrml",),
                        "io_ply": ("io_ply",),
                        "viz": ("viz",),
                    }
                    modules: set[str] = set()
                    for feature in enabled:
                        modules.update(feature_to_modules.get(feature, (feature,)))
                    return frozenset(modules) & BUILD_AVAILABLE_MODULES
        except Exception:
            continue
    return BUILD_AVAILABLE_MODULES


AVAILABLE_MODULES: frozenset[str] = _runtime_available_modules()


# ---------------------------------------------------------------------------
# Convenience re-exports from the public modules.
# ---------------------------------------------------------------------------

from . import core  # noqa: E402

if "geom" in AVAILABLE_MODULES:
    from . import curves, curves2d, geom, surfaces  # noqa: E402
if "topo" in AVAILABLE_MODULES:
    from . import topo, topo_algo  # noqa: E402
if "prim" in AVAILABLE_MODULES:
    from . import prim  # noqa: E402
if "text" in AVAILABLE_MODULES:
    from . import text  # noqa: E402
if "bool_" in AVAILABLE_MODULES:
    from . import bool_  # noqa: E402
if "mesh" in AVAILABLE_MODULES:
    from . import mesh  # noqa: E402
if "heal" in AVAILABLE_MODULES:
    from . import heal  # noqa: E402
if "de" in AVAILABLE_MODULES:
    from . import de  # noqa: E402
if "io_brep" in AVAILABLE_MODULES:
    from . import io_brep  # noqa: E402
if "io_step" in AVAILABLE_MODULES:
    from . import io_step  # noqa: E402
if "io_iges" in AVAILABLE_MODULES:
    from . import io_iges  # noqa: E402
if "io_stl" in AVAILABLE_MODULES:
    from . import io_stl  # noqa: E402
if "io_obj" in AVAILABLE_MODULES:
    from . import io_obj  # noqa: E402
if "io_gltf" in AVAILABLE_MODULES:
    from . import io_gltf  # noqa: E402
if "io_vrml" in AVAILABLE_MODULES:
    from . import io_vrml  # noqa: E402
if "io_ply" in AVAILABLE_MODULES:
    from . import io_ply  # noqa: E402
if "viz" in AVAILABLE_MODULES:
    from . import viz  # noqa: E402

# Common entry points at the package root.
from .core import (  # noqa: E402
    runtime_abi_version,
    runtime_occt_version,
    runtime_version,
    status_to_string,
)
if "topo" in AVAILABLE_MODULES:
    from .topo import Color  # noqa: E402
    from .topo import Graph  # noqa: E402
if "geom" in AVAILABLE_MODULES:
    from .geom import (  # noqa: E402
        Point2,
        Point3,
        Vector3,
        Direction3,
        Axis1Placement,
        Axis2Placement,
        Axis3Placement,
        Transform,
    )

__all__ = [
    # Version / status
    "ABI_VERSION",
    "LIBRARY_VERSION",
    "AVAILABLE_MODULES",
    "Status",
    "runtime_abi_version",
    "runtime_occt_version",
    "runtime_version",
    "status_to_string",
    # Errors
    "Error",
    "AbiMismatchError",
    "GenericError",
    "InvalidArgumentError",
    "InvalidHandleError",
    "NotFoundError",
    "OutOfMemoryError",
    "OutOfRangeError",
    "NotDoneError",
    "GeometryInvalidError",
    "TopologyInvalidError",
    "IoError",
    "FormatError",
    "UnsupportedError",
    "CancelledError",
    "BufferTooSmallError",
    "VersionMismatchError",
    "InternalError",
    "WrongKindError",
    # IDs
    "NodeId",
    "Uid",
    "RefId",
    "RefUid",
    "RepId",
    "NODE_ID_INVALID",
    "UID_INVALID",
    "REF_ID_INVALID",
    "REF_UID_INVALID",
    "REP_ID_INVALID",
    # Modules
    "core",
]

if "geom" in AVAILABLE_MODULES:
    __all__ += [
        "Point2",
        "Point3",
        "Vector3",
        "Direction3",
        "Axis1Placement",
        "Axis2Placement",
        "Axis3Placement",
        "Transform",
        "geom",
        "curves",
        "curves2d",
        "surfaces",
    ]
if "topo" in AVAILABLE_MODULES:
    __all__ += ["Graph", "Color", "topo", "topo_algo"]
if "prim" in AVAILABLE_MODULES:
    __all__.append("prim")
if "text" in AVAILABLE_MODULES:
    __all__.append("text")
if "bool_" in AVAILABLE_MODULES:
    __all__ += ["bool_"]
if "mesh" in AVAILABLE_MODULES:
    __all__.append("mesh")
if "heal" in AVAILABLE_MODULES:
    __all__.append("heal")
if "de" in AVAILABLE_MODULES:
    __all__.append("de")
if "io_brep" in AVAILABLE_MODULES:
    __all__.append("io_brep")
if "io_step" in AVAILABLE_MODULES:
    __all__.append("io_step")
if "io_iges" in AVAILABLE_MODULES:
    __all__.append("io_iges")
if "io_stl" in AVAILABLE_MODULES:
    __all__.append("io_stl")
if "io_obj" in AVAILABLE_MODULES:
    __all__.append("io_obj")
if "io_gltf" in AVAILABLE_MODULES:
    __all__.append("io_gltf")
if "io_vrml" in AVAILABLE_MODULES:
    __all__.append("io_vrml")
if "io_ply" in AVAILABLE_MODULES:
    __all__.append("io_ply")
if "viz" in AVAILABLE_MODULES:
    __all__.append("viz")
