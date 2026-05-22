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
 * @file occtl_geom.h
 * @brief OCCT-Light: geometry primitive types and math utilities.
 *
 * Exposes nine POD value types — 2D/3D points, vectors, unit directions,
 * axis-placement coordinate systems, and a 3-by-4 affine transform — together
 * with pure-math utility functions that operate on them.  All types are plain
 * C structs; no allocation, no handles, no ownership semantics.
 *
 * Type name conventions follow the design documents (full English words, not
 * OCCT-internal abbreviations) and the ISO 10303 STEP "axis placement"
 * terminology for coordinate-system types.
 *
 * All functions in this header are thread-safe: they perform stateless
 * floating-point arithmetic on value-typed arguments; the only shared state
 * they touch (on failure) is the caller's thread-local error slot.
 */

#ifndef OCCTL_GEOM_H
#define OCCTL_GEOM_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 2D point. Coordinates in whatever unit the caller uses. */
typedef struct
{
  double x; /**< X coordinate. */
  double y; /**< Y coordinate. */
} occtl_point2_t;

/** 2D free vector. */
typedef struct
{
  double x; /**< X component. */
  double y; /**< Y component. */
} occtl_vector2_t;

/**
 * 2D unit-direction vector.
 *
 * Invariant: @c x²+y²=1 within floating-point precision.  Constructing a
 * literal with non-unit coordinates is undefined behaviour.  Use
 * #occtl_direction2_from_vector to build one safely.
 */
typedef struct
{
  double x; /**< X component (normalised). */
  double y; /**< Y component (normalised). */
} occtl_direction2_t;

/** 3D point. Coordinates in whatever unit the caller uses. */
typedef struct
{
  double x; /**< X coordinate. */
  double y; /**< Y coordinate. */
  double z; /**< Z coordinate. */
} occtl_point3_t;

/** 3D free vector. */
typedef struct
{
  double x; /**< X component. */
  double y; /**< Y component. */
  double z; /**< Z component. */
} occtl_vector3_t;

/**
 * 3D unit-direction vector.
 *
 * Invariant: @c x²+y²+z²=1 within floating-point precision.  Constructing a
 * literal with non-unit coordinates is undefined behaviour.  Use
 * #occtl_direction3_from_vector to build one safely.
 */
typedef struct
{
  double x; /**< X component (normalised). */
  double y; /**< Y component (normalised). */
  double z; /**< Z component (normalised). */
} occtl_direction3_t;

/**
 * 1-axis placement — a directed line (STEP entity AXIS1_PLACEMENT).
 *
 * Defines a unique axis in 3D space: an origin point and a unit-direction
 * vector.  Used as the rotation axis for #occtl_transform_rotation.
 */
typedef struct
{
  occtl_point3_t     location;  /**< Origin of the axis. */
  occtl_direction3_t direction; /**< Unit direction along the axis. */
} occtl_axis1_placement_t;

/**
 * 2-axis placement — a right-handed coordinate frame (STEP entity
 * AXIS2_PLACEMENT_3D).
 *
 * Stores an origin, an X (main) direction, and a reference direction in the
 * XY plane.  The Y direction is derived as the component of @c x_dir_ref
 * orthogonal to @c x_dir; the Z direction is @c x_dir × Y.
 *
 * @note @c x_dir_ref must not be parallel to @c x_dir, otherwise
 *       #occtl_transform_from_axis2 returns OCCTL_GEOMETRY_INVALID.
 */
typedef struct
{
  occtl_point3_t     location; /**< Origin of the frame. */
  occtl_direction3_t x_dir;    /**< Main (X) axis direction. */
  occtl_direction3_t
    x_dir_ref; /**< Reference direction in the XY plane (not necessarily orthogonal to x_dir). */
} occtl_axis2_placement_t;

/**
 * 3-axis placement — a general coordinate frame with all three axes stored
 * explicitly (no derivation).  Use this when the consumer needs to round-trip
 * an arbitrary, possibly left-handed, frame.
 *
 * @note The library does not enforce orthonormality or right-handedness; it is
 *       the caller's responsibility to provide valid directions.
 *       #occtl_transform_from_axis3 validates and returns
 *       OCCTL_GEOMETRY_INVALID for a singular (linearly dependent) frame.
 */
