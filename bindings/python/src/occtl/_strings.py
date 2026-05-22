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

"""UTF-8 string marshalling helpers between Python and the C ABI.

The C ABI is UTF-8 everywhere (see ``ABI_PATTERNS.md §9``). Conversion is:

- Python ``str`` → bytes: ``s.encode('utf-8')`` immediately before the call.
  The encoded bytes' lifetime is bounded by the call.
- Borrowed C output: decode immediately, never hold the pointer past the next
  call (the C ABI promises only "valid until next call on this thread").

Length-counted variants (``ptr + length``) are preferred over NUL-terminated
strings when both are offered by the ABI; this module exposes both shapes.
"""

from __future__ import annotations

from typing import Optional, Tuple


def encode(value: Optional[str]) -> bytes:
    """Encode a Python string to UTF-8 bytes. ``None`` becomes ``b""``."""
    if value is None:
        return b""
    if isinstance(value, bytes):
        return value
    return value.encode("utf-8")


def decode_cstr(ffi, ptr) -> str:
    """Decode a NUL-terminated UTF-8 ``const char*`` into a Python string.

    Returns ``""`` if ``ptr`` is NULL. The decode copies the data immediately;
    callers may use the result past the next C call.
    """
    if not ptr:
        return ""
    return ffi.string(ptr).decode("utf-8", errors="replace")


def decode_bounded(ffi, ptr, length: int) -> str:
    """Decode a length-counted UTF-8 buffer."""
    if not ptr or length <= 0:
        return ""
    raw = ffi.buffer(ptr, int(length))[:]
    return raw.decode("utf-8", errors="replace")


def two_call_string(
    sizing_call,
    refill_call,
    ffi,
    initial_size: int = 0,
) -> str:
    """Wrap the §10.1 two-call buffer pattern around a UTF-8 sized output.

    ``sizing_call`` is invoked with ``(NULL, 0)`` and is expected to either
    return the required size via an out-parameter or report
    ``OCCTL_BUFFER_TOO_SMALL`` with the size populated. ``refill_call`` is
    then invoked with ``(buf, size)`` to fill the buffer.

    This helper is generic and not currently used by the generated wrappers;
    it stays here so per-module hand-customisation can call it consistently.
    """
    size = sizing_call() if callable(sizing_call) else int(sizing_call)
    if size <= 0:
        return ""
    if initial_size and size < initial_size:
        size = initial_size
    buf = ffi.new("char[]", int(size))
    refill_call(buf, size)
    return ffi.string(buf, size).decode("utf-8", errors="replace")
