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

class PrimSplineTest : public ::testing::Test
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

TEST_F(PrimSplineTest, Spline_ThroughFivePoints_CreatesWire)
{
  const occtl_point3_t     aPts[5] = {{0, 0, 0}, {1, 1, 0}, {2, 0, 1}, {3, -1, 0}, {4, 0, 0}};
  occtl_prim_spline_info_t anInfo  = OCCTL_PRIM_SPLINE_INFO_INIT;
  anInfo.points                    = aPts;
  anInfo.point_count               = 5;
  anInfo.degree_min                = 1;
  anInfo.degree_max                = 8;
  anInfo.tolerance                 = 1.0e-6;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_spline(myGraph, &anInfo, &aWire), OCCTL_OK);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aWire, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_WIRE);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
}

TEST_F(PrimSplineTest, Spline_NullPointsArray_InvalidArgument)
{
  occtl_prim_spline_info_t anInfo = OCCTL_PRIM_SPLINE_INFO_INIT;
  occtl_node_id_t          aWire  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_spline(myGraph, &anInfo, &aWire), OCCTL_GEOMETRY_INVALID);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimSplineTest, Spline_NullPointers_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_prim_make_spline(nullptr, nullptr, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimSplineTest, Spline_VersionMismatch_Rejected)
{
  const occtl_point3_t     aPts[2] = {{0, 0, 0}, {1, 0, 0}};
  occtl_prim_spline_info_t anInfo  = OCCTL_PRIM_SPLINE_INFO_INIT;
  anInfo.struct_version            = 0u;
  anInfo.points                    = aPts;
  anInfo.point_count               = 2;
  occtl_node_id_t aWire            = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_spline(myGraph, &anInfo, &aWire), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimSplineTest, Spline_InvalidDegreeRange_InvalidArgument)
{
  const occtl_point3_t     aPts[2] = {{0, 0, 0}, {1, 0, 0}};
  occtl_prim_spline_info_t anInfo  = OCCTL_PRIM_SPLINE_INFO_INIT;
  anInfo.points                    = aPts;
  anInfo.point_count               = 2;
  anInfo.degree_min                = 5;
  anInfo.degree_max                = 3;
  occtl_node_id_t aWire            = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_spline(myGraph, &anInfo, &aWire), OCCTL_GEOMETRY_INVALID);
}

} // namespace
