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
 * @file occtl_topo_build.h
 * @brief OCCT-Light: Topology builders, mutations, and property setters.
 *
 * Covers low-level element construction (make_vertex, make_edge, make_wire,
 * make_face, make_shell, make_solid, make_compound, make_compsolid),
 * wire/edge operations (edges_to_wires, wire_offset_2d, curves_to_wire,
 * wire_fix_degenerate, face_chamfer_2d, wire_chamfer_2d), in-place
 * mutations (remove, replace, add/remove from parent, edge_split), and
 * property setters.
 */

#ifndef OCCTL_TOPO_BUILD_H
#define OCCTL_TOPO_BUILD_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_curves.h"
#include "occtl_geom.h"
#include "occtl_surfaces.h"
#include "occtl_topo_relation.h"
#include "occtl_topo_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_TOPO_MAKE_VERTEX_INFO_VERSION_1 1u

/**
 * Info for #occtl_topo_make_vertex.
 */
typedef struct occtl_topo_make_vertex_info
{
  uint32_t       struct_version; /**< Must be #OCCTL_TOPO_MAKE_VERTEX_INFO_VERSION_1. */
  const void*    p_next;         /**< Reserved; set to NULL. */
  occtl_point3_t point;          /**< Point position. */
  double         tolerance;      /**< Vertex tolerance. */
} occtl_topo_make_vertex_info_t;

#define OCCTL_TOPO_MAKE_VERTEX_INFO_INIT                                                           \
  {OCCTL_TOPO_MAKE_VERTEX_INFO_VERSION_1, NULL, {0.0, 0.0, 0.0}, 0.0}

/**
 * Initialises @p info to default values via #OCCTL_TOPO_MAKE_VERTEX_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_vertex
 */
OCCTL_API void OCCTL_CALL occtl_topo_make_vertex_info_init(occtl_topo_make_vertex_info_t* info);

#define OCCTL_TOPO_MAKE_EDGE_INFO_VERSION_1 1u

/**
 * Info for #occtl_topo_make_edge.
 *
 * @c curve is a rep ID referencing a 3D curve node in the graph.
 * Use #OCCTL_REP_ID_INVALID for a degenerate edge (no 3D curve).
 */
typedef struct occtl_topo_make_edge_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_TOPO_MAKE_EDGE_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t start_vertex;   /**< Start vertex node ID. */
  occtl_node_id_t end_vertex;     /**< End vertex node ID. */
  occtl_rep_id_t
         curve; /**< Rep ID of the 3D curve node. Use #OCCTL_REP_ID_INVALID for degenerate edge. */
  double first; /**< First parameter on curve. */
  double last;  /**< Last parameter on curve. */
  double tolerance; /**< Edge tolerance. */
} occtl_topo_make_edge_info_t;

#define OCCTL_TOPO_MAKE_EDGE_INFO_INIT                                                             \
  {OCCTL_TOPO_MAKE_EDGE_INFO_VERSION_1,                                                            \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_REP_ID_INVALID,                                                                           \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0}

/**
 * Initialises @p info to default values via #OCCTL_TOPO_MAKE_EDGE_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_edge
 */
OCCTL_API void OCCTL_CALL occtl_topo_make_edge_info_init(occtl_topo_make_edge_info_t* info);

#define OCCTL_TOPO_MAKE_WIRE_INFO_VERSION_1 1u

/**
 * Info for #occtl_topo_make_wire.
 *
 * @c edges is a borrowed span of (edge, orientation) pairs; the
 * graph copies during construction.  NULL is valid only when
 * @c edge_count is 0.
 */
typedef struct occtl_topo_make_wire_info
{
  uint32_t                     struct_version; /**< Must be #OCCTL_TOPO_MAKE_WIRE_INFO_VERSION_1. */
  const void*                  p_next;         /**< Reserved; set to NULL. */
  const occtl_oriented_node_t* edges; /**< Ordered span of (edge, orientation) pairs.  Borrowed. */
  size_t                       edge_count; /**< Number of entries in @c edges. */
} occtl_topo_make_wire_info_t;

#define OCCTL_TOPO_MAKE_WIRE_INFO_INIT {OCCTL_TOPO_MAKE_WIRE_INFO_VERSION_1, NULL, NULL, 0}

/**
 * Initialises @p info to default values via #OCCTL_TOPO_MAKE_WIRE_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_wire
 */
OCCTL_API void OCCTL_CALL occtl_topo_make_wire_info_init(occtl_topo_make_wire_info_t* info);

#define OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_topo_edges_to_wires.
 *
 * @c edges is a borrowed span of Edge node IDs.  The graph copies only the
 * resulting Wire topology; the input span need not remain valid after the
 * call returns.
 */
typedef struct occtl_topo_edges_to_wires_options
{
  uint32_t    struct_version;        /**< Must be #OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_VERSION_1. */
  const void* p_next;                /**< Reserved; set to NULL. */
  const occtl_node_id_t* edges;      /**< Unordered Edge node IDs.  Borrowed. */
  size_t                 edge_count; /**< Number of entries in @c edges. */
  double  tolerance;  /**< Endpoint matching tolerance; 0 uses shared vertex IDs only. */
  int32_t allow_open; /**< 0/1; when 0, reject non-closed output wires. */
} occtl_topo_edges_to_wires_options_t;

#define OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_INIT                                                     \
  {OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_VERSION_1, NULL, NULL, 0, 0.0, 1}

/**
 * Initialises @p options to default values via #OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_edges_to_wires
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_edges_to_wires_options_init(occtl_topo_edges_to_wires_options_t* options);

#define OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_VERSION_1 1u

/**
 * Join style for planar wire offsets.
 */
typedef enum occtl_topo_wire_offset_2d_join
{
  OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC     = 0, /**< Insert circular arcs at sharp corners. */
  OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_TANGENT = 1, /**< Use tangent-continuous joins. */
  OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION =
    2, /**< Extend adjacent edges until they intersect. */
  OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_RESERVED_FUTURE = 0x7fffffff
} occtl_topo_wire_offset_2d_join_t;

/**
 * Edge-join style for 3D offset, thicken, and hollow operations.
 *
 * Shared by #occtl_prim_make_offset_shape, #occtl_prim_make_thick_solid,
 * and #occtl_topo_offset_features.
 */
typedef enum occtl_offset_join_type
{
  OCCTL_OFFSET_JOIN_ARC             = 0, /**< Insert rolling arcs across sharp edges. */
  OCCTL_OFFSET_JOIN_TANGENT         = 1, /**< Insert tangent-continuous patches. */
  OCCTL_OFFSET_JOIN_INTERSECTION    = 2, /**< Extend adjacent faces until they intersect. */
  OCCTL_OFFSET_JOIN_RESERVED_FUTURE = 0x7fffffff
} occtl_offset_join_type_t;

/**
 * Options for #occtl_topo_wire_offset_2d.
 *
 * @c distance is signed; its side follows OCCT's planar-wire offset
 * convention for the wire orientation.  The result is inserted into the
 * same graph as a new topology root.
 */
typedef struct occtl_topo_wire_offset_2d_options
{
  uint32_t        struct_version; /**< Must be #OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t wire;           /**< Source Wire node ID. */
  double          distance;       /**< Signed planar offset distance. */
  occtl_topo_wire_offset_2d_join_t join;        /**< Corner join style. */
  int32_t                          open_result; /**< 0/1; request an open offset result. */
  int32_t approximate; /**< 0/1; approximate input contours by arcs/segments. */
} occtl_topo_wire_offset_2d_options_t;

#define OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_INIT                                                     \
  {OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_VERSION_1,                                                    \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   1.0,                                                                                            \
   OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC,                                                             \
   0,                                                                                              \
   0}

/**
 * Initialises @p options to default values via #OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_wire_offset_2d
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_wire_offset_2d_options_init(occtl_topo_wire_offset_2d_options_t* options);

#define OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_topo_wire_fix_degenerate.
 *
 * The operation detaches coedge usages whose underlying Edge has OCCT length
 * less than or equal to @c min_length.  The Edge and Vertex definitions remain
 * in the graph for other users.
 */
typedef struct occtl_topo_wire_fix_degenerate_edges_options
{
  uint32_t struct_version; /**< Must be #OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_VERSION_1. */
  const void*     p_next;  /**< Reserved; set to NULL. */
  occtl_node_id_t wire;    /**< Wire node ID to repair in place. */
  double          min_length; /**< Remove edge usages with length <= this value. */
} occtl_topo_wire_fix_degenerate_edges_options_t;

#define OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_INIT                                          \
  {OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, 1.0e-9}

/**
 * Initialises @p options to default values via
 * #OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_wire_fix_degenerate
 */
OCCTL_API void OCCTL_CALL occtl_topo_wire_fix_degenerate_options_init(
  occtl_topo_wire_fix_degenerate_edges_options_t* options);

#define OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_topo_face_chamfer_2d.
 *
 * @c vertices is a borrowed optional span of Vertex node IDs. When @c vertices
 * is NULL and @c vertex_count is 0, OCCT attempts to chamfer every vertex of
 * @c face. The result is inserted into the same graph as a new Face node; the
 * source face is not modified.
 */
typedef struct occtl_topo_face_chamfer_2d_options
{
  uint32_t        struct_version;  /**< Must be #OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_VERSION_1. */
  const void*     p_next;          /**< Reserved; set to NULL. */
  occtl_node_id_t face;            /**< Source planar Face node ID. */
  const occtl_node_id_t* vertices; /**< Optional Vertex node IDs.  Borrowed. */
  size_t                 vertex_count; /**< Number of entries in @c vertices. */
  double                 distance1;    /**< Distance from corner along the first adjacent edge. */
  double                 distance2;    /**< Distance from corner along the second adjacent edge. */
} occtl_topo_face_chamfer_2d_options_t;

#define OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_INIT                                                    \
  {OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, NULL, 0, 1.0, 1.0}

#define OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_topo_wire_chamfer_2d.
 *
 * The source Wire must be planar.  The resulting outer Wire is inserted into
 * the same graph as a new Wire node; the source wire is not modified.
 */
typedef struct occtl_topo_wire_chamfer_2d_options
{
  uint32_t        struct_version;  /**< Must be #OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_VERSION_1. */
  const void*     p_next;          /**< Reserved; set to NULL. */
  occtl_node_id_t wire;            /**< Source planar Wire node ID. */
  const occtl_node_id_t* vertices; /**< Optional Vertex node IDs. Borrowed. */
  size_t                 vertex_count; /**< Number of entries in @c vertices. */
  double                 distance1;    /**< Distance from corner along the first adjacent edge. */
  double                 distance2;    /**< Distance from corner along the second adjacent edge. */
} occtl_topo_wire_chamfer_2d_options_t;

#define OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_INIT                                                    \
  {OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, NULL, 0, 1.0, 1.0}

/**
 * Initialises @p options to default values via #OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_face_chamfer_2d
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_face_chamfer_2d_options_init(occtl_topo_face_chamfer_2d_options_t* options);

/**
 * Initialises @p options to default values via #OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_wire_chamfer_2d
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_wire_chamfer_2d_options_init(occtl_topo_wire_chamfer_2d_options_t* options);

#define OCCTL_TOPO_MAKE_FACE_INFO_VERSION_1 1u

/**
 * Info for #occtl_topo_make_face.
 *
 * @c surface is a rep ID referencing a surface node in the graph.
 * Must be valid (not #OCCTL_REP_ID_INVALID).
 * @c inner_wires is a borrowed span of wire NodeIds; NULL when
 * @c inner_wire_count is 0.
 */
typedef struct occtl_topo_make_face_info
{
  uint32_t    struct_version; /**< Must be #OCCTL_TOPO_MAKE_FACE_INFO_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  occtl_rep_id_t
    surface; /**< Rep ID of the surface node. Must be valid (not #OCCTL_REP_ID_INVALID). */
  occtl_node_id_t        outer_wire;       /**< Outer wire node ID. */
  const occtl_node_id_t* inner_wires;      /**< Optional span of inner wire NodeIds.  Borrowed. */
  size_t                 inner_wire_count; /**< Number of entries in @c inner_wires. */
  double                 tolerance;        /**< Face tolerance. */
} occtl_topo_make_face_info_t;

#define OCCTL_TOPO_MAKE_FACE_INFO_INIT                                                             \
  {OCCTL_TOPO_MAKE_FACE_INFO_VERSION_1,                                                            \
   NULL,                                                                                           \
   OCCTL_REP_ID_INVALID,                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   NULL,                                                                                           \
   0,                                                                                              \
   0.0}

/**
 * Initialises @p info to default values via #OCCTL_TOPO_MAKE_FACE_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_face
 */
OCCTL_API void OCCTL_CALL occtl_topo_make_face_info_init(occtl_topo_make_face_info_t* info);

