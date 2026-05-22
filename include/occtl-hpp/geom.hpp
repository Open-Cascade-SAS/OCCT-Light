// Copyright (c) 2026 Capgemini Engineering Research and Development.
//
// This file is part of OCCT-Light software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License version 3 as published
// by the Free Software Foundation, with an option to use any later version.
// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
// for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of a commercial
// license or contractual agreement.
//
// SPDX-License-Identifier: AGPL-3.0-or-later

/**
 * @file
 * @brief C++ veneer for the geom module.
 *
 * Header-only value wrappers and helpers over the C ABI. The public API is
 * STL-shaped while local identifiers follow OCCT style.
 */

#ifndef OCCTL_HPP_GEOM_HPP
#define OCCTL_HPP_GEOM_HPP

#include <occtl/occtl_geom.h>

#include <occtl-hpp/core.hpp>

#include <cmath>

namespace occtl
{

/// @brief 2D point value type. Mirrors @c occtl_point2_t with STL-flavoured access.
struct Point2
{
  /// @brief Default-constructs the origin (0, 0).
  Point2() noexcept
      : myData{0.0, 0.0}
  {
  }

  /// @brief Constructs from explicit coordinates.
  Point2(const double theX, const double theY) noexcept
      : myData{theX, theY}
  {
  }

  /// @brief Wraps an existing C value type (zero-cost).
  explicit Point2(const occtl_point2_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Borrows-it view of the underlying C value type, for direct ABI calls.
  const occtl_point2_t& c_type() const noexcept { return myData; }

  double x() const noexcept { return myData.x; } ///< X coordinate.

  double y() const noexcept { return myData.y; } ///< Y coordinate.

  /// @brief Euclidean distance to another point. Never throws.
  double distance_to(const Point2& theOther) const noexcept
  {
    return ::occtl_point2_distance(myData, theOther.myData);
  }

  /// @brief Midpoint between this and another point. Never throws.
  Point2 midpoint(const Point2& theOther) const noexcept
  {
    return Point2(::occtl_point2_midpoint(myData, theOther.myData));
  }

private:
  occtl_point2_t myData;
};

/// @brief 3D point value type. Mirrors @c occtl_point3_t with STL-flavoured access.
struct Point3
{
  /// @brief Default-constructs the origin (0, 0, 0).
  Point3() noexcept
      : myData{0.0, 0.0, 0.0}
  {
  }

  /// @brief Constructs from explicit coordinates.
  Point3(const double theX, const double theY, const double theZ) noexcept
      : myData{theX, theY, theZ}
  {
  }

  /// @brief Wraps an existing C value type (zero-cost).
  explicit Point3(const occtl_point3_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Borrows-it view of the underlying C value type, for direct ABI calls.
  const occtl_point3_t& c_type() const noexcept { return myData; }

  double x() const noexcept { return myData.x; } ///< X coordinate.

  double y() const noexcept { return myData.y; } ///< Y coordinate.

  double z() const noexcept { return myData.z; } ///< Z coordinate.

  /// @brief Euclidean distance to another point. Never throws.
  double distance_to(const Point3& theOther) const noexcept
  {
    return ::occtl_point3_distance(myData, theOther.myData);
  }

  /// @brief Midpoint between this and another point. Never throws.
  Point3 midpoint(const Point3& theOther) const noexcept
  {
    return Point3(::occtl_point3_midpoint(myData, theOther.myData));
  }

private:
  occtl_point3_t myData;
};

/// @brief 2D free-vector value type. Mirrors @c occtl_vector2_t.
struct Vector2
{
  /// @brief Default-constructs the zero vector.
  Vector2() noexcept
      : myData{0.0, 0.0}
  {
  }

  /// @brief Constructs from explicit components.
  Vector2(const double theX, const double theY) noexcept
      : myData{theX, theY}
  {
  }

