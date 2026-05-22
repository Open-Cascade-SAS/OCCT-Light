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
 * @file occtl_prim_sketch.h
 * @brief OCCT-Light: sketch, curve, wire, and planar construction API.
 */

#ifndef OCCTL_PRIM_SKETCH_H
#define OCCTL_PRIM_SKETCH_H

#include "occtl_core.h"
#include "occtl_curves2d.h"
#include "occtl_geom.h"
#include "occtl_surfaces.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_PRIM_POLYLINE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_polyline.
 *
 * Builds an open or closed wire from a sequence of straight-line edges
 * threaded between @c points (a polygon wire).
 */
typedef struct occtl_prim_polyline_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_PRIM_POLYLINE_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved for extensions; must be NULL. */
  const occtl_point3_t* points; /**< Borrows it. Array of vertex coordinates; @c >= 2 points. */
  size_t                point_count; /**< Length of @c points. */
  int32_t closed; /**< 0/1. When 1, a closing segment from last to first point is added. */
} occtl_prim_polyline_info_t;

#define OCCTL_PRIM_POLYLINE_INFO_INIT {OCCTL_PRIM_POLYLINE_INFO_VERSION_1, NULL, NULL, 0, 0}

/**
 * Runtime initialiser for #occtl_prim_polyline_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_POLYLINE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_polyline
 */
OCCTL_API void OCCTL_CALL occtl_prim_polyline_info_init(occtl_prim_polyline_info_t* info);

/**
 * Builds a Wire from a sequence of straight-line edges.
 *
 * Convenience entry point for sketch construction; avoids the verbose
 * make_vertex / make_edge / make_wire trio when the segments are purely
 * linear.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL or @c point_count < 2.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Two consecutive points coincide.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_polyline_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_polyline(occtl_graph_t* graph,
                                                             const occtl_prim_polyline_info_t* info,
                                                             occtl_node_id_t* out_wire);

#define OCCTL_PRIM_REGULAR_POLYGON_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_regular_polygon.
 *
 * Builds a closed @c sides-sided wire inscribed in a circle of radius
 * @c circumradius lying in the XY plane of @c placement. @c rotation
 * offsets the first vertex from the placement's X axis (radians).
 */
typedef struct occtl_prim_regular_polygon_info
{
  uint32_t    struct_version;        /**< Must be #OCCTL_PRIM_REGULAR_POLYGON_INFO_VERSION_1. */
  const void* p_next;                /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement; /**< Centre and orientation; defaults to XOY. */
  double  circumradius;              /**< Radius of the circumscribing circle; strictly positive. */
  int32_t sides;                     /**< Number of sides; @c >= 3. */
  double  rotation;                  /**< Angle of the first vertex above the X axis (radians). */
} occtl_prim_regular_polygon_info_t;

#define OCCTL_PRIM_REGULAR_POLYGON_INFO_INIT                                                       \
  {OCCTL_PRIM_REGULAR_POLYGON_INFO_VERSION_1,                                                      \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   3,                                                                                              \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_regular_polygon_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_REGULAR_POLYGON_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_regular_polygon
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_regular_polygon_info_init(occtl_prim_regular_polygon_info_t* info);

/**
 * Builds a closed Wire for a regular n-gon.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL or @c sides < 3 or @c circumradius <= 0.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_regular_polygon_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_regular_polygon(occtl_graph_t*                           graph,
                                  const occtl_prim_regular_polygon_info_t* info,
                                  occtl_node_id_t*                         out_wire);

#define OCCTL_PRIM_RECTANGLE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_rectangle.
 *
 * Builds a closed four-edge wire centred on @c placement.location, with
 * @c width along the placement's X axis and @c height along its Y axis.
 */
typedef struct occtl_prim_rectangle_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_RECTANGLE_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Centre and orientation; defaults to XOY. */
  double                  width;  /**< Extent along the placement X axis; strictly positive. */
  double                  height; /**< Extent along the placement Y axis; strictly positive. */
} occtl_prim_rectangle_info_t;