#define OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_topo_make_face_from_wires_auto.
 *
 * @c surface is a rep ID referencing a surface node in the graph.
 * Must be valid (not #OCCTL_REP_ID_INVALID).
 * @c wires is a borrowed span of Wire node IDs.
 */
typedef struct occtl_topo_make_face_from_wires_auto_options
{
  uint32_t struct_version; /**< Must be #OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_VERSION_1. */
  const void* p_next;      /**< Reserved; set to NULL. */
  occtl_rep_id_t
    surface; /**< Rep ID of the surface node. Must be valid (not #OCCTL_REP_ID_INVALID). */
  const occtl_node_id_t* wires;      /**< Candidate boundary wires.  Borrowed. */
  size_t                 wire_count; /**< Number of entries in @c wires. */
  double                 tolerance;  /**< Face tolerance and OCCT degenerated-edge tolerance. */
  double area_tolerance;             /**< Ambiguity tolerance when ranking candidate outer wires. */
} occtl_topo_make_face_from_wires_auto_options_t;

#define OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_INIT                                          \
  {OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_VERSION_1,                                         \
   NULL,                                                                                           \
   OCCTL_REP_ID_INVALID,                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   0.0,                                                                                            \
   1.0e-9}

/**
 * Initialises @p options to default values via
 * #OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_face_from_wires_auto
 */
OCCTL_API void OCCTL_CALL occtl_topo_make_face_from_wires_auto_options_init(
  occtl_topo_make_face_from_wires_auto_options_t* options);

#define OCCTL_TOPO_MAKE_SHELL_INFO_VERSION_1 1u

/**
 * Info for #occtl_topo_make_shell.
 */
typedef struct occtl_topo_make_shell_info
{
  uint32_t    struct_version;         /**< Must be #OCCTL_TOPO_MAKE_SHELL_INFO_VERSION_1. */
  const void* p_next;                 /**< Reserved; set to NULL. */
  const occtl_oriented_node_t* faces; /**< Ordered span of (face, orientation) pairs.  Borrowed. */
  size_t                       face_count; /**< Number of entries in @c faces. */
  int32_t                      is_closed;  /**< Set to 1 if the shell is watertight. */
} occtl_topo_make_shell_info_t;

#define OCCTL_TOPO_MAKE_SHELL_INFO_INIT {OCCTL_TOPO_MAKE_SHELL_INFO_VERSION_1, NULL, NULL, 0, 0}

/**
 * Initialises @p info to default values via #OCCTL_TOPO_MAKE_SHELL_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_shell
 */
OCCTL_API void OCCTL_CALL occtl_topo_make_shell_info_init(occtl_topo_make_shell_info_t* info);

#define OCCTL_TOPO_MAKE_SOLID_INFO_VERSION_1 1u

/**
 * Info for #occtl_topo_make_solid.
 */
typedef struct occtl_topo_make_solid_info
{
  uint32_t    struct_version; /**< Must be #OCCTL_TOPO_MAKE_SOLID_INFO_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  const occtl_oriented_node_t*
         shells;      /**< Ordered span of (shell, orientation) pairs.  Borrowed. */
  size_t shell_count; /**< Number of entries in @c shells. */
} occtl_topo_make_solid_info_t;

#define OCCTL_TOPO_MAKE_SOLID_INFO_INIT {OCCTL_TOPO_MAKE_SOLID_INFO_VERSION_1, NULL, NULL, 0}

/**
 * Initialises @p info to default values via #OCCTL_TOPO_MAKE_SOLID_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_solid
 */
OCCTL_API void OCCTL_CALL occtl_topo_make_solid_info_init(occtl_topo_make_solid_info_t* info);

#define OCCTL_TOPO_MAKE_COMPOUND_INFO_VERSION_1 1u

/**
 * Info for #occtl_topo_make_compound.
 */
typedef struct occtl_topo_make_compound_info
{
  uint32_t    struct_version; /**< Must be #OCCTL_TOPO_MAKE_COMPOUND_INFO_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  const occtl_oriented_node_t*
         children;    /**< Ordered span of (child, orientation) pairs.  Borrowed. */
  size_t child_count; /**< Number of entries in @c children. */
} occtl_topo_make_compound_info_t;

#define OCCTL_TOPO_MAKE_COMPOUND_INFO_INIT {OCCTL_TOPO_MAKE_COMPOUND_INFO_VERSION_1, NULL, NULL, 0}

/**
 * Initialises @p info to default values via #OCCTL_TOPO_MAKE_COMPOUND_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_compound
 */
OCCTL_API void OCCTL_CALL occtl_topo_make_compound_info_init(occtl_topo_make_compound_info_t* info);

/**
 * Creates a vertex in the graph.
 *
 * @param[in,out] graph      Borrows it.  Must be non-NULL.
 * @param[in]     info       Borrows it.  Must be non-NULL.
 * @param[out]    out_vertex Borrows it.  Must be non-NULL.  Receives
 *                           the new vertex NodeId on success.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, or @p out_vertex is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version is unsupported.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_vertex(occtl_graph_t*                       graph,
                         const occtl_topo_make_vertex_info_t* info,
                         occtl_node_id_t*                     out_vertex);

/**
 * Creates an edge in the graph.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     info     Borrows it.  Must be non-NULL.
 *                         @c start_vertex and @c end_vertex must be valid,
 *                         active vertex NodeIds.  @c curve may be NULL
 *                         for a degenerate edge.
 * @param[out]    out_edge Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, or @p out_edge is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         A vertex NodeId is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A vertex NodeId is not a vertex.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_make_edge(occtl_graph_t*                     graph,
                                                         const occtl_topo_make_edge_info_t* info,
                                                         occtl_node_id_t* out_edge);

/**
 * Creates a wire in the graph from an ordered span of (edge,
 * orientation) pairs.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     info     Borrows it.  Must be non-NULL.  The edges span
 *                         is borrowed; it need not remain valid after the
 *                         call returns.
 * @param[out]    out_wire Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, or @p out_wire is NULL;
 *                                 or @c edges is NULL when @c edge_count > 0;
 *                                 or @c edges is non-NULL when @c edge_count == 0.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         A child NodeId is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A child NodeId does not have the expected kind.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_make_wire(occtl_graph_t*                     graph,
                                                         const occtl_topo_make_wire_info_t* info,
                                                         occtl_node_id_t* out_wire);

/**
 * Connects unordered Edge nodes into one or more Wire nodes.
 *
 * On a sizing call, pass @p out_wires as NULL with @p cap = 0; the function
 * writes the number of wires that would be created into @p out_count without
 * mutating @p graph.  Reissue with a buffer of at least that many
 * #occtl_node_id_t entries to create the wires and receive their IDs.
 *
 * Endpoint matching prefers shared BRepGraph vertex IDs and falls back to
 * endpoint point distance when @c options->tolerance > 0.  Created wires
 * reference the original Edge nodes; no Edge or Vertex nodes are duplicated.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     options   Borrows it.  Must be non-NULL.
 * @param[out]    out_wires Borrows it.  May be NULL on the sizing call.
 * @param[in]     cap       Capacity of @p out_wires in entries.
 * @param[out]    out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_count is
 *                                 NULL; an options field is malformed; or
 *                                 open wires are produced while
 *                                 @c options->allow_open is 0.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         An Edge node ID is invalid or removed.
 * @retval OCCTL_WRONG_KIND        An input node is not an Edge.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_wires is non-NULL and @p cap is too small.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph when @p out_wires is non-NULL and large enough).
 *
 * @sa occtl_topo_make_wire, occtl_topo_wire_order_edges
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edges_to_wires(occtl_graph_t*                             graph,
                            const occtl_topo_edges_to_wires_options_t* options,
                            occtl_node_id_t*                           out_wires,
                            size_t                                     cap,
                            size_t*                                    out_count);

/**
 * Creates a planar 2D offset of a Wire node and inserts it into the graph.
 *
 * The source wire must be planar.  The resulting offset Wire is inserted into
 * @p graph as a new Wire node.  The source wire is not modified.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     options  Borrows it.  Must be non-NULL.
 * @param[out]    out_wire Borrows it.  Must be non-NULL.  Receives the new
 *                         Wire node ID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_wire is
 *                                 NULL; an options field is malformed; or
 *                                 @c distance is zero / non-finite.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         @c options->wire is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @c options->wire is not a Wire node.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not create a valid offset wire.
 * @retval OCCTL_TOPOLOGY_INVALID  The offset result could not be ingested as
 *                                 a graph Wire.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_edges_to_wires, occtl_topo_make_face_from_wires_auto
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wire_offset_2d(occtl_graph_t*                             graph,
                            const occtl_topo_wire_offset_2d_options_t* options,
                            occtl_node_id_t*                           out_wire);

/**
 * Creates a wire from an ordered array of curve rep IDs.
 *
 * For each curve the function evaluates the start and end point, creates
 * vertices (sharing vertices between consecutive curves whose endpoints
 * match within tolerance), creates a single-edge wire per curve, then
 * joins all edges into one wire.
 *
 * A single closed curve (e.g. a circle) produces one vertex used as both
 * start and end of a single edge.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     curve_ids Borrows it.  Array of @p count curve rep IDs.
 *                          Must be non-NULL when @p count > 0.
 * @param[in]     count     Number of curves.  Must be >= 1.
 * @param[out]    out_wire  Owns it.  Receives the wire node ID.
 *                          Must be non-NULL.
 *
 * @retval OCCTL_OK                  On success.
 * @retval OCCTL_INVALID_ARGUMENT    Any required pointer is NULL,
 *                                   @p count == 0, or a curve rep ID
 *                                   in the array is invalid.
 * @retval OCCTL_NOT_FOUND           A curve evaluation failed.
 * @retval OCCTL_GEOMETRY_INVALID    Could not create geometry.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_edges_to_wires, occtl_topo_make_edge, occtl_curve_eval_d0
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_curves_to_wire(occtl_graph_t*        graph,
                                                              const occtl_rep_id_t* curve_ids,
                                                              size_t                count,
                                                              occtl_node_id_t*      out_wire);

/**
 * Removes degenerate edge usages from a Wire in place.
 *
 * The function scans the Wire's coedge usages, measures each underlying Edge
 * with OCCT linear properties when possible, and detaches every usage whose
 * length is less than or equal to @c options->min_length.  The underlying Edge
 * definitions are not deleted; only their usage in @c options->wire is
 * removed.  This makes the operation safe for shared graph topology.
 *
 * @param[in,out] graph       Borrows it.  Must be non-NULL.
 * @param[in]     options     Borrows it.  Must be non-NULL.
 * @param[out]    out_removed Borrows it.  Must be non-NULL.  Receives the
 *                            number of detached coedge usages.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_removed is
 *                                 NULL; @c p_next is non-NULL; or
 *                                 @c min_length is negative / non-finite.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         @c options->wire is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @c options->wire is not a Wire node.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_wire_fix_degenerate_options_init, occtl_topo_wire_order_edges
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wire_fix_degenerate(occtl_graph_t*                                        graph,
                                 const occtl_topo_wire_fix_degenerate_edges_options_t* options,
                                 size_t*                                               out_removed);

/**
 * Chamfers corners of a planar Face and inserts the result into the graph.
 *
 * The source face must be planar. The chamfered Face is inserted into @p graph
 * as a new Face node. The source face is not modified.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     options  Borrows it.  Must be non-NULL.
 * @param[out]    out_face Borrows it.  Must be non-NULL.  Receives the new
 *                         Face node ID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_face is
 *                                 NULL; an options field is malformed; or
 *                                 a distance is non-positive / non-finite.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         @c options->face is invalid or removed; or a
 *                                 selected vertex is invalid / removed.
 * @retval OCCTL_WRONG_KIND        @c options->face is not a Face node, or a
 *                                 selected node is not a Vertex.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not create a valid chamfered face.
 * @retval OCCTL_TOPOLOGY_INVALID  The chamfer result could not be ingested as
 *                                 a graph Face.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_face_chamfer_2d_options_init, occtl_topo_wire_offset_2d
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_chamfer_2d(occtl_graph_t*                              graph,
                             const occtl_topo_face_chamfer_2d_options_t* options,
                             occtl_node_id_t*                            out_face);

/**
 * Chamfers corners of a planar Wire and inserts the result into the graph.
 *
 * The source wire must be planar.  The resulting outer Wire is inserted into
 * @p graph as a new Wire node.  The source wire is not modified.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     options  Borrows it. Must be non-NULL.
 * @param[out]    out_wire Borrows it. Must be non-NULL. Receives the new
 *                         Wire node ID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_wire is
 *                                 NULL; an options field is malformed; or
 *                                 a distance is non-positive / non-finite.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         @c options->wire is invalid or removed; or a
 *                                 selected vertex is invalid / removed.
 * @retval OCCTL_WRONG_KIND        @c options->wire is not a Wire node, or a
 *                                 selected node is not a Vertex.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not create a valid chamfered wire.
 * @retval OCCTL_TOPOLOGY_INVALID  The chamfer result could not be ingested as
 *                                 a graph Wire.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_wire_chamfer_2d_options_init, occtl_topo_face_chamfer_2d
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wire_chamfer_2d(occtl_graph_t*                              graph,
                             const occtl_topo_wire_chamfer_2d_options_t* options,
                             occtl_node_id_t*                            out_wire);

/**
 * Creates a face in the graph from a surface, outer wire, and
 * optional inner wires.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     info     Borrows it.  Must be non-NULL.  @c surface
 *                         must be non-NULL.  The inner-wires span is
 *                         borrowed; it need not remain valid after the
 *                         call returns.
 * @param[out]    out_face Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, @p out_face, or
 *                                 @c info->surface is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         A wire NodeId is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A wire NodeId is not a wire.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_make_face(occtl_graph_t*                     graph,
                                                         const occtl_topo_make_face_info_t* info,
                                                         occtl_node_id_t* out_face);

/**
 * Creates a face by classifying candidate wires into outer and inner loops.
 *
 * The function classifies the candidate Wires by planar enclosed area and uses
 * the largest unambiguous loop as the outer wire.
 * All other input wires become inner wires in their input order.  The stored
 * graph Face references the original Wire nodes; no boundary topology is
 * duplicated.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     options  Borrows it.  Must be non-NULL.
 * @param[out]    out_face Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, @p out_face,
 *                                 @c options->surface, or @c options->wires is
 *                                 NULL; @c wire_count is 0; an option value is
 *                                 malformed; or the outer wire is ambiguous.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         A Wire node ID is invalid or removed.
 * @retval OCCTL_WRONG_KIND        An input node is not a Wire.
 * @retval OCCTL_ERROR             The candidate wires could not be classified.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_make_face, occtl_topo_edges_to_wires
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_make_face_from_wires_auto(
  occtl_graph_t*                                        graph,
  const occtl_topo_make_face_from_wires_auto_options_t* options,
  occtl_node_id_t*                                      out_face);

/**
 * Creates a shell in the graph from a span of (face, orientation) pairs.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     info      Borrows it.  Must be non-NULL.  The faces span
 *                          is borrowed; it need not remain valid after the
 *                          call returns.
 * @param[out]    out_shell Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, or @p out_shell is NULL;
 *                                 or @c faces is NULL when @c face_count > 0;
 *                                 or @c faces is non-NULL when @c face_count == 0.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         A child NodeId is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A child NodeId does not have the expected kind.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_make_shell_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_make_shell(occtl_graph_t*                      graph,
                                                          const occtl_topo_make_shell_info_t* info,
                                                          occtl_node_id_t* out_shell);

/**
 * Creates a solid in the graph from a span of (shell, orientation) pairs.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     info      Borrows it.  Must be non-NULL.  The shells span
 *                          is borrowed; it need not remain valid after the
 *                          call returns.
 * @param[out]    out_solid Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, or @p out_solid is NULL;
 *                                 or @c shells is NULL when @c shell_count > 0;
 *                                 or @c shells is non-NULL when @c shell_count == 0.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         A child NodeId is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A child NodeId does not have the expected kind.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_make_solid_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_make_solid(occtl_graph_t*                      graph,
                                                          const occtl_topo_make_solid_info_t* info,
                                                          occtl_node_id_t* out_solid);

/**
 * Creates a compound in the graph from a span of mixed-kind
 * (child, orientation) pairs.
 *
 * @param[in,out] graph        Borrows it.  Must be non-NULL.
 * @param[in]     info         Borrows it.  Must be non-NULL.  The children
 *                             span is borrowed; it need not remain valid
 *                             after the call returns.
 * @param[out]    out_compound Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, or @p out_compound is NULL;
 *                                 or @c children is NULL when @c child_count > 0;
 *                                 or @c children is non-NULL when @c child_count == 0.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         A child NodeId is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A child NodeId does not have the expected kind.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_make_compound_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_compound(occtl_graph_t*                         graph,
                           const occtl_topo_make_compound_info_t* info,
                           occtl_node_id_t*                       out_compound);

#define OCCTL_TOPO_MAKE_COMPSOLID_INFO_VERSION_1 1u

/**
 * Info for #occtl_topo_make_compsolid.
 */
typedef struct occtl_topo_make_compsolid_info
{
  uint32_t    struct_version; /**< Must be #OCCTL_TOPO_MAKE_COMPSOLID_INFO_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  const occtl_oriented_node_t*
         solids;      /**< Ordered span of (solid, orientation) pairs.  Borrowed. */
  size_t solid_count; /**< Number of entries in @c solids. */
} occtl_topo_make_compsolid_info_t;

#define OCCTL_TOPO_MAKE_COMPSOLID_INFO_INIT                                                        \
  {OCCTL_TOPO_MAKE_COMPSOLID_INFO_VERSION_1, NULL, NULL, 0}

/**
 * Initialises @p info to default values via #OCCTL_TOPO_MAKE_COMPSOLID_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_compsolid
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_make_compsolid_info_init(occtl_topo_make_compsolid_info_t* info);

/**
 * Creates a compsolid in the graph from a span of (solid, orientation) pairs.
 *
 * @param[in,out] graph         Borrows it.  Must be non-NULL.
 * @param[in]     info          Borrows it.  Must be non-NULL.  The solids
 *                              span is borrowed; it need not remain valid
 *                              after the call returns.
 * @param[out]    out_compsolid Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, or @p out_compsolid is NULL;
 *                                 or @c solids is NULL when @c solid_count > 0;
 *                                 or @c solids is non-NULL when @c solid_count == 0.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version unsupported.
 * @retval OCCTL_NOT_FOUND         A child NodeId is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A child NodeId does not have the expected kind.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_make_compsolid_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_compsolid(occtl_graph_t*                          graph,
                            const occtl_topo_make_compsolid_info_t* info,
                            occtl_node_id_t*                        out_compsolid);

/**
 * Removes a node from the graph.
 *
 * The node's children (direct references) are also removed.
 * Removing a node that is referenced by other active nodes is valid:
 * those references become dangling and will resolve to invalid on
 * subsequent refinement.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     id     NodeId of the entity to remove.  All-zero value
 *                       returns #OCCTL_NOT_FOUND.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p first/@p last are
 *                                 non-finite, or @p last is less than @p first.
 * @retval OCCTL_NOT_FOUND         @p id is invalid or already removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_remove_subgraph
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove(occtl_graph_t* graph, occtl_node_id_t id);

/**
 * Recursively removes a node and its entire subgraph.
 *
 * All descendants reachable through reference edges are removed.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     id     NodeId of the root to remove.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p id is invalid or already removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_remove
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove_subgraph(occtl_graph_t*  graph,
                                                               occtl_node_id_t id);

/**
 * Opaque handle for a batched graph mutation scope.
 *
 * Created by #occtl_graph_begin_batch.  Mutations performed on
 * @p graph while a batch is open are deferred; #occtl_batch_commit
 * applies them all at once.  #occtl_batch_abort discards uncommitted
 * changes.
 *
 * Outside a batch, every mutation is its own implicit scope.
 *
 * @sa occtl_graph_begin_batch, occtl_batch_commit, occtl_batch_abort
 */
typedef struct occtl_batch occtl_batch_t;

/**
 * Opens a batched mutation scope on @p graph.
 *
 * Subsequent mutations on @p graph are deferred until
 * #occtl_batch_commit is called on the returned handle.
 * Release with #occtl_batch_commit (apply) or #occtl_batch_abort (discard).
 *
 * @param[in,out] graph      Borrows it.  Must be non-NULL.
 * @param[out]    out_batch  Owns it.  Must be non-NULL.  The caller is
 *                           responsible for calling commit or abort.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_batch is NULL.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph batch state).
 *
 * @sa occtl_batch_commit, occtl_batch_abort
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_begin_batch(occtl_graph_t*  graph,
                                                            occtl_batch_t** out_batch);

/**
 * Commits all deferred mutations in the batch to the graph.
 *
 * After this call @p batch is freed and must not be reused.
 * NULL-tolerant (no-op).
 *
 * @param[in] batch  Owning pointer.  May be NULL.
 *
 * @retval OCCTL_OK  On success.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_graph_begin_batch, occtl_batch_abort
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_batch_commit(occtl_batch_t* batch);

/**
 * Aborts the batch, discarding all deferred mutations.
 *
 * After this call @p batch is freed and must not be reused.
 * NULL-tolerant (no-op).
 *
 * @param[in] batch  Owning pointer.  May be NULL.
 *
 * @retval OCCTL_OK  On success.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_graph_begin_batch, occtl_batch_commit
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_batch_abort(occtl_batch_t* batch);

/**
 * Replaces the 3D curve of an edge.
 *
 * Replaces the underlying Curve3DRep geometry without rebuilding the edge
 * topology.  Callers can later query the new extent through
 * #occtl_topo_edge_range or evaluate via #occtl_topo_edge_eval.
 *
 * Passing @p curve_id == #OCCTL_REP_ID_INVALID clears the curve binding,
 * making the edge degenerate.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     edge     Edge node ID.  Must be a valid, active edge.
 * @param[in]     curve_id Rep ID of the 3D curve node to bind, or
 *                         #OCCTL_REP_ID_INVALID to clear.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_replace_face_surface, occtl_topo_replace_coedge_pcurve,
 *     occtl_topo_make_edge
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_replace_edge_curve(occtl_graph_t*  graph,
                                                                  occtl_node_id_t edge,
                                                                  occtl_rep_id_t  curve_id);

/**
 * Replaces the surface of a face.
 *
 * Replaces the underlying SurfaceRep geometry without rebuilding the face
 * topology.  Passing @p surface_id == #OCCTL_REP_ID_INVALID clears the
 * surface binding.
 *
 * @param[in,out] graph      Borrows it.  Must be non-NULL.
 * @param[in]     face       Face node ID.  Must be a valid, active face.
 * @param[in]     surface_id Rep ID of the surface node to bind, or
 *                           #OCCTL_REP_ID_INVALID to clear.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 * @retval OCCTL_WRONG_KIND        @p face is not a face.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_replace_edge_curve, occtl_topo_replace_coedge_pcurve,
 *     occtl_topo_make_face
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_replace_face_surface(occtl_graph_t*  graph,
                                                                    occtl_node_id_t face,
                                                                    occtl_rep_id_t  surface_id);

/**
 * Replaces the PCurve of a coedge.
 *
 * Pass @p pcurve_id == #OCCTL_REP_ID_INVALID to clear the PCurve binding.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     coedge    CoEdge node ID.  Must be a valid, active coedge.
 * @param[in]     pcurve_id Rep ID of the 2D curve node to bind, or
 *                          #OCCTL_REP_ID_INVALID to clear.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 * @retval OCCTL_WRONG_KIND        @p coedge is not a coedge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_replace_edge_curve, occtl_topo_add_pcurve
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_replace_coedge_pcurve(occtl_graph_t*  graph,
                                                                     occtl_node_id_t coedge,
                                                                     occtl_rep_id_t  pcurve_id);

/**
 * Adds a PCurve binding between an edge and a face.
 *
 * Creates a new CoEdge entity with a Curve2DRep for the given edge-face pair.
 * This is the canonical repair operation when an edge lacks a pcurve on a face
 * it bounds.
 *
 * @param[in,out] graph       Borrows it.  Must be non-NULL.
 * @param[in]     edge        Edge node ID.  Must be a valid, active edge.
 * @param[in]     face        Face node ID.  Must be a valid, active face.
 * @param[in]     pcurve_id   Rep ID of the 2D parametric curve node.
 *                            Must be valid (not #OCCTL_REP_ID_INVALID).
 * @param[in]     first       First parameter on the PCurve.
 * @param[in]     last        Last parameter on the PCurve.
 * @param[in]     orientation Edge orientation on the face.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL or @p pcurve_id is invalid.
 * @retval OCCTL_NOT_FOUND         @p edge or @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge or @p face is not a face.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_replace_coedge_pcurve
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_add_pcurve(occtl_graph_t*      graph,
                                                          occtl_node_id_t     edge,
                                                          occtl_node_id_t     face,
                                                          occtl_rep_id_t      pcurve_id,
                                                          double              first,
                                                          double              last,
                                                          occtl_orientation_t orientation);

/**
 * Adds a face to an existing shell.
 *
 * @param[in,out] graph       Borrows it.  Must be non-NULL.
 * @param[in]     shell       Shell node ID.  Must be a valid, active shell.
 * @param[in]     face        Face node ID.  Must be a valid, active face.
 * @param[in]     orientation Orientation of the face inside the shell.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p shell or @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p shell is not a shell or @p face is not a face.
 * @retval OCCTL_ERROR              the operation was rejected.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_shell_remove_face, occtl_topo_make_shell
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_shell_add_face(occtl_graph_t*      graph,
                                                              occtl_node_id_t     shell,
                                                              occtl_node_id_t     face,
                                                              occtl_orientation_t orientation);

/**
 * Removes a face from a shell without deleting the face definition.
 *
 * The face remains in the graph for use by other shells or compounds.
 *
 * @param[in,out] graph Borrows it.  Must be non-NULL.
 * @param[in]     shell Shell node ID.  Must be a valid, active shell.
 * @param[in]     face  Face node ID.  Must be the definition ID of a face
 *                         referenced by the shell.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p shell or @p face is invalid, removed,
 *                                 has wrong kind, or the face is not in the shell.
 * @retval OCCTL_WRONG_KIND        @p shell is not a shell.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_shell_add_face
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_shell_remove_face(occtl_graph_t*  graph,
                                                                 occtl_node_id_t shell,
                                                                 occtl_node_id_t face);

/**
 * Adds inner wire references to a face without duplicating wire definitions.
 *
 * Each entry in @p holes must name an active Wire definition.  The function
 * attaches each wire as a non-outer wire reference owned by @p face.  Wires
 * already referenced by @p face are rejected to avoid duplicate hole usages.
 *
 * @param[in,out] graph      Borrows it.  Must be non-NULL.
 * @param[in]     face       Face node ID.  Must be a valid, active face.
 * @param[in]     holes      Borrowed span of Wire node IDs to add as holes.
 *                           Must be non-NULL.
 * @param[in]     hole_count Number of entries in @p holes.  Must be greater
 *                           than zero.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p holes is NULL;
 *                                 @p hole_count is zero; a hole ID appears
 *                                 more than once; or a wire is already
 *                                 referenced by @p face.
 * @retval OCCTL_NOT_FOUND         @p face or a requested hole is invalid or
 *                                 removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a face, or a requested hole
 *                                 node is not a wire.
 * @retval OCCTL_ERROR             The graph editor rejected the wire usage.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_face_remove_holes, occtl_topo_make_face_from_wires_auto
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_add_holes(occtl_graph_t*         graph,
                                                              occtl_node_id_t        face,
                                                              const occtl_node_id_t* holes,
                                                              size_t                 hole_count);

/**
 * Removes inner wire references from a face without deleting wire definitions.
 *
 * When @p holes is NULL and @p hole_count is 0, all non-outer wire references
 * are removed from @p face.  Otherwise, @p holes must name inner Wire
 * definition IDs referenced by @p face; each matching wire reference is
 * detached.  The outer wire is never removed by this function.
 *
 * @param[in,out] graph      Borrows it.  Must be non-NULL.
 * @param[in]     face       Face node ID.  Must be a valid, active face.
 * @param[in]     holes      Borrowed span of inner Wire node IDs, or NULL to
 *                           remove all inner wires.
 * @param[in]     hole_count Number of entries in @p holes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL; @p holes is NULL while
 *                                 @p hole_count > 0; or @p holes is non-NULL
 *                                 while @p hole_count == 0.
 * @retval OCCTL_NOT_FOUND         @p face or a requested hole is invalid,
 *                                 removed, not referenced by @p face, or is
 *                                 the outer wire.
 * @retval OCCTL_WRONG_KIND        @p face is not a face, or a requested hole
 *                                 node is not a wire.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_make_face_from_wires_auto, occtl_topo_make_face
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_remove_holes(occtl_graph_t*         graph,
                                                                 occtl_node_id_t        face,
                                                                 const occtl_node_id_t* holes,
                                                                 size_t                 hole_count);

/**
 * Adds a shell to an existing solid.
 *
 * @param[in,out] graph       Borrows it.  Must be non-NULL.
 * @param[in]     solid       Solid node ID.  Must be a valid, active solid.
 * @param[in]     shell       Shell node ID.  Must be a valid, active shell.
 * @param[in]     orientation Orientation of the shell inside the solid.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p solid or @p shell is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p solid is not a solid or @p shell is not a shell.
 * @retval OCCTL_ERROR              the operation was rejected.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_solid_remove_shell, occtl_topo_make_solid
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_solid_add_shell(occtl_graph_t*      graph,
                                                               occtl_node_id_t     solid,
                                                               occtl_node_id_t     shell,
                                                               occtl_orientation_t orientation);

/**
 * Removes a shell from a solid without deleting the shell definition.
 *
 * @param[in,out] graph Borrows it.  Must be non-NULL.
 * @param[in]     solid Solid node ID.  Must be a valid, active solid.
 * @param[in]     shell Shell node ID.  Must be the definition ID of a shell
 *                         referenced by the solid.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p solid or @p shell is invalid, removed,
 *                                 has wrong kind, or the shell is not in the solid.
 * @retval OCCTL_WRONG_KIND        @p solid is not a solid.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_solid_add_shell
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_solid_remove_shell(occtl_graph_t*  graph,
                                                                  occtl_node_id_t solid,
                                                                  occtl_node_id_t shell);

/**
 * Adds a child entity to an existing compound.
 *
 * @param[in,out] graph       Borrows it.  Must be non-NULL.
 * @param[in]     compound    Compound node ID.  Must be a valid, active compound.
 * @param[in]     child       Child node ID of any topology kind.
 * @param[in]     orientation Orientation of the child in the compound.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p compound or @p child is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p compound is not a compound.
 * @retval OCCTL_ERROR              the operation was rejected.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_compound_remove_child, occtl_topo_make_compound
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_compound_add_child(occtl_graph_t*      graph,
                                                                  occtl_node_id_t     compound,
                                                                  occtl_node_id_t     child,
                                                                  occtl_orientation_t orientation);

/**
 * Removes a child from a compound without deleting the child definition.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     compound Compound node ID.  Must be a valid, active compound.
 * @param[in]     child    Child node ID.  Must be the definition ID of a child
 *                            referenced by the compound.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p compound or @p child is invalid, removed,
 *                                 has wrong kind, or the child is not in the compound.
 * @retval OCCTL_WRONG_KIND        @p compound is not a compound.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_compound_add_child
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_compound_remove_child(occtl_graph_t*  graph,
                                                                     occtl_node_id_t compound,
                                                                     occtl_node_id_t child);

/**
 * Splits an edge at the given parameter on its 3D curve.
 *
 * Creates a new vertex at the split point and produces two sub-edges.
 * The original edge is soft-removed; all wires containing it are updated
 * to reference the two new edges.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     edge      Edge node ID.  Must be a valid, active edge with a 3D curve.
 * @param[in]     parameter Parameter on the edge's 3D curve at which to split.
 * @param[out]    out_edge1  Borrows it.  Receives the first sub-edge NodeId (start → split).
 * @param[out]    out_edge2  Borrows it.  Receives the second sub-edge NodeId (split → end).
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, not an edge, or
 *                                 the edge has no 3D curve.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 * @retval OCCTL_ERROR              the operation was rejected.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_edge_add_internal_vertex, occtl_topo_make_edge
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_split(occtl_graph_t*   graph,
                                                          occtl_node_id_t  edge,
                                                          double           parameter,
                                                          occtl_node_id_t* out_edge1,
                                                          occtl_node_id_t* out_edge2);

/**
 * Adds an internal vertex to an edge.
 *
 * The vertex is recorded on the edge with INTERNAL orientation.  This does
 * not split the edge; for splitting see #occtl_topo_edge_split.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     edge      Edge node ID.  Must be a valid, active edge.
 * @param[in]     vertex    Vertex node ID.  Must be a valid, active vertex.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge or @p vertex is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge or @p vertex is not a vertex.
 * @retval OCCTL_ERROR              the operation was rejected.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_edge_remove_vertex, occtl_topo_edge_split
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_add_internal_vertex(occtl_graph_t*  graph,
                                                                        occtl_node_id_t edge,
                                                                        occtl_node_id_t vertex);

/**
 * Removes a vertex from an edge.
 *
 * Supports both boundary vertices (start/end) and internal vertices.
 * The vertex definition is not deleted.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     edge   Edge node ID.  Must be a valid, active edge.
 * @param[in]     vertex Vertex node ID.  Must be the definition ID of a vertex
 *                          referenced by the edge.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge or @p vertex is invalid, removed,
 *                                 has wrong kind, or the vertex is not on the edge.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_edge_add_internal_vertex
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_remove_vertex(occtl_graph_t*  graph,
                                                                  occtl_node_id_t edge,
                                                                  occtl_node_id_t vertex);

/**
 * Returns the geometric continuity between two faces at their shared edge.
 *
 * The continuity describes how smoothly the surfaces of @p faceA and
 * @p faceB meet along @p edge.
 *
 * @param[in]  graph          Must be non-NULL.
 * @param[in]  edge           Edge node ID shared by both faces.
 * @param[in]  faceA          First face node ID.
 * @param[in]  faceB          Second face node ID.
 * @param[out] out_continuity  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_continuity is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge, @p faceA, or @p faceB is invalid
 *                                 or removed.
 * @retval OCCTL_WRONG_KIND        A NodeId has the wrong kind.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_has_continuity, occtl_topo_edge_max_continuity
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_continuity(const occtl_graph_t* const      graph,
                             occtl_node_id_t                 edge,
                             occtl_node_id_t                 faceA,
                             occtl_node_id_t                 faceB,
                             occtl_shape_continuity_t* const out_continuity);

/**
 * Returns whether continuity information is recorded between two faces
 * at a shared edge.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  edge     Edge node ID shared by both faces.
 * @param[in]  faceA    First face node ID.
 * @param[in]  faceB    Second face node ID.
 * @param[out] out_has_continuity  Borrows it.  Set to 1 if continuity is recorded,
 *                                 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_continuity is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge, @p faceA, or @p faceB is invalid
 *                                 or removed.
 * @retval OCCTL_WRONG_KIND        A NodeId has the wrong kind.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_continuity, occtl_topo_edge_max_continuity
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_has_continuity(const occtl_graph_t* const graph,
                                 occtl_node_id_t            edge,
                                 occtl_node_id_t            faceA,
                                 occtl_node_id_t            faceB,
                                 int32_t* const             out_has_continuity);

/**
 * Returns the maximum continuity across all incident faces at an edge.
 *
 * This is the highest continuity value across every face pair that
 * shares @p edge.
 *
 * @param[in]  graph          Must be non-NULL.
 * @param[in]  edge           Edge node ID.
 * @param[out] out_continuity  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_continuity is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_continuity, occtl_topo_edge_has_continuity
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_max_continuity(const occtl_graph_t* const      graph,
                                 occtl_node_id_t                 edge,
                                 occtl_shape_continuity_t* const out_continuity);

/**
 * Returns the full orientation (FORWARD/REVERSED/INTERNAL/EXTERNAL) of a coedge.
 *
 * Unlike #occtl_topo_coedge_is_reversed, which only tests the REVERSED
 * flag, this returns the complete TopAbs_Orientation.
 *
 * @param[in]  graph          Must be non-NULL.
 * @param[in]  coedge         CoEdge node ID.
 * @param[out] out_orientation Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_orientation is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedge_is_reversed
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_orientation(const occtl_graph_t* const graph,
                                occtl_node_id_t            coedge,
                                occtl_orientation_t* const out_orientation);

/**
 * Finds the coedge belonging to the given (edge, face) pair.
 *
 * O(1) lookup through the edge's coedge reverse-index.  Returns
 * #OCCTL_NOT_FOUND when @p edge is not incident to @p face.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  edge       Edge node ID.
 * @param[in]  face       Face node ID.
 * @param[out] out_coedge  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_coedge is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge or @p face is invalid/removed,
 *                                 or no coedge binds this edge-face pair.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge or @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_find_coedge_on_face_oriented
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_find_coedge_on_face(const occtl_graph_t* const graph,
                                      occtl_node_id_t            edge,
                                      occtl_node_id_t            face,
                                      occtl_node_id_t* const     out_coedge);

/**
 * Finds the coedge for an (edge, face, orientation) triple.
 *
 * On seam edges two coedges share the same face with opposite
 * orientations; this overload disambiguates by the requested
 * @p orientation.
 *
 * @param[in]  graph        Must be non-NULL.
 * @param[in]  edge         Edge node ID.
 * @param[in]  face         Face node ID.
 * @param[in]  orientation  Orientation filter (by value).
 * @param[out] out_coedge    Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_coedge is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge or @p face is invalid/removed,
 *                                 or no coedge matches the triple.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge or @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_find_coedge_on_face
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_find_coedge_on_face_oriented(const occtl_graph_t* const graph,
                                               occtl_node_id_t            edge,
                                               occtl_node_id_t            face,
                                               occtl_orientation_t        orientation,
                                               occtl_node_id_t* const     out_coedge);

/**
 * Returns the 3D point of a vertex with the parent's Location applied.
 *
 * The parent can be any node that references the vertex (edge,
 * face, compound, occurrence, etc.).  The returned point
 * incorporates the Location from the parent's reference chain.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  vertex   Vertex node ID.
 * @param[in]  parent   Parent node ID whose Location is applied.
 * @param[out] out_point Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_point is NULL.
 * @retval OCCTL_NOT_FOUND         @p vertex or @p parent is invalid or removed, or
 *                                 @p parent does not reference @p vertex.
 * @retval OCCTL_WRONG_KIND        @p vertex is not a vertex.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_vertex_point
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_vertex_point_in_usage(const occtl_graph_t* const graph,
                                   occtl_node_id_t            vertex,
                                   occtl_node_id_t            parent,
                                   occtl_point3_t* const      out_point);

/**
 * Returns the 2D parameter of a vertex on a coedge's pcurve.
 *
 * @param[in]  graph  Must be non-NULL.
 * @param[in]  vertex Vertex node ID.
 * @param[in]  coedge CoEdge node ID.
 * @param[out] out_u   Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_u is NULL.
 * @retval OCCTL_NOT_FOUND         @p vertex or @p coedge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p vertex is not a vertex or @p coedge is not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_vertex_parameter, occtl_topo_coedge_pcurve_parameter
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_vertex_pcurve_parameter(const occtl_graph_t* const graph,
                                     occtl_node_id_t            vertex,
                                     occtl_node_id_t            coedge,
                                     double* const              out_u);

/**
 * Returns whether an edge has a 3D polygon discretization.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  edge    Edge node ID.
 * @param[out] out_has_polygon3d Borrows it.  Set to 1 if polygon present, 0 otherwise.
 *                               Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_polygon3d is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedge_has_polygon_on_surface
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_has_polygon3d(const occtl_graph_t* const graph,
                                                                  occtl_node_id_t            edge,
                                                                  int32_t* const out_has_polygon3d);

/**
 * Returns whether a coedge has a polygon-on-surface discretization.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  coedge  CoEdge node ID.
 * @param[out] out_has_polygon_on_surface Borrows it.  Set to 1 if polygon present, 0 otherwise.
 *                                        Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_polygon_on_surface is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_has_polygon3d
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_has_polygon_on_surface(const occtl_graph_t* const graph,
                                           occtl_node_id_t            coedge,
                                           int32_t* const             out_has_polygon_on_surface);

/**
 * Evaluates a face surface and its first partial derivatives inside a
 * restricted UV box.
 *
 * The surface is evaluated at (@p u, @p v).  The adaptor is clamped to
 * [@p umin, @p umax] x [@p vmin, @p vmax], which binds the evaluation
 * range separately from the surface's natural bounds.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  face     Face node ID.
 * @param[in]  umin     Lower U bound.
 * @param[in]  umax     Upper U bound.
 * @param[in]  vmin     Lower V bound.
 * @param[in]  vmax     Upper V bound.
 * @param[in]  u        U evaluation parameter.
 * @param[in]  v        V evaluation parameter.
 * @param[out] out_point Borrows it.  Must be non-NULL.
 * @param[out] out_d1u   Borrows it.  Must be non-NULL.
 * @param[out] out_d1v   Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_eval_d1, occtl_topo_face_uv_bounds
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_uv_bounds_restricted(const occtl_graph_t* const graph,
                                       occtl_node_id_t            face,
                                       double                     umin,
                                       double                     umax,
                                       double                     vmin,
                                       double                     vmax,
                                       double                     u,
                                       double                     v,
                                       occtl_point3_t* const      out_point,
                                       occtl_vector3_t* const     out_d1u,
                                       occtl_vector3_t* const     out_d1v);

/**
 * Sets the 3D point of a vertex.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     vertex Vertex node ID.
 * @param[in]     point  New point coordinates (by value).
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p tol is negative
 *                                 or non-finite.
 * @retval OCCTL_NOT_FOUND         @p vertex is invalid, removed, or not a vertex.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_vertex_tolerance
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_vertex_point(occtl_graph_t*  graph,
                                                                occtl_node_id_t vertex,
                                                                occtl_point3_t  point);

/**
 * Sets the tolerance of a vertex.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     vertex Vertex node ID.
 * @param[in]     tol    New tolerance value.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p tol is negative
 *                                 or non-finite.
 * @retval OCCTL_NOT_FOUND         @p vertex is invalid, removed, or not a vertex.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_vertex_point
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_vertex_tolerance(occtl_graph_t*  graph,
                                                                    occtl_node_id_t vertex,
                                                                    double          tol);

/**
 * Sets the tolerance of an edge.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     edge   Edge node ID.
 * @param[in]     tol    New tolerance value.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p tol is negative
 *                                 or non-finite.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_face_tolerance
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_tolerance(occtl_graph_t*  graph,
                                                                  occtl_node_id_t edge,
                                                                  double          tol);

/**
 * Sets the tolerance of a face.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     face   Face node ID.
 * @param[in]     tol    New tolerance value.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p tol is negative
 *                                 or non-finite.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_edge_tolerance
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_face_tolerance(occtl_graph_t*  graph,
                                                                  occtl_node_id_t face,
                                                                  double          tol);

/**
 * Sets the parametric range of an edge's 3D curve.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     edge   Edge node ID.
 * @param[in]     first  First parameter (lower bound).
 * @param[in]     last   Last parameter (upper bound).
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p first/@p last are
 *                                 non-finite, or @p last is less than @p first.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_edge_same_parameter, occtl_topo_set_coedge_param_range
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_param_range(occtl_graph_t*  graph,
                                                                    occtl_node_id_t edge,
                                                                    double          first,
                                                                    double          last);

/**
 * Sets the same-parameter flag on an edge.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     edge   Edge node ID.
 * @param[in]     flag   1 to set, 0 to clear.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p first/@p last are
 *                                 non-finite, or @p last is less than @p first.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_edge_same_range
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_same_parameter(occtl_graph_t*  graph,
                                                                       occtl_node_id_t edge,
                                                                       int32_t         flag);

/**
 * Sets the same-range flag on an edge.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     edge   Edge node ID.
 * @param[in]     flag   1 to set, 0 to clear.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_edge_same_parameter
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_same_range(occtl_graph_t*  graph,
                                                                   occtl_node_id_t edge,
                                                                   int32_t         flag);

/**
 * Sets the is-degenerate flag on an edge.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     edge   Edge node ID.
 * @param[in]     flag   1 to mark degenerate, 0 otherwise.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_edge_is_closed, occtl_topo_edge_is_degenerated
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_is_degenerate(occtl_graph_t*  graph,
                                                                      occtl_node_id_t edge,
                                                                      int32_t         flag);

/**
 * Sets the is-closed flag on an edge (start vertex == end vertex).
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     edge   Edge node ID.
 * @param[in]     flag   1 to mark closed, 0 otherwise.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_edge_is_degenerate, occtl_topo_set_wire_is_closed
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_is_closed(occtl_graph_t*  graph,
                                                                  occtl_node_id_t edge,
                                                                  int32_t         flag);

/**
 * Sets the is-closed flag on a wire.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     wire   Wire node ID.
 * @param[in]     flag   1 to mark closed, 0 otherwise.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid, removed, or not a wire.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_shell_is_closed
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_wire_is_closed(occtl_graph_t*  graph,
                                                                  occtl_node_id_t wire,
                                                                  int32_t         flag);

/**
 * Sets the is-closed flag on a shell (watertightness).
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     shell  Shell node ID.
 * @param[in]     flag   1 to mark closed, 0 otherwise.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p shell is invalid, removed, or not a shell.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_wire_is_closed
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_shell_is_closed(occtl_graph_t*  graph,
                                                                   occtl_node_id_t shell,
                                                                   int32_t         flag);

/**
 * Sets the natural-restriction flag on a face.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     face   Face node ID.
 * @param[in]     flag   1 to set natural restriction, 0 to clear.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_coedge_param_range
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_face_natural_restriction(occtl_graph_t*  graph,
                                                                            occtl_node_id_t face,
                                                                            int32_t         flag);

/**
 * Sets the parametric range of a coedge.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     coedge CoEdge node ID.
 * @param[in]     first  First parameter (lower bound).
 * @param[in]     last   Last parameter (upper bound).
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p first/@p last are
 *                                 non-finite, or @p last is less than @p first.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_edge_param_range, occtl_topo_set_coedge_uv_box
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_coedge_param_range(occtl_graph_t*  graph,
                                                                      occtl_node_id_t coedge,
                                                                      double          first,
                                                                      double          last);

/**
 * Sets the UV box (start UV, end UV) of a coedge.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     coedge CoEdge node ID.
 * @param[in]     uv_lo   UV point at parameter first (by value).
 * @param[in]     uv_hi   UV point at parameter last (by value).
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_coedge_param_range
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_coedge_uv_box(occtl_graph_t*  graph,
                                                                 occtl_node_id_t coedge,
                                                                 occtl_point2_t  uv_lo,
                                                                 occtl_point2_t  uv_hi);

/**
 * RGBA colour value for use with graph annotation functions.
 *
 * Each channel is in the range @c [0.0f, 1.0f].  The alpha channel follows
 * the OpenGL convention: 1.0f is opaque, 0.0f is fully transparent.
 *
 * This is a plain-data struct; no struct_version is needed.
 */
typedef struct occtl_color_rgba
{
  float r; /**< Red channel, [0.0f, 1.0f]. */
  float g; /**< Green channel, [0.0f, 1.0f]. */
  float b; /**< Blue channel, [0.0f, 1.0f]. */
  float a; /**< Alpha channel, [0.0f, 1.0f].  Opaque by default. */
} occtl_color_rgba_t;

#define OCCTL_MATERIAL_INFO_VERSION_1 1u

/**
 * Material-lite record for graph material layers.
 *
 * @c name is a borrowed UTF-8 byte span when setting material data.  For
 * #occtl_graph_material_get, @c name points at the caller-provided output
 * buffer when a buffer was supplied, otherwise it is NULL and @c name_len is
 * the stored byte length.  A zero @c name_len is allowed.
 */
typedef struct occtl_material_info
{
  uint32_t           struct_version;    /**< Must be #OCCTL_MATERIAL_INFO_VERSION_1. */
  const void*        p_next;            /**< Reserved; set to NULL. */
  const char*        name;              /**< Material name bytes.  Borrowed. */
  size_t             name_len;          /**< Length of @c name in bytes. */
  int32_t            has_density;       /**< 0/1; whether @c density is valid. */
  double             density;           /**< Density in kg/m^3 when @c has_density != 0. */
  int32_t            has_diffuse_color; /**< 0/1; whether @c diffuse_color is valid. */
  occtl_color_rgba_t diffuse_color;     /**< Diffuse display colour. */
  occtl_uid_t        metadata_uid;      /**< Optional custom metadata UID, or invalid. */
} occtl_material_info_t;

#define OCCTL_MATERIAL_INFO_INIT                                                                   \
  {OCCTL_MATERIAL_INFO_VERSION_1,                                                                  \
   NULL,                                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   0,                                                                                              \
   0.0,                                                                                            \
   0,                                                                                              \
   {1.0f, 1.0f, 1.0f, 1.0f},                                                                       \
   OCCTL_UID_INVALID}

/**
 * Assembly joint kind.
 *
 * These values describe the intended relative motion between two graph nodes.
 * The library stores the declaration as graph metadata; constraint solving is
 * layered separately.
 */
typedef enum occtl_joint_kind
{
  OCCTL_JOINT_RIGID           = 0, /**< Fixed relative transform. */
  OCCTL_JOINT_REVOLUTE        = 1, /**< One rotational degree of freedom. */
  OCCTL_JOINT_LINEAR          = 2, /**< One translational degree of freedom. */
  OCCTL_JOINT_CYLINDRICAL     = 3, /**< Coaxial rotation plus translation. */
  OCCTL_JOINT_BALL            = 4, /**< Spherical orientation freedom. */
  OCCTL_JOINT_RESERVED_FUTURE = 0x7fffffff
} occtl_joint_kind_t;

#define OCCTL_JOINT_INFO_VERSION_1 1u

/**
 * Assembly joint record stored in graph-owned metadata.
 *
 * The endpoint nodes may be products, occurrences, or topology nodes.  Frames
 * are local joint frames on the respective endpoints.  Limits are optional and
 * are interpreted in radians for rotational joints and graph length units for
 * translational joints.
 */
typedef struct occtl_joint_info
{
  uint32_t           struct_version; /**< Must be #OCCTL_JOINT_INFO_VERSION_1. */
  const void*        p_next;         /**< Reserved; set to NULL. */
  occtl_joint_id_t   id;             /**< Joint identity. Ignored by create; filled by get/list. */
  occtl_joint_kind_t kind;           /**< Joint motion kind. */
  occtl_node_id_t    node_a;         /**< First endpoint node. */
  occtl_node_id_t    node_b;         /**< Second endpoint node. */
  occtl_transform_t  frame_a;        /**< Local joint frame on @c node_a. */
  occtl_transform_t  frame_b;        /**< Local joint frame on @c node_b. */
  int32_t            has_limit_min;  /**< 0/1; whether @c limit_min is valid. */
  double             limit_min;      /**< Optional lower motion limit. */
  int32_t            has_limit_max;  /**< 0/1; whether @c limit_max is valid. */
  double             limit_max;      /**< Optional upper motion limit. */
  occtl_uid_t        metadata_uid;   /**< Optional custom metadata UID, or invalid. */
} occtl_joint_info_t;

#define OCCTL_JOINT_INFO_INIT                                                                      \
  {OCCTL_JOINT_INFO_VERSION_1,                                                                     \
   NULL,                                                                                           \
   OCCTL_JOINT_ID_INVALID,                                                                         \
   OCCTL_JOINT_RIGID,                                                                              \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0}},                                 \
   {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0}},                                 \
   0,                                                                                              \
   0.0,                                                                                            \
   0,                                                                                              \
   0.0,                                                                                            \
   OCCTL_UID_INVALID}

