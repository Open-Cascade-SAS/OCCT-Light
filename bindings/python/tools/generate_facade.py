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

"""Generate the cffi raw layer and Python wrappers from ``build/abi.json``.

Inputs
------
- ``build/abi.json`` (from ``tools/abi_dump.py``).

Outputs (under ``bindings/python/src/occtl/``)
------------------------------------------
- ``_generated/_raw.py`` — single ``ffi.cdef(...)`` plus ``ffi.dlopen`` boot.
- ``_generated/_abi.py`` — every ``OCCTL_*`` constant + ``Status`` / enum aliases.
- ``_generated/*.py``    — one module per C header, exposing a thin Python
                          function per ``OCCTL_API`` declaration. The hand-
                          written ``occtl.core / .geom / .topo / .prim / .text``
                          modules import these.

Design
------
Each Python wrapper has a small predictable shape:

- ``occtl_status_t`` returns turn into ``_check(...)`` calls.
- ``void`` returns become Python ``None``.
- Non-status non-void returns turn into the Python return value verbatim.
- Out-parameters are allocated on the stack via ``ffi.new`` and unpacked
  into the Python return tuple.
- ``occtl_node_id_t`` / ``occtl_uid_t`` etc. round-trip through their
  ``NamedTuple`` wrappers from ``occtl._ids``.
- Opaque handle pointers (``occtl_graph_t*``) are unwrapped via ``.as_ptr()``
  on input and returned as raw cffi pointers; the hand-written facade in
  ``occtl/topo.py`` etc. adopts them into RAII wrappers.

The generator is intentionally simple — it never tries to *be* the idiomatic
layer. Hand-written modules build the ergonomic surface on top of these
generated thin shims.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import textwrap
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Header → public module mapping
# ---------------------------------------------------------------------------

HEADER_TO_MODULE: Dict[str, str] = {
    "occtl.h": "umbrella",
    "occtl_core.h": "core",
    "occtl_geom.h": "geom",
    "occtl_curves_common.h": "geom",
    "occtl_curves.h": "curves",
    "occtl_curves2d.h": "curves2d",
    "occtl_surfaces.h": "surfaces",
    "occtl_topo.h": "topo",
    "occtl_topo_build.h": "topo",
    "occtl_topo_relation.h": "topo",
    "occtl_topo_types.h": "topo",
    "occtl_prim_feature.h": "prim",
    "occtl_prim_sketch.h": "prim",
    "occtl_prim_solid.h": "prim",
    "occtl_prim_sweep.h": "prim",
    "occtl_prim.h": "prim",
    "occtl_text.h": "text",
    "occtl_bool.h": "bool_",
    "occtl_mesh.h": "mesh",
    "occtl_topo_algo.h": "topo_algo",
    "occtl_io_brep.h": "io_brep",
    "occtl_io_step.h": "io_step",
    "occtl_io_iges.h": "io_iges",
    "occtl_io_stl.h": "io_stl",
    "occtl_io_obj.h": "io_obj",
    "occtl_io_gltf.h": "io_gltf",
    "occtl_io_vrml.h": "io_vrml",
    "occtl_io_ply.h": "io_ply",
    "occtl_viz.h": "viz",
    "occtl_de.h": "de",
    "occtl_heal.h": "heal",
}


def _validate_known_headers(abi: dict) -> None:
    headers = set(abi.get("headers") or ())
    headers.update(f.get("header", "") for f in abi.get("functions", ()))
    unknown = sorted(h for h in headers if h and h not in HEADER_TO_MODULE)
    if unknown:
        raise SystemExit(
            "generate_facade (python): abi.json contains unmapped headers: "
            + ", ".join(unknown)
        )

# Names that the C ABI uses but cffi cannot have in cdef (already in stdint).
SKIP_TYPES = set()

# Strip surrounding ``const`` for cffi argument types — cffi accepts the
# stripped spelling and gives identical behaviour for our uses.
def _strip_const(type_spelling: str) -> str:
    return re.sub(r"\bconst\b\s*", "", type_spelling).strip()


# ---------------------------------------------------------------------------
# cdef rendering
# ---------------------------------------------------------------------------

def _render_cdef(abi: dict) -> str:
    """Build a single cffi cdef block from the ABI dump.

    cffi's parser is strict: every type must be declared before use, and
    function-like macros / preprocessor directives are not allowed. We sort
    declarations in dependency order:

    1. fixed-width integer typedefs (cffi understands ``uint32_t`` natively,
       so we omit aliases for those).
    2. forward struct declarations for opaque handles.
    3. enum bodies + their typedefs.
    4. value-handle struct bodies + their typedefs.
    5. regular POD struct bodies + their typedefs.
    6. plain aliases.
    7. function prototypes.
    """
    types = abi["types"]
    functions = abi["functions"]

    out: List[str] = []

    # 1. Forward declarations for opaque handles.
    seen_tags: set[str] = set()
    for t in types:
        if t["kind"] != "opaque_handle":
            continue
        # The typedef name has trailing ``_t``; the C tag is its stem.
        name = t["name"]
        stem = name[:-2] if name.endswith("_t") else name
        if stem in seen_tags:
            continue
        seen_tags.add(stem)
        out.append(f"struct {stem};")
        out.append(f"typedef struct {stem} {name};")
    out.append("")

    # 2. Enums. cffi only understands the enum tag form when it's also
    #    typedef'd; we emit both, but dedupe by name so the typedef line is
    #    only the "ends with _t" pass.
    emitted_enums: set[str] = set()
    for t in types:
        if t["kind"] != "enum":
            continue
        name = t["name"]
        # The dump records both the enum-tag (``occtl_status``) and the
        # typedef (``occtl_status_t``). For cffi we want the typedef to map
        # to the tag definition once.
        if name.endswith("_t"):
            continue  # handled when we see the tag form
        if name in emitted_enums:
            continue
        emitted_enums.add(name)
        body = ",\n".join(
            f"    {v['name']} = {v['value']}" for v in t["values"]
        )
        out.append(f"typedef enum {name} {{\n{body}\n}} {name}_t;")
    out.append("")

    # 3. Value-handle structs.
    for t in types:
        if t["kind"] != "value_handle":
            continue
        name = t["name"]
        stem = name[:-2] if name.endswith("_t") else name
        bits_type = t.get("bits_field", "uint64_t")
        # The bits field is conventionally uint64_t. Some dumps record "int"
        # which is the C-level platform-int from the original typedef; force
        # a portable 64-bit field for cffi.
        if "uint" not in bits_type and "long" not in bits_type:
            bits_type = "uint64_t"
        out.append(f"typedef struct {stem} {{ {bits_type} bits; }} {name};")
    out.append("")

    # 4. POD structs. Two passes: structs with no occtl-typed fields first
    #    so a later struct can reference them.
    pod_structs = [t for t in types if t["kind"] == "struct"]
    # De-dupe by typedef name (struct-tag-only entries get rolled into the
    # _t one when present).
    emitted_structs: set[str] = set()
    pending: List[dict] = []
    for t in pod_structs:
        name = t["name"]
        if not name.endswith("_t"):
            # Omit non-typedef'd tags; in practice the dump always provides
            # both, and the typedef pass is enough.
            continue
        if name in emitted_structs:
            continue
        pending.append(t)

    def _struct_refs(t: dict) -> set[str]:
        refs: set[str] = set()
        for f in t.get("fields", []):
            spell = _strip_const(f["type"])
            tok = spell.replace("*", " ").replace("[", " ").split()
            for x in tok:
                if x.startswith("occtl_") and x.endswith("_t"):
                    refs.add(x)
        return refs

    # Topological sort by occtl-typed dependency edges.
    name_to_t: Dict[str, dict] = {t["name"]: t for t in pending}
    visiting: set[str] = set()
    ordered: List[dict] = []
    visited: set[str] = set()

    def _visit(name: str) -> None:
        if name in visited or name not in name_to_t:
            return
        if name in visiting:  # cycle — emit anyway
            return
        visiting.add(name)
        t = name_to_t[name]
        for dep in _struct_refs(t):
            if dep != name:
                _visit(dep)
        visiting.discard(name)
        visited.add(name)
        ordered.append(t)

    for t in pending:
        _visit(t["name"])

    for t in ordered:
        name = t["name"]
        emitted_structs.add(name)
        stem = name[:-2]
        # Some types are array members; emit a struct with the fields as-is.
        if not t.get("fields"):
            # Empty struct — cffi accepts ``struct {};``.
            out.append(f"typedef struct {stem} {{ }} {name};")
            continue
        field_lines = []
        for f in t["fields"]:
            field_type = _strip_const(f["type"])
            # libclang renders array fields as ``double[12]``; cffi expects
            # the C-canonical ``double m[12]`` form. Detect a trailing
            # ``[...]`` chain and relocate it to after the name.
            m_array = re.match(r"^(.*?)(\[[^\]]*\](?:\[[^\]]*\])*)\s*$", field_type)
            if m_array and m_array.group(2):
                base = m_array.group(1).strip()
                dims = m_array.group(2)
                field_lines.append(f"    {base} {f['name']}{dims};")
            else:
                field_lines.append(f"    {field_type} {f['name']};")
        body = "\n".join(field_lines)
        out.append(f"typedef struct {stem} {{\n{body}\n}} {name};")
    out.append("")

    # 5. Plain aliases.
    for t in types:
        if t["kind"] != "alias":
            continue
        name = t["name"]
        underlying = _strip_const(t["underlying"])
        # Function-pointer typedef: "RET (*)(ARGS)" → "typedef RET (*NAME)(ARGS);"
        if "(*)" in underlying:
            out.append(f"typedef {underlying.replace('(*)', f'(*{name})', 1)};")
        else:
            out.append(f"typedef {underlying} {name};")
    out.append("")

    # 6. Function prototypes.
    for f in sorted(functions, key=lambda f: f["name"]):
        ret = f["return_type"]
        params = f["params"]
        if not params:
            arg_str = "void"
        else:
            parts = []
            for p in params:
                # Drop ``const`` qualifiers — cffi accepts both forms but the
                # stripped form is canonical here.
                t = p["type"]
                parts.append(f"{t} {p['name']}")
            arg_str = ", ".join(parts)
        out.append(f"{ret} {f['name']}({arg_str});")

    return "\n".join(out)


# ---------------------------------------------------------------------------
# _raw.py
# ---------------------------------------------------------------------------

_RAW_HEADER = '''\
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
# AUTO-GENERATED by ``tools/generate_facade.py``. Do not edit by hand.
#
# Re-run with::
#
#     python3 tools/abi_dump.py --output build/abi.json
#     python3 bindings/python/tools/generate_facade.py
#
# This module is the cffi ABI-mode binding layer. It is **private**:
# users `import occtl`, never `occtl._raw`.

from __future__ import annotations

import ctypes
import os
import sys
from pathlib import Path

import cffi

ffi = cffi.FFI()

_CDEF = r"""
__CDEF_PLACEHOLDER__
"""

ffi.cdef(_CDEF)


def _shared_suffix() -> str:
    if sys.platform == "darwin":
        return "dylib"
    if sys.platform.startswith("win"):
        return "dll"
    return "so"


def _shared_lib_filename(base: str) -> str:
    suf = _shared_suffix()
    if sys.platform.startswith("win"):
        return f"{base}.{suf}"
    return f"lib{base}.{suf}"


_LIBRARY_BASES = ("full-viz", "full", "cad", "minimal", "geom", "core", "custom")


def _candidate_directories() -> "list[Path]":
    """Search path for the native library."""
    here = Path(__file__).resolve().parent
    out: list[Path] = []
    env = os.environ.get("OCCTL_LIBRARY_PATH")
    if env:
        for p in env.split(os.pathsep):
            if p:
                out.append(Path(p))
    # Adjacent to the package (wheel install layout).
    out.append(here)
    out.append(here / "native")
    # In a source checkout the libs land in build/<preset>/lib/.
    repo_root = here.parents[2]
    for preset in ("minimal", "cad", "full", "full-with-viz", "full-with-viz-shared", "geom-only", "core-only"):
        out.append(repo_root / "build" / preset / "lib")
        out.append(repo_root / "build" / preset / "bin")
    if sys.platform == "darwin":
        out.append(Path("/usr/local/lib"))
        out.append(Path("/opt/homebrew/lib"))
    return out


def _find_lib(base: str) -> "Path | None":
    """Return an absolute path to ``libocctl-<base>.<suf>`` or None."""
    fname = _shared_lib_filename(f"occtl-{base}")
    for d in _candidate_directories():
        candidate = d / fname
        if candidate.is_file():
            return candidate
    return None


def _preferred_bases(raw_name: str) -> "tuple[str, ...]":
    """Accept ``full``, ``occtl-full``, or full filename variants."""
    raw = raw_name.strip()
    if not raw:
        return ()

    suffix = "." + _shared_suffix()
    candidates: list[str] = [raw]
    if raw.endswith(suffix):
        candidates.append(raw[: -len(suffix)])

    normalized: list[str] = []
    seen: set[str] = set()
    for candidate in candidates:
        value = candidate
        if value.startswith("lib"):
            value = value[3:]
        if value.startswith("occtl-"):
            value = value[len("occtl-") :]
        if value and value not in seen:
            normalized.append(value)
            seen.add(value)
    return tuple(normalized)


def _open_library():
    """Locate and load the single OCCT-Light feature-set shared library."""
    preferred = os.environ.get("OCCTL_LIBRARY_NAME")
    bases = _preferred_bases(preferred) if preferred else _LIBRARY_BASES
    for base in bases:
        if not base:
            continue
        path = _find_lib(base)
        if path is None:
            continue
        try:
            return ffi.dlopen(str(path))
        except OSError:
            continue

    raise OSError(
        "occtl: failed to locate an OCCT-Light feature-set shared library. "
        "Build with OCCTL_SHARED_LIBS=ON and either install or set "
        "OCCTL_LIBRARY_PATH to the build/<preset>/lib/ directory. "
        "Set OCCTL_LIBRARY_NAME to values like 'full' or 'occtl-full'."
    )


lib = _open_library()
'''


def write_raw(out_path: Path, cdef: str) -> int:
    text = _RAW_HEADER.replace("__CDEF_PLACEHOLDER__", cdef)
    out_path.write_text(text, encoding="utf-8")
    return text.count("\n") + 1


# ---------------------------------------------------------------------------
# _abi.py — constants + version stamps
# ---------------------------------------------------------------------------

_ABI_HEADER = '''\
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
# AUTO-GENERATED by ``tools/generate_facade.py``. Do not edit by hand.
"""Mirror of ``OCCTL_*`` ABI version stamps and constants.

