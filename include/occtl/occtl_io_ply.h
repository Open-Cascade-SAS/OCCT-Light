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
 * @file occtl_io_ply.h
 * @brief OCCT-Light: PLY file export.
 *
 * Writes Stanford PLY polygon mesh files via OCCT's
 * @c DEPLY_Provider / @c DEPLY_ConfigurationNode.
 *
 * OCCT 8.0.0 does not support PLY import through @c DEPLY_Provider, so this
 * module is intentionally write-only.  Callers that need deterministic mesh
 * output should run #occtl_mesh_generate before writing.
 */

#ifndef OCCTL_IO_PLY_H
#define OCCTL_IO_PLY_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Coordinate system used by PLY export conversion.
 */
typedef enum occtl_io_ply_coordinate_system
{
  OCCTL_IO_PLY_COORDINATE_SYSTEM_Y_UP            = 0, /**< Y-up coordinates. */
  OCCTL_IO_PLY_COORDINATE_SYSTEM_Z_UP            = 1, /**< Z-up coordinates. */
  OCCTL_IO_PLY_COORDINATE_SYSTEM_GLTF            = 2, /**< glTF-compatible coordinates. */
  OCCTL_IO_PLY_COORDINATE_SYSTEM_RESERVED_FUTURE = 0x7fffffff
} occtl_io_ply_coordinate_system_t;

#define OCCTL_IO_PLY_WRITE_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_io_ply_write.  Pass NULL for defaults.
 */
typedef struct occtl_io_ply_write_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_IO_PLY_WRITE_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  occtl_io_ply_coordinate_system_t
    system_coordinate_system; /**< Source system coordinates; default Z-up. */
  occtl_io_ply_coordinate_system_t
              file_coordinate_system; /**< Output file coordinates; default Y-up. */
  int32_t     write_normals;          /**< 1 to export normals. */
  int32_t     write_colors;           /**< 1 to export color attributes when available. */
  int32_t     write_texcoords;        /**< 1 to export UV attributes when available. */
  int32_t     write_part_id;          /**< 1 to export part ids. */
  int32_t     write_face_id; /**< 1 to export face ids; cannot be combined with write_part_id. */
  const char* comment;       /**< Borrows it.  Optional file comment; may be NULL. */
  const char* author;        /**< Borrows it.  Optional file author; may be NULL. */
} occtl_io_ply_write_options_t;

#define OCCTL_IO_PLY_WRITE_OPTIONS_INIT                                                            \
  {OCCTL_IO_PLY_WRITE_OPTIONS_VERSION_1,                                                           \
   NULL,                                                                                           \
   OCCTL_IO_PLY_COORDINATE_SYSTEM_Z_UP,                                                            \
   OCCTL_IO_PLY_COORDINATE_SYSTEM_Y_UP,                                                            \
   1,                                                                                              \
   1,                                                                                              \
   0,                                                                                              \
   1,                                                                                              \
   0,                                                                                              \
   NULL,                                                                                           \
   NULL}

/**
 * Initialises @p options to default values matching #OCCTL_IO_PLY_WRITE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_ply_write
 */
OCCTL_API void OCCTL_CALL occtl_io_ply_write_options_init(occtl_io_ply_write_options_t* options);

/**
 * Writes the topology rooted at @p root to a PLY file.
 *
 * @param[in] graph    Borrows it.  Must be non-NULL.
 * @param[in] root     Root node id to serialise.
 * @param[in] path     Borrows it.  NUL-terminated UTF-8 path; existing file is overwritten.
 * @param[in] options  Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p path is NULL; @p options has invalid fields
 * (non-NULL @c p_next, non-0/1 flags); or mutually exclusive options are enabled.
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_OUT_OF_RANGE      @p options has an unsupported coordinate-system value.
 * @retval OCCTL_IO_ERROR          Filesystem failure, unwritable file, or OCCT writer failure.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_ply_write(const occtl_graph_t*                graph,
                                                       occtl_node_id_t                     root,
                                                       const char*                         path,
                                                       const occtl_io_ply_write_options_t* options);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_IO_PLY_H */
