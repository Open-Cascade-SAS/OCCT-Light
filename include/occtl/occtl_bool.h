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
 * @file occtl_bool.h
 * @brief OCCT-Light: boolean operations module public API.
 *
 * Five operations:
 *   - Fuse    (union of two argument groups)
 *   - Cut     (subtraction of tools from objects)
 *   - Common  (intersection of two argument groups)
 *   - Section (intersection edges/vertices of all arguments)
 *   - Split   (splits objects using tools as cutters)
 *
 * Every entry point takes a single #occtl_graph_t* in which the inputs
 * already live, runs the operation, and **merges the result back into
 * that same graph** as a new topology root. The result root's NodeId is
 * returned in @c out_root. Input NodeIds remain valid and continue to
 * refer to the originating subgraph; the operation does not mutate them.
 *
 * When @c opts->build_history is non-zero, per-input Modified / Generated /
 * Deleted mappings are recorded on @p graph and queried with
 * #occtl_graph_history_modified, #occtl_graph_history_generated, and
 * #occtl_graph_history_deleted_all.
 */

#ifndef OCCTL_BOOL_H
#define OCCTL_BOOL_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Current options struct version. Bumped only on a binary-incompatible
 * change. New fields are appended via @c p_next-chained extension structs.
 */
#define OCCTL_BOOL_OPTIONS_VERSION_1 1u

/**
 * Tunable parameters shared by all five boolean operations.
 *
 * Defaults (see #OCCTL_BOOL_OPTIONS_INIT and #occtl_bool_options_init) are
 * the conservative ones: no fuzzy tolerance override, single-threaded
 * execution, no post-op simplification, history collection enabled.
 */
typedef struct occtl_bool_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_BOOL_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved for extensions; must be NULL. */
  double      fuzzy_value;  /**< Additional tolerance applied to all inputs; 0 keeps the default. */
  int32_t     run_parallel; /**< 0/1; non-zero enables parallel execution. */
  int32_t simplify_result; /**< 0/1; if non-zero, simplifies the result topology after the build. */
  double  simplify_angular_tolerance; /**< Angular tolerance for result simplification; used only
                                         when @c simplify_result != 0. */
  int32_t build_history;              /**< 0/1; if 0 disables history collection entirely. */
} occtl_bool_options_t;

/**
 * Static initialiser for #occtl_bool_options_t. Suitable for
 * @code occtl_bool_options_t opts = OCCTL_BOOL_OPTIONS_INIT; @endcode
 */
#define OCCTL_BOOL_OPTIONS_INIT {OCCTL_BOOL_OPTIONS_VERSION_1, NULL, 0.0, 0, 0, 1.0e-2, 1}

/**
 * Runtime initialiser for #occtl_bool_options_t.
 *
 * Sets all fields to #OCCTL_BOOL_OPTIONS_INIT.
 *
 * @param[out] options Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_bool_fuse
 */
OCCTL_API void OCCTL_CALL occtl_bool_options_init(occtl_bool_options_t* options);

