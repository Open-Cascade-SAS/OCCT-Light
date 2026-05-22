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
 * @file occtl_topo_relation.h
 * @brief OCCT-Light: Topology relation queries, hit tests, and contact analysis.
 *
 * Defines enums, structs, iterators, and functions for computing spatial
 * relationships between topology entities: distance pairs, adjacency,
 * connectivity, axis hits, touch points, intersection tests, and geometric
 * classification.
 */

#ifndef OCCTL_TOPO_RELATION_H
#define OCCTL_TOPO_RELATION_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_geom.h"
#include "occtl_topo_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Shape continuity level, mirroring GeomAbs_Shape.
 *
 * Describes the geometric continuity at an edge between two
 * adjacent faces.
 */
typedef enum occtl_shape_continuity
{
  OCCTL_CONTINUITY_C0              = 0, /**< Positional continuity only. */
  OCCTL_CONTINUITY_C1              = 1, /**< First-derivative continuity. */
  OCCTL_CONTINUITY_C2              = 2, /**< Second-derivative continuity. */
  OCCTL_CONTINUITY_C3              = 3, /**< Third-derivative continuity. */
  OCCTL_CONTINUITY_CN              = 4, /**< Infinite (analytical) continuity. */
  OCCTL_CONTINUITY_G1              = 5, /**< Tangent (geometric) continuity. */
  OCCTL_CONTINUITY_G2              = 6, /**< Curvature (geometric) continuity. */
  OCCTL_CONTINUITY_RESERVED_FUTURE = 0x7fffffff
} occtl_shape_continuity_t;

/**
 * Orientation used when placing a child entity inside its parent.
 *
 * Values mirror TopAbs_Orientation 1:1 in numeric order so
 * conversion is a static_cast (with a sanity assert in debug).
 */
typedef enum occtl_orientation
{
  OCCTL_ORIENTATION_FORWARD         = 0, /**< Forward orientation. */
  OCCTL_ORIENTATION_REVERSED        = 1, /**< Reversed orientation. */
  OCCTL_ORIENTATION_INTERNAL        = 2, /**< Internal orientation. */
  OCCTL_ORIENTATION_EXTERNAL        = 3, /**< External orientation. */
  OCCTL_ORIENTATION_RESERVED_FUTURE = 0x7fffffff
} occtl_orientation_t;

/**
 * (NodeId, orientation) pair used in builder info structs and ordered
 * traversal results to specify a child entity and its parent orientation.
 *
 * Passed by value; embedded in spans inside info structs.  Not versioned.
 */
typedef struct occtl_oriented_node
{
  occtl_node_id_t     id;          /**< Child node ID. */
  occtl_orientation_t orientation; /**< Child orientation. */
} occtl_oriented_node_t;

/**
 * Lists a wire's edges in endpoint-chaining order.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to
 * learn the required count in @p out_count, then reissue with a buffer
 * of at least that many #occtl_oriented_node_t entries.
 *
 * Each returned entry contains an Edge node ID and the coedge orientation of
 * that edge inside @p wire.  The order matches #occtl_topo_wire_explorer_create.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  wire      Wire node ID.
 * @param[out] out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]  cap       Capacity of @p out_buf in entries.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND        @p wire is invalid or removed.
 * @retval OCCTL_WRONG_KIND       @p wire is not a wire.
 * @retval OCCTL_BUFFER_TOO_SMALL @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_wire_explorer_create, occtl_topo_make_wire
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_order_edges(const occtl_graph_t*   graph,
                                                                occtl_node_id_t        wire,
                                                                occtl_oriented_node_t* out_buf,
                                                                size_t                 cap,
                                                                size_t*                out_count);

/**
 * Opaque iterator over accumulated-location/orientation graph traversal.
 *
 * Created by #occtl_topo_child_explorer_create and
 * #occtl_topo_parent_explorer_create.  Each yield includes the current
 * node's definition ID, accumulated transform, and accumulated orientation.
 * Release with #occtl_topo_explorer_iter_free.
 *
 * @threadsafe No — single-owner; concurrent reads on distinct iterators over
 *             the same graph are safe when the graph is not mutated.
 *
 * @sa occtl_topo_child_explorer_create, occtl_topo_parent_explorer_create
 */
typedef struct occtl_topo_explorer_iter occtl_topo_explorer_iter_t;

/**
 * Traversal mode for child and parent explorers.
 */
typedef enum occtl_topo_explorer_traversal
{
  OCCTL_TOPO_EXPLORER_RECURSIVE                 = 0, /**< Recursive depth-first traversal. */
  OCCTL_TOPO_EXPLORER_DIRECT_CHILDREN           = 1, /**< Direct children only. */
  OCCTL_TOPO_EXPLORER_TRAVERSAL_RESERVED_FUTURE = 0x7fffffff
} occtl_topo_explorer_traversal_t;

#define OCCTL_TOPO_CHILD_EXPLORER_CONFIG_VERSION_1 1u

