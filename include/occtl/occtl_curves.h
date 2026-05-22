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
 * @file occtl_curves.h
 * @brief OCCT-Light: 3D curve representation in the topology graph.
 *
 * A curve is stored as an #occtl_rep_id_t inside an #occtl_graph_t.  Every
 * curve function takes a graph pointer and a rep id.  The kind is available
 * via #occtl_curve_kind for O(1) introspection.
 *
 * This file covers construction, introspection, data extraction, and
 * parametric evaluation of 3D curve representations.
 */

#ifndef OCCTL_CURVES_H
#define OCCTL_CURVES_H

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
 * Creates a 3D curve representation from a line.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  line     Line data (origin + unit direction).
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_kind, occtl_curve_as_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_line(occtl_graph_t*    graph,
                                                            occtl_geom_line_t line,
                                                            occtl_rep_id_t*   out_id);

/**
 * Creates a 3D curve representation from a circle.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  circle   Circle data (frame + radius).  @c radius must be > 0.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID @c circle.radius is not positive.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_as_circle
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_circle(occtl_graph_t*      graph,
                                                              occtl_geom_circle_t circle,
                                                              occtl_rep_id_t*     out_id);

/**
 * Creates a 3D curve representation from an ellipse.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  ellipse  Ellipse data.  @c major_radius >= @c minor_radius > 0.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID Radii are not positive or major < minor.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_as_ellipse
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_ellipse(occtl_graph_t*       graph,
                                                               occtl_geom_ellipse_t ellipse,
                                                               occtl_rep_id_t*      out_id);

/**
 * Creates a 3D curve representation from a hyperbola.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  hyperbola  Hyperbola data.  Both radii must be > 0.
 * @param[out] out_id     Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID A radius is not positive.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_as_hyperbola
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_hyperbola(occtl_graph_t*         graph,
                                                                 occtl_geom_hyperbola_t hyperbola,
                                                                 occtl_rep_id_t*        out_id);

/**
 * Creates a 3D curve representation from a parabola.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  parabola Parabola data.  @c focal_length must be > 0.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID @c focal_length is not positive.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_as_parabola
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_parabola(occtl_graph_t*        graph,
                                                                occtl_geom_parabola_t parabola,
                                                                occtl_rep_id_t*       out_id);

/**
 * Versioned create-info struct for B-spline curve construction.
 *
 * Knots are in compact form (distinct values + multiplicities), which is
 * OCCT's canonical representation.  Arrays are borrowed for the duration
 * of the #occtl_curve_create_bspline call; they are not retained afterwards.
 *
 * For a non-rational B-spline set @c weights to NULL.
 * For a rational B-spline (NURBS) provide @c weights with @c pole_count
 * entries; all values must be > 0.
 */
typedef struct occtl_curve_bspline_create_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_CURVE_BSPLINE_CREATE_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved for extensions; must be NULL. */
  const occtl_point3_t* poles;          /**< [in] Borrows it; count = @c pole_count. */
  size_t                pole_count;     /**< Number of poles. */
  const double*         weights;        /**< [in] Borrows it; count = @c pole_count when non-NULL.
                                         *       NULL -> non-rational; non-NULL -> rational (NURBS). */
  const double*  knots; /**< [in] Borrows it; distinct knot values; count = @c knot_count. */
  const int32_t* multiplicities; /**< [in] Borrows it; count = @c knot_count. */
  size_t         knot_count;     /**< Number of distinct knot values. */
  int32_t        degree;         /**< Polynomial degree; must be >= 1. */
  int32_t        is_periodic;    /**< 0 = non-periodic; 1 = periodic. */
} occtl_curve_bspline_create_info_t;

#define OCCTL_CURVE_BSPLINE_CREATE_INFO_VERSION_1 1u

#define OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT                                                       \
  {OCCTL_CURVE_BSPLINE_CREATE_INFO_VERSION_1, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, 0}

/**
 * Runtime initialiser for #occtl_curve_bspline_create_info_t.
 *
 * Sets @c struct_version and zeroes all other fields.  Use instead of the
 * @c _INIT macro when C macros are not available (e.g. C# P/Invoke).
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API void OCCTL_CALL
  occtl_curve_bspline_create_info_init(occtl_curve_bspline_create_info_t* info);

/**
 * Creates a 3D curve representation from a B-spline definition.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  info     Versioned create-info struct.  Must be non-NULL.  Arrays inside
 *                      are borrowed; they need not remain valid after the call returns.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL; or required array
 *                                fields are NULL.
 * @retval OCCTL_VERSION_MISMATCH @c info->struct_version is not supported.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the knot/pole/degree combination.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_bspline_degree, occtl_curve_bspline_poles
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_bspline(occtl_graph_t*                           graph,
                             const occtl_curve_bspline_create_info_t* info,
                             occtl_rep_id_t*                          out_id);

/**
 * Versioned create-info struct for Bezier curve construction.
 */
typedef struct occtl_curve_bezier_create_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_CURVE_BEZIER_CREATE_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved; must be NULL. */
  const occtl_point3_t* poles;          /**< [in] Borrows it; count = @c pole_count. */
  size_t                pole_count;     /**< Number of poles; must be >= 2. */
  const double*         weights;        /**< [in] Borrows it; count = @c pole_count when non-NULL.
                                         *       NULL -> non-rational. */
} occtl_curve_bezier_create_info_t;

#define OCCTL_CURVE_BEZIER_CREATE_INFO_VERSION_1 1u

