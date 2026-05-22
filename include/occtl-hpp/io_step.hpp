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
 * @brief C++ veneer for STEP I/O.
 */

#ifndef OCCTL_HPP_IO_STEP_HPP
#define OCCTL_HPP_IO_STEP_HPP

#include <occtl/occtl_io_step.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace occtl::io_step
{

/// @brief Options for #read.  PascalCase mirror of @c ::occtl_io_step_read_options_t.
struct ReadOptions
{
  bool read_color = true;
  bool read_name  = true;
  bool read_layer = true;

  [[nodiscard]] ::occtl_io_step_read_options_t to_c() const noexcept
  {
    ::occtl_io_step_read_options_t aOpts = OCCTL_IO_STEP_READ_OPTIONS_INIT;
    aOpts.read_color                     = read_color ? 1 : 0;
    aOpts.read_name                      = read_name ? 1 : 0;
    aOpts.read_layer                     = read_layer ? 1 : 0;
    return aOpts;
  }
};

/// @brief Options for #write.  PascalCase mirror of @c ::occtl_io_step_write_options_t.
struct WriteOptions
{
  ::occtl_io_step_length_unit_t unit                 = OCCTL_IO_STEP_UNIT_MM;
  ::occtl_io_step_schema_t      schema               = OCCTL_IO_STEP_SCHEMA_AP242;
  bool                          write_surface_curves = true;
  bool                          write_tessellated    = true;

  [[nodiscard]] ::occtl_io_step_write_options_t to_c() const noexcept
  {
    ::occtl_io_step_write_options_t aOpts = OCCTL_IO_STEP_WRITE_OPTIONS_INIT;
    aOpts.unit                            = unit;
    aOpts.schema                          = schema;
    aOpts.write_surface_curves            = write_surface_curves ? 1 : 0;
    aOpts.write_tessellated               = write_tessellated ? 1 : 0;
    return aOpts;
  }
};

/// @brief Reads a STEP file into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read(const std::string& thePath,
                                     const ReadOptions& theOptions = ReadOptions{})
{
  ::occtl_graph_t*                     aRaw = nullptr;
  ::occtl_node_id_t                    aRoot{};
  const ::occtl_io_step_read_options_t aOpts = theOptions.to_c();
  check(::occtl_io_step_read(thePath.c_str(), &aRaw, &aRoot, &aOpts));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Reads a STEP memory payload into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read_memory(const uint8_t* const theData,
                                            const std::size_t    theSize,
                                            const ReadOptions&   theOptions = ReadOptions{})
{
  ::occtl_graph_t*                     aRaw = nullptr;
  ::occtl_node_id_t                    aRoot{};
  const ::occtl_io_step_read_options_t aOpts = theOptions.to_c();
  check(::occtl_io_step_read_memory(theData, theSize, &aRaw, &aRoot, &aOpts));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Reads a STEP memory payload into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read_memory(const std::vector<uint8_t>& theData,
                                            const ReadOptions&          theOptions = ReadOptions{})
{
  return read_memory(theData.empty() ? nullptr : theData.data(), theData.size(), theOptions);
}

/// @brief Writes the topology rooted at @p theRoot to a STEP file.
/// @throws Error on failure.
inline void write(const Graph&        theGraph,
                  const NodeId        theRoot,
                  const std::string&  thePath,
                  const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_step_write_options_t aOpts = theOptions.to_c();
  check(::occtl_io_step_write(theGraph.get(), theRoot.get(), thePath.c_str(), &aOpts));
}

/// @brief Writes the topology rooted at @p theRoot into a STEP memory payload.
/// @throws Error on failure.
inline std::vector<uint8_t> write_memory(const Graph&        theGraph,
                                         const NodeId        theRoot,
                                         const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_step_write_options_t aOpts = theOptions.to_c();
  std::size_t                           aSize = 0;
  check(::occtl_io_step_write_memory(theGraph.get(), theRoot.get(), &aOpts, nullptr, 0, &aSize));

  std::vector<uint8_t> aData(aSize);
  check(::occtl_io_step_write_memory(theGraph.get(),
                                     theRoot.get(),
                                     &aOpts,
                                     aData.empty() ? nullptr : aData.data(),
                                     aData.size(),
                                     &aSize));
  return aData;
}

} // namespace occtl::io_step

#endif // OCCTL_HPP_IO_STEP_HPP