/**
 * Borrowed view of a metadata key stored on a graph node.
 *
 * Pointer lifetime is tied to the owning graph metadata.  The view is invalidated
 * by metadata mutation on the graph, graph mutation that remaps/removes the
 * target node, graph clone/free, or graph compaction.
 */
typedef struct occtl_metadata_key_view
{
  const char* key;     /**< Borrowed key bytes; not necessarily NUL-terminated. */
  size_t      key_len; /**< Length of @c key in bytes. */
} occtl_metadata_key_view_t;

/**
 * Borrowed view of a tag stored on a graph node.
 *
 * Pointer lifetime is tied to the owning graph metadata.  The view is invalidated
 * by tag mutation on the graph, graph mutation that remaps/removes the target
 * node, graph clone/free, or graph compaction.
 */
typedef struct occtl_tag_view
{
  const char* tag;     /**< Borrowed tag bytes; not necessarily NUL-terminated. */
  size_t      tag_len; /**< Length of @c tag in bytes. */
} occtl_tag_view_t;

/**
 * Opaque iterator over selected graph nodes.
 *
 * Created by #occtl_select_iter_create and released with
 * #occtl_select_iter_free.
 *
 * @threadsafe No — single-owner; concurrent reads on distinct iterators are
 *             safe when the owning graph is not mutated.
 */