#define OCCTL_CURVE_BEZIER_CREATE_INFO_INIT                                                        \
  {OCCTL_CURVE_BEZIER_CREATE_INFO_VERSION_1, NULL, NULL, 0, NULL}

/**
 * Runtime initialiser for #occtl_curve_bezier_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bezier
 */
OCCTL_API void OCCTL_CALL
  occtl_curve_bezier_create_info_init(occtl_curve_bezier_create_info_t* info);

/**
 * Creates a 3D curve representation from a Bezier definition.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  info     Versioned create-info.  Must be non-NULL.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the pole count or weights.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_bezier(occtl_graph_t*                          graph,
                            const occtl_curve_bezier_create_info_t* info,
                            occtl_rep_id_t*                         out_id);

/**
 * Creates a trimmed 3D curve (bounded section of a basis curve).
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  info     Versioned create-info.  Must be non-NULL.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL; or @c info->basis
 *                                is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID @c u_last <= @c u_first, or OCCT rejected the interval.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_trimmed(occtl_graph_t*                           graph,
                             const occtl_curve_trimmed_create_info_t* info,
                             occtl_rep_id_t*                          out_id);

/**
 * Versioned create-info struct for an offset curve.
 *
 * The constructor makes an internal deep copy of the basis curve.
 */
typedef struct occtl_curve_offset_create_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_CURVE_OFFSET_CREATE_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_rep_id_t  basis;          /**< [in] Deep-copied internally. */
  occtl_vector3_t offset_dir;     /**< Direction of the offset (must be non-zero). */
  double          offset;         /**< Signed offset distance. */
} occtl_curve_offset_create_info_t;

#define OCCTL_CURVE_OFFSET_CREATE_INFO_VERSION_1 1u

#define OCCTL_CURVE_OFFSET_CREATE_INFO_INIT                                                        \
  {OCCTL_CURVE_OFFSET_CREATE_INFO_VERSION_1, NULL, {0}, {0.0, 0.0, 1.0}, 0.0}

/**
 * Runtime initialiser for #occtl_curve_offset_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_offset
 */
OCCTL_API void OCCTL_CALL
  occtl_curve_offset_create_info_init(occtl_curve_offset_create_info_t* info);