/**
 * Configuration for #occtl_topo_child_explorer_create.
 */
typedef struct occtl_topo_child_explorer_config
{
  uint32_t    struct_version;           /**< Must be #OCCTL_TOPO_CHILD_EXPLORER_CONFIG_VERSION_1. */
  const void* p_next;                   /**< Reserved; set to NULL. */
  occtl_topo_explorer_traversal_t mode; /**< Traversal depth. */
  occtl_node_kind_t
    target_kind; /**< Only visit nodes of this kind, or #OCCTL_KIND_INVALID for all. */
  occtl_node_kind_t
          avoid_kind;      /**< Do not enter subtrees under this kind, or #OCCTL_KIND_INVALID. */
  int32_t emit_avoid_kind; /**< 1 to emit the avoid-kind node itself (not its children). */
  int32_t
    accumulate_location; /**< 1 to accumulate Locations into the output transform (default). */
  int32_t accumulate_orientation; /**< 1 to accumulate Orientations into the output orientation
                                     (default). */
} occtl_topo_child_explorer_config_t;

#define OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT                                                      \
  {OCCTL_TOPO_CHILD_EXPLORER_CONFIG_VERSION_1,                                                     \
   NULL,                                                                                           \
   OCCTL_TOPO_EXPLORER_RECURSIVE,                                                                  \
   OCCTL_KIND_INVALID,                                                                             \
   OCCTL_KIND_INVALID,                                                                             \
   0,                                                                                              \
   1,                                                                                              \
   1}

/**
 * Initialises @p config to default values via #OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] config Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_child_explorer_create
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_child_explorer_config_init(occtl_topo_child_explorer_config_t* config);

/**
 * Creates a downward depth-first child explorer starting from @p root.
 *
 * The explorer accumulates location and orientation along reference
 * chains and yields each visited node with its accumulated transform
 * and orientation.  Use #occtl_topo_explorer_iter_next to step and
 * #occtl_topo_explorer_iter_free to release.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  root        Root node ID for the traversal.
 * @param[in]  config      Borrows it.  May be NULL for defaults (recursive,
 *                         all kinds, location+orientation accumulated).
 * @param[out] out_iter    Owns it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL, or
 *                                 @c config->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p root is invalid or removed.
 * @retval OCCTL_VERSION_MISMATCH  @c config->struct_version unsupported.
 * @retval OCCTL_OUT_OF_RANGE      @c config->mode, @c config->target_kind, or
 *                                 @c config->avoid_kind is not valid.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_explorer_iter_next, occtl_topo_parent_explorer_create
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_child_explorer_create(const occtl_graph_t*                      graph,
                                   occtl_node_id_t                           root,
                                   const occtl_topo_child_explorer_config_t* config,
                                   occtl_topo_explorer_iter_t**              out_iter);

#define OCCTL_TOPO_PARENT_EXPLORER_CONFIG_VERSION_1 1u

/**
 * Configuration for #occtl_topo_parent_explorer_create.
 */
typedef struct occtl_topo_parent_explorer_config
{
  uint32_t    struct_version; /**< Must be #OCCTL_TOPO_PARENT_EXPLORER_CONFIG_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  occtl_topo_explorer_traversal_t mode; /**< Traversal depth. */
  occtl_node_kind_t
    target_kind; /**< Only visit nodes of this kind, or #OCCTL_KIND_INVALID for all. */
  occtl_node_kind_t
          avoid_kind;      /**< Do not enter subtrees under this kind, or #OCCTL_KIND_INVALID. */
  int32_t emit_avoid_kind; /**< 1 to emit the avoid-kind node itself (not its children). */
} occtl_topo_parent_explorer_config_t;

#define OCCTL_TOPO_PARENT_EXPLORER_CONFIG_INIT                                                     \
  {OCCTL_TOPO_PARENT_EXPLORER_CONFIG_VERSION_1,                                                    \
   NULL,                                                                                           \
   OCCTL_TOPO_EXPLORER_RECURSIVE,                                                                  \
   OCCTL_KIND_INVALID,                                                                             \
   OCCTL_KIND_INVALID,                                                                             \
   0}

