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

#include <occtl-hpp/topo.hpp>
#include <occtl/occtl_core.h>
#include <occtl/occtl_geom.h>
#include <occtl/occtl_topo.h>

#include "test_helpers_internal.hxx"

#include <cmath>

namespace
{

class TopoGeomTest : public ::testing::Test
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

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(TopoGeomTest, VertexPoint_ReturnsCorrectCoordinates)
{
  const occtl_node_id_t aVertexId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertexId.bits, 0u);

  occtl_point3_t aPoint;
  ASSERT_EQ(occtl_topo_vertex_point(myGraph, aVertexId, &aPoint), OCCTL_OK);

  const bool aValidX = (std::abs(aPoint.x - 0.0) < 0.01 || std::abs(aPoint.x - 10.0) < 0.01);
  const bool aValidY = (std::abs(aPoint.y - 0.0) < 0.01 || std::abs(aPoint.y - 20.0) < 0.01);
  const bool aValidZ = (std::abs(aPoint.z - 0.0) < 0.01 || std::abs(aPoint.z - 30.0) < 0.01);
  EXPECT_TRUE(aValidX) << "x = " << aPoint.x;
  EXPECT_TRUE(aValidY) << "y = " << aPoint.y;
  EXPECT_TRUE(aValidZ) << "z = " << aPoint.z;
}

TEST_F(TopoGeomTest, VertexPoint_NullGraph_ReturnsInvalidArgument)
{
  occtl_point3_t  aPoint;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_vertex_point(nullptr, anId, &aPoint), OCCTL_INVALID_ARGUMENT);

  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoGeomTest, VertexPoint_NullOutParam_ReturnsInvalidArgument)
{
  occtl_node_id_t aVertexId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  EXPECT_EQ(occtl_topo_vertex_point(myGraph, aVertexId, nullptr), OCCTL_INVALID_ARGUMENT);

  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoGeomTest, VertexPoint_InvalidId_ReturnsNotFound)
{
  occtl_point3_t  aPoint;
  occtl_node_id_t anInvalidId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_vertex_point(myGraph, anInvalidId, &aPoint), OCCTL_NOT_FOUND);

  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoGeomTest, VertexPoint_WrongKind_ReturnsWrongKind)
{
  occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_point3_t aPoint;
  EXPECT_EQ(occtl_topo_vertex_point(myGraph, anEdgeId, &aPoint), OCCTL_WRONG_KIND);

  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoGeomTest, VertexTolerance_ReturnsPositiveValue)
{
  const occtl_node_id_t aVertexId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertexId.bits, 0u);

  double aTol = 0.0;
  ASSERT_EQ(occtl_topo_vertex_tolerance(myGraph, aVertexId, &aTol), OCCTL_OK);
  EXPECT_GT(aTol, 0.0);
  EXPECT_LT(aTol, 1.0);
}

TEST_F(TopoGeomTest, VertexNbEdges_ReturnsPositiveCount)
{
  const occtl_node_id_t aVertexId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertexId.bits, 0u);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_vertex_edge_count(myGraph, aVertexId, &aCount), OCCTL_OK);
  EXPECT_GE(aCount, 3u);
}

TEST_F(TopoGeomTest, EdgeRange_ReturnsFiniteRange)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_edge_range(myGraph, anEdgeId, &aFirst, &aLast), OCCTL_OK);
  EXPECT_LT(aFirst, aLast);
}

TEST_F(TopoGeomTest, EdgeTolerance_ReturnsPositiveValue)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  double aTol = 0.0;
  ASSERT_EQ(occtl_topo_edge_tolerance(myGraph, anEdgeId, &aTol), OCCTL_OK);
  EXPECT_GT(aTol, 0.0);
}

TEST_F(TopoGeomTest, EdgeIsDegenerated_ReturnsFalseForBoxEdge)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_edge_is_degenerated(myGraph, anEdgeId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 0);
}

TEST_F(TopoGeomTest, EdgeHasCurve_ReturnsTrueForBoxEdge)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  int32_t aHas = 0;
  ASSERT_EQ(occtl_topo_edge_has_curve(myGraph, anEdgeId, &aHas), OCCTL_OK);
  EXPECT_EQ(aHas, 1);
}

TEST_F(TopoGeomTest, EdgeCurveKind_BoxEdge_ReturnsLine)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_curve_kind_t aKind = OCCTL_CURVE_KIND_UNDEFINED;
  ASSERT_EQ(occtl_topo_edge_curve_kind(myGraph, anEdgeId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_LINE);
}

TEST_F(TopoGeomTest, EdgeStartAndEndVertex_AreDistinctValidVertices)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_node_id_t aStart = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t aEnd   = OCCTL_NODE_ID_INVALID;

  ASSERT_EQ(occtl_topo_edge_start_vertex(myGraph, anEdgeId, &aStart), OCCTL_OK);
  ASSERT_EQ(occtl_topo_edge_end_vertex(myGraph, anEdgeId, &aEnd), OCCTL_OK);

  EXPECT_NE(aStart.bits, 0u);
  EXPECT_NE(aEnd.bits, 0u);
  EXPECT_NE(aStart.bits, aEnd.bits);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aStart, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_VERTEX);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aEnd, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_VERTEX);
}

TEST_F(TopoGeomTest, EdgeQuery_NullGraph_ReturnsInvalidArgument)
{
  double          aFirst, aLast;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_edge_range(nullptr, anId, &aFirst, &aLast), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, EdgeQuery_InvalidId_ReturnsNotFound)
{
  int32_t         aFlag;
  occtl_node_id_t anInvalidId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_edge_is_degenerated(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);

  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoGeomTest, CoEdgeIsSeam_ReturnsFalseForBoxCoEdge)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_coedge_is_seam(myGraph, aCoId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 0);
}