typedef struct
{
  occtl_point3_t     location; /**< Origin of the frame. */
  occtl_direction3_t x_dir;    /**< X axis direction. */
  occtl_direction3_t y_dir;    /**< Y axis direction. */
  occtl_direction3_t z_dir;    /**< Z axis direction. */
} occtl_axis3_placement_t;

/**
 * 3-by-4 affine transform, row-major, double precision.
 *
 * Represents the linear part and translation of an affine map from 3D to 3D.
 * Layout (row indices 0–2, column indices 0–3):
 *
 * @code
 *   m[0..3]  — row 0: [a00 a01 a02 tx]
 *   m[4..7]  — row 1: [a10 a11 a12 ty]
 *   m[8..11] — row 2: [a20 a21 a22 tz]
 * @endcode
 *
 * The implicit last row is @c [0 0 0 1].  #occtl_transform_identity returns a
 * valid identity transform.  The rotation sub-matrix is not guaranteed
 * orthonormal unless the transform was produced by #occtl_transform_rotation
 * or composed from other valid transforms.
 */
typedef struct
{
  double m[12]; /**< Row-major 3×4 coefficients. */
} occtl_transform_t;

/**
 * Euclidean distance between two 3D points.
 *
 * @param[in] a  First point.
 * @param[in] b  Second point.
 *
 * @return Non-negative distance; zero when @c a == @c b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_point3_midpoint, occtl_vector3_magnitude
 */
OCCTL_API double OCCTL_CALL occtl_point3_distance(occtl_point3_t a, occtl_point3_t b);

/**
 * Midpoint of two 3D points.
 *
 * @param[in] a  First point.
 * @param[in] b  Second point.
 *
 * @return Point halfway between @c a and @c b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_point3_distance, occtl_point3_translate
 */
OCCTL_API occtl_point3_t OCCTL_CALL occtl_point3_midpoint(occtl_point3_t a, occtl_point3_t b);

/**
 * Translates a 3D point by a vector.
 *
 * @param[in] p  Point to translate.
 * @param[in] v  Translation vector.
 *
 * @return @c p + @c v.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_apply_point3, occtl_transform_translation
 */
OCCTL_API occtl_point3_t OCCTL_CALL occtl_point3_translate(occtl_point3_t p, occtl_vector3_t v);

/**
 * Applies the full affine transform to a 3D point (includes translation).
 *
 * @param[in] t  Transform to apply.
 * @param[in] p  Point to transform.
 *
 * @return Transformed point.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_apply_vector3, occtl_transform_identity, occtl_point3_translate
 */
OCCTL_API occtl_point3_t OCCTL_CALL occtl_transform_apply_point3(occtl_transform_t t,
                                                                 occtl_point3_t    p);

/**
 * Euclidean distance between two 2D points.
 *
 * @param[in] a  First point.
 * @param[in] b  Second point.
 *
 * @return Non-negative distance; zero when @c a == @c b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_point2_midpoint, occtl_point3_distance
 */
OCCTL_API double OCCTL_CALL occtl_point2_distance(occtl_point2_t a, occtl_point2_t b);

/**
 * Midpoint of two 2D points.
 *
 * @param[in] a  First point.
 * @param[in] b  Second point.
 *
 * @return Point halfway between @c a and @c b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_point2_distance, occtl_point3_midpoint
 */
OCCTL_API occtl_point2_t OCCTL_CALL occtl_point2_midpoint(occtl_point2_t a, occtl_point2_t b);

/**
 * Dot product of two 3D vectors.
 *
 * @param[in] a  First vector.
 * @param[in] b  Second vector.
 *
 * @return @c a·b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_cross, occtl_direction3_dot
 */
OCCTL_API double OCCTL_CALL occtl_vector3_dot(occtl_vector3_t a, occtl_vector3_t b);

