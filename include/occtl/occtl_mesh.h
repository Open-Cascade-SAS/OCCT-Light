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
 * @file occtl_mesh.h
 * @brief OCCT-Light: mesh (triangulation) module public API.
 *
 * Generates surface tessellations and edge polylines for a graph and
 * exposes the cached results — face triangulations, edge polygons,
 * coedge polygons-on-triangulation — as POD views into library-owned
 * memory.
 *
 * @par Indexing convention.
 *      Triangle vertex indices and polygon-on-triangulation node indices
 *      are **0-indexed** in this ABI (matching NumPy / WASM / C#
 *      conventions). View pointers expose the converted buffers directly.
 *
 * @par Lifetime.
 *      Mesh views borrow from the source graph. Their pointers remain
 *      valid until the graph is mutated (any builder call, any subsequent
 *      #occtl_mesh_generate, or graph free) — see each accessor for the
 *      exact contract.
 *
 * @sa occtl_topo_face_has_triangulation (in @c occtl_topo.h) — predicate
 *     for whether a face has a cached triangulation.
 *
 * @see ../../docs/design/MODULES.md §8 for the module charter.
 */

#ifndef OCCTL_MESH_H
#define OCCTL_MESH_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_geom.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Axis-aligned bounding box in 3D.
 *
 * Math-POD per @c ABI_PATTERNS.md §15: passed by value, not versioned.
 * Used only by #occtl_mesh_options_t today; if a second consumer
 * appears in another module, promote this declaration to
 * @c occtl_geom.h in a follow-up.
 */
typedef struct occtl_aabb3
{
  occtl_point3_t min; /**< Lower corner (componentwise minimum). */
  occtl_point3_t max; /**< Upper corner (componentwise maximum). */
} occtl_aabb3_t;

/**
 * Current options struct version. Bumped only on a binary-incompatible
 * change. New fields are appended via @c p_next-chained extension structs.
 */
#define OCCTL_MESH_OPTIONS_VERSION_1 1u

/**
 * Tunable parameters for #occtl_mesh_generate.
 *
 * Two parameter sources are supported, selected by the @c use_bbox
 * field: explicit deflection / angle controls, or bbox-derived where
 * the linear deflection is recomputed from a caller-supplied bounding
 * box and a relative coefficient.
 *
 * Defaults (see #OCCTL_MESH_OPTIONS_INIT and #occtl_mesh_options_init)
 * are the conservative ones: linear deflection 0.001, angular
 * deflection 0.5 rad, interior fields = -1.0 (inherit from boundary),
 * single threaded, control-surface deflection on, model cleanup on.
 *
 * @par Cancellation and algorithm selection.
 *      Cooperative cancellation and a tessellation-algorithm enum will
 *      be added behind @c p_next when a binding asks for them; this
 *      v1 surface is locked to the default algorithm and runs to
 *      completion.
 */
typedef struct occtl_mesh_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_MESH_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved for extensions; must be NULL. */

  double deflection; /**< Boundary linear deflection. Default 0.001. */
  double angle;      /**< Boundary angular deflection (rad). Default 0.5. */
  double
    deflection_interior; /**< Interior linear deflection; -1.0 to inherit boundary. Default -1.0. */
  double angle_interior; /**< Interior angular deflection (rad); -1.0 to inherit boundary. Default
                            -1.0. */
  double
    min_size; /**< Minimum triangle edge length; -1.0 to compute from deflection. Default -1.0. */

  int32_t in_parallel; /**< 0/1; non-zero enables parallel face meshing. Default 0. */
  int32_t relative;    /**< 0/1; non-zero treats deflection as relative to edge size. Default 0. */
  int32_t internal_vertices_mode;     /**< 0/1; non-zero adds interior vertices to the surface mesh.
                                         Default 1. */
  int32_t control_surface_deflection; /**< 0/1; non-zero verifies surface deviation. Default 1. */
  int32_t control_surface_deflection_all; /**< 0/1; if non-zero applies the surface-deflection check
                                             to all surface kinds. Default 0. */
  int32_t clean_model;     /**< 0/1; non-zero discards stale intermediate mesh data after the run.
                              Default 1. */
  int32_t adjust_min_size; /**< 0/1; non-zero adjusts minimum size per edge. Default 0. */
  int32_t force_face_deflection;  /**< 0/1; non-zero overrides face tolerance with the supplied
                                     deflection. Default 0. */
  int32_t allow_quality_decrease; /**< 0/1; non-zero permits the algorithm to coarsen an existing
                                     mesh. Default 0. */

  int32_t use_bbox;   /**< 0/1; when non-zero the algorithm derives @c deflection from the @c bbox /
                         @c deviation_coefficient pair instead of using the explicit @c deflection
                         field. Default 0. */
  occtl_aabb3_t bbox; /**< Bounding box used iff @c use_bbox != 0. */
  double deviation_coefficient; /**< Relative deflection multiplier for bbox mode. Default 0.001. */
  double deviation_angle;       /**< Angular deflection (rad) for bbox mode. Default 20°. */
} occtl_mesh_options_t;

/**
 * Static initialiser for #occtl_mesh_options_t. Suitable for
 * @code occtl_mesh_options_t opts = OCCTL_MESH_OPTIONS_INIT; @endcode
 *
 * The angular-deflection default uses #OCCTL_ANGLE_20_DEG_RAD (20 degrees).
 */
#define OCCTL_MESH_OPTIONS_INIT                                                                    \
  {OCCTL_MESH_OPTIONS_VERSION_1,                                                                   \
   NULL,                                                                                           \
   0.001,                                                                                          \
   0.5,                                                                                            \
   -1.0,                                                                                           \
   -1.0,                                                                                           \
   -1.0,                                                                                           \
   0,                                                                                              \
   0,                                                                                              \
   1,                                                                                              \
   1,                                                                                              \
   0,                                                                                              \
   1,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},                                                             \
   0.001,                                                                                          \
   OCCTL_ANGLE_20_DEG_RAD}

/**
 * Runtime initialiser for #occtl_mesh_options_t.
 *
 * Sets all fields to #OCCTL_MESH_OPTIONS_INIT.
 *
 * @param[out] options Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 */
OCCTL_API void OCCTL_CALL occtl_mesh_options_init(occtl_mesh_options_t* options);

