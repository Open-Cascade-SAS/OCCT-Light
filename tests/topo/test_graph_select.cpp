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

#include <gtest/gtest.h>

#include "test_helpers_internal.hxx"

#include "../src/topo/GraphMeasureCache.hxx"

#include <limits>
#include <string>
#include <vector>

namespace
{

std::vector<occtl_node_id_t> collectSelection(occtl_graph_t* const                theGraph,
                                              const occtl_select_options_t* const theOptions)
{
  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(theGraph, theOptions, &anIter), OCCTL_OK);
  EXPECT_NE(anIter, nullptr);

  std::vector<occtl_node_id_t> aNodes;
  for (;;)
  {
    occtl_node_id_t      aNode   = OCCTL_NODE_ID_INVALID;
    const occtl_status_t aStatus = occtl_select_iter_next(anIter, &aNode);
    if (aStatus == OCCTL_NOT_FOUND)
    {
      break;
    }
    EXPECT_EQ(aStatus, OCCTL_OK);
    EXPECT_NE(aNode.bits, 0u);
    aNodes.push_back(aNode);
  }
  occtl_select_iter_free(anIter);
  return aNodes;
}

std::vector<occtl_node_id_t> collectTaggedSelection(occtl_graph_t* const                theGraph,
                                                    const occtl_select_options_t* const theOptions,
                                                    const char* const                   theTag,
                                                    const size_t                        theTagLen)
{
  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_tagged_iter_create(theGraph, theOptions, theTag, theTagLen, &anIter),
            OCCTL_OK);
  EXPECT_NE(anIter, nullptr);

  std::vector<occtl_node_id_t> aNodes;
  for (;;)
  {
    occtl_node_id_t      aNode   = OCCTL_NODE_ID_INVALID;
    const occtl_status_t aStatus = occtl_select_iter_next(anIter, &aNode);
    if (aStatus == OCCTL_NOT_FOUND)
    {
      break;
    }
    EXPECT_EQ(aStatus, OCCTL_OK);
    EXPECT_NE(aNode.bits, 0u);
    aNodes.push_back(aNode);
  }
  occtl_select_iter_free(anIter);
  return aNodes;
}

struct CollectedGroup
{
  occtl_select_group_view_t    view = OCCTL_SELECT_GROUP_VIEW_INIT;
  std::vector<occtl_node_id_t> nodes;
  std::string                  name;
};

std::vector<CollectedGroup> collectGroups(occtl_graph_t* const                theGraph,
                                          const occtl_select_options_t* const theSelectOptions,
                                          const occtl_select_group_options_t* const theGroupOptions)
{
  occtl_select_group_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_group_iter_create(theGraph, theSelectOptions, theGroupOptions, &anIter),
            OCCTL_OK);
  EXPECT_NE(anIter, nullptr);

  std::vector<CollectedGroup> aGroups;
  for (;;)
  {
    occtl_select_group_view_t aView   = OCCTL_SELECT_GROUP_VIEW_INIT;
    const occtl_status_t      aStatus = occtl_select_group_iter_next(anIter, &aView);
    if (aStatus == OCCTL_NOT_FOUND)
    {
      break;
    }
    EXPECT_EQ(aStatus, OCCTL_OK);
    CollectedGroup aGroup;
    aGroup.view = aView;
    if (aView.name != nullptr && aView.name_len > 0u)
    {
      aGroup.name.assign(aView.name, aView.name + aView.name_len);
      aGroup.view.name = aGroup.name.data();
    }
    aGroup.nodes.assign(aView.nodes, aView.nodes + aView.node_count);
    aGroup.view.nodes = aGroup.nodes.data();
    aGroups.push_back(aGroup);
  }
  occtl_select_group_iter_free(anIter);
  return aGroups;
}

class GraphSelectTest : public ::testing::Test
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

