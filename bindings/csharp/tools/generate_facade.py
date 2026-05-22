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
C# facade generator for OCCT-Light.

Consumes ``build/abi.json`` (produced by ``tools/abi_dump.py``) and emits the
``.g.cs`` files under ``bindings/csharp/src/ffi/Generated`` and
``bindings/csharp/src/api/_Generated``.

Run:

    python3 bindings/csharp/tools/generate_facade.py \\
        --abi build/abi.json \\
        --root bindings/csharp

The generator is intentionally simple — one Python file, no Jinja, no AST.
Each emitter function appends to a ``StringBuilder``-shaped list of lines and
the result is written verbatim. Re-running with the same input produces
byte-identical output (sorted iteration order, no timestamps).

Schema notes (see ``tools/abi_dump.py``):

- ``functions[i]`` carries ``name``, ``return_type``, ``params[]``, ``header``.
- ``types[i].kind`` ∈ {enum, struct, value_handle, opaque_handle, alias}.
- Each enum and each value handle appears twice: once with the bare name
  (``occtl_status``) and once with the ``_t`` suffix (``occtl_status_t``).
  We deduplicate on the ``_t``-suffixed form.
- For value handles the ``bits_field`` reported by libclang is ``int``
  even though the header says ``uint64_t``; we override to ``ulong``.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Type mapping
# ---------------------------------------------------------------------------

# Map a C scalar/typedef name → (managed type, blittable flag, P/Invoke marshal hint).
# Pointer types and known-handle types are resolved by the type emitter, not here.
PRIM_MAP: "dict[str, str]" = {
    "void": "void",
    "char": "sbyte",
    "signed char": "sbyte",
    "unsigned char": "byte",
    "short": "short",
    "unsigned short": "ushort",
    "int": "int",
    "unsigned int": "uint",
    "long": "long",
    "unsigned long": "ulong",
    "long long": "long",
    "unsigned long long": "ulong",
    "float": "float",
    "double": "double",
    "size_t": "nuint",
    "ssize_t": "nint",
    "ptrdiff_t": "nint",
    "int8_t": "sbyte",
    "uint8_t": "byte",
    "int16_t": "short",
    "uint16_t": "ushort",
    "int32_t": "int",
    "uint32_t": "uint",
    "int64_t": "long",
    "uint64_t": "ulong",
    "bool": "byte",  # Should never appear in a C ABI per ABI_PATTERNS.md §13.
    "_Bool": "byte",
}

# Struct-version fields are reported as "int" by libclang but the header
# explicitly types them ``uint32_t``. Same for ``extended`` on ``occtl_error``.
FIELD_TYPE_OVERRIDES: "dict[tuple[str, str], str]" = {
    # (struct name, field name) → managed type
    ("occtl_uid", "bits"): "ulong",
    ("occtl_uid_t", "bits"): "ulong",
    ("occtl_node_id", "bits"): "ulong",
    ("occtl_node_id_t", "bits"): "ulong",
    ("occtl_ref_id", "bits"): "ulong",
    ("occtl_ref_id_t", "bits"): "ulong",
    ("occtl_ref_uid", "bits"): "ulong",
    ("occtl_ref_uid_t", "bits"): "ulong",
    ("occtl_rep_id", "bits"): "ulong",
    ("occtl_rep_id_t", "bits"): "ulong",
    # ``extended`` is uint32_t in the header. (libclang flattens to int.)
    ("occtl_error", "extended"): "uint",
    ("occtl_error_t", "extended"): "uint",
}

# All known opaque handles. Surfaced as ``IntPtr`` in P/Invoke signatures.
OPAQUE_HANDLES: "set[str]" = {
    "occtl_curve_t",
    "occtl_curve_t *",
    "occtl_curve2d_t",
    "occtl_curve2d_t *",
    "occtl_surface_t",
    "occtl_surface_t *",
    "occtl_graph_t",
    "occtl_graph_t *",
    "occtl_node_iter_t",
    "occtl_node_iter_t *",
    "occtl_topo_explorer_iter_t",
    "occtl_topo_explorer_iter_t *",
    "occtl_topo_related_iter_t",
    "occtl_topo_related_iter_t *",
    "occtl_topo_axis_hit_iter_t",
    "occtl_topo_axis_hit_iter_t *",
    "occtl_topo_touch_iter_t",
    "occtl_topo_touch_iter_t *",
    "occtl_topo_intersection_iter_t",
    "occtl_topo_intersection_iter_t *",
    "occtl_select_iter_t",
    "occtl_select_iter_t *",
    "occtl_select_group_iter_t",
    "occtl_select_group_iter_t *",
    "occtl_batch_t",
    "occtl_batch_t *",
    "occtl_viz_driver_t",
    "occtl_viz_driver_t *",
    "occtl_viz_viewer_t",
    "occtl_viz_viewer_t *",
    "occtl_viz_view_t",
    "occtl_viz_view_t *",
    "occtl_viz_presentable_t",
    "occtl_viz_presentable_t *",
}