  /// @brief Wraps an existing C value type (zero-cost).
  explicit Vector2(const occtl_vector2_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Borrows-it view of the underlying C value type.
  const occtl_vector2_t& c_type() const noexcept { return myData; }

  double x() const noexcept { return myData.x; } ///< X component.

  double y() const noexcept { return myData.y; } ///< Y component.

  /// @brief Dot product with another vector. Never throws.
  double dot(const Vector2& theOther) const noexcept
  {
    return ::occtl_vector2_dot(myData, theOther.myData);
  }

  /// @brief Signed scalar cross product (Z-component). Never throws.
  double cross(const Vector2& theOther) const noexcept
  {
    return ::occtl_vector2_cross(myData, theOther.myData);
  }

  /// @brief Euclidean length. Never throws.
  double magnitude() const noexcept { return ::occtl_vector2_magnitude(myData); }

  /// @brief Returns this vector normalised to unit length.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when this vector has zero length.
  Vector2 normalized() const
  {
    occtl_vector2_t aResult{};
    check(::occtl_vector2_normalized(myData, &aResult));
    return Vector2(aResult);
  }

private:
  occtl_vector2_t myData;
};

/// @brief 3D free-vector value type. Mirrors @c occtl_vector3_t.
struct Vector3
{
  /// @brief Default-constructs the zero vector.
  Vector3() noexcept
      : myData{0.0, 0.0, 0.0}
  {
  }

  /// @brief Constructs from explicit components.
  Vector3(const double theX, const double theY, const double theZ) noexcept
      : myData{theX, theY, theZ}
  {
  }