/**
 * Creates an offset 3D curve.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  info     Versioned create-info.  Must be non-NULL.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL; or @c info->basis
 *                                is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID @c offset_dir has zero length.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_offset(occtl_graph_t*                          graph,
                            const occtl_curve_offset_create_info_t* info,
                            occtl_rep_id_t*                         out_id);

/**
 * Creates a new curve representation that is the reversed copy of @p curve_id.
 *
 * The returned rep owns an independent curve whose parameterisation
 * runs in the opposite direction: @c reversed(u) = @c curve(u_last - u).
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to reverse.
 * @param[out] out_id     Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_reverse(occtl_graph_t*  graph,
                                                        occtl_rep_id_t  curve_id,
                                                        occtl_rep_id_t* out_id);

/**
 * Creates a new curve representation by applying a 3D transform to @p curve_id.
 *
 * The returned rep owns an independent transformed copy.
 *
 * @param[in]  graph           Owning graph.  Must be non-NULL.
 * @param[in]  curve_id        Curve to transform.
 * @param[in]  transform       Transform to apply.
 * @param[out] out_id          Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_transformed(occtl_graph_t*    graph,
                                                            occtl_rep_id_t    curve_id,
                                                            occtl_transform_t transform,
                                                            occtl_rep_id_t*   out_id);

/**
 * Creates a new curve representation by translating @p curve_id.
 *
 * Convenience wrapper equivalent to applying a translation transform.
 *
 * @param[in]  graph        Owning graph.  Must be non-NULL.
 * @param[in]  curve_id     Curve to translate.
 * @param[in]  delta        Translation vector.
 * @param[out] out_id       Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_translated(occtl_graph_t*  graph,
                                                           occtl_rep_id_t  curve_id,
                                                           occtl_vector3_t delta,
                                                           occtl_rep_id_t* out_id);

/**
 * Creates a new curve representation by rotating @p curve_id.
 *
 * Convenience wrapper equivalent to applying a rotation transform.
 *
 * @param[in]  graph        Owning graph.  Must be non-NULL.
 * @param[in]  curve_id     Curve to rotate.
 * @param[in]  axis         Rotation axis (origin + direction).
 * @param[in]  angle        Rotation angle in radians.
 * @param[out] out_id       Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_rotated(occtl_graph_t*          graph,
                                                        occtl_rep_id_t          curve_id,
                                                        occtl_axis1_placement_t axis,
                                                        double                  angle,
                                                        occtl_rep_id_t*         out_id);

/**
 * Creates a new curve representation by scaling @p curve_id about @p origin.
 *
 * Convenience wrapper equivalent to applying a non-uniform scale transform.
 *
 * @param[in]  graph        Owning graph.  Must be non-NULL.
 * @param[in]  curve_id     Curve to scale.
 * @param[in]  origin       Origin point for scaling.
 * @param[in]  factor       Scale factor; must be non-zero.
 * @param[out] out_id       Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID @c factor is zero.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_scaled(occtl_graph_t*  graph,
                                                       occtl_rep_id_t  curve_id,
                                                       occtl_point3_t  origin,
                                                       double          factor,
                                                       occtl_rep_id_t* out_id);

/**
 * Computes the length of the curve over its full parameter range.
 *
 * The length is computed by numerical integration using OCCT's
 * @c GCPnts_AbscissaPoint.
 *
 * @param[in]  graph       Owning graph.  Must be non-NULL.
 * @param[in]  curve_id    Curve to measure.
 * @param[out] out_length  Receives the curve length.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_length is NULL.
 *
 * @threadsafe Only when distinct @p graph instances are used.
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_length(const occtl_graph_t* graph,
                                                       occtl_rep_id_t       curve_id,
                                                       double*              out_length);

/**
 * Projects a 3D point onto the curve and returns the closest parameter.
 *
 * Uses OCCT's @c GeomAPI_ProjectPointOnCurve.  When multiple solutions
 * exist (e.g. a circle), the closest point is returned.
 *
 * @param[in]  graph         Owning graph.  Must be non-NULL.
 * @param[in]  curve_id      Curve to project onto.
 * @param[in]  point         Point to project.
 * @param[out] out_param     Receives the parameter of the closest point on the curve.
 *                           Must be non-NULL.
 * @param[out] out_distance  Receives the shortest distance.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_param is NULL.
 *
 * @threadsafe Only when distinct @p graph instances are used.
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_project_point(const occtl_graph_t* graph,
                                                              occtl_rep_id_t       curve_id,
                                                              occtl_point3_t       point,
                                                              double*              out_param,
                                                              double*              out_distance);

/**
 * Returns the parameter on the curve nearest to a given 3D point.
 *
 * Convenience function equivalent to #occtl_curve_project_point with
 * @c out_distance = NULL.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to query.
 * @param[in]  point      Point to find the parameter for.
 * @param[out] out_param  Receives the parameter.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_param is NULL.
 *
 * @threadsafe Only when distinct @p graph instances are used.
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_parameter_of_point(const occtl_graph_t* graph,
                                                                   occtl_rep_id_t       curve_id,
                                                                   occtl_point3_t       point,
                                                                   double*              out_param);

/**
 * Returns the kind of geometry wrapped by a curve representation.
 *
 * @param[in]  graph     Owning graph.  Must be non-NULL.
 * @param[in]  curve_id  Curve to query.
 * @param[out] out_kind  Receives the curve kind.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_kind is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_as_circle, occtl_curve_as_ellipse
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_kind(const occtl_graph_t* graph,
                                                     occtl_rep_id_t       curve_id,
                                                     occtl_curve_kind_t*  out_kind);

/**
 * Returns non-zero if the curve is periodic.
 *
 * @param[in]  graph            Owning graph.  Must be non-NULL.
 * @param[in]  curve_id         Curve to query.
 * @param[out] out_is_periodic  Receives 1 if periodic, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_periodic is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_is_periodic(const occtl_graph_t* graph,
                                                            occtl_rep_id_t       curve_id,
                                                            int32_t*             out_is_periodic);

/**
 * Returns non-zero if the curve is closed.
 *
 * A curve is closed when its start and end points coincide within OCCT's
 * confusion tolerance.  Periodic curves are always closed; the converse
 * does not hold.
 *
 * @param[in]  graph         Owning graph.  Must be non-NULL.
 * @param[in]  curve_id      Curve to query.
 * @param[out] out_is_closed Receives 1 if closed, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_closed is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_is_periodic
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_is_closed(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          int32_t*             out_is_closed);

/**
 * Returns the continuity class of the curve.
 *
 * @param[in]  graph            Owning graph.  Must be non-NULL.
 * @param[in]  curve_id         Curve to query.
 * @param[out] out_continuity   Receives the continuity class.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_continuity is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_continuity(const occtl_graph_t*     graph,
                                                           occtl_rep_id_t           curve_id,
                                                           occtl_geom_continuity_t* out_continuity);

/**
 * Returns the parameter range of the curve.
 *
 * For analytic infinite curves (lines, parabolas, hyperbolas) OCCT returns
 * @c -Precision::Infinite() and @c +Precision::Infinite() (numerically
 * @c -1e100 and @c +1e100).  Bounded analytic curves (full circles, ellipses)
 * return [0, 2@c pi].  Trimmed and B-spline curves return their explicit range.
 *
 * @param[in]  graph       Owning graph.  Must be non-NULL.
 * @param[in]  curve_id    Curve to query.
 * @param[out] out_u_min   First (start) parameter.  May be NULL.
 * @param[out] out_u_max   Last (end) parameter.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_parameter_range(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       curve_id,
                                                                double*              out_u_min,
                                                                double*              out_u_max);

/**
 * Extracts line data from a curve representation.
 *
 * @param[in]  graph     Owning graph.  Must be non-NULL.
 * @param[in]  curve_id  Curve to extract; kind must be @c OCCTL_CURVE_KIND_LINE.
 * @param[out] out_line  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_line is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a line.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_line(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        occtl_geom_line_t*   out_line);

/**
 * Extracts circle data from a curve representation.
 *
 * @param[in]  graph       Owning graph.  Must be non-NULL.
 * @param[in]  curve_id    Curve to extract; kind must be @c OCCTL_CURVE_KIND_CIRCLE.
 * @param[out] out_circle  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_circle is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a circle.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_circle(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          occtl_geom_circle_t* out_circle);

/**
 * Extracts ellipse data from a curve representation.
 *
 * @param[in]  graph        Owning graph.  Must be non-NULL.
 * @param[in]  curve_id     Curve to extract; kind must be @c OCCTL_CURVE_KIND_ELLIPSE.
 * @param[out] out_ellipse  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_ellipse is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not an ellipse.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_ellipse(const occtl_graph_t*  graph,
                                                           occtl_rep_id_t        curve_id,
                                                           occtl_geom_ellipse_t* out_ellipse);

/**
 * Extracts hyperbola data from a curve representation.
 *
 * @param[in]  graph          Owning graph.  Must be non-NULL.
 * @param[in]  curve_id       Curve to extract; kind must be @c OCCTL_CURVE_KIND_HYPERBOLA.
 * @param[out] out_hyperbola  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_hyperbola is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a hyperbola.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_hyperbola(const occtl_graph_t*    graph,
                                                             occtl_rep_id_t          curve_id,
                                                             occtl_geom_hyperbola_t* out_hyperbola);

/**
 * Extracts parabola data from a curve representation.
 *
 * @param[in]  graph         Owning graph.  Must be non-NULL.
 * @param[in]  curve_id      Curve to extract; kind must be @c OCCTL_CURVE_KIND_PARABOLA.
 * @param[out] out_parabola  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_parabola is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a parabola.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_parabola(const occtl_graph_t*   graph,
                                                            occtl_rep_id_t         curve_id,
                                                            occtl_geom_parabola_t* out_parabola);

/**
 * Extracts the parameter bounds of a trimmed curve.
 *
 * The reported @c u_first / @c u_last are the parameter bounds the trimmed
 * curve was constructed with (already adjusted for any sense flip at
 * construction time), so reconstructing via #occtl_curve_create_trimmed
 * with @c sense=1 reproduces an equivalent curve.
 *
 * @param[in]  graph        Owning graph.  Must be non-NULL.
 * @param[in]  curve_id     Curve to extract; kind must be @c OCCTL_CURVE_KIND_TRIMMED.
 * @param[out] out_u_first  First parameter.  May be NULL.
 * @param[out] out_u_last   Last parameter.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a trimmed curve.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_trimmed(const occtl_graph_t* graph,
                                                           occtl_rep_id_t       curve_id,
                                                           double*              out_u_first,
                                                           double*              out_u_last);

/**
 * Extracts the scalar offset and reference direction of an offset curve.
 *
 * @param[in]  graph           Owning graph.  Must be non-NULL.
 * @param[in]  curve_id        Curve to extract; kind must be @c OCCTL_CURVE_KIND_OFFSET.
 * @param[out] out_offset      Signed offset distance.  May be NULL.
 * @param[out] out_offset_dir  Reference direction (unit length).  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not an offset curve.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_offset(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          double*              out_offset,
                                                          occtl_vector3_t*     out_offset_dir);

/**
 * Returns the polynomial degree of a B-spline curve.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to query; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_degree Receives the degree.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_degree is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_degree(const occtl_graph_t* graph,
                                                               occtl_rep_id_t       curve_id,
                                                               int32_t*             out_degree);

/**
 * Returns the pole count of a B-spline curve.
 *
 * @param[in]  graph         Owning graph.  Must be non-NULL.
 * @param[in]  curve_id      Curve to query; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_count     Receives the pole count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_pole_count(const occtl_graph_t* graph,
                                                                   occtl_rep_id_t       curve_id,
                                                                   size_t*              out_count);

/**
 * Returns the knot count (number of distinct knot values) of a B-spline curve.
 *
 * @param[in]  graph         Owning graph.  Must be non-NULL.
 * @param[in]  curve_id      Curve to query; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_count     Receives the knot count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_knot_count(const occtl_graph_t* graph,
                                                                   occtl_rep_id_t       curve_id,
                                                                   size_t*              out_count);

/**
 * Returns non-zero if the B-spline curve is rational (has per-pole weights).
 *
 * @param[in]  graph            Owning graph.  Must be non-NULL.
 * @param[in]  curve_id         Curve to query; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_is_rational  Receives 1 if rational, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_rational is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_is_rational(const occtl_graph_t* graph,
                                                                    occtl_rep_id_t       curve_id,
                                                                    int32_t* out_is_rational);

/**
 * Returns the polynomial degree of a Bezier curve.
 *
 * Equivalent to @c pole_count - 1 but exposed for symmetry with
 * #occtl_curve_bspline_degree.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to query; kind must be @c OCCTL_CURVE_KIND_BEZIER.
 * @param[out] out_degree Receives the degree.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_degree is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a Bezier curve.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bezier
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bezier_degree(const occtl_graph_t* graph,
                                                              occtl_rep_id_t       curve_id,
                                                              int32_t*             out_degree);

/**
 * Returns the pole count of a Bezier curve.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to query; kind must be @c OCCTL_CURVE_KIND_BEZIER.
 * @param[out] out_count  Receives the pole count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a Bezier curve.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bezier
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bezier_pole_count(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       curve_id,
                                                                  size_t*              out_count);

/**
 * Returns non-zero if the Bezier curve is rational (has per-pole weights).
 *
 * @param[in]  graph            Owning graph.  Must be non-NULL.
 * @param[in]  curve_id         Curve to query; kind must be @c OCCTL_CURVE_KIND_BEZIER.
 * @param[out] out_is_rational  Receives 1 if rational, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_rational is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a Bezier curve.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bezier
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bezier_is_rational(const occtl_graph_t* graph,
                                                                   occtl_rep_id_t       curve_id,
                                                                   int32_t* out_is_rational);

/**
 * Extracts the poles of a B-spline curve into a caller-supplied buffer.
 *
 * Two-call pattern: call with @p out_buf == NULL to learn the required count in
 * @p out_count, allocate, then call again with a buffer of at least that size.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to extract; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_buf    Buffer to receive pole data.  May be NULL on the sizing call.
 * @param[in]  capacity   Number of elements @p out_buf can hold.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success (poles written).
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL @p out_buf is non-NULL but @p capacity < *@p out_count.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_poles(const occtl_graph_t* graph,
                                                              occtl_rep_id_t       curve_id,
                                                              occtl_point3_t*      out_buf,
                                                              size_t               capacity,
                                                              size_t*              out_count);

/**
 * Extracts the distinct knot values of a B-spline curve.
 *
 * Two-call pattern: same as #occtl_curve_bspline_poles.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to extract; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_buf    Buffer to receive knot values.  May be NULL on sizing call.
 * @param[in]  capacity   Capacity of @p out_buf in elements.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_knots(const occtl_graph_t* graph,
                                                              occtl_rep_id_t       curve_id,
                                                              double*              out_buf,
                                                              size_t               capacity,
                                                              size_t*              out_count);

/**
 * Extracts the knot multiplicities of a B-spline curve.
 *
 * Two-call pattern: same as #occtl_curve_bspline_poles.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to extract; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_buf    Buffer to receive multiplicity values.  May be NULL on sizing call.
 * @param[in]  capacity   Capacity of @p out_buf in elements.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_multiplicities(const occtl_graph_t* graph,
                                                                       occtl_rep_id_t curve_id,
                                                                       int32_t*       out_buf,
                                                                       size_t         capacity,
                                                                       size_t*        out_count);

/**
 * Extracts the per-pole weights of a rational B-spline curve.
 *
 * Returns @c OCCTL_WRONG_KIND if the curve is non-rational.  Check
 * #occtl_curve_bspline_is_rational before calling this function.
 *
 * Two-call pattern: same as #occtl_curve_bspline_poles.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to extract; kind must be @c OCCTL_CURVE_KIND_BSPLINE;
 *                        curve must be rational.
 * @param[out] out_buf    Buffer to receive weight values.  May be NULL on sizing call.
 * @param[in]  capacity   Capacity of @p out_buf in elements.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline, or B-spline is non-rational.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_weights(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       curve_id,
                                                                double*              out_buf,
                                                                size_t               capacity,
                                                                size_t*              out_count);

/**
 * Zero-copy view of the poles array of a B-spline curve.
 *
 * Returns a direct pointer into the OCCT internal storage.  Valid only
 * while the graph rep is alive and no mutating call has been made on it.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to view; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_data   Receives a borrowed pointer to the pole array.  Must be non-NULL.
 * @param[out] out_count  Receives the pole count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_data, or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline.
 *
 * @threadsafe Yes (read-only).
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_poles_view(const occtl_graph_t*   graph,
                                                                   occtl_rep_id_t         curve_id,
                                                                   const occtl_point3_t** out_data,
                                                                   size_t* out_count);

/**
 * Extracts the expanded (flat) knot sequence of a B-spline curve.
 *
 * The flat knot sequence repeats each distinct knot value by its
 * multiplicity, which is the form most numerical libraries (NumPy, etc.)
 * expect.  The total count equals the sum of multiplicities.
 *
 * Two-call pattern: same as #occtl_curve_bspline_poles.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to extract; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[out] out_buf    Buffer to receive flat knot values.  May be NULL on sizing call.
 * @param[in]  capacity   Capacity of @p out_buf in elements.
 * @param[out] out_count  Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bspline
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_flat_knots(const occtl_graph_t* graph,
                                                                   occtl_rep_id_t       curve_id,
                                                                   double*              out_buf,
                                                                   size_t               capacity,
                                                                   size_t*              out_count);

/**
 * Aggregate inspection view of a B-spline curve.
 *
 * Read-only snapshot of every field a caller typically needs in the same
 * pass: scalars (degree, periodicity, continuity, counts) plus borrowed
 * pointers into the underlying OCCT storage for poles, weights, distinct
 * knots, multiplicities, and the expanded flat knot sequence.  One call
 * replaces ~10 atomized accessors; pair with #occtl_curve_as_bspline.
 *
 * Pointers in @p out borrow from the graph rep and are valid until the rep
 * is removed from the graph.
 *
 * The caller declares the layout version they understand in @c struct_version
 * before the call; the library fills only fields up to that version.  The
 * @c weights pointer is set to NULL when @c is_rational is 0.
 */
