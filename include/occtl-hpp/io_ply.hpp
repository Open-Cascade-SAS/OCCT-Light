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
 * @brief C++ veneer for PLY export.
 */

#ifndef OCCTL_HPP_IO_PLY_HPP
#define OCCTL_HPP_IO_PLY_HPP

#include <occtl/occtl_io_ply.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace occtl::io_ply
{

/// @brief Coordinate system used by PLY export conversion.
enum class CoordinateSystem
{
  YUp  = OCCTL_IO_PLY_COORDINATE_SYSTEM_Y_UP,
  ZUp  = OCCTL_IO_PLY_COORDINATE_SYSTEM_Z_UP,
  Gltf = OCCTL_IO_PLY_COORDINATE_SYSTEM_GLTF
};

/// @brief Options for #write.  PascalCase mirror of @c ::occtl_io_ply_write_options_t.
struct WriteOptions
{
  CoordinateSystem system_coordinate_system = CoordinateSystem::ZUp;
  CoordinateSystem file_coordinate_system   = CoordinateSystem::YUp;
  bool             write_normals            = true;
  bool             write_colors             = true;
  bool             write_texcoords          = false;
  bool             write_part_id            = true;
  bool             write_face_id            = false;
  std::string      comment;
  std::string      author;

  [[nodiscard]] ::occtl_io_ply_write_options_t to_c() const noexcept
  {
    ::occtl_io_ply_write_options_t aOpts = OCCTL_IO_PLY_WRITE_OPTIONS_INIT;
    aOpts.system_coordinate_system =
      static_cast<::occtl_io_ply_coordinate_system_t>(system_coordinate_system);
    aOpts.file_coordinate_system =
      static_cast<::occtl_io_ply_coordinate_system_t>(file_coordinate_system);
    aOpts.write_normals   = write_normals ? 1 : 0;
    aOpts.write_colors    = write_colors ? 1 : 0;
    aOpts.write_texcoords = write_texcoords ? 1 : 0;
    aOpts.write_part_id   = write_part_id ? 1 : 0;
    aOpts.write_face_id   = write_face_id ? 1 : 0;
    aOpts.comment         = comment.empty() ? nullptr : comment.c_str();
    aOpts.author          = author.empty() ? nullptr : author.c_str();
    return aOpts;
  }
};

/// @brief Writes the topology rooted at @p theRoot to a PLY file.
/// @throws Error on failure.
inline void write(const Graph&        theGraph,
                  const NodeId        theRoot,
                  const std::string&  thePath,
                  const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_ply_write_options_t aOpts = theOptions.to_c();
  check(::occtl_io_ply_write(theGraph.get(), theRoot.get(), thePath.c_str(), &aOpts));
}

} // namespace occtl::io_ply

#endif // OCCTL_HPP_IO_PLY_HPP
