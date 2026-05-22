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

"""Schema validator for ``build/abi.json``.

The producer (``tools/abi_dump.py``) and every binding generator
(``bindings/<lang>/tools/generate_facade.{py,ts}``) consume the same JSON.
This module is the single contract between them: it asserts the shape and
fails loud with a one-line diagnostic pointing at the offending record.

Two entry points
----------------
* ``validate(catalog: dict, *, expected_schema_version: int = SCHEMA_VERSION)``
  raises :class:`SchemaMismatch` on the first problem found. Producer and
  Python generators call this directly.

* ``python tools/abi_schema.py path/to/abi.json``
  exits non-zero with the same diagnostic on stderr. TS generators (Node,
  WASM) shell out to this before parsing the JSON.

Both code paths share the same checks. Bumping the schema_version is the
backstop when a check would otherwise mass-fail every consumer.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple


SCHEMA_VERSION = 1

_TOP_KEYS = {
    "schema_version", "abi_version", "library_version", "content_hash",
    "functions", "types", "constants", "headers", "header_docs",
}

_FN_KEYS = {
    "name", "header", "return_type", "return_doc", "params",
    "retvals", "see_also", "threadsafe", "doc", "calling_convention",
}

_PARAM_KEYS = {
    "name", "type", "pointer_depth", "direction", "elem_type",
    "ownership", "doc",
}

_DIRECTIONS = {"in", "out", "inout"}
_OWNERSHIPS = {"owns", "borrows", None}
_TYPE_KINDS = {"struct", "enum", "value_handle", "opaque_handle", "alias"}

_TYPE_COMMON_KEYS = {"name", "kind", "header", "doc"}
_TYPE_PER_KIND_KEYS = {
    "struct":        _TYPE_COMMON_KEYS | {"fields", "size", "align"},
    "enum":          _TYPE_COMMON_KEYS | {"values"},
    "value_handle":  _TYPE_COMMON_KEYS | {"bits_field", "size", "align"},
    "opaque_handle": _TYPE_COMMON_KEYS,
    "alias":         _TYPE_COMMON_KEYS | {"underlying"},
}

_CONST_KEYS = {"name", "header", "value", "doc"}


class SchemaMismatch(Exception):
    """Raised when ``abi.json`` does not match the expected schema.

    The first argument is a one-line, grep-friendly diagnostic identifying
    the offending record. Always carries enough context (function name,
    type name, field path) to point a maintainer at the fix.
    """


def validate(catalog: Dict[str, Any], *, expected_schema_version: int = SCHEMA_VERSION) -> None:
    """Validate ``catalog`` in-place; raise :class:`SchemaMismatch` on first error."""
    if not isinstance(catalog, dict):
        raise SchemaMismatch(f"top-level must be object, got {type(catalog).__name__}")

    actual = catalog.get("schema_version")
    if actual != expected_schema_version:
        raise SchemaMismatch(
            f"schema_version mismatch: abi.json says {actual!r}, "
            f"validator expects {expected_schema_version} — regenerate abi.json "
            f"or update the consumer to the new schema"
        )

    for key in ("functions", "types", "constants"):
        if key not in catalog:
            raise SchemaMismatch(f"top-level key missing: {key!r}")
        if not isinstance(catalog[key], list):
            raise SchemaMismatch(f"top-level {key!r} must be a list")

    for i, fn in enumerate(catalog["functions"]):
        _validate_function(fn, i)

    for i, t in enumerate(catalog["types"]):
        _validate_type(t, i)

    for i, c in enumerate(catalog["constants"]):
        _validate_constant(c, i)


def _validate_function(fn: Dict[str, Any], index: int) -> None:
    name = fn.get("name", f"<unnamed @ functions[{index}]>")
    missing = _FN_KEYS - set(fn.keys())
    if missing:
        raise SchemaMismatch(
            f"functions[{index}] {name!r}: missing field(s) {sorted(missing)}"
        )
    if not isinstance(fn["params"], list):
        raise SchemaMismatch(f"functions[{index}] {name!r}: params must be a list")
    for j, p in enumerate(fn["params"]):
        _validate_param(p, name, j)
    if not isinstance(fn["retvals"], list):
        raise SchemaMismatch(f"functions[{index}] {name!r}: retvals must be a list")
    if not isinstance(fn["see_also"], list):
        raise SchemaMismatch(f"functions[{index}] {name!r}: see_also must be a list")


def _validate_param(p: Dict[str, Any], fn_name: str, index: int) -> None:
    missing = _PARAM_KEYS - set(p.keys())
    if missing:
        raise SchemaMismatch(
            f"functions {fn_name!r} params[{index}] {p.get('name','?')!r}: "
            f"missing field(s) {sorted(missing)}"
        )
    if p["direction"] not in _DIRECTIONS:
        raise SchemaMismatch(
            f"functions {fn_name!r} params[{index}] {p['name']!r}: "
            f"direction={p['direction']!r} not in {sorted(_DIRECTIONS)}"
        )
    if p["ownership"] not in _OWNERSHIPS:
        raise SchemaMismatch(
            f"functions {fn_name!r} params[{index}] {p['name']!r}: "
            f"ownership={p['ownership']!r} not in {{'owns','borrows',null}}"
        )
    if not isinstance(p["pointer_depth"], int):
        raise SchemaMismatch(
            f"functions {fn_name!r} params[{index}] {p['name']!r}: "
            f"pointer_depth must be int, got {type(p['pointer_depth']).__name__}"
        )


def _validate_type(t: Dict[str, Any], index: int) -> None:
    name = t.get("name", f"<unnamed @ types[{index}]>")
    kind = t.get("kind")
    if kind not in _TYPE_KINDS:
        raise SchemaMismatch(
            f"types[{index}] {name!r}: kind={kind!r} not in {sorted(_TYPE_KINDS)}"
        )
    required = _TYPE_PER_KIND_KEYS[kind]
    missing = required - set(t.keys())
    if missing:
        raise SchemaMismatch(
            f"types[{index}] {name!r} (kind={kind}): missing field(s) {sorted(missing)}"
        )
    if kind == "struct" and not isinstance(t["fields"], list):
        raise SchemaMismatch(f"types[{index}] {name!r}: fields must be a list")
    if kind == "enum" and not isinstance(t["values"], list):
        raise SchemaMismatch(f"types[{index}] {name!r}: values must be a list")


def _validate_constant(c: Dict[str, Any], index: int) -> None:
    name = c.get("name", f"<unnamed @ constants[{index}]>")
    missing = _CONST_KEYS - set(c.keys())
    if missing:
        raise SchemaMismatch(
            f"constants[{index}] {name!r}: missing field(s) {sorted(missing)}"
        )


def _cli(argv: List[str]) -> int:
    p = argparse.ArgumentParser(
        description="Validate build/abi.json against the OCCT-Light ABI schema."
    )
    p.add_argument("path", help="Path to abi.json")
    p.add_argument(
        "--expected-schema-version",
        type=int,
        default=SCHEMA_VERSION,
        help=f"Schema version this caller was built against (default: {SCHEMA_VERSION})",
    )
    args = p.parse_args(argv)
    try:
        with Path(args.path).open("r", encoding="utf-8") as f:
            catalog = json.load(f)
    except OSError as e:
        print(f"abi_schema: cannot read {args.path}: {e}", file=sys.stderr)
        return 2
    except json.JSONDecodeError as e:
        print(f"abi_schema: {args.path} is not valid JSON: {e}", file=sys.stderr)
        return 2
    try:
        validate(catalog, expected_schema_version=args.expected_schema_version)
    except SchemaMismatch as e:
        print(f"abi_schema: {e}", file=sys.stderr)
        return 1
    print(f"abi_schema: {args.path} validates against schema_version={args.expected_schema_version}")
    return 0


if __name__ == "__main__":
    sys.exit(_cli(sys.argv[1:]))
