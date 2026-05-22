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
#include <occtl/occtl_geom.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_surfaces.h>
#include <occtl/occtl_topo.h>

#include "test_helpers.hxx"

#include <cmath>

namespace
{

class TopoMutationTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);
    ASSERT_NE(myGraph, nullptr);
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
                                  const double         theXMin,
                                  const double         theYMin,
                                  const double         theWidth,
                                  const double         theHeight)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  const double    aCoords[4][2] = {{theXMin, theYMin},
                                   {theXMin + theWidth, theYMin},
                                   {theXMin + theWidth, theYMin + theHeight},
                                   {theXMin, theYMin + theHeight}};
  occtl_node_id_t aVertices[4]  = {OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID};
  for (int anI = 0; anI < 4; ++anI)
  {
    aVertInfo.point = {aCoords[anI][0], aCoords[anI][1], 0.0};
    EXPECT_EQ(occtl_topo_make_vertex(theGraph, &aVertInfo, &aVertices[anI]), OCCTL_OK);
  }

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_oriented_node_t aEdges[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    occtl_node_id_t anEdge  = OCCTL_NODE_ID_INVALID;
    anEdgeInfo.start_vertex = aVertices[anI];
    anEdgeInfo.end_vertex   = aVertices[(anI + 1) % 4];
    anEdgeInfo.first        = 0.0;
    anEdgeInfo.last         = (anI % 2 == 0) ? theWidth : theHeight;
    EXPECT_EQ(occtl_topo_make_edge(theGraph, &anEdgeInfo, &anEdge), OCCTL_OK);
    aEdges[anI].id          = anEdge;
    aEdges[anI].orientation = OCCTL_ORIENTATION_FORWARD;
  }

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = aEdges;
  aWireInfo.edge_count                  = 4;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_wire(theGraph, &aWireInfo, &aWire), OCCTL_OK);
  return aWire;
}

TEST_F(TopoMutationTest, MakeVertex_CreatesVertexAtPoint)
{
  occtl_topo_make_vertex_info_t aInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aInfo.point                         = {1.0, 2.0, 3.0};
  aInfo.tolerance                     = 1e-6;

  occtl_node_id_t aVertex = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aInfo, &aVertex), OCCTL_OK);
  ASSERT_NE(aVertex.bits, 0u);

  occtl_point3_t aPoint;
  ASSERT_EQ(occtl_topo_vertex_point(myGraph, aVertex, &aPoint), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aPoint.x, 1.0);
  EXPECT_DOUBLE_EQ(aPoint.y, 2.0);
  EXPECT_DOUBLE_EQ(aPoint.z, 3.0);

  double aTol = 0.0;
  ASSERT_EQ(occtl_topo_vertex_tolerance(myGraph, aVertex, &aTol), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aTol, 1e-6);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aVertex, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_VERTEX);
}

TEST_F(TopoMutationTest, MakeEdge_BetweenTwoVertices_CreatesEdge)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  aVertInfo.point        = {0.0, 0.0, 0.0};
  occtl_node_id_t aStart = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aStart), OCCTL_OK);

  aVertInfo.point       = {10.0, 0.0, 0.0};
  occtl_node_id_t anEnd = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &anEnd), OCCTL_OK);

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.start_vertex                = aStart;
  anEdgeInfo.end_vertex                  = anEnd;
  anEdgeInfo.curve                       = {};
  anEdgeInfo.first                       = 0.0;
  anEdgeInfo.last                        = 10.0;
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &anEdge), OCCTL_OK);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_node_id_t aStartOut = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_edge_start_vertex(myGraph, anEdge, &aStartOut), OCCTL_OK);
  EXPECT_EQ(aStartOut.bits, aStart.bits);

  occtl_node_id_t anEndOut = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_edge_end_vertex(myGraph, anEdge, &anEndOut), OCCTL_OK);
  EXPECT_EQ(anEndOut.bits, anEnd.bits);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, anEdge, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_EDGE);
}

TEST_F(TopoMutationTest, MakeEdge_WithCurve_CreatesCurvedEdge)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  aVertInfo.point        = {0.0, 0.0, 0.0};
  occtl_node_id_t aStart = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aStart), OCCTL_OK);

  aVertInfo.point       = {10.0, 0.0, 0.0};
  occtl_node_id_t anEnd = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &anEnd), OCCTL_OK);

  occtl_geom_line_t aLineData = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t    aCurve    = {};
  ASSERT_EQ(occtl_curve_create_line(myGraph, aLineData, &aCurve), OCCTL_OK);
  ASSERT_NE(aCurve.bits, 0u);

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.start_vertex                = aStart;
  anEdgeInfo.end_vertex                  = anEnd;
  anEdgeInfo.curve                       = aCurve;
  anEdgeInfo.first                       = 0.0;
  anEdgeInfo.last                        = 10.0;
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &anEdge), OCCTL_OK);
  ASSERT_NE(anEdge.bits, 0u);

  int32_t aHas = 0;
  ASSERT_EQ(occtl_topo_edge_has_curve(myGraph, anEdge, &aHas), OCCTL_OK);
  EXPECT_EQ(aHas, 1);
}

