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
 * @file occtl_heal.h
 * @brief OCCT-Light: shape healing module.
 *
 * Exposes OCCT's ShapeFix, ShapeAnalysis, ShapeProcess, and ShapeUpgrade
 * toolkits for repairing and validating CAD geometry.
 */

#ifndef OCCTL_HEAL_H
#define OCCTL_HEAL_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Healing mode selector.
 *
 * Controls which family of OCCT repair tools is activated during
 * #occtl_heal_shape.  Each mode is cumulative — the higher modes
 * include every check from the lower ones.
 */
typedef enum occtl_heal_mode
{
  OCCTL_HEAL_MODE_BASIC = 0, /**< Fix basic issues: wire reorder, wire connected, wire-on-face. */
  OCCTL_HEAL_MODE_STANDARD = 1, /**< Standard fix: basic + face orientation, shell/solid closure. */
  OCCTL_HEAL_MODE_FULL     = 2, /**< Full fix: standard + SameParameter, seam, small-edge. */
  OCCTL_HEAL_MODE_RESERVED_FUTURE = 0x7fffffff
} occtl_heal_mode_t;

/**
 * Versioned options for shape healing.
 *
 * Call #occtl_heal_options_init for a runtime fill; use
 * #OCCTL_HEAL_OPTIONS_INIT for static zero-cost initialisation.  Pass
 * NULL as the @p options argument to #occtl_heal_shape to accept every
 * default.
 */
typedef struct occtl_heal_options
{
  uint32_t          struct_version; /**< Must be #OCCTL_HEAL_OPTIONS_VERSION_1. */
  const void*       p_next;         /**< Reserved; must be NULL. */
  occtl_heal_mode_t mode;           /**< Healing mode.  Default: #OCCTL_HEAL_MODE_STANDARD. */
  double  tolerance; /**< Working tolerance.  0.0 = use OCCT default (Precision::Confusion). */
  int32_t fix_same_parameter; /**< 0/1.  Default: 1.  Enforce SameParameter on edges. */
  int32_t fix_small_edges;    /**< 0/1.  Default: 1.  Fix small edges in wires. */
  int32_t fix_face_orient;    /**< 0/1.  Default: 1.  Fix face-wire orientation. */
  int32_t fix_missing_seam;   /**< 0/1.  Default: 0.  Fix missing seam edges on closed surfaces. */
} occtl_heal_options_t;

#define OCCTL_HEAL_OPTIONS_VERSION_1 1u

#define OCCTL_HEAL_OPTIONS_INIT                                                                    \
  {OCCTL_HEAL_OPTIONS_VERSION_1, NULL, OCCTL_HEAL_MODE_STANDARD, 0.0, 1, 1, 1, 0}

/**
 * Runtime initialiser for #occtl_heal_options_t.
 *
 * @param[out] options  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_heal_shape
 */
OCCTL_API void OCCTL_CALL occtl_heal_options_init(occtl_heal_options_t* options);

/**
 * Versioned options for unifying same-domain edges and faces.
 *
 * Call #occtl_heal_unify_same_domain_options_init for a runtime fill;
 * use #OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_INIT for static zero-cost
 * initialisation.  Pass NULL as the @p options argument to
 * #occtl_heal_unify_same_domain to accept every default.
 */
typedef struct occtl_heal_unify_same_domain_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; must be NULL. */
  int32_t     unify_edges;    /**< 0/1.  Default: 1.  Merge same-domain edges. */
  int32_t     unify_faces;    /**< 0/1.  Default: 1.  Merge same-domain faces. */
  int32_t     concat_bspline; /**< 0/1.  Default: 0.  Concatenate compatible B-spline geometry. */
  int32_t     allow_internal_edges; /**< 0/1.  Default: 0.  Allow faces with internal edges. */
  int32_t     safe_input;           /**< 0/1.  Default: 1.  Preserve input shape data. */
  double      linear_tolerance;     /**< Linear tolerance.  0.0 = use OCCT default. */
  double      angular_tolerance;    /**< Angular tolerance in radians.  0.0 = use OCCT default. */
} occtl_heal_unify_same_domain_options_t;

#define OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_VERSION_1 1u

#define OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_INIT                                                  \
  {OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_VERSION_1, NULL, 1, 1, 0, 0, 1, 0.0, 0.0}

/**
 * Runtime initialiser for #occtl_heal_unify_same_domain_options_t.
 *
 * @param[out] options  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_heal_unify_same_domain
 */
OCCTL_API void OCCTL_CALL
  occtl_heal_unify_same_domain_options_init(occtl_heal_unify_same_domain_options_t* options);

/**
 * Performs shape healing on a shape referenced by @p node_id.
 *
 * Wraps @c ShapeFix_Shape from OCCT's TKShHealing toolkit.  The healed
 * shape is ingested back into @p graph as a new topology root (the old
 * node is unaffected).  Caller must ensure exclusive access to the graph
 * during the call.
 *
 * @param[inout] graph    Borrows it.  Must be non-NULL.
 * @param[in]    node_id  Node whose OCCT shape will be healed.
 *                        The node must resolve to a valid, non-null shape.
 * @param[in]    options  Borrows it.  May be NULL for defaults
 *                        (#OCCTL_HEAL_OPTIONS_INIT).
 *
 * @retval OCCTL_OK               Healing completed (may produce warnings).
 * @retval OCCTL_INVALID_ARGUMENT @p graph is NULL; or @p options has invalid fields
 *                                (non-NULL @c p_next, non-finite/negative tolerance,
 *                                non-0/1 fix flags).
 * @retval OCCTL_NOT_FOUND        @p node_id does not resolve to a valid shape.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version in @p options.
 * @retval OCCTL_OUT_OF_RANGE     @p options->mode is outside supported enum values.
 * @retval OCCTL_TOPOLOGY_INVALID The healed result could not be added to the graph.
 *
 * @threadsafe No — mutates the graph.
 *
 * @sa occtl_heal_options_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_heal_shape(occtl_graph_t*              graph,
                                                     occtl_node_id_t             node_id,
                                                     const occtl_heal_options_t* options);

/**
 * Unifies same-domain edges and faces of a shape.
 *
 * Wraps OCCT's @c ShapeUpgrade_UnifySameDomain.  The unified shape is
 * ingested back into @p graph as a new topology root; the original node
 * is unaffected.
 *
 * @param[inout] graph    Borrows it.  Must be non-NULL.
 * @param[in]    node_id  Node whose OCCT shape will be unified.
 * @param[in]    options  Borrows it.  May be NULL for defaults
 *                        (#OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_INIT).
 * @param[out]   out_root Borrows it.  Receives the new topology root.  Must be non-NULL.
 *
 * @retval OCCTL_OK               Shape was unified and ingested.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_root is NULL; or @p options has invalid
 *                                fields (non-NULL @c p_next, non-0/1 flags, non-finite/negative
 *                                tolerances).
 * @retval OCCTL_NOT_FOUND        @p node_id does not resolve to a valid shape.
 * @retval OCCTL_VERSION_MISMATCH Unsupported @c struct_version in @p options.
 * @retval OCCTL_TOPOLOGY_INVALID The unified result could not be added to the graph.
 *
 * @threadsafe No — mutates the graph.
 *
 * @sa occtl_heal_unify_same_domain_options_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_heal_unify_same_domain(occtl_graph_t*                                graph,
                               occtl_node_id_t                               node_id,
                               const occtl_heal_unify_same_domain_options_t* options,
                               occtl_node_id_t*                              out_root);

#ifdef __cplusplus
}
#endif

#endif /* OCCTL_HEAL_H */