/**
 * Initialises @p config to default values via #OCCTL_TOPO_PARENT_EXPLORER_CONFIG_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] config Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_parent_explorer_create
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_parent_explorer_config_init(occtl_topo_parent_explorer_config_t* config);

/**
 * Creates an upward parent explorer starting from @p node.
 *
 * The explorer walks the reverse reference indices upward from the
 * starting node, accumulating location and orientation along each
 * parent chain.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  node        Starting node ID.
 * @param[in]  config      Borrows it.  May be NULL for defaults.
 * @param[out] out_iter    Owns it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL, or
 *                                 @c config->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_VERSION_MISMATCH  @c config->struct_version unsupported.
 * @retval OCCTL_OUT_OF_RANGE      @c config->mode, @c config->target_kind, or
 *                                 @c config->avoid_kind is not valid.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_explorer_iter_next, occtl_topo_child_explorer_create
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_parent_explorer_create(const occtl_graph_t*                       graph,
                                    occtl_node_id_t                            node,
                                    const occtl_topo_parent_explorer_config_t* config,
                                    occtl_topo_explorer_iter_t**               out_iter);

/**
 * Advances the explorer iterator to the next node.
 *
 * Writes the current node's definition ID, accumulated transform,
 * and accumulated orientation.
 *
 * @param[in,out] iter            Borrows it.  Must be non-NULL.
 * @param[out]    out_node        Borrows it.  Must be non-NULL.
 * @param[out]    out_transform   Borrows it.  Must be non-NULL.
 * @param[out]    out_orientation Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                Next value written.
 * @retval OCCTL_NOT_FOUND         Exhausted; out params set to invalid/identity.
 * @retval OCCTL_INVALID_ARGUMENT  An out-param is NULL.
 *
 * @threadsafe No (mutates iter; concurrent reads on distinct iters OK).
 *
 * @sa occtl_topo_child_explorer_create, occtl_topo_parent_explorer_create,
 *     occtl_topo_explorer_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_explorer_iter_next(occtl_topo_explorer_iter_t* iter,
                                occtl_node_id_t*            out_node,
                                occtl_transform_t*          out_transform,
                                occtl_orientation_t*        out_orientation);

/**
 * Releases an explorer iterator.
 *
 * @param[in] iter Owns it.  NULL is a no-op.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_explorer_iter_next
 */
OCCTL_API void OCCTL_CALL occtl_topo_explorer_iter_free(occtl_topo_explorer_iter_t* iter);

/**
 * Opaque iterator over a node's semantic neighbours and their relation kinds.
 *
 * Created by #occtl_topo_related_iter_create.  Release with
 * #occtl_topo_related_iter_free.
 *
 * @threadsafe No — single-owner; concurrent reads on distinct iterators over
 *             the same graph are safe when the graph is not mutated.
 */
typedef struct occtl_topo_related_iter occtl_topo_related_iter_t;

/**
 * Opaque iterator over axis/face intersection hits.
 *
 * Created by #occtl_topo_axis_intersect_faces and released with
 * #occtl_topo_axis_hit_iter_free.
 *
 * @threadsafe No — single-owner; concurrent reads on distinct iterators are
 *             safe when the owning graph is not mutated.
 */
typedef struct occtl_topo_axis_hit_iter occtl_topo_axis_hit_iter_t;

#define OCCTL_TOPO_RELATION_OPTIONS_VERSION_1 1u

/**
 * Options for relation/contact queries.
 *
 * The first version is used by #occtl_topo_touch_iter_create.  Tangent
 * contacts are distance-zero solutions reported by OCCT.  Overlap solutions
 * are included when @c include_overlaps is 1; set it to 0 to suppress OCCT
 * inner-overlap reports.
 */
typedef struct occtl_topo_relation_options
{
  uint32_t    struct_version;           /**< Must be #OCCTL_TOPO_RELATION_OPTIONS_VERSION_1. */
  const void* p_next;                   /**< Reserved for extensions; must be NULL. */
  double      tolerance;                /**< Non-negative contact tolerance. */
  int32_t     include_tangent_contacts; /**< 0/1; include zero-distance tangent contacts. */
  int32_t     include_overlaps;         /**< 0/1; include overlap/inner-solution reports. */
  int32_t     include_lower_dimension_results; /**< 0/1; include vertex/edge support nodes. */
} occtl_topo_relation_options_t;

#define OCCTL_TOPO_RELATION_OPTIONS_INIT                                                           \
  {OCCTL_TOPO_RELATION_OPTIONS_VERSION_1, NULL, 1.0e-7, 1, 1, 1}

/**
 * Runtime initialiser for #occtl_topo_relation_options_t.
 *
 * Sets all fields to #OCCTL_TOPO_RELATION_OPTIONS_INIT.
 *
 * @param[out] options Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_touch_iter_create
 */
OCCTL_API void OCCTL_CALL occtl_topo_relation_options_init(occtl_topo_relation_options_t* options);

/**
 * Opaque iterator over contact solutions between two graph nodes.
 *
 * Created by #occtl_topo_touch_iter_create and released with
 * #occtl_topo_touch_iter_free.
 *
 * @threadsafe No — single-owner; concurrent reads on distinct iterators are
 *             safe when the owning graph is not mutated.
 */
typedef struct occtl_topo_touch_iter occtl_topo_touch_iter_t;

/**
 * Opaque iterator over topology generated by an intersection query.
 *
 * Created by #occtl_topo_intersection_iter_create and released with
 * #occtl_topo_intersection_iter_free.
 *
 * @threadsafe No — single-owner; concurrent reads on distinct iterators are
 *             safe when the owning graph is not mutated.
 */
typedef struct occtl_topo_intersection_iter occtl_topo_intersection_iter_t;

/**
 * Semantic relation between two nodes in the graph.
 */
