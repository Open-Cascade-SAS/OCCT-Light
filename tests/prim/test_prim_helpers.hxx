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

#ifndef OCCTL_TEST_PRIM_HELPERS_HXX
#define OCCTL_TEST_PRIM_HELPERS_HXX

#include <occtl/occtl_core.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>

/// @brief Count active nodes of @p theKind in @p theGraph.
inline std::size_t countOfKind(const occtl_graph_t* const theGraph, const occtl_node_kind_t theKind)
{
  switch (theKind)
  {
    case OCCTL_KIND_SOLID:
      return occtl_graph_count_value(occtl_graph_solid_count, theGraph);
    case OCCTL_KIND_SHELL:
      return occtl_graph_count_value(occtl_graph_shell_count, theGraph);
    case OCCTL_KIND_FACE:
      return occtl_graph_count_value(occtl_graph_face_count, theGraph);
    case OCCTL_KIND_WIRE:
      return occtl_graph_count_value(occtl_graph_wire_count, theGraph);
    case OCCTL_KIND_EDGE:
      return occtl_graph_count_value(occtl_graph_edge_count, theGraph);
    case OCCTL_KIND_VERTEX:
      return occtl_graph_count_value(occtl_graph_vertex_count, theGraph);
    case OCCTL_KIND_COMPOUND:
      return occtl_graph_count_value(occtl_graph_compound_count, theGraph);
    case OCCTL_KIND_COMPSOLID:
      return occtl_graph_count_value(occtl_graph_compsolid_count, theGraph);
    case OCCTL_KIND_COEDGE:
      return occtl_graph_count_value(occtl_graph_coedge_count, theGraph);
    case OCCTL_KIND_PRODUCT:
      return occtl_graph_count_value(occtl_graph_product_count, theGraph);
    case OCCTL_KIND_OCCURRENCE:
      return occtl_graph_count_value(occtl_graph_occurrence_count, theGraph);
    default:
      return 0;
  }
}

/// @brief Returns the first active node of @p theKind, or OCCTL_NODE_ID_INVALID.
inline occtl_node_id_t firstNodeOfKind(const occtl_graph_t* const theGraph,
                                       const occtl_node_kind_t    theKind)
{
  occtl_node_iter_t* anIter  = nullptr;
  occtl_status_t     aStatus = OCCTL_INVALID_ARGUMENT;
  switch (theKind)
  {
    case OCCTL_KIND_SOLID:
      aStatus = occtl_graph_solid_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_SHELL:
      aStatus = occtl_graph_shell_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_FACE:
      aStatus = occtl_graph_face_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_WIRE:
      aStatus = occtl_graph_wire_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_EDGE:
      aStatus = occtl_graph_edge_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_VERTEX:
      aStatus = occtl_graph_vertex_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_COMPOUND:
      aStatus = occtl_graph_compound_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_COMPSOLID:
      aStatus = occtl_graph_compsolid_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_COEDGE:
      aStatus = occtl_graph_coedge_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_PRODUCT:
      aStatus = occtl_graph_product_iter_create(theGraph, &anIter);
      break;
    case OCCTL_KIND_OCCURRENCE:
      aStatus = occtl_graph_occurrence_iter_create(theGraph, &anIter);
      break;
    default:
      break;
  }
  if (aStatus != OCCTL_OK || anIter == nullptr)
  {
    return OCCTL_NODE_ID_INVALID;
  }

  occtl_node_id_t      aNode       = OCCTL_NODE_ID_INVALID;
  const occtl_status_t aNextStatus = occtl_node_iter_next(anIter, &aNode);
  occtl_node_iter_free(anIter);
  return aNextStatus == OCCTL_OK ? aNode : OCCTL_NODE_ID_INVALID;
}

/// @brief Returns the first descendant of @p theRoot with @p theKind.
inline occtl_node_id_t firstChildOfKind(const occtl_graph_t* const theGraph,
                                        const occtl_node_id_t      theRoot,
                                        const occtl_node_kind_t    theKind)
{
  occtl_topo_child_explorer_config_t aConfig = OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT;
  aConfig.target_kind                        = theKind;

  occtl_topo_explorer_iter_t* anIter = nullptr;
  if (occtl_topo_child_explorer_create(theGraph, theRoot, &aConfig, &anIter) != OCCTL_OK
      || anIter == nullptr)
  {
    return OCCTL_NODE_ID_INVALID;
  }

  occtl_node_id_t      aNode = OCCTL_NODE_ID_INVALID;
  occtl_transform_t    aTransform{};
  occtl_orientation_t  anOrientation = OCCTL_ORIENTATION_FORWARD;
  const occtl_status_t aStatus =
    occtl_topo_explorer_iter_next(anIter, &aNode, &aTransform, &anOrientation);
  occtl_topo_explorer_iter_free(anIter);
  return aStatus == OCCTL_OK ? aNode : OCCTL_NODE_ID_INVALID;
}

/// @brief Asserts that the last ABI error carries a non-empty diagnostic.
inline void expectLastErrorMessage()
{
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  ASSERT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

#endif // OCCTL_TEST_PRIM_HELPERS_HXX
