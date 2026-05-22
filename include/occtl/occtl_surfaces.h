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
 * @file occtl_surfaces.h
 * @brief OCCT-Light: surface geometry via graph + rep_id.
 *
 * Surface geometry is accessed and created through the BRepGraph topology
 * DAG using #occtl_rep_id_t identifiers.  Every surface function takes an
 * #occtl_graph_t* as the first argument followed by the surface's rep id.
 *
 * Constructors and query functions write output rep ids into caller-provided
 * slots.  The caller passes the graph that owns the surface representation;
 * all rep ids are scoped to that graph instance.
 */

#ifndef OCCTL_SURFACES_H
#define OCCTL_SURFACES_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_geom.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @cond */
typedef struct occtl_graph occtl_graph_t;

/** @endcond */

/**
 * Discriminates the underlying geometry of a surface rep.
 */
typedef enum occtl_surface_kind
{
  OCCTL_SURFACE_KIND_PLANE       = 0, /**< Infinite plane. */
  OCCTL_SURFACE_KIND_CYLINDRICAL = 1, /**< Cylindrical surface. */
  OCCTL_SURFACE_KIND_CONICAL     = 2, /**< Conical surface. */
  OCCTL_SURFACE_KIND_SPHERICAL   = 3, /**< Spherical surface. */
  OCCTL_SURFACE_KIND_TOROIDAL    = 4, /**< Toroidal surface. */
  OCCTL_SURFACE_KIND_BSPLINE     = 5, /**< B-spline surface. */
  OCCTL_SURFACE_KIND_BEZIER      = 6, /**< Bezier surface. */
  OCCTL_SURFACE_KIND_REVOLUTION  = 7, /**< Surface of revolution. */
  OCCTL_SURFACE_KIND_EXTRUSION   = 8, /**< Surface of linear extrusion. */
  OCCTL_SURFACE_KIND_RECTANGULAR_TRIMMED =
    9, /**< Rectangular UV-bounded section of a basis surface. */
  OCCTL_SURFACE_KIND_OFFSET          = 10, /**< Offset of a basis surface. */
  OCCTL_SURFACE_KIND_UNDEFINED       = 11, /**< Unknown or unrecognised subtype. */
  OCCTL_SURFACE_KIND_RESERVED_FUTURE = 0x7fffffff
} occtl_surface_kind_t;

/**
 * Creates a plane surface rep in the graph.
 *
 * @param[in]  graph      Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id     Receives the rep id of the new surface.  Must be non-NULL.
 * @param[in]  plane      Plane data.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_create_plane(occtl_graph_t*     graph,
                                                               occtl_rep_id_t*    out_id,
                                                               occtl_geom_plane_t plane);

/**
 * Creates a cylindrical surface rep in the graph.
 *
 * @param[in]  graph     Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id    Receives the rep id of the new surface.  Must be non-NULL.
 * @param[in]  cylinder  Cylinder data.  @c radius must be > 0.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID @c radius is not positive.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_cylinder(occtl_graph_t*                   graph,
                                occtl_rep_id_t*                  out_id,
                                occtl_geom_cylindrical_surface_t cylinder);

/**
 * Creates a conical surface rep in the graph.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id Receives the rep id of the new surface.  Must be non-NULL.
 * @param[in]  cone   Cone data.  @c semi_angle must be in (0, pi/2).
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID Invalid @c semi_angle.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_create_cone(occtl_graph_t*               graph,
                                                              occtl_rep_id_t*              out_id,
                                                              occtl_geom_conical_surface_t cone);

/**
 * Creates a spherical surface rep in the graph.
 *
 * @param[in]  graph   Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id  Receives the rep id of the new surface.  Must be non-NULL.
 * @param[in]  sphere  Sphere data.  @c radius must be > 0.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID @c radius is not positive.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_sphere(occtl_graph_t*                 graph,
                              occtl_rep_id_t*                out_id,
                              occtl_geom_spherical_surface_t sphere);

/**
 * Creates a toroidal surface rep in the graph.
 *
 * @param[in]  graph   Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id  Receives the rep id of the new surface.  Must be non-NULL.
 * @param[in]  torus   Torus data.  Both radii must be > 0 and major > minor.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL.
 * @retval OCCTL_GEOMETRY_INVALID Invalid radii.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_create_torus(occtl_graph_t*                graph,
                                                               occtl_rep_id_t*               out_id,
                                                               occtl_geom_toroidal_surface_t torus);

/**
 * Versioned create-info for a surface of revolution.
 *
 * The profile curve is rotated 2pi around @c axis to form the surface.
 * The @c basis must be a valid curve rep id in the graph.
 */
typedef struct occtl_surface_revolution_create_info
{
  uint32_t       struct_version; /**< Must be #OCCTL_SURFACE_REVOLUTION_CREATE_INFO_VERSION_1. */
  const void*    p_next;
  occtl_rep_id_t basis;         /**< [in] Curve rep id.  Must be valid. */
  occtl_axis1_placement_t axis; /**< Rotation axis. */
} occtl_surface_revolution_create_info_t;

#define OCCTL_SURFACE_REVOLUTION_CREATE_INFO_VERSION_1 1u

#define OCCTL_SURFACE_REVOLUTION_CREATE_INFO_INIT                                                  \
  {                                                                                                \
    OCCTL_SURFACE_REVOLUTION_CREATE_INFO_VERSION_1, NULL, OCCTL_REP_ID_INVALID,                    \
    {                                                                                              \
      {0, 0, 0}, {0, 0, 1}                                                                         \
    }                                                                                              \
  }

/**
 * Runtime initialiser for #occtl_surface_revolution_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_revolution_create_info_init(occtl_surface_revolution_create_info_t* info);

/**
 * Creates a surface of revolution rep in the graph.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id Receives the rep id.  Must be non-NULL.
 * @param[in]  info   Versioned create-info.  Must be non-NULL.  The @c basis
 *                    must be a valid curve rep id.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL, or
 *                                @c info->basis is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the parameters.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_revolution(occtl_graph_t*                                graph,
                                  occtl_rep_id_t*                               out_id,
                                  const occtl_surface_revolution_create_info_t* info);

/**
 * Versioned create-info for a surface of linear extrusion.
 *
 * The profile curve is swept along @c direction to form the surface.
 */
typedef struct occtl_surface_extrusion_create_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_SURFACE_EXTRUSION_CREATE_INFO_VERSION_1. */
  const void*     p_next;
  occtl_rep_id_t  basis;     /**< [in] Curve rep id.  Must be valid. */
  occtl_vector3_t direction; /**< Extrusion direction (must be non-zero). */
} occtl_surface_extrusion_create_info_t;

#define OCCTL_SURFACE_EXTRUSION_CREATE_INFO_VERSION_1 1u

#define OCCTL_SURFACE_EXTRUSION_CREATE_INFO_INIT                                                   \
  {OCCTL_SURFACE_EXTRUSION_CREATE_INFO_VERSION_1, NULL, OCCTL_REP_ID_INVALID, {0.0, 0.0, 1.0}}

/**
 * Runtime initialiser for #occtl_surface_extrusion_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_extrusion_create_info_init(occtl_surface_extrusion_create_info_t* info);

/**
 * Creates a surface of linear extrusion rep in the graph.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id Receives the rep id.  Must be non-NULL.
 * @param[in]  info   Versioned create-info.  Must be non-NULL.  The @c basis
 *                    must be a valid curve rep id.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL, or
 *                                @c info->basis is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID @c direction has zero length.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_extrusion(occtl_graph_t*                               graph,
                                 occtl_rep_id_t*                              out_id,
                                 const occtl_surface_extrusion_create_info_t* info);

/**
 * Versioned create-info for a rectangular-trimmed surface.
 *
 * Trims the basis surface to the parametric box [@c u_first, @c u_last] x
 * [@c v_first, @c v_last].  When @c trim_u is non-zero the trim is applied
 * in U; when @c trim_v is non-zero it is applied in V.
 */
