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

#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>

#include "test_helpers_internal.hxx"

#include <vector>

namespace
{

class TopoIterTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);
    ASSERT_NE(myGraph, nullptr);
    loadBox(myGraph);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  BRepGraph_NodeId firstNodeOfKind(BRepGraph_NodeId::Kind theKind) const
  {
    return ::firstNodeOfKind(myGraph, theKind);
  }

  occtl_node_id_t firstAbiNodeOfKind(occtl_node_kind_t theKind) const
  {
    return ::firstAbiNodeOfKind(myGraph, theKind);
  }

  static uint32_t countAll(occtl_node_iter_t* theIter)
  {
    uint32_t        aCount = 0;
    occtl_node_id_t anId;
    while (occtl_node_iter_next(theIter, &anId) == OCCTL_OK)
    {
      ++aCount;
    }
    return aCount;
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(TopoIterTest, NullGraph_ReturnsInvalidArgument)
{
  occtl_node_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_graph_face_iter_create(nullptr, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(TopoIterTest, NullOutIter_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_graph_face_iter_create(myGraph, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoIterTest, Next_NullIter_ReturnsInvalidArgument)
{
  occtl_node_id_t anId;
  EXPECT_EQ(occtl_node_iter_next(nullptr, &anId), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoIterTest, Next_NullOutId_ReturnsInvalidArgument)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &anIter), OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  EXPECT_EQ(occtl_node_iter_next(anIter, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);

  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, Free_Null_NoCrash)
{
  occtl_node_iter_free(nullptr);
}

TEST_F(TopoIterTest, EmptyGraph_ReturnsNotFoundImmediately)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  ASSERT_NE(aGraph, nullptr);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(aGraph, &anIter), OCCTL_OK);

  occtl_node_id_t anId = {0xdeadbeef};
  EXPECT_EQ(occtl_node_iter_next(anIter, &anId), OCCTL_NOT_FOUND);
  EXPECT_EQ(anId.bits, 0u);

  occtl_node_iter_free(anIter);
  occtl_graph_free(aGraph);
}

TEST_F(TopoIterTest, IdempotentEnd_AfterExhaustionStillNotFound)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &anIter), OCCTL_OK);

  occtl_node_id_t anId;
  while (occtl_node_iter_next(anIter, &anId) == OCCTL_OK)
  {
    ;
  }

  anId = {0xdeadbeef};
  EXPECT_EQ(occtl_node_iter_next(anIter, &anId), OCCTL_NOT_FOUND);
  EXPECT_EQ(anId.bits, 0u);

  anId = {0xdeadbeef};
  EXPECT_EQ(occtl_node_iter_next(anIter, &anId), OCCTL_NOT_FOUND);
  EXPECT_EQ(anId.bits, 0u);

  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, SolidIter_Box_CorrectCount)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_solid_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 1u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, ShellIter_Box_CorrectCount)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_shell_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 1u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, FaceIter_Box_CorrectCount)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 6u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, WireIter_Box_CorrectCount)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_wire_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 6u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, EdgeIter_Box_CorrectCount)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_edge_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 12u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, VertexIter_Box_CorrectCount)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_vertex_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 8u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, CoedgeIter_Box_CorrectCount)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_coedge_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 24u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, CompoundIter_Box_Zero)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_compound_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 0u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, CompSolidIter_Box_Zero)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_compsolid_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 0u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, ProductIter_Box_One)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_product_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 1u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, OccurrenceIter_Box_One)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_occurrence_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 1u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, RootProductIter_Box_One)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_root_product_iter_create(myGraph, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 1u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, FaceIter_YieldsFaceKinds)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &anIter), OCCTL_OK);

  occtl_node_id_t aFaceId;
  while (occtl_node_iter_next(anIter, &aFaceId) == OCCTL_OK)
  {
    occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
    ASSERT_EQ(occtl_graph_node_kind(myGraph, aFaceId, &aKind), OCCTL_OK);
    EXPECT_EQ(aKind, OCCTL_KIND_FACE);
  }

  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, ShellsOfSolid_Box_One)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_shells_of_solid_iter_create(myGraph, aSolidId, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 1u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, FacesOfShell_Box_Six)
{
  const occtl_node_id_t aShellId = firstAbiNodeOfKind(OCCTL_KIND_SHELL);
  ASSERT_NE(aShellId.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_faces_of_shell_iter_create(myGraph, aShellId, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 6u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, WiresOfFace_Box_One)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_wires_of_face_iter_create(myGraph, aFaceId, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 1u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, CoedgesOfWire_Box_Four)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_coedges_of_wire_iter_create(myGraph, aWireId, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 4u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, EdgesOfWire_Box_Four)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_edges_of_wire_iter_create(myGraph, aWireId, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 4u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, VerticesOfEdge_Box_Two)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_vertices_of_edge_iter_create(myGraph, anEdgeId, &anIter), OCCTL_OK);
  EXPECT_EQ(countAll(anIter), 2u);
  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, OccurrencesOfProduct_Box_Zero)
{
  occtl_node_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_occurrences_of_product_iter_create(myGraph, OCCTL_NODE_ID_INVALID, &anIter),
            OCCTL_NOT_FOUND);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(TopoIterTest, CoedgeAccessorsMatch_IteratorYieldsConsumableIds)
{
  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_coedge_iter_create(myGraph, &anIter), OCCTL_OK);

  occtl_node_id_t aCoId;
  while (occtl_node_iter_next(anIter, &aCoId) == OCCTL_OK)
  {
    occtl_node_id_t anEdgeId = OCCTL_NODE_ID_INVALID;
    ASSERT_EQ(occtl_topo_coedge_edge_of(myGraph, aCoId, &anEdgeId), OCCTL_OK);
    EXPECT_NE(anEdgeId.bits, 0u);

    occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
    ASSERT_EQ(occtl_graph_node_kind(myGraph, anEdgeId, &aKind), OCCTL_OK);
    EXPECT_EQ(aKind, OCCTL_KIND_EDGE);

    occtl_node_id_t aFaceId = OCCTL_NODE_ID_INVALID;
    ASSERT_EQ(occtl_topo_coedge_face_of(myGraph, aCoId, &aFaceId), OCCTL_OK);
    EXPECT_NE(aFaceId.bits, 0u);

    ASSERT_EQ(occtl_graph_node_kind(myGraph, aFaceId, &aKind), OCCTL_OK);
    EXPECT_EQ(aKind, OCCTL_KIND_FACE);
  }

  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, WireExplorer_Box_TraversesCoedges)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_wire_explorer_create(myGraph, aWireId, &anIter), OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  const size_t aNbCoedges = countAll(anIter);
  EXPECT_GT(aNbCoedges, 0u);

  uint32_t aExpected = 0;
  ASSERT_EQ(occtl_topo_wire_coedge_count(myGraph, aWireId, &aExpected), OCCTL_OK);
  EXPECT_EQ(aNbCoedges, static_cast<size_t>(aExpected));

  occtl_node_iter_free(anIter);
}

