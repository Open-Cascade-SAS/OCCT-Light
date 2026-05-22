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
#include <occtl/occtl_curves.h>
#include <occtl/occtl_curves2d.h>
#include <occtl/occtl_geom.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_surfaces.h>
#include <occtl/occtl_topo.h>

#include "test_helpers_internal.hxx"

#include <BRepGraph_EditorView.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_TopoView.hxx>

#include <GeomAdaptor_TransformedCurve.hxx>

#include <gtest/gtest.h>

namespace
{

class TopoComposeTest : public ::testing::Test
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

  occtl_node_id_t firstAbiNodeOfKind(occtl_node_kind_t theKind) const
  {
    return ::firstAbiNodeOfKind(myGraph, theKind);
  }

  occtl_graph_t* myGraph = nullptr;
};

occtl_node_id_t makeRectangleWire(occtl_graph_t* const theGraph,
                                  const double         theX,
                                  const double         theY,
                                  const double         theWidth,
                                  const double         theHeight)
{
  occtl_prim_rectangle_info_t aInfo = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aInfo.placement.location          = {theX, theY, 0.0};
  aInfo.width                       = theWidth;
  aInfo.height                      = theHeight;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_rectangle(theGraph, &aInfo, &aWire), OCCTL_OK);
  return aWire;
}

occtl_rep_id_t makePlaneSurface(occtl_graph_t* const theGraph)
{
  occtl_geom_plane_t aPlane = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  occtl_rep_id_t aSurfaceId = {};
  EXPECT_EQ(occtl_surface_create_plane(theGraph, &aSurfaceId, aPlane), OCCTL_OK);
  return aSurfaceId;
}

occtl_node_id_t makeFaceWithWires(occtl_graph_t* const         theGraph,
                                  const occtl_node_id_t* const theWires,
                                  const size_t                 theWireCount)
{
  occtl_rep_id_t aSurfaceId = makePlaneSurface(theGraph);

  occtl_topo_make_face_from_wires_auto_options_t anOptions =
    OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_INIT;
  anOptions.surface    = aSurfaceId;
  anOptions.wires      = theWires;
  anOptions.wire_count = theWireCount;
  anOptions.tolerance  = 1e-6;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_face_from_wires_auto(theGraph, &anOptions, &aFace), OCCTL_OK);
  return aFace;
}

TEST_F(TopoComposeTest, ShellAddFace_AddsFaceAndIncreasesCount)
{
  occtl_topo_make_shell_info_t aShellInfo = OCCTL_TOPO_MAKE_SHELL_INFO_INIT;
  aShellInfo.faces                        = nullptr;
  aShellInfo.face_count                   = 0;

  occtl_node_id_t aShell = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_shell(myGraph, &aShellInfo, &aShell), OCCTL_OK);
  ASSERT_NE(aShell.bits, 0u);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_shell_face_count(myGraph, aShell, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 0u);

  const occtl_node_id_t aFace = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  ASSERT_EQ(occtl_topo_shell_add_face(myGraph, aShell, aFace, OCCTL_ORIENTATION_FORWARD), OCCTL_OK);

  ASSERT_EQ(occtl_topo_shell_face_count(myGraph, aShell, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);
}

TEST_F(TopoComposeTest, ShellRemoveFace_RemovesFaceFromShell)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  occtl_oriented_node_t        aFaces[1]  = {{aFace, OCCTL_ORIENTATION_FORWARD}};
  occtl_topo_make_shell_info_t aShellInfo = OCCTL_TOPO_MAKE_SHELL_INFO_INIT;
  aShellInfo.faces                        = aFaces;
  aShellInfo.face_count                   = 1;

  occtl_node_id_t aShell = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_shell(myGraph, &aShellInfo, &aShell), OCCTL_OK);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_shell_face_count(myGraph, aShell, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);

  ASSERT_EQ(occtl_topo_shell_remove_face(myGraph, aShell, aFace), OCCTL_OK);

  ASSERT_EQ(occtl_topo_shell_face_count(myGraph, aShell, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 0u);
}