TEST_F(TopoMutationTest, MakeWire_FromFourEdges_CreatesClosedWire)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aV[4]         = {OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID};
  const double    aCoords[4][2] = {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}};
  for (int anI = 0; anI < 4; ++anI)
  {
    aVertInfo.point = {aCoords[anI][0], aCoords[anI][1], 0.0};
    ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[anI]), OCCTL_OK);
  }

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.curve                       = {};
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t aE[4] = {OCCTL_NODE_ID_INVALID,
                           OCCTL_NODE_ID_INVALID,
                           OCCTL_NODE_ID_INVALID,
                           OCCTL_NODE_ID_INVALID};
  for (int anI = 0; anI < 4; ++anI)
  {
    anEdgeInfo.start_vertex = aV[anI];
    anEdgeInfo.end_vertex   = aV[(anI + 1) % 4];
    anEdgeInfo.first        = 0.0;
    anEdgeInfo.last         = 10.0;
    ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &aE[anI]), OCCTL_OK);
  }

  occtl_oriented_node_t aEdges[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    aEdges[anI].id          = aE[anI];
    aEdges[anI].orientation = OCCTL_ORIENTATION_FORWARD;
  }

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = aEdges;
  aWireInfo.edge_count                  = 4;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWire), OCCTL_OK);
  ASSERT_NE(aWire.bits, 0u);

  int32_t aClosed = 0;
  ASSERT_EQ(occtl_topo_wire_is_closed(myGraph, aWire, &aClosed), OCCTL_OK);
  EXPECT_EQ(aClosed, 1);

  uint32_t aNbCoedges = 0;
  ASSERT_EQ(occtl_topo_wire_coedge_count(myGraph, aWire, &aNbCoedges), OCCTL_OK);
  EXPECT_EQ(aNbCoedges, 4u);
}

TEST_F(TopoMutationTest, EdgesToWires_UnorderedRectangle_CreatesSingleClosedWire)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aV[4]         = {OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID};
  const double    aCoords[4][2] = {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}};
  for (int anI = 0; anI < 4; ++anI)
  {
    aVertInfo.point = {aCoords[anI][0], aCoords[anI][1], 0.0};
    ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[anI]), OCCTL_OK);
  }

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t aE[4] = {OCCTL_NODE_ID_INVALID,
                           OCCTL_NODE_ID_INVALID,
                           OCCTL_NODE_ID_INVALID,
                           OCCTL_NODE_ID_INVALID};
  for (int anI = 0; anI < 4; ++anI)
  {
    anEdgeInfo.start_vertex = aV[anI];
    anEdgeInfo.end_vertex   = aV[(anI + 1) % 4];
    anEdgeInfo.first        = 0.0;
    anEdgeInfo.last         = 10.0;
    ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &aE[anI]), OCCTL_OK);
  }

  const occtl_node_id_t               aShuffled[4] = {aE[2], aE[0], aE[3], aE[1]};
  occtl_topo_edges_to_wires_options_t anOptions    = OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_INIT;
  anOptions.edges                                  = aShuffled;
  anOptions.edge_count                             = 4;

  const size_t aWireCountBefore = occtl_graph_count_value(occtl_graph_wire_count, myGraph);
  size_t       aCount           = 0;
  ASSERT_EQ(occtl_topo_edges_to_wires(myGraph, &anOptions, nullptr, 0, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_wire_count, myGraph), aWireCountBefore);

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_edges_to_wires(myGraph, &anOptions, &aWire, 1, &aCount), OCCTL_OK);
  ASSERT_NE(aWire.bits, 0u);
  EXPECT_EQ(aCount, 1u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_wire_count, myGraph), aWireCountBefore + 1u);

  int32_t aClosed = 0;
  ASSERT_EQ(occtl_topo_wire_is_closed(myGraph, aWire, &aClosed), OCCTL_OK);
  EXPECT_EQ(aClosed, 1);

  uint32_t aNbCoedges = 0;
  ASSERT_EQ(occtl_topo_wire_coedge_count(myGraph, aWire, &aNbCoedges), OCCTL_OK);
  EXPECT_EQ(aNbCoedges, 4u);
}

TEST_F(TopoMutationTest, EdgesToWires_BufferTooSmall_DoesNotMutate)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aV[2] = {OCCTL_NODE_ID_INVALID, OCCTL_NODE_ID_INVALID};
  aVertInfo.point       = {0.0, 0.0, 0.0};
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[0]), OCCTL_OK);
  aVertInfo.point = {10.0, 0.0, 0.0};
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[1]), OCCTL_OK);

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.start_vertex                = aV[0];
  anEdgeInfo.end_vertex                  = aV[1];
  anEdgeInfo.first                       = 0.0;
  anEdgeInfo.last                        = 10.0;
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &anEdge), OCCTL_OK);

  occtl_topo_edges_to_wires_options_t anOptions = OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_INIT;
  anOptions.edges                               = &anEdge;
  anOptions.edge_count                          = 1;

  occtl_node_id_t aWire  = OCCTL_NODE_ID_INVALID;
  size_t          aCount = 0;
  EXPECT_EQ(occtl_topo_edges_to_wires(myGraph, &anOptions, &aWire, 0, &aCount),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aCount, 1u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_wire_count, myGraph), 0u);
}

TEST_F(TopoMutationTest, EdgesToWires_OpenRejectedWhenDisallowed)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aV[3] = {OCCTL_NODE_ID_INVALID, OCCTL_NODE_ID_INVALID, OCCTL_NODE_ID_INVALID};
  for (int anI = 0; anI < 3; ++anI)
  {
    aVertInfo.point = {static_cast<double>(anI), 0.0, 0.0};
    ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[anI]), OCCTL_OK);
  }

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t aE[2] = {OCCTL_NODE_ID_INVALID, OCCTL_NODE_ID_INVALID};
  for (int anI = 0; anI < 2; ++anI)
  {
    anEdgeInfo.start_vertex = aV[anI];
    anEdgeInfo.end_vertex   = aV[anI + 1];
    anEdgeInfo.first        = 0.0;
    anEdgeInfo.last         = 1.0;
    ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &aE[anI]), OCCTL_OK);
  }

  occtl_topo_edges_to_wires_options_t anOptions = OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_INIT;
  anOptions.edges                               = aE;
  anOptions.edge_count                          = 2;
  anOptions.allow_open                          = 0;

  size_t aCount = 0;
  EXPECT_EQ(occtl_topo_edges_to_wires(myGraph, &anOptions, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_wire_count, myGraph), 0u);
}