These values are baked in at generation time (read from
``build/abi.json``). At runtime, ``occtl.__init__`` compares
:data:`ABI_VERSION` against ``occtl_runtime_abi_version()`` and refuses to
proceed on mismatch.
"""

from __future__ import annotations

'''


def write_abi(out_path: Path, abi: dict) -> int:
    constants = abi["constants"]
    lib_v = abi["library_version"] or {}
    headers = sorted(abi.get("headers") or [])
    modules = sorted(
        {
            module
            for header in headers
            for module in [HEADER_TO_MODULE.get(header)]
            if module not in (None, "umbrella")
        }
    )
    lines = [_ABI_HEADER]
    lines.append(f"ABI_VERSION: int = {int(abi.get('abi_version') or 0)}")
    lines.append(
        f"LIBRARY_VERSION: tuple[int, int, int] = ("
        f"{int(lib_v.get('major') or 0)}, "
        f"{int(lib_v.get('minor') or 0)}, "
        f"{int(lib_v.get('patch') or 0)})"
    )
    lines.append(f"ABI_HEADERS: tuple[str, ...] = {tuple(headers)!r}")
    lines.append(f"AVAILABLE_MODULES: frozenset[str] = frozenset({tuple(modules)!r})")
    lines.append("")
    lines.append("# Public OCCTL_* constants extracted from the headers.")
    for c in sorted(constants, key=lambda c: c["name"]):
        value = c["value"]
        if isinstance(value, int):
            lines.append(f"{c['name']}: int = {value}")
        elif isinstance(value, str):
            # Best-effort: parse int-ish strings; otherwise expose as repr.
            stripped = value.strip()
            if re.fullmatch(r"-?\d+", stripped):
                lines.append(f"{c['name']}: int = {int(stripped)}")
            elif re.fullmatch(r"0x[0-9a-fA-F]+", stripped):
                lines.append(f"{c['name']}: int = {int(stripped, 16)}")
            else:
                # Omit non-int macros (e.g. compound literals).
                lines.append(f"# {c['name']} = {stripped!r}  # non-int macro")
        else:
            lines.append(f"# {c['name']} = None  # no value extracted")
    text = "\n".join(lines) + "\n"
    out_path.write_text(text, encoding="utf-8")
    return text.count("\n")


# ---------------------------------------------------------------------------
# Generated wrapper modules
# ---------------------------------------------------------------------------

_WRAPPER_HEADER = '''\
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
# AUTO-GENERATED by ``tools/generate_facade.py``. Do not edit by hand.
"""Thin Python wrappers around the {header} surface.