TEST_F(GraphSelectTest, OptionsInit_HasExpectedDefaults)
{
  occtl_select_options_t aOptions;
  occtl_select_options_init(&aOptions);

  EXPECT_EQ(aOptions.struct_version, OCCTL_SELECT_OPTIONS_VERSION_1);
  EXPECT_EQ(aOptions.p_next, nullptr);
  EXPECT_EQ(aOptions.root.bits, 0u);
  EXPECT_EQ(aOptions.kind_mask, 0u);
  EXPECT_EQ(aOptions.include_root, 0);
  EXPECT_EQ(aOptions.name, nullptr);
  EXPECT_EQ(aOptions.name_len, 0u);
  EXPECT_EQ(aOptions.use_color, 0);
  EXPECT_EQ(aOptions.use_bbox, 0);
  EXPECT_EQ(aOptions.use_curve_kind, 0);
  EXPECT_EQ(aOptions.curve_kind, OCCTL_CURVE_KIND_UNDEFINED);
  EXPECT_EQ(aOptions.use_surface_kind, 0);
  EXPECT_EQ(aOptions.surface_kind, OCCTL_SURFACE_KIND_UNDEFINED);
  EXPECT_EQ(aOptions.use_axis_position, 0);
  EXPECT_EQ(aOptions.axis, OCCTL_SELECT_AXIS_Z);
  EXPECT_EQ(aOptions.axis_position, OCCTL_SELECT_AXIS_POSITION_MAX);
  EXPECT_DOUBLE_EQ(aOptions.axis_tolerance, 1.0e-7);
  EXPECT_EQ(aOptions.use_normal, 0);
  EXPECT_DOUBLE_EQ(aOptions.normal.x, 0.0);
  EXPECT_DOUBLE_EQ(aOptions.normal.y, 0.0);
  EXPECT_DOUBLE_EQ(aOptions.normal.z, 1.0);
  EXPECT_EQ(aOptions.normal_mode, OCCTL_SELECT_NORMAL_PARALLEL);
  EXPECT_DOUBLE_EQ(aOptions.normal_angle_tolerance, 1.0e-7);
  EXPECT_EQ(aOptions.use_measure, 0);
  EXPECT_EQ(aOptions.measure_kind, OCCTL_SELECT_MEASURE_FACE_AREA);
  EXPECT_DOUBLE_EQ(aOptions.measure_min, 0.0);
  EXPECT_DOUBLE_EQ(aOptions.measure_max, 0.0);
  EXPECT_EQ(aOptions.sort_key, OCCTL_SELECT_SORT_NONE);
  EXPECT_EQ(aOptions.sort_direction, OCCTL_SELECT_SORT_ASCENDING);
  EXPECT_EQ(aOptions.sort_axis, OCCTL_SELECT_AXIS_Z);
  EXPECT_EQ(aOptions.sort_measure_kind, OCCTL_SELECT_MEASURE_FACE_AREA);
  EXPECT_DOUBLE_EQ(aOptions.sort_point.x, 0.0);
  EXPECT_DOUBLE_EQ(aOptions.sort_point.y, 0.0);
  EXPECT_DOUBLE_EQ(aOptions.sort_point.z, 0.0);

  occtl_select_metadata_filter_t aMetadata = OCCTL_SELECT_METADATA_FILTER_INIT;
  EXPECT_EQ(aMetadata.struct_version, OCCTL_SELECT_METADATA_FILTER_VERSION_1);
  EXPECT_EQ(aMetadata.p_next, nullptr);
  EXPECT_EQ(aMetadata.key, nullptr);
  EXPECT_EQ(aMetadata.key_len, 0u);
  EXPECT_EQ(aMetadata.value, nullptr);
  EXPECT_EQ(aMetadata.value_len, 0u);
  EXPECT_EQ(aMetadata.match_value, 0);
}