typedef struct occtl_select_iter occtl_select_iter_t;

/**
 * Opaque iterator over grouped selector results.
 *
 * Created by #occtl_select_group_iter_create and released with
 * #occtl_select_group_iter_free.
 *
 * @threadsafe No — single-owner; concurrent reads on distinct iterators are
 *             safe when the owning graph is not mutated.
 */
typedef struct occtl_select_group_iter occtl_select_group_iter_t;

/**
 * Axis-aligned box used by selector filters.
 */
typedef struct occtl_select_bbox
{
  occtl_point3_t min; /**< Minimum corner. */
  occtl_point3_t max; /**< Maximum corner. */
} occtl_select_bbox_t;

/**
 * Oriented box value computed for a graph node.
 *
 * The three direction vectors form the local box axes.  Half sizes are
 * non-negative distances from @c center along the matching axis.
 */
typedef struct occtl_graph_obb
{
  occtl_point3_t     center;      /**< Box centre in model coordinates. */
  occtl_direction3_t x_direction; /**< Unit X axis of the oriented box. */
  occtl_direction3_t y_direction; /**< Unit Y axis of the oriented box. */
  occtl_direction3_t z_direction; /**< Unit Z axis of the oriented box. */
  double             x_half_size; /**< Half size along @c x_direction. */
  double             y_half_size; /**< Half size along @c y_direction. */
  double             z_half_size; /**< Half size along @c z_direction. */
} occtl_graph_obb_t;

