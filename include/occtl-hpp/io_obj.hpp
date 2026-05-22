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
 * @brief C++ veneer for Wavefront OBJ I/O.
 */

#ifndef OCCTL_HPP_IO_OBJ_HPP
#define OCCTL_HPP_IO_OBJ_HPP

#include <occtl/occtl_io_obj.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <string>
#include <utility>
#include <vector>

namespace occtl::io_obj
{

/// @brief Coordinate system used by OBJ import/export conversion.
enum class CoordinateSystem
{
  YUp  = OCCTL_IO_OBJ_COORDINATE_SYSTEM_Y_UP,
  ZUp  = OCCTL_IO_OBJ_COORDINATE_SYSTEM_Z_UP,
  Gltf = OCCTL_IO_OBJ_COORDINATE_SYSTEM_GLTF
};

/// @brief Options for #read.  PascalCase mirror of @c ::occtl_io_obj_read_options_t.
struct ReadOptions
{
  double           file_length_unit_m       = 1.0;
  CoordinateSystem system_coordinate_system = CoordinateSystem::ZUp;
  CoordinateSystem file_coordinate_system   = CoordinateSystem::YUp;
  bool             single_precision         = false;
  bool             create_shapes            = false;
  bool             fill_incomplete          = true;
  int              memory_limit_mib         = -1;
  std::string      root_prefix;

  [[nodiscard]] ::occtl_io_obj_read_options_t to_c() const noexcept
  {
    ::occtl_io_obj_read_options_t aOpts = OCCTL_IO_OBJ_READ_OPTIONS_INIT;
    aOpts.file_length_unit_m            = file_length_unit_m;
    aOpts.system_coordinate_system =
      static_cast<::occtl_io_obj_coordinate_system_t>(system_coordinate_system);
    aOpts.file_coordinate_system =
      static_cast<::occtl_io_obj_coordinate_system_t>(file_coordinate_system);
    aOpts.single_precision = single_precision ? 1 : 0;
    aOpts.create_shapes    = create_shapes ? 1 : 0;
    aOpts.fill_incomplete  = fill_incomplete ? 1 : 0;
    aOpts.memory_limit_mib = memory_limit_mib;
    aOpts.root_prefix      = root_prefix.empty() ? nullptr : root_prefix.c_str();
    return aOpts;
  }
};

/// @brief Options for #write.  PascalCase mirror of @c ::occtl_io_obj_write_options_t.
struct WriteOptions
{
  CoordinateSystem system_coordinate_system = CoordinateSystem::ZUp;
  CoordinateSystem file_coordinate_system   = CoordinateSystem::YUp;
  std::string      comment;
  std::string      author;

  [[nodiscard]] ::occtl_io_obj_write_options_t to_c() const noexcept
  {
    ::occtl_io_obj_write_options_t aOpts = OCCTL_IO_OBJ_WRITE_OPTIONS_INIT;
    aOpts.system_coordinate_system =
      static_cast<::occtl_io_obj_coordinate_system_t>(system_coordinate_system);
    aOpts.file_coordinate_system =
      static_cast<::occtl_io_obj_coordinate_system_t>(file_coordinate_system);
    aOpts.comment = comment.empty() ? nullptr : comment.c_str();
    aOpts.author  = author.empty() ? nullptr : author.c_str();
    return aOpts;
  }
};

/// @brief Reads an OBJ file into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read(const std::string& thePath,
                                     const ReadOptions& theOptions = ReadOptions{})
{
  ::occtl_graph_t*                    aRaw = nullptr;
  ::occtl_node_id_t                   aRoot{};
  const ::occtl_io_obj_read_options_t aOpts = theOptions.to_c();
  check(::occtl_io_obj_read(thePath.c_str(), &aRaw, &aRoot, &aOpts));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Writes the topology rooted at @p theRoot to an OBJ file.
/// @throws Error on failure.
inline void write(const Graph&        theGraph,
                  const NodeId        theRoot,
                  const std::string&  thePath,
                  const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_obj_write_options_t aOpts = theOptions.to_c();
  check(::occtl_io_obj_write(theGraph.get(), theRoot.get(), thePath.c_str(), &aOpts));
}

} // namespace occtl::io_obj

#endif // OCCTL_HPP_IO_OBJ_HPP
