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
 * @file occtl_io_gltf.h
 * @brief OCCT-Light: glTF 2.0 / GLB file I/O.
 *
 * Reads and writes glTF 2.0 JSON streams (`.gltf`) and binary GLB streams
 * (`.glb`) via OCCT's @c DEGLTF_Provider / @c DEGLTF_ConfigurationNode.
 *
 * glTF is primarily a mesh exchange format.  Precise geometry may be
 * tessellated during export; callers that need deterministic triangulation
 * should run #occtl_mesh_generate before writing.
 *
 * UIDs are not preserved across the round-trip in v1.  Use
 * #occtl_uid_to_bytes / #occtl_uid_from_bytes for an out-of-band UID
 * handshake if persistent identity is required.
 */

#ifndef OCCTL_IO_GLTF_H
#define OCCTL_IO_GLTF_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Transformation encoding used when writing glTF node transforms.
 */
typedef enum occtl_io_gltf_transform_format
{
  OCCTL_IO_GLTF_TRANSFORM_COMPACT = 0, /**< Let OCCT choose the most compact representation. */
  OCCTL_IO_GLTF_TRANSFORM_MAT4    = 1, /**< Always write a 4x4 matrix. */
  OCCTL_IO_GLTF_TRANSFORM_TRS     = 2, /**< Write translation, rotation quaternion, and scale. */
  OCCTL_IO_GLTF_TRANSFORM_RESERVED_FUTURE = 0x7fffffff
} occtl_io_gltf_transform_format_t;

#define OCCTL_IO_GLTF_READ_OPTIONS_VERSION_1 1u
#define OCCTL_IO_GLTF_WRITE_OPTIONS_VERSION_1 1u

/**
 * Options for #occtl_io_gltf_read.  Pass NULL for defaults.
 */
typedef struct occtl_io_gltf_read_options
{
  uint32_t    struct_version;        /**< Must be #OCCTL_IO_GLTF_READ_OPTIONS_VERSION_1. */
  const void* p_next;                /**< Reserved; set to NULL. */
  int32_t     load_all_scenes;       /**< 1 to load all scenes; default 0. */
  int32_t     skip_empty_nodes;      /**< 1 to ignore nodes without geometry; default 1. */
  int32_t use_mesh_name_as_fallback; /**< 1 to use mesh name when node name is empty; default 1. */
  int32_t apply_scale;               /**< 1 to apply scale to triangulations; default 1. */
  int32_t parallel;                  /**< 1 to enable OCCT parallel read; default 0. */
  int32_t single_precision;          /**< 1 to read vertex data as single precision; default 1. */
  int32_t fill_incomplete;  /**< 1 to keep partially retrieved data on reader error; default 1. */
  int32_t memory_limit_mib; /**< Memory limit in MiB; -1 uses OCCT default. */
} occtl_io_gltf_read_options_t;

#define OCCTL_IO_GLTF_READ_OPTIONS_INIT                                                            \
  {OCCTL_IO_GLTF_READ_OPTIONS_VERSION_1, NULL, 0, 1, 1, 1, 0, 1, 1, -1}

/**
 * Options for #occtl_io_gltf_write.  Pass NULL for defaults.
 */
typedef struct occtl_io_gltf_write_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_IO_GLTF_WRITE_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved; set to NULL. */
  occtl_io_gltf_transform_format_t transform_format; /**< Transform encoding; default compact. */
  int32_t force_uv_export;       /**< 1 to export UV coordinates without textures. */
  int32_t embed_textures_in_glb; /**< 1 to embed image textures into GLB; default 1. */
  int32_t merge_faces;           /**< 1 to merge faces within a single part. */
  int32_t split_indices_16;      /**< 1 to prefer 16-bit indices while merging faces. */
  int32_t parallel;              /**< Reserved for future OCCT parallel write support; set 0. */
} occtl_io_gltf_write_options_t;

#define OCCTL_IO_GLTF_WRITE_OPTIONS_INIT                                                           \
  {OCCTL_IO_GLTF_WRITE_OPTIONS_VERSION_1, NULL, OCCTL_IO_GLTF_TRANSFORM_COMPACT, 0, 1, 0, 0, 0}

/**
 * Initialises @p options to default values matching #OCCTL_IO_GLTF_READ_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_gltf_read
 */
OCCTL_API void OCCTL_CALL occtl_io_gltf_read_options_init(occtl_io_gltf_read_options_t* options);

/**
 * Initialises @p options to default values matching #OCCTL_IO_GLTF_WRITE_OPTIONS_INIT.
 *
 * NULL-tolerant.
 *
 * @param[out] options  Borrows it.  May be NULL (no-op).
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_gltf_write
 */
OCCTL_API void OCCTL_CALL occtl_io_gltf_write_options_init(occtl_io_gltf_write_options_t* options);

/**
 * Reads a glTF or GLB file and ingests it into a freshly-created graph.
 *
 * @param[in]  path        Borrows it.  NUL-terminated UTF-8 path to the file.  Must be non-NULL.
 * @param[out] out_graph   Owns it.  Receives a new graph on success; NULL on failure.
 * @param[out] out_root    Borrows it.  Receives the NodeId of the root entity.
 * @param[in]  options     Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p path, @p out_graph, or @p out_root is NULL; or @p options has
 *                                 invalid fields (non-NULL @c p_next, non-0/1 flags, invalid
 *                                 @c memory_limit_mib).
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_IO_ERROR          File not found, unreadable, or blocked by OCCT reader limits.
 * @retval OCCTL_FORMAT_ERROR      File contents were not a valid glTF / GLB stream.
 * @retval OCCTL_TOPOLOGY_INVALID  Imported shape could not be ingested into BRepGraph.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_io_gltf_write, occtl_de_read
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_io_gltf_read(const char*      path,
                                                       occtl_graph_t**  out_graph,
                                                       occtl_node_id_t* out_root,
                                                       const occtl_io_gltf_read_options_t* options);

/**
 * Writes the topology rooted at @p root to a glTF or GLB file.
 *
 * The output container is inferred from @p path extension: `.gltf` writes a
 * JSON glTF stream and `.glb` writes a binary GLB stream.
 *
 * @param[in] graph    Borrows it.  Must be non-NULL.
 * @param[in] root     Root node id to serialise.
 * @param[in] path     Borrows it.  NUL-terminated UTF-8 path; existing file is overwritten.
 * @param[in] options  Borrows it.  May be NULL for defaults.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p graph or @p path is NULL; or @p options has invalid fields
 *                                 (non-NULL @c p_next, non-0/1 flags, non-zero reserved
 *                                 @c parallel).
 * @retval OCCTL_NOT_FOUND         @p root is invalid or has been removed.
 * @retval OCCTL_VERSION_MISMATCH  @p options has an unsupported struct_version.
 * @retval OCCTL_OUT_OF_RANGE      @p options has an unsupported transform_format.
 * @retval OCCTL_IO_ERROR          Filesystem failure, unwritable file, or OCCT writer failure.
 *
 * @threadsafe Yes (read-only on graph).
 *
 * @sa occtl_io_gltf_read, occtl_de_write
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_gltf_write(const occtl_graph_t*                 graph,
                      occtl_node_id_t                      root,
                      const char*                          path,
                      const occtl_io_gltf_write_options_t* options);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_IO_GLTF_H */