Every public ``occtl_*`` function in this header has a same-named Python
function here that:

- Allocates cffi storage for out-parameters.
- Unwraps ``NodeId`` / ``Uid`` / ``RefId`` / ``RepId`` value handles to
  their underlying ``.bits`` value on input.
- Wraps the same value handles back into their ``NamedTuple`` form on output.
- Translates a non-OK status into a typed :mod:`occtl._errors` exception.
- Returns out-parameter values directly (as a tuple if there are multiple).

Opaque handle pointers (``occtl_graph_t*``, ``occtl_curve_t*``, …) cross
unchanged as raw cffi pointers; the hand-written ``occtl.core / .geom / .topo
/ .prim / .text / .curves / .curves2d / .surfaces`` modules adopt them into
RAII wrappers from :mod:`occtl._handles`.

Do not import this module directly. Import the corresponding public module
instead.
"""

from __future__ import annotations

from ._raw import ffi, lib
from .._errors import _check
from .._ids import NodeId, Uid, RefId, RefUid, RepId, RepUid

'''


# C type name → (cffi out-slot ctype, post-call extractor expression)
_VALUE_HANDLE_CTYPES = {
    "occtl_node_id_t": ("occtl_node_id_t", "NodeId(int({slot}[0].bits))"),
    "occtl_uid_t":     ("occtl_uid_t",     "Uid(int({slot}[0].bits))"),
    "occtl_ref_id_t":  ("occtl_ref_id_t",  "RefId(int({slot}[0].bits))"),
    "occtl_ref_uid_t": ("occtl_ref_uid_t", "RefUid(int({slot}[0].bits))"),
    "occtl_rep_id_t":  ("occtl_rep_id_t",  "RepId(int({slot}[0].bits))"),
    "occtl_rep_uid_t": ("occtl_rep_uid_t", "RepUid(int({slot}[0].bits))"),
}


def _is_value_handle(elem_type: str) -> bool:
    return elem_type in _VALUE_HANDLE_CTYPES


def _is_opaque_handle_ptr(elem_type: str, opaque_types: set[str]) -> bool:
    if elem_type is None:
        return False
    stripped = _strip_const(elem_type)
    return stripped in opaque_types


def _is_two_call_elem(
    elem: str | None,
    pod_types: set[str],
    enum_types: set[str],
) -> bool:
    if elem is None:
        return False
    return (
        _is_value_handle(elem)
        or elem in pod_types
        or elem in enum_types
        or elem in ("int", "uint32_t", "uint64_t", "int32_t", "int64_t", "size_t", "double", "float")
    )


def _find_two_call_buffer_pattern(
    params: list[dict],
    pod_types: set[str],
    enum_types: set[str],
) -> tuple[int, int, int, str] | None:
    out_idx = -1
    out_elem: str | None = None
    for idx, p in enumerate(params):
        elem = _strip_const(p["elem_type"]) if p.get("elem_type") else None
        if (
            p.get("direction") == "out"
            and p.get("pointer_depth") == 1
            and p.get("name") == "out_buf"
            and _is_two_call_elem(elem, pod_types, enum_types)
        ):
            out_idx = idx
            out_elem = elem
            break
    if out_idx < 0 or out_elem is None:
        return None

    cap_idx = -1
    count_idx = -1
    for idx, p in enumerate(params):
        elem = _strip_const(p["elem_type"]) if p.get("elem_type") else _strip_const(p["type"])
        if p.get("pointer_depth") == 0 and p.get("name") in ("cap", "capacity"):
            if elem in ("int", "uint32_t", "size_t"):
                cap_idx = idx
        if (
            p.get("direction") == "out"
            and p.get("pointer_depth") == 1
            and p.get("name") == "out_count"
            and elem in ("int", "uint32_t", "size_t")
        ):
            count_idx = idx
    if cap_idx < 0 or count_idx < 0:
        return None
    return out_idx, cap_idx, count_idx, out_elem


def _append_input_param_for_call(
    p: dict,
    in_args: list[str],
    setup: list[str],
    call_args: list[str],
) -> None:
    pname = p["name"] or f"arg{len(in_args)}"
    py_pname = _safe_pyname(pname)
    ctype = p["type"]
    elem = _strip_const(p["elem_type"]) if p["elem_type"] else None
    ptr_depth = p["pointer_depth"]

    in_args.append(py_pname)
    if ptr_depth == 0 and _is_value_handle(_strip_const(ctype)):
        base = _strip_const(ctype)
        call_args.append(f"_pack_value_handle({py_pname}, '{base}')")
    elif ptr_depth == 1 and _is_value_handle(elem):
        tmp = f"_in_{py_pname}"
        setup.append(f"    {tmp} = _pack_value_handle_ptr({py_pname}, '{elem}')")
        call_args.append(tmp)
    elif ptr_depth >= 1:
        call_args.append(f"({py_pname} if {py_pname} is not None else ffi.NULL)")
    else:
        call_args.append(py_pname)


def _two_call_extract_expr(elem: str, slot: str) -> str:
    if _is_value_handle(elem):
        cls = {
            "occtl_node_id_t": "NodeId",
            "occtl_uid_t": "Uid",
            "occtl_ref_id_t": "RefId",
            "occtl_ref_uid_t": "RefUid",
            "occtl_rep_id_t": "RepId",
            "occtl_rep_uid_t": "RepUid",
        }[elem]
        return f"{cls}(int({slot}.bits))"
    if elem in ("double", "float"):
        return f"float({slot})"
    if elem in ("int", "uint32_t", "uint64_t", "int32_t", "int64_t", "size_t"):
        return f"int({slot})"
    return slot


def _function_docstring(f: dict) -> str:
    doc_lines: "list[str]" = []
    summary = (f.get("doc") or "").strip()
    if summary:
        doc_lines.append(summary)
        doc_lines.append("")

    for p in f.get("params", []):
        pd = (p.get("doc") or "").strip()
        if not pd:
            continue
        pname = p.get("name") or ""
        if not pname:
            continue
        pdir = p.get("direction", "in")
        doc_lines.append(f":param {_safe_pyname(pname)}: {pd}")
        if pdir != "in":
            doc_lines[-1] += f"  [{pdir}]"
    if any(p.get("doc") for p in f.get("params", [])):
        doc_lines.append("")

    for rv in f.get("retvals", []):
        code = rv.get("code", "")
        rd = (rv.get("doc") or "").strip()
        if not code:
            continue
        doc_lines.append(f":retval {code}: {rd}")
    rd = (f.get("return_doc") or "").strip()
    if rd:
        doc_lines.append(f":returns: {rd}")
    if f.get("retvals") or rd:
        doc_lines.append("")

    ts = (f.get("threadsafe") or "").strip()
    if ts:
        doc_lines.append(f"**Threadsafe:** {ts}")
        doc_lines.append("")

    sa = f.get("see_also") or []
    if sa:
        refs = [r for r in sa if r.startswith("occtl_")]
        if refs:
            doc_lines.append(".. seealso:: " + ", ".join(refs))
            doc_lines.append("")

    while doc_lines and not doc_lines[-1]:
        doc_lines.pop()

    if not doc_lines:
        return ""
    raw = "\n".join(doc_lines).replace('"""', "'''")
    return f'    """{raw}"""'


