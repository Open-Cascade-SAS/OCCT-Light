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

#include "test_mesh_helpers.hxx"

#include <occtl/occtl_mesh.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace
{

using mesh_test::makeBox;
using mesh_test::makeCylinder;
using mesh_test::makeSphere;
using mesh_test::MeshFixture;

// Iterate every face in the graph; first face wins.
occtl_node_id_t firstFaceOf(occtl_graph_t* const theGraph)
{
  occtl_node_iter_t* aIter   = nullptr;
  occtl_node_id_t    aFaceId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_graph_face_iter_create(theGraph, &aIter), OCCTL_OK);
  occtl_node_iter_next(aIter, &aFaceId);
  occtl_node_iter_free(aIter);
  return aFaceId;
}

// Iterate every coedge in the graph; first coedge wins.
occtl_node_id_t firstCoedgeOf(occtl_graph_t* const theGraph)
{
  occtl_node_iter_t* aIter     = nullptr;
  occtl_node_id_t    aCoedgeId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_graph_coedge_iter_create(theGraph, &aIter), OCCTL_OK);
  occtl_node_iter_next(aIter, &aCoedgeId);
  occtl_node_iter_free(aIter);
  return aCoedgeId;
}

// Iterate every edge in the graph; first edge wins.
occtl_node_id_t firstEdgeOf(occtl_graph_t* const theGraph)
{
  occtl_node_iter_t* aIter   = nullptr;
  occtl_node_id_t    aEdgeId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_graph_edge_iter_create(theGraph, &aIter), OCCTL_OK);
  occtl_node_iter_next(aIter, &aEdgeId);
  occtl_node_iter_free(aIter);
  return aEdgeId;
}

TEST_F(MeshFixture, FaceTriangulation_NullArgs_InvalidArgument)
{
  occtl_triangulation_view_t aView{};
  EXPECT_EQ(occtl_mesh_face_triangulation(nullptr, OCCTL_NODE_ID_INVALID, &aView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_face_triangulation(myGraph, OCCTL_NODE_ID_INVALID, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST(MeshFromBuffersTest, OptionsInit_HasExpectedDefaults)
{
  occtl_mesh_from_buffers_options_t anOptions{};
  occtl_mesh_from_buffers_options_init(&anOptions);

  EXPECT_EQ(anOptions.struct_version, OCCTL_MESH_FROM_BUFFERS_OPTIONS_VERSION_1);
  EXPECT_EQ(anOptions.p_next, nullptr);
  EXPECT_EQ(anOptions.nodes, nullptr);
  EXPECT_EQ(anOptions.node_count, 0u);
  EXPECT_EQ(anOptions.triangles, nullptr);
  EXPECT_EQ(anOptions.triangle_count, 0u);
  EXPECT_DOUBLE_EQ(anOptions.deflection, 0.0);

  occtl_mesh_from_buffers_options_init(nullptr);
}

TEST(MeshFromBuffersTest, SingleTriangle_CreatesGraphFaceWithTriangulation)
{
  const double   aNodes[]     = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0};
  const uint32_t aTriangles[] = {0u, 1u, 2u};

  occtl_mesh_from_buffers_options_t anOptions = OCCTL_MESH_FROM_BUFFERS_OPTIONS_INIT;
  anOptions.nodes                             = aNodes;
  anOptions.node_count                        = 3u;
  anOptions.triangles                         = aTriangles;
  anOptions.triangle_count                    = 1u;
  anOptions.deflection                        = 0.01;

  occtl_graph_t*  aGraph = nullptr;
  occtl_node_id_t aRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_mesh_from_buffers(&anOptions, &aGraph, &aRoot), OCCTL_OK);
  ASSERT_NE(aGraph, nullptr);
  ASSERT_NE(aRoot.bits, 0u);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(aGraph, aRoot, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aGraph), 1u);

  occtl_triangulation_view_t aView{};
  ASSERT_EQ(occtl_mesh_face_triangulation(aGraph, aRoot, &aView), OCCTL_OK);
  ASSERT_EQ(aView.node_count, 3u);
  ASSERT_EQ(aView.triangle_count, 1u);
  ASSERT_NE(aView.nodes, nullptr);
  ASSERT_NE(aView.triangles, nullptr);
  EXPECT_DOUBLE_EQ(aView.deflection, 0.01);
  EXPECT_EQ(aView.triangles[0], 0u);
  EXPECT_EQ(aView.triangles[1], 1u);
  EXPECT_EQ(aView.triangles[2], 2u);

  occtl_graph_free(aGraph);
}

TEST(MeshFromBuffersTest, InvalidIndex_ReturnsInvalidArgument)
{
  const double   aNodes[]     = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0};
  const uint32_t aTriangles[] = {0u, 1u, 3u};

  occtl_mesh_from_buffers_options_t anOptions = OCCTL_MESH_FROM_BUFFERS_OPTIONS_INIT;
  anOptions.nodes                             = aNodes;
  anOptions.node_count                        = 3u;
  anOptions.triangles                         = aTriangles;
  anOptions.triangle_count                    = 1u;

  occtl_graph_t*  aGraph = nullptr;
  occtl_node_id_t aRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_mesh_from_buffers(&anOptions, &aGraph, &aRoot), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aGraph, nullptr);
  EXPECT_EQ(aRoot.bits, 0u);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST(MeshFromBuffersTest, BadVersion_ReturnsVersionMismatch)
{
  occtl_mesh_from_buffers_options_t anOptions = OCCTL_MESH_FROM_BUFFERS_OPTIONS_INIT;
  anOptions.struct_version                    = 999u;

  occtl_graph_t*  aGraph = nullptr;
  occtl_node_id_t aRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_mesh_from_buffers(&anOptions, &aGraph, &aRoot), OCCTL_VERSION_MISMATCH);
}

