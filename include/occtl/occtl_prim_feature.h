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
 * @file occtl_prim_feature.h
 * @brief OCCT-Light: feature and recipe-based model construction API.
 */

#ifndef OCCTL_PRIM_FEATURE_H
#define OCCTL_PRIM_FEATURE_H

#include "occtl_core.h"
#include "occtl_curves2d.h"
#include "occtl_geom.h"
#include "occtl_surfaces.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_prim_make_face_from_surface.
 *
 * Builds a Face from a surface rep and optional boundary wires.  When
 * @c outer_wire is invalid, no boundary wires may be supplied and OCCT builds
 * a face from the surface's natural bounds.  When @c outer_wire is valid, it
 * is used as the outer boundary and @c inner_wires, if any, are added as holes.
 */
typedef struct occtl_prim_face_from_surface_options
{
  uint32_t       struct_version; /**< Must be #OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_VERSION_1. */
  const void*    p_next;         /**< Reserved; set to NULL. */
  occtl_rep_id_t surface_id;     /**< Rep ID of the surface in the graph. */
  occtl_node_id_t
    outer_wire; /**< Borrows it. Optional outer Wire, or invalid for natural bounds. */
  const occtl_node_id_t* inner_wires;      /**< Borrows it. Optional inner Wire array. */
  size_t                 inner_wire_count; /**< Number of entries in @c inner_wires. */
  double                 tolerance;        /**< OCCT face-building tolerance. Default 1e-6. */
} occtl_prim_face_from_surface_options_t;

#define OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_INIT                                                  \
  {OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_VERSION_1,                                                 \
   NULL,                                                                                           \
   OCCTL_REP_ID_INVALID,                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   NULL,                                                                                           \
   0,                                                                                              \
   1.0e-6}

/**
 * Initialises @p options to defaults matching
 * #OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_face_from_surface
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_face_from_surface_options_init(occtl_prim_face_from_surface_options_t* options);

/**
 * Builds a Face from a surface handle and optional boundary wires.
 *
 * The operation inserts a Face node into @p graph from a graph-owned surface
 * rep and optional boundary Wires. The source surface handle and boundary
 * wires are not modified.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     options  Borrows it. Must be non-NULL and carry a supported
 *                         @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. Receives the new Face.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  A required pointer is NULL, @c p_next is
 *                                 non-NULL, @c tolerance is invalid, or inner
 *                                 wires are supplied without an outer wire.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         A boundary Wire node is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A boundary node is not a Wire.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the surface/wire combination.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested as a Face.
 *
 * @threadsafe No (mutates @p graph).
 *
 * @sa occtl_topo_make_filling_patch, occtl_surface_create_interpolated
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_face_from_surface(occtl_graph_t*                                graph,
                                    const occtl_prim_face_from_surface_options_t* options,
                                    occtl_node_id_t*                              out_face);

#define OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_prim_make_face_from_point_grid.
 *
 * Builds a B-spline surface from a row-major point grid and inserts a Face
 * using the generated surface's natural bounds.  This is the graph-facing
 * counterpart to #occtl_surface_create_from_point_grid.
 */
typedef struct occtl_prim_face_from_point_grid_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  occtl_surface_point_grid_create_info_t surface; /**< Point-grid surface construction options. */
  double tolerance; /**< OCCT face-building tolerance. Default 1e-6. */
} occtl_prim_face_from_point_grid_options_t;

#define OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_INIT                                               \
  {OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_VERSION_1,                                              \
   NULL,                                                                                           \
   OCCTL_SURFACE_POINT_GRID_CREATE_INFO_INIT,                                                      \
   1.0e-6}

/**
 * Initialises @p options to defaults matching
 * #OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_face_from_point_grid
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_face_from_point_grid_options_init(occtl_prim_face_from_point_grid_options_t* options);

/**
 * Builds a graph Face directly from a point-grid B-spline surface.
 *
 * The operation delegates surface fitting to OCCT through
 * #occtl_surface_create_from_point_grid, delegates face construction to OCCT,
 * and inserts the resulting Face topology into @p graph.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     options  Borrows it. Must be non-NULL and carry a supported
 *                         @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. Receives the new Face.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  A required pointer is NULL, @c p_next is
 *                                 non-NULL, @c tolerance is invalid, or the
 *                                 embedded point-grid surface options are
 *                                 invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options or the embedded surface options
 *                                 have an unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the point grid or face build.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested as a Face.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates @p graph).
 *
 * @sa occtl_prim_make_face_from_surface, occtl_surface_create_from_point_grid
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_face_from_point_grid(occtl_graph_t*                                   graph,
                                       const occtl_prim_face_from_point_grid_options_t* options,
                                       occtl_node_id_t*                                 out_face);

#define OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_prim_make_face_from_boundary_curves.
 *
 * Builds a B-spline surface from two, three, or four contiguous boundary
 * curves and inserts a Face using the generated surface's natural bounds.
 */