/**
 * Boolean Fuse (union) of two argument groups.
 *
 * Both groups must hold entities of equal dimension. The result is added
 * to @p graph as a single new topology root.
 *
 * @param[in,out] graph       Borrows it. Must be non-NULL. Mutated: receives
 *                            the new topology merged in.
 * @param[in]     objects     Borrows it. Array of @p n_objects input NodeIds
 *                            that already live in @p graph. Must be non-NULL
 *                            when @p n_objects > 0.
 * @param[in]     n_objects   Number of object inputs. Must be >= 1.
 * @param[in]     tools       Borrows it. Array of @p n_tools input NodeIds.
 * @param[in]     n_tools     Number of tool inputs. Must be >= 1.
 * @param[in]     opts        Borrows it. Must be non-NULL with a recognised
 *                            @c struct_version.
 * @param[out]    out_root    Borrows it. Must be non-NULL. On success
 *                            receives the result topology root NodeId.
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  Required pointer is NULL; @p n_objects or
 *                                 @p n_tools is 0; options flags are not 0/1;
 *                                 @c opts->p_next is non-NULL; or numeric
 *                                 options are out of range / non-finite.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         An input NodeId is invalid or removed.
 * @retval OCCTL_GEOMETRY_INVALID  The boolean algorithm failed to produce
 *                                 a valid result (e.g. the input dimensions
 *                                 were incompatible).
 * @retval OCCTL_TOPOLOGY_INVALID  The result topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No — mutates @p graph.
 *
 * @sa occtl_bool_cut, occtl_bool_common, occtl_graph_history_modified
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_bool_fuse(occtl_graph_t*              graph,
                                                    const occtl_node_id_t*      objects,
                                                    size_t                      n_objects,
                                                    const occtl_node_id_t*      tools,
                                                    size_t                      n_tools,
                                                    const occtl_bool_options_t* opts,
                                                    occtl_node_id_t*            out_root);

/**
 * Boolean Cut (objects minus tools).
 *
 * The result has the dimension of the objects: tools must have dimension
 * not less than the maximum dimension of the objects.
 *
 * Parameters and return codes mirror #occtl_bool_fuse.
 *
 * @param[in,out] graph       See #occtl_bool_fuse.
 * @param[in]     objects     See #occtl_bool_fuse.
 * @param[in]     n_objects   See #occtl_bool_fuse.
 * @param[in]     tools       See #occtl_bool_fuse.
 * @param[in]     n_tools     See #occtl_bool_fuse.
 * @param[in]     opts        See #occtl_bool_fuse.
 * @param[out]    out_root    See #occtl_bool_fuse.
 *
 * @threadsafe No.
 *
 * @sa occtl_bool_fuse
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_bool_cut(occtl_graph_t*              graph,
                                                   const occtl_node_id_t*      objects,
                                                   size_t                      n_objects,
                                                   const occtl_node_id_t*      tools,
                                                   size_t                      n_tools,
                                                   const occtl_bool_options_t* opts,
                                                   occtl_node_id_t*            out_root);

/**
 * Boolean Common (intersection) of two argument groups.
 *
 * Arguments may have any dimension; the result has the minimum dimension
 * among the inputs.
 *
 * Parameters and return codes mirror #occtl_bool_fuse.
 *
 * @param[in,out] graph       See #occtl_bool_fuse.
 * @param[in]     objects     See #occtl_bool_fuse.
 * @param[in]     n_objects   See #occtl_bool_fuse.
 * @param[in]     tools       See #occtl_bool_fuse.
 * @param[in]     n_tools     See #occtl_bool_fuse.
 * @param[in]     opts        See #occtl_bool_fuse.
 * @param[out]    out_root    See #occtl_bool_fuse.
 *
 * @threadsafe No.
 *
 * @sa occtl_bool_fuse
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_bool_common(occtl_graph_t*              graph,
                                                      const occtl_node_id_t*      objects,
                                                      size_t                      n_objects,
                                                      const occtl_node_id_t*      tools,
                                                      size_t                      n_tools,
                                                      const occtl_bool_options_t* opts,
                                                      occtl_node_id_t*            out_root);

/**
 * Boolean Section: the intersection edges and vertices of all arguments.
 *
 * Arguments may be of any type. The result is typically a Compound of
 * edges (and possibly vertices) added to @p graph as a single root.
 *
 * Parameters and return codes mirror #occtl_bool_fuse.
 *
 * @param[in,out] graph       See #occtl_bool_fuse.
 * @param[in]     objects     See #occtl_bool_fuse.
 * @param[in]     n_objects   See #occtl_bool_fuse.
 * @param[in]     tools       See #occtl_bool_fuse.
 * @param[in]     n_tools     See #occtl_bool_fuse.
 * @param[in]     opts        See #occtl_bool_fuse.
 * @param[out]    out_root    See #occtl_bool_fuse.
 *
 * @threadsafe No.
 *
 * @sa occtl_bool_fuse
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_bool_section(occtl_graph_t*              graph,
                                                       const occtl_node_id_t*      objects,
                                                       size_t                      n_objects,
                                                       const occtl_node_id_t*      tools,
                                                       size_t                      n_tools,
                                                       const occtl_bool_options_t* opts,
                                                       occtl_node_id_t*            out_root);

/**
 * Boolean Split: split each object using the tools as cutters.
 *
 * Tools contribute nothing to the result body; they merely partition the
 * objects. The result is a Compound of the partitions, added as a single
 * new topology root.
 *
 * Parameters and return codes mirror #occtl_bool_fuse.
 *
 * @param[in,out] graph       See #occtl_bool_fuse.
 * @param[in]     objects     See #occtl_bool_fuse.
 * @param[in]     n_objects   See #occtl_bool_fuse.
 * @param[in]     tools       See #occtl_bool_fuse.
 * @param[in]     n_tools     See #occtl_bool_fuse.
 * @param[in]     opts        See #occtl_bool_fuse.
 * @param[out]    out_root    See #occtl_bool_fuse.
 *
 * @threadsafe No.
 *
 * @sa occtl_bool_fuse
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_bool_split(occtl_graph_t*              graph,
                                                     const occtl_node_id_t*      objects,
                                                     size_t                      n_objects,
                                                     const occtl_node_id_t*      tools,
                                                     size_t                      n_tools,
                                                     const occtl_bool_options_t* opts,
                                                     occtl_node_id_t*            out_root);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_BOOL_H */