# Enums whose values may exceed int32 range. (None today, but the sentinel
# 0x7fffffff is fine.) We always emit ``: int``.
HEADER_TO_MODULE = {
    "occtl_core.h": "Core",
    "occtl_geom.h": "Geom",
    "occtl_curves_common.h": "Geom",
    "occtl_curves.h": "Geom",
    "occtl_curves2d.h": "Geom",
    "occtl_surfaces.h": "Geom",
    "occtl_topo.h": "Topo",
    "occtl_topo_algo.h": "Topo",
    "occtl_topo_build.h": "Topo",
    "occtl_topo_relation.h": "Topo",
    "occtl_topo_types.h": "Topo",
    "occtl_prim_feature.h": "Prim",
    "occtl_prim_sketch.h": "Prim",
    "occtl_prim_solid.h": "Prim",
    "occtl_prim_sweep.h": "Prim",
    "occtl_prim.h": "Prim",
    "occtl_text.h": "Text",
    "occtl_bool.h": "Bool",
    "occtl_mesh.h": "Mesh",
    "occtl_io_brep.h": "IoBrep",
    "occtl_io_step.h": "IoStep",
    "occtl_io_iges.h": "IoIges",
    "occtl_io_stl.h": "IoStl",
    "occtl_io_obj.h": "IoObj",
    "occtl_io_gltf.h": "IoGltf",
    "occtl_io_vrml.h": "IoVrml",
    "occtl_io_ply.h": "IoPly",
    "occtl_viz.h": "Viz",
    "occtl_de.h": "De",
    "occtl_heal.h": "Heal",
    "occtl.h": "Core",  # umbrella; rarely used directly
}


def validate_known_headers(data: dict) -> None:
    headers = set(data.get("headers") or ())
    headers.update(f.get("header", "") for f in data.get("functions", ()))
    unknown = sorted(h for h in headers if h and h not in HEADER_TO_MODULE)
    if unknown:
        raise SystemExit(
            "generate_facade (csharp): abi.json contains unmapped headers: "
            + ", ".join(unknown)
        )

COPYRIGHT_HEADER = """// Copyright (c) 2026 Capgemini Engineering Research and Development.
//
// This file is part of OCCT-Light software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License version 3 as published
// by the Free Software Foundation, with an option to use any later version.
// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
// for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of a commercial
// license or contractual agreement.
//
// SPDX-License-Identifier: AGPL-3.0-or-later
//"""
GENERATED_HEADER = "// <auto-generated>This file was emitted by bindings/csharp/tools/generate_facade.py — do not edit.</auto-generated>"

PRIM_TYPED_NAMES = {
    "box": "PrimBoxInfo",
    "sphere": "PrimSphereInfo",
    "cylinder": "PrimCylinderInfo",
    "cone": "PrimConeInfo",
    "torus": "PrimTorusInfo",
    "wedge": "PrimWedgeInfo",
}

PRIM_REQUIRED_FIELDS = {
    "box": ("dx", "dy", "dz"),
    "sphere": ("radius",),
    "cylinder": ("radius", "height"),
    "cone": ("r1", "r2", "height"),
    "torus": ("r1", "r2"),
    "wedge": ("dx", "dy", "dz", "ltx"),
}

PRIM_METHOD_ORDER = ("box", "sphere", "cylinder", "cone", "torus", "wedge")


# ---------------------------------------------------------------------------
# Naming helpers
# ---------------------------------------------------------------------------

# C → C# name conversion. ``occtl_prim_make_box`` → ``MakeBox`` when the
# caller already lives inside the ``Prim`` class; ``OcctlPrimMakeBox`` if
# we want a fully qualified name. The generator emits the un-prefixed form
# inside per-module partial classes.

_OCCT_PREFIX_RE = re.compile(r"^occtl_")
_TYPE_SUFFIX_RE = re.compile(r"_t$")


def pascal_case(snake: str) -> str:
    """``occtl_topo_make_vertex_info`` → ``MakeVertexInfo``."""
    parts = re.split(r"_+", snake)
    return "".join(p[:1].upper() + p[1:].lower() for p in parts if p)


def strip_t(name: str) -> str:
    return _TYPE_SUFFIX_RE.sub("", name)


def cs_type_name(c_name: str) -> str:
    """Convert an OCCT-Light C type name to its managed-side equivalent.

    ``occtl_status_t`` → ``OcctlStatus``; ``occtl_uid_t`` → ``OcctlUid``;
    ``occtl_topo_make_vertex_info_t`` → ``OcctlTopoMakeVertexInfo``. Primitive C scalars
    (``int``, ``double``, ``uint32_t``, …) map through :data:`PRIM_MAP`.
    """
    bare = c_name.strip()
    # Strip leading/trailing ``const`` and whitespace runs.
    bare = re.sub(r"\bconst\b", "", bare).strip()
    bare = re.sub(r"\s+", " ", bare)
    if bare in PRIM_MAP:
        return PRIM_MAP[bare]
    base = strip_t(bare)
    return pascal_case(base)


def cs_func_name(c_name: str, module: str) -> str:
    """Convert ``occtl_prim_make_box`` (module ``Prim``) → ``MakeBox``.

    Strips the module prefix when present; keeps the rest in PascalCase.
    ``occtl_graph_solid_count`` (Topo) → ``GraphSolidCount`` (since "graph"
    isn't the module name in our taxonomy).
    """
    s = _OCCT_PREFIX_RE.sub("", c_name)
    parts = s.split("_")
    if parts and parts[0].lower() == module.lower():
        parts = parts[1:]
    return pascal_case("_".join(parts)) or pascal_case(c_name)