TEST_F(GraphSelectTest, GroupOptionsInit_HasExpectedDefaults)
{
  occtl_select_group_options_t aOptions;
  occtl_select_group_options_init(&aOptions);

  EXPECT_EQ(aOptions.struct_version, OCCTL_SELECT_GROUP_OPTIONS_VERSION_1);
  EXPECT_EQ(aOptions.p_next, nullptr);
  EXPECT_EQ(aOptions.key, OCCTL_SELECT_GROUP_KIND);
  EXPECT_EQ(aOptions.axis, OCCTL_SELECT_AXIS_Z);
  EXPECT_DOUBLE_EQ(aOptions.tolerance, 1.0e-7);
  EXPECT_FLOAT_EQ(aOptions.color_tolerance, 0.0f);
  EXPECT_EQ(aOptions.include_missing, 0);
}

TEST_F(GraphSelectTest, ByKind_Faces_ReturnsSix)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 6u);
}

TEST_F(GraphSelectTest, ByNameAndColor_ReturnsTaggedFace)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  const char               aName[] = "named-face";
  const occtl_color_rgba_t aColor  = {0.2f, 0.4f, 0.6f, 1.0f};
  ASSERT_EQ(occtl_graph_name_set(myGraph, aFace, aName, sizeof(aName) - 1), OCCTL_OK);
  ASSERT_EQ(occtl_graph_color_set(myGraph, aFace, aColor), OCCTL_OK);

  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);
  aOptions.name                   = aName;
  aOptions.name_len               = sizeof(aName) - 1;
  aOptions.use_color              = 1;
  aOptions.color                  = aColor;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  ASSERT_EQ(aNodes.size(), 1u);
  EXPECT_EQ(aNodes[0].bits, aFace.bits);
}

TEST_F(GraphSelectTest, ByMetadataKeyAndValue_ReturnsTaggedFace)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  ASSERT_EQ(occtl_graph_node_metadata_set(myGraph, aFace, "role", 4, "mount", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_node_metadata_set(myGraph, aFace, "owner", 5, "fixture", 7), OCCTL_OK);

  occtl_select_options_t         aOptions  = OCCTL_SELECT_OPTIONS_INIT;
  occtl_select_metadata_filter_t aMetadata = OCCTL_SELECT_METADATA_FILTER_INIT;
  aOptions.kind_mask = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);
  aOptions.p_next    = &aMetadata;
  aMetadata.key      = "role";
  aMetadata.key_len  = 4;

  std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  ASSERT_EQ(aNodes.size(), 1u);
  EXPECT_EQ(aNodes[0].bits, aFace.bits);

  aMetadata.value       = "mount";
  aMetadata.value_len   = 5;
  aMetadata.match_value = 1;
  aNodes                = collectSelection(myGraph, &aOptions);
  ASSERT_EQ(aNodes.size(), 1u);
  EXPECT_EQ(aNodes[0].bits, aFace.bits);

  aMetadata.value     = "other";
  aMetadata.value_len = 5;
  aNodes              = collectSelection(myGraph, &aOptions);
  EXPECT_TRUE(aNodes.empty());
}

TEST_F(GraphSelectTest, ByGraphTagAndKind_ReturnsTaggedFace)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  ASSERT_EQ(occtl_graph_tag_add(myGraph, aFace, "mount", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_tag_add(myGraph, anEdge, "mount", 5), OCCTL_OK);

  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);

  const std::vector<occtl_node_id_t> aNodes =
    collectTaggedSelection(myGraph, &aOptions, "mount", 5);
  ASSERT_EQ(aNodes.size(), 1u);
  EXPECT_EQ(aNodes[0].bits, aFace.bits);

  aOptions.kind_mask = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_EDGE);
  const std::vector<occtl_node_id_t> anEdges =
    collectTaggedSelection(myGraph, &aOptions, "mount", 5);
  ASSERT_EQ(anEdges.size(), 1u);
  EXPECT_EQ(anEdges[0].bits, anEdge.bits);
}