/**
 * Cross product of two 3D vectors.
 *
 * @param[in] a  First vector.
 * @param[in] b  Second vector.
 *
 * @return @c a×b.  Result is the zero vector when @c a and @c b are parallel.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_dot, occtl_direction3_cross
 */
OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_cross(occtl_vector3_t a, occtl_vector3_t b);

/**
 * Euclidean magnitude (length) of a 3D vector.
 *
 * @param[in] v  Vector.
 *
 * @return Non-negative length; zero for the zero vector.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_normalized, occtl_vector2_magnitude
 */
OCCTL_API double OCCTL_CALL occtl_vector3_magnitude(occtl_vector3_t v);

/**
 * Normalises a 3D vector to unit length.
 *
 * @param[in]  v           Vector to normalise.
 * @param[out] out_result  Owns it (caller-allocated output slot).  Must be
 *                         non-NULL.  On success receives the unit-length
 *                         vector; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_result is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  @c v has zero (or near-zero) length.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_magnitude, occtl_direction3_from_vector
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_vector3_normalized(occtl_vector3_t  v,
                                                             occtl_vector3_t* out_result);

/**
 * Angle between two 3D vectors, in radians.
 *
 * @param[in]  a           First vector.
 * @param[in]  b           Second vector.
 * @param[out] out_radians Owns it (caller-allocated output slot).  Must be
 *                         non-NULL.  On success receives a value in
 *                         @c [0, π]; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_radians is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  Either vector has zero length.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_direction3_angle
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_vector3_angle(occtl_vector3_t a,
                                                        occtl_vector3_t b,
                                                        double*         out_radians);

/**
 * Adds two 3D vectors.
 *
 * @param[in] a  First vector.
 * @param[in] b  Second vector.
 *
 * @return @c a + @c b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_sub, occtl_vector3_scaled
 */
OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_add(occtl_vector3_t a, occtl_vector3_t b);

/**
 * Subtracts two 3D vectors.
 *
 * @param[in] a  Minuend.
 * @param[in] b  Subtrahend.
 *
 * @return @c a - @c b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_add, occtl_vector3_reversed
 */
OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_sub(occtl_vector3_t a, occtl_vector3_t b);

/**
 * Scales a 3D vector by a scalar.
 *
 * @param[in] v  Vector.
 * @param[in] s  Scalar factor.
 *
 * @return @c s*v.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_magnitude, occtl_vector3_add
 */
OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_scaled(occtl_vector3_t v, double s);

/**
 * Reverses a 3D vector.
 *
 * @param[in] v  Vector.
 *
 * @return @c -v.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_direction3_reversed, occtl_vector3_scaled
 */
OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_reversed(occtl_vector3_t v);

/**
 * Dot product of two 2D vectors.
 *
 * @param[in] a  First vector.
 * @param[in] b  Second vector.
 *
 * @return @c a·b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector2_cross, occtl_vector3_dot
 */
OCCTL_API double OCCTL_CALL occtl_vector2_dot(occtl_vector2_t a, occtl_vector2_t b);

/**
 * Signed Z-component of the cross product of two 2D vectors.
 *
 * Equivalent to the determinant @c a.x*b.y - a.y*b.x.  Positive when @c b is
 * counter-clockwise from @c a.
 *
 * @param[in] a  First vector.
 * @param[in] b  Second vector.
 *
 * @return Scalar Z of @c a×b.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector2_dot, occtl_vector3_cross
 */
OCCTL_API double OCCTL_CALL occtl_vector2_cross(occtl_vector2_t a, occtl_vector2_t b);

/**
 * Euclidean magnitude of a 2D vector.
 *
 * @param[in] v  Vector.
 *
 * @return Non-negative length.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector2_normalized, occtl_vector3_magnitude
 */
OCCTL_API double OCCTL_CALL occtl_vector2_magnitude(occtl_vector2_t v);

