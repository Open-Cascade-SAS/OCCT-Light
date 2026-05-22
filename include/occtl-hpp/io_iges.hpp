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
 * @brief C++ veneer for IGES I/O.
 */

#ifndef OCCTL_HPP_IO_IGES_HPP
#define OCCTL_HPP_IO_IGES_HPP

#include <occtl/occtl_io_iges.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace occtl::io_iges
{

/// @brief Options for #read.  PascalCase mirror of @c ::occtl_io_iges_read_options_t.
struct ReadOptions
{
  bool read_color = true;
  bool read_name  = true;

  [[nodiscard]] ::occtl_io_iges_read_options_t to_c() const noexcept
  {
    ::occtl_io_iges_read_options_t aOpts = OCCTL_IO_IGES_READ_OPTIONS_INIT;
    aOpts.read_color                     = read_color ? 1 : 0;
    aOpts.read_name                      = read_name ? 1 : 0;
    return aOpts;
  }
};

/// @brief Options for #write.  PascalCase mirror of @c ::occtl_io_iges_write_options_t.
struct WriteOptions
{
  bool write_brep = false;

  [[nodiscard]] ::occtl_io_iges_write_options_t to_c() const noexcept
  {
    ::occtl_io_iges_write_options_t aOpts = OCCTL_IO_IGES_WRITE_OPTIONS_INIT;
    aOpts.write_brep                      = write_brep ? 1 : 0;
    return aOpts;
  }
};

/// @brief Reads an IGES file into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read(const std::string& thePath,
                                     const ReadOptions& theOptions = ReadOptions{})
{
  ::occtl_graph_t*                     aRaw = nullptr;
  ::occtl_node_id_t                    aRoot{};
  const ::occtl_io_iges_read_options_t aOpts = theOptions.to_c();
  check(::occtl_io_iges_read(thePath.c_str(), &aRaw, &aRoot, &aOpts));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Writes the topology rooted at @p theRoot to an IGES file.
/// @throws Error on failure.
inline void write(const Graph&        theGraph,
                  const NodeId        theRoot,
                  const std::string&  thePath,
                  const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_iges_write_options_t aOpts = theOptions.to_c();
  check(::occtl_io_iges_write(theGraph.get(), theRoot.get(), thePath.c_str(), &aOpts));
}

} // namespace occtl::io_iges

#endif // OCCTL_HPP_IO_IGES_HPP