TEST_F(GraphSelectTest, ByGraphTagHonorsRootAndSort)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aRoot.bits, 0u);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &anIter), OCCTL_OK);
  std::vector<occtl_node_id_t> aFaces;
  for (;;)
  {
    occtl_node_id_t      aFace   = OCCTL_NODE_ID_INVALID;
    const occtl_status_t aStatus = occtl_node_iter_next(anIter, &aFace);
    if (aStatus == OCCTL_NOT_FOUND)
    {
      break;
    }
    ASSERT_EQ(aStatus, OCCTL_OK);
    aFaces.push_back(aFace);
  }
  occtl_node_iter_free(anIter);
  ASSERT_GE(aFaces.size(), 2u);

  ASSERT_EQ(occtl_graph_tag_add(myGraph, aFaces[0], "export", 6), OCCTL_OK);
  ASSERT_EQ(occtl_graph_tag_add(myGraph, aFaces[1], "export", 6), OCCTL_OK);

  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.root                   = aRoot;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);
  aOptions.sort_key               = OCCTL_SELECT_SORT_UID;
  aOptions.sort_direction         = OCCTL_SELECT_SORT_DESCENDING;

  const std::vector<occtl_node_id_t> aNodes =
    collectTaggedSelection(myGraph, &aOptions, "export", 6);
  ASSERT_EQ(aNodes.size(), 2u);
  EXPECT_GT(aNodes[0].bits, aNodes[1].bits);
}

TEST_F(GraphSelectTest, ByBBoxCenter_Vertices_ReturnsOriginVertex)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_VERTEX);
  aOptions.use_bbox               = 1;
  aOptions.bbox.min               = {-1.0, -1.0, -1.0};
  aOptions.bbox.max               = {1.0, 1.0, 1.0};
  aOptions.bbox_mode              = OCCTL_SELECT_BBOX_CONTAINS_CENTER;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 1u);
}

TEST_F(GraphSelectTest, ByCurveKind_Edges_ReturnsBoxLines)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_EDGE);
  aOptions.use_curve_kind         = 1;
  aOptions.curve_kind             = OCCTL_CURVE_KIND_LINE;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 12u);
}

TEST_F(GraphSelectTest, BySurfaceKind_Faces_ReturnsBoxPlanes)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);
  aOptions.use_surface_kind       = 1;
  aOptions.surface_kind           = OCCTL_SURFACE_KIND_PLANE;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 6u);
}

TEST_F(GraphSelectTest, ByAxisPosition_TopFace_ReturnsOneFace)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);
  aOptions.use_axis_position      = 1;
  aOptions.axis                   = OCCTL_SELECT_AXIS_Z;
  aOptions.axis_position          = OCCTL_SELECT_AXIS_POSITION_MAX;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 1u);
}

TEST_F(GraphSelectTest, ByAxisPosition_BottomVertices_ReturnsFourVertices)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_VERTEX);
  aOptions.use_axis_position      = 1;
  aOptions.axis                   = OCCTL_SELECT_AXIS_Z;
  aOptions.axis_position          = OCCTL_SELECT_AXIS_POSITION_MIN;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 4u);
}

TEST_F(GraphSelectTest, ByNormal_TopFace_ReturnsOneFace)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);
  aOptions.use_normal             = 1;
  aOptions.normal                 = {0.0, 0.0, 1.0};
  aOptions.normal_mode            = OCCTL_SELECT_NORMAL_PARALLEL;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 1u);
}

TEST_F(GraphSelectTest, ByNormalEither_ZFaces_ReturnsTwoFaces)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);
  aOptions.use_normal             = 1;
  aOptions.normal                 = {0.0, 0.0, 1.0};
  aOptions.normal_mode            = OCCTL_SELECT_NORMAL_EITHER;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 2u);
}

TEST_F(GraphSelectTest, ByMeasure_EdgeLength_ReturnsFourShortEdges)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_EDGE);
  aOptions.use_measure            = 1;
  aOptions.measure_kind           = OCCTL_SELECT_MEASURE_EDGE_LENGTH;
  aOptions.measure_min            = 9.99;
  aOptions.measure_max            = 10.01;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 4u);
}