typedef struct occtl_curve_bspline
{
  uint32_t    struct_version; /**< [in]  Must be #OCCTL_CURVE_BSPLINE_VERSION_1. */
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
   * Borrowed view pointers.  Lifetime is tied to the graph rep and ends
   * when the rep is removed.
   */
  const occtl_point3_t* poles;   /**< [out] Borrows it; @c pole_count elements. */
  const double*         weights; /**< [out] Borrows it; @c pole_count elements,
                                  *        or NULL when @c is_rational == 0. */
  const double*  knots;          /**< [out] Borrows it; @c knot_count distinct knots. */
  const int32_t* multiplicities; /**< [out] Borrows it; @c knot_count multiplicities. */
  const double*  flat_knots;     /**< [out] Borrows it; @c flat_knot_count elements. */
} occtl_curve_bspline_t;

#define OCCTL_CURVE_BSPLINE_VERSION_1 1u

#define OCCTL_CURVE_BSPLINE_INIT                                                                   \
  {OCCTL_CURVE_BSPLINE_VERSION_1, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL}

/**
 * Runtime initialiser for #occtl_curve_bspline_t.
 *
 * Sets @c struct_version and zeroes all other fields.  Use instead of the
 * @c _INIT macro when C macros are not available (e.g. C# P/Invoke).
 *
 * @param[out] out  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_as_bspline
 */