typedef struct occtl_surface_rectangular_trimmed_create_info
{
  uint32_t struct_version; /**< Must be #OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_VERSION_1. */
  const void*    p_next;   /**< Reserved; must be NULL. */
  occtl_rep_id_t basis;    /**< [in] Surface rep id.  Must be valid. */
  double         u_first;
  double         u_last;
  double         v_first;
  double         v_last;
  int32_t        u_sense; /**< 1 = same direction; -1 = reversed. */
  int32_t        v_sense; /**< 1 = same direction; -1 = reversed. */
} occtl_surface_rectangular_trimmed_create_info_t;

#define OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_VERSION_1 1u

#define OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_INIT                                         \
  {OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_VERSION_1,                                        \
   NULL,                                                                                           \
   OCCTL_REP_ID_INVALID,                                                                           \
   0.0,                                                                                            \
   1.0,                                                                                            \
   0.0,                                                                                            \
   1.0,                                                                                            \
   1,                                                                                              \
   1}

/**
 * Runtime initialiser for #occtl_surface_rectangular_trimmed_create_info_t.
 *
 * Sets all fields to #OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 */
OCCTL_API void OCCTL_CALL occtl_surface_rectangular_trimmed_create_info_init(
  occtl_surface_rectangular_trimmed_create_info_t* info);

/**
 * Creates a rectangular-trimmed surface rep in the graph.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id Receives the rep id.  Must be non-NULL.
 * @param[in]  info   Borrows it.  Versioned create-info.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL, or
 *                                @c info->basis is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID Basis surface or trim parameters are invalid.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_create_rectangular_trimmed(
  occtl_graph_t*                                         graph,
  occtl_rep_id_t*                                        out_id,
  const occtl_surface_rectangular_trimmed_create_info_t* info);

/**
 * Versioned create-info for an offset surface.
 */
typedef struct occtl_surface_offset_create_info
{
  uint32_t       struct_version; /**< Must be #OCCTL_SURFACE_OFFSET_CREATE_INFO_VERSION_1. */
  const void*    p_next;         /**< Reserved; must be NULL. */
  occtl_rep_id_t basis;          /**< [in] Surface rep id.  Must be valid. */
  double         offset;         /**< Signed offset distance along the surface normal. */
} occtl_surface_offset_create_info_t;

#define OCCTL_SURFACE_OFFSET_CREATE_INFO_VERSION_1 1u

#define OCCTL_SURFACE_OFFSET_CREATE_INFO_INIT                                                      \
  {OCCTL_SURFACE_OFFSET_CREATE_INFO_VERSION_1, NULL, OCCTL_REP_ID_INVALID, 0.0}

/**
 * Runtime initialiser for #occtl_surface_offset_create_info_t.
 *
 * Sets all fields to #OCCTL_SURFACE_OFFSET_CREATE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_create_offset
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_offset_create_info_init(occtl_surface_offset_create_info_t* info);

/**
 * Creates an offset surface rep in the graph.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id Receives the rep id.  Must be non-NULL.
 * @param[in]  info   Borrows it.  Versioned create-info.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL, or
 *                                @c info->basis is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID Basis surface is invalid for offsetting.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_offset(occtl_graph_t*                            graph,
                              occtl_rep_id_t*                           out_id,
                              const occtl_surface_offset_create_info_t* info);

/**
 * Versioned create-info struct for B-spline surface construction.
 *
 * Poles are stored row-major: index = @c u * @c v_pole_count + @c v.
 * U and V knot sequences are each in compact form (distinct + multiplicities).
 */
typedef struct occtl_surface_bspline_create_info
{
  uint32_t    struct_version;  /**< Must be #OCCTL_SURFACE_BSPLINE_CREATE_INFO_VERSION_1. */
  const void* p_next;          /**< Reserved; must be NULL. */
  const occtl_point3_t* poles; /**< [in] Borrows it; row-major: index = u*v_pole_count + v. */
  size_t                u_pole_count; /**< Number of poles in the U direction. */
  size_t                v_pole_count; /**< Number of poles in the V direction. */
  const double*         weights; /**< [in] Borrows it; NULL = non-rational; same layout as poles. */
  const double*         u_knots; /**< [in] Borrows it; distinct U knot values. */
  const int32_t*        u_multiplicities; /**< [in] Borrows it; count = @c u_knot_count. */
  size_t                u_knot_count;
  const double*         v_knots;          /**< [in] Borrows it; distinct V knot values. */
  const int32_t*        v_multiplicities; /**< [in] Borrows it; count = @c v_knot_count. */
  size_t                v_knot_count;
  int32_t               u_degree;
  int32_t               v_degree;
  int32_t               is_u_periodic; /**< 0/1. */
  int32_t               is_v_periodic; /**< 0/1. */
} occtl_surface_bspline_create_info_t;

#define OCCTL_SURFACE_BSPLINE_CREATE_INFO_VERSION_1 1u

#define OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT                                                     \
  {OCCTL_SURFACE_BSPLINE_CREATE_INFO_VERSION_1,                                                    \
   NULL,                                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   0,                                                                                              \
   NULL,                                                                                           \
   NULL,                                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   NULL,                                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0}

/**
 * Runtime initialiser for #occtl_surface_bspline_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_create_bspline
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_bspline_create_info_init(occtl_surface_bspline_create_info_t* info);

/**
 * Creates a B-spline surface rep in the graph.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id Receives the rep id.  Must be non-NULL.
 * @param[in]  info   Versioned create-info.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL,
 *                                required arrays are NULL, or periodic flags
 *                                are not 0/1.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the definition.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_bspline(occtl_graph_t*                             graph,
                               occtl_rep_id_t*                            out_id,
                               const occtl_surface_bspline_create_info_t* info);

/**
 * Versioned create-info struct for Bezier surface construction.
 *
 * Poles are stored row-major: index = @c u * @c v_pole_count + @c v.
 */
typedef struct occtl_surface_bezier_create_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_SURFACE_BEZIER_CREATE_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved; must be NULL. */
  const occtl_point3_t* poles; /**< [in] Borrows it; row-major: index = u*v_pole_count + v. */
  size_t                u_pole_count; /**< Number of poles in the U direction; must be >= 1. */
  size_t                v_pole_count; /**< Number of poles in the V direction; must be >= 1. */
  const double*         weights; /**< [in] Borrows it; NULL = non-rational; same layout as poles. */
} occtl_surface_bezier_create_info_t;

#define OCCTL_SURFACE_BEZIER_CREATE_INFO_VERSION_1 1u

#define OCCTL_SURFACE_BEZIER_CREATE_INFO_INIT                                                      \
  {OCCTL_SURFACE_BEZIER_CREATE_INFO_VERSION_1, NULL, NULL, 0, 0, NULL}