TEST_F(GraphSelectTest, ByMeasure_EdgeLength_ComputesExpectedMeasure)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_EDGE);
  aOptions.use_measure            = 1;
  aOptions.measure_kind           = OCCTL_SELECT_MEASURE_EDGE_LENGTH;
  aOptions.measure_min            = 9.99;
  aOptions.measure_max            = 10.01;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  ASSERT_FALSE(aNodes.empty());

  const BRepGraph_NodeId anEdge = OcctL::Topo::UnpackNodeId(aNodes.front());
  double                 aValue = 0.0;
  ASSERT_TRUE(OcctL::Topo::ComputeMeasureValue(myGraph->graph,
                                               anEdge,
                                               OCCTL_SELECT_MEASURE_EDGE_LENGTH,
                                               aValue));
  EXPECT_NEAR(aValue, 10.0, 1e-7);
}

TEST_F(GraphSelectTest, ByMeasure_FaceArea_ReturnsTwoSmallFaces)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);
  aOptions.use_measure            = 1;
  aOptions.measure_kind           = OCCTL_SELECT_MEASURE_FACE_AREA;
  aOptions.measure_min            = 199.9;
  aOptions.measure_max            = 200.1;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 2u);
}

TEST_F(GraphSelectTest, ByMeasure_Volume_ReturnsSolid)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_SOLID);
  aOptions.use_measure            = 1;
  aOptions.measure_kind           = OCCTL_SELECT_MEASURE_VOLUME;
  aOptions.measure_min            = 5999.9;
  aOptions.measure_max            = 6000.1;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_EQ(aNodes.size(), 1u);
}

TEST_F(GraphSelectTest, SortByAxisCoordinate_VerticesDescendingZ_ReturnsTopFirst)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_VERTEX);
  aOptions.sort_key               = OCCTL_SELECT_SORT_AXIS_COORDINATE;
  aOptions.sort_direction         = OCCTL_SELECT_SORT_DESCENDING;
  aOptions.sort_axis              = OCCTL_SELECT_AXIS_Z;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  ASSERT_EQ(aNodes.size(), 8u);

  occtl_point3_t aFirstPoint = {0.0, 0.0, 0.0};
  occtl_point3_t aLastPoint  = {0.0, 0.0, 0.0};
  ASSERT_EQ(occtl_topo_vertex_point(myGraph, aNodes.front(), &aFirstPoint), OCCTL_OK);
  ASSERT_EQ(occtl_topo_vertex_point(myGraph, aNodes.back(), &aLastPoint), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aFirstPoint.z, 30.0);
  EXPECT_DOUBLE_EQ(aLastPoint.z, 0.0);
}

TEST_F(GraphSelectTest, SortByDistanceToPoint_VerticesAscending_ReturnsOriginFirst)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_VERTEX);
  aOptions.sort_key               = OCCTL_SELECT_SORT_DISTANCE_TO_POINT;
  aOptions.sort_point             = {0.0, 0.0, 0.0};

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  ASSERT_EQ(aNodes.size(), 8u);

  occtl_point3_t aPoint = {1.0, 1.0, 1.0};
  ASSERT_EQ(occtl_topo_vertex_point(myGraph, aNodes.front(), &aPoint), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aPoint.x, 0.0);
  EXPECT_DOUBLE_EQ(aPoint.y, 0.0);
  EXPECT_DOUBLE_EQ(aPoint.z, 0.0);
}

