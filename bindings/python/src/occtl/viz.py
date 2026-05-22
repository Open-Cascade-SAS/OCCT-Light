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

"""Graph-native visualization — :mod:`occtl.viz`.

The generated symbols expose the full C ABI.  The small ergonomic layer below
adds Python-owned handles for offscreen rendering workflows.
"""

from __future__ import annotations

from dataclasses import dataclass

from ._generated import viz as _viz_gen
from ._errors import _check
from ._generated._raw import ffi, lib

_SENTINEL = object()
for _name in dir(_viz_gen):
    if _name.startswith("occtl_"):
        if globals().get(_name, _SENTINEL) is _SENTINEL:
            globals()[_name] = getattr(_viz_gen, _name)
        else:
            import warnings
            warnings.warn(
                f"occtl.viz: skipping {_name} — already defined in viz namespace",
                stacklevel=2,
            )
del _name, _SENTINEL


@dataclass(frozen=True)
class ViewSize:
    width: int = 640
    height: int = 480


class Driver:
    def __init__(self) -> None:
        self._ptr = None
        out = ffi.new("occtl_viz_driver_t**")
        _check(lib.occtl_viz_driver_create(ffi.NULL, out))
        self._ptr = out[0]

    def close(self) -> None:
        if self._ptr is not None:
            occtl_viz_driver_free(self._ptr)
            self._ptr = None

    def __del__(self) -> None:
        self.close()

    def as_ptr(self):
        if self._ptr is None:
            raise RuntimeError("Driver is closed")
        return self._ptr


class Viewer:
    def __init__(self, driver: Driver) -> None:
        self._ptr = None
        out = ffi.new("occtl_viz_viewer_t**")
        _check(lib.occtl_viz_viewer_create(driver.as_ptr(), out))
        self._ptr = out[0]

    def close(self) -> None:
        if self._ptr is not None:
            occtl_viz_viewer_free(self._ptr)
            self._ptr = None

    def __del__(self) -> None:
        self.close()

    def as_ptr(self):
        if self._ptr is None:
            raise RuntimeError("Viewer is closed")
        return self._ptr


class View:
    def __init__(self, viewer: Viewer, size: ViewSize = ViewSize()) -> None:
        self._ptr = None
        self.size = size
        opts = ffi.new("occtl_viz_view_options_t*")
        lib.occtl_viz_view_options_init(opts)
        opts.width = int(size.width)
        opts.height = int(size.height)
        opts.offscreen = 1
        out = ffi.new("occtl_viz_view_t**")
        _check(lib.occtl_viz_view_create(viewer.as_ptr(), opts, out))
        self._ptr = out[0]

    def close(self) -> None:
        if self._ptr is not None:
            occtl_viz_view_free(self._ptr)
            self._ptr = None

    def __del__(self) -> None:
        self.close()

    def as_ptr(self):
        if self._ptr is None:
            raise RuntimeError("View is closed")
        return self._ptr

    def display(self, presentable: "Presentable") -> None:
        _check(lib.occtl_viz_view_display(self.as_ptr(), presentable.as_ptr()))

    def fit_all(self) -> None:
        _check(lib.occtl_viz_view_fit_all(self.as_ptr()))

    def redraw(self) -> None:
        _check(lib.occtl_viz_view_redraw(self.as_ptr()))

    def read_pixels_rgba(self) -> bytes:
        out_count = ffi.new("size_t*")
        _check(lib.occtl_viz_view_read_pixels_rgba(self.as_ptr(), ffi.NULL, 0, out_count))
        count = int(out_count[0])
        data = bytearray(count)
        buf = ffi.from_buffer(data)
        _check(lib.occtl_viz_view_read_pixels_rgba(self.as_ptr(), buf, len(data), out_count))
        return bytes(data)

    def dump_image(self, path: str) -> None:
        _check(lib.occtl_viz_view_dump_image(self.as_ptr(), path.encode("utf-8")))


class Presentable:
    def __init__(self, viewer: Viewer, graph, root) -> None:
        self._ptr = None
        out = ffi.new("occtl_viz_presentable_t**")
        root_c = ffi.new("occtl_node_id_t*")
        root_c.bits = int(root.bits if hasattr(root, "bits") else root)
        _check(lib.occtl_viz_presentable_create(viewer.as_ptr(), graph._as_ptr(), root_c[0], out))
        self._ptr = out[0]

    def close(self) -> None:
        if self._ptr is not None:
            occtl_viz_presentable_free(self._ptr)
            self._ptr = None

    def __del__(self) -> None:
        self.close()

    def as_ptr(self):
        if self._ptr is None:
            raise RuntimeError("Presentable is closed")
        return self._ptr

    def synchronize(self) -> bool:
        changed = ffi.new("int32_t*")
        _check(lib.occtl_viz_presentable_synchronize(self.as_ptr(), changed))
        return bool(changed[0])