/**
 * Runtime initialiser for #occtl_surface_bezier_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_create_bezier
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_bezier_create_info_init(occtl_surface_bezier_create_info_t* info);

/**
 * Creates a Bezier surface rep in the graph.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id Receives the rep id.  Must be non-NULL.
 * @param[in]  info   Borrows it.  Versioned create-info.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the definition.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_bezier(occtl_graph_t*                            graph,
                              occtl_rep_id_t*                           out_id,
                              const occtl_surface_bezier_create_info_t* info);

/**
 * Creates a Bezier surface from a row-major pole grid.
 *
 * This is the grid-facing alias for #occtl_surface_create_bezier.  It uses
 * the same create-info struct: @c poles are laid out as
 * @c u * v_pole_count + v, and @c weights may be NULL for a non-rational
 * Bezier surface.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id Receives the rep id.  Must be non-NULL.
 * @param[in]  info   Borrows it.  Versioned create-info.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL, or
 *                                the pole grid is invalid.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID OCCT rejected the definition.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_surface_bezier_create_info_init,
 *     occtl_surface_create_bezier,
 *     occtl_surface_create_from_point_grid
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_bezier_grid(occtl_graph_t*                            graph,
                                   occtl_rep_id_t*                           out_id,
                                   const occtl_surface_bezier_create_info_t* info);

/**
 * Creates a new surface rep that is the U-reversed copy of @p surface_id.
 *
 * @param[in]  graph       Graph that owns both surfaces.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_id      Receives the rep id of the reversed surface.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL, or @p surface_id is invalid.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_reverse(occtl_graph_t*  graph,
                                                          occtl_rep_id_t  surface_id,
                                                          occtl_rep_id_t* out_id);

/**
 * Creates a new surface rep by applying a 3D transform to @p surface_id.
 *
 * @param[in]  graph       Graph that will own the new rep.  Must be non-NULL.
 * @param[in]  surface_id  Source surface rep id.  Must be valid.
 * @param[in]  transform   Transform to apply.
 * @param[out] out_id      Receives the rep id of the transformed surface.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL, or @p surface_id is invalid.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_transformed(occtl_graph_t*    graph,
                                                              occtl_rep_id_t    surface_id,
                                                              occtl_transform_t transform,
                                                              occtl_rep_id_t*   out_id);

/**
 * Creates a new surface rep by translating @p surface_id.
 *
 * @param[in]  graph       Graph that will own the new rep.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[in]  delta       Translation vector.
 * @param[out] out_id      Receives the rep id of the translated surface.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL, or @p surface_id is invalid.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_translated(occtl_graph_t*  graph,
                                                             occtl_rep_id_t  surface_id,
                                                             occtl_vector3_t delta,
                                                             occtl_rep_id_t* out_id);

/**
 * Creates a new surface rep by rotating @p surface_id.
 *
 * @param[in]  graph       Graph that will own the new rep.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[in]  axis        Rotation axis.
 * @param[in]  angle       Rotation angle in radians.
 * @param[out] out_id      Receives the rep id of the rotated surface.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL, or @p surface_id is invalid.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_rotated(occtl_graph_t*          graph,
                                                          occtl_rep_id_t          surface_id,
                                                          occtl_axis1_placement_t axis,
                                                          double                  angle,
                                                          occtl_rep_id_t*         out_id);

/**
 * Creates a new surface rep by scaling @p surface_id about @p origin.
 *
 * @param[in]  graph       Graph that will own the new rep.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[in]  origin      Origin point for scaling.
 * @param[in]  factor      Scale factor; must be non-zero.
 * @param[out] out_id      Receives the rep id of the scaled surface.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_id is NULL, or @p surface_id is invalid.
 * @retval OCCTL_GEOMETRY_INVALID @c factor is zero.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_scaled(occtl_graph_t*  graph,
                                                         occtl_rep_id_t  surface_id,
                                                         occtl_point3_t  origin,
                                                         double          factor,
                                                         occtl_rep_id_t* out_id);

/**
 * Computes the surface area over the full bounded UV parameter domain.
 *
 * Unbounded surfaces, such as an infinite plane, cannot produce a finite area.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_area    Receives the surface area.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_area is NULL, or @p surface_id is invalid.
 * @retval OCCTL_GEOMETRY_INVALID The surface is invalid or unbounded.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_area(const occtl_graph_t* graph,
                                                       occtl_rep_id_t       surface_id,
                                                       double*              out_area);

/**
 * Projects a 3D point onto the surface and returns the closest UV coordinates.
 *
 * Uses OCCT's @c GeomAPI_ProjectPointOnSurf.
 *
 * @param[in]  graph         Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id    Surface rep id.  Must be valid.
 * @param[in]  point         Point to project.
 * @param[out] out_u         Receives the U parameter of the closest point.  Must be non-NULL.
 * @param[out] out_v         Receives the V parameter of the closest point.  Must be non-NULL.
 * @param[out] out_distance  Receives the shortest distance.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_u, or @p out_v is NULL, or
 *                                @p surface_id is invalid.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_project_point(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       surface_id,
                                                                occtl_point3_t       point,
                                                                double*              out_u,
                                                                double*              out_v,
                                                                double*              out_distance);

/**
 * Returns the UV coordinates on the surface nearest to a given 3D point.
 *
 * Convenience function equivalent to #occtl_surface_project_point with
 * @c out_distance = NULL.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[in]  point       Point to find UV for.
 * @param[out] out_u       Receives the U parameter.  Must be non-NULL.
 * @param[out] out_v       Receives the V parameter.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_u, or @p out_v is NULL, or
 *                                @p surface_id is invalid.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_uv_of_point(const occtl_graph_t* graph,
                                                              occtl_rep_id_t       surface_id,
                                                              occtl_point3_t       point,
                                                              double*              out_u,
                                                              double*              out_v);

/**
 * Returns the kind of geometry of a surface rep.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_kind    Receives the surface kind.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_kind is NULL, or @p surface_id is invalid.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_kind(const occtl_graph_t*  graph,
                                                       occtl_rep_id_t        surface_id,
                                                       occtl_surface_kind_t* out_kind);

/**
 * Returns non-zero if the surface is periodic in the U direction.
 *
 * @param[in]  graph           Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id      Surface rep id.  Must be valid.
 * @param[out] out_is_periodic Receives 1 if U-periodic, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_periodic is NULL, or
 *                                @p surface_id is invalid.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_is_u_periodic(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       surface_id,
                                                                int32_t* out_is_periodic);

/**
 * Returns non-zero if the surface is periodic in the V direction.
 *
 * @param[in]  graph           Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id      Surface rep id.  Must be valid.
 * @param[out] out_is_periodic Receives 1 if V-periodic, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_periodic is NULL, or
 *                                @p surface_id is invalid.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_is_v_periodic(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       surface_id,
                                                                int32_t* out_is_periodic);

/**
 * Returns non-zero if the surface is closed in both U and V directions.
 *
 * Convenience over separate U and V closed checks. A surface is closed
 * when IsUClosed() and IsVClosed() are both true.
 *
 * @param[in]  graph        Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id   Surface rep id.  Must be valid.
 * @param[out] out_is_closed Receives 1 if closed in both U and V, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_closed is NULL, or
 *                                @p surface_id is invalid.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_is_closed(const occtl_graph_t* graph,
                                                            occtl_rep_id_t       surface_id,
                                                            int32_t*             out_is_closed);

/**
 * Returns non-zero if the surface is periodic in either U or V direction.
 *
 * Convenience over separate U and V periodic checks. A surface is periodic
 * when IsUPeriodic() or IsVPeriodic() is true.
 *
 * @param[in]  graph           Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id      Surface rep id.  Must be valid.
 * @param[out] out_is_periodic Receives 1 if periodic in either U or V, 0 otherwise.  Must be
 * non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_periodic is NULL, or
 *                                @p surface_id is invalid.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_is_periodic(const occtl_graph_t* graph,
                                                              occtl_rep_id_t       surface_id,
                                                              int32_t*             out_is_periodic);

/**
 * Returns the continuity class of the surface.
 *
 * @param[in]  graph           Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id      Surface rep id.  Must be valid.
 * @param[out] out_continuity  Receives the continuity class.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_continuity is NULL, or
 *                                @p surface_id is invalid.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_continuity(occtl_graph_t*           graph,
                           occtl_rep_id_t           surface_id,
                           occtl_geom_continuity_t* out_continuity);

/**
 * Returns the parameter range of the surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_u_min   Start U parameter.  May be NULL.
 * @param[out] out_u_max   End U parameter.  May be NULL.
 * @param[out] out_v_min   Start V parameter.  May be NULL.
 * @param[out] out_v_max   End V parameter.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL or @p surface_id is invalid.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_parameter_range(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       surface_id,
                                                                  double*              out_u_min,
                                                                  double*              out_u_max,
                                                                  double*              out_v_min,
                                                                  double*              out_v_max);

/**
 * Extracts plane data from a surface rep.
 *
 * @param[in]  graph      Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id Surface rep id.  Must be valid; kind must be @c OCCTL_SURFACE_KIND_PLANE.
 * @param[out] out_plane  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_plane is NULL, or @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a plane.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_plane(const occtl_graph_t* graph,
                                                           occtl_rep_id_t       surface_id,
                                                           occtl_geom_plane_t*  out_plane);

/**
 * Extracts cylindrical surface data.
 *
 * @param[in]  graph        Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id   Surface rep id.  Must be valid; kind must be
 *                          @c OCCTL_SURFACE_KIND_CYLINDRICAL.
 * @param[out] out_cylinder Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_cylinder is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a cylindrical surface.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_as_cylinder(occtl_graph_t*                    graph,
                            occtl_rep_id_t                    surface_id,
                            occtl_geom_cylindrical_surface_t* out_cylinder);

/**
 * Extracts conical surface data.
 *
 * @param[in]  graph      Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id Surface rep id.  Must be valid; kind must be @c
 * OCCTL_SURFACE_KIND_CONICAL.
 * @param[out] out_cone   Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_cone is NULL, or @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a conical surface.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_cone(const occtl_graph_t*          graph,
                                                          occtl_rep_id_t                surface_id,
                                                          occtl_geom_conical_surface_t* out_cone);

/**
 * Extracts spherical surface data.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid; kind must be
 *                         @c OCCTL_SURFACE_KIND_SPHERICAL.
 * @param[out] out_sphere  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_sphere is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a spherical surface.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_as_sphere(occtl_graph_t*                  graph,
                          occtl_rep_id_t                  surface_id,
                          occtl_geom_spherical_surface_t* out_sphere);

/**
 * Extracts toroidal surface data.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid; kind must be
 *                         @c OCCTL_SURFACE_KIND_TOROIDAL.
 * @param[out] out_torus   Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_torus is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a toroidal surface.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_as_torus(const occtl_graph_t*           graph,
                         occtl_rep_id_t                 surface_id,
                         occtl_geom_toroidal_surface_t* out_torus);

/**
 * Extracts the rotation axis of a surface of revolution.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid; kind must be
 *                         @c OCCTL_SURFACE_KIND_REVOLUTION.
 * @param[out] out_axis    Rotation axis.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL or @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a surface of revolution.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_revolution(const occtl_graph_t*     graph,
                                                                occtl_rep_id_t           surface_id,
                                                                occtl_axis1_placement_t* out_axis);

/**
 * Extracts the direction of a surface of linear extrusion.
 *
 * @param[in]  graph          Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id     Surface rep id.  Must be valid; kind must be
 *                            @c OCCTL_SURFACE_KIND_EXTRUSION.
 * @param[out] out_direction  Extrusion direction.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL or @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a surface of linear extrusion.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_extrusion(const occtl_graph_t* graph,
                                                               occtl_rep_id_t       surface_id,
                                                               occtl_vector3_t*     out_direction);

/**
 * Extracts the parametric trim bounds of a rectangular-trimmed surface.
 *
 * @param[in]  graph        Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id   Surface rep id.  Must be valid; kind must be
 *                          @c OCCTL_SURFACE_KIND_RECTANGULAR_TRIMMED.
 * @param[out] out_u_first  First U trim bound.  May be NULL.
 * @param[out] out_u_last   Last U trim bound.  May be NULL.
 * @param[out] out_v_first  First V trim bound.  May be NULL.
 * @param[out] out_v_last   Last V trim bound.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL or @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a rectangular-trimmed surface.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_rectangular_trimmed(const occtl_graph_t* graph,
                                                                         occtl_rep_id_t surface_id,
                                                                         double*        out_u_first,
                                                                         double*        out_u_last,
                                                                         double*        out_v_first,
                                                                         double*        out_v_last);

/**
 * Extracts the signed offset distance of an offset surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid; kind must be
 *                         @c OCCTL_SURFACE_KIND_OFFSET.
 * @param[out] out_offset  Signed offset distance.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL or @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not an offset surface.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_offset(const occtl_graph_t* graph,
                                                            occtl_rep_id_t       surface_id,
                                                            double*              out_offset);

/**
 * Returns the U degree of a B-spline surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_degree  Receives the U degree.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_degree is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a B-spline.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_u_degree(const occtl_graph_t* graph,
                                                                   occtl_rep_id_t       surface_id,
                                                                   int32_t*             out_degree);

/**
 * Returns the V degree of a B-spline surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_degree  Receives the V degree.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_degree is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a B-spline.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_v_degree(const occtl_graph_t* graph,
                                                                   occtl_rep_id_t       surface_id,
                                                                   int32_t*             out_degree);

/**
 * Returns the number of poles in the U direction of a B-spline surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_count   Receives the U pole count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a B-spline.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_u_pole_count(const occtl_graph_t* graph,
                                                                       occtl_rep_id_t surface_id,
                                                                       size_t*        out_count);

/**
 * Returns the number of poles in the V direction of a B-spline surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_count   Receives the V pole count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a B-spline.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_v_pole_count(const occtl_graph_t* graph,
                                                                       occtl_rep_id_t surface_id,
                                                                       size_t*        out_count);

/**
 * Returns non-zero if the B-spline surface is rational.
 *
 * @param[in]  graph         Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id    Surface rep id.  Must be valid.
 * @param[out] out_is_rational  Receives 1 if rational; 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_rational is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a B-spline.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_is_rational(const occtl_graph_t* graph,
                                                                      occtl_rep_id_t surface_id,
                                                                      int32_t* out_is_rational);

/**
 * Returns the number of distinct U knot values of a B-spline surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_count   Receives the U knot count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a B-spline.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_u_knot_count(const occtl_graph_t* graph,
                                                                       occtl_rep_id_t surface_id,
                                                                       size_t*        out_count);

/**
 * Returns the number of distinct V knot values of a B-spline surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_count   Receives the V knot count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a B-spline.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_v_knot_count(const occtl_graph_t* graph,
                                                                       occtl_rep_id_t surface_id,
                                                                       size_t*        out_count);

/**
 * Returns the U degree of a Bezier surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_degree  Receives the U degree.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_degree is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a Bezier.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_u_degree(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       surface_id,
                                                                  int32_t*             out_degree);

/**
 * Returns the V degree of a Bezier surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_degree  Receives the V degree.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_degree is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a Bezier.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_v_degree(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       surface_id,
                                                                  int32_t*             out_degree);

/**
 * Returns the number of poles in the U direction of a Bezier surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_count   Receives the U pole count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a Bezier.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_u_pole_count(const occtl_graph_t* graph,
                                                                      occtl_rep_id_t surface_id,
                                                                      size_t*        out_count);

/**
 * Returns the number of poles in the V direction of a Bezier surface.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_count   Receives the V pole count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a Bezier.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_v_pole_count(const occtl_graph_t* graph,
                                                                      occtl_rep_id_t surface_id,
                                                                      size_t*        out_count);

/**
 * Returns non-zero if the Bezier surface is rational (in U or V).
 *
 * @param[in]  graph         Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id    Surface rep id.  Must be valid.
 * @param[out] out_is_rational  Receives 1 if rational; 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_rational is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Surface is not a Bezier.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_is_rational(const occtl_graph_t* graph,
                                                                     occtl_rep_id_t surface_id,
                                                                     int32_t* out_is_rational);

/**
 * Extracts the poles of a B-spline surface.  Two-call pattern.
 *
 * Poles are returned row-major: index = @c u * @c v_pole_count + @c v.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid; kind must be
 *                         @c OCCTL_SURFACE_KIND_BSPLINE.
 * @param[out] out_buf     May be NULL on sizing call.
 * @param[in]  capacity    Elements @p out_buf can hold.
 * @param[out] out_count   Required count (= u_pole_count * v_pole_count).  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_poles(const occtl_graph_t* graph,
                                                                occtl_rep_id_t       surface_id,
                                                                occtl_point3_t*      out_buf,
                                                                size_t               capacity,
                                                                size_t*              out_count);

/**
 * Extracts the distinct U knot values of a B-spline surface.  Two-call pattern.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_buf     May be NULL on sizing call.
 * @param[in]  capacity    Elements @p out_buf can hold.
 * @param[out] out_count   Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_u_knots(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       surface_id,
                                                                  double*              out_buf,
                                                                  size_t               capacity,
                                                                  size_t*              out_count);

/**
 * Extracts the distinct V knot values of a B-spline surface.  Two-call pattern.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_buf     May be NULL on sizing call.
 * @param[in]  capacity    Elements @p out_buf can hold.
 * @param[out] out_count   Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_v_knots(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       surface_id,
                                                                  double*              out_buf,
                                                                  size_t               capacity,
                                                                  size_t*              out_count);

/**
 * Extracts the U knot multiplicities of a B-spline surface.  Two-call pattern.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_buf     May be NULL on sizing call.
 * @param[in]  capacity    Elements @p out_buf can hold.
 * @param[out] out_count   Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_u_multiplicities(occtl_graph_t* graph,
                                         occtl_rep_id_t surface_id,
                                         int32_t*       out_buf,
                                         size_t         capacity,
                                         size_t*        out_count);

/**
 * Extracts the V knot multiplicities of a B-spline surface.  Two-call pattern.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_buf     May be NULL on sizing call.
 * @param[in]  capacity    Elements @p out_buf can hold.
 * @param[out] out_count   Required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_v_multiplicities(occtl_graph_t* graph,
                                         occtl_rep_id_t surface_id,
                                         int32_t*       out_buf,
                                         size_t         capacity,
                                         size_t*        out_count);

/**
 * Extracts the per-pole weights of a rational B-spline surface.  Two-call pattern.
 *
 * Layout matches the poles array: row-major @c index = u * v_pole_count + v.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[out] out_buf     Output buffer, or NULL for sizing.
 * @param[in]  capacity    Capacity of @p out_buf in doubles.
 * @param[out] out_count   Required/filled double count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Not a B-spline or surface is non-rational.
 * @retval OCCTL_BUFFER_TOO_SMALL Buffer too small.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_weights(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       surface_id,
                                                                  double*              out_buf,
                                                                  size_t               capacity,
                                                                  size_t*              out_count);

/**
 * Extracts the expanded (flat) U knot sequence of a B-spline surface.  Two-call pattern.
 *
 * The flat sequence repeats each distinct knot value by its multiplicity.
 * Its length equals @c sum(u_multiplicities), which is the standard B-spline
 * knot vector consumed by most numerical libraries.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid; kind must be
 *                         @c OCCTL_SURFACE_KIND_BSPLINE.
 * @param[out] out_buf     May be NULL on the sizing call.
 * @param[in]  capacity    Number of @c double elements @p out_buf can hold.
 * @param[out] out_count   Receives the required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success (including the sizing call).
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Handle is not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL @p capacity is smaller than @c *out_count.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_bspline_u_knots, occtl_surface_bspline_u_multiplicities,
 *     occtl_surface_bspline_v_flat_knots
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_u_flat_knots(const occtl_graph_t* graph,
                                                                       occtl_rep_id_t surface_id,
                                                                       double*        out_buf,
                                                                       size_t         capacity,
                                                                       size_t*        out_count);

/**
 * Extracts the expanded (flat) V knot sequence of a B-spline surface.  Two-call pattern.
 *
 * The flat sequence repeats each distinct knot value by its multiplicity.
 * Its length equals @c sum(v_multiplicities), which is the standard B-spline
 * knot vector consumed by most numerical libraries.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid; kind must be
 *                         @c OCCTL_SURFACE_KIND_BSPLINE.
 * @param[out] out_buf     May be NULL on the sizing call.
 * @param[in]  capacity    Number of @c double elements @p out_buf can hold.
 * @param[out] out_count   Receives the required count.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success (including the sizing call).
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND       Handle is not a B-spline.
 * @retval OCCTL_BUFFER_TOO_SMALL @p capacity is smaller than @c *out_count.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_bspline_v_knots, occtl_surface_bspline_v_multiplicities,
 *     occtl_surface_bspline_u_flat_knots
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_v_flat_knots(const occtl_graph_t* graph,
                                                                       occtl_rep_id_t surface_id,
                                                                       double*        out_buf,
                                                                       size_t         capacity,
                                                                       size_t*        out_count);

/**
 * Zero-copy view of the poles array of a B-spline surface.
 *
 * Returns a direct pointer into the OCCT internal storage and the U / V
 * pole counts.  Layout is row-major: @c index = u * v_pole_count + v.  Valid
 * only while the surface rep is alive in the graph and no mutating call has
 * been made on it.
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid; kind must be
 *                         @c OCCTL_SURFACE_KIND_BSPLINE.
 * @param[out] out_data    Receives a borrowed pointer to the pole array.  Must be non-NULL.
 * @param[out] out_u_count    Receives the number of poles in U.  Must be non-NULL.
 * @param[out] out_v_count    Receives the number of poles in V.  Must be non-NULL.
 *
 * @retval OCCTL_OK
 * @retval OCCTL_INVALID_ARGUMENT  Any out-pointer is NULL, or @p surface_id is invalid.
 * @retval OCCTL_WRONG_KIND        Surface is not a B-spline.
 * @retval OCCTL_INTERNAL          OCCT's internal pole array is not contiguous
 *                                 row-major (does not occur with current OCCT;
 *                                 reported defensively if a future OCCT version
 *                                 changes the layout).
 *
 * @threadsafe Yes (read-only).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_poles_view(occtl_graph_t*         graph,
                                   occtl_rep_id_t         surface_id,
                                   const occtl_point3_t** out_data,
                                   size_t*                out_u_count,
                                   size_t*                out_v_count);

/**
 * Aggregate inspection view of a B-spline surface.
 *
 * Read-only snapshot of every field a caller typically needs in the same
 * pass: scalars (U/V degree, periodicity, rational flag, per-direction
 * counts) plus borrowed pointers into the underlying OCCT storage for the
 * pole grid, weight grid, distinct U/V knots, U/V multiplicities, and the
 * expanded U/V flat knot sequences.  One call replaces ~16 atomized
 * accessors; pair with #occtl_surface_as_bspline.
 *
 * Pointers in @p out borrow from the surface rep and are valid while the
 * rep exists in the graph.
 *
 * The pole and weight grids are stored row-major with U as the major
 * (slow-varying) axis: element @c (u_index, v_index) is at
 * @c poles[u_index * v_pole_count + v_index].  This layout matches
 * #occtl_surface_bspline_create_info_t so values flow round-trip without
 * reshuffling.
 *
 * The caller declares the layout version they understand in @c struct_version
 * before the call; the library fills only fields up to that version.  The
 * @c weights pointer is set to NULL when @c is_rational is 0.
 */
