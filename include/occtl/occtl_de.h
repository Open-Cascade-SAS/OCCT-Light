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
 * @file occtl_de.h
 * @brief OCCT-Light: unified data-exchange dispatch.
 *
 * Mirrors OCCT's @c DE_Wrapper.  Reads / writes a file by routing based
 * on path extension to whichever @c io_* module is enabled at link
 * time.  When no @c io_* module is enabled this module compiles but
 * every operation returns #OCCTL_UNSUPPORTED.
 *
 * Format ids are stable lowercase tokens: @c "brep", @c "step",
 * @c "iges", @c "stl", @c "obj", and @c "gltf" when the matching
 * @c io_* modules are enabled.
 */

#ifndef OCCTL_DE_H
#define OCCTL_DE_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_DE_FORMAT_INFO_VERSION_1 1u

/**
 * Borrowed description of one supported data-exchange format.
 *
 * String fields are library-owned and remain valid for the process lifetime.
 * Producers set @c struct_version to #OCCTL_DE_FORMAT_INFO_VERSION_1 and
 * @c p_next to NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_format_info_at, occtl_de_format_info_by_id
 */
typedef struct occtl_de_format_info
{
  uint32_t    struct_version;  /**< Set to #OCCTL_DE_FORMAT_INFO_VERSION_1 by producers. */
  const void* p_next;          /**< Reserved; set to NULL. */
  const char* id;              /**< Stable lowercase format id. */
  const char* label;           /**< Human-readable format label. */
  size_t      extension_count; /**< Number of extensions reported by #occtl_de_format_extensions. */
  int32_t     can_read_file;   /**< 1 when #occtl_de_read can dispatch this format. */
  int32_t     can_write_file;  /**< 1 when #occtl_de_write can dispatch this format. */
  int32_t     can_read_memory; /**< 1 when #occtl_de_read_memory supports this format. */
  int32_t     can_write_memory; /**< 1 when #occtl_de_write_memory supports this format. */
} occtl_de_format_info_t;

/**
 * Static initializer for #occtl_de_format_info_t.
 */
#define OCCTL_DE_FORMAT_INFO_INIT {OCCTL_DE_FORMAT_INFO_VERSION_1, NULL, NULL, NULL, 0, 0, 0, 0, 0}

/**
 * Runtime initialiser for #occtl_de_format_info_t.
 *
 * Sets all fields to #OCCTL_DE_FORMAT_INFO_INIT.
 *
 * @param[out] info Borrows it.  NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_format_info_at, occtl_de_format_info_by_id
 */
OCCTL_API void OCCTL_CALL occtl_de_format_info_init(occtl_de_format_info_t* info);

