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
 * @file occtl_topo.h
 * @brief OCCT-Light: topology module public API.
 *
 * Defines occtl_graph_t (the topology graph), session-local identity
 * types (occtl_node_id_t / occtl_ref_id_t / occtl_rep_id_t), persistent
 * identity (occtl_uid_t, declared in occtl_core.h, and
 * occtl_ref_uid_t), graph lifecycle,
 * count queries, identity conversion, geometry accessors, opaque
 * node-iteration, and high-level topology builders.
 */

#ifndef OCCTL_TOPO_H
#define OCCTL_TOPO_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_curves.h"
#include "occtl_curves2d.h"
#include "occtl_geom.h"
#include "occtl_surfaces.h"
#include "occtl_topo_build.h"
#include "occtl_topo_relation.h"
#include "occtl_topo_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Creates an empty topology graph.
 *
 * The returned graph is heap-allocated; the caller owns it and must
 * release it with #occtl_graph_free.  The graph starts with zero
 * entities.
 *
 * @param[out] out_graph  Owns it.  Must be non-NULL.  On success
 *                        receives a valid handle (never NULL); on
 *                        failure set to NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_graph is NULL.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_graph_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_create(occtl_graph_t** out_graph);

/**
 * Releases a graph and all associated resources.
 *
 * NULL-tolerant (free on NULL is a no-op).  After this returns, all
 * NodeIds, RefIds, and borrowed pointers obtained from this graph are
 * invalidated.
 *
 * @param[in] graph  Graph to free.  May be NULL.
 *
 * @threadsafe No — do not free a graph that another thread may be using.
 *
 * @sa occtl_graph_create
 */
OCCTL_API void OCCTL_CALL occtl_graph_free(occtl_graph_t* graph);

/**
 * Resolves a persistent UID to its current NodeId.
 *
 * UIDs survive node removal and any future compaction operation; call
 * this after any operation that may have reindexed nodes.  Returns
 * #OCCTL_NOT_FOUND if the entity has been removed.
 *
 * @param[in]  graph       Must be non-NULL.
 * @param[in]  uid         UID to resolve.  An all-zero (invalid) UID
 *                         always returns #OCCTL_NOT_FOUND.
 * @param[out] out_node_id Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_node_id is NULL.
 * @retval OCCTL_NOT_FOUND         The UID refers to a removed entity, or
 *                                 the UID is all-zero (invalid).
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_uid_from_node_id
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_id_from_uid(const occtl_graph_t* graph,
                                                                 occtl_uid_t          uid,
                                                                 occtl_node_id_t*     out_node_id);

/**
 * Returns the persistent UID for a NodeId.
 *
 * The UID survives node removal and any future compaction operation
 * and can be stored for later resolution.  If the node is later
 * removed the UID becomes permanently unresolvable.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  id      Node ID.  An all-zero (invalid) NodeId always
 *                     returns #OCCTL_NOT_FOUND.
 * @param[out] out_uid Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_uid is NULL.
 * @retval OCCTL_NOT_FOUND         The NodeId refers to a removed entity, or
 *                                 the NodeId is all-zero (invalid).
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_node_id_from_uid
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_uid_from_node_id(const occtl_graph_t* graph,
                                                                 occtl_node_id_t      id,
                                                                 occtl_uid_t*         out_uid);

/**
 * Look up Modified history images of an input UID recorded on @p graph.
 *
 * History is owned by the graph that received the operation result. The
 * function uses the two-call buffer pattern: pass @p out_buf NULL to query
 * the required count, then call again with enough capacity.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  input_uid  Input entity UID recorded by a shape-modifying operation.
 * @param[out] out_buf    Borrows it. May be NULL for sizing.
 * @param[in]  cap        Capacity of @p out_buf in entries.
 * @param[out] out_count  Borrows it. Must be non-NULL. Receives required count.
 *
 * @retval OCCTL_OK                On sizing or successful fill.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL, or @p input_uid is malformed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_history_generated, occtl_graph_history_deleted_all
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_history_modified(const occtl_graph_t* graph,
                                                                 occtl_uid_t          input_uid,
                                                                 occtl_uid_t*         out_buf,
                                                                 size_t               cap,
                                                                 size_t*              out_count);

/**
 * Look up Generated history images of an input UID recorded on @p graph.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  input_uid  Input entity UID recorded by a shape-modifying operation.
 * @param[out] out_buf    Borrows it. May be NULL for sizing.
 * @param[in]  cap        Capacity of @p out_buf in entries.
 * @param[out] out_count  Borrows it. Must be non-NULL. Receives required count.
 *
 * @retval OCCTL_OK                On sizing or successful fill.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL, or @p input_uid is malformed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_history_modified, occtl_graph_history_deleted_all
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_history_generated(const occtl_graph_t* graph,
                                                                  occtl_uid_t          input_uid,
                                                                  occtl_uid_t*         out_buf,
                                                                  size_t               cap,
                                                                  size_t*              out_count);

/**
 * Dump all deleted input UIDs recorded on @p graph.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[out] out_buf    Borrows it. May be NULL for sizing.
 * @param[in]  cap        Capacity of @p out_buf in entries.
 * @param[out] out_count  Borrows it. Must be non-NULL. Receives required count.
 *
 * @retval OCCTL_OK                On sizing or successful fill.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_buf is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_history_modified, occtl_graph_history_generated
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_history_deleted_all(const occtl_graph_t* graph,
                                                                    occtl_uid_t*         out_buf,
                                                                    size_t               cap,
                                                                    size_t*              out_count);

/**
 * Resolves a persistent RefUID to its current RefId.
 *
 * RefUIDs survive graph compaction; call this after operations that may have
 * reindexed reference entries.  Returns #OCCTL_NOT_FOUND if the reference has
 * been removed or if @p ref_uid is invalid.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  ref_uid    RefUID to resolve.
 * @param[out] out_ref_id Borrows it (caller-allocated slot). Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_ref_id is NULL.
 * @retval OCCTL_NOT_FOUND         @p ref_uid is invalid or not active.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_ref_uid_from_ref_id, occtl_graph_ref_uid_table
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_ref_id_from_ref_uid(const occtl_graph_t* graph,
                                                                    occtl_ref_uid_t      ref_uid,
                                                                    occtl_ref_id_t* out_ref_id);

/**
 * Returns the persistent RefUID for a RefId.
 *
 * The RefUID survives compaction and can be stored for later resolution.  If
 * the reference is later removed the RefUID becomes unresolvable.
 *
 * @param[in]  graph       Borrows it. Must be non-NULL.
 * @param[in]  ref_id      RefId to inspect.
 * @param[out] out_ref_uid Borrows it (caller-allocated slot). Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_ref_uid is NULL.
 * @retval OCCTL_NOT_FOUND         @p ref_id is invalid or removed.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_ref_id_from_ref_uid, occtl_graph_ref_uid_table
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_ref_uid_from_ref_id(const occtl_graph_t* graph,
                                                                    occtl_ref_id_t       ref_id,
                                                                    occtl_ref_uid_t* out_ref_uid);

/**
 * Resolves a persistent RepUID to its current RepId.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  rep_uid    RepUID to resolve.
 * @param[out] out_rep_id Borrows it (caller-allocated slot). Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_rep_id is NULL.
 * @retval OCCTL_NOT_FOUND         @p rep_uid is invalid or not active.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_rep_uid_from_rep_id
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_rep_id_from_rep_uid(const occtl_graph_t* graph,
                                                                    occtl_rep_uid_t      rep_uid,
                                                                    occtl_rep_id_t* out_rep_id);

/**
 * Returns the persistent RepUID for a RepId.
 *
 * @param[in]  graph       Borrows it. Must be non-NULL.
 * @param[in]  rep_id      RepId to inspect.
 * @param[out] out_rep_uid Borrows it (caller-allocated slot). Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_rep_uid is NULL.
 * @retval OCCTL_NOT_FOUND         @p rep_id is invalid or removed.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_rep_id_from_rep_uid
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_rep_uid_from_rep_id(const occtl_graph_t* graph,
                                                                    occtl_rep_id_t       rep_id,
                                                                    occtl_rep_uid_t* out_rep_uid);

/**
 * Encodes a RefUID into its fixed-width wire format.
 *
 * @param[in]  ref_uid   RefUID to encode (may be #OCCTL_REF_UID_INVALID).
 * @param[out] out_bytes Owns it (caller-allocated). Must point to at least
 *                       #OCCTL_REF_UID_WIRE_SIZE writable bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_bytes is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_ref_uid_from_bytes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_ref_uid_to_bytes(occtl_ref_uid_t ref_uid,
                                                           uint8_t*        out_bytes);

/**
 * Decodes a fixed-width wire-format RefUID.
 *
 * Rejects payloads whose reserved bytes are non-zero with
 * #OCCTL_FORMAT_ERROR; those bytes are reserved for a future wider identity
 * encoding.
 *
 * @param[in]  in_bytes    Borrows it. Must point to at least
 *                         #OCCTL_REF_UID_WIRE_SIZE readable bytes.
 * @param[out] out_ref_uid Borrows it (caller-allocated slot). Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p in_bytes or @p out_ref_uid is NULL.
 * @retval OCCTL_FORMAT_ERROR      Reserved bytes are non-zero.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_ref_uid_to_bytes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_ref_uid_from_bytes(const uint8_t*   in_bytes,
                                                             occtl_ref_uid_t* out_ref_uid);

/**
 * Encodes a RepUID into its fixed-width wire format.
 *
 * @param[in]  rep_uid   RepUID to encode (may be #OCCTL_REP_UID_INVALID).
 * @param[out] out_bytes Owns it (caller-allocated). Must point to at least
 *                       #OCCTL_REP_UID_WIRE_SIZE writable bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_bytes is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_rep_uid_from_bytes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_rep_uid_to_bytes(occtl_rep_uid_t rep_uid,
                                                           uint8_t*        out_bytes);

/**
 * Decodes a fixed-width wire-format RepUID.
 *
 * @param[in]  in_bytes    Borrows it. Must point to at least
 *                         #OCCTL_REP_UID_WIRE_SIZE readable bytes.
 * @param[out] out_rep_uid Borrows it (caller-allocated slot). Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p in_bytes or @p out_rep_uid is NULL.
 * @retval OCCTL_FORMAT_ERROR      Reserved bytes are non-zero.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_rep_uid_to_bytes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_rep_uid_from_bytes(const uint8_t*   in_bytes,
                                                             occtl_rep_uid_t* out_rep_uid);

/**
 * Dumps the (UID → NodeId) mapping for every active node in @p graph.
 *
 * Two-call buffer (§10.1): pass @p out_uids and @p out_nodes as NULL,
 * any @p cap, to size the dump; reissue with both arrays of length at
 * least @p out_count (the value the sizing call returned).  The
 * resulting parallel arrays form the wire-format-stable handshake that
 * survives a Compact / save / load cycle.
 *
 * Iteration order is implementation-defined but stable for a given
 * graph state.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[out] out_uids  Borrows it.  Length @p cap; may be NULL on the
 *                       sizing call.
 * @param[out] out_nodes Borrows it.  Length @p cap; may be NULL on the
 *                       sizing call.  Must be the same length as
 *                       @p out_uids on the refill call.
 * @param[in]  cap       Capacity of both arrays.  Ignored when both
 *                       arrays are NULL (sizing call).
 * @param[out] out_count Borrows it.  Receives the total number of
 *                       (UID, NodeId) pairs in the graph.
 *
 * @retval OCCTL_OK                Success — either sizing or full fill.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p cap < @p out_count and at least one
 *                                 of the arrays is non-NULL.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_uid_to_bytes, occtl_graph_uid_from_node_id
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_uid_table(const occtl_graph_t* graph,
                                                          occtl_uid_t*         out_uids,
                                                          occtl_node_id_t*     out_nodes,
                                                          size_t               cap,
                                                          size_t*              out_count);

/**
 * Dumps the (RefUID -> RefId) mapping for every active reference in @p graph.
 *
 * Two-call buffer pattern: pass @p out_ref_uids and @p out_refs as NULL to
 * query @p out_count, then call again with both arrays of at least that size.
 * Returned arrays are parallel: entry @c i is (@c out_ref_uids[i],
 * @c out_refs[i]).
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[out] out_ref_uids Owns it (caller-allocated). Length @p cap; may be
 *                          NULL only on the sizing call.
 * @param[out] out_refs     Owns it (caller-allocated). Length @p cap; may be
 *                          NULL only on the sizing call.
 * @param[in]  cap          Capacity of both output arrays.
 * @param[out] out_count    Borrows it. Must be non-NULL. Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL, or only
 *                                 one output array is NULL on a refill call.
 * @retval OCCTL_BUFFER_TOO_SMALL  Output arrays are non-NULL and @p cap is
 *                                 smaller than @p out_count.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_ref_uid_from_ref_id, occtl_graph_ref_id_from_ref_uid
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_ref_uid_table(const occtl_graph_t* graph,
                                                              occtl_ref_uid_t*     out_ref_uids,
                                                              occtl_ref_id_t*      out_refs,
                                                              size_t               cap,
                                                              size_t*              out_count);

/**
 * Returns the kind of a node ID.
 *
 * No bit manipulation is exposed to the caller; treat #occtl_node_id_t
 * as opaque and query its kind through this function.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  id        Node ID to inspect.
 * @param[out] out_kind  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_kind is NULL.
 * @retval OCCTL_NOT_FOUND         The node has been removed, or @p id is
 *                                 all-zero (invalid).
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_uid_kind, occtl_graph_ref_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_kind(const occtl_graph_t* graph,
                                                          occtl_node_id_t      id,
                                                          occtl_node_kind_t*   out_kind);

/**
 * Returns the kind embedded in a UID.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  uid       UID to inspect.
 * @param[out] out_kind  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_kind is NULL.
 * @retval OCCTL_NOT_FOUND         The UID is all-zero (invalid) or the
 *                                 entity has been removed.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_node_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_uid_kind(const occtl_graph_t* graph,
                                                         occtl_uid_t          uid,
                                                         occtl_node_kind_t*   out_kind);

/**
 * Returns the kind of a ref ID.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  id        Ref ID to inspect.
 * @param[out] out_kind  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_kind is NULL.
 * @retval OCCTL_NOT_FOUND         The reference has been removed, or @p id
 *                                 is all-zero (invalid).
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_ref_kind(const occtl_graph_t* graph,
                                                         occtl_ref_id_t       id,
                                                         occtl_ref_kind_t*    out_kind);

/**
 * Returns the kind embedded in a RefUID.
 *
 * @param[in]  graph     Borrows it. Must be non-NULL.
 * @param[in]  ref_uid   RefUID to inspect.
 * @param[out] out_kind  Borrows it (caller-allocated slot). Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_kind is NULL.
 * @retval OCCTL_NOT_FOUND         @p ref_uid is invalid or not active.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_ref_kind
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_ref_uid_kind(const occtl_graph_t* graph,
                                                             occtl_ref_uid_t      ref_uid,
                                                             occtl_ref_kind_t*    out_kind);

/**
 * Returns the kind of a rep ID.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  id        Rep ID to inspect.
 * @param[out] out_kind  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_kind is NULL.
 * @retval OCCTL_NOT_FOUND         The representation has been removed, or
 *                                 @p id is all-zero (invalid).
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_rep_kind(const occtl_graph_t* graph,
                                                         occtl_rep_id_t       id,
                                                         occtl_rep_kind_t*    out_kind);

/**
 * Returns the number of active (non-removed) solids in the graph.
 *
 * @param[in]  graph      Graph pointer.  Must be non-NULL.
 * @param[out] out_count  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 *
 * @threadsafe Yes (read-only).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_solid_count(const occtl_graph_t* graph,
                                                            size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_shell_count(const occtl_graph_t* graph,
                                                            size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_face_count(const occtl_graph_t* graph,
                                                           size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_wire_count(const occtl_graph_t* graph,
                                                           size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_edge_count(const occtl_graph_t* graph,
                                                           size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_vertex_count(const occtl_graph_t* graph,
                                                             size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_compound_count(const occtl_graph_t* graph,
                                                               size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_compsolid_count(const occtl_graph_t* graph,
                                                                size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_coedge_count(const occtl_graph_t* graph,
                                                             size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_product_count(const occtl_graph_t* graph,
                                                              size_t*              out_count);

/** @copydoc occtl_graph_solid_count */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_occurrence_count(const occtl_graph_t* graph,
                                                                 size_t*              out_count);