  /// @brief Wraps an existing C value type (zero-cost).
  explicit Vector3(const occtl_vector3_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Borrows-it view of the underlying C value type.
  const occtl_vector3_t& c_type() const noexcept { return myData; }

  double x() const noexcept { return myData.x; } ///< X component.

  double y() const noexcept { return myData.y; } ///< Y component.

  double z() const noexcept { return myData.z; } ///< Z component.

  /// @brief Dot product with another vector. Never throws.
  double dot(const Vector3& theOther) const noexcept
  {
    return ::occtl_vector3_dot(myData, theOther.myData);
  }

  /// @brief Cross product with another vector. Never throws.
  Vector3 cross(const Vector3& theOther) const noexcept
  {
    return Vector3(::occtl_vector3_cross(myData, theOther.myData));
  }

  /// @brief Euclidean length. Never throws.
  double magnitude() const noexcept { return ::occtl_vector3_magnitude(myData); }

  /// @brief Returns this vector normalised to unit length.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when this vector has zero length.
  Vector3 normalized() const
  {
    occtl_vector3_t aResult{};
    check(::occtl_vector3_normalized(myData, &aResult));
    return Vector3(aResult);
  }

  /// @brief Component-wise sum.
  Vector3 operator+(const Vector3& theOther) const noexcept
  {
    return Vector3(::occtl_vector3_add(myData, theOther.myData));
  }

  /// @brief Component-wise difference.
  Vector3 operator-(const Vector3& theOther) const noexcept
  {
    return Vector3(::occtl_vector3_sub(myData, theOther.myData));
  }

  /// @brief Scalar multiplication.
  Vector3 operator*(const double theS) const noexcept
  {
    return Vector3(::occtl_vector3_scaled(myData, theS));
  }

  /// @brief Returns this vector negated.
  Vector3 reversed() const noexcept { return Vector3(::occtl_vector3_reversed(myData)); }

  /// @brief Angle in radians between this and @c theOther, in @c [0, π].
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when either vector has zero length.
  double angle(const Vector3& theOther) const
  {
    double aRad = 0.0;
    check(::occtl_vector3_angle(myData, theOther.myData, &aRad));
    return aRad;
  }

private:
  occtl_vector3_t myData;
};

/// @brief 2D unit-direction value type. Mirrors @c occtl_direction2_t.
///
/// There is no default constructor: a default direction is not meaningful.
/// Use #from_vector to build one from a non-zero @c Vector2.
struct Direction2
{
  /// @brief Wraps an existing C value type (zero-cost). Caller guarantees unit norm.
  explicit Direction2(const occtl_direction2_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Builds a normalised direction from a vector.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when @c theV has zero length.
  static Direction2 from_vector(const Vector2& theV)
  {
    occtl_direction2_t aResult{};
    check(::occtl_direction2_from_vector(theV.c_type(), &aResult));
    return Direction2(aResult);
  }

  /// @brief Borrows-it view of the underlying C value type.
  const occtl_direction2_t& c_type() const noexcept { return myData; }

  double x() const noexcept { return myData.x; } ///< X component (unit-normalised).

  double y() const noexcept { return myData.y; } ///< Y component (unit-normalised).

  /// @brief Angle in radians between this and @c theOther, in @c [0, π]. Never throws.
  double angle(const Direction2& theOther) const noexcept
  {
    return ::occtl_direction2_angle(myData, theOther.myData);
  }

private:
  occtl_direction2_t myData;
};

/// @brief 3D unit-direction value type. Mirrors @c occtl_direction3_t.
///
/// There is no default constructor: a default direction is not meaningful.
/// Use #from_vector to build one from a non-zero @c Vector3.
struct Direction3
{
  /// @brief Wraps an existing C value type (zero-cost). Caller guarantees unit norm.
  explicit Direction3(const occtl_direction3_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Builds a normalised direction from a vector.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when @c theV has zero length.
  static Direction3 from_vector(const Vector3& theV)
  {
    occtl_direction3_t aResult{};
    check(::occtl_direction3_from_vector(theV.c_type(), &aResult));
    return Direction3(aResult);
  }

  /// @brief Borrows-it view of the underlying C value type.
  const occtl_direction3_t& c_type() const noexcept { return myData; }

  double x() const noexcept { return myData.x; } ///< X component (unit-normalised).

  double y() const noexcept { return myData.y; } ///< Y component (unit-normalised).

  double z() const noexcept { return myData.z; } ///< Z component (unit-normalised).

  /// @brief Dot product. Returns a value in @c [-1, 1]. Never throws.
  double dot(const Direction3& theOther) const noexcept
  {
    return ::occtl_direction3_dot(myData, theOther.myData);
  }

  /// @brief Cross product as a free vector. Zero for parallel/anti-parallel inputs.
  Vector3 cross(const Direction3& theOther) const noexcept
  {
    return Vector3(::occtl_direction3_cross(myData, theOther.myData));
  }

  /// @brief Angle in radians, in @c [0, π]. Never throws.
  double angle(const Direction3& theOther) const noexcept
  {
    return ::occtl_direction3_angle(myData, theOther.myData);
  }

  /// @brief Returns this direction negated. Never throws.
  Direction3 reversed() const noexcept { return Direction3(::occtl_direction3_reversed(myData)); }

private:
  occtl_direction3_t myData;
};

/// @brief Directed line in 3D (origin + unit direction). Mirrors @c occtl_axis1_placement_t.
struct Axis1Placement
{
  /// @brief Wraps an existing C value type (zero-cost).
  explicit Axis1Placement(const occtl_axis1_placement_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Constructs from origin and direction.
  Axis1Placement(const Point3& theLocation, const Direction3& theDir) noexcept
      : myData{theLocation.c_type(), theDir.c_type()}
  {
  }

  /// @brief Borrows-it view of the underlying C value type.
  const occtl_axis1_placement_t& c_type() const noexcept { return myData; }

private:
  occtl_axis1_placement_t myData;
};

/// @brief Right-handed coordinate frame (origin + main direction + reference direction).
/// Mirrors @c occtl_axis2_placement_t.
struct Axis2Placement
{
  /// @brief Wraps an existing C value type (zero-cost).
  explicit Axis2Placement(const occtl_axis2_placement_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Constructs from origin, X direction, and an in-plane reference direction.
  /// @c theXDirRef must not be parallel to @c theXDir; the Y axis is derived as the
  /// component of @c theXDirRef orthogonal to @c theXDir.
  Axis2Placement(const Point3&     theLocation,
                 const Direction3& theXDir,
                 const Direction3& theXDirRef) noexcept
      : myData{theLocation.c_type(), theXDir.c_type(), theXDirRef.c_type()}
  {
  }

  /// @brief Borrows-it view of the underlying C value type.
  const occtl_axis2_placement_t& c_type() const noexcept { return myData; }

private:
  occtl_axis2_placement_t myData;
};

/// @brief Explicit 3-axis coordinate frame. Mirrors @c occtl_axis3_placement_t.
/// No orthonormality or right-handedness is enforced.
struct Axis3Placement
{
  /// @brief Wraps an existing C value type (zero-cost).
  explicit Axis3Placement(const occtl_axis3_placement_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Constructs from origin and three explicit axis directions.
  Axis3Placement(const Point3&     theLocation,
                 const Direction3& theXDir,
                 const Direction3& theYDir,
                 const Direction3& theZDir) noexcept
      : myData{theLocation.c_type(), theXDir.c_type(), theYDir.c_type(), theZDir.c_type()}
  {
  }

  /// @brief Borrows-it view of the underlying C value type.
  const occtl_axis3_placement_t& c_type() const noexcept { return myData; }

private:
  occtl_axis3_placement_t myData;
};

/// @brief 3-by-4 affine transform value type. Mirrors @c occtl_transform_t.
///
/// Composition convention: see #compose.
struct Transform
{
  /// @brief Wraps an existing C value type (zero-cost).
  explicit Transform(const occtl_transform_t& theC) noexcept
      : myData(theC)
  {
  }

