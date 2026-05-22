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
 * @brief C++ veneer for STL I/O.
 */

#ifndef OCCTL_HPP_IO_STL_HPP
#define OCCTL_HPP_IO_STL_HPP

#include <occtl/occtl_io_stl.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace occtl::io_stl
{

/// @brief Options for #write.  PascalCase mirror of @c ::occtl_io_stl_write_options_t.
struct WriteOptions
{
  bool ascii_mode = false;

  [[nodiscard]] ::occtl_io_stl_write_options_t to_c() const noexcept
  {
    ::occtl_io_stl_write_options_t aOpts = OCCTL_IO_STL_WRITE_OPTIONS_INIT;
    aOpts.ascii_mode                     = ascii_mode ? 1 : 0;
    return aOpts;
  }
};

/// @brief Reads an STL file into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read(const std::string& thePath)
{
  ::occtl_graph_t*  aRaw = nullptr;
  ::occtl_node_id_t aRoot{};
  check(::occtl_io_stl_read(thePath.c_str(), &aRaw, &aRoot));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Reads an STL payload from memory into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read_memory(const uint8_t* const theData, const std::size_t theSize)
{
  ::occtl_graph_t*  aRaw = nullptr;
  ::occtl_node_id_t aRoot{};
  check(::occtl_io_stl_read_memory(theData, theSize, &aRaw, &aRoot));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Reads an STL payload from memory into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read_memory(const std::vector<uint8_t>& theData)
{
  return read_memory(theData.data(), theData.size());
}

/// @brief Writes the topology rooted at @p theRoot to an STL file.
/// @throws Error on failure.
inline void write(const Graph&        theGraph,
                  const NodeId        theRoot,
                  const std::string&  thePath,
                  const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_stl_write_options_t aOpts = theOptions.to_c();
  check(::occtl_io_stl_write(theGraph.get(), theRoot.get(), thePath.c_str(), &aOpts));
}

/// @brief Writes the topology rooted at @p theRoot to an STL payload in memory.
/// @throws Error on failure.
inline std::vector<uint8_t> write_memory(const Graph&        theGraph,
                                         const NodeId        theRoot,
                                         const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_stl_write_options_t aOpts = theOptions.to_c();
  std::size_t                          aSize = 0;
  check(::occtl_io_stl_write_memory(theGraph.get(), theRoot.get(), &aOpts, nullptr, 0, &aSize));
  std::vector<uint8_t> aBytes(aSize);
  check(::occtl_io_stl_write_memory(theGraph.get(),
                                    theRoot.get(),
                                    &aOpts,
                                    aBytes.data(),
                                    aBytes.size(),
                                    &aSize));
  aBytes.resize(aSize);
  return aBytes;
}

} // namespace occtl::io_stl

#endif // OCCTL_HPP_IO_STL_HPP
