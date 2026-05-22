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
 * @brief C++ veneer for the heal module.
 *
 * Header-only options and exception translation over the C ABI.
 * Local identifiers follow OCCT style.
 */

#ifndef OCCTL_HPP_HEAL_HPP
#define OCCTL_HPP_HEAL_HPP

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>
#include <occtl/occtl_heal.h>

namespace occtl::heal
{

/// @brief Healing mode selector.
using Mode = ::occtl_heal_mode_t;

/// @brief Versioned options for shape healing.
///
/// The default constructor calls occtl_heal_options_init; fluent setters
/// return `*this` so callers can chain:
/// @code
///   heal::Options anOpts{}.set_mode(OCCTL_HEAL_MODE_FULL).set_tolerance(1e-5);
/// @endcode
struct Options : ::occtl_heal_options_t
{
  Options() { ::occtl_heal_options_init(this); }

  Options& set_mode(Mode m)
  {
    mode = m;
    return *this;
  }

  Options& set_tolerance(double t)
  {
    tolerance = t;
    return *this;
  }

  Options& set_fix_same_parameter(bool v)
  {
    fix_same_parameter = v ? 1 : 0;
    return *this;
  }

  Options& set_fix_small_edges(bool v)
  {
    fix_small_edges = v ? 1 : 0;
    return *this;
  }

  Options& set_fix_face_orient(bool v)
  {
    fix_face_orient = v ? 1 : 0;
    return *this;
  }

  Options& set_fix_missing_seam(bool v)
  {
    fix_missing_seam = v ? 1 : 0;
    return *this;
  }
};

/// @brief Versioned options for same-domain unification.
struct UnifySameDomainOptions : ::occtl_heal_unify_same_domain_options_t
{
  UnifySameDomainOptions() { ::occtl_heal_unify_same_domain_options_init(this); }

  UnifySameDomainOptions& set_unify_edges(bool v)
  {
    unify_edges = v ? 1 : 0;
    return *this;
  }

  UnifySameDomainOptions& set_unify_faces(bool v)
  {
    unify_faces = v ? 1 : 0;
    return *this;
  }

  UnifySameDomainOptions& set_concat_bspline(bool v)
  {
    concat_bspline = v ? 1 : 0;
    return *this;
  }

  UnifySameDomainOptions& set_allow_internal_edges(bool v)
  {
    allow_internal_edges = v ? 1 : 0;
    return *this;
  }

  UnifySameDomainOptions& set_safe_input(bool v)
  {
    safe_input = v ? 1 : 0;
    return *this;
  }

  UnifySameDomainOptions& set_linear_tolerance(double v)
  {
    linear_tolerance = v;
    return *this;
  }

  UnifySameDomainOptions& set_angular_tolerance(double v)
  {
    angular_tolerance = v;
    return *this;
  }
};

/// @brief Heals the shape referenced by @p theNodeId and ingests the
///        result as a new topology root into @p theGraph.
///
/// @param[in,out] theGraph   Graph owning the shape to heal.
/// @param[in]     theNodeId  Root node of the shape to heal.
/// @param[in]     theOptions Healing configuration; default is STABLE.
///
/// @throws Error on any non-OK status code.
inline void heal_shape(Graph& theGraph, NodeId theNodeId, const Options& theOptions = Options{})
{
  check(::occtl_heal_shape(theGraph.get(), theNodeId.get(), &theOptions));
}

/// @brief Unifies same-domain edges/faces and returns the new topology root.
inline NodeId unify_same_domain(Graph&                        theGraph,
                                NodeId                        theNodeId,
                                const UnifySameDomainOptions& theOptions = UnifySameDomainOptions{})
{
  ::occtl_node_id_t aRoot{};
  check(::occtl_heal_unify_same_domain(theGraph.get(), theNodeId.get(), &theOptions, &aRoot));
  return NodeId(aRoot);
}

} // namespace occtl::heal

#endif // OCCTL_HPP_HEAL_HPP
