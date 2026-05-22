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
 * @file occtl_curves2d.h
 * @brief OCCT-Light: 2D curve (pcurve) API on BRepGraph.
 *
 * 2D curves are stored as graph representations (#occtl_rep_id_t with kind
 * #OCCTL_REP_KIND_CURVE2D) on nodes of an #occtl_graph_t.  Every function
 * in this header takes a graph pointer as its first parameter and identifies
 * curves by representation ID.
 *
 * Kept in its own header so topology consumers that need only pcurves
 * can include this without dragging in the 3D curve API.
 */

#ifndef OCCTL_CURVES2D_H
#define OCCTL_CURVES2D_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_curves_common.h"
#include "occtl_geom.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @cond */
typedef struct occtl_graph occtl_graph_t;

/** @endcond */

/**
 * Versioned create-info for fixed-radius 2D circles tangent to two curves.
 */
typedef struct occtl_curve2d_circle_tangent_to_two_radius_info
{
  uint32_t
    struct_version; /**< Must be #OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_VERSION_1. */
  const void*                        p_next;      /**< Reserved; must be NULL. */
  occtl_rep_id_t                     curve_a;     /**< [in] First tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_a; /**< Tangency side qualifier for @c curve_a. */
  occtl_rep_id_t                     curve_b;     /**< [in] Second tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_b; /**< Tangency side qualifier for @c curve_b. */
  double                             radius;      /**< Fixed solution-circle radius; must be > 0. */
  double tolerance; /**< OCCT tolerance for limit cases; must be >= 0. */
} occtl_curve2d_circle_tangent_to_two_radius_info_t;

#define OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_INIT                                       \
  {OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_VERSION_1,                                      \
   NULL,                                                                                           \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   0.0,                                                                                            \
   1.0e-9}

/**
 * Runtime initialiser for #occtl_curve2d_circle_tangent_to_two_radius_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_tangent_circle_to_two_radius
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_to_two_radius_info_init(
  occtl_curve2d_circle_tangent_to_two_radius_info_t* info);

/**
 * Versioned create-info for fixed-radius 2D blend arcs tangent to two curves.
 */
typedef struct occtl_curve2d_blend_arc_info
{
  uint32_t       struct_version; /**< Must be #OCCTL_CURVE2D_BLEND_ARC_INFO_VERSION_1. */
  const void*    p_next;         /**< Reserved; must be NULL. */
  occtl_rep_id_t curve_a;        /**< [in] First tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_a; /**< Tangency side qualifier for @c curve_a. */
  occtl_rep_id_t                     curve_b;     /**< [in] Second tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_b; /**< Tangency side qualifier for @c curve_b. */
  double                             radius;      /**< Fixed solution-circle radius; must be > 0. */
  int32_t long_arc;  /**< 0/1; choose the longer complementary arc when non-zero. */
  double  tolerance; /**< OCCT tolerance for limit cases; must be >= 0. */
} occtl_curve2d_blend_arc_info_t;

#define OCCTL_CURVE2D_BLEND_ARC_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_BLEND_ARC_INFO_INIT                                                          \
  {OCCTL_CURVE2D_BLEND_ARC_INFO_VERSION_1,                                                         \
   NULL,                                                                                           \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   0.0,                                                                                            \
   0,                                                                                              \
   1.0e-9}

/**
 * Runtime initialiser for #occtl_curve2d_blend_arc_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_blend_arc
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_blend_arc_info_init(occtl_curve2d_blend_arc_info_t* info);

/**
 * Versioned create-info for 2D lines tangent to two curves.
 */
typedef struct occtl_curve2d_line_tangent_to_two_info
{
  uint32_t       struct_version; /**< Must be #OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_VERSION_1. */
  const void*    p_next;         /**< Reserved; must be NULL. */
  occtl_rep_id_t curve_a;        /**< [in] First tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_a; /**< Tangency side qualifier for @c curve_a. */
  occtl_rep_id_t                     curve_b;     /**< [in] Second tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_b; /**< Tangency side qualifier for @c curve_b. */
  double tolerance; /**< OCCT angular tolerance for tangency; must be >= 0. */
} occtl_curve2d_line_tangent_to_two_info_t;

#define OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_INIT                                                \
  {OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_VERSION_1,                                               \
   NULL,                                                                                           \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   1.0e-9}

/**
 * Runtime initialiser for #occtl_curve2d_line_tangent_to_two_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_tangent_line_to_two
 */
OCCTL_API void OCCTL_CALL
  occtl_curve2d_line_tangent_to_two_info_init(occtl_curve2d_line_tangent_to_two_info_t* info);

/**
 * Versioned create-info for 2D lines tangent to a curve and passing through a point.
 */
typedef struct occtl_curve2d_line_tangent_through_point_info
{
  uint32_t struct_version; /**< Must be #OCCTL_CURVE2D_LINE_TANGENT_THROUGH_POINT_INFO_VERSION_1. */
  const void*                        p_next;    /**< Reserved; must be NULL. */
  occtl_rep_id_t                     curve;     /**< [in] Tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier; /**< Tangency side qualifier for @c curve. */
  occtl_point2_t                     point;     /**< Point the resulting line must pass through. */
  double tolerance; /**< OCCT angular tolerance for tangency; must be >= 0. */
} occtl_curve2d_line_tangent_through_point_info_t;

#define OCCTL_CURVE2D_LINE_TANGENT_THROUGH_POINT_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_LINE_TANGENT_THROUGH_POINT_INFO_INIT                                         \
  {OCCTL_CURVE2D_LINE_TANGENT_THROUGH_POINT_INFO_VERSION_1,                                        \
   NULL,                                                                                           \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0.0, 0.0},                                                                                     \
   1.0e-9}

/**
 * Runtime initialiser for #occtl_curve2d_line_tangent_through_point_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_tangent_line_through_point
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_line_tangent_through_point_info_init(
  occtl_curve2d_line_tangent_through_point_info_t* info);

/**
 * Versioned create-info for 2D lines tangent to a curve at a fixed angle to a line.
 */
typedef struct occtl_curve2d_line_tangent_with_angle_info
{
  uint32_t    struct_version; /**< Must be #OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_VERSION_1. */
  const void* p_next;         /**< Reserved; must be NULL. */
  occtl_rep_id_t                     curve;     /**< [in] Tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier; /**< Tangency side qualifier for @c curve. */
  occtl_geom2d_line_t reference_line;           /**< Reference 2D line used for the fixed angle. */
  double              angle_radians; /**< Signed angle to @c reference_line, in radians. */
  int32_t use_initial_parameter; /**< 0/1; use @c initial_parameter for iterative OCCT solvers. */
  double
    initial_parameter; /**< Initial curve parameter when @c use_initial_parameter is non-zero. */
  double tolerance;    /**< OCCT angular tolerance for tangency; must be >= 0. */
} occtl_curve2d_line_tangent_with_angle_info_t;