/**
 * Sets mesh-model metadata on the owning graph.
 *
 * This is a mesh-module spelling for graph-level document/model metadata
 * used by mesh exchange formats such as 3MF, glTF, OBJ, and VRML.  The data
 * is stored in the graph metadata storage, so it survives graph clone and
 * native graph snapshots and is visible through #occtl_graph_metadata_get.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL.
 * @param[in]     key      Metadata key bytes. Borrowed; copied internally.
 *                         Must be non-NULL and non-empty.
 * @param[in]     keyLen   Length of @p key in bytes.
 * @param[in]     value    Metadata value bytes. Borrowed; copied internally.
 *                         May be NULL only when @p valueLen is 0.
 * @param[in]     valueLen Length of @p value in bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL; @p key is NULL/empty; or
 *                                 @p value is NULL when @p valueLen > 0.
 *
 * @threadsafe No (mutates graph metadata).
 *
 * @sa occtl_mesh_model_metadata_get, occtl_graph_metadata_set
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_model_metadata_set(occtl_graph_t* graph,
                                                                  const char*    key,
                                                                  size_t         keyLen,
                                                                  const char*    value,
                                                                  size_t         valueLen);

/**
 * Retrieves mesh-model metadata from the owning graph.
 *
 * Uses the two-call string buffer pattern: call once with @p buf == NULL to
 * learn @p out_required, then call again with a buffer of at least that many
 * bytes.  The written value is NUL-terminated; @p out_required includes the
 * NUL terminator.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  key          Metadata key bytes. Borrowed; must be non-NULL
 *                          and non-empty.
 * @param[in]  keyLen       Length of @p key in bytes.
 * @param[out] buf          Owns it (caller-allocated). May be NULL to query
 *                          required size.
 * @param[in]  bufSize      Size of @p buf in bytes.
 * @param[out] out_required Borrows it. Must be non-NULL. Receives required
 *                          size including NUL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_required is NULL, or
 *                                 @p key is NULL/empty.
 * @retval OCCTL_NOT_FOUND         @p key is not set on @p graph.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p buf is non-NULL and @p bufSize is too
 *                                 small.
 *
 * @threadsafe Yes (read-only on graph metadata).
 *
 * @sa occtl_mesh_model_metadata_set, occtl_mesh_model_metadata_keys
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_model_metadata_get(const occtl_graph_t* graph,
                                                                  const char*          key,
                                                                  size_t               keyLen,
                                                                  char*                buf,
                                                                  size_t               bufSize,
                                                                  size_t* out_required);

/**
 * Lists mesh-model metadata keys.
 *
 * Uses the two-call buffer pattern: pass @p out_keys as NULL with @p cap 0 to
 * learn the key count, then call again with an array of at least that many
 * entries.  Returned key pointers borrow from the graph metadata storage and
 * are not necessarily NUL-terminated; use @c key_len.
 *
 * @param[in]  graph     Borrows it. Must be non-NULL.
 * @param[out] out_keys  Borrows it (caller-allocated). Length @p cap; may be
 *                       NULL to query count.
 * @param[in]  cap       Capacity of @p out_keys in elements.
 * @param[out] out_count Borrows it. Must be non-NULL. Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_keys is non-NULL and @p cap is too
 *                                 small.
 *
 * @threadsafe Yes (read-only on graph metadata).
 *
 * @sa occtl_mesh_model_metadata_get, occtl_graph_metadata_keys
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_model_metadata_keys(const occtl_graph_t*       graph,
                                 occtl_metadata_key_view_t* out_keys,
                                 size_t                     cap,
                                 size_t*                    out_count);

/**
 * Removes one mesh-model metadata key.
 *
 * Idempotent: removing a missing key is a successful no-op.
 *
 * @param[in,out] graph  Borrows it. Must be non-NULL.
 * @param[in]     key    Metadata key bytes. Borrowed; must be non-NULL and
 *                       non-empty.
 * @param[in]     keyLen Length of @p key in bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p key is NULL/empty.
 *
 * @threadsafe No (mutates graph metadata).
 *
 * @sa occtl_mesh_model_metadata_set, occtl_mesh_model_metadata_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_model_metadata_unset(occtl_graph_t* graph,
                                                                    const char*    key,
                                                                    size_t         keyLen);

#define OCCTL_MESH_FROM_BUFFERS_OPTIONS_VERSION_1 1u

/**
 * Input buffers for #occtl_mesh_from_buffers.
 *
 * Nodes are xyz-interleaved doubles.  Triangles are 0-indexed triplets into
 * @c nodes, matching the rest of the public mesh ABI.  The library copies
 * the buffers into an OCCT #Poly_Triangulation owned by a new graph, so the
 * caller may free the input arrays after the function returns.
 */
typedef struct occtl_mesh_from_buffers_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_MESH_FROM_BUFFERS_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved for extensions; must be NULL. */

  const double*   nodes;      /**< xyz interleaved doubles, length 3 * @c node_count. Borrows it. */
  size_t          node_count; /**< Number of vertices in @c nodes. */
  const uint32_t* triangles;  /**< 0-indexed triplets, length 3 * @c triangle_count. Borrows it. */
  size_t          triangle_count; /**< Number of triangles in @c triangles. */
  double deflection; /**< Stored triangulation deflection. Must be finite and non-negative. */
} occtl_mesh_from_buffers_options_t;

#define OCCTL_MESH_FROM_BUFFERS_OPTIONS_INIT                                                       \
  {OCCTL_MESH_FROM_BUFFERS_OPTIONS_VERSION_1, NULL, NULL, 0, NULL, 0, 0.0}

/**
 * Runtime initialiser for #occtl_mesh_from_buffers_options_t.
 *
 * Sets all fields to #OCCTL_MESH_FROM_BUFFERS_OPTIONS_INIT.
 *
 * @param[out] options Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_mesh_from_buffers
 */
OCCTL_API void OCCTL_CALL
  occtl_mesh_from_buffers_options_init(occtl_mesh_from_buffers_options_t* options);

/**
 * Creates a graph containing one triangulated Face from caller buffers.
 *
 * This is the inverse of the triangle-soup extraction helper for workflows
 * that receive mesh data from engines, WASM, or 3D-print pipelines.  The
 * implementation copies caller buffers into OCCT #Poly_Triangulation and
 * attaches that triangulation to a Face using OCCT topology builders before
 * ingesting the result through BRepGraph.  No wrapper-local mesh store is
 * kept.
 *
 * @param[in]  options   Borrows it. Must be non-NULL and fully initialised.
 * @param[out] out_graph Owns it. Receives a new graph handle. Must be
 *                       released with #occtl_graph_free.
 * @param[out] out_root  Borrows it. Receives the triangulated Face root.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p options, @p out_graph, or @p out_root
 *                                 is NULL; @c p_next is non-NULL; buffers
 *                                 are inconsistent; coordinates are not
 *                                 finite; an index is out of range; or
 *                                 @c deflection is negative / non-finite.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version is unsupported.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not construct a triangulated
 *                                 face from the buffers.
 * @retval OCCTL_TOPOLOGY_INVALID  The triangulated face could not be
 *                                 ingested into BRepGraph.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (allocates a new graph; does not touch shared state).
 *
 * @sa occtl_mesh_triangle_buffers, occtl_mesh_face_triangulation
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_from_buffers(const occtl_mesh_from_buffers_options_t* options,
                          occtl_graph_t**                          out_graph,
                          occtl_node_id_t*                         out_root);

/**
 * Tessellates the graph (or a node subset).
 *
 * Three modes folded into a single entry point through @p nodes /
 * @p n_nodes:
 *   - @p nodes == NULL && @p n_nodes == 0 — mesh every active face and
 *     free edge in @p graph (whole-graph dispatch).
 *   - @p nodes != NULL && @p n_nodes == 1 — mesh the subtree rooted at
 *     that node (single-root dispatch).
 *   - @p nodes != NULL && @p n_nodes  > 1 — mesh the deduplicated set
 *     of faces reachable from any of the given nodes (multi-node
 *     dispatch).
 *
 * If @c options->use_bbox is non-zero the algorithm derives parameters
 * from @c options->bbox / @c options->deviation_coefficient /
 * @c options->deviation_angle; otherwise the explicit @c deflection /
 * @c angle / ... fields are used directly.
 *
 * On success, any cached mesh views over @p graph are invalidated;
 * re-fetch via the accessor functions below before reading.
 *
 * @param[in,out] graph    Borrows it. Must be non-NULL. Mutated:
 *                         receives generated mesh data on its faces /
 *                         edges / coedges.
 * @param[in]     nodes    Borrows it. May be NULL when @p n_nodes is 0.
 *                         When non-NULL must point to at least
 *                         @p n_nodes valid #occtl_node_id_t entries
 *                         that already live in @p graph.
 * @param[in]     n_nodes  Number of node IDs. 0 selects the whole-graph
 *                         dispatch. Must be 0 when @p nodes is NULL.
 * @param[in]     options  Borrows it. Must be non-NULL with a
 *                         recognised @c struct_version.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p options is NULL, or
 *                                 @p nodes / @p n_nodes is inconsistent.
 * @retval OCCTL_VERSION_MISMATCH  @c options->struct_version is
 *                                 unrecognised.
 * @retval OCCTL_NOT_FOUND         A NodeId in @p nodes is invalid or
 *                                 has been removed from @p graph.
 * @retval OCCTL_NOT_DONE          The tessellation algorithm reported failure.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 * @retval OCCTL_INTERNAL          A C++ exception was caught at the
 *                                 ABI boundary.
 *
 * @threadsafe No — mutates @p graph.
 *
 * @sa occtl_mesh_face_triangulation, occtl_mesh_edge_polygon3d
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_generate(occtl_graph_t*              graph,
                                                        const occtl_node_id_t*      nodes,
                                                        size_t                      n_nodes,
                                                        const occtl_mesh_options_t* options);

/**
 * Zero-copy view of a face triangulation.
 *
 * Math-POD per @c ABI_PATTERNS.md §15 — not versioned (re-shape via a
 * new type if the field set ever changes).
 *
 * All pointer fields borrow from a per-graph internal cache. Each cache
 * slot remembers the source node's version stamp; the next fetch after
 * any mutation (builder, boolean op, compact, #occtl_mesh_generate) sees
 * the slot as stale and re-materialises into a fresh allocation.
 * Existing pointers remain valid until *this* graph mutates and *another*
 * fetch on the same face triggers re-materialisation, so callers that
 * straddle a mutation must copy out or refetch.
 *
 * Triangle indices are **0-indexed** into @c nodes — buffers are
 * materialised on first read into a per-face cache.
 *
 * @c normals and @c uvs are NULL when the underlying triangulation
 * carries no per-vertex normals / UV nodes; check before dereferencing.
 */
