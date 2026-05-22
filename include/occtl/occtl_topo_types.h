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
 * @file occtl_topo_types.h
 * @brief OCCT-Light: Core topology identity types.
 *
 * Defines the fundamental topology identity types used throughout the
 * topo module and its sub-headers: occtl_node_id_t, occtl_ref_id_t,
 * occtl_ref_uid_t, occtl_rep_uid_t, occtl_joint_id_t, occtl_graph_t,
 * and their associated sentinel macros and wire-format constants.
 */

#ifndef OCCTL_TOPO_TYPES_H
#define OCCTL_TOPO_TYPES_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Session-local identity of a graph node.
 *
 * Invalidated by graph compaction (introduced in a later phase).  For
 * persistent identity use #occtl_uid_t.  The all-zero value
 * (#OCCTL_NODE_ID_INVALID) is the invalid sentinel — check with
 * @c id.bits == 0.
 *
 * To query the kind of a node ID, call #occtl_graph_node_kind.
 */
typedef struct occtl_node_id
{
  uint64_t bits; /**< Opaque 64-bit identity. */
} occtl_node_id_t;

/**
 * Session-local identity of a reference entry in the graph.
 *
 * Invalidated by graph compaction together with node IDs.  The
 * all-zero value (#OCCTL_REF_ID_INVALID) is the invalid sentinel.
 */
typedef struct occtl_ref_id
{
  uint64_t bits; /**< Opaque 64-bit reference identity. */
} occtl_ref_id_t;

/**
 * Persistent identity of a reference entry in the graph.
 *
 * RefUIDs survive graph compaction and can be resolved back to the current
 * #occtl_ref_id_t while the reference is active.  They identify reference
 * entries such as shell/face/wire/coedge refs and assembly occurrence refs;
 * for topology nodes use #occtl_uid_t instead.
 *
 * The all-zero value (#OCCTL_REF_UID_INVALID) is the invalid sentinel.
 */
typedef struct occtl_ref_uid
{
  uint64_t bits; /**< Opaque 64-bit persistent reference identity. */
} occtl_ref_uid_t;

/**
 * Persistent identity of a representation entry in the graph.
 *
 * RepUIDs survive graph compaction and can be resolved back to the current
 * #occtl_rep_id_t while the representation is active.  They identify geometry
 * and mesh representation entries such as surfaces, curves, triangulations,
 * and polygons; for topology nodes use #occtl_uid_t instead.
 *
 * The all-zero value (#OCCTL_REP_UID_INVALID) is the invalid sentinel.
 */
typedef struct occtl_rep_uid
{
  uint64_t bits; /**< Opaque 64-bit persistent representation identity. */
} occtl_rep_uid_t;

/**
 * Number of bytes in the wire-format encoding of an #occtl_ref_uid_t.
 *
 * The encoding has the same size and reserved-byte policy as
 * #OCCTL_UID_WIRE_SIZE: the top 8 bytes carry the current packed RefUID in
 * big-endian order, and the bottom 8 bytes are reserved zero bytes.
 */
#define OCCTL_REF_UID_WIRE_SIZE OCCTL_UID_WIRE_SIZE

/**
 * Number of bytes in the wire-format encoding of an #occtl_rep_uid_t.
 *
 * The encoding has the same size and reserved-byte policy as
 * #OCCTL_UID_WIRE_SIZE: the top 8 bytes carry the current packed RepUID in
 * big-endian order, and the bottom 8 bytes are reserved zero bytes.
 */
#define OCCTL_REP_UID_WIRE_SIZE OCCTL_UID_WIRE_SIZE

/**
 * Session-local identity of an assembly joint record.
 *
 * Joint IDs are scoped to one graph.  The all-zero value
 * (#OCCTL_JOINT_ID_INVALID) is the invalid sentinel.
 */
typedef struct occtl_joint_id
{
  uint64_t bits; /**< Opaque 64-bit joint identity. */
} occtl_joint_id_t;

/**
 * Invalid sentinels for the four ID types.
 *
 * Defined as C99 compound literals in C and as C++ braced initialisers
 * in C++; this keeps the macros usable on MSVC and pedantic C++ modes
 * without changing their value.
 */
#ifdef __cplusplus
  #define OCCTL_NODE_ID_INVALID (occtl_node_id_t{0})
  #define OCCTL_REF_ID_INVALID (occtl_ref_id_t{0})
  #define OCCTL_REF_UID_INVALID (occtl_ref_uid_t{0})
  #define OCCTL_REP_UID_INVALID (occtl_rep_uid_t{0})
  #define OCCTL_JOINT_ID_INVALID (occtl_joint_id_t{0})
#else
  #define OCCTL_NODE_ID_INVALID ((occtl_node_id_t){0})
  #define OCCTL_REF_ID_INVALID ((occtl_ref_id_t){0})
  #define OCCTL_REF_UID_INVALID ((occtl_ref_uid_t){0})
  #define OCCTL_REP_UID_INVALID ((occtl_rep_uid_t){0})
  #define OCCTL_JOINT_ID_INVALID ((occtl_joint_id_t){0})
#endif

/**
 * Opaque handle to a topology graph.
 *
 * Created by #occtl_graph_create and released by #occtl_graph_free.
 * Not thread-safe for concurrent mutation; concurrent reads on
 * distinct graphs are safe.
 */
typedef struct occtl_graph occtl_graph_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_TOPO_TYPES_H */