#define OCCTL_PRIM_RECTANGLE_INFO_INIT                                                             \
  {OCCTL_PRIM_RECTANGLE_INFO_VERSION_1,                                                            \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_rectangle_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_RECTANGLE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_rectangle
 */
OCCTL_API void OCCTL_CALL occtl_prim_rectangle_info_init(occtl_prim_rectangle_info_t* info);

/**
 * Builds a closed Wire for an axis-aligned rectangle.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c width or @c height is non-positive.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_rectangle_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_rectangle(occtl_graph_t*                     graph,
                            const occtl_prim_rectangle_info_t* info,
                            occtl_node_id_t*                   out_wire);

#define OCCTL_PRIM_CIRCLE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_circle.
 *
 * Builds a one-edge closed wire for a full circle of radius @c radius
 * lying in the XY plane of @c placement.
 */
typedef struct occtl_prim_circle_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_CIRCLE_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Centre and orientation; defaults to XOY. */
  double                  radius;         /**< Strictly positive. */
} occtl_prim_circle_info_t;

#define OCCTL_PRIM_CIRCLE_INFO_INIT                                                                \
  {OCCTL_PRIM_CIRCLE_INFO_VERSION_1, NULL, {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}}, 0.0}

/**
 * Runtime initialiser for #occtl_prim_circle_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_CIRCLE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_circle
 */
OCCTL_API void OCCTL_CALL occtl_prim_circle_info_init(occtl_prim_circle_info_t* info);

/**
 * Builds a one-edge closed Wire on a full circle.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c radius is non-positive.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_circle_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_circle(occtl_graph_t*                  graph,
                                                           const occtl_prim_circle_info_t* info,
                                                           occtl_node_id_t* out_wire);

#define OCCTL_PRIM_ELLIPSE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_ellipse.
 *
 * Builds a one-edge closed wire for a full ellipse of semi-axes @c major
 * (along placement X) and @c minor (along placement Y), with the centre
 * at @c placement.location.
 */
typedef struct occtl_prim_ellipse_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_ELLIPSE_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Centre and orientation; defaults to XOY. */
  double                  major; /**< Semi-major axis (along placement X); @c >= minor > 0. */
  double                  minor; /**< Semi-minor axis (along placement Y); strictly positive. */
} occtl_prim_ellipse_info_t;

#define OCCTL_PRIM_ELLIPSE_INFO_INIT                                                               \
  {OCCTL_PRIM_ELLIPSE_INFO_VERSION_1,                                                              \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_ellipse_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_ELLIPSE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_ellipse
 */
OCCTL_API void OCCTL_CALL occtl_prim_ellipse_info_init(occtl_prim_ellipse_info_t* info);

/**
 * Builds a one-edge closed Wire on a full ellipse.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c major < @c minor or either non-positive.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_ellipse_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_ellipse(occtl_graph_t*                   graph,
                                                            const occtl_prim_ellipse_info_t* info,
                                                            occtl_node_id_t* out_wire);

#define OCCTL_PRIM_PLANAR_FACE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_planar_face.
 *
 * Builds a planar face from one closed @c outer_wire and zero or more
 * @c inner_wires (holes). The face's supporting plane is inferred from
 * @c outer_wire (OCCT auto-detects); inner wires must lie in the same
 * plane.
 */
typedef struct occtl_prim_planar_face_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_PLANAR_FACE_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t outer_wire;     /**< Borrows it. Closed planar wire forming the outer boundary. */
  const occtl_node_id_t*
         inner_wires;      /**< Borrows it. Optional array of hole wires (may be NULL). */
  size_t inner_wire_count; /**< Length of @c inner_wires. */
} occtl_prim_planar_face_info_t;

#define OCCTL_PRIM_PLANAR_FACE_INFO_INIT                                                           \
  {OCCTL_PRIM_PLANAR_FACE_INFO_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, NULL, 0}

/**
 * Runtime initialiser for #occtl_prim_planar_face_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_PLANAR_FACE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_planar_face
 */
