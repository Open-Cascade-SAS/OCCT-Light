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

#include <limits>

namespace
{

class PrimFillet2dTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    // Build a 10x10 rectangle face to fillet.
    occtl_prim_rectangle_info_t aRectInfo = OCCTL_PRIM_RECTANGLE_INFO_INIT;
    aRectInfo.width                       = 10.0;
    aRectInfo.height                      = 10.0;
    occtl_node_id_t aRectWire             = OCCTL_NODE_ID_INVALID;
    ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRectInfo, &aRectWire), OCCTL_OK);

    occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
    aFaceInfo.outer_wire                    = aRectWire;
    ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &myRectFace), OCCTL_OK);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t*  myGraph    = nullptr;
  occtl_node_id_t myRectFace = OCCTL_NODE_ID_INVALID;
};

TEST_F(PrimFillet2dTest, Fillet2d_AllCorners_CreatesFilletedFace)
{
  occtl_prim_fillet_2d_info_t anInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  anInfo.face                        = myRectFace;
  anInfo.radius                      = 1.0;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_fillet_2d(myGraph, &anInfo, &aResult), OCCTL_OK);
  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aResult, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
  // Rectangle (4 edges) → filleted face has 4 original + 4 arc = 8 edges total.
  EXPECT_GE(countOfKind(myGraph, OCCTL_KIND_EDGE), 4u);
}

TEST_F(PrimFillet2dTest, Fillet2d_NegativeRadius_GeometryInvalid)
{
  occtl_prim_fillet_2d_info_t anInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  anInfo.face                        = myRectFace;
  anInfo.radius                      = -1.0;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_fillet_2d(myGraph, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);

  expectLastErrorMessage();
}

TEST_F(PrimFillet2dTest, Fillet2d_NonFaceInput_WrongKind)
{
  occtl_geom_circle_t aCircGeom;
  aCircGeom.position        = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  aCircGeom.radius          = 1.0;
  occtl_rep_id_t aCircCurve = {};
  ASSERT_EQ(occtl_curve_create_circle(myGraph, aCircGeom, &aCircCurve), OCCTL_OK);
  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_curves_to_wire(myGraph, &aCircCurve, 1, &aWire), OCCTL_OK);

  occtl_prim_fillet_2d_info_t anInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  anInfo.face                        = aWire; // a Wire, not a Face
  anInfo.radius                      = 1.0;
  occtl_node_id_t aResult            = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_fillet_2d(myGraph, &anInfo, &aResult), OCCTL_WRONG_KIND);
}

TEST_F(PrimFillet2dTest, Fillet2d_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_fillet_2d_info_t anInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  anInfo.face                        = myRectFace;
  anInfo.radius                      = 1.0;
  occtl_node_id_t aResult            = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_fillet_2d(nullptr, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_fillet_2d(myGraph, nullptr, &aResult), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_fillet_2d(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimFillet2dTest, Fillet2d_VersionMismatch_Rejected)
{
  occtl_prim_fillet_2d_info_t anInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  anInfo.struct_version              = 0u;
  anInfo.face                        = myRectFace;
  anInfo.radius                      = 1.0;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_fillet_2d(myGraph, &anInfo, &aResult), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimFillet2dTest, Fillet2d_NonNullPNext_ReturnsInvalidArgument)
{
  occtl_prim_fillet_2d_info_t anInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  anInfo.face                        = myRectFace;
  anInfo.radius                      = 1.0;
  anInfo.p_next                      = &anInfo;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_fillet_2d(myGraph, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimFillet2dTest, Fillet2d_NanRadius_ReturnsInvalidArgument)
{
  occtl_prim_fillet_2d_info_t anInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  anInfo.face                        = myRectFace;
  anInfo.radius                      = std::numeric_limits<double>::quiet_NaN();

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_fillet_2d(myGraph, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimFillet2dTest, Fillet2d_NullVerticesWithCount_InvalidArgument)
{
  occtl_prim_fillet_2d_info_t anInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  anInfo.face                        = myRectFace;
  anInfo.radius                      = 1.0;
  anInfo.vertices                    = nullptr;
  anInfo.vertex_count                = 1;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_fillet_2d(myGraph, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimFillet2dTest, FullRound2d_AutoRadius_ReplacesFaceEnd)
{
  const occtl_node_id_t anEdge = firstChildOfKind(myGraph, myRectFace, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  double anOriginalArea = 0.0;
  ASSERT_EQ(
    occtl_graph_measure_get(myGraph, myRectFace, OCCTL_SELECT_MEASURE_FACE_AREA, &anOriginalArea),
    OCCTL_OK);

  occtl_prim_full_round_2d_info_t anInfo = OCCTL_PRIM_FULL_ROUND_2D_INFO_INIT;
  anInfo.face                            = myRectFace;
  anInfo.edge                            = anEdge;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_full_round_2d(myGraph, &anInfo, &aResult), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aResult, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);

  double aRoundedArea = 0.0;
  ASSERT_EQ(
    occtl_graph_measure_get(myGraph, aResult, OCCTL_SELECT_MEASURE_FACE_AREA, &aRoundedArea),
    OCCTL_OK);
  EXPECT_GT(aRoundedArea, 0.0);
  EXPECT_LT(aRoundedArea, anOriginalArea);
}

TEST_F(PrimFillet2dTest, FullRound2d_ExplicitRadius_CreatesFace)
{
  const occtl_node_id_t anEdge = firstChildOfKind(myGraph, myRectFace, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_prim_full_round_2d_info_t anInfo{};
  occtl_prim_full_round_2d_info_init(&anInfo);
  anInfo.face   = myRectFace;
  anInfo.edge   = anEdge;
  anInfo.radius = 1.0;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_full_round_2d(myGraph, &anInfo, &aResult), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aResult, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(PrimFillet2dTest, FullRound2d_InvalidArguments_ReturnExpectedStatus)
{
  const occtl_node_id_t anEdge = firstChildOfKind(myGraph, myRectFace, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_prim_full_round_2d_info_t anInfo = OCCTL_PRIM_FULL_ROUND_2D_INFO_INIT;
  anInfo.face                            = myRectFace;
  anInfo.edge                            = anEdge;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_full_round_2d(nullptr, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_full_round_2d(myGraph, nullptr, &aResult), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_full_round_2d(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);

  anInfo.struct_version = 0u;
  EXPECT_EQ(occtl_prim_make_full_round_2d(myGraph, &anInfo, &aResult), OCCTL_VERSION_MISMATCH);

  anInfo        = OCCTL_PRIM_FULL_ROUND_2D_INFO_INIT;
  anInfo.face   = myRectFace;
  anInfo.edge   = anEdge;
  anInfo.radius = -1.0;
  EXPECT_EQ(occtl_prim_make_full_round_2d(myGraph, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);

  anInfo      = OCCTL_PRIM_FULL_ROUND_2D_INFO_INIT;
  anInfo.face = anEdge;
  anInfo.edge = anEdge;
  EXPECT_EQ(occtl_prim_make_full_round_2d(myGraph, &anInfo, &aResult), OCCTL_WRONG_KIND);
}

} // namespace
