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
 * @file occtl_topo_algo.h
 * @brief OCCT-Light: graph-native algorithms — sewing, same-parameter
 *        recompute, validation.
 *
 * These wrap the @c BRepGraphAlgo and @c BRepGraphCheck families in OCCT.
 * They are part of the @c topo module; this header is gated by
 * @c OCCTL_HAS_TOPO in the umbrella include.
 */

#ifndef OCCTL_TOPO_ALGO_H
#define OCCTL_TOPO_ALGO_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_geom.h"
#include "occtl_surfaces.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_TOPO_SEW_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_sew.
 *
 * Mirrors @c BRepGraphAlgo_Sewing::Options.  All fields after @c p_next
 * are inputs.  Pass NULL as @p options to #occtl_topo_sew to accept the
 * defaults baked into #OCCTL_TOPO_SEW_OPTIONS_INIT.
 */
typedef struct occtl_topo_sew_options
{
  uint32_t    struct_version;      /**< Must be #OCCTL_TOPO_SEW_OPTIONS_VERSION_1. */
  const void* p_next;              /**< Reserved; set to NULL. */
  double      tolerance;           /**< Edge-matching tolerance.  Default 1e-6. */
  double      min_tolerance;       /**< Minimum edge-length threshold; 0 = auto. */
  double      max_tolerance;       /**< Upper bound for merge tolerance; 0 = +Infinity. */
  int32_t     cutting;             /**< 1 to cut edges at T-vertex intersections. */
  int32_t     same_parameter_mode; /**< 1 to enforce SameParameter on sewn edges. */
  int32_t     non_manifold_mode;   /**< 1 to allow >2 faces per edge. */
  int32_t     parallel;            /**< 1 to enable parallel execution. */
  int32_t     history_mode;        /**< 1 to record history on the graph. */
  int32_t     face_analysis;       /**< 1 to run face analysis preprocessing. */
  int32_t     floating_edges_mode; /**< 1 to include edges with 0 adjacent faces. */
  int32_t local_tolerances_mode;   /**< 1 to use adaptive tolerance (work = tol + tolE1 + tolE2). */
} occtl_topo_sew_options_t;

/** Default-value initialiser for #occtl_topo_sew_options_t. */
#define OCCTL_TOPO_SEW_OPTIONS_INIT                                                                \
  {OCCTL_TOPO_SEW_OPTIONS_VERSION_1, NULL, 1.0e-6, 0.0, 0.0, 1, 1, 0, 0, 1, 1, 0, 0}

/**
 * Initialises @p options to default values matching #OCCTL_TOPO_SEW_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_sew
 */
OCCTL_API void OCCTL_CALL occtl_topo_sew_options_init(occtl_topo_sew_options_t* options);

#define OCCTL_TOPO_SEW_RESULT_VERSION_1 1u

/** Diagnostic snapshot produced by #occtl_topo_sew. */
typedef struct occtl_topo_sew_result
{
  uint32_t    struct_version;         /**< INPUT — caller declares which version they understand. */
  const void* p_next;                 /**< Reserved; set to NULL. */
  int32_t     is_done;                /**< 1 if sewing completed successfully. */
  uint32_t    free_edge_count_before; /**< Free edges detected before sewing. */
  uint32_t    free_edge_count_after;  /**< Free edges remaining after sewing. */
  uint32_t    sewn_edge_count;        /**< Edge pairs successfully sewn. */
  uint32_t    multiple_edge_count;    /**< Edges shared by >2 faces. */
  uint32_t    degenerated_edge_count; /**< Degenerate edges detected or created. */
  uint32_t    deleted_face_count;     /**< Small faces removed by face analysis. */
  uint32_t    rejected_by_tolerance_count; /**< Candidate pairs rejected by MaxTolerance. */
} occtl_topo_sew_result_t;

/** Default-value initialiser for #occtl_topo_sew_result_t. */
#define OCCTL_TOPO_SEW_RESULT_INIT                                                                 \
  {OCCTL_TOPO_SEW_RESULT_VERSION_1, NULL, 0, 0u, 0u, 0u, 0u, 0u, 0u, 0u}

/**
 * Initialises @p result to default zeros, ready for filling by
 * #occtl_topo_sew.
 *
 * NULL-tolerant.
 *
 * @param[out] result  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_sew
 */
OCCTL_API void OCCTL_CALL occtl_topo_sew_result_init(occtl_topo_sew_result_t* result);

/**
 * Sews free edges of @p graph in place.
 *
 * Wraps @c BRepGraphAlgo_Sewing::Perform.  Caller-allocated @p options
 * and @p out_result; both versioned.  Pass NULL for @p options to use
 * the defaults; @p out_result is optional and may be NULL when the
 * caller does not need diagnostics.
 *
 * @param[in,out] graph        Borrows it.  Must be non-NULL.
 * @param[in]     options      Borrows it.  May be NULL.
 * @param[out]    out_result   Borrows it.  May be NULL.  When non-NULL must
 *                             carry a supported struct_version.
 *
 * @retval OCCTL_OK                On success (sewing may still report is_done == 0).
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @p options or @p out_result has an
 *                                 unsupported struct_version.
 *
 * @threadsafe No — mutates the graph.
 *
 * @sa occtl_topo_sew_options_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_sew(occtl_graph_t*                  graph,
                                                   const occtl_topo_sew_options_t* options,
                                                   occtl_topo_sew_result_t*        out_result);

#define OCCTL_TOPO_SAME_PARAMETER_OPTIONS_VERSION_1 1u

/** Configuration for #occtl_topo_recompute_same_parameter. */
typedef struct occtl_topo_same_parameter_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_TOPO_SAME_PARAMETER_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  double      tolerance;      /**< Reference tolerance for pass/fail.  Default 0 (auto). */
  int32_t     history_mode;   /**< 1 to record tolerance mutations to graph history. */
} occtl_topo_same_parameter_options_t;

#define OCCTL_TOPO_SAME_PARAMETER_OPTIONS_INIT                                                     \
  {OCCTL_TOPO_SAME_PARAMETER_OPTIONS_VERSION_1, NULL, 0.0, 0}

/** @copydoc occtl_topo_sew_options_init */
OCCTL_API void OCCTL_CALL
  occtl_topo_same_parameter_options_init(occtl_topo_same_parameter_options_t* options);

/**
 * Recomputes SameParameter on every active edge of @p graph.
 *
 * Wraps @c BRepGraphAlgo_SameParameter::Perform on the full edge set.
 * Tolerance is updated edge-by-edge.
 *
 * @param[in,out] graph                  Borrows it.  Must be non-NULL.
 * @param[in]     options                Borrows it.  May be NULL for defaults.
 * @param[out]    out_c0_fallback_count    Borrows it.  May be NULL.  Receives
 *                                       the number of edges where a C0 BSpline
 *                                       PCurve could not be promoted to C1.
 * @param[out]    out_approx_fallback_count Borrows it.  May be NULL.  Receives
 *                                        the number of edges where Approx
 *                                        SameParameter failed.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 *
 * @threadsafe No — mutates the graph.
 *
 * @sa occtl_topo_sew
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_recompute_same_parameter(occtl_graph_t*                             graph,
                                      const occtl_topo_same_parameter_options_t* options,
                                      uint32_t* out_c0_fallback_count,
                                      uint32_t* out_approx_fallback_count);

/** Severity classification for #occtl_topo_check_issue_t. */
typedef enum occtl_topo_check_severity
{
  OCCTL_TOPO_CHECK_WARNING = 0, /**< Non-critical; may be acceptable. */
  OCCTL_TOPO_CHECK_ERROR   = 1, /**< Invalid topology/geometry; should be fixed. */
  OCCTL_TOPO_CHECK_FATAL   = 2, /**< Severe corruption; further processing blocked. */
  OCCTL_TOPO_CHECK_SEVERITY_RESERVED_FUTURE = 0x7fffffff
} occtl_topo_check_severity_t;

