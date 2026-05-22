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

"""RAII handle wrappers for opaque ``occtl_*_t`` pointers.

Every opaque handle the C ABI hands back is owned by a ``_Handle`` subclass.
The wrapper:

- Holds the cffi pointer in ``_ptr``.
- Calls the matching ``occtl_*_free`` from ``__del__`` and ``close()``.
- Supports the context-manager protocol so ``with Graph() as g:`` is the
  idiomatic spelling.
- Is idempotent on close (the wrapper drops the pointer first, then frees;
  subsequent calls are no-ops).
- Forbids handle resurrection (re-binding a pointer after close).

This mirrors :class:`pygit2.Repository` and other libgit2-style bindings.
"""

from __future__ import annotations

from typing import Any, Callable, Optional, Type, TypeVar


_T = TypeVar("_T", bound="_Handle")


class _Handle:
    """Base class for opaque-pointer handles.

    Subclasses set the class-level attribute ``_free_fn`` to a callable that
    takes the raw cffi pointer and releases it (the ``occtl_*_free`` family is
    NULL-tolerant per the ABI contract). Subclasses also set ``_c_typename`` to
    the cffi type spelling, used by ``_from_ptr`` to validate input.
    """

    # Filled in by concrete subclasses.
    _free_fn: Optional[Callable[[Any], None]] = None
    _c_typename: Optional[str] = None

    __slots__ = ("_ptr", "_closed", "__weakref__")

    def __init__(self) -> None:
        # Concrete handles are constructed via factory class methods, not
        # this default. Direct ``Cls()`` invocation should usually go through
        # an idiomatic constructor in the subclass (e.g. ``Graph()`` which
        # calls ``occtl_graph_create``).
        self._ptr: Optional[Any] = None
        self._closed = True

    # ------------------------------------------------------------------ #
    # Construction
    # ------------------------------------------------------------------ #

    @classmethod
    def _adopt(cls: Type[_T], ptr: Any) -> _T:
        """Wrap an already-created cffi pointer.

        Raises :class:`occtl.InvalidHandleError` if ``ptr`` is NULL or falsy.
        The caller relinquishes ownership; the handle takes responsibility for
        eventually calling ``_free_fn``.
        """
        from ._errors import InvalidHandleError, Status

        if not ptr:
            raise InvalidHandleError(
                Status.INVALID_HANDLE,
                f"{cls.__name__}: refusing to adopt a NULL pointer",
            )
        # Bypass __init__ to preserve the adopted open-state.
        obj = cls.__new__(cls)
        obj._ptr = ptr
        obj._closed = False
        return obj

    # ------------------------------------------------------------------ #
    # Disposal
    # ------------------------------------------------------------------ #

    def close(self) -> None:
        """Release the underlying native handle.

        Idempotent: calling ``close`` a second time is a no-op. After ``close``
        every method that requires the native handle raises
        :class:`InvalidHandleError`.
        """
        if self._closed:
            return
        ptr = self._ptr
        # Mark closed *before* freeing so a callback re-entering this object
        # during free (unlikely but defensive) cannot observe a live pointer.
        self._ptr = None
        self._closed = True
        free_fn = type(self)._free_fn
        if free_fn is not None and ptr is not None:
            try:
                free_fn(ptr)
            except Exception:  # pragma: no cover — disposal should never raise
                pass

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:  # pragma: no cover
            pass

    def __enter__(self: _T) -> _T:
        return self

    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> None:
        self.close()

    # ------------------------------------------------------------------ #
    # Inspection helpers
    # ------------------------------------------------------------------ #

    @property
    def closed(self) -> bool:
        return self._closed

    def _check_open(self) -> None:
        """Raise :class:`InvalidHandleError` if the handle has been closed."""
        if self._closed or self._ptr is None:
            from ._errors import InvalidHandleError, Status

            raise InvalidHandleError(
                Status.INVALID_HANDLE,
                f"{type(self).__name__}: handle has been closed",
            )

    def _as_ptr(self) -> Any:
        """Return the live cffi pointer; raise if closed."""
        self._check_open()
        return self._ptr

    def __repr__(self) -> str:
        state = "closed" if self._closed else "open"
        return f"<{type(self).__name__} {state}>"

    # Block accidental copying — handles are move-only in spirit.
    def __copy__(self) -> "_Handle":  # pragma: no cover
        raise TypeError(f"{type(self).__name__} cannot be copied")

    def __deepcopy__(self, memo: dict) -> "_Handle":  # pragma: no cover
        raise TypeError(f"{type(self).__name__} cannot be deep-copied")
