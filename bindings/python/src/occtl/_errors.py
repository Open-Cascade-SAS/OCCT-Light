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

"""Status-code-to-exception translation for the occtl binding.

Every non-OK ``occtl_status_t`` returned across the C ABI boundary is converted
to a typed Python exception by :func:`_check`. The exception subclass is keyed
on the primary status code so callers can catch by category::

    try:
        graph.make_vertex(x=1.0, y=2.0, z=3.0)
    except occtl.InvalidArgumentError as exc:
        ...

Iterator end-of-iteration (``OCCTL_NOT_FOUND`` returned from a ``_next`` call)
is **not** an error and never produces an exception. See ``_iters.py`` for the
generator machinery that handles that sentinel.

The error subclass list mirrors the C ``occtl_status_t`` enum one-for-one — we
do **not** invent additional categories (see ``BINDINGS.md`` §9).
"""

from __future__ import annotations

import enum
from typing import Optional


class Status(enum.IntEnum):
    """Mirror of the C ``occtl_status_t`` enum.

    Values are stable forever; new codes may be appended.
    """

    OK = 0
    ERROR = 1
    INVALID_ARGUMENT = 2
    INVALID_HANDLE = 3
    NOT_FOUND = 4
    OUT_OF_MEMORY = 5
    OUT_OF_RANGE = 6
    NOT_DONE = 7
    GEOMETRY_INVALID = 8
    TOPOLOGY_INVALID = 9
    IO_ERROR = 10
    FORMAT_ERROR = 11
    UNSUPPORTED = 12
    CANCELLED = 13
    BUFFER_TOO_SMALL = 14
    VERSION_MISMATCH = 15
    INTERNAL = 16
    WRONG_KIND = 17


class Error(Exception):
    """Base class for every translated ``occtl_status_t`` failure.

    Attributes:
        status:    The Python ``Status`` enum value for the C status code.
        message:   The UTF-8 decoded ``occtl_error_last()->message`` snapshot,
                   copied across the ABI boundary at the moment of failure.
        source:    The 64-bit UID of the offending entity (zero if not
                   applicable).
        extended:  The SQLite-style extended subcode, or 0.
    """

    status: Status
    message: str
    source: int
    extended: int

    def __init__(
        self,
        status: Status,
        message: str,
        source: int = 0,
        extended: int = 0,
    ) -> None:
        self.status = status
        self.message = message
        self.source = int(source)
        self.extended = int(extended)
        super().__init__(self._format())

    def _format(self) -> str:
        base = f"{self.status.name}"
        if self.message:
            base += f": {self.message}"
        if self.source:
            base += f" (source=0x{self.source:016x})"
        if self.extended:
            base += f" (extended={self.extended})"
        return base


class GenericError(Error):
    """``OCCTL_ERROR`` — unspecified failure."""


class InvalidArgumentError(Error):
    """``OCCTL_INVALID_ARGUMENT`` — required pointer was NULL or argument malformed."""


class InvalidHandleError(Error):
    """``OCCTL_INVALID_HANDLE`` — handle was NULL or freed."""


class NotFoundError(Error):
    """``OCCTL_NOT_FOUND`` — requested entity does not exist."""


class OutOfMemoryError(Error):
    """``OCCTL_OUT_OF_MEMORY`` — native allocation failed."""


class OutOfRangeError(Error):
    """``OCCTL_OUT_OF_RANGE`` — numeric argument outside valid range."""


class NotDoneError(Error):
    """``OCCTL_NOT_DONE`` — OCCT operation reported ``IsDone()==false``."""


class GeometryInvalidError(Error):
    """``OCCTL_GEOMETRY_INVALID`` — input geometry was not well-formed."""


class TopologyInvalidError(Error):
    """``OCCTL_TOPOLOGY_INVALID`` — input topology was not well-formed."""


class IoError(Error):
    """``OCCTL_IO_ERROR`` — filesystem or I/O failure."""