TEST_F(TopoGeomTest, CoEdgeEdgeOf_ReturnsValidEdge)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  occtl_node_id_t aEdgeId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_coedge_edge_of(myGraph, aCoId, &aEdgeId), OCCTL_OK);
  EXPECT_NE(aEdgeId.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aEdgeId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_EDGE);
}

TEST_F(TopoGeomTest, CoEdgeFaceOf_ReturnsValidFace)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  occtl_node_id_t aFaceId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_coedge_face_of(myGraph, aCoId, &aFaceId), OCCTL_OK);
  EXPECT_NE(aFaceId.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFaceId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(TopoGeomTest, FaceTolerance_ReturnsPositiveValue)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  double aTol = 0.0;
  ASSERT_EQ(occtl_topo_face_tolerance(myGraph, aFaceId, &aTol), OCCTL_OK);
  EXPECT_GT(aTol, 0.0);
}

TEST_F(TopoGeomTest, FaceNbWires_ReturnsOneForBoxFace)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_face_wire_count(myGraph, aFaceId, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);
}

TEST_F(TopoGeomTest, FaceOuterWire_ReturnsValidWire)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_node_id_t aWireId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_face_outer_wire(myGraph, aFaceId, &aWireId), OCCTL_OK);
  EXPECT_NE(aWireId.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aWireId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_WIRE);
}

TEST_F(TopoGeomTest, FaceUvBounds_ReturnsFiniteRange)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  double aUmin = 0.0, aUmax = 0.0, aVmin = 0.0, aVmax = 0.0;
  ASSERT_EQ(occtl_topo_face_uv_bounds(myGraph, aFaceId, &aUmin, &aUmax, &aVmin, &aVmax), OCCTL_OK);
  EXPECT_LT(aUmin, aUmax);
  EXPECT_LT(aVmin, aVmax);
  EXPECT_TRUE(std::isfinite(aUmin));
  EXPECT_TRUE(std::isfinite(aVmin));
}

TEST_F(TopoGeomTest, FaceHasSurface_ReturnsTrueForBoxFace)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  int32_t aHas = 0;
  ASSERT_EQ(occtl_topo_face_has_surface(myGraph, aFaceId, &aHas), OCCTL_OK);
  EXPECT_EQ(aHas, 1);
}

TEST_F(TopoGeomTest, FaceSurfaceKind_BoxFace_ReturnsPlane)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_surface_kind_t aKind = OCCTL_SURFACE_KIND_UNDEFINED;
  ASSERT_EQ(occtl_topo_face_surface_kind(myGraph, aFaceId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_PLANE);
}

TEST_F(TopoGeomTest, FaceQuery_NullGraph_ReturnsInvalidArgument)
{
  double          aTol;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_face_tolerance(nullptr, anId, &aTol), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, WireIsClosed_ReturnsTrueForBoxWire)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  int32_t aFlag = 0;
  ASSERT_EQ(occtl_topo_wire_is_closed(myGraph, aWireId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);
}

TEST_F(TopoGeomTest, WireNbCoedges_ReturnsFourForBoxWire)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_wire_coedge_count(myGraph, aWireId, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 4u);
}

TEST_F(TopoGeomTest, ShellIsClosed_ReturnsTrueForBoxShell)
{
  const occtl_node_id_t aShellId = firstAbiNodeOfKind(OCCTL_KIND_SHELL);
  ASSERT_NE(aShellId.bits, 0u);

  int32_t aFlag = 0;
  ASSERT_EQ(occtl_topo_shell_is_closed(myGraph, aShellId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);
}

TEST_F(TopoGeomTest, ShellNbFaces_ReturnsSixForBox)
{
  const occtl_node_id_t aShellId = firstAbiNodeOfKind(OCCTL_KIND_SHELL);
  ASSERT_NE(aShellId.bits, 0u);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_shell_face_count(myGraph, aShellId, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 6u);
}

TEST_F(TopoGeomTest, VertexParameter_OnBoxEdge_ReturnsFiniteValue)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  occtl_node_id_t anEdgeId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_coedge_edge_of(myGraph, aCoId, &anEdgeId), OCCTL_OK);
  occtl_node_id_t aVertId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_edge_start_vertex(myGraph, anEdgeId, &aVertId), OCCTL_OK);
  ASSERT_NE(aVertId.bits, 0u);
  ASSERT_NE(anEdgeId.bits, 0u);

  double aParam = 0.0;
  ASSERT_EQ(occtl_topo_vertex_parameter(myGraph, aVertId, anEdgeId, &aParam), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aParam));
}

TEST_F(TopoGeomTest, VertexParameter_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t aVertId  = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  EXPECT_EQ(occtl_topo_vertex_parameter(myGraph, aVertId, anEdgeId, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, VertexParameters_OnBoxFace_ReturnsFiniteUV)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  occtl_node_id_t anEdgeId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_coedge_edge_of(myGraph, aCoId, &anEdgeId), OCCTL_OK);
  occtl_node_id_t aVertId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_edge_start_vertex(myGraph, anEdgeId, &aVertId), OCCTL_OK);
  occtl_node_id_t aFaceId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_coedge_face_of(myGraph, aCoId, &aFaceId), OCCTL_OK);
  ASSERT_NE(aVertId.bits, 0u);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_point2_t aUV;
  ASSERT_EQ(occtl_topo_vertex_parameters(myGraph, aVertId, aFaceId, &aUV), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aUV.x));
  EXPECT_TRUE(std::isfinite(aUV.y));
}