OCCTL_API void OCCTL_CALL occtl_prim_planar_face_info_init(occtl_prim_planar_face_info_t* info);

/**
 * Builds a planar Face from a closed planar wire and optional hole wires.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. On success
 *                          receives the new Face NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, or @c inner_wires is NULL while @c
 * inner_wire_count > 0.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A wire NodeId refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        A wire NodeId is not of kind #OCCTL_KIND_WIRE.
 * @retval OCCTL_GEOMETRY_INVALID  The wire is not planar / not closed / inconsistent with the
 * inferred plane.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_planar_face_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_planar_face(occtl_graph_t*                       graph,
                              const occtl_prim_planar_face_info_t* info,
                              occtl_node_id_t*                     out_face);

#define OCCTL_PRIM_CONVEX_HULL_2D_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_convex_hull_2d.
 *
 * Builds a closed convex hull from explicit points and/or existing graph
 * Vertex nodes projected into @c placement.  The result is either a Wire or a
 * planar Face depending on @c make_face.  OCCT constructs the resulting
 * topology; the hull ordering is a deterministic 2D projected ordering in the
 * requested sketch plane.
 */
typedef struct occtl_prim_convex_hull_2d_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_CONVEX_HULL_2D_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Sketch plane frame; defaults to global XOY. */
  const occtl_point3_t*   points;         /**< Borrows it. Optional point array. */
  size_t                  point_count;    /**< Length of @c points. */
  const occtl_node_id_t*  vertices;       /**< Borrows it. Optional Vertex node array. */
  size_t                  vertex_count;   /**< Length of @c vertices. */
  double                  tolerance;      /**< Positive duplicate/collinearity tolerance. */
  int32_t                 make_face; /**< 0/1; when 1 returns a planar Face, otherwise a Wire. */
} occtl_prim_convex_hull_2d_info_t;

#define OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT                                                        \
  {OCCTL_PRIM_CONVEX_HULL_2D_INFO_VERSION_1,                                                       \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   NULL,                                                                                           \
   0u,                                                                                             \
   NULL,                                                                                           \
   0u,                                                                                             \
   1.0e-7,                                                                                         \
   0}

/**
 * Runtime initialiser for #occtl_prim_convex_hull_2d_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_convex_hull_2d
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_convex_hull_2d_info_init(occtl_prim_convex_hull_2d_info_t* info);

/**
 * Builds a convex-hull Wire or planar Face from points and/or Vertex nodes.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     info     Borrows it. Must be non-NULL with a recognised
 *                         @c struct_version.
 * @param[out]    out_node Borrows it. Must be non-NULL. On success receives
 *                         the new Wire or Face NodeId; on failure set to
 *                         #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 both input arrays are empty, an array pointer
 *                                 is NULL while its count is non-zero,
 *                                 @c tolerance is non-positive/non-finite, or
 *                                 @c make_face is not 0/1.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A Vertex node is removed / absent.
 * @retval OCCTL_WRONG_KIND        A node in @c vertices is not a Vertex.
 * @retval OCCTL_GEOMETRY_INVALID  Fewer than three non-collinear projected
 *                                 points remain, or OCCT cannot build the
 *                                 requested topology.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_convex_hull_2d_info_init, occtl_prim_make_polyline
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_convex_hull_2d(occtl_graph_t*                          graph,
                                 const occtl_prim_convex_hull_2d_info_t* info,
                                 occtl_node_id_t*                        out_node);

#define OCCTL_PRIM_TRACE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_trace.
 *
 * Builds a constant-width planar face around an Edge or open Wire path.
 * OCCT constructs the offset contour; OCCT-Light only wraps the resulting
 * closed contour as a planar Face and inserts it into the graph.  For a
 * single linear Edge, @c normal defines the sketch plane normal used to
 * centre the rectangular trace around the edge.
 */