TEST(MeshFromBuffersTest, NullArgs_ReturnInvalidArgument)
{
  occtl_mesh_from_buffers_options_t anOptions = OCCTL_MESH_FROM_BUFFERS_OPTIONS_INIT;
  occtl_graph_t*                    aGraph    = nullptr;
  occtl_node_id_t                   aRoot     = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_mesh_from_buffers(nullptr, &aGraph, &aRoot), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_from_buffers(&anOptions, nullptr, &aRoot), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_from_buffers(&anOptions, &aGraph, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(MeshModelMetadataTest, SetGetKeysUnset_UsesGraphMetadataLayer)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  ASSERT_NE(aGraph, nullptr);

  ASSERT_EQ(occtl_mesh_model_metadata_set(aGraph, "author", 6, "unit-test", 9), OCCTL_OK);
  ASSERT_EQ(occtl_mesh_model_metadata_set(aGraph, "source", 6, "mesh", 4), OCCTL_OK);

  size_t aRequired = 0;
  ASSERT_EQ(occtl_mesh_model_metadata_get(aGraph, "author", 6, nullptr, 0, &aRequired), OCCTL_OK);
  ASSERT_EQ(aRequired, 10u);

  char aValue[16] = {};
  ASSERT_EQ(occtl_mesh_model_metadata_get(aGraph, "author", 6, aValue, sizeof(aValue), &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aValue, "unit-test");

  ASSERT_EQ(occtl_graph_metadata_get(aGraph, "author", 6, nullptr, 0, &aRequired), OCCTL_OK);

  size_t aKeyCount = 0;
  ASSERT_EQ(occtl_mesh_model_metadata_keys(aGraph, nullptr, 0, &aKeyCount), OCCTL_OK);
  ASSERT_EQ(aKeyCount, 2u);

  occtl_metadata_key_view_t aSmallKeys[1] = {};
  EXPECT_EQ(occtl_mesh_model_metadata_keys(aGraph, aSmallKeys, 1, &aKeyCount),
            OCCTL_BUFFER_TOO_SMALL);

  std::vector<occtl_metadata_key_view_t> aKeys(aKeyCount);
  ASSERT_EQ(occtl_mesh_model_metadata_keys(aGraph, aKeys.data(), aKeys.size(), &aKeyCount),
            OCCTL_OK);
  std::set<std::string> aNames;
  for (const occtl_metadata_key_view_t& aKey : aKeys)
  {
    aNames.emplace(aKey.key, aKey.key_len);
  }
  EXPECT_TRUE(aNames.count("author") != 0);
  EXPECT_TRUE(aNames.count("source") != 0);

  ASSERT_EQ(occtl_mesh_model_metadata_unset(aGraph, "author", 6), OCCTL_OK);
  EXPECT_EQ(occtl_mesh_model_metadata_get(aGraph, "author", 6, nullptr, 0, &aRequired),
            OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(MeshModelMetadataTest, InvalidArguments_ReturnError)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  ASSERT_NE(aGraph, nullptr);

  size_t aRequired = 0;
  EXPECT_EQ(occtl_mesh_model_metadata_set(nullptr, "key", 3, "value", 5), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_model_metadata_set(aGraph, "", 0, "value", 5), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_model_metadata_set(aGraph, "key", 3, nullptr, 1), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_model_metadata_get(nullptr, "key", 3, nullptr, 0, &aRequired),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_model_metadata_get(aGraph, "key", 3, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_model_metadata_keys(nullptr, nullptr, 0, &aRequired),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_model_metadata_keys(aGraph, nullptr, 0, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_model_metadata_unset(nullptr, "key", 3), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_model_metadata_unset(aGraph, nullptr, 0), OCCTL_INVALID_ARGUMENT);

  occtl_graph_free(aGraph);
}

TEST_F(MeshFixture, FaceTriangulation_BadId_NotFound)
{
  occtl_triangulation_view_t aView{};
  EXPECT_EQ(occtl_mesh_face_triangulation(myGraph, OCCTL_NODE_ID_INVALID, &aView), OCCTL_NOT_FOUND);
}

TEST_F(MeshFixture, FaceTriangulation_OnEdgeId_WrongKind)
{
  ASSERT_NE(makeBox(myGraph, 5.0, 5.0, 5.0).bits, 0u);
  const occtl_node_id_t aEdge = firstEdgeOf(myGraph);
  ASSERT_NE(aEdge.bits, 0u);

  occtl_triangulation_view_t aView{};
  EXPECT_EQ(occtl_mesh_face_triangulation(myGraph, aEdge, &aView), OCCTL_WRONG_KIND);
}

TEST_F(MeshFixture, FaceTriangulation_NoMesh_NotFound)
{
  ASSERT_NE(makeBox(myGraph, 5.0, 5.0, 5.0).bits, 0u);
  const occtl_node_id_t aFace = firstFaceOf(myGraph);
  ASSERT_NE(aFace.bits, 0u);

  occtl_triangulation_view_t aView{};
  EXPECT_EQ(occtl_mesh_face_triangulation(myGraph, aFace, &aView), OCCTL_NOT_FOUND);
}

TEST_F(MeshFixture, FaceTriangulation_BoxFace_NodesAndTrianglesPopulated)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);

  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  const occtl_node_id_t aFace = firstFaceOf(myGraph);
  ASSERT_NE(aFace.bits, 0u);

  occtl_triangulation_view_t aView{};
  ASSERT_EQ(occtl_mesh_face_triangulation(myGraph, aFace, &aView), OCCTL_OK);

  EXPECT_GT(aView.node_count, 0u);
  EXPECT_GT(aView.triangle_count, 0u);
  EXPECT_NE(aView.nodes, nullptr);
  EXPECT_NE(aView.triangles, nullptr);
  // Box faces are planar — the algorithm produces 2 triangles per face by default.
  EXPECT_EQ(aView.node_count, 4u);
  EXPECT_EQ(aView.triangle_count, 2u);
}

TEST_F(MeshFixture, FaceTriangulation_BoxFace_TrianglesAreZeroIndexed)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);
  const occtl_node_id_t aFace = firstFaceOf(myGraph);
  ASSERT_NE(aFace.bits, 0u);

  occtl_triangulation_view_t aView{};
  ASSERT_EQ(occtl_mesh_face_triangulation(myGraph, aFace, &aView), OCCTL_OK);

  // Every triangle index must be in [0, node_count). Lowest must be 0.
  uint32_t aMin = std::numeric_limits<uint32_t>::max();
  for (size_t i = 0; i < aView.triangle_count * 3; ++i)
  {
    EXPECT_LT(aView.triangles[i], static_cast<uint32_t>(aView.node_count))
      << "triangle vertex index " << aView.triangles[i] << " out of bounds at i=" << i;
    if (aView.triangles[i] < aMin)
    {
      aMin = aView.triangles[i];
    }
  }
  EXPECT_EQ(aMin, 0u) << "0-indexed buffer should reach index 0";
}

TEST_F(MeshFixture, FaceTriangulation_RepeatedFetch_SamePointers)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);
  const occtl_node_id_t aFace = firstFaceOf(myGraph);

  occtl_triangulation_view_t aFirst{};
  occtl_triangulation_view_t aSecond{};
  ASSERT_EQ(occtl_mesh_face_triangulation(myGraph, aFace, &aFirst), OCCTL_OK);
  ASSERT_EQ(occtl_mesh_face_triangulation(myGraph, aFace, &aSecond), OCCTL_OK);

  // Cache hit: pointers must be identical, not just contents.
  EXPECT_EQ(aFirst.nodes, aSecond.nodes);
  EXPECT_EQ(aFirst.triangles, aSecond.triangles);
}

TEST_F(MeshFixture, FaceNbTriangulations_BoxFace_AtLeastOne)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  uint32_t aCount = 0;
  ASSERT_EQ(occtl_mesh_face_triangulation_count(myGraph, firstFaceOf(myGraph), &aCount), OCCTL_OK);
  EXPECT_GE(aCount, 1u);
}