typedef struct occtl_triangulation_view
{
  const double* nodes;      /**< xyz interleaved doubles, length 3 * @c node_count. */
  size_t        node_count; /**< Number of vertices in the triangulation. */
  const double*
    normals;         /**< xyz interleaved doubles, length 3 * @c node_count, or NULL when absent. */
  const double* uvs; /**< uv interleaved doubles, length 2 * @c node_count, or NULL when absent. */
  const uint32_t* triangles; /**< 0-indexed triplets into @c nodes, length 3 * @c triangle_count. */
  size_t          triangle_count; /**< Number of triangles. */
  double          deflection; /**< Linear deflection used when this triangulation was generated. */
  occtl_uid_t     source_uid; /**< UID of the parent face. #OCCTL_UID_INVALID if the cache could not
                                 resolve it. */
} occtl_triangulation_view_t;

/**
 * Returns the active cached triangulation of @p face.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  face     NodeId of a face in @p graph.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *                      On success populated with a borrowed view; on
 *                      failure left untouched.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_view is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid / removed, or it
 *                                 has no cached triangulation (run
 *                                 #occtl_mesh_generate first).
 * @retval OCCTL_WRONG_KIND        @p face is not #OCCTL_KIND_FACE.
 * @retval OCCTL_INTERNAL          Internal failure.
 *
 * @threadsafe Yes (read-only on @p graph).
 *
 * @sa occtl_topo_face_has_triangulation, occtl_mesh_face_triangulation_count,
 *     occtl_mesh_face_triangulation_indexed
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_face_triangulation(const occtl_graph_t*        graph,
                                occtl_node_id_t             face,
                                occtl_triangulation_view_t* out_view);

/**
 * Returns the number of cached triangulations on @p face.
 *
 * Faces may carry several LOD-style triangulations; this accessor
 * reports how many. Most workflows use the active triangulation
 * (#occtl_mesh_face_triangulation) and ignore the multi-mesh case.
 *
 * @param[in]  graph     Borrows it. Must be non-NULL.
 * @param[in]  face      NodeId of a face in @p graph.
 * @param[out] out_count Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK                Success; @p out_count receives the count
 *                                 (0 when there is no cached mesh).
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not #OCCTL_KIND_FACE.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_mesh_face_triangulation, occtl_mesh_face_triangulation_indexed
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_face_triangulation_count(const occtl_graph_t* graph,
                                                                        occtl_node_id_t      face,
                                                                        uint32_t* out_count);

/**
 * Returns the @p index-th cached triangulation of @p face.
 *
 * @p index ranges over @c [0, occtl_mesh_face_triangulation_count(graph, face)).
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  face     NodeId of a face in @p graph.
 * @param[in]  index    0-based triangulation index.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_view is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or has no cached mesh.
 * @retval OCCTL_OUT_OF_RANGE      @p index is past the number of cached
 *                                 triangulations.
 * @retval OCCTL_WRONG_KIND        @p face is not #OCCTL_KIND_FACE.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_mesh_face_triangulation
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_face_triangulation_indexed(const occtl_graph_t*        graph,
                                        occtl_node_id_t             face,
                                        uint32_t                    index,
                                        occtl_triangulation_view_t* out_view);

/**
 * Zero-copy view of a 3D polyline cached on an edge.
 *
 * Zero-copy span/view per ABI §10.2; pointers borrow from a per-graph
 * internal cache; lifetime matches #occtl_triangulation_view_t.
 *
 * @c parameters carries the curve parameter at each polyline node
 * when one was stored alongside the polyline, NULL otherwise.
 */
typedef struct occtl_polygon3d_view
{
  const double* nodes;      /**< xyz interleaved doubles, length 3 * @c node_count. */
  size_t        node_count; /**< Number of polyline nodes. */
  const double*
         parameters; /**< Curve parameter per node, length @c node_count, or NULL when absent. */
  double deflection; /**< Polygon deflection. */
  occtl_uid_t source_uid; /**< UID of the parent edge. */
} occtl_polygon3d_view_t;

/**
 * Returns the cached 3D polygon on @p edge.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  edge     NodeId of an edge in @p graph.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_view is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid / removed, or it
 *                                 has no cached 3D polygon.
 * @retval OCCTL_WRONG_KIND        @p edge is not #OCCTL_KIND_EDGE.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_edge_polygon3d(const occtl_graph_t*    graph,
                                                              occtl_node_id_t         edge,
                                                              occtl_polygon3d_view_t* out_view);

/**
 * Zero-copy view of a coedge polyline indexed onto its parent face's
 * triangulation.
 *
 * @c node_indices entries are 0-indexed into the @c nodes array of
 * the parent face's triangulation. Buffers are materialised on first
 * read into a per-coedge cache.
 *
 * Zero-copy span/view per ABI §10.2; pointer fields borrow from a
 * per-graph internal cache; lifetime matches #occtl_triangulation_view_t.
 */
typedef struct occtl_polygon_on_tri_view
{
  const uint32_t* node_indices; /**< 0-indexed indices into the parent face triangulation, length @c
                                   node_count. */
  size_t node_count;            /**< Number of polyline nodes. */
  const double*
         parameters; /**< Curve parameter per node, length @c node_count, or NULL when absent. */
  double deflection; /**< Polygon deflection. */
  occtl_uid_t source_uid; /**< UID of the parent coedge. */
} occtl_polygon_on_tri_view_t;

/**
 * Returns the cached polygon-on-triangulation for @p coedge.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  coedge   NodeId of a coedge in @p graph.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_view is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid / removed, or it
 *                                 has no cached polygon-on-triangulation.
 * @retval OCCTL_WRONG_KIND        @p coedge is not #OCCTL_KIND_COEDGE.
 *
 * @threadsafe Yes.
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_coedge_polygon_on_tri(const occtl_graph_t*         graph,
                                   occtl_node_id_t              coedge,
                                   occtl_polygon_on_tri_view_t* out_view);

/**
 * Borrowed aggregate triangle-soup view over cached face triangulations.
 *
 * The view copies all included face triangulations into graph-owned ABI
 * buffers.  Nodes are xyz interleaved doubles; triangle indices are
 * 0-indexed into @c nodes.  The buffers remain valid until @p graph is
 * mutated, freed, or #occtl_mesh_triangle_buffers is called again on the
 * same graph.
 *
 * When a Product / Occurrence hierarchy is traversed, accumulated graph
 * locations are applied to copied vertices.  The buffer is intentionally a
 * triangle soup: vertices shared by neighboring faces may appear more than
 * once.
 */