def cs_param_name(c_name: str) -> str:
    """Reserved-keyword-safe camelCase ``out_graph`` → ``@out_graph``."""
    # camelCase: lowercase first part, PascalCase rest.
    parts = c_name.split("_")
    if not parts:
        return c_name
    first = parts[0].lower()
    rest = "".join(p[:1].upper() + p[1:].lower() for p in parts[1:])
    name = first + rest
    if name in CS_RESERVED:
        name = "@" + name
    return name


CS_RESERVED = {
    "abstract", "as", "base", "bool", "break", "byte", "case", "catch",
    "char", "checked", "class", "const", "continue", "decimal", "default",
    "delegate", "do", "double", "else", "enum", "event", "explicit",
    "extern", "false", "finally", "fixed", "float", "for", "foreach",
    "goto", "if", "implicit", "in", "int", "interface", "internal", "is",
    "lock", "long", "namespace", "new", "null", "object", "operator",
    "out", "override", "params", "private", "protected", "public",
    "readonly", "ref", "return", "sbyte", "sealed", "short", "sizeof",
    "stackalloc", "static", "string", "struct", "switch", "this", "throw",
    "true", "try", "typeof", "uint", "ulong", "unchecked", "unsafe",
    "ushort", "using", "virtual", "void", "volatile", "while",
}


# ---------------------------------------------------------------------------
# Type mapping for params + struct fields
# ---------------------------------------------------------------------------

_ARRAY_RE = re.compile(r"^(?P<base>.+?)\[(?P<size>\d+)\]$")


def map_field_type(struct_name: str, field_name: str, c_type: str) -> str:
    """Map a struct field's C type → managed type, used inside Types.g.cs.

    Fixed-size C arrays (``double[12]``) are reported as the array form and
    handled by the caller (it emits a sibling ``[InlineArray]`` struct and
    references it). Here we return the special form ``__inlinearray:N:elem``.
    """
    override = FIELD_TYPE_OVERRIDES.get((struct_name, field_name))
    if override is not None:
        return override
    # All ``struct_version`` fields are declared ``uint32_t`` in the public C
    # headers; libclang resolves them to plain ``int`` once typedefs are folded.
    if field_name == "struct_version" and c_type.strip() == "int":
        return "uint"
    m = _ARRAY_RE.match(c_type.strip())
    if m:
        size = int(m.group("size"))
        elem = map_c_type(m.group("base"), context="field")
        return f"__inlinearray:{size}:{elem}"
    return map_c_type(c_type, context="field")


def map_c_type(c_type: str, *, context: str = "param") -> str:
    """Map a C type spelling → managed type.

    ``context``:
    - ``param`` — use ``IntPtr`` for opaque handles and pointer-to-pointer.
    - ``field`` — same but no marshal-as decorators.
    - ``return`` — same.
    """
    t = c_type.strip()

    # Strip trailing const, leading const.
    t = re.sub(r"\bconst\b", "", t).strip()
    t = re.sub(r"\s+", " ", t)

    # Opaque handle pointers, including ``occtl_graph_t **``.
    if t.endswith("**"):
        base = t[:-2].strip()
        if base in OPAQUE_HANDLES or (base + " *") in OPAQUE_HANDLES:
            return "out IntPtr" if context == "param" else "IntPtr"
        # Out-pointer to struct/POD — caller passes ``out X``.
        inner = map_c_type(base, context="field")
        return f"out {inner}" if context == "param" else "IntPtr"
    if t.endswith("*"):
        base = t[:-1].strip()
        if base in {"char", "const char", "void", "const void"}:
            return "IntPtr"
        if (base + " *") in OPAQUE_HANDLES or base in OPAQUE_HANDLES:
            return "IntPtr"
        # Struct pointer — passed as ``ref`` or ``IntPtr``. We use ``IntPtr``
        # for maximum flexibility; the idiomatic wrappers pin/marshal.
        return "IntPtr"

    # Bare scalar / typedef.
    if t in PRIM_MAP:
        return PRIM_MAP[t]
    if t.endswith("_t") or t.startswith("occtl_"):
        # Treat as a managed enum/struct type by PascalCase conversion.
        # Opaque handle bare (occurs in `out_graph` style) → IntPtr.
        if t in OPAQUE_HANDLES or (t + " *") in OPAQUE_HANDLES:
            return "IntPtr"
        return cs_type_name(t)
    # Fallback: assume blittable struct, PascalCase.
    return pascal_case(t)


# ---------------------------------------------------------------------------
# Emitters
# ---------------------------------------------------------------------------

class Lines:
    def __init__(self) -> None:
        self.buf: "list[str]" = []

    def add(self, s: str = "") -> None:
        self.buf.append(s)

    def write(self, path: Path) -> int:
        path.parent.mkdir(parents=True, exist_ok=True)
        text = "\n".join(self.buf).rstrip() + "\n"
        path.write_text(text, encoding="utf-8")
        return len(self.buf)