TEST_F(TopoGeomTest, VertexParameters_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t aVertId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  EXPECT_EQ(occtl_topo_vertex_parameters(myGraph, aVertId, aFaceId, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, EdgeSameParameter_OnBox_ReturnsOne)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_edge_same_parameter(myGraph, anEdgeId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);
}

TEST_F(TopoGeomTest, EdgeSameRange_OnBox_ReturnsOne)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_edge_same_range(myGraph, anEdgeId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);
}

TEST_F(TopoGeomTest, EdgeIsManifold_OnBox_ReturnsOne)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_edge_is_manifold(myGraph, anEdgeId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);
}

TEST_F(TopoGeomTest, EdgeIsBoundary_OnBox_ReturnsZero)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_edge_is_boundary(myGraph, anEdgeId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 0);
}

TEST_F(TopoGeomTest, EdgeIsSeamOnFace_ReturnsValidFlag)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  const occtl_node_id_t aFaceId  = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(anEdgeId.bits, 0u);
  ASSERT_NE(aFaceId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_edge_is_seam_on_face(myGraph, anEdgeId, aFaceId, &aFlag), OCCTL_OK);
  EXPECT_TRUE(aFlag == 0 || aFlag == 1);
}

TEST_F(TopoGeomTest, EdgeIsSeamOnFace_NullOutFlag_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  const occtl_node_id_t aFaceId  = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  EXPECT_EQ(occtl_topo_edge_is_seam_on_face(myGraph, anEdgeId, aFaceId, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, EdgeNbFaces_OnBox_ReturnsTwo)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_edge_face_count(myGraph, anEdgeId, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 2u);
}

TEST_F(TopoGeomTest, EdgeNbFaces_NullOutCount_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  EXPECT_EQ(occtl_topo_edge_face_count(myGraph, anEdgeId, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, CoEdgeIsReversed_OnBox_ReturnsZeroOrOne)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_coedge_is_reversed(myGraph, aCoId, &aFlag), OCCTL_OK);
  EXPECT_TRUE(aFlag == 0 || aFlag == 1);
}

TEST_F(TopoGeomTest, CoEdgeHasPcurve_OnBox_ReturnsOne)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  int32_t aFlag = 0;
  ASSERT_EQ(occtl_topo_coedge_has_pcurve(myGraph, aCoId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);
}

TEST_F(TopoGeomTest, CoEdgePcurveParameter_ReturnsFiniteValue)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  occtl_node_id_t anEdgeId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_coedge_edge_of(myGraph, aCoId, &anEdgeId), OCCTL_OK);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_node_id_t aStartVert = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_edge_start_vertex(myGraph, anEdgeId, &aStartVert), OCCTL_OK);
  ASSERT_NE(aStartVert.bits, 0u);

  double aParam = 0.0;
  ASSERT_EQ(occtl_topo_coedge_pcurve_parameter(myGraph, aCoId, aStartVert, &aParam), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aParam));
}

TEST_F(TopoGeomTest, CoEdgeRange_ReturnsFirstLessThanLast)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_coedge_range(myGraph, aCoId, &aFirst, &aLast), OCCTL_OK);
  EXPECT_LT(aFirst, aLast);
}

TEST_F(TopoGeomTest, CoEdgeUvPoints_ReturnsFiniteUV)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  occtl_point2_t aUVstart, aUVend;
  ASSERT_EQ(occtl_topo_coedge_uv_points(myGraph, aCoId, &aUVstart, &aUVend), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aUVstart.x));
  EXPECT_TRUE(std::isfinite(aUVstart.y));
  EXPECT_TRUE(std::isfinite(aUVend.x));
  EXPECT_TRUE(std::isfinite(aUVend.y));
}

TEST_F(TopoGeomTest, CoEdgeSeamPair_OnBox_ReturnsInvalid)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  occtl_node_id_t aPair = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_coedge_seam_pair(myGraph, aCoId, &aPair), OCCTL_OK);
  EXPECT_EQ(aPair.bits, OCCTL_NODE_ID_INVALID.bits);
}

TEST_F(TopoGeomTest, FaceNaturalRestriction_OnBox_ReturnsZero)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  int32_t aFlag = 0;
  ASSERT_EQ(occtl_topo_face_natural_restriction(myGraph, aFaceId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 0);
}

TEST_F(TopoGeomTest, FaceHasTriangulation_OnBox_ReturnsValidFlag)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_face_has_triangulation(myGraph, aFaceId, &aFlag), OCCTL_OK);
  EXPECT_TRUE(aFlag == 0 || aFlag == 1);
}

TEST_F(TopoGeomTest, WireNbDistinctEdges_OnBox_ReturnsFour)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_topo_wire_distinct_edge_count(myGraph, aWireId, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 4u);
}