TEST_F(GraphSelectTest, SortByDistanceToNode_VerticesAscending_ReturnsTargetFirst)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_VERTEX);

  const std::vector<occtl_node_id_t> aUnsorted = collectSelection(myGraph, &aOptions);
  ASSERT_EQ(aUnsorted.size(), 8u);
  const occtl_node_id_t aTarget = aUnsorted.back();

  occtl_select_distance_to_node_sort_t aSort = OCCTL_SELECT_DISTANCE_TO_NODE_SORT_INIT;
  aSort.target                               = aTarget;
  aOptions.sort_key                          = OCCTL_SELECT_SORT_DISTANCE_TO_NODE;
  aOptions.p_next                            = &aSort;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  ASSERT_EQ(aNodes.size(), 8u);
  EXPECT_EQ(aNodes.front().bits, aTarget.bits);
}

TEST_F(GraphSelectTest, SortByName_NamedVerticesComeFirstAlphabetically)
{
  const std::vector<occtl_node_id_t> aVertices = collectSelection(myGraph, nullptr);
  std::vector<occtl_node_id_t>       aVertexNodes;
  for (const occtl_node_id_t aNode : aVertices)
  {
    occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
    ASSERT_EQ(occtl_graph_node_kind(myGraph, aNode, &aKind), OCCTL_OK);
    if (aKind == OCCTL_KIND_VERTEX)
    {
      aVertexNodes.push_back(aNode);
    }
  }
  ASSERT_GE(aVertexNodes.size(), 2u);

  ASSERT_EQ(occtl_graph_name_set(myGraph, aVertexNodes[0], "b", 1), OCCTL_OK);
  ASSERT_EQ(occtl_graph_name_set(myGraph, aVertexNodes[1], "a", 1), OCCTL_OK);

  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_VERTEX);
  aOptions.sort_key               = OCCTL_SELECT_SORT_NAME;

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  ASSERT_GE(aNodes.size(), 2u);
  EXPECT_EQ(aNodes[0].bits, aVertexNodes[1].bits);
  EXPECT_EQ(aNodes[1].bits, aVertexNodes[0].bits);
}

TEST_F(GraphSelectTest, GroupByKind_AllNodes_ReturnsTopologyAndAssemblyGroups)
{
  occtl_select_group_options_t aGroupOptions = OCCTL_SELECT_GROUP_OPTIONS_INIT;
  aGroupOptions.key                          = OCCTL_SELECT_GROUP_KIND;

  const std::vector<CollectedGroup> aGroups = collectGroups(myGraph, nullptr, &aGroupOptions);
  EXPECT_GE(aGroups.size(), 6u);

  bool hasFaceGroup   = false;
  bool hasVertexGroup = false;
  for (const CollectedGroup& aGroup : aGroups)
  {
    if (aGroup.view.node_kind == OCCTL_KIND_FACE)
    {
      hasFaceGroup = true;
      EXPECT_EQ(aGroup.nodes.size(), 6u);
    }
    if (aGroup.view.node_kind == OCCTL_KIND_VERTEX)
    {
      hasVertexGroup = true;
      EXPECT_EQ(aGroup.nodes.size(), 8u);
    }
  }
  EXPECT_TRUE(hasFaceGroup);
  EXPECT_TRUE(hasVertexGroup);
}

TEST_F(GraphSelectTest, GroupByAxisCoordinate_VerticesByZ_ReturnsTwoGroups)
{
  occtl_select_options_t aSelectOptions = OCCTL_SELECT_OPTIONS_INIT;
  aSelectOptions.kind_mask = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_VERTEX);

  occtl_select_group_options_t aGroupOptions = OCCTL_SELECT_GROUP_OPTIONS_INIT;
  aGroupOptions.key                          = OCCTL_SELECT_GROUP_AXIS_COORDINATE;
  aGroupOptions.axis                         = OCCTL_SELECT_AXIS_Z;
  aGroupOptions.tolerance                    = 1.0e-7;

  const std::vector<CollectedGroup> aGroups =
    collectGroups(myGraph, &aSelectOptions, &aGroupOptions);
  ASSERT_EQ(aGroups.size(), 2u);
  EXPECT_EQ(aGroups[0].nodes.size(), 4u);
  EXPECT_EQ(aGroups[1].nodes.size(), 4u);
}