def emit_enums(types: list, out: Path) -> int:
    lines = Lines()
    lines.add(COPYRIGHT_HEADER)
    lines.add(GENERATED_HEADER)
    lines.add("#nullable enable")
    lines.add("")
    lines.add("namespace OcctL;")
    lines.add("")

    seen: set[str] = set()
    enums = [t for t in types if t["kind"] == "enum"]
    # Prefer the ``_t``-suffixed copy so the doc/name matches what bindings
    # use. Iterate sorted for determinism.
    enums.sort(key=lambda t: t["name"])
    for t in enums:
        # Pick canonical name (strip _t for the managed enum).
        canonical_c = t["name"]
        managed = cs_type_name(canonical_c)
        if managed in seen:
            continue
        seen.add(managed)

        # Doc comment.
        if t.get("doc"):
            lines.add(f"/// <summary>{xml_escape(t['doc'])}</summary>")
        lines.add(f"public enum {managed} : int")
        lines.add("{")
        for v in t["values"]:
            doc = v.get("doc") or ""
            if doc:
                lines.add(f"    /// <summary>{xml_escape(doc)}</summary>")
            member = pascal_case(_OCCT_PREFIX_RE.sub("", v["name"]).removeprefix("OCCTL_").lower())
            if not member or member[0].isdigit():
                member = "_" + member
            lines.add(f"    {member} = unchecked((int){v['value']}),")
        lines.add("}")
        lines.add("")
    return lines.write(out)


def emit_types(types: list, out: Path) -> int:
    lines = Lines()
    lines.add(COPYRIGHT_HEADER)
    lines.add(GENERATED_HEADER)
    lines.add("#nullable enable")
    lines.add("using System;")
    lines.add("using System.Runtime.InteropServices;")
    lines.add("")
    lines.add("namespace OcctL.Native;")
    lines.add("")

    seen: set[str] = set()
    # Emit value handles as nominal record-structs (single ulong Bits field).
    vhs = sorted([t for t in types if t["kind"] == "value_handle"], key=lambda t: t["name"])
    for t in vhs:
        managed = cs_type_name(t["name"])
        if managed in seen:
            continue
        seen.add(managed)
        if t.get("doc"):
            lines.add(f"/// <summary>{xml_escape(t['doc'])}</summary>")
        lines.add("[StructLayout(LayoutKind.Sequential)]")
        lines.add(f"public readonly record struct {managed}(ulong Bits);")
        lines.add("")

    # Collect inline-array helper types so we can emit them once at the end.
    inline_arrays: dict[tuple[int, str], str] = {}

    def inline_array_name(size: int, elem: str) -> str:
        key = (size, elem)
        if key not in inline_arrays:
            inline_arrays[key] = f"InlineArray{size}{pascal_case(elem)}"
        return inline_arrays[key]

    # Emit value-bearing structs (kind==struct). Omit the ones already emitted
    # as value handles (the bare ``occtl_uid`` struct is the same shape as
    # ``occtl_uid_t`` value-handle — we keep the value-handle form).
    structs = sorted([t for t in types if t["kind"] == "struct"], key=lambda t: t["name"])
    for t in structs:
        managed = cs_type_name(t["name"])
        # If the struct is a value-handle in disguise (single ``bits`` field),
        # the value_handle emitter already wrote a record-struct for it.
        if managed in seen:
            continue
        # Omit ``_t`` doublet if the bare name already emitted.
        if t["name"].endswith("_t"):
            bare = managed  # cs_type_name strips the _t already
            if bare in seen:
                continue
        seen.add(managed)
        if t.get("doc"):
            lines.add(f"/// <summary>{xml_escape(t['doc'])}</summary>")
        lines.add("[StructLayout(LayoutKind.Sequential)]")
        lines.add(f"public struct {managed}")
        lines.add("{")
        for f in t["fields"]:
            ct = map_field_type(t["name"], f["name"], f["type"])
            doc = f.get("doc") or ""
            if doc:
                lines.add(f"    /// <summary>{xml_escape(doc)}</summary>")
            field_name = pascal_case(f["name"])
            if ct.startswith("__inlinearray:"):
                _, size_s, elem = ct.split(":", 2)
                helper = inline_array_name(int(size_s), elem)
                lines.add(f"    public {helper} {field_name};")
            else:
                lines.add(f"    public {ct} {field_name};")
        lines.add("}")
        lines.add("")

    # Aliases: emit as `using` style — emit a record-struct wrapping the
    # underlying type for nominal distinction. Keep minimal.
    aliases = sorted([t for t in types if t["kind"] == "alias"], key=lambda t: t["name"])
    for t in aliases:
        managed = cs_type_name(t["name"])
        if managed in seen:
            continue
        seen.add(managed)
        if "(*" in t["underlying"]:
            underlying = "IntPtr"
        else:
            underlying = cs_type_name(t["underlying"])
        if t.get("doc"):
            lines.add(f"/// <summary>{xml_escape(t['doc'])}</summary>")
        lines.add(f"// Alias of {underlying}; layout-identical.")
        lines.add(f"[StructLayout(LayoutKind.Sequential)]")
        lines.add(f"public struct {managed}")
        lines.add("{")
        lines.add(f"    public {underlying} Value;")
        lines.add("}")
        lines.add("")

    # Emit inline-array helper structs collected during struct emission.
    for (size, elem), name in sorted(inline_arrays.items()):
        lines.add(f"/// <summary>Inline-array view of <c>{elem}[{size}]</c> for blittable interop.</summary>")
        lines.add(f"[System.Runtime.CompilerServices.InlineArray({size})]")
        lines.add(f"public struct {name}")
        lines.add("{")
        lines.add(f"    private {elem} _element0;")
        lines.add("}")
        lines.add("")

    return lines.write(out)