typedef enum occtl_relation_kind
{
  OCCTL_RELATION_BOUNDARY_EDGE        = 0, /**< Boundary edge of a face. */
  OCCTL_RELATION_ADJACENT_FACE        = 1, /**< Face adjacent via shared edge. */
  OCCTL_RELATION_OUTER_WIRE           = 2, /**< Outer wire of a face. */
  OCCTL_RELATION_REFERENCED_BY        = 3, /**< Face referenced by this coedge. */
  OCCTL_RELATION_INCIDENT_VERTEX      = 4, /**< Vertex incident to this edge. */
  OCCTL_RELATION_WIRE_COEDGE          = 5, /**< Coedge belonging to this wire. */
  OCCTL_RELATION_OWNING_FACE          = 6, /**< Face owning this coedge/wire. */
  OCCTL_RELATION_INCIDENT_EDGE        = 7, /**< Edge incident to this vertex. */
  OCCTL_RELATION_PARENT_EDGE          = 8, /**< Parent edge of this coedge. */
  OCCTL_RELATION_SEAM_PAIR            = 9, /**< Seam-pair coedge. */
  OCCTL_RELATION_KIND_RESERVED_FUTURE = 0x7fffffff
} occtl_relation_kind_t;

/**
 * Point classification result for graph topology.
 */
typedef enum occtl_topo_point_class
{
  OCCTL_TOPO_POINT_CLASS_IN              = 0, /**< Point is inside the target. */
  OCCTL_TOPO_POINT_CLASS_OUT             = 1, /**< Point is outside the target. */
  OCCTL_TOPO_POINT_CLASS_ON              = 2, /**< Point lies on the target boundary. */
  OCCTL_TOPO_POINT_CLASS_UNKNOWN         = 3, /**< Classification failed or is undefined. */
  OCCTL_TOPO_POINT_CLASS_RESERVED_FUTURE = 0x7fffffff
} occtl_topo_point_class_t;

/**
 * Closest-distance result between two graph nodes.
 *
 * Support nodes are best-effort mappings from OCCT's support shapes back to
 * graph node IDs.  They are #OCCTL_NODE_ID_INVALID when OCCT reports a
 * support that is not represented by an active graph node.
 */
typedef struct occtl_topo_distance_pair
{
  double          distance;       /**< Minimum distance. */
  occtl_point3_t  point_a;        /**< Closest point on the first node. */
  occtl_point3_t  point_b;        /**< Closest point on the second node. */
  occtl_node_id_t support_a;      /**< Support node on the first input, or invalid. */
  occtl_node_id_t support_b;      /**< Support node on the second input, or invalid. */
  int32_t         inner_solution; /**< 1 if one shape is inside the other, 0 otherwise. */
  int32_t         solution_count; /**< Number of closest solutions reported by OCCT. */
} occtl_topo_distance_pair_t;

/**
 * One intersection between an axis and a face under a graph root.
 */
typedef struct occtl_topo_axis_hit
{
  occtl_node_id_t          face;      /**< Face node hit by the axis. */
  occtl_point3_t           point;     /**< 3D hit point. */
  occtl_point2_t           uv;        /**< Surface parameters on @c face. */
  double                   parameter; /**< Signed axis parameter from the axis location. */
  occtl_direction3_t       normal;    /**< Oriented face normal at @c uv when @c has_normal != 0. */
  int32_t                  has_normal; /**< 1 when @c normal is valid, 0 otherwise. */
  occtl_topo_point_class_t location;   /**< OCCT face-state classification for the hit. */
} occtl_topo_axis_hit_t;

/**
 * One contact solution between two graph nodes.
 *
 * Support node IDs are best-effort mappings from OCCT support shapes back to
 * original graph nodes.  They may be #OCCTL_NODE_ID_INVALID when OCCT reports
 * a support that is not represented directly by an active graph node.
 */
typedef struct occtl_topo_touch_hit
{
  occtl_node_id_t node_a;         /**< First queried node. */
  occtl_node_id_t node_b;         /**< Second queried node. */
  occtl_node_id_t support_a;      /**< Support node on @c node_a, or invalid. */
  occtl_node_id_t support_b;      /**< Support node on @c node_b, or invalid. */
  occtl_point3_t  point_a;        /**< Contact / closest point on @c node_a. */
  occtl_point3_t  point_b;        /**< Contact / closest point on @c node_b. */
  double          distance;       /**< OCCT distance for this solution. */
  int32_t         inner_solution; /**< 1 when OCCT reports an overlap/inner solution. */
} occtl_topo_touch_hit_t;