/**
 * Face UV bounds value computed for a graph node.
 */
typedef struct occtl_graph_uv_bounds
{
  double  u_min;                  /**< Minimum U parameter. */
  double  u_max;                  /**< Maximum U parameter. */
  double  v_min;                  /**< Minimum V parameter. */
  double  v_max;                  /**< Maximum V parameter. */
  int32_t is_natural_restriction; /**< 0/1; face uses the full surface domain. */
} occtl_graph_uv_bounds_t;

/**
 * Combined OCCT mass-property value computed for a graph node.
 *
 * @c linear_length is the total curve length of edges in the shape,
 * @c surface_area is the total face area, and @c volume is the signed solid
 * volume. @c mass uses unit density and follows the highest non-zero
 * dimensional property: volume, then area, then length. The inertia matrix is
 * row-major, about @c centre_of_mass, in the global XYZ frame.
 */
typedef struct occtl_graph_mass_properties
{
  double         linear_length;  /**< Total edge length. */
  double         surface_area;   /**< Total face area. */
  double         volume;         /**< Signed solid volume. */
  double         mass;           /**< Unit-density mass for the highest non-zero dimension. */
  occtl_point3_t centre_of_mass; /**< Centre of mass for @c mass. */
  double         inertia[9];     /**< Row-major 3x3 inertia matrix. */
} occtl_graph_mass_properties_t;

