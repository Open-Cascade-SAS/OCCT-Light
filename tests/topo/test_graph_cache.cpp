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

#include <occtl/occtl_topo.h>

#include <occtl-hpp/topo.hpp>

#include <gtest/gtest.h>

#include "test_helpers_internal.hxx"

#include <algorithm>
#include <vector>

namespace
{

class GraphCacheTest : public ::testing::Test
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

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(GraphCacheTest, CacheBBoxGet_ComputesValue)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_select_bbox_t aBox{};
  ASSERT_EQ(occtl_graph_bbox_get(myGraph, aSolidId, &aBox), OCCTL_OK);
  EXPECT_NEAR(aBox.min.x, 0.0, 1.0e-6);
  EXPECT_NEAR(aBox.min.y, 0.0, 1.0e-6);
  EXPECT_NEAR(aBox.min.z, 0.0, 1.0e-6);
  EXPECT_NEAR(aBox.max.x, 10.0, 1.0e-6);
  EXPECT_NEAR(aBox.max.y, 20.0, 1.0e-6);
  EXPECT_NEAR(aBox.max.z, 30.0, 1.0e-6);
}

TEST_F(GraphCacheTest, CacheObbGet_ComputesValue)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_graph_obb_t aBox{};
  ASSERT_EQ(occtl_graph_obb_get(myGraph, aSolidId, &aBox), OCCTL_OK);
  EXPECT_NEAR(aBox.center.x, 5.0, 1.0e-6);
  EXPECT_NEAR(aBox.center.y, 10.0, 1.0e-6);
  EXPECT_NEAR(aBox.center.z, 15.0, 1.0e-6);
  EXPECT_GT(aBox.x_half_size, 0.0);
  EXPECT_GT(aBox.y_half_size, 0.0);
  EXPECT_GT(aBox.z_half_size, 0.0);
}

TEST_F(GraphCacheTest, CacheFaceUvBoundsGet_ComputesValue)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_graph_uv_bounds_t aBounds{};
  ASSERT_EQ(occtl_graph_face_uv_bounds_get(myGraph, aFaceId, &aBounds), OCCTL_OK);
  EXPECT_LE(aBounds.u_min, aBounds.u_max);
  EXPECT_LE(aBounds.v_min, aBounds.v_max);
}

TEST_F(GraphCacheTest, CacheMeasureGet_ComputesValue)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  double aVolume = 0.0;
  ASSERT_EQ(occtl_graph_measure_get(myGraph, aSolidId, OCCTL_SELECT_MEASURE_VOLUME, &aVolume),
            OCCTL_OK);
  EXPECT_NEAR(aVolume, 6000.0, 1.0e-7);
}

TEST_F(GraphCacheTest, CacheMassPropertiesGet_ComputesValue)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_graph_mass_properties_t aProperties{};
  ASSERT_EQ(occtl_graph_mass_properties_get(myGraph, aSolidId, &aProperties), OCCTL_OK);
  EXPECT_GT(aProperties.linear_length, 0.0);
  EXPECT_NEAR(aProperties.surface_area, 2200.0, 1.0e-7);
  EXPECT_NEAR(aProperties.volume, 6000.0, 1.0e-7);
  EXPECT_NEAR(aProperties.mass, 6000.0, 1.0e-7);
  EXPECT_NEAR(aProperties.centre_of_mass.x, 5.0, 1.0e-7);
  EXPECT_NEAR(aProperties.centre_of_mass.y, 10.0, 1.0e-7);
  EXPECT_NEAR(aProperties.centre_of_mass.z, 15.0, 1.0e-7);
  EXPECT_GT(aProperties.inertia[0], 0.0);
  EXPECT_GT(aProperties.inertia[4], 0.0);
  EXPECT_GT(aProperties.inertia[8], 0.0);
}