class FormatError(Error):
    """``OCCTL_FORMAT_ERROR`` — file contents malformed."""


class UnsupportedError(Error):
    """``OCCTL_UNSUPPORTED`` — operation not supported in this build."""


class CancelledError(Error):
    """``OCCTL_CANCELLED`` — cooperative cancellation honoured."""


class BufferTooSmallError(Error):
    """``OCCTL_BUFFER_TOO_SMALL`` — two-call pattern: caller buffer too small."""


class VersionMismatchError(Error):
    """``OCCTL_VERSION_MISMATCH`` — options ``struct_version`` not supported."""


class InternalError(Error):
    """``OCCTL_INTERNAL`` — C++ exception caught at the ABI boundary."""


class WrongKindError(Error):
    """``OCCTL_WRONG_KIND`` — handle holds a different kind than expected."""


class AbiMismatchError(RuntimeError):
    """The runtime ABI version disagrees with the compiled binding."""

    def __init__(self, expected: int, actual: int) -> None:
        self.expected = expected
        self.actual = actual
        super().__init__(
            f"OCCT-Light ABI mismatch: binding compiled for ABI {expected}, "
            f"runtime reports {actual}. Refuse to proceed."
        )


_STATUS_TO_ERROR: dict[int, type[Error]] = {
    Status.ERROR: GenericError,
    Status.INVALID_ARGUMENT: InvalidArgumentError,
    Status.INVALID_HANDLE: InvalidHandleError,
    Status.NOT_FOUND: NotFoundError,
    Status.OUT_OF_MEMORY: OutOfMemoryError,
    Status.OUT_OF_RANGE: OutOfRangeError,
    Status.NOT_DONE: NotDoneError,
    Status.GEOMETRY_INVALID: GeometryInvalidError,
    Status.TOPOLOGY_INVALID: TopologyInvalidError,
    Status.IO_ERROR: IoError,
    Status.FORMAT_ERROR: FormatError,
    Status.UNSUPPORTED: UnsupportedError,
    Status.CANCELLED: CancelledError,
    Status.BUFFER_TOO_SMALL: BufferTooSmallError,
    Status.VERSION_MISMATCH: VersionMismatchError,
    Status.INTERNAL: InternalError,
    Status.WRONG_KIND: WrongKindError,
}


def _decode_status(raw: int) -> Status:
    """Best-effort coerce a C status int to ``Status``; unknown values map to GENERIC."""
    try:
        return Status(int(raw))
    except ValueError:
        return Status.ERROR


def _check(status_int: int) -> None:
    """Translate a non-OK status to the matching :class:`Error` subclass.

    No-op on ``OCCTL_OK``. On failure reads ``occtl_error_last()`` exactly once
    and raises the typed exception. The raw message is decoded immediately (the
    library promises the buffer is only valid until the next call on this
    thread).
    """
    if status_int == Status.OK:
        return
    status = _decode_status(status_int)
    cls = _STATUS_TO_ERROR.get(status, GenericError)
    message, source, extended = _snapshot_last_error()
    raise cls(status, message, source, extended)


# Filled by occtl/__init__.py once `_raw` is loaded — avoids an import cycle.
_snapshot_callback: Optional[
    "tuple[callable, callable]"
] = None  # (snapshot_fn, decode_fn); process-wide graph snapshot hook


def install_snapshot_callback(fn) -> None:
    """Idempotently install the ``occtl_error_last`` reader.

    Called by ``occtl.__init__`` after the raw cffi layer has loaded. The
    callback returns a ``(message, source, extended)`` triple."""
    global _snapshot_callback
    _snapshot_callback = fn


def _snapshot_last_error() -> tuple[str, int, int]:
    if _snapshot_callback is None:
        return ("", 0, 0)
    try:
        return _snapshot_callback()
    except Exception:
        import logging
        logging.getLogger("occtl").exception(
            "_snapshot_last_error: callback raised; falling back to empty error."
        )
        return ("", 0, 0)