TEST_F(TopoMutationTest, EdgesToWires_BadVersion_VersionMismatch)
{
  occtl_topo_edges_to_wires_options_t anOptions = OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_INIT;
  anOptions.struct_version                      = 999;

  size_t aCount = 0;
  EXPECT_EQ(occtl_topo_edges_to_wires(myGraph, &anOptions, nullptr, 0, &aCount),
            OCCTL_VERSION_MISMATCH);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, EdgesToWires_WrongKindInput_WrongKind)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.point                         = {0.0, 0.0, 0.0};
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aVertex = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aVertex), OCCTL_OK);

  occtl_topo_edges_to_wires_options_t anOptions = OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_INIT;
  anOptions.edges                               = &aVertex;
  anOptions.edge_count                          = 1;

  size_t aCount = 0;
  EXPECT_EQ(occtl_topo_edges_to_wires(myGraph, &anOptions, nullptr, 0, &aCount), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, WireOffset2d_InitDefaults)
{
  occtl_topo_wire_offset_2d_options_t anOptions{};
  occtl_topo_wire_offset_2d_options_init(&anOptions);

  EXPECT_EQ(anOptions.struct_version, OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_VERSION_1);
  EXPECT_EQ(anOptions.p_next, nullptr);
  EXPECT_EQ(anOptions.wire.bits, 0u);
  EXPECT_DOUBLE_EQ(anOptions.distance, 1.0);
  EXPECT_EQ(anOptions.join, OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC);
  EXPECT_EQ(anOptions.open_result, 0);
  EXPECT_EQ(anOptions.approximate, 0);

  occtl_topo_wire_offset_2d_options_init(nullptr);
}

TEST_F(TopoMutationTest, WireOffset2d_RectangleWire_CreatesOffsetWire)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 4.0;
  aRect.height                      = 2.0;

  occtl_node_id_t aSourceWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aSourceWire), OCCTL_OK);
  ASSERT_NE(aSourceWire.bits, 0u);

  occtl_topo_wire_offset_2d_options_t anOptions = OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_INIT;
  anOptions.wire                                = aSourceWire;
  anOptions.distance                            = 0.25;
  anOptions.join                                = OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION;

  const size_t    aWireCountBefore = occtl_graph_count_value(occtl_graph_wire_count, myGraph);
  occtl_node_id_t aOffsetWire      = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_wire_offset_2d(myGraph, &anOptions, &aOffsetWire), OCCTL_OK);
  ASSERT_NE(aOffsetWire.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_wire_count, myGraph), aWireCountBefore);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aOffsetWire, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_WIRE);

  uint32_t aNbCoedges = 0;
  ASSERT_EQ(occtl_topo_wire_coedge_count(myGraph, aOffsetWire, &aNbCoedges), OCCTL_OK);
  EXPECT_GT(aNbCoedges, 0u);
}

TEST_F(TopoMutationTest, WireOffset2d_ZeroDistance_ReturnsInvalidArgument)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 4.0;
  aRect.height                      = 2.0;

  occtl_node_id_t aSourceWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aSourceWire), OCCTL_OK);

  occtl_topo_wire_offset_2d_options_t anOptions = OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_INIT;
  anOptions.wire                                = aSourceWire;
  anOptions.distance                            = 0.0;

  occtl_node_id_t aOffsetWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_wire_offset_2d(myGraph, &anOptions, &aOffsetWire), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, WireOffset2d_WrongKind_ReturnsWrongKind)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.point                         = {0.0, 0.0, 0.0};
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aVertex = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aVertex), OCCTL_OK);

  occtl_topo_wire_offset_2d_options_t anOptions = OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_INIT;
  anOptions.wire                                = aVertex;

  occtl_node_id_t aOffsetWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_wire_offset_2d(myGraph, &anOptions, &aOffsetWire), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, WireFixDegenerateEdges_InitDefaults)
{
  occtl_topo_wire_fix_degenerate_edges_options_t anOptions{};
  occtl_topo_wire_fix_degenerate_options_init(&anOptions);

  EXPECT_EQ(anOptions.struct_version, OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_VERSION_1);
  EXPECT_EQ(anOptions.p_next, nullptr);
  EXPECT_EQ(anOptions.wire.bits, 0u);
  EXPECT_DOUBLE_EQ(anOptions.min_length, 1.0e-9);

  occtl_topo_wire_fix_degenerate_options_init(nullptr);
}