TEST_F(GraphCacheTest, CacheGeometryKindGet_ComputesValue)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  const occtl_node_id_t aFaceId  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(anEdgeId.bits, 0u);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_curve_kind_t aCurveKind = OCCTL_CURVE_KIND_UNDEFINED;
  ASSERT_EQ(occtl_graph_edge_curve_kind_get(myGraph, anEdgeId, &aCurveKind), OCCTL_OK);
  EXPECT_EQ(aCurveKind, OCCTL_CURVE_KIND_LINE);

  occtl_surface_kind_t aSurfaceKind = OCCTL_SURFACE_KIND_UNDEFINED;
  ASSERT_EQ(occtl_graph_face_surface_kind_get(myGraph, aFaceId, &aSurfaceKind), OCCTL_OK);
  EXPECT_EQ(aSurfaceKind, OCCTL_SURFACE_KIND_PLANE);
}

TEST_F(GraphCacheTest, CacheDescendantVerticesGet_ComputesValue)
{
  const occtl_node_id_t aSolidId  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aVertexId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_VERTEX);
  ASSERT_NE(aSolidId.bits, 0u);
  ASSERT_NE(aVertexId.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_graph_descendant_vertices_get(myGraph, aSolidId, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 8u);

  std::vector<occtl_node_id_t> aVertices(aCount);
  ASSERT_EQ(occtl_graph_descendant_vertices_get(myGraph,
                                                aSolidId,
                                                aVertices.data(),
                                                aVertices.size(),
                                                &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 8u);
  EXPECT_TRUE(std::all_of(aVertices.begin(), aVertices.end(), [](const occtl_node_id_t theNode) {
    return theNode.bits != 0u;
  }));

  size_t          aVertexCount = 0;
  occtl_node_id_t aSelf        = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_graph_descendant_vertices_get(myGraph, aVertexId, &aSelf, 1, &aVertexCount),
            OCCTL_OK);
  EXPECT_EQ(aVertexCount, 1u);
  EXPECT_EQ(aSelf.bits, aVertexId.bits);
}

TEST_F(GraphCacheTest, CacheDescendantEdgesAndFacesGet_ComputesValue)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  const occtl_node_id_t aFaceId  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aSolidId.bits, 0u);
  ASSERT_NE(anEdgeId.bits, 0u);
  ASSERT_NE(aFaceId.bits, 0u);

  size_t anEdgeCount = 0;
  ASSERT_EQ(occtl_graph_descendant_edges_get(myGraph, aSolidId, nullptr, 0, &anEdgeCount),
            OCCTL_OK);
  ASSERT_EQ(anEdgeCount, 12u);

  std::vector<occtl_node_id_t> anEdges(anEdgeCount);
  ASSERT_EQ(occtl_graph_descendant_edges_get(myGraph,
                                             aSolidId,
                                             anEdges.data(),
                                             anEdges.size(),
                                             &anEdgeCount),
            OCCTL_OK);
  EXPECT_EQ(anEdgeCount, 12u);

  size_t          aSelfEdgeCount = 0;
  occtl_node_id_t aSelfEdge      = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_graph_descendant_edges_get(myGraph, anEdgeId, &aSelfEdge, 1, &aSelfEdgeCount),
            OCCTL_OK);
  EXPECT_EQ(aSelfEdgeCount, 1u);
  EXPECT_EQ(aSelfEdge.bits, anEdgeId.bits);

  size_t aFaceCount = 0;
  ASSERT_EQ(occtl_graph_descendant_faces_get(myGraph, aSolidId, nullptr, 0, &aFaceCount), OCCTL_OK);
  ASSERT_EQ(aFaceCount, 6u);

  std::vector<occtl_node_id_t> aFaces(aFaceCount);
  ASSERT_EQ(
    occtl_graph_descendant_faces_get(myGraph, aSolidId, aFaces.data(), aFaces.size(), &aFaceCount),
    OCCTL_OK);
  EXPECT_EQ(aFaceCount, 6u);

  size_t          aSelfFaceCount = 0;
  occtl_node_id_t aSelfFace      = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_graph_descendant_faces_get(myGraph, aFaceId, &aSelfFace, 1, &aSelfFaceCount),
            OCCTL_OK);
  EXPECT_EQ(aSelfFaceCount, 1u);
  EXPECT_EQ(aSelfFace.bits, aFaceId.bits);

  size_t aGenericWireCount = 0;
  ASSERT_EQ(
    occtl_graph_descendants_get(myGraph, aSolidId, OCCTL_KIND_WIRE, nullptr, 0, &aGenericWireCount),
    OCCTL_OK);
  ASSERT_EQ(aGenericWireCount, 6u);

  std::vector<occtl_node_id_t> aGenericWires(aGenericWireCount);
  ASSERT_EQ(occtl_graph_descendants_get(myGraph,
                                        aSolidId,
                                        OCCTL_KIND_WIRE,
                                        aGenericWires.data(),
                                        aGenericWires.size(),
                                        &aGenericWireCount),
            OCCTL_OK);
  EXPECT_EQ(aGenericWireCount, 6u);
}