def _emit_function(
    f: dict,
    opaque_types: set[str],
    pod_types: set[str],
    enum_types: set[str],
) -> str:
    """Emit one Python function wrapping ``f``."""
    name = f["name"]
    py_name = name  # keep occtl_ prefix; the public modules re-export under tidier names
    params = f["params"]
    ret_type = _strip_const(f["return_type"])

    in_args: List[str] = []     # python signature
    setup: List[str] = []        # body before the call
    call_args: List[str] = []    # arguments passed to the C function
    extract: List[Tuple[str, str]] = []  # (var, python-expr) for output values
    has_status_return = ret_type == "occtl_status_t"
    returns_void = ret_type == "void"

    out_slot_counter = 0
    two_call = _find_two_call_buffer_pattern(params, pod_types, enum_types)

    if has_status_return and two_call is not None:
        out_idx, cap_idx, count_idx, out_elem = two_call
        for idx, p in enumerate(params):
            if idx in (out_idx, cap_idx, count_idx):
                continue
            _append_input_param_for_call(p, in_args, setup, call_args)

        sizing_args: list[str] = []
        refill_args: list[str] = []
        call_iter = iter(call_args)
        for idx, _p in enumerate(params):
            if idx == out_idx:
                sizing_args.append("ffi.NULL")
                refill_args.append("_out_buf")
            elif idx == cap_idx:
                sizing_args.append("0")
                refill_args.append("_count")
            elif idx == count_idx:
                sizing_args.append("_out_count")
                refill_args.append("_out_count")
            else:
                arg = next(call_iter)
                sizing_args.append(arg)
                refill_args.append(arg)

        body: List[str] = []
        body.extend(setup)
        body.append("    _out_count = ffi.new('size_t*')")
        body.append(f"    _status = lib.{name}({', '.join(sizing_args)})")
        body.append("    _check(_status)")
        body.append("    _count = int(_out_count[0])")
        body.append(f"    _out_buf = ffi.new('{out_elem}[]', _count)")
        body.append("    if _count:")
        body.append(f"        _status = lib.{name}({', '.join(refill_args)})")
        body.append("        _check(_status)")
        expr = _two_call_extract_expr(out_elem, "_out_buf[_i]")
        body.append(f"    return [{expr} for _i in range(_count)]")

        sig = f"def {py_name}({', '.join(in_args)}):"

        docstring = _function_docstring(f)
        parts = [sig]
        if docstring:
            parts.append(docstring)
        parts.extend(body)
        return "\n".join(parts) + "\n"

    for p in params:
        pname = p["name"] or f"arg{len(in_args)}"
        py_pname = _safe_pyname(pname)
        ctype = p["type"]
        elem = _strip_const(p["elem_type"]) if p["elem_type"] else None
        ptr_depth = p["pointer_depth"]
        direction = p["direction"]

        if direction == "out" and ptr_depth == 1 and _is_value_handle(elem):
            slot_var = f"_out_{py_pname}"
            cspec, extractor = _VALUE_HANDLE_CTYPES[elem]
            setup.append(f"    {slot_var} = ffi.new('{cspec}*')")
            call_args.append(slot_var)
            extract.append((slot_var, extractor.format(slot=slot_var)))
            continue

        if direction == "out" and ptr_depth == 1 and elem in pod_types:
            slot_var = f"_out_{py_pname}"
            setup.append(f"    {slot_var} = ffi.new('{elem}*')")
            call_args.append(slot_var)
            extract.append((slot_var, f"{slot_var}[0]"))
            continue

        if direction == "out" and ptr_depth == 1 and elem in enum_types:
            slot_var = f"_out_{py_pname}"
            setup.append(f"    {slot_var} = ffi.new('{elem}*')")
            call_args.append(slot_var)
            extract.append((slot_var, f"int({slot_var}[0])"))
            continue

        if direction == "out" and ptr_depth == 1 and elem in ("int", "uint32_t", "uint64_t", "int32_t", "int64_t", "size_t"):
            slot_var = f"_out_{py_pname}"
            setup.append(f"    {slot_var} = ffi.new('{elem}*')")
            call_args.append(slot_var)
            extract.append((slot_var, f"int({slot_var}[0])"))
            continue

        if direction == "out" and ptr_depth == 1 and elem in ("double", "float"):
            slot_var = f"_out_{py_pname}"
            setup.append(f"    {slot_var} = ffi.new('{elem}*')")
            call_args.append(slot_var)
            extract.append((slot_var, f"float({slot_var}[0])"))
            continue

        if (
            direction == "out"
            and ptr_depth == 2
            and elem
            and _is_opaque_handle_ptr(elem, opaque_types)
        ):
            slot_var = f"_out_{py_pname}"
            setup.append(f"    {slot_var} = ffi.new('{elem}**')")
            call_args.append(slot_var)
            extract.append((slot_var, f"{slot_var}[0]"))
            continue

        # Input-side handling.
        in_args.append(py_pname)

        # NodeId / Uid / RefId / RepId by value → unwrap.bits if present.
        if ptr_depth == 0 and _is_value_handle(_strip_const(ctype)):
            base = _strip_const(ctype)
            call_args.append(f"_pack_value_handle({py_pname}, '{base}')")
            continue

        # Pointer to a value-handle in input — pass through ``ffi.new`` if user
        # passes a NamedTuple, else assume already-pointer.
        if ptr_depth == 1 and _is_value_handle(elem):
            tmp = f"_in_{py_pname}"
            setup.append(
                f"    {tmp} = _pack_value_handle_ptr({py_pname}, '{elem}')"
            )
            call_args.append(tmp)
            continue

        # NULL-able pointer input. Accept Python ``None``.
        if ptr_depth >= 1:
            call_args.append(f"({py_pname} if {py_pname} is not None else ffi.NULL)")
            continue

        # Scalar by value — pass through.
        call_args.append(py_pname)

    arg_str_python = ", ".join(in_args) if in_args else ""

    body: List[str] = []
    body.extend(setup)

    call_expr = f"lib.{name}({', '.join(call_args)})"
    if has_status_return:
        body.append(f"    _status = {call_expr}")
        body.append("    _check(_status)")
    elif returns_void:
        body.append(f"    {call_expr}")
    else:
        body.append(f"    _ret = {call_expr}")

    # Build the return tuple.
    return_pieces: List[str] = []
    if not has_status_return and not returns_void:
        # Top-level return value from C.
        if ret_type in _VALUE_HANDLE_CTYPES:
            cls = {
                "occtl_node_id_t": "NodeId",
                "occtl_uid_t": "Uid",
                "occtl_ref_id_t": "RefId",
                "occtl_ref_uid_t": "RefUid",
                "occtl_rep_id_t": "RepId",
                "occtl_rep_uid_t": "RepUid",
            }[ret_type]
            return_pieces.append(f"{cls}(int(_ret.bits))")
        elif ret_type in ("const char *", "char *"):
            return_pieces.append(
                "(ffi.string(_ret).decode('utf-8', errors='replace') if _ret else '')"
            )
        elif ret_type == "const occtl_error_t *":
            return_pieces.append("_ret")  # caller-level wrapping
        elif ret_type in ("int", "uint32_t", "int32_t", "uint64_t", "int64_t", "size_t"):
            return_pieces.append("int(_ret)")
        elif ret_type in ("double", "float"):
            return_pieces.append("float(_ret)")
        elif ret_type in enum_types:
            return_pieces.append("int(_ret)")
        else:
            return_pieces.append("_ret")

    for _slot, expr in extract:
        return_pieces.append(expr)

    if not return_pieces:
        # Nothing to return.
        ret_stmt = ""
    elif len(return_pieces) == 1:
        ret_stmt = f"    return {return_pieces[0]}"
    else:
        ret_stmt = f"    return ({', '.join(return_pieces)})"

    if ret_stmt:
        body.append(ret_stmt)
    else:
        body.append("    return None")

    sig = f"def {py_name}({arg_str_python}):"

    # Build a rich docstring from the ABI dump: summary + param docs + retvals
    # + thread-safety annotation + @sa cross-references.
    doc_lines: "list[str]" = []
    summary = (f.get("doc") or "").strip()
    if summary:
        doc_lines.append(summary)
        doc_lines.append("")

    for p in f.get("params", []):
        pd = (p.get("doc") or "").strip()
        if not pd:
            continue
        pname = p.get("name") or ""
        if not pname:
            continue
        pdir = p.get("direction", "in")
        doc_lines.append(f":param {_safe_pyname(pname)}: {pd}")
        if pdir != "in":
            doc_lines[-1] += f"  [{pdir}]"
    if any(p.get("doc") for p in f.get("params", [])):
        doc_lines.append("")

    for rv in f.get("retvals", []):
        code = rv.get("code", "")
        rd = (rv.get("doc") or "").strip()
        if not code:
            continue
        doc_lines.append(f":retval {code}: {rd}")
    rd = (f.get("return_doc") or "").strip()
    if rd:
        doc_lines.append(f":returns: {rd}")
    if f.get("retvals") or rd:
        doc_lines.append("")

    ts = (f.get("threadsafe") or "").strip()
    if ts:
        doc_lines.append(f"**Threadsafe:** {ts}")
        doc_lines.append("")

    sa = f.get("see_also") or []
    if sa:
        # Filter down to just the public function names
        refs = [r for r in sa if r.startswith("occtl_")]
        if refs:
            doc_lines.append(".. seealso:: " + ", ".join(refs))
            doc_lines.append("")

    # Trim trailing blank lines so the closing triple-quote sits flush.
    while doc_lines and not doc_lines[-1]:
        doc_lines.pop()

    if doc_lines:
        raw = "\n".join(doc_lines).replace('"""', "'''")
        docstring = f'    """{raw}"""'
    else:
        docstring = ""

    parts = [sig]
    if docstring:
        parts.append(docstring)
    parts.extend(body)
    return "\n".join(parts) + "\n"


