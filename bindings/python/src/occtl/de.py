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

"""Format-dispatch I/O — :mod:`occtl.de`.

Wraps :c:`occtl_de_read` / :c:`occtl_de_write` (automatic format
dispatch by file extension) and the format-enumeration helpers.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional

from ._generated import de as _de_gen
from ._errors import _check
from ._generated._raw import ffi, lib


@dataclass(frozen=True, slots=True)
class FormatInfo:
    """Stable metadata for a supported data-exchange format."""

    id: str
    label: str
    extensions: tuple[str, ...]
    can_read_file: bool
    can_write_file: bool
    can_read_memory: bool
    can_write_memory: bool


def _cstr(text: str):
    return text.encode("utf-8")


def format_id_for_path(path: str) -> Optional[str]:
    """Return the stable format id for ``path``, or ``None`` if unsupported."""
    out = ffi.new("const char **")
    _check(lib.occtl_de_format_id_from_path(_cstr(path), out))
    ptr = out[0]
    if not ptr:
        return None
    return ffi.string(ptr).decode("utf-8", errors="replace")


def supported_formats() -> tuple[str, ...]:
    """Return stable lowercase format ids enabled in this build."""
    count = ffi.new("size_t*")
    _check(lib.occtl_de_format_ids(ffi.NULL, 0, count))
    buf = ffi.new("const char*[]", count[0])
    _check(lib.occtl_de_format_ids(buf, count[0], count))
    return tuple(
        ffi.string(buf[i]).decode("utf-8", errors="replace")
        for i in range(count[0])
    )


def format_extensions(format_id: str) -> tuple[str, ...]:
    """Return lowercase file extensions for ``format_id``."""
    encoded = _cstr(format_id)
    count = ffi.new("size_t*")
    _check(lib.occtl_de_format_extensions(encoded, ffi.NULL, 0, count))
    buf = ffi.new("const char*[]", count[0])
    _check(lib.occtl_de_format_extensions(encoded, buf, count[0], count))
    return tuple(
        ffi.string(buf[i]).decode("utf-8", errors="replace")
        for i in range(count[0])
    )


def _format_info_from_raw(raw) -> FormatInfo:
    return FormatInfo(
        id=ffi.string(raw.id).decode("utf-8", errors="replace") if raw.id else "",
        label=ffi.string(raw.label).decode("utf-8", errors="replace") if raw.label else "",
        extensions=format_extensions(ffi.string(raw.id).decode("utf-8", errors="replace")),
        can_read_file=bool(raw.can_read_file),
        can_write_file=bool(raw.can_write_file),
        can_read_memory=bool(raw.can_read_memory),
        can_write_memory=bool(raw.can_write_memory),
    )


def format_info(format_id: str) -> FormatInfo:
    """Return metadata for ``format_id``."""
    raw = ffi.new("occtl_de_format_info_t*")
    _check(lib.occtl_de_format_info_by_id(_cstr(format_id), raw))
    return _format_info_from_raw(raw[0])


def format_infos() -> tuple[FormatInfo, ...]:
    """Return metadata for every enabled data-exchange format."""
    count = ffi.new("size_t*")
    _check(lib.occtl_de_format_count(count))
    out = []
    for i in range(count[0]):
        raw = ffi.new("occtl_de_format_info_t*")
        _check(lib.occtl_de_format_info_at(i, raw))
        out.append(_format_info_from_raw(raw[0]))
    return tuple(out)

# Re-export every generated wrapper.
_SENTINEL = object()
_IDIOMATIC_OVERRIDES = {
    "occtl_de_format_id_from_path",
    "occtl_de_format_ids",
    "occtl_de_format_extensions",
    "occtl_de_format_info_by_id",
    "occtl_de_format_info_at",
    "occtl_de_format_count",
}
for _name in dir(_de_gen):
    if _name.startswith("occtl_"):
        if _name in _IDIOMATIC_OVERRIDES:
            continue
        if globals().get(_name, _SENTINEL) is _SENTINEL:
            globals()[_name] = getattr(_de_gen, _name)
        else:
            import warnings
            warnings.warn(
                f"occtl.de: skipping {_name} — already defined in de namespace",
                stacklevel=2,
            )
del _name, _SENTINEL