TEST_F(GraphCacheTest, CacheAdjacentFacesGet_ComputesValue)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_graph_adjacent_faces_get(myGraph, aFaceId, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 4u);

  std::vector<occtl_node_id_t> aFaces(aCount);
  ASSERT_EQ(occtl_graph_adjacent_faces_get(myGraph, aFaceId, aFaces.data(), aFaces.size(), &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 4u);
  EXPECT_TRUE(std::all_of(aFaces.begin(), aFaces.end(), [](const occtl_node_id_t theNode) {
    return theNode.bits != 0u;
  }));
  EXPECT_EQ(
    std::find_if(aFaces.begin(),
                 aFaces.end(),
                 [&](const occtl_node_id_t theNode) { return theNode.bits == aFaceId.bits; }),
    aFaces.end());
}

TEST_F(GraphCacheTest, CacheAdjacentEdgesGet_ComputesValue)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_graph_adjacent_edges_get(myGraph, anEdgeId, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_GE(aCount, 2u);

  std::vector<occtl_node_id_t> anEdges(aCount);
  ASSERT_EQ(
    occtl_graph_adjacent_edges_get(myGraph, anEdgeId, anEdges.data(), anEdges.size(), &aCount),
    OCCTL_OK);
  EXPECT_GE(aCount, 2u);
  EXPECT_TRUE(std::all_of(anEdges.begin(), anEdges.end(), [](const occtl_node_id_t theNode) {
    return theNode.bits != 0u;
  }));
  EXPECT_EQ(
    std::find_if(anEdges.begin(),
                 anEdges.end(),
                 [&](const occtl_node_id_t theNode) { return theNode.bits == anEdgeId.bits; }),
    anEdges.end());
}

TEST_F(GraphCacheTest, CachePairDistanceGet_ComputesValue)
{
  BRepGraph_VertexIterator anIt(myGraph->graph);
  ASSERT_TRUE(anIt.More());
  const occtl_node_id_t aFirstVertex = OcctL::Topo::PackNodeId(anIt.CurrentId());
  anIt.Next();
  ASSERT_TRUE(anIt.More());
  const occtl_node_id_t aSecondVertex = OcctL::Topo::PackNodeId(anIt.CurrentId());
  ASSERT_NE(aFirstVertex.bits, 0u);
  ASSERT_NE(aSecondVertex.bits, 0u);

  double aDistance = -1.0;
  ASSERT_EQ(occtl_graph_pair_distance_get(myGraph, aFirstVertex, aSecondVertex, &aDistance),
            OCCTL_OK);
  EXPECT_GT(aDistance, 0.0);

  double aReverseDistance = -1.0;
  ASSERT_EQ(occtl_graph_pair_distance_get(myGraph, aSecondVertex, aFirstVertex, &aReverseDistance),
            OCCTL_OK);
  EXPECT_DOUBLE_EQ(aReverseDistance, aDistance);

  double aSelfDistance = -1.0;
  ASSERT_EQ(occtl_graph_pair_distance_get(myGraph, aFirstVertex, aFirstVertex, &aSelfDistance),
            OCCTL_OK);
  EXPECT_DOUBLE_EQ(aSelfDistance, 0.0);
}