def emit_constants(constants: list, out: Path) -> int:
    lines = Lines()
    lines.add(COPYRIGHT_HEADER)
    lines.add(GENERATED_HEADER)
    lines.add("#nullable enable")
    lines.add("")
    lines.add("namespace OcctL.Native;")
    lines.add("")
    lines.add("public static class OcctlConstants")
    lines.add("{")
    for c in sorted(constants, key=lambda c: c["name"]):
        v = c["value"]
        name = pascal_case(c["name"].removeprefix("OCCTL_").lower())
        if c.get("doc"):
            lines.add(f"    /// <summary>{xml_escape(c['doc'])}</summary>")
        if isinstance(v, bool):
            lines.add(f"    public const bool {name} = {'true' if v else 'false'};")
        elif isinstance(v, int):
            lines.add(f"    public const int {name} = unchecked((int){v});")
        elif isinstance(v, float):
            lines.add(f"    public const double {name} = {v}d;")
        elif isinstance(v, str):
            esc = v.replace("\\", "\\\\").replace("\"", "\\\"")
            lines.add(f"    public const string {name} = \"{esc}\";")
        else:
            lines.add(f"    // Unsupported constant kind for {c['name']}.")
    lines.add("}")
    return lines.write(out)


def emit_library_name(library_name: str, out: Path) -> int:
    lines = Lines()
    lines.add(COPYRIGHT_HEADER)
    lines.add("#nullable enable")
    lines.add("")
    lines.add("namespace OcctL.Native;")
    lines.add("")
    lines.add("/// <summary>")
    lines.add("/// Name of the OCCT-Light native shared library. The runtime resolves this via the")
    lines.add("/// platform's library-load rules (DYLD_LIBRARY_PATH / LD_LIBRARY_PATH / PATH plus")
    lines.add("/// NuGet's runtimes/&lt;rid&gt;/native/ layout when packaged).")
    lines.add("/// </summary>")
    lines.add("internal static class LibraryName")
    lines.add("{")
    lines.add("    /// <summary>")
    lines.add("    /// Base name passed to <see cref=\"System.Runtime.InteropServices.LibraryImportAttribute\"/>.")
    lines.add("    /// </summary>")
    lines.add(f"    internal const string Value = \"{xml_escape(library_name)}\";")
    lines.add("}")
    return lines.write(out)


def emit_pinvoke(functions: list, header: str, out: Path) -> int:
    """One ``Pinvoke.<Module>.g.cs`` per public header."""
    module = HEADER_TO_MODULE[header]
    lines = Lines()
    lines.add(COPYRIGHT_HEADER)
    lines.add(GENERATED_HEADER)
    lines.add("#nullable enable")
    lines.add("using System;")
    lines.add("using System.Runtime.InteropServices;")
    lines.add("")
    lines.add("namespace OcctL.Native;")
    lines.add("")
    lines.add("public static partial class NativeMethods")
    lines.add("{")

    fns = [f for f in functions if f["header"] == header]
    fns.sort(key=lambda f: f["name"])
    for f in fns:
        ret = pinvoke_return_type(f["return_type"])
        params = ", ".join(pinvoke_param(p) for p in f["params"])
        if f.get("doc"):
            lines.add(f"    /// <summary>{xml_escape(f['doc'])}</summary>")
        for p in f["params"]:
            doc = (p.get("doc") or "").replace("\n", " ").strip()
            if doc:
                lines.add(f"    /// <param name=\"{cs_param_name(p['name']).lstrip('@')}\">{xml_escape(doc)}</param>")
        # LibraryImport requires the function be partial.
        lines.add(f"    [LibraryImport(LibraryName.Value, EntryPoint = \"{f['name']}\")]")
        # Set CallingConvention via UnmanagedCallConv (on .NET 8 LibraryImport).
        lines.add("    [UnmanagedCallConv(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]")
        unsafe = " unsafe" if "*" in params else ""
        lines.add(f"    public static{unsafe} partial {ret} {pascal_case(f['name'])}({params});")
        lines.add("")
    lines.add("}")
    return lines.write(out)


def pinvoke_return_type(c_type: str) -> str:
    t = c_type.strip()
    if t == "occtl_status_t":
        return "OcctL.OcctlStatus"
    if t in ("const char *", "const char*"):
        return "IntPtr"
    if t.endswith("*"):
        return "IntPtr"
    return map_c_type(t, context="return")


def pinvoke_param(p: dict) -> str:
    name = cs_param_name(p["name"])
    c_type = p["type"]
    depth = p["pointer_depth"]
    direction = "out" if p["name"].startswith("out_") else p["direction"]
    elem = p.get("elem_type") or ""

    if depth == 0:
        # Pass-by-value POD or enum.
        managed = map_c_type(c_type, context="param")
        if managed.startswith("out "):
            managed = managed[4:]
        return f"{managed} {name}"

    if depth == 1:
        # Decide what kind of pointer this is.
        elem_clean = re.sub(r"\bconst\b", "", elem).strip()
        if elem_clean in {"char"}:
            # Char pointer: UTF-8 string. Marshal as input string by default;
            # use IntPtr for output strings.
            if direction == "out":
                return f"IntPtr {name}"
            return f"[MarshalAs(UnmanagedType.LPUTF8Str)] string? {name}"
        if elem_clean in {"void"}:
            return f"IntPtr {name}"
        if elem_clean in {"uint8_t", "unsigned char"}:
            return f"byte* {name}"
        if elem_clean in OPAQUE_HANDLES or elem_clean + " *" in OPAQUE_HANDLES or elem_clean in {h.rstrip(" *") for h in OPAQUE_HANDLES}:
            # Pointer to opaque handle = handle itself.
            return f"IntPtr {name}"
        # Pointer to POD/struct/value-handle.
        managed = cs_type_name(elem_clean)
        if direction == "out":
            return f"out {managed} {name}"
        if direction == "inout":
            return f"ref {managed} {name}"
        # "in" pointer to value-handle / POD — use ref (no copy) by default,
        # but the doc says NULLable for many — use IntPtr for max flexibility.
        return f"in {managed} {name}"

    # depth >= 2: opaque-handle out-param (``occtl_graph_t **``).
    return f"out IntPtr {name}"