OCCTL_API void OCCTL_CALL occtl_curve_bspline_init(occtl_curve_bspline_t* out);

/**
 * Fills a caller-allocated #occtl_curve_bspline_t with a complete read-only
 * inspection view of the underlying B-spline curve.
 *
 * Pointers in @p out borrow from the graph rep and are valid until the rep
 * is removed from the graph.
 *
 * @param[in]     graph    Owning graph.  Must be non-NULL.
 * @param[in]     curve_id Curve to inspect; kind must be @c OCCTL_CURVE_KIND_BSPLINE.
 * @param[in,out] out      Borrows it.  Caller-allocated; must be non-NULL.  The
 *                         caller sets @c struct_version on entry; the library
 *                         fills the remaining fields on success and leaves them
 *                         unchanged on failure.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out is NULL.
 * @retval OCCTL_VERSION_MISMATCH @c out->struct_version is not supported.
 * @retval OCCTL_WRONG_KIND       Curve is not a B-spline curve.
 *
 * @threadsafe Yes (read-only).
 *
 * @sa occtl_curve_bspline_init, occtl_curve_bspline_poles_view,
 *     occtl_curve_bspline_degree, occtl_curve_bspline_pole_count
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_bspline(const occtl_graph_t*   graph,
                                                           occtl_rep_id_t         curve_id,
                                                           occtl_curve_bspline_t* out);

/**
 * Evaluates the curve at parameter @p u.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to evaluate.
 * @param[in]  u          Parameter value.
 * @param[out] out_point  Receives the point on the curve.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     @p u is outside the parameter domain.
 *
 * @threadsafe Only when distinct @p graph instances are used.
 *
 * @sa occtl_curve_eval_d1, occtl_curve_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_d0(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        double               u,
                                                        occtl_point3_t*      out_point);

/**
 * Evaluates the curve and its first derivative at @p u.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to evaluate.
 * @param[in]  u          Parameter value.
 * @param[out] out_point  Receives the point.  May be NULL.
 * @param[out] out_d1     Receives the first derivative vector.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     @p u is outside the parameter domain.
 *
 * @threadsafe Only when distinct @p graph instances are used.
 *
 * @sa occtl_curve_eval_d0, occtl_curve_eval_d2
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_d1(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        double               u,
                                                        occtl_point3_t*      out_point,
                                                        occtl_vector3_t*     out_d1);

/**
 * Evaluates point, first, and second derivatives at @p u.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to evaluate.
 * @param[in]  u          Parameter value.
 * @param[out] out_point  Receives the point.  May be NULL.
 * @param[out] out_d1     Receives the first derivative vector.  May be NULL.
 * @param[out] out_d2     Receives the second derivative vector.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     @p u is outside the parameter domain.
 *
 * @threadsafe Only when distinct @p graph instances are used.
 *
 * @sa occtl_curve_eval_d1, occtl_curve_eval_d3
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_d2(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        double               u,
                                                        occtl_point3_t*      out_point,
                                                        occtl_vector3_t*     out_d1,
                                                        occtl_vector3_t*     out_d2);

/**
 * Evaluates point and first three derivatives at @p u.
 *
 * @param[in]  graph      Owning graph.  Must be non-NULL.
 * @param[in]  curve_id   Curve to evaluate.
 * @param[in]  u          Parameter value.
 * @param[out] out_point  Receives the point.  May be NULL.
 * @param[out] out_d1     Receives the first derivative vector.  May be NULL.
 * @param[out] out_d2     Receives the second derivative vector.  May be NULL.
 * @param[out] out_d3     Receives the third derivative vector.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     @p u is outside the parameter domain.
 *
 * @threadsafe Only when distinct @p graph instances are used.
 *
 * @sa occtl_curve_eval_d2, occtl_curve_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_d3(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        double               u,
                                                        occtl_point3_t*      out_point,
                                                        occtl_vector3_t*     out_d1,
                                                        occtl_vector3_t*     out_d2,
                                                        occtl_vector3_t*     out_d3);

/**
 * Returns the N-th derivative vector at @p u.
 *
 * @param[in]  graph           Owning graph.  Must be non-NULL.
 * @param[in]  curve_id        Curve to evaluate.
 * @param[in]  u               Parameter value.
 * @param[in]  n               Derivative order (0 returns a zero-length vector
 *                             whose origin is on the curve; 1 returns the first
 *                             derivative, etc.).
 * @param[out] out_derivative  Receives the derivative vector.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL.
 * @retval OCCTL_OUT_OF_RANGE     @p u is outside the parameter domain.
 *
 * @threadsafe Only when distinct @p graph instances are used.
 *
 * @sa occtl_curve_eval_d0, occtl_curve_eval_d3
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_dn(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        double               u,
                                                        int32_t              n,
                                                        occtl_vector3_t*     out_derivative);

/**
 * Versioned create-info for a NACA 4-digit airfoil profile curve.
 *
 * The generated profile lies in the XY plane with the leading edge at
 * @c (0,0,0) and the chord along +X.  Use curve transforms to place or
 * scale it after construction.
 */
