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
 * @file occtl_io_obj.h
 * @brief OCCT-Light: Wavefront OBJ file I/O.
 *
 * Reads and writes Wavefront OBJ mesh files via OCCT's
 * @c DEOBJ_Provider / @c DEOBJ_ConfigurationNode.
 *
 * OBJ is primarily a polygon mesh exchange format.  Precise geometry may be
 * tessellated during export; callers that need deterministic triangulation
 * should run #occtl_mesh_generate before writing.
 *
 * UIDs are not preserved across the round-trip in v1.  Use
 * #occtl_uid_to_bytes / #occtl_uid_from_bytes for an out-of-band UID
 * handshake if persistent identity is required.
 */

#ifndef OCCTL_IO_OBJ_H
#define OCCTL_IO_OBJ_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Coordinate system used by OBJ import/export conversion.
 */
typedef enum occtl_io_obj_coordinate_system
{
  OCCTL_IO_OBJ_COORDINATE_SYSTEM_Y_UP            = 0, /**< Y-up coordinates. */
  OCCTL_IO_OBJ_COORDINATE_SYSTEM_Z_UP            = 1, /**< Z-up coordinates. */
  OCCTL_IO_OBJ_COORDINATE_SYSTEM_GLTF            = 2, /**< glTF-compatible coordinates. */
  OCCTL_IO_OBJ_COORDINATE_SYSTEM_RESERVED_FUTURE = 0x7fffffff
} occtl_io_obj_coordinate_system_t;

#define OCCTL_IO_OBJ_READ_OPTIONS_VERSION_1 1u
#define OCCTL_IO_OBJ_WRITE_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_io_obj_read.  Pass NULL for defaults.
 */
typedef struct occtl_io_obj_read_options
{
  uint32_t    struct_version;     /**< Must be #OCCTL_IO_OBJ_READ_OPTIONS_VERSION_1. */
  const void* p_next;             /**< Reserved; set to NULL. */
  double      file_length_unit_m; /**< File length unit in meters; default 1.0. */
  occtl_io_obj_coordinate_system_t
    system_coordinate_system; /**< Target system coordinates; default Z-up. */
  occtl_io_obj_coordinate_system_t
              file_coordinate_system; /**< Source file coordinates; default Y-up. */
  int32_t     single_precision;       /**< 1 to read vertex data as single precision. */
  int32_t     create_shapes;          /**< 1 to create per-group shapes; default 0. */
  int32_t     fill_incomplete;        /**< 1 to keep partially retrieved data on reader error. */
  int32_t     memory_limit_mib;       /**< Memory limit in MiB; -1 uses OCCT default. */
  const char* root_prefix;            /**< Borrows it.  Optional root-name prefix; may be NULL. */
} occtl_io_obj_read_options_t;

#define OCCTL_IO_OBJ_READ_OPTIONS_INIT                                                             \
  {OCCTL_IO_OBJ_READ_OPTIONS_VERSION_1,                                                            \
   NULL,                                                                                           \
   1.0,                                                                                            \
   OCCTL_IO_OBJ_COORDINATE_SYSTEM_Z_UP,                                                            \
   OCCTL_IO_OBJ_COORDINATE_SYSTEM_Y_UP,                                                            \
   0,                                                                                              \
   0,                                                                                              \
   1,                                                                                              \
   -1,                                                                                             \
   NULL}

/**
 * Options for #occtl_io_obj_write.  Pass NULL for defaults.
 */
typedef struct occtl_io_obj_write_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_IO_OBJ_WRITE_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  occtl_io_obj_coordinate_system_t
    system_coordinate_system; /**< Source system coordinates; default Z-up. */
  occtl_io_obj_coordinate_system_t
              file_coordinate_system; /**< Output file coordinates; default Y-up. */
  const char* comment;                /**< Borrows it.  Optional file comment; may be NULL. */
  const char* author;                 /**< Borrows it.  Optional file author; may be NULL. */
} occtl_io_obj_write_options_t;

#define OCCTL_IO_OBJ_WRITE_OPTIONS_INIT                                                            \
  {OCCTL_IO_OBJ_WRITE_OPTIONS_VERSION_1,                                                           \
   NULL,                                                                                           \
   OCCTL_IO_OBJ_COORDINATE_SYSTEM_Z_UP,                                                            \
   OCCTL_IO_OBJ_COORDINATE_SYSTEM_Y_UP,                                                            \
   NULL,                                                                                           \
   NULL}

/**
 * Initialises @p options to default values matching #OCCTL_IO_OBJ_READ_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_obj_read
 */
OCCTL_API void OCCTL_CALL occtl_io_obj_read_options_init(occtl_io_obj_read_options_t* options);

/**
 * Initialises @p options to default values matching #OCCTL_IO_OBJ_WRITE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_obj_write
 */
OCCTL_API void OCCTL_CALL occtl_io_obj_write_options_init(occtl_io_obj_write_options_t* options);

/**
 * Reads an OBJ file and ingests it into a freshly-created graph.
 *
 * @param[in]  path        Borrows it.  NUL-terminated UTF-8 path to the file.  Must be non-NULL.
 * @param[out] out_graph   Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root    Borrows it.  Receives the NodeId of the root entity.
 * @param[in]  options     Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p path, @p out_graph, or @p out_root is NULL; or @p options
 * has invalid fields (non-NULL @c p_next, non-finite/non-positive
 * @c file_length_unit_m, invalid 0/1 flags, negative @c memory_limit_mib
 * other than -1).
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_OUT_OF_RANGE      @p options has an unsupported coordinate-system value.
 * @retval OCCTL_IO_ERROR          File not found, unreadable, or blocked by OCCT reader limits.
 * @retval OCCTL_FORMAT_ERROR      File contents were not a valid OBJ stream.
 * @retval OCCTL_TOPOLOGY_INVALID  Imported shape could not be ingested into BRepGraph.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_obj_write, occtl_de_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_obj_read(const char*                        path,
                                                      occtl_graph_t**                    out_graph,
                                                      occtl_node_id_t*                   out_root,
                                                      const occtl_io_obj_read_options_t* options);

/**
 * Writes the topology rooted at @p root to an OBJ file.
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
 * @retval OCCTL_OUT_OF_RANGE      @p options has an unsupported coordinate-system value.
 * @retval OCCTL_IO_ERROR          Filesystem failure, unwritable file, or OCCT writer failure.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_obj_read, occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_obj_write(const occtl_graph_t*                graph,
                                                       occtl_node_id_t                     root,
                                                       const char*                         path,
                                                       const occtl_io_obj_write_options_t* options);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_IO_OBJ_H */
