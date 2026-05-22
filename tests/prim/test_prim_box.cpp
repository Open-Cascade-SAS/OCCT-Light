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

class PrimBoxTest : public ::testing::Test
{
protected:
  void SetUp() override { ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK); }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(PrimBoxTest, MakeBox_AxisAligned_CreatesSolid)
{
  occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.dx                    = 10.0;
  anInfo.dy                    = 20.0;
  anInfo.dz                    = 30.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_box(myGraph, &anInfo, &aSolid), OCCTL_OK);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSolid, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);

  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_SOLID), 1u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_SHELL), 1u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_FACE), 6u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_WIRE), 6u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 12u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_VERTEX), 8u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_PRODUCT), 0u);
}

TEST_F(PrimBoxTest, MakeBox_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.dx                    = 1.0;
  anInfo.dy                    = 1.0;
  anInfo.dz                    = 1.0;
  occtl_node_id_t aSolid       = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_box(nullptr, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_box(myGraph, nullptr, &aSolid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_box(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimBoxTest, MakeBox_VersionMismatch_Rejected)
{
  occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.struct_version        = 0u;
  anInfo.dx                    = 1.0;
  anInfo.dy                    = 1.0;
  anInfo.dz                    = 1.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_box(myGraph, &anInfo, &aSolid), OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aSolid.bits, 0u);
}

TEST_F(PrimBoxTest, MakeBox_NonNullPNext_ReturnsInvalidArgument)
{
  int                   aTag   = 0;
  occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.p_next                = &aTag;
  anInfo.dx                    = 1.0;
  anInfo.dy                    = 1.0;
  anInfo.dz                    = 1.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_box(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimBoxTest, MakeBox_NanDimension_ReturnsInvalidArgument)
{
  occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.dx                    = std::numeric_limits<double>::quiet_NaN();
  anInfo.dy                    = 1.0;
  anInfo.dz                    = 1.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_box(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimBoxTest, MakeBox_ZeroDimension_GeometryInvalid)
{
  occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.dx                    = 10.0;
  anInfo.dy                    = 0.0;
  anInfo.dz                    = 10.0;

  occtl_node_id_t      aSolid  = OCCTL_NODE_ID_INVALID;
  const occtl_status_t aStatus = occtl_prim_make_box(myGraph, &anInfo, &aSolid);
  EXPECT_NE(aStatus, OCCTL_OK);
  EXPECT_EQ(aSolid.bits, 0u);

  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_NE(anErr->message, nullptr);
  if (anErr->message != nullptr)
  {
    EXPECT_GT(std::string(anErr->message).size(), 0u);
  }
}

TEST_F(PrimBoxTest, MakeBox_TranslatedPlacement_LocatesSolid)
{
  occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.placement.location    = {5.0, 5.0, 5.0};
  anInfo.dx                    = 1.0;
  anInfo.dy                    = 2.0;
  anInfo.dz                    = 3.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_box(myGraph, &anInfo, &aSolid), OCCTL_OK);
  EXPECT_NE(aSolid.bits, 0u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_VERTEX), 8u);
}

} // namespace