typedef struct occtl_prim_face_from_boundary_curves_options
{
  uint32_t struct_version; /**< Must be #OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_VERSION_1. */
  const void* p_next;      /**< Reserved; set to NULL. */
  occtl_surface_boundary_curves_create_info_t
         surface;   /**< Boundary-curve surface construction options. */
  double tolerance; /**< OCCT face-building tolerance. Default 1e-6. */
} occtl_prim_face_from_boundary_curves_options_t;

#define OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_INIT                                          \
  {OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_VERSION_1,                                         \
   NULL,                                                                                           \
   OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_INIT,                                                 \
   1.0e-6}

/**
 * Initialises @p options to defaults matching
 * #OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_face_from_boundary_curves
 */
OCCTL_API void OCCTL_CALL occtl_prim_face_from_boundary_curves_options_init(
  occtl_prim_face_from_boundary_curves_options_t* options);

/**
 * Builds a graph Face directly from boundary-curve surface filling.
 *
 * The operation delegates surface construction to OCCT through
 * #occtl_surface_create_from_boundary_curves, delegates face construction to
 * OCCT, and inserts the resulting Face topology into @p graph.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     options  Borrows it. Must be non-NULL and carry a supported
 *                         @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. Receives the new Face.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  A required pointer is NULL, @c p_next is
 *                                 non-NULL, @c tolerance is invalid, or the
 *                                 embedded boundary-curve options are invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options or the embedded surface options
 *                                 have an unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the boundary curves or face build.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested as a Face.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates @p graph).
 *
 * @sa occtl_prim_make_face_from_surface, occtl_surface_create_from_boundary_curves
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_face_from_boundary_curves(
  occtl_graph_t*                                        graph,
  const occtl_prim_face_from_boundary_curves_options_t* options,
  occtl_node_id_t*                                      out_face);

#define OCCTL_PRIM_FACE_FROM_CURVE_GRID_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_prim_make_face_from_curve_grid.
 *
 * Builds a Gordon B-spline surface from an intersecting U/V curve network and
 * inserts a Face using the generated surface's natural bounds.
 */
typedef struct occtl_prim_face_from_curve_grid_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_PRIM_FACE_FROM_CURVE_GRID_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  occtl_surface_curve_grid_create_info_t surface; /**< Curve-grid surface construction options. */
  double tolerance; /**< OCCT face-building tolerance. Default 1e-6. */
} occtl_prim_face_from_curve_grid_options_t;

#define OCCTL_PRIM_FACE_FROM_CURVE_GRID_OPTIONS_INIT                                               \
  {OCCTL_PRIM_FACE_FROM_CURVE_GRID_OPTIONS_VERSION_1,                                              \
   NULL,                                                                                           \
   OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_INIT,                                                      \
   1.0e-6}

/**
 * Initialises @p options to defaults matching
 * #OCCTL_PRIM_FACE_FROM_CURVE_GRID_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_face_from_curve_grid
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_face_from_curve_grid_options_init(occtl_prim_face_from_curve_grid_options_t* options);

/**
 * Builds a graph Face directly from a curve-grid Gordon surface.
 *
 * The operation delegates surface construction to OCCT through
 * #occtl_surface_create_from_curve_grid, delegates face construction to OCCT,
 * and inserts the resulting Face topology into @p graph.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     options  Borrows it. Must be non-NULL and carry a supported
 *                         @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. Receives the new Face.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  A required pointer is NULL, @c p_next is
 *                                 non-NULL, @c tolerance is invalid, or the
 *                                 embedded curve-grid options are invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options or the embedded surface options
 *                                 have an unsupported @c struct_version.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the curve grid or face build.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested as a Face.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates @p graph).
 *
 * @sa occtl_prim_make_face_from_surface, occtl_surface_create_from_curve_grid
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_face_from_curve_grid(occtl_graph_t*                                   graph,
                                       const occtl_prim_face_from_curve_grid_options_t* options,
                                       occtl_node_id_t*                                 out_face);

#define OCCTL_PRIM_BRAKE_FORMED_OPTIONS_VERSION_1 1u

/**
 * Offset side for #occtl_prim_make_brake_formed.
 */