def _safe_pyname(name: str) -> str:
    """Avoid Python keyword collisions."""
    import keyword
    if keyword.iskeyword(name) or name in ("type", "id", "input"):
        return name + "_"
    return name


_VALUE_HANDLE_HELPERS = '''
def _pack_value_handle(value, ctype):
    """Coerce a ``NodeId`` / ``Uid`` / ``RefId`` / ``RepId`` / int / cdata into a
    cffi struct value of ``ctype``.
    """
    if hasattr(value, "bits"):
        bits = int(value.bits)
    elif isinstance(value, int):
        bits = value
    else:
        # Already a cdata struct; pass through.
        return value
    slot = ffi.new(ctype + "*")
    slot.bits = bits
    return slot[0]


def _pack_value_handle_ptr(value, ctype):
    if value is None:
        return ffi.NULL
    if hasattr(value, "bits"):
        slot = ffi.new(ctype + "*")
        slot.bits = int(value.bits)
        return slot
    if isinstance(value, int):
        slot = ffi.new(ctype + "*")
        slot.bits = value
        return slot
    return value
'''


def write_generated_wrappers(
    package_dir: Path,
    abi: dict,
) -> Dict[str, int]:
    gen_dir = package_dir / "_generated"
    gen_dir.mkdir(parents=True, exist_ok=True)
    (gen_dir / "__init__.py").write_text(
        "# Copyright (c) 2026 Capgemini Engineering Research and Development.\n"
        "#\n"
        "# This file is part of OCCT-Light software library.\n"
        "#\n"
        "# This library is free software; you can redistribute it and/or modify it under\n"
        "# the terms of the GNU Affero General Public License version 3 as published\n"
        "# by the Free Software Foundation, with an option to use any later version.\n"
        "# Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution\n"
        "# for complete text of the license and disclaimer of any warranty.\n"
        "#\n"
        "# Alternatively, this file may be used under the terms of a commercial\n"
        "# license or contractual agreement.\n"
        "#\n"
        "# SPDX-License-Identifier: AGPL-3.0-or-later\n"
        "# AUTO-GENERATED. Private subpackage holding thin wrappers per C header.\n",
        encoding="utf-8",
    )

    types = abi["types"]
    opaque_types = {t["name"] for t in types if t["kind"] == "opaque_handle"}
    pod_types = {t["name"] for t in types if t["kind"] == "struct" and t["name"].endswith("_t")}
    enum_types = {t["name"] for t in types if t["kind"] == "enum" and t["name"].endswith("_t")}

    # Group functions by generated Python module. Several public headers can
    # intentionally feed one module after the topology header split.
    by_module: Dict[str, List[dict]] = {}
    for f in abi["functions"]:
        module_name = HEADER_TO_MODULE.get(f["header"])
        if module_name in (None, "umbrella"):
            continue
        by_module.setdefault(module_name, []).append(f)

    line_counts: Dict[str, int] = {}
    for module_name, functions in sorted(by_module.items()):
        out_path = gen_dir / f"{module_name}.py"
        body = [
            _WRAPPER_HEADER.replace("{header}", f"{module_name} module headers"),
            _VALUE_HANDLE_HELPERS,
            "",
        ]
        for f in sorted(functions, key=lambda f: f["name"]):
            body.append(_emit_function(f, opaque_types, pod_types, enum_types))
        text = "\n".join(body)
        out_path.write_text(text, encoding="utf-8")
        line_counts[module_name] = text.count("\n") + 1

    return line_counts