typedef struct occtl_prim_trace_info
{
  uint32_t                         struct_version; /**< Must be #OCCTL_PRIM_TRACE_INFO_VERSION_1. */
  const void*                      p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t                  path;           /**< Borrows it. Edge or Wire node to thicken. */
  double                           width;          /**< Full trace width; strictly positive. */
  occtl_direction3_t               normal; /**< Sketch-plane normal for single linear Edge paths. */
  occtl_topo_wire_offset_2d_join_t join;   /**< Corner join style used by OCCT. */
  int32_t approximate;                     /**< 0/1; approximate input contours by arcs/segments. */
} occtl_prim_trace_info_t;

#define OCCTL_PRIM_TRACE_INFO_INIT                                                                 \
  {OCCTL_PRIM_TRACE_INFO_VERSION_1,                                                                \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   1.0,                                                                                            \
   {0.0, 0.0, 1.0},                                                                                \
   OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC,                                                             \
   0}

/**
 * Runtime initialiser for #occtl_prim_trace_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_TRACE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_trace
 */
OCCTL_API void OCCTL_CALL occtl_prim_trace_info_init(occtl_prim_trace_info_t* info);

/**
 * Builds a constant-width planar Face around an Edge or open Wire path.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     info     Borrows it. Must be non-NULL with a recognised
 *                         @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. On success receives
 *                         the new Face NodeId; on failure set to
 *                         #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 @c width is non-positive / non-finite, or
 *                                 @c normal / @c join / @c approximate is
 *                                 invalid.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c path refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c path is neither Edge nor Wire.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not construct a closed planar
 *                                 offset contour or planar face.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_trace_info_init, occtl_topo_wire_offset_2d
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_trace(occtl_graph_t*                 graph,
                                                          const occtl_prim_trace_info_t* info,
                                                          occtl_node_id_t*               out_face);

#define OCCTL_PRIM_CONSTRAINED_EDGE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_constrained_edge.
 *
 * Projects a 2D curve handle (for example a tangent/blend solution from
 * @c occtl_curves2d.h) onto a 3D sketch plane and builds one Edge from it.
 * OCCT performs the 2D-to-3D conversion; OCCT-Light only validates the ABI
 * inputs and inserts the resulting Edge into the graph. Unbounded curves
 * such as 2D lines need an explicit finite parameter range.
 */
typedef struct occtl_prim_constrained_edge_info
{
  uint32_t       struct_version;     /**< Must be #OCCTL_PRIM_CONSTRAINED_EDGE_INFO_VERSION_1. */
  const void*    p_next;             /**< Reserved; must be NULL. */
  occtl_rep_id_t curve;              /**< Rep ID of the 2D curve to place on @c placement. */
  occtl_axis2_placement_t placement; /**< Sketch plane frame; defaults to global XOY. */
  int32_t use_parameter_range;       /**< 0/1; when 1 use @c first_parameter / @c last_parameter. */
  double  first_parameter;           /**< First curve parameter when @c use_parameter_range is 1. */
  double  last_parameter;            /**< Last curve parameter when @c use_parameter_range is 1. */
} occtl_prim_constrained_edge_info_t;