typedef enum occtl_prim_brake_side
{
  OCCTL_PRIM_BRAKE_SIDE_LEFT            = 0, /**< Offset to OCCT's positive planar-wire side. */
  OCCTL_PRIM_BRAKE_SIDE_RIGHT           = 1, /**< Offset to OCCT's negative planar-wire side. */
  OCCTL_PRIM_BRAKE_SIDE_RESERVED_FUTURE = 0x7fffffff
} occtl_prim_brake_side_t;

/**
 * Configuration for #occtl_prim_make_brake_formed.
 *
 * Builds a sheet-metal style solid from a planar bend line.  @c station_widths
 * may contain one width reused for every bend-line station, or one width per
 * station vertex in @c line.
 */
typedef struct occtl_prim_brake_formed_options
{
  uint32_t        struct_version;        /**< Must be #OCCTL_PRIM_BRAKE_FORMED_OPTIONS_VERSION_1. */
  const void*     p_next;                /**< Reserved; set to NULL. */
  occtl_node_id_t line;                  /**< Borrows it. Edge or Wire bend line. */
  double          thickness;             /**< Positive sheet thickness. */
  const double*   station_widths;        /**< Borrows it. Width array; length 1 or station count. */
  size_t          station_width_count;   /**< Number of entries in @c station_widths. */
  occtl_prim_brake_side_t          side; /**< Offset side; default LEFT. */
  occtl_topo_wire_offset_2d_join_t join; /**< Offset corner join style; default ARC. */
  int32_t approximate;                   /**< 0/1. Ask OCCT to approximate offset contours. */
  double  tolerance;                     /**< Positive loft tolerance; default 1e-6. */
} occtl_prim_brake_formed_options_t;

#define OCCTL_PRIM_BRAKE_FORMED_OPTIONS_INIT                                                       \
  {OCCTL_PRIM_BRAKE_FORMED_OPTIONS_VERSION_1,                                                      \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   1.0,                                                                                            \
   NULL,                                                                                           \
   0,                                                                                              \
   OCCTL_PRIM_BRAKE_SIDE_LEFT,                                                                     \
   OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC,                                                             \
   0,                                                                                              \
   1.0e-6}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_PRIM_BRAKE_FORMED_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_brake_formed
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_brake_formed_options_init(occtl_prim_brake_formed_options_t* opts);

/**
 * Builds a sheet-metal brake-formed solid from a planar bend line.
 *
 * The operation offsets @c line by @c thickness, creates station sections
 * between the original and offset stations, extends those sections by the
 * supplied station widths, and delegates solid skinning to OCCT.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the brake-formed Solid NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, @c p_next is
 *                                 non-NULL, thickness / tolerance / widths are
 *                                 invalid, or side / join flags are unknown.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c line is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @c line is neither an Edge nor a Wire.
 * @retval OCCTL_GEOMETRY_INVALID  Offset, station construction, or solid
 *                                 skinning failed.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into the output graph.
 *
 * @threadsafe No. Allocates a new graph.
 *
 * @sa occtl_topo_make_face_extrusion
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_brake_formed(const occtl_graph_t*                     graph,
                               const occtl_prim_brake_formed_options_t* options,
                               occtl_graph_t**                          out_graph,
                               occtl_node_id_t*                         out_root);

#define OCCTL_PRIM_DRAFT_PRISM_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_draft_prism.
 *
 * Builds a tapered solid from one planar Face using OCCT @c LocOpe_DPrism.
 * This is the accurate fast path for draft/tapered extrusion of a single
 * closed profile.
 */
typedef struct occtl_prim_draft_prism_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_DRAFT_PRISM_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t profile;        /**< Borrows it. Face to taper/extrude. */
  double          height;         /**< Positive OCCT draft-prism height. */
  double          taper_angle;    /**< Draft angle in radians. */
} occtl_prim_draft_prism_info_t;

