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

"""Normalize OCCT-style license headers across tracked source files.

This tool inserts a standardized OCCT-Light header that mirrors the original
OCCT wording pattern while using AGPL terms and an alternate commercial /
contractual clause.

Usage:
  python3 tools/license_headers.py --check
  python3 tools/license_headers.py --fix
"""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

SKIP_NAMES = {
    "LICENSE",
    "LICENSE.md",
    "LICENSE.txt",
    "LICENSE_AGPL_30.txt",
    "OCCT_LGPL_EXCEPTION.txt",
}

SLASH_EXT = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".ipp",
    ".tpp",
    ".inl",
    ".go",
    ".rs",
    ".js",
    ".mjs",
    ".cjs",
    ".ts",
    ".tsx",
    ".cs",
    ".java",
    ".swift",
    ".kt",
}

HASH_EXT = {
    ".py",
    ".sh",
    ".bash",
    ".zsh",
    ".cmake",
    ".toml",
    ".yml",
    ".yaml",
    ".mk",
    ".ini",
    ".cfg",
    ".conf",
    ".ps1",
    ".bat",
    ".gyp",
    ".gypi",
}

XML_EXT = {
    ".xml",
    ".csproj",
    ".props",
    ".targets",
    ".xaml",
    ".html",
    ".htm",
}

HASH_NAMES = {
    ".gitignore",
    ".gitattributes",
    ".editorconfig",
    ".clang-format",
    ".clang-tidy",
}


def git_tracked_files() -> list[Path]:
    out = subprocess.check_output(["git", "ls-files"], cwd=ROOT, text=True)
    return [ROOT / p for p in out.splitlines() if p]


def style_for(path: Path) -> str | None:
    name = path.name
    if name in SKIP_NAMES or name.startswith("LICENSE"):
        return None
    if name.lower().endswith(".md"):
        return None
    if name in HASH_NAMES:
        return "hash"
    if name in {"CMakeLists.txt", "Makefile", "GNUmakefile"}:
        return "hash"

    lower = name.lower()
    if lower.endswith(".tpl"):
        inner = lower[: -len(".tpl")]
        if inner:
            # Treat template files by their source syntax (e.g. .cc.tpl, .ts.tpl).
            p = Path(inner)
            if p.suffix:
                lower = inner
    for ext in XML_EXT:
        if lower.endswith(ext):
            return "xml"
    for ext in HASH_EXT:
        if lower.endswith(ext):
            return "hash"
    for ext in SLASH_EXT:
        if lower.endswith(ext):
            return "slash"
    return None


def build_header(style: str) -> str:
    lines = [
        "Copyright (c) 2026 Capgemini Engineering Research and Development.",
        "",
        "This file is part of OCCT-Light software library.",
        "",
        "This library is free software; you can redistribute it and/or modify it under",
        "the terms of the GNU Affero General Public License version 3 as published",
        "by the Free Software Foundation, with an option to use any later version.",
        "Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution",
        "for complete text of the license and disclaimer of any warranty.",
        "",
        "Alternatively, this file may be used under the terms of a commercial",
        "license or contractual agreement.",
        "",
        "SPDX-License-Identifier: AGPL-3.0-or-later",
    ]

    if style == "slash":
        return "\n".join(f"// {line}" if line else "//" for line in lines) + "\n\n"
    if style == "hash":
        return "\n".join(f"# {line}" if line else "#" for line in lines) + "\n\n"
    if style == "xml":
        body = "\n".join(f"  {line}" if line else "" for line in lines)
        return f"<!--\n{body}\n-->\n\n"
    raise ValueError(f"unsupported style: {style}")


def _is_license_line(line: str) -> bool:
    s = line.strip()
    if not s:
        return True
    tokens = (
        "SPDX-License-Identifier",
        "This file is part of OCCT-Light software library",
        "This file is part of Open CASCADE Technology software library",
        "This library is free software; you can redistribute it and/or modify it under",
        "the terms of the GNU Affero General Public License version 3 as published",
        "by the Free Software Foundation, with an option to use any later version.",
        "Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution",
        "for complete text of the license and disclaimer of any warranty.",
        "Alternatively, this file may be used under the terms of a commercial",
        "license or contractual agreement.",
        "LICENSE_AGPL_30.txt",
        "LICENSE_LGPL_21.txt",
        "OCCT_LGPL_EXCEPTION.txt",
        "commercial license or contractual agreement",
        "GNU Affero General Public License",
        "GNU Lesser General Public License",
        "Open CASCADE commercial license",
        "Copyright (c)",
        "Capgemini Engineering Research and Development",
        "Capgemini Engineering Research and Development",
    )
    return any(tok in s for tok in tokens)


def _is_comment_line(line: str, style: str) -> bool:
    s = line.strip()
    if not s:
        return True
    if style == "slash":
        return s.startswith("//") or s.startswith("/*") or s.startswith("*") or s.startswith("*/")
    if style == "hash":
        return s.startswith("#")
    if style == "xml":
        return s.startswith("<!--") or s.startswith("-->") or s.startswith("*") or s.startswith("<!--") or s.startswith("-")
    return False


