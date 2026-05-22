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
 * @brief C++ veneer for VRML I/O.
 */

#ifndef OCCTL_HPP_IO_VRML_HPP
#define OCCTL_HPP_IO_VRML_HPP

#include <occtl/occtl_io_vrml.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <string>
#include <utility>
#include <vector>

namespace occtl::io_vrml
{

/// @brief Coordinate system used by VRML import/export conversion.
enum class CoordinateSystem
{
  YUp  = OCCTL_IO_VRML_COORDINATE_SYSTEM_Y_UP,
  ZUp  = OCCTL_IO_VRML_COORDINATE_SYSTEM_Z_UP,
  Gltf = OCCTL_IO_VRML_COORDINATE_SYSTEM_GLTF
};

/// @brief VRML writer version.
enum class WriterVersion
{
  V1 = OCCTL_IO_VRML_WRITER_VERSION_1,
  V2 = OCCTL_IO_VRML_WRITER_VERSION_2
};

/// @brief VRML output representation.
enum class Representation
{
  Shaded    = OCCTL_IO_VRML_REPRESENTATION_SHADED,
  Wireframe = OCCTL_IO_VRML_REPRESENTATION_WIREFRAME,
  Both      = OCCTL_IO_VRML_REPRESENTATION_BOTH
};

/// @brief Options for #read.  PascalCase mirror of @c ::occtl_io_vrml_read_options_t.
struct ReadOptions
{
  double           file_length_unit_m       = 1.0;
  CoordinateSystem system_coordinate_system = CoordinateSystem::ZUp;
  CoordinateSystem file_coordinate_system   = CoordinateSystem::YUp;
  bool             fill_incomplete          = true;

  [[nodiscard]] ::occtl_io_vrml_read_options_t to_c() const noexcept
  {
    ::occtl_io_vrml_read_options_t aOpts = OCCTL_IO_VRML_READ_OPTIONS_INIT;
    aOpts.file_length_unit_m             = file_length_unit_m;
    aOpts.system_coordinate_system =
      static_cast<::occtl_io_vrml_coordinate_system_t>(system_coordinate_system);
    aOpts.file_coordinate_system =
      static_cast<::occtl_io_vrml_coordinate_system_t>(file_coordinate_system);
    aOpts.fill_incomplete = fill_incomplete ? 1 : 0;
    return aOpts;
  }
};

/// @brief Options for #write.  PascalCase mirror of @c ::occtl_io_vrml_write_options_t.
struct WriteOptions
{
  WriterVersion  writer_version = WriterVersion::V2;
  Representation representation = Representation::Wireframe;

  [[nodiscard]] ::occtl_io_vrml_write_options_t to_c() const noexcept
  {
    ::occtl_io_vrml_write_options_t aOpts = OCCTL_IO_VRML_WRITE_OPTIONS_INIT;
    aOpts.writer_version = static_cast<::occtl_io_vrml_writer_version_t>(writer_version);
    aOpts.representation = static_cast<::occtl_io_vrml_representation_t>(representation);
    return aOpts;
  }
};

/// @brief Reads a VRML file into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read(const std::string& thePath,
                                     const ReadOptions& theOptions = ReadOptions{})
{
  ::occtl_graph_t*                     aRaw = nullptr;
  ::occtl_node_id_t                    aRoot{};
  const ::occtl_io_vrml_read_options_t aOpts = theOptions.to_c();
  check(::occtl_io_vrml_read(thePath.c_str(), &aRaw, &aRoot, &aOpts));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Writes the topology rooted at @p theRoot to a VRML file.
/// @throws Error on failure.
inline void write(const Graph&        theGraph,
                  const NodeId        theRoot,
                  const std::string&  thePath,
                  const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_vrml_write_options_t aOpts = theOptions.to_c();
  check(::occtl_io_vrml_write(theGraph.get(), theRoot.get(), thePath.c_str(), &aOpts));
}

/// @brief Reads a VRML memory buffer into a fresh graph.
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read_memory(const std::vector<uint8_t>& theData,
                                            const ReadOptions&          theOptions = ReadOptions{})
{
  ::occtl_graph_t*                     aRaw = nullptr;
  ::occtl_node_id_t                    aRoot{};
  const ::occtl_io_vrml_read_options_t aOpts = theOptions.to_c();
  check(::occtl_io_vrml_read_memory(theData.data(), theData.size(), &aRaw, &aRoot, &aOpts));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Writes the topology rooted at @p theRoot to a VRML memory buffer.
/// @throws Error on failure.
inline std::vector<uint8_t> write_memory(const Graph&        theGraph,
                                         const NodeId        theRoot,
                                         const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_vrml_write_options_t aOpts = theOptions.to_c();
  size_t                                aSize = 0;
  check(::occtl_io_vrml_write_memory(theGraph.get(), theRoot.get(), &aOpts, nullptr, 0, &aSize));
  std::vector<uint8_t> aBuffer(aSize);
  check(::occtl_io_vrml_write_memory(theGraph.get(),
                                     theRoot.get(),
                                     &aOpts,
                                     aBuffer.data(),
                                     aBuffer.size(),
                                     &aSize));
  return aBuffer;
}

} // namespace occtl::io_vrml

#endif // OCCTL_HPP_IO_VRML_HPP