TEST_F(GraphSelectTest, GroupByCurveKind_Edges_ReturnsLineGroup)
{
  occtl_select_options_t aSelectOptions = OCCTL_SELECT_OPTIONS_INIT;
  aSelectOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_EDGE);

  occtl_select_group_options_t aGroupOptions = OCCTL_SELECT_GROUP_OPTIONS_INIT;
  aGroupOptions.key                          = OCCTL_SELECT_GROUP_CURVE_KIND;

  const std::vector<CollectedGroup> aGroups =
    collectGroups(myGraph, &aSelectOptions, &aGroupOptions);
  ASSERT_EQ(aGroups.size(), 1u);
  EXPECT_EQ(aGroups[0].view.curve_kind, OCCTL_CURVE_KIND_LINE);
  EXPECT_EQ(aGroups[0].nodes.size(), 12u);
}

TEST_F(GraphSelectTest, GroupByName_OnlyNamedNodes_ReturnsNamedGroups)
{
  const occtl_node_id_t aFace  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aFace.bits, 0u);
  ASSERT_NE(anEdge.bits, 0u);
  ASSERT_EQ(occtl_graph_name_set(myGraph, aFace, "alpha", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_name_set(myGraph, anEdge, "beta", 4), OCCTL_OK);

  occtl_select_group_options_t aGroupOptions = OCCTL_SELECT_GROUP_OPTIONS_INIT;
  aGroupOptions.key                          = OCCTL_SELECT_GROUP_NAME;

  const std::vector<CollectedGroup> aGroups = collectGroups(myGraph, nullptr, &aGroupOptions);
  ASSERT_EQ(aGroups.size(), 2u);
  EXPECT_EQ(aGroups[0].name, "alpha");
  EXPECT_EQ(aGroups[1].name, "beta");
  EXPECT_EQ(aGroups[0].nodes.size(), 1u);
  EXPECT_EQ(aGroups[1].nodes.size(), 1u);
}

TEST_F(GraphSelectTest, RootedSelection_IncludeRoot_ReturnsSolid)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.root                   = aSolid;
  aOptions.include_root           = 1;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_SOLID);

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  ASSERT_EQ(aNodes.size(), 1u);
  EXPECT_EQ(aNodes[0].bits, aSolid.bits);
}

TEST_F(GraphSelectTest, RootedSelection_ExcludeRoot_DoesNotReturnSolid)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.root                   = aSolid;
  aOptions.kind_mask              = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_SOLID);

  const std::vector<occtl_node_id_t> aNodes = collectSelection(myGraph, &aOptions);
  EXPECT_TRUE(aNodes.empty());
}

