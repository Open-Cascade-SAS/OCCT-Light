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

class PrimRevolTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    // Use a face of a box translated off-axis, so revolving it produces a non-degenerate solid.
    occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
    aBox.placement.location    = {2.0, 0.0, 0.0};
    aBox.dx                    = 1.0;
    aBox.dy                    = 1.0;
    aBox.dz                    = 1.0;
    ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &myBoxSolid), OCCTL_OK);

    occtl_node_iter_t* anIter = nullptr;
    ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &anIter), OCCTL_OK);
    ASSERT_EQ(occtl_node_iter_next(anIter, &myFace), OCCTL_OK);
    occtl_node_iter_free(anIter);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t*  myGraph    = nullptr;
  occtl_node_id_t myBoxSolid = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t myFace     = OCCTL_NODE_ID_INVALID;
};

TEST_F(PrimRevolTest, MakeRevol_FullTurn_CreatesShape)
{
  occtl_prim_revol_info_t anInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  anInfo.profile                 = myFace;
  anInfo.axis                    = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_revol(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimRevolTest, MakeRevol_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_revol_info_t anInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  anInfo.profile                 = myFace;
  occtl_node_id_t aShape         = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_revol(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_revol(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_revol(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimRevolTest, MakeRevol_VersionMismatch_Rejected)
{
  occtl_prim_revol_info_t anInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  anInfo.struct_version          = 0u;
  anInfo.profile                 = myFace;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_revol(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimRevolTest, MakeRevol_NonNullPNext_ReturnsInvalidArgument)
{
  int                     aTag   = 0;
  occtl_prim_revol_info_t anInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  anInfo.p_next                  = &aTag;
  anInfo.profile                 = myFace;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_revol(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimRevolTest, MakeRevol_NanAngle_ReturnsInvalidArgument)
{
  occtl_prim_revol_info_t anInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  anInfo.profile                 = myFace;
  anInfo.angle                   = std::numeric_limits<double>::quiet_NaN();

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_revol(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimRevolTest, MakeRevol_BadBoolean_ReturnsInvalidArgument)
{
  occtl_prim_revol_info_t anInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  anInfo.profile                 = myFace;
  anInfo.copy                    = 2;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_revol(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimRevolTest, MakeRevol_SolidProfile_WrongKind)
{
  occtl_prim_revol_info_t anInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  anInfo.profile                 = myBoxSolid;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_revol(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

TEST_F(PrimRevolTest, MakeRevol_ZeroAxisDirection_InvalidArgument)
{
  occtl_prim_revol_info_t anInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  anInfo.profile                 = myFace;
  anInfo.axis                    = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_revol(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

} // namespace