TEST_F(TopoMutationTest, WireFixDegenerateEdges_OpenWire_RemovesShortUsage)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1.0e-7;

  occtl_node_id_t aV[4]      = {OCCTL_NODE_ID_INVALID,
                                OCCTL_NODE_ID_INVALID,
                                OCCTL_NODE_ID_INVALID,
                                OCCTL_NODE_ID_INVALID};
  const double    aCoords[4] = {0.0, 1.0e-5, 2.0, 4.0};
  for (int anI = 0; anI < 4; ++anI)
  {
    aVertInfo.point = {aCoords[anI], 0.0, 0.0};
    ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[anI]), OCCTL_OK);
  }

  occtl_geom_line_t aLineData = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t    aCurve    = {};
  ASSERT_EQ(occtl_curve_create_line(myGraph, aLineData, &aCurve), OCCTL_OK);
  ASSERT_NE(aCurve.bits, 0u);

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.curve                       = aCurve;
  anEdgeInfo.tolerance                   = 1.0e-7;

  occtl_oriented_node_t aEdges[3];
  for (int anI = 0; anI < 3; ++anI)
  {
    occtl_node_id_t anEdge  = OCCTL_NODE_ID_INVALID;
    anEdgeInfo.start_vertex = aV[anI];
    anEdgeInfo.end_vertex   = aV[anI + 1];
    anEdgeInfo.first        = aCoords[anI];
    anEdgeInfo.last         = aCoords[anI + 1];
    ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &anEdge), OCCTL_OK);
    aEdges[anI] = {anEdge, OCCTL_ORIENTATION_FORWARD};
  }

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = aEdges;
  aWireInfo.edge_count                  = 3;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWire), OCCTL_OK);

  occtl_topo_wire_fix_degenerate_edges_options_t anOptions =
    OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_INIT;
  anOptions.wire       = aWire;
  anOptions.min_length = 1.0e-4;

  size_t aRemoved = 0;
  ASSERT_EQ(occtl_topo_wire_fix_degenerate(myGraph, &anOptions, &aRemoved), OCCTL_OK);
  EXPECT_EQ(aRemoved, 1u);

  uint32_t aNbCoedges = 0;
  ASSERT_EQ(occtl_topo_wire_coedge_count(myGraph, aWire, &aNbCoedges), OCCTL_OK);
  EXPECT_EQ(aNbCoedges, 2u);

  int32_t aClosed = 1;
  ASSERT_EQ(occtl_topo_wire_is_closed(myGraph, aWire, &aClosed), OCCTL_OK);
  EXPECT_EQ(aClosed, 0);
}

TEST_F(TopoMutationTest, WireFixDegenerateEdges_NegativeLength_ReturnsInvalidArgument)
{
  occtl_topo_wire_fix_degenerate_edges_options_t anOptions =
    OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_INIT;
  anOptions.min_length = -1.0;

  size_t aRemoved = 99;
  EXPECT_EQ(occtl_topo_wire_fix_degenerate(myGraph, &anOptions, &aRemoved), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aRemoved, 0u);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, FaceChamfer2d_InitDefaults)
{
  occtl_topo_face_chamfer_2d_options_t anOptions{};
  occtl_topo_face_chamfer_2d_options_init(&anOptions);

  EXPECT_EQ(anOptions.struct_version, OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_VERSION_1);
  EXPECT_EQ(anOptions.p_next, nullptr);
  EXPECT_EQ(anOptions.face.bits, 0u);
  EXPECT_EQ(anOptions.vertices, nullptr);
  EXPECT_EQ(anOptions.vertex_count, 0u);
  EXPECT_DOUBLE_EQ(anOptions.distance1, 1.0);
  EXPECT_DOUBLE_EQ(anOptions.distance2, 1.0);

  occtl_topo_face_chamfer_2d_options_init(nullptr);
}

TEST_F(TopoMutationTest, WireChamfer2d_InitDefaults)
{
  occtl_topo_wire_chamfer_2d_options_t anOptions{};
  occtl_topo_wire_chamfer_2d_options_init(&anOptions);

  EXPECT_EQ(anOptions.struct_version, OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_VERSION_1);
  EXPECT_EQ(anOptions.p_next, nullptr);
  EXPECT_EQ(anOptions.wire.bits, 0u);
  EXPECT_EQ(anOptions.vertices, nullptr);
  EXPECT_EQ(anOptions.vertex_count, 0u);
  EXPECT_DOUBLE_EQ(anOptions.distance1, 1.0);
  EXPECT_DOUBLE_EQ(anOptions.distance2, 1.0);

  occtl_topo_wire_chamfer_2d_options_init(nullptr);
}

TEST_F(TopoMutationTest, WireChamfer2d_RectangleWire_CreatesChamferedWire)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 8.0;
  aRect.height                      = 4.0;

  occtl_node_id_t aSourceWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aSourceWire), OCCTL_OK);
  ASSERT_NE(aSourceWire.bits, 0u);

  occtl_topo_wire_chamfer_2d_options_t anOptions = OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_INIT;
  anOptions.wire                                 = aSourceWire;
  anOptions.distance1                            = 0.5;
  anOptions.distance2                            = 0.5;

  const size_t    aWireCountBefore = occtl_graph_count_value(occtl_graph_wire_count, myGraph);
  occtl_node_id_t aChamferedWire   = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_wire_chamfer_2d(myGraph, &anOptions, &aChamferedWire), OCCTL_OK);
  ASSERT_NE(aChamferedWire.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_wire_count, myGraph), aWireCountBefore);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aChamferedWire, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_WIRE);

  uint32_t aNbCoedges = 0;
  ASSERT_EQ(occtl_topo_wire_coedge_count(myGraph, aChamferedWire, &aNbCoedges), OCCTL_OK);
  EXPECT_GT(aNbCoedges, 4u);
}

TEST_F(TopoMutationTest, WireChamfer2d_SelectedVertex_CreatesChamferedWire)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 8.0;
  aRect.height                      = 4.0;

  occtl_node_id_t aSourceWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aSourceWire), OCCTL_OK);

  const occtl_node_id_t aVertex = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertex.bits, 0u);

  occtl_topo_wire_chamfer_2d_options_t anOptions = OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_INIT;
  anOptions.wire                                 = aSourceWire;
  anOptions.vertices                             = &aVertex;
  anOptions.vertex_count                         = 1;
  anOptions.distance1                            = 0.5;
  anOptions.distance2                            = 0.25;

  occtl_node_id_t aChamferedWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_wire_chamfer_2d(myGraph, &anOptions, &aChamferedWire), OCCTL_OK);
  ASSERT_NE(aChamferedWire.bits, 0u);
}