/**
 * Clears graph-owned computed data related to one node or one reference.
 *
 * Exactly one of @p node or @p ref must be valid.  The implementation removes
 * graph-owned computed data attached to the selected target; callers must treat
 * cached calculation results as an optimization only.
 *
 * @param[in,out] graph Borrows it.  Must be non-NULL.
 * @param[in]     node  Node ID to clear, or #OCCTL_NODE_ID_INVALID.
 * @param[in]     ref   Ref ID to clear, or #OCCTL_REF_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or both/neither IDs are valid.
 * @retval OCCTL_NOT_FOUND         The selected node or reference is invalid or removed.
 *
 * @threadsafe No (mutates internal computed-data state).
 *
 * @sa occtl_graph_bbox_get, occtl_graph_pair_distance_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_clear_cached(occtl_graph_t*  graph,
                                                             occtl_node_id_t node,
                                                             occtl_ref_id_t  ref);

/**
 * Bounding-box predicate for #occtl_select_options_t.
 */
typedef enum occtl_select_bbox_mode
{
  OCCTL_SELECT_BBOX_INTERSECTS           = 0, /**< Candidate box intersects @c bbox. */
  OCCTL_SELECT_BBOX_INSIDE               = 1, /**< Candidate box is fully inside @c bbox. */
  OCCTL_SELECT_BBOX_CONTAINS_CENTER      = 2, /**< Candidate box centre is inside @c bbox. */
  OCCTL_SELECT_BBOX_MODE_RESERVED_FUTURE = 0x7fffffff
} occtl_select_bbox_mode_t;

/**
 * Principal axis used by selector position filters.
 */
typedef enum occtl_select_axis
{
  OCCTL_SELECT_AXIS_X               = 0, /**< X coordinate. */
  OCCTL_SELECT_AXIS_Y               = 1, /**< Y coordinate. */
  OCCTL_SELECT_AXIS_Z               = 2, /**< Z coordinate. */
  OCCTL_SELECT_AXIS_RESERVED_FUTURE = 0x7fffffff
} occtl_select_axis_t;

/**
 * Axis-position predicate for #occtl_select_options_t.
 *
 * Position is measured from each candidate bounding-box centre along
 * #occtl_select_options_t::axis.  MIN / MAX select candidates on the lowest
 * / highest centre plane.  CENTER selects candidates on the mid-plane between
 * the lowest and highest candidate centres.
 */
typedef enum occtl_select_axis_position
{
  OCCTL_SELECT_AXIS_POSITION_MIN    = 0, /**< Lowest candidate centre coordinate. */
  OCCTL_SELECT_AXIS_POSITION_MAX    = 1, /**< Highest candidate centre coordinate. */
  OCCTL_SELECT_AXIS_POSITION_CENTER = 2, /**< Mid-plane between lowest and highest centres. */
  OCCTL_SELECT_AXIS_POSITION_RESERVED_FUTURE = 0x7fffffff
} occtl_select_axis_position_t;

/**
 * Face-normal predicate for #occtl_select_options_t.
 */
typedef enum occtl_select_normal_mode
{
  OCCTL_SELECT_NORMAL_PARALLEL             = 0, /**< Normal points with @c normal. */
  OCCTL_SELECT_NORMAL_ANTIPARALLEL         = 1, /**< Normal points opposite @c normal. */
  OCCTL_SELECT_NORMAL_EITHER               = 2, /**< Normal is parallel or antiparallel. */
  OCCTL_SELECT_NORMAL_MODE_RESERVED_FUTURE = 0x7fffffff
} occtl_select_normal_mode_t;

/**
 * OCCT mass-property predicate for #occtl_select_options_t.
 */
typedef enum occtl_select_measure_kind
{
  OCCTL_SELECT_MEASURE_EDGE_LENGTH          = 0, /**< Linear mass of Edge nodes. */
  OCCTL_SELECT_MEASURE_WIRE_LENGTH          = 1, /**< Linear mass of Wire nodes. */
  OCCTL_SELECT_MEASURE_FACE_AREA            = 2, /**< Surface mass of Face nodes. */
  OCCTL_SELECT_MEASURE_SURFACE_AREA         = 3, /**< Surface mass of any candidate node shape. */
  OCCTL_SELECT_MEASURE_VOLUME               = 4, /**< Volume mass of any candidate node shape. */
  OCCTL_SELECT_MEASURE_KIND_RESERVED_FUTURE = 0x7fffffff
} occtl_select_measure_kind_t;

/**
 * Sort key for selected-node output.
 */
typedef enum occtl_select_sort_key
{
  OCCTL_SELECT_SORT_NONE            = 0, /**< Preserve traversal order. */
  OCCTL_SELECT_SORT_AXIS_COORDINATE = 1, /**< Sort by computed bbox-centre coordinate. */
  OCCTL_SELECT_SORT_MEASURE         = 2, /**< Sort by OCCT mass property. */
  OCCTL_SELECT_SORT_DISTANCE_TO_POINT =
    3, /**< Sort by computed bbox-centre distance to @c sort_point. */
  OCCTL_SELECT_SORT_NAME             = 4, /**< Sort by metadata-backed name. */
  OCCTL_SELECT_SORT_UID              = 5, /**< Sort by persistent UID bits. */
  OCCTL_SELECT_SORT_DISTANCE_TO_NODE = 6, /**< Sort by computed OCCT distance to a target node. */
  OCCTL_SELECT_SORT_KEY_RESERVED_FUTURE = 0x7fffffff
} occtl_select_sort_key_t;

/**
 * Sort direction for selected-node output.
 */
typedef enum occtl_select_sort_direction
{
  OCCTL_SELECT_SORT_ASCENDING                 = 0, /**< Smallest key first. */
  OCCTL_SELECT_SORT_DESCENDING                = 1, /**< Largest key first. */
  OCCTL_SELECT_SORT_DIRECTION_RESERVED_FUTURE = 0x7fffffff
} occtl_select_sort_direction_t;

/**
 * Grouping key for selector results.
 */
typedef enum occtl_select_group_key
{
  OCCTL_SELECT_GROUP_KIND                = 0, /**< Group by #occtl_node_kind_t. */
  OCCTL_SELECT_GROUP_AXIS_COORDINATE     = 1, /**< Group by approximate bbox-centre coordinate. */
  OCCTL_SELECT_GROUP_CURVE_KIND          = 2, /**< Group Edge nodes by #occtl_curve_kind_t. */
  OCCTL_SELECT_GROUP_SURFACE_KIND        = 3, /**< Group Face nodes by #occtl_surface_kind_t. */
  OCCTL_SELECT_GROUP_NAME                = 4, /**< Group by metadata-backed name. */
  OCCTL_SELECT_GROUP_COLOR               = 5, /**< Group by metadata-backed colour. */
  OCCTL_SELECT_GROUP_KEY_RESERVED_FUTURE = 0x7fffffff
} occtl_select_group_key_t;

#define OCCTL_SELECT_OPTIONS_VERSION_1 1u
#define OCCTL_SELECT_METADATA_FILTER_VERSION_1 1u
#define OCCTL_SELECT_DISTANCE_TO_NODE_SORT_VERSION_1 1u
#define OCCTL_SELECT_GROUP_OPTIONS_VERSION_1 1u
#define OCCTL_SELECT_GROUP_VIEW_VERSION_1 1u

/**
 * Selection options for #occtl_select_iter_create.
 *
 * @c kind_mask uses @c (1ull << OCCTL_KIND_*) bits.  A zero mask means all
 * node kinds.  When @c root is invalid, selection scans the whole graph;
 * otherwise it walks descendants from @c root.  Set @c include_root to 1 to
 * test @c root itself before its descendants.
 *
 * Metadata filters are supplied through #occtl_select_metadata_filter_t chained
 * from @c p_next.
 */
typedef struct occtl_select_options
{
  uint32_t                 struct_version; /**< Must be #OCCTL_SELECT_OPTIONS_VERSION_1. */
  const void*              p_next;         /**< Optional #occtl_select_metadata_filter_t chain. */
  occtl_node_id_t          root;      /**< Root node, or #OCCTL_NODE_ID_INVALID for whole graph. */
  uint64_t                 kind_mask; /**< Bit mask of #occtl_node_kind_t values; 0 means all. */
  int32_t                  include_root;     /**< 0/1; only used when @c root is valid. */
  const char*              name;             /**< Optional exact UTF-8 name filter. */
  size_t                   name_len;         /**< Length of @c name in bytes. */
  int32_t                  use_color;        /**< 0/1; match nodes with an explicit colour. */
  occtl_color_rgba_t       color;            /**< Colour filter when @c use_color != 0. */
  float                    color_tolerance;  /**< Per-channel absolute tolerance. */
  int32_t                  use_bbox;         /**< 0/1; match by computed node AABB. */
  occtl_select_bbox_t      bbox;             /**< Bounding box filter when @c use_bbox != 0. */
  occtl_select_bbox_mode_t bbox_mode;        /**< Bounding-box predicate. */
  int32_t                  use_curve_kind;   /**< 0/1; match Edge nodes by 3D curve kind. */
  occtl_curve_kind_t       curve_kind;       /**< Curve kind filter when @c use_curve_kind != 0. */
  int32_t                  use_surface_kind; /**< 0/1; match Face nodes by surface kind. */
  occtl_surface_kind_t     surface_kind; /**< Surface kind filter when @c use_surface_kind != 0. */
  int32_t             use_axis_position; /**< 0/1; filter by bbox-centre position along @c axis. */
  occtl_select_axis_t axis;              /**< Principal axis for @c use_axis_position. */
  occtl_select_axis_position_t axis_position; /**< Axis-position predicate. */
  double  axis_tolerance; /**< Absolute coordinate tolerance for axis-position matching. */
  int32_t use_normal;     /**< 0/1; match Face nodes by midpoint surface normal. */
  occtl_direction3_t            normal; /**< Normal direction filter when @c use_normal != 0. */
  occtl_select_normal_mode_t    normal_mode;            /**< Normal predicate. */
  double                        normal_angle_tolerance; /**< Angular tolerance in radians. */
  int32_t                       use_measure;            /**< 0/1; match by OCCT mass property. */
  occtl_select_measure_kind_t   measure_kind;           /**< Measure predicate kind. */
  double                        measure_min;            /**< Inclusive minimum measure value. */
  double                        measure_max;            /**< Inclusive maximum measure value. */
  occtl_select_sort_key_t       sort_key; /**< Output sort key; NONE preserves traversal order. */
  occtl_select_sort_direction_t sort_direction; /**< Output sort direction. */
  occtl_select_axis_t           sort_axis; /**< Axis for @c OCCTL_SELECT_SORT_AXIS_COORDINATE. */
  occtl_select_measure_kind_t   sort_measure_kind; /**< Measure for @c OCCTL_SELECT_SORT_MEASURE. */
  occtl_point3_t sort_point; /**< Point for @c OCCTL_SELECT_SORT_DISTANCE_TO_POINT. */
} occtl_select_options_t;

#define OCCTL_SELECT_OPTIONS_INIT                                                                  \
  {OCCTL_SELECT_OPTIONS_VERSION_1,                                                                 \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   0u,                                                                                             \
   0,                                                                                              \
   NULL,                                                                                           \
   0u,                                                                                             \
   0,                                                                                              \
   {0.0f, 0.0f, 0.0f, 1.0f},                                                                       \
   0.0f,                                                                                           \
   0,                                                                                              \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},                                                             \
   OCCTL_SELECT_BBOX_INTERSECTS,                                                                   \
   0,                                                                                              \
   OCCTL_CURVE_KIND_UNDEFINED,                                                                     \
   0,                                                                                              \
   OCCTL_SURFACE_KIND_UNDEFINED,                                                                   \
   0,                                                                                              \
   OCCTL_SELECT_AXIS_Z,                                                                            \
   OCCTL_SELECT_AXIS_POSITION_MAX,                                                                 \
   1.0e-7,                                                                                         \
   0,                                                                                              \
   {0.0, 0.0, 1.0},                                                                                \
   OCCTL_SELECT_NORMAL_PARALLEL,                                                                   \
   1.0e-7,                                                                                         \
   0,                                                                                              \
   OCCTL_SELECT_MEASURE_FACE_AREA,                                                                 \
   0.0,                                                                                            \
   0.0,                                                                                            \
   OCCTL_SELECT_SORT_NONE,                                                                         \
   OCCTL_SELECT_SORT_ASCENDING,                                                                    \
   OCCTL_SELECT_AXIS_Z,                                                                            \
   OCCTL_SELECT_MEASURE_FACE_AREA,                                                                 \
   {0.0, 0.0, 0.0}}

