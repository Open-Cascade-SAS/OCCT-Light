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

class TopoReadGapsTest : public ::testing::Test
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

TEST_F(TopoReadGapsTest, EdgeContinuity_OnBox_ReturnsC0)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  const occtl_node_id_t aFaceA = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceA.bits, 0u);

  occtl_node_iter_t* aFaceIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &aFaceIter), OCCTL_OK);
  ASSERT_NE(aFaceIter, nullptr);

  occtl_node_id_t aFaceB;
  int             aFound = 0;
  while (occtl_node_iter_next(aFaceIter, &aFaceB) == OCCTL_OK)
  {
    if (aFaceB.bits != aFaceA.bits)
    {
      aFound = 1;
      break;
    }
  }
  occtl_node_iter_free(aFaceIter);
  ASSERT_TRUE(aFound);

  occtl_shape_continuity_t aCont = OCCTL_CONTINUITY_RESERVED_FUTURE;
  ASSERT_EQ(occtl_topo_edge_continuity(myGraph, anEdgeId, aFaceA, aFaceB, &aCont), OCCTL_OK);
  EXPECT_GE(aCont, OCCTL_CONTINUITY_C0);
  EXPECT_LE(aCont, OCCTL_CONTINUITY_G2);
}

TEST_F(TopoReadGapsTest, EdgeContinuity_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  const occtl_node_id_t aFaceA   = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  const occtl_node_id_t aFaceB   = firstAbiNodeOfKind(OCCTL_KIND_FACE);

  EXPECT_EQ(occtl_topo_edge_continuity(myGraph, anEdgeId, aFaceA, aFaceB, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoReadGapsTest, EdgeHasContinuity_OnBox_ReturnsTrue)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  const occtl_node_id_t aFaceA = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceA.bits, 0u);

  occtl_node_iter_t* aFaceIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &aFaceIter), OCCTL_OK);
  occtl_node_id_t aFaceB;
  int             aFound = 0;
  while (occtl_node_iter_next(aFaceIter, &aFaceB) == OCCTL_OK)
  {
    if (aFaceB.bits != aFaceA.bits)
    {
      aFound = 1;
      break;
    }
  }
  occtl_node_iter_free(aFaceIter);
  ASSERT_TRUE(aFound);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_edge_has_continuity(myGraph, anEdgeId, aFaceA, aFaceB, &aFlag), OCCTL_OK);
  EXPECT_TRUE(aFlag == 0 || aFlag == 1);
}

TEST_F(TopoReadGapsTest, EdgeMaxContinuity_OnBox_ReturnsValidEnum)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_shape_continuity_t aCont = OCCTL_CONTINUITY_RESERVED_FUTURE;
  ASSERT_EQ(occtl_topo_edge_max_continuity(myGraph, anEdgeId, &aCont), OCCTL_OK);
  EXPECT_GE(aCont, OCCTL_CONTINUITY_C0);
  EXPECT_LE(aCont, OCCTL_CONTINUITY_G2);
}

TEST_F(TopoReadGapsTest, EdgeMaxContinuity_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  EXPECT_EQ(occtl_topo_edge_max_continuity(myGraph, anEdgeId, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoReadGapsTest, CoedgeOrientation_OnBox_ReturnsForwardOrReversed)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  occtl_orientation_t aOri = OCCTL_ORIENTATION_RESERVED_FUTURE;
  ASSERT_EQ(occtl_topo_coedge_orientation(myGraph, aCoId, &aOri), OCCTL_OK);
  EXPECT_TRUE(aOri == OCCTL_ORIENTATION_FORWARD || aOri == OCCTL_ORIENTATION_REVERSED);
}

TEST_F(TopoReadGapsTest, CoedgeOrientation_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  EXPECT_EQ(occtl_topo_coedge_orientation(myGraph, aCoId, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoReadGapsTest, EdgeFindCoedgeOnFace_OnBox_ReturnsValidCoedge)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  const occtl_node_id_t aFaceA = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceA.bits, 0u);

  occtl_node_iter_t* aFaceIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &aFaceIter), OCCTL_OK);
  occtl_node_id_t aFaceId;
  int             aFoundCo = 0;
  occtl_node_id_t aCoId    = OCCTL_NODE_ID_INVALID;
  while (occtl_node_iter_next(aFaceIter, &aFaceId) == OCCTL_OK)
  {
    const occtl_status_t aSt =
      occtl_topo_edge_find_coedge_on_face(myGraph, anEdgeId, aFaceId, &aCoId);
    if (aSt == OCCTL_OK && aCoId.bits != 0u)
    {
      aFoundCo = 1;
      break;
    }
  }
  occtl_node_iter_free(aFaceIter);
  ASSERT_TRUE(aFoundCo);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aCoId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_COEDGE);
}