/**
 * Normalises a 2D vector to unit length.
 *
 * @param[in]  v           Vector.
 * @param[out] out_result  Owns it (caller-allocated output slot).  Must be
 *                         non-NULL.  On success receives the unit-length
 *                         vector; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_result is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  @c v has zero length.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector2_magnitude, occtl_direction2_from_vector
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_vector2_normalized(occtl_vector2_t  v,
                                                             occtl_vector2_t* out_result);

/**
 * Builds a unit-direction from a 3D vector, normalising it.
 *
 * @param[in]  v              Source vector.
 * @param[out] out_direction  Owns it (caller-allocated output slot).  Must be
 *                            non-NULL.  On success receives the normalised
 *                            direction; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_direction is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  @c v has zero (or near-zero) length.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_normalized, occtl_direction2_from_vector
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_direction3_from_vector(occtl_vector3_t     v,
                                                                 occtl_direction3_t* out_direction);

/**
 * Dot product of two unit directions.
 *
 * @param[in] a  First direction.
 * @param[in] b  Second direction.
 *
 * @return Value in @c [-1, 1].
 *
 * @threadsafe Yes.
 *
 * @sa occtl_direction3_angle, occtl_vector3_dot
 */
OCCTL_API double OCCTL_CALL occtl_direction3_dot(occtl_direction3_t a, occtl_direction3_t b);

/**
 * Cross product of two unit directions, returned as a free vector.
 *
 * The result is the zero vector when @c a and @c b are parallel or
 * anti-parallel.
 *
 * @param[in] a  First direction.
 * @param[in] b  Second direction.
 *
 * @return @c a×b as a free @c occtl_vector3_t.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_cross, occtl_direction3_dot
 */
OCCTL_API occtl_vector3_t OCCTL_CALL occtl_direction3_cross(occtl_direction3_t a,
                                                            occtl_direction3_t b);

/**
 * Angle between two unit directions, in radians.
 *
 * Uses @c acos(a·b), clamped to @c [0, π] to avoid domain errors from
 * floating-point rounding.
 *
 * @param[in] a  First direction.
 * @param[in] b  Second direction.
 *
 * @return Angle in @c [0, π].
 *
 * @threadsafe Yes.
 *
 * @sa occtl_direction3_dot, occtl_vector3_angle
 */
OCCTL_API double OCCTL_CALL occtl_direction3_angle(occtl_direction3_t a, occtl_direction3_t b);

/**
 * Reverses a unit direction.
 *
 * @param[in] d  Direction.
 *
 * @return @c -d.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector3_reversed
 */
OCCTL_API occtl_direction3_t OCCTL_CALL occtl_direction3_reversed(occtl_direction3_t d);

/**
 * Applies the linear (rotation/scale) part of a transform to a unit direction,
 * then re-normalises the result.
 *
 * Translation is not applied; only the 3×3 sub-matrix of @c t is used.
 *
 * @param[in]  d              Direction to transform.
 * @param[in]  t              Transform.
 * @param[out] out_direction  Owns it (caller-allocated output slot).  Must be
 *                            non-NULL.  On success receives the transformed
 *                            and re-normalised direction; left unchanged on
 *                            failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_direction is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  The transform collapses the direction to zero
 *                                 (e.g. a zero-scale transform).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_apply_vector3, occtl_transform_apply_point3
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_direction3_transform(occtl_direction3_t  d,
                                                               occtl_transform_t   t,
                                                               occtl_direction3_t* out_direction);

/**
 * Builds a 2D unit-direction from a 2D vector, normalising it.
 *
 * @param[in]  v              Source vector.
 * @param[out] out_direction  Owns it (caller-allocated output slot).  Must be
 *                            non-NULL.  On success receives the normalised
 *                            direction; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_direction is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  @c v has zero length.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_vector2_normalized, occtl_direction3_from_vector
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_direction2_from_vector(occtl_vector2_t     v,
                                                                 occtl_direction2_t* out_direction);

/**
 * Angle from direction @c a to direction @c b, in radians, in @c [0, π].
 *
 * @param[in] a  First direction.
 * @param[in] b  Second direction.
 *
 * @return Angle in @c [0, π].
 *
 * @threadsafe Yes.
 *
 * @sa occtl_direction3_angle
 */
OCCTL_API double OCCTL_CALL occtl_direction2_angle(occtl_direction2_t a, occtl_direction2_t b);