TEST_F(TopoMutationTest, WireChamfer2d_BadVersion_ReturnsVersionMismatch)
{
  occtl_topo_wire_chamfer_2d_options_t anOptions = OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_INIT;
  anOptions.struct_version                       = 999u;

  occtl_node_id_t aChamferedWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_wire_chamfer_2d(myGraph, &anOptions, &aChamferedWire),
            OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aChamferedWire.bits, 0u);
}

TEST_F(TopoMutationTest, WireChamfer2d_WrongKind_ReturnsWrongKind)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 4.0;
  aRect.height                      = 2.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aWire), OCCTL_OK);

  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = aWire;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &aFace), OCCTL_OK);

  occtl_topo_wire_chamfer_2d_options_t anOptions = OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_INIT;
  anOptions.wire                                 = aFace;

  occtl_node_id_t aChamferedWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_wire_chamfer_2d(myGraph, &anOptions, &aChamferedWire), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, FaceChamfer2d_RectangleFace_CreatesChamferedFace)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 10.0;
  aRect.height                      = 10.0;

  occtl_node_id_t aRectWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aRectWire), OCCTL_OK);

  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = aRectWire;

  occtl_node_id_t aSourceFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &aSourceFace), OCCTL_OK);
  ASSERT_NE(aSourceFace.bits, 0u);

  occtl_topo_face_chamfer_2d_options_t anOptions = OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_INIT;
  anOptions.face                                 = aSourceFace;
  anOptions.distance1                            = 1.0;
  anOptions.distance2                            = 1.0;

  const size_t    aFaceCountBefore = occtl_graph_count_value(occtl_graph_face_count, myGraph);
  occtl_node_id_t aChamferedFace   = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_face_chamfer_2d(myGraph, &anOptions, &aChamferedFace), OCCTL_OK);
  ASSERT_NE(aChamferedFace.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, myGraph), aFaceCountBefore);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aChamferedFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);

  uint32_t aWireCount = 0;
  ASSERT_EQ(occtl_topo_face_wire_count(myGraph, aChamferedFace, &aWireCount), OCCTL_OK);
  EXPECT_EQ(aWireCount, 1u);
}

TEST_F(TopoMutationTest, FaceChamfer2d_SelectedVertex_CreatesChamferedFace)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 10.0;
  aRect.height                      = 10.0;

  occtl_node_id_t aRectWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aRectWire), OCCTL_OK);

  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = aRectWire;

  occtl_node_id_t aSourceFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &aSourceFace), OCCTL_OK);

  const occtl_node_id_t aVertex = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertex.bits, 0u);

  occtl_topo_face_chamfer_2d_options_t anOptions = OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_INIT;
  anOptions.face                                 = aSourceFace;
  anOptions.vertices                             = &aVertex;
  anOptions.vertex_count                         = 1;
  anOptions.distance1                            = 1.0;
  anOptions.distance2                            = 0.5;

  occtl_node_id_t aChamferedFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_face_chamfer_2d(myGraph, &anOptions, &aChamferedFace), OCCTL_OK);
  ASSERT_NE(aChamferedFace.bits, 0u);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aChamferedFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(TopoMutationTest, FaceChamfer2d_NegativeDistance_ReturnsInvalidArgument)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 10.0;
  aRect.height                      = 10.0;

  occtl_node_id_t aRectWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aRectWire), OCCTL_OK);

  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = aRectWire;

  occtl_node_id_t aSourceFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &aSourceFace), OCCTL_OK);

  occtl_topo_face_chamfer_2d_options_t anOptions = OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_INIT;
  anOptions.face                                 = aSourceFace;
  anOptions.distance1                            = -1.0;

  occtl_node_id_t aChamferedFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_face_chamfer_2d(myGraph, &anOptions, &aChamferedFace),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, FaceChamfer2d_WrongKind_ReturnsWrongKind)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 4.0;
  aRect.height                      = 2.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aWire), OCCTL_OK);

  occtl_topo_face_chamfer_2d_options_t anOptions = OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_INIT;
  anOptions.face                                 = aWire;

  occtl_node_id_t aChamferedFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_face_chamfer_2d(myGraph, &anOptions, &aChamferedFace), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, MakeFace_WithPlaneSurface_CreatesFace)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aV[4]         = {OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID,
                                   OCCTL_NODE_ID_INVALID};
  const double    aCoords[4][2] = {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}};
  for (int anI = 0; anI < 4; ++anI)
  {
    aVertInfo.point = {aCoords[anI][0], aCoords[anI][1], 0.0};
    ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[anI]), OCCTL_OK);
  }

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.curve                       = {};
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t aE[4] = {OCCTL_NODE_ID_INVALID,
                           OCCTL_NODE_ID_INVALID,
                           OCCTL_NODE_ID_INVALID,
                           OCCTL_NODE_ID_INVALID};
  for (int anI = 0; anI < 4; ++anI)
  {
    anEdgeInfo.start_vertex = aV[anI];
    anEdgeInfo.end_vertex   = aV[(anI + 1) % 4];
    anEdgeInfo.first        = 0.0;
    anEdgeInfo.last         = 10.0;
    ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &aE[anI]), OCCTL_OK);
  }

  occtl_oriented_node_t aEdges[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    aEdges[anI].id          = aE[anI];
    aEdges[anI].orientation = OCCTL_ORIENTATION_FORWARD;
  }

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = aEdges;
  aWireInfo.edge_count                  = 4;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWire), OCCTL_OK);

  occtl_geom_plane_t aPlaneData = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(occtl_surface_create_plane(myGraph, &aSurfaceId, aPlaneData), OCCTL_OK);
  ASSERT_NE(aSurfaceId.bits, 0u);

  occtl_topo_make_face_info_t aFaceInfo = OCCTL_TOPO_MAKE_FACE_INFO_INIT;
  aFaceInfo.surface                     = aSurfaceId;
  aFaceInfo.outer_wire                  = aWire;
  aFaceInfo.inner_wires                 = nullptr;
  aFaceInfo.inner_wire_count            = 0;
  aFaceInfo.tolerance                   = 1e-6;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_face(myGraph, &aFaceInfo, &aFace), OCCTL_OK);
  ASSERT_NE(aFace.bits, 0u);

  uint32_t aNbWires = 0;
  ASSERT_EQ(occtl_topo_face_wire_count(myGraph, aFace, &aNbWires), OCCTL_OK);
  EXPECT_EQ(aNbWires, 1u);

  occtl_node_id_t anOuterWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_face_outer_wire(myGraph, aFace, &anOuterWire), OCCTL_OK);
  EXPECT_EQ(anOuterWire.bits, aWire.bits);

  int32_t aHasSurface = 0;
  ASSERT_EQ(occtl_topo_face_has_surface(myGraph, aFace, &aHasSurface), OCCTL_OK);
  EXPECT_EQ(aHasSurface, 1);
}

