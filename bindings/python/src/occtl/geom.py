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

"""POD geometry values + arithmetic helpers — :mod:`occtl.geom`.

The C ABI uses small POD structs (``occtl_point3_t``, ``occtl_vector3_t``, …)
that travel by value. In Python we wrap them as ``@dataclass(frozen=True,
slots=True)`` so they can be hashed and used as dict keys.

Each dataclass has a ``_to_c(ffi)`` method to build a cffi struct on demand,
and a ``_from_c(struct)`` classmethod to read one back. The hand-written
helpers below call the auto-generated wrappers in :mod:`occtl._generated.geom`
for the math operations (distance, midpoint, transform_apply_*, …).
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Tuple

from ._generated import geom as _geom_gen
from ._generated._raw import ffi, lib


@dataclass(frozen=True, slots=True)
class Point2:
    """2D point. Maps to ``occtl_point2_t``."""

    x: float
    y: float

    def _to_c(self):
        c = ffi.new("occtl_point2_t*")
        c.x = float(self.x)
        c.y = float(self.y)
        return c[0]

    @classmethod
    def _from_c(cls, c) -> "Point2":
        return cls(x=float(c.x), y=float(c.y))


@dataclass(frozen=True, slots=True)
class Point3:
    """3D point. Maps to ``occtl_point3_t``."""

    x: float
    y: float
    z: float

    def _to_c(self):
        c = ffi.new("occtl_point3_t*")
        c.x = float(self.x)
        c.y = float(self.y)
        c.z = float(self.z)
        return c[0]

    @classmethod
    def _from_c(cls, c) -> "Point3":
        return cls(x=float(c.x), y=float(c.y), z=float(c.z))

    def as_tuple(self) -> Tuple[float, float, float]:
        return (self.x, self.y, self.z)


@dataclass(frozen=True, slots=True)
class Vector3:
    """3D vector. Maps to ``occtl_vector3_t``."""

    x: float
    y: float
    z: float

    def _to_c(self):
        c = ffi.new("occtl_vector3_t*")
        c.x = float(self.x)
        c.y = float(self.y)
        c.z = float(self.z)
        return c[0]

    @classmethod
    def _from_c(cls, c) -> "Vector3":
        return cls(x=float(c.x), y=float(c.y), z=float(c.z))


@dataclass(frozen=True, slots=True)
class Direction3:
    """Unit-length direction. Maps to ``occtl_direction3_t``."""

    x: float
    y: float
    z: float

    def _to_c(self):
        c = ffi.new("occtl_direction3_t*")
        c.x = float(self.x)
        c.y = float(self.y)
        c.z = float(self.z)
        return c[0]

    @classmethod
    def _from_c(cls, c) -> "Direction3":
        return cls(x=float(c.x), y=float(c.y), z=float(c.z))


@dataclass(frozen=True, slots=True)
class Axis1Placement:
    """1D axis placement: location + axis direction."""

    location: Point3
    direction: Direction3

    def _to_c(self):
        c = ffi.new("occtl_axis1_placement_t*")
        c.location = self.location._to_c()
        c.direction = self.direction._to_c()
        return c[0]


@dataclass(frozen=True, slots=True)
class Axis2Placement:
    """Right-handed coordinate frame: origin + main (X) direction + reference
    direction in the XY plane.

    Mirrors ``occtl_axis2_placement_t``.  The Y direction is derived as the
    component of ``x_dir_ref`` orthogonal to ``x_dir``; Z is x_dir × Y.
    """

    location: Point3
    x_dir: Direction3
    x_dir_ref: Direction3

    @classmethod
    def world(cls) -> "Axis2Placement":
        """Default world frame: origin at (0,0,0), X along +X, X-ref along +Y."""
        return cls(
            location=Point3(0.0, 0.0, 0.0),
            x_dir=Direction3(1.0, 0.0, 0.0),
            x_dir_ref=Direction3(0.0, 1.0, 0.0),
        )

    def _write_into(self, c) -> None:
        """Copy this frame into a cffi-allocated ``occtl_axis2_placement_t``."""
        c.location.x = float(self.location.x)
        c.location.y = float(self.location.y)
        c.location.z = float(self.location.z)
        c.x_dir.x = float(self.x_dir.x)
        c.x_dir.y = float(self.x_dir.y)
        c.x_dir.z = float(self.x_dir.z)
        c.x_dir_ref.x = float(self.x_dir_ref.x)
        c.x_dir_ref.y = float(self.x_dir_ref.y)
        c.x_dir_ref.z = float(self.x_dir_ref.z)


@dataclass(frozen=True, slots=True)
class Axis3Placement:
    """Right-handed coordinate system: location + three orthonormal axes."""

    location: Point3
    x_dir: Direction3
    y_dir: Direction3
    z_dir: Direction3


@dataclass(frozen=True, slots=True)
class Transform:
    """A 3D rigid + uniform-scale transform (3×4 matrix)."""

    matrix: Tuple[float, ...]  # 12 doubles, row-major

    def __post_init__(self) -> None:
        if len(self.matrix) != 12:
            raise ValueError("Transform.matrix must contain exactly 12 values")

    def _to_c(self):
        c = ffi.new("occtl_transform_t*")
        for i, value in enumerate(self.matrix):
            c.m[i] = float(value)
        return c[0]

    @classmethod
    def _from_c(cls, c) -> "Transform":
        return cls(tuple(float(c.m[i]) for i in range(12)))


def identity_transform() -> Transform:
    """Return the identity transform."""
    return Transform._from_c(lib.occtl_transform_identity())


def translation(delta: Vector3) -> Transform:
    """Return a pure-translation transform."""
    return Transform._from_c(lib.occtl_transform_translation(delta._to_c()))


def rotation(axis: Axis1Placement, angle: float) -> Transform:
    """Return a rotation transform around ``axis`` by ``angle`` radians."""
    out = ffi.new("occtl_transform_t*")
    from ._errors import _check

    _check(lib.occtl_transform_rotation(axis._to_c(), float(angle), out))
    return Transform._from_c(out[0])


def scale(center: Point3, factor: float) -> Transform:
    """Return a uniform-scale transform centred on ``center``."""
    out = ffi.new("occtl_transform_t*")
    from ._errors import _check

    _check(lib.occtl_transform_scale(center._to_c(), float(factor), out))
    return Transform._from_c(out[0])


# Re-export every generated wrapper so the symbol-coverage check can find
# them. The hand-written API surface above is the recommended one; the raw
# ``occtl_*`` entry points stay available as escape hatches.
for _name in dir(_geom_gen):
    if _name.startswith("occtl_"):
        globals()[_name] = getattr(_geom_gen, _name)

del _name
