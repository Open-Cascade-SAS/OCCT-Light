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

#include <occtl/occtl_curves.h>

namespace
{

class PrimPipeTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    // Build a face from a small box at the origin.
    occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
    aBox.dx                    = 0.5;
    aBox.dy                    = 0.5;
    aBox.dz                    = 0.5;
    occtl_node_id_t aSolid     = OCCTL_NODE_ID_INVALID;
    ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &aSolid), OCCTL_OK);

    occtl_node_iter_t* anIter = nullptr;
    ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &anIter), OCCTL_OK);
    ASSERT_EQ(occtl_node_iter_next(anIter, &myProfile), OCCTL_OK);
    occtl_node_iter_free(anIter);

    // Build a straight spine wire V0 -> V1 along Z with a real line curve.
    occtl_node_id_t aV0 = OCCTL_NODE_ID_INVALID, aV1 = OCCTL_NODE_ID_INVALID;
    {
      occtl_topo_make_vertex_info_t aVi = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
      aVi.tolerance                     = 1e-6;
      aVi.point                         = {0.0, 0.0, 0.0};
      ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVi, &aV0), OCCTL_OK);
      aVi.point = {0.0, 0.0, 5.0};
      ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVi, &aV1), OCCTL_OK);
    }

    occtl_geom_line_t aLine  = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    occtl_rep_id_t    aCurve = {};
    ASSERT_EQ(occtl_curve_create_line(myGraph, aLine, &aCurve), OCCTL_OK);

    occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
    {
      occtl_topo_make_edge_info_t aEi = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
      aEi.start_vertex                = aV0;
      aEi.end_vertex                  = aV1;
      aEi.curve                       = aCurve;
      aEi.first                       = 0.0;
      aEi.last                        = 5.0;
      aEi.tolerance                   = 1e-6;
      ASSERT_EQ(occtl_topo_make_edge(myGraph, &aEi, &anEdge), OCCTL_OK);
    }

    occtl_oriented_node_t       aOe = {anEdge, OCCTL_ORIENTATION_FORWARD};
    occtl_topo_make_wire_info_t aWi = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
    aWi.edges                       = &aOe;
    aWi.edge_count                  = 1;
    ASSERT_EQ(occtl_topo_make_wire(myGraph, &aWi, &mySpine), OCCTL_OK);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t*  myGraph   = nullptr;
  occtl_node_id_t myProfile = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t mySpine   = OCCTL_NODE_ID_INVALID;
};

TEST_F(PrimPipeTest, MakePipe_FaceProfileStraightSpine_CreatesShape)
{
  occtl_prim_pipe_info_t anInfo = OCCTL_PRIM_PIPE_INFO_INIT;
  anInfo.profile                = myProfile;
  anInfo.spine_wire             = mySpine;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_pipe(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimPipeTest, MakePipe_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_pipe_info_t anInfo = OCCTL_PRIM_PIPE_INFO_INIT;
  anInfo.profile                = myProfile;
  anInfo.spine_wire             = mySpine;
  occtl_node_id_t aShape        = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_pipe(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_pipe(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_pipe(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimPipeTest, MakePipe_VersionMismatch_Rejected)
{
  occtl_prim_pipe_info_t anInfo = OCCTL_PRIM_PIPE_INFO_INIT;
  anInfo.struct_version         = 0u;
  anInfo.profile                = myProfile;
  anInfo.spine_wire             = mySpine;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimPipeTest, MakePipe_NonNullPNext_ReturnsInvalidArgument)
{
  int                    aTag   = 0;
  occtl_prim_pipe_info_t anInfo = OCCTL_PRIM_PIPE_INFO_INIT;
  anInfo.p_next                 = &aTag;
  anInfo.profile                = myProfile;
  anInfo.spine_wire             = mySpine;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimPipeTest, MakePipe_NonWireSpine_WrongKind)
{
  occtl_prim_pipe_info_t anInfo = OCCTL_PRIM_PIPE_INFO_INIT;
  anInfo.profile                = myProfile;
  anInfo.spine_wire             = myProfile; // a Face, not a Wire

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

} // namespace