/**
 * Returns the identity transform.
 *
 * Applying the identity to a point or vector leaves it unchanged.
 *
 * @return Identity transform.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_apply_point3, occtl_transform_compose
 */
OCCTL_API occtl_transform_t OCCTL_CALL occtl_transform_identity(void);

/**
 * Returns a pure-translation transform.
 *
 * @param[in] v  Translation vector.
 *
 * @return Transform that moves points by @c v.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_compose, occtl_point3_translate
 */
OCCTL_API occtl_transform_t OCCTL_CALL occtl_transform_translation(occtl_vector3_t v);

/**
 * Builds a rotation transform around an axis by an angle.
 *
 * Uses the Rodrigues rotation formula.  The rotation is about the line defined
 * by @c axis: points on the axis are fixed; other points rotate by @c angle
 * radians (right-hand rule).
 *
 * @param[in]  axis          Rotation axis (origin + unit direction).
 * @param[in]  angle         Rotation angle in radians.
 * @param[out] out_transform Owns it (caller-allocated output slot).  Must be
 *                           non-NULL.  On success receives the rotation
 *                           transform; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_transform is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  @c axis.direction has zero (or near-zero)
 *                                 length.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_compose, occtl_transform_translation
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_rotation(occtl_axis1_placement_t axis,
                                                             double                  angle,
                                                             occtl_transform_t*      out_transform);

/**
 * Builds a uniform-scale transform centred on a point.
 *
 * @param[in]  center        Centre of scaling.
 * @param[in]  s             Scale factor.  Must not be zero.
 * @param[out] out_transform Owns it (caller-allocated output slot).  Must be
 *                           non-NULL.  On success receives the scale
 *                           transform; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_transform is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  @c s is zero.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_compose
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_scale(occtl_point3_t     center,
                                                          double             s,
                                                          occtl_transform_t* out_transform);

/**
 * Composes two transforms: applies @c first, then @c second.
 *
 * Equivalent to the matrix product @c second * @c first (standard
 * right-to-left composition convention).
 *
 * @param[in] first   Transform applied first.
 * @param[in] second  Transform applied second.
 *
 * @return Composed transform.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_inverted, occtl_transform_identity
 */
OCCTL_API occtl_transform_t OCCTL_CALL occtl_transform_compose(occtl_transform_t first,
                                                               occtl_transform_t second);

/**
 * Computes the inverse of an affine transform.
 *
 * @param[in]  t             Transform to invert.
 * @param[out] out_transform Owns it (caller-allocated output slot).  Must be
 *                           non-NULL.  On success receives the inverse
 *                           transform; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_transform is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  @c t is singular (determinant near zero).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_compose
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_inverted(occtl_transform_t  t,
                                                             occtl_transform_t* out_transform);

/**
 * Applies only the linear (3×3) part of a transform to a 3D vector (no
 * translation).
 *
 * @param[in] t  Transform.
 * @param[in] v  Vector to transform.
 *
 * @return Transformed vector.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_apply_point3, occtl_direction3_transform
 */
OCCTL_API occtl_vector3_t OCCTL_CALL occtl_transform_apply_vector3(occtl_transform_t t,
                                                                   occtl_vector3_t   v);