typedef struct occtl_surface_bspline
{
  uint32_t    struct_version; /**< [in]  Must be #OCCTL_SURFACE_BSPLINE_VERSION_1. */
  const void* p_next;         /**< Reserved for extensions; must be NULL. */

  int32_t u_degree;          /**< Polynomial degree along U. */
  int32_t v_degree;          /**< Polynomial degree along V. */
  int32_t is_rational;       /**< 0 = non-rational; 1 = rational (NURBS) in U or V. */
  int32_t is_u_periodic;     /**< 0/1. */
  int32_t is_v_periodic;     /**< 0/1. */
  size_t  u_pole_count;      /**< Number of poles along U. */
  size_t  v_pole_count;      /**< Number of poles along V. */
  size_t  u_knot_count;      /**< Number of distinct U knot values. */
  size_t  v_knot_count;      /**< Number of distinct V knot values. */
  size_t  u_flat_knot_count; /**< Sum of U multiplicities (length of @c u_flat_knots). */
  size_t  v_flat_knot_count; /**< Sum of V multiplicities (length of @c v_flat_knots). */

  /**
   * Borrowed view pointers.  Lifetime is tied to the parent surface rep and ends
   * when the parent surface is removed from the graph or mutated.
   */
  const occtl_point3_t* poles;     /**< [out] Borrows it; row-major:
                                    *        index = u * @c v_pole_count + v;
                                    *        @c u_pole_count * @c v_pole_count elements. */
  const double* weights;           /**< [out] Borrows it; same row-major layout as
                                    *        @c poles, or NULL when @c is_rational == 0. */
  const double*  u_knots;          /**< [out] Borrows it; @c u_knot_count distinct knots. */
  const double*  v_knots;          /**< [out] Borrows it; @c v_knot_count distinct knots. */
  const int32_t* u_multiplicities; /**< [out] Borrows it; @c u_knot_count multiplicities. */
  const int32_t* v_multiplicities; /**< [out] Borrows it; @c v_knot_count multiplicities. */
  const double*  u_flat_knots;     /**< [out] Borrows it; @c u_flat_knot_count elements. */
  const double*  v_flat_knots;     /**< [out] Borrows it; @c v_flat_knot_count elements. */
} occtl_surface_bspline_t;