#define OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_INIT                                            \
  {OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_VERSION_1,                                           \
   NULL,                                                                                           \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {{{0.0, 0.0}, {1.0, 0.0}}},                                                                     \
   0.0,                                                                                            \
   0,                                                                                              \
   0.0,                                                                                            \
   1.0e-9}

/**
 * Runtime initialiser for #occtl_curve2d_line_tangent_with_angle_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_tangent_line_with_angle
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_line_tangent_with_angle_info_init(
  occtl_curve2d_line_tangent_with_angle_info_t* info);

/**
 * Versioned create-info for 2D circles tangent to three curves.
 */
typedef struct occtl_curve2d_circle_tangent_to_three_info
{
  uint32_t    struct_version; /**< Must be #OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_VERSION_1. */
  const void* p_next;         /**< Reserved; must be NULL. */
  occtl_rep_id_t                     curve_a;     /**< [in] First tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_a; /**< Tangency side qualifier for @c curve_a. */
  occtl_rep_id_t                     curve_b;     /**< [in] Second tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_b; /**< Tangency side qualifier for @c curve_b. */
  occtl_rep_id_t                     curve_c;     /**< [in] Third tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_c; /**< Tangency side qualifier for @c curve_c. */
  double initial_parameter_a; /**< Initial curve parameter for iterative OCCT solvers. */
  double initial_parameter_b; /**< Initial curve parameter for iterative OCCT solvers. */
  double initial_parameter_c; /**< Initial curve parameter for iterative OCCT solvers. */
  double tolerance;           /**< OCCT tolerance for limit cases; must be >= 0. */
} occtl_curve2d_circle_tangent_to_three_info_t;

#define OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_INIT                                            \
  {OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_VERSION_1,                                           \
   NULL,                                                                                           \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   1.0e-9}

/**
 * Runtime initialiser for #occtl_curve2d_circle_tangent_to_three_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_tangent_circle_to_three
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_to_three_info_init(
  occtl_curve2d_circle_tangent_to_three_info_t* info);

/**
 * Versioned create-info for 2D circles tangent to a curve with fixed center.
 */
typedef struct occtl_curve2d_circle_tangent_fixed_center_info
{
  uint32_t
    struct_version; /**< Must be #OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_VERSION_1. */
  const void*                        p_next;    /**< Reserved; must be NULL. */
  occtl_rep_id_t                     curve;     /**< [in] Tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier; /**< Tangency side qualifier for @c curve. */
  occtl_point2_t                     center;    /**< Fixed center of the resulting circle. */
  double tolerance; /**< OCCT tolerance for limit cases; must be >= 0. */
} occtl_curve2d_circle_tangent_fixed_center_info_t;

#define OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_INIT                                        \
  {OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_VERSION_1,                                       \
   NULL,                                                                                           \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0.0, 0.0},                                                                                     \
   1.0e-9}

/**
 * Runtime initialiser for #occtl_curve2d_circle_tangent_fixed_center_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_tangent_circle_fixed_center
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_fixed_center_info_init(
  occtl_curve2d_circle_tangent_fixed_center_info_t* info);

/**
 * Versioned create-info for 2D circles tangent to two curves with center on a curve.
 */
typedef struct occtl_curve2d_circle_tangent_center_on_curve_info
{
  uint32_t
    struct_version; /**< Must be #OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_VERSION_1. */
  const void*                        p_next;      /**< Reserved; must be NULL. */
  occtl_rep_id_t                     curve_a;     /**< [in] First tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_a; /**< Tangency side qualifier for @c curve_a. */
  occtl_rep_id_t                     curve_b;     /**< [in] Second tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier_b; /**< Tangency side qualifier for @c curve_b. */
  occtl_rep_id_t center_curve; /**< [in] Curve carrying solution centers; must be non-NULL. */
  double initial_parameter_a;  /**< Initial parameter on @c curve_a for iterative OCCT solvers. */
  double initial_parameter_b;  /**< Initial parameter on @c curve_b for iterative OCCT solvers. */
  double initial_parameter_center; /**< Initial parameter on @c center_curve for iterative OCCT
                                      solvers. */
  double tolerance;                /**< OCCT tolerance for limit cases; must be >= 0. */
} occtl_curve2d_circle_tangent_center_on_curve_info_t;

#define OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_INIT                                     \
  {OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_VERSION_1,                                    \
   NULL,                                                                                           \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0},                                                                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   1.0e-9}

/**
 * Runtime initialiser for #occtl_curve2d_circle_tangent_center_on_curve_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_tangent_circle_center_on_curve
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_center_on_curve_info_init(
  occtl_curve2d_circle_tangent_center_on_curve_info_t* info);

/**
 * Versioned create-info for fixed-radius 2D circles tangent to a curve with
 * centers constrained to a second curve.
 */
typedef struct occtl_curve2d_circle_tangent_on_curve_radius_info
{
  uint32_t
    struct_version; /**< Must be #OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_VERSION_1. */
  const void*                        p_next;    /**< Reserved; must be NULL. */
  occtl_rep_id_t                     curve;     /**< [in] Tangent curve id. */
  occtl_curve2d_tangency_qualifier_t qualifier; /**< Tangency side qualifier for @c curve. */
  occtl_rep_id_t center_curve; /**< [in] Curve carrying solution centers; must be non-NULL. */
  double         radius;       /**< Fixed solution-circle radius; must be > 0. */
  double         tolerance;    /**< OCCT tolerance for limit cases; must be >= 0. */
} occtl_curve2d_circle_tangent_on_curve_radius_info_t;

#define OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_INIT                                     \
  {OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_VERSION_1,                                    \
   NULL,                                                                                           \
   {0},                                                                                            \
   OCCTL_GEOM_TANGENCY_UNQUALIFIED,                                                                \
   {0},                                                                                            \
   0.0,                                                                                            \
   1.0e-9}