/**
 * Structured diagnostic record produced by #occtl_topo_check.
 *
 * Mirrors @c BRepGraphCheck_Issue.  @p status_bit is a single power-of-2
 * value from one of OCCT's per-topology-level status enums (the
 * caller's responsibility to map by NodeId kind via
 * #occtl_graph_node_kind).
 */
typedef struct occtl_topo_check_issue
{
  occtl_node_id_t node_id;         /**< Node with the problem. */
  occtl_node_id_t context_node_id; /**< Parent context (e.g. face for edge-in-face), or invalid. */
  uint32_t        status_bit;      /**< Single bit from the per-level status enum. */
  occtl_topo_check_severity_t severity; /**< Severity classification. */
} occtl_topo_check_issue_t;

/**
 * Runs the full validation suite on @p graph and reports diagnostic
 * issues.
 *
 * Two-call buffer (§10.1 of @c ABI_PATTERNS.md): pass @p out_issues as
 * NULL with @p cap=0 to size; reissue with a buffer of length at least
 * the returned @p out_count.
 *
 * @param[in]     graph       Must be non-NULL.
 * @param[out]    out_issues  Borrows it.  Length @p cap.  May be NULL on
 *                            the sizing call.
 * @param[in]     cap         Capacity of @p out_issues.
 * @param[out]    out_count   Borrows it.  Receives the total issue count.
 *
 * @retval OCCTL_OK                Success — either sizing or full fill.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p cap < total issue count and
 *                                 @p out_issues is non-NULL.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_sew
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_check(const occtl_graph_t*      graph,
                                                     occtl_topo_check_issue_t* out_issues,
                                                     size_t                    cap,
                                                     size_t*                   out_count);

#define OCCTL_TOPO_FILLET_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_fillet.
 *
 * When @c chamfer_mode is 0, a constant-radius fillet is applied using
 * @c radius.  When @c chamfer_mode is 1, a chamfer is applied using
 * @c chamfer_dist1 and @c chamfer_dist2.
 */
typedef struct occtl_topo_fillet_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_TOPO_FILLET_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  double      radius;         /**< Fillet radius.  Default 1.0. */
  int32_t     chamfer_mode;   /**< 1 = chamfer, 0 = fillet.  Default 0. */
  double      chamfer_dist1;  /**< First chamfer distance.  Default 0.0. */
  double      chamfer_dist2;  /**< Second chamfer distance.  Default 0.0. */
} occtl_topo_fillet_options_t;

/** Default-value initialiser for #occtl_topo_fillet_options_t. */
#define OCCTL_TOPO_FILLET_OPTIONS_INIT {OCCTL_TOPO_FILLET_OPTIONS_VERSION_1, NULL, 1.0, 0, 0.0, 0.0}

/**
 * Initialises @p opts to default values matching #OCCTL_TOPO_FILLET_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_fillet
 */
OCCTL_API void OCCTL_CALL occtl_topo_fillet_options_init(occtl_topo_fillet_options_t* opts);

/**
 * Applies a 3D fillet or chamfer to all solids and standalone shells in @p graph.
 *
 * Every edge of every top-level solid and standalone shell is processed;
 * the result shape is placed into a newly-allocated output graph.
 *
 * @param[in]     graph      Borrows it.  Must be non-NULL.  Provides the
 *                           input topology (not modified).
 * @param[in]     opts       Borrows it.  Must be non-NULL and carry a
 *                           supported struct_version.
 * @param[out]    out_graph  Owns it.  Must be non-NULL.  Receives a
 *                           newly-allocated graph handle.
 * @param[out]    out_root   Borrows it.  Must be non-NULL.  Receives the
 *                           root NodeId of the filleted result in
 *                           @p *out_graph.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p opts, @p out_graph, or
 *                                 @p out_root is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @p opts has an unsupported struct_version.
 * @retval OCCTL_GEOMETRY_INVALID  The algorithm failed to produce a valid
 *                                 result (e.g. radius too large for the
 *                                 geometry).
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_fillet_options_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_fillet(occtl_graph_t*                     graph,
                                                      const occtl_topo_fillet_options_t* opts,
                                                      occtl_graph_t**                    out_graph,
                                                      occtl_node_id_t*                   out_root);

#define OCCTL_TOPO_EDGE_BLEND_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_blend_edges.
 *
 * When @c chamfer_mode is 0, every selected edge receives a constant-radius
 * fillet using @c radius. When @c chamfer_mode is 1, every selected edge
 * receives a chamfer using @c chamfer_dist1 and @c chamfer_dist2.
 */
typedef struct occtl_topo_edge_blend_options
{
  uint32_t               struct_version; /**< Must be #OCCTL_TOPO_EDGE_BLEND_OPTIONS_VERSION_1. */
  const void*            p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t        root;           /**< Borrows it. Shape root to modify. */
  const occtl_node_id_t* edges;          /**< Borrows it. Selected Edge NodeIds. */
  size_t                 edge_count;     /**< Number of entries in @c edges. */
  double                 radius;         /**< Fillet radius. Default 1.0. */
  int32_t                chamfer_mode;   /**< 1 = chamfer, 0 = fillet. Default 0. */
  double                 chamfer_dist1;  /**< First chamfer distance. Default 0.0. */
  double                 chamfer_dist2;  /**< Second chamfer distance. Default 0.0. */
} occtl_topo_edge_blend_options_t;

#define OCCTL_TOPO_EDGE_BLEND_OPTIONS_INIT                                                         \
  {OCCTL_TOPO_EDGE_BLEND_OPTIONS_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, NULL, 0, 1.0, 0, 0.0, 0.0}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_EDGE_BLEND_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_blend_edges
 */
OCCTL_API void OCCTL_CALL occtl_topo_edge_blend_options_init(occtl_topo_edge_blend_options_t* opts);

/**
 * Applies a selected-edge 3D fillet or chamfer to @p options->root.
 *
 * Output is a new graph; the input graph is not modified. Unlike
 * #occtl_topo_fillet, this operation is scoped to the selected edge set in
 * @p options.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the modified root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, @c edge_count
 *                                 is zero, or a distance/radius is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root or a selected edge is invalid,
 *                                 removed, or cannot be resolved.
 * @retval OCCTL_WRONG_KIND        A selected NodeId is not an Edge.
 * @retval OCCTL_GEOMETRY_INVALID  The algorithm failed to produce a valid result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_transformed
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_blend_edges(const occtl_graph_t*                   graph,
                         const occtl_topo_edge_blend_options_t* options,
                         occtl_graph_t**                        out_graph,
                         occtl_node_id_t*                       out_root);

#define OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_max_fillet_radius.
 *
 * The operation estimates a conservative maximum constant fillet radius for a
 * selected edge set by repeatedly running OCCT's 3D fillet builder.
 */
typedef struct occtl_topo_max_fillet_radius_options
{
  uint32_t        struct_version; /**< Must be #OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t root;           /**< Shape root that owns the selected edges. */
  const occtl_node_id_t* edges;   /**< Borrows it. Selected Edge NodeIds. */
  size_t                 edge_count; /**< Number of entries in @c edges. */
  double                 min_radius; /**< Lower search bound. Default 1.0e-6. */
  double                 max_radius; /**< Upper search bound; <= 0 lets OCCT-Light derive one. */
  double                 tolerance;  /**< Stop tolerance for binary search. Default 1.0e-4. */
  int32_t                max_iterations; /**< Maximum trial builds. Default 24. */
} occtl_topo_max_fillet_radius_options_t;

