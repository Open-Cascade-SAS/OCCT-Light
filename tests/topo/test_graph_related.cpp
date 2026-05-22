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

#include "test_helpers_internal.hxx"

#include <gp_Pnt.hxx>

#include <vector>

namespace
{

class TopoRelatedTest : public ::testing::Test
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

TEST_F(TopoRelatedTest, DistancePair_TwoVertices_ReturnsClosestPoints)
{
  occtl_topo_make_vertex_info_t aInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aInfo.tolerance                     = 1e-7;

  aInfo.point              = {0.0, 0.0, 0.0};
  occtl_node_id_t aVertexA = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aInfo, &aVertexA), OCCTL_OK);

  aInfo.point              = {3.0, 4.0, 0.0};
  occtl_node_id_t aVertexB = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aInfo, &aVertexB), OCCTL_OK);

  occtl_topo_distance_pair_t aPair{};
  ASSERT_EQ(occtl_topo_distance_pair(myGraph, aVertexA, aVertexB, &aPair), OCCTL_OK);

  EXPECT_DOUBLE_EQ(aPair.distance, 5.0);
  EXPECT_DOUBLE_EQ(aPair.point_a.x, 0.0);
  EXPECT_DOUBLE_EQ(aPair.point_a.y, 0.0);
  EXPECT_DOUBLE_EQ(aPair.point_a.z, 0.0);
  EXPECT_DOUBLE_EQ(aPair.point_b.x, 3.0);
  EXPECT_DOUBLE_EQ(aPair.point_b.y, 4.0);
  EXPECT_DOUBLE_EQ(aPair.point_b.z, 0.0);
  EXPECT_EQ(aPair.support_a.bits, aVertexA.bits);
  EXPECT_EQ(aPair.support_b.bits, aVertexB.bits);
  EXPECT_EQ(aPair.inner_solution, 0);
  EXPECT_GE(aPair.solution_count, 1);
}