/**
 * Builds a transform that maps the world frame onto the given coordinate
 * frame (#occtl_axis2_placement_t).
 *
 * The returned transform takes a point expressed in the world frame and
 * produces its equivalent expressed relative to @c frame.  The Y direction
 * is derived from @c frame.x_dir and @c frame.x_dir_ref.
 *
 * @param[in]  frame         Frame to map onto.
 * @param[out] out_transform Owns it (caller-allocated output slot).  Must be
 *                           non-NULL.  On success receives the world→frame
 *                           transform; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_transform is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  @c frame.x_dir and @c frame.x_dir_ref are
 *                                 parallel, or any direction has zero length.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_from_axis3, occtl_transform_compose
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_from_axis2(occtl_axis2_placement_t frame,
                                                               occtl_transform_t* out_transform);

/**
 * Builds a transform from an explicit 3-axis frame (#occtl_axis3_placement_t).
 *
 * The 3×3 linear part of the result is the matrix whose columns are
 * @c frame.x_dir, @c frame.y_dir, @c frame.z_dir; the translation column is
 * @c frame.location.  Useful for round-tripping arbitrary (possibly
 * left-handed) frames produced by an external system.
 *
 * @param[in]  frame         Frame to map onto.
 * @param[out] out_transform Owns it (caller-allocated output slot).  Must be
 *                           non-NULL.  On success receives the frame
 *                           transform; left unchanged on failure.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @c out_transform is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  The three axis directions are linearly
 *                                 dependent (the frame is degenerate).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_transform_from_axis2, occtl_transform_compose
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_from_axis3(occtl_axis3_placement_t frame,
                                                               occtl_transform_t* out_transform);

/**
 * 2D axis placement — an origin and a main (X) direction in the 2D plane.
 *
 * Used as the base frame for 2D analytic curve geometry (circles,
 * ellipses, etc.).
 */
typedef struct
{
  occtl_point2_t     location; /**< Origin of the 2D frame. */
  occtl_direction2_t x_dir;    /**< Main axis direction. */
} occtl_axis2_placement2d_t;

/**
 * Continuity classification used by curve and surface handles.
 *
 * Values match those of @c GeomAbs_Shape in OCCT (C0=0, G1=1, …).
 * Append-only; do not reorder.
 */
typedef enum occtl_geom_continuity
{
  OCCTL_GEOM_CONTINUITY_C0              = 0, /**< Positional continuity only. */
  OCCTL_GEOM_CONTINUITY_G1              = 1, /**< Tangent (geometric) continuity. */
  OCCTL_GEOM_CONTINUITY_C1              = 2, /**< First-derivative continuity. */
  OCCTL_GEOM_CONTINUITY_G2              = 3, /**< Curvature (geometric) continuity. */
  OCCTL_GEOM_CONTINUITY_C2              = 4, /**< Second-derivative continuity. */
  OCCTL_GEOM_CONTINUITY_C3              = 5, /**< Third-derivative continuity. */
  OCCTL_GEOM_CONTINUITY_CN              = 6, /**< Infinite (analytical) continuity. */
  OCCTL_GEOM_CONTINUITY_RESERVED_FUTURE = 0x7fffffff
} occtl_geom_continuity_t;

/**
 * 3D infinite line data.
 *
 * Structurally identical to #occtl_axis1_placement_t (origin + unit direction).
 * Used as input to #occtl_curve_create_line and output of #occtl_curve_as_line.
 */
typedef occtl_axis1_placement_t occtl_geom_line_t;

/**
 * 3D circle data — axis-2 frame (center, normal, x-direction reference) plus radius.
 *
 * The @c position.x_dir field is the normal to the circle plane (Z-axis of the
 * local frame); @c position.x_dir_ref is the X-direction reference (where u=0 lies).
 */
typedef struct
{
  occtl_axis2_placement_t position; /**< Circle plane and orientation. */
  double                  radius;   /**< Circle radius; must be > 0. */
} occtl_geom_circle_t;

/**
 * 3D ellipse data — axis-2 frame plus major and minor radii.
 *
 * The major axis lies along the X direction derived from @c position.x_dir_ref.
 * @c major_radius >= @c minor_radius > 0.
 */
typedef struct
{
  occtl_axis2_placement_t position;     /**< Ellipse plane and orientation. */
  double                  major_radius; /**< Semi-major axis length. */
  double                  minor_radius; /**< Semi-minor axis length. */
} occtl_geom_ellipse_t;

/**
 * 3D hyperbola data — axis-2 frame plus real and imaginary semi-axes.
 *
 * @c major_radius is the real semi-axis; @c minor_radius is the imaginary semi-axis.
 * Both must be > 0.
 */
typedef struct
{
  occtl_axis2_placement_t position;     /**< Hyperbola plane and orientation. */
  double                  major_radius; /**< Real semi-axis length. */
  double                  minor_radius; /**< Imaginary semi-axis length. */
} occtl_geom_hyperbola_t;