TEST_F(MeshFixture, FaceTriangulationIndexed_OutOfRange_ReturnsRangeError)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  occtl_triangulation_view_t aView{};
  EXPECT_EQ(occtl_mesh_face_triangulation_indexed(myGraph, firstFaceOf(myGraph), 9999u, &aView),
            OCCTL_OUT_OF_RANGE);
}

TEST_F(MeshFixture, EdgePolygon3D_NullArgs_InvalidArgument)
{
  occtl_polygon3d_view_t aView{};
  EXPECT_EQ(occtl_mesh_edge_polygon3d(nullptr, OCCTL_NODE_ID_INVALID, &aView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_edge_polygon3d(myGraph, OCCTL_NODE_ID_INVALID, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(MeshFixture, EdgePolygon3D_OnFaceId_WrongKind)
{
  ASSERT_NE(makeBox(myGraph, 5.0, 5.0, 5.0).bits, 0u);
  const occtl_node_id_t aFace = firstFaceOf(myGraph);
  ASSERT_NE(aFace.bits, 0u);

  occtl_polygon3d_view_t aView{};
  EXPECT_EQ(occtl_mesh_edge_polygon3d(myGraph, aFace, &aView), OCCTL_WRONG_KIND);
}

TEST_F(MeshFixture, EdgePolygon3D_NoCachedPolygon_NotFound)
{
  // Box edges typically have no cached 3D polygon (only polygon-on-tri).
  ASSERT_NE(makeBox(myGraph, 5.0, 5.0, 5.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  const occtl_node_id_t aEdge = firstEdgeOf(myGraph);
  ASSERT_NE(aEdge.bits, 0u);

  occtl_polygon3d_view_t aView{};
  EXPECT_EQ(occtl_mesh_edge_polygon3d(myGraph, aEdge, &aView), OCCTL_NOT_FOUND);
}

TEST_F(MeshFixture, CoedgePolygonOnTri_NullArgs_InvalidArgument)
{
  occtl_polygon_on_tri_view_t aView{};
  EXPECT_EQ(occtl_mesh_coedge_polygon_on_tri(nullptr, OCCTL_NODE_ID_INVALID, &aView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_coedge_polygon_on_tri(myGraph, OCCTL_NODE_ID_INVALID, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(MeshFixture, CoedgePolygonOnTri_OnFaceId_WrongKind)
{
  ASSERT_NE(makeBox(myGraph, 5.0, 5.0, 5.0).bits, 0u);
  const occtl_node_id_t aFace = firstFaceOf(myGraph);

  occtl_polygon_on_tri_view_t aView{};
  EXPECT_EQ(occtl_mesh_coedge_polygon_on_tri(myGraph, aFace, &aView), OCCTL_WRONG_KIND);
}

TEST_F(MeshFixture, CoedgePolygonOnTri_BoxCoedge_NodeIndicesAreZeroIndexed)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  const occtl_node_id_t aCoedge = firstCoedgeOf(myGraph);
  ASSERT_NE(aCoedge.bits, 0u);

  occtl_polygon_on_tri_view_t aView{};
  ASSERT_EQ(occtl_mesh_coedge_polygon_on_tri(myGraph, aCoedge, &aView), OCCTL_OK);

  EXPECT_GE(aView.node_count, 2u);
  EXPECT_NE(aView.node_indices, nullptr);
  uint32_t aMin = std::numeric_limits<uint32_t>::max();
  for (size_t i = 0; i < aView.node_count; ++i)
  {
    if (aView.node_indices[i] < aMin)
    {
      aMin = aView.node_indices[i];
    }
  }
  EXPECT_EQ(aMin, 0u) << "0-indexed buffer should reach index 0";
}

TEST_F(MeshFixture, FaceTriangulation_SourceUid_RoundTripsThroughGraph)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  const occtl_node_id_t aFace        = firstFaceOf(myGraph);
  occtl_uid_t           aExpectedUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(myGraph, aFace, &aExpectedUid), OCCTL_OK);

  occtl_triangulation_view_t aView{};
  ASSERT_EQ(occtl_mesh_face_triangulation(myGraph, aFace, &aView), OCCTL_OK);
  EXPECT_EQ(aView.source_uid.bits, aExpectedUid.bits);
}

TEST_F(MeshFixture, FaceTriangulation_Deflection_IsFinite)
{
  // The triangulation carries the deflection scalar that the source
  // Poly_Triangulation reports. Per-face values can be 0.0 for planar
  // box faces (the BRepMesh algorithm reports the achieved deflection,
  // which is exactly 0 for analytic planes). Assert the value is at
  // least finite and non-negative — a stale negative or NaN would fail.
  ASSERT_NE(makeBox(myGraph, 100.0, 100.0, 100.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  aOpts.deflection           = 0.25;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  occtl_triangulation_view_t aView{};
  ASSERT_EQ(occtl_mesh_face_triangulation(myGraph, firstFaceOf(myGraph), &aView), OCCTL_OK);
  EXPECT_GE(aView.deflection, 0.0);
  EXPECT_FALSE(std::isnan(aView.deflection));
}

TEST_F(MeshFixture, FaceNbTriangulations_NullOutCount_InvalidArgument)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  EXPECT_EQ(occtl_mesh_face_triangulation_count(myGraph, firstFaceOf(myGraph), nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(MeshFixture, CoedgePolygonOnTri_NodeIndices_BoundedByParentFaceNodeCount)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  // Walk every coedge; for each, fetch its polygon-on-tri view AND its
  // parent face's triangulation, then confirm every node_index is in
  // [0, parent.node_count).
  occtl_node_iter_t* aIter = nullptr;
  ASSERT_EQ(occtl_graph_coedge_iter_create(myGraph, &aIter), OCCTL_OK);
  occtl_node_id_t aCoEdge = OCCTL_NODE_ID_INVALID;
  while (occtl_node_iter_next(aIter, &aCoEdge) == OCCTL_OK)
  {
    occtl_polygon_on_tri_view_t aCoEdgeView{};
    if (occtl_mesh_coedge_polygon_on_tri(myGraph, aCoEdge, &aCoEdgeView) != OCCTL_OK)
    {
      continue;
    }

    occtl_node_id_t aParentFace = OCCTL_NODE_ID_INVALID;
    ASSERT_EQ(occtl_topo_coedge_face_of(myGraph, aCoEdge, &aParentFace), OCCTL_OK);

    occtl_triangulation_view_t aFaceView{};
    ASSERT_EQ(occtl_mesh_face_triangulation(myGraph, aParentFace, &aFaceView), OCCTL_OK);

    for (size_t i = 0; i < aCoEdgeView.node_count; ++i)
    {
      EXPECT_LT(aCoEdgeView.node_indices[i], static_cast<uint32_t>(aFaceView.node_count))
        << "coedge node_index out of parent face's node range at i=" << i;
    }
  }
  occtl_node_iter_free(aIter);
}

TEST_F(MeshFixture, GenerateAfterFetch_InvalidatesCache)
{
  // Materialise a view, then re-mesh and re-fetch. The cache must be
  // dropped on the second generate so the second fetch succeeds (and
  // does not return a stale dangling pointer when the first
  // triangulation handle is replaced inside the graph).
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);
  const occtl_node_id_t aFace = firstFaceOf(myGraph);

  occtl_triangulation_view_t aFirst{};
  ASSERT_EQ(occtl_mesh_face_triangulation(myGraph, aFace, &aFirst), OCCTL_OK);
  ASSERT_NE(aFirst.nodes, nullptr);

  // Coarser regeneration. Forces re-meshing and cache invalidation.
  aOpts.deflection             = 0.5;
  aOpts.allow_quality_decrease = 1;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  occtl_triangulation_view_t aSecond{};
  ASSERT_EQ(occtl_mesh_face_triangulation(myGraph, aFace, &aSecond), OCCTL_OK);
  EXPECT_GT(aSecond.node_count, 0u);
  EXPECT_GT(aSecond.triangle_count, 0u);
}

TEST_F(MeshFixture, TriangleBuffers_BoxGraph_AggregatesAllFaces)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  occtl_mesh_triangle_buffers_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_buffers(myGraph, OCCTL_NODE_ID_INVALID, &aView), OCCTL_OK);

  EXPECT_EQ(aView.face_count, 6u);
  EXPECT_EQ(aView.node_count, 24u);
  EXPECT_EQ(aView.triangle_count, 12u);
  EXPECT_NE(aView.nodes, nullptr);
  EXPECT_NE(aView.triangles, nullptr);
  EXPECT_EQ(aView.root.bits, OCCTL_NODE_ID_INVALID.bits);

  for (size_t i = 0; i < aView.triangle_count * 3; ++i)
  {
    EXPECT_LT(aView.triangles[i], static_cast<uint32_t>(aView.node_count));
  }
}

TEST_F(MeshFixture, TriangleBuffers_RootFace_AggregatesSingleFace)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  const occtl_node_id_t aFace = firstFaceOf(myGraph);
  ASSERT_NE(aFace.bits, 0u);

  occtl_mesh_triangle_buffers_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_buffers(myGraph, aFace, &aView), OCCTL_OK);

  EXPECT_EQ(aView.face_count, 1u);
  EXPECT_EQ(aView.node_count, 4u);
  EXPECT_EQ(aView.triangle_count, 2u);
  EXPECT_EQ(aView.root.bits, aFace.bits);
}

TEST_F(MeshFixture, TriangleAnalysis_BoxGraph_ReturnsNormalsAndAdjacency)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  occtl_mesh_triangle_analysis_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_analysis(myGraph, OCCTL_NODE_ID_INVALID, &aView), OCCTL_OK);

  EXPECT_EQ(aView.face_count, 6u);
  EXPECT_EQ(aView.triangle_count, 12u);
  EXPECT_NE(aView.triangle_normals, nullptr);
  EXPECT_NE(aView.triangle_adjacency, nullptr);
  EXPECT_EQ(aView.root.bits, OCCTL_NODE_ID_INVALID.bits);

  size_t aBoundaryEdges = 0;
  for (size_t aTriIdx = 0; aTriIdx < aView.triangle_count; ++aTriIdx)
  {
    const double aNx     = aView.triangle_normals[aTriIdx * 3u];
    const double aNy     = aView.triangle_normals[aTriIdx * 3u + 1u];
    const double aNz     = aView.triangle_normals[aTriIdx * 3u + 2u];
    const double aLength = std::sqrt(aNx * aNx + aNy * aNy + aNz * aNz);
    EXPECT_NEAR(aLength, 1.0, 1.0e-12);

    for (size_t aLocal = 0; aLocal < 3u; ++aLocal)
    {
      const uint32_t aNeighbor = aView.triangle_adjacency[aTriIdx * 3u + aLocal];
      if (aNeighbor == OCCTL_MESH_TRIANGLE_ADJACENCY_BOUNDARY)
      {
        ++aBoundaryEdges;
      }
      else
      {
        EXPECT_LT(aNeighbor, static_cast<uint32_t>(aView.triangle_count));
      }
    }
  }
  EXPECT_GT(aBoundaryEdges, 0u);
}

TEST_F(MeshFixture, TriangleAnalysis_RootFace_ReturnsTwoTrianglesWithSharedEdge)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  const occtl_node_id_t aFace = firstFaceOf(myGraph);
  ASSERT_NE(aFace.bits, 0u);

  occtl_mesh_triangle_analysis_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_analysis(myGraph, aFace, &aView), OCCTL_OK);

  EXPECT_EQ(aView.face_count, 1u);
  EXPECT_EQ(aView.triangle_count, 2u);
  EXPECT_EQ(aView.root.bits, aFace.bits);

  size_t aSharedEdges = 0;
  for (size_t anIdx = 0; anIdx < aView.triangle_count * 3u; ++anIdx)
  {
    if (aView.triangle_adjacency[anIdx] != OCCTL_MESH_TRIANGLE_ADJACENCY_BOUNDARY)
    {
      ++aSharedEdges;
    }
  }
  EXPECT_EQ(aSharedEdges, 2u);
}

TEST_F(MeshFixture, TriangleComponents_RootFace_ReturnsSingleComponent)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  const occtl_node_id_t aFace = firstFaceOf(myGraph);
  ASSERT_NE(aFace.bits, 0u);

  occtl_mesh_triangle_components_options_t anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  anOpts.root                                     = aFace;

  occtl_mesh_triangle_components_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_components(myGraph, &anOpts, &aView), OCCTL_OK);

  EXPECT_EQ(aView.triangle_count, 2u);
  EXPECT_EQ(aView.component_count, 1u);
  ASSERT_NE(aView.triangle_component_ids, nullptr);
  ASSERT_NE(aView.component_sizes, nullptr);
  EXPECT_EQ(aView.component_sizes[0], 2u);
  EXPECT_EQ(aView.triangle_component_ids[0], 0u);
  EXPECT_EQ(aView.triangle_component_ids[1], 0u);
}