#define OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_INIT                                                  \
  {OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_VERSION_1,                                                 \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   NULL,                                                                                           \
   0,                                                                                              \
   1.0e-6,                                                                                         \
   0.0,                                                                                            \
   1.0e-4,                                                                                         \
   24}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_max_fillet_radius
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_max_fillet_radius_options_init(occtl_topo_max_fillet_radius_options_t* opts);

/**
 * Estimates the largest constant-radius fillet accepted for selected edges.
 *
 * The input graph is not modified. The estimate is conservative: each trial
 * radius is validated by OCCT @c BRepFilletAPI_MakeFillet::Build(), and the
 * largest successful trial within @p options->tolerance is returned.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_radius Borrows it. Must be non-NULL. Receives the estimated
 *                        maximum radius.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, @c edge_count
 *                                 is zero, or a search option is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root or a selected edge is invalid,
 *                                 removed, or cannot be resolved.
 * @retval OCCTL_WRONG_KIND        A selected NodeId is not an Edge.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not find a successful radius.
 *
 * @threadsafe No. Runs trial OCCT fillet builds.
 *
 * @sa occtl_topo_blend_edges, occtl_topo_edge_blend_options_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_max_fillet_radius(const occtl_graph_t*                          graph,
                               const occtl_topo_max_fillet_radius_options_t* options,
                               double*                                       out_radius);

/**
 * Creates a transformed copy of the shape rooted at @p root.
 *
 * Output is a new graph; the input graph is not modified.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  root       NodeId of the shape to transform.
 * @param[in]  transform  Affine transform to apply.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the transformed root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_graph, or @p out_root is NULL.
 * @retval OCCTL_NOT_FOUND         @p root is invalid, removed, or cannot be resolved.
 * @retval OCCTL_GEOMETRY_INVALID  The transformed shape is null.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_blend_edges
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_transformed(const occtl_graph_t* graph,
                                                           occtl_node_id_t      root,
                                                           occtl_transform_t    transform,
                                                           occtl_graph_t**      out_graph,
                                                           occtl_node_id_t*     out_root);

#define OCCTL_TOPO_PROJECT_ON_FACE_OPTIONS_VERSION_1 1u

/** Configuration for #occtl_topo_project_on_face. */
typedef struct occtl_topo_project_on_face_options
{
  uint32_t        struct_version; /**< Must be #OCCTL_TOPO_PROJECT_ON_FACE_OPTIONS_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t source;         /**< Borrows it. Edge or wire to project. */
  occtl_node_id_t face;           /**< Borrows it. Target Face NodeId. */
  double          tolerance_3d;   /**< 3D approximation tolerance. Default 1.0e-4. */
  double          tolerance_2d;   /**< 2D approximation tolerance. Default 1.0e-8. */
  int32_t         max_degree;     /**< Maximum approximation degree. Default 14. */
  int32_t         max_segments;   /**< Maximum approximation segments. Default 16. */
  double          max_distance;  /**< Max source-to-target distance. Negative disables the limit. */
  int32_t         limit_to_face; /**< 0/1. Trim projections to face boundaries. Default 1. */
  int32_t         compute_3d;    /**< 0/1. Build 3D projected curves. Default 1. */
} occtl_topo_project_on_face_options_t;

#define OCCTL_TOPO_PROJECT_ON_FACE_OPTIONS_INIT                                                    \
  {OCCTL_TOPO_PROJECT_ON_FACE_OPTIONS_VERSION_1,                                                   \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   1.0e-4,                                                                                         \
   1.0e-8,                                                                                         \
   14,                                                                                             \
   16,                                                                                             \
   -1.0,                                                                                           \
   1,                                                                                              \
   1}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_PROJECT_ON_FACE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_project_on_face
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_project_on_face_options_init(occtl_topo_project_on_face_options_t* opts);

/**
 * Projects an edge or wire onto a target face along face normals.
 *
 * Output is a new graph; the input graph is not modified.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the projected root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, or an option
 *                                 value is outside the supported range.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c source or @c face is invalid, removed,
 *                                 or cannot be resolved.
 * @retval OCCTL_WRONG_KIND        @c source is not an Edge or Wire, or
 *                                 @c face is not a Face.
 * @retval OCCTL_GEOMETRY_INVALID  Projection failed to produce a result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_make_filling
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_project_on_face(const occtl_graph_t*                        graph,
                             const occtl_topo_project_on_face_options_t* options,
                             occtl_graph_t**                             out_graph,
                             occtl_node_id_t*                            out_root);

#define OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_wrap_on_face.
 *
 * Wraps a planar Edge, Wire, or Face onto @c target_face, starting from a
 * surface placement.  The placement origin must lie on or near the target
 * face.  Its X axis defines the local planar +X direction on the surface;
 * its Z axis defines the starting surface normal.  The algorithm samples the
 * planar source, follows the target surface through OCCT face intersections,
 * interpolates wrapped B-spline edges, fixes wires, and builds a filling face
 * when @c source is a Face.
 */
typedef struct occtl_topo_wrap_on_face_options
{
  uint32_t        struct_version; /**< Must be #OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t source;         /**< Borrows it. Planar Edge, Wire, or Face to wrap. */
  occtl_node_id_t target_face;    /**< Borrows it. Target Face node ID. */
  occtl_axis3_placement_t surface_location; /**< Start placement on the target surface. */
  double                  tolerance; /**< Allowed wrapped-edge length error. Default 1.0e-3. */
  int32_t                 initial_subdivisions; /**< Initial samples per source edge. Default 4. */
  int32_t                 max_refinements;      /**< Maximum adaptive refinements. Default 8. */
  double intersection_extent; /**< Ray half-length; <= 0 derives from target bounds. */
  double wire_fix_tolerance;  /**< Wire fixing tolerance; <= 0 uses @c tolerance. */
} occtl_topo_wrap_on_face_options_t;

#define OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_INIT                                                       \
  {OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_VERSION_1,                                                      \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}},                           \
   1.0e-3,                                                                                         \
   4,                                                                                              \
   8,                                                                                              \
   0.0,                                                                                            \
   0.0}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_wrap_on_face
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_wrap_on_face_options_init(occtl_topo_wrap_on_face_options_t* opts);

/**
 * Wraps a planar Edge, Wire, or Face onto a target Face.
 *
 * Output is a new graph; the input graph is not modified. The result conforms
 * planar input to the target Face.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the wrapped result root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL or an option is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c source or @c target_face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @c source is not an Edge, Wire, or Face, or
 *                                 @c target_face is not a Face.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not construct a wrapped result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into the output graph.
 *
 * @threadsafe No. Allocates a new graph and runs OCCT modeling algorithms.
 *
 * @sa occtl_topo_project_on_face, occtl_topo_make_filling_patch
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wrap_on_face(const occtl_graph_t*                     graph,
                          const occtl_topo_wrap_on_face_options_t* options,
                          occtl_graph_t**                          out_graph,
                          occtl_node_id_t*                         out_root);

#define OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_VERSION_1 1u

/** Configuration for #occtl_topo_project_face_along_direction. */
typedef struct occtl_topo_project_face_direction_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t    source_face;  /**< Borrows it. Planar or curved Face to project. */
  occtl_node_id_t    target;       /**< Borrows it. Target Face, Shell, Solid, or Compound. */
  occtl_direction3_t direction;    /**< Projection direction. Need not be unit length. */
  double             max_distance; /**< Extrusion distance. <= 0 derives one from bounds. */
  int32_t            copy_source;  /**< 0/1. Copy source geometry before extrusion. Default 1. */
} occtl_topo_project_face_direction_options_t;

