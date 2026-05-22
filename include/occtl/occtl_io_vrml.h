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
 * @file occtl_io_vrml.h
 * @brief OCCT-Light: VRML file I/O.
 *
 * Reads and writes VRML 1.0 / 2.0 files via OCCT's
 * @c DEVRML_Provider / @c DEVRML_ConfigurationNode.
 *
 * VRML is an older scene and mesh exchange format.  Precise geometry may be
 * tessellated during export; callers that need deterministic triangulation
 * should run #occtl_mesh_generate before writing.
 */

#ifndef OCCTL_IO_VRML_H
#define OCCTL_IO_VRML_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Coordinate system used by VRML import/export conversion.
 */
typedef enum occtl_io_vrml_coordinate_system
{
  OCCTL_IO_VRML_COORDINATE_SYSTEM_Y_UP            = 0, /**< Y-up coordinates. */
  OCCTL_IO_VRML_COORDINATE_SYSTEM_Z_UP            = 1, /**< Z-up coordinates. */
  OCCTL_IO_VRML_COORDINATE_SYSTEM_GLTF            = 2, /**< glTF-compatible coordinates. */
  OCCTL_IO_VRML_COORDINATE_SYSTEM_RESERVED_FUTURE = 0x7fffffff
} occtl_io_vrml_coordinate_system_t;

/**
 * VRML writer version.
 */
typedef enum occtl_io_vrml_writer_version
{
  OCCTL_IO_VRML_WRITER_VERSION_1               = 1, /**< VRML 1.0 output. */
  OCCTL_IO_VRML_WRITER_VERSION_2               = 2, /**< VRML 2.0 output. */
  OCCTL_IO_VRML_WRITER_VERSION_RESERVED_FUTURE = 0x7fffffff
} occtl_io_vrml_writer_version_t;

/**
 * VRML output representation.
 */
typedef enum occtl_io_vrml_representation
{
  OCCTL_IO_VRML_REPRESENTATION_SHADED          = 0, /**< Shaded representation. */
  OCCTL_IO_VRML_REPRESENTATION_WIREFRAME       = 1, /**< Wireframe representation. */
  OCCTL_IO_VRML_REPRESENTATION_BOTH            = 2, /**< Shaded and wireframe representation. */
  OCCTL_IO_VRML_REPRESENTATION_RESERVED_FUTURE = 0x7fffffff
} occtl_io_vrml_representation_t;

#define OCCTL_IO_VRML_READ_OPTIONS_VERSION_1 1u
#define OCCTL_IO_VRML_WRITE_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_io_vrml_read.  Pass NULL for defaults.
 */
typedef struct occtl_io_vrml_read_options
{
  uint32_t    struct_version;     /**< Must be #OCCTL_IO_VRML_READ_OPTIONS_VERSION_1. */
  const void* p_next;             /**< Reserved; set to NULL. */
  double      file_length_unit_m; /**< File length unit in meters; default 1.0. */
  occtl_io_vrml_coordinate_system_t
    system_coordinate_system; /**< Target system coordinates; default Z-up. */
  occtl_io_vrml_coordinate_system_t
          file_coordinate_system; /**< Source file coordinates; default Y-up. */
  int32_t fill_incomplete;        /**< 1 to keep partially retrieved data on reader error. */
} occtl_io_vrml_read_options_t;

#define OCCTL_IO_VRML_READ_OPTIONS_INIT                                                            \
  {OCCTL_IO_VRML_READ_OPTIONS_VERSION_1,                                                           \
   NULL,                                                                                           \
   1.0,                                                                                            \
   OCCTL_IO_VRML_COORDINATE_SYSTEM_Z_UP,                                                           \
   OCCTL_IO_VRML_COORDINATE_SYSTEM_Y_UP,                                                           \
   1}

/**
 * Options for #occtl_io_vrml_write.  Pass NULL for defaults.
 */
typedef struct occtl_io_vrml_write_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_IO_VRML_WRITE_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  occtl_io_vrml_writer_version_t writer_version; /**< VRML writer version; default 2. */
  occtl_io_vrml_representation_t representation; /**< Output representation; default wireframe. */
} occtl_io_vrml_write_options_t;

#define OCCTL_IO_VRML_WRITE_OPTIONS_INIT                                                           \
  {OCCTL_IO_VRML_WRITE_OPTIONS_VERSION_1,                                                          \
   NULL,                                                                                           \
   OCCTL_IO_VRML_WRITER_VERSION_2,                                                                 \
   OCCTL_IO_VRML_REPRESENTATION_WIREFRAME}

/**
 * Initialises @p options to default values matching #OCCTL_IO_VRML_READ_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_vrml_read
 */
OCCTL_API void OCCTL_CALL occtl_io_vrml_read_options_init(occtl_io_vrml_read_options_t* options);