/**
 * Runtime initialiser for #occtl_curve2d_circle_tangent_on_curve_radius_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_tangent_circle_on_curve_radius
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_on_curve_radius_info_init(
  occtl_curve2d_circle_tangent_on_curve_radius_info_t* info);

/**
 * Computes all fixed-radius 2D circles tangent to two input 2D curves.
 *
 * The results are returned one at a time by zero-based solution index.  Pass
 * @p out_circle as NULL to read only the solution count, then request each
 * solution by index.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  info            Borrows it.  Versioned solver inputs; must be non-NULL.
 * @param[in]  solution_index  Zero-based solution index to read when @p out_circle is non-NULL.
 * @param[out] out_circle      Borrows it.  Caller-owned slot, or NULL for count-only query.
 * @param[out] out_count       Borrows it.  Receives the total solution count; must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p info or @p out_count is NULL, required
 *                                fields are invalid, or @c p_next is non-NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the tangent construction.
 * @retval OCCTL_NOT_FOUND        @p out_circle is non-NULL and @p solution_index
 *                                is outside the available solution range.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_circle, occtl_curve2d_circle_tangent_to_two_radius_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_to_two_radius(
  occtl_graph_t*                                           graph,
  const occtl_curve2d_circle_tangent_to_two_radius_info_t* info,
  size_t                                                   solution_index,
  occtl_geom2d_circle_t*                                   out_circle,
  size_t*                                                  out_count);

/**
 * Computes fixed-radius 2D blend arcs tangent to two input 2D curves.
 *
 * The results are returned one at a time by zero-based solution index.  Pass
 * @p out_id as NULL to read only the solution count, then request each
 * solution by index.  Each returned curve is a trimmed 2D circle spanning the
 * OCCT-computed tangency points.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  info            Borrows it.  Versioned solver inputs; must be non-NULL.
 * @param[in]  solution_index  Zero-based solution index to read when @p out_id is non-NULL.
 * @param[out] out_id          Borrows it.  Caller-allocated slot for a curve rep id,
 *                             or NULL for count-only query.
 * @param[out] out_count       Borrows it.  Receives the total solution count; must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p info or @p out_count is NULL, required fields
 *                                are invalid, @c p_next is non-NULL, or the 0/1 flag is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the tangent construction.
 * @retval OCCTL_NOT_FOUND        @p out_id is non-NULL and @p solution_index
 *                                is outside the available solution range.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_blend_arc_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_create_blend_arc(occtl_graph_t*                        graph,
                                 const occtl_curve2d_blend_arc_info_t* info,
                                 size_t                                solution_index,
                                 occtl_rep_id_t*                       out_id,
                                 size_t*                               out_count);

/**
 * Computes all 2D lines tangent to two input 2D curves.
 *
 * The results are returned one at a time by zero-based solution index.  Pass
 * @p out_line as NULL to read only the solution count, then request each
 * solution by index.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  info            Borrows it.  Versioned solver inputs; must be non-NULL.
 * @param[in]  solution_index  Zero-based solution index to read when @p out_line is non-NULL.
 * @param[out] out_line        Borrows it.  Caller-owned slot, or NULL for count-only query.
 * @param[out] out_count       Borrows it.  Receives the total solution count; must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p info or @p out_count is NULL, required
 *                                fields are invalid, or @c p_next is non-NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the tangent construction.
 * @retval OCCTL_NOT_FOUND        @p out_line is non-NULL and @p solution_index
 *                                is outside the available solution range.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_line, occtl_curve2d_line_tangent_to_two_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_create_tangent_line_to_two(occtl_graph_t*                                  graph,
                                           const occtl_curve2d_line_tangent_to_two_info_t* info,
                                           size_t               solution_index,
                                           occtl_geom2d_line_t* out_line,
                                           size_t*              out_count);

/**
 * Computes all 2D lines tangent to a curve and passing through a point.
 *
 * The results are returned one at a time by zero-based solution index.  Pass
 * @p out_line as NULL to read only the solution count, then request each
 * solution by index.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  info            Borrows it.  Versioned solver inputs; must be non-NULL.
 * @param[in]  solution_index  Zero-based solution index to read when @p out_line is non-NULL.
 * @param[out] out_line        Borrows it.  Caller-owned slot, or NULL for count-only query.
 * @param[out] out_count       Borrows it.  Receives the total solution count; must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p info or @p out_count is NULL, required
 *                                fields are invalid, or @c p_next is non-NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the tangent construction.
 * @retval OCCTL_NOT_FOUND        @p out_line is non-NULL and @p solution_index
 *                                is outside the available solution range.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_line, occtl_curve2d_line_tangent_through_point_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_line_through_point(
  occtl_graph_t*                                         graph,
  const occtl_curve2d_line_tangent_through_point_info_t* info,
  size_t                                                 solution_index,
  occtl_geom2d_line_t*                                   out_line,
  size_t*                                                out_count);

/**
 * Computes all 2D lines tangent to a curve at a fixed angle to a reference line.
 *
 * The results are returned one at a time by zero-based solution index.  Pass
 * @p out_line as NULL to read only the solution count, then request each
 * solution by index.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  info            Borrows it.  Versioned solver inputs; must be non-NULL.
 * @param[in]  solution_index  Zero-based solution index to read when @p out_line is non-NULL.
 * @param[out] out_line        Borrows it.  Caller-owned slot, or NULL for count-only query.
 * @param[out] out_count       Borrows it.  Receives the total solution count; must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p info or @p out_count is NULL, required
 *                                fields are invalid, @c p_next is non-NULL, or the 0/1 flag
 *                                is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the tangent construction.
 * @retval OCCTL_NOT_FOUND        @p out_line is non-NULL and @p solution_index
 *                                is outside the available solution range.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_line, occtl_curve2d_line_tangent_with_angle_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_line_with_angle(
  occtl_graph_t*                                      graph,
  const occtl_curve2d_line_tangent_with_angle_info_t* info,
  size_t                                              solution_index,
  occtl_geom2d_line_t*                                out_line,
  size_t*                                             out_count);

/**
 * Computes all 2D circles tangent to three input 2D curves.
 *
 * The results are returned one at a time by zero-based solution index.  Pass
 * @p out_circle as NULL to read only the solution count, then request each
 * solution by index.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  info            Borrows it.  Versioned solver inputs; must be non-NULL.
 * @param[in]  solution_index  Zero-based solution index to read when @p out_circle is non-NULL.
 * @param[out] out_circle      Borrows it.  Caller-owned slot, or NULL for count-only query.
 * @param[out] out_count       Borrows it.  Receives the total solution count; must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p info or @p out_count is NULL, required
 *                                fields are invalid, or @c p_next is non-NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the tangent construction.
 * @retval OCCTL_NOT_FOUND        @p out_circle is non-NULL and @p solution_index
 *                                is outside the available solution range.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_circle, occtl_curve2d_circle_tangent_to_three_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_to_three(
  occtl_graph_t*                                      graph,
  const occtl_curve2d_circle_tangent_to_three_info_t* info,
  size_t                                              solution_index,
  occtl_geom2d_circle_t*                              out_circle,
  size_t*                                             out_count);

/**
 * Computes all 2D circles tangent to a curve with a fixed center point.
 *
 * The results are returned one at a time by zero-based solution index.  Pass
 * @p out_circle as NULL to read only the solution count, then request each
 * solution by index.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  info            Borrows it.  Versioned solver inputs; must be non-NULL.
 * @param[in]  solution_index  Zero-based solution index to read when @p out_circle is non-NULL.
 * @param[out] out_circle      Borrows it.  Caller-owned slot, or NULL for count-only query.
 * @param[out] out_count       Borrows it.  Receives the total solution count; must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p info or @p out_count is NULL, required
 *                                fields are invalid, or @c p_next is non-NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the tangent construction.
 * @retval OCCTL_NOT_FOUND        @p out_circle is non-NULL and @p solution_index
 *                                is outside the available solution range.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_circle, occtl_curve2d_circle_tangent_fixed_center_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_fixed_center(
  occtl_graph_t*                                          graph,
  const occtl_curve2d_circle_tangent_fixed_center_info_t* info,
  size_t                                                  solution_index,
  occtl_geom2d_circle_t*                                  out_circle,
  size_t*                                                 out_count);

/**
 * Computes all 2D circles tangent to two curves with centers constrained to a curve.
 *
 * The results are returned one at a time by zero-based solution index.  Pass
 * @p out_circle as NULL to read only the solution count, then request each
 * solution by index.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  info            Borrows it.  Versioned solver inputs; must be non-NULL.
 * @param[in]  solution_index  Zero-based solution index to read when @p out_circle is non-NULL.
 * @param[out] out_circle      Borrows it.  Caller-owned slot, or NULL for count-only query.
 * @param[out] out_count       Borrows it.  Receives the total solution count; must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p info or @p out_count is NULL, required
 *                                fields are invalid, or @c p_next is non-NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the tangent construction.
 * @retval OCCTL_NOT_FOUND        @p out_circle is non-NULL and @p solution_index
 *                                is outside the available solution range.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_circle, occtl_curve2d_circle_tangent_center_on_curve_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_center_on_curve(
  occtl_graph_t*                                             graph,
  const occtl_curve2d_circle_tangent_center_on_curve_info_t* info,
  size_t                                                     solution_index,
  occtl_geom2d_circle_t*                                     out_circle,
  size_t*                                                    out_count);

/**
 * Computes fixed-radius 2D circles tangent to one curve with centers
 * constrained to another curve.
 *
 * The results are returned one at a time by zero-based solution index.  Pass
 * @p out_circle as NULL to read only the solution count, then request each
 * solution by index.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  info            Borrows it.  Versioned solver inputs; must be non-NULL.
 * @param[in]  solution_index  Zero-based solution index to read when @p out_circle is non-NULL.
 * @param[out] out_circle      Borrows it.  Caller-owned slot, or NULL for count-only query.
 * @param[out] out_count       Borrows it.  Receives the total solution count; must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p info or @p out_count is NULL, required
 *                                fields are invalid, or @c p_next is non-NULL, or
 *                                radius/tolerance is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the tangent construction.
 * @retval OCCTL_NOT_FOUND        @p out_circle is non-NULL and @p solution_index
 *                                is outside the available solution range.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_circle, occtl_curve2d_circle_tangent_on_curve_radius_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_on_curve_radius(
  occtl_graph_t*                                             graph,
  const occtl_curve2d_circle_tangent_on_curve_radius_info_t* info,
  size_t                                                     solution_index,
  occtl_geom2d_circle_t*                                     out_circle,
  size_t*                                                    out_count);

/**
 * Creates a 2D curve representation from a line.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  line    2D line data.
 * @param[out] out_id  Borrows it.  Caller-allocated slot for the curve rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_line(occtl_graph_t*      graph,
                                                              occtl_geom2d_line_t line,
                                                              occtl_rep_id_t*     out_id);

/**
 * Creates a 2D curve representation from a circle.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  circle  2D circle data.  @c radius must be > 0.
 * @param[out] out_id  Borrows it.  Caller-allocated slot for the curve rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID @c radius is not positive.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_circle(occtl_graph_t*        graph,
                                                                occtl_geom2d_circle_t circle,
                                                                occtl_rep_id_t*       out_id);

/**
 * Creates a 2D curve representation from an ellipse.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  ellipse  2D ellipse data.
 * @param[out] out_id   Borrows it.  Caller-allocated slot for the curve rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID Radii invalid.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_ellipse(occtl_graph_t*         graph,
                                                                 occtl_geom2d_ellipse_t ellipse,
                                                                 occtl_rep_id_t*        out_id);

/**
 * Creates a 2D curve representation from a hyperbola.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  hyperbola  2D hyperbola data.
 * @param[out] out_id     Borrows it.  Caller-allocated slot for the curve rep id.
 *                        Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID Radii invalid.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_create_hyperbola(occtl_graph_t*           graph,
                                 occtl_geom2d_hyperbola_t hyperbola,
                                 occtl_rep_id_t*          out_id);

/**
 * Creates a 2D curve representation from a parabola.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  parabola  2D parabola data.  @c focal_length must be > 0.
 * @param[out] out_id    Borrows it.  Caller-allocated slot for the curve rep id.
 *                       Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID @c focal_length is not positive.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_parabola(occtl_graph_t*          graph,
                                                                  occtl_geom2d_parabola_t parabola,
                                                                  occtl_rep_id_t*         out_id);

/**
 * Versioned create-info struct for 2D B-spline curve construction.
 *
 * Same semantics as #occtl_curve_bspline_create_info_t but uses 2D poles.
 */