TEST_F(TopoMutationTest, MakeFaceFromWiresAuto_InnerThenOuter_ChoosesLargestOuter)
{
  const occtl_node_id_t anOuterWire = makeRectangleWire(myGraph, 0.0, 0.0, 10.0, 10.0);
  const occtl_node_id_t anInnerWire = makeRectangleWire(myGraph, 3.0, 3.0, 2.0, 2.0);
  ASSERT_NE(anOuterWire.bits, 0u);
  ASSERT_NE(anInnerWire.bits, 0u);

  const occtl_node_id_t aWires[2]  = {anInnerWire, anOuterWire};
  occtl_geom_plane_t    aPlaneData = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(occtl_surface_create_plane(myGraph, &aSurfaceId, aPlaneData), OCCTL_OK);
  ASSERT_NE(aSurfaceId.bits, 0u);

  occtl_topo_make_face_from_wires_auto_options_t anOptions =
    OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_INIT;
  anOptions.surface    = aSurfaceId;
  anOptions.wires      = aWires;
  anOptions.wire_count = 2;
  anOptions.tolerance  = 1e-6;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_face_from_wires_auto(myGraph, &anOptions, &aFace), OCCTL_OK);
  ASSERT_NE(aFace.bits, 0u);

  occtl_node_id_t aDetectedOuter = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_face_outer_wire(myGraph, aFace, &aDetectedOuter), OCCTL_OK);
  EXPECT_EQ(aDetectedOuter.bits, anOuterWire.bits);

  uint32_t aNbWires = 0;
  ASSERT_EQ(occtl_topo_face_wire_count(myGraph, aFace, &aNbWires), OCCTL_OK);
  EXPECT_EQ(aNbWires, 2u);
}

TEST_F(TopoMutationTest, MakeFaceFromWiresAuto_EqualAreas_ReturnsInvalidArgument)
{
  const occtl_node_id_t aWireA = makeRectangleWire(myGraph, 0.0, 0.0, 2.0, 2.0);
  const occtl_node_id_t aWireB = makeRectangleWire(myGraph, 4.0, 0.0, 2.0, 2.0);
  ASSERT_NE(aWireA.bits, 0u);
  ASSERT_NE(aWireB.bits, 0u);

  const occtl_node_id_t aWires[2]  = {aWireA, aWireB};
  occtl_geom_plane_t    aPlaneData = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(occtl_surface_create_plane(myGraph, &aSurfaceId, aPlaneData), OCCTL_OK);
  ASSERT_NE(aSurfaceId.bits, 0u);

  occtl_topo_make_face_from_wires_auto_options_t anOptions =
    OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_INIT;
  anOptions.surface    = aSurfaceId;
  anOptions.wires      = aWires;
  anOptions.wire_count = 2;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_face_from_wires_auto(myGraph, &anOptions, &aFace),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, MakeFaceFromWiresAuto_BadVersion_VersionMismatch)
{
  occtl_topo_make_face_from_wires_auto_options_t anOptions =
    OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_INIT;
  anOptions.struct_version = 999;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_face_from_wires_auto(myGraph, &anOptions, &aFace),
            OCCTL_VERSION_MISMATCH);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoMutationTest, MakeShell_FromOneFace_CreatesShell)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aV[4];
  const double    aCoords[4][2] = {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}};
  for (int anI = 0; anI < 4; ++anI)
  {
    aVertInfo.point = {aCoords[anI][0], aCoords[anI][1], 0.0};
    ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[anI]), OCCTL_OK);
  }

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.curve                       = {};
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t aE[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    anEdgeInfo.start_vertex = aV[anI];
    anEdgeInfo.end_vertex   = aV[(anI + 1) % 4];
    anEdgeInfo.first        = 0.0;
    anEdgeInfo.last         = 10.0;
    ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &aE[anI]), OCCTL_OK);
  }

  occtl_oriented_node_t aEdges[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    aEdges[anI].id          = aE[anI];
    aEdges[anI].orientation = OCCTL_ORIENTATION_FORWARD;
  }

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = aEdges;
  aWireInfo.edge_count                  = 4;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWire), OCCTL_OK);

  occtl_geom_plane_t aPlaneData = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(occtl_surface_create_plane(myGraph, &aSurfaceId, aPlaneData), OCCTL_OK);

  occtl_topo_make_face_info_t aFaceInfo = OCCTL_TOPO_MAKE_FACE_INFO_INIT;
  aFaceInfo.surface                     = aSurfaceId;
  aFaceInfo.outer_wire                  = aWire;
  aFaceInfo.inner_wires                 = nullptr;
  aFaceInfo.inner_wire_count            = 0;
  aFaceInfo.tolerance                   = 1e-6;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_face(myGraph, &aFaceInfo, &aFace), OCCTL_OK);

  occtl_oriented_node_t aFaces[1];
  aFaces[0].id          = aFace;
  aFaces[0].orientation = OCCTL_ORIENTATION_FORWARD;

  occtl_topo_make_shell_info_t aShellInfo = OCCTL_TOPO_MAKE_SHELL_INFO_INIT;
  aShellInfo.faces                        = aFaces;
  aShellInfo.face_count                   = 1;
  aShellInfo.is_closed                    = 0;

  occtl_node_id_t aShell = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_shell(myGraph, &aShellInfo, &aShell), OCCTL_OK);
  ASSERT_NE(aShell.bits, 0u);

  uint32_t aNbFaces = 0;
  ASSERT_EQ(occtl_topo_shell_face_count(myGraph, aShell, &aNbFaces), OCCTL_OK);
  EXPECT_EQ(aNbFaces, 1u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_faces_of_shell_iter_create(myGraph, aShell, &anIter), OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  occtl_node_id_t anIterFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_node_iter_next(anIter, &anIterFace), OCCTL_OK);
  EXPECT_EQ(anIterFace.bits, aFace.bits);

  occtl_node_iter_free(anIter);
}