#define OCCTL_SURFACE_BSPLINE_VERSION_1 1u

#define OCCTL_SURFACE_BSPLINE_INIT                                                                 \
  {OCCTL_SURFACE_BSPLINE_VERSION_1,                                                                \
   NULL,                                                                                           \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   NULL,                                                                                           \
   NULL,                                                                                           \
   NULL,                                                                                           \
   NULL,                                                                                           \
   NULL,                                                                                           \
   NULL,                                                                                           \
   NULL,                                                                                           \
   NULL}

/**
 * Runtime initialiser for #occtl_surface_bspline_t.
 *
 * Sets @c struct_version and zeroes all other fields.  Use instead of the
 * @c _INIT macro when C macros are not available (e.g. C# P/Invoke).
 *
 * @param[out] out  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_as_bspline
 */
OCCTL_API void OCCTL_CALL occtl_surface_bspline_init(occtl_surface_bspline_t* out);

/**
 * Fills a caller-allocated #occtl_surface_bspline_t with a complete read-only
 * inspection view of the underlying B-spline surface.
 *
 * Pointers in @p out borrow from the surface rep and are valid while the rep
 * exists in the graph.
 *
 * @param[in]     graph      Graph containing the surface.  Must be non-NULL.
 * @param[in]     surface_id Surface rep id.  Must be valid; kind must be
 *                           @c OCCTL_SURFACE_KIND_BSPLINE.
 * @param[in,out] out        Borrows it.  Caller-allocated; must be non-NULL.  The
 *                           caller sets @c struct_version on entry; the library
 *                           fills the remaining fields on success and leaves them
 *                           unchanged on failure.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out is NULL, or @p surface_id is invalid.
 * @retval OCCTL_VERSION_MISMATCH @c out->struct_version is not supported.
 * @retval OCCTL_WRONG_KIND       Surface is not a B-spline surface.
 *
 * @threadsafe Yes (read-only).
 *
 * @sa occtl_surface_bspline_init, occtl_surface_bspline_poles_view,
 *     occtl_surface_bspline_u_degree, occtl_surface_bspline_u_pole_count
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_bspline(const occtl_graph_t*     graph,
                                                             occtl_rep_id_t           surface_id,
                                                             occtl_surface_bspline_t* out);

/**
 * Evaluates the surface at (u, v).
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[in]  u           U parameter.
 * @param[in]  v           V parameter.
 * @param[out] out_point   Receives the point on the surface.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL or @p surface_id is invalid.
 * @retval OCCTL_OUT_OF_RANGE     (u, v) is outside the parameter domain.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_d0(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          double               u,
                                                          double               v,
                                                          occtl_point3_t*      out_point);

/**
 * Evaluates the surface and its first-order partial derivatives at (u, v).
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[in]  u           U parameter.
 * @param[in]  v           V parameter.
 * @param[out] out_point   Receives the point.  May be NULL.
 * @param[out] out_d1u     Receives dS/du.  May be NULL.
 * @param[out] out_d1v     Receives dS/dv.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL or @p surface_id is invalid.
 * @retval OCCTL_OUT_OF_RANGE     Parameter is outside the domain.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_d1(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          double               u,
                                                          double               v,
                                                          occtl_point3_t*      out_point,
                                                          occtl_vector3_t*     out_d1u,
                                                          occtl_vector3_t*     out_d1v);

/**
 * Evaluates the surface and its first- and second-order partial
 * derivatives at (u, v).
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[in]  u           U parameter.
 * @param[in]  v           V parameter.
 * @param[out] out_point   Receives the point.  May be NULL.
 * @param[out] out_d1u     Receives dS/du.  May be NULL.
 * @param[out] out_d1v     Receives dS/dv.  May be NULL.
 * @param[out] out_d2u     Receives d2S/du2.  May be NULL.
 * @param[out] out_d2v     Receives d2S/dv2.  May be NULL.
 * @param[out] out_d2uv    Receives d2S/dudv.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL or @p surface_id is invalid.
 * @retval OCCTL_OUT_OF_RANGE     Parameter is outside the domain.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_d2(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          double               u,
                                                          double               v,
                                                          occtl_point3_t*      out_point,
                                                          occtl_vector3_t*     out_d1u,
                                                          occtl_vector3_t*     out_d1v,
                                                          occtl_vector3_t*     out_d2u,
                                                          occtl_vector3_t*     out_d2v,
                                                          occtl_vector3_t*     out_d2uv);

/**
 * Evaluates the surface and its first-, second-, and third-order partial
 * derivatives at (u, v).
 *
 * @param[in]  graph       Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id  Surface rep id.  Must be valid.
 * @param[in]  u           U parameter.
 * @param[in]  v           V parameter.
 * @param[out] out_point   Receives the point.  May be NULL.
 * @param[out] out_d1u     Receives dS/du.  May be NULL.
 * @param[out] out_d1v     Receives dS/dv.  May be NULL.
 * @param[out] out_d2u     Receives d2S/du2.  May be NULL.
 * @param[out] out_d2v     Receives d2S/dv2.  May be NULL.
 * @param[out] out_d2uv    Receives d2S/dudv.  May be NULL.
 * @param[out] out_d3u     Receives d3S/du3.  May be NULL.
 * @param[out] out_d3v     Receives d3S/dv3.  May be NULL.
 * @param[out] out_d3uuv   Receives d3S/du2dv.  May be NULL.
 * @param[out] out_d3uvv   Receives d3S/dudv2.  May be NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL or @p surface_id is invalid.
 * @retval OCCTL_OUT_OF_RANGE     Parameter is outside the domain.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_d3(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          double               u,
                                                          double               v,
                                                          occtl_point3_t*      out_point,
                                                          occtl_vector3_t*     out_d1u,
                                                          occtl_vector3_t*     out_d1v,
                                                          occtl_vector3_t*     out_d2u,
                                                          occtl_vector3_t*     out_d2v,
                                                          occtl_vector3_t*     out_d2uv,
                                                          occtl_vector3_t*     out_d3u,
                                                          occtl_vector3_t*     out_d3v,
                                                          occtl_vector3_t*     out_d3uuv,
                                                          occtl_vector3_t*     out_d3uvv);

/**
 * Returns a partial derivative vector at (u, v).
 *
 * @param[in]  graph           Graph containing the surface.  Must be non-NULL.
 * @param[in]  surface_id      Surface rep id.  Must be valid.
 * @param[in]  u               U parameter.
 * @param[in]  v               V parameter.
 * @param[in]  nu              Derivative order in U (>= 0).
 * @param[in]  nv              Derivative order in V (>= 0).
 * @param[out] out_derivative  Receives d^(nu+nv)S / du^nu dv^nv.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_derivative is NULL, or
 *                                @p surface_id is invalid.
 * @retval OCCTL_OUT_OF_RANGE     Parameter is outside the domain.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_dn(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          double               u,
                                                          double               v,
                                                          int32_t              nu,
                                                          int32_t              nv,
                                                          occtl_vector3_t*     out_derivative);

/**
 * Versioned create-info struct for surface interpolation.
 *
 * Interpolates a B-spline surface passing through the given grid of
 * points.  Points are stored row-major: index = @c u * @c v_point_count + @c v.
 * Uses OCCT's @c GeomAPI_PointsToBSplineSurface internally with uniform
 * parameters.
 */