/**
 * Returns the total number of active nodes across all kinds.
 *
 * @param[in]  graph      Graph pointer.  Must be non-NULL.
 * @param[out] out_count  Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 *
 * @threadsafe Yes (read-only).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_count(const occtl_graph_t* graph,
                                                           size_t*              out_count);

/**
 * Returns the 3D point of a vertex.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  vertex    Vertex node ID.  Must be a valid, active vertex.
 * @param[out] out_point Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_point is NULL.
 * @retval OCCTL_NOT_FOUND         @p vertex is invalid, removed, or not a vertex.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_vertex_tolerance
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_point(const occtl_graph_t* graph,
                                                            occtl_node_id_t      vertex,
                                                            occtl_point3_t*      out_point);

/**
 * Returns the tolerance of a vertex.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  vertex        Vertex node ID.
 * @param[out] out_tolerance Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_tolerance is NULL.
 * @retval OCCTL_NOT_FOUND         @p vertex is invalid, removed, or not a vertex.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_vertex_point
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_tolerance(const occtl_graph_t* graph,
                                                                occtl_node_id_t      vertex,
                                                                double*              out_tolerance);

/**
 * Returns the number of edges that reference a vertex.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  vertex    Vertex node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p vertex is invalid, removed, or not a vertex.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_edge_count(const occtl_graph_t* graph,
                                                                 occtl_node_id_t      vertex,
                                                                 uint32_t*            out_count);

/**
 * Returns the parametric range of an edge's 3D curve.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  edge      Edge node ID.
 * @param[out] out_first Borrows it.  Must be non-NULL.
 * @param[out] out_last  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_first, or @p out_last is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_tolerance
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_range(const occtl_graph_t* graph,
                                                          occtl_node_id_t      edge,
                                                          double*              out_first,
                                                          double*              out_last);

/**
 * Returns the tolerance of an edge.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  edge          Edge node ID.
 * @param[out] out_tolerance Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_tolerance is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_tolerance(const occtl_graph_t* graph,
                                                              occtl_node_id_t      edge,
                                                              double*              out_tolerance);

/**
 * Returns whether an edge is degenerated.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  edge     Edge node ID.
 * @param[out] out_is_degenerated Borrows it.  Set to 1 if degenerated, 0 otherwise.
 *                                Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_is_degenerated is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_is_degenerated(const occtl_graph_t* graph,
                                                                   occtl_node_id_t      edge,
                                                                   int32_t* out_is_degenerated);

/**
 * Returns whether an edge has a 3D curve.
 *
 * @param[in]  graph  Must be non-NULL.
 * @param[in]  edge   Edge node ID.
 * @param[out] out_has_curve Borrows it.  Set to 1 if the edge has a curve, 0 otherwise.
 *                           Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_curve is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_has_curve(const occtl_graph_t* graph,
                                                              occtl_node_id_t      edge,
                                                              int32_t*             out_has_curve);

/**
 * Returns the OCCT curve kind carried by an edge's 3D curve.
 *
 * Edges without a 3D curve return #OCCTL_CURVE_KIND_UNDEFINED.
 *
 * @param[in]  graph    Borrows it.  Must be non-NULL.
 * @param[in]  edge     Edge node ID.
 * @param[out] out_kind Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_kind is NULL.
 * @retval OCCTL_NOT_FOUND        @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND       @p edge is not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_has_curve, occtl_select_iter_create
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_curve_kind(const occtl_graph_t* graph,
                                                               occtl_node_id_t      edge,
                                                               occtl_curve_kind_t*  out_kind);

/**
 * Returns the start vertex of an edge.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  edge       Edge node ID.
 * @param[out] out_vertex Borrows it.  Receives the start vertex NodeId.
 *                        Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_vertex is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_end_vertex
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_start_vertex(const occtl_graph_t* graph,
                                                                 occtl_node_id_t      edge,
                                                                 occtl_node_id_t*     out_vertex);

/**
 * Returns the end vertex of an edge.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  edge       Edge node ID.
 * @param[out] out_vertex Borrows it.  Receives the end vertex NodeId.
 *                        Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_vertex is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_start_vertex
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_end_vertex(const occtl_graph_t* graph,
                                                               occtl_node_id_t      edge,
                                                               occtl_node_id_t*     out_vertex);

/**
 * Returns whether a coedge is a seam (closed-surface) edge.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  coedge   CoEdge node ID.
 * @param[out] out_is_seam Borrows it.  Set to 1 if seam, 0 otherwise.
 *                         Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_is_seam is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_is_seam(const occtl_graph_t* graph,
                                                              occtl_node_id_t      coedge,
                                                              int32_t*             out_is_seam);

/**
 * Returns the parent edge of a coedge.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  coedge   CoEdge node ID.
 * @param[out] out_edge Borrows it.  Receives the parent edge NodeId.
 *                      Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_edge is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedge_face_of
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_edge_of(const occtl_graph_t* graph,
                                                              occtl_node_id_t      coedge,
                                                              occtl_node_id_t*     out_edge);

/**
 * Returns the parent face of a coedge.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  coedge   CoEdge node ID.
 * @param[out] out_face Borrows it.  Receives the parent face NodeId.
 *                      Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_face is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedge_edge_of
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_face_of(const occtl_graph_t* graph,
                                                              occtl_node_id_t      coedge,
                                                              occtl_node_id_t*     out_face);

/**
 * Returns the tolerance of a face.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  face          Face node ID.
 * @param[out] out_tolerance Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_tolerance is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_tolerance(const occtl_graph_t* graph,
                                                              occtl_node_id_t      face,
                                                              double*              out_tolerance);

/**
 * Returns the number of wires on a face.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  face      Face node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_wire_count(const occtl_graph_t* graph,
                                                               occtl_node_id_t      face,
                                                               uint32_t*            out_count);

/**
 * Returns the outer wire of a face.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  face     Face node ID.
 * @param[out] out_wire Borrows it.  Receives the outer wire NodeId.
 *                      Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_wire is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face,
 *                                 or the face has no outer wire.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_outer_wire(const occtl_graph_t* graph,
                                                               occtl_node_id_t      face,
                                                               occtl_node_id_t*     out_wire);

/**
 * Returns the UV parameter bounds of a face.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  face      Face node ID.
 * @param[out] out_umin  Borrows it.  Must be non-NULL.
 * @param[out] out_umax  Borrows it.  Must be non-NULL.
 * @param[out] out_vmin  Borrows it.  Must be non-NULL.
 * @param[out] out_vmax  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or any out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_uv_bounds(const occtl_graph_t* graph,
                                                              occtl_node_id_t      face,
                                                              double*              out_umin,
                                                              double*              out_umax,
                                                              double*              out_vmin,
                                                              double*              out_vmax);

/**
 * Evaluates the 3D point on an edge at parameter @p u.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  edge    Edge node ID.
 * @param[in]  u       Curve parameter.
 * @param[out] out_p   Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_p is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_eval_dn, occtl_topo_edge_eval_d2
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval(const occtl_graph_t* graph,
                                                         occtl_node_id_t      edge,
                                                         double               u,
                                                         occtl_point3_t*      out_p);

/**
 * Evaluates the 3D point and first derivative on an edge at parameter @p u.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  edge      Edge node ID.
 * @param[in]  u         Curve parameter.
 * @param[out] out_p     Borrows it.  Must be non-NULL.
 * @param[out] out_d1    Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_p, or @p out_d1 is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_eval_dn, occtl_topo_edge_eval_d2
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval_d1(const occtl_graph_t* graph,
                                                            occtl_node_id_t      edge,
                                                            double               u,
                                                            occtl_point3_t*      out_p,
                                                            occtl_vector3_t*     out_d1);

/**
 * Evaluates the 3D point, first derivative, and second derivative
 * on an edge at parameter @p u.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  edge    Edge node ID.
 * @param[in]  u       Curve parameter.
 * @param[out] out_p   Borrows it.  Must be non-NULL.
 * @param[out] out_d1  Borrows it.  Must be non-NULL.
 * @param[out] out_d2  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_eval_d3, occtl_topo_edge_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval_d2(const occtl_graph_t* graph,
                                                            occtl_node_id_t      edge,
                                                            double               u,
                                                            occtl_point3_t*      out_p,
                                                            occtl_vector3_t*     out_d1,
                                                            occtl_vector3_t*     out_d2);

/**
 * Evaluates the 3D point, first derivative, second derivative, and
 * third derivative on an edge at parameter @p u.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  edge    Edge node ID.
 * @param[in]  u       Curve parameter.
 * @param[out] out_p   Borrows it.  Must be non-NULL.
 * @param[out] out_d1  Borrows it.  Must be non-NULL.
 * @param[out] out_d2  Borrows it.  Must be non-NULL.
 * @param[out] out_d3  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_eval_d2, occtl_topo_edge_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval_d3(const occtl_graph_t* graph,
                                                            occtl_node_id_t      edge,
                                                            double               u,
                                                            occtl_point3_t*      out_p,
                                                            occtl_vector3_t*     out_d1,
                                                            occtl_vector3_t*     out_d2,
                                                            occtl_vector3_t*     out_d3);

/**
 * Evaluates the Nth derivative vector on an edge at parameter @p u.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  edge    Edge node ID.
 * @param[in]  u       Curve parameter.
 * @param[in]  n       Derivative order (0 = point position as vector).
 * @param[out] out_dn  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_dn is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_eval_d2, occtl_topo_edge_eval_d3
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval_dn(const occtl_graph_t* graph,
                                                            occtl_node_id_t      edge,
                                                            double               u,
                                                            uint32_t             n,
                                                            occtl_vector3_t*     out_dn);

/**
 * Evaluates the PCurve UV point on a coedge at parameter @p u.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  coedge  CoEdge node ID.
 * @param[in]  u       PCurve parameter.
 * @param[out] out_uv  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_uv is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 * @retval OCCTL_WRONG_KIND        @p coedge is not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedge_pcurve_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_pcurve_eval(const occtl_graph_t* graph,
                                                                  occtl_node_id_t      coedge,
                                                                  double               u,
                                                                  occtl_point2_t*      out_uv);

/**
 * Evaluates the PCurve UV point and first derivative on a coedge
 * at parameter @p u.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  coedge  CoEdge node ID.
 * @param[in]  u       PCurve parameter.
 * @param[out] out_uv  Borrows it.  Must be non-NULL.
 * @param[out] out_d1  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p coedge is not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedge_pcurve_eval_d2, occtl_topo_coedge_pcurve_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_pcurve_eval_d1(const occtl_graph_t* graph,
                                                                     occtl_node_id_t      coedge,
                                                                     double               u,
                                                                     occtl_point2_t*      out_uv,
                                                                     occtl_vector2_t*     out_d1);

/**
 * Evaluates the PCurve UV point and first two derivatives on a coedge
 * at parameter @p u.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  coedge  CoEdge node ID.
 * @param[in]  u       PCurve parameter.
 * @param[out] out_uv  Borrows it.  Must be non-NULL.
 * @param[out] out_d1  Borrows it.  Must be non-NULL.
 * @param[out] out_d2  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p coedge is not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedge_pcurve_eval_d1, occtl_topo_coedge_pcurve_eval_d3
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_pcurve_eval_d2(const occtl_graph_t* graph,
                                                                     occtl_node_id_t      coedge,
                                                                     double               u,
                                                                     occtl_point2_t*      out_uv,
                                                                     occtl_vector2_t*     out_d1,
                                                                     occtl_vector2_t*     out_d2);

/**
 * Evaluates the PCurve UV point and first three derivatives on a coedge
 * at parameter @p u.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  coedge  CoEdge node ID.
 * @param[in]  u       PCurve parameter.
 * @param[out] out_uv  Borrows it.  Must be non-NULL.
 * @param[out] out_d1  Borrows it.  Must be non-NULL.
 * @param[out] out_d2  Borrows it.  Must be non-NULL.
 * @param[out] out_d3  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p coedge is not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedge_pcurve_eval_d2, occtl_topo_coedge_pcurve_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_pcurve_eval_d3(const occtl_graph_t* graph,
                                                                     occtl_node_id_t      coedge,
                                                                     double               u,
                                                                     occtl_point2_t*      out_uv,
                                                                     occtl_vector2_t*     out_d1,
                                                                     occtl_vector2_t*     out_d2,
                                                                     occtl_vector2_t*     out_d3);

/**
 * Evaluates the Nth derivative vector on a coedge pcurve at parameter @p u.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  coedge  CoEdge node ID.
 * @param[in]  u       PCurve parameter.
 * @param[in]  n       Derivative order (0 = point position as vector).
 * @param[out] out_dn  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_dn is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p coedge is not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedge_pcurve_eval_d3
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_pcurve_eval_dn(const occtl_graph_t* graph,
                                                                     occtl_node_id_t      coedge,
                                                                     double               u,
                                                                     uint32_t             n,
                                                                     occtl_vector2_t*     out_dn);

/**
 * Evaluates the 3D point on a face at UV parameters (@p u, @p v).
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  face    Face node ID.
 * @param[in]  u       U parameter.
 * @param[in]  v       V parameter.
 * @param[out] out_p   Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_p is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 * @retval OCCTL_WRONG_KIND        @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_eval_dn, occtl_topo_face_eval_d1
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval(const occtl_graph_t* graph,
                                                         occtl_node_id_t      face,
                                                         double               u,
                                                         double               v,
                                                         occtl_point3_t*      out_p);

/**
 * Evaluates the 3D point and first partial derivatives (D1U, D1V)
 * on a face at UV parameters (@p u, @p v).
 *
 * The surface normal can be computed from the derivatives as
 * @c cross(out_d1u, out_d1v).normalized().
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  face      Face node ID.
 * @param[in]  u         U parameter.
 * @param[in]  v         V parameter.
 * @param[out] out_p     Borrows it.  Must be non-NULL.
 * @param[out] out_d1u   Borrows it.  Must be non-NULL.
 * @param[out] out_d1v   Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_eval, occtl_topo_face_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval_d1(const occtl_graph_t* graph,
                                                            occtl_node_id_t      face,
                                                            double               u,
                                                            double               v,
                                                            occtl_point3_t*      out_p,
                                                            occtl_vector3_t*     out_d1u,
                                                            occtl_vector3_t*     out_d1v);

/**
 * Evaluates the 3D point and first and second partial derivatives
 * on a face at UV parameters (@p u, @p v).
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  face     Face node ID.
 * @param[in]  u        U parameter.
 * @param[in]  v        V parameter.
 * @param[out] out_p    Borrows it.  Must be non-NULL.
 * @param[out] out_d1u  Borrows it.  Must be non-NULL.
 * @param[out] out_d1v  Borrows it.  Must be non-NULL.
 * @param[out] out_d2u  Borrows it.  Must be non-NULL.
 * @param[out] out_d2v  Borrows it.  Must be non-NULL.
 * @param[out] out_d2uv Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_eval_d1, occtl_topo_face_eval_d3
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval_d2(const occtl_graph_t* graph,
                                                            occtl_node_id_t      face,
                                                            double               u,
                                                            double               v,
                                                            occtl_point3_t*      out_p,
                                                            occtl_vector3_t*     out_d1u,
                                                            occtl_vector3_t*     out_d1v,
                                                            occtl_vector3_t*     out_d2u,
                                                            occtl_vector3_t*     out_d2v,
                                                            occtl_vector3_t*     out_d2uv);

/**
 * Evaluates the 3D point and first, second, and third partial
 * derivatives on a face at UV parameters (@p u, @p v).
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  face      Face node ID.
 * @param[in]  u         U parameter.
 * @param[in]  v         V parameter.
 * @param[out] out_p     Borrows it.  Must be non-NULL.
 * @param[out] out_d1u   Borrows it.  Must be non-NULL.
 * @param[out] out_d1v   Borrows it.  Must be non-NULL.
 * @param[out] out_d2u   Borrows it.  Must be non-NULL.
 * @param[out] out_d2v   Borrows it.  Must be non-NULL.
 * @param[out] out_d2uv  Borrows it.  Must be non-NULL.
 * @param[out] out_d3u   Borrows it.  Must be non-NULL.
 * @param[out] out_d3v   Borrows it.  Must be non-NULL.
 * @param[out] out_d3uuv Borrows it.  Must be non-NULL.
 * @param[out] out_d3uvv Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL or an out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_eval_d2, occtl_topo_face_eval_dn
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval_d3(const occtl_graph_t* graph,
                                                            occtl_node_id_t      face,
                                                            double               u,
                                                            double               v,
                                                            occtl_point3_t*      out_p,
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
 * Evaluates the (Nu,Nv)th cross derivative vector on a face at
 * UV parameters (@p u, @p v).
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  face    Face node ID.
 * @param[in]  u       U parameter.
 * @param[in]  v       V parameter.
 * @param[in]  nu      U derivative order.
 * @param[in]  nv      V derivative order.
 * @param[out] out_dn  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_dn is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_eval, occtl_topo_face_eval_d1
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval_dn(const occtl_graph_t* graph,
                                                            occtl_node_id_t      face,
                                                            double               u,
                                                            double               v,
                                                            uint32_t             nu,
                                                            uint32_t             nv,
                                                            occtl_vector3_t*     out_dn);

/**
 * Returns whether a face has a surface.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  face    Face node ID.
 * @param[out] out_has_surface Borrows it.  Set to 1 if the face has a surface, 0 otherwise.
 *                             Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_surface is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_has_surface(const occtl_graph_t* graph,
                                                                occtl_node_id_t      face,
                                                                int32_t* out_has_surface);

/**
 * Returns the OCCT surface kind carried by a face.
 *
 * Faces without a surface return #OCCTL_SURFACE_KIND_UNDEFINED.
 *
 * @param[in]  graph    Borrows it.  Must be non-NULL.
 * @param[in]  face     Face node ID.
 * @param[out] out_kind Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK               On success.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p out_kind is NULL.
 * @retval OCCTL_NOT_FOUND        @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND       @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_has_surface, occtl_select_iter_create
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_surface_kind(const occtl_graph_t*  graph,
                                                                 occtl_node_id_t       face,
                                                                 occtl_surface_kind_t* out_kind);

/**
 * Returns whether a wire is topologically closed.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  wire     Wire node ID.
 * @param[out] out_is_closed Borrows it.  Set to 1 if closed, 0 otherwise.
 *                           Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_is_closed is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid, removed, or not a wire.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_is_closed(const occtl_graph_t* graph,
                                                              occtl_node_id_t      wire,
                                                              int32_t*             out_is_closed);

/**
 * Returns the number of coedges in a wire.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  wire      Wire node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid, removed, or not a wire.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_coedge_count(const occtl_graph_t* graph,
                                                                 occtl_node_id_t      wire,
                                                                 uint32_t*            out_count);

/**
 * Returns whether a shell is topologically closed (watertight).
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  shell    Shell node ID.
 * @param[out] out_is_closed Borrows it.  Set to 1 if closed, 0 otherwise.
 *                           Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_is_closed is NULL.
 * @retval OCCTL_NOT_FOUND         @p shell is invalid, removed, or not a shell.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_shell_is_closed(const occtl_graph_t* graph,
                                                               occtl_node_id_t      shell,
                                                               int32_t*             out_is_closed);

/**
 * Returns the number of faces in a shell.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  shell     Shell node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p shell is invalid, removed, or not a shell.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_shell_face_count(const occtl_graph_t* graph,
                                                                occtl_node_id_t      shell,
                                                                uint32_t*            out_count);

/**
 * Parameter of a vertex on a given edge.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  vertex        Vertex node ID.
 * @param[in]  edge          Edge node ID.
 * @param[out] out_parameter Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_parameter is NULL.
 * @retval OCCTL_NOT_FOUND         @p vertex or @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p vertex is not a vertex or @p edge is not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_vertex_parameters
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_parameter(const occtl_graph_t* graph,
                                                                occtl_node_id_t      vertex,
                                                                occtl_node_id_t      edge,
                                                                double*              out_parameter);

/**
 * UV parameters of a vertex on a given face.
 *
 * @param[in]  graph  Must be non-NULL.
 * @param[in]  vertex Vertex node ID.
 * @param[in]  face   Face node ID.
 * @param[out] out_uv Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_uv is NULL.
 * @retval OCCTL_NOT_FOUND         @p vertex or @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p vertex is not a vertex or @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_vertex_parameter
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_parameters(const occtl_graph_t* graph,
                                                                 occtl_node_id_t      vertex,
                                                                 occtl_node_id_t      face,
                                                                 occtl_point2_t*      out_uv);

/**
 * Returns whether an edge has the same parameterisation on every face it bounds.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  edge     Edge node ID.
 * @param[out] out_has_same_parameter Borrows it.  Set to 1 if same-parameter, 0 otherwise.  Must be
 * non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_same_parameter is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_same_parameter(const occtl_graph_t* graph,
                                                                   occtl_node_id_t      edge,
                                                                   int32_t* out_has_same_parameter);

/**
 * Returns whether an edge has the same range in 3D and on its pcurves.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  edge     Edge node ID.
 * @param[out] out_has_same_range Borrows it.  Set to 1 if same-range, 0 otherwise.  Must be
 * non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_same_range is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_same_range(const occtl_graph_t* graph,
                                                               occtl_node_id_t      edge,
                                                               int32_t* out_has_same_range);

/**
 * Returns whether an edge is manifold (shared by at most 2 faces).
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  edge     Edge node ID.
 * @param[out] out_is_manifold Borrows it.  Set to 1 if manifold, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_is_manifold is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_is_boundary, occtl_topo_edge_is_seam_on_face
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_is_manifold(const occtl_graph_t* graph,
                                                                occtl_node_id_t      edge,
                                                                int32_t* out_is_manifold);

/**
 * Returns whether an edge is a boundary edge (belongs to exactly 1 face).
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  edge     Edge node ID.
 * @param[out] out_is_boundary Borrows it.  Set to 1 if boundary, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_is_boundary is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_is_manifold, occtl_topo_edge_is_seam_on_face
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_is_boundary(const occtl_graph_t* graph,
                                                                occtl_node_id_t      edge,
                                                                int32_t* out_is_boundary);

/**
 * Returns whether an edge is a seam edge on a given face.
 *
 * A seam edge bounds the periodic seam of the face — e.g. the join on a
 * cylindrical or spherical face.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  edge     Edge node ID.
 * @param[in]  face     Face node ID.
 * @param[out] out_is_seam Borrows it.  Set to 1 if the edge is a seam on the face, 0 otherwise.
 * Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_is_seam is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge or @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge or @p face is not a face.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_is_seam_on_face(const occtl_graph_t* graph,
                                                                    occtl_node_id_t      edge,
                                                                    occtl_node_id_t      face,
                                                                    int32_t* out_is_seam);

/**
 * Returns the number of faces referencing an edge.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  edge      Edge node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_face_count(const occtl_graph_t* graph,
                                                               occtl_node_id_t      edge,
                                                               uint32_t*            out_count);

/**
 * Returns whether a coedge is reversed (senses its parent edge in the opposite direction).
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  coedge   CoEdge node ID.
 * @param[out] out_is_reversed Borrows it.  Set to 1 if reversed, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_is_reversed is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_is_reversed(const occtl_graph_t* graph,
                                                                  occtl_node_id_t      coedge,
                                                                  int32_t* out_is_reversed);

/**
 * Returns whether a coedge has a pcurve.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  coedge   CoEdge node ID.
 * @param[out] out_has_pcurve Borrows it.  Set to 1 if pcurve present, 0 otherwise.  Must be
 * non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_pcurve is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_has_pcurve(const occtl_graph_t* graph,
                                                                 occtl_node_id_t      coedge,
                                                                 int32_t* out_has_pcurve);

/**
 * Parameter of @p vertex on the pcurve carried by @p coedge.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  coedge        CoEdge node ID.
 * @param[in]  vertex        Vertex node ID.
 * @param[out] out_parameter Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_parameter is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge or @p vertex is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p coedge is not a coedge or @p vertex is not a vertex.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_pcurve_parameter(const occtl_graph_t* graph,
                                                                       occtl_node_id_t      coedge,
                                                                       occtl_node_id_t      vertex,
                                                                       double* out_parameter);

/**
 * Returns the parametric range of a coedge.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  coedge    CoEdge node ID.
 * @param[out] out_first Borrows it.  Must be non-NULL.
 * @param[out] out_last  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or any out-param is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_range(const occtl_graph_t* graph,
                                                            occtl_node_id_t      coedge,
                                                            double*              out_first,
                                                            double*              out_last);

/**
 * Returns the UV points at the start and end of a coedge on its parent face surface.
 *
 * @param[in]  graph         Must be non-NULL.
 * @param[in]  coedge        CoEdge node ID.
 * @param[out] out_uv_start  Borrows it.  May be NULL.
 * @param[out] out_uv_end    Borrows it.  May be NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or any color channel in
 *                                 @p color is non-finite.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_uv_points(const occtl_graph_t* graph,
                                                                occtl_node_id_t      coedge,
                                                                occtl_point2_t*      out_uv_start,
                                                                occtl_point2_t*      out_uv_end);

/**
 * Returns the paired coedge for a seam edge.
 *
 * For non-seam coedges sets @p out_pair to #OCCTL_NODE_ID_INVALID and returns
 * #OCCTL_OK.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  coedge   CoEdge node ID.
 * @param[out] out_pair Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success (including non-seam case).
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_pair is NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid, removed, or not a coedge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_seam_pair(const occtl_graph_t* graph,
                                                                occtl_node_id_t      coedge,
                                                                occtl_node_id_t*     out_pair);

/**
 * Returns whether a face has natural restriction (inherent bounds from its surface).
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  face     Face node ID.
 * @param[out] out_has_natural_restriction Borrows it.  Set to 1 if natural restriction, 0
 * otherwise. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_natural_restriction is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_has_triangulation
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_natural_restriction(const occtl_graph_t* graph,
                                      occtl_node_id_t      face,
                                      int32_t*             out_has_natural_restriction);

/**
 * Returns whether a face has a triangulation.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  face     Face node ID.
 * @param[out] out_has_triangulation Borrows it.  Set to 1 if triangulation present, 0 otherwise.
 *                                   Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_triangulation is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid, removed, or not a face.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_natural_restriction
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_has_triangulation(const occtl_graph_t* graph,
                                    occtl_node_id_t      face,
                                    int32_t*             out_has_triangulation);

/**
 * Returns the number of distinct edges in a wire.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  wire      Wire node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid, removed, or not a wire.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_distinct_edge_count(const occtl_graph_t* graph,
                                                                        occtl_node_id_t      wire,
                                                                        uint32_t* out_count);

/**
 * Returns the face a wire belongs to.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  wire     Wire node ID.
 * @param[out] out_face Borrows it.  Receives the parent face NodeId.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_face is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid, removed, or not a wire,
 *                                 or the wire is free (not owned by any face).
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_wire_is_outer
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_face_of(const occtl_graph_t* graph,
                                                            occtl_node_id_t      wire,
                                                            occtl_node_id_t*     out_face);

/**
 * Returns whether a wire is the outer wire of its parent face.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  wire     Wire node ID.
 * @param[out] out_is_outer Borrows it.  Set to 1 if outer, 0 otherwise.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_is_outer is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid, removed, or not a wire.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_wire_face_of
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_is_outer(const occtl_graph_t* graph,
                                                             occtl_node_id_t      wire,
                                                             int32_t*             out_is_outer);

/**
 * Returns the number of shells in a solid.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  solid     Solid node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p solid is invalid, removed, or not a solid.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_solid_shell_count(const occtl_graph_t* graph,
                                                                 occtl_node_id_t      solid,
                                                                 uint32_t*            out_count);

/**
 * Returns the number of edges in a wire.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  wire      Wire node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid, removed, or not a wire.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_edge_count(const occtl_graph_t* graph,
                                                               occtl_node_id_t      wire,
                                                               uint32_t*            out_count);

/**
 * Returns the number of vertices on an edge.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  edge      Edge node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid, removed, or not an edge.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_vertex_count(const occtl_graph_t* graph,
                                                                 occtl_node_id_t      edge,
                                                                 uint32_t*            out_count);

/**
 * Returns the number of children (direct references) in a compound.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  compound  Compound node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p compound is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p compound is not a compound.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_make_compound
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_compound_child_count(const occtl_graph_t* graph,
                                                                    occtl_node_id_t      compound,
                                                                    uint32_t*            out_count);

/**
 * Returns the number of solids in a compsolid.
 *
 * @param[in]  graph      Must be non-NULL.
 * @param[in]  compsolid  CompSolid node ID.
 * @param[out] out_count  Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p compsolid is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p compsolid is not a compsolid.
 *
 * @threadsafe Yes (read-only on graph).
 *
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_compsolid_solid_count(const occtl_graph_t* graph,
                                                                     occtl_node_id_t      compsolid,
                                                                     uint32_t* out_count);

/**
 * Returns the number of occurrences of a product.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  product   Product node ID.
 * @param[out] out_count Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p product is invalid, removed, or not a product.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_product_occurrence_count(const occtl_graph_t* graph,
                                                                        occtl_node_id_t product,
                                                                        uint32_t*       out_count);

/**
 * Opaque iterator over a sequence of node IDs.
 *
 * Holds a snapshot view rooted in a graph; created by one of the
 * enumeration factories below.  Yielded IDs are session-local
 * #occtl_node_id_t values whose kind is determined by the factory used
 * to create the iterator (e.g. #occtl_graph_face_iter_create yields
 * #OCCTL_KIND_FACE).  Removed nodes are filtered internally and never
 * exposed.  Iteration order is documented per factory.
 *
 * Lifetime: borrows from the source graph.  Valid until either
 * #occtl_node_iter_free is called on the iterator or the source graph
 * is freed.  Adding nodes, calling a future compact, or freeing the
 * graph while an iterator is live is undefined behaviour.  Removing a
 * node that has not yet been visited is well-defined (the iterator
 * filters it out); removing the node currently positioned at is undefined.
 * Release iterators before any mutating call to keep the contract
 * simple.
 *
 * Not thread-safe with respect to its own state.  Distinct iterators
 * over the same graph may be advanced from distinct threads if the
 * graph is not concurrently mutated (matching the read-only
 * concurrency contract on graph reads).
 */
