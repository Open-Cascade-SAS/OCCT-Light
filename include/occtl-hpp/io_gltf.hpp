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
 * @file
 * @brief C++ veneer for glTF / GLB I/O.
 */

#ifndef OCCTL_HPP_IO_GLTF_HPP
#define OCCTL_HPP_IO_GLTF_HPP

#include <occtl/occtl_io_gltf.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <string>
#include <utility>
#include <vector>

namespace occtl::io_gltf
{

/// @brief Transform encoding used when writing glTF node transforms.
enum class TransformFormat
{
  Compact = OCCTL_IO_GLTF_TRANSFORM_COMPACT,
  Mat4    = OCCTL_IO_GLTF_TRANSFORM_MAT4,
  Trs     = OCCTL_IO_GLTF_TRANSFORM_TRS
};

/// @brief Options for #read.  PascalCase mirror of @c ::occtl_io_gltf_read_options_t.
struct ReadOptions
{
  bool load_all_scenes           = false;
  bool skip_empty_nodes          = true;
  bool use_mesh_name_as_fallback = true;
  bool apply_scale               = true;
  bool parallel                  = false;
  bool single_precision          = true;
  bool fill_incomplete           = true;
  int  memory_limit_mib          = -1;

  [[nodiscard]] ::occtl_io_gltf_read_options_t to_c() const noexcept
  {
    ::occtl_io_gltf_read_options_t aOpts = OCCTL_IO_GLTF_READ_OPTIONS_INIT;
    aOpts.load_all_scenes                = load_all_scenes ? 1 : 0;
    aOpts.skip_empty_nodes               = skip_empty_nodes ? 1 : 0;
    aOpts.use_mesh_name_as_fallback      = use_mesh_name_as_fallback ? 1 : 0;
    aOpts.apply_scale                    = apply_scale ? 1 : 0;
    aOpts.parallel                       = parallel ? 1 : 0;
    aOpts.single_precision               = single_precision ? 1 : 0;
    aOpts.fill_incomplete                = fill_incomplete ? 1 : 0;
    aOpts.memory_limit_mib               = memory_limit_mib;
    return aOpts;
  }
};

/// @brief Options for #write.  PascalCase mirror of @c ::occtl_io_gltf_write_options_t.
struct WriteOptions
{
  TransformFormat transform_format      = TransformFormat::Compact;
  bool            force_uv_export       = false;
  bool            embed_textures_in_glb = true;
  bool            merge_faces           = false;
  bool            split_indices_16      = false;

  [[nodiscard]] ::occtl_io_gltf_write_options_t to_c() const noexcept
  {
    ::occtl_io_gltf_write_options_t aOpts = OCCTL_IO_GLTF_WRITE_OPTIONS_INIT;
    aOpts.transform_format      = static_cast<::occtl_io_gltf_transform_format_t>(transform_format);
    aOpts.force_uv_export       = force_uv_export ? 1 : 0;
    aOpts.embed_textures_in_glb = embed_textures_in_glb ? 1 : 0;
    aOpts.merge_faces           = merge_faces ? 1 : 0;
    aOpts.split_indices_16      = split_indices_16 ? 1 : 0;
    return aOpts;
  }
};

/// @brief Reads a glTF or GLB file into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read(const std::string& thePath,
                                     const ReadOptions& theOptions = ReadOptions{})
{
  ::occtl_graph_t*                     aRaw = nullptr;
  ::occtl_node_id_t                    aRoot{};
  const ::occtl_io_gltf_read_options_t aOpts = theOptions.to_c();
  check(::occtl_io_gltf_read(thePath.c_str(), &aRaw, &aRoot, &aOpts));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Writes the topology rooted at @p theRoot to a glTF or GLB file.
/// @throws Error on failure.
inline void write(const Graph&        theGraph,
                  const NodeId        theRoot,
                  const std::string&  thePath,
                  const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_gltf_write_options_t aOpts = theOptions.to_c();
  check(::occtl_io_gltf_write(theGraph.get(), theRoot.get(), thePath.c_str(), &aOpts));
}

} // namespace occtl::io_gltf

#endif // OCCTL_HPP_IO_GLTF_HPP