TEST_F(TopoMutationTest, MakeSolid_FromOneShell_CreatesSolid)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aV[4];
  const double    aCoords[4][2] = {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}};
  for (int anI = 0; anI < 4; ++anI)
  {
    aVertInfo.point = {aCoords[anI][0], aCoords[anI][1], 0.0};
    ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[anI]), OCCTL_OK);
  }

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.curve                       = {};
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t aE[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    anEdgeInfo.start_vertex = aV[anI];
    anEdgeInfo.end_vertex   = aV[(anI + 1) % 4];
    anEdgeInfo.first        = 0.0;
    anEdgeInfo.last         = 10.0;
    ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &aE[anI]), OCCTL_OK);
  }

  occtl_oriented_node_t aEdges[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    aEdges[anI].id          = aE[anI];
    aEdges[anI].orientation = OCCTL_ORIENTATION_FORWARD;
  }

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = aEdges;
  aWireInfo.edge_count                  = 4;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWire), OCCTL_OK);

  occtl_geom_plane_t aPlaneData = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(occtl_surface_create_plane(myGraph, &aSurfaceId, aPlaneData), OCCTL_OK);

  occtl_topo_make_face_info_t aFaceInfo = OCCTL_TOPO_MAKE_FACE_INFO_INIT;
  aFaceInfo.surface                     = aSurfaceId;
  aFaceInfo.outer_wire                  = aWire;
  aFaceInfo.inner_wires                 = nullptr;
  aFaceInfo.inner_wire_count            = 0;
  aFaceInfo.tolerance                   = 1e-6;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_face(myGraph, &aFaceInfo, &aFace), OCCTL_OK);

  occtl_oriented_node_t aShellFaces[1];
  aShellFaces[0].id          = aFace;
  aShellFaces[0].orientation = OCCTL_ORIENTATION_FORWARD;

  occtl_topo_make_shell_info_t aShellInfo = OCCTL_TOPO_MAKE_SHELL_INFO_INIT;
  aShellInfo.faces                        = aShellFaces;
  aShellInfo.face_count                   = 1;
  aShellInfo.is_closed                    = 0;

  occtl_node_id_t aShell = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_shell(myGraph, &aShellInfo, &aShell), OCCTL_OK);

  occtl_oriented_node_t aSolidShells[1];
  aSolidShells[0].id          = aShell;
  aSolidShells[0].orientation = OCCTL_ORIENTATION_FORWARD;

  occtl_topo_make_solid_info_t aSolidInfo = OCCTL_TOPO_MAKE_SOLID_INFO_INIT;
  aSolidInfo.shells                       = aSolidShells;
  aSolidInfo.shell_count                  = 1;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_solid(myGraph, &aSolidInfo, &aSolid), OCCTL_OK);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_shells_of_solid_iter_create(myGraph, aSolid, &anIter), OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  occtl_node_id_t anIterShell = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_node_iter_next(anIter, &anIterShell), OCCTL_OK);
  EXPECT_EQ(anIterShell.bits, aShell.bits);

  occtl_node_iter_free(anIter);
}

TEST_F(TopoMutationTest, MakeCompound_FromTwoVertices_CreatesCompound)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  aVertInfo.point     = {0.0, 0.0, 0.0};
  occtl_node_id_t aV0 = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV0), OCCTL_OK);

  aVertInfo.point     = {1.0, 0.0, 0.0};
  occtl_node_id_t aV1 = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV1), OCCTL_OK);

  occtl_oriented_node_t aChildren[2];
  aChildren[0].id          = aV0;
  aChildren[0].orientation = OCCTL_ORIENTATION_FORWARD;
  aChildren[1].id          = aV1;
  aChildren[1].orientation = OCCTL_ORIENTATION_FORWARD;

  occtl_topo_make_compound_info_t aCompInfo = OCCTL_TOPO_MAKE_COMPOUND_INFO_INIT;
  aCompInfo.children                        = aChildren;
  aCompInfo.child_count                     = 2;

  occtl_node_id_t aCompound = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_compound(myGraph, &aCompInfo, &aCompound), OCCTL_OK);
  ASSERT_NE(aCompound.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aCompound, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_COMPOUND);

  EXPECT_GE(occtl_graph_count_value(occtl_graph_compound_count, myGraph), 1u);
}