typedef struct occtl_surface_interpolated_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_SURFACE_INTERPOLATED_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved; must be NULL. */
  const occtl_point3_t* points;         /**< [in] Borrows it; row-major:
                                         *        index = u * v_point_count + v. */
  size_t  u_point_count;                /**< Number of point rows in U. */
  size_t  v_point_count;                /**< Number of point columns in V. */
  int32_t is_u_periodic;                /**< 0/1. Enables OCCT U-periodic interpolation. */
} occtl_surface_interpolated_info_t;

#define OCCTL_SURFACE_INTERPOLATED_INFO_VERSION_1 1u
#define OCCTL_SURFACE_INTERPOLATED_INFO_INIT                                                       \
  {OCCTL_SURFACE_INTERPOLATED_INFO_VERSION_1, NULL, NULL, 0, 0, 0}

/**
 * Runtime initialiser for #occtl_surface_interpolated_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_create_interpolated
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_interpolated_info_init(occtl_surface_interpolated_info_t* info);

/**
 * Creates a B-spline surface rep that interpolates (passes through) the
 * given point grid.
 *
 * Uses OCCT's @c GeomAPI_PointsToBSplineSurface with uniform parameters.
 * When @c is_u_periodic is set, the surface is made periodic in U.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[in]  info   Versioned create-info.  Must be non-NULL.
 * @param[out] out_id Receives the rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL,
 *                                required fields are NULL / too small, or
 *                                @c is_u_periodic is not 0/1.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID Interpolation failed.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_surface_interpolated_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_interpolated(occtl_graph_t*                           graph,
                                    const occtl_surface_interpolated_info_t* info,
                                    occtl_rep_id_t*                          out_id);

/**
 * Versioned create-info struct for surface approximation.
 *
 * Fits a B-spline surface to the given point grid within tolerance using
 * OCCT's @c GeomAPI_PointsToBSplineSurface.
 */