TEST_F(GraphCacheTest, CacheMeasureGet_WrongKind_ReturnsWrongKind)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  double aValue = 0.0;
  EXPECT_EQ(occtl_graph_measure_get(myGraph, aFaceId, OCCTL_SELECT_MEASURE_EDGE_LENGTH, &aValue),
            OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(GraphCacheTest, CacheGetters_InvalidArgs_ReturnInvalidArgument)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_select_bbox_t aBox{};
  EXPECT_EQ(occtl_graph_bbox_get(nullptr, aFaceId, &aBox), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_bbox_get(myGraph, aFaceId, nullptr), OCCTL_INVALID_ARGUMENT);
  occtl_graph_obb_t anObb{};
  EXPECT_EQ(occtl_graph_obb_get(nullptr, aFaceId, &anObb), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_obb_get(myGraph, aFaceId, nullptr), OCCTL_INVALID_ARGUMENT);
  occtl_graph_uv_bounds_t aBounds{};
  EXPECT_EQ(occtl_graph_face_uv_bounds_get(nullptr, aFaceId, &aBounds), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_face_uv_bounds_get(myGraph, aFaceId, nullptr), OCCTL_INVALID_ARGUMENT);
  double aValue = 0.0;
  EXPECT_EQ(
    occtl_graph_measure_get(myGraph, aFaceId, OCCTL_SELECT_MEASURE_KIND_RESERVED_FUTURE, &aValue),
    OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_measure_get(myGraph, aFaceId, OCCTL_SELECT_MEASURE_FACE_AREA, nullptr),
            OCCTL_INVALID_ARGUMENT);
  occtl_graph_mass_properties_t aProperties{};
  EXPECT_EQ(occtl_graph_mass_properties_get(nullptr, aFaceId, &aProperties),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_mass_properties_get(myGraph, aFaceId, nullptr), OCCTL_INVALID_ARGUMENT);
  occtl_curve_kind_t aCurveKind = OCCTL_CURVE_KIND_UNDEFINED;
  EXPECT_EQ(occtl_graph_edge_curve_kind_get(nullptr, aFaceId, &aCurveKind), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_edge_curve_kind_get(myGraph, aFaceId, nullptr), OCCTL_INVALID_ARGUMENT);
  occtl_surface_kind_t aSurfaceKind = OCCTL_SURFACE_KIND_UNDEFINED;
  EXPECT_EQ(occtl_graph_face_surface_kind_get(nullptr, aFaceId, &aSurfaceKind),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_face_surface_kind_get(myGraph, aFaceId, nullptr), OCCTL_INVALID_ARGUMENT);
  size_t aCount = 0;
  EXPECT_EQ(occtl_graph_descendant_vertices_get(nullptr, aFaceId, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_descendant_vertices_get(myGraph, aFaceId, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_descendant_edges_get(nullptr, aFaceId, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_descendant_edges_get(myGraph, aFaceId, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_descendant_faces_get(nullptr, aFaceId, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_descendant_faces_get(myGraph, aFaceId, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_descendants_get(nullptr, aFaceId, OCCTL_KIND_FACE, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_descendants_get(myGraph, aFaceId, OCCTL_KIND_INVALID, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_descendants_get(myGraph, aFaceId, OCCTL_KIND_FACE, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_adjacent_faces_get(nullptr, aFaceId, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_adjacent_faces_get(myGraph, aFaceId, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_adjacent_edges_get(nullptr, aFaceId, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_adjacent_edges_get(myGraph, aFaceId, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_pair_distance_get(nullptr, aFaceId, aFaceId, &aValue),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_pair_distance_get(myGraph, aFaceId, aFaceId, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphCacheTest, ClearCached_NodeAndRefTargets_ReturnExpectedStatuses)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);

  double anArea = 0.0;
  ASSERT_EQ(occtl_graph_measure_get(myGraph, aFaceId, OCCTL_SELECT_MEASURE_FACE_AREA, &anArea),
            OCCTL_OK);
  EXPECT_EQ(occtl_graph_clear_cached(myGraph, aFaceId, OCCTL_REF_ID_INVALID), OCCTL_OK);

  size_t aRefCount = 0;
  ASSERT_EQ(occtl_graph_ref_uid_table(myGraph, nullptr, nullptr, 0, &aRefCount), OCCTL_OK);
  ASSERT_GT(aRefCount, 0u);
  std::vector<occtl_ref_uid_t> aRefUids(aRefCount);
  std::vector<occtl_ref_id_t>  aRefs(aRefCount);
  ASSERT_EQ(
    occtl_graph_ref_uid_table(myGraph, aRefUids.data(), aRefs.data(), aRefs.size(), &aRefCount),
    OCCTL_OK);
  EXPECT_EQ(occtl_graph_clear_cached(myGraph, OCCTL_NODE_ID_INVALID, aRefs.front()), OCCTL_OK);

  EXPECT_EQ(occtl_graph_clear_cached(nullptr, aFaceId, OCCTL_REF_ID_INVALID),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_clear_cached(myGraph, OCCTL_NODE_ID_INVALID, OCCTL_REF_ID_INVALID),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_clear_cached(myGraph, aFaceId, aRefs.front()), OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphCacheTest, GraphClone_ComputesValuesOnClone)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_uid_t aFaceUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(myGraph, aFaceId, &aFaceUid), OCCTL_OK);

  occtl_graph_t* aClone = nullptr;
  ASSERT_EQ(occtl_graph_clone(myGraph, &aClone), OCCTL_OK);
  ASSERT_NE(aClone, nullptr);

  occtl_node_id_t aCloneFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_graph_node_id_from_uid(aClone, aFaceUid, &aCloneFace), OCCTL_OK);

  occtl_select_bbox_t aBox{};
  ASSERT_EQ(occtl_graph_bbox_get(aClone, aCloneFace, &aBox), OCCTL_OK);
  EXPECT_LE(aBox.min.x, aBox.max.x);

  occtl_graph_free(aClone);
}

TEST(GraphCacheVeneerTest, GraphCacheMethods_ReturnComputedValues)
{
  occtl::Graph aGraph;
  loadBox(aGraph.get());

  const occtl::NodeId aFace(firstAbiNodeOfKind(aGraph.get(), OCCTL_KIND_FACE));
  ASSERT_TRUE(aFace.is_valid());

  const occtl_select_bbox_t aBox = aGraph.bbox_get(aFace);
  EXPECT_LE(aBox.min.x, aBox.max.x);
  EXPECT_LE(aBox.min.y, aBox.max.y);
  EXPECT_LE(aBox.min.z, aBox.max.z);

  const occtl_graph_obb_t anObb = aGraph.obb_get(aFace);
  EXPECT_GT(anObb.x_half_size, 0.0);

  const occtl_graph_uv_bounds_t aBounds = aGraph.face_uv_bounds_get(aFace);
  EXPECT_LE(aBounds.u_min, aBounds.u_max);

  const double anArea = aGraph.measure_get(aFace, OCCTL_SELECT_MEASURE_FACE_AREA);
  EXPECT_GT(anArea, 0.0);

  const occtl_graph_mass_properties_t aProperties = aGraph.mass_properties_get(aFace);
  EXPECT_GT(aProperties.surface_area, 0.0);

  EXPECT_EQ(aGraph.face_surface_kind_get(aFace), OCCTL_SURFACE_KIND_PLANE);

  const std::vector<occtl::NodeId> aDescendantVertices = aGraph.descendant_vertices_get(aFace);
  EXPECT_EQ(aDescendantVertices.size(), 4u);

  const std::vector<occtl::NodeId> aDescendantEdges = aGraph.descendant_edges_get(aFace);
  EXPECT_EQ(aDescendantEdges.size(), 4u);

  const std::vector<occtl::NodeId> aDescendantFaces = aGraph.descendant_faces_get(aFace);
  EXPECT_EQ(aDescendantFaces.size(), 1u);

  const std::vector<occtl::NodeId> aGenericDescendantEdges =
    aGraph.descendants_get(aFace, OCCTL_KIND_EDGE);
  EXPECT_EQ(aGenericDescendantEdges.size(), 4u);

  const std::vector<occtl::NodeId> anAdjacentFaces = aGraph.adjacent_faces_get(aFace);
  EXPECT_EQ(anAdjacentFaces.size(), 4u);

  const occtl::NodeId anEdge(firstAbiNodeOfKind(aGraph.get(), OCCTL_KIND_EDGE));
  ASSERT_TRUE(anEdge.is_valid());
  const std::vector<occtl::NodeId> anAdjacentEdges = aGraph.adjacent_edges_get(anEdge);
  EXPECT_GE(anAdjacentEdges.size(), 2u);

  EXPECT_EQ(aGraph.edge_curve_kind_get(anEdge), OCCTL_CURVE_KIND_LINE);

  const double aDistance = aGraph.pair_distance_get(aFace, aFace);
  EXPECT_DOUBLE_EQ(aDistance, 0.0);
}

} // namespace