#define OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_INIT                                             \
  {OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_VERSION_1,                                            \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {0.0, 0.0, -1.0},                                                                               \
   -1.0,                                                                                           \
   1}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_project_face_along_direction
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_project_face_direction_options_init(occtl_topo_project_face_direction_options_t* opts);

/**
 * Projects a face onto target boundary faces along a fixed direction.
 *
 * Output is a new graph; the input graph is not modified. Internally OCCT-Light
 * builds a prism from @c source_face, intersects it with the target boundary
 * faces using OCCT Boolean common, and stores the resulting faces or shells in
 * a fresh BRepGraph. For Solid, CompSolid, and Compound targets, only boundary
 * faces are used, so the operation returns projected surface fragments rather
 * than a volumetric intersection.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the projected result root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, @c direction
 *                                 is zero/non-finite, or @c max_distance is
 *                                 non-finite.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c source_face or @c target is invalid,
 *                                 removed, or cannot be resolved.
 * @retval OCCTL_WRONG_KIND        @c source_face is not a Face, or @c target
 *                                 has no boundary faces.
 * @retval OCCTL_GEOMETRY_INVALID  Projection failed to produce a result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_project_on_face
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_project_face_along_direction(
  const occtl_graph_t*                               graph,
  const occtl_topo_project_face_direction_options_t* options,
  occtl_graph_t**                                    out_graph,
  occtl_node_id_t*                                   out_root);

#define OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_VERSION_1 1u

/** Configuration for #occtl_topo_face_to_arcs. */
typedef struct occtl_topo_face_to_arcs_options
{
  uint32_t        struct_version; /**< Must be #OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t source;         /**< Borrows it. Face or closed planar Wire to convert. */
  double angular_tolerance; /**< Arc/segment approximation angular tolerance. Default 1.0e-3. */
} occtl_topo_face_to_arcs_options_t;

#define OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_INIT                                                       \
  {OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, 1.0e-3}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_face_to_arcs
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_face_to_arcs_options_init(occtl_topo_face_to_arcs_options_t* opts);

/**
 * Converts planar face or wire boundaries to line and circular-arc edges.
 *
 * Output is a new graph; the input graph is not modified. Face input returns
 * a Face root. Wire input must be closed and planar enough to define a support
 * face; the returned root is the converted outer Wire.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the converted Face or Wire root.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, or
 *                                 @c angular_tolerance is non-positive or
 *                                 non-finite.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c source is invalid, removed, or cannot be
 *                                 resolved.
 * @retval OCCTL_WRONG_KIND        @c source is neither Face nor Wire.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT could not build or convert the face.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into the output graph.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_project_face_along_direction
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_to_arcs(const occtl_graph_t*                     graph,
                          const occtl_topo_face_to_arcs_options_t* options,
                          occtl_graph_t**                          out_graph,
                          occtl_node_id_t*                         out_root);

#define OCCTL_TOPO_HLR_OPTIONS_VERSION_1 1u

/** Hidden-line removal algorithm used by #occtl_topo_make_hlr_projection. */
typedef enum occtl_topo_hlr_mode
{
  OCCTL_TOPO_HLR_BREP                 = 0, /**< Exact BRep HLR via OCCT HLRBRep_Algo. */
  OCCTL_TOPO_HLR_POLY                 = 1, /**< Poly HLR via OCCT HLRBRep_PolyAlgo. */
  OCCTL_TOPO_HLR_MODE_RESERVED_FUTURE = 0x7fffffff
} occtl_topo_hlr_mode_t;

/**
 * Configuration for #occtl_topo_make_hlr_projection.
 *
 * Projects a graph root through OCCT hidden-line removal. The frame follows
 * OCCT HLR convention: @c projection_frame.location is the projection target,
 * @c projection_frame.z_dir is the viewing direction, and
 * @c projection_frame.x_dir is the horizontal direction of the drawing plane.
 * Set @c focus <= 0.0 for parallel projection, or a positive focal distance
 * for perspective projection.
 */
typedef struct occtl_topo_hlr_options
{
  uint32_t                struct_version;   /**< Must be #OCCTL_TOPO_HLR_OPTIONS_VERSION_1. */
  const void*             p_next;           /**< Reserved; set to NULL. */
  occtl_node_id_t         root;             /**< Borrows it. Shape root to project. */
  occtl_axis3_placement_t projection_frame; /**< Camera-like projection frame. */
  double                focus; /**< Perspective focus distance; <= 0 selects parallel projection. */
  int32_t               include_hidden;  /**< 0/1; include hidden categories. Default 1. */
  int32_t               include_smooth;  /**< 0/1; include smooth and seam categories. Default 1. */
  int32_t               include_outline; /**< 0/1; include outline categories. Default 1. */
  occtl_topo_hlr_mode_t mode;            /**< HLR algorithm. Default #OCCTL_TOPO_HLR_BREP. */
} occtl_topo_hlr_options_t;

#define OCCTL_TOPO_HLR_OPTIONS_INIT                                                                \
  {OCCTL_TOPO_HLR_OPTIONS_VERSION_1,                                                               \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}},                           \
   0.0,                                                                                            \
   1,                                                                                              \
   1,                                                                                              \
   1,                                                                                              \
   OCCTL_TOPO_HLR_BREP}

#define OCCTL_TOPO_HLR_RESULT_VERSION_1 1u

/**
 * Result roots produced by #occtl_topo_make_hlr_projection.
 *
 * @c struct_version is INPUT — the caller declares which version they
 * understand.  @c graph owns all non-invalid category roots and must be
 * released with #occtl_graph_free.  Categories that OCCT did not produce,
 * or that were disabled in #occtl_topo_hlr_options_t, are set to
 * #OCCTL_NODE_ID_INVALID.
 */
typedef struct occtl_topo_hlr_result
{
  uint32_t        struct_version;  /**< Must be #OCCTL_TOPO_HLR_RESULT_VERSION_1. */
  const void*     p_next;          /**< Reserved; set to NULL. */
  occtl_graph_t*  graph;           /**< Owns it. Output graph, or NULL on failure. */
  occtl_node_id_t visible_sharp;   /**< Visible hard/sharp edge compound root. */
  occtl_node_id_t visible_smooth;  /**< Visible smooth tangent edge compound root. */
  occtl_node_id_t visible_seam;    /**< Visible seam edge compound root. */
  occtl_node_id_t visible_outline; /**< Visible outline edge compound root. */
  occtl_node_id_t hidden_sharp;    /**< Hidden hard/sharp edge compound root. */
  occtl_node_id_t hidden_smooth;   /**< Hidden smooth tangent edge compound root. */
  occtl_node_id_t hidden_seam;     /**< Hidden seam edge compound root. */
  occtl_node_id_t hidden_outline;  /**< Hidden outline edge compound root. */
} occtl_topo_hlr_result_t;

/** Default-value initialiser for #occtl_topo_hlr_result_t. */
#define OCCTL_TOPO_HLR_RESULT_INIT                                                                 \
  {OCCTL_TOPO_HLR_RESULT_VERSION_1,                                                                \
   NULL,                                                                                           \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID}

/**
 * Initialises @p result to defaults matching #OCCTL_TOPO_HLR_RESULT_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] result  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_hlr_projection
 */