typedef struct occtl_curve_airfoil_naca4_info
{
  uint32_t    struct_version;  /**< Must be #OCCTL_CURVE_AIRFOIL_NACA4_INFO_VERSION_1. */
  const void* p_next;          /**< Reserved; must be NULL. */
  double      max_camber;      /**< Maximum camber as a chord fraction; must be >= 0. */
  double      camber_position; /**< Position of maximum camber as a chord fraction in [0,1]. */
  double      thickness;       /**< Maximum thickness as a chord fraction; must be > 0. */
  double      chord_length;    /**< Chord length in model units; must be > 0. */
  size_t      point_count;     /**< Number of samples per side; must be >= 5. */
  int32_t
    finite_trailing_edge; /**< 0/1; use NACA finite trailing-edge coefficient when non-zero. */
  int32_t degree_min;     /**< Minimum B-spline degree for OCCT fitting; must be >= 1. */
  int32_t degree_max; /**< Maximum B-spline degree for OCCT fitting; must be >= @c degree_min. */
  double  tolerance;  /**< OCCT approximation tolerance; must be >= 0. */
} occtl_curve_airfoil_naca4_info_t;

#define OCCTL_CURVE_AIRFOIL_NACA4_INFO_VERSION_1 1u

#define OCCTL_CURVE_AIRFOIL_NACA4_INFO_INIT                                                        \
  {OCCTL_CURVE_AIRFOIL_NACA4_INFO_VERSION_1, NULL, 0.0, 0.0, 0.12, 1.0, 50, 0, 3, 8, 1.0e-5}