typedef struct occtl_node_iter occtl_node_iter_t;

/**
 * Advances the iterator and returns the next node ID.
 *
 * On a successful step writes the next ID to @p out_id and returns
 * OCCTL_OK.  When the iterator is exhausted writes
 * OCCTL_NODE_ID_INVALID to @p out_id and returns OCCTL_NOT_FOUND;
 * subsequent calls remain OCCTL_NOT_FOUND (idempotent end).
 *
 * @param[in,out] iter  Borrows it.  Must be non-NULL.  Advanced in place.
 * @param[out]    out_id Borrows it (caller-allocated slot).  Must be non-NULL.
 *
 * @retval OCCTL_OK                On a successful step; @p out_id receives the ID.
 * @retval OCCTL_NOT_FOUND         Iterator is exhausted; @p out_id set to invalid.
 * @retval OCCTL_INVALID_ARGUMENT  @p iter or @p out_id is NULL.
 *
 * @threadsafe No (mutates iterator state).
 *
 * @sa occtl_node_iter_free
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_node_iter_next(occtl_node_iter_t* iter,
                                                         occtl_node_id_t*   out_id);

/**
 * Releases an iterator.  NULL-tolerant; idempotent.
 *
 * After this call @p iter must not be passed to #occtl_node_iter_next.
 *
 * @param[in] iter  Iterator to release.  May be NULL.
 *
 * @threadsafe No.
 *
 * @sa occtl_node_iter_next
 */