typedef struct occtl_surface_approximated_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_SURFACE_APPROXIMATED_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved; must be NULL. */
  const occtl_point3_t* points;         /**< [in] Borrows it; row-major. */
  size_t                u_point_count;  /**< Number of point rows in U. */
  size_t                v_point_count;  /**< Number of point columns in V. */
  int32_t               degree_min;     /**< Minimum degree (>= 1). */
  int32_t               degree_max;     /**< Maximum degree. */
  double                tolerance;      /**< Approximation tolerance in 3D. */
} occtl_surface_approximated_info_t;

#define OCCTL_SURFACE_APPROXIMATED_INFO_VERSION_1 1u
#define OCCTL_SURFACE_APPROXIMATED_INFO_INIT                                                       \
  {OCCTL_SURFACE_APPROXIMATED_INFO_VERSION_1, NULL, NULL, 0, 0, 1, 3, 1e-3}

/**
 * Runtime initialiser for #occtl_surface_approximated_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_create_approximated
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_approximated_info_init(occtl_surface_approximated_info_t* info);

/**
 * Fits a B-spline surface rep to the given point grid within tolerance.
 *
 * Uses OCCT's @c GeomAPI_PointsToBSplineSurface (approximation mode).
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[in]  info   Versioned create-info.  Must be non-NULL.
 * @param[out] out_id Receives the rep id.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_id, or @p info is NULL,
 *                                required fields are NULL / too small, degree
 *                                range is invalid, or @c tolerance is negative.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID Approximation failed.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_surface_approximated_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_approximated(occtl_graph_t*                           graph,
                                    const occtl_surface_approximated_info_t* info,
                                    occtl_rep_id_t*                          out_id);

/**
 * Selects the OCCT point-grid fitting algorithm.
 */
typedef enum occtl_surface_point_grid_mode
{
  OCCTL_SURFACE_POINT_GRID_MODE_APPROXIMATE     = 0, /**< Fit within tolerance. */
  OCCTL_SURFACE_POINT_GRID_MODE_INTERPOLATE     = 1, /**< Pass through every point. */
  OCCTL_SURFACE_POINT_GRID_MODE_RESERVED_FUTURE = 0x7fffffff
} occtl_surface_point_grid_mode_t;

/**
 * Versioned create-info struct for point-grid surface construction.
 *
 * Builds a B-spline surface from a row-major point grid. Approximation mode
 * uses degree range and tolerance. Interpolation mode uses only the point
 * grid and U periodic flag.
 */
typedef struct occtl_surface_point_grid_create_info
{
  uint32_t    struct_version;   /**< Must be #OCCTL_SURFACE_POINT_GRID_CREATE_INFO_VERSION_1. */
  const void* p_next;           /**< Reserved; must be NULL. */
  const occtl_point3_t* points; /**< [in] Borrows it; row-major:
                                 *        index = u * v_point_count + v. */
  size_t                          u_point_count; /**< Number of point rows in U; must be >= 2. */
  size_t                          v_point_count; /**< Number of point columns in V; must be >= 2. */
  occtl_surface_point_grid_mode_t mode;          /**< OCCT fitting algorithm. */
  int32_t                         degree_min;    /**< Approximation minimum degree (>= 1). */
  int32_t                         degree_max;    /**< Approximation maximum degree. */
  int32_t                         is_u_periodic; /**< 0/1. Used by interpolation mode. */
  double                          tolerance;     /**< Approximation tolerance in 3D. */
} occtl_surface_point_grid_create_info_t;

#define OCCTL_SURFACE_POINT_GRID_CREATE_INFO_VERSION_1 1u
#define OCCTL_SURFACE_POINT_GRID_CREATE_INFO_INIT                                                  \
  {OCCTL_SURFACE_POINT_GRID_CREATE_INFO_VERSION_1,                                                 \
   NULL,                                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   0,                                                                                              \
   OCCTL_SURFACE_POINT_GRID_MODE_APPROXIMATE,                                                      \
   1,                                                                                              \
   3,                                                                                              \
   0,                                                                                              \
   1e-3}

/**
 * Runtime initialiser for #occtl_surface_point_grid_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_create_from_point_grid
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_point_grid_create_info_init(occtl_surface_point_grid_create_info_t* info);

/**
 * Creates a B-spline surface rep from a row-major point grid.
 *
 * This is the broad point-grid constructor. It delegates to OCCT's
 * @c GeomAPI_PointsToBSplineSurface in approximation or interpolation mode.
 *
 * @param[in]  graph  Graph that will own the rep.  Must be non-NULL.
 * @param[out] out_id Receives the rep id. Must be non-NULL.
 * @param[in]  info   Borrows it. Must be non-NULL with a recognised
 *                    @c struct_version.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_id, or @p info is NULL,
 *                                 @c p_next is non-NULL, the point grid is
 *                                 NULL / too small, the mode is unknown,
 *                                 degree range is invalid, or @c tolerance is
 *                                 negative.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the point grid.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_surface_point_grid_create_info_init,
 *     occtl_surface_create_interpolated,
 *     occtl_surface_create_approximated
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_from_point_grid(occtl_graph_t*                                graph,
                                       occtl_rep_id_t*                               out_id,
                                       const occtl_surface_point_grid_create_info_t* info);

/**
 * Filling style for #occtl_surface_create_from_boundary_curves.
 */
typedef enum occtl_surface_filling_style
{
  OCCTL_SURFACE_FILLING_STRETCH         = 0, /**< Flattest OCCT filling style. */
  OCCTL_SURFACE_FILLING_COONS           = 1, /**< Coons-style rounded patch. */
  OCCTL_SURFACE_FILLING_CURVED          = 2, /**< Most rounded OCCT filling style. */
  OCCTL_SURFACE_FILLING_RESERVED_FUTURE = 0x7fffffff
} occtl_surface_filling_style_t;

/**
 * Versioned create-info struct for a B-spline surface from boundary curves.
 *
 * Builds a filled B-spline surface from two, three, or four contiguous
 * boundary curves. Input curves are specified as rep ids in the graph.
 */
typedef struct occtl_surface_boundary_curves_create_info
{
  uint32_t    struct_version; /**< Must be #OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_VERSION_1. */
  const void* p_next;         /**< Reserved; must be NULL. */
  const occtl_rep_id_t*         curves;      /**< [in] Boundary curve rep ids array. */
  size_t                        curve_count; /**< Number of curves; must be 2, 3, or 4. */
  occtl_surface_filling_style_t style;       /**< OCCT filling style. */
} occtl_surface_boundary_curves_create_info_t;

#define OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_VERSION_1 1u
#define OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_INIT                                             \
  {OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_VERSION_1,                                            \
   NULL,                                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   OCCTL_SURFACE_FILLING_STRETCH}