/**
 * Runtime initialiser for #occtl_curve_airfoil_naca4_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_airfoil_naca4
 */
OCCTL_API void OCCTL_CALL
  occtl_curve_airfoil_naca4_info_init(occtl_curve_airfoil_naca4_info_t* info);

/**
 * Creates a B-spline curve approximating a NACA 4-digit airfoil profile.
 *
 * The wrapper generates cosine-spaced NACA profile samples and delegates
 * curve fitting to OCCT's @c GeomAPI_PointsToBSpline.  For a standard NACA
 * 2412 profile, set @c max_camber=0.02, @c camber_position=0.4, and
 * @c thickness=0.12.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  info     Borrows it. Versioned airfoil parameters; must be non-NULL.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL, @c p_next
 *                                is non-NULL, a 0/1 flag is invalid, or a
 *                                numeric parameter is outside its valid range.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the generated point fit.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_airfoil_naca4_info_init, occtl_curve_create_approximated
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_airfoil_naca4(occtl_graph_t*                          graph,
                                   const occtl_curve_airfoil_naca4_info_t* info,
                                   occtl_rep_id_t*                         out_id);

/**
 * Creates a trimmed circular arc through three 3D points.
 *
 * The arc runs from @p p1 through @p p2 to @p p3.  The three points
 * must not be collinear and must be distinct.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  p1       First point.
 * @param[in]  p2       Second point (intermediate).
 * @param[in]  p3       Third point.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID  The three points are collinear or
 *                                 cannot define a unique circle.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_circle, occtl_curve_create_trimmed
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_arc_of_circle_3pt(occtl_graph_t*  graph,
                                                                         occtl_point3_t  p1,
                                                                         occtl_point3_t  p2,
                                                                         occtl_point3_t  p3,
                                                                         occtl_rep_id_t* out_id);

/**
 * Versioned create-info struct for curve interpolation.
 *
 * Interpolates a B-spline curve passing through the given points.
 * Uses OCCT's @c GeomAPI_Interpolate internally, which computes
 * uniform parameters and creates a C2 continuous cubic B-spline.
 */
typedef struct occtl_curve_interpolated_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_CURVE_INTERPOLATED_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved; must be NULL. */
  const occtl_point3_t* points;         /**< [in] Borrows it; count = @c point_count. */
  size_t                point_count;    /**< Number of interpolation points. */
  int32_t               is_periodic;    /**< 0 = non-periodic; 1 = periodic. */
  double                tolerance;      /**< Interpolation tolerance. */
} occtl_curve_interpolated_info_t;

#define OCCTL_CURVE_INTERPOLATED_INFO_VERSION_1 1u
#define OCCTL_CURVE_INTERPOLATED_INFO_INIT                                                         \
  {OCCTL_CURVE_INTERPOLATED_INFO_VERSION_1, NULL, NULL, 0, 0, 0.0}

/**
 * Runtime initialiser for #occtl_curve_interpolated_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_interpolated
 */
OCCTL_API void OCCTL_CALL occtl_curve_interpolated_info_init(occtl_curve_interpolated_info_t* info);

/**
 * Creates a B-spline curve that interpolates (passes through) the given points.
 *
 * Uses OCCT's @c GeomAPI_Interpolate.  The resulting curve is always a
 * C2 continuous cubic B-spline.  Set @c is_periodic to make the curve
 * periodic (first and last points must match within tolerance).
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  info     Versioned create-info.  Must be non-NULL.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL, or
 *                                required fields are NULL / zero.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID Interpolation failed.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_interpolated_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_interpolated(occtl_graph_t*                         graph,
                                  const occtl_curve_interpolated_info_t* info,
                                  occtl_rep_id_t*                        out_id);

/**
 * Versioned create-info struct for curve approximation (fitting).
 *
 * Fits a B-spline curve to the given points within the specified tolerance
 * using OCCT's @c GeomAPI_PointsToBSpline.  The resulting curve does not
 * necessarily pass through the points; it approximates them.
 */