typedef struct occtl_curve2d_bspline_create_info
{
  uint32_t    struct_version;  /**< Must be #OCCTL_CURVE2D_BSPLINE_CREATE_INFO_VERSION_1. */
  const void* p_next;          /**< Reserved; must be NULL. */
  const occtl_point2_t* poles; /**< [in] Borrows it; count = @c pole_count. */
  size_t                pole_count;
  const double*         weights; /**< [in] Borrows it; NULL implies non-rational. */
  const double*         knots;   /**< [in] Borrows it; distinct values; count = @c knot_count. */
  const int32_t*        multiplicities; /**< [in] Borrows it; count = @c knot_count. */
  size_t                knot_count;
  int32_t               degree;
  int32_t               is_periodic;
} occtl_curve2d_bspline_create_info_t;

#define OCCTL_CURVE2D_BSPLINE_CREATE_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT                                                     \
  {OCCTL_CURVE2D_BSPLINE_CREATE_INFO_VERSION_1, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, 0}

/**
 * Runtime initialiser for #occtl_curve2d_bspline_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_bspline
 */
OCCTL_API void OCCTL_CALL
  occtl_curve2d_bspline_create_info_init(occtl_curve2d_bspline_create_info_t* info);

/**
 * Creates a 2D curve representation from a B-spline definition.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[out] out_id  Borrows it.  Caller-allocated slot for the curve rep id.
 *                     Must be non-NULL.
 * @param[in]  info    Versioned create-info.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id or @p info is NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the definition.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_create_bspline(occtl_graph_t*                             graph,
                               const occtl_curve2d_bspline_create_info_t* info,
                               occtl_rep_id_t*                            out_id);

/**
 * Versioned create-info struct for 2D Bezier curve construction.
 */