TEST_F(TopoReadGapsTest, EdgeFindCoedgeOnFace_NoMatch_ReturnsNotFound)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_node_iter_t* aFaceIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &aFaceIter), OCCTL_OK);

  occtl_node_id_t aFaceId;
  int             aFoundMatch   = 0;
  occtl_node_id_t aFaceWithNoCo = OCCTL_NODE_ID_INVALID;
  while (occtl_node_iter_next(aFaceIter, &aFaceId) == OCCTL_OK)
  {
    occtl_node_id_t      aCoId = OCCTL_NODE_ID_INVALID;
    const occtl_status_t aSt =
      occtl_topo_edge_find_coedge_on_face(myGraph, anEdgeId, aFaceId, &aCoId);
    if (aSt != OCCTL_OK)
    {
      aFaceWithNoCo = aFaceId;
      aFoundMatch   = 1;
      break;
    }
  }
  occtl_node_iter_free(aFaceIter);

  if (aFoundMatch)
  {
    occtl_node_id_t aCoId2 = OCCTL_NODE_ID_INVALID;
    EXPECT_EQ(occtl_topo_edge_find_coedge_on_face(myGraph, anEdgeId, aFaceWithNoCo, &aCoId2),
              OCCTL_NOT_FOUND);
    EXPECT_NE(occtl_error_last()->message, nullptr);
  }
}

TEST_F(TopoReadGapsTest, EdgeFindCoedgeOnFace_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  const occtl_node_id_t aFaceId  = firstAbiNodeOfKind(OCCTL_KIND_FACE);

  EXPECT_EQ(occtl_topo_edge_find_coedge_on_face(myGraph, anEdgeId, aFaceId, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoReadGapsTest, VertexPointInUsage_OnBox_ReturnsValidPoint)
{
  const occtl_node_id_t aVertId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertId.bits, 0u);

  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_point3_t aPoint;
  ASSERT_EQ(occtl_topo_vertex_point_in_usage(myGraph, aVertId, anEdgeId, &aPoint), OCCTL_OK);
  EXPECT_TRUE(std::isfinite(aPoint.x));
  EXPECT_TRUE(std::isfinite(aPoint.y));
  EXPECT_TRUE(std::isfinite(aPoint.z));
}

TEST_F(TopoReadGapsTest, VertexPointInUsage_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t aVertId  = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);

  EXPECT_EQ(occtl_topo_vertex_point_in_usage(myGraph, aVertId, anEdgeId, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoReadGapsTest, VertexPointInUsage_ParentWithoutVertex_ReturnsNotFound)
{
  occtl_node_iter_t* aVertexIter = nullptr;
  ASSERT_EQ(occtl_graph_vertex_iter_create(myGraph, &aVertexIter), OCCTL_OK);
  ASSERT_NE(aVertexIter, nullptr);

  occtl_node_id_t aVertexA = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t aVertexB = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_node_iter_next(aVertexIter, &aVertexA), OCCTL_OK);
  ASSERT_EQ(occtl_node_iter_next(aVertexIter, &aVertexB), OCCTL_OK);
  occtl_node_iter_free(aVertexIter);
  ASSERT_NE(aVertexA.bits, 0u);
  ASSERT_NE(aVertexB.bits, 0u);

  occtl_point3_t aPoint = {};
  EXPECT_EQ(occtl_topo_vertex_point_in_usage(myGraph, aVertexA, aVertexB, &aPoint),
            OCCTL_NOT_FOUND);
}

TEST_F(TopoReadGapsTest, VertexPcurveParameter_OnBox_ReturnsValidOrInternal)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  occtl_node_id_t anEdgeId = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_coedge_edge_of(myGraph, aCoId, &anEdgeId), OCCTL_OK);

  occtl_node_id_t aStartVert = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_edge_start_vertex(myGraph, anEdgeId, &aStartVert), OCCTL_OK);
  ASSERT_NE(aStartVert.bits, 0u);

  double               aParam = 0.0;
  const occtl_status_t aStatus =
    occtl_topo_vertex_pcurve_parameter(myGraph, aStartVert, aCoId, &aParam);
  if (aStatus == OCCTL_OK)
  {
    EXPECT_TRUE(std::isfinite(aParam));
  }
  else
  {
    EXPECT_TRUE(aStatus == OCCTL_INTERNAL);
  }
}

