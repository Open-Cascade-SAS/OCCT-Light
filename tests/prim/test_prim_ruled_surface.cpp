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

#include "test_prim_helpers.hxx"

#include <occtl-hpp/prim.hpp>
#include <occtl/occtl_curves.h>

#include <cmath>
#include <cstring>

namespace
{

occtl_node_id_t makeLineEdge(occtl_graph_t* const theGraph,
                             const occtl_point3_t theStart,
                             const occtl_point3_t theEnd)
{
  occtl_node_id_t aV0 = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t aV1 = OCCTL_NODE_ID_INVALID;

  occtl_topo_make_vertex_info_t aVertex = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertex.tolerance                     = 1.0e-7;
  aVertex.point                         = theStart;
  EXPECT_EQ(occtl_topo_make_vertex(theGraph, &aVertex, &aV0), OCCTL_OK);
  aVertex.point = theEnd;
  EXPECT_EQ(occtl_topo_make_vertex(theGraph, &aVertex, &aV1), OCCTL_OK);

  const double dx      = theEnd.x - theStart.x;
  const double dy      = theEnd.y - theStart.y;
  const double dz      = theEnd.z - theStart.z;
  const double aLength = occtl_point3_distance(theStart, theEnd);

  occtl_geom_line_t aLine  = {theStart, {dx / aLength, dy / aLength, dz / aLength}};
  occtl_rep_id_t    aCurve = {};
  EXPECT_EQ(occtl_curve_create_line(theGraph, aLine, &aCurve), OCCTL_OK);

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.start_vertex                = aV0;
  anEdgeInfo.end_vertex                  = aV1;
  anEdgeInfo.curve                       = aCurve;
  anEdgeInfo.first                       = 0.0;
  anEdgeInfo.last                        = aLength;
  anEdgeInfo.tolerance                   = 1.0e-7;

  occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_edge(theGraph, &anEdgeInfo, &anEdge), OCCTL_OK);
  return anEdge;
}

occtl_node_id_t makeSquareWire(occtl_graph_t* const theGraph,
                               const double         theZ,
                               const double         theSize)
{
  const occtl_point3_t aP[4] = {
    {0.0, 0.0, theZ},
    {theSize, 0.0, theZ},
    {theSize, theSize, theZ},
    {0.0, theSize, theZ},
  };

  occtl_node_id_t anEdges[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    anEdges[anI] = makeLineEdge(theGraph, aP[anI], aP[(anI + 1) % 4]);
  }

  occtl_oriented_node_t anOriented[4] = {
    {anEdges[0], OCCTL_ORIENTATION_FORWARD},
    {anEdges[1], OCCTL_ORIENTATION_FORWARD},
    {anEdges[2], OCCTL_ORIENTATION_FORWARD},
    {anEdges[3], OCCTL_ORIENTATION_FORWARD},
  };

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = anOriented;
  aWireInfo.edge_count                  = 4;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_wire(theGraph, &aWireInfo, &aWire), OCCTL_OK);
  return aWire;
}

class PrimRuledSurfaceTest : public ::testing::Test
{
protected:
  void SetUp() override { ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK); }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(PrimRuledSurfaceTest, RuledSurfaceInfoInit_HasDefaults)
{
  occtl_prim_ruled_surface_info_t anInfo{};
  occtl_prim_ruled_surface_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_RULED_SURFACE_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.section_a.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(anInfo.section_b.bits, OCCTL_NODE_ID_INVALID.bits);
}

