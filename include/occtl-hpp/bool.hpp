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
 * @brief C++ veneer for boolean operations.
 *
 * Provides idiomatic free functions for the five boolean operations,
 * returning the result root.
 * Failures translate to occtl::Error via check().
 */

#ifndef OCCTL_HPP_BOOL_HPP
#define OCCTL_HPP_BOOL_HPP

#include <occtl/occtl_bool.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

/// @namespace occtl::bool_
/// @brief Boolean-operation veneer namespace; trailing underscore avoids the C++ keyword.
namespace occtl::bool_
{

/// @brief PascalCase mirror of @c ::occtl_bool_options_t with idiomatic defaults.
struct BoolOptions
{
  double fuzzy_value                = 0.0;
  bool   run_parallel               = false;
  bool   simplify_result            = false;
  double simplify_angular_tolerance = 1.0e-2;
  bool   build_history              = true;

  /// @brief Project into the C ABI options struct.
  [[nodiscard]] ::occtl_bool_options_t to_c() const noexcept
  {
    ::occtl_bool_options_t aOpts     = OCCTL_BOOL_OPTIONS_INIT;
    aOpts.fuzzy_value                = fuzzy_value;
    aOpts.run_parallel               = run_parallel ? 1 : 0;
    aOpts.simplify_result            = simplify_result ? 1 : 0;
    aOpts.simplify_angular_tolerance = simplify_angular_tolerance;
    aOpts.build_history              = build_history ? 1 : 0;
    return aOpts;
  }
};

namespace detail
{
using EntryFn = ::occtl_status_t (*)(::occtl_graph_t*,
                                     const ::occtl_node_id_t*,
                                     std::size_t,
                                     const ::occtl_node_id_t*,
                                     std::size_t,
                                     const ::occtl_bool_options_t*,
                                     ::occtl_node_id_t*);

inline NodeId run(const EntryFn              theFn,
                  Graph&                     theGraph,
                  const std::vector<NodeId>& theObjects,
                  const std::vector<NodeId>& theTools,
                  const BoolOptions&         theOpts)
{
  std::vector<::occtl_node_id_t> aObjIds;
  aObjIds.reserve(theObjects.size());
  for (const NodeId& aId : theObjects)
  {
    aObjIds.emplace_back(aId.get());
  }

  std::vector<::occtl_node_id_t> aToolIds;
  aToolIds.reserve(theTools.size());
  for (const NodeId& aId : theTools)
  {
    aToolIds.emplace_back(aId.get());
  }

  ::occtl_bool_options_t aOpts = theOpts.to_c();
  ::occtl_node_id_t      aRoot = OCCTL_NODE_ID_INVALID;

  check(theFn(theGraph.get(),
              aObjIds.empty() ? nullptr : aObjIds.data(),
              aObjIds.size(),
              aToolIds.empty() ? nullptr : aToolIds.data(),
              aToolIds.size(),
              &aOpts,
              &aRoot));

  return NodeId(aRoot);
}
} // namespace detail

/// @brief Boolean Fuse of two argument groups.  See occtl_bool_fuse.
inline NodeId fuse(Graph&                     theGraph,
                   const std::vector<NodeId>& theObjects,
                   const std::vector<NodeId>& theTools,
                   const BoolOptions&         theOpts = {})
{
  return detail::run(&::occtl_bool_fuse, theGraph, theObjects, theTools, theOpts);
}

/// @brief Boolean Cut.  See occtl_bool_cut.
inline NodeId cut(Graph&                     theGraph,
                  const std::vector<NodeId>& theObjects,
                  const std::vector<NodeId>& theTools,
                  const BoolOptions&         theOpts = {})
{
  return detail::run(&::occtl_bool_cut, theGraph, theObjects, theTools, theOpts);
}

/// @brief Boolean Common (intersection).  See occtl_bool_common.
inline NodeId common(Graph&                     theGraph,
                     const std::vector<NodeId>& theObjects,
                     const std::vector<NodeId>& theTools,
                     const BoolOptions&         theOpts = {})
{
  return detail::run(&::occtl_bool_common, theGraph, theObjects, theTools, theOpts);
}

/// @brief Boolean Section.  See occtl_bool_section.
inline NodeId section(Graph&                     theGraph,
                      const std::vector<NodeId>& theObjects,
                      const std::vector<NodeId>& theTools,
                      const BoolOptions&         theOpts = {})
{
  return detail::run(&::occtl_bool_section, theGraph, theObjects, theTools, theOpts);
}

/// @brief Boolean Split.  See occtl_bool_split.
inline NodeId split(Graph&                     theGraph,
                    const std::vector<NodeId>& theObjects,
                    const std::vector<NodeId>& theTools,
                    const BoolOptions&         theOpts = {})
{
  return detail::run(&::occtl_bool_split, theGraph, theObjects, theTools, theOpts);
}

} // namespace occtl::bool_

#endif // OCCTL_HPP_BOOL_HPP