TEST_F(MeshFixture, TriangleComponents_BoxGraph_ReturnsOneComponentPerFace)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_components_options_t anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;

  occtl_mesh_triangle_components_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_components(myGraph, &anOpts, &aView), OCCTL_OK);

  EXPECT_EQ(aView.triangle_count, 12u);
  EXPECT_EQ(aView.component_count, 6u);
  for (size_t anIdx = 0; anIdx < aView.component_count; ++anIdx)
  {
    EXPECT_EQ(aView.component_sizes[anIdx], 2u);
  }
}

TEST_F(MeshFixture, TriangleComponentTriangles_BoxGraph_ReturnsSelectedTriangleIds)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_components_options_t anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;

  occtl_mesh_triangle_component_triangles_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_component_triangles(myGraph, &anOpts, 0u, &aView), OCCTL_OK);

  EXPECT_EQ(aView.component_id, 0u);
  EXPECT_EQ(aView.triangle_count, 2u);
  ASSERT_NE(aView.triangles, nullptr);
  EXPECT_LT(aView.triangles[0], 12u);
  EXPECT_LT(aView.triangles[1], 12u);
  EXPECT_NE(aView.triangles[0], aView.triangles[1]);
}

TEST_F(MeshFixture, TriangleComponentTriangles_UnknownComponent_ReturnsOutOfRange)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_components_options_t anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  occtl_mesh_triangle_component_triangles_view_t aView{};
  EXPECT_EQ(occtl_mesh_triangle_component_triangles(myGraph, &anOpts, 99u, &aView),
            OCCTL_OUT_OF_RANGE);
  EXPECT_EQ(aView.triangle_count, 0u);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(MeshFixture, TriangleComponentBoundary_BoxGraph_ReturnsPatchPerimeter)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_components_options_t anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;

  occtl_mesh_triangle_component_boundary_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_component_boundary(myGraph, &anOpts, 0u, &aView), OCCTL_OK);

  EXPECT_EQ(aView.component_id, 0u);
  EXPECT_EQ(aView.edge_count, 4u);
  ASSERT_NE(aView.edges, nullptr);
  for (size_t anIdx = 0; anIdx < aView.edge_count; ++anIdx)
  {
    EXPECT_LT(aView.edges[anIdx].triangle, 12u);
    EXPECT_LT(aView.edges[anIdx].local_edge, 3u);
    EXPECT_LT(aView.edges[anIdx].node0, 24u);
    EXPECT_LT(aView.edges[anIdx].node1, 24u);
    EXPECT_EQ(aView.edges[anIdx].adjacent_triangle, OCCTL_MESH_TRIANGLE_ADJACENCY_BOUNDARY);
  }
}

