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

from __future__ import annotations

import argparse
import shutil
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class BindingLayout:
    name: str
    root: str
    api: tuple[str, ...]
    ffi: tuple[str, ...]
    tests: tuple[str, ...]
    tools: tuple[str, ...]
    generated: tuple[str, ...]


LAYOUTS: tuple[BindingLayout, ...] = (
    BindingLayout(
        name="python",
        root="bindings/python",
        api=("src/occtl",),
        ffi=("_abi.py", "src/occtl/_generated/_raw.py"),
        tests=("tests",),
        tools=("tools",),
        generated=("src/occtl/_generated",),
    ),
    BindingLayout(
        name="csharp",
        root="bindings/csharp",
        api=("src/api",),
        ffi=("src/ffi",),
        tests=("tests/unit", "tests/parity"),
        tools=("tools",),
        generated=("src/api/_Generated", "src/ffi/Generated"),
    ),
    BindingLayout(
        name="node",
        root="bindings/node",
        api=("src/ts",),
        ffi=("src/native",),
        tests=("tests",),
        tools=("tools",),
        generated=("src/ts/generated", "src/native/raw.cc"),
    ),
    BindingLayout(
        name="wasm",
        root="bindings/wasm",
        api=("src/ts",),
        ffi=("src/native",),
        tests=("tests",),
        tools=("tools",),
        generated=("src/ts/generated", "src/native/raw.cc"),
    ),
    BindingLayout(
        name="rust",
        root="bindings/rust",
        api=("src/occtl",),
        ffi=("src/occtl-sys",),
        tests=("src/occtl/tests", "tests/parity_runner"),
        tools=(".",),
        generated=("src/occtl-sys/generated",),
    ),
    BindingLayout(
        name="go",
        root="bindings/go",
        api=("src/occtl",),
        ffi=("src/occtl/raw.go",),
        tests=("src/occtl", "tests/parity_runner"),
        tools=("tools",),
        generated=("generated",),
    ),
    BindingLayout(
        name="java",
        root="bindings/java",
        api=("src/main/java/org/occtl",),
        ffi=("src/main/java/org/occtl/generated",),
        tests=("src/test/java/org/occtl",),
        tools=("tools",),
        generated=("src/main/java/org/occtl/generated",),
    ),
)

TRANSIENT_PATTERNS = (
    ".DS_Store",
    ".pytest_cache",
    ".venv",
    "node_modules",
    ".emsdk",
    "build",
    "dist",
    "target",
    "prebuilds",
    "Testing",
    "*.tgz",
    "*.nupkg",
    "*.snupkg",
    "*.jar",
    "*.whl",
    "*.egg-info",
    "tests/parity_runner/parity_runner",
)

RECURSIVE_TRANSIENT_PATTERNS = (
    "**/bin",
    "**/obj",
    "**/.DS_Store",
)


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _print_section(name: str, values: tuple[str, ...]) -> None:
    joined = ", ".join(values)
    print(f"  {name:<10} {joined}")


def cmd_overview() -> int:
    print("OCCT-Light bindings layout overview")
    for layout in LAYOUTS:
        print(f"- {layout.name} ({layout.root})")
        _print_section("api", layout.api)
        _print_section("ffi", layout.ffi)
        _print_section("tests", layout.tests)
        _print_section("tools", layout.tools)
        _print_section("generated", layout.generated)
    return 0


def _exists(root: Path, rel: str) -> bool:
    if rel == ".":
        return True
    return (root / rel).exists()


def cmd_audit() -> int:
    base = _repo_root()
    has_error = False
    for layout in LAYOUTS:
        root = base / layout.root
        print(f"[audit] {layout.name} ({root})")
        if not root.exists():
            print("  missing root directory")
            has_error = True
            continue
        for label, values in (
            ("api", layout.api),
            ("ffi", layout.ffi),
            ("tests", layout.tests),
            ("tools", layout.tools),
        ):
            missing = [value for value in values if not _exists(root, value)]
            if missing:
                print(f"  missing {label}: {', '.join(missing)}")
                has_error = True
    return 1 if has_error else 0


def _iter_transient_paths(binding_root: Path):
    for pattern in TRANSIENT_PATTERNS:
        for path in binding_root.glob(pattern):
            yield path
    for pattern in RECURSIVE_TRANSIENT_PATTERNS:
        for path in binding_root.glob(pattern):
            yield path


def cmd_clean(dry_run: bool) -> int:
    base = _repo_root()
    candidates: dict[Path, None] = {}
    for layout in LAYOUTS:
        root = base / layout.root
        if not root.exists():
            continue
        for path in _iter_transient_paths(root):
            if path == root:
                continue
            candidates[path] = None
    all_paths = sorted(candidates.keys(), key=lambda p: (len(p.parts), str(p)))
    paths: list[Path] = []
    selected = set()
    for path in all_paths:
        keep = True
        for parent in path.parents:
            if parent in selected:
                keep = False
                break
        if keep:
            paths.append(path)
            selected.add(path)
    for path in paths:
        rel = path.relative_to(base)
        print(f"[clean] {rel}")
        if dry_run:
            continue
        if path.is_dir():
            shutil.rmtree(path, ignore_errors=True)
        elif path.exists():
            path.unlink()
    print(f"[done] {'planned' if dry_run else 'removed'} {len(paths)} paths")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Unified workspace tool for bindings.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("overview", help="Show logical layout map for each binding.")
    subparsers.add_parser("audit", help="Validate required layout paths exist.")

    clean_parser = subparsers.add_parser(
        "clean",
        help="Remove transient local build artefacts from bindings folders.",
    )
    clean_parser.add_argument(
        "--dry-run",
        action="store_true",
        help="List cleanup actions without deleting files.",
    )

    args = parser.parse_args()
    if args.command == "overview":
        return cmd_overview()
    if args.command == "audit":
        return cmd_audit()
    if args.command == "clean":
        return cmd_clean(dry_run=args.dry_run)
    raise RuntimeError("unknown command")


if __name__ == "__main__":
    raise SystemExit(main())