typedef struct occtl_curve2d_bezier_create_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_CURVE2D_BEZIER_CREATE_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved; must be NULL. */
  const occtl_point2_t* poles;
  size_t                pole_count;
  const double*         weights; /**< NULL implies non-rational. */
} occtl_curve2d_bezier_create_info_t;

#define OCCTL_CURVE2D_BEZIER_CREATE_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_BEZIER_CREATE_INFO_INIT                                                      \
  {OCCTL_CURVE2D_BEZIER_CREATE_INFO_VERSION_1, NULL, NULL, 0, NULL}

/**
 * Runtime initialiser for #occtl_curve2d_bezier_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_bezier
 */
OCCTL_API void OCCTL_CALL
  occtl_curve2d_bezier_create_info_init(occtl_curve2d_bezier_create_info_t* info);

/**
 * Creates a 2D curve representation from a Bezier definition.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[out] out_id  Borrows it.  Caller-allocated slot for the curve rep id.
 *                     Must be non-NULL.
 * @param[in]  info    Versioned create-info.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id or @p info is NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the definition.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_create_bezier(occtl_graph_t*                            graph,
                              const occtl_curve2d_bezier_create_info_t* info,
                              occtl_rep_id_t*                           out_id);

/**
 * Creates a trimmed 2D curve representation.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[out] out_id  Borrows it.  Caller-allocated slot for the curve rep id.
 *                     Must be non-NULL.
 * @param[in]  info    Versioned create-info.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id or @p info is NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID Invalid interval.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_create_trimmed(occtl_graph_t*                           graph,
                               const occtl_curve_trimmed_create_info_t* info,
                               occtl_rep_id_t*                          out_id);

/**
 * Versioned create-info for a 2D offset curve.
 *
 * 2D offset curves are always perpendicular to the basis curve in the
 * parametric plane; only a scalar offset is needed (no direction).
 */
typedef struct occtl_curve2d_offset_create_info
{
  uint32_t       struct_version; /**< Must be #OCCTL_CURVE2D_OFFSET_CREATE_INFO_VERSION_1. */
  const void*    p_next;         /**< Reserved; must be NULL. */
  occtl_rep_id_t basis;          /**< [in] Basis curve rep id. */
  double         offset;         /**< Signed offset distance. */
} occtl_curve2d_offset_create_info_t;

#define OCCTL_CURVE2D_OFFSET_CREATE_INFO_VERSION_1 1u

#define OCCTL_CURVE2D_OFFSET_CREATE_INFO_INIT                                                      \
  {OCCTL_CURVE2D_OFFSET_CREATE_INFO_VERSION_1, NULL, {0}, 0.0}

/**
 * Runtime initialiser for #occtl_curve2d_offset_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_offset
 */
OCCTL_API void OCCTL_CALL
  occtl_curve2d_offset_create_info_init(occtl_curve2d_offset_create_info_t* info);