typedef struct occtl_curve_approximated_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_CURVE_APPROXIMATED_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved; must be NULL. */
  const occtl_point3_t* points;         /**< [in] Borrows it; count = @c point_count. */
  size_t                point_count;    /**< Number of points to fit. */
  int32_t               degree_min;     /**< Minimum degree for the B-spline (>= 1). */
  int32_t               degree_max;     /**< Maximum degree for the B-spline. */
  double                tolerance;      /**< Approximation tolerance in 3D. */
  int32_t               is_periodic;    /**< 0 = non-periodic; 1 = periodic (not yet supported). */
} occtl_curve_approximated_info_t;

#define OCCTL_CURVE_APPROXIMATED_INFO_VERSION_1 1u
#define OCCTL_CURVE_APPROXIMATED_INFO_INIT                                                         \
  {OCCTL_CURVE_APPROXIMATED_INFO_VERSION_1, NULL, NULL, 0, 1, 3, 1e-3, 0}

/**
 * Runtime initialiser for #occtl_curve_approximated_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_approximated
 */
OCCTL_API void OCCTL_CALL occtl_curve_approximated_info_init(occtl_curve_approximated_info_t* info);

/**
 * Fits a B-spline curve to the given points within tolerance.
 *
 * Uses OCCT's @c GeomAPI_PointsToBSpline.  The resulting curve
 * approximates the points; it does not necessarily pass through them.
 *
 * @param[in]  graph    Owning graph.  Must be non-NULL.
 * @param[in]  info     Versioned create-info.  Must be non-NULL.
 * @param[out] out_id   Receives the new rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL, or
 *                                required fields are NULL / zero.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID Approximation failed.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_curve_approximated_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_approximated(occtl_graph_t*                         graph,
                                  const occtl_curve_approximated_info_t* info,
                                  occtl_rep_id_t*                        out_id);

/**
 * Decomposes a curve into adjacent Bezier curve segments.
 *
 * The implementation delegates conversion and segmentation to OCCT
 * @c GeomConvert and @c GeomConvert_BSplineCurveToBezierCurve.  Each output
 * entry is a new rep id of kind #OCCTL_CURVE_KIND_BEZIER.  Free the returned
 * array with #occtl_curve_free_bezier_segments.
 *
 * @param[in]  graph        Owning graph.  Must be non-NULL.
 * @param[in]  curve_id     Curve to decompose.
 * @param[in]  options      Borrows it. May be NULL for default full-domain options.
 * @param[out] out_ids      Owns it. Receives an allocated array of rep ids.
 *                          Must be non-NULL.  Free with #occtl_curve_free_bezier_segments.
 * @param[out] out_count    Borrows it. Receives the number of returned
 *                          segments. Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_ids, or @p out_count
 *                                is NULL; @c p_next is non-NULL; a 0/1 flag
 *                                is invalid; range/tolerance values are invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c options->struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT could not convert or segment the curve.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_bezier, occtl_curve_bezier_segments_options_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_to_bezier_segments(occtl_graph_t*                               graph,
                                 occtl_rep_id_t                               curve_id,
                                 const occtl_curve_bezier_segments_options_t* options,
                                 occtl_rep_id_t**                             out_ids,
                                 size_t*                                      out_count);

/**
 * Frees the array returned by #occtl_curve_to_bezier_segments.
 *
 * NULL-tolerant; passing NULL is a no-op.
 *
 * @param[in] ids  Owns it.  May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_to_bezier_segments
 */
OCCTL_API void OCCTL_CALL occtl_curve_free_bezier_segments(occtl_rep_id_t* ids);

/**
 * Describes a single intersection point between two curves.
 */
typedef struct occtl_curve_intersection_point
{
  occtl_point3_t point;   /**< Intersection point in 3D. */
  double         param_a; /**< Parameter on curve A. */
  double         param_b; /**< Parameter on curve B. */
} occtl_curve_intersection_point_t;

/**
 * Finds the closest points between two curves.
 *
 * Uses OCCT's @c GeomAPI_ExtremaCurveCurve.  Returns all extrema points
 * (minima and maxima of distance) between the two curves.  When the
 * curves intersect, the distance is zero and the returned points coincide.
 *
 * The caller owns the returned array and must free it with
 * #occtl_curve_free_intersection_points.
 *
 * @param[in]  graph        Owning graph.  Must be non-NULL.
 * @param[in]  curve_id_a   First curve.
 * @param[in]  curve_id_b   Second curve.
 * @param[out] out_results  Receives a caller-owned array of intersection
 *                          points.  Must be non-NULL.
 *                          Free with #occtl_curve_free_intersection_points.
 * @param[out] out_count    Receives the number of intersection points.
 *                          Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_results,
 *                                or @p out_count is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_create_line
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_intersect(occtl_graph_t*                           graph,
                        occtl_rep_id_t                           curve_id_a,
                        occtl_rep_id_t                           curve_id_b,
                        const occtl_curve_intersection_point_t** out_results,
                        size_t*                                  out_count);

/**
 * Frees the array returned by #occtl_curve_intersect.
 *
 * NULL-tolerant; passing NULL is a no-op.
 *
 * @param[in] results  Owns it.  May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_curve_intersect
 */
OCCTL_API void OCCTL_CALL
  occtl_curve_free_intersection_points(occtl_curve_intersection_point_t* results);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_CURVES_H */