#define OCCTL_PRIM_DRAFT_PRISM_INFO_INIT                                                           \
  {OCCTL_PRIM_DRAFT_PRISM_INFO_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, 1.0, 0.0}

/**
 * Runtime initialiser for #occtl_prim_draft_prism_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_DRAFT_PRISM_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_draft_prism
 */
OCCTL_API void OCCTL_CALL occtl_prim_draft_prism_info_init(occtl_prim_draft_prism_info_t* info);

/**
 * Builds a tapered prism solid from one Face profile.
 *
 * The result is inserted into @p graph as a new topology root. The input
 * profile is not modified.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Solid NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, or @c height is
 *                                 non-positive.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c profile refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c profile is not a Face.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the draft prism.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_prism
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_draft_prism(occtl_graph_t*                       graph,
                              const occtl_prim_draft_prism_info_t* info,
                              occtl_node_id_t*                     out_shape);

#define OCCTL_PRIM_EXTRUDE_TAPERED_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_extrude_tapered.
 *
 * Semantic wrapper for tapered extrusion of one Face profile. The taper
 * follows the profile Face normal.
 */
typedef struct occtl_prim_extrude_tapered_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_EXTRUDE_TAPERED_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t profile_face;   /**< Borrows it. Face to taper/extrude. */
  double          height;         /**< Positive OCCT draft-prism height. */
  double          taper_angle;    /**< Draft angle in radians. */
} occtl_prim_extrude_tapered_info_t;

#define OCCTL_PRIM_EXTRUDE_TAPERED_INFO_INIT                                                       \
  {OCCTL_PRIM_EXTRUDE_TAPERED_INFO_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, 1.0, 0.0}

/**
 * Runtime initialiser for #occtl_prim_extrude_tapered_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_EXTRUDE_TAPERED_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_extrude_tapered
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_extrude_tapered_info_init(occtl_prim_extrude_tapered_info_t* info);

/**
 * Builds a tapered extrusion from one Face profile.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Solid NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 or @c height is non-positive.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c profile_face refers to a removed / absent
 *                                 node.
 * @retval OCCTL_WRONG_KIND        @c profile_face is not a Face.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the tapered extrusion.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_draft_prism
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_extrude_tapered(occtl_graph_t*                           graph,
                                  const occtl_prim_extrude_tapered_info_t* info,
                                  occtl_node_id_t*                         out_shape);

#define OCCTL_PRIM_RULED_SURFACE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_ruled_surface.
 *
 * Builds a ruled surface between two existing graph sections using OCCT
 * @c BRepFill. Both sections must have the same topological kind:
 * Edge + Edge produces a Face, while Wire + Wire produces a Shell.
 */
typedef struct occtl_prim_ruled_surface_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_RULED_SURFACE_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t section_a;      /**< Borrows it. First Edge or Wire section. */
  occtl_node_id_t section_b;      /**< Borrows it. Second Edge or Wire section. */
} occtl_prim_ruled_surface_info_t;

#define OCCTL_PRIM_RULED_SURFACE_INFO_INIT                                                         \
  {OCCTL_PRIM_RULED_SURFACE_INFO_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, OCCTL_NODE_ID_INVALID}

/**
 * Runtime initialiser for #occtl_prim_ruled_surface_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_RULED_SURFACE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_ruled_surface
 */
OCCTL_API void OCCTL_CALL occtl_prim_ruled_surface_info_init(occtl_prim_ruled_surface_info_t* info);

/**
 * Builds a ruled Face or Shell between two Edge or Wire sections.
 *
 * The result is inserted into @p graph as a new topology root. The input
 * sections are not modified. Wire sections must be compatible for OCCT
 * ruled-shell construction, including matching edge counts.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Face or Shell NodeId; on failure
 *                          set to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 or the sections do not have the same kind.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A section refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        A section is neither an Edge nor a Wire.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the ruled surface.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_loft, occtl_prim_make_planar_face
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_ruled_surface(occtl_graph_t*                         graph,
                                const occtl_prim_ruled_surface_info_t* info,
                                occtl_node_id_t*                       out_shape);

/**
 * Until-mode for #occtl_prim_make_feat_prism.
 *
 * Selects how the extrusion is terminated (fixed length, until a target
 * shape, all the way through, or until a target capped at a length).
 */
