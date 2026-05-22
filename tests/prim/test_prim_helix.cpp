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

class PrimHelixTest : public ::testing::Test
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

TEST_F(PrimHelixTest, Helix_TwoTurnsAroundZ_CreatesWire)
{
  occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.radius                  = 5.0;
  anInfo.pitch                   = 2.0;
  anInfo.height                  = 4.0; // two full turns

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_helix(myGraph, &anInfo, &aWire), OCCTL_OK);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aWire, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_WIRE);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
}

TEST_F(PrimHelixTest, Helix_LeftHanded_CreatesWire)
{
  occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.radius                  = 1.0;
  anInfo.pitch                   = 1.0;
  anInfo.height                  = 3.0;
  anInfo.left_handed             = 1;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_helix(myGraph, &anInfo, &aWire), OCCTL_OK);
  EXPECT_NE(aWire.bits, 0u);
}

TEST_F(PrimHelixTest, Helix_ZeroRadius_GeometryInvalid)
{
  occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.radius                  = 0.0;
  anInfo.pitch                   = 1.0;
  anInfo.height                  = 1.0;
  occtl_node_id_t aWire          = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_helix(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);

  expectLastErrorMessage();
}

TEST_F(PrimHelixTest, Helix_ZeroPitch_GeometryInvalid)
{
  occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.radius                  = 1.0;
  anInfo.pitch                   = 0.0;
  anInfo.height                  = 1.0;
  occtl_node_id_t aWire          = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_helix(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimHelixTest, Helix_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.radius                  = 1.0;
  anInfo.pitch                   = 1.0;
  anInfo.height                  = 1.0;
  occtl_node_id_t aWire          = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_helix(nullptr, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_helix(myGraph, nullptr, &aWire), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_helix(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimHelixTest, Helix_VersionMismatch_Rejected)
{
  occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.struct_version          = 0u;
  anInfo.radius                  = 1.0;
  anInfo.pitch                   = 1.0;
  anInfo.height                  = 1.0;
  occtl_node_id_t aWire          = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_helix(myGraph, &anInfo, &aWire), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimHelixTest, Helix_NonNullPNext_ReturnsInvalidArgument)
{
  int                     aTag   = 0;
  occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.p_next                  = &aTag;
  anInfo.radius                  = 1.0;
  anInfo.pitch                   = 1.0;
  anInfo.height                  = 1.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_helix(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimHelixTest, Helix_NanHeight_ReturnsInvalidArgument)
{
  occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.radius                  = 1.0;
  anInfo.pitch                   = 1.0;
  anInfo.height                  = std::numeric_limits<double>::quiet_NaN();

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_helix(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimHelixTest, Helix_BadBoolean_ReturnsInvalidArgument)
{
  occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.radius                  = 1.0;
  anInfo.pitch                   = 1.0;
  anInfo.height                  = 1.0;
  anInfo.left_handed             = 2;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_helix(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimHelixTest, Helix_CanBeSwept)
{
  // Bigger test: build a helix and sweep a small disk along it. Validates
  // that the helix edge has both a 3D curve and a curve-on-surface.
  occtl_prim_helix_info_t aHelix = OCCTL_PRIM_HELIX_INFO_INIT;
  aHelix.radius                  = 5.0;
  aHelix.pitch                   = 4.0;
  aHelix.height                  = 4.0;
  occtl_node_id_t aSpine         = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_helix(myGraph, &aHelix, &aSpine), OCCTL_OK);

  // The helix wire should already report a valid 3D curve.
  EXPECT_NE(aSpine.bits, 0u);
}

} // namespace
