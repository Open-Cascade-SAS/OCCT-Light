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

"""Build community-facing package artifacts for OCCT-Light bindings.

Script-first release helper for local/dev pipelines. Designed to run on macOS,
Linux, and Windows. This does not publish artifacts; it prepares them.

Targets:
- python-wheel : PyPI-style wheel + sdist
- python-conda : conda package via local recipe
- csharp-nuget : NuGet package
- node-npm     : npm tarball for Node binding
- wasm-npm     : npm tarball for WASM binding
- java-maven   : Maven package (JAR)
- rust-crate   : Cargo .crate archives for occtl + occtl-sys
- go-module    : Go module source zip (distribution snapshot)
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable

ALL_TARGETS = (
    "python-wheel",
    "python-conda",
    "csharp-nuget",
    "node-npm",
    "wasm-npm",
    "java-maven",
    "rust-crate",
    "go-module",
)

TARGET_OUT_SUBDIR: dict[str, str] = {
    "python-wheel": "python/pypi",
    "python-conda": "python/conda",
    "csharp-nuget": "csharp/nuget",
    "node-npm": "node/npm",
    "wasm-npm": "wasm/npm",
    "java-maven": "java/maven",
    "rust-crate": "rust/crate",
    "go-module": "go/module",
}


def _run(cmd: list[str], cwd: Path, dry_run: bool) -> None:
    pretty = " ".join(cmd)
    print(f"[run] ({cwd}) {pretty}")
    if dry_run:
        return
    subprocess.run(cmd, cwd=str(cwd), check=True)


def _ensure_dir(path: Path, dry_run: bool) -> None:
    if dry_run:
        print(f"[mkdir] {path}")
        return
    path.mkdir(parents=True, exist_ok=True)


def _copy_glob(src_dir: Path, pattern: str, dst_dir: Path, dry_run: bool) -> None:
    matches = sorted(src_dir.glob(pattern))
    if not matches:
        raise RuntimeError(f"no files matched {pattern!r} in {src_dir}")
    for src in matches:
        dst = dst_dir / src.name
        print(f"[copy] {src} -> {dst}")
        if not dry_run:
            shutil.copy2(src, dst)


def _require_cmd(name: str, dry_run: bool) -> None:
    if dry_run:
        return
    if shutil.which(name) is None:
        raise RuntimeError(f"required command not found in PATH: {name}")


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while True:
            chunk = handle.read(1 << 20)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def _append_manifest_entries(
    manifest_entries: list[dict[str, object]],
    target: str,
    target_out_dir: Path,
    existing_files: set[Path],
    root_out_dir: Path,
) -> None:
    if not target_out_dir.exists():
        return
    artifacts = sorted(path for path in target_out_dir.rglob("*") if path.is_file())
    for artifact in artifacts:
        rel = artifact.relative_to(root_out_dir)
        status = "updated" if artifact in existing_files else "new"
        manifest_entries.append(
            {
                "target": target,
                "status": status,
                "path": str(rel).replace("\\", "/"),
                "size_bytes": artifact.stat().st_size,
                "sha256": _sha256(artifact),
            }
        )


def _venv_python(venv_dir: Path) -> Path:
    if os.name == "nt":
        return venv_dir / "Scripts" / "python.exe"
    return venv_dir / "bin" / "python"


def _ensure_python_packaging_venv(root: Path, out: Path, dry_run: bool) -> Path:
    venv_dir = out / "_python-packaging-venv"
    venv_python = _venv_python(venv_dir)
    if venv_python.exists():
        return venv_python
    _run([sys.executable, "-m", "venv", str(venv_dir)], root, dry_run)
    _run([str(venv_python), "-m", "pip", "install", "--upgrade", "pip", "build"], root, dry_run)
    return venv_python


def _build_python_wheel(root: Path, out: Path, dry_run: bool) -> None:
    out_dir = out / "python" / "pypi"
    _ensure_dir(out_dir, dry_run)
    venv_python = _ensure_python_packaging_venv(root, out, dry_run)
    _run(
        [
            str(venv_python),
            "-m",
            "build",
            "--sdist",
            "--wheel",
            "--outdir",
            str(out_dir),
            str(root / "bindings" / "python"),
        ],
        root,
        dry_run,
    )


def _build_python_conda(root: Path, out: Path, dry_run: bool) -> None:
    out_dir = out / "python" / "conda"
    recipe_dir = root / "bindings" / "python" / "conda" / "recipe"
    _ensure_dir(out_dir, dry_run)
    _require_cmd("conda", dry_run)
    _run(["conda", "install", "-y", "conda-build"], root, dry_run)
    _run(
        [
            "conda",
            "build",
            str(recipe_dir),
            "--output-folder",
            str(out_dir),
            "--no-test",
        ],
        root,
        dry_run,
    )


def _build_csharp_nuget(root: Path, out: Path, dry_run: bool) -> None:
    out_dir = out / "csharp" / "nuget"
    _ensure_dir(out_dir, dry_run)
    _require_cmd("dotnet", dry_run)
    _run(
        [
            "dotnet",
            "pack",
            str(root / "bindings" / "csharp" / "src" / "api" / "OcctL.csproj"),
            "-c",
            "Release",
            "-o",
            str(out_dir),
        ],
        root,
        dry_run,
    )


def _build_node_npm(root: Path, out: Path, dry_run: bool) -> None:
    node_dir = root / "bindings" / "node"
    out_dir = out / "node" / "npm"
    _ensure_dir(out_dir, dry_run)
    _require_cmd("npm", dry_run)
    _run(["npm", "ci"], node_dir, dry_run)
    _run(["npm", "run", "build"], node_dir, dry_run)
    _run(["npm", "pack", "--pack-destination", str(out_dir)], node_dir, dry_run)


def _build_wasm_npm(root: Path, out: Path, dry_run: bool) -> None:
    wasm_dir = root / "bindings" / "wasm"
    out_dir = out / "wasm" / "npm"
    _ensure_dir(out_dir, dry_run)
    _require_cmd("npm", dry_run)
    _run(["npm", "ci"], wasm_dir, dry_run)
    _run(["npm", "run", "build"], wasm_dir, dry_run)
    _run(["npm", "pack", "--pack-destination", str(out_dir)], wasm_dir, dry_run)


def _build_java_maven(root: Path, out: Path, dry_run: bool) -> None:
    java_dir = root / "bindings" / "java"
    out_dir = out / "java" / "maven"
    _ensure_dir(out_dir, dry_run)
    _require_cmd("mvn", dry_run)
    _run(["mvn", "-B", "-DskipTests", "package"], java_dir, dry_run)
    if dry_run:
        print(f"[copy] {java_dir / 'target'}/*.jar -> {out_dir}")
        return
    _copy_glob(java_dir / "target", "*.jar", out_dir, dry_run)
    shutil.copy2(java_dir / "pom.xml", out_dir / "pom.xml")


def _build_rust_crate(root: Path, out: Path, dry_run: bool) -> None:
    rust_dir = root / "bindings" / "rust"
    out_dir = out / "rust" / "crate"
    _ensure_dir(out_dir, dry_run)
    _require_cmd("cargo", dry_run)
    _run(["cargo", "package", "-p", "occtl-sys", "--allow-dirty", "--no-verify"], rust_dir, dry_run)
    try:
        _run(["cargo", "package", "-p", "occtl", "--allow-dirty", "--no-verify"], rust_dir, dry_run)
    except subprocess.CalledProcessError:
        print(
            "[warn] cargo package for occtl failed (occtl depends on occtl-sys from registry). "
            "Exporting source tarball as fallback."
        )
        archive_base = out_dir / "occtl-source"
        if dry_run:
            print(f"[archive] {rust_dir / 'src' / 'occtl'} -> {archive_base}.tar.gz")
        else:
            shutil.make_archive(str(archive_base), "gztar", root_dir=str(rust_dir / "src" / "occtl"))
            note = out_dir / "occtl-packaging-note.txt"
            note.write_text(
                "cargo package for occtl requires published occtl-sys on registry.\n"
                "Fallback artifact generated: occtl-source.tar.gz\n",
                encoding="utf-8",
            )
    if dry_run:
        print(f"[copy] {rust_dir / 'target' / 'package'}/*.crate -> {out_dir}")
        return
    _copy_glob(rust_dir / "target" / "package", "*.crate", out_dir, dry_run)


def _build_go_module(root: Path, out: Path, dry_run: bool) -> None:
    go_dir = root / "bindings" / "go"
    out_dir = out / "go" / "module"
    _ensure_dir(out_dir, dry_run)
    _require_cmd("go", dry_run)
    _run(["go", "list", "./..."], go_dir, dry_run)
    archive_base = out_dir / "occtl-go-module"
    if dry_run:
        print(f"[archive] {go_dir} -> {archive_base}.zip")
        return
    shutil.make_archive(str(archive_base), "zip", root_dir=str(go_dir))


BUILDERS: dict[str, Callable[[Path, Path, bool], None]] = {
    "python-wheel": _build_python_wheel,
    "python-conda": _build_python_conda,
    "csharp-nuget": _build_csharp_nuget,
    "node-npm": _build_node_npm,
    "wasm-npm": _build_wasm_npm,
    "java-maven": _build_java_maven,
    "rust-crate": _build_rust_crate,
    "go-module": _build_go_module,
}


def _parse_targets(raw: str) -> list[str]:
    requested = [part.strip() for part in raw.split(",") if part.strip()]
    if not requested or requested == ["all"]:
        return list(ALL_TARGETS)
    unknown = sorted(set(requested) - set(ALL_TARGETS))
    if unknown:
        raise SystemExit(f"unknown target(s): {', '.join(unknown)}")
    return requested


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--targets",
        default="all",
        help=f"Comma-separated targets (default: all). Available: {', '.join(ALL_TARGETS)}",
    )
    parser.add_argument(
        "--output-dir",
        default="build/binding-packages",
        help="Output directory for built artifacts (default: build/binding-packages).",
    )
    parser.add_argument(
        "--repo-root",
        default=".",
        help="Repository root (default: current directory).",
    )
    parser.add_argument(
        "--manifest-path",
        default="binding-packages-manifest.json",
        help=(
            "Manifest file name under output directory "
            "(default: binding-packages-manifest.json)."
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print commands without executing them.",
    )
    args = parser.parse_args()

    root = Path(args.repo_root).resolve()
    out = (root / args.output_dir).resolve()
    targets = _parse_targets(args.targets)
    manifest_entries: list[dict[str, object]] = []

    print(f"[info] repo_root={root}")
    print(f"[info] output_dir={out}")
    print(f"[info] targets={','.join(targets)}")
    print(f"[info] dry_run={args.dry_run}")

    if not args.dry_run:
        out.mkdir(parents=True, exist_ok=True)

    for target in targets:
        print(f"[target] {target}")
        out_subdir = TARGET_OUT_SUBDIR[target]
        target_out_dir = out / out_subdir
        existing_files = (
            {path for path in target_out_dir.rglob("*") if path.is_file()}
            if target_out_dir.exists() and not args.dry_run
            else set()
        )
        BUILDERS[target](root, out, args.dry_run)
        if not args.dry_run:
            _append_manifest_entries(manifest_entries, target, target_out_dir, existing_files, out)

    if not args.dry_run:
        manifest = {
            "generated_at_utc": datetime.now(timezone.utc).isoformat(),
            "repo_root": str(root),
            "output_dir": str(out),
            "targets": targets,
            "artifacts": manifest_entries,
        }
        manifest_path = out / args.manifest_path
        manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        print(f"[manifest] {manifest_path}")

    print("[done] packaging targets completed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