typedef enum occtl_prim_until_kind
{
  OCCTL_UNTIL_LENGTH          = 0, /**< Extrude a fixed length along @c direction. */
  OCCTL_UNTIL_SHAPE           = 1, /**< Extrude until the result meets @c until_shape. */
  OCCTL_UNTIL_THRU_ALL        = 2, /**< Extrude through all of @c base_shape. */
  OCCTL_UNTIL_HEIGHT          = 3, /**< Extrude until @c until_shape, capped at @c length. */
  OCCTL_UNTIL_RESERVED_FUTURE = 0x7fffffff
} occtl_prim_until_kind_t;

/**
 * Combine-mode for feature operations.
 *
 * Controls how the produced material interacts with the base shape.
 */
typedef enum occtl_prim_feat_combine
{
  OCCTL_FEAT_SEPARATE = 0, /**< Produce a separate body — no Boolean against @c base_shape. */
  OCCTL_FEAT_CUT      = 1, /**< Subtract the produced material from @c base_shape. */
  OCCTL_FEAT_FUSE     = 2, /**< Add the produced material to @c base_shape. */
  OCCTL_FEAT_COMBINE_RESERVED_FUTURE = 0x7fffffff
} occtl_prim_feat_combine_t;

#define OCCTL_PRIM_FEAT_PRISM_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_feat_prism.
 *
 * Builds a feature prism on an existing body: extrudes @c profile and
 * combines the result with @c base_shape (fuse / cut / separate). Supports
 * termination by target shape or height.
 */
typedef struct occtl_prim_feat_prism_info
{
  uint32_t           struct_version; /**< Must be #OCCTL_PRIM_FEAT_PRISM_INFO_VERSION_1. */
  const void*        p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t    base_shape; /**< Borrows it. Existing Solid / Shell the feature is built on. */
  occtl_node_id_t    profile;    /**< Borrows it. Face / Wire to extrude. */
  occtl_node_id_t    sketch_face;    /**< Borrows it. Face of @c base_shape hosting @c profile. */
  occtl_direction3_t direction;      /**< Extrusion direction. */
  occtl_prim_feat_combine_t combine; /**< Cut / Fuse / Separate. */
  int32_t                   modify;  /**< 0/1. When 1, allow OCCT to modify base topology. */
  occtl_prim_until_kind_t   until_kind; /**< Termination rule. */
  occtl_node_id_t until_shape; /**< Borrows it. Required for @c SHAPE and @c HEIGHT modes. */
  double          length;      /**< Required for @c LENGTH and @c HEIGHT modes. */
} occtl_prim_feat_prism_info_t;

