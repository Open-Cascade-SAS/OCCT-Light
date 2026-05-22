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
 * @brief C++ veneer for native BRep I/O.
 */

#ifndef OCCTL_HPP_IO_BREP_HPP
#define OCCTL_HPP_IO_BREP_HPP

#include <occtl/occtl_io_brep.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace occtl::io_brep
{

/// @brief Options for #write.  PascalCase mirror of @c ::occtl_io_brep_write_options_t.
/// The output format (binary / ASCII) is managed internally by the OCCT
/// @c DEBREP_Provider.
struct WriteOptions
{
  bool write_triangulation = true;

  [[nodiscard]] ::occtl_io_brep_write_options_t to_c() const noexcept
  {
    ::occtl_io_brep_write_options_t aOpts = OCCTL_IO_BREP_WRITE_OPTIONS_INIT;
    aOpts.write_triangulation             = write_triangulation ? 1 : 0;
    return aOpts;
  }
};

/// @brief Reads a BRep file into a fresh graph and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read(const std::string& thePath)
{
  ::occtl_graph_t*  aRaw = nullptr;
  ::occtl_node_id_t aRoot{};
  check(::occtl_io_brep_read(thePath.c_str(), &aRaw, &aRoot));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Writes the topology rooted at @p theRoot to a BRep file.
/// @throws Error on failure.
inline void write(const Graph&        theGraph,
                  const NodeId        theRoot,
                  const std::string&  thePath,
                  const WriteOptions& theOptions = WriteOptions{})
{
  const ::occtl_io_brep_write_options_t aOpts = theOptions.to_c();
  check(::occtl_io_brep_write(theGraph.get(), theRoot.get(), thePath.c_str(), &aOpts));
}

} // namespace occtl::io_brep

#endif // OCCTL_HPP_IO_BREP_HPP