TEST_F(TopoMutationTest, Remove_Vertex_ReducesCount)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.point                         = {1.0, 2.0, 3.0};
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aVertex = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aVertex), OCCTL_OK);

  EXPECT_EQ(occtl_graph_count_value(occtl_graph_vertex_count, myGraph), 1u);

  ASSERT_EQ(occtl_topo_remove(myGraph, aVertex), OCCTL_OK);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_vertex_count, myGraph), 0u);
}

TEST_F(TopoMutationTest, Remove_Subgraph_RemovesDescendants)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aV[4];
  const double    aCoords[4][2] = {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}};
  for (int anI = 0; anI < 4; ++anI)
  {
    aVertInfo.point = {aCoords[anI][0], aCoords[anI][1], 0.0};
    ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV[anI]), OCCTL_OK);
  }

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.curve                       = {};
  anEdgeInfo.tolerance                   = 1e-6;

  occtl_node_id_t aE[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    anEdgeInfo.start_vertex = aV[anI];
    anEdgeInfo.end_vertex   = aV[(anI + 1) % 4];
    anEdgeInfo.first        = 0.0;
    anEdgeInfo.last         = 10.0;
    ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &aE[anI]), OCCTL_OK);
  }

  occtl_oriented_node_t aEdges[4];
  for (int anI = 0; anI < 4; ++anI)
  {
    aEdges[anI].id          = aE[anI];
    aEdges[anI].orientation = OCCTL_ORIENTATION_FORWARD;
  }

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = aEdges;
  aWireInfo.edge_count                  = 4;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWire), OCCTL_OK);

  ASSERT_EQ(occtl_topo_remove_subgraph(myGraph, aWire), OCCTL_OK);

  EXPECT_EQ(occtl_graph_count_value(occtl_graph_wire_count, myGraph), 0u);
}

TEST_F(TopoMutationTest, MakeVertex_NullGraph_InvalidArgument)
{
  occtl_topo_make_vertex_info_t aInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aInfo.point                         = {0.0, 0.0, 0.0};
  aInfo.tolerance                     = 1e-6;

  occtl_node_id_t aVertex = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_vertex(nullptr, &aInfo, &aVertex), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoMutationTest, MakeVertex_NullInfo_InvalidArgument)
{
  occtl_node_id_t aVertex = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_vertex(myGraph, nullptr, &aVertex), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoMutationTest, MakeVertex_NullOut_InvalidArgument)
{
  occtl_topo_make_vertex_info_t aInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aInfo.point                         = {0.0, 0.0, 0.0};
  aInfo.tolerance                     = 1e-6;

  EXPECT_EQ(occtl_topo_make_vertex(myGraph, &aInfo, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoMutationTest, MakeVertex_BadVersion_VersionMismatch)
{
  occtl_topo_make_vertex_info_t aInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aInfo.point                         = {0.0, 0.0, 0.0};
  aInfo.tolerance                     = 1e-6;
  aInfo.struct_version                = 999;

  occtl_node_id_t aVertex = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_vertex(myGraph, &aInfo, &aVertex), OCCTL_VERSION_MISMATCH);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoMutationTest, MakeWire_NullEdgesWithNonZeroCount_InvalidArgument)
{
  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = nullptr;
  aWireInfo.edge_count                  = 1;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWire), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoMutationTest, MakeWire_InvalidChildId_NotFound)
{
  occtl_oriented_node_t aBadEdges[1];
  aBadEdges[0].id          = OCCTL_NODE_ID_INVALID;
  aBadEdges[0].orientation = OCCTL_ORIENTATION_FORWARD;

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = aBadEdges;
  aWireInfo.edge_count                  = 1;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWire), OCCTL_NOT_FOUND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoMutationTest, MakeWire_WrongKindChild_WrongKind)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.point                         = {0.0, 0.0, 0.0};
  aVertInfo.tolerance                     = 1e-6;

  occtl_node_id_t aVertex = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aVertex), OCCTL_OK);

  occtl_oriented_node_t aBadEdges[1];
  aBadEdges[0].id          = aVertex;
  aBadEdges[0].orientation = OCCTL_ORIENTATION_FORWARD;

  occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                       = aBadEdges;
  aWireInfo.edge_count                  = 1;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_wire(myGraph, &aWireInfo, &aWire), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoMutationTest, RemovedVertexNotVisibleInIterator)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-6;

  aVertInfo.point     = {0.0, 0.0, 0.0};
  occtl_node_id_t aV0 = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV0), OCCTL_OK);

  aVertInfo.point     = {1.0, 0.0, 0.0};
  occtl_node_id_t aV1 = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aV1), OCCTL_OK);

  EXPECT_EQ(occtl_graph_count_value(occtl_graph_vertex_count, myGraph), 2u);

  ASSERT_EQ(occtl_topo_remove(myGraph, aV0), OCCTL_OK);

  EXPECT_EQ(occtl_graph_count_value(occtl_graph_vertex_count, myGraph), 1u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_vertex_iter_create(myGraph, &anIter), OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  bool            aFoundRemoved = false;
  occtl_node_id_t anId          = OCCTL_NODE_ID_INVALID;
  while (occtl_node_iter_next(anIter, &anId) == OCCTL_OK)
  {
    if (anId.bits == aV0.bits)
    {
      aFoundRemoved = true;
    }
  }
  EXPECT_FALSE(aFoundRemoved);

  occtl_node_iter_free(anIter);
}

} // anonymous namespace