def emit_idiomatic(functions: list, header: str, out: Path) -> int:
    """One ``<Module>.Idiomatic.g.cs`` per header.

    Emits a partial class wrapping each P/Invoke. For functions returning
    ``occtl_status_t`` the wrapper throws on failure via ``Check``. Other
    functions are surfaced as-is.
    """
    module = HEADER_TO_MODULE[header]
    lines = Lines()
    lines.add(COPYRIGHT_HEADER)
    lines.add(GENERATED_HEADER)
    lines.add("#nullable enable")
    lines.add("using System;")
    lines.add("using OcctL.Native;")
    lines.add("")
    lines.add("namespace OcctL;")
    lines.add("")
    lines.add(f"/// <summary>Checked raw facade for native functions in the {module} module.</summary>")
    lines.add(f"public static partial class {module}Raw")
    lines.add("{")

    fns = [f for f in functions if f["header"] == header]
    fns.sort(key=lambda f: f["name"])
    for f in fns:
        method_name = cs_func_name(f["name"], module)
        rt_c = f["return_type"].strip()
        returns_status = (rt_c == "occtl_status_t")

        # Build managed signature.
        managed_params = []
        forward_args = []
        for p in f["params"]:
            sig_part, forward = idiomatic_param(p)
            managed_params.append(sig_part)
            forward_args.append(forward)

        managed_sig_params = ", ".join(managed_params)
        call_args = ", ".join(forward_args)
        pinvoke = f"NativeMethods.{pascal_case(f['name'])}({call_args})"

        if f.get("doc"):
            lines.add(f"    /// <summary>{xml_escape(f['doc'])}</summary>")
        # Param docs.
        for p in f["params"]:
            doc = (p.get("doc") or "").replace("\n", " ").strip()
            if doc:
                lines.add(f"    /// <param name=\"{cs_param_name(p['name']).lstrip('@')}\">{xml_escape(doc)}</param>")
        # Retval docs.
        if f.get("retvals"):
            for rv in f["retvals"]:
                if returns_status:
                    code_member = pascal_case(rv['code'].removeprefix('OCCTL_').lower())
                    lines.add(f"    /// <returns>When <see cref=\"OcctlStatus.{code_member}\"/>: {xml_escape(rv['doc'])}</returns>")
                else:
                    lines.add(f"    /// <returns>When {xml_escape(rv['code'])}: {xml_escape(rv['doc'])}</returns>")
        # Thread-safety note.
        if f.get("threadsafe"):
            lines.add(f"    /// <remarks>Threadsafe: {xml_escape(f['threadsafe'])}</remarks>")
        # See-also references.
        if f.get("see_also"):
            for sa in f["see_also"]:
                lines.add(f"    /// <seealso>{xml_escape(sa)}</seealso>")
        if returns_status:
            unsafe = " unsafe" if "*" in managed_sig_params else ""
            lines.add(f"    public static{unsafe} void {method_name}({managed_sig_params})")
            lines.add("    {")
            lines.add(f"        OcctlException.Check({pinvoke});")
            lines.add("    }")
        else:
            ret = pinvoke_return_type(rt_c)
            unsafe = " unsafe" if "*" in managed_sig_params else ""
            lines.add(f"    public static{unsafe} {ret} {method_name}({managed_sig_params})")
            lines.add("    {")
            if ret == "void":
                lines.add(f"        {pinvoke};")
            else:
                lines.add(f"        return {pinvoke};")
            lines.add("    }")
        lines.add("")
    lines.add("}")
    return lines.write(out)


def idiomatic_param(p: dict) -> "tuple[str, str]":
    """Build ``(signature_part, forward_argument)`` for an idiomatic wrapper."""
    name = cs_param_name(p["name"])
    bare = name.lstrip("@")
    c_type = p["type"]
    depth = p["pointer_depth"]
    direction = "out" if p["name"].startswith("out_") else p["direction"]
    elem = p.get("elem_type") or ""

    if depth == 0:
        managed = map_c_type(c_type, context="param")
        return (f"{managed} {name}", name)

    elem_clean = re.sub(r"\bconst\b", "", elem).strip()
    if depth == 1 and elem_clean == "char":
        if direction == "out":
            return (f"IntPtr {name}", name)
        return (f"string? {name}", name)
    if depth == 1 and elem_clean == "void":
        return (f"IntPtr {name}", name)
    if depth == 1 and elem_clean in {"uint8_t", "unsigned char"}:
        return (f"byte* {name}", name)
    if depth == 1 and (elem_clean in OPAQUE_HANDLES or elem_clean + " *" in OPAQUE_HANDLES):
        return (f"IntPtr {name}", name)
    if depth == 1:
        managed = cs_type_name(elem_clean)
        if direction == "out":
            return (f"out {managed} {name}", f"out {name}")
        if direction == "inout":
            return (f"ref {managed} {name}", f"ref {name}")
        return (f"in {managed} {name}", f"in {name}")

    # depth >= 2: handle out-param.
    return (f"out IntPtr {name}", f"out {name}")