TEST_F(MeshFixture, TriangleComponentBoundaryChains_BoxGraph_ReturnsOrderedClosedChain)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_components_options_t anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;

  occtl_mesh_triangle_component_boundary_chains_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_component_boundary_chains(myGraph, &anOpts, 0u, &aView), OCCTL_OK);

  EXPECT_EQ(aView.component_id, 0u);
  EXPECT_EQ(aView.edge_count, 4u);
  ASSERT_NE(aView.edges, nullptr);
  ASSERT_NE(aView.chains, nullptr);
  ASSERT_EQ(aView.chain_count, 1u);
  EXPECT_EQ(aView.chains[0].first_edge, 0u);
  EXPECT_EQ(aView.chains[0].edge_count, 4u);
  EXPECT_EQ(aView.chains[0].is_closed, 1);

  for (uint32_t anIdx = 0; anIdx + 1u < aView.chains[0].edge_count; ++anIdx)
  {
    EXPECT_EQ(aView.edges[anIdx].node1, aView.edges[anIdx + 1u].node0);
  }
  EXPECT_EQ(aView.edges[aView.chains[0].edge_count - 1u].node1,
            aView.edges[aView.chains[0].first_edge].node0);
}

TEST_F(MeshFixture, TriangleComponentBoundaryPolylines_BoxGraph_ReturnsClosedPolyline)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_components_options_t anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;

  occtl_mesh_component_boundary_polylines_view_t aView{};
  ASSERT_EQ(occtl_mesh_component_boundary_polylines(myGraph, &anOpts, 0u, &aView), OCCTL_OK);

  EXPECT_EQ(aView.component_id, 0u);
  ASSERT_NE(aView.points, nullptr);
  ASSERT_NE(aView.polylines, nullptr);
  ASSERT_EQ(aView.polyline_count, 1u);
  EXPECT_EQ(aView.polylines[0].first_point, 0u);
  EXPECT_EQ(aView.polylines[0].point_count, 5u);
  EXPECT_EQ(aView.polylines[0].is_closed, 1);
  EXPECT_EQ(aView.point_count, 5u);
  EXPECT_DOUBLE_EQ(aView.points[0].x, aView.points[4].x);
  EXPECT_DOUBLE_EQ(aView.points[0].y, aView.points[4].y);
  EXPECT_DOUBLE_EQ(aView.points[0].z, aView.points[4].z);
}

TEST_F(MeshFixture, TriangleComponentSummaries_BoxGraph_ReturnsAreaCentroidNormalAndBounds)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_components_options_t anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;

  occtl_mesh_triangle_component_summaries_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_component_summaries(myGraph, &anOpts, &aView), OCCTL_OK);

  EXPECT_EQ(aView.triangle_count, 12u);
  EXPECT_EQ(aView.component_count, 6u);
  ASSERT_NE(aView.summaries, nullptr);

  double anAreaSum = 0.0;
  for (size_t anIdx = 0; anIdx < aView.component_count; ++anIdx)
  {
    const occtl_mesh_triangle_component_summary_t& aSummary = aView.summaries[anIdx];
    EXPECT_EQ(aSummary.component_id, static_cast<uint32_t>(anIdx));
    EXPECT_EQ(aSummary.triangle_count, 2u);
    EXPECT_NEAR(aSummary.area, 100.0, 1.0e-9);
    const double aNormalLength =
      std::sqrt(aSummary.normal.x * aSummary.normal.x + aSummary.normal.y * aSummary.normal.y
                + aSummary.normal.z * aSummary.normal.z);
    EXPECT_NEAR(aNormalLength, 1.0, 1.0e-12);
    EXPECT_LE(aSummary.bounds.min.x, aSummary.centroid.x);
    EXPECT_LE(aSummary.bounds.min.y, aSummary.centroid.y);
    EXPECT_LE(aSummary.bounds.min.z, aSummary.centroid.z);
    EXPECT_GE(aSummary.bounds.max.x, aSummary.centroid.x);
    EXPECT_GE(aSummary.bounds.max.y, aSummary.centroid.y);
    EXPECT_GE(aSummary.bounds.max.z, aSummary.centroid.z);
    anAreaSum += aSummary.area;
  }
  EXPECT_NEAR(anAreaSum, 600.0, 1.0e-9);
}

TEST_F(MeshFixture, TrianglePlaneComponents_BoxGraph_ReturnsSixPlaneComponents)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_plane_components_options_t anOpts =
    OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;

  occtl_mesh_triangle_plane_components_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_plane_components(myGraph, &anOpts, &aView), OCCTL_OK);

  EXPECT_EQ(aView.triangle_count, 12u);
  EXPECT_EQ(aView.component_count, 6u);
  ASSERT_NE(aView.components, nullptr);

  for (size_t anIdx = 0; anIdx < aView.component_count; ++anIdx)
  {
    const occtl_mesh_triangle_plane_component_t& aPlane = aView.components[anIdx];
    EXPECT_EQ(aPlane.triangle_count, 2u);
    EXPECT_NEAR(aPlane.area, 100.0, 1.0e-9);
    EXPECT_LE(aPlane.max_distance, 1.0e-12);
    const double aNormalLength =
      std::sqrt(aPlane.normal.x * aPlane.normal.x + aPlane.normal.y * aPlane.normal.y
                + aPlane.normal.z * aPlane.normal.z);
    EXPECT_NEAR(aNormalLength, 1.0, 1.0e-12);
  }
}