TEST_F(TopoRelatedTest, DistancePair_InvalidNode_ReturnsNotFound)
{
  const occtl_node_id_t aVertex = firstAbiNodeOfKind(myGraph, OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertex.bits, 0u);

  occtl_topo_distance_pair_t aPair{};
  EXPECT_EQ(occtl_topo_distance_pair(myGraph, aVertex, OCCTL_NODE_ID_INVALID, &aPair),
            OCCTL_NOT_FOUND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, DistancePair_NullOutParam_ReturnsInvalidArgument)
{
  const occtl_node_id_t aVertex = firstAbiNodeOfKind(myGraph, OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertex.bits, 0u);

  EXPECT_EQ(occtl_topo_distance_pair(myGraph, aVertex, aVertex, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, RelationOptionsInit_HasDefaults)
{
  occtl_topo_relation_options_t anOptions;
  occtl_topo_relation_options_init(&anOptions);

  EXPECT_EQ(anOptions.struct_version, OCCTL_TOPO_RELATION_OPTIONS_VERSION_1);
  EXPECT_EQ(anOptions.p_next, nullptr);
  EXPECT_DOUBLE_EQ(anOptions.tolerance, 1.0e-7);
  EXPECT_EQ(anOptions.include_tangent_contacts, 1);
  EXPECT_EQ(anOptions.include_overlaps, 1);
  EXPECT_EQ(anOptions.include_lower_dimension_results, 1);
}

TEST_F(TopoRelatedTest, TouchIter_TouchingBoxes_ReturnsContactHits)
{
  TopoDS_Shape aBoxB =
    BRepPrimAPI_MakeBox(gp_Pnt(10.0, 0.0, 0.0), gp_Pnt(20.0, 20.0, 30.0)).Shape();
  BRepGraph::ShapesView::Result aRes = myGraph->graph.Shapes().Add(aBoxB);
  ASSERT_TRUE(aRes.Ok);

  std::vector<occtl_node_id_t> aSolids;
  for (BRepGraph_SolidIterator anIt(myGraph->graph); anIt.More(); anIt.Next())
  {
    aSolids.push_back(OcctL::Topo::PackNodeId(anIt.CurrentId()));
  }
  ASSERT_GE(aSolids.size(), 2u);

  occtl_topo_relation_options_t anOptions = OCCTL_TOPO_RELATION_OPTIONS_INIT;
  anOptions.tolerance                     = 1.0e-6;

  occtl_topo_touch_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_touch_iter_create(myGraph, aSolids[0], aSolids[1], &anOptions, &anIter),
            OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  std::vector<occtl_topo_touch_hit_t> aHits;
  occtl_topo_touch_hit_t              aHit{};
  while (occtl_topo_touch_iter_next(anIter, &aHit) == OCCTL_OK)
  {
    aHits.push_back(aHit);
  }
  occtl_topo_touch_iter_free(anIter);

  ASSERT_GT(aHits.size(), 0u);
  EXPECT_EQ(aHits.front().node_a.bits, aSolids[0].bits);
  EXPECT_EQ(aHits.front().node_b.bits, aSolids[1].bits);
  EXPECT_LE(aHits.front().distance, anOptions.tolerance);
  EXPECT_NE(aHits.front().support_a.bits, 0u);
  EXPECT_NE(aHits.front().support_b.bits, 0u);
}

TEST_F(TopoRelatedTest, TouchIter_SeparatedBoxes_ReturnsEmptyIterator)
{
  TopoDS_Shape aBoxB =
    BRepPrimAPI_MakeBox(gp_Pnt(30.0, 0.0, 0.0), gp_Pnt(40.0, 20.0, 30.0)).Shape();
  BRepGraph::ShapesView::Result aRes = myGraph->graph.Shapes().Add(aBoxB);
  ASSERT_TRUE(aRes.Ok);

  std::vector<occtl_node_id_t> aSolids;
  for (BRepGraph_SolidIterator anIt(myGraph->graph); anIt.More(); anIt.Next())
  {
    aSolids.push_back(OcctL::Topo::PackNodeId(anIt.CurrentId()));
  }
  ASSERT_GE(aSolids.size(), 2u);

  occtl_topo_touch_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_touch_iter_create(myGraph, aSolids[0], aSolids[1], nullptr, &anIter),
            OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  occtl_topo_touch_hit_t aHit{};
  EXPECT_EQ(occtl_topo_touch_iter_next(anIter, &aHit), OCCTL_NOT_FOUND);
  occtl_topo_touch_iter_free(anIter);
}

TEST_F(TopoRelatedTest, TouchIter_InvalidArguments_ReturnExpectedStatuses)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_topo_touch_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_touch_iter_create(nullptr, aSolid, aSolid, nullptr, &anIter),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_touch_iter_create(myGraph, aSolid, aSolid, nullptr, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_touch_iter_create(myGraph, aSolid, OCCTL_NODE_ID_INVALID, nullptr, &anIter),
            OCCTL_NOT_FOUND);

  occtl_topo_relation_options_t anOptions = OCCTL_TOPO_RELATION_OPTIONS_INIT;
  anOptions.struct_version                = 0u;
  EXPECT_EQ(occtl_topo_touch_iter_create(myGraph, aSolid, aSolid, &anOptions, &anIter),
            OCCTL_VERSION_MISMATCH);

  anOptions           = OCCTL_TOPO_RELATION_OPTIONS_INIT;
  anOptions.tolerance = -1.0;
  EXPECT_EQ(occtl_topo_touch_iter_create(myGraph, aSolid, aSolid, &anOptions, &anIter),
            OCCTL_INVALID_ARGUMENT);

  anOptions                          = OCCTL_TOPO_RELATION_OPTIONS_INIT;
  anOptions.include_tangent_contacts = 2;
  EXPECT_EQ(occtl_topo_touch_iter_create(myGraph, aSolid, aSolid, &anOptions, &anIter),
            OCCTL_INVALID_ARGUMENT);

  EXPECT_EQ(occtl_topo_touch_iter_next(nullptr, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, IntersectionIter_OverlappingBoxes_ReturnsGeneratedEdges)
{
  TopoDS_Shape aBoxB = BRepPrimAPI_MakeBox(gp_Pnt(5.0, 5.0, 5.0), gp_Pnt(15.0, 15.0, 15.0)).Shape();
  BRepGraph::ShapesView::Result aRes = myGraph->graph.Shapes().Add(aBoxB);
  ASSERT_TRUE(aRes.Ok);

  std::vector<occtl_node_id_t> aSolids;
  for (BRepGraph_SolidIterator anIt(myGraph->graph); anIt.More(); anIt.Next())
  {
    aSolids.push_back(OcctL::Topo::PackNodeId(anIt.CurrentId()));
  }
  ASSERT_GE(aSolids.size(), 2u);

  occtl_topo_relation_options_t anOptions = OCCTL_TOPO_RELATION_OPTIONS_INIT;
  anOptions.include_overlaps              = 0;

  occtl_topo_intersection_iter_t* anIter = nullptr;
  ASSERT_EQ(
    occtl_topo_intersection_iter_create(myGraph, aSolids[0], aSolids[1], &anOptions, &anIter),
    OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  std::vector<occtl_node_id_t> aNodes;
  occtl_node_id_t              aNode = OCCTL_NODE_ID_INVALID;
  while (occtl_topo_intersection_iter_next(anIter, &aNode) == OCCTL_OK)
  {
    aNodes.push_back(aNode);
  }
  occtl_topo_intersection_iter_free(anIter);

  ASSERT_GT(aNodes.size(), 0u);
  for (const occtl_node_id_t aResultNode : aNodes)
  {
    occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
    ASSERT_EQ(occtl_graph_node_kind(myGraph, aResultNode, &aKind), OCCTL_OK);
    EXPECT_TRUE(aKind == OCCTL_KIND_EDGE || aKind == OCCTL_KIND_VERTEX);
  }
}

TEST_F(TopoRelatedTest, IntersectionIter_DefaultOptions_IncludesOverlapFaces)
{
  TopoDS_Shape aBoxB = BRepPrimAPI_MakeBox(gp_Pnt(5.0, 5.0, 5.0), gp_Pnt(15.0, 15.0, 15.0)).Shape();
  BRepGraph::ShapesView::Result aRes = myGraph->graph.Shapes().Add(aBoxB);
  ASSERT_TRUE(aRes.Ok);

  std::vector<occtl_node_id_t> aSolids;
  for (BRepGraph_SolidIterator anIt(myGraph->graph); anIt.More(); anIt.Next())
  {
    aSolids.push_back(OcctL::Topo::PackNodeId(anIt.CurrentId()));
  }
  ASSERT_GE(aSolids.size(), 2u);

  occtl_topo_intersection_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_intersection_iter_create(myGraph, aSolids[0], aSolids[1], nullptr, &anIter),
            OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  bool            hasFace = false;
  occtl_node_id_t aNode   = OCCTL_NODE_ID_INVALID;
  while (occtl_topo_intersection_iter_next(anIter, &aNode) == OCCTL_OK)
  {
    occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
    ASSERT_EQ(occtl_graph_node_kind(myGraph, aNode, &aKind), OCCTL_OK);
    hasFace = hasFace || aKind == OCCTL_KIND_FACE;
  }
  occtl_topo_intersection_iter_free(anIter);

  EXPECT_TRUE(hasFace);
}

TEST_F(TopoRelatedTest, IntersectionIter_SeparatedBoxes_ReturnsEmptyIterator)
{
  TopoDS_Shape aBoxB =
    BRepPrimAPI_MakeBox(gp_Pnt(30.0, 0.0, 0.0), gp_Pnt(40.0, 20.0, 30.0)).Shape();
  BRepGraph::ShapesView::Result aRes = myGraph->graph.Shapes().Add(aBoxB);
  ASSERT_TRUE(aRes.Ok);

  std::vector<occtl_node_id_t> aSolids;
  for (BRepGraph_SolidIterator anIt(myGraph->graph); anIt.More(); anIt.Next())
  {
    aSolids.push_back(OcctL::Topo::PackNodeId(anIt.CurrentId()));
  }
  ASSERT_GE(aSolids.size(), 2u);

  occtl_topo_intersection_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_intersection_iter_create(myGraph, aSolids[0], aSolids[1], nullptr, &anIter),
            OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_intersection_iter_next(anIter, &aNode), OCCTL_NOT_FOUND);
  occtl_topo_intersection_iter_free(anIter);
}

TEST_F(TopoRelatedTest, IntersectionIter_InvalidArguments_ReturnExpectedStatuses)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_topo_intersection_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_intersection_iter_create(nullptr, aSolid, aSolid, nullptr, &anIter),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_intersection_iter_create(myGraph, aSolid, aSolid, nullptr, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(
    occtl_topo_intersection_iter_create(myGraph, aSolid, OCCTL_NODE_ID_INVALID, nullptr, &anIter),
    OCCTL_NOT_FOUND);

  occtl_topo_relation_options_t anOptions = OCCTL_TOPO_RELATION_OPTIONS_INIT;
  anOptions.struct_version                = 0u;
  EXPECT_EQ(occtl_topo_intersection_iter_create(myGraph, aSolid, aSolid, &anOptions, &anIter),
            OCCTL_VERSION_MISMATCH);

  EXPECT_EQ(occtl_topo_intersection_iter_next(nullptr, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, FacesIntersectedByAxis_BoxSolid_ReturnsOrderedFaceHits)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  const occtl_axis1_placement_t anAxis = {{-5.0, 10.0, 15.0}, {1.0, 0.0, 0.0}};
  occtl_topo_axis_hit_iter_t*   anIter = nullptr;
  ASSERT_EQ(occtl_topo_axis_intersect_faces(myGraph, aSolid, anAxis, 0.0, 20.0, 1e-7, &anIter),
            OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  std::vector<occtl_topo_axis_hit_t> aHits;
  occtl_topo_axis_hit_t              aHit{};
  while (occtl_topo_axis_hit_iter_next(anIter, &aHit) == OCCTL_OK)
  {
    aHits.push_back(aHit);
  }
  occtl_topo_axis_hit_iter_free(anIter);

  ASSERT_EQ(aHits.size(), 2u);
  EXPECT_NE(aHits[0].face.bits, 0u);
  EXPECT_NE(aHits[1].face.bits, 0u);
  EXPECT_NE(aHits[0].face.bits, aHits[1].face.bits);
  EXPECT_NEAR(aHits[0].parameter, 5.0, 1e-7);
  EXPECT_NEAR(aHits[1].parameter, 15.0, 1e-7);
  EXPECT_NEAR(aHits[0].point.x, 0.0, 1e-7);
  EXPECT_NEAR(aHits[1].point.x, 10.0, 1e-7);
  EXPECT_NEAR(aHits[0].point.y, 10.0, 1e-7);
  EXPECT_NEAR(aHits[0].point.z, 15.0, 1e-7);
  EXPECT_NEAR(aHits[1].point.y, 10.0, 1e-7);
  EXPECT_NEAR(aHits[1].point.z, 15.0, 1e-7);
  EXPECT_EQ(aHits[0].location, OCCTL_TOPO_POINT_CLASS_IN);
  EXPECT_EQ(aHits[1].location, OCCTL_TOPO_POINT_CLASS_IN);
  EXPECT_EQ(aHits[0].has_normal, 1);
  EXPECT_EQ(aHits[1].has_normal, 1);
}

TEST_F(TopoRelatedTest, FacesIntersectedByAxis_MissedBox_ReturnsEmptyIterator)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  const occtl_axis1_placement_t anAxis = {{-5.0, 50.0, 15.0}, {1.0, 0.0, 0.0}};
  occtl_topo_axis_hit_iter_t*   anIter = nullptr;
  ASSERT_EQ(occtl_topo_axis_intersect_faces(myGraph, aSolid, anAxis, 0.0, 20.0, 1e-7, &anIter),
            OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  occtl_topo_axis_hit_t aHit{};
  EXPECT_EQ(occtl_topo_axis_hit_iter_next(anIter, &aHit), OCCTL_NOT_FOUND);
  occtl_topo_axis_hit_iter_free(anIter);
}

TEST_F(TopoRelatedTest, FacesIntersectedByAxis_InvalidArguments_ReturnInvalidArgument)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  const occtl_axis1_placement_t aValidAxis = {{-5.0, 10.0, 15.0}, {1.0, 0.0, 0.0}};
  occtl_topo_axis_hit_iter_t*   anIter     = nullptr;

  EXPECT_EQ(occtl_topo_axis_intersect_faces(nullptr, aSolid, aValidAxis, 0.0, 20.0, 1e-7, &anIter),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_axis_intersect_faces(myGraph, aSolid, aValidAxis, 20.0, 0.0, 1e-7, &anIter),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_axis_intersect_faces(myGraph, aSolid, aValidAxis, 0.0, 20.0, -1.0, &anIter),
            OCCTL_INVALID_ARGUMENT);

  const occtl_axis1_placement_t aBadAxis = {{-5.0, 10.0, 15.0}, {0.0, 0.0, 0.0}};
  EXPECT_EQ(occtl_topo_axis_intersect_faces(myGraph, aSolid, aBadAxis, 0.0, 20.0, 1e-7, &anIter),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_axis_intersect_faces(myGraph, aSolid, aValidAxis, 0.0, 20.0, 1e-7, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, IsSameGeometry_SameFace_ReturnsTrue)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  int32_t aFlag = 0;
  ASSERT_EQ(occtl_topo_is_same_geometry(myGraph, aFace, aFace, 1e-7, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);
}

TEST_F(TopoRelatedTest, IsSameGeometry_DistinctBoxFaces_ReturnsFalse)
{
  std::vector<occtl_node_id_t> aFaces;
  for (BRepGraph_FaceIterator anIt(myGraph->graph); anIt.More(); anIt.Next())
  {
    aFaces.push_back(OcctL::Topo::PackNodeId(anIt.CurrentId()));
  }
  ASSERT_GE(aFaces.size(), 2u);

  int32_t aFlag = 1;
  ASSERT_EQ(occtl_topo_is_same_geometry(myGraph, aFaces[0], aFaces[1], 1e-7, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 0);
}

TEST_F(TopoRelatedTest, IsSameGeometry_SameEdge_ReturnsTrue)
{
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  int32_t aFlag = 0;
  ASSERT_EQ(occtl_topo_is_same_geometry(myGraph, anEdge, anEdge, 1e-7, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);
}

TEST_F(TopoRelatedTest, IsSameGeometry_WrongKind_ReturnsWrongKind)
{
  const occtl_node_id_t aFace  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aFace.bits, 0u);
  ASSERT_NE(aSolid.bits, 0u);

  int32_t aFlag = 0;
  EXPECT_EQ(occtl_topo_is_same_geometry(myGraph, aFace, aSolid, 1e-7, &aFlag), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, IsSameGeometry_InvalidArguments_ReturnInvalidArgument)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  int32_t aFlag = 0;
  EXPECT_EQ(occtl_topo_is_same_geometry(nullptr, aFace, aFace, 1e-7, &aFlag),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_is_same_geometry(myGraph, aFace, aFace, -1.0, &aFlag),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_is_same_geometry(myGraph, aFace, aFace, 1e-7, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoRelatedTest, CommonVertices_TwoEdgesSharingVertex_ReturnsSharedVertex)
{
  occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                     = 1e-7;

  aVertInfo.point         = {0.0, 0.0, 0.0};
  occtl_node_id_t aCenter = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aCenter), OCCTL_OK);

  aVertInfo.point          = {1.0, 0.0, 0.0};
  occtl_node_id_t aVertexX = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aVertexX), OCCTL_OK);

  aVertInfo.point          = {0.0, 1.0, 0.0};
  occtl_node_id_t aVertexY = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertInfo, &aVertexY), OCCTL_OK);

  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.start_vertex                = aCenter;
  anEdgeInfo.end_vertex                  = aVertexX;
  anEdgeInfo.first                       = 0.0;
  anEdgeInfo.last                        = 1.0;
  anEdgeInfo.tolerance                   = 1e-7;
  occtl_node_id_t anEdgeX                = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &anEdgeX), OCCTL_OK);

  anEdgeInfo.end_vertex   = aVertexY;
  occtl_node_id_t anEdgeY = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_edge(myGraph, &anEdgeInfo, &anEdgeY), OCCTL_OK);

  size_t aCount = 0;
  ASSERT_EQ(occtl_topo_common_vertices(myGraph, anEdgeX, anEdgeY, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 1u);

  occtl_node_id_t aCommon = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_common_vertices(myGraph, anEdgeX, anEdgeY, &aCommon, 1, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);
  EXPECT_EQ(aCommon.bits, aCenter.bits);
}

TEST_F(TopoRelatedTest, CommonVertices_BufferTooSmall_ReturnsRequiredCount)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aSolid.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  occtl_node_id_t aBuffer = OCCTL_NODE_ID_INVALID;
  size_t          aCount  = 0;
  EXPECT_EQ(occtl_topo_common_vertices(myGraph, aSolid, aFace, &aBuffer, 1, &aCount),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_GT(aCount, 1u);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, CommonVertices_InvalidNode_ReturnsNotFound)
{
  const occtl_node_id_t aVertex = firstAbiNodeOfKind(myGraph, OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertex.bits, 0u);

  size_t aCount = 0;
  EXPECT_EQ(
    occtl_topo_common_vertices(myGraph, aVertex, OCCTL_NODE_ID_INVALID, nullptr, 0, &aCount),
    OCCTL_NOT_FOUND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, CommonVertices_NullCount_ReturnsInvalidArgument)
{
  const occtl_node_id_t aVertex = firstAbiNodeOfKind(myGraph, OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertex.bits, 0u);

  EXPECT_EQ(occtl_topo_common_vertices(myGraph, aVertex, aVertex, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, AdjacentEdges_FromBoxEdge_ReturnsFourNeighbours)
{
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_topo_adjacent_edges(myGraph, anEdge, nullptr, 0, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 4u);

  std::vector<occtl_node_id_t> anEdges(aCount);
  ASSERT_EQ(occtl_topo_adjacent_edges(myGraph, anEdge, anEdges.data(), anEdges.size(), &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 4u);
  for (const occtl_node_id_t anAdjacent : anEdges)
  {
    EXPECT_NE(anAdjacent.bits, anEdge.bits);
  }
}

TEST_F(TopoRelatedTest, AdjacentFaces_FromBoxFace_ReturnsFourNeighbours)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_topo_adjacent_faces(myGraph, aFace, nullptr, 0, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 4u);

  std::vector<occtl_node_id_t> aFaces(aCount);
  ASSERT_EQ(occtl_topo_adjacent_faces(myGraph, aFace, aFaces.data(), aFaces.size(), &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 4u);
  for (const occtl_node_id_t anAdjacent : aFaces)
  {
    EXPECT_NE(anAdjacent.bits, aFace.bits);
  }
}

TEST_F(TopoRelatedTest, AdjacentQueries_InvalidArgs_ReturnErrors)
{
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  const occtl_node_id_t aFace  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(anEdge.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  size_t aCount = 0;
  EXPECT_EQ(occtl_topo_adjacent_edges(nullptr, anEdge, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_adjacent_edges(myGraph, aFace, nullptr, 0, &aCount), OCCTL_WRONG_KIND);
  EXPECT_EQ(occtl_topo_adjacent_faces(myGraph, anEdge, nullptr, 0, &aCount), OCCTL_WRONG_KIND);
  EXPECT_EQ(occtl_topo_adjacent_faces(myGraph, aFace, nullptr, 0, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, ConnectedEdges_FromBoxEdge_ReturnsAllBoxEdges)
{
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_topo_connected_edges(myGraph, anEdge, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 12u);

  std::vector<occtl_node_id_t> anEdges(aCount);
  ASSERT_EQ(occtl_topo_connected_edges(myGraph, anEdge, anEdges.data(), anEdges.size(), &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 12u);
  EXPECT_EQ(anEdges.front().bits, anEdge.bits);
}

TEST_F(TopoRelatedTest, ConnectedFaces_FromBoxFace_ReturnsAllBoxFaces)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_topo_connected_faces(myGraph, aFace, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 6u);

  std::vector<occtl_node_id_t> aFaces(aCount);
  ASSERT_EQ(occtl_topo_connected_faces(myGraph, aFace, aFaces.data(), aFaces.size(), &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 6u);
  EXPECT_EQ(aFaces.front().bits, aFace.bits);
}

TEST_F(TopoRelatedTest, ConnectedEdges_WrongKind_ReturnsWrongKind)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  size_t aCount = 0;
  EXPECT_EQ(occtl_topo_connected_edges(myGraph, aFace, nullptr, 0, &aCount), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, ConnectedFaces_BufferTooSmall_ReturnsRequiredCount)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  occtl_node_id_t aBuffer = OCCTL_NODE_ID_INVALID;
  size_t          aCount  = 0;
  EXPECT_EQ(occtl_topo_connected_faces(myGraph, aFace, &aBuffer, 1, &aCount),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aCount, 6u);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, GraphDistance_FacesUnderSolid_ReturnsHopCounts)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aSolid.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_topo_connected_faces(myGraph, aFace, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 6u);

  std::vector<occtl_node_id_t> aFaces(aCount);
  ASSERT_EQ(occtl_topo_connected_faces(myGraph, aFace, aFaces.data(), aFaces.size(), &aCount),
            OCCTL_OK);

  int aZeroCount = 0;
  int aOneCount  = 0;
  int aTwoCount  = 0;
  for (const occtl_node_id_t& aTarget : aFaces)
  {
    int32_t aDistance = -2;
    ASSERT_EQ(occtl_topo_graph_distance(myGraph, aSolid, &aFace, 1, aTarget, &aDistance), OCCTL_OK);
    if (aDistance == 0)
    {
      ++aZeroCount;
    }
    else if (aDistance == 1)
    {
      ++aOneCount;
    }
    else if (aDistance == 2)
    {
      ++aTwoCount;
    }
    else
    {
      ADD_FAILURE() << "unexpected graph distance " << aDistance;
    }
  }

  EXPECT_EQ(aZeroCount, 1);
  EXPECT_EQ(aOneCount, 4);
  EXPECT_EQ(aTwoCount, 1);
}

TEST_F(TopoRelatedTest, GraphDistance_MultipleSources_UsesNearestSource)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aSolid.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  size_t aCount = 0;
  ASSERT_EQ(occtl_topo_connected_faces(myGraph, aFace, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 6u);

  std::vector<occtl_node_id_t> aFaces(aCount);
  ASSERT_EQ(occtl_topo_connected_faces(myGraph, aFace, aFaces.data(), aFaces.size(), &aCount),
            OCCTL_OK);

  occtl_node_id_t aSources[2] = {aFace, aFaces.back()};
  int32_t         aDistance   = -2;
  ASSERT_EQ(occtl_topo_graph_distance(myGraph, aSolid, aSources, 2, aFaces.back(), &aDistance),
            OCCTL_OK);
  EXPECT_EQ(aDistance, 0);
}

TEST_F(TopoRelatedTest, GraphDistance_InvalidArgs_ReturnInvalidArgument)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aSolid.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  int32_t aDistance = -1;
  EXPECT_EQ(occtl_topo_graph_distance(nullptr, aSolid, &aFace, 1, aFace, &aDistance),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_graph_distance(myGraph, aSolid, nullptr, 1, aFace, &aDistance),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_graph_distance(myGraph, aSolid, &aFace, 0, aFace, &aDistance),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_graph_distance(myGraph, aSolid, &aFace, 1, aFace, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoRelatedTest, GraphDistance_WrongKind_ReturnsWrongKind)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aSolid.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);
  ASSERT_NE(anEdge.bits, 0u);

  int32_t aDistance = -1;
  EXPECT_EQ(occtl_topo_graph_distance(myGraph, aSolid, &aFace, 1, anEdge, &aDistance),
            OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, ClassifyPoint_BoxSolid_ReturnsInOutOn)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_topo_point_class_t aClass = OCCTL_TOPO_POINT_CLASS_UNKNOWN;
  ASSERT_EQ(occtl_topo_classify_point(myGraph, aSolid, {5.0, 10.0, 15.0}, 1e-7, &aClass), OCCTL_OK);
  EXPECT_EQ(aClass, OCCTL_TOPO_POINT_CLASS_IN);

  ASSERT_EQ(occtl_topo_classify_point(myGraph, aSolid, {20.0, 40.0, 60.0}, 1e-7, &aClass),
            OCCTL_OK);
  EXPECT_EQ(aClass, OCCTL_TOPO_POINT_CLASS_OUT);

  ASSERT_EQ(occtl_topo_classify_point(myGraph, aSolid, {0.0, 10.0, 15.0}, 1e-7, &aClass), OCCTL_OK);
  EXPECT_EQ(aClass, OCCTL_TOPO_POINT_CLASS_ON);
}

TEST_F(TopoRelatedTest, IsInside_BoxSolid_HonoursBoundaryMode)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  int32_t aFlag = 0;
  ASSERT_EQ(occtl_topo_is_inside(myGraph, aSolid, {5.0, 10.0, 15.0}, 1e-7, 1, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);

  ASSERT_EQ(occtl_topo_is_inside(myGraph, aSolid, {0.0, 10.0, 15.0}, 1e-7, 1, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);

  ASSERT_EQ(occtl_topo_is_inside(myGraph, aSolid, {0.0, 10.0, 15.0}, 1e-7, 0, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 0);
}

TEST_F(TopoRelatedTest, ClassifyPoint_WrongKind_ReturnsWrongKind)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  occtl_topo_point_class_t aClass = OCCTL_TOPO_POINT_CLASS_UNKNOWN;
  EXPECT_EQ(occtl_topo_classify_point(myGraph, aFace, {5.0, 10.0, 15.0}, 1e-7, &aClass),
            OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, IsInside_InvalidArguments_ReturnInvalidArgument)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  int32_t aFlag = 0;
  EXPECT_EQ(occtl_topo_is_inside(myGraph, aSolid, {5.0, 10.0, 15.0}, -1.0, 1, &aFlag),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_is_inside(myGraph, aSolid, {5.0, 10.0, 15.0}, 1e-7, 2, &aFlag),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_is_inside(myGraph, aSolid, {5.0, 10.0, 15.0}, 1e-7, 1, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoRelatedTest, Related_Face_YieldsNeighbours)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_topo_related_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_related_iter_create(myGraph, aFaceId, &anIter), OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  int  aCount             = 0;
  bool aFoundBoundaryEdge = false;
  bool aFoundOuterWire    = false;

  occtl_node_id_t       anId;
  occtl_relation_kind_t aKind;
  while (occtl_topo_related_iter_next(anIter, &anId, &aKind) == OCCTL_OK)
  {
    EXPECT_NE(anId.bits, 0u);
    if (aKind == OCCTL_RELATION_BOUNDARY_EDGE)
    {
      aFoundBoundaryEdge = true;
    }
    if (aKind == OCCTL_RELATION_OUTER_WIRE)
    {
      aFoundOuterWire = true;
    }
    ++aCount;
  }
  EXPECT_GT(aCount, 0);
  EXPECT_TRUE(aFoundBoundaryEdge);
  EXPECT_TRUE(aFoundOuterWire);
  occtl_topo_related_iter_free(anIter);
}

TEST_F(TopoRelatedTest, Related_NotFound)
{
  occtl_topo_related_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_related_iter_create(myGraph, OCCTL_NODE_ID_INVALID, &anIter),
            OCCTL_NOT_FOUND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoRelatedTest, Related_NullOutParam)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_topo_related_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_related_iter_create(myGraph, aFaceId, &anIter), OCCTL_OK);
  occtl_node_id_t anId;
  EXPECT_EQ(occtl_topo_related_iter_next(anIter, &anId, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  occtl_topo_related_iter_free(anIter);
}

} // anonymous namespace
