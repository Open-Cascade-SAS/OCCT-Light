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

#include <occtl-hpp/prim.hpp>

#include <cstring>
#include <limits>

namespace
{

class PrimTwistExtrusionTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
    aRect.width                       = 4.0;
    aRect.height                      = 2.0;
    ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &myWire), OCCTL_OK);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t*  myGraph = nullptr;
  occtl_node_id_t myWire  = OCCTL_NODE_ID_INVALID;
};

TEST_F(PrimTwistExtrusionTest, TwistExtrusionInfoInit_HasDefaults)
{
  occtl_prim_twist_extrusion_info_t anInfo{};
  occtl_prim_twist_extrusion_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_TWIST_EXTRUSION_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.profile_wire.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anInfo.axis.location.x, 0.0);
  EXPECT_DOUBLE_EQ(anInfo.axis.direction.z, 1.0);
  EXPECT_DOUBLE_EQ(anInfo.height, 1.0);
  EXPECT_DOUBLE_EQ(anInfo.angle, 0.0);
  EXPECT_EQ(anInfo.section_count, 9);
  EXPECT_EQ(anInfo.make_solid, 1);
  EXPECT_EQ(anInfo.ruled, 1);
  EXPECT_DOUBLE_EQ(anInfo.pres3d, 1.0e-6);
}

TEST_F(PrimTwistExtrusionTest, ExtrudeTwistInfoInit_HasDefaults)
{
  occtl_prim_extrude_twist_info_t anInfo{};
  occtl_prim_extrude_twist_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_EXTRUDE_TWIST_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.profile_wire.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anInfo.axis.location.x, 0.0);
  EXPECT_DOUBLE_EQ(anInfo.axis.direction.z, 1.0);
  EXPECT_DOUBLE_EQ(anInfo.height, 1.0);
  EXPECT_DOUBLE_EQ(anInfo.angle, 0.0);
  EXPECT_EQ(anInfo.section_count, 9);
  EXPECT_EQ(anInfo.make_solid, 1);
  EXPECT_EQ(anInfo.ruled, 1);
  EXPECT_DOUBLE_EQ(anInfo.pres3d, 1.0e-6);
}

TEST_F(PrimTwistExtrusionTest, TwistExtrusion_RectangleWire_ReturnsSolid)
{
  occtl_prim_twist_extrusion_info_t anInfo = OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT;
  anInfo.profile_wire                      = myWire;
  anInfo.height                            = 6.0;
  anInfo.angle                             = OCCTL_PI_OVER_TWO;
  anInfo.section_count                     = 13;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, OCCTL_NODE_ID_INVALID.bits);

  occtl_node_kind_t aKind = OCCTL_KIND_VERTEX;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimTwistExtrusionTest, ExtrudeTwist_RectangleWire_ReturnsSolid)
{
  occtl_prim_extrude_twist_info_t anInfo = OCCTL_PRIM_EXTRUDE_TWIST_INFO_INIT;
  anInfo.profile_wire                    = myWire;
  anInfo.height                          = 6.0;
  anInfo.angle                           = OCCTL_PI_OVER_TWO;
  anInfo.section_count                   = 13;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_extrude_twist(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, OCCTL_NODE_ID_INVALID.bits);

  occtl_node_kind_t aKind = OCCTL_KIND_VERTEX;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimTwistExtrusionTest, ExtrudeTwist_InvalidOptions_ReturnInvalidArgument)
{
  occtl_prim_extrude_twist_info_t anInfo = OCCTL_PRIM_EXTRUDE_TWIST_INFO_INIT;
  anInfo.profile_wire                    = myWire;
  anInfo.height                          = 5.0;
  anInfo.p_next                          = &anInfo;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_extrude_twist(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.p_next = nullptr;
  anInfo.height = 0.0;
  EXPECT_EQ(occtl_prim_make_extrude_twist(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.height = 5.0;
  anInfo.angle  = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_prim_make_extrude_twist(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimTwistExtrusionTest, TwistExtrusion_ShellMode_ReturnsShell)
{
  occtl_prim_twist_extrusion_info_t anInfo = OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT;
  anInfo.profile_wire                      = myWire;
  anInfo.height                            = 5.0;
  anInfo.angle                             = OCCTL_PI_OVER_TWO;
  anInfo.make_solid                        = 0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, &aShape), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_VERTEX;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SHELL);
}

TEST_F(PrimTwistExtrusionTest, TwistExtrusion_NullPointers_ReturnInvalidArgument)
{
  occtl_prim_twist_extrusion_info_t anInfo = OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT;
  anInfo.profile_wire                      = myWire;
  occtl_node_id_t aShape                   = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_twist_extrusion(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_twist_extrusion(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimTwistExtrusionTest, TwistExtrusion_VersionMismatch_ReturnsVersionMismatch)
{
  occtl_prim_twist_extrusion_info_t anInfo = OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT;
  anInfo.struct_version                    = 0u;
  anInfo.profile_wire                      = myWire;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimTwistExtrusionTest, TwistExtrusion_InvalidOptions_ReturnInvalidArgument)
{
  occtl_prim_twist_extrusion_info_t anInfo = OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT;
  anInfo.profile_wire                      = myWire;
  anInfo.height                            = 0.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);

  anInfo.height        = 5.0;
  anInfo.section_count = 1;
  EXPECT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.section_count = 9;
  anInfo.pres3d        = 0.0;
  EXPECT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.pres3d = 1.0e-6;
  anInfo.angle  = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.angle      = 0.0;
  anInfo.make_solid = 2;
  EXPECT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimTwistExtrusionTest, TwistExtrusion_WrongKind_ReturnsWrongKind)
{
  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = myWire;
  occtl_node_id_t aFace                   = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &aFace), OCCTL_OK);

  occtl_prim_twist_extrusion_info_t anInfo = OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT;
  anInfo.profile_wire                      = aFace;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_twist_extrusion(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

TEST(PrimTwistExtrusionVeneerTest, MakeTwistExtrusion_RectangleWire_ReturnsShape)
{
  occtl::Graph aGraph;

  const occtl::NodeId           aWire  = occtl::prim::make_rectangle(aGraph, 4.0, 2.0);
  const occtl_axis1_placement_t anAxis = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
  const occtl::NodeId           aShape =
    occtl::prim::make_twist_extrusion(aGraph, aWire, anAxis, 6.0, OCCTL_PI_OVER_TWO);

  EXPECT_NE(aShape.get().bits, OCCTL_NODE_ID_INVALID.bits);
}

TEST(PrimTwistExtrusionVeneerTest, MakeExtrudeTwist_RectangleWire_ReturnsShape)
{
  occtl::Graph aGraph;

  const occtl::NodeId           aWire  = occtl::prim::make_rectangle(aGraph, 4.0, 2.0);
  const occtl_axis1_placement_t anAxis = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
  const occtl::NodeId           aShape =
    occtl::prim::make_extrude_twist(aGraph, aWire, anAxis, 6.0, OCCTL_PI_OVER_TWO);

  EXPECT_NE(aShape.get().bits, OCCTL_NODE_ID_INVALID.bits);
}

} // namespace