TEST_F(MeshFixture, TriangleSphereComponents_SphereGraph_ReturnsSphereComponent)
{
  ASSERT_NE(makeSphere(myGraph, 5.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  aMeshOpts.deflection           = 0.2;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_sphere_components_options_t anOpts =
    OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  anOpts.max_normal_angle   = OCCTL_PI;
  anOpts.max_distance       = 0.25;
  anOpts.min_triangle_count = 8u;

  occtl_mesh_triangle_sphere_components_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_sphere_components(myGraph, &anOpts, &aView), OCCTL_OK);

  EXPECT_GT(aView.triangle_count, 0u);
  ASSERT_EQ(aView.component_count, 1u);
  ASSERT_NE(aView.components, nullptr);

  const occtl_mesh_triangle_sphere_component_t& aSphere = aView.components[0];
  EXPECT_GT(aSphere.triangle_count, 8u);
  EXPECT_NEAR(aSphere.center.x, 0.0, 1.0e-6);
  EXPECT_NEAR(aSphere.center.y, 0.0, 1.0e-6);
  EXPECT_NEAR(aSphere.center.z, 0.0, 1.0e-6);
  EXPECT_NEAR(aSphere.radius, 5.0, 0.25);
  EXPECT_LE(aSphere.max_distance, 0.25);
}

TEST_F(MeshFixture, TriangleCylinderComponents_CylinderGraph_ReturnsCylinderComponent)
{
  ASSERT_NE(makeCylinder(myGraph, 3.0, 7.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  aMeshOpts.deflection           = 0.1;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_cylinder_components_options_t anOpts =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  anOpts.max_distance       = 0.15;
  anOpts.min_triangle_count = 8u;

  occtl_mesh_triangle_cylinder_components_view_t aView{};
  ASSERT_EQ(occtl_mesh_triangle_cylinder_components(myGraph, &anOpts, &aView), OCCTL_OK);

  EXPECT_GT(aView.triangle_count, 0u);
  ASSERT_EQ(aView.component_count, 1u);
  ASSERT_NE(aView.components, nullptr);

  const occtl_mesh_triangle_cylinder_component_t& aCylinder = aView.components[0];
  EXPECT_GT(aCylinder.triangle_count, 8u);
  EXPECT_NEAR(aCylinder.axis_origin.x, 0.0, 1.0e-6);
  EXPECT_NEAR(aCylinder.axis_origin.y, 0.0, 1.0e-6);
  EXPECT_NEAR(std::abs(aCylinder.axis_direction.z), 1.0, 1.0e-3);
  EXPECT_NEAR(aCylinder.radius, 3.0, 0.15);
  EXPECT_NEAR(aCylinder.height_max - aCylinder.height_min, 7.0, 0.2);
  EXPECT_LE(aCylinder.max_distance, 0.15);
}

TEST_F(MeshFixture, MakeSphereComponentSolid_SphereGraph_ReturnsSolidNode)
{
  ASSERT_NE(makeSphere(myGraph, 5.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  aMeshOpts.deflection           = 0.2;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_sphere_components_options_t anOpts =
    OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  anOpts.max_normal_angle   = OCCTL_PI;
  anOpts.max_distance       = 0.25;
  anOpts.min_triangle_count = 8u;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_mesh_make_sphere_component_solid(myGraph, &anOpts, 0u, &aSolid), OCCTL_OK);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSolid, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(MeshFixture, MakeCylinderComponentSolid_CylinderGraph_ReturnsSolidNode)
{
  ASSERT_NE(makeCylinder(myGraph, 3.0, 7.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  aMeshOpts.deflection           = 0.1;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_cylinder_components_options_t anOpts =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  anOpts.max_distance       = 0.15;
  anOpts.min_triangle_count = 8u;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_mesh_make_cylinder_component_solid(myGraph, &anOpts, 0u, &aSolid), OCCTL_OK);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSolid, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(MeshFixture, MakeSphereComponentSolids_SphereGraph_ReturnsSolidNodes)
{
  ASSERT_NE(makeSphere(myGraph, 5.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  aMeshOpts.deflection           = 0.2;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_sphere_components_options_t anOpts =
    OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  anOpts.max_normal_angle   = OCCTL_PI;
  anOpts.max_distance       = 0.25;
  anOpts.min_triangle_count = 8u;

  size_t aCount = 0u;
  ASSERT_EQ(occtl_mesh_make_sphere_component_solids(myGraph, &anOpts, nullptr, 0u, &aCount),
            OCCTL_OK);
  ASSERT_EQ(aCount, 1u);

  occtl_node_id_t aTooSmall[1] = {};
  EXPECT_EQ(occtl_mesh_make_sphere_component_solids(myGraph, &anOpts, aTooSmall, 0u, &aCount),
            OCCTL_BUFFER_TOO_SMALL);

  occtl_node_id_t aSolids[1] = {};
  ASSERT_EQ(occtl_mesh_make_sphere_component_solids(myGraph, &anOpts, aSolids, 1u, &aCount),
            OCCTL_OK);
  ASSERT_EQ(aCount, 1u);
  ASSERT_NE(aSolids[0].bits, 0u);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSolids[0], &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(MeshFixture, MakeCylinderComponentSolids_CylinderGraph_ReturnsSolidNodes)
{
  ASSERT_NE(makeCylinder(myGraph, 3.0, 7.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  aMeshOpts.deflection           = 0.1;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_cylinder_components_options_t anOpts =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  anOpts.max_distance       = 0.15;
  anOpts.min_triangle_count = 8u;

  size_t aCount = 0u;
  ASSERT_EQ(occtl_mesh_make_cylinder_component_solids(myGraph, &anOpts, nullptr, 0u, &aCount),
            OCCTL_OK);
  ASSERT_EQ(aCount, 1u);

  occtl_node_id_t aTooSmall[1] = {};
  EXPECT_EQ(occtl_mesh_make_cylinder_component_solids(myGraph, &anOpts, aTooSmall, 0u, &aCount),
            OCCTL_BUFFER_TOO_SMALL);

  occtl_node_id_t aSolids[1] = {};
  ASSERT_EQ(occtl_mesh_make_cylinder_component_solids(myGraph, &anOpts, aSolids, 1u, &aCount),
            OCCTL_OK);
  ASSERT_EQ(aCount, 1u);
  ASSERT_NE(aSolids[0].bits, 0u);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSolids[0], &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(MeshFixture, MakePlaneComponentFace_BoxGraph_ReturnsFaceNode)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_plane_components_options_t anOpts =
    OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_mesh_make_plane_component_face(myGraph, &anOpts, 0u, &aFace), OCCTL_OK);
  ASSERT_NE(aFace.bits, 0u);

  occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(MeshFixture, MakePlaneComponentFaces_BoxGraph_ReturnsFaceNodes)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);
  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aMeshOpts), OCCTL_OK);

  occtl_mesh_triangle_plane_components_options_t anOpts =
    OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;

  size_t aCount = 0u;
  ASSERT_EQ(occtl_mesh_make_plane_component_faces(myGraph, &anOpts, nullptr, 0u, &aCount),
            OCCTL_OK);
  ASSERT_EQ(aCount, 6u);

  occtl_node_id_t aTooSmall[5] = {};
  EXPECT_EQ(occtl_mesh_make_plane_component_faces(myGraph, &anOpts, aTooSmall, 5u, &aCount),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aCount, 6u);

  occtl_node_id_t aFaces[6] = {};
  ASSERT_EQ(occtl_mesh_make_plane_component_faces(myGraph, &anOpts, aFaces, 6u, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 6u);

  for (size_t anIdx = 0u; anIdx < aCount; ++anIdx)
  {
    ASSERT_NE(aFaces[anIdx].bits, 0u);
    occtl_node_kind_t aKind = static_cast<occtl_node_kind_t>(0);
    ASSERT_EQ(occtl_graph_node_kind(myGraph, aFaces[anIdx], &aKind), OCCTL_OK);
    EXPECT_EQ(aKind, OCCTL_KIND_FACE);
  }
}

TEST(MeshTrianglePlaneComponentsTest, OptionsInit_HasExpectedDefaults)
{
  occtl_mesh_triangle_plane_components_options_t anOpts{};
  occtl_mesh_triangle_plane_components_options_init(&anOpts);

  EXPECT_EQ(anOpts.struct_version, OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_VERSION_1);
  EXPECT_EQ(anOpts.p_next, nullptr);
  EXPECT_EQ(anOpts.root.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anOpts.max_normal_angle, 0.017453292519943295);
  EXPECT_EQ(anOpts.include_opposite_normals, 1);
  EXPECT_DOUBLE_EQ(anOpts.max_distance, 1.0e-6);
  EXPECT_DOUBLE_EQ(anOpts.min_area, 0.0);
  EXPECT_EQ(anOpts.min_triangle_count, 1u);

  occtl_mesh_triangle_plane_components_options_init(nullptr);
}

TEST(MeshTriangleSphereComponentsTest, OptionsInit_HasExpectedDefaults)
{
  occtl_mesh_triangle_sphere_components_options_t anOpts{};
  occtl_mesh_triangle_sphere_components_options_init(&anOpts);

  EXPECT_EQ(anOpts.struct_version, OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_VERSION_1);
  EXPECT_EQ(anOpts.p_next, nullptr);
  EXPECT_EQ(anOpts.root.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anOpts.max_normal_angle, 0.5235987755982989);
  EXPECT_EQ(anOpts.include_opposite_normals, 1);
  EXPECT_DOUBLE_EQ(anOpts.max_distance, 1.0e-3);
  EXPECT_DOUBLE_EQ(anOpts.min_area, 0.0);
  EXPECT_EQ(anOpts.min_triangle_count, 4u);
  EXPECT_DOUBLE_EQ(anOpts.min_radius, 1.0e-9);

  occtl_mesh_triangle_sphere_components_options_init(nullptr);
}

TEST(MeshTriangleCylinderComponentsTest, OptionsInit_HasExpectedDefaults)
{
  occtl_mesh_triangle_cylinder_components_options_t anOpts{};
  occtl_mesh_triangle_cylinder_components_options_init(&anOpts);

  EXPECT_EQ(anOpts.struct_version, OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_VERSION_1);
  EXPECT_EQ(anOpts.p_next, nullptr);
  EXPECT_EQ(anOpts.root.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anOpts.max_normal_angle, 0.5235987755982989);
  EXPECT_EQ(anOpts.include_opposite_normals, 1);
  EXPECT_DOUBLE_EQ(anOpts.max_distance, 1.0e-3);
  EXPECT_DOUBLE_EQ(anOpts.min_area, 0.0);
  EXPECT_EQ(anOpts.min_triangle_count, 4u);
  EXPECT_DOUBLE_EQ(anOpts.min_radius, 1.0e-9);

  occtl_mesh_triangle_cylinder_components_options_init(nullptr);
}

TEST(MeshTriangleComponentsTest, OptionsInit_HasExpectedDefaults)
{
  occtl_mesh_triangle_components_options_t anOpts{};
  occtl_mesh_triangle_components_options_init(&anOpts);

  EXPECT_EQ(anOpts.struct_version, OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_VERSION_1);
  EXPECT_EQ(anOpts.p_next, nullptr);
  EXPECT_EQ(anOpts.root.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anOpts.max_normal_angle, 0.017453292519943295);
  EXPECT_EQ(anOpts.include_opposite_normals, 1);

  occtl_mesh_triangle_components_options_init(nullptr);
}

TEST_F(MeshFixture, TriangleBuffers_NoMesh_ReturnsNotFound)
{
  ASSERT_NE(makeBox(myGraph, 10.0, 10.0, 10.0).bits, 0u);

  occtl_mesh_triangle_buffers_view_t aView{};
  EXPECT_EQ(occtl_mesh_triangle_buffers(myGraph, OCCTL_NODE_ID_INVALID, &aView), OCCTL_NOT_FOUND);
}

TEST_F(MeshFixture, TriangleBuffers_InvalidArguments_ReturnExpectedStatus)
{
  occtl_mesh_triangle_buffers_view_t aView{};
  EXPECT_EQ(occtl_mesh_triangle_buffers(nullptr, OCCTL_NODE_ID_INVALID, &aView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_buffers(myGraph, OCCTL_NODE_ID_INVALID, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_buffers(myGraph, ::occtl_node_id_t{0xCAFEu}, &aView),
            OCCTL_NOT_FOUND);
}

TEST_F(MeshFixture, TriangleAnalysis_InvalidArguments_ReturnExpectedStatus)
{
  occtl_mesh_triangle_analysis_view_t aView{};
  EXPECT_EQ(occtl_mesh_triangle_analysis(nullptr, OCCTL_NODE_ID_INVALID, &aView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_analysis(myGraph, OCCTL_NODE_ID_INVALID, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_analysis(myGraph, ::occtl_node_id_t{0xCAFEu}, &aView),
            OCCTL_NOT_FOUND);
}

TEST_F(MeshFixture, TriangleComponents_InvalidArguments_ReturnExpectedStatus)
{
  occtl_mesh_triangle_components_options_t anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  occtl_mesh_triangle_components_view_t    aView{};

  EXPECT_EQ(occtl_mesh_triangle_components(nullptr, &anOpts, &aView), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_components(myGraph, nullptr, &aView), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_components(myGraph, &anOpts, nullptr), OCCTL_INVALID_ARGUMENT);

  anOpts.struct_version = 999u;
  EXPECT_EQ(occtl_mesh_triangle_components(myGraph, &anOpts, &aView), OCCTL_VERSION_MISMATCH);

  anOpts                  = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  anOpts.max_normal_angle = -1.0;
  EXPECT_EQ(occtl_mesh_triangle_components(myGraph, &anOpts, &aView), OCCTL_INVALID_ARGUMENT);

  occtl_mesh_triangle_component_summaries_view_t aSummaryView{};
  anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_triangle_component_summaries(nullptr, &anOpts, &aSummaryView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_component_summaries(myGraph, nullptr, &aSummaryView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_component_summaries(myGraph, &anOpts, nullptr),
            OCCTL_INVALID_ARGUMENT);

  occtl_mesh_triangle_plane_components_options_t aPlaneOpts =
    OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;
  occtl_mesh_triangle_plane_components_view_t aPlaneView{};
  EXPECT_EQ(occtl_mesh_triangle_plane_components(nullptr, &aPlaneOpts, &aPlaneView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_plane_components(myGraph, nullptr, &aPlaneView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_plane_components(myGraph, &aPlaneOpts, nullptr),
            OCCTL_INVALID_ARGUMENT);

  aPlaneOpts.struct_version = 999u;
  EXPECT_EQ(occtl_mesh_triangle_plane_components(myGraph, &aPlaneOpts, &aPlaneView),
            OCCTL_VERSION_MISMATCH);

  aPlaneOpts              = OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;
  aPlaneOpts.max_distance = -1.0;
  EXPECT_EQ(occtl_mesh_triangle_plane_components(myGraph, &aPlaneOpts, &aPlaneView),
            OCCTL_INVALID_ARGUMENT);

  occtl_mesh_triangle_sphere_components_options_t aSphereOpts =
    OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  occtl_mesh_triangle_sphere_components_view_t aSphereView{};
  EXPECT_EQ(occtl_mesh_triangle_sphere_components(nullptr, &aSphereOpts, &aSphereView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_sphere_components(myGraph, nullptr, &aSphereView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_sphere_components(myGraph, &aSphereOpts, nullptr),
            OCCTL_INVALID_ARGUMENT);

  aSphereOpts.struct_version = 999u;
  EXPECT_EQ(occtl_mesh_triangle_sphere_components(myGraph, &aSphereOpts, &aSphereView),
            OCCTL_VERSION_MISMATCH);

  aSphereOpts              = OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  aSphereOpts.max_distance = -1.0;
  EXPECT_EQ(occtl_mesh_triangle_sphere_components(myGraph, &aSphereOpts, &aSphereView),
            OCCTL_INVALID_ARGUMENT);

  occtl_mesh_triangle_cylinder_components_options_t aCylinderOpts =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  occtl_mesh_triangle_cylinder_components_view_t aCylinderView{};
  EXPECT_EQ(occtl_mesh_triangle_cylinder_components(nullptr, &aCylinderOpts, &aCylinderView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_cylinder_components(myGraph, nullptr, &aCylinderView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_cylinder_components(myGraph, &aCylinderOpts, nullptr),
            OCCTL_INVALID_ARGUMENT);

  aCylinderOpts.struct_version = 999u;
  EXPECT_EQ(occtl_mesh_triangle_cylinder_components(myGraph, &aCylinderOpts, &aCylinderView),
            OCCTL_VERSION_MISMATCH);

  aCylinderOpts              = OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  aCylinderOpts.max_distance = -1.0;
  EXPECT_EQ(occtl_mesh_triangle_cylinder_components(myGraph, &aCylinderOpts, &aCylinderView),
            OCCTL_INVALID_ARGUMENT);

  occtl_node_id_t aCurvedSolid = OCCTL_NODE_ID_INVALID;
  aSphereOpts                  = OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_make_sphere_component_solid(nullptr, &aSphereOpts, 0u, &aCurvedSolid),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_sphere_component_solid(myGraph, nullptr, 0u, &aCurvedSolid),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_sphere_component_solid(myGraph, &aSphereOpts, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);

  aCylinderOpts = OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_make_cylinder_component_solid(nullptr, &aCylinderOpts, 0u, &aCurvedSolid),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_cylinder_component_solid(myGraph, nullptr, 0u, &aCurvedSolid),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_cylinder_component_solid(myGraph, &aCylinderOpts, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);

  size_t aCurvedCount = 0u;
  aSphereOpts         = OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  EXPECT_EQ(
    occtl_mesh_make_sphere_component_solids(nullptr, &aSphereOpts, nullptr, 0u, &aCurvedCount),
    OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_sphere_component_solids(myGraph, nullptr, nullptr, 0u, &aCurvedCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_sphere_component_solids(myGraph, &aSphereOpts, nullptr, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);

  aCylinderOpts = OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  EXPECT_EQ(
    occtl_mesh_make_cylinder_component_solids(nullptr, &aCylinderOpts, nullptr, 0u, &aCurvedCount),
    OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_cylinder_component_solids(myGraph, nullptr, nullptr, 0u, &aCurvedCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(
    occtl_mesh_make_cylinder_component_solids(myGraph, &aCylinderOpts, nullptr, 0u, nullptr),
    OCCTL_INVALID_ARGUMENT);

  occtl_mesh_triangle_component_triangles_view_t aTriangleView{};
  anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_triangle_component_triangles(nullptr, &anOpts, 0u, &aTriangleView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_component_triangles(myGraph, nullptr, 0u, &aTriangleView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_component_triangles(myGraph, &anOpts, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);

  occtl_mesh_triangle_component_boundary_view_t aBoundaryView{};
  anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_triangle_component_boundary(nullptr, &anOpts, 0u, &aBoundaryView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_component_boundary(myGraph, nullptr, 0u, &aBoundaryView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_component_boundary(myGraph, &anOpts, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);

  occtl_mesh_triangle_component_boundary_chains_view_t aChainView{};
  anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_triangle_component_boundary_chains(nullptr, &anOpts, 0u, &aChainView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_component_boundary_chains(myGraph, nullptr, 0u, &aChainView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_triangle_component_boundary_chains(myGraph, &anOpts, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);

  occtl_mesh_component_boundary_polylines_view_t aPolylineView{};
  anOpts = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_component_boundary_polylines(nullptr, &anOpts, 0u, &aPolylineView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_component_boundary_polylines(myGraph, nullptr, 0u, &aPolylineView),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_component_boundary_polylines(myGraph, &anOpts, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);

  occtl_mesh_triangle_plane_components_options_t aRebuildOpts =
    OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;
  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_mesh_make_plane_component_face(nullptr, &aRebuildOpts, 0u, &aFace),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_plane_component_face(myGraph, nullptr, 0u, &aFace),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_plane_component_face(myGraph, &aRebuildOpts, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);

  size_t aFaceCount = 0u;
  EXPECT_EQ(occtl_mesh_make_plane_component_faces(nullptr, &aRebuildOpts, nullptr, 0u, &aFaceCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_plane_component_faces(myGraph, nullptr, nullptr, 0u, &aFaceCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_mesh_make_plane_component_faces(myGraph, &aRebuildOpts, nullptr, 0u, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

} // namespace