OCCTL_API void OCCTL_CALL occtl_topo_hlr_result_init(occtl_topo_hlr_result_t* result);

/**
 * Initialises @p options to default values matching
 * #OCCTL_TOPO_HLR_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_hlr_projection
 */
OCCTL_API void OCCTL_CALL occtl_topo_hlr_options_init(occtl_topo_hlr_options_t* options);

/**
 * Projects a graph root with OCCT hidden-line removal.
 *
 * The output graph contains one root per non-empty requested category. Each
 * category is an OCCT-generated edge compound ingested into BRepGraph, so
 * downstream drawing exporters can traverse ordinary graph topology instead
 * of owning HLR-specific objects.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_result Borrows it. Receives an owned output graph and
 *                        category roots. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  A required pointer is NULL, @c p_next is
 *                                 non-NULL, or an option value is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid, removed, or cannot be
 *                                 resolved.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT HLR produced no requested edge category.
 * @retval OCCTL_TOPOLOGY_INVALID  A projected category could not be ingested
 *                                 into the output graph.
 *
 * @threadsafe No. Allocates a new graph and runs OCCT HLR algorithms.
 *
 * @sa occtl_curve_to_bezier_segments, occtl_topo_face_to_arcs
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_hlr_projection(const occtl_graph_t*            graph,
                                 const occtl_topo_hlr_options_t* options,
                                 occtl_topo_hlr_result_t*        out_result);

#define OCCTL_TOPO_DRAFT_FACES_OPTIONS_VERSION_1 1u

/** Configuration for #occtl_topo_draft_faces. */
typedef struct occtl_topo_draft_faces_options
{
  uint32_t               struct_version; /**< Must be #OCCTL_TOPO_DRAFT_FACES_OPTIONS_VERSION_1. */
  const void*            p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t        root;           /**< Borrows it. Shape root to draft. */
  const occtl_node_id_t* faces;          /**< Borrows it. Selected Face NodeIds. */
  size_t                 face_count;     /**< Number of entries in @c faces. */
  occtl_direction3_t     pull_direction; /**< Direction that defines material side. Default +Z. */
  occtl_point3_t         neutral_point;  /**< Point on the neutral plane. Default origin. */
  occtl_direction3_t     neutral_normal; /**< Neutral plane normal. Default +Z. */
  double  angle;       /**< Draft angle in radians. Default #OCCTL_ANGLE_5_DEG_RAD (5 degrees). */
  int32_t keep_inside; /**< OCCT draft flag. Default 1. */
} occtl_topo_draft_faces_options_t;

#define OCCTL_TOPO_DRAFT_FACES_OPTIONS_INIT                                                        \
  {OCCTL_TOPO_DRAFT_FACES_OPTIONS_VERSION_1,                                                       \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   NULL,                                                                                           \
   0,                                                                                              \
   {0.0, 0.0, 1.0},                                                                                \
   {0.0, 0.0, 0.0},                                                                                \
   {0.0, 0.0, 1.0},                                                                                \
   OCCTL_ANGLE_5_DEG_RAD,                                                                          \
   1}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_DRAFT_FACES_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_draft_faces
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_draft_faces_options_init(occtl_topo_draft_faces_options_t* opts);

/**
 * Applies a draft angle to selected faces of a shape.
 *
 * Output is a new graph; the input graph is not modified.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the drafted root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, @c face_count
 *                                 is zero, or @c angle is zero.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root or a selected face is invalid,
 *                                 removed, or cannot be resolved.
 * @retval OCCTL_WRONG_KIND        A selected NodeId is not a Face.
 * @retval OCCTL_GEOMETRY_INVALID  Draft failed to produce a valid result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_blend_edges
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_draft_faces(const occtl_graph_t*                    graph,
                         const occtl_topo_draft_faces_options_t* options,
                         occtl_graph_t**                         out_graph,
                         occtl_node_id_t*                        out_root);

#define OCCTL_TOPO_DEFEATURE_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_defeature.
 */
typedef struct occtl_topo_defeature_options
{
  uint32_t               struct_version; /**< Must be #OCCTL_TOPO_DEFEATURE_OPTIONS_VERSION_1. */
  const void*            p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t        root;       /**< Borrows it. Solid, compsolid, shell, or compound root. */
  const occtl_node_id_t* selections; /**< Borrows it. Feature topology selections. */
  size_t                 selection_count; /**< Number of entries in @c selections. */
  int32_t                parallel;        /**< 0/1. Allow OCCT parallel execution. Default 0. */
} occtl_topo_defeature_options_t;

#define OCCTL_TOPO_DEFEATURE_OPTIONS_INIT                                                          \
  {OCCTL_TOPO_DEFEATURE_OPTIONS_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, NULL, 0, 0}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_DEFEATURE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_defeature
 */
OCCTL_API void OCCTL_CALL occtl_topo_defeature_options_init(occtl_topo_defeature_options_t* opts);

/**
 * Removes selected features from @p options->root.
 *
 * Output is a new graph; the input graph is not modified. Each selected node
 * may be a Face, Shell, Solid, CompSolid, or Compound. Non-face selections are
 * expanded to their contained faces before calling OCCT's defeaturing kernel.
 * This covers feature selections at several topology levels while preserving
 * OCCT's face-based removal semantics internally.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  options      Borrows it. Must be non-NULL and carry a supported
 *                          @c struct_version.
 * @param[out] out_graph    Owns it. Receives a new graph on success.
 * @param[out] out_root     Borrows it. Receives the defeatured root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, or
 *                                 @c selection_count is zero.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root or a selected node is invalid,
 *                                 removed, or cannot be resolved.
 * @retval OCCTL_WRONG_KIND        A selected NodeId cannot be expanded to faces.
 * @retval OCCTL_GEOMETRY_INVALID  Defeaturing failed to produce a valid result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No. Allocates a new graph.
 *
 * @sa occtl_topo_draft_faces
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_defeature(const occtl_graph_t*                  graph,
                       const occtl_topo_defeature_options_t* options,
                       occtl_graph_t**                       out_graph,
                       occtl_node_id_t*                      out_root);

#define OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_make_offset_features.
 */
typedef struct occtl_topo_offset_features_options
{
  uint32_t        struct_version; /**< Must be #OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t root;           /**< Borrows it. Shape root to offset. */
  const occtl_node_id_t*   selections;       /**< Borrows it. Feature topology selections. */
  size_t                   selection_count;  /**< Number of entries in @c selections. */
  double                   base_offset;      /**< Offset for unselected faces. Default 0.0. */
  double                   selection_offset; /**< Offset for selected faces. Default 1.0. */
  double                   tolerance;        /**< Construction tolerance. Default 1.0e-3. */
  occtl_offset_join_type_t join;         /**< Edge join style. Default #OCCTL_OFFSET_JOIN_ARC. */
  int32_t                  intersection; /**< 0/1. Intersect generated parallels. Default 0. */
  int32_t self_intersection;             /**< 0/1. Request self-intersection handling. Default 0. */
  int32_t remove_internal_edges;         /**< 0/1. Remove internal result edges. Default 0. */
} occtl_topo_offset_features_options_t;

#define OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT                                                    \
  {OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_VERSION_1,                                                   \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   NULL,                                                                                           \
   0,                                                                                              \
   0.0,                                                                                            \
   1.0,                                                                                            \
   1.0e-3,                                                                                         \
   OCCTL_OFFSET_JOIN_ARC,                                                                          \
   0,                                                                                              \
   0,                                                                                              \
   0}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_offset_features
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_offset_features_options_init(occtl_topo_offset_features_options_t* opts);