OCCTL_API void OCCTL_CALL occtl_node_iter_free(occtl_node_iter_t* iter);

/**
 * Creates an iterator over all active solids.
 *
 * Iterator borrows from @p graph; release with #occtl_node_iter_free
 * before mutating or freeing the graph.  Iteration order is
 * implementation-defined but stable for a given graph state.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[out] out_iter  Owns it.  Must be non-NULL.  On success receives
 *                       a new iterator; on failure set to NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_solid_iter_create(const occtl_graph_t* graph,
                                                                  occtl_node_iter_t**  out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_shell_iter_create(const occtl_graph_t* graph,
                                                                  occtl_node_iter_t**  out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_face_iter_create(const occtl_graph_t* graph,
                                                                 occtl_node_iter_t**  out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_wire_iter_create(const occtl_graph_t* graph,
                                                                 occtl_node_iter_t**  out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_edge_iter_create(const occtl_graph_t* graph,
                                                                 occtl_node_iter_t**  out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_vertex_iter_create(const occtl_graph_t* graph,
                                                                   occtl_node_iter_t**  out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_compound_iter_create(const occtl_graph_t* graph,
                                                                     occtl_node_iter_t**  out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_compsolid_iter_create(const occtl_graph_t* graph,
                                                                      occtl_node_iter_t** out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_coedge_iter_create(const occtl_graph_t* graph,
                                                                   occtl_node_iter_t**  out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_product_iter_create(const occtl_graph_t* graph,
                                                                    occtl_node_iter_t**  out_iter);

/** @copydoc occtl_graph_solid_iter_create */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_occurrence_iter_create(const occtl_graph_t* graph, occtl_node_iter_t** out_iter);

/**
 * Creates an iterator over all root products (products not referenced
 * by any active occurrence).
 *
 * @copydoc occtl_graph_solid_iter_create
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_root_product_iter_create(const occtl_graph_t* graph, occtl_node_iter_t** out_iter);

/**
 * Creates an iterator over the shells of a solid.
 *
 * Yielded IDs are #OCCTL_KIND_SHELL.  Iteration order is
 * implementation-defined.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  solid     Solid node ID.
 * @param[out] out_iter  Owns it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_NOT_FOUND         @p solid is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p solid is not a solid.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_shells_of_solid_iter_create(const occtl_graph_t* graph,
                                         occtl_node_id_t      solid,
                                         occtl_node_iter_t**  out_iter);

/**
 * Creates an iterator over the faces of a shell.
 *
 * Yielded IDs are #OCCTL_KIND_FACE. Iteration order is implementation-defined.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  shell     Shell node ID.
 * @param[out] out_iter  Owns it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_NOT_FOUND         @p shell is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p shell is not a shell.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_faces_of_shell_iter_create(const occtl_graph_t* graph,
                                        occtl_node_id_t      shell,
                                        occtl_node_iter_t**  out_iter);

/**
 * Creates an iterator over the wires of a face.
 *
 * Yielded IDs are #OCCTL_KIND_WIRE. Iteration order is implementation-defined.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  face      Face node ID.
 * @param[out] out_iter  Owns it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a face.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wires_of_face_iter_create(const occtl_graph_t* graph,
                                       occtl_node_id_t      face,
                                       occtl_node_iter_t**  out_iter);

/**
 * Creates an iterator over the coedges of a wire.
 *
 * Yielded IDs are #OCCTL_KIND_COEDGE. Iteration order is implementation-defined.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  wire      Wire node ID.
 * @param[out] out_iter  Owns it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p wire is not a wire.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedges_of_wire_iter_create(const occtl_graph_t* graph,
                                         occtl_node_id_t      wire,
                                         occtl_node_iter_t**  out_iter);

/**
 * Creates an iterator over the distinct edges referenced by a wire.
 *
 * Yielded IDs are #OCCTL_KIND_EDGE. Iteration order is implementation-defined.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  wire      Wire node ID.
 * @param[out] out_iter  Owns it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p wire is not a wire.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edges_of_wire_iter_create(const occtl_graph_t* graph,
                                       occtl_node_id_t      wire,
                                       occtl_node_iter_t**  out_iter);

/**
 * Creates an iterator over the coedges of a wire in geometric
 * traversal order.
 *
 * The wire explorer follows coedges in endpoint-chaining order,
 * unlike #occtl_topo_coedges_of_wire_iter_create which visits
 * coedge definitions in stored order.  The traversal auto-chains
 * open endpoints and wraps around a closed wire.
 *
 * Yielded IDs are #OCCTL_KIND_COEDGE.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  wire     Wire node ID.
 * @param[out] out_iter Owns it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p wire is not a wire.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_coedges_of_wire_iter_create
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_explorer_create(const occtl_graph_t* graph,
                                                                    occtl_node_id_t      wire,
                                                                    occtl_node_iter_t**  out_iter);

/**
 * Creates an iterator over the occurrences of a product.
 *
 * Yielded IDs are #OCCTL_KIND_OCCURRENCE. Iteration order is implementation-defined.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  product   Product node ID.
 * @param[out] out_iter  Owns it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_NOT_FOUND         @p product is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p product is not a product.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_occurrences_of_product_iter_create(const occtl_graph_t* graph,
                                                occtl_node_id_t      product,
                                                occtl_node_iter_t**  out_iter);

/**
 * Creates an iterator over the vertices of an edge.
 *
 * Yielded order is: start vertex, end vertex, then internal vertex IDs
 * in their stored order.  This pinned order lets callers distinguish
 * roles by position in the yield sequence.
 *
 * @param[in]  graph     Must be non-NULL.
 * @param[in]  edge      Edge node ID.
 * @param[out] out_iter  Owns it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_iter is NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe Yes (read-only on graph).
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_vertices_of_edge_iter_create(const occtl_graph_t* graph,
                                          occtl_node_id_t      edge,
                                          occtl_node_iter_t**  out_iter);

/** @brief Callback for #occtl_graph_for_each. */
typedef occtl_status_t(OCCTL_CALL* occtl_node_visitor_t)(occtl_node_id_t node, void* user_data);

/**
 * Iterates over all nodes in @p graph whose kind is in @p kind_mask using
 * internal per-kind iterators. Calls @p visitor for each matching node.
 *
 * @param[in]     graph      Borrows it. Must be non-NULL.
 * @param[in]     kind_mask  Bitwise OR of #occtl_node_kind_t values.
 * @param[in]     visitor    Callback invoked for each matching node.
 * @param[in,out] user_data  Passed through to @p visitor. May be NULL.
 * @retval OCCTL_OK               On completion.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p visitor is NULL.
 * @threadsafe Yes (read-only).
 * @sa occtl_node_iter_create
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_for_each(const occtl_graph_t* graph,
                                                         uint64_t             kind_mask,
                                                         occtl_node_visitor_t visitor,
                                                         void*                user_data);

/** @brief Callback for #occtl_graph_for_each_ref. */
typedef occtl_status_t(OCCTL_CALL* occtl_ref_visitor_t)(occtl_ref_id_t ref, void* user_data);

/** @brief Callback for #occtl_graph_for_each_rep. */
typedef occtl_status_t(OCCTL_CALL* occtl_rep_visitor_t)(occtl_rep_id_t rep, void* user_data);