# ---------------------------------------------------------------------------
# Generated typed facades
# ---------------------------------------------------------------------------

_TYPED_HEADER = '''\
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
# AUTO-GENERATED by ``bindings/python/tools/generate_facade.py``. Do not edit by hand.
"""Generated typed Python facade for selected OCCT-Light APIs.

This layer is generated from ``build/abi.json``.  The public modules may
re-export these symbols and add only the small amount of genuinely ergonomic
code that cannot be inferred from the C ABI metadata.
"""

'''


_PRIM_TYPED_NAMES = {
    "box": "BoxInfo",
    "sphere": "SphereInfo",
    "cylinder": "CylinderInfo",
    "cone": "ConeInfo",
    "torus": "TorusInfo",
    "wedge": "WedgeInfo",
}

_PRIM_REQUIRED_FIELDS = {
    "box": ("dx", "dy", "dz"),
    "sphere": ("radius",),
    "cylinder": ("radius", "height"),
    "cone": ("r1", "r2", "height"),
    "torus": ("r1", "r2"),
    "wedge": ("dx", "dy", "dz", "ltx"),
}

_PRIM_DEFAULT_FIELDS = {
    ("sphere", "angle1"): "-math.pi / 2.0",
    ("sphere", "angle2"): "math.pi / 2.0",
    ("sphere", "angle"): "math.tau",
    ("cylinder", "angle"): "math.tau",
    ("cone", "angle"): "math.tau",
    ("torus", "angle1"): "0.0",
    ("torus", "angle2"): "math.tau",
    ("torus", "angle"): "math.tau",
}