TEST_F(GraphSelectTest, BadOptionsVersion_ReturnsVersionMismatch)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.struct_version         = 999u;

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadGeometryKind_ReturnsInvalidArgument)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.use_curve_kind         = 1;
  aOptions.curve_kind             = OCCTL_CURVE_KIND_RESERVED_FUTURE;

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadAxisPosition_ReturnsInvalidArgument)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.use_axis_position      = 1;
  aOptions.axis_position          = OCCTL_SELECT_AXIS_POSITION_RESERVED_FUTURE;

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadNormal_ReturnsInvalidArgument)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.use_normal             = 1;
  aOptions.normal                 = {0.0, 0.0, 0.0};

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadMeasureRange_ReturnsInvalidArgument)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.use_measure            = 1;
  aOptions.measure_min            = 2.0;
  aOptions.measure_max            = 1.0;

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadMeasureKind_ReturnsInvalidArgument)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.use_measure            = 1;
  aOptions.measure_kind           = OCCTL_SELECT_MEASURE_KIND_RESERVED_FUTURE;

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadMetadataFilter_ReturnsInvalidArgument)
{
  occtl_select_options_t         aOptions  = OCCTL_SELECT_OPTIONS_INIT;
  occtl_select_metadata_filter_t aMetadata = OCCTL_SELECT_METADATA_FILTER_INIT;
  aOptions.p_next                          = &aMetadata;
  aMetadata.key_len                        = 4;

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);

  aMetadata             = OCCTL_SELECT_METADATA_FILTER_INIT;
  aOptions.p_next       = &aMetadata;
  aMetadata.match_value = 1;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);

  aMetadata             = OCCTL_SELECT_METADATA_FILTER_INIT;
  aOptions.p_next       = &aMetadata;
  aMetadata.key         = "role";
  aMetadata.key_len     = 4;
  aMetadata.match_value = 1;
  aMetadata.value_len   = 5;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);

  aMetadata             = OCCTL_SELECT_METADATA_FILTER_INIT;
  aOptions.p_next       = &aMetadata;
  aMetadata.key         = "role";
  aMetadata.key_len     = 4;
  aMetadata.match_value = 2;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);

  aMetadata                = OCCTL_SELECT_METADATA_FILTER_INIT;
  aOptions.p_next          = &aMetadata;
  aMetadata.struct_version = 999u;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadSortKey_ReturnsInvalidArgument)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.sort_key               = OCCTL_SELECT_SORT_KEY_RESERVED_FUTURE;

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadSortPoint_ReturnsInvalidArgument)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.sort_key               = OCCTL_SELECT_SORT_DISTANCE_TO_POINT;
  aOptions.sort_point.x           = std::numeric_limits<double>::infinity();

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadDistanceToNodeSort_ReturnsError)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.sort_key               = OCCTL_SELECT_SORT_DISTANCE_TO_NODE;

  occtl_select_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);

  occtl_select_distance_to_node_sort_t aSort = OCCTL_SELECT_DISTANCE_TO_NODE_SORT_INIT;
  aOptions.p_next                            = &aSort;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);

  aSort.target = occtl_node_id_t{0x00ff000000000001ull};
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_NOT_FOUND);
  EXPECT_EQ(anIter, nullptr);

  aSort                = OCCTL_SELECT_DISTANCE_TO_NODE_SORT_INIT;
  aSort.struct_version = 999u;
  aOptions.p_next      = &aSort;
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, &anIter), OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, BadGroupOptions_ReturnInvalidArgument)
{
  occtl_select_group_options_t aOptions = OCCTL_SELECT_GROUP_OPTIONS_INIT;
  aOptions.key                          = OCCTL_SELECT_GROUP_KEY_RESERVED_FUTURE;

  occtl_select_group_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_select_group_iter_create(myGraph, nullptr, &aOptions, &anIter),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(anIter, nullptr);
}

TEST_F(GraphSelectTest, NullArgs_ReturnInvalidArgument)
{
  occtl_select_options_t aOptions = OCCTL_SELECT_OPTIONS_INIT;
  occtl_select_iter_t*   anIter   = nullptr;
  EXPECT_EQ(occtl_select_iter_create(nullptr, &aOptions, &anIter), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_select_iter_create(myGraph, &aOptions, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_select_tagged_iter_create(nullptr, &aOptions, "tag", 3, &anIter),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_select_tagged_iter_create(myGraph, &aOptions, nullptr, 0, &anIter),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_select_tagged_iter_create(myGraph, &aOptions, "tag", 3, nullptr),
            OCCTL_INVALID_ARGUMENT);

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_select_iter_next(nullptr, &aNode), OCCTL_INVALID_ARGUMENT);

  occtl_select_group_iter_t* aGroupIter = nullptr;
  EXPECT_EQ(occtl_select_group_iter_create(nullptr, nullptr, nullptr, &aGroupIter),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_select_group_iter_create(myGraph, nullptr, nullptr, nullptr),
            OCCTL_INVALID_ARGUMENT);

  occtl_select_group_view_t aView = OCCTL_SELECT_GROUP_VIEW_INIT;
  EXPECT_EQ(occtl_select_group_iter_next(nullptr, &aView), OCCTL_INVALID_ARGUMENT);
}

} // namespace