/**
 * Iterates over all references in @p graph whose kind is in @p ref_kind_mask.
 * Calls @p visitor for each matching reference.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  ref_kind_mask Bitwise OR of #occtl_ref_kind_t values.
 * @param[in]  visitor      Callback invoked for each matching ref.
 * @param[in,out] user_data Passed through. May be NULL.
 *
 * @retval OCCTL_OK               On completion.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p visitor is NULL.
 *
 * @threadsafe Yes (read-only).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_for_each_ref(const occtl_graph_t* graph,
                                                             uint64_t             ref_kind_mask,
                                                             occtl_ref_visitor_t  visitor,
                                                             void*                user_data);

/**
 * Iterates over all representations in @p graph whose kind is in @p rep_kind_mask.
 * Calls @p visitor for each matching representation.
 *
 * @param[in]  graph        Borrows it. Must be non-NULL.
 * @param[in]  rep_kind_mask Bitwise OR of #occtl_rep_kind_t values.
 * @param[in]  visitor      Callback invoked for each matching rep.
 * @param[in,out] user_data Passed through. May be NULL.
 *
 * @retval OCCTL_OK               On completion.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p visitor is NULL.
 *
 * @threadsafe Yes (read-only).
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_for_each_rep(const occtl_graph_t* graph,
                                                             uint64_t             rep_kind_mask,
                                                             occtl_rep_visitor_t  visitor,
                                                             void*                user_data);

/**
 * Iterates over nodes related to @p node and calls @p visitor for each.
 *
 * @param[in]     graph      Borrows it. Must be non-NULL.
 * @param[in]     node       Root node.
 * @param[in]     visitor    Callback invoked for each related node.
 * @param[in,out] user_data  Passed through. May be NULL.
 * @retval OCCTL_OK               On completion.
 * @retval OCCTL_INVALID_ARGUMENT @p graph or @p visitor is NULL.
 * @retval OCCTL_NOT_FOUND        @p node is invalid or removed.
 * @threadsafe Yes (read-only, allocates internal iterator).
 * @sa occtl_topo_related_iter_create
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_for_each_related(const occtl_graph_t* graph,
                                                                occtl_node_id_t      node,
                                                                occtl_node_visitor_t visitor,
                                                                void*                user_data);

/**
 * Sets a colour on a target node.
 *
 * The colour is stored as an RGBA value and associated with @p target
 * in an internal graph-wide colour metadata.
 *
 * @param[in,out] graph   Borrows it.  Must be non-NULL.
 * @param[in]     target  Node ID to colour.  Must be valid and active.
 * @param[in]     color   RGBA colour value (by value).
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or any color channel in
 *                                 @p color is non-finite.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_graph_color_get, occtl_graph_color_unset
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_color_set(occtl_graph_t*     graph,
                                                          occtl_node_id_t    target,
                                                          occtl_color_rgba_t color);

/**
 * Retrieves the colour of a target node.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  target    Node ID to query.  Must be valid and active.
 * @param[out] out_color  Borrows it.  Must be non-NULL.  Receives the
 *                          stored colour, or opaque white if no colour
 *                          was previously set.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_color is NULL.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_color_set, occtl_graph_color_unset
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_color_get(const occtl_graph_t* graph,
                                                          occtl_node_id_t      target,
                                                          occtl_color_rgba_t*  out_color);

/**
 * Lists nodes that have an explicit colour entry.
 *
 * Two-call buffer pattern: pass @p out_nodes and @p out_colors as NULL with
 * @p cap 0 to learn the entry count, then call again with both arrays of at
 * least that many elements.  Values are snapshots copied from the graph-owned
 * colour metadata.
 *
 * @param[in]  graph      Borrows it.  Must be non-NULL.
 * @param[out] out_nodes  Borrows it.  Length @p cap; may be NULL only on the
 *                        sizing call.
 * @param[out] out_colors Borrows it.  Length @p cap; may be NULL only on the
 *                        sizing call.
 * @param[in]  cap        Capacity of @p out_nodes and @p out_colors.
 * @param[out] out_count  Borrows it.  Must be non-NULL.  Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL, or only
 *                                 one output array is NULL on a refill call.
 * @retval OCCTL_BUFFER_TOO_SMALL  Output arrays are non-NULL and @p cap is
 *                                 smaller than @p out_count.
 *
 * @threadsafe Yes (read-only on graph metadata).
 *
 * @sa occtl_graph_color_get, occtl_graph_name_nodes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_color_entries(const occtl_graph_t* graph,
                                                              occtl_node_id_t*     out_nodes,
                                                              occtl_color_rgba_t*  out_colors,
                                                              size_t               cap,
                                                              size_t*              out_count);

/**
 * Removes the colour associated with a target node.
 *
 * After this call, #occtl_graph_color_get returns the default colour
 * (opaque white).  Idempotent — calling on a node with no colour set
 * is a successful no-op.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     target Node ID to clear.  Must be valid and active.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_graph_color_set
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_color_unset(occtl_graph_t*  graph,
                                                            occtl_node_id_t target);

/**
 * Sets a human-readable name on a target node.
 *
 * @p name is a byte buffer of length @p nameLen; it need not be
 * NUL-terminated.  The internal copy is NUL-terminated.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     target   Node ID to name.  Must be valid and active.
 * @param[in]     name     Byte buffer containing the name.  Borrowed;
 *                            copied internally.  May be NULL only when
 *                            @p nameLen is 0.
 * @param[in]     nameLen  Length of @p name in bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p name is NULL
 *                                 when @p nameLen > 0.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_graph_name_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_name_set(occtl_graph_t*  graph,
                                                         occtl_node_id_t target,
                                                         const char*     name,
                                                         size_t          nameLen);

/**
 * Retrieves the name of a target node (two-call buffer pattern).
 *
 * Call once with @p buf == NULL to learn the required buffer size in
 * @p out_required, then call again with a buffer of at least that
 * size.  The written string is NUL-terminated; @p out_required
 * includes the NUL terminator.
 *
 * If a node has no name set, the written string is empty (just the NUL
 * terminator) and *@p out_required is 1.
 *
 * @param[in]  graph       Borrows it.  Must be non-NULL.
 * @param[in]  target      Node ID to query.  Must be valid and active.
 * @param[out] buf         Owns it (caller-allocated).  May be NULL
 *                            to query required size.
 * @param[in]  bufSize     Size of @p buf in bytes.
 * @param[out] out_required Borrows it.  Must be non-NULL.  Receives
 *                            the required buffer size (including NUL).
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_required is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p buf is non-NULL and @p bufSize
 *                                 is too small; @p out_required receives
 *                                 the needed size.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_name_set
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_name_get(const occtl_graph_t* graph,
                                                         occtl_node_id_t      target,
                                                         char*                buf,
                                                         size_t               bufSize,
                                                         size_t*              out_required);

/**
 * Lists nodes that have an explicit name entry.
 *
 * Two-call buffer pattern: pass @p out_nodes as NULL with @p cap 0 to learn
 * the count, then call again with an array of at least that many elements.
 * Retrieve each name with #occtl_graph_name_get.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[out] out_nodes Borrows it.  Length @p cap; may be NULL to query count.
 * @param[in]  cap       Capacity of @p out_nodes in elements.
 * @param[out] out_count Borrows it.  Must be non-NULL.  Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_nodes is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph metadata).
 *
 * @sa occtl_graph_name_get, occtl_graph_color_entries
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_name_nodes(const occtl_graph_t* graph,
                                                           occtl_node_id_t*     out_nodes,
                                                           size_t               cap,
                                                           size_t*              out_count);

/**
 * Sets material-lite data on a target node.
 *
 * The material record is copied into graph-owned metadata.  The @c name
 * field in @p info is a borrowed byte span and does not need to be
 * NUL-terminated.  Density is optional; when present it must be finite and
 * strictly positive.  Diffuse colour is optional and uses the same channel
 * convention as #occtl_color_rgba_t.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     target Node ID to annotate.  Must be valid and active.
 * @param[in]     info   Borrows it.  Must be non-NULL with a recognised
 *                       @c struct_version and NULL @c p_next.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p info is NULL, or an info
 *                                 field is malformed.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version is unsupported.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe No (mutates graph metadata).
 *
 * @sa occtl_graph_material_get, occtl_graph_material_unset
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_material_set(occtl_graph_t*               graph,
                                                             occtl_node_id_t              target,
                                                             const occtl_material_info_t* info);

/**
 * Retrieves material-lite data from a target node (two-call buffer pattern).
 *
 * Call once with @p name_buf == NULL to learn the required material-name
 * buffer size in @p out_name_required, then call again with a buffer of at
 * least that size.  @p out_info is always populated on success; when
 * @p name_buf is NULL its @c name pointer is NULL and @c name_len is the
 * stored name byte length.  When @p name_buf is supplied, @c out_info->name
 * points at @p name_buf.  The written name is NUL-terminated and
 * @p out_name_required includes that terminator.
 *
 * @param[in]  graph             Borrows it.  Must be non-NULL.
 * @param[in]  target            Node ID to query.  Must be valid and active.
 * @param[out] out_info          Borrows it.  Must be non-NULL.
 * @param[out] name_buf          Owns it (caller-allocated).  May be NULL to
 *                               query required size.
 * @param[in]  name_buf_size     Size of @p name_buf in bytes.
 * @param[out] out_name_required Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_info, or
 *                                 @p out_name_required is NULL.
 * @retval OCCTL_NOT_FOUND         @p target is invalid / removed, or has no
 *                                 material set.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p name_buf is non-NULL and
 *                                 @p name_buf_size is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_material_set, occtl_graph_material_unset
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_material_get(const occtl_graph_t*   graph,
                                                             occtl_node_id_t        target,
                                                             occtl_material_info_t* out_info,
                                                             char*                  name_buf,
                                                             size_t                 name_buf_size,
                                                             size_t* out_name_required);

/**
 * Lists nodes that have explicit material-lite data.
 *
 * Two-call buffer pattern: pass @p out_nodes as NULL with @p cap 0 to learn
 * the count, then call again with an array of at least that many elements.
 * Retrieve each material record with #occtl_graph_material_get.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[out] out_nodes Borrows it.  Length @p cap; may be NULL to query count.
 * @param[in]  cap       Capacity of @p out_nodes in elements.
 * @param[out] out_count Borrows it.  Must be non-NULL.  Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_nodes is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph metadata).
 *
 * @sa occtl_graph_material_get, occtl_graph_node_metadata_nodes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_material_nodes(const occtl_graph_t* graph,
                                                               occtl_node_id_t*     out_nodes,
                                                               size_t               cap,
                                                               size_t*              out_count);

/**
 * Removes material-lite data from a target node.
 *
 * Idempotent: removing a missing material is a successful no-op.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     target Node ID to modify.  Must be valid and active.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe No (mutates graph metadata).
 *
 * @sa occtl_graph_material_set, occtl_graph_material_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_material_unset(occtl_graph_t*  graph,
                                                               occtl_node_id_t target);

/**
 * Sets graph-level length-unit metadata.
 *
 * @p name is a byte buffer of length @p nameLen; it need not be
 * NUL-terminated.  The internal copy is NUL-terminated.  A zero-length
 * name is allowed and means "unnamed unit"; an unset graph reports
 * a default unit of @c 1.0 meter named @c m.
 *
 * @param[in,out] graph                Borrows it.  Must be non-NULL.
 * @param[in]     length_unit_to_meter Scale factor from one model length
 *                                     unit to meters; must be finite and
 *                                     strictly positive.
 * @param[in]     name                 Unit name bytes.  Borrowed; copied
 *                                     internally.  May be NULL only when
 *                                     @p nameLen is 0.
 * @param[in]     nameLen              Length of @p name in bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL; @p length_unit_to_meter
 *                                 is non-finite / non-positive; or @p name
 *                                 is NULL when @p nameLen > 0.
 *
 * @threadsafe No (mutates graph metadata).
 *
 * @sa occtl_graph_units_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_units_set(occtl_graph_t* graph,
                                                          double         length_unit_to_meter,
                                                          const char*    name,
                                                          size_t         nameLen);

/**
 * Retrieves graph-level length-unit metadata (two-call buffer pattern).
 *
 * Call once with @p name_buf == NULL to learn the required buffer size in
 * @p out_name_required, then call again with a buffer of at least that size.
 * The written unit name is NUL-terminated; @p out_name_required includes the
 * NUL terminator.
 *
 * @param[in]  graph                    Borrows it.  Must be non-NULL.
 * @param[out] out_length_unit_to_meter Borrows it.  Must be non-NULL.
 *                                      Receives the scale factor from one
 *                                      model length unit to meters.
 * @param[out] name_buf                 Owns it (caller-allocated).  May be
 *                                      NULL to query required size.
 * @param[in]  name_buf_size            Size of @p name_buf in bytes.
 * @param[out] out_name_required        Borrows it.  Must be non-NULL.
 *                                      Receives required size including NUL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p out_length_unit_to_meter, or
 *                                 @p out_name_required is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p name_buf is non-NULL and
 *                                 @p name_buf_size is too small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_units_set
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_units_get(const occtl_graph_t* graph,
                                                          double* out_length_unit_to_meter,
                                                          char*   name_buf,
                                                          size_t  name_buf_size,
                                                          size_t* out_name_required);

/**
 * Sets UTF-8 metadata on a target node.
 *
 * @p key and @p value are byte buffers; neither needs to be NUL-terminated.
 * A key must be non-empty.  A zero-length value is allowed and stores an
 * empty string.  Metadata is stored as graph-owned metadata and follows
 * graph compact / clone remapping.
 *
 * @param[in,out] graph    Borrows it.  Must be non-NULL.
 * @param[in]     target   Node ID to annotate.  Must be valid and active.
 * @param[in]     key      Metadata key bytes.  Borrowed; copied internally.
 *                         Must be non-NULL and non-empty.
 * @param[in]     keyLen   Length of @p key in bytes.
 * @param[in]     value    Metadata value bytes.  Borrowed; copied internally.
 *                         May be NULL only when @p valueLen is 0.
 * @param[in]     valueLen Length of @p value in bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL; @p key is NULL/empty; or
 *                                 @p value is NULL when @p valueLen > 0.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe No (mutates graph metadata).
 *
 * @sa occtl_graph_node_metadata_get, occtl_graph_node_metadata_unset
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_metadata_set(occtl_graph_t*  graph,
                                                                  occtl_node_id_t target,
                                                                  const char*     key,
                                                                  size_t          keyLen,
                                                                  const char*     value,
                                                                  size_t          valueLen);

/**
 * Retrieves UTF-8 metadata from a target node (two-call buffer pattern).
 *
 * Call once with @p buf == NULL to learn the required buffer size in
 * @p out_required, then call again with a buffer of at least that size.
 * The written value is NUL-terminated; @p out_required includes the NUL
 * terminator.
 *
 * @param[in]  graph        Borrows it.  Must be non-NULL.
 * @param[in]  target       Node ID to query.  Must be valid and active.
 * @param[in]  key          Metadata key bytes.  Borrowed; must be non-NULL
 *                          and non-empty.
 * @param[in]  keyLen       Length of @p key in bytes.
 * @param[out] buf          Owns it (caller-allocated).  May be NULL to query
 *                          required size.
 * @param[in]  bufSize      Size of @p buf in bytes.
 * @param[out] out_required Borrows it.  Must be non-NULL.  Receives required
 *                          size including NUL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_required is NULL, or
 *                                 @p key is NULL/empty.
 * @retval OCCTL_NOT_FOUND         @p target is invalid / removed, or @p key
 *                                 is not set on @p target.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p buf is non-NULL and @p bufSize is too
 *                                 small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_graph_node_metadata_set, occtl_graph_node_metadata_unset
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_metadata_get(const occtl_graph_t* graph,
                                                                  occtl_node_id_t      target,
                                                                  const char*          key,
                                                                  size_t               keyLen,
                                                                  char*                buf,
                                                                  size_t               bufSize,
                                                                  size_t* out_required);

/**
 * Lists metadata keys stored on a target node.
 *
 * Uses the two-call buffer pattern: pass @p out_keys as NULL with @p cap 0 to
 * learn the key count, then call again with an array of at least that many
 * entries.  Returned key pointers borrow from the graph's internal metadata
 * storage and are not necessarily NUL-terminated; use @c key_len.
 *
 * A valid target with no metadata keys returns #OCCTL_OK and count 0.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  target    Node ID to query.  Must be valid and active.
 * @param[out] out_keys  Borrows it (caller-allocated).  Length @p cap; may be
 *                       NULL to query count.
 * @param[in]  cap       Capacity of @p out_keys in elements.
 * @param[out] out_count Borrows it.  Must be non-NULL.  Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_keys is non-NULL and @p cap is too
 *                                 small.
 *
 * @threadsafe Yes (read-only on graph metadata).
 *
 * @sa occtl_graph_node_metadata_set, occtl_graph_node_metadata_get
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_node_metadata_keys(const occtl_graph_t*       graph,
                                 occtl_node_id_t            target,
                                 occtl_metadata_key_view_t* out_keys,
                                 size_t                     cap,
                                 size_t*                    out_count);

/**
 * Lists nodes that have at least one metadata key.
 *
 * Two-call buffer pattern: pass @p out_nodes as NULL with @p cap 0 to learn
 * the count, then call again with an array of at least that many elements.
 * Retrieve each node's keys with #occtl_graph_node_metadata_keys and each value
 * with #occtl_graph_node_metadata_get.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[out] out_nodes Borrows it.  Length @p cap; may be NULL to query count.
 * @param[in]  cap       Capacity of @p out_nodes in elements.
 * @param[out] out_count Borrows it.  Must be non-NULL.  Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_nodes is non-NULL and @p cap is too small.
 *
 * @threadsafe Yes (read-only on graph metadata).
 *
 * @sa occtl_graph_node_metadata_keys, occtl_graph_name_nodes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_metadata_nodes(const occtl_graph_t* graph,
                                                                    occtl_node_id_t*     out_nodes,
                                                                    size_t               cap,
                                                                    size_t*              out_count);

/**
 * Sets UTF-8 metadata on the graph itself.
 *
 * Graph metadata is for document/model-level attributes such as author,
 * source format, exchange notes, or mesh-model metadata.  It is stored in the
 * graph metadata storage, survives graph clone and native graph snapshots, and
 * is not attached to any particular node.  @p key and @p value are byte
 * buffers; neither needs to be NUL-terminated.  A key must be non-empty.
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
 * @sa occtl_graph_metadata_get, occtl_graph_metadata_unset,
 *     occtl_graph_node_metadata_set
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_metadata_set(occtl_graph_t* graph,
                                                             const char*    key,
                                                             size_t         keyLen,
                                                             const char*    value,
                                                             size_t         valueLen);

/**
 * Retrieves UTF-8 metadata from the graph itself (two-call buffer pattern).
 *
 * Call once with @p buf == NULL to learn the required buffer size in
 * @p out_required, then call again with a buffer of at least that size.
 * The written value is NUL-terminated; @p out_required includes the NUL
 * terminator.
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
 * @sa occtl_graph_metadata_set, occtl_graph_metadata_keys
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_metadata_get(const occtl_graph_t* graph,
                                                             const char*          key,
                                                             size_t               keyLen,
                                                             char*                buf,
                                                             size_t               bufSize,
                                                             size_t*              out_required);

/**
 * Lists graph-level metadata keys.
 *
 * Uses the two-call buffer pattern: pass @p out_keys as NULL with @p cap 0 to
 * learn the key count, then call again with an array of at least that many
 * entries.  Returned key pointers borrow from the graph's internal metadata
 * storage and are not necessarily NUL-terminated; use @c key_len.
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
 * @sa occtl_graph_metadata_get, occtl_graph_node_metadata_keys
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_metadata_keys(const occtl_graph_t*       graph,
                                                              occtl_metadata_key_view_t* out_keys,
                                                              size_t                     cap,
                                                              size_t*                    out_count);

/**
 * Removes one graph-level metadata key.
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
 * @sa occtl_graph_metadata_set, occtl_graph_metadata_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_metadata_unset(occtl_graph_t* graph,
                                                               const char*    key,
                                                               size_t         keyLen);

/**
 * Removes one metadata key from a target node.
 *
 * Idempotent: removing a missing key is a successful no-op.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     target Node ID to modify.  Must be valid and active.
 * @param[in]     key    Metadata key bytes.  Borrowed; must be non-NULL and
 *                       non-empty.
 * @param[in]     keyLen Length of @p key in bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p key is NULL/empty.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe No (mutates graph metadata).
 *
 * @sa occtl_graph_node_metadata_set, occtl_graph_node_metadata_get
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_metadata_unset(occtl_graph_t*  graph,
                                                                    occtl_node_id_t target,
                                                                    const char*     key,
                                                                    size_t          keyLen);

/**
 * Adds a UTF-8 tag to a target node.
 *
 * Tags are byte strings; they do not need to be NUL-terminated.  A tag must be
 * non-empty.  Tags are stored as graph-owned metadata and follow graph
 * compact / clone remapping.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     target Node ID to annotate.  Must be valid and active.
 * @param[in]     tag    Tag bytes.  Borrowed; copied internally.  Must be
 *                       non-NULL and non-empty.
 * @param[in]     tagLen Length of @p tag in bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p tag is NULL/empty.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe No (mutates graph tags).
 *
 * @sa occtl_graph_tag_remove, occtl_graph_tag_has
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_add(occtl_graph_t*  graph,
                                                        occtl_node_id_t target,
                                                        const char*     tag,
                                                        size_t          tagLen);

/**
 * Removes a UTF-8 tag from a target node.
 *
 * Idempotent: removing a missing tag is a successful no-op.
 *
 * @param[in,out] graph  Borrows it.  Must be non-NULL.
 * @param[in]     target Node ID to modify.  Must be valid and active.
 * @param[in]     tag    Tag bytes.  Borrowed; must be non-NULL and non-empty.
 * @param[in]     tagLen Length of @p tag in bytes.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p tag is NULL/empty.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe No (mutates graph tags).
 *
 * @sa occtl_graph_tag_add, occtl_graph_tag_has
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_remove(occtl_graph_t*  graph,
                                                           occtl_node_id_t target,
                                                           const char*     tag,
                                                           size_t          tagLen);

/**
 * Tests whether a target node carries a UTF-8 tag.
 *
 * @param[in]  graph   Borrows it.  Must be non-NULL.
 * @param[in]  target  Node ID to query.  Must be valid and active.
 * @param[in]  tag     Tag bytes.  Borrowed; must be non-NULL and non-empty.
 * @param[in]  tagLen  Length of @p tag in bytes.
 * @param[out] out_has_tag Borrows it.  Must be non-NULL.  Receives 0 or 1.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_has_tag is NULL, or @p tag is
 *                                 NULL/empty.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 *
 * @threadsafe Yes (read-only on graph tags).
 *
 * @sa occtl_graph_tag_add, occtl_graph_tag_list
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_has(const occtl_graph_t* graph,
                                                        occtl_node_id_t      target,
                                                        const char*          tag,
                                                        size_t               tagLen,
                                                        int32_t*             out_has_tag);

/**
 * Lists tags stored on a target node.
 *
 * Uses the two-call buffer pattern: pass @p out_tags as NULL with @p cap 0 to
 * learn the tag count, then call again with an array of at least that many
 * entries.  Returned tag pointers borrow from the graph's internal tag storage
 * and are not necessarily NUL-terminated; use @c tag_len.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  target    Node ID to query.  Must be valid and active.
 * @param[out] out_tags  Borrows it (caller-allocated).  Length @p cap; may be
 *                       NULL to query count.
 * @param[in]  cap       Capacity of @p out_tags in elements.
 * @param[out] out_count Borrows it.  Must be non-NULL.  Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p target is invalid or removed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_tags is non-NULL and @p cap is too
 *                                 small.
 *
 * @threadsafe Yes (read-only on graph tags).
 *
 * @sa occtl_graph_tag_add, occtl_graph_tag_nodes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_list(const occtl_graph_t* graph,
                                                         occtl_node_id_t      target,
                                                         occtl_tag_view_t*    out_tags,
                                                         size_t               cap,
                                                         size_t*              out_count);

/**
 * Lists nodes carrying tags.
 *
 * Pass @p tag == NULL and @p tagLen == 0 to list nodes that have at least one
 * tag.  Pass a non-empty tag to list nodes carrying that exact tag.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  tag       Optional tag bytes.  Borrowed; must be non-NULL when
 *                       @p tagLen is non-zero.
 * @param[in]  tagLen    Length of @p tag in bytes.
 * @param[out] out_nodes Borrows it.  Length @p cap; may be NULL to query count.
 * @param[in]  cap       Capacity of @p out_nodes in elements.
 * @param[out] out_count Borrows it.  Must be non-NULL.  Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL, @p tag is
 *                                 NULL with non-zero @p tagLen, or @p tag is
 *                                 non-NULL with zero @p tagLen.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_nodes is non-NULL and @p cap is too
 *                                 small.
 *
 * @threadsafe Yes (read-only on graph tags).
 *
 * @sa occtl_graph_tag_list, occtl_graph_node_metadata_nodes
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_tag_nodes(const occtl_graph_t* graph,
                                                          const char*          tag,
                                                          size_t               tagLen,
                                                          occtl_node_id_t*     out_nodes,
                                                          size_t               cap,
                                                          size_t*              out_count);

/**
 * Initialises @p info to default values via #OCCTL_JOINT_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it. May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_joint_create
 */