/**
 * Metadata predicate extension for #occtl_select_options_t.
 *
 * Chain one instance through #occtl_select_options_t::p_next.  Set @c key_len
 * to a non-zero value to match key presence.  Also set @c match_value to 1 to
 * require an exact value match.
 */
typedef struct occtl_select_metadata_filter
{
  uint32_t    struct_version; /**< Must be #OCCTL_SELECT_METADATA_FILTER_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  const char* key;            /**< Metadata key bytes; not necessarily NUL-terminated. */
  size_t      key_len;        /**< Length of @c key in bytes. */
  const char* value;          /**< Metadata value bytes; not necessarily NUL-terminated. */
  size_t      value_len;      /**< Length of @c value in bytes. */
  int32_t     match_value;    /**< 0/1; 0 matches key presence only. */
} occtl_select_metadata_filter_t;

#define OCCTL_SELECT_METADATA_FILTER_INIT                                                          \
  {OCCTL_SELECT_METADATA_FILTER_VERSION_1, NULL, NULL, 0u, NULL, 0u, 0}

/**
 * Distance-to-node sort extension for #occtl_select_options_t.
 *
 * Chain one instance through #occtl_select_options_t::p_next when
 * #occtl_select_options_t::sort_key is
 * #OCCTL_SELECT_SORT_DISTANCE_TO_NODE.  The selector computes OCCT minimum
 * distances from each candidate node to @c target.
 *
 * To combine this with a metadata filter, set this struct as the first
 * #occtl_select_options_t::p_next entry and chain
 * #occtl_select_metadata_filter_t from this struct's @c p_next.
 */
typedef struct occtl_select_distance_to_node_sort
{
  uint32_t        struct_version; /**< Must be #OCCTL_SELECT_DISTANCE_TO_NODE_SORT_VERSION_1. */
  const void*     p_next;         /**< Optional #occtl_select_metadata_filter_t chain. */
  occtl_node_id_t target;         /**< Target node for distance sorting. Must be active. */
} occtl_select_distance_to_node_sort_t;

#define OCCTL_SELECT_DISTANCE_TO_NODE_SORT_INIT                                                    \
  {OCCTL_SELECT_DISTANCE_TO_NODE_SORT_VERSION_1, NULL, OCCTL_NODE_ID_INVALID}

/**
 * Grouping options for #occtl_select_group_iter_create.
 */
typedef struct occtl_select_group_options
{
  uint32_t                 struct_version;  /**< Must be #OCCTL_SELECT_GROUP_OPTIONS_VERSION_1. */
  const void*              p_next;          /**< Reserved; set to NULL. */
  occtl_select_group_key_t key;             /**< Grouping key. */
  occtl_select_axis_t      axis;            /**< Axis for #OCCTL_SELECT_GROUP_AXIS_COORDINATE. */
  double                   tolerance;       /**< Absolute bucket tolerance for numeric grouping. */
  float                    color_tolerance; /**< Per-channel tolerance for colour grouping. */
  int32_t include_missing; /**< 0/1; include nodes missing name/colour/geometry keys. */
} occtl_select_group_options_t;

#define OCCTL_SELECT_GROUP_OPTIONS_INIT                                                            \
  {OCCTL_SELECT_GROUP_OPTIONS_VERSION_1,                                                           \
   NULL,                                                                                           \
   OCCTL_SELECT_GROUP_KIND,                                                                        \
   OCCTL_SELECT_AXIS_Z,                                                                            \
   1.0e-7,                                                                                         \
   0.0f,                                                                                           \
   0}

/**
 * Immutable view of one selected-node group.
 *
 * Pointer fields borrow from the owning #occtl_select_group_iter_t and remain
 * valid until the iterator is advanced or freed.
 */
typedef struct occtl_select_group_view
{
  uint32_t                 struct_version; /**< Must be #OCCTL_SELECT_GROUP_VIEW_VERSION_1. */
  const void*              p_next;         /**< Reserved; set to NULL. */
  occtl_select_group_key_t key;            /**< Grouping key that produced this view. */
  occtl_node_kind_t        node_kind;      /**< Valid for #OCCTL_SELECT_GROUP_KIND. */
  occtl_curve_kind_t       curve_kind;     /**< Valid for #OCCTL_SELECT_GROUP_CURVE_KIND. */
  occtl_surface_kind_t     surface_kind;   /**< Valid for #OCCTL_SELECT_GROUP_SURFACE_KIND. */
  double                 numeric_key; /**< Bucket value for #OCCTL_SELECT_GROUP_AXIS_COORDINATE. */
  const char*            name;        /**< Name bytes for #OCCTL_SELECT_GROUP_NAME; may be NULL. */
  size_t                 name_len;    /**< Length of @c name in bytes. */
  int32_t                has_color;   /**< 0/1; whether @c color is valid. */
  occtl_color_rgba_t     color;       /**< Colour for #OCCTL_SELECT_GROUP_COLOR. */
  const occtl_node_id_t* nodes;       /**< Borrowed span of grouped nodes. */
  size_t                 node_count;  /**< Number of elements in @c nodes. */
} occtl_select_group_view_t;

#define OCCTL_SELECT_GROUP_VIEW_INIT                                                               \
  {OCCTL_SELECT_GROUP_VIEW_VERSION_1,                                                              \
   NULL,                                                                                           \
   OCCTL_SELECT_GROUP_KIND,                                                                        \
   OCCTL_KIND_INVALID,                                                                             \
   OCCTL_CURVE_KIND_UNDEFINED,                                                                     \
   OCCTL_SURFACE_KIND_UNDEFINED,                                                                   \
   0.0,                                                                                            \
   NULL,                                                                                           \
   0,                                                                                              \
   0,                                                                                              \
   {0.0f, 0.0f, 0.0f, 0.0f},                                                                       \
   NULL,                                                                                           \
   0}

/**
 * Initialises @p options to #OCCTL_SELECT_OPTIONS_INIT.
 *
 * @param[out] options Borrows it.  May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_select_iter_create
 */
OCCTL_API void OCCTL_CALL occtl_select_options_init(occtl_select_options_t* options);

/**
 * Initialises @p options to #OCCTL_SELECT_GROUP_OPTIONS_INIT.
 *
 * @param[out] options Borrows it.  May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_select_group_iter_create
 */
OCCTL_API void OCCTL_CALL occtl_select_group_options_init(occtl_select_group_options_t* options);

/**
 * Initialises @p view to #OCCTL_SELECT_GROUP_VIEW_INIT.
 *
 * @param[out] view Borrows it. May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_select_group_iter_next
 */
OCCTL_API void OCCTL_CALL occtl_select_group_view_init(occtl_select_group_view_t* view);

/**
 * Returns a computed axis-aligned bounding box for a node.
 *
 * The value is computed from current graph state. OCCT may use internal transient
 * caches, but OCCT-Light exposes only the computed POD result.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     node     Node ID to query.  Must be valid and active.
 * @param[out]    out_bbox Borrows it.  Must be non-NULL.  Receives the box.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_bbox is NULL.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not compute a finite box.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_graph_obb_get, occtl_graph_measure_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_bbox_get(occtl_graph_t*       graph,
                                                         occtl_node_id_t      node,
                                                         occtl_select_bbox_t* out_bbox);

/**
 * Returns a computed oriented bounding box for a node.
 *
 * The value is computed through OCCT from current graph state. Callers receive a
 * POD copy.
 *
 * @param[in,out] graph   Borrows it.  Must be non-NULL.
 * @param[in]     node    Node ID to query.  Must be valid and active.
 * @param[out]    out_obb Borrows it.  Must be non-NULL.  Receives the box.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_obb is NULL.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not compute an oriented box.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_graph_bbox_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_obb_get(occtl_graph_t*     graph,
                                                        occtl_node_id_t    node,
                                                        occtl_graph_obb_t* out_obb);

/**
 * Returns computed UV parameter bounds for a Face node.
 *
 * The value is computed through BRepGraph UV-bounds algorithms from current graph
 * state. OCCT may use internal transient caches; callers receive a POD copy.
 *
 * @param[in,out] graph         Borrows it.  Must be non-NULL.
 * @param[in]     face          Face node ID to query.  Must be valid and active.
 * @param[out]    out_uv_bounds Borrows it.  Must be non-NULL.  Receives bounds.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_uv_bounds is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a Face node.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not compute valid UV bounds.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_topo_face_uv_bounds
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_face_uv_bounds_get(occtl_graph_t*           graph,
                                 occtl_node_id_t          face,
                                 occtl_graph_uv_bounds_t* out_uv_bounds);

/**
 * Returns a computed OCCT mass-property scalar for a node.
 *
 * The value is computed through OCCT mass-property algorithms from current graph
 * state.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     node      Node ID to query.  Must be valid and active.
 * @param[in]     kind      Measure scalar to compute.
 * @param[out]    out_value Borrows it.  Must be non-NULL.  Receives the value.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_value is NULL, or
 *                                 @p kind is unsupported.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p kind is not applicable to @p node.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not compute the requested value.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_graph_bbox_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_measure_get(occtl_graph_t*              graph,
                                                            occtl_node_id_t             node,
                                                            occtl_select_measure_kind_t kind,
                                                            double*                     out_value);

/**
 * Returns computed combined OCCT mass properties for a node.
 *
 * The value is computed through OCCT mass-property algorithms from current graph
 * state. Callers receive a POD copy.
 *
 * @param[in,out] graph          Borrows it.  Must be non-NULL.
 * @param[in]     node           Node ID to query.  Must be valid and active.
 * @param[out]    out_properties Borrows it.  Must be non-NULL.  Receives the
 *                               mass-property summary.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_properties is NULL.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not compute finite properties.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_graph_measure_get
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_mass_properties_get(occtl_graph_t*                 graph,
                                  occtl_node_id_t                node,
                                  occtl_graph_mass_properties_t* out_properties);

/**
 * Returns a computed curve-kind classification for an Edge node.
 *
 * The classification delegates to OCCT edge adaptors. Callers receive a POD enum
 * copy.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     edge     Edge node ID to classify.  Must be valid and active.
 * @param[out]    out_kind Borrows it.  Must be non-NULL.  Receives the curve
 *                         kind.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_kind is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an Edge node.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not classify the edge geometry.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_topo_edge_curve_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_edge_curve_kind_get(occtl_graph_t*      graph,
                                                                    occtl_node_id_t     edge,
                                                                    occtl_curve_kind_t* out_kind);

/**
 * Returns a computed surface-kind classification for a Face node.
 *
 * The classification delegates to OCCT surface adaptors. Callers receive a POD
 * enum copy.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     face     Face node ID to classify.  Must be valid and active.
 * @param[out]    out_kind Borrows it.  Must be non-NULL.  Receives the surface
 *                         kind.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_kind is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a Face node.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not classify the face geometry.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_topo_face_surface_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_face_surface_kind_get(occtl_graph_t*        graph,
                                    occtl_node_id_t       face,
                                    occtl_surface_kind_t* out_kind);

/**
 * Returns computed descendant Vertex nodes for a graph root.
 *
 * The list is computed through BRepGraph child traversal from current graph
 * state.  Duplicate Vertex definitions are
 * collapsed.  If @p node is itself a Vertex, the returned list contains that
 * Vertex.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to learn
 * the required count in @p out_count, then reissue with a buffer of at least
 * that many #occtl_node_id_t entries.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     node      Root node ID to query.  Must be valid and active.
 * @param[out]    out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]     cap       Capacity of @p out_buf in entries.
 * @param[out]    out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_topo_common_vertices
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_descendant_vertices_get(occtl_graph_t*   graph,
                                                                        occtl_node_id_t  node,
                                                                        occtl_node_id_t* out_buf,
                                                                        size_t           cap,
                                                                        size_t*          out_count);

/**
 * Returns computed descendant Edge nodes for a graph root.
 *
 * The list is computed through BRepGraph child traversal from current graph
 * state.  Duplicate Edge definitions are
 * collapsed.  If @p node is itself an Edge, the returned list contains that
 * Edge.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to learn
 * the required count in @p out_count, then reissue with a buffer of at least
 * that many #occtl_node_id_t entries.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     node      Root node ID to query.  Must be valid and active.
 * @param[out]    out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]     cap       Capacity of @p out_buf in entries.
 * @param[out]    out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_topo_connected_edges
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_descendant_edges_get(occtl_graph_t*   graph,
                                                                     occtl_node_id_t  node,
                                                                     occtl_node_id_t* out_buf,
                                                                     size_t           cap,
                                                                     size_t*          out_count);

/**
 * Returns computed descendant Face nodes for a graph root.
 *
 * The list is computed through BRepGraph child traversal from current graph
 * state.  Duplicate Face definitions are
 * collapsed.  If @p node is itself a Face, the returned list contains that
 * Face.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to learn
 * the required count in @p out_count, then reissue with a buffer of at least
 * that many #occtl_node_id_t entries.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     node      Root node ID to query.  Must be valid and active.
 * @param[out]    out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]     cap       Capacity of @p out_buf in entries.
 * @param[out]    out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_topo_connected_faces
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_descendant_faces_get(occtl_graph_t*   graph,
                                                                     occtl_node_id_t  node,
                                                                     occtl_node_id_t* out_buf,
                                                                     size_t           cap,
                                                                     size_t*          out_count);

/**
 * Returns computed descendant nodes of one requested kind for a graph root.
 *
 * The list is computed through BRepGraph child traversal from current graph state and
 * keyed by @p descendant_kind.  Duplicate
 * definitions are collapsed.  If @p node itself has @p descendant_kind, the
 * returned list includes @p node.
 *
 * This is the broad descendant query surface for root-scoped scans.
 * Use the narrower Vertex / Edge / Face functions only
 * when a binding wants a more explicitly named convenience wrapper.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to learn
 * the required count in @p out_count, then reissue with a buffer of at least
 * that many #occtl_node_id_t entries.
 *
 * @param[in,out] graph           Borrows it.  Must be non-NULL.
 * @param[in]     node            Root node ID to query.  Must be valid and active.
 * @param[in]     descendant_kind Requested node kind.  Must not be #OCCTL_KIND_INVALID.
 * @param[out]    out_buf         Borrows it.  May be NULL on the sizing call.
 * @param[in]     cap             Capacity of @p out_buf in entries.
 * @param[out]    out_count       Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL, or
 *                                 @p descendant_kind is invalid.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_graph_descendant_vertices_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_descendants_get(occtl_graph_t*    graph,
                                                                occtl_node_id_t   node,
                                                                occtl_node_kind_t descendant_kind,
                                                                occtl_node_id_t*  out_buf,
                                                                size_t            cap,
                                                                size_t*           out_count);

/**
 * Returns computed adjacent Face nodes for one Face node.
 *
 * The list is computed through BRepGraph face-adjacency relations from current graph state.
 * Duplicate Face definitions are collapsed.  This is a reusable graph-owned adjacency primitive for
 * connected-face walks, selector-style relation predicates, and future
 * reverse-engineering patch traversal.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to learn
 * the required count in @p out_count, then reissue with a buffer of at least
 * that many #occtl_node_id_t entries.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     face      Face node ID to query.  Must be valid and active.
 * @param[out]    out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]     cap       Capacity of @p out_buf in entries.
 * @param[out]    out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a Face node.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_topo_connected_faces
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_adjacent_faces_get(occtl_graph_t*   graph,
                                                                   occtl_node_id_t  face,
                                                                   occtl_node_id_t* out_buf,
                                                                   size_t           cap,
                                                                   size_t*          out_count);

/**
 * Returns computed adjacent Edge nodes for one Edge node.
 *
 * The list is computed through BRepGraph shared-Vertex incidence from current graph state.
 * Duplicate Edge definitions are collapsed and @p edge itself is not returned.  This is a reusable
 * graph-owned adjacency primitive for connected-edge walks, wire grouping,
 * selector-style relation predicates, and future sketch cleanup helpers.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to learn
 * the required count in @p out_count, then reissue with a buffer of at least
 * that many #occtl_node_id_t entries.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     edge      Edge node ID to query.  Must be valid and active.
 * @param[out]    out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]     cap       Capacity of @p out_buf in entries.
 * @param[out]    out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an Edge node.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_topo_connected_edges
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_adjacent_edges_get(occtl_graph_t*   graph,
                                                                   occtl_node_id_t  edge,
                                                                   occtl_node_id_t* out_buf,
                                                                   size_t           cap,
                                                                   size_t*          out_count);

/**
 * Returns a computed OCCT minimum distance between two graph nodes.
 *
 * The value is computed through OCCT from current graph state. Callers receive a
 * POD scalar copy.
 *
 * @param[in,out] graph        Borrows it.  Must be non-NULL.
 * @param[in]     first        First node ID.  Must be valid and active.
 * @param[in]     second       Second node ID.  Must be valid and active.
 * @param[out]    out_distance Borrows it.  Must be non-NULL.  Receives the
 *                             minimum distance.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_distance is NULL.
 * @retval OCCTL_NOT_FOUND         @p first or @p second is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not compute a finite distance.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_topo_distance_pair
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_pair_distance_get(occtl_graph_t*  graph,
                                                                  occtl_node_id_t first,
                                                                  occtl_node_id_t second,
                                                                  double*         out_distance);

/**
 * Creates an iterator over nodes matching @p options.
 *
 * The selector computes derived data such as node bounding boxes; pass a mutable
 * graph handle because OCCT algorithms may update internal graph state.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     options  Borrows it.  May be NULL for defaults.
 * @param[out]    out_iter Owns it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL, or an
 *                                 optional pointer field is inconsistent.
 * @retval OCCTL_NOT_FOUND         @c options->root is invalid or removed.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version unsupported.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_select_iter_next, occtl_select_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_select_iter_create(occtl_graph_t*                graph,
                                                             const occtl_select_options_t* options,
                                                             occtl_select_iter_t** out_iter);

/**
 * Creates an iterator over tagged nodes matching @p options.
 *
 * This is equivalent to #occtl_select_iter_create followed by an exact graph
 * tag predicate, but filtering is performed during selector traversal.
 * The selector may populate BRepGraph transient caches for expensive derived
 * data such as node bounding boxes; pass a mutable graph handle.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     options  Borrows it.  May be NULL for defaults.
 * @param[in]     tag      Tag bytes.  Borrowed; must be non-NULL and non-empty.
 * @param[in]     tagLen   Length of @p tag in bytes.
 * @param[out]    out_iter Owns it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL, @p tag is
 *                                 NULL/empty, or an optional pointer field is
 *                                 inconsistent.
 * @retval OCCTL_NOT_FOUND         @c options->root is invalid or removed.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version unsupported.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_graph_tag_add, occtl_select_iter_next, occtl_select_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_select_tagged_iter_create(occtl_graph_t*                graph,
                                  const occtl_select_options_t* options,
                                  const char*                   tag,
                                  size_t                        tagLen,
                                  occtl_select_iter_t**         out_iter);

/**
 * Advances a selection iterator.
 *
 * @param[in,out] iter     Borrows it.  Must be non-NULL.
 * @param[out]    out_node Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                Next selected node written.
 * @retval OCCTL_NOT_FOUND         Iterator exhausted; @p out_node is invalid.
 * @retval OCCTL_INVALID_ARGUMENT  @p iter or @p out_node is NULL.
 *
 * @threadsafe No (mutates iterator).
 *
 * @sa occtl_select_iter_create, occtl_select_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_select_iter_next(occtl_select_iter_t* iter,
                                                           occtl_node_id_t*     out_node);

/**
 * Releases a selection iterator.
 *
 * @param[in] iter Owns it.  May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_select_iter_next
 */