/**
 * Creates an offset 2D curve representation.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[out] out_id  Borrows it.  Caller-allocated slot for the curve rep id.
 *                     Must be non-NULL.
 * @param[in]  info    Versioned create-info.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id or @p info is NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the offset.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_create_offset(occtl_graph_t*                            graph,
                              const occtl_curve2d_offset_create_info_t* info,
                              occtl_rep_id_t*                           out_id);

/**
 * Creates a new 2D curve representation that is the reversed copy of @p curve_id.
 *
 * The returned rep owns an independent curve whose parameterisation
 * runs in the opposite direction.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  curve_id Curve to reverse.
 * @param[out] out_id   Borrows it.  Caller-allocated slot for the reversed curve rep id.
 *                      Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_reverse(occtl_graph_t*  graph,
                                                          occtl_rep_id_t  curve_id,
                                                          occtl_rep_id_t* out_id);

/**
 * Creates a new 2D curve representation by applying a 2D transform to @p curve_id.
 *
 * The transform is applied in the 2D parametric plane via @c gp_Trsf2d.
 *
 * @param[in]  graph            Must be non-NULL.
 * @param[in]  curve_id         Curve to transform.
 * @param[in]  translate_x      Translation along X.
 * @param[in]  translate_y      Translation along Y.
 * @param[in]  rotate_angle     Rotation angle in radians.
 * @param[in]  scale_x          Scale factor along X.
 * @param[in]  scale_y          Scale factor along Y.
 * @param[out] out_id           Borrows it.  Caller-allocated slot for the transformed
 *                               curve rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_transformed(occtl_graph_t*  graph,
                                                              occtl_rep_id_t  curve_id,
                                                              double          translate_x,
                                                              double          translate_y,
                                                              double          rotate_angle,
                                                              double          scale_x,
                                                              double          scale_y,
                                                              occtl_rep_id_t* out_id);

/**
 * Creates a new 2D curve representation by translating @p curve_id.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to translate.
 * @param[in]  delta       Translation vector in 2D.
 * @param[out] out_id      Borrows it.  Caller-allocated slot for the translated curve rep id.
 *                         Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_translated(occtl_graph_t*  graph,
                                                             occtl_rep_id_t  curve_id,
                                                             occtl_vector2_t delta,
                                                             occtl_rep_id_t* out_id);

/**
 * Creates a new 2D curve representation by rotating @p curve_id about the origin.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to rotate.
 * @param[in]  angle      Rotation angle in radians.
 * @param[out] out_id     Borrows it.  Caller-allocated slot for the rotated curve rep id.
 *                        Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_rotated(occtl_graph_t*  graph,
                                                          occtl_rep_id_t  curve_id,
                                                          double          angle,
                                                          occtl_rep_id_t* out_id);

/**
 * Creates a new 2D curve representation by scaling @p curve_id about @p origin.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to scale.
 * @param[in]  origin     Origin point for scaling.
 * @param[in]  factor     Scale factor; must be non-zero.
 * @param[out] out_id     Borrows it.  Caller-allocated slot for the scaled curve rep id.
 *                        Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID @c factor is zero.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_scaled(occtl_graph_t*  graph,
                                                         occtl_rep_id_t  curve_id,
                                                         occtl_point2_t  origin,
                                                         double          factor,
                                                         occtl_rep_id_t* out_id);

/**
 * Computes the length of the 2D curve over its full parameter range.
 *
 * @param[in]  graph        Must be non-NULL.
 * @param[in]  curve_id     Curve to measure.
 * @param[out] out_length   Receives the curve length.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_length is NULL.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_length(const occtl_graph_t* graph,
                                                         occtl_rep_id_t       curve_id,
                                                         double*              out_length);

/**
 * Projects a 2D point onto the curve and returns the closest parameter.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  curve_id      Curve to project onto.
 * @param[in]  point         Point to project.
 * @param[out] out_param     Receives the parameter of the closest point.  Must be non-NULL.
 * @param[out] out_distance  Receives the shortest distance.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_param is NULL.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_project_point(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       curve_id,
                                                                occtl_point2_t       point,
                                                                double*              out_param,
                                                                double*              out_distance);

/**
 * Returns the parameter on the 2D curve nearest to a given point.
 *
 * Convenience function equivalent to #occtl_curve2d_project_point with
 * @c out_distance = NULL.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to query.
 * @param[in]  point      Point to find the parameter for.
 * @param[out] out_param  Receives the parameter.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_param is NULL.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_parameter_of_point(const occtl_graph_t* graph,
                                                                     occtl_rep_id_t       curve_id,
                                                                     occtl_point2_t       point,
                                                                     double* out_param);

/**
 * Returns the kind of geometry of a 2D curve representation.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  curve_id  Curve to query.
 * @param[out] out_kind  Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_kind is NULL.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_as_circle, occtl_curve2d_as_ellipse
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_kind(const occtl_graph_t* graph,
                                                       occtl_rep_id_t       curve_id,
                                                       occtl_curve_kind_t*  out_kind);

/**
 * Returns non-zero if the 2D curve is periodic.
 *
 * @param[in]  graph        Must be non-NULL.
 * @param[in]  curve_id     Curve to query.
 * @param[out] out_is_periodic Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *                          1 if periodic, 0 otherwise.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_periodic is NULL.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_is_periodic(const occtl_graph_t* graph,
                                                              occtl_rep_id_t       curve_id,
                                                              int32_t*             out_is_periodic);

/**
 * Returns non-zero if the 2D curve is closed.
 *
 * A 2D curve is closed when its start and end points coincide within
 * OCCT's confusion tolerance.  Periodic curves are always closed; the
 * converse does not hold.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to query.
 * @param[out] out_is_closed Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *                         1 if closed, 0 otherwise.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_closed is NULL.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_is_periodic
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_is_closed(const occtl_graph_t* graph,
                                                            occtl_rep_id_t       curve_id,
                                                            int32_t*             out_is_closed);

/**
 * Returns the continuity class of the 2D curve.
 *
 * @param[in]  graph           Must be non-NULL.
 * @param[in]  curve_id        Curve to query.
 * @param[out] out_continuity  Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_continuity is NULL.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_continuity(occtl_graph_t*           graph,
                           occtl_rep_id_t           curve_id,
                           occtl_geom_continuity_t* out_continuity);

/**
 * Returns the parameter range of a 2D curve.
 *
 * For analytic infinite curves (lines, parabolas, hyperbolas) OCCT returns
 * @c -Precision::Infinite() and @c +Precision::Infinite() (numerically
 * @c -1e100 and @c +1e100).  Bounded analytic curves (circles, ellipses)
 * return [0, 2π].
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to query.
 * @param[out] out_u_min   Start parameter.  May be NULL.
 * @param[out] out_u_max   End parameter.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_parameter_range(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       curve_id,
                                                                  double*              out_u_min,
                                                                  double*              out_u_max);

/**
 * Extracts line data from a 2D curve representation.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  curve_id  Curve to extract; kind must be @c OCCTL_CURVE_KIND_LINE.
 * @param[out] out_line  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_line is NULL.
 * @retval OCCTL_WRONG_KIND       Representation is not a line.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_as_line(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          occtl_geom2d_line_t* out_line);

/**
 * Extracts circle data from a 2D curve representation.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to extract; kind must be @c OCCTL_CURVE_KIND_CIRCLE.
 * @param[out] out_circle  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_circle is NULL.
 * @retval OCCTL_WRONG_KIND       Representation is not a circle.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_as_circle(const occtl_graph_t*   graph,
                                                            occtl_rep_id_t         curve_id,
                                                            occtl_geom2d_circle_t* out_circle);

/**
 * Extracts ellipse data from a 2D curve representation.
 *
 * @param[in]  graph        Must be non-NULL.
 * @param[in]  curve_id     Curve to extract; kind must be @c OCCTL_CURVE_KIND_ELLIPSE.
 * @param[out] out_ellipse  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_ellipse is NULL.
 * @retval OCCTL_WRONG_KIND       Representation is not an ellipse.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_as_ellipse(const occtl_graph_t*    graph,
                                                             occtl_rep_id_t          curve_id,
                                                             occtl_geom2d_ellipse_t* out_ellipse);

/**
 * Extracts hyperbola data from a 2D curve representation.
 *
 * @param[in]  graph          Must be non-NULL.
 * @param[in]  curve_id       Curve to extract; kind must be @c OCCTL_CURVE_KIND_HYPERBOLA.
 * @param[out] out_hyperbola  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_hyperbola is NULL.
 * @retval OCCTL_WRONG_KIND       Representation is not a hyperbola.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_as_hyperbola(occtl_graph_t*            graph,
                             occtl_rep_id_t            curve_id,
                             occtl_geom2d_hyperbola_t* out_hyperbola);

/**
 * Extracts parabola data from a 2D curve representation.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  curve_id      Curve to extract; kind must be @c OCCTL_CURVE_KIND_PARABOLA.
 * @param[out] out_parabola  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_parabola is NULL.
 * @retval OCCTL_WRONG_KIND       Representation is not a parabola.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_as_parabola(occtl_graph_t*           graph,
                            occtl_rep_id_t           curve_id,
                            occtl_geom2d_parabola_t* out_parabola);

/**
 * Extracts the parameter bounds of a trimmed 2D curve.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to extract; kind must be @c OCCTL_CURVE_KIND_TRIMMED.
 * @param[out] out_u_first First parameter.  May be NULL.
 * @param[out] out_u_last  Last parameter.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_WRONG_KIND       Representation is not a trimmed curve.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_as_trimmed(const occtl_graph_t* graph,
                                                             occtl_rep_id_t       curve_id,
                                                             double*              out_u_first,
                                                             double*              out_u_last);

/**
 * Extracts the scalar offset of a 2D offset curve.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to extract; kind must be @c OCCTL_CURVE_KIND_OFFSET.
 * @param[out] out_offset  Signed offset distance.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_WRONG_KIND       Representation is not an offset curve.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_as_offset(const occtl_graph_t* graph,
                                                            occtl_rep_id_t       curve_id,
                                                            double*              out_offset);

/**
 * Returns the polynomial degree of a 2D B-spline curve.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to query; kind should be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_degree  Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_degree is NULL.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bspline_degree(const occtl_graph_t* graph,
                                                                 occtl_rep_id_t       curve_id,
                                                                 int32_t*             out_degree);

/**
 * Returns the pole count of a 2D B-spline curve.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to query.
 * @param[out] out_count   Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bspline_pole_count(const occtl_graph_t* graph,
                                                                     occtl_rep_id_t       curve_id,
                                                                     size_t* out_count);

/**
 * Returns the knot count of a 2D B-spline curve.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to query.
 * @param[out] out_count   Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bspline_knot_count(const occtl_graph_t* graph,
                                                                     occtl_rep_id_t       curve_id,
                                                                     size_t* out_count);

/**
 * Returns non-zero if the 2D B-spline curve is rational.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  curve_id      Curve to query.
 * @param[out] out_is_rational  Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *                           1 if rational; 0 otherwise.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_rational is NULL.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bspline_is_rational(const occtl_graph_t* graph,
                                                                      occtl_rep_id_t       curve_id,
                                                                      int32_t* out_is_rational);

/**
 * Returns the polynomial degree of a 2D Bezier curve.
 *
 * Equivalent to @c pole_count - 1 but exposed for symmetry with
 * #occtl_curve2d_bspline_degree.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to query; kind must be @c OCCTL_CURVE_KIND_BEZIER.
 * @param[out] out_degree  Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_degree is NULL.
 * @retval OCCTL_WRONG_KIND       Not a Bezier curve.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bezier_degree(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       curve_id,
                                                                int32_t*             out_degree);

/**
 * Returns the pole count of a 2D Bezier curve.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  curve_id    Curve to query; kind must be @c OCCTL_CURVE_KIND_BEZIER.
 * @param[out] out_count   Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Not a Bezier curve.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bezier_pole_count(const occtl_graph_t* graph,
                                                                    occtl_rep_id_t       curve_id,
                                                                    size_t*              out_count);

/**
 * Returns non-zero if the 2D Bezier curve is rational (has per-pole weights).
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  curve_id      Curve to query; kind must be @c OCCTL_CURVE_KIND_BEZIER.
 * @param[out] out_is_rational  Borrows it.  Caller-allocated slot.  Must be non-NULL.
 *                           1 if rational; 0 if non-rational or not a Bezier curve.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_rational is NULL.
 * @retval OCCTL_WRONG_KIND       Not a Bezier curve.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bezier_is_rational(const occtl_graph_t* graph,
                                                                     occtl_rep_id_t       curve_id,
                                                                     int32_t* out_is_rational);

/**
 * Extracts the poles of a 2D B-spline curve.  Two-call pattern.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to query; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_buf    May be NULL on sizing call.
 * @param[in]  capacity   Elements @p out_buf can hold.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bspline_poles(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       curve_id,
                                                                occtl_point2_t*      out_buf,
                                                                size_t               capacity,
                                                                size_t*              out_count);

/**
 * Extracts the distinct knot values of a 2D B-spline curve.  Two-call pattern.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to query.
 * @param[out] out_buf    May be NULL on sizing call.
 * @param[in]  capacity   Elements @p out_buf can hold.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bspline_knots(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       curve_id,
                                                                double*              out_buf,
                                                                size_t               capacity,
                                                                size_t*              out_count);

/**
 * Extracts the knot multiplicities of a 2D B-spline curve.  Two-call pattern.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to query.
 * @param[out] out_buf    May be NULL on sizing call.
 * @param[in]  capacity   Elements @p out_buf can hold.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bspline_multiplicities(const occtl_graph_t* graph,
                                                                         occtl_rep_id_t curve_id,
                                                                         int32_t*       out_buf,
                                                                         size_t         capacity,
                                                                         size_t*        out_count);

/**
 * Extracts the per-pole weights of a rational 2D B-spline curve.  Two-call pattern.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to query; must be rational.
 * @param[out] out_buf    May be NULL on sizing call.
 * @param[in]  capacity   Elements @p out_buf can hold.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Not a B-spline or not rational.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bspline_weights(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       curve_id,
                                                                  double*              out_buf,
                                                                  size_t               capacity,
                                                                  size_t*              out_count);

/**
 * Extracts the expanded (flat) knot sequence of a 2D B-spline curve.  Two-call pattern.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to query.
 * @param[out] out_buf    May be NULL on sizing call.
 * @param[in]  capacity   Elements @p out_buf can hold.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_bspline_flat_knots(const occtl_graph_t* graph,
                                                                     occtl_rep_id_t       curve_id,
                                                                     double*              out_buf,
                                                                     size_t               capacity,
                                                                     size_t* out_count);

/**
 * Zero-copy view of the poles array of a 2D B-spline curve.
 *
 * Returns a direct pointer into the OCCT internal storage.  Valid only
 * while the underlying curve is alive and no mutating call has been made
 * on it.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  curve_id   Curve to query; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_data   Receives a borrowed pointer to the pole array.  Must be non-NULL.
 * @param[out] out_count  Receives the pole count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_data, or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Representation is not a B-spline.
 *
 * @threadsafe Yes (read-only).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_bspline_poles_view(occtl_graph_t*         graph,
                                   occtl_rep_id_t         curve_id,
                                   const occtl_point2_t** out_data,
                                   size_t*                out_count);

/**
 * Aggregate inspection view of a 2D B-spline curve.
 *
 * Read-only snapshot of every field a caller typically needs in the same
 * pass: scalars (degree, periodicity, continuity, counts) plus borrowed
 * pointers into the underlying OCCT storage for poles, weights, distinct
 * knots, multiplicities, and the expanded flat knot sequence.  One call
 * replaces ~10 atomized accessors; pair with #occtl_curve2d_as_bspline.
 *
 * Pointers in @p out borrow from the underlying curve and are valid until
 * the representation is freed from the graph.
 *
 * The caller declares the layout version they understand in @c struct_version
 * before the call; the library fills only fields up to that version.  The
 * @c weights pointer is set to NULL when @c is_rational is 0.
 */