TEST_F(TopoIterTest, WireOrderEdges_Box_ReturnsOrderedEdgesWithOrientation)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_topo_wire_order_edges(myGraph, aWireId, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  std::vector<occtl_oriented_node_t> anEdges(aCount);
  ASSERT_EQ(occtl_topo_wire_order_edges(myGraph, aWireId, anEdges.data(), anEdges.size(), &aCount),
            OCCTL_OK);
  EXPECT_EQ(anEdges.size(), aCount);

  uint32_t aExpected = 0;
  ASSERT_EQ(occtl_topo_wire_coedge_count(myGraph, aWireId, &aExpected), OCCTL_OK);
  EXPECT_EQ(aCount, static_cast<size_t>(aExpected));

  for (const occtl_oriented_node_t& anEntry : anEdges)
  {
    occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
    ASSERT_EQ(occtl_graph_node_kind(myGraph, anEntry.id, &aKind), OCCTL_OK);
    EXPECT_EQ(aKind, OCCTL_KIND_EDGE);
    EXPECT_TRUE(anEntry.orientation == OCCTL_ORIENTATION_FORWARD
                || anEntry.orientation == OCCTL_ORIENTATION_REVERSED
                || anEntry.orientation == OCCTL_ORIENTATION_INTERNAL
                || anEntry.orientation == OCCTL_ORIENTATION_EXTERNAL);
  }
}

TEST_F(TopoIterTest, WireOrderEdges_BufferTooSmall_ReturnsRequiredCount)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  occtl_oriented_node_t aBuffer{};
  size_t                aCount = 0;
  EXPECT_EQ(occtl_topo_wire_order_edges(myGraph, aWireId, &aBuffer, 1, &aCount),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_GT(aCount, 1u);
}

TEST_F(TopoIterTest, WireOrderEdges_WrongKind)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  size_t aCount = 0;
  EXPECT_EQ(occtl_topo_wire_order_edges(myGraph, aFaceId, nullptr, 0, &aCount), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoIterTest, WireExplorer_WrongKind)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_wire_explorer_create(myGraph, aFaceId, &anIter), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(TopoIterTest, InvalidParentId_NotFound)
{
  occtl_node_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_faces_of_shell_iter_create(myGraph, OCCTL_NODE_ID_INVALID, &anIter),
            OCCTL_NOT_FOUND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(TopoIterTest, WrongKindParent_WrongKind)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_shells_of_solid_iter_create(myGraph, aFaceId, &anIter), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
  EXPECT_EQ(anIter, nullptr);
}

} // anonymous namespace
