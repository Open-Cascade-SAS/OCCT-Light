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
OCCT-Light ABI dumper.

Walks ``include/occtl/*.h`` via libclang and emits a single JSON document that
the per-binding raw-layer generators (Python cffi, C# Roslyn, Node N-API,
WASM Embind) consume. The schema is the binding contract; see
``docs/design/BINDINGS.md``.

Output JSON shape::

    {
      "abi_version": 1,
      "library_version": {"major": 0, "minor": 1, "patch": 0},
      "headers": ["occtl_core.h", ...],
      "functions": [{...}],
      "types":     [{...}],
      "constants": [{...}]
    }

Idempotency: same input headers produce identical JSON (sort_keys=True,
trailing newline, stable source order within each list).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import logging
import re
import subprocess
import sys
from pathlib import Path

_LOGGER = logging.getLogger("abi_dump")

# clang is imported lazily in main() so --help / --version work without it.
cindex = None  # type: ignore[assignment]


def _import_cindex() -> None:
    """Import clang.cindex; exits gracefully on failure."""
    global cindex
    try:
        from clang import cindex as _cix
        cindex = _cix
    except ImportError as exc:
        _LOGGER.error(
            "clang.cindex is required.\n"
            "Install with `pip install libclang` (the PyPI package bundles the native\n"
            "libclang dynamic library so no system libclang is required).\n"
            "Underlying ImportError: %s", exc
        )
        raise SystemExit(2)

# ----------------------------------------------------------------------------
# Preset / module mapping
# ----------------------------------------------------------------------------

# Headers that ship in every build.
ALWAYS_HEADERS = ("occtl.h", "occtl_core.h")
PRIM_HEADERS = (
    "occtl_prim_feature.h",
    "occtl_prim_sketch.h",
    "occtl_prim_solid.h",
    "occtl_prim_sweep.h",
    "occtl_prim.h",
)

# Preset → set of header basenames that participate. Keep in sync with the
# OCCTL_BUILD_<MODULE> options in cmake/OCCTLOptions.cmake.
PRESET_HEADERS: "dict[str, tuple[str, ...]]" = {
    "core-only": ALWAYS_HEADERS,
    "geom-only": ALWAYS_HEADERS
    + ("occtl_geom.h", "occtl_curves.h", "occtl_curves2d.h", "occtl_curves_common.h", "occtl_surfaces.h"),
    "minimal": ALWAYS_HEADERS
    + (
        "occtl_geom.h",
        "occtl_curves.h",
        "occtl_curves2d.h",
        "occtl_curves_common.h",
        "occtl_surfaces.h",
        "occtl_topo_types.h",
        "occtl_topo.h",
        "occtl_topo_build.h",
        "occtl_topo_relation.h",
        "occtl_topo_algo.h",
        *PRIM_HEADERS,
    ),
    "cad": ALWAYS_HEADERS
    + (
        "occtl_geom.h",
        "occtl_curves.h",
        "occtl_curves2d.h",
        "occtl_curves_common.h",
        "occtl_surfaces.h",
        "occtl_topo_types.h",
        "occtl_topo.h",
        "occtl_topo_build.h",
        "occtl_topo_relation.h",
        *PRIM_HEADERS,
        "occtl_text.h",
        "occtl_bool.h",
        "occtl_mesh.h",
        "occtl_heal.h",
        "occtl_de.h",
        "occtl_io_brep.h",
        "occtl_io_step.h",
        "occtl_io_stl.h",
        "occtl_topo_algo.h",
    ),
    "full": ALWAYS_HEADERS
    + (
        "occtl_geom.h",
        "occtl_curves.h",
        "occtl_curves2d.h",
        "occtl_curves_common.h",
        "occtl_surfaces.h",
        "occtl_topo_types.h",
        "occtl_topo.h",
        "occtl_topo_build.h",
        "occtl_topo_relation.h",
        *PRIM_HEADERS,
        "occtl_text.h",
        "occtl_bool.h",
        "occtl_mesh.h",
        "occtl_heal.h",
        "occtl_de.h",
        "occtl_io_brep.h",
        "occtl_io_step.h",
        "occtl_io_iges.h",
        "occtl_io_stl.h",
        "occtl_io_obj.h",
        "occtl_io_gltf.h",
        "occtl_io_vrml.h",
        "occtl_io_ply.h",
        "occtl_topo_algo.h",
    ),
    "full-with-viz": ALWAYS_HEADERS
    + (
        "occtl_geom.h",
        "occtl_curves.h",
        "occtl_curves2d.h",
        "occtl_curves_common.h",
        "occtl_surfaces.h",
        "occtl_topo_types.h",
        "occtl_topo.h",
        "occtl_topo_build.h",
        "occtl_topo_relation.h",
        *PRIM_HEADERS,
        "occtl_text.h",
        "occtl_bool.h",
        "occtl_mesh.h",
        "occtl_heal.h",
        "occtl_de.h",
        "occtl_io_brep.h",
        "occtl_io_step.h",
        "occtl_io_iges.h",
        "occtl_io_stl.h",
        "occtl_io_obj.h",
        "occtl_io_gltf.h",
        "occtl_io_vrml.h",
        "occtl_io_ply.h",
        "occtl_viz.h",
        "occtl_topo_algo.h",
    ),
}

# ----------------------------------------------------------------------------
# Doxygen extraction
# ----------------------------------------------------------------------------

# `@param[in] name  …`, `@param[out] name  …`, `@param[in,out] name  …`,
# `@param name  …` (untagged → defaults to "in").
_PARAM_RE = re.compile(
    r"@param(?:\[(?P<dir>[a-z,\s]+)\])?\s+(?P<name>\w+)\b(?P<rest>.*?)(?=(?:\n\s*[\\@]|\Z))",
    re.DOTALL,
)
_RETVAL_RE = re.compile(
    r"@retval\s+(?P<code>\w+)\s+(?P<doc>.*?)(?=(?:\n\s*[\\@]|\Z))",
    re.DOTALL,
)
_RETURN_RE = re.compile(
    r"@return\s+(?P<doc>.*?)(?=(?:\n\s*[\\@]|\Z))",
    re.DOTALL,
)
_THREADSAFE_RE = re.compile(
    r"@threadsafe\s+(?P<doc>.*?)(?=(?:\n\s*[\\@]|\Z))",
    re.DOTALL,
)
_SEE_ALSO_RE = re.compile(
    r"@sa\s+(?P<doc>.*?)(?=(?:\n\s*[\\@]|\Z))",
    re.DOTALL,
)
_COPYDOC_RE = re.compile(r"@copy(?:doc|details)\s+(?P<name>\w+)")

# First `/** … */` block in a header — the file-level `@file @brief` Doxygen.
_FILE_DOC_RE = re.compile(
    r"/\*\*[\s\S]*?\*/",
    re.DOTALL,
)


def _extract_file_doc(header: Path) -> str:
    """Return the stripped text of the first ``/** … */`` block in *header*."""
    try:
        raw = header.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""
    m = _FILE_DOC_RE.search(raw)
    return _strip_comment(m.group(0)) if m else ""


def _strip_comment(raw: str) -> str:
    """Strip leading ``/**``, trailing ``*/``, leading ``*`` per line, and
    the Doxygen trailing-comment ``<`` marker (``/**< */`` style)."""
    if raw is None:
        return ""
    text = raw.strip()
    if text.startswith("/**"):
        text = text[3:]
    elif text.startswith("/*"):
        text = text[2:]
    # Handle trailing-comment marker: `/**< field desc */`
    if text.startswith("<"):
        text = text[1:]
        if text.startswith(" "):
            text = text[1:]
    if text.endswith("*/"):
        text = text[:-2]
    out_lines = []
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("*"):
            s = s[1:]
            if s.startswith(" "):
                s = s[1:]
        out_lines.append(s)
    # Collapse internal multi-space runs but preserve paragraph structure.
    return "\n".join(out_lines).strip()


def _flatten(text: str) -> str:
    """Collapse a doc paragraph into a single whitespace-normalised line."""
    return re.sub(r"\s+", " ", text or "").strip()


def _summary(text: str) -> str:
    """First paragraph (up to first blank line, or first '@'/'\\'-tagged line)."""
    if not text:
        return ""
    out = []
    for line in text.splitlines():
        if line.lstrip().startswith(("@", "\\")):
            break
        if not line.strip():
            if out:
                break
            continue
        out.append(line.strip())
    return _flatten(" ".join(out))


def _parse_direction(raw: str | None) -> str:
    if not raw:
        return "in"
    parts = [p.strip().lower() for p in raw.split(",")]
    has_in = "in" in parts
    has_out = "out" in parts
    if has_in and has_out:
        return "inout"
    if has_out:
        return "out"
    if "inout" in parts:
        return "inout"
    return "in"


def _parse_ownership(rest: str, is_pointer: bool) -> str | None:
    """``Owns it``/``Borrows it`` markers in the @param block."""
    lower = rest.lower()
    if "owns it" in lower:
        return "owns"
    if "borrows it" in lower:
        return "borrows"
    if is_pointer:
        return "borrows"
    return None


def _parse_params_doc(comment_text: str) -> "dict[str, dict]":
    """Returns ``{param_name: {"direction": ..., "ownership": ..., "doc": ...}}``."""
    out: "dict[str, dict]" = {}
    for m in _PARAM_RE.finditer(comment_text):
        out[m.group("name")] = {
            "direction": _parse_direction(m.group("dir")),
            "raw_rest": m.group("rest"),
            "doc": _flatten(m.group("rest")),
        }
    return out


def _parse_retvals(comment_text: str) -> "list[dict]":
    out: "list[dict]" = []
    for m in _RETVAL_RE.finditer(comment_text):
        out.append(
            {"code": m.group("code"), "doc": _flatten(m.group("doc"))}
        )
    return out


def _parse_threadsafe(comment_text: str) -> str | None:
    m = _THREADSAFE_RE.search(comment_text)
    if not m:
        return None
    return _flatten(m.group("doc"))


def _parse_see_also(comment_text: str) -> "list[str]":
    items: "list[str]" = []
    for m in _SEE_ALSO_RE.finditer(comment_text):
        body = m.group("doc")
        for token in re.split(r"[,\s]+", body):
            t = token.strip().strip(".")
            if t:
                items.append(t)
    return items


def _parse_return_doc(comment_text: str) -> str | None:
    m = _RETURN_RE.search(comment_text)
    if not m:
        return None
    return _flatten(m.group("doc"))


def _parse_copydoc(comment_text: str) -> str | None:
    m = _COPYDOC_RE.search(comment_text)
    return m.group("name") if m else None


# ----------------------------------------------------------------------------
# Type helpers
# ----------------------------------------------------------------------------


def _type_pointer_depth(t: "cindex.Type") -> int:
    depth = 0
    cur = t
    while cur.kind == cindex.TypeKind.POINTER:
        depth += 1
        cur = cur.get_pointee()
    return depth


def _type_elem(t: "cindex.Type") -> str:
    cur = t
    while cur.kind == cindex.TypeKind.POINTER:
        cur = cur.get_pointee()
    return _type_name(cur)


def _type_name(t: "cindex.Type") -> str:
    # `Type.spelling` gives a canonical-ish C-friendly rendering for our uses.
    return t.spelling.strip()


def _system_include_args() -> "list[str]":
    """Return platform system include arguments for libclang header parsing."""
    if sys.platform != "darwin":
        return []
    try:
        sdk_path = subprocess.check_output(
            ["xcrun", "--sdk", "macosx", "--show-sdk-path"],
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return []
    if not sdk_path:
        return []
    return [f"-isysroot{sdk_path}", f"-I{sdk_path}/usr/include"]


# ----------------------------------------------------------------------------
# Walker
# ----------------------------------------------------------------------------

# Bumped when the dumper's output shape changes (key renames, new top-level
# sections, type-model evolution).  Every binding generator must assert
# against this value; a mismatch means the generator needs updating.
SCHEMA_VERSION = 1


class AbiWalker:
    """Walks the public C ABI surface via libclang and emits a JSON catalog.

    The walker opens one translation unit per header file, extracts every
    ``occtl_*`` function / ``OCCTL_*`` macro / public typedef / struct / enum,
    parses the Doxygen comment for parameter ownership, return-value
    contracts, thread-safety annotations, and ``@sa`` cross-references, then
    calls :meth:`_emit` to assemble the final ``dict``.

    Parameters
    ----------
    include_dir : Path
        Resolved path to ``include/occtl/`` — the ``-I`` root passed to
        libclang.
    headers : list[Path]
        Resolved paths to the ``.h`` files to walk. Only declarations whose
        source location matches one of these headers are kept.

    Attributes
    ----------
    functions : list[dict]
        Accumulated function records.  Each is a dict with ``name``,
        ``header``, ``return_type``, ``params`` (list of param dicts with
        ``name``, ``type``, ``direction``, ``ownership``, ``pointer_depth``,
        ``elem_type``, ``doc``), ``retvals``, ``threadsafe``, ``see_also``,
        and ``doc`` (the summary line).
    types : list[dict]
        Accumulated type records — structs, enums, opaque_handles,
        value_handles, and aliases.  Each carries at minimum ``kind``,
        ``name``, ``header``, and ``doc``.
    constants : list[dict]
        Accumulated ``OCCTL_*`` macro definitions with ``name``, ``value``,
        ``header``, and ``doc``.
    """
    def __init__(self, include_dir: Path, headers: "list[Path]") -> None:
        self.include_dir = include_dir.resolve()
        self.headers = [h.resolve() for h in headers]
        self.functions: "list[dict]" = []
        self.types: "list[dict]" = []
        self.constants: "list[dict]" = []
        self.header_docs: "dict[str, str]" = {}
        self._seen_function: "set[str]" = set()
        self._seen_type: "set[str]" = set()
        self._seen_constant: "set[str]" = set()

    def run(self) -> dict:
        """Walk every header and return the assembled ABI catalog dict.

        Creates one clang ``Index`` and re-uses it across headers.  Each
        header becomes a single translation unit with ``-x c -std=c11`` and
        ``OCCTL_API= / OCCTL_CALL=`` pre-defines so the ABI attributes don't
        confuse the parser.
        """
        index = cindex.Index.create()
        # One TU per header so cursor locations are easy to filter.
        for header in self.headers:
            self.header_docs[header.name] = _extract_file_doc(header)
            args = [
                "-x", "c",
                "-std=c11",
                f"-I{self.include_dir}",
                # Force OCCTL_API to expand to nothing so libclang treats
                # tagged declarations like normal extern functions.
                "-DOCCTL_API=",
                "-DOCCTL_CALL=",
                "-DOCCTL_STATIC_BUILD=1",
            ]
            args.extend(_system_include_args())
            tu = index.parse(
                str(header),
                args=args,
                options=(
                    cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
                ),
            )
            self._walk_tu(tu, header)
        self._resolve_copydocs()
        return self._emit()

    def _is_local(self, loc, header: Path) -> bool:
        if loc is None or loc.file is None:
            return False
        try:
            return Path(loc.file.name).resolve() == header
        except OSError:
            return False

    def _walk_tu(self, tu: "cindex.TranslationUnit", header: Path) -> None:
        # Pass 1: declarations.
        for cursor in tu.cursor.walk_preorder():
            if not self._is_local(cursor.location, header):
                continue
            kind = cursor.kind
            if kind == cindex.CursorKind.FUNCTION_DECL:
                self._visit_function(cursor, header)
            elif kind == cindex.CursorKind.STRUCT_DECL:
                self._visit_struct(cursor, header)
            elif kind == cindex.CursorKind.ENUM_DECL:
                self._visit_enum(cursor, header)
            elif kind == cindex.CursorKind.TYPEDEF_DECL:
                self._visit_typedef(cursor, header)
            elif kind == cindex.CursorKind.MACRO_DEFINITION:
                self._visit_macro(cursor, header, tu)

    # ---------------- functions ---------------- #

    def _visit_function(self, cursor: "cindex.Cursor", header: Path) -> None:
        """Extract a single ``occtl_*`` function declaration.

        Filters by the ``occtl_`` naming convention, ignores ``static inline``
        definitions (which are internal helpers, not ABI), parses the Doxygen
        block attached to the cursor for ``@param`` ownership/direction,
        ``@retval`` contracts, ``@threadsafe`` annotation, and ``@sa``
        cross-references, then appends the record to :attr:`functions`.
        """
        name = cursor.spelling
        if not name or name in self._seen_function:
            return
        # Public surface filter: every public function starts with `occtl_`.
        if not name.startswith("occtl_"):
            return
        # Ignore definitions of static inline helpers in headers (cursor.is_definition()
        # is True and storage class STATIC). Public ABI is declarations only.
        if (
            cursor.is_definition()
            and cursor.storage_class == cindex.StorageClass.STATIC
        ):
            return
        comment_raw = cursor.raw_comment or ""
        comment = _strip_comment(comment_raw)
        params_doc = _parse_params_doc(comment)

        params: "list[dict]" = []
        for arg in cursor.get_arguments():
            t = arg.type
            ptr_depth = _type_pointer_depth(t)
            is_ptr = ptr_depth > 0
            doc_entry = params_doc.get(arg.spelling, {})
            params.append(
                {
                    "name": arg.spelling,
                    "type": _type_name(t),
                    "direction": doc_entry.get("direction", "in"),
                    "ownership": _parse_ownership(
                        doc_entry.get("raw_rest", ""), is_ptr
                    ),
                    "pointer_depth": ptr_depth,
                    "elem_type": _type_elem(t) if is_ptr else None,
                    "doc": doc_entry.get("doc", ""),
                }
            )

        for param in params:
            if param["doc"]:
                continue
            param_name_re = re.compile(rf"\b{re.escape(param['name'])}\b")
            for source_param in params:
                if source_param is param:
                    continue
                if param_name_re.search(source_param["doc"]):
                    param["doc"] = source_param["doc"]
                    param["direction"] = source_param["direction"]
                    param["ownership"] = source_param["ownership"]
                    break

        entry = {
            "name": name,
            "header": header.name,
            "return_type": _type_name(cursor.result_type),
            "calling_convention": "OCCTL_CALL",
            "params": params,
            "retvals": _parse_retvals(comment),
            "return_doc": _parse_return_doc(comment),
            "threadsafe": _parse_threadsafe(comment),
            "see_also": _parse_see_also(comment),
            "doc": _summary(comment),
        }
        copydoc = _parse_copydoc(comment)
        if copydoc:
            entry["_copydoc"] = copydoc
        self.functions.append(entry)
        self._seen_function.add(name)

    def _resolve_copydocs(self) -> None:
        by_name = {f["name"]: f for f in self.functions}
        for entry in self.functions:
            source_name = entry.pop("_copydoc", None)
            if not source_name:
                continue
            source = by_name.get(source_name)
            if source is None:
                continue
            for key in ("doc", "return_doc", "threadsafe"):
                if not entry.get(key):
                    entry[key] = source.get(key)
            for key in ("retvals", "see_also"):
                if not entry.get(key):
                    entry[key] = list(source.get(key) or [])
            source_params = {p["name"]: p for p in source.get("params", [])}
            if len(source_params) == len(entry.get("params", [])):
                source_values = list(source_params.values())
                for idx, param in enumerate(entry["params"]):
                    source_param = source_params.get(param["name"])
                    if source_param is None and idx < len(source_values):
                        source_param = source_values[idx]
                    if source_param is None:
                        continue
                    for key in ("doc", "direction", "ownership"):
                        if not param.get(key):
                            param[key] = source_param.get(key)

    # ---------------- types ---------------- #

    def _visit_struct(self, cursor: "cindex.Cursor", header: Path) -> None:
        # Some `typedef struct foo { ... } foo_t;` produces a STRUCT_DECL we
        # also want to capture under its typedef name. Handle the un-typedef'd
        # named-struct case here; the typedef-only case is handled below.
        if not cursor.is_definition():
            return
        name = cursor.spelling or ""
        if not name:
            return
        fields = self._collect_fields(cursor)
        if not fields:
            return  # not a struct definition we care about
        # Stable key: prefer the struct tag; the typedef visitor de-dupes.
        if name in self._seen_type:
            return
        entry = self._build_struct_entry(cursor, fields, name, header.name)
        self.types.append(entry)
        self._seen_type.add(name)

    def _collect_fields(self, cursor: "cindex.Cursor") -> "list[dict]":
        fields: "list[dict]" = []
        for child in cursor.get_children():
            if child.kind != cindex.CursorKind.FIELD_DECL:
                continue
            field = {
                "name": child.spelling,
                "type": _type_name(child.type),
                "doc": _strip_comment(child.raw_comment or ""),
            }
            try:
                field["size"] = child.type.get_size()
            except Exception:
                pass
            try:
                field["align"] = child.type.get_align()
            except Exception:
                pass
            fields.append(field)
        return fields

    def _build_struct_entry(
        self,
        cursor: "cindex.Cursor",
        fields: "list[dict]",
        name: str,
        header_name: str,
        doc_override: "str | None" = None,
    ) -> dict:
        entry: dict = {
            "kind": "struct",
            "name": name,
            "header": header_name,
            "fields": fields,
            "doc": doc_override
            if doc_override is not None
            else _summary(_strip_comment(cursor.raw_comment or "")),
        }
        try:
            entry["size"] = cursor.type.get_size()
        except Exception:
            pass
        try:
            entry["align"] = cursor.type.get_align()
        except Exception:
            pass
        # clang Type.get_offset(fieldname) returns offset in bits; convert to bytes.
        for f in fields:
            try:
                f["offset"] = cursor.type.get_offset(f["name"]) // 8
            except Exception:
                pass
        return entry

    def _visit_enum(self, cursor: "cindex.Cursor", header: Path) -> None:
        if not cursor.is_definition():
            return
        name = cursor.spelling
        if not name:
            return
        values: "list[dict]" = []
        for child in cursor.get_children():
            if child.kind != cindex.CursorKind.ENUM_CONSTANT_DECL:
                continue
            values.append(
                {
                    "name": child.spelling,
                    "value": child.enum_value,
                    "doc": _strip_comment(child.raw_comment or ""),
                }
            )
        if name in self._seen_type:
            return
        self.types.append(
            {
                "kind": "enum",
                "name": name,
                "header": header.name,
                "values": values,
                "doc": _summary(_strip_comment(cursor.raw_comment or "")),
            }
        )
        self._seen_type.add(name)

    def _visit_typedef(self, cursor: "cindex.Cursor", header: Path) -> None:
        name = cursor.spelling
        if not name or name in self._seen_type:
            return
        underlying = cursor.underlying_typedef_type
        underlying_name = _type_name(underlying)
        decl_cursor = underlying.get_declaration()
        decl_kind = decl_cursor.kind if decl_cursor else None
        doc = _summary(_strip_comment(cursor.raw_comment or ""))

        # `typedef struct occtl_graph occtl_graph_t;` (forward declaration, no body)
        # → opaque_handle.
        if decl_kind == cindex.CursorKind.STRUCT_DECL and not decl_cursor.is_definition():
            self.types.append(
                {
                    "kind": "opaque_handle",
                    "name": name,
                    "header": header.name,
                    "doc": doc,
                }
            )
            self._seen_type.add(name)
            return

        # Value-handle: `typedef struct foo { uint64_t bits; } foo_t;` — a one-field
        # struct whose sole field is `bits`. Common to occtl_node_id_t / uid / etc.
        if (
            decl_kind == cindex.CursorKind.STRUCT_DECL
            and decl_cursor.is_definition()
        ):
            fields = self._collect_fields(decl_cursor)
            if len(fields) == 1 and fields[0]["name"] == "bits":
                entry = {
                    "kind": "value_handle",
                    "name": name,
                    "header": header.name,
                    "bits_field": fields[0]["type"],
                    "doc": doc,
                }
                try:
                    entry["size"] = underlying.get_size()
                except Exception:
                    pass
                try:
                    entry["align"] = underlying.get_align()
                except Exception:
                    pass
                self.types.append(entry)
                self._seen_type.add(name)
                return
            # Generic struct typedef. Make sure the type list carries the typedef
            # name (some headers use the tag name only, others only the typedef).
            entry = self._build_struct_entry(
                decl_cursor, fields, name, header.name, doc
            )
            self.types.append(entry)
            self._seen_type.add(name)
            return

        if decl_kind == cindex.CursorKind.ENUM_DECL and decl_cursor.is_definition():
            # The enum visitor already recorded the tag name; also record the
            # typedef alias so downstream tools can find either spelling.
            values: "list[dict]" = []
            for child in decl_cursor.get_children():
                if child.kind != cindex.CursorKind.ENUM_CONSTANT_DECL:
                    continue
                values.append(
                    {
                        "name": child.spelling,
                        "value": child.enum_value,
                        "doc": _summary(
                            _strip_comment(child.raw_comment or "")
                        ),
                    }
                )
            self.types.append(
                {
                    "kind": "enum",
                    "name": name,
                    "header": header.name,
                    "underlying": underlying_name,
                    "values": values,
                    "doc": doc,
                }
            )
            self._seen_type.add(name)
            return

        # Plain alias (e.g. `typedef uint32_t occtl_foo_t;`).
        self.types.append(
            {
                "kind": "alias",
                "name": name,
                "header": header.name,
                "underlying": underlying_name,
                "doc": doc,
            }
        )
        self._seen_type.add(name)

    # ---------------- macros ---------------- #

    def _visit_macro(
        self,
        cursor: "cindex.Cursor",
        header: Path,
        tu: "cindex.TranslationUnit",
    ) -> None:
        name = cursor.spelling
        if not name or name in self._seen_constant:
            return
        # Public-naming convention: every public macro starts with `OCCTL_`.
        if not name.startswith("OCCTL_"):
            return
        # Header include guards (`OCCTL_FOO_H`) are noise.
        if re.fullmatch(r"OCCTL_[A-Z0-9_]+_H", name):
            return
        if cursor.location.file is None:
            return  # built-in macro from -D flags
        # Extract token stream after the macro name.
        tokens = list(cursor.get_tokens())
        if len(tokens) < 2:
            return
        # Ignore function-like macros — they're not constants.
        # Heuristic: if the very next token after the name is "(" with no
        # whitespace, this is function-like. clang stores them with no easy
        # `is_functionlike` accessor, so we rely on the token stream.
        body_tokens = tokens[1:]
        if body_tokens and body_tokens[0].spelling == "(":
            # Could be function-like or `(value)` parenthesised constant.
            # Distinguish by whether the next ')' comes before any operand
            # token referencing arguments. Cheap proxy: function-like macros
            # have an identifier inside the parens that doesn't appear as a
            # token elsewhere. Conservative: ignore anything that *looks*
            # function-like by token adjacency to the name.
            # We test offset adjacency in the source.
            name_tok = tokens[0]
            paren_tok = body_tokens[0]
            if (
                name_tok.extent.end.line == paren_tok.extent.start.line
                and name_tok.extent.end.column == paren_tok.extent.start.column
            ):
                return
        body = " ".join(t.spelling for t in body_tokens).strip()
        # Try to evaluate integer-ish bodies (e.g. "1u", "0x7fffffff", "1").
        value: "int | str | None" = None
        m = re.fullmatch(r"\s*(?P<num>0x[0-9a-fA-F]+|[0-9]+)[uUlL]*\s*", body)
        if m:
            try:
                value = int(m.group("num"), 0)
            except ValueError:
                value = body
        else:
            value = body if body else None
        self.constants.append(
            {
                "name": name,
                "header": header.name,
                "value": value,
                "doc": _summary(_strip_comment(cursor.raw_comment or "")),
            }
        )
        self._seen_constant.add(name)

    # ---------------- emit ---------------- #

    def _emit(self) -> dict:
        """Assemble the final ABI catalog.

        Pulls version macros from extracted constants, falling back to
        ``None`` when they are missing (non-CMake build).  The top-level
        ``schema_version`` is the dumper's own format version so consumers
        can detect a breaking change to the JSON shape.
        """
        # Pull version macros (OCCTL_VERSION_*, OCCTL_ABI_VERSION) from the
        # extracted constants if present, falling back to None.
        cmap = {c["name"]: c["value"] for c in self.constants}

        def _as_int(v):
            if isinstance(v, int):
                return v
            if isinstance(v, str):
                try:
                    return int(v, 0)
                except ValueError:
                    return None
            return None

        return {
            "schema_version": SCHEMA_VERSION,
            "content_hash": _hash_headers(self.headers),
            "header_docs": self.header_docs,
            "abi_version": _as_int(cmap.get("OCCTL_ABI_VERSION")),
            "library_version": {
                "major": _as_int(cmap.get("OCCTL_VERSION_MAJOR")),
                "minor": _as_int(cmap.get("OCCTL_VERSION_MINOR")),
                "patch": _as_int(cmap.get("OCCTL_VERSION_PATCH")),
            },
            "headers": [h.name for h in self.headers],
            "functions": self.functions,
            "types": self.types,
            "constants": self.constants,
        }


# ----------------------------------------------------------------------------
# CLI
# ----------------------------------------------------------------------------


def _hash_headers(headers: "list[Path]") -> str:
    """SHA-256 of the concatenated header contents, for cache invalidation."""
    h = hashlib.sha256()
    for f in sorted(headers):
        h.update(f.read_bytes())
    return h.hexdigest()


def _read_cmake_version(project_root: Path) -> "dict[str, int]":
    """Pull the SemVer triplet from the top-level CMakeLists so the JSON
    library_version block reflects the source-of-truth rather than the
    headers' zero-fallback values.

    Returns a dict with major/minor/patch keys (each None if not found).
    """
    out = {"major": None, "minor": None, "patch": None}
    cm = project_root / "CMakeLists.txt"
    if not cm.is_file():
        return out
    text = cm.read_text(encoding="utf-8", errors="replace")
    for component in ("MAJOR", "MINOR", "PATCH"):
        m = re.search(
            rf"set\(\s*OCCTL_VERSION_{component}\s+(\d+)\s*\)",
            text,
        )
        if m:
            out[component.lower()] = int(m.group(1))
    return out


def _resolve_headers(include_dir: Path, preset: str | None) -> "list[Path]":
    available = sorted(include_dir.glob("occtl*.h"))
    if preset is None:
        return available
    if preset not in PRESET_HEADERS:
        raise SystemExit(
            f"abi_dump: unknown preset '{preset}'. Known: "
            + ", ".join(sorted(PRESET_HEADERS))
        )
    wanted = set(PRESET_HEADERS[preset])
    out = [h for h in available if h.name in wanted]
    missing = wanted.difference(h.name for h in out)
    if missing:
        _LOGGER.warning(
            "preset '%s' expects headers not present: %s",
            preset, ", ".join(sorted(missing))
        )
    return out


def _resolve_explicit_headers(headers: "list[Path]") -> "list[Path]":
    out: "list[Path]" = []
    for header in headers:
        resolved = header.resolve()
        if not resolved.is_file():
            raise SystemExit(f"abi_dump: header does not exist: {resolved}")
        out.append(resolved)
    return sorted(out)


def main(argv: "list[str] | None" = None) -> int:
    parser = argparse.ArgumentParser(
        description="Dump the OCCT-Light public C ABI to JSON via libclang."
    )
    here = Path(__file__).resolve().parent.parent
    parser.add_argument(
        "--include-dir",
        type=Path,
        default=here / "include" / "occtl",
        help="Directory containing occtl_*.h headers (default: %(default)s)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output JSON path (default: stdout)",
    )
    parser.add_argument(
        "--preset",
        choices=sorted(PRESET_HEADERS),
        default=None,
        help="Walk only headers enabled by this CMake preset (default: all).",
    )
    parser.add_argument(
        "--headers",
        nargs="+",
        type=Path,
        default=None,
        help="Explicit enabled header list. Overrides --preset and include-dir glob.",
    )
    parser.add_argument(
        "--verbose", "-v",
        action="count",
        default=0,
        help="Increase verbosity (-v = INFO, -vv = DEBUG).",
    )
    parser.add_argument(
        "--version", "-V",
        action="version",
        version=f"abi_dump schema v{SCHEMA_VERSION}",
    )
    args = parser.parse_args(argv)

    # Configure logging.
    _LOGGER.setLevel(
        logging.DEBUG if args.verbose >= 2
        else logging.INFO if args.verbose >= 1
        else logging.WARNING
    )
    _LOGGER.addHandler(logging.StreamHandler(sys.stderr))

    include_dir = args.include_dir.resolve()
    _LOGGER.info("walking %s", include_dir)
    if not include_dir.is_dir():

        raise SystemExit(f"abi_dump: include dir does not exist: {include_dir}")

    _import_cindex()

    headers = (
        _resolve_explicit_headers(args.headers)
        if args.headers is not None
        else _resolve_headers(include_dir, args.preset)
    )
    if not headers:
        raise SystemExit(
            f"abi_dump: no headers matched under {include_dir}"
            + (f" for preset '{args.preset}'." if args.preset else ".")
        )

    walker = AbiWalker(include_dir, headers)
    doc = walker.run()

    # Override library_version with the canonical values in CMakeLists.txt.
    # The headers' fallback values are zero when not built via CMake.
    cm_version = _read_cmake_version(here)
    if any(v is not None for v in cm_version.values()):
        doc["library_version"] = cm_version

    # Producer-side schema check: if the dumper's emitted shape drifts away
    # from what the validator (and therefore every binding generator) expects,
    # fail loud here rather than letting consumers crash with stack traces.
    from abi_schema import SCHEMA_VERSION as _SCHEMA_EXPECTED, SchemaMismatch, validate
    try:
        validate(doc, expected_schema_version=_SCHEMA_EXPECTED)
    except SchemaMismatch as e:
        raise SystemExit(f"abi_dump: emitted catalog fails schema check: {e}")

    text = json.dumps(doc, indent=2, sort_keys=True) + "\n"
    if args.output is None:
        sys.stdout.write(text)
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