/**
 * Reads a file and ingests it into a freshly-created graph.
 *
 * Format is dispatched by extension.  When the format is recognised
 * but the matching @c io_* module is not linked, returns
 * #OCCTL_UNSUPPORTED.
 *
 * @param[in]  path       Borrows it.  NUL-terminated UTF-8 path.
 * @param[out] out_graph  Owns it.  Receives a new graph on success.
 * @param[out] out_root   Borrows it.  Receives the NodeId of the root.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL.
 * @retval OCCTL_UNSUPPORTED       Format unknown or matching module not linked.
 * @retval OCCTL_IO_ERROR          Filesystem failure.
 * @retval OCCTL_FORMAT_ERROR      File contents malformed.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_read(const char*      path,
                                                  occtl_graph_t**  out_graph,
                                                  occtl_node_id_t* out_root);

/**
 * Reads a memory payload and ingests it into a freshly-created graph.
 *
 * Format is selected by explicit stable format id instead of a path
 * extension.  The dispatch still uses OCCT's data-exchange providers
 * internally, so format support matches #occtl_de_format_ids.
 *
 * @param[in]  format_id  Borrows it.  Stable lowercase format id.
 * @param[in]  data       Borrows it.  Readable byte buffer containing
 *                        provider-native data for @p format_id.
 * @param[in]  size       Number of bytes available at @p data. Must be > 0.
 * @param[out] out_graph  Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root   Borrows it.  Receives the NodeId of the root entity.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p format_id, @p data, @p out_graph, or
 *                                 @p out_root is NULL, or @p size is zero.
 * @retval OCCTL_UNSUPPORTED       Format unknown or not readable in this build.
 * @retval OCCTL_IO_ERROR          Temporary stream or provider failure.
 * @retval OCCTL_FORMAT_ERROR      Payload contents malformed.
 * @retval OCCTL_TOPOLOGY_INVALID  Payload decoded but could not be ingested into BRepGraph.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_write_memory, occtl_de_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_read_memory(const char*      format_id,
                                                         const uint8_t*   data,
                                                         size_t           size,
                                                         occtl_graph_t**  out_graph,
                                                         occtl_node_id_t* out_root);

/**
 * Writes the topology rooted at @p root to a file, dispatched by extension.
 *
 * @param[in] graph  Borrows it.  Must be non-NULL.
 * @param[in] root   Root node id.
 * @param[in] path   Borrows it.  NUL-terminated UTF-8 path; overwritten if exists.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p path is NULL.
 * @retval OCCTL_UNSUPPORTED       Format unknown or matching module not linked.
 * @retval OCCTL_NOT_FOUND         @p root is invalid or removed.
 * @retval OCCTL_IO_ERROR          Filesystem failure.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_de_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_write(const occtl_graph_t* graph,
                                                   occtl_node_id_t      root,
                                                   const char*          path);

/**
 * Writes the topology rooted at @p root to caller-owned memory.
 *
 * Format is selected by explicit stable format id.  Two-call buffer
 * pattern: call with @p out_data == NULL and @p capacity == 0 to learn
 * the required byte count in @p out_size, allocate a buffer, then call
 * again with @p capacity at least @p out_size.
 *
 * @param[in]  graph      Borrows it.  Must be non-NULL.
 * @param[in]  root       Root node id to serialise.
 * @param[in]  format_id  Borrows it.  Stable lowercase format id.
 * @param[out] out_data   Borrows it.  Writable output buffer, or NULL for sizing.
 * @param[in]  capacity   Size of @p out_data in bytes.
 * @param[out] out_size   Borrows it.  Receives the required byte count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph, @p format_id, or @p out_size is NULL.
 * @retval OCCTL_UNSUPPORTED       Format unknown or not writable in this build.
 * @retval OCCTL_NOT_FOUND         @p root is invalid or removed.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_data is non-NULL and @p capacity is
 *                                 smaller than @p out_size.
 * @retval OCCTL_IO_ERROR          Temporary stream or provider failure.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_de_read_memory, occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_write_memory(const occtl_graph_t* graph,
                                                          occtl_node_id_t      root,
                                                          const char*          format_id,
                                                          uint8_t*             out_data,
                                                          size_t               capacity,
                                                          size_t*              out_size);

/**
 * Enumerates the stable lowercase format ids the build supports.
 *
 * Two-call buffer: pass @p out_format_ids as NULL with @p cap=0 to
 * size.  The returned strings are library-owned and remain valid for
 * the process lifetime.
 *
 * @param[out] out_format_ids  Borrows it.  Length @p cap; may be NULL.
 * @param[in]  cap             Capacity of @p out_format_ids.
 * @param[out] out_count       Borrows it.  Receives the supported-format count.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_count is NULL.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p cap < @p out_count and out_format_ids non-NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_format_id_from_path
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_ids(const char** out_format_ids,
                                                        size_t       cap,
                                                        size_t*      out_count);

/**
 * Returns the number of supported format descriptors.
 *
 * @param[out] out_count Borrows it.  Receives the supported-format count.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_count is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_format_info_at
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_count(size_t* out_count);

/**
 * Returns metadata for the @p index-th supported format.
 *
 * @param[in]  index    Zero-based format index.
 * @param[out] out_info Borrows it.  Receives borrowed static strings and
 *                      writes #OCCTL_DE_FORMAT_INFO_VERSION_1 to
 *                      @c out_info->struct_version.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p out_info is NULL.
 * @retval OCCTL_OUT_OF_RANGE      @p index is not a supported format index.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_format_count, occtl_de_format_info_by_id
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_info_at(size_t                  index,
                                                            occtl_de_format_info_t* out_info);

/**
 * Returns metadata for a supported format id.
 *
 * @param[in]  format_id Borrows it.  Stable lowercase format id.
 * @param[out] out_info  Borrows it.  Receives borrowed static strings and
 *                       writes #OCCTL_DE_FORMAT_INFO_VERSION_1 to
 *                       @c out_info->struct_version.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p format_id or @p out_info is NULL.
 * @retval OCCTL_NOT_FOUND         @p format_id is not supported by this build.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_format_ids
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_info_by_id(const char*             format_id,
                                                               occtl_de_format_info_t* out_info);

/**
 * Enumerates the lowercase extensions for a supported format id.
 *
 * Extensions include the leading dot.  Two-call buffer: pass
 * @p out_extensions as NULL with @p cap=0 to size.  Returned strings are
 * library-owned and remain valid for the process lifetime.
 *
 * @param[in]  format_id      Borrows it.  Stable lowercase format id.
 * @param[out] out_extensions Borrows it.  Length @p cap; may be NULL.
 * @param[in]  cap            Capacity of @p out_extensions.
 * @param[out] out_count      Borrows it.  Receives the extension count.
 *
 * @retval OCCTL_OK                Success.
 * @retval OCCTL_INVALID_ARGUMENT  @p format_id or @p out_count is NULL.
 * @retval OCCTL_NOT_FOUND         @p format_id is not supported by this build.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p cap < @p out_count and @p out_extensions is non-NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_format_info_by_id
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_extensions(const char*  format_id,
                                                               const char** out_extensions,
                                                               size_t       cap,
                                                               size_t*      out_count);

/**
 * Returns the format id for @p path.
 *
 * The returned string is library-owned (one of the static tokens
 * enumerated by #occtl_de_format_ids) and never needs freeing.
 *
 * @param[in]  path           Borrows it.  NUL-terminated UTF-8 path.  Must be non-NULL.
 * @param[out] out_format_id  Borrows it (caller-allocated slot).  Receives a borrowed format id,
 *                            or NULL when no extension matches.  Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.  @p out_format_id may receive NULL.
 * @retval OCCTL_INVALID_ARGUMENT  @p path or @p out_format_id is NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_de_format_ids
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_id_from_path(const char*  path,
                                                                 const char** out_format_id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_DE_H */