/**
 * Creates an iterator over the semantic neighbours of @p node.
 *
 * Neighbours are definition IDs without accumulated location or
 * orientation.  Each yield includes a #occtl_relation_kind_t that
 * describes the semantic relationship.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  node     Node to query neighbours for.
 * @param[out] out_iter Owns it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_NOT_FOUND         @p node is invalid or removed.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_related_iter_next, occtl_topo_related_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_related_iter_create(const occtl_graph_t*        graph,
                                 occtl_node_id_t             node,
                                 occtl_topo_related_iter_t** out_iter);

/**
 * Computes the closest-distance pair between two graph nodes.
 *
 * The calculation is delegated to OCCT and returns the minimum distance,
 * closest points, and best-effort support-node IDs for the first closest
 * solution.
 *
 * @param[in]  graph    Borrows it.  Must be non-NULL.
 * @param[in]  node_a   First node to query.
 * @param[in]  node_b   Second node to query.
 * @param[out] out_pair Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_pair is NULL.
 * @retval OCCTL_NOT_FOUND        @p node_a or @p node_b is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID OCCT could not compute a closest solution.
 *
 * @threadsafe Yes (read-only on graph; may populate internal shape caches).
 *
 * @sa occtl_topo_related_iter_create, occtl_graph_pair_distance_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_distance_pair(const occtl_graph_t*        graph,
                                                             occtl_node_id_t             node_a,
                                                             occtl_node_id_t             node_b,
                                                             occtl_topo_distance_pair_t* out_pair);

/**
 * Finds the closest point on a graph node to a world-space point.
 *
 * The graph is not mutated. The returned point lies on @p node, and
 * @p out_distance receives the world-space distance from @p point when
 * requested.
 *
 * @param[in]  graph        Borrows it.  Must be non-NULL.
 * @param[in]  node         Node to search.
 * @param[in]  point        Query point.
 * @param[out] out_closest  Borrows it.  Must be non-NULL.  Receives the closest point on @p node.
 * @param[out] out_distance Borrows it.  May be NULL.  Receives the minimum distance.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_closest is NULL.
 * @retval OCCTL_NOT_FOUND        @p node is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID OCCT could not compute a closest solution.
 *
 * @threadsafe Yes (read-only on graph; may populate internal shape caches).
 *
 * @sa occtl_topo_distance_pair, occtl_graph_pair_distance_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_closest_point_to_point(const occtl_graph_t* graph,
                                                                      occtl_node_id_t      node,
                                                                      occtl_point3_t       point,
                                                                      occtl_point3_t* out_closest,
                                                                      double*         out_distance);

/**
 * Creates an iterator over faces intersected by an axis under @p root.
 *
 * The calculation is delegated to OCCT's face/curve intersector.  Hits are
 * ordered by signed axis parameter and include the hit point, UV parameters,
 * and a best-effort oriented surface normal.
 *
 * @param[in]  graph         Borrows it.  Must be non-NULL.
 * @param[in]  root          Root shape node whose faces are tested.
 * @param[in]  axis          Directed axis used as the query line.
 * @param[in]  min_parameter Inclusive lower signed axis parameter.
 * @param[in]  max_parameter Inclusive upper signed axis parameter.
 * @param[in]  tolerance     Intersection tolerance.  Must be non-negative.
 * @param[out] out_iter      Owns it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_iter is NULL,
 *                                @p tolerance is negative, the parameter
 *                                interval is invalid, or @p axis direction is
 *                                zero/non-finite.
 * @retval OCCTL_NOT_FOUND        @p root is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID OCCT could not complete the intersection.
 * @retval OCCTL_OUT_OF_MEMORY    Allocation failed.
 *
 * @threadsafe Yes (read-only on graph; uses local OCCT intersector state).
 *
 * @sa occtl_topo_axis_hit_iter_next, occtl_topo_axis_hit_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_axis_intersect_faces(const occtl_graph_t*         graph,
                                  occtl_node_id_t              root,
                                  occtl_axis1_placement_t      axis,
                                  double                       min_parameter,
                                  double                       max_parameter,
                                  double                       tolerance,
                                  occtl_topo_axis_hit_iter_t** out_iter);

/**
 * Creates an iterator over contact solutions between two graph nodes.
 *
 * The calculation is delegated to OCCT `BRepExtrema_DistShapeShape`.  When the
 * minimum distance is greater than the requested tolerance, the iterator is
 * valid but empty.  Results are original-graph support nodes plus the OCCT
 * contact / closest points; the queried graph is not mutated.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  node_a   First node to query.
 * @param[in]  node_b   Second node to query.
 * @param[in]  options  Borrows it. May be NULL for defaults.
 * @param[out] out_iter Owns it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL, or
 *                                 @p options contains an invalid @c p_next,
 *                                 tolerance, or 0/1 flag.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported
 *                                 @c struct_version.
 * @retval OCCTL_NOT_FOUND         @p node_a or @p node_b is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not compute distance/contact
 *                                 solutions.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph; may populate internal shape caches).
 *
 * @sa occtl_topo_touch_iter_next, occtl_topo_touch_iter_free,
 *     occtl_topo_distance_pair
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_touch_iter_create(const occtl_graph_t*                 graph,
                               occtl_node_id_t                      node_a,
                               occtl_node_id_t                      node_b,
                               const occtl_topo_relation_options_t* options,
                               occtl_topo_touch_iter_t**            out_iter);

/**
 * Creates an iterator over topology generated by intersecting two graph nodes.
 *
 * The calculation is delegated to OCCT section/common algorithms.  Generated
 * intersection topology is inserted into @p graph as fresh topology roots so
 * returned NodeIds are meaningful in the caller's graph.  Section edges and
 * vertices are returned first.  When @p options is NULL or
 * @c include_overlaps is 1, an OCCT common result is also used to return
 * overlap faces when the two inputs have area/volume overlap.  The iterator is
 * valid but empty when no requested intersection topology is produced.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     node_a   First node to intersect.
 * @param[in]     node_b   Second node to intersect.
 * @param[in]     options  Borrows it. May be NULL for defaults.
 * @param[out]    out_iter Owns it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL, or
 *                                 @p options contains an invalid @c p_next,
 *                                 tolerance, or 0/1 flag.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported
 *                                 @c struct_version.
 * @retval OCCTL_NOT_FOUND         @p node_a or @p node_b is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not compute the intersection.
 * @retval OCCTL_TOPOLOGY_INVALID  the generated topology could not be inserted.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (may mutate @p graph by inserting generated topology).
 *
 * @sa occtl_topo_intersection_iter_next, occtl_topo_intersection_iter_free,
 *     occtl_topo_touch_iter_create
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_intersection_iter_create(occtl_graph_t*                       graph,
                                      occtl_node_id_t                      node_a,
                                      occtl_node_id_t                      node_b,
                                      const occtl_topo_relation_options_t* options,
                                      occtl_topo_intersection_iter_t**     out_iter);

/**
 * Tests whether two edge or face nodes share the same geometric support.
 *
 * This is a geometry predicate, not topological identity: two different
 * bounded edges/faces may compare equal when OCCT reports coincident support
 * within @p tolerance.  Node kinds must match and must be Edge or Face.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  node_a    First edge or face node.
 * @param[in]  node_b    Second edge or face node.
 * @param[in]  tolerance Linear tolerance.  Must be non-negative.
 * @param[out] out_is_same_geometry  Borrows it.  Receives 1 for same geometry, 0 otherwise.
 *                                   Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_same_geometry is NULL, or
 *                                @p tolerance is negative/non-finite.
 * @retval OCCTL_NOT_FOUND        @p node_a or @p node_b is invalid or removed.
 * @retval OCCTL_WRONG_KIND       Nodes are not both edges or both faces.
 *
 * @threadsafe Yes (read-only on graph; may use local OCCT projection state).
 *
 * @sa occtl_topo_distance_pair, occtl_topo_axis_intersect_faces
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_is_same_geometry(const occtl_graph_t* graph,
                                                                occtl_node_id_t      node_a,
                                                                occtl_node_id_t      node_b,
                                                                double               tolerance,
                                                                int32_t* out_is_same_geometry);

/**
 * Lists vertex nodes common to two graph roots.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to
 * learn the required count in @p out_count, then reissue with a buffer
 * of at least that many #occtl_node_id_t entries.
 *
 * The order follows the first root's OCCT vertex traversal order.  Duplicate
 * vertex uses are collapsed.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  node_a    First root node.
 * @param[in]  node_b    Second root node.
 * @param[out] out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]  cap       Capacity of @p out_buf in entries.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND        @p node_a or @p node_b is invalid or removed.
 * @retval OCCTL_BUFFER_TOO_SMALL @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe No (may update OCCT internal caches).
 *
 * @sa occtl_graph_descendant_vertices_get, occtl_topo_related_iter_create
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_common_vertices(const occtl_graph_t* graph,
                                                               occtl_node_id_t      node_a,
                                                               occtl_node_id_t      node_b,
                                                               occtl_node_id_t*     out_buf,
                                                               size_t               cap,
                                                               size_t*              out_count);

/**
 * Lists edges adjacent to @p edge through shared vertices.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to
 * learn the required count in @p out_count, then reissue with a buffer
 * of at least that many #occtl_node_id_t entries.
 *
 * The input edge itself is not included.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  edge      Edge node ID.
 * @param[out] out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]  cap       Capacity of @p out_buf in entries.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND        @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND       @p edge is not an edge.
 * @retval OCCTL_BUFFER_TOO_SMALL @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_connected_edges, occtl_topo_adjacent_faces
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_adjacent_edges(const occtl_graph_t* graph,
                                                              occtl_node_id_t      edge,
                                                              occtl_node_id_t*     out_buf,
                                                              size_t               cap,
                                                              size_t*              out_count);

/**
 * Lists faces adjacent to @p face through shared edges.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to
 * learn the required count in @p out_count, then reissue with a buffer
 * of at least that many #occtl_node_id_t entries.
 *
 * The input face itself is not included.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  face      Face node ID.
 * @param[out] out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]  cap       Capacity of @p out_buf in entries.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND        @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND       @p face is not a face.
 * @retval OCCTL_BUFFER_TOO_SMALL @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_connected_faces, occtl_topo_adjacent_edges
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_adjacent_faces(const occtl_graph_t* graph,
                                                              occtl_node_id_t      face,
                                                              occtl_node_id_t*     out_buf,
                                                              size_t               cap,
                                                              size_t*              out_count);

/**
 * Lists all edges connected to @p seed_edge through shared vertices.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to
 * learn the required count in @p out_count, then reissue with a buffer
 * of at least that many #occtl_node_id_t entries.
 *
 * The seed edge is included in the result.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  seed_edge Edge where the walk starts.
 * @param[out] out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]  cap       Capacity of @p out_buf in entries.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND        @p seed_edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND       @p seed_edge is not an edge.
 * @retval OCCTL_BUFFER_TOO_SMALL @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_connected_faces, occtl_topo_common_vertices
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_connected_edges(const occtl_graph_t* graph,
                                                               occtl_node_id_t      seed_edge,
                                                               occtl_node_id_t*     out_buf,
                                                               size_t               cap,
                                                               size_t*              out_count);

/**
 * Lists all faces connected to @p seed_face through shared edges.
 *
 * Two-call buffer (§10.1): pass @p out_buf as NULL with @p cap = 0 to
 * learn the required count in @p out_count, then reissue with a buffer
 * of at least that many #occtl_node_id_t entries.
 *
 * The seed face is included in the result.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  seed_face Face where the walk starts.
 * @param[out] out_buf   Borrows it.  May be NULL on the sizing call.
 * @param[in]  cap       Capacity of @p out_buf in entries.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND        @p seed_face is invalid or removed.
 * @retval OCCTL_WRONG_KIND       @p seed_face is not a face.
 * @retval OCCTL_BUFFER_TOO_SMALL @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_connected_edges, occtl_topo_common_vertices
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_connected_faces(const occtl_graph_t* graph,
                                                               occtl_node_id_t      seed_face,
                                                               occtl_node_id_t*     out_buf,
                                                               size_t               cap,
                                                               size_t*              out_count);

/**
 * Computes same-kind topological graph distance under a shared root.
 *
 * Distance is the minimum number of adjacency hops from any source node to
 * @p target while walking only peers of the same kind reachable from @p root.
 * Supported peer kinds are Vertex, Edge, Wire, Face, Shell, and Solid.
 * Vertex/Edge/Wire peers connect through shared vertices; Face peers connect
 * through shared edges; Shell/Solid peers connect through shared faces.
 *
 * A source node has distance 0 from itself.  If @p target is reachable from
 * @p root but not connected to any source peer, the function succeeds and
 * writes -1 to @p out_distance.
 *
 * @param[in]  graph        Borrows it.  Must be non-NULL.
 * @param[in]  root         Root that defines the peer set.  Must be active.
 * @param[in]  sources      Borrows it.  Array of source nodes.  Must be
 *                          non-NULL and contain @p source_count entries.
 * @param[in]  source_count Number of entries in @p sources.  Must be non-zero.
 * @param[in]  target       Target node.  Must have the same kind as all sources.
 * @param[out] out_distance Borrows it.  Must be non-NULL.  Receives the hop
 *                          count, or -1 when unreachable within the peer graph.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p sources, or @p out_distance is
 *                                 NULL, or @p source_count is zero.
 * @retval OCCTL_NOT_FOUND         @p root, @p target, or a source is invalid,
 *                                 removed, or not reachable from @p root.
 * @retval OCCTL_WRONG_KIND        Sources and target are not the same kind, or
 *                                 the kind is not supported for this query.
 *
 * @threadsafe No (may populate graph caches for descendant vertices).
 *
 * @sa occtl_topo_connected_edges, occtl_topo_connected_faces,
 *     occtl_topo_common_vertices
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_graph_distance(const occtl_graph_t*   graph,
                                                              occtl_node_id_t        root,
                                                              const occtl_node_id_t* sources,
                                                              size_t                 source_count,
                                                              occtl_node_id_t        target,
                                                              int32_t*               out_distance);

/**
 * Classifies a 3D point relative to a solid node.
 *
 * Classification is graph-native and delegates to OCCT
 * BRepGraphAlgo_SolidClassifier.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  solid     Solid node to classify against.
 * @param[in]  point     Point to classify.
 * @param[in]  tolerance Tolerance for boundary detection.  Must be non-negative.
 * @param[out] out_class Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_class is NULL, or
 *                                @p tolerance is negative.
 * @retval OCCTL_NOT_FOUND        @p solid is invalid or removed.
 * @retval OCCTL_WRONG_KIND       @p solid is not a solid.
 *
 * @threadsafe Yes (read-only on graph; may build classifier-local caches).
 *
 * @sa occtl_topo_is_inside, occtl_topo_distance_pair
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_classify_point(const occtl_graph_t*      graph,
                                                              occtl_node_id_t           solid,
                                                              occtl_point3_t            point,
                                                              double                    tolerance,
                                                              occtl_topo_point_class_t* out_class);

/**
 * Tests whether a 3D point is inside a solid node.
 *
 * @param[in]  graph            Borrows it.  Must be non-NULL.
 * @param[in]  solid            Solid node to test.
 * @param[in]  point            Point to test.
 * @param[in]  tolerance        Tolerance for boundary detection.  Must be non-negative.
 * @param[in]  include_boundary 1 treats boundary points as inside; 0 does not.
 * @param[out] out_is_inside    Borrows it.  Receives 1 or 0.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_is_inside is NULL,
 *                                @p tolerance is negative, or
 *                                @p include_boundary is not 0 or 1.
 * @retval OCCTL_NOT_FOUND        @p solid is invalid or removed.
 * @retval OCCTL_WRONG_KIND       @p solid is not a solid.
 *
 * @threadsafe Yes (read-only on graph; may build classifier-local caches).
 *
 * @sa occtl_topo_classify_point
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_is_inside(const occtl_graph_t* graph,
                                                         occtl_node_id_t      solid,
                                                         occtl_point3_t       point,
                                                         double               tolerance,
                                                         int32_t              include_boundary,
                                                         int32_t*             out_is_inside);

/**
 * Advances an axis-hit iterator.
 *
 * @param[in,out] iter    Borrows it.  Must be non-NULL.
 * @param[out]    out_hit Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               Next hit written.
 * @retval OCCTL_NOT_FOUND        Iterator exhausted.
 * @retval OCCTL_INVALID_ARGUMENT @p iter or @p out_hit is NULL.
 *
 * @threadsafe No (mutates iterator).
 *
 * @sa occtl_topo_axis_intersect_faces, occtl_topo_axis_hit_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_axis_hit_iter_next(occtl_topo_axis_hit_iter_t* iter,
                                                                  occtl_topo_axis_hit_t* out_hit);

/**
 * Releases an axis-hit iterator.
 *
 * @param[in] iter Owns it.  May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_axis_intersect_faces
 */