def _public_prim_name(c_name: str) -> str | None:
    prefix = "occtl_prim_"
    suffix = "_info_t"
    if not c_name.startswith(prefix) or not c_name.endswith(suffix):
        return None
    stem = c_name[len(prefix):-len(suffix)]
    return stem if stem in _PRIM_TYPED_NAMES else None


def _field_doc(field: dict) -> str:
    doc = (field.get("doc") or "").strip()
    doc = re.sub(r"@(c|p)\s+", "", doc)
    return re.sub(r"\s+", " ", doc).replace('"""', "'''")


def _emit_prim_dataclass(stem: str, struct: dict) -> str:
    cls_name = _PRIM_TYPED_NAMES[stem]
    required = set(_PRIM_REQUIRED_FIELDS[stem])
    fields = [
        f for f in struct.get("fields", [])
        if f["name"] not in ("struct_version", "p_next")
    ]

    lines: List[str] = [
        "@dataclass(frozen=True, slots=True)",
        f"class {cls_name}:",
        f'    """Options for :func:`make_{stem}`.',
        "",
        f"    Generated from ``{struct['name']}``.",
    ]
    for field in fields:
        if field["name"] == "placement":
            continue
        doc = _field_doc(field)
        if doc:
            lines.append(f"    :attr {field['name']}: {doc}")
    lines.extend(['    """', ""])

    ordered = [f for f in fields if f["name"] in required]
    ordered.extend(f for f in fields if f["name"] not in required)

    for field in ordered:
        name = field["name"]
        c_type = _strip_const(field["type"])
        if name == "placement":
            lines.append(
                "    placement: Axis2Placement = "
                "field(default_factory=Axis2Placement.world)"
            )
        elif name in required:
            lines.append(f"    {name}: float")
        elif c_type in ("double", "float"):
            default = _PRIM_DEFAULT_FIELDS.get((stem, name), "0.0")
            lines.append(f"    {name}: float = {default}")
        else:
            lines.append(f"    {name}: int = 0")
    return "\n".join(lines) + "\n"