/**
 * Runtime initialiser for #occtl_surface_boundary_curves_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_create_from_boundary_curves
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_boundary_curves_create_info_init(occtl_surface_boundary_curves_create_info_t* info);

/**
 * Creates a B-spline surface rep from contiguous boundary curves.
 *
 * Uses OCCT's @c GeomFill_BSplineCurves. The input curves must form a
 * compatible boundary loop or strip accepted by OCCT.
 *
 * @param[in]  graph  Graph that will own the rep. Must be non-NULL.
 * @param[out] out_id Receives the rep id. Must be non-NULL.
 * @param[in]  info   Borrows it. Must be non-NULL with a recognised
 *                    @c struct_version.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_id, or @p info is NULL,
 *                                 @c p_next is non-NULL, @c curves is NULL,
 *                                 @c curve_count is not 2, 3, or 4, an
 *                                 array entry is invalid, or @c style is
 *                                 unknown.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the boundary curves.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_surface_boundary_curves_create_info_init, occtl_surface_create_gordon
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_create_from_boundary_curves(
  occtl_graph_t*                                     graph,
  occtl_rep_id_t*                                    out_id,
  const occtl_surface_boundary_curves_create_info_t* info);

/**
 * Versioned create-info struct for Gordon surface construction.
 *
 * Builds a B-spline surface interpolating a network of profile and guide
 * curves. Profiles and guides are specified as rep id arrays; each array
 * must contain at least two valid entries.
 */
typedef struct occtl_surface_gordon_create_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_SURFACE_GORDON_CREATE_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved; must be NULL. */
  const occtl_rep_id_t* profiles;       /**< [in] Profile curve rep ids array. */
  size_t                profile_count;  /**< Number of profile curves; must be >= 2. */
  const occtl_rep_id_t* guides;         /**< [in] Guide curve rep ids array. */
  size_t                guide_count;    /**< Number of guide curves; must be >= 2. */
  double                tolerance;      /**< Intersection/network tolerance; > 0. */
  int32_t               parallel;       /**< 0/1; enables OCCT parallel internal stages. */
} occtl_surface_gordon_create_info_t;

#define OCCTL_SURFACE_GORDON_CREATE_INFO_VERSION_1 1u
#define OCCTL_SURFACE_GORDON_CREATE_INFO_INIT                                                      \
  {OCCTL_SURFACE_GORDON_CREATE_INFO_VERSION_1, NULL, NULL, 0, NULL, 0, 1.0e-7, 0}

/**
 * Runtime initialiser for #occtl_surface_gordon_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_create_gordon
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_gordon_create_info_init(occtl_surface_gordon_create_info_t* info);

/**
 * Creates a Gordon B-spline surface rep from a profile/guide curve network.
 *
 * Uses OCCT's @c GeomFill_Gordon. The input curves must form a compatible
 * intersecting network; non-rational curve networks are the OCCT-supported
 * target for this constructor.
 *
 * @param[in]  graph  Graph that will own the rep. Must be non-NULL.
 * @param[out] out_id Receives the rep id. Must be non-NULL.
 * @param[in]  info   Borrows it. Must be non-NULL with a recognised
 *                    @c struct_version.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_id, or @p info is NULL,
 *                                 @c p_next is non-NULL, a curve array is
 *                                 NULL / too small, an array entry is invalid,
 *                                 @c tolerance is non-positive, or
 *                                 @c parallel is not 0/1.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the curve network.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_surface_gordon_create_info_init, occtl_surface_create_interpolated
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_gordon(occtl_graph_t*                            graph,
                              occtl_rep_id_t*                           out_id,
                              const occtl_surface_gordon_create_info_t* info);

/**
 * Versioned create-info struct for curve-grid surface construction.
 *
 * Builds a B-spline surface from an intersecting grid of U-direction and
 * V-direction curves. This is a binding-friendly semantic wrapper over
 * OCCT's Gordon surface construction.
 */
typedef struct occtl_surface_curve_grid_create_info
{
  uint32_t    struct_version;     /**< Must be #OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_VERSION_1. */
  const void* p_next;             /**< Reserved; must be NULL. */
  const occtl_rep_id_t* u_curves; /**< [in] U-direction curve rep ids array. */
  size_t                u_curve_count; /**< Number of U-direction curves; must be >= 2. */
  const occtl_rep_id_t* v_curves;      /**< [in] V-direction curve rep ids array. */
  size_t                v_curve_count; /**< Number of V-direction curves; must be >= 2. */
  double                tolerance;     /**< Intersection/network tolerance; > 0. */
  int32_t               parallel;      /**< 0/1; enables OCCT parallel internal stages. */
} occtl_surface_curve_grid_create_info_t;

#define OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_VERSION_1 1u
#define OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_INIT                                                  \
  {OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_VERSION_1, NULL, NULL, 0, NULL, 0, 1.0e-7, 0}

/**
 * Runtime initialiser for #occtl_surface_curve_grid_create_info_t.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_create_from_curve_grid
 */
OCCTL_API void OCCTL_CALL
  occtl_surface_curve_grid_create_info_init(occtl_surface_curve_grid_create_info_t* info);

/**
 * Creates a B-spline surface rep from an intersecting U/V curve grid.
 *
 * Uses OCCT's @c GeomFill_Gordon. The input curves must form a compatible
 * network where each U-direction curve intersects each V-direction curve.
 *
 * @param[in]  graph  Graph that will own the rep. Must be non-NULL.
 * @param[out] out_id Receives the rep id. Must be non-NULL.
 * @param[in]  info   Borrows it. Must be non-NULL with a recognised
 *                    @c struct_version.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_id, or @p info is NULL,
 *                                 @c p_next is non-NULL, a curve array is
 *                                 NULL / too small, an array entry is invalid,
 *                                 @c tolerance is non-positive, or
 *                                 @c parallel is not 0/1.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the curve network.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_surface_curve_grid_create_info_init, occtl_surface_create_gordon
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_from_curve_grid(occtl_graph_t*                                graph,
                                       occtl_rep_id_t*                               out_id,
                                       const occtl_surface_curve_grid_create_info_t* info);

/**
 * Computes intersection points between a curve rep and a surface rep.
 *
 * Uses OCCT's @c GeomAPI_IntCS.  Output uses the two-call buffer pattern:
 * call with @p out_buf == NULL to learn the required count, allocate, then
 * call again with a buffer of at least that size.
 *
 * @param[in]  graph      Graph containing both reps.  Must be non-NULL.
 * @param[in]  curve_id   Curve rep id.  Must be valid.
 * @param[in]  surface_id Surface rep id.  Must be valid.
 * @param[out] out_buf    Buffer to receive intersection points.
 *                        May be NULL on sizing call.
 * @param[in]  capacity   Number of elements @p out_buf can hold.
 * @param[out] out_count  Receives the number of intersection points.
 *                        Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_count is NULL, or a rep id is invalid.
 * @retval OCCTL_BUFFER_TOO_SMALL @p capacity < *@p out_count.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_surface_intersect
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_intersect_curve(const occtl_graph_t* graph,
                                                                  occtl_rep_id_t       surface_id,
                                                                  occtl_rep_id_t       curve_id,
                                                                  occtl_point3_t*      out_buf,
                                                                  size_t               capacity,
                                                                  size_t*              out_count);

/**
 * Computes intersection curves between two surfaces.
 *
 * Uses OCCT's @c GeomAPI_IntSS.  Returns an array of intersection rep ids.
 * The array itself is allocated by the function; the caller must free it
 * with @c free().
 *
 * @param[in]  graph       Graph containing both surfaces.  Must be non-NULL.
 * @param[in]  surface_a   First surface rep id.  Must be valid.
 * @param[in]  surface_b   Second surface rep id.  Must be valid.
 * @param[in]  tolerance   Intersection tolerance in 3D.
 * @param[out] out_ids     Receives a caller-owned array of curve rep ids.
 *                         Must be non-NULL.  Free with @c free().
 * @param[out] out_count   Receives the number of intersection curves.
 *                         Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph, @p out_ids, or @p out_count is NULL, or a surface rep id
 * is invalid.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_surface_intersect_curve
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_surface_intersect(occtl_graph_t*   graph,
                                                                    occtl_rep_id_t   surface_a,
                                                                    occtl_rep_id_t   surface_b,
                                                                    double           tolerance,
                                                                    occtl_rep_id_t** out_ids,
                                                                    size_t*          out_count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_SURFACES_H */