OCCTL_API void OCCTL_CALL occtl_topo_axis_hit_iter_free(occtl_topo_axis_hit_iter_t* iter);

/**
 * Advances a touch iterator.
 *
 * @param[in,out] iter    Borrows it. Must be non-NULL.
 * @param[out]    out_hit Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK                Next hit written.
 * @retval OCCTL_NOT_FOUND         Iterator exhausted.
 * @retval OCCTL_INVALID_ARGUMENT  @p iter or @p out_hit is NULL.
 *
 * @threadsafe No (mutates iterator).
 *
 * @sa occtl_topo_touch_iter_create, occtl_topo_touch_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_touch_iter_next(occtl_topo_touch_iter_t* iter,
                                                               occtl_topo_touch_hit_t*  out_hit);

/**
 * Releases a touch iterator.
 *
 * @param[in] iter Owns it. May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_touch_iter_create
 */
OCCTL_API void OCCTL_CALL occtl_topo_touch_iter_free(occtl_topo_touch_iter_t* iter);

/**
 * Advances an intersection iterator.
 *
 * @param[in,out] iter     Borrows it. Must be non-NULL.
 * @param[out]    out_node Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK                Next generated node written.
 * @retval OCCTL_NOT_FOUND         Iterator exhausted.
 * @retval OCCTL_INVALID_ARGUMENT  @p iter or @p out_node is NULL.
 *
 * @threadsafe No (mutates iterator).
 *
 * @sa occtl_topo_intersection_iter_create, occtl_topo_intersection_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_intersection_iter_next(occtl_topo_intersection_iter_t* iter,
                                    occtl_node_id_t*                out_node);

/**
 * Releases an intersection iterator.
 *
 * @param[in] iter Owns it. May be NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_intersection_iter_create
 */
OCCTL_API void OCCTL_CALL occtl_topo_intersection_iter_free(occtl_topo_intersection_iter_t* iter);

/**
 * Advances the related-iterator to the next semantic neighbour.
 *
 * @param[in,out] iter      Borrows it.  Must be non-NULL.
 * @param[out]    out_node   Borrows it.  Must be non-NULL.
 * @param[out]    out_kind   Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                Next value written.
 * @retval OCCTL_NOT_FOUND         Exhausted; out params set to invalid.
 * @retval OCCTL_INVALID_ARGUMENT  An out-param is NULL.
 *
 * @threadsafe No (mutates iter; concurrent reads on distinct iters OK).
 *
 * @sa occtl_topo_related_iter_create, occtl_topo_related_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_related_iter_next(occtl_topo_related_iter_t* iter,
                                                                 occtl_node_id_t*       out_node,
                                                                 occtl_relation_kind_t* out_kind);

/**
 * Releases a related-iterator.
 *
 * @param[in] iter Owns it.  NULL is a no-op.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_related_iter_next
 */
OCCTL_API void OCCTL_CALL occtl_topo_related_iter_free(occtl_topo_related_iter_t* iter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_TOPO_RELATION_H */