def _emit_prim_function(stem: str, struct: dict) -> str:
    cls_name = _PRIM_TYPED_NAMES[stem]
    c_struct = struct["name"]
    c_init = f"occtl_prim_{stem}_info_init"
    c_make = f"occtl_prim_make_{stem}"
    fields = [
        f for f in struct.get("fields", [])
        if f["name"] not in ("struct_version", "p_next")
    ]

    union = " | float" if stem == "sphere" else ""
    lines = [
        f"def make_{stem}(graph: \"Graph\", info: {cls_name}{union}) -> NodeId:",
        f'    """Build ``{stem}`` and insert it into ``graph``."""',
    ]
    if stem == "sphere":
        lines.extend([
            f"    if not isinstance(info, {cls_name}):",
            f"        info = {cls_name}(radius=float(info))",
        ])
    lines.extend([
        f'    c = ffi.new("{c_struct}*")',
        f"    lib.{c_init}(c)",
    ])
    for field in fields:
        name = field["name"]
        if name == "placement":
            lines.append("    info.placement._write_into(c.placement)")
        elif _strip_const(field["type"]) in ("double", "float"):
            lines.append(f"    c.{name} = float(info.{name})")
        else:
            lines.append(f"    c.{name} = int(info.{name})")
    lines.extend([
        '    out = ffi.new("occtl_node_id_t*")',
        f"    _check(lib.{c_make}(graph._as_ptr(), c, out))",
        "    return NodeId(int(out.bits))",
    ])
    return "\n".join(lines) + "\n"


def write_typed_facades(package_dir: Path, abi: dict) -> Dict[str, int]:
    typed_dir = package_dir / "_generated" / "_typed"
    typed_dir.mkdir(parents=True, exist_ok=True)
    (typed_dir / "__init__.py").write_text(
        "# Copyright (c) 2026 Capgemini Engineering Research and Development.\n"
        "#\n"
        "# This file is part of OCCT-Light software library.\n"
        "#\n"
        "# This library is free software; you can redistribute it and/or modify it under\n"
        "# the terms of the GNU Affero General Public License version 3 as published\n"
        "# by the Free Software Foundation, with an option to use any later version.\n"
        "# Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution\n"
        "# for complete text of the license and disclaimer of any warranty.\n"
        "#\n"
        "# Alternatively, this file may be used under the terms of a commercial\n"
        "# license or contractual agreement.\n"
        "#\n"
        "# SPDX-License-Identifier: AGPL-3.0-or-later\n"
        "# AUTO-GENERATED. Private package holding typed facades.\n",
        encoding="utf-8",
    )

    structs = {
        stem: t
        for t in abi["types"]
        for stem in [_public_prim_name(t.get("name", ""))]
        if stem is not None and t.get("kind") == "struct"
    }

    ordered_stems = ["box", "sphere", "cylinder", "cone", "torus", "wedge"]
    body: List[str] = [
        _TYPED_HEADER,
        "from __future__ import annotations",
        "",
        "from dataclasses import dataclass, field",
        "import math",
        "from typing import TYPE_CHECKING",
        "",
        "from ..._errors import _check",
        "from ..._ids import NodeId",
        "from .._raw import ffi, lib",
        "from ...geom import Axis2Placement",
        "",
        "if TYPE_CHECKING:",
        "    from ...topo import Graph",
        "",
    ]

    exported: List[str] = []
    overrides: List[str] = []
    for stem in ordered_stems:
        struct = structs.get(stem)
        if not struct:
            continue
        body.append(_emit_prim_dataclass(stem, struct))
        body.append("")
        exported.append(_PRIM_TYPED_NAMES[stem])
    for stem in ordered_stems:
        struct = structs.get(stem)
        if not struct:
            continue
        body.append(_emit_prim_function(stem, struct))
        body.append("")
        exported.append(f"make_{stem}")
        overrides.append(f"occtl_prim_make_{stem}")
        overrides.append(f"occtl_prim_{stem}_info_init")

    body.append(f"__all__ = {exported!r}")
    body.append(f"_IDIOMATIC_OVERRIDES = frozenset({tuple(sorted(overrides))!r})")
    body.append("")

    text = "\n".join(body)
    out_path = typed_dir / "prim.py"
    out_path.write_text(text, encoding="utf-8")
    return {"_generated/_typed/prim.py": text.count("\n") + 1}


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv: "list[str] | None" = None) -> int:
    parser = argparse.ArgumentParser(
        description="Generate the cffi raw layer and auto-wrappers from build/abi.json."
    )
    here = Path(__file__).resolve()
    repo_root = here.parents[3]  # tools -> python -> bindings -> repo
    parser.add_argument(
        "--abi-json",
        type=Path,
        default=repo_root / "build" / "abi.json",
    )
    parser.add_argument(
        "--package-root",
        type=Path,
        default=here.parents[1] / "src" / "occtl",
    )
    args = parser.parse_args(argv)

    abi = json.loads(args.abi_json.read_text(encoding="utf-8"))

    # Shared schema check — see tools/abi_schema.py. Fails loud if the
    # producer drifted away from the shape this generator expects.
    sys.path.insert(0, str(repo_root / "tools"))
    from abi_schema import SCHEMA_VERSION, SchemaMismatch, validate
    try:
        validate(abi, expected_schema_version=SCHEMA_VERSION)
    except SchemaMismatch as e:
        raise SystemExit(f"generate_facade (python): abi.json schema mismatch: {e}")
    _validate_known_headers(abi)

    pkg_root: Path = args.package_root
    pkg_root.mkdir(parents=True, exist_ok=True)

    cdef = _render_cdef(abi)
    gen_root = pkg_root / "_generated"
    gen_root.mkdir(parents=True, exist_ok=True)

    raw_lines = write_raw(gen_root / "_raw.py", cdef)
    abi_lines = write_abi(gen_root / "_abi.py", abi)
    gen_lines = write_generated_wrappers(pkg_root, abi)
    typed_lines = write_typed_facades(pkg_root, abi)

    summary = {
        "_generated/_raw.py": raw_lines,
        "_generated/_abi.py": abi_lines,
        **{f"_generated/{k}.py": v for k, v in gen_lines.items()},
        **typed_lines,
    }
    for k, v in summary.items():
        print(f"  {k:32s}{v} lines")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