/**
 * 3D parabola data — axis-2 frame plus focal length.
 *
 * @c focal_length is the distance from the apex to the focus; must be > 0.
 */
typedef struct
{
  occtl_axis2_placement_t position;     /**< Parabola plane and orientation. */
  double                  focal_length; /**< Focal length (distance apex to focus). */
} occtl_geom_parabola_t;

/**
 * 2D infinite line data — origin and unit direction in the 2D plane.
 */
typedef struct
{
  occtl_axis2_placement2d_t position; /**< Origin and direction of the 2D line. */
} occtl_geom2d_line_t;

/**
 * 2D circle data — axis-2d frame (center, x-direction) plus radius.
 */
typedef struct
{
  occtl_axis2_placement2d_t position; /**< Circle center and X-direction. */
  double                    radius;   /**< Circle radius; must be > 0. */
} occtl_geom2d_circle_t;

/**
 * 2D ellipse data — axis-2d frame plus major and minor radii.
 *
 * @c major_radius >= @c minor_radius > 0.
 */
typedef struct
{
  occtl_axis2_placement2d_t position;     /**< Ellipse center and orientation. */
  double                    major_radius; /**< Semi-major axis length. */
  double                    minor_radius; /**< Semi-minor axis length. */
} occtl_geom2d_ellipse_t;

/**
 * 2D hyperbola data — axis-2d frame plus real and imaginary semi-axes.
 */
typedef struct
{
  occtl_axis2_placement2d_t position;     /**< Hyperbola center and orientation. */
  double                    major_radius; /**< Real semi-axis length. */
  double                    minor_radius; /**< Imaginary semi-axis length. */
} occtl_geom2d_hyperbola_t;

/**
 * 2D parabola data — axis-2d frame plus focal length.
 */
typedef struct
{
  occtl_axis2_placement2d_t position;     /**< Parabola vertex and orientation. */
  double                    focal_length; /**< Focal length; must be > 0. */
} occtl_geom2d_parabola_t;

/**
 * Plane data — a 3-axis frame defining the plane origin and orientation.
 *
 * The plane equation is @c z_dir · (P - location) = 0.
 */
typedef struct
{
  occtl_axis3_placement_t position; /**< Plane origin and axis directions. */
} occtl_geom_plane_t;

/**
 * Cylindrical surface data — a 3-axis frame plus radius.
 *
 * The cylinder axis is @c position.z_dir; the radius is measured from that axis.
 */
typedef struct
{
  occtl_axis3_placement_t position; /**< Axis and origin of the cylinder. */
  double                  radius;   /**< Cylinder radius; must be > 0. */
} occtl_geom_cylindrical_surface_t;

/**
 * Spherical surface data — a 3-axis frame plus radius.
 *
 * The sphere centre is @c position.location.
 */
typedef struct
{
  occtl_axis3_placement_t position; /**< Sphere centre and axis directions. */
  double                  radius;   /**< Sphere radius; must be > 0. */
} occtl_geom_spherical_surface_t;

/**
 * Conical surface data — a 3-axis frame, half-angle, and apex-to-reference-plane distance.
 *
 * @c semi_angle is the half-opening angle in radians.  @c radius is the radius at
 * the reference circle (which lies in the @c z=0 plane of the local frame).
 */
typedef struct
{
  occtl_axis3_placement_t position;   /**< Cone axis and origin. */
  double                  semi_angle; /**< Half-opening angle (radians). */
  double                  radius;     /**< Radius at the reference circle. */
} occtl_geom_conical_surface_t;

/**
 * Toroidal surface data — a 3-axis frame plus major and minor radii.
 *
 * @c major_radius is the distance from the torus centre to the tube centre.
 * @c minor_radius is the tube radius.  Both must be > 0 and @c major_radius > @c minor_radius.
 */
typedef struct
{
  occtl_axis3_placement_t position;     /**< Torus centre and axis directions. */
  double                  major_radius; /**< Distance from centre to tube centre. */
  double                  minor_radius; /**< Tube radius. */
} occtl_geom_toroidal_surface_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_GEOM_H */