#define OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT                                                      \
  {OCCTL_PRIM_CONSTRAINED_EDGE_INFO_VERSION_1,                                                     \
   NULL,                                                                                           \
   OCCTL_REP_ID_INVALID,                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0,                                                                                              \
   0.0,                                                                                            \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_constrained_edge_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_constrained_edge
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_constrained_edge_info_init(occtl_prim_constrained_edge_info_t* info);

/**
 * Builds one Edge from a 2D curve placed on a 3D sketch plane.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     info     Borrows it. Must be non-NULL with a recognised
 *                         @c struct_version.
 * @param[out]    out_edge Borrows it. Must be non-NULL. On success receives
 *                         the new Edge NodeId; on failure set to
 *                         #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 the placement is degenerate,
 *                                 @c use_parameter_range is not 0/1, or the
 *                                 explicit range is non-finite / reversed.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not build an Edge from the
 *                                 placed curve.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_constrained_edge_info_init, occtl_curve2d_create_blend_arc
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_constrained_edge(occtl_graph_t*                            graph,
                                   const occtl_prim_constrained_edge_info_t* info,
                                   occtl_node_id_t*                          out_edge);

/**
 * Trihedron mode for #occtl_prim_make_pipe_shell.
 *
 * Selects the rotation rule applied to the profile as it is swept along the
 * spine.
 */
#define OCCTL_PRIM_ARC_3PT_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_arc_3pt.
 *
 * Builds a single-edge wire on the arc of the unique circle passing through
 * @c start, @c via, and @c end. @c via must lie strictly between the other
 * two on the arc.
 */
typedef struct occtl_prim_arc_3pt_info
{
  uint32_t       struct_version; /**< Must be #OCCTL_PRIM_ARC_3PT_INFO_VERSION_1. */
  const void*    p_next;         /**< Reserved; must be NULL. */
  occtl_point3_t start;          /**< Start of the arc. */
  occtl_point3_t via;            /**< Intermediate point on the arc. */
  occtl_point3_t end;            /**< End of the arc. */
} occtl_prim_arc_3pt_info_t;

#define OCCTL_PRIM_ARC_3PT_INFO_INIT                                                               \
  {OCCTL_PRIM_ARC_3PT_INFO_VERSION_1, NULL, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}

/**
 * Runtime initialiser for #occtl_prim_arc_3pt_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_ARC_3PT_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_arc_3pt
 */
OCCTL_API void OCCTL_CALL occtl_prim_arc_3pt_info_init(occtl_prim_arc_3pt_info_t* info);

/**
 * Builds a single-edge wire for the circular arc through three points.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  The three points are collinear / coincident.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_arc_3pt_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_arc_3pt(occtl_graph_t*                   graph,
                                                            const occtl_prim_arc_3pt_info_t* info,
                                                            occtl_node_id_t* out_wire);

#define OCCTL_PRIM_ARC_CENTER_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_arc_center.
 *
 * Builds a single-edge wire on a circular arc defined by its supporting
 * circle (@c placement + @c radius) and the @c start_angle / @c end_angle
 * sweep (radians). @c end_angle > @c start_angle; the sweep direction
 * follows the placement's Z axis (right-hand rule).
 */
typedef struct occtl_prim_arc_center_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_ARC_CENTER_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Centre and orientation; defaults to XOY. */
  double                  radius;         /**< Strictly positive. */
  double                  start_angle;    /**< Sweep start in radians. */
  double                  end_angle;      /**< Sweep end in radians; @c > start_angle. */
} occtl_prim_arc_center_info_t;

#define OCCTL_PRIM_ARC_CENTER_INFO_INIT                                                            \
  {OCCTL_PRIM_ARC_CENTER_INFO_VERSION_1,                                                           \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_arc_center_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_ARC_CENTER_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_arc_center
 */
OCCTL_API void OCCTL_CALL occtl_prim_arc_center_info_init(occtl_prim_arc_center_info_t* info);

/**
 * Builds a single-edge wire for a circular arc swept around a placement's Z axis.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c radius is non-positive or @c end_angle <= start_angle.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_arc_center_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_arc_center(occtl_graph_t*                      graph,
                             const occtl_prim_arc_center_info_t* info,
                             occtl_node_id_t*                    out_wire);

#define OCCTL_PRIM_SPLINE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_spline.
 *
 * Builds a one-edge Wire on a B-spline curve approximating the supplied
 * @c points.
 */
typedef struct occtl_prim_spline_info
{
  uint32_t              struct_version; /**< Must be #OCCTL_PRIM_SPLINE_INFO_VERSION_1. */
  const void*           p_next;         /**< Reserved; must be NULL. */
  const occtl_point3_t* points;         /**< Borrows it. Points to interpolate; @c >= 2. */
  size_t                point_count;    /**< Length of @c points. */
  int32_t               degree_min;     /**< Minimum spline degree (default 3). */
  int32_t               degree_max;     /**< Maximum spline degree (default 8); @c >= degree_min. */
  double                tolerance; /**< Maximum deviation from the points (default @c 1.0e-3). */
} occtl_prim_spline_info_t;

#define OCCTL_PRIM_SPLINE_INFO_INIT {OCCTL_PRIM_SPLINE_INFO_VERSION_1, NULL, NULL, 0, 3, 8, 1.0e-3}

/**
 * Runtime initialiser for #occtl_prim_spline_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_SPLINE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_spline
 */
OCCTL_API void OCCTL_CALL occtl_prim_spline_info_init(occtl_prim_spline_info_t* info);

/**
 * Builds a B-spline Wire approximating a sequence of points.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c point_count < 2, or
 *                                 the degree range is invalid (@c degree_min < 1 or @c degree_max <
 * degree_min).
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed (coincident / collinear points beyond
 * tolerance, etc.).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_spline_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_spline(occtl_graph_t*                  graph,
                                                           const occtl_prim_spline_info_t* info,
                                                           occtl_node_id_t* out_wire);

#define OCCTL_PRIM_PLANE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_plane.
 *
 * Builds a planar Face on the XY plane of @c placement, bounded by a
 * rectangle of @c width by @c height centred on @c placement.location.
 * Equivalent to building a rectangle wire and feeding it to
 * #occtl_prim_make_planar_face, but built directly for slightly less
 * topology churn.
 */
typedef struct occtl_prim_plane_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_PLANE_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Centre + plane; defaults to XOY. */
  double                  width;          /**< Extent along placement X; strictly positive. */
  double                  height;         /**< Extent along placement Y; strictly positive. */
} occtl_prim_plane_info_t;