typedef struct occtl_curve2d_bspline
{
  uint32_t    struct_version; /**< [in]  Must be #OCCTL_CURVE2D_BSPLINE_VERSION_1. */
  const void* p_next;         /**< Reserved for extensions; must be NULL. */

  int32_t degree;          /**< Polynomial degree. */
  int32_t is_rational;     /**< 0 = non-rational; 1 = rational (NURBS). */
  int32_t is_periodic;     /**< 0/1. */
  int32_t is_closed;       /**< 0/1. */
  int32_t continuity;      /**< Geometric continuity (#occtl_geom_continuity_t value). */
  size_t  pole_count;      /**< Number of poles. */
  size_t  knot_count;      /**< Number of distinct knot values. */
  size_t  flat_knot_count; /**< Sum of multiplicities (length of @c flat_knots). */

  /**
   * Borrowed view pointers.  Lifetime is tied to the parent curve and ends
   * when the parent curve is freed or mutated.
   */
  const occtl_point2_t* poles;   /**< [out] Borrows it; @c pole_count elements. */
  const double*         weights; /**< [out] Borrows it; @c pole_count elements,
                                  *        or NULL when @c is_rational == 0. */
  const double*  knots;          /**< [out] Borrows it; @c knot_count distinct knots. */
  const int32_t* multiplicities; /**< [out] Borrows it; @c knot_count multiplicities. */
  const double*  flat_knots;     /**< [out] Borrows it; @c flat_knot_count elements. */
} occtl_curve2d_bspline_t;