OCCTL_API void OCCTL_CALL occtl_joint_info_init(occtl_joint_info_t* info);

/**
 * Creates an assembly joint record.
 *
 * The joint is copied into graph-owned metadata. Endpoint nodes may be products,
 * occurrences, or topology nodes; metadata follows graph clone / compact
 * remapping and removes joints whose endpoint is removed without replacement.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version and NULL @c p_next.
 * @param[out]    out_joint Borrows it. Must be non-NULL. Receives the new
 *                          graph-local joint ID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, or @p out_joint is NULL,
 *                                 or an info field is malformed.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version is unsupported.
 * @retval OCCTL_NOT_FOUND         An endpoint node is invalid or removed.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph metadata).
 *
 * @sa occtl_joint_get, occtl_joint_list, occtl_joint_remove
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_joint_create(occtl_graph_t*            graph,
                                                       const occtl_joint_info_t* info,
                                                       occtl_joint_id_t*         out_joint);

/**
 * Retrieves an assembly joint record.
 *
 * @param[in]  graph    Borrows it. Must be non-NULL.
 * @param[in]  joint    Joint ID to query. Must be valid in @p graph.
 * @param[out] out_info Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_info is NULL, or
 *                                 @p joint is the invalid sentinel.
 * @retval OCCTL_NOT_FOUND         @p joint is not present in @p graph.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_joint_create, occtl_joint_list, occtl_joint_remove
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_joint_get(const occtl_graph_t* graph,
                                                    occtl_joint_id_t     joint,
                                                    occtl_joint_info_t*  out_info);

/**
 * Removes an assembly joint record.
 *
 * Idempotent: removing a missing joint is a successful no-op.
 *
 * @param[in,out] graph Borrows it. Must be non-NULL.
 * @param[in]     joint Joint ID to remove. Must not be the invalid sentinel.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p joint is invalid.
 *
 * @threadsafe No (mutates graph metadata).
 *
 * @sa occtl_joint_create, occtl_joint_get, occtl_joint_list
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_joint_remove(occtl_graph_t*   graph,
                                                       occtl_joint_id_t joint);

/**
 * Lists assembly joints (two-call buffer pattern).
 *
 * Pass #OCCTL_NODE_ID_INVALID as @p node to list every joint in the graph.
 * Otherwise, only joints whose first or second endpoint is @p node are listed.
 * Call once with @p out_joints == NULL to learn the required count, then call
 * again with an array of at least that size.
 *
 * @param[in]  graph      Borrows it. Must be non-NULL.
 * @param[in]  node       Endpoint filter, or #OCCTL_NODE_ID_INVALID for all.
 * @param[out] out_joints Owns it (caller-allocated). May be NULL to query
 *                        required count.
 * @param[in]  cap        Capacity of @p out_joints in elements.
 * @param[out] out_count  Borrows it. Must be non-NULL. Receives total count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p node is non-zero and invalid or removed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_joints is non-NULL and @p cap is too
 *                                 small.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_joint_create, occtl_joint_get, occtl_joint_remove
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_joint_list(const occtl_graph_t* graph,
                                                     occtl_node_id_t      node,
                                                     occtl_joint_id_t*    out_joints,
                                                     size_t               cap,
                                                     size_t*              out_count);

#define OCCTL_TOPO_MAKE_PRODUCT_INFO_VERSION_1 1u

/**
 * Info for #occtl_topo_make_product.
 *
 * When @c root == OCCTL_NODE_ID_INVALID, an empty product is created.
 * Otherwise the product wraps @c root with @c placement as a shape root.
 */