/**
 * Offsets selected features of @p options->root.
 *
 * Output is a new graph; the input graph is not modified. Each selected node
 * may be a Face, Shell, Solid, CompSolid, or Compound. Non-face selections are
 * expanded to their contained faces, and those faces receive
 * @c selection_offset while unselected faces use @c base_offset.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  options      Borrows it. Must be non-NULL and carry a supported
 *                          @c struct_version.
 * @param[out] out_graph    Owns it. Receives a new graph on success.
 * @param[out] out_root     Borrows it. Receives the offset result root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL,
 *                                 @c selection_count is zero, @c tolerance is
 *                                 non-positive, or both offsets are zero.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root or a selected node is invalid,
 *                                 removed, or cannot be resolved.
 * @retval OCCTL_WRONG_KIND        A selected NodeId cannot be expanded to faces.
 * @retval OCCTL_GEOMETRY_INVALID  Offset failed to produce a valid result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No. Allocates a new graph.
 *
 * @sa occtl_topo_defeature
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_offset_features(const occtl_graph_t*                        graph,
                                  const occtl_topo_offset_features_options_t* options,
                                  occtl_graph_t**                             out_graph,
                                  occtl_node_id_t*                            out_root);

typedef enum occtl_topo_filling_continuity
{
  OCCTL_TOPO_FILLING_C0                         = 0, /**< Position continuity. */
  OCCTL_TOPO_FILLING_G1                         = 1, /**< Tangency continuity. */
  OCCTL_TOPO_FILLING_G2                         = 2, /**< Curvature continuity. */
  OCCTL_TOPO_FILLING_CONTINUITY_RESERVED_FUTURE = 0x7fffffff
} occtl_topo_filling_continuity_t;

#define OCCTL_TOPO_FILLING_OPTIONS_VERSION_1 1u

/** Configuration for #occtl_topo_make_filling. */
typedef struct occtl_topo_filling_options
{
  uint32_t               struct_version;      /**< Must be #OCCTL_TOPO_FILLING_OPTIONS_VERSION_1. */
  const void*            p_next;              /**< Reserved; set to NULL. */
  const occtl_node_id_t* edges;               /**< Borrows it. Ordered boundary Edge NodeIds. */
  size_t                 edge_count;          /**< Number of entries in @c edges. */
  occtl_topo_filling_continuity_t continuity; /**< Boundary continuity. Default C0. */
  int32_t                         degree;     /**< Energy criterion degree. Default 3. */
  int32_t point_count_on_curve;               /**< Discretisation points per curve. Default 15. */
  int32_t iteration_count;                    /**< Solver iterations. Default 2. */
  int32_t anisotropic;         /**< 0/1. Improve elongated surface handling. Default 0. */
  double  tolerance_2d;        /**< 2D constraint tolerance. Default 1.0e-5. */
  double  tolerance_3d;        /**< 3D constraint tolerance. Default 1.0e-4. */
  double  angular_tolerance;   /**< Angular constraint tolerance. Default 1.0e-2. */
  double  curvature_tolerance; /**< Curvature constraint tolerance. Default 1.0e-1. */
  int32_t max_degree;          /**< Maximum approximation degree. Default 8. */
  int32_t max_segments;        /**< Maximum approximation segments. Default 9. */
} occtl_topo_filling_options_t;

#define OCCTL_TOPO_FILLING_OPTIONS_INIT                                                            \
  {OCCTL_TOPO_FILLING_OPTIONS_VERSION_1,                                                           \
   NULL,                                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   OCCTL_TOPO_FILLING_C0,                                                                          \
   3,                                                                                              \
   15,                                                                                             \
   2,                                                                                              \
   0,                                                                                              \
   1.0e-5,                                                                                         \
   1.0e-4,                                                                                         \
   1.0e-2,                                                                                         \
   1.0e-1,                                                                                         \
   8,                                                                                              \
   9}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_FILLING_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_filling
 */
OCCTL_API void OCCTL_CALL occtl_topo_filling_options_init(occtl_topo_filling_options_t* opts);

/**
 * Builds an N-side filling face from ordered boundary edges.
 *
 * Output is a new graph; the input graph is not modified.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  options      Borrows it. Must be non-NULL and carry a supported
 *                          @c struct_version.
 * @param[out] out_graph    Owns it. Receives a new graph on success.
 * @param[out] out_root     Borrows it. Receives the filling face root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, @c edge_count
 *                                 is less than 2, or a solver option is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         A selected edge is invalid, removed, or cannot
 *                                 be resolved.
 * @retval OCCTL_WRONG_KIND        A selected NodeId is not an Edge.
 * @retval OCCTL_GEOMETRY_INVALID  Filling failed to produce a valid result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_project_on_face
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_filling(const occtl_graph_t*                graph,
                          const occtl_topo_filling_options_t* options,
                          occtl_graph_t**                     out_graph,
                          occtl_node_id_t*                    out_root);

#define OCCTL_TOPO_FILLING_PATCH_OPTIONS_VERSION_1 1u

/**
 * Edge constraint for #occtl_topo_make_filling_patch.
 */
typedef struct occtl_topo_filling_patch_edge
{
  occtl_node_id_t edge;         /**< Boundary or free Edge constraint. */
  occtl_node_id_t support_face; /**< Optional support Face; use invalid NodeId for none. */
  occtl_topo_filling_continuity_t continuity; /**< C0/G1/G2 constraint. */
  int32_t is_boundary; /**< 0/1. Non-zero when edge bounds the output face. */
} occtl_topo_filling_patch_edge_t;

/**
 * Configuration for #occtl_topo_make_filling_patch.
 */
typedef struct occtl_topo_filling_patch_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_TOPO_FILLING_PATCH_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  const occtl_topo_filling_patch_edge_t* edges;      /**< Borrows it. Edge constraints. */
  size_t                                 edge_count; /**< Number of entries in @c edges. */
  const occtl_point3_t*                  points;     /**< Borrows it. Optional point constraints. */
  size_t                                 point_count; /**< Number of entries in @c points. */
  int32_t                                degree;      /**< Energy criterion degree. Default 3. */
  int32_t point_count_on_curve; /**< Discretisation points per curve. Default 15. */
  int32_t iteration_count;      /**< Solver iterations. Default 2. */
  int32_t anisotropic;          /**< 0/1. Improve elongated surface handling. Default 0. */
  double  tolerance_2d;         /**< 2D constraint tolerance. Default 1.0e-5. */
  double  tolerance_3d;         /**< 3D constraint tolerance. Default 1.0e-4. */
  double  angular_tolerance;    /**< Angular constraint tolerance. Default 1.0e-2. */
  double  curvature_tolerance;  /**< Curvature constraint tolerance. Default 1.0e-1. */
  int32_t max_degree;           /**< Maximum approximation degree. Default 8. */
  int32_t max_segments;         /**< Maximum approximation segments. Default 9. */
} occtl_topo_filling_patch_options_t;

#define OCCTL_TOPO_FILLING_PATCH_OPTIONS_INIT                                                      \
  {OCCTL_TOPO_FILLING_PATCH_OPTIONS_VERSION_1,                                                     \
   NULL,                                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   NULL,                                                                                           \
   0,                                                                                              \
   3,                                                                                              \
   15,                                                                                             \
   2,                                                                                              \
   0,                                                                                              \
   1.0e-5,                                                                                         \
   1.0e-4,                                                                                         \
   1.0e-2,                                                                                         \
   1.0e-1,                                                                                         \
   8,                                                                                              \
   9}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_FILLING_PATCH_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_filling_patch
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_filling_patch_options_init(occtl_topo_filling_patch_options_t* opts);