#define OCCTL_PRIM_PLANE_INFO_INIT                                                                 \
  {OCCTL_PRIM_PLANE_INFO_VERSION_1,                                                                \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_plane_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_PLANE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_plane
 */
OCCTL_API void OCCTL_CALL occtl_prim_plane_info_init(occtl_prim_plane_info_t* info);

/**
 * Builds a rectangular planar Face.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. On success
 *                          receives the new Face NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c width or @c height is non-positive.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_plane_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_plane(occtl_graph_t*                 graph,
                                                          const occtl_prim_plane_info_t* info,
                                                          occtl_node_id_t*               out_face);

#define OCCTL_PRIM_DISK_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_disk.
 *
 * Builds a circular planar Face on the XY plane of @c placement, with the
 * boundary being a single-edge circular wire of @c radius.
 */
typedef struct occtl_prim_disk_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_DISK_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Centre + plane; defaults to XOY. */
  double                  radius;         /**< Strictly positive. */
} occtl_prim_disk_info_t;

#define OCCTL_PRIM_DISK_INFO_INIT                                                                  \
  {OCCTL_PRIM_DISK_INFO_VERSION_1, NULL, {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}}, 0.0}

/**
 * Runtime initialiser for #occtl_prim_disk_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_DISK_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_disk
 */
OCCTL_API void OCCTL_CALL occtl_prim_disk_info_init(occtl_prim_disk_info_t* info);

/**
 * Builds a circular planar Face (disk).
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. On success
 *                          receives the new Face NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c radius is non-positive.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_disk_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_disk(occtl_graph_t*                graph,
                                                         const occtl_prim_disk_info_t* info,
                                                         occtl_node_id_t*              out_face);

#define OCCTL_PRIM_SLOT_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_slot.
 *
 * Builds a closed "stadium" (slot) wire: two straight edges along the X
 * axis of @c placement connected by two semicircular arcs at the ends.
 * The slot is centred on @c placement.location with overall length
 * @c length along X and width @c width along Y. The two end caps each
 * have radius @c width / 2; therefore @c length must be strictly greater
 * than @c width (else there is no straight section).
 */
typedef struct occtl_prim_slot_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_SLOT_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Centre + plane; defaults to XOY. */
  double                  length;         /**< Overall length along X; strictly @c > width. */
  double                  width;          /**< Width along Y; strictly positive. */
} occtl_prim_slot_info_t;