  /// @brief Returns the identity transform.
  static Transform identity() noexcept { return Transform(::occtl_transform_identity()); }

  /// @brief Returns a pure-translation transform.
  static Transform translation(const Vector3& theV) noexcept
  {
    return Transform(::occtl_transform_translation(theV.c_type()));
  }

  /// @brief Returns a rotation transform around @c theAxis by @c theAngle radians.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when the axis direction is zero.
  static Transform rotation(const Axis1Placement& theAxis, const double theAngle)
  {
    occtl_transform_t aResult{};
    check(::occtl_transform_rotation(theAxis.c_type(), theAngle, &aResult));
    return Transform(aResult);
  }

  /// @brief Returns a uniform-scale transform centred on @c theCenter.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when @c theFactor is zero.
  static Transform scale(const Point3& theCenter, const double theFactor)
  {
    occtl_transform_t aResult{};
    check(::occtl_transform_scale(theCenter.c_type(), theFactor, &aResult));
    return Transform(aResult);
  }

  /// @brief Returns the world→frame transform for the given coordinate frame.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when the frame is degenerate.
  static Transform from_axis2(const Axis2Placement& theFrame)
  {
    occtl_transform_t aResult{};
    check(::occtl_transform_from_axis2(theFrame.c_type(), &aResult));
    return Transform(aResult);
  }

  /// @brief Returns the transform whose linear columns are the explicit axes.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when the three axes are linearly dependent.
  static Transform from_axis3(const Axis3Placement& theFrame)
  {
    occtl_transform_t aResult{};
    check(::occtl_transform_from_axis3(theFrame.c_type(), &aResult));
    return Transform(aResult);
  }

  /// @brief Borrows-it view of the underlying C value type.
  const occtl_transform_t& c_type() const noexcept { return myData; }

  /// @brief Applies the full affine transform to a point (includes translation).
  Point3 apply(const Point3& theP) const noexcept
  {
    return Point3(::occtl_transform_apply_point3(myData, theP.c_type()));
  }

  /// @brief Applies only the linear (3×3) part to a vector. No translation.
  Vector3 apply(const Vector3& theV) const noexcept
  {
    return Vector3(::occtl_transform_apply_vector3(myData, theV.c_type()));
  }

  /// @brief Applies the linear part to a unit direction and re-normalises the result.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when the transform collapses the direction to
  /// zero.
  Direction3 apply(const Direction3& theD) const
  {
    occtl_direction3_t aResult{};
    check(::occtl_direction3_transform(theD.c_type(), myData, &aResult));
    return Direction3(aResult);
  }

  /// @brief Composition: returns "apply *this first, then @c theSecond".
  /// Equivalent to the matrix product @c theSecond * @c *this.
  Transform compose(const Transform& theSecond) const noexcept
  {
    return Transform(::occtl_transform_compose(myData, theSecond.myData));
  }

  /// @brief Returns the inverse transform.
  /// @throws Error with code OCCTL_GEOMETRY_INVALID when @c *this is singular.
  Transform inverted() const
  {
    occtl_transform_t aResult{};
    check(::occtl_transform_inverted(myData, &aResult));
    return Transform(aResult);
  }

private:
  occtl_transform_t myData;
};

} // namespace occtl

#endif // OCCTL_HPP_GEOM_HPP
