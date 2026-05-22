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

class PrimPrismTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
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

TEST_F(PrimPrismTest, MakePrism_FaceProfile_CreatesSolid)
{
  occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.profile                 = myFace;
  anInfo.direction               = {0.0, 0.0, 5.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_prism(myGraph, &anInfo, &aShape), OCCTL_OK);
  ASSERT_NE(aShape.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  // Extruding a Face yields a Solid.
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimPrismTest, MakePrism_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.profile                 = myFace;
  anInfo.direction               = {0.0, 0.0, 1.0};
  occtl_node_id_t aShape         = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_prism(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_prism(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_prism(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimPrismTest, MakePrism_VersionMismatch_Rejected)
{
  occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.struct_version          = 0u;
  anInfo.profile                 = myFace;
  anInfo.direction               = {0.0, 0.0, 1.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_prism(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimPrismTest, MakePrism_NonNullPNext_ReturnsInvalidArgument)
{
  int                     aTag   = 0;
  occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.p_next                  = &aTag;
  anInfo.profile                 = myFace;
  anInfo.direction               = {0.0, 0.0, 1.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimPrismTest, MakePrism_NanDirection_ReturnsInvalidArgument)
{
  occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.profile                 = myFace;
  anInfo.direction               = {0.0, std::numeric_limits<double>::quiet_NaN(), 1.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimPrismTest, MakePrism_BadBoolean_ReturnsInvalidArgument)
{
  occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.profile                 = myFace;
  anInfo.direction               = {0.0, 0.0, 1.0};
  anInfo.copy                    = 2;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimPrismTest, MakePrism_SolidProfile_WrongKind)
{
  occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.profile                 = myBoxSolid; // Solid cannot be extruded
  anInfo.direction               = {0.0, 0.0, 1.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_prism(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

TEST_F(PrimPrismTest, MakePrism_ZeroDirection_InvalidArgument)
{
  occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.profile                 = myFace;
  anInfo.direction               = {0.0, 0.0, 0.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimPrismTest, MakePrism_InvalidProfileNode_NotFound)
{
  occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.profile                 = OCCTL_NODE_ID_INVALID;
  anInfo.direction               = {0.0, 0.0, 1.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_prism(myGraph, &anInfo, &aShape), OCCTL_NOT_FOUND);
}

} // namespace