TEST_F(TopoGeomTest, WireFaceOf_OnBox_ReturnsValidFace)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  occtl_node_id_t aFaceId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_wire_face_of(myGraph, aWireId, &aFaceId), OCCTL_OK);
  EXPECT_NE(aFaceId.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFaceId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(TopoGeomTest, WireIsOuter_OnBox_ReturnsOne)
{
  const occtl_node_id_t aWireId = firstAbiNodeOfKind(OCCTL_KIND_WIRE);
  ASSERT_NE(aWireId.bits, 0u);

  int32_t aFlag = 0;
  ASSERT_EQ(occtl_topo_wire_is_outer(myGraph, aWireId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);
}

TEST_F(TopoGeomTest, Phase2a_NullGraph_ReturnsInvalidArgument)
{
  int32_t         aFlag;
  uint32_t        aCount;
  double          aParam;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  occtl_point2_t  aUV;

  EXPECT_EQ(occtl_topo_vertex_parameter(nullptr, anId, anId, &aParam), OCCTL_INVALID_ARGUMENT);

  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);

  EXPECT_EQ(occtl_topo_vertex_parameters(nullptr, anId, anId, &aUV), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_same_parameter(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_same_range(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_is_manifold(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_is_boundary(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_is_seam_on_face(nullptr, anId, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_face_count(nullptr, anId, &aCount), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_coedge_is_reversed(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_coedge_has_pcurve(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_coedge_pcurve_parameter(nullptr, anId, anId, &aParam),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_coedge_range(nullptr, anId, &aParam, &aParam), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_coedge_uv_points(nullptr, anId, &aUV, &aUV), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_coedge_seam_pair(nullptr, anId, &anId), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_face_natural_restriction(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_face_has_triangulation(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_wire_distinct_edge_count(nullptr, anId, &aCount), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_wire_face_of(nullptr, anId, &anId), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_wire_is_outer(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoGeomTest, Phase2a_InvalidNodeId_ReturnsNotFound)
{
  int32_t         aFlag;
  uint32_t        aCount;
  double          aParam;
  occtl_node_id_t aNodeId     = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t anInvalidId = OCCTL_NODE_ID_INVALID;
  occtl_point2_t  aUV;

  EXPECT_EQ(occtl_topo_vertex_parameter(myGraph, anInvalidId, anInvalidId, &aParam),
            OCCTL_NOT_FOUND);

  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);

  EXPECT_EQ(occtl_topo_vertex_parameters(myGraph, anInvalidId, anInvalidId, &aUV), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_edge_same_parameter(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_edge_same_range(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_edge_is_manifold(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_edge_is_boundary(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_edge_is_seam_on_face(myGraph, anInvalidId, anInvalidId, &aFlag),
            OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_edge_face_count(myGraph, anInvalidId, &aCount), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_coedge_is_reversed(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_coedge_has_pcurve(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_coedge_pcurve_parameter(myGraph, anInvalidId, anInvalidId, &aParam),
            OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_coedge_range(myGraph, anInvalidId, &aParam, &aParam), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_coedge_uv_points(myGraph, anInvalidId, &aUV, &aUV), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_coedge_seam_pair(myGraph, anInvalidId, &aNodeId), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_face_natural_restriction(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_face_has_triangulation(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_wire_distinct_edge_count(myGraph, anInvalidId, &aCount), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_wire_face_of(myGraph, anInvalidId, &aNodeId), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_topo_wire_is_outer(myGraph, anInvalidId, &aFlag), OCCTL_NOT_FOUND);
}

TEST(TopoVeneerTest, Smoke_CompilesAndMatchesCAbi)
{
  // Create a fresh graph owned by the veneer RAII wrapper.
  occtl::Graph aVeneer;

  // Import a box internally via OCCT APIs.
  TopoDS_Shape                  aBox    = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();
  BRepGraph::ShapesView::Result aResult = aVeneer.get()->graph.Shapes().Add(aBox);
  ASSERT_TRUE(aResult.Ok);

  // Find first vertex via OCCT iterator.
  BRepGraph_VertexIterator anIt(aVeneer.get()->graph);
  ASSERT_TRUE(anIt.More());
  const occtl::NodeId aVertex(OcctL::Topo::PackNodeId(anIt.CurrentId()));

  const occtl::Point3 aPoint = aVeneer.vertex_point(aVertex);
  EXPECT_TRUE(std::abs(aPoint.x()) < 20.0 || std::abs(aPoint.y()) < 30.0);

  const double aTol = aVeneer.vertex_tolerance(aVertex);
  EXPECT_GT(aTol, 0.0);

  // Find first edge and test edge queries.
  BRepGraph_EdgeIterator anEdgeIt(aVeneer.get()->graph);
  ASSERT_TRUE(anEdgeIt.More());
  const occtl::NodeId anEdge(OcctL::Topo::PackNodeId(anEdgeIt.CurrentId()));

  const auto aRange = aVeneer.edge_range(anEdge);
  EXPECT_LT(aRange.first, aRange.second);
  EXPECT_GT(aVeneer.edge_tolerance(anEdge), 0.0);
  EXPECT_FALSE(aVeneer.edge_is_degenerated(anEdge));
  EXPECT_TRUE(aVeneer.edge_has_curve(anEdge));
  EXPECT_EQ(aVeneer.edge_curve_kind(anEdge), OCCTL_CURVE_KIND_LINE);

  const occtl::NodeId aStart = aVeneer.edge_start_vertex(anEdge);
  const occtl::NodeId aEnd   = aVeneer.edge_end_vertex(anEdge);
  EXPECT_TRUE(aStart.is_valid());
  EXPECT_TRUE(aEnd.is_valid());
  EXPECT_NE(aStart, aEnd);
  EXPECT_TRUE(std::isfinite(aVeneer.vertex_parameter(aStart, anEdge)));

  // Face + wire + shell smoke calls.
  BRepGraph_FaceIterator aFaceIt(aVeneer.get()->graph);
  ASSERT_TRUE(aFaceIt.More());
  const occtl::NodeId aFace(OcctL::Topo::PackNodeId(aFaceIt.CurrentId()));
  EXPECT_EQ(aVeneer.face_wire_count(aFace), 1u);
  EXPECT_TRUE(aVeneer.face_has_surface(aFace));
  EXPECT_EQ(aVeneer.face_surface_kind(aFace), OCCTL_SURFACE_KIND_PLANE);
  EXPECT_TRUE(aVeneer.face_outer_wire(aFace).is_valid());

  BRepGraph_WireIterator aWireIt(aVeneer.get()->graph);
  ASSERT_TRUE(aWireIt.More());
  const occtl::NodeId aWire(OcctL::Topo::PackNodeId(aWireIt.CurrentId()));
  EXPECT_TRUE(aVeneer.wire_is_closed(aWire));
  EXPECT_EQ(aVeneer.wire_coedge_count(aWire), 4u);

  BRepGraph_CoEdgeIterator aCoedgeIt(aVeneer.get()->graph);
  ASSERT_TRUE(aCoedgeIt.More());
  const occtl::NodeId aCoedge(OcctL::Topo::PackNodeId(aCoedgeIt.CurrentId()));
  const occtl::NodeId aCoedgeEdge  = aVeneer.coedge_edge_of(aCoedge);
  const occtl::NodeId aCoedgeStart = aVeneer.edge_start_vertex(aCoedgeEdge);
  EXPECT_TRUE(aVeneer.coedge_has_pcurve(aCoedge));
  EXPECT_TRUE(std::isfinite(aVeneer.coedge_pcurve_parameter(aCoedge, aCoedgeStart)));
  const occtl::Point2 aVertexUv =
    aVeneer.vertex_parameters(aCoedgeStart, aVeneer.coedge_face_of(aCoedge));
  EXPECT_TRUE(std::isfinite(aVertexUv.x()));
  EXPECT_TRUE(std::isfinite(aVertexUv.y()));

  BRepGraph_ShellIterator aShellIt(aVeneer.get()->graph);
  ASSERT_TRUE(aShellIt.More());
  const occtl::NodeId aShell(OcctL::Topo::PackNodeId(aShellIt.CurrentId()));
  EXPECT_TRUE(aVeneer.shell_is_closed(aShell));
  EXPECT_EQ(aVeneer.shell_face_count(aShell), 6u);

  // Graph is freed automatically when aVeneer goes out of scope.
}

TEST_F(TopoGeomTest, VerticesOfEdge_Box_Order)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_node_id_t aStart = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t aEnd   = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_edge_start_vertex(myGraph, anEdgeId, &aStart), OCCTL_OK);
  ASSERT_EQ(occtl_topo_edge_end_vertex(myGraph, anEdgeId, &aEnd), OCCTL_OK);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_vertices_of_edge_iter_create(myGraph, anEdgeId, &anIter), OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  occtl_node_id_t aVertexId;
  ASSERT_EQ(occtl_node_iter_next(anIter, &aVertexId), OCCTL_OK);
  EXPECT_EQ(aVertexId.bits, aStart.bits);

  ASSERT_EQ(occtl_node_iter_next(anIter, &aVertexId), OCCTL_OK);
  EXPECT_EQ(aVertexId.bits, aEnd.bits);

  occtl_node_iter_free(anIter);
}

TEST_F(TopoGeomTest, EdgeEval_Box_Midpoint)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_edge_range(myGraph, anEdgeId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point3_t aP;
  ASSERT_EQ(occtl_topo_edge_eval(myGraph, anEdgeId, aMid, &aP), OCCTL_OK);
  EXPECT_GE(aP.x, 0.0);
  EXPECT_LE(aP.x, 10.0);
  EXPECT_GE(aP.y, 0.0);
  EXPECT_LE(aP.y, 20.0);
  EXPECT_GE(aP.z, 0.0);
  EXPECT_LE(aP.z, 30.0);
}

TEST_F(TopoGeomTest, EdgeEval_NullGraph_ReturnsInvalidArgument)
{
  occtl_point3_t  aP;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_edge_eval(nullptr, anId, 0.0, &aP), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, EdgeEval_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  EXPECT_EQ(occtl_topo_edge_eval(myGraph, anEdgeId, 0.0, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, EdgeEval_InvalidId_ReturnsNotFound)
{
  occtl_point3_t aP;
  EXPECT_EQ(occtl_topo_edge_eval(myGraph, OCCTL_NODE_ID_INVALID, 0.0, &aP), OCCTL_NOT_FOUND);
}

TEST_F(TopoGeomTest, EdgeEval_WrongKind_ReturnsWrongKind)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  occtl_point3_t        aP;
  EXPECT_EQ(occtl_topo_edge_eval(myGraph, aFaceId, 0.0, &aP), OCCTL_WRONG_KIND);
}

TEST_F(TopoGeomTest, EdgeEvalD1_Box_Midpoint)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_edge_range(myGraph, anEdgeId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point3_t  aP;
  occtl_vector3_t aD1;
  ASSERT_EQ(occtl_topo_edge_eval_d1(myGraph, anEdgeId, aMid, &aP, &aD1), OCCTL_OK);
  EXPECT_GE(aP.x, 0.0);
  EXPECT_LE(aP.x, 10.0);
  EXPECT_GE(aP.y, 0.0);
  EXPECT_LE(aP.y, 20.0);
  EXPECT_GE(aP.z, 0.0);
  EXPECT_LE(aP.z, 30.0);
  EXPECT_TRUE(std::isfinite(aD1.x));
  EXPECT_TRUE(std::isfinite(aD1.y));
  EXPECT_TRUE(std::isfinite(aD1.z));
}

TEST_F(TopoGeomTest, EdgeEvalD1_NullOutParams_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  occtl_point3_t        aP;
  occtl_vector3_t       aD1;
  EXPECT_EQ(occtl_topo_edge_eval_d1(myGraph, anEdgeId, 0.0, &aP, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_eval_d1(myGraph, anEdgeId, 0.0, nullptr, &aD1), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, CoEdgePcurveEval_Box_Midpoint)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_coedge_range(myGraph, aCoId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point2_t aUV;
  ASSERT_EQ(occtl_topo_coedge_pcurve_eval(myGraph, aCoId, aMid, &aUV), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aUV.x));
  EXPECT_TRUE(std::isfinite(aUV.y));
}

TEST_F(TopoGeomTest, CoEdgePcurveEval_NullGraph_ReturnsInvalidArgument)
{
  occtl_point2_t  aUV;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_coedge_pcurve_eval(nullptr, anId, 0.0, &aUV), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, CoEdgePcurveEval_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  EXPECT_EQ(occtl_topo_coedge_pcurve_eval(myGraph, aCoId, 0.0, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, CoEdgePcurveEval_InvalidId_ReturnsNotFound)
{
  occtl_point2_t aUV;
  EXPECT_EQ(occtl_topo_coedge_pcurve_eval(myGraph, OCCTL_NODE_ID_INVALID, 0.0, &aUV),
            OCCTL_NOT_FOUND);
}

TEST_F(TopoGeomTest, FaceEval_Box_BottomFace)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  double aUmin = 0.0, aUmax = 0.0, aVmin = 0.0, aVmax = 0.0;
  ASSERT_EQ(occtl_topo_face_uv_bounds(myGraph, aFaceId, &aUmin, &aUmax, &aVmin, &aVmax), OCCTL_OK);
  const double aMidU = 0.5 * (aUmin + aUmax);
  const double aMidV = 0.5 * (aVmin + aVmax);

  occtl_point3_t aP;
  ASSERT_EQ(occtl_topo_face_eval(myGraph, aFaceId, aMidU, aMidV, &aP), OCCTL_OK);
  EXPECT_GE(aP.x, 0.0);
  EXPECT_LE(aP.x, 10.0);
  EXPECT_GE(aP.y, 0.0);
  EXPECT_LE(aP.y, 20.0);
  EXPECT_GE(aP.z, 0.0);
  EXPECT_LE(aP.z, 30.0);
}

TEST_F(TopoGeomTest, FaceEval_NullGraph_ReturnsInvalidArgument)
{
  occtl_point3_t  aP;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_face_eval(nullptr, anId, 0.0, 0.0, &aP), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, FaceEval_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  EXPECT_EQ(occtl_topo_face_eval(myGraph, aFaceId, 0.0, 0.0, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, FaceEval_InvalidId_ReturnsNotFound)
{
  occtl_point3_t aP;
  EXPECT_EQ(occtl_topo_face_eval(myGraph, OCCTL_NODE_ID_INVALID, 0.0, 0.0, &aP), OCCTL_NOT_FOUND);
}

TEST_F(TopoGeomTest, FaceEvalD1_Box_BottomFace)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  double aUmin = 0.0, aUmax = 0.0, aVmin = 0.0, aVmax = 0.0;
  ASSERT_EQ(occtl_topo_face_uv_bounds(myGraph, aFaceId, &aUmin, &aUmax, &aVmin, &aVmax), OCCTL_OK);
  const double aMidU = 0.5 * (aUmin + aUmax);
  const double aMidV = 0.5 * (aVmin + aVmax);

  occtl_point3_t  aP;
  occtl_vector3_t aD1U, aD1V;
  ASSERT_EQ(occtl_topo_face_eval_d1(myGraph, aFaceId, aMidU, aMidV, &aP, &aD1U, &aD1V), OCCTL_OK);
  EXPECT_GE(aP.x, 0.0);
  EXPECT_LE(aP.x, 10.0);
  EXPECT_GE(aP.y, 0.0);
  EXPECT_LE(aP.y, 20.0);
  EXPECT_GE(aP.z, 0.0);
  EXPECT_LE(aP.z, 30.0);
  // D1U and D1V should be non-zero tangent vectors
  const double aMagU = ::occtl_vector3_magnitude(aD1U);
  const double aMagV = ::occtl_vector3_magnitude(aD1V);
  EXPECT_GT(aMagU, 0.0);
  EXPECT_GT(aMagV, 0.0);
}

TEST_F(TopoGeomTest, FaceEvalD1_NullOutParams_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  occtl_point3_t        aP;
  occtl_vector3_t       aD1U, aD1V;
  EXPECT_EQ(occtl_topo_face_eval_d1(myGraph, aFaceId, 0.0, 0.0, &aP, &aD1U, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_face_eval_d1(myGraph, aFaceId, 0.0, 0.0, &aP, nullptr, &aD1V),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_face_eval_d1(myGraph, aFaceId, 0.0, 0.0, nullptr, &aD1U, &aD1V),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, EdgeEvalD2_Box)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_edge_range(myGraph, anEdgeId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point3_t  aP;
  occtl_vector3_t aD1, aD2;
  ASSERT_EQ(occtl_topo_edge_eval_d2(myGraph, anEdgeId, aMid, &aP, &aD1, &aD2), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aP.x));
  EXPECT_TRUE(std::isfinite(aD1.x));
  EXPECT_TRUE(std::isfinite(aD2.x));
  // D1 should be non-zero for a non-degenerate box edge
  const double aD1Mag = ::occtl_vector3_magnitude(aD1);
  EXPECT_GT(aD1Mag, 0.0);
}

TEST_F(TopoGeomTest, EdgeEvalD2_NullOutParams_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  occtl_point3_t        aP;
  occtl_vector3_t       aD1, aD2;
  EXPECT_EQ(occtl_topo_edge_eval_d2(myGraph, anEdgeId, 0.0, &aP, &aD1, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_EQ(occtl_topo_edge_eval_d2(myGraph, anEdgeId, 0.0, &aP, nullptr, &aD2),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_eval_d2(myGraph, anEdgeId, 0.0, nullptr, &aD1, &aD2),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoGeomTest, EdgeEvalD2_NullGraph_ReturnsInvalidArgument)
{
  occtl_point3_t  aP;
  occtl_vector3_t aD1, aD2;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_edge_eval_d2(nullptr, anId, 0.0, &aP, &aD1, &aD2), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, EdgeEvalD3_Box)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_edge_range(myGraph, anEdgeId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point3_t  aP;
  occtl_vector3_t aD1, aD2, aD3;
  ASSERT_EQ(occtl_topo_edge_eval_d3(myGraph, anEdgeId, aMid, &aP, &aD1, &aD2, &aD3), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aP.x));
  EXPECT_TRUE(std::isfinite(aD1.x));
  EXPECT_TRUE(std::isfinite(aD2.x));
  EXPECT_TRUE(std::isfinite(aD3.x));
}

TEST_F(TopoGeomTest, EdgeEvalDn_Box_D1Matches)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_edge_range(myGraph, anEdgeId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point3_t  aP;
  occtl_vector3_t aD1_d1;
  ASSERT_EQ(occtl_topo_edge_eval_d1(myGraph, anEdgeId, aMid, &aP, &aD1_d1), OCCTL_OK);

  occtl_vector3_t aD1_dn;
  ASSERT_EQ(occtl_topo_edge_eval_dn(myGraph, anEdgeId, aMid, 1u, &aD1_dn), OCCTL_OK);

  EXPECT_NEAR(aD1_d1.x, aD1_dn.x, 1e-12);
  EXPECT_NEAR(aD1_d1.y, aD1_dn.y, 1e-12);
  EXPECT_NEAR(aD1_d1.z, aD1_dn.z, 1e-12);
}

TEST_F(TopoGeomTest, CoedgePcurveEvalD1_Box)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_coedge_range(myGraph, aCoId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point2_t  aUV;
  occtl_vector2_t aD1;
  ASSERT_EQ(occtl_topo_coedge_pcurve_eval_d1(myGraph, aCoId, aMid, &aUV, &aD1), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aUV.x));
  EXPECT_TRUE(std::isfinite(aUV.y));
  const double aMag = ::occtl_vector2_magnitude(aD1);
  EXPECT_GT(aMag, 0.0);
}

TEST_F(TopoGeomTest, CoedgePcurveEvalD2_Box)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_coedge_range(myGraph, aCoId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point2_t  aUV;
  occtl_vector2_t aD1, aD2;
  ASSERT_EQ(occtl_topo_coedge_pcurve_eval_d2(myGraph, aCoId, aMid, &aUV, &aD1, &aD2), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aUV.x));
  EXPECT_TRUE(std::isfinite(aD1.x));
  EXPECT_TRUE(std::isfinite(aD2.x));
}

TEST_F(TopoGeomTest, CoedgePcurveEvalD3_Box)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_coedge_range(myGraph, aCoId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point2_t  aUV;
  occtl_vector2_t aD1, aD2, aD3;
  ASSERT_EQ(occtl_topo_coedge_pcurve_eval_d3(myGraph, aCoId, aMid, &aUV, &aD1, &aD2, &aD3),
            OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aUV.x));
  EXPECT_TRUE(std::isfinite(aD1.x));
  EXPECT_TRUE(std::isfinite(aD2.x));
  EXPECT_TRUE(std::isfinite(aD3.x));
}

TEST_F(TopoGeomTest, CoedgePcurveEvalDn_Box_D1Matches)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_coedge_range(myGraph, aCoId, &aFirst, &aLast), OCCTL_OK);
  const double aMid = 0.5 * (aFirst + aLast);

  occtl_point2_t  aUV;
  occtl_vector2_t aD1_d1;
  ASSERT_EQ(occtl_topo_coedge_pcurve_eval_d1(myGraph, aCoId, aMid, &aUV, &aD1_d1), OCCTL_OK);

  occtl_vector2_t aD1_dn;
  ASSERT_EQ(occtl_topo_coedge_pcurve_eval_dn(myGraph, aCoId, aMid, 1u, &aD1_dn), OCCTL_OK);

  EXPECT_NEAR(aD1_d1.x, aD1_dn.x, 1e-12);
  EXPECT_NEAR(aD1_d1.y, aD1_dn.y, 1e-12);
}

TEST_F(TopoGeomTest, CoedgePcurveEvalD1_NullGraph_ReturnsInvalidArgument)
{
  occtl_point2_t  aUV;
  occtl_vector2_t aD1;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_coedge_pcurve_eval_d1(nullptr, anId, 0.0, &aUV, &aD1),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, CoedgePcurveEvalD1_NullOutParams_ReturnsInvalidArgument)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  occtl_point2_t        aUV;
  occtl_vector2_t       aD1;
  EXPECT_EQ(occtl_topo_coedge_pcurve_eval_d1(myGraph, aCoId, 0.0, &aUV, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_coedge_pcurve_eval_d1(myGraph, aCoId, 0.0, nullptr, &aD1),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, FaceEvalDn_Box_D1UMatches)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  double aUmin = 0.0, aUmax = 0.0, aVmin = 0.0, aVmax = 0.0;
  ASSERT_EQ(occtl_topo_face_uv_bounds(myGraph, aFaceId, &aUmin, &aUmax, &aVmin, &aVmax), OCCTL_OK);
  const double aMidU = 0.5 * (aUmin + aUmax);
  const double aMidV = 0.5 * (aVmin + aVmax);

  occtl_point3_t  aP;
  occtl_vector3_t aD1U_d1, aD1V;
  ASSERT_EQ(occtl_topo_face_eval_d1(myGraph, aFaceId, aMidU, aMidV, &aP, &aD1U_d1, &aD1V),
            OCCTL_OK);

  occtl_vector3_t aD1U_dn;
  ASSERT_EQ(occtl_topo_face_eval_dn(myGraph, aFaceId, aMidU, aMidV, 1u, 0u, &aD1U_dn), OCCTL_OK);

  EXPECT_NEAR(aD1U_d1.x, aD1U_dn.x, 1e-12);
  EXPECT_NEAR(aD1U_d1.y, aD1U_dn.y, 1e-12);
  EXPECT_NEAR(aD1U_d1.z, aD1U_dn.z, 1e-12);
}

TEST_F(TopoGeomTest, FaceEvalDn_NullOutParams_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  EXPECT_EQ(occtl_topo_face_eval_dn(myGraph, aFaceId, 0.0, 0.0, 0u, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, FaceEvalDn_NullGraph_ReturnsInvalidArgument)
{
  occtl_vector3_t aDN;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_face_eval_dn(nullptr, anId, 0.0, 0.0, 0u, 0u, &aDN), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, EdgeEvalDn_NullGraph_ReturnsInvalidArgument)
{
  occtl_vector3_t aDN;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_edge_eval_dn(nullptr, anId, 0.0, 0u, &aDN), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, EdgeEvalDn_NullOutParams_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  EXPECT_EQ(occtl_topo_edge_eval_dn(myGraph, anEdgeId, 0.0, 0u, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, CoedgePcurveEvalDn_NullGraph_ReturnsInvalidArgument)
{
  occtl_vector2_t aDN;
  occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_coedge_pcurve_eval_dn(nullptr, anId, 0.0, 0u, &aDN), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, FaceEvalD2_Box_BottomFace)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  double aUMin = 0, aUMax = 0, aVMin = 0, aVMax = 0;
  ASSERT_EQ(occtl_topo_face_uv_bounds(myGraph, aFaceId, &aUMin, &aUMax, &aVMin, &aVMax), OCCTL_OK);
  const double aMidU = (aUMin + aUMax) * 0.5;
  const double aMidV = (aVMin + aVMax) * 0.5;

  occtl_point3_t  aOutP{};
  occtl_vector3_t aOutD1U{}, aOutD1V{}, aOutD2U{}, aOutD2V{}, aOutD2UV{};
  ASSERT_EQ(occtl_topo_face_eval_d2(myGraph,
                                    aFaceId,
                                    aMidU,
                                    aMidV,
                                    &aOutP,
                                    &aOutD1U,
                                    &aOutD1V,
                                    &aOutD2U,
                                    &aOutD2V,
                                    &aOutD2UV),
            OCCTL_OK);

  EXPECT_GE(aOutP.x, 0.0);
  EXPECT_LE(aOutP.x, 10.0);
  EXPECT_GE(aOutP.y, 0.0);
  EXPECT_LE(aOutP.y, 20.0);
  EXPECT_GE(aOutP.z, 0.0);
  EXPECT_LE(aOutP.z, 30.0);
  EXPECT_GT(::occtl_vector3_magnitude(aOutD1U), 0.0);
  EXPECT_GT(::occtl_vector3_magnitude(aOutD1V), 0.0);
  // Planar face: second derivatives are zero
  EXPECT_NEAR(::occtl_vector3_magnitude(aOutD2U), 0.0, 1e-12);
  EXPECT_NEAR(::occtl_vector3_magnitude(aOutD2V), 0.0, 1e-12);
  EXPECT_NEAR(::occtl_vector3_magnitude(aOutD2UV), 0.0, 1e-12);
}

TEST_F(TopoGeomTest, FaceEvalD2_NullOut_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  EXPECT_EQ(occtl_topo_face_eval_d2(myGraph,
                                    aFaceId,
                                    0.0,
                                    0.0,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoGeomTest, FaceEvalD3_Box_BottomFace)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  double aUMin = 0, aUMax = 0, aVMin = 0, aVMax = 0;
  ASSERT_EQ(occtl_topo_face_uv_bounds(myGraph, aFaceId, &aUMin, &aUMax, &aVMin, &aVMax), OCCTL_OK);
  const double aMidU = (aUMin + aUMax) * 0.5;
  const double aMidV = (aVMin + aVMax) * 0.5;

  occtl_point3_t  aOutP{};
  occtl_vector3_t aOutD1U{}, aOutD1V{}, aOutD2U{}, aOutD2V{}, aOutD2UV{};
  occtl_vector3_t aOutD3U{}, aOutD3V{}, aOutD3UUV{}, aOutD3UVV{};
  ASSERT_EQ(occtl_topo_face_eval_d3(myGraph,
                                    aFaceId,
                                    aMidU,
                                    aMidV,
                                    &aOutP,
                                    &aOutD1U,
                                    &aOutD1V,
                                    &aOutD2U,
                                    &aOutD2V,
                                    &aOutD2UV,
                                    &aOutD3U,
                                    &aOutD3V,
                                    &aOutD3UUV,
                                    &aOutD3UVV),
            OCCTL_OK);

  EXPECT_GE(aOutP.x, 0.0);
  EXPECT_LE(aOutP.x, 10.0);
  EXPECT_GE(aOutP.y, 0.0);
  EXPECT_LE(aOutP.y, 20.0);
  EXPECT_GE(aOutP.z, 0.0);
  EXPECT_LE(aOutP.z, 30.0);
  EXPECT_GT(::occtl_vector3_magnitude(aOutD1U), 0.0);
  EXPECT_GT(::occtl_vector3_magnitude(aOutD1V), 0.0);
  // Planar face: second and third derivatives are zero
  EXPECT_NEAR(::occtl_vector3_magnitude(aOutD2U), 0.0, 1e-12);
  EXPECT_NEAR(::occtl_vector3_magnitude(aOutD3U), 0.0, 1e-12);
}

TEST_F(TopoGeomTest, FaceEvalD3_NullOut_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  EXPECT_EQ(occtl_topo_face_eval_d3(myGraph,
                                    aFaceId,
                                    0.0,
                                    0.0,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

} // anonymous namespace