def _comment_payload(line: str, style: str) -> str:
    s = line.rstrip("\n")
    if style == "slash":
        t = s.strip()
        if t.startswith("//"):
            return t[2:].strip()
        if t.startswith("/*"):
            return t[2:].strip().strip("*/").strip()
        if t.startswith("*"):
            return t[1:].strip().strip("*/").strip()
        if t.startswith("*/"):
            return ""
        return t
    if style == "hash":
        t = s.strip()
        if t.startswith("#"):
            return t[1:].strip()
        return t
    if style == "xml":
        t = s.strip()
        t = t.removeprefix("<!--").removesuffix("-->").strip()
        return t
    return s.strip()


def _render_payload_lines(payloads: list[str], style: str) -> str:
    if not payloads:
        return ""
    if style == "slash":
        rendered: list[str] = []
        for p in payloads:
            if p.startswith("go:build") or p.startswith("+build"):
                rendered.append(f"//{p}\n")
            else:
                rendered.append(f"// {p}\n")
        return "".join(rendered) + "\n"
    if style == "hash":
        return "".join(f"# {p}\n" for p in payloads) + "\n"
    if style == "xml":
        body = "\n".join(f"  {p}" for p in payloads)
        return f"<!--\n{body}\n-->\n\n"
    return ""


def strip_existing_header(lines: list[str], style: str) -> tuple[list[str], bool]:
    if style == "xml":
        idx = 0
        while idx < len(lines) and not lines[idx].strip():
            idx += 1
        if idx < len(lines) and lines[idx].lstrip().startswith("<!--"):
            end = idx
            had_license = False
            while end < len(lines) and end - idx < 120:
                if _is_license_line(lines[end]):
                    had_license = True
                if "-->" in lines[end]:
                    end += 1
                    break
                end += 1
            if had_license:
                rest = lines[end:]
                while rest and not rest[0].strip():
                    rest = rest[1:]
                return rest, True

    # Inspect the first comment block after preamble; replace only if it looks like a license header.
    idx = 0
    consumed = 0
    had_license = False
    while idx < len(lines) and idx < 80:
        line = lines[idx]
        if not _is_comment_line(line, style):
            break
        consumed += 1
        if _is_license_line(line):
            had_license = True
        idx += 1

    if had_license and consumed > 0:
        # Do not erase files that are entirely comments; only strip when there is body.
        if consumed >= len(lines):
            return lines, False
        block = lines[:consumed]
        preserved: list[str] = []
        for raw in block:
            payload = _comment_payload(raw, style)
            if not payload:
                continue
            if _is_license_line(payload):
                continue
            preserved.append(payload)
        rest = lines[consumed:]
        while rest and not rest[0].strip():
            rest = rest[1:]
        if preserved:
            rest = [_render_payload_lines(preserved, style)] + rest
        return rest, True

    return lines, False


def split_preamble(lines: list[str]) -> tuple[list[str], list[str]]:
    preamble: list[str] = []
    idx = 0
    if idx < len(lines) and lines[idx].startswith("#!"):
        preamble.append(lines[idx])
        idx += 1
    if idx < len(lines):
        s = lines[idx].strip()
        if s.startswith("# -*- coding:") or s.startswith("# coding:"):
            preamble.append(lines[idx])
            idx += 1
    if idx < len(lines) and lines[idx].lstrip().startswith("<?xml"):
        preamble.append(lines[idx])
        idx += 1
    if preamble and idx < len(lines) and lines[idx].strip() == "":
        preamble.append(lines[idx])
        idx += 1
    return preamble, lines[idx:]


def normalize_file(path: Path, style: str) -> tuple[bool, str]:
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return False, "binary-or-non-utf8"

    lines = text.splitlines(keepends=True)
    preamble, body = split_preamble(lines)
    new_header = build_header(style)
    if "".join(body).startswith(new_header):
        return False, text
    body, _ = strip_existing_header(body, style)
    new_text = "".join(preamble) + new_header + "".join(body)

    changed = new_text != text
    return changed, new_text


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fix", action="store_true", help="write changes in-place")
    parser.add_argument("--check", action="store_true", help="check-only mode (default)")
    args = parser.parse_args()

    do_fix = args.fix

    files = git_tracked_files()
    considered = 0
    changed_paths: list[Path] = []

    for path in files:
        style = style_for(path)
        if style is None:
            continue
        considered += 1
        changed, result = normalize_file(path, style)
        if changed:
            changed_paths.append(path)
            if do_fix:
                path.write_text(result, encoding="utf-8")

    mode = "fix" if do_fix else "check"
    print(f"license_headers: mode={mode} considered={considered} changed={len(changed_paths)}")
    if changed_paths and not do_fix:
        for p in changed_paths[:50]:
            print(p.relative_to(ROOT))
        if len(changed_paths) > 50:
            print(f"... and {len(changed_paths) - 50} more")

    return 1 if (changed_paths and not do_fix) else 0


if __name__ == "__main__":
    raise SystemExit(main())