/**
 * Initialises @p options to default values matching #OCCTL_IO_VRML_WRITE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_vrml_write
 */
OCCTL_API void OCCTL_CALL occtl_io_vrml_write_options_init(occtl_io_vrml_write_options_t* options);

/**
 * Reads a VRML file and ingests it into a freshly-created graph.
 *
 * @param[in]  path        Borrows it.  NUL-terminated UTF-8 path to the file.  Must be non-NULL.
 * @param[out] out_graph   Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root    Borrows it.  Receives the NodeId of the root entity.
 * @param[in]  options     Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p path, @p out_graph, or @p out_root is NULL; or @p options has
 *                                 invalid fields (non-NULL @c p_next, non-finite/non-positive
 *                                 @c file_length_unit_m, non-0/1 @c fill_incomplete).
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_OUT_OF_RANGE      @p options has an unsupported coordinate-system value.
 * @retval OCCTL_IO_ERROR          File not found, unreadable, or blocked by OCCT reader limits.
 * @retval OCCTL_FORMAT_ERROR      File contents were not a valid VRML stream.
 * @retval OCCTL_TOPOLOGY_INVALID  Imported shape could not be ingested into BRepGraph.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_vrml_write, occtl_de_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_vrml_read(const char*      path,
                                                       occtl_graph_t**  out_graph,
                                                       occtl_node_id_t* out_root,
                                                       const occtl_io_vrml_read_options_t* options);

/**
 * Writes the topology rooted at @p root to a VRML file.
 *
 * @param[in] graph    Borrows it.  Must be non-NULL.
 * @param[in] root     Root node id to serialise.
 * @param[in] path     Borrows it.  NUL-terminated UTF-8 path; existing file is overwritten.
 * @param[in] options  Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p path is NULL; or @p options->p_next is non-NULL.
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_OUT_OF_RANGE      @p options has an unsupported writer_version or representation.
 * @retval OCCTL_IO_ERROR          Filesystem failure, unwritable file, or OCCT writer failure.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_vrml_read, occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_vrml_write(const occtl_graph_t*                 graph,
                      occtl_node_id_t                      root,
                      const char*                          path,
                      const occtl_io_vrml_write_options_t* options);

/**
 * Reads a VRML file from a caller-owned memory buffer.
 *
 * Delegates to the OCCT VRML provider stream reader; OCCT-Light does not
 * create a filesystem path for this call.
 *
 * @param[in]  data        Borrows it.  Byte buffer containing VRML data.  Must be non-NULL.
 * @param[in]  size        Number of bytes available at @p data.  Must be > 0.
 * @param[out] out_graph   Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root    Borrows it.  Receives the NodeId of the root entity.
 * @param[in]  options     Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p data, @p out_graph, or @p out_root is NULL, or @p size == 0;
 *                                 or @p options has invalid fields (non-NULL @c p_next,
 *                                 non-finite/non-positive @c file_length_unit_m,
 *                                 non-0/1 @c fill_incomplete).
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_IO_ERROR          OCCT reader failed.
 * @retval OCCTL_FORMAT_ERROR      Buffer contents were not a valid VRML stream.
 * @retval OCCTL_TOPOLOGY_INVALID  Imported shape could not be ingested into BRepGraph.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_vrml_read, occtl_de_read_memory
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_vrml_read_memory(const uint8_t*                      data,
                            size_t                              size,
                            occtl_graph_t**                     out_graph,
                            occtl_node_id_t*                    out_root,
                            const occtl_io_vrml_read_options_t* options);

/**
 * Writes the topology rooted at @p root to a caller-owned memory buffer
 * in VRML format.
 *
 * Two-call buffer pattern: pass @p out_data == NULL with @p capacity == 0
 * to learn the required byte count in @p out_size, then call again with a
 * buffer of at least that size.
 *
 * @param[in]  graph      Borrows it.  Must be non-NULL.
 * @param[in]  root       Root node id to serialise.
 * @param[in]  options    Borrows it.  May be NULL for defaults.
 * @param[out] out_data   Borrows it.  Writable output buffer, or NULL for sizing.
 * @param[in]  capacity   Size of @p out_data in bytes.
 * @param[out] out_size   Borrows it.  Receives the required byte count.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p out_size is NULL; or @p options->p_next is
 *                                 non-NULL.
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_BUFFER_TOO_SMALL  @p out_data is non-NULL and @p capacity is too small.
 * @retval OCCTL_IO_ERROR          OCCT writer failure.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_vrml_read_memory, occtl_de_write_memory
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_vrml_write_memory(const occtl_graph_t*                 graph,
                             occtl_node_id_t                      root,
                             const occtl_io_vrml_write_options_t* options,
                             uint8_t*                             out_data,
                             size_t                               capacity,
                             size_t*                              out_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_IO_VRML_H */
