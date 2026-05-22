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
 * @file occtl_io_step.h
 * @brief OCCT-Light: STEP file I/O (ISO 10303-21).
 *
 * Reads and writes STEP AP203 / AP214 / AP242 files via OCCT's
 * @c DESTEP_Provider / @c DESTEP_ConfigurationNode.  OCAF is linked
 * privately and never reaches the public surface.
 *
 * UIDs are *not* preserved across the round-trip — the STEP file
 * format has its own entity-id metadata.  Use #occtl_uid_to_bytes /
 * #occtl_uid_from_bytes for an out-of-band UID handshake if persistent
 * identity is required.
 */

#ifndef OCCTL_IO_STEP_H
#define OCCTL_IO_STEP_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Length unit for STEP file I/O.
 */
typedef enum occtl_io_step_length_unit
{
  OCCTL_IO_STEP_UNIT_MM              = 0, /**< Millimetres (default). */
  OCCTL_IO_STEP_UNIT_M               = 1, /**< Metres. */
  OCCTL_IO_STEP_UNIT_INCH            = 2, /**< Inches. */
  OCCTL_IO_STEP_UNIT_RESERVED_FUTURE = 0x7fffffff
} occtl_io_step_length_unit_t;

/**
 * STEP schema version for write.
 */
typedef enum occtl_io_step_schema
{
  OCCTL_IO_STEP_SCHEMA_AP203 = 0, /**< AP203 (configuration-controlled 3D design). */
  OCCTL_IO_STEP_SCHEMA_AP214 = 1, /**< AP214 (core data for automotive). */
  OCCTL_IO_STEP_SCHEMA_AP242 = 2, /**< AP242 (managed model-based 3D engineering, default). */
  OCCTL_IO_STEP_SCHEMA_RESERVED_FUTURE = 0x7fffffff
} occtl_io_step_schema_t;

#define OCCTL_IO_STEP_READ_OPTIONS_VERSION_1 1u
#define OCCTL_IO_STEP_WRITE_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_io_step_read.  Pass NULL for defaults.
 */
typedef struct occtl_io_step_read_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_IO_STEP_READ_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  int32_t     read_color;     /**< 1 to import colour from file (default). */
  int32_t     read_name;      /**< 1 to import names from file (default). */
  int32_t     read_layer;     /**< 1 to import layers from file (default). */
} occtl_io_step_read_options_t;

#define OCCTL_IO_STEP_READ_OPTIONS_INIT {OCCTL_IO_STEP_READ_OPTIONS_VERSION_1, NULL, 1, 1, 1}

/**
 * Options for #occtl_io_step_write.  Pass NULL for defaults.
 */
typedef struct occtl_io_step_write_options
{
  uint32_t    struct_version;         /**< Must be #OCCTL_IO_STEP_WRITE_OPTIONS_VERSION_1. */
  const void* p_next;                 /**< Reserved; set to NULL. */
  occtl_io_step_length_unit_t unit;   /**< Output length unit; default mm. */
  occtl_io_step_schema_t      schema; /**< STEP schema; default AP242. */
  int32_t                     write_surface_curves; /**< 1 to write parametric curves (default). */
  int32_t write_tessellated; /**< 1 to include triangulated geometry (default). */
} occtl_io_step_write_options_t;

#define OCCTL_IO_STEP_WRITE_OPTIONS_INIT                                                           \
  {OCCTL_IO_STEP_WRITE_OPTIONS_VERSION_1,                                                          \
   NULL,                                                                                           \
   OCCTL_IO_STEP_UNIT_MM,                                                                          \
   OCCTL_IO_STEP_SCHEMA_AP242,                                                                     \
   1,                                                                                              \
   1}

/**
 * Initialises @p options to default values matching #OCCTL_IO_STEP_READ_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_step_read
 */
OCCTL_API void OCCTL_CALL occtl_io_step_read_options_init(occtl_io_step_read_options_t* options);

/**
 * Initialises @p options to default values matching #OCCTL_IO_STEP_WRITE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_step_write
 */
OCCTL_API void OCCTL_CALL occtl_io_step_write_options_init(occtl_io_step_write_options_t* options);