# ---------------------------------------------------------------------------
# Misc
# ---------------------------------------------------------------------------

def xml_escape(s: str) -> str:
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;").replace("\n", " ")


def clean_doc(s: str) -> str:
    s = re.sub(r"@(c|p)\s+", "", s or "")
    return re.sub(r"\s+", " ", s).strip()


def public_prim_name(c_name: str) -> str | None:
    prefix = "occtl_prim_"
    suffix = "_info_t"
    if not c_name.startswith(prefix) or not c_name.endswith(suffix):
        return None
    stem = c_name[len(prefix):-len(suffix)]
    return stem if stem in PRIM_TYPED_NAMES else None


def emit_csharp_typed_facades(data: dict, out: Path) -> int:
    structs = {
        stem: t
        for t in data["types"]
        for stem in [public_prim_name(t.get("name", ""))]
        if stem is not None and t.get("kind") == "struct"
    }

    lines = Lines()
    lines.add(COPYRIGHT_HEADER)
    lines.add(GENERATED_HEADER)
    lines.add("#nullable enable")
    lines.add("using System;")
    lines.add("using OcctL.Native;")
    lines.add("")
    lines.add("namespace OcctL.Options")
    lines.add("{")
    for stem in PRIM_METHOD_ORDER:
        struct = structs.get(stem)
        if not struct:
            continue
        cls = PRIM_TYPED_NAMES[stem]
        native = cs_type_name(struct["name"])
        init_method = pascal_case(f"occtl_prim_{stem}_info_init")
        fields = [
            f for f in struct.get("fields", [])
            if f["name"] not in ("struct_version", "p_next")
        ]
        lines.add(f"    /// <summary>Typed options for <c>{xml_escape(struct['name'])}</c>.</summary>")
        lines.add(f"    public sealed record {cls}")
        lines.add("    {")
        for field in fields:
            name = field["name"]
            prop = pascal_case(name)
            doc = clean_doc(field.get("doc", ""))
            is_required = name in PRIM_REQUIRED_FIELDS[stem]
            if doc:
                lines.add(f"        /// <summary>{xml_escape(doc)}</summary>")
            if name == "placement":
                lines.add(f"        public OcctlAxis2Placement? {prop} {{ get; init; }}")
            elif is_required:
                lines.add(f"        public double {prop} {{ get; init; }}")
            else:
                lines.add(f"        public double? {prop} {{ get; init; }}")
        lines.add("")
        lines.add("        /// <summary>Builds the native versioned options struct for the C ABI call.</summary>")
        lines.add(f"        public {native} ToNative()")
        lines.add("        {")
        lines.add(f"            PrimRaw.{cs_func_name(f'occtl_prim_{stem}_info_init', 'Prim')}(out {native} info);")
        for field in fields:
            name = field["name"]
            prop = pascal_case(name)
            if name == "placement":
                lines.add(f"            if ({prop}.HasValue) info.{prop} = {prop}.Value;")
            elif name in PRIM_REQUIRED_FIELDS[stem]:
                lines.add(f"            info.{prop} = {prop};")
            else:
                lines.add(f"            if ({prop}.HasValue) info.{prop} = {prop}.Value;")
        lines.add("            return info;")
        lines.add("        }")
        lines.add("    }")
        lines.add("")
    lines.add("}")
    lines.add("")
    lines.add("namespace OcctL")
    lines.add("{")
    lines.add("    public static partial class Prim")
    lines.add("    {")
    for stem in PRIM_METHOD_ORDER:
        struct = structs.get(stem)
        if not struct:
            continue
        cls = PRIM_TYPED_NAMES[stem]
        method = pascal_case(f"make_{stem}")
        native = cs_type_name(struct["name"])
        out_name = "outSolid"
        fields = [
            f for f in struct.get("fields", [])
            if f["name"] not in ("struct_version", "p_next", "placement")
        ]
        required = set(PRIM_REQUIRED_FIELDS[stem])
        simple_fields = [f for f in fields if f["name"] in required]
        lines.add(f"        /// <summary>Builds a {stem} and returns the NodeId of the new topology root.</summary>")
        lines.add(f"        public static NodeId {method}(Graph graph, OcctL.Options.{cls} info)")
        lines.add("        {")
        lines.add("            if (graph is null) throw new ArgumentNullException(nameof(graph));")
        lines.add("            if (info is null) throw new ArgumentNullException(nameof(info));")
        lines.add(f"            {native} native = info.ToNative();")
        lines.add(f"            PrimRaw.{method}(graph.Handle.DangerousGetHandle_(), in native, out OcctlNodeId {out_name});")
        lines.add(f"            return new NodeId({out_name}.Bits);")
        lines.add("        }")
        lines.add("")
        if simple_fields:
            args = ", ".join(f"double {cs_param_name(f['name'])}" for f in simple_fields)
            init = ", ".join(f"{pascal_case(f['name'])} = {cs_param_name(f['name'])}" for f in simple_fields)
            lines.add(f"        /// <summary>Builds a {stem} with default placement/options.</summary>")
            lines.add(f"        public static NodeId {method}(Graph graph, {args})")
            lines.add("        {")
            lines.add(f"            return {method}(graph, new OcctL.Options.{cls} {{ {init} }});")
            lines.add("        }")
            lines.add("")
    lines.add("    }")
    lines.add("")
    lines.add("    /// <summary>Primitive-shape convenience methods for <see cref=\"Graph\"/>.</summary>")
    lines.add("    public static class PrimGraphExtensions")
    lines.add("    {")
    for stem in PRIM_METHOD_ORDER:
        struct = structs.get(stem)
        if not struct:
            continue
        cls = PRIM_TYPED_NAMES[stem]
        method = pascal_case(f"make_{stem}")
        fields = [
            f for f in struct.get("fields", [])
            if f["name"] not in ("struct_version", "p_next", "placement")
        ]
        required = set(PRIM_REQUIRED_FIELDS[stem])
        simple_fields = [f for f in fields if f["name"] in required]
        lines.add(f"        /// <summary>Builds a {stem} in this graph and returns the new topology root.</summary>")
        lines.add(f"        public static NodeId {method}(this Graph graph, OcctL.Options.{cls} info)")
        lines.add("        {")
        lines.add(f"            return Prim.{method}(graph, info);")
        lines.add("        }")
        lines.add("")
        if simple_fields:
            args = ", ".join(f"double {cs_param_name(f['name'])}" for f in simple_fields)
            params_call = ", ".join(cs_param_name(f["name"]) for f in simple_fields)
            lines.add(f"        /// <summary>Builds a {stem} in this graph with default placement/options.</summary>")
            lines.add(f"        public static NodeId {method}(this Graph graph, {args})")
            lines.add("        {")
            lines.add(f"            return Prim.{method}(graph, {params_call});")
            lines.add("        }")
            lines.add("")
    lines.add("    }")
    lines.add("}")
    return lines.write(out)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main(argv: list[str]) -> int:
    p = argparse.ArgumentParser(description="Generate C# facade .g.cs files from abi.json.")
    p.add_argument("--abi", required=True, type=Path, help="Path to build/abi.json")
    p.add_argument("--root", default=Path(__file__).resolve().parent.parent, type=Path,
                   help="Path to bindings/csharp (defaults to script's grandparent dir).")
    p.add_argument("--library-name", default="occtl-full",
                   help="Native feature-set library basename used by LibraryImport.")
    args = p.parse_args(argv)

    data = json.loads(args.abi.read_text(encoding="utf-8"))

    # Shared schema check — see tools/abi_schema.py. Fails loud if the
    # producer drifted away from the shape this generator expects.
    repo_root = Path(__file__).resolve().parents[3]  # tools -> csharp -> bindings -> repo
    sys.path.insert(0, str(repo_root / "tools"))
    from abi_schema import SCHEMA_VERSION, SchemaMismatch, validate
    try:
        validate(data, expected_schema_version=SCHEMA_VERSION)
    except SchemaMismatch as e:
        raise SystemExit(f"generate_facade (csharp): abi.json schema mismatch: {e}")
    validate_known_headers(data)

    native_gen = args.root / "src" / "ffi" / "Generated"
    occtl_gen = args.root / "src" / "api" / "_Generated"
    for gen_dir in (native_gen, occtl_gen):
        if gen_dir.is_dir():
            for old in gen_dir.glob("*.g.cs"):
                old.unlink()

    written: dict[str, int] = {}

    written["Enums.g.cs"] = emit_enums(data["types"], native_gen / "Enums.g.cs")
    written["Types.g.cs"] = emit_types(data["types"], native_gen / "Types.g.cs")
    written["Constants.g.cs"] = emit_constants(data["constants"], native_gen / "Constants.g.cs")
    written["LibraryName.cs"] = emit_library_name(args.library_name, args.root / "src" / "ffi" / "LibraryName.cs")

    for header in sorted(set(f["header"] for f in data["functions"])):
        module = HEADER_TO_MODULE[header]
        # All headers in a module share one P/Invoke file. We emit once per
        # header for clarity; the partial-class trick lets them coexist.
        # Use header basename without `occtl_` prefix and `.h` suffix.
        slug = header.removeprefix("occtl_").removesuffix(".h").removesuffix(".") or module.lower()
        if slug == "":
            slug = module.lower()
        pi_file = native_gen / f"Pinvoke.{pascal_case(slug)}.g.cs"
        id_file = occtl_gen / f"{module}.{pascal_case(slug)}.Idiomatic.g.cs"
        written[pi_file.name] = emit_pinvoke(data["functions"], header, pi_file)
        written[id_file.name] = emit_idiomatic(data["functions"], header, id_file)

    written["Prim.Typed.g.cs"] = emit_csharp_typed_facades(data, occtl_gen / "Prim.Typed.g.cs")

    total = sum(written.values())
    print(f"Generated {len(written)} files, {total} lines.")
    for name, n in sorted(written.items()):
        print(f"  {name}: {n} lines")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
