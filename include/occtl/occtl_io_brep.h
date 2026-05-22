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
 * @file occtl_io_brep.h
 * @brief OCCT-Light: native OCCT BRep file I/O.
 *
 * Read and write `.brep` files via OCCT's @c DEBREP_Provider /
 * @c DEBREP_ConfigurationNode.  The provider round-trips through OCCT's
 * internal shape representation.
 */

#ifndef OCCTL_IO_BREP_H
#define OCCTL_IO_BREP_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_IO_BREP_WRITE_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_io_brep_write.  Pass NULL for defaults (binary
 * output, triangulations included).  The format flavour (binary vs
 * ASCII) is determined by the @c DEBREP_Provider from its
 * configuration — the caller can rely on OCCT's default binary
 * encoding without setting it explicitly here.
 */
typedef struct occtl_io_brep_write_options
{
  uint32_t    struct_version;      /**< Must be #OCCTL_IO_BREP_WRITE_OPTIONS_VERSION_1. */
  const void* p_next;              /**< Reserved; set to NULL. */
  int32_t     write_triangulation; /**< 1 to include cached triangulations (default). */
} occtl_io_brep_write_options_t;

#define OCCTL_IO_BREP_WRITE_OPTIONS_INIT {OCCTL_IO_BREP_WRITE_OPTIONS_VERSION_1, NULL, 1}

/**
 * Initialises @p options to default values matching #OCCTL_IO_BREP_WRITE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_brep_write
 */
OCCTL_API void OCCTL_CALL occtl_io_brep_write_options_init(occtl_io_brep_write_options_t* options);

/**
 * Reads a BRep file and ingests it into a freshly-created graph.
 *
 * The returned graph contains the topology rooted at @p out_root.
 * Caller owns the graph and must release it via #occtl_graph_free.
 *
 * @param[in]  path        Borrows it.  NUL-terminated UTF-8 path to the file.
 * @param[out] out_graph   Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root    Borrows it.  Receives the NodeId of the root entity.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p path, @p out_graph, or @p out_root is NULL.
 * @retval OCCTL_IO_ERROR          Filesystem failure or unreadable file.
 * @retval OCCTL_FORMAT_ERROR      File contents were not a valid BRep stream.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_brep_write, occtl_de_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_brep_read(const char*      path,
                                                       occtl_graph_t**  out_graph,
                                                       occtl_node_id_t* out_root);

/**
 * Writes the topology rooted at @p root to a BRep file.
 *
 * Triangulation policy is taken from @p options; pass NULL for
 * defaults (binary, triangulations included).  The output format
 * (binary / ASCII) is managed by the @c DEBREP_Provider internally.
 *
 * @param[in]  graph    Borrows it.  Must be non-NULL.
 * @param[in]  root     Root node id to serialise.
 * @param[in]  path     Borrows it.  NUL-terminated UTF-8 path; existing file is overwritten.
 * @param[in]  options  Borrows it.  May be NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p path is NULL; or @p options has invalid fields
 *                                 (non-NULL @c p_next, @c write_triangulation not in {0,1}).
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_IO_ERROR          Filesystem failure or unwritable file.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_brep_read, occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_brep_write(const occtl_graph_t*                 graph,
                      occtl_node_id_t                      root,
                      const char*                          path,
                      const occtl_io_brep_write_options_t* options);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_IO_BREP_H */
