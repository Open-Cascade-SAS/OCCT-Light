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
 * @file occtl_io_stl.h
 * @brief OCCT-Light: STL file I/O (stereolithography mesh format).
 *
 * Reads and writes STL files (binary and ASCII) via OCCT's
 * @c DESTL_Provider / @c DESTL_ConfigurationNode.
 *
 * STL is mesh-only (no B-spline preservation) — the caller should
 * tessellate with #occtl_mesh_build beforehand if the source is a
 * precise geometry.
 *
 * UIDs are *not* preserved across the round-trip — the STL file
 * format has no identity slot.  Use #occtl_uid_to_bytes /
 * #occtl_uid_from_bytes for an out-of-band UID handshake if persistent
 * identity is required.
 */

#ifndef OCCTL_IO_STL_H
#define OCCTL_IO_STL_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_IO_STL_WRITE_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_io_stl_write and #occtl_io_stl_write_memory.
 * Pass NULL for defaults (binary output).
 */
typedef struct occtl_io_stl_write_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_IO_STL_WRITE_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  int32_t     ascii_mode;     /**< 0 = binary (default), 1 = ASCII. */
} occtl_io_stl_write_options_t;

#define OCCTL_IO_STL_WRITE_OPTIONS_INIT {OCCTL_IO_STL_WRITE_OPTIONS_VERSION_1, NULL, 0}

/**
 * Initialises @p options to default values matching #OCCTL_IO_STL_WRITE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_stl_write, occtl_io_stl_write_memory
 */
OCCTL_API void OCCTL_CALL occtl_io_stl_write_options_init(occtl_io_stl_write_options_t* options);

/**
 * Reads an STL file and ingests it into a freshly-created graph.
 *
 * @param[in]  path        Borrows it.  NUL-terminated UTF-8 path to the file.  Must be non-NULL.
 * @param[out] out_graph   Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root    Borrows it.  Receives the NodeId of the root entity.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p path, @p out_graph, or @p out_root is NULL.
 * @retval OCCTL_IO_ERROR          File not found or unreadable.
 * @retval OCCTL_FORMAT_ERROR      File contents were not a valid STL stream.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_stl_write, occtl_de_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_stl_read(const char*      path,
                                                      occtl_graph_t**  out_graph,
                                                      occtl_node_id_t* out_root);

/**
 * Reads an STL payload from caller-owned memory into a freshly-created graph.
 *
 * @param[in]  data       Borrows it.  Readable byte buffer containing STL data.  Must be non-NULL
 * when
 *                        @p size is non-zero.
 * @param[in]  size       Number of bytes available at @p data.  Must be > 0.
 * @param[out] out_graph  Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root   Borrows it.  Receives the NodeId of the root entity.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p data, @p out_graph, or @p out_root is NULL, or @p size is
 * zero.
 * @retval OCCTL_FORMAT_ERROR      Payload contents were not a valid STL stream.
 * @retval OCCTL_TOPOLOGY_INVALID  Payload decoded but could not be ingested into BRepGraph.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_stl_write_memory, occtl_io_stl_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_stl_read_memory(const uint8_t*   data,
                                                             size_t           size,
                                                             occtl_graph_t**  out_graph,
                                                             occtl_node_id_t* out_root);

/**
 * Writes the topology rooted at @p root to an STL file.
 *
 * Format (binary/ASCII) is taken from @p options; pass NULL for
 * default binary output.
 *
 * @param[in] graph    Borrows it.  Must be non-NULL.
 * @param[in] root     Root node id to serialise.
 * @param[in] path     Borrows it.  NUL-terminated UTF-8 path; existing file is overwritten.
 * @param[in] options  Borrows it.  May be NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p path is NULL; or @p options has invalid fields
 * (non-NULL @c p_next, @c ascii_mode not in {0,1}).
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_IO_ERROR          Filesystem failure or unwritable file.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_stl_read, occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_stl_write(const occtl_graph_t*                graph,
                                                       occtl_node_id_t                     root,
                                                       const char*                         path,
                                                       const occtl_io_stl_write_options_t* options);

/**
 * Writes the topology rooted at @p root to caller-owned memory.
 *
 * Use the standard two-call buffer pattern: call once with @p out_data NULL
 * and @p capacity zero to receive the required byte count in @p out_size, then
 * call again with a buffer of at least that size.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  root      Root node id to serialise.
 * @param[in]  options   Borrows it.  May be NULL.
 * @param[out] out_data  Borrows it.  Writable output buffer, or NULL for sizing.
 * @param[in]  capacity  Size of @p out_data in bytes.
 * @param[out] out_size  Borrows it.  Receives the required byte count.
 *
 * @retval OCCTL_OK                On success.  Sizing calls also return OK.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_size is NULL; or @p options has invalid
 * fields (non-NULL @c p_next, @c ascii_mode not in {0,1}).
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_data is non-NULL and @p capacity is smaller than @p
 * out_size.
 * @retval OCCTL_IO_ERROR          OCCT failed to serialise the shape.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_stl_read_memory, occtl_io_stl_write
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_stl_write_memory(const occtl_graph_t*                graph,
                            occtl_node_id_t                     root,
                            const occtl_io_stl_write_options_t* options,
                            uint8_t*                            out_data,
                            size_t                              capacity,
                            size_t*                             out_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_IO_STL_H */