typedef struct occtl_topo_make_product_info
{
  uint32_t          struct_version; /**< Must be #OCCTL_TOPO_MAKE_PRODUCT_INFO_VERSION_1. */
  const void*       p_next;         /**< Reserved; set to NULL. */
  occtl_node_id_t   root;           /**< Topology root node, or all-zero for empty product. */
  occtl_transform_t placement;      /**< Placement of the root, or identity. */
} occtl_topo_make_product_info_t;

#define OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT                                                          \
  {OCCTL_TOPO_MAKE_PRODUCT_INFO_VERSION_1,                                                         \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0}}

/**
 * Initialises @p info to default values via #OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] info Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_make_product
 */
OCCTL_API void OCCTL_CALL occtl_topo_make_product_info_init(occtl_topo_make_product_info_t* info);

/**
 * Creates a product in the graph.
 *
 * The product is an assembly root.  When @c info->root is a valid
 * (non-zero) NodeId the product wraps that topology root via a
 * placement; otherwise an empty product with no shape root is created.
 *
 * @param[in,out] graph      Borrows it.  Must be non-NULL.
 * @param[in]     info       Borrows it.  Must be non-NULL.
 * @param[out]    out_product Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p info, or @p out_product is NULL;
 *                                 @c info->p_next is non-NULL; or @c info->placement
 *                                 contains non-finite values.
 * @retval OCCTL_VERSION_MISMATCH  @c info->struct_version is unsupported.
 * @retval OCCTL_NOT_FOUND         @c info->root is non-zero and invalid or removed.
 * @retval OCCTL_OUT_OF_MEMORY     Allocation failed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_link_product, occtl_topo_link_products
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_product(occtl_graph_t*                        graph,
                          const occtl_topo_make_product_info_t* info,
                          occtl_node_id_t*                      out_product);

/**
 * Links an existing product to a topology root with a placement.
 *
 * Creates a new occurrence whose child is the topology root and adds it
 * as the product's shape root.  The product must have been created by
 * #occtl_topo_make_product.
 *
 * @param[in,out] graph      Borrows it.  Must be non-NULL.
 * @param[in]     product    Product node ID.  Must be valid and active.
 * @param[in]     root       Topology root node ID.  Must be valid and active.
 * @param[in]     placement  Placement transform for the occurrence.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p placement contains
 *                                 non-finite values.
 * @retval OCCTL_NOT_FOUND         @p product or @p root is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p product is not a product.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_make_product, occtl_topo_link_products
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_link_product(occtl_graph_t*    graph,
                                                            occtl_node_id_t   product,
                                                            occtl_node_id_t   root,
                                                            occtl_transform_t placement);

/**
 * Links an existing product to a topology root and returns the new occurrence.
 *
 * This is the binding-friendly variant of
 * #occtl_topo_link_product.  It performs the same mutation but
 * also reports the created occurrence node so callers do not need to scan the
 * product's occurrence list to find it.
 *
 * @param[in,out] graph          Borrows it.  Must be non-NULL.
 * @param[in]     product        Product node ID.  Must be valid and active.
 * @param[in]     root           Topology root node ID.  Must be valid and active.
 * @param[in]     placement      Placement transform for the occurrence.
 * @param[out]    out_occurrence Borrows it.  Must be non-NULL.  Receives the
 *                               created occurrence node.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_occurrence is NULL, or
 *                                 @p placement contains non-finite values.
 * @retval OCCTL_NOT_FOUND         @p product or @p root is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p product is not a product.
 * @retval OCCTL_ERROR             OCCT failed to create the occurrence.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_link_product, occtl_topo_occurrence_transform
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_link_product_occurrence(occtl_graph_t*    graph,
                                     occtl_node_id_t   product,
                                     occtl_node_id_t   root,
                                     occtl_transform_t placement,
                                     occtl_node_id_t*  out_occurrence);

/**
 * Links two products via a parent-child occurrence.
 *
 * The child product is instantiated as an occurrence within the parent
 * product.  The optional @p parentOccurrence parameter allows nesting
 * the new child under an existing occurrence chain.
 *
 * @param[in,out] graph            Borrows it.  Must be non-NULL.
 * @param[in]     parentProduct    Parent product node ID.  Must be valid and active.
 * @param[in]     childProduct     Child product node ID.  Must be valid and active.
 * @param[in]     placement        Placement transform for the child occurrence.
 * @param[in]     parentOccurrence Existing parent occurrence, or all-zero for
 *                                    root-level placement.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         A product NodeId is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A NodeId is not a product.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_make_product, occtl_topo_remove_occurrence
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_link_products(occtl_graph_t*    graph,
                                                             occtl_node_id_t   parentProduct,
                                                             occtl_node_id_t   childProduct,
                                                             occtl_transform_t placement,
                                                             occtl_node_id_t   parentOccurrence);

/**
 * Links two products and returns the new occurrence node.
 *
 * This is the binding-friendly variant of #occtl_topo_link_products.  It
 * performs the same assembly mutation but also reports the created occurrence
 * node for immediate naming, tagging, transforms, or joint creation.
 *
 * @param[in,out] graph            Borrows it.  Must be non-NULL.
 * @param[in]     parentProduct    Parent product node ID.  Must be valid and active.
 * @param[in]     childProduct     Child product node ID.  Must be valid and active.
 * @param[in]     placement        Placement transform for the child occurrence.
 * @param[in]     parentOccurrence Existing parent occurrence, or all-zero for
 *                                 root-level placement.
 * @param[out]    out_occurrence   Borrows it.  Must be non-NULL.  Receives the
 *                                 created occurrence node.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_occurrence is NULL, or
 *                                 @p placement contains non-finite values.
 * @retval OCCTL_NOT_FOUND         A product NodeId is invalid or removed.
 * @retval OCCTL_WRONG_KIND        A NodeId is not a product.
 * @retval OCCTL_ERROR             OCCT failed to create the occurrence.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_link_products, occtl_topo_occurrence_transform
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_link_products_occurrence(occtl_graph_t*    graph,
                                      occtl_node_id_t   parentProduct,
                                      occtl_node_id_t   childProduct,
                                      occtl_transform_t placement,
                                      occtl_node_id_t   parentOccurrence,
                                      occtl_node_id_t*  out_occurrence);

/**
 * Removes an occurrence reference from the graph.
 *
 * Detaches a child usage from its parent product.  The referenced child
 * definition (product or topology root) is not removed unless it has
 * no other active usages.
 *
 * @param[in,out] graph       Borrows it.  Must be non-NULL.
 * @param[in]     occurrence_ref Occurrence reference to remove.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL.
 * @retval OCCTL_NOT_FOUND         @p occurrence_ref is invalid or removed.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_link_products
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove_occurrence(occtl_graph_t* graph,
                                                                 occtl_ref_id_t occurrence_ref);

/**
 * Sets the local placement carried by an occurrence node.
 *
 * The placement is graph-stored on the unique active occurrence reference
 * that owns @p occurrence; the occurrence definition and child product/root
 * are not copied.  If the occurrence definition has more than one active
 * reference, use the generic reference-location editor instead.
 *
 * @param[in,out] graph      Borrows it.  Must be non-NULL.
 * @param[in]     occurrence Occurrence node ID to modify.
 * @param[in]     transform  New local placement.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph is NULL, or @p transform contains
 *                                 non-finite values, or @p occurrence has more
 *                                 than one active reference.
 * @retval OCCTL_NOT_FOUND         @p occurrence is invalid, removed, or has no
 *                                 active occurrence reference.
 * @retval OCCTL_WRONG_KIND        @p occurrence is not an occurrence node.
 *
 * @threadsafe No (mutates graph).
 *
 * @sa occtl_topo_occurrence_transform, occtl_topo_set_ref_location
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_occurrence_set_transform(occtl_graph_t*    graph,
                                      occtl_node_id_t   occurrence,
                                      occtl_transform_t transform);

/**
 * Retrieves the local placement carried by an occurrence node.
 *
 * @param[in]  graph         Borrows it.  Must be non-NULL.
 * @param[in]  occurrence    Occurrence node ID to query.
 * @param[out] out_transform Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_transform is NULL.
 * @retval OCCTL_NOT_FOUND         @p occurrence is invalid, removed, or has no
 *                                 active occurrence reference.
 * @retval OCCTL_WRONG_KIND        @p occurrence is not an occurrence node.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_occurrence_set_transform, occtl_topo_occurrence_world_transform
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_occurrence_transform(const occtl_graph_t* graph,
                                  occtl_node_id_t      occurrence,
                                  occtl_transform_t*   out_transform);

/**
 * Retrieves an occurrence's accumulated transform from a traversal root.
 *
 * The helper walks downward from @p root using BRepGraph's child explorer and
 * returns the accumulated placement for the first path that reaches
 * @p occurrence.  Pass a root Product for assembly-world placement.
 *
 * @param[in]  graph         Borrows it.  Must be non-NULL.
 * @param[in]  root          Traversal root.  Must be valid and active.
 * @param[in]  occurrence    Occurrence node ID to locate below @p root.
 * @param[out] out_transform Borrows it.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_transform is NULL.
 * @retval OCCTL_NOT_FOUND         @p root / @p occurrence is invalid or
 *                                 @p occurrence is not reachable from @p root.
 * @retval OCCTL_WRONG_KIND        @p occurrence is not an occurrence node.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_child_explorer_create, occtl_topo_occurrence_transform
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_occurrence_world_transform(const occtl_graph_t* graph,
                                        occtl_node_id_t      root,
                                        occtl_node_id_t      occurrence,
                                        occtl_transform_t*   out_transform);

#define OCCTL_EDGE_VIEW_VERSION_1 1u

/**
 * Aggregate snapshot of an edge's scalar state.
 *
 * Each aggregate view struct opens with @c struct_version and @c p_next.
 * Callers must initialise the struct with the matching init function or
 * INIT literal so the library can dispatch on the version they understand.
 * Filler functions write scalars only, so view lifetime ends with the
 * function call.
 *
 * Fields after @c p_next are written by the library.  All fields are
 * caller-owned (no borrowed pointers): the snapshot is decoupled from
 * the source graph and survives subsequent mutation.
 *
 * The boolean fields are 0/1; the int32_t storage forbids @c bool per
 * the ABI rules.  Face-context queries (is_seam_on_face,
 * is_boundary_on_face) are not part of the snapshot; use the
 * @c occtl_topo_edge_is_*_on_face accessors for those.
 */
typedef struct occtl_edge_view
{
  uint32_t    struct_version; /**< INPUT — caller declares which version they understand. */
  const void* p_next;         /**< Reserved; must be NULL. */

  double          t_min;                 /**< Parameter range minimum. */
  double          t_max;                 /**< Parameter range maximum. */
  double          tolerance;             /**< Geometric tolerance. */
  occtl_node_id_t start_vertex;          /**< Start-vertex node id. */
  occtl_node_id_t end_vertex;            /**< End-vertex node id. */
  uint32_t        internal_vertex_count; /**< Internal vertex count (excluding start/end). */
  uint32_t        face_count;            /**< Number of faces incident to this edge. */
  int32_t         has_curve;             /**< 1 if the edge carries a 3D curve. */
  int32_t         is_degenerated;        /**< 1 if marked degenerated. */
  int32_t         is_closed;             /**< 1 if the start vertex equals the end vertex. */
  int32_t         same_parameter;        /**< 1 if the @c SameParameter flag is set. */
  int32_t         same_range;            /**< 1 if the @c SameRange flag is set. */
  int32_t         is_manifold;           /**< 1 if the edge is shared by at most two faces. */
  int32_t         is_boundary; /**< 1 if the edge is on the boundary of exactly one face. */
} occtl_edge_view_t;

#define OCCTL_EDGE_VIEW_INIT                                                                       \
  {OCCTL_EDGE_VIEW_VERSION_1,                                                                      \
   NULL,                                                                                           \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   0u,                                                                                             \
   0u,                                                                                             \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0,                                                                                              \
   0}

/**
 * Initialises @p view to default values matching #OCCTL_EDGE_VIEW_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] view  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_topo_edge_view
 */
OCCTL_API void OCCTL_CALL occtl_edge_view_init(occtl_edge_view_t* view);