/**
 * Builds an N-side filling patch from edge, support-face, and point constraints.
 *
 * Output is a new graph; the input graph is not modified. This is the richer
 * graph-facing variant of #occtl_topo_make_filling: each edge may optionally
 * name a support Face for G1/G2 continuity, and additional 3D points can be
 * added as surface constraints.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the filling patch face root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, @c p_next is
 *                                 non-NULL, @c edge_count is less than 2,
 *                                 @c points is NULL while @c point_count is
 *                                 non-zero, or a solver option is invalid.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         A selected edge or support face is invalid,
 *                                 removed, or cannot be resolved.
 * @retval OCCTL_WRONG_KIND        A selected edge is not an Edge, or a support
 *                                 node is not a Face.
 * @retval OCCTL_GEOMETRY_INVALID  Filling failed to produce a valid result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_make_filling
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_filling_patch(const occtl_graph_t*                      graph,
                                const occtl_topo_filling_patch_options_t* options,
                                occtl_graph_t**                           out_graph,
                                occtl_node_id_t*                          out_root);

typedef enum occtl_topo_split_keep
{
  OCCTL_TOPO_SPLIT_KEEP_ALL             = 0, /**< Return all pieces produced by the split. */
  OCCTL_TOPO_SPLIT_KEEP_POSITIVE        = 1, /**< Keep the side in the normal direction. */
  OCCTL_TOPO_SPLIT_KEEP_NEGATIVE        = 2, /**< Keep the side opposite the normal direction. */
  OCCTL_TOPO_SPLIT_KEEP_RESERVED_FUTURE = 0x7fffffff
} occtl_topo_split_keep_t;

#define OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_make_split_by_plane.
 */
typedef struct occtl_topo_split_by_plane_options
{
  uint32_t           struct_version; /**< Must be #OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_VERSION_1. */
  const void*        p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t    root;           /**< Shape root to split. */
  occtl_point3_t     point;          /**< Point on the splitting plane. */
  occtl_direction3_t normal;         /**< Normal direction of the splitting plane. */
  occtl_topo_split_keep_t keep;      /**< Which side(s) to keep. Default ALL. */
} occtl_topo_split_by_plane_options_t;

#define OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_INIT                                                     \
  {OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_VERSION_1,                                                    \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {0.0, 0.0, 0.0},                                                                                \
   {0.0, 0.0, 1.0},                                                                                \
   OCCTL_TOPO_SPLIT_KEEP_ALL}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_split_by_plane
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_split_by_plane_options_init(occtl_topo_split_by_plane_options_t* opts);

/**
 * Splits a shape by a plane and returns selected side(s).
 *
 * Output is a new graph; the input graph is not modified. Keeping all pieces
 * uses OCCT's splitter with a finite plane face sized from the input bounds.
 * Keeping one side intersects the shape with an OCCT half-space.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the split result root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL or @c keep
 *                                 is not a supported value.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid, removed, or cannot be
 *                                 resolved.
 * @retval OCCTL_GEOMETRY_INVALID  The split failed or produced a null result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No. Allocates a new graph.
 *
 * @sa occtl_topo_mirrored
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_split_by_plane(const occtl_graph_t*                       graph,
                                 const occtl_topo_split_by_plane_options_t* options,
                                 occtl_graph_t**                            out_graph,
                                 occtl_node_id_t*                           out_root);

/**
 * One plane used by #occtl_topo_make_sections_by_planes.
 */
typedef struct occtl_topo_section_plane
{
  occtl_point3_t     point;  /**< Point on the section plane. */
  occtl_direction3_t normal; /**< Normal direction of the section plane. */
} occtl_topo_section_plane_t;

#define OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_make_sections_by_planes.
 */
typedef struct occtl_topo_section_by_planes_options
{
  uint32_t        struct_version; /**< Must be #OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t root;           /**< Shape root to section. */
  const occtl_topo_section_plane_t* planes;      /**< Borrows it. Section plane array. */
  size_t                            plane_count; /**< Number of entries in @c planes. */
  int32_t approximate;              /**< 0/1. Approximate generated section curves. Default 1. */
  int32_t compute_pcurves_on_root;  /**< 0/1. Attach pcurves on the source shape when possible. */
  int32_t compute_pcurves_on_plane; /**< 0/1. Attach pcurves on the plane tool when possible. */
} occtl_topo_section_by_planes_options_t;

#define OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_INIT                                                  \
  {OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, NULL, 0, 1, 0, 0}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_sections_by_planes
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_section_by_planes_options_init(occtl_topo_section_by_planes_options_t* opts);

/**
 * Sections a shape by one or more planes.
 *
 * Output is a new graph containing the generated section edges, usually as a
 * Compound root. The input graph is not modified. Multiple planes are sectioned
 * independently and collected into one compound.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the section result root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL or
 *                                 @c plane_count is zero.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @c root is invalid, removed, or cannot be
 *                                 resolved.
 * @retval OCCTL_GEOMETRY_INVALID  Sectioning failed or produced a null result.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No. Allocates a new graph.
 *
 * @sa occtl_topo_make_split_by_plane
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_sections_by_planes(const occtl_graph_t*                          graph,
                                     const occtl_topo_section_by_planes_options_t* options,
                                     occtl_graph_t**                               out_graph,
                                     occtl_node_id_t*                              out_root);

#define OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_VERSION_1 1u

/**
 * Configuration for #occtl_topo_make_face_extrusion.
 */
typedef struct occtl_topo_extrude_faces_options
{
  uint32_t    struct_version;        /**< Must be #OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_VERSION_1. */
  const void* p_next;                /**< Reserved; set to NULL. */
  const occtl_node_id_t* faces;      /**< Borrows it. Face NodeIds to extrude. */
  size_t                 face_count; /**< Number of entries in @c faces. */
  double                 thickness;  /**< Positive extrusion thickness. */
  int32_t                both_sides; /**< 0/1. Centre the thickness around each source face. */
  int32_t                use_normal; /**< 0/1. Use @c normal instead of each face normal. */
  occtl_direction3_t     normal;     /**< Override direction when @c use_normal != 0. */
  int32_t                copy; /**< 0/1. Copy source topology for the prism base. Default 1. */
  int32_t canonize;            /**< 0/1. Ask OCCT to canonicalize generated surfaces. Default 1. */
} occtl_topo_extrude_faces_options_t;

#define OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_INIT                                                      \
  {OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_VERSION_1, NULL, NULL, 0, 1.0, 0, 0, {0.0, 0.0, 1.0}, 1, 1}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_face_extrusion
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_extrude_faces_options_init(occtl_topo_extrude_faces_options_t* opts);

