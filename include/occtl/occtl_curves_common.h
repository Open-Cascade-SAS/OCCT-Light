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
 * @file occtl_curves_common.h
 * @brief OCCT-Light: Types shared by 3D and 2D curve APIs.
 *
 * Defines enums and create-info structs that apply to both 3D and 2D
 * curve representations.  Individual curve headers (§occtl_curves.h,
 * §occtl_curves2d.h) include this common foundation plus their dimension-
 * specific types and functions.
 */

#ifndef OCCTL_CURVES_COMMON_H
#define OCCTL_CURVES_COMMON_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Discriminates the underlying geometry of a curve representation
 * (3D or 2D).  Values are stable; new kinds are appended before
 * @c OCCTL_CURVE_KIND_RESERVED_FUTURE.
 */
typedef enum occtl_curve_kind
{
  OCCTL_CURVE_KIND_LINE            = 0, /**< Straight line. */
  OCCTL_CURVE_KIND_CIRCLE          = 1, /**< Full circle. */
  OCCTL_CURVE_KIND_ELLIPSE         = 2, /**< Ellipse. */
  OCCTL_CURVE_KIND_HYPERBOLA       = 3, /**< Hyperbola. */
  OCCTL_CURVE_KIND_PARABOLA        = 4, /**< Parabola. */
  OCCTL_CURVE_KIND_BSPLINE         = 5, /**< B-spline curve. */
  OCCTL_CURVE_KIND_BEZIER          = 6, /**< Bezier curve. */
  OCCTL_CURVE_KIND_TRIMMED         = 7, /**< Bounded section of a basis curve. */
  OCCTL_CURVE_KIND_OFFSET          = 8, /**< Offset of a basis curve. */
  OCCTL_CURVE_KIND_UNDEFINED       = 9, /**< Unknown or unrecognised subtype. */
  OCCTL_CURVE_KIND_RESERVED_FUTURE = 0x7fffffff
} occtl_curve_kind_t;

/**
 * Qualifies which side relation a tangent-circle solution must have
 * with an input 2D curve.
 *
 * These values mirror OCCT's GccEnt_Position without exposing OCCT types
 * in the C ABI.  For line inputs, only @c UNQUALIFIED, @c ENCLOSED, and
 * @c OUTSIDE are meaningful to OCCT.
 */
typedef enum occtl_curve2d_tangency_qualifier
{
  OCCTL_GEOM_TANGENCY_UNQUALIFIED     = 0, /**< Accept every valid tangent solution. */
  OCCTL_GEOM_TANGENCY_ENCLOSING       = 1, /**< Solution encloses the input curve. */
  OCCTL_GEOM_TANGENCY_ENCLOSED        = 2, /**< Solution is enclosed by the input curve. */
  OCCTL_GEOM_TANGENCY_OUTSIDE         = 3, /**< Solution and input are mutually external. */
  OCCTL_GEOM_TANGENCY_NO_QUALIFIER    = 4, /**< Returned metadata only; invalid as input. */
  OCCTL_GEOM_TANGENCY_RESERVED_FUTURE = 0x7fffffff
} occtl_curve2d_tangency_qualifier_t;

#define OCCTL_CURVE_TRIMMED_CREATE_INFO_VERSION_1 1u

/**
 * Versioned create-info struct for a trimmed curve (3D or 2D).
 *
 * The constructor makes an internal deep copy of the basis curve;
 * @c basis may be freed after the call returns.
 */
typedef struct occtl_curve_trimmed_create_info
{
  uint32_t       struct_version; /**< Must be #OCCTL_CURVE_TRIMMED_CREATE_INFO_VERSION_1. */
  const void*    p_next;         /**< Reserved; must be NULL. */
  occtl_rep_id_t basis;          /**< [in] Deep-copied internally. */
  double         u_first;        /**< Start parameter on the basis curve. */
  double         u_last;         /**< End parameter on the basis curve; must be > @c u_first. */
  int32_t        sense;          /**< 1 = same orientation as basis; -1 = reversed. */
} occtl_curve_trimmed_create_info_t;

#define OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT                                                       \
  {OCCTL_CURVE_TRIMMED_CREATE_INFO_VERSION_1, NULL, {0}, 0.0, 1.0, 1}

/**
 * Runtime initialiser for #occtl_curve_trimmed_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_trimmed, occtl_curve2d_create_trimmed
 */
OCCTL_API void OCCTL_CALL
  occtl_curve_trimmed_create_info_init(occtl_curve_trimmed_create_info_t* info);

#define OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_VERSION_1 1u

/**
 * Options for decomposing a curve into adjacent Bezier segments.
 *
 * Shared by #occtl_curve_to_bezier_segments and
 * #occtl_curve2d_to_bezier_segments.
 */
typedef struct occtl_curve_bezier_segments_options
{
  uint32_t    struct_version;       /**< Must be #OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_VERSION_1. */
  const void* p_next;               /**< Reserved; must be NULL. */
  int32_t     use_range;            /**< 0/1.  When non-zero, use @c u_first and @c u_last. */
  double      u_first;              /**< First parameter when @c use_range is non-zero. */
  double      u_last;               /**< Last parameter when @c use_range is non-zero. */
  double      parametric_tolerance; /**< Parameter comparison tolerance. Default 1.0e-9. */
} occtl_curve_bezier_segments_options_t;

#define OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_INIT                                                   \
  {OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_VERSION_1, NULL, 0, 0.0, 0.0, 1.0e-9}

/**
 * Runtime initialiser for #occtl_curve_bezier_segments_options_t.
 *
 * @param[out] options  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_to_bezier_segments, occtl_curve2d_to_bezier_segments
 */
OCCTL_API void OCCTL_CALL
  occtl_curve_bezier_segments_options_init(occtl_curve_bezier_segments_options_t* options);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_CURVES_COMMON_H */