TEST_F(PrimRuledSurfaceTest, RuledSurface_TwoEdges_ReturnsFace)
{
  const occtl_node_id_t anEdgeA = makeLineEdge(myGraph, {0.0, 0.0, 0.0}, {2.0, 0.0, 0.0});
  const occtl_node_id_t anEdgeB = makeLineEdge(myGraph, {0.0, 1.0, 1.0}, {2.0, 1.0, 1.0});

  occtl_prim_ruled_surface_info_t anInfo = OCCTL_PRIM_RULED_SURFACE_INFO_INIT;
  anInfo.section_a                       = anEdgeA;
  anInfo.section_b                       = anEdgeB;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_ruled_surface(myGraph, &anInfo, &aFace), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_VERTEX;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(PrimRuledSurfaceTest, RuledSurface_TwoWires_ReturnsShell)
{
  const occtl_node_id_t aWireA = makeSquareWire(myGraph, 0.0, 2.0);
  const occtl_node_id_t aWireB = makeSquareWire(myGraph, 3.0, 2.0);

  occtl_prim_ruled_surface_info_t anInfo = OCCTL_PRIM_RULED_SURFACE_INFO_INIT;
  anInfo.section_a                       = aWireA;
  anInfo.section_b                       = aWireB;

  occtl_node_id_t aShell = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_ruled_surface(myGraph, &anInfo, &aShell), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_VERTEX;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShell, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SHELL);
}

TEST_F(PrimRuledSurfaceTest, RuledSurface_NullPointers_ReturnInvalidArgument)
{
  occtl_prim_ruled_surface_info_t anInfo = OCCTL_PRIM_RULED_SURFACE_INFO_INIT;
  occtl_node_id_t                 aShape = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_ruled_surface(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_ruled_surface(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_ruled_surface(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimRuledSurfaceTest, RuledSurface_VersionMismatch_ReturnsVersionMismatch)
{
  occtl_prim_ruled_surface_info_t anInfo = OCCTL_PRIM_RULED_SURFACE_INFO_INIT;
  anInfo.struct_version                  = 0u;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_ruled_surface(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimRuledSurfaceTest, RuledSurface_MixedKinds_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdge = makeLineEdge(myGraph, {0.0, 0.0, 0.0}, {2.0, 0.0, 0.0});
  const occtl_node_id_t aWire  = makeSquareWire(myGraph, 2.0, 2.0);

  occtl_prim_ruled_surface_info_t anInfo = OCCTL_PRIM_RULED_SURFACE_INFO_INIT;
  anInfo.section_a                       = anEdge;
  anInfo.section_b                       = aWire;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_ruled_surface(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimRuledSurfaceTest, RuledSurface_WireEdgeCountMismatch_ReturnsInvalidArgument)
{
  const occtl_node_id_t aWireA = makeSquareWire(myGraph, 0.0, 2.0);

  const occtl_node_id_t       anEdge     = makeLineEdge(myGraph, {0.0, 0.0, 3.0}, {2.0, 0.0, 3.0});
  occtl_oriented_node_t       anOriented = {anEdge, OCCTL_ORIENTATION_FORWARD};
  occtl_topo_make_wire_info_t aWireInfo  = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                        = &anOriented;
  aWireInfo.edge_count                   = 1;
  occtl_node_id_t aWireB                 = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWireB), OCCTL_OK);

  occtl_prim_ruled_surface_info_t anInfo = OCCTL_PRIM_RULED_SURFACE_INFO_INIT;
  anInfo.section_a                       = aWireA;
  anInfo.section_b                       = aWireB;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_ruled_surface(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimRuledSurfaceTest, RuledSurface_WrongKind_ReturnsWrongKind)
{
  occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
  aBox.dx                    = 1.0;
  aBox.dy                    = 1.0;
  aBox.dz                    = 1.0;
  occtl_node_id_t aSolid     = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &aSolid), OCCTL_OK);

  occtl_prim_ruled_surface_info_t anInfo = OCCTL_PRIM_RULED_SURFACE_INFO_INIT;
  anInfo.section_a                       = aSolid;
  anInfo.section_b                       = aSolid;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_ruled_surface(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

TEST(PrimRuledSurfaceVeneerTest, MakeRuledSurface_TwoEdges_ReturnsFace)
{
  occtl::Graph        aGraph;
  const occtl::NodeId anEdgeA(makeLineEdge(aGraph.get(), {0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}));
  const occtl::NodeId anEdgeB(makeLineEdge(aGraph.get(), {0.0, 1.0, 1.0}, {2.0, 1.0, 1.0}));

  const occtl::NodeId aFace = occtl::prim::make_ruled_surface(aGraph, anEdgeA, anEdgeB);
  EXPECT_NE(aFace.get().bits, OCCTL_NODE_ID_INVALID.bits);
}

} // namespace