/**
 * Fills @p view with the current scalar state of @p edge.
 *
 * @p view's @c struct_version must match a known version constant; only
 * fields up to that version are written.  The fill is atomic with
 * respect to the C-call boundary but races with concurrent graph
 * mutation.
 *
 * @param[in]  graph Must be non-NULL.
 * @param[in]  edge  Edge node id.
 * @param[out] view  Borrows it (caller-allocated).  Must be non-NULL and
 *                   initialised via #occtl_edge_view_init or
 *                   #OCCTL_EDGE_VIEW_INIT.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p view is NULL, or @p view->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p edge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p edge is not an edge.
 * @retval OCCTL_VERSION_MISMATCH  @p view's struct_version is not a supported value.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_view, occtl_topo_coedge_view
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_view(const occtl_graph_t* graph,
                                                         occtl_node_id_t      edge,
                                                         occtl_edge_view_t*   view);

#define OCCTL_COEDGE_VIEW_VERSION_1 1u

/**
 * Aggregate snapshot of a coedge's scalar state.
 *
 * @c uv_start / @c uv_end are the UV-space endpoints of the coedge's
 * pcurve on the parent face's surface, as reported by
 * @c BRepGraph_Tool::CoEdge::UVPoints.  When @c has_pcurve is 0 they
 * default to (0,0).
 */
typedef struct occtl_coedge_view
{
  uint32_t    struct_version; /**< INPUT — caller declares which version they understand. */
  const void* p_next;         /**< Reserved; must be NULL. */

  occtl_node_id_t     edge_of;     /**< Underlying edge node id. */
  occtl_node_id_t     face_of;     /**< Parent face node id. */
  occtl_orientation_t orientation; /**< Coedge orientation. */
  double              t_min;       /**< Parameter range minimum. */
  double              t_max;       /**< Parameter range maximum. */
  occtl_point2_t      uv_start;    /**< UV endpoint at @c t_min. */
  occtl_point2_t      uv_end;      /**< UV endpoint at @c t_max. */
  int32_t             is_seam;     /**< 1 if the coedge is a seam pair member. */
  int32_t             is_reversed; /**< 1 if reversed relative to its edge. */
  int32_t             has_pcurve;  /**< 1 if a 2D pcurve is attached. */
} occtl_coedge_view_t;

#define OCCTL_COEDGE_VIEW_INIT                                                                     \
  {OCCTL_COEDGE_VIEW_VERSION_1,                                                                    \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_ORIENTATION_FORWARD,                                                                      \
   0.0,                                                                                            \
   0.0,                                                                                            \
   {0.0, 0.0},                                                                                     \
   {0.0, 0.0},                                                                                     \
   0,                                                                                              \
   0,                                                                                              \
   0}

/** @copydoc occtl_edge_view_init */
OCCTL_API void OCCTL_CALL occtl_coedge_view_init(occtl_coedge_view_t* view);

/**
 * Fills @p view with the current scalar state of @p coedge.
 *
 * @p view's @c struct_version must match a known version constant; only
 * fields up to that version are written.
 *
 * @param[in]  graph   Must be non-NULL.
 * @param[in]  coedge  Coedge node id.
 * @param[out] view    Borrows it (caller-allocated). Must be non-NULL and
 *                     initialised via #occtl_coedge_view_init or
 *                     #OCCTL_COEDGE_VIEW_INIT.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p view is NULL, or @p view->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p coedge is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p coedge is not a coedge.
 * @retval OCCTL_VERSION_MISMATCH  @p view's struct_version is not a supported value.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_view, occtl_topo_face_view
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_view(const occtl_graph_t* graph,
                                                           occtl_node_id_t      coedge,
                                                           occtl_coedge_view_t* view);

#define OCCTL_FACE_VIEW_VERSION_1 1u

/**
 * Aggregate snapshot of a face's scalar state.
 */
typedef struct occtl_face_view
{
  uint32_t    struct_version; /**< INPUT — caller declares which version they understand. */
  const void* p_next;         /**< Reserved; must be NULL. */

  double          u_min;               /**< U parameter minimum. */
  double          u_max;               /**< U parameter maximum. */
  double          v_min;               /**< V parameter minimum. */
  double          v_max;               /**< V parameter maximum. */
  double          tolerance;           /**< Geometric tolerance. */
  occtl_node_id_t outer_wire;          /**< Outer-wire node id, or invalid if none. */
  uint32_t        wire_count;          /**< Total wire count (including the outer wire). */
  int32_t         has_surface;         /**< 1 if a surface representation is attached. */
  int32_t         has_triangulation;   /**< 1 if any triangulation is attached. */
  int32_t         natural_restriction; /**< 1 if the face uses the natural surface restriction. */
} occtl_face_view_t;

#define OCCTL_FACE_VIEW_INIT                                                                       \
  {OCCTL_FACE_VIEW_VERSION_1, NULL, 0.0, 0.0, 0.0, 0.0, 0.0, OCCTL_NODE_ID_INVALID, 0u, 0, 0, 0}

/** @copydoc occtl_edge_view_init */
OCCTL_API void OCCTL_CALL occtl_face_view_init(occtl_face_view_t* view);

/**
 * Fills @p view with the current scalar state of @p face.
 *
 * @p view's @c struct_version must match a known version constant; only
 * fields up to that version are written.
 *
 * @param[in]  graph Must be non-NULL.
 * @param[in]  face  Face node id.
 * @param[out] view  Borrows it (caller-allocated). Must be non-NULL and
 *                   initialised via #occtl_face_view_init or
 *                   #OCCTL_FACE_VIEW_INIT.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p view is NULL, or @p view->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p face is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p face is not a face.
 * @retval OCCTL_VERSION_MISMATCH  @p view's struct_version is not a supported value.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_view, occtl_topo_wire_view
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_view(const occtl_graph_t* graph,
                                                         occtl_node_id_t      face,
                                                         occtl_face_view_t*   view);

#define OCCTL_VERTEX_VIEW_VERSION_1 1u

/**
 * Aggregate snapshot of a vertex's scalar state.
 */
typedef struct occtl_vertex_view
{
  uint32_t    struct_version; /**< INPUT — caller declares which version they understand. */
  const void* p_next;         /**< Reserved; must be NULL. */

  occtl_point3_t point;     /**< 3D point in definition frame. */
  double         tolerance; /**< Vertex tolerance. */
} occtl_vertex_view_t;

#define OCCTL_VERTEX_VIEW_INIT {OCCTL_VERTEX_VIEW_VERSION_1, NULL, {0.0, 0.0, 0.0}, 0.0}

/** @copydoc occtl_edge_view_init */
OCCTL_API void OCCTL_CALL occtl_vertex_view_init(occtl_vertex_view_t* view);

/**
 * Fills @p view with the current scalar state of @p vertex.
 *
 * @p view's @c struct_version must match a known version constant; only
 * fields up to that version are written.
 *
 * @param[in]  graph  Must be non-NULL.
 * @param[in]  vertex Vertex node id.
 * @param[out] view   Borrows it (caller-allocated). Must be non-NULL and
 *                    initialised via #occtl_vertex_view_init or
 *                    #OCCTL_VERTEX_VIEW_INIT.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p view is NULL, or @p view->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p vertex is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p vertex is not a vertex.
 * @retval OCCTL_VERSION_MISMATCH  @p view's struct_version is not a supported value.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_edge_view
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_view(const occtl_graph_t* graph,
                                                           occtl_node_id_t      vertex,
                                                           occtl_vertex_view_t* view);

#define OCCTL_WIRE_VIEW_VERSION_1 1u

/**
 * Aggregate snapshot of a wire's scalar state.
 */
typedef struct occtl_wire_view
{
  uint32_t    struct_version; /**< INPUT — caller declares which version they understand. */
  const void* p_next;         /**< Reserved; must be NULL. */

  int32_t is_closed;           /**< 1 if the wire is topologically closed. */
  size_t  coedge_count;        /**< Number of coedge entries in the wire. */
  size_t  distinct_edge_count; /**< Number of distinct underlying edges. */
} occtl_wire_view_t;

#define OCCTL_WIRE_VIEW_INIT {OCCTL_WIRE_VIEW_VERSION_1, NULL, 0, 0, 0}

/** @copydoc occtl_edge_view_init */
OCCTL_API void OCCTL_CALL occtl_wire_view_init(occtl_wire_view_t* view);

/**
 * Fills @p view with the current scalar state of @p wire.
 *
 * @p view's @c struct_version must match a known version constant; only
 * fields up to that version are written.
 *
 * @param[in]  graph Must be non-NULL.
 * @param[in]  wire  Wire node id.
 * @param[out] view  Borrows it (caller-allocated). Must be non-NULL and
 *                   initialised via #occtl_wire_view_init or
 *                   #OCCTL_WIRE_VIEW_INIT.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p view is NULL, or @p view->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p wire is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p wire is not a wire.
 * @retval OCCTL_VERSION_MISMATCH  @p view's struct_version is not a supported value.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_view, occtl_topo_edge_view
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_view(const occtl_graph_t* graph,
                                                         occtl_node_id_t      wire,
                                                         occtl_wire_view_t*   view);

#define OCCTL_SHELL_VIEW_VERSION_1 1u

/**
 * Aggregate snapshot of a shell's scalar state.
 */
typedef struct occtl_shell_view
{
  uint32_t    struct_version; /**< INPUT — caller declares which version they understand. */
  const void* p_next;         /**< Reserved; must be NULL. */

  int32_t is_closed;  /**< 1 if the shell is watertight. */
  size_t  face_count; /**< Number of face entries in the shell. */
} occtl_shell_view_t;

#define OCCTL_SHELL_VIEW_INIT {OCCTL_SHELL_VIEW_VERSION_1, NULL, 0, 0}

/** @copydoc occtl_edge_view_init */
OCCTL_API void OCCTL_CALL occtl_shell_view_init(occtl_shell_view_t* view);

/**
 * Fills @p view with the current scalar state of @p shell.
 *
 * @p view's @c struct_version must match a known version constant; only
 * fields up to that version are written.
 *
 * @param[in]  graph Must be non-NULL.
 * @param[in]  shell Shell node id.
 * @param[out] view  Borrows it (caller-allocated). Must be non-NULL and
 *                   initialised via #occtl_shell_view_init or
 *                   #OCCTL_SHELL_VIEW_INIT.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p view is NULL, or @p view->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p shell is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p shell is not a shell.
 * @retval OCCTL_VERSION_MISMATCH  @p view's struct_version is not a supported value.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_face_view, occtl_topo_solid_view
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_shell_view(const occtl_graph_t* graph,
                                                          occtl_node_id_t      shell,
                                                          occtl_shell_view_t*  view);

#define OCCTL_SOLID_VIEW_VERSION_1 1u

/**
 * Aggregate snapshot of a solid's scalar state.
 */
typedef struct occtl_solid_view
{
  uint32_t    struct_version; /**< INPUT — caller declares which version they understand. */
  const void* p_next;         /**< Reserved; must be NULL. */

  size_t shell_count; /**< Number of shell entries in the solid. */
} occtl_solid_view_t;

#define OCCTL_SOLID_VIEW_INIT {OCCTL_SOLID_VIEW_VERSION_1, NULL, 0}

/** @copydoc occtl_edge_view_init */
OCCTL_API void OCCTL_CALL occtl_solid_view_init(occtl_solid_view_t* view);

/**
 * Fills @p view with the current scalar state of @p solid.
 *
 * @p view's @c struct_version must match a known version constant; only
 * fields up to that version are written.
 *
 * @param[in]  graph Must be non-NULL.
 * @param[in]  solid Solid node id.
 * @param[out] view  Borrows it (caller-allocated). Must be non-NULL and
 *                   initialised via #occtl_solid_view_init or
 *                   #OCCTL_SOLID_VIEW_INIT.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p view is NULL, or @p view->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p solid is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p solid is not a solid.
 * @retval OCCTL_VERSION_MISMATCH  @p view's struct_version is not a supported value.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_shell_view, occtl_topo_compound_view
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_solid_view(const occtl_graph_t* graph,
                                                          occtl_node_id_t      solid,
                                                          occtl_solid_view_t*  view);

#define OCCTL_COMPOUND_VIEW_VERSION_1 1u

/**
 * Aggregate snapshot of a compound's scalar state.
 */
typedef struct occtl_compound_view
{
  uint32_t    struct_version; /**< INPUT — caller declares which version they understand. */
  const void* p_next;         /**< Reserved; must be NULL. */

  size_t child_count; /**< Number of child entries in the compound. */
} occtl_compound_view_t;

#define OCCTL_COMPOUND_VIEW_INIT {OCCTL_COMPOUND_VIEW_VERSION_1, NULL, 0}

/** @copydoc occtl_edge_view_init */
OCCTL_API void OCCTL_CALL occtl_compound_view_init(occtl_compound_view_t* view);

/**
 * Fills @p view with the current scalar state of @p compound.
 *
 * @p view's @c struct_version must match a known version constant; only
 * fields up to that version are written.
 *
 * @param[in]  graph    Must be non-NULL.
 * @param[in]  compound Compound node id.
 * @param[out] view     Borrows it (caller-allocated). Must be non-NULL and
 *                      initialised via #occtl_compound_view_init or
 *                      #OCCTL_COMPOUND_VIEW_INIT.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p view is NULL, or @p view->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p compound is invalid or removed.
 * @retval OCCTL_WRONG_KIND        @p compound is not a compound.
 * @retval OCCTL_VERSION_MISMATCH  @p view's struct_version is not a supported value.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_topo_solid_view
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_compound_view(const occtl_graph_t*   graph,
                                                             occtl_node_id_t        compound,
                                                             occtl_compound_view_t* view);

#ifdef __cplusplus
} /* extern "C" */

inline size_t occtl_graph_count_value(occtl_status_t(OCCTL_CALL* count_fn)(const occtl_graph_t*,
                                                                           size_t*),
                                      const occtl_graph_t* graph) noexcept
{
  size_t aCount = 0;
  return count_fn(graph, &aCount) == OCCTL_OK ? aCount : 0;
}
#endif

#endif /* OCCTL_TOPO_H */