typedef struct occtl_mesh_triangle_buffers_view
{
  const double*   nodes;      /**< xyz interleaved doubles, length 3 * @c node_count. */
  size_t          node_count; /**< Number of copied triangle vertices. */
  const uint32_t* triangles; /**< 0-indexed triplets into @c nodes, length 3 * @c triangle_count. */
  size_t          triangle_count; /**< Number of copied triangles. */
  size_t          face_count;     /**< Number of faces that contributed triangulations. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_buffers_view_t;

#define OCCTL_MESH_TRIANGLE_ADJACENCY_BOUNDARY UINT32_MAX

/**
 * Borrowed graph-owned analysis buffers over aggregate triangulations.
 *
 * This view uses the same root traversal as #occtl_mesh_triangle_buffers,
 * then materialises per-triangle normals and triangle adjacency into the
 * graph-owned mesh cache.  Normals are xyz interleaved doubles, one normal
 * per triangle.  Adjacency is three uint32_t entries per triangle; entry
 * @c 3*i+0 is the neighbouring triangle across edge (v0,v1), @c 3*i+1
 * across edge (v1,v2), and @c 3*i+2 across edge (v2,v0).  Boundary edges
 * use #OCCTL_MESH_TRIANGLE_ADJACENCY_BOUNDARY.
 *
 * The buffers remain valid until @p graph is mutated, freed, or
 * #occtl_mesh_triangle_analysis is called again on the same graph.
 */
typedef struct occtl_mesh_triangle_analysis_view
{
  const double* triangle_normals; /**< xyz interleaved doubles, length 3 * @c triangle_count. */
  const uint32_t*
         triangle_adjacency; /**< 3 neighbours per triangle, length 3 * @c triangle_count. */
  size_t triangle_count;     /**< Number of analysed triangles. */
  size_t face_count;         /**< Number of faces that contributed triangulations. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_analysis_view_t;

#define OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_mesh_triangle_components.
 *
 * Components are grown across triangle adjacency when neighbouring triangle
 * normals differ by no more than @c max_normal_angle.  When
 * @c include_opposite_normals is non-zero, normals with opposite signs are
 * treated as equivalent by comparing @c abs(dot(n0,n1)); this is useful for
 * imported triangle soups with inconsistent winding.
 */
typedef struct occtl_mesh_triangle_components_options
{
  uint32_t        struct_version; /**< Must be #OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t root;    /**< Root node to analyse, or #OCCTL_NODE_ID_INVALID for whole graph. */
  double max_normal_angle; /**< Maximum normal angle in radians. Default #OCCTL_ANGLE_1_DEG_RAD
                              (1 degree). */
  int32_t include_opposite_normals; /**< 0/1; compare absolute normal dot product. Default 1. */
} occtl_mesh_triangle_components_options_t;

#define OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT                                                \
  {OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_VERSION_1,                                               \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_ANGLE_1_DEG_RAD,                                                                          \
   1}

/**
 * Borrowed graph-owned connected-component labels over aggregate triangulations.
 *
 * @c triangle_component_ids has one entry per triangle.  IDs are dense and
 * 0-indexed in first-discovery order.  @c component_sizes has one entry per
 * component and counts how many triangles carry that component ID.
 *
 * The buffers remain valid until @p graph is mutated, freed, or
 * #occtl_mesh_triangle_components is called again on the same graph.
 */
typedef struct occtl_mesh_triangle_components_view
{
  const uint32_t*
         triangle_component_ids;   /**< Component ID per triangle, length @c triangle_count. */
  size_t triangle_count;           /**< Number of labelled triangles. */
  const uint32_t* component_sizes; /**< Triangle count per component, length @c component_count. */
  size_t          component_count; /**< Number of normal-connected components. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_components_view_t;

/**
 * Borrowed graph-owned triangle-index view for one component.
 *
 * Triangle indices refer to the aggregate triangle order returned by
 * #occtl_mesh_triangle_buffers for the same root/options.  The buffer
 * remains valid until @p graph is mutated, freed, or
 * #occtl_mesh_triangle_component_triangles is called again on the same
 * graph.
 */
typedef struct occtl_mesh_triangle_component_triangles_view
{
  const uint32_t* triangles; /**< Triangle indices in the component, length @c triangle_count. */
  size_t          triangle_count; /**< Number of triangle indices. */
  uint32_t        component_id;   /**< Component ID selected by the caller. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_component_triangles_view_t;

/**
 * Boundary edge of one normal-connected triangle component.
 *
 * @c node0 and @c node1 are aggregate triangle-buffer node indices in the
 * edge orientation of @c triangle.  @c adjacent_triangle is either
 * #OCCTL_MESH_TRIANGLE_ADJACENCY_BOUNDARY for an open mesh edge, or the
 * neighbouring triangle that belongs to a different component across a
 * sharp component boundary.
 */
typedef struct occtl_mesh_triangle_component_boundary_edge
{
  uint32_t triangle;          /**< Aggregate triangle index owning this boundary edge. */
  uint32_t local_edge;        /**< Local triangle edge: 0=(v0,v1), 1=(v1,v2), 2=(v2,v0). */
  uint32_t node0;             /**< First aggregate node index of the edge. */
  uint32_t node1;             /**< Second aggregate node index of the edge. */
  uint32_t adjacent_triangle; /**< Boundary sentinel or adjacent triangle from another component. */
} occtl_mesh_triangle_component_boundary_edge_t;

/**
 * Ordered boundary chain descriptor for one triangle component.
 *
 * Chains index into the ordered edge array returned by
 * #occtl_mesh_triangle_component_boundary_chains.  A closed chain has
 * matching end/start nodes; an open chain represents a non-manifold or open
 * mesh boundary.
 */
typedef struct occtl_mesh_triangle_component_boundary_chain
{
  uint32_t first_edge; /**< First edge index in the ordered edge array. */
  uint32_t edge_count; /**< Number of ordered edges in this chain. */
  int32_t  is_closed;  /**< 1 if the last edge ends at the first edge start node, 0 otherwise. */
} occtl_mesh_triangle_component_boundary_chain_t;

/**
 * Ordered boundary polyline descriptor for one triangle component.
 *
 * Polylines index into the ordered point array returned by
 * #occtl_mesh_component_boundary_polylines.  Closed polylines
 * repeat the first point as the final point so consumers can draw or build
 * closed loops without consulting the edge array.
 */
typedef struct occtl_mesh_component_boundary_polyline
{
  uint32_t first_point; /**< First point index in the ordered point array. */
  uint32_t point_count; /**< Number of points in this polyline. */
  int32_t  is_closed;   /**< 1 for a closed loop, 0 for an open chain. */
} occtl_mesh_component_boundary_polyline_t;

/**
 * Borrowed graph-owned boundary-edge view for one triangle component.
 *
 * The buffer remains valid until @p graph is mutated, freed, or
 * #occtl_mesh_triangle_component_boundary is called again on the same
 * graph.
 */
typedef struct occtl_mesh_triangle_component_boundary_view
{
  const occtl_mesh_triangle_component_boundary_edge_t*
                  edges;        /**< Boundary edges, length @c edge_count. */
  size_t          edge_count;   /**< Number of boundary edges. */
  uint32_t        component_id; /**< Component ID selected by the caller. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_component_boundary_view_t;

/**
 * Borrowed graph-owned ordered boundary-chain view for one triangle component.
 *
 * Edges are oriented so @c edges[i].node1 equals @c edges[i+1].node0 inside
 * each chain.  Chain descriptors provide contiguous ranges into @c edges.
 * The buffer remains valid until @p graph is mutated, freed, or
 * #occtl_mesh_triangle_component_boundary_chains is called again on the
 * same graph.
 */
typedef struct occtl_mesh_triangle_component_boundary_chains_view
{
  const occtl_mesh_triangle_component_boundary_edge_t*
         edges;      /**< Ordered boundary edges, length @c edge_count. */
  size_t edge_count; /**< Number of ordered boundary edges. */
  const occtl_mesh_triangle_component_boundary_chain_t*
                  chains;       /**< Boundary chains, length @c chain_count. */
  size_t          chain_count;  /**< Number of boundary chains. */
  uint32_t        component_id; /**< Component ID selected by the caller. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_component_boundary_chains_view_t;

/**
 * Borrowed graph-owned ordered boundary-polyline view for one triangle component.
 *
 * Points are copied from the aggregate triangle-buffer node coordinates and
 * ordered according to #occtl_mesh_triangle_component_boundary_chains.  The
 * buffer remains valid until @p graph is mutated, freed, or
 * #occtl_mesh_component_boundary_polylines is called again on the
 * same graph.
 */
typedef struct occtl_mesh_component_boundary_polylines_view
{
  const occtl_point3_t* points;      /**< Ordered polyline points, length @c point_count. */
  size_t                point_count; /**< Number of ordered polyline points. */
  const occtl_mesh_component_boundary_polyline_t*
                  polylines;      /**< Polyline descriptors, length @c polyline_count. */
  size_t          polyline_count; /**< Number of boundary polylines. */
  uint32_t        component_id;   /**< Component ID selected by the caller. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_component_boundary_polylines_view_t;

/**
 * Summary statistics for one normal-connected triangle component.
 *
 * Area, centroid, average normal, and bounds are computed from the
 * graph-owned aggregate triangle buffers.  The centroid and normal are
 * area-weighted.  Degenerate components report zero area and a zero normal.
 */
typedef struct occtl_mesh_triangle_component_summary
{
  uint32_t        component_id;   /**< Dense 0-indexed component ID. */
  uint32_t        triangle_count; /**< Number of triangles in this component. */
  double          area;           /**< Total triangle area. */
  occtl_point3_t  centroid;       /**< Area-weighted component centroid. */
  occtl_vector3_t normal; /**< Area-weighted average normal, unit length when non-degenerate. */
  occtl_aabb3_t   bounds; /**< Axis-aligned bounds of all component triangle vertices. */
} occtl_mesh_triangle_component_summary_t;

/**
 * Borrowed graph-owned component summaries over aggregate triangulations.
 *
 * Summaries are ordered by component ID and use the same component labelling
 * rules as #occtl_mesh_triangle_components for the supplied options.
 *
 * The buffer remains valid until @p graph is mutated, freed, or
 * #occtl_mesh_triangle_component_summaries is called again on the same graph.
 */
typedef struct occtl_mesh_triangle_component_summaries_view
{
  const occtl_mesh_triangle_component_summary_t*
                  summaries;       /**< Component summaries, length @c component_count. */
  size_t          component_count; /**< Number of summaries. */
  size_t          triangle_count;  /**< Number of analysed triangles. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_component_summaries_view_t;

#define OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_mesh_triangle_plane_components.
 *
 * Components are first built with the same normal-angle rules as
 * #occtl_mesh_triangle_components.  A component is accepted as plane-like
 * when it satisfies the minimum size filters and every triangle vertex is
 * within @c max_distance of the component plane.
 */
typedef struct occtl_mesh_triangle_plane_components_options
{
  uint32_t struct_version; /**< Must be #OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_VERSION_1. */
  const void*     p_next;  /**< Reserved; set to NULL. */
  occtl_node_id_t root;    /**< Root node to analyse, or #OCCTL_NODE_ID_INVALID for whole graph. */
  double max_normal_angle; /**< Maximum normal angle in radians. Default #OCCTL_ANGLE_1_DEG_RAD
                              (1 degree). */
  int32_t  include_opposite_normals; /**< 0/1; compare absolute normal dot product. Default 1. */
  double   max_distance;       /**< Maximum vertex distance to fitted plane. Default 1.0e-6. */
  double   min_area;           /**< Minimum component area. Default 0.0. */
  uint32_t min_triangle_count; /**< Minimum component triangle count. Default 1. */
} occtl_mesh_triangle_plane_components_options_t;

#define OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT                                          \
  {OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_VERSION_1,                                         \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_ANGLE_1_DEG_RAD,                                                                          \
   1,                                                                                              \
   1.0e-6,                                                                                         \
   0.0,                                                                                            \
   1u}

/**
 * Plane-like normal-connected triangle component.
 *
 * The plane passes through @c origin with unit @c normal.  @c max_distance
 * is the largest absolute OCCT plane distance among all component triangle
 * vertices.
 */
typedef struct occtl_mesh_triangle_plane_component
{
  uint32_t        component_id;   /**< Source normal-connected component ID. */
  uint32_t        triangle_count; /**< Number of triangles in this component. */
  double          area;           /**< Total component area. */
  occtl_point3_t  origin;         /**< Plane origin, equal to component centroid. */
  occtl_vector3_t normal;         /**< Unit plane normal. */
  occtl_aabb3_t   bounds;         /**< Axis-aligned bounds of all component triangle vertices. */
  double          max_distance;   /**< Maximum vertex distance to the fitted plane. */
} occtl_mesh_triangle_plane_component_t;

/**
 * Borrowed graph-owned plane-like component view.
 *
 * The buffer remains valid until @p graph is mutated, freed, or
 * #occtl_mesh_triangle_plane_components is called again on the same graph.
 */
typedef struct occtl_mesh_triangle_plane_components_view
{
  const occtl_mesh_triangle_plane_component_t*
                  components;      /**< Plane-like components, length @c component_count. */
  size_t          component_count; /**< Number of accepted plane-like components. */
  size_t          triangle_count;  /**< Number of analysed triangles. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_plane_components_view_t;

#define OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_mesh_triangle_sphere_components.
 *
 * Components are first built with the same normal-angle rules as
 * #occtl_mesh_triangle_components.  A component is accepted as sphere-like
 * when an OCCT least-squares sphere fit satisfies the minimum size filters
 * and every triangle vertex is within @c max_distance of the fitted sphere.
 */
typedef struct occtl_mesh_triangle_sphere_components_options
{
  uint32_t struct_version; /**< Must be #OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_VERSION_1. */
  const void*     p_next;  /**< Reserved; set to NULL. */
  occtl_node_id_t root;    /**< Root node to analyse, or #OCCTL_NODE_ID_INVALID for whole graph. */
  double max_normal_angle; /**< Maximum normal angle in radians. Default #OCCTL_ANGLE_30_DEG_RAD
                              (30 degrees). */
  int32_t  include_opposite_normals; /**< 0/1; compare absolute normal dot product. Default 1. */
  double   max_distance;             /**< Maximum vertex radial residual. Default 1.0e-3. */
  double   min_area;                 /**< Minimum component area. Default 0.0. */
  uint32_t min_triangle_count;       /**< Minimum component triangle count. Default 4. */
  double   min_radius;               /**< Minimum accepted sphere radius. Default 1.0e-9. */
} occtl_mesh_triangle_sphere_components_options_t;

#define OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT                                         \
  {OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_VERSION_1,                                        \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_ANGLE_30_DEG_RAD,                                                                         \
   1,                                                                                              \
   1.0e-3,                                                                                         \
   0.0,                                                                                            \
   4u,                                                                                             \
   1.0e-9}

/**
 * Sphere-like normal-connected triangle component.
 *
 * The sphere is represented by @c center and positive @c radius.
 * @c max_distance is the largest absolute radial residual among all
 * component triangle vertices.
 */
typedef struct occtl_mesh_triangle_sphere_component
{
  uint32_t       component_id;   /**< Source normal-connected component ID. */
  uint32_t       triangle_count; /**< Number of triangles in this component. */
  double         area;           /**< Total component area. */
  occtl_point3_t center;         /**< Fitted sphere centre. */
  double         radius;         /**< Fitted sphere radius. */
  occtl_aabb3_t  bounds;         /**< Axis-aligned bounds of all component triangle vertices. */
  double         max_distance;   /**< Maximum absolute radial residual. */
} occtl_mesh_triangle_sphere_component_t;

/**
 * Borrowed graph-owned sphere-like component view.
 *
 * The buffer remains valid until @p graph is mutated, freed, or
 * #occtl_mesh_triangle_sphere_components is called again on the same graph.
 */
typedef struct occtl_mesh_triangle_sphere_components_view
{
  const occtl_mesh_triangle_sphere_component_t*
                  components;      /**< Sphere-like components, length @c component_count. */
  size_t          component_count; /**< Number of accepted sphere-like components. */
  size_t          triangle_count;  /**< Number of analysed triangles. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_sphere_components_view_t;

#define OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_mesh_triangle_cylinder_components.
 *
 * Components are first built with the same normal-angle rules as
 * #occtl_mesh_triangle_components.  A component is accepted as
 * cylinder-like when an OCCT eigen / least-squares cylinder fit satisfies
 * the minimum size filters and every triangle vertex is within
 * @c max_distance of the fitted cylinder.
 */
typedef struct occtl_mesh_triangle_cylinder_components_options
{
  uint32_t
    struct_version; /**< Must be #OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_VERSION_1. */
  const void*     p_next;  /**< Reserved; set to NULL. */
  occtl_node_id_t root;    /**< Root node to analyse, or #OCCTL_NODE_ID_INVALID for whole graph. */
  double max_normal_angle; /**< Maximum normal angle in radians. Default #OCCTL_ANGLE_30_DEG_RAD
                              (30 degrees). */
  int32_t  include_opposite_normals; /**< 0/1; compare absolute normal dot product. Default 1. */
  double   max_distance;             /**< Maximum vertex radial residual. Default 1.0e-3. */
  double   min_area;                 /**< Minimum component area. Default 0.0. */
  uint32_t min_triangle_count;       /**< Minimum component triangle count. Default 4. */
  double   min_radius;               /**< Minimum accepted cylinder radius. Default 1.0e-9. */
} occtl_mesh_triangle_cylinder_components_options_t;

#define OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT                                       \
  {OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_VERSION_1,                                      \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_ANGLE_30_DEG_RAD,                                                                         \
   1,                                                                                              \
   1.0e-3,                                                                                         \
   0.0,                                                                                            \
   4u,                                                                                             \
   1.0e-9}

/**
 * Cylinder-like normal-connected triangle component.
 *
 * The cylinder axis passes through @c axis_origin in unit direction
 * @c axis_direction.  @c height_min and @c height_max are the minimum and
 * maximum vertex projections along that axis relative to @c axis_origin.
 * @c max_distance is the largest absolute radial residual among all
 * component triangle vertices.
 */
typedef struct occtl_mesh_triangle_cylinder_component
{
  uint32_t        component_id;   /**< Source normal-connected component ID. */
  uint32_t        triangle_count; /**< Number of triangles in this component. */
  double          area;           /**< Total component area. */
  occtl_point3_t  axis_origin;    /**< Point on the fitted cylinder axis. */
  occtl_vector3_t axis_direction; /**< Unit fitted cylinder axis direction. */
  double          radius;         /**< Fitted cylinder radius. */
  double          height_min;     /**< Minimum vertex projection on axis. */
  double          height_max;     /**< Maximum vertex projection on axis. */
  occtl_aabb3_t   bounds;         /**< Axis-aligned bounds of all component triangle vertices. */
  double          max_distance;   /**< Maximum absolute radial residual. */
} occtl_mesh_triangle_cylinder_component_t;

/**
 * Borrowed graph-owned cylinder-like component view.
 *
 * The buffer remains valid until @p graph is mutated, freed, or
 * #occtl_mesh_triangle_cylinder_components is called again on the same graph.
 */
typedef struct occtl_mesh_triangle_cylinder_components_view
{
  const occtl_mesh_triangle_cylinder_component_t*
                  components;      /**< Cylinder-like components, length @c component_count. */
  size_t          component_count; /**< Number of accepted cylinder-like components. */
  size_t          triangle_count;  /**< Number of analysed triangles. */
  occtl_node_id_t root; /**< Root used for extraction, or #OCCTL_NODE_ID_INVALID for whole graph. */
} occtl_mesh_triangle_cylinder_components_view_t;

/**
 * Aggregates cached face triangulations into engine-friendly buffers.
 *
 * @p root may be #OCCTL_NODE_ID_INVALID to include every active face in
 * @p graph.  Otherwise @p root must name an active graph node; all faces
 * reachable below that root are included.  Run #occtl_mesh_generate first.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  root     Root node to extract, or #OCCTL_NODE_ID_INVALID for
 *                      whole-graph extraction.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *                      Receives borrowed buffers owned by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_view is NULL.
 * @retval OCCTL_NOT_FOUND         @p root is invalid / removed, or no
 *                                 cached face triangulation was found.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_generate, occtl_mesh_face_triangulation
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_triangle_buffers(const occtl_graph_t*                graph,
                              occtl_node_id_t                     root,
                              occtl_mesh_triangle_buffers_view_t* out_view);

/**
 * Computes cached per-triangle normals and adjacency for mesh buffers.
 *
 * @p root may be #OCCTL_NODE_ID_INVALID to include every active face in
 * @p graph.  Otherwise @p root must name an active graph node; all faces
 * reachable below that root are included.  Run #occtl_mesh_generate first.
 * Triangle normals are computed with OCCT vector operations from the cached
 * triangulation coordinates; adjacency is built from shared triangle edges.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  root     Root node to analyse, or #OCCTL_NODE_ID_INVALID for
 *                      whole-graph analysis.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *                      Receives borrowed buffers owned by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_view is NULL.
 * @retval OCCTL_NOT_FOUND         @p root is invalid / removed, or no
 *                                 cached face triangulation was found.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_buffers, occtl_mesh_generate
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_triangle_analysis(const occtl_graph_t*                 graph,
                               occtl_node_id_t                      root,
                               occtl_mesh_triangle_analysis_view_t* out_view);

/**
 * Initialises @p options to default triangle-component settings.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_mesh_triangle_components
 */
OCCTL_API void OCCTL_CALL
  occtl_mesh_triangle_components_options_init(occtl_mesh_triangle_components_options_t* options);

/**
 * Groups adjacent triangles into normal-connected components.
 *
 * This is a graph-owned cache view intended as a deterministic substrate for
 * mesh reverse-engineering and primitive patch detection.  It first builds
 * the same aggregate triangle soup as #occtl_mesh_triangle_buffers, computes
 * normals/adjacency as #occtl_mesh_triangle_analysis does, then runs a BFS
 * over adjacent triangles whose normals satisfy @p options.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  options  Borrows it. Must be non-NULL and carry a supported
 *                      @c struct_version.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *                      Receives borrowed buffers owned by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_view is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_analysis, occtl_mesh_triangle_buffers
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_triangle_components(const occtl_graph_t*                            graph,
                                 const occtl_mesh_triangle_components_options_t* options,
                                 occtl_mesh_triangle_components_view_t*          out_view);

/**
 * Returns aggregate triangle indices belonging to one component.
 *
 * Uses the same traversal and normal-connected component labelling as
 * #occtl_mesh_triangle_components for @p options, then filters the dense
 * triangle labels for @p component_id.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  options      Borrows it. Must be non-NULL and carry a
 *                          supported @c struct_version.
 * @param[in]  component_id Component ID to select.
 * @param[out] out_view     Borrows it (caller-allocated). Must be non-NULL.
 *                          Receives a borrowed triangle-index buffer owned
 *                          by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_view is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_OUT_OF_RANGE      @p component_id is not present.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_components, occtl_mesh_triangle_buffers
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_triangle_component_triangles(const occtl_graph_t*                            graph,
                                          const occtl_mesh_triangle_components_options_t* options,
                                          uint32_t component_id,
                                          occtl_mesh_triangle_component_triangles_view_t* out_view);

/**
 * Returns boundary edges for one normal-connected triangle component.
 *
 * Uses the same traversal and component labelling as
 * #occtl_mesh_triangle_components for @p options.  An edge is reported when
 * it has no adjacent triangle, or when the adjacent triangle belongs to a
 * different component.  Returned node indices refer to the aggregate
 * triangle buffers for the same root/options.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  options      Borrows it. Must be non-NULL and carry a
 *                          supported @c struct_version.
 * @param[in]  component_id Component ID to select.
 * @param[out] out_view     Borrows it (caller-allocated). Must be non-NULL.
 *                          Receives a borrowed boundary-edge buffer owned
 *                          by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_view is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_OUT_OF_RANGE      @p component_id is not present.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_components, occtl_mesh_triangle_analysis
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_triangle_component_boundary(const occtl_graph_t*                            graph,
                                         const occtl_mesh_triangle_components_options_t* options,
                                         uint32_t component_id,
                                         occtl_mesh_triangle_component_boundary_view_t* out_view);

/**
 * Returns ordered boundary chains for one normal-connected triangle component.
 *
 * This first computes #occtl_mesh_triangle_component_boundary, then orders
 * the component perimeter edges into contiguous head-to-tail chains.  Closed
 * mesh patches normally return one or more closed chains; open or
 * non-manifold patches may return open chains.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  options      Borrows it. Must be non-NULL and carry a
 *                          supported @c struct_version.
 * @param[in]  component_id Component ID to select.
 * @param[out] out_view     Borrows it (caller-allocated). Must be non-NULL.
 *                          Receives borrowed ordered chain buffers owned by
 *                          @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_view is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_OUT_OF_RANGE      @p component_id is not present.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_component_boundary, occtl_mesh_triangle_components
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_component_boundary_chains(
  const occtl_graph_t*                                  graph,
  const occtl_mesh_triangle_components_options_t*       options,
  uint32_t                                              component_id,
  occtl_mesh_triangle_component_boundary_chains_view_t* out_view);

/**
 * Returns ordered 3D boundary polylines for one triangle component.
 *
 * This computes ordered boundary chains and materialises their aggregate
 * node indices as 3D points.  Closed chains repeat their first point at the
 * end of the polyline.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  options      Borrows it. Must be non-NULL and carry a
 *                          supported @c struct_version.
 * @param[in]  component_id Component ID to select.
 * @param[out] out_view     Borrows it (caller-allocated). Must be non-NULL.
 *                          Receives borrowed ordered polyline buffers owned
 *                          by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_view is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_OUT_OF_RANGE      @p component_id is not present.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_component_boundary_chains, occtl_mesh_triangle_buffers
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_component_boundary_polylines(const occtl_graph_t*                            graph,
                                          const occtl_mesh_triangle_components_options_t* options,
                                          uint32_t component_id,
                                          occtl_mesh_component_boundary_polylines_view_t* out_view);

/**
 * Computes summaries for normal-connected triangle components.
 *
 * This uses the same traversal, normal-angle grouping, and graph-owned
 * buffers as #occtl_mesh_triangle_components, then computes one summary per
 * component.  Geometry accumulation uses OCCT point/vector primitives over
 * the cached triangulation buffers.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  options  Borrows it. Must be non-NULL and carry a supported
 *                      @c struct_version.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *                      Receives a borrowed summary array owned by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_view is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_components, occtl_mesh_triangle_analysis
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_triangle_component_summaries(const occtl_graph_t*                            graph,
                                          const occtl_mesh_triangle_components_options_t* options,
                                          occtl_mesh_triangle_component_summaries_view_t* out_view);

/**
 * Initialises @p options to default plane-component detector settings.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_mesh_triangle_plane_components
 */
OCCTL_API void OCCTL_CALL occtl_mesh_triangle_plane_components_options_init(
  occtl_mesh_triangle_plane_components_options_t* options);

/**
 * Detects plane-like normal-connected triangle components.
 *
 * This is a first reverse-engineering primitive detector over graph-owned
 * mesh buffers.  It groups triangles by normal connectivity, computes
 * component summaries, and validates each component against an OCCT
 * #gp_Pln plane using @c max_distance.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  options  Borrows it. Must be non-NULL and carry a supported
 *                      @c struct_version.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *                      Receives a borrowed component array owned by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_view is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_component_summaries, occtl_mesh_triangle_components
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_plane_components(
  const occtl_graph_t*                                  graph,
  const occtl_mesh_triangle_plane_components_options_t* options,
  occtl_mesh_triangle_plane_components_view_t*          out_view);

/**
 * Initialises @p options to default sphere-component detector settings.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_mesh_triangle_sphere_components
 */
OCCTL_API void OCCTL_CALL occtl_mesh_triangle_sphere_components_options_init(
  occtl_mesh_triangle_sphere_components_options_t* options);

/**
 * Detects sphere-like normal-connected triangle components.
 *
 * This reverse-engineering primitive detector groups triangles by normal
 * connectivity, computes component summaries, fits each component to a sphere
 * with OCCT @c math_SVD, and validates radial residuals with
 * OCCT point-distance calculations.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  options  Borrows it. Must be non-NULL and carry a supported
 *                      @c struct_version.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *                      Receives a borrowed component array owned by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_view is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_component_summaries,
 *     occtl_mesh_triangle_plane_components
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_sphere_components(
  const occtl_graph_t*                                   graph,
  const occtl_mesh_triangle_sphere_components_options_t* options,
  occtl_mesh_triangle_sphere_components_view_t*          out_view);

/**
 * Initialises @p options to default cylinder-component detector settings.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_mesh_triangle_cylinder_components
 */
OCCTL_API void OCCTL_CALL occtl_mesh_triangle_cylinder_components_options_init(
  occtl_mesh_triangle_cylinder_components_options_t* options);

/**
 * Detects cylinder-like normal-connected triangle components.
 *
 * This reverse-engineering primitive detector groups triangles by normal
 * connectivity, computes component summaries, fits each component to a
 * cylinder with OCCT @c math_Jacobi and @c math_SVD, and validates radial
 * residuals with OCCT point-to-line distance calculations.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  options  Borrows it. Must be non-NULL and carry a supported
 *                      @c struct_version.
 * @param[out] out_view Borrows it (caller-allocated). Must be non-NULL.
 *                      Receives a borrowed component array owned by @p graph.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_view is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on @p graph; serialises cache materialisation).
 *
 * @sa occtl_mesh_triangle_component_summaries,
 *     occtl_mesh_triangle_sphere_components
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_cylinder_components(
  const occtl_graph_t*                                     graph,
  const occtl_mesh_triangle_cylinder_components_options_t* options,
  occtl_mesh_triangle_cylinder_components_view_t*          out_view);

/**
 * Rebuilds one sphere-like triangle component as an analytic sphere Solid.
 *
 * This detects sphere-like components using @p options, builds an OCCT
 * analytic sphere Solid from the selected fitted component, then inserts it
 * into @p graph as a new topology root.  The source mesh component is not
 * removed or modified.
 *
 * @param[in,out] graph        Borrows it. Must be non-NULL. Receives the new
 *                             Solid root on success.
 * @param[in]     options      Borrows it. Must be non-NULL and carry a
 *                             supported @c struct_version.
 * @param[in]     component_id Sphere-like component ID to rebuild.
 * @param[out]    out_solid    Borrows it (caller-allocated). Must be
 *                             non-NULL. Receives the new Solid node ID.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_solid is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, no cached
 *                                 face triangulation was found, or
 *                                 @p component_id is not sphere-like.
 * @retval OCCTL_OUT_OF_RANGE      @p component_id is not present.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not build the analytic sphere.
 * @retval OCCTL_TOPOLOGY_INVALID  BRepGraph could not ingest the rebuilt Solid.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No. Mutates @p graph by inserting a new Solid.
 *
 * @sa occtl_mesh_triangle_sphere_components
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_sphere_component_solid(
  occtl_graph_t*                                         graph,
  const occtl_mesh_triangle_sphere_components_options_t* options,
  uint32_t                                               component_id,
  occtl_node_id_t*                                       out_solid);

/**
 * Rebuilds one cylinder-like triangle component as an analytic cylinder Solid.
 *
 * This detects cylinder-like components using @p options, builds an OCCT
 * analytic cylinder Solid from the selected fitted component, then inserts
 * it into @p graph as a new topology root.  The source mesh component is not
 * removed or modified.
 *
 * @param[in,out] graph        Borrows it. Must be non-NULL. Receives the new
 *                             Solid root on success.
 * @param[in]     options      Borrows it. Must be non-NULL and carry a
 *                             supported @c struct_version.
 * @param[in]     component_id Cylinder-like component ID to rebuild.
 * @param[out]    out_solid    Borrows it (caller-allocated). Must be
 *                             non-NULL. Receives the new Solid node ID.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_solid is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, no cached
 *                                 face triangulation was found, or
 *                                 @p component_id is not cylinder-like.
 * @retval OCCTL_OUT_OF_RANGE      @p component_id is not present.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not build the analytic cylinder.
 * @retval OCCTL_TOPOLOGY_INVALID  BRepGraph could not ingest the rebuilt Solid.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No. Mutates @p graph by inserting a new Solid.
 *
 * @sa occtl_mesh_triangle_cylinder_components
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_cylinder_component_solid(
  occtl_graph_t*                                           graph,
  const occtl_mesh_triangle_cylinder_components_options_t* options,
  uint32_t                                                 component_id,
  occtl_node_id_t*                                         out_solid);

/**
 * Rebuilds all detected sphere-like triangle components as analytic Solids.
 *
 * This is the bulk form of #occtl_mesh_make_sphere_component_solid.  It
 * first detects sphere-like components using @p options, then builds OCCT
 * analytic sphere Solids and inserts them into @p graph.  The source mesh
 * components are not removed or modified.
 *
 * Uses the two-call buffer pattern: pass @p out_buf as NULL with @p cap 0
 * to query @p out_count without mutating @p graph, then call again with an
 * array of at least that many entries to receive the new Solid node IDs.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL. Receives new Solid
 *                          roots on the fill call.
 * @param[in]     options   Borrows it. Must be non-NULL and carry a
 *                          supported @c struct_version.
 * @param[out]    out_buf   Owns it (caller-allocated). May be NULL to query
 *                          the required count. Receives @p out_count Solid
 *                          node IDs on success.
 * @param[in]     cap       Capacity of @p out_buf in elements.
 * @param[out]    out_count Borrows it (caller-allocated). Must be non-NULL.
 *                          Receives the number of sphere-like components.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_count is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too
 *                                 small.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not build an analytic sphere.
 * @retval OCCTL_TOPOLOGY_INVALID  BRepGraph could not ingest a rebuilt Solid.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No. The fill call mutates @p graph by inserting new Solids.
 *             The sizing call is read-only except for cache materialisation.
 *
 * @sa occtl_mesh_make_sphere_component_solid,
 *     occtl_mesh_triangle_sphere_components
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_sphere_component_solids(
  occtl_graph_t*                                         graph,
  const occtl_mesh_triangle_sphere_components_options_t* options,
  occtl_node_id_t*                                       out_buf,
  size_t                                                 cap,
  size_t*                                                out_count);

/**
 * Rebuilds all detected cylinder-like triangle components as analytic Solids.
 *
 * This is the bulk form of #occtl_mesh_make_cylinder_component_solid.  It
 * first detects cylinder-like components using @p options, then builds OCCT
 * analytic cylinder Solids and inserts them into @p graph.  The source mesh
 * components are not removed or modified.
 *
 * Uses the two-call buffer pattern: pass @p out_buf as NULL with @p cap 0
 * to query @p out_count without mutating @p graph, then call again with an
 * array of at least that many entries to receive the new Solid node IDs.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL. Receives new Solid
 *                          roots on the fill call.
 * @param[in]     options   Borrows it. Must be non-NULL and carry a
 *                          supported @c struct_version.
 * @param[out]    out_buf   Owns it (caller-allocated). May be NULL to query
 *                          the required count. Receives @p out_count Solid
 *                          node IDs on success.
 * @param[in]     cap       Capacity of @p out_buf in elements.
 * @param[out]    out_count Borrows it (caller-allocated). Must be non-NULL.
 *                          Receives the number of cylinder-like components.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_count is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too
 *                                 small.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not build an analytic cylinder.
 * @retval OCCTL_TOPOLOGY_INVALID  BRepGraph could not ingest a rebuilt Solid.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No. The fill call mutates @p graph by inserting new Solids.
 *             The sizing call is read-only except for cache materialisation.
 *
 * @sa occtl_mesh_make_cylinder_component_solid,
 *     occtl_mesh_triangle_cylinder_components
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_cylinder_component_solids(
  occtl_graph_t*                                           graph,
  const occtl_mesh_triangle_cylinder_components_options_t* options,
  occtl_node_id_t*                                         out_buf,
  size_t                                                   cap,
  size_t*                                                  out_count);

/**
 * Rebuilds one plane-like triangle component as a planar BRepGraph Face.
 *
 * This is a first mesh-to-BRep recovery bridge.  It detects plane-like
 * components using @p options, materialises ordered boundary polylines for
 * @p component_id, builds OCCT wires and a planar Face, then inserts that
 * Face into @p graph as a new topology root.  The source mesh component is
 * not removed or modified.
 *
 * @param[in,out] graph        Borrows it. Must be non-NULL. Receives the new
 *                             Face root on success.
 * @param[in]     options      Borrows it. Must be non-NULL and carry a
 *                             supported @c struct_version.
 * @param[in]     component_id Plane-like component ID to rebuild.
 * @param[out]    out_face     Borrows it (caller-allocated). Must be
 *                             non-NULL. Receives the new Face node ID.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_face is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, no cached
 *                                 face triangulation was found, or
 *                                 @p component_id is not plane-like.
 * @retval OCCTL_OUT_OF_RANGE      @p component_id is not present.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not build wires / Face from the
 *                                 component boundary.
 * @retval OCCTL_TOPOLOGY_INVALID  BRepGraph could not ingest the rebuilt Face.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No. Mutates @p graph by inserting a new Face.
 *
 * @sa occtl_mesh_triangle_plane_components,
 *     occtl_mesh_component_boundary_polylines
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_plane_component_face(
  occtl_graph_t*                                        graph,
  const occtl_mesh_triangle_plane_components_options_t* options,
  uint32_t                                              component_id,
  occtl_node_id_t*                                      out_face);

/**
 * Rebuilds all detected plane-like triangle components as planar Face roots.
 *
 * This is the bulk form of #occtl_mesh_make_plane_component_face.  It first
 * detects plane-like components using @p options, then builds OCCT planar
 * Faces from each component boundary and inserts them into @p graph.  The
 * source mesh components are not removed or modified.
 *
 * Uses the two-call buffer pattern: pass @p out_buf as NULL with @p cap 0
 * to query @p out_count without mutating @p graph, then call again with an
 * array of at least that many entries to receive the new Face node IDs.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL. Receives new Face
 *                          roots on the fill call.
 * @param[in]     options   Borrows it. Must be non-NULL and carry a
 *                          supported @c struct_version.
 * @param[out]    out_buf   Owns it (caller-allocated). May be NULL to query
 *                          the required count. Receives @p out_count Face
 *                          node IDs on success.
 * @param[in]     cap       Capacity of @p out_buf in elements.
 * @param[out]    out_count Borrows it (caller-allocated). Must be non-NULL.
 *                          Receives the number of plane-like components.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p options, or @p out_count is
 *                                 NULL; @c p_next is non-NULL; or an option
 *                                 value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid / removed, or no cached
 *                                 face triangulation was found.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too
 *                                 small.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not build wires / Faces from a
 *                                 component boundary.
 * @retval OCCTL_TOPOLOGY_INVALID  BRepGraph could not ingest a rebuilt Face.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No. The fill call mutates @p graph by inserting new Faces.
 *             The sizing call is read-only except for cache materialisation.
 *
 * @sa occtl_mesh_make_plane_component_face,
 *     occtl_mesh_triangle_plane_components
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_plane_component_faces(
  occtl_graph_t*                                        graph,
  const occtl_mesh_triangle_plane_components_options_t* options,
  occtl_node_id_t*                                      out_buf,
  size_t                                                cap,
  size_t*                                               out_count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_MESH_H */