OCCTL_API void OCCTL_CALL occtl_select_iter_free(occtl_select_iter_t* iter);

/**
 * Creates an iterator over grouped selector results.
 *
 * @param[in,out] graph         Borrows it.  Must be non-NULL.
 * @param[in]     select_options Borrows it.  May be NULL for defaults.
 * @param[in]     group_options Borrows it.  May be NULL for defaults.
 * @param[out]    out_iter      Owns it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL, or an
 *                                 option value is invalid.
 * @retval OCCTL_NOT_FOUND         @c select_options->root is invalid or removed.
 * @retval OCCTL_VERSION_MISMATCH  An options struct version is unsupported.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_select_group_iter_next, occtl_select_group_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_select_group_iter_create(occtl_graph_t*                      graph,
                                 const occtl_select_options_t*       select_options,
                                 const occtl_select_group_options_t* group_options,
                                 occtl_select_group_iter_t**         out_iter);

/**
 * Advances a grouped-selection iterator.
 *
 * @param[in,out] iter     Borrows it.  Must be non-NULL.
 * @param[out]    out_view Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                Next group view written.
 * @retval OCCTL_NOT_FOUND         Iterator exhausted.
 * @retval OCCTL_INVALID_ARGUMENT  @p iter or @p out_view is NULL.
 *
 * @threadsafe No (mutates iterator).
 *
 * @sa occtl_select_group_iter_create, occtl_select_group_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_select_group_iter_next(occtl_select_group_iter_t* iter,
                               occtl_select_group_view_t* out_view);

/**
 * Releases a grouped-selection iterator.
 *
 * @param[in] iter Owns it.  May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_select_group_iter_create
 */
OCCTL_API void OCCTL_CALL occtl_select_group_iter_free(occtl_select_group_iter_t* iter);

/**
 * Deep-clones a topology graph.
 *
 * Copies all active nodes, references, representations, and registered
 * layers.  Geometry handles are deep-copied so the new graph is fully
 * independent.  The caller owns the returned graph and must release it
 * with #occtl_graph_free.
 *
 * @param[in]  source     Borrows it.  Must be non-NULL.
 * @param[out] out_graph   Owns it.  Must be non-NULL.  On success
 *                           receives a valid handle; on failure set to NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p source or @p out_graph is NULL.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (reads source, but source must not be concurrently mutated).
 *
 * @sa occtl_graph_create, occtl_graph_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_clone(const occtl_graph_t* source,
                                                      occtl_graph_t**      out_graph);

/**
 * Compacts the graph by reclaiming slots from removed nodes.
 *
 * After compaction all #occtl_node_id_t and #occtl_ref_id_t values are
 * invalidated (they are session-local indices that may be renumbered).
 * Persistent #occtl_uid_t values survive compaction and can be resolved
 * back to new NodeIds via #occtl_graph_node_id_from_uid.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 *
 * @note After this call, all previously obtained NodeId and RefId values
 *       are invalid.  Resolve UIDs to obtain fresh NodeId values.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_graph_node_id_from_uid
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_compact(occtl_graph_t* graph);

/**
 * Removes a node and reparents its children to a replacement node.
 *
 * For edge nodes: all CoEdges referencing the removed edge are
 * reparented to the replacement edge.  Metadata observers are notified with both
 * old and replacement NodeIds for data migration.
 *
 * @param[in,out] graph       Borrows it.  Must be non-NULL.
 * @param[in]     node        NodeId of the entity to remove.
 * @param[in]     replacement NodeId of the replacement entity.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p node or @p replacement is invalid or removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_remove
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove_with_replacement(occtl_graph_t*  graph,
                                                                       occtl_node_id_t node,
                                                                       occtl_node_id_t replacement);

/**
 * Removes a reference entry by its RefId.
 *
 * This is the builder-level API for detaching a child usage from its
 * parent without removing the referenced definition itself.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     ref_id  Reference entry to remove.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p ref_id is invalid or already removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_remove_rep
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove_ref(occtl_graph_t* graph,
                                                          occtl_ref_id_t ref_id);

/**
 * Removes a representation entry by its RepId.
 *
 * Owning topology entities are marked modified so generation-based
 * caches observe the representation as absent.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     rep_id  Representation entry to remove.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p rep_id is invalid or already removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_remove_ref
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove_rep(occtl_graph_t* graph,
                                                          occtl_rep_id_t rep_id);

/**
 * Cleans up stale references after removal operations.
 *
 * Forces the graph to finalise deferred invalidation and validate
 * internal consistency.  Idempotent — calling multiple times is safe
 * and has no additional side effects.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 *
 * @threadsafe No (mutates graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_cleanup_removed_refs(occtl_graph_t* graph);

/**
 * Rebinds an edge's start vertex to a different vertex definition.
 *
 * Rewires the existing start-vertex reference to point to @p vertex.
 * The previous vertex definition is no longer referenced by this edge.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     edge   Edge node ID.  Must be a valid, active edge.
 * @param[in]     vertex Vertex node ID.  Must be a valid, active vertex.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge or @p vertex is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge or @p vertex is not a vertex.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_edge_end_vertex
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_start_vertex(occtl_graph_t*  graph,
                                                                     occtl_node_id_t edge,
                                                                     occtl_node_id_t vertex);

/**
 * Rebinds an edge's end vertex to a different vertex definition.
 *
 * @copydetails occtl_topo_set_edge_start_vertex
 *
 * @sa occtl_topo_set_edge_start_vertex
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_end_vertex(occtl_graph_t*  graph,
                                                                   occtl_node_id_t edge,
                                                                   occtl_node_id_t vertex);

/**
 * Sets the orientation of a reference entry.
 *
 * Dispatches internally by RefId kind.  Supported ref kinds: Shell, Face,
 * Wire, CoEdge, Vertex, Solid, Child.
 *
 * @param[in,out] graph       Borrows it.  Must be non-NULL.
 * @param[in]     ref_id       Reference entry to modify.
 * @param[in]     orientation New orientation value.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p ref_id is invalid or removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_ref_location
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_ref_orientation(occtl_graph_t*      graph,
                                                                   occtl_ref_id_t      ref_id,
                                                                   occtl_orientation_t orientation);

/**
 * Sets the local location of a reference entry.
 *
 * Dispatches internally by RefId kind.  Supported ref kinds: Shell, Face,
 * Wire, CoEdge, Vertex, Solid, Child, Occurrence.
 *
 * @param[in,out] graph     Borrows it.  Must be non-NULL.
 * @param[in]     ref_id     Reference entry to modify.
 * @param[in]     transform New local location as an affine transform.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p ref_id is invalid or removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_ref_orientation
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_ref_location(occtl_graph_t*    graph,
                                                                occtl_ref_id_t    ref_id,
                                                                occtl_transform_t transform);

/**
 * Sets the IsOuter flag on a wire reference (marks it as the outer wire
 * on a face).
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     ref_id  Wire reference entry to modify.
 * @param[in]     flag   1 to mark as outer wire, 0 to clear.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p ref_id is invalid, removed, or not a wire ref.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_set_ref_orientation
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_wire_ref_is_outer(occtl_graph_t* graph,
                                                                     occtl_ref_id_t ref_id,
                                                                     int32_t        flag);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_TOPO_BUILD_H */