#define OCCTL_PRIM_SLOT_INFO_INIT                                                                  \
  {OCCTL_PRIM_SLOT_INFO_VERSION_1,                                                                 \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_slot_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_SLOT_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_slot
 */
OCCTL_API void OCCTL_CALL occtl_prim_slot_info_init(occtl_prim_slot_info_t* info);

/**
 * Builds a closed slot (stadium) Wire.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c width is non-positive or @c length <= @c width.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_slot_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_slot(occtl_graph_t*                graph,
                                                         const occtl_prim_slot_info_t* info,
                                                         occtl_node_id_t*              out_wire);

#define OCCTL_PRIM_TUBE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_tube.
 *
 * Builds a Solid hollow cylinder with outer radius @c outer_radius and
 * inner radius @c inner_radius, centred on @c placement.location and
 * extruded along @c placement's Z axis by @c height. Internally a
 * rectangular cross-section is revolved a full turn around the placement
 * Z axis. Both annular caps are present in the resulting Solid.
 */
typedef struct occtl_prim_tube_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_TUBE_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Local frame; axis = Z. */
  double                  outer_radius;   /**< Strictly @c > inner_radius > 0. */
  double                  inner_radius;   /**< Strictly @c > 0. */
  double                  height;         /**< Height along placement Z; strictly positive. */
} occtl_prim_tube_info_t;

#define OCCTL_PRIM_TUBE_INFO_INIT                                                                  \
  {OCCTL_PRIM_TUBE_INFO_VERSION_1,                                                                 \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_tube_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_TUBE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_tube
 */
OCCTL_API void OCCTL_CALL occtl_prim_tube_info_init(occtl_prim_tube_info_t* info);

/**
 * Builds a hollow-cylinder (tube) Solid.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_solid Borrows it. Must be non-NULL. On success
 *                          receives the new Solid NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c inner_radius / @c outer_radius / @c height are degenerate
 *                                 (non-positive, or @c outer_radius <= @c inner_radius).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_cylinder, occtl_prim_make_thick_solid
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_tube(occtl_graph_t*                graph,
                                                         const occtl_prim_tube_info_t* info,
                                                         occtl_node_id_t*              out_solid);

#define OCCTL_PRIM_HELIX_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_helix.
 *
 * Builds a Wire on a helical edge of @c radius, @c pitch (height gained
 * per full turn), and total @c height. The helix winds around the Z axis
 * of @c placement, centred at @c placement.location. Right-handedness is
 * the default (counter-clockwise rotation as the helix advances along +Z).
 */
typedef struct occtl_prim_helix_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_HELIX_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Local frame; helix axis = Z. */
  double                  radius;         /**< Strictly positive. */
  double                  pitch;          /**< Height per turn; strictly positive. */
  double                  height;         /**< Total helix height along Z; strictly positive. */
  int32_t left_handed; /**< 0 (default) = right-handed. 1 = left-handed (CW from +Z). */
} occtl_prim_helix_info_t;

#define OCCTL_PRIM_HELIX_INFO_INIT                                                                 \
  {OCCTL_PRIM_HELIX_INFO_VERSION_1,                                                                \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0}

/**
 * Runtime initialiser for #occtl_prim_helix_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_HELIX_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_helix
 */
OCCTL_API void OCCTL_CALL occtl_prim_helix_info_init(occtl_prim_helix_info_t* info);

/**
 * Builds a helical Wire (one edge).
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_wire Borrows it. Must be non-NULL. On success
 *                          receives the new Wire NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Any of @c radius / @c pitch / @c height is non-positive.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_helix_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_helix(occtl_graph_t*                 graph,
                                                          const occtl_prim_helix_info_t* info,
                                                          occtl_node_id_t*               out_wire);

#define OCCTL_PRIM_FILLET_2D_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_fillet_2d.
 *
 * Rounds corners of @c face to @c radius. If @c vertices is NULL or
 * @c vertex_count is 0, every Vertex of @c face is filleted; otherwise
 * only the listed Vertex nodes.
 */
typedef struct occtl_prim_fillet_2d_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_FILLET_2D_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t face;           /**< Borrows it. Planar Face to fillet. */
  const occtl_node_id_t*
         vertices;     /**< Borrows it. Optional Vertex selection; NULL = fillet all corners. */
  size_t vertex_count; /**< Length of @c vertices. */
  double radius;       /**< Fillet radius; strictly positive. */
} occtl_prim_fillet_2d_info_t;