TEST_F(TopoComposeTest, CompoundAddChild_AddsChildAndIncreasesCount)
{
  occtl_topo_make_compound_info_t aCompInfo = OCCTL_TOPO_MAKE_COMPOUND_INFO_INIT;
  aCompInfo.children                        = nullptr;
  aCompInfo.child_count                     = 0;

  occtl_node_id_t aComp = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_compound(myGraph, &aCompInfo, &aComp), OCCTL_OK);
  ASSERT_NE(aComp.bits, 0u);

  const occtl_node_id_t aFace = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  ASSERT_EQ(occtl_topo_compound_add_child(myGraph, aComp, aFace, OCCTL_ORIENTATION_FORWARD),
            OCCTL_OK);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_compound_child_count(myGraph, aComp, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);
}

TEST_F(TopoComposeTest, CompoundRemoveChild_RemovesChildFromCompound)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  occtl_oriented_node_t           aChildren[1] = {{aFace, OCCTL_ORIENTATION_FORWARD}};
  occtl_topo_make_compound_info_t aCompInfo    = OCCTL_TOPO_MAKE_COMPOUND_INFO_INIT;
  aCompInfo.children                           = aChildren;
  aCompInfo.child_count                        = 1;

  occtl_node_id_t aComp = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_compound(myGraph, &aCompInfo, &aComp), OCCTL_OK);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_compound_child_count(myGraph, aComp, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);

  ASSERT_EQ(occtl_topo_compound_remove_child(myGraph, aComp, aFace), OCCTL_OK);

  ASSERT_EQ(occtl_topo_compound_child_count(myGraph, aComp, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 0u);
}

TEST_F(TopoComposeTest, EdgeSplit_SplitsEdgeAtParameter)
{
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  BRepGraph_EdgeId aBrepEdge;
  {
    const occtl_node_id_t  aPacked = anEdge;
    const BRepGraph_NodeId aNode   = OcctL::Topo::UnpackNodeId(aPacked);
    aBrepEdge                      = BRepGraph_EdgeId(aNode);
  }

  double aMid = 0.0;
  {
    auto aRange = BRepGraph_Tool::Edge::Range(myGraph->graph, aBrepEdge);
    aMid        = (aRange.first + aRange.second) * 0.5;
  }

  ASSERT_EQ(occtl_graph_count_value(occtl_graph_edge_count, myGraph), 12u); // box has 12 edges

  occtl_node_id_t aSub1 = OCCTL_NODE_ID_INVALID, aSub2 = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_edge_split(myGraph, anEdge, aMid, &aSub1, &aSub2), OCCTL_OK);
  ASSERT_NE(aSub1.bits, 0u);
  ASSERT_NE(aSub2.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSub1, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_EDGE);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSub2, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_EDGE);

  ASSERT_EQ(occtl_graph_count_value(occtl_graph_edge_count, myGraph),
            13u); // 12 - 1 removed + 2 new
}