#define OCCTL_PRIM_FEAT_PRISM_INFO_INIT                                                            \
  {OCCTL_PRIM_FEAT_PRISM_INFO_VERSION_1,                                                           \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {0.0, 0.0, 1.0},                                                                                \
   OCCTL_FEAT_FUSE,                                                                                \
   1,                                                                                              \
   OCCTL_UNTIL_LENGTH,                                                                             \
   OCCTL_NODE_ID_INVALID,                                                                          \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_feat_prism_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_FEAT_PRISM_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_feat_prism
 */
OCCTL_API void OCCTL_CALL occtl_prim_feat_prism_info_init(occtl_prim_feat_prism_info_t* info);

/**
 * Builds a feature prism — the canonical "extrude profile on existing body until …" operation.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, or @c length is non-positive for
 * LENGTH/HEIGHT modes, or @c direction has zero length.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A node id refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c sketch_face is not a Face, or @c until_shape is required but
 *                                 missing / has wrong kind.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_prism
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_feat_prism(occtl_graph_t*                      graph,
                             const occtl_prim_feat_prism_info_t* info,
                             occtl_node_id_t*                    out_shape);

/**
 * Target side for #occtl_prim_make_extrude_until.
 */
typedef enum occtl_prim_extrude_until_side
{
  OCCTL_EXTRUDE_UNTIL_NEXT            = 0, /**< Extrude along @c direction to the target. */
  OCCTL_EXTRUDE_UNTIL_LAST            = 1, /**< Extrude along @c direction to the target. */
  OCCTL_EXTRUDE_UNTIL_PREVIOUS        = 2, /**< Extrude opposite @c direction to the target. */
  OCCTL_EXTRUDE_UNTIL_FIRST           = 3, /**< Extrude opposite @c direction to the target. */
  OCCTL_EXTRUDE_UNTIL_RESERVED_FUTURE = 0x7fffffff
} occtl_prim_extrude_until_side_t;

#define OCCTL_PRIM_EXTRUDE_UNTIL_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_extrude_until.
 *
 * Builds a feature extrusion from @c profile on @c base_shape until
 * @c target_shape. FIRST/PREVIOUS map to the opposite of @c direction, and
 * NEXT/LAST map to @c direction.
 */
typedef struct occtl_prim_extrude_until_info
{
  uint32_t           struct_version; /**< Must be #OCCTL_PRIM_EXTRUDE_UNTIL_INFO_VERSION_1. */
  const void*        p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t    base_shape; /**< Borrows it. Existing Solid / Shell the feature is built on. */
  occtl_node_id_t    profile;    /**< Borrows it. Face / Wire to extrude. */
  occtl_node_id_t    sketch_face;  /**< Borrows it. Face of @c base_shape hosting @c profile. */
  occtl_node_id_t    target_shape; /**< Borrows it. Shape that terminates the extrusion. */
  occtl_direction3_t direction;    /**< Reference extrusion direction. */
  occtl_prim_extrude_until_side_t side;    /**< FIRST / PREVIOUS / NEXT / LAST side selector. */
  occtl_prim_feat_combine_t       combine; /**< Cut / Fuse / Separate. */
  int32_t                         modify;  /**< 0/1. When 1, allow OCCT to modify base topology. */
  double limit; /**< Optional positive cap length; 0 means unbounded target mode. */
} occtl_prim_extrude_until_info_t;

#define OCCTL_PRIM_EXTRUDE_UNTIL_INFO_INIT                                                         \
  {OCCTL_PRIM_EXTRUDE_UNTIL_INFO_VERSION_1,                                                        \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {0.0, 0.0, 1.0},                                                                                \
   OCCTL_EXTRUDE_UNTIL_NEXT,                                                                       \
   OCCTL_FEAT_FUSE,                                                                                \
   1,                                                                                              \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_extrude_until_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_EXTRUDE_UNTIL_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_extrude_until
 */
OCCTL_API void OCCTL_CALL occtl_prim_extrude_until_info_init(occtl_prim_extrude_until_info_t* info);

/**
 * Builds a feature extrusion until a target shape.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 @c direction is zero, @c side / @c combine /
 *                                 @c modify is invalid, or @c limit is negative.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A node id refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c sketch_face is not a Face.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the feature extrusion.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_feat_prism, occtl_prim_make_draft_prism
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_extrude_until(occtl_graph_t*                         graph,
                                const occtl_prim_extrude_until_info_t* info,
                                occtl_node_id_t*                       out_shape);

#define OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_feat_draft_prism.
 *
 * Builds a draft prism feature on an existing body.  Unlike
 * #occtl_prim_make_draft_prism, this operation delegates to OCCT
 * @c BRepFeat_MakeDPrism and combines the tapered feature with
 * @c base_shape.
 */
typedef struct occtl_prim_feat_draft_prism_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t base_shape;   /**< Borrows it. Existing Solid / Shell the feature is built on. */
  occtl_node_id_t profile_face; /**< Borrows it. Face of @c base_shape to taper/extrude. */
  occtl_node_id_t sketch_face;  /**< Borrows it. Face of @c base_shape hosting the sketch. */
  double          taper_angle;  /**< Draft angle in radians. */
  occtl_prim_feat_combine_t combine;    /**< Cut / Fuse / Separate. */
  int32_t                   modify;     /**< 0/1. When 1, allow OCCT to modify base topology. */
  occtl_prim_until_kind_t   until_kind; /**< Termination rule. */
  occtl_node_id_t until_shape; /**< Borrows it. Required for @c SHAPE and @c HEIGHT modes. */
  double          length;      /**< Required for @c LENGTH and @c HEIGHT modes. */
} occtl_prim_feat_draft_prism_info_t;

#define OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_INIT                                                      \
  {OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_VERSION_1,                                                     \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   0.0,                                                                                            \
   OCCTL_FEAT_FUSE,                                                                                \
   1,                                                                                              \
   OCCTL_UNTIL_LENGTH,                                                                             \
   OCCTL_NODE_ID_INVALID,                                                                          \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_feat_draft_prism_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_feat_draft_prism
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_feat_draft_prism_info_init(occtl_prim_feat_draft_prism_info_t* info);

/**
 * Builds a draft-prism feature on an existing body.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version and NULL @c p_next.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 @c length is non-positive for LENGTH/HEIGHT
 *                                 modes, @c taper_angle is not finite, or an
 *                                 enum value is unknown.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A node id refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c profile_face or @c sketch_face is not a
 *                                 Face, or @c until_shape is required but has
 *                                 wrong kind.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_feat_prism, occtl_prim_make_draft_prism
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_feat_draft_prism(occtl_graph_t*                            graph,
                                   const occtl_prim_feat_draft_prism_info_t* info,
                                   occtl_node_id_t*                          out_shape);

/**
 * Cylindrical-hole termination mode for #occtl_prim_make_cylindrical_hole.
 *
 * Selects how far the cylindrical cutting volume extends along the supplied
 * axis.  The default through-all mode cuts with an infinite cylinder.
 */
typedef enum occtl_prim_cylindrical_hole_kind
{
  OCCTL_CYLINDRICAL_HOLE_THROUGH_ALL =
    0, /**< Cut all material intersected by the infinite cylinder. */
  OCCTL_CYLINDRICAL_HOLE_BETWEEN_PARAMS =
    1,                                  /**< Cut between @c p_from and @c p_to along @c axis. */
  OCCTL_CYLINDRICAL_HOLE_THRU_NEXT = 2, /**< Cut to the next encountered boundary along @c axis. */
  OCCTL_CYLINDRICAL_HOLE_UNTIL_END = 3, /**< Cut all material after the axis origin. */
  OCCTL_CYLINDRICAL_HOLE_BLIND     = 4, /**< Cut a blind hole of @c length from the axis origin. */
  OCCTL_CYLINDRICAL_HOLE_RESERVED_FUTURE = 0x7fffffff
} occtl_prim_cylindrical_hole_kind_t;

#define OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_cylindrical_hole.
 *
 * Cuts one cylindrical hole feature from @c base_shape.  @c axis.location is
 * the start point for directed modes and @c axis.direction gives the drilling
 * direction.  The result is inserted as a new topology root; the input body is
 * left unchanged in the graph.
 */
typedef struct occtl_prim_cylindrical_hole_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t base_shape;     /**< Borrows it. Existing Solid / Shell / Shape to cut. */
  occtl_axis1_placement_t            axis;   /**< Hole axis; direction must be non-zero. */
  double                             radius; /**< Hole radius; strictly positive. */
  occtl_prim_cylindrical_hole_kind_t kind;   /**< Hole extent mode. */
  double                             p_from; /**< Start parameter for @c BETWEEN_PARAMS mode. */
  double  p_to;   /**< End parameter for @c BETWEEN_PARAMS mode; must be greater than @c p_from. */
  double  length; /**< Positive depth for @c BLIND mode. */
  int32_t with_control; /**< 0/1. When 1, OCCT validates the result after cutting. */
} occtl_prim_cylindrical_hole_info_t;

#define OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_INIT                                                      \
  {OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_VERSION_1,                                                     \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}},                                                             \
   0.0,                                                                                            \
   OCCTL_CYLINDRICAL_HOLE_THROUGH_ALL,                                                             \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   1}

/**
 * Runtime initialiser for #occtl_prim_cylindrical_hole_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_cylindrical_hole
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_cylindrical_hole_info_init(occtl_prim_cylindrical_hole_info_t* info);

/**
 * Cuts a cylindrical hole feature into an existing shape.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 @c axis contains non-finite components, @c with_control is
 *                                 not 0/1, @c radius is non-positive, @c axis.direction has
 *                                 zero length, @c p_from or @c p_to is non-finite,
 *                                 @c p_to <= p_from in BETWEEN_PARAMS mode, @c length is
 *                                 non-positive in BLIND mode, or @c kind is unknown.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c base_shape refers to a removed / absent node.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the hole placement or failed
 *                                 to build the feature.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_feat_prism, occtl_prim_make_thick_solid
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_cylindrical_hole(occtl_graph_t*                            graph,
                                   const occtl_prim_cylindrical_hole_info_t* info,
                                   occtl_node_id_t*                          out_shape);

#ifdef __cplusplus
}
#endif

#endif /* OCCTL_PRIM_FEATURE_H */