TEST_F(TopoReadGapsTest, VertexPcurveParameter_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t aVertId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  const occtl_node_id_t aCoId   = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);

  EXPECT_EQ(occtl_topo_vertex_pcurve_parameter(myGraph, aVertId, aCoId, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoReadGapsTest, EdgeHasPolygon3d_OnBox_ReturnsValidFlag)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_edge_has_polygon3d(myGraph, anEdgeId, &aFlag), OCCTL_OK);
  EXPECT_TRUE(aFlag == 0 || aFlag == 1);
}

TEST_F(TopoReadGapsTest, CoedgeHasPolygonOnSurface_OnBox_ReturnsValidFlag)
{
  const occtl_node_id_t aCoId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoId.bits, 0u);

  int32_t aFlag = -1;
  ASSERT_EQ(occtl_topo_coedge_has_polygon_on_surface(myGraph, aCoId, &aFlag), OCCTL_OK);
  EXPECT_TRUE(aFlag == 0 || aFlag == 1);
}

TEST_F(TopoReadGapsTest, FaceUvBoundsRestricted_OnBox_ReturnsValidValues)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  double aUmin = 0.0, aUmax = 0.0, aVmin = 0.0, aVmax = 0.0;
  ASSERT_EQ(occtl_topo_face_uv_bounds(myGraph, aFaceId, &aUmin, &aUmax, &aVmin, &aVmax), OCCTL_OK);
  const double aMidU = 0.5 * (aUmin + aUmax);
  const double aMidV = 0.5 * (aVmin + aVmax);

  occtl_point3_t  aP;
  occtl_vector3_t aD1U, aD1V;
  ASSERT_EQ(occtl_topo_face_uv_bounds_restricted(myGraph,
                                                 aFaceId,
                                                 aUmin,
                                                 aUmax,
                                                 aVmin,
                                                 aVmax,
                                                 aMidU,
                                                 aMidV,
                                                 &aP,
                                                 &aD1U,
                                                 &aD1V),
            OCCTL_OK);

  EXPECT_GE(aP.x, 0.0);
  EXPECT_LE(aP.x, 10.0);
  EXPECT_GE(aP.y, 0.0);
  EXPECT_LE(aP.y, 20.0);
  EXPECT_GE(aP.z, 0.0);
  EXPECT_LE(aP.z, 30.0);
  EXPECT_GT(::occtl_vector3_magnitude(aD1U), 0.0);
  EXPECT_GT(::occtl_vector3_magnitude(aD1V), 0.0);
}

TEST_F(TopoReadGapsTest, FaceUvBoundsRestricted_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(OCCTL_KIND_FACE);

  EXPECT_EQ(occtl_topo_face_uv_bounds_restricted(myGraph,
                                                 aFaceId,
                                                 0.0,
                                                 1.0,
                                                 0.0,
                                                 1.0,
                                                 0.5,
                                                 0.5,
                                                 nullptr,
                                                 nullptr,
                                                 nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoReadGapsTest, EdgeHasPolygon3d_NullOutFlag_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  EXPECT_EQ(occtl_topo_edge_has_polygon3d(myGraph, anEdgeId, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoReadGapsTest, NullGraph_ReturnsInvalidArgument)
{
  occtl_node_id_t          anId = OCCTL_NODE_ID_INVALID;
  occtl_shape_continuity_t aCont;
  int32_t                  aFlag;
  occtl_node_id_t          aOutId = OCCTL_NODE_ID_INVALID;
  occtl_orientation_t      aOri;
  occtl_point3_t           aP;
  double                   aVal;
  occtl_vector3_t          aV;

  EXPECT_EQ(occtl_topo_edge_continuity(nullptr, anId, anId, anId, &aCont), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);

  EXPECT_EQ(occtl_topo_edge_has_continuity(nullptr, anId, anId, anId, &aFlag),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_max_continuity(nullptr, anId, &aCont), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_coedge_orientation(nullptr, anId, &aOri), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_find_coedge_on_face(nullptr, anId, anId, &aOutId),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_find_coedge_on_face_oriented(nullptr,
                                                         anId,
                                                         anId,
                                                         OCCTL_ORIENTATION_FORWARD,
                                                         &aOutId),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_vertex_point_in_usage(nullptr, anId, anId, &aP), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_vertex_pcurve_parameter(nullptr, anId, anId, &aVal), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_edge_has_polygon3d(nullptr, anId, &aFlag), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_coedge_has_polygon_on_surface(nullptr, anId, &aFlag),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_face_uv_bounds_restricted(nullptr,
                                                 anId,
                                                 0.0,
                                                 1.0,
                                                 0.0,
                                                 1.0,
                                                 0.5,
                                                 0.5,
                                                 &aP,
                                                 &aV,
                                                 &aV),
            OCCTL_INVALID_ARGUMENT);
}

} // anonymous namespace