#define OCCTL_PRIM_FILLET_2D_INFO_INIT                                                             \
  {OCCTL_PRIM_FILLET_2D_INFO_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, NULL, 0, 0.0}

/**
 * Runtime initialiser for #occtl_prim_fillet_2d_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_FILLET_2D_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_fillet_2d
 */
OCCTL_API void OCCTL_CALL occtl_prim_fillet_2d_info_init(occtl_prim_fillet_2d_info_t* info);

/**
 * Fillets the corners of a planar Face with a single uniform @c radius.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. On success
 *                          receives the new Face NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, or @c vertices is NULL while
 *                                 @c vertex_count is greater than 0.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c face refers to a removed / absent node, or a vertex does.
 * @retval OCCTL_WRONG_KIND        @c face is not of kind #OCCTL_KIND_FACE, or a listed node is not
 * a Vertex.
 * @retval OCCTL_GEOMETRY_INVALID  @c radius is non-positive, or construction failed (e.g. radius
 * too large for corner).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_fillet_2d_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_fillet_2d(occtl_graph_t*                     graph,
                            const occtl_prim_fillet_2d_info_t* info,
                            occtl_node_id_t*                   out_face);

#define OCCTL_PRIM_FULL_ROUND_2D_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_full_round_2d.
 *
 * Replaces an end Edge of a planar Face with the largest same-radius pair of
 * OCCT 2D fillets that can be applied at the Edge's two vertices.  When
 * @c radius is positive, that exact radius is used.  When @c radius is 0,
 * OCCT-Light searches for the largest radius up to half of the selected Edge
 * length by repeatedly asking OCCT's 2D fillet builder to construct the result.
 */
typedef struct occtl_prim_full_round_2d_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_FULL_ROUND_2D_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t face;           /**< Borrows it. Planar Face to modify. */
  occtl_node_id_t edge;           /**< Borrows it. Edge whose endpoints define the full round. */
  double          radius; /**< 0 = auto maximum; otherwise strictly positive fillet radius. */
  uint32_t search_steps;  /**< Auto-radius binary-search steps; 0 uses implementation default. */
} occtl_prim_full_round_2d_info_t;

#define OCCTL_PRIM_FULL_ROUND_2D_INFO_INIT                                                         \
  {OCCTL_PRIM_FULL_ROUND_2D_INFO_VERSION_1,                                                        \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   0.0,                                                                                            \
   32u}

/**
 * Runtime initialiser for #occtl_prim_full_round_2d_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_FULL_ROUND_2D_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_full_round_2d
 */
OCCTL_API void OCCTL_CALL occtl_prim_full_round_2d_info_init(occtl_prim_full_round_2d_info_t* info);

/**
 * Builds a planar Face with a selected Edge replaced by a full-round arc.
 *
 * The topology construction delegates to OCCT @c BRepFilletAPI_MakeFillet2d.
 * The result is inserted into @p graph as a new bare Face root; the input Face
 * is left unchanged.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     info     Borrows it. Must be non-NULL with a recognised
 *                         @c struct_version.
 * @param[out]    out_face Borrows it. Must be non-NULL. On success receives
 *                         the new Face NodeId; on failure set to
 *                         #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 @c radius is negative/non-finite, or
 *                                 @c search_steps is too large.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c face or @c edge refers to a removed /
 *                                 absent node.
 * @retval OCCTL_WRONG_KIND        @c face is not a Face, or @c edge is not an
 *                                 Edge.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT cannot construct the full round.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_full_round_2d_info_init, occtl_prim_make_fillet_2d
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_full_round_2d(occtl_graph_t*                         graph,
                                const occtl_prim_full_round_2d_info_t* info,
                                occtl_node_id_t*                       out_face);

#ifdef __cplusplus
}
#endif

#endif /* OCCTL_PRIM_SKETCH_H */