#define OCCTL_CURVE2D_BSPLINE_VERSION_1 1u

#define OCCTL_CURVE2D_BSPLINE_INIT                                                                 \
  {OCCTL_CURVE2D_BSPLINE_VERSION_1, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL}

/**
 * Runtime initialiser for #occtl_curve2d_bspline_t.
 *
 * @param[out] out  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_as_bspline
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_bspline_init(occtl_curve2d_bspline_t* out);

/**
 * Fills a caller-allocated #occtl_curve2d_bspline_t with a complete read-only
 * inspection view of the underlying 2D B-spline curve.
 *
 * Pointers in @p out borrow from the underlying curve and are valid until
 * the representation is freed from the graph.
 *
 * @param[in]     graph    Must be non-NULL.
 * @param[in]     curve_id Curve to inspect; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[in,out] out      Borrows it.  Caller-allocated; must be non-NULL.  The
 *                         caller sets @c struct_version on entry; the library
 *                         fills the remaining fields on success and leaves them
 *                         unchanged on failure.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out is NULL.
 * @retval OCCTL_VERSION_MISMATCH @c out->struct_version is not supported.
 * @retval OCCTL_WRONG_KIND       Representation is not a 2D B-spline curve.
 *
 * @threadsafe Yes (read-only).
 *
 * @sa occtl_curve2d_bspline_init, occtl_curve2d_bspline_poles_view,
 *     occtl_curve2d_bspline_degree, occtl_curve2d_bspline_pole_count
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_as_bspline(const occtl_graph_t*     graph,
                                                             occtl_rep_id_t           curve_id,
                                                             occtl_curve2d_bspline_t* out);

/**
 * Decomposes a 2D curve into adjacent Bezier curve segments.
 *
 * The implementation delegates conversion and segmentation to OCCT
 * @c Geom2dConvert and @c Geom2dConvert_BSplineCurveToBezierCurve.  Each
 * output entry is a new bezier curve rep id.  Free the returned array with
 * #occtl_curve2d_free_bezier_segments.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  curve_id      Curve to decompose.
 * @param[in]  options       Borrows it. May be NULL for default full-domain options.
 * @param[out] out_segments  Owns it. Receives a caller-owned array of Bezier
 *                           curve rep ids. Must be non-NULL.
 *                           Free with #occtl_curve2d_free_bezier_segments.
 * @param[out] out_count     Borrows it. Receives the number of returned
 *                           segments. Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_segments, or @p out_count
 *                                is NULL; @c p_next is non-NULL; a 0/1 flag
 *                                is invalid; range/tolerance values are invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c options->struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT could not convert or segment the curve.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_create_bezier, occtl_curve_bezier_segments_options_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_to_bezier_segments(occtl_graph_t*                               graph,
                                   occtl_rep_id_t                               curve_id,
                                   const occtl_curve_bezier_segments_options_t* options,
                                   occtl_rep_id_t**                             out_segments,
                                   size_t*                                      out_count);

/**
 * Frees the array returned by #occtl_curve2d_to_bezier_segments.
 *
 * NULL-tolerant; passing NULL is a no-op.
 *
 * @param[in] ids  Owns it.  May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve2d_to_bezier_segments
 */
OCCTL_API void OCCTL_CALL occtl_curve2d_free_bezier_segments(occtl_rep_id_t* ids);

/**
 * Evaluates the 2D curve at parameter @p u.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  curve_id  Curve to evaluate.
 * @param[in]  u         Parameter value.
 * @param[out] out_point Receives the point on the curve.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     Parameter is outside the domain.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_eval_d1, occtl_curve2d_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_d0(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          double               u,
                                                          occtl_point2_t*      out_point);

/**
 * Evaluates the 2D curve and its first derivative at @p u.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  curve_id  Curve to evaluate.
 * @param[in]  u         Parameter value.
 * @param[out] out_point Receives the point.  May be NULL.
 * @param[out] out_d1    Receives the first derivative vector.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     Parameter is outside the domain.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_eval_d0, occtl_curve2d_eval_d2
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_d1(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          double               u,
                                                          occtl_point2_t*      out_point,
                                                          occtl_vector2_t*     out_d1);

/**
 * Evaluates point, first, and second derivatives at @p u.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  curve_id  Curve to evaluate.
 * @param[in]  u         Parameter value.
 * @param[out] out_point Receives the point.  May be NULL.
 * @param[out] out_d1    Receives the first derivative vector.  May be NULL.
 * @param[out] out_d2    Receives the second derivative vector.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     Parameter is outside the domain.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_eval_d1, occtl_curve2d_eval_d3
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_d2(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          double               u,
                                                          occtl_point2_t*      out_point,
                                                          occtl_vector2_t*     out_d1,
                                                          occtl_vector2_t*     out_d2);

/**
 * Evaluates point and first three derivatives at @p u.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  curve_id  Curve to evaluate.
 * @param[in]  u         Parameter value.
 * @param[out] out_point Receives the point.  May be NULL.
 * @param[out] out_d1    Receives the first derivative vector.  May be NULL.
 * @param[out] out_d2    Receives the second derivative vector.  May be NULL.
 * @param[out] out_d3    Receives the third derivative vector.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     Parameter is outside the domain.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_eval_d2, occtl_curve2d_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_d3(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          double               u,
                                                          occtl_point2_t*      out_point,
                                                          occtl_vector2_t*     out_d1,
                                                          occtl_vector2_t*     out_d2,
                                                          occtl_vector2_t*     out_d3);

/**
 * Returns the N-th derivative vector of the 2D curve at @p u.
 *
 * @param[in]  graph          Must be non-NULL.
 * @param[in]  curve_id       Curve to evaluate.
 * @param[in]  u              Parameter value.
 * @param[in]  n              Derivative order (0 returns a zero-length vector
 *                             whose origin is on the curve; 1 returns the first
 *                             derivative, etc.).
 * @param[out] out_derivative Receives the derivative vector.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     Parameter is outside the domain.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_curve2d_eval_d0, occtl_curve2d_eval_d3
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_dn(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          double               u,
                                                          int32_t              n,
                                                          occtl_vector2_t*     out_derivative);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_CURVES2D_H */