/**
 * Extrudes one or more Face nodes into prism solids.
 *
 * Output is a new graph; the input graph is not modified. Each Face is swept
 * with OCCT @c BRepPrimAPI_MakePrism. Without @c use_normal, the sweep
 * direction is the oriented face normal evaluated at the middle of the face
 * parameter bounds. Multiple faces are collected into a Compound result.
 *
 * This is a sweep operation, distinct from the hollow/shell operation
 * #occtl_prim_make_thick_solid.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  options    Borrows it. Must be non-NULL and carry a supported
 *                        @c struct_version.
 * @param[out] out_graph  Owns it. Receives a new graph on success.
 * @param[out] out_root   Borrows it. Receives the extrude result root NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any required pointer is NULL, @c face_count
 *                                 is zero, @c thickness is non-positive, or
 *                                 @c normal is invalid when requested.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         A selected face is invalid, removed, or
 *                                 cannot be resolved.
 * @retval OCCTL_WRONG_KIND        A selected NodeId is not a Face.
 * @retval OCCTL_GEOMETRY_INVALID  Extrusion failed or a face normal could
 *                                 not be evaluated.
 * @retval OCCTL_TOPOLOGY_INVALID  The result could not be ingested into BRepGraph.
 *
 * @threadsafe No. Allocates a new graph.
 *
 * @sa occtl_prim_make_thick_solid, occtl_topo_section_by_planes
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_face_extrusion(const occtl_graph_t*                      graph,
                                 const occtl_topo_extrude_faces_options_t* options,
                                 occtl_graph_t**                           out_graph,
                                 occtl_node_id_t*                          out_root);

/**
 * Creates a mirrored copy of the shape rooted at @p root across a plane
 * defined by @p point and @p normal.
 *
 * Output is a new graph; the input graph is not modified.
 *
 * @param[in]     graph      Borrows it.  Must be non-NULL.
 * @param[in]     root       NodeId of the shape to mirror.  Must be a valid
 *                           solid, shell, or compound node.
 * @param[in]     point      A point on the mirror plane.
 * @param[in]     normal     Normal direction of the mirror plane (must be
 *                           unit-length).
 * @param[out]    out_graph  Owns it.  Must be non-NULL.  Receives a
 *                           newly-allocated graph handle.
 * @param[out]    out_root   Borrows it.  Must be non-NULL.  Receives the
 *                           root NodeId of the mirrored result in
 *                           @p *out_graph.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_graph, or @p out_root
 *                                 is NULL.
 * @retval OCCTL_NOT_FOUND         @p root is invalid, removed, or cannot be
 *                                 resolved to an OCCT shape.
 * @retval OCCTL_GEOMETRY_INVALID  The result shape is null.
 *
 * @threadsafe No — allocates a new graph.
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_mirrored(const occtl_graph_t* graph,
                                                        occtl_node_id_t      root,
                                                        occtl_point3_t       point,
                                                        occtl_direction3_t   normal,
                                                        occtl_graph_t**      out_graph,
                                                        occtl_node_id_t*     out_root);

#define OCCTL_TOPO_LINEAR_PATTERN_OPTIONS_VERSION_1 1u

/** Configuration for #occtl_topo_make_linear_pattern. */
typedef struct occtl_topo_linear_pattern_options
{
  uint32_t        struct_version; /**< Must be #OCCTL_TOPO_LINEAR_PATTERN_OPTIONS_VERSION_1. */
  const void*     p_next;         /**< Reserved; set to NULL. */
  occtl_vector3_t direction;      /**< Translation direction per step.  Default (0,0,1). */
  int32_t         count;          /**< Number of instances (including original).  Default 2. */
  double          step;           /**< Translation distance per step.  Default 10.0. */
} occtl_topo_linear_pattern_options_t;

/** Default-value initialiser for #occtl_topo_linear_pattern_options_t. */
#define OCCTL_TOPO_LINEAR_PATTERN_OPTIONS_INIT                                                     \
  {OCCTL_TOPO_LINEAR_PATTERN_OPTIONS_VERSION_1, NULL, {0, 0, 1}, 2, 10.0}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_LINEAR_PATTERN_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_linear_pattern
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_linear_pattern_options_init(occtl_topo_linear_pattern_options_t* opts);

/**
 * Creates @c count instances of the shape rooted at @p root, translated
 * along @c direction by multiples of @c step.
 *
 * The original shape is included as the first instance.  Results are
 * placed into a newly-allocated compound in a new graph.
 *
 * @param[in]     graph      Borrows it.  Must be non-NULL.
 * @param[in]     root       NodeId of the shape to pattern.  Must be a
 *                           valid solid, shell, or compound node.
 * @param[in]     opts       Borrows it.  Must be non-NULL and carry a
 *                           supported struct_version.
 * @param[out]    out_graph  Borrows it.  Must be non-NULL.  Receives a
 *                           newly-allocated graph handle (owns it).
 * @param[out]    out_root   Borrows it.  Must be non-NULL.  Receives the
 *                           root NodeId of the resulting compound in
 *                           @p *out_graph.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p opts, @p out_graph, or
 *                                 @p out_root is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @p opts has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @p root is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID  The result compound is null.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_linear_pattern_options_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_linear_pattern(const occtl_graph_t*                       graph,
                                 occtl_node_id_t                            root,
                                 const occtl_topo_linear_pattern_options_t* opts,
                                 occtl_graph_t**                            out_graph,
                                 occtl_node_id_t*                           out_root);

#define OCCTL_TOPO_CIRCULAR_PATTERN_OPTIONS_VERSION_1 1u

/** Configuration for #occtl_topo_make_circular_pattern. */
typedef struct occtl_topo_circular_pattern_options
{
  uint32_t    struct_version;    /**< Must be #OCCTL_TOPO_CIRCULAR_PATTERN_OPTIONS_VERSION_1. */
  const void* p_next;            /**< Reserved; set to NULL. */
  occtl_axis1_placement_t axis;  /**< Rotation axis.  Default Z axis at origin. */
  int32_t                 count; /**< Number of instances (including original).  Default 4. */
  double angle; /**< Angular step in radians. Default #OCCTL_ANGLE_90_DEG_RAD (90 degrees). */
} occtl_topo_circular_pattern_options_t;

/** Default-value initialiser for #occtl_topo_circular_pattern_options_t. */
#define OCCTL_TOPO_CIRCULAR_PATTERN_OPTIONS_INIT                                                   \
  {OCCTL_TOPO_CIRCULAR_PATTERN_OPTIONS_VERSION_1,                                                  \
   NULL,                                                                                           \
   {{0, 0, 0}, {0, 0, 1}},                                                                         \
   4,                                                                                              \
   OCCTL_ANGLE_90_DEG_RAD}

/**
 * Initialises @p opts to default values matching
 * #OCCTL_TOPO_CIRCULAR_PATTERN_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] opts  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_circular_pattern
 */
OCCTL_API void OCCTL_CALL
  occtl_topo_circular_pattern_options_init(occtl_topo_circular_pattern_options_t* opts);

/**
 * Creates @c count instances of the shape rooted at @p root, rotated
 * around @c axis by multiples of @c angle.
 *
 * The original shape is included as the first instance.  Results are
 * placed into a newly-allocated compound in a new graph.
 *
 * @param[in]     graph      Borrows it.  Must be non-NULL.
 * @param[in]     root       NodeId of the shape to pattern.  Must be a
 *                           valid solid, shell, or compound node.
 * @param[in]     opts       Borrows it.  Must be non-NULL and carry a
 *                           supported struct_version.
 * @param[out]    out_graph  Borrows it.  Must be non-NULL.  Receives a
 *                           newly-allocated graph handle (owns it).
 * @param[out]    out_root   Borrows it.  Must be non-NULL.  Receives the
 *                           root NodeId of the resulting compound in
 *                           @p *out_graph.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p opts, @p out_graph, or
 *                                 @p out_root is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @p opts has an unsupported struct_version.
 * @retval OCCTL_NOT_FOUND         @p root is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID  The result compound is null.
 *
 * @threadsafe No — allocates a new graph.
 *
 * @sa occtl_topo_circular_pattern_options_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_circular_pattern(const occtl_graph_t*                         graph,
                                   occtl_node_id_t                              root,
                                   const occtl_topo_circular_pattern_options_t* opts,
                                   occtl_graph_t**                              out_graph,
                                   occtl_node_id_t*                             out_root);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_TOPO_ALGO_H */
