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
 * @file occtl_io_iges.h
 * @brief OCCT-Light: IGES file I/O (US PRO/IP 100 / ANSI).
 *
 * Reads and writes IGES 5.3 files via OCCT's
 * @c DEIGES_Provider / @c DEIGES_ConfigurationNode.  OCAF is linked
 * privately and never reaches the public surface.
 *
 * UIDs are *not* preserved across the round-trip — the IGES file
 * format has no UID slot.  Use #occtl_uid_to_bytes /
 * #occtl_uid_from_bytes for an out-of-band UID handshake if persistent
 * identity is required.
 */

#ifndef OCCTL_IO_IGES_H
#define OCCTL_IO_IGES_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_IO_IGES_READ_OPTIONS_VERSION_1 1u
#define OCCTL_IO_IGES_WRITE_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_io_iges_read.  Pass NULL for defaults.
 */
typedef struct occtl_io_iges_read_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_IO_IGES_READ_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  int32_t     read_color;     /**< 1 to import colour from file (default). */
  int32_t     read_name;      /**< 1 to import names from file (default). */
} occtl_io_iges_read_options_t;

#define OCCTL_IO_IGES_READ_OPTIONS_INIT {OCCTL_IO_IGES_READ_OPTIONS_VERSION_1, NULL, 1, 1}

/**
 * Options for #occtl_io_iges_write.  Pass NULL for defaults
 * (faces-only output).
 */
typedef struct occtl_io_iges_write_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_IO_IGES_WRITE_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  int32_t     write_brep;     /**< 1 = write BRep, 0 = write faces only (default). */
} occtl_io_iges_write_options_t;

#define OCCTL_IO_IGES_WRITE_OPTIONS_INIT {OCCTL_IO_IGES_WRITE_OPTIONS_VERSION_1, NULL, 0}

/**
 * Initialises @p options to default values matching #OCCTL_IO_IGES_READ_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_iges_read
 */
OCCTL_API void OCCTL_CALL occtl_io_iges_read_options_init(occtl_io_iges_read_options_t* options);

/**
 * Initialises @p options to default values matching #OCCTL_IO_IGES_WRITE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_iges_write
 */
OCCTL_API void OCCTL_CALL occtl_io_iges_write_options_init(occtl_io_iges_write_options_t* options);

/**
 * Reads an IGES file and ingests it into a freshly-created graph.
 *
 * @param[in]  path        Borrows it.  NUL-terminated UTF-8 path to the file.  Must be non-NULL.
 * @param[out] out_graph   Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root    Borrows it.  Receives the NodeId of the root entity.
 * @param[in]  options     Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p path, @p out_graph, or @p out_root is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_IO_ERROR          File not found or unreadable.
 * @retval OCCTL_FORMAT_ERROR      File contents were not a valid IGES stream.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_iges_write, occtl_de_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_iges_read(const char*      path,
                                                       occtl_graph_t**  out_graph,
                                                       occtl_node_id_t* out_root,
                                                       const occtl_io_iges_read_options_t* options);

/**
 * Writes the topology rooted at @p root to an IGES file.
 *
 * @param[in] graph    Borrows it.  Must be non-NULL.
 * @param[in] root     Root node id to serialise.
 * @param[in] path     Borrows it.  NUL-terminated UTF-8 path; existing file is overwritten.
 * @param[in] options  Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p path is NULL.
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_IO_ERROR          Filesystem failure or unwritable file.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_iges_read, occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_iges_write(const occtl_graph_t*                 graph,
                      occtl_node_id_t                      root,
                      const char*                          path,
                      const occtl_io_iges_write_options_t* options);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_IO_IGES_H */
