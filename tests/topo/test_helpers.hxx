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

#ifndef OCCTL_TEST_TOPO_HELPERS_HXX
#define OCCTL_TEST_TOPO_HELPERS_HXX

#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

/// @brief Returns the first active node of the requested kind in @p theGraph,
///        or @c OCCTL_NODE_ID_INVALID if none exist.
///
/// Uses the public @c occtl_graph_*_iter_create / @c occtl_node_iter_next /
/// @c occtl_node_iter_free API exclusively — no internal headers.
inline occtl_node_id_t firstAbiNodeOfKind(const occtl_graph_t* const theGraph,
                                          const occtl_node_kind_t    theKind)
{
  occtl_node_iter_t* anIter  = nullptr;
  occtl_status_t     aStatus = OCCTL_OK;
  switch (theKind)
  {
    case OCCTL_KIND_SOLID:
      aStatus = ::occtl_graph_solid_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_SHELL:
      aStatus = ::occtl_graph_shell_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_FACE:
      aStatus = ::occtl_graph_face_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_WIRE:
      aStatus = ::occtl_graph_wire_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_EDGE:
      aStatus = ::occtl_graph_edge_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_VERTEX:
      aStatus = ::occtl_graph_vertex_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_COMPOUND:
      aStatus = ::occtl_graph_compound_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_COMPSOLID:
      aStatus = ::occtl_graph_compsolid_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_COEDGE:
      aStatus = ::occtl_graph_coedge_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_PRODUCT:
      aStatus = ::occtl_graph_product_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_OCCURRENCE:
      aStatus = ::occtl_graph_occurrence_iter_create(theGraph, &anIter);
      break;
    default:
      return OCCTL_NODE_ID_INVALID;
  }
  if (aStatus != OCCTL_OK || anIter == nullptr)
  {
    return OCCTL_NODE_ID_INVALID;
  }

  occtl_node_id_t      anId  = OCCTL_NODE_ID_INVALID;
  const occtl_status_t aNext = ::occtl_node_iter_next(anIter, &anId);
  ::occtl_node_iter_free(anIter);
  return aNext == OCCTL_OK ? anId : OCCTL_NODE_ID_INVALID;
}

#endif // OCCTL_TEST_TOPO_HELPERS_HXX