/**
 * Reads a STEP file and ingests it into a freshly-created graph.
 *
 * @param[in]  path        Borrows it.  NUL-terminated UTF-8 path to the file.  Must be non-NULL.
 * @param[out] out_graph   Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root    Borrows it.  Receives the NodeId of the root entity.
 * @param[in]  options     Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p path, @p out_graph, or @p out_root is NULL; or @p options has
 *                                 invalid fields (non-NULL @c p_next, non-0/1 flags).
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_IO_ERROR          File not found or unreadable.
 * @retval OCCTL_FORMAT_ERROR      File contents were not a valid STEP stream.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_step_write, occtl_de_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_step_read(const char*      path,
                                                       occtl_graph_t**  out_graph,
                                                       occtl_node_id_t* out_root,
                                                       const occtl_io_step_read_options_t* options);

/**
 * Reads a STEP payload from caller-owned memory into a freshly-created graph.
 *
 * Caller owns the returned graph and must release it via #occtl_graph_free.
 *
 * @param[in]  data       Borrows it.  Readable byte buffer containing STEP data.
 *                        Must be non-NULL when @p size is non-zero.
 * @param[in]  size       Number of bytes available at @p data.  Must be > 0.
 * @param[out] out_graph  Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root   Borrows it.  Receives the NodeId of the root entity.
 * @param[in]  options    Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p data, @p out_graph, or @p out_root is NULL,
 *                                 or @p size is zero; or @p options has invalid fields
 *                                 (non-NULL @c p_next, non-0/1 flags).
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_IO_ERROR          Temporary stream failure.
 * @retval OCCTL_FORMAT_ERROR      Payload contents were not a valid STEP stream.
 * @retval OCCTL_TOPOLOGY_INVALID  Payload decoded but could not be ingested into BRepGraph.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_step_write_memory, occtl_io_step_read
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_step_read_memory(const uint8_t*                      data,
                            size_t                              size,
                            occtl_graph_t**                     out_graph,
                            occtl_node_id_t*                    out_root,
                            const occtl_io_step_read_options_t* options);

/**
 * Writes the topology rooted at @p root to a STEP file.
 *
 * @param[in] graph    Borrows it.  Must be non-NULL.
 * @param[in] root     Root node id to serialise.
 * @param[in] path     Borrows it.  NUL-terminated UTF-8 path; existing file is overwritten.
 * @param[in] options  Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p path is NULL; or @p options has invalid fields
 *                                 (non-NULL @c p_next, non-0/1 flags).
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_OUT_OF_RANGE      @p options has unsupported unit or schema enum.
 * @retval OCCTL_IO_ERROR          Filesystem failure or unwritable file.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_step_read, occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_step_write(const occtl_graph_t*                 graph,
                      occtl_node_id_t                      root,
                      const char*                          path,
                      const occtl_io_step_write_options_t* options);

/**
 * Writes the topology rooted at @p root to caller-owned memory.
 *
 * Two-call buffer pattern: call with @p out_data == NULL and @p capacity == 0
 * to learn the required byte count in @p out_size, allocate a buffer, then
 * call again with @p capacity at least @p out_size.
 *
 * @param[in]  graph     Borrows it.  Must be non-NULL.
 * @param[in]  root      Root node id to serialise.
 * @param[in]  options   Borrows it.  May be NULL for defaults.
 * @param[out] out_data  Borrows it.  Writable output buffer, or NULL for sizing.
 * @param[in]  capacity  Size of @p out_data in bytes.
 * @param[out] out_size  Borrows it.  Receives the required byte count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_size is NULL; or @p options has invalid
 *                                 fields (non-NULL @c p_next, non-0/1 flags).
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_OUT_OF_RANGE      @p options has unsupported unit or schema enum.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_data is non-NULL and @p capacity is
 *                                 smaller than @p out_size.
 * @retval OCCTL_IO_ERROR          OCCT failed to serialise the shape.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_step_read_memory, occtl_io_step_write
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_step_write_memory(const occtl_graph_t*                 graph,
                             occtl_node_id_t                      root,
                             const occtl_io_step_write_options_t* options,
                             uint8_t*                             out_data,
                             size_t                               capacity,
                             size_t*                              out_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_IO_STEP_H */
