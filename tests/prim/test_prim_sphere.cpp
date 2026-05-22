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

class PrimSphereTest : public ::testing::Test
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

TEST_F(PrimSphereTest, MakeSphere_FullSphere_CreatesSolid)
{
  occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.radius                   = 5.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_sphere(myGraph, &anInfo, &aSolid), OCCTL_OK);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSolid, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);

  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_SOLID), 1u);
  EXPECT_GE(countOfKind(myGraph, OCCTL_KIND_FACE), 1u);
}

TEST_F(PrimSphereTest, MakeSphere_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.radius                   = 1.0;
  occtl_node_id_t aSolid          = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_sphere(nullptr, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_sphere(myGraph, nullptr, &aSolid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_sphere(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimSphereTest, MakeSphere_VersionMismatch_Rejected)
{
  occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.struct_version           = 99u;
  anInfo.radius                   = 1.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_sphere(myGraph, &anInfo, &aSolid), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimSphereTest, MakeSphere_NonNullPNext_ReturnsInvalidArgument)
{
  int                      aTag   = 0;
  occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.p_next                   = &aTag;
  anInfo.radius                   = 1.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_sphere(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimSphereTest, MakeSphere_NanAngle_ReturnsInvalidArgument)
{
  occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.radius                   = 1.0;
  anInfo.angle                    = std::numeric_limits<double>::quiet_NaN();

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_sphere(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimSphereTest, MakeSphere_NegativeRadius_GeometryInvalid)
{
  occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.radius                   = -1.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_NE(occtl_prim_make_sphere(myGraph, &anInfo, &aSolid), OCCTL_OK);
  EXPECT_EQ(aSolid.bits, 0u);
}

TEST_F(PrimSphereTest, MakeSphere_PartialWedge_CreatesSolid)
{
  occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.radius                   = 3.0;
  anInfo.angle                    = 3.141592653589793; // half-sphere wedge

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_sphere(myGraph, &anInfo, &aSolid), OCCTL_OK);
  EXPECT_NE(aSolid.bits, 0u);
}

} // namespace
