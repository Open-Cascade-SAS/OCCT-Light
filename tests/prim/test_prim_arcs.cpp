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

#include <cstring>

namespace
{

class PrimArcsTest : public ::testing::Test
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

// ---- arc_3pt ----

TEST_F(PrimArcsTest, Arc3pt_HalfCircle_CreatesWire)
{
  occtl_prim_arc_3pt_info_t anInfo = OCCTL_PRIM_ARC_3PT_INFO_INIT;
  anInfo.start                     = {1.0, 0.0, 0.0};
  anInfo.via                       = {0.0, 1.0, 0.0};
  anInfo.end                       = {-1.0, 0.0, 0.0};

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_arc_3pt(myGraph, &anInfo, &aWire), OCCTL_OK);

  EXPECT_NE(aWire.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aWire, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_WIRE);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
}

TEST_F(PrimArcsTest, Arc3pt_CollinearPoints_GeometryInvalid)
{
  occtl_prim_arc_3pt_info_t anInfo = OCCTL_PRIM_ARC_3PT_INFO_INIT;
  anInfo.start                     = {0.0, 0.0, 0.0};
  anInfo.via                       = {1.0, 0.0, 0.0};
  anInfo.end                       = {2.0, 0.0, 0.0};

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_arc_3pt(myGraph, &anInfo, &aWire), OCCTL_GEOMETRY_INVALID);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimArcsTest, Arc3pt_NullOutCurve_ReturnsInvalidArgument)
{
  occtl_prim_arc_3pt_info_t anInfo = OCCTL_PRIM_ARC_3PT_INFO_INIT;
  EXPECT_EQ(occtl_prim_make_arc_3pt(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

// ---- arc_center ----

TEST_F(PrimArcsTest, ArcCenter_QuarterCircle_CreatesWire)
{
  occtl_prim_arc_center_info_t anInfo = OCCTL_PRIM_ARC_CENTER_INFO_INIT;
  anInfo.radius                       = 1.0;
  anInfo.start_angle                  = 0.0;
  anInfo.end_angle                    = 1.5707963267948966; // pi/2

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_arc_center(myGraph, &anInfo, &aWire), OCCTL_OK);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
}

TEST_F(PrimArcsTest, ArcCenter_EndAngleNotGreater_GeometryInvalid)
{
  occtl_prim_arc_center_info_t anInfo = OCCTL_PRIM_ARC_CENTER_INFO_INIT;
  anInfo.radius                       = 1.0;
  anInfo.start_angle                  = 1.0;
  anInfo.end_angle                    = 1.0;
  occtl_node_id_t aWire               = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_arc_center(myGraph, &anInfo, &aWire), OCCTL_GEOMETRY_INVALID);

  EXPECT_TRUE(std::strlen(occtl_error_last()->message) > 0u);
}

TEST_F(PrimArcsTest, ArcCenter_NullOutCurve_ReturnsInvalidArgument)
{
  occtl_prim_arc_center_info_t anInfo = OCCTL_PRIM_ARC_CENTER_INFO_INIT;
  EXPECT_EQ(occtl_prim_make_arc_center(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimArcsTest, ArcCenter_VersionMismatch_Rejected)
{
  occtl_prim_arc_center_info_t anInfo = OCCTL_PRIM_ARC_CENTER_INFO_INIT;
  anInfo.struct_version               = 0u;
  anInfo.radius                       = 1.0;
  anInfo.end_angle                    = 1.0;
  occtl_node_id_t aWire               = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_arc_center(myGraph, &anInfo, &aWire), OCCTL_VERSION_MISMATCH);
}

} // namespace
