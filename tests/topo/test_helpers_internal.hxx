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

#ifndef OCCTL_TEST_TOPO_HELPERS_INTERNAL_HXX
#define OCCTL_TEST_TOPO_HELPERS_INTERNAL_HXX

#include "test_helpers.hxx"

#include <BRepGraph.hxx>
#include <BRepGraph_Iterator.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepPrimAPI_MakeBox.hxx>

#include <gtest/gtest.h>

#include "../src/topo/GraphHandle.hxx"
#include "../src/topo/TopoMath.hxx"

/// @brief Builds a 10x20x30 box via BRepPrimAPI_MakeBox and loads it
///        into @p theGraph. Returns the raw TopoDS_Shape for tests that
///        need OCCT-level access.
///
/// There is no public-ABI counterpart for this helper because the C ABI
/// intentionally does not expose primitive constructors or TopoDS_Shape.
inline TopoDS_Shape loadBox(occtl_graph_t* const theGraph)
{
  TopoDS_Shape                  aBox = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();
  BRepGraph::ShapesView::Result aRes = theGraph->graph.Shapes().Add(aBox);
  EXPECT_TRUE(aRes.Ok);
  return aBox;
}

/// @brief Returns the first active BRepGraph_NodeId of the requested kind,
///        or an invalid NodeId if none exist. Used only by tests that
///        need to call BRepGraph_Tool / BRepGraph_TopoView directly.
inline BRepGraph_NodeId firstNodeOfKind(const occtl_graph_t* const   theGraph,
                                        const BRepGraph_NodeId::Kind theKind)
{
  switch (theKind)
  {
    case BRepGraph_NodeId::Kind::Solid: {
      BRepGraph_SolidIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::Shell: {
      BRepGraph_ShellIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::Face: {
      BRepGraph_FaceIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::Wire: {
      BRepGraph_WireIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::Edge: {
      BRepGraph_EdgeIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::Vertex: {
      BRepGraph_VertexIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::CoEdge: {
      BRepGraph_CoEdgeIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::Compound: {
      BRepGraph_CompoundIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::CompSolid: {
      BRepGraph_CompSolidIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::Product: {
      BRepGraph_ProductIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    case BRepGraph_NodeId::Kind::Occurrence: {
      BRepGraph_OccurrenceIterator anIt(theGraph->graph);
      return anIt.More() ? BRepGraph_NodeId(anIt.CurrentId()) : BRepGraph_NodeId();
    }
    default:
      return BRepGraph_NodeId();
  }
}

#endif // OCCTL_TEST_TOPO_HELPERS_INTERNAL_HXX
