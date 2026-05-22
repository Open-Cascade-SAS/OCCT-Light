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

#include <gtest/gtest.h>

#include <limits>

namespace
{

using mesh_test::makeBox;
using mesh_test::MeshFixture;

void expectLastErrorMessage()
{
  const occtl_error_t* const anError = occtl_error_last();
  ASSERT_NE(anError, nullptr);
  ASSERT_NE(anError->message, nullptr);
  EXPECT_NE(anError->message[0], '\0');
}

TEST_F(MeshFixture, Generate_NullGraph_InvalidArgument)
{
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_generate(nullptr, nullptr, 0, &aOpts), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(MeshFixture, Generate_NullOptions_InvalidArgument)
{
  EXPECT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, nullptr), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(MeshFixture, Generate_NullNodesWithCount_InvalidArgument)
{
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_generate(myGraph, nullptr, 3, &aOpts), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(MeshFixture, Generate_BadVersion_VersionMismatch)
{
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  aOpts.struct_version       = 0xDEADBEEFu;
  EXPECT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_VERSION_MISMATCH);
  expectLastErrorMessage();
}

TEST_F(MeshFixture, Generate_NonNullPNext_InvalidArgument)
{
  int                  aDummy = 0;
  occtl_mesh_options_t aOpts  = OCCTL_MESH_OPTIONS_INIT;
  aOpts.p_next                = &aDummy;
  EXPECT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(MeshFixture, Generate_NanDeflection_InvalidArgument)
{
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  aOpts.deflection           = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(MeshFixture, Generate_BadBoolean_InvalidArgument)
{
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  aOpts.in_parallel          = 2;
  EXPECT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(MeshFixture, Generate_EmptyGraph_Ok)
{
  // Whole-graph dispatch on an empty graph is a no-op success.
  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);
}

TEST_F(MeshFixture, Generate_VertexOnlyGraph_Ok)
{
  // A graph carrying only a free vertex has nothing to mesh; whole-graph
  // dispatch should succeed without crashing or warning.
  occtl_topo_make_vertex_info_t aInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aInfo.point                         = {1.0, 2.0, 3.0};
  aInfo.tolerance                     = 1.0e-7;
  occtl_node_id_t aVertex             = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aInfo, &aVertex), OCCTL_OK);

  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);
}

TEST_F(MeshFixture, Generate_WholeGraphBox_OkAndFacesGetTriangulation)
{
  const occtl_node_id_t aBox = makeBox(myGraph, 10.0, 10.0, 10.0);
  ASSERT_NE(aBox.bits, 0u);

  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);

  // After meshing, every face should have a triangulation. Use the
  // face iterator and the existing topo predicate to confirm.
  occtl_node_iter_t* aIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &aIter), OCCTL_OK);

  size_t          aFacesSeen   = 0;
  size_t          aFacesMeshed = 0;
  occtl_node_id_t aFaceId{};
  while (occtl_node_iter_next(aIter, &aFaceId) == OCCTL_OK)
  {
    int32_t aHas = 0;
    ASSERT_EQ(occtl_topo_face_has_triangulation(myGraph, aFaceId, &aHas), OCCTL_OK);
    ++aFacesSeen;
    if (aHas != 0)
    {
      ++aFacesMeshed;
    }
  }
  occtl_node_iter_free(aIter);

  EXPECT_EQ(aFacesSeen, 6u);
  EXPECT_EQ(aFacesMeshed, 6u);
}

TEST_F(MeshFixture, Generate_SingleRoot_OkAndSubtreeMeshed)
{
  const occtl_node_id_t aBox = makeBox(myGraph, 5.0, 5.0, 5.0);
  ASSERT_NE(aBox.bits, 0u);

  occtl_mesh_options_t aOpts = OCCTL_MESH_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_generate(myGraph, &aBox, 1, &aOpts), OCCTL_OK);
}

TEST_F(MeshFixture, Generate_BadNodeId_NotFound)
{
  occtl_mesh_options_t  aOpts  = OCCTL_MESH_OPTIONS_INIT;
  const occtl_node_id_t aBadId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_mesh_generate(myGraph, &aBadId, 1, &aOpts), OCCTL_NOT_FOUND);
}

TEST_F(MeshFixture, Generate_BboxMode_Ok)
{
  const occtl_node_id_t aBox = makeBox(myGraph, 10.0, 10.0, 10.0);
  ASSERT_NE(aBox.bits, 0u);

  occtl_mesh_options_t aOpts  = OCCTL_MESH_OPTIONS_INIT;
  aOpts.use_bbox              = 1;
  aOpts.bbox.min              = {0.0, 0.0, 0.0};
  aOpts.bbox.max              = {10.0, 10.0, 10.0};
  aOpts.deviation_coefficient = 0.005;
  EXPECT_EQ(occtl_mesh_generate(myGraph, nullptr, 0, &aOpts), OCCTL_OK);
}

TEST_F(MeshFixture, Generate_MultiNode_OkAndAllFacesMeshed)
{
  const occtl_node_id_t aBoxA = makeBox(myGraph, 5.0, 5.0, 5.0);
  const occtl_node_id_t aBoxB = makeBox(myGraph, 5.0, 5.0, 5.0);
  ASSERT_NE(aBoxA.bits, 0u);
  ASSERT_NE(aBoxB.bits, 0u);

  const occtl_node_id_t aRoots[2] = {aBoxA, aBoxB};
  occtl_mesh_options_t  aOpts     = OCCTL_MESH_OPTIONS_INIT;
  EXPECT_EQ(occtl_mesh_generate(myGraph, aRoots, 2, &aOpts), OCCTL_OK);

  // Every face across both boxes should be meshed (12 total).
  occtl_node_iter_t* aIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &aIter), OCCTL_OK);
  size_t          aMeshed = 0;
  occtl_node_id_t aFaceId{};
  while (occtl_node_iter_next(aIter, &aFaceId) == OCCTL_OK)
  {
    int32_t aHas = 0;
    ASSERT_EQ(occtl_topo_face_has_triangulation(myGraph, aFaceId, &aHas), OCCTL_OK);
    if (aHas != 0)
    {
      ++aMeshed;
    }
  }
  occtl_node_iter_free(aIter);

  EXPECT_EQ(aMeshed, 12u);
}

} // namespace