TEST_F(TopoComposeTest, FaceAddHoles_AddsInnerWireReferences)
{
  const occtl_node_id_t aOuter = makeRectangleWire(myGraph, 0.0, 0.0, 10.0, 10.0);
  const occtl_node_id_t aHoleA = makeRectangleWire(myGraph, -2.0, 0.0, 2.0, 2.0);
  const occtl_node_id_t aHoleB = makeRectangleWire(myGraph, 2.0, 0.0, 1.0, 1.0);
  ASSERT_NE(aOuter.bits, 0u);
  ASSERT_NE(aHoleA.bits, 0u);
  ASSERT_NE(aHoleB.bits, 0u);

  const occtl_node_id_t aFace = makeFaceWithWires(myGraph, &aOuter, 1);
  ASSERT_NE(aFace.bits, 0u);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_face_wire_count(myGraph, aFace, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 1u);

  const occtl_node_id_t aHoles[2] = {aHoleA, aHoleB};
  ASSERT_EQ(occtl_topo_face_add_holes(myGraph, aFace, aHoles, 2), OCCTL_OK);

  ASSERT_EQ(occtl_topo_face_wire_count(myGraph, aFace, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 3u);
}

TEST_F(TopoComposeTest, FaceAddHoles_DuplicateInput_ReturnsInvalidArgument)
{
  const occtl_node_id_t aOuter = makeRectangleWire(myGraph, 0.0, 0.0, 10.0, 10.0);
  const occtl_node_id_t aHole  = makeRectangleWire(myGraph, 0.0, 0.0, 2.0, 2.0);
  ASSERT_NE(aOuter.bits, 0u);
  ASSERT_NE(aHole.bits, 0u);

  const occtl_node_id_t aFace = makeFaceWithWires(myGraph, &aOuter, 1);
  ASSERT_NE(aFace.bits, 0u);

  const occtl_node_id_t aHoles[2] = {aHole, aHole};
  EXPECT_EQ(occtl_topo_face_add_holes(myGraph, aFace, aHoles, 2), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoComposeTest, FaceRemoveHoles_AllInnerWires_RemovesHoles)
{
  const occtl_node_id_t aOuter = makeRectangleWire(myGraph, 0.0, 0.0, 10.0, 10.0);
  const occtl_node_id_t aHole  = makeRectangleWire(myGraph, 0.0, 0.0, 2.0, 2.0);
  ASSERT_NE(aOuter.bits, 0u);
  ASSERT_NE(aHole.bits, 0u);

  const occtl_node_id_t aWires[2] = {aHole, aOuter};
  const occtl_node_id_t aFace     = makeFaceWithWires(myGraph, aWires, 2);
  ASSERT_NE(aFace.bits, 0u);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_face_wire_count(myGraph, aFace, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 2u);

  ASSERT_EQ(occtl_topo_face_remove_holes(myGraph, aFace, nullptr, 0), OCCTL_OK);

  ASSERT_EQ(occtl_topo_face_wire_count(myGraph, aFace, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);
}

TEST_F(TopoComposeTest, FaceRemoveHoles_SelectedInnerWire_RemovesOnlyRequestedHole)
{
  const occtl_node_id_t aOuter = makeRectangleWire(myGraph, 0.0, 0.0, 10.0, 10.0);
  const occtl_node_id_t aHoleA = makeRectangleWire(myGraph, -2.0, 0.0, 2.0, 2.0);
  const occtl_node_id_t aHoleB = makeRectangleWire(myGraph, 2.0, 0.0, 1.0, 1.0);
  ASSERT_NE(aOuter.bits, 0u);
  ASSERT_NE(aHoleA.bits, 0u);
  ASSERT_NE(aHoleB.bits, 0u);

  const occtl_node_id_t aWires[3] = {aHoleA, aOuter, aHoleB};
  const occtl_node_id_t aFace     = makeFaceWithWires(myGraph, aWires, 3);
  ASSERT_NE(aFace.bits, 0u);

  ASSERT_EQ(occtl_topo_face_remove_holes(myGraph, aFace, &aHoleA, 1), OCCTL_OK);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_face_wire_count(myGraph, aFace, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 2u);
}

TEST_F(TopoComposeTest, FaceRemoveHoles_OuterWire_ReturnsNotFound)
{
  const occtl_node_id_t aOuter = makeRectangleWire(myGraph, 0.0, 0.0, 10.0, 10.0);
  const occtl_node_id_t aHole  = makeRectangleWire(myGraph, 0.0, 0.0, 2.0, 2.0);
  ASSERT_NE(aOuter.bits, 0u);
  ASSERT_NE(aHole.bits, 0u);

  const occtl_node_id_t aWires[2] = {aHole, aOuter};
  const occtl_node_id_t aFace     = makeFaceWithWires(myGraph, aWires, 2);
  ASSERT_NE(aFace.bits, 0u);

  EXPECT_EQ(occtl_topo_face_remove_holes(myGraph, aFace, &aOuter, 1), OCCTL_NOT_FOUND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoComposeTest, ShellAddFace_NullGraph_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  EXPECT_EQ(
    occtl_topo_shell_add_face(nullptr, OCCTL_NODE_ID_INVALID, aFace, OCCTL_ORIENTATION_FORWARD),
    OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

} // anonymous namespace
