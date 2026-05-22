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
#include <limits>

namespace
{

class PrimConvenienceTest : public ::testing::Test
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

// ---- plane ----

TEST_F(PrimConvenienceTest, Plane_Axisaligned_CreatesFace)
{
  occtl_point3_t          aCenter = {0.0, 0.0, 0.0};
  occtl_direction3_t      aNormal = {0.0, 0.0, 1.0};
  occtl_axis2_placement_t aPlace  = {aCenter, aNormal, {1.0, 0.0, 0.0}};

  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.placement                   = aPlace;
  aRect.width                       = 4.0;
  aRect.height                      = 2.0;
  occtl_node_id_t aRectWire         = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aRectWire), OCCTL_OK);

  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = aRectWire;
  occtl_node_id_t aFace                   = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &aFace), OCCTL_OK);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(PrimConvenienceTest, Plane_NegativeWidth_GeometryInvalid)
{
  occtl_point3_t          aCenter = {0.0, 0.0, 0.0};
  occtl_direction3_t      aNormal = {0.0, 0.0, 1.0};
  occtl_axis2_placement_t aPlace  = {aCenter, aNormal, {1.0, 0.0, 0.0}};

  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.placement                   = aPlace;
  aRect.width                       = -1.0;
  aRect.height                      = 1.0;
  occtl_node_id_t aRectWire         = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aRectWire), OCCTL_GEOMETRY_INVALID);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimConvenienceTest, Plane_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 1.0;
  aRect.height                      = 1.0;
  occtl_node_id_t aWire             = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_rectangle(nullptr, &aRect, &aWire), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_rectangle(myGraph, nullptr, &aWire), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimConvenienceTest, Plane_VersionMismatch_Rejected)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.struct_version              = 0u;
  aRect.width                       = 1.0;
  aRect.height                      = 1.0;
  occtl_node_id_t aWire             = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aWire), OCCTL_VERSION_MISMATCH);
}

// ---- disk ----

TEST_F(PrimConvenienceTest, Disk_PositiveRadius_CreatesFace)
{
  occtl_geom_circle_t aCircInfo;
  aCircInfo.position    = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  aCircInfo.radius      = 3.0;
  occtl_rep_id_t aCurve = {};
  ASSERT_EQ(occtl_curve_create_circle(myGraph, aCircInfo, &aCurve), OCCTL_OK);

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_curves_to_wire(myGraph, &aCurve, 1, &aWire), OCCTL_OK);

  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = aWire;
  occtl_node_id_t aFace                   = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &aFace), OCCTL_OK);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 2u);
}

TEST_F(PrimConvenienceTest, Disk_NegativeRadius_GeometryInvalid)
{
  occtl_geom_circle_t aCircInfo;
  aCircInfo.position    = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  aCircInfo.radius      = -1.0;
  occtl_rep_id_t aCurve = {};
  EXPECT_EQ(occtl_curve_create_circle(myGraph, aCircInfo, &aCurve), OCCTL_GEOMETRY_INVALID);
}

TEST_F(PrimConvenienceTest, Disk_NullPointers_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_curve_create_circle(myGraph, {}, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimConvenienceTest, Disk_VersionMismatch_Rejected)
{
  occtl_geom_circle_t aCircInfo;
  aCircInfo.position    = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  aCircInfo.radius      = 1.0;
  occtl_rep_id_t aCurve = {};
  ASSERT_EQ(occtl_curve_create_circle(myGraph, aCircInfo, &aCurve), OCCTL_OK);
}

// ---- slot ----

TEST_F(PrimConvenienceTest, Slot_Stadium_CreatesWire)
{
  occtl_prim_slot_info_t anInfo = OCCTL_PRIM_SLOT_INFO_INIT;
  anInfo.length                 = 10.0;
  anInfo.width                  = 4.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_slot(myGraph, &anInfo, &aWire), OCCTL_OK);
  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aWire, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_WIRE);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 4u); // 2 lines + 2 arcs
}

TEST_F(PrimConvenienceTest, Slot_LengthEqualWidth_GeometryInvalid)
{
  occtl_prim_slot_info_t anInfo = OCCTL_PRIM_SLOT_INFO_INIT;
  anInfo.length                 = 4.0;
  anInfo.width                  = 4.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_slot(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimConvenienceTest, Slot_NegativeWidth_GeometryInvalid)
{
  occtl_prim_slot_info_t anInfo = OCCTL_PRIM_SLOT_INFO_INIT;
  anInfo.length                 = 10.0;
  anInfo.width                  = -1.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_slot(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimConvenienceTest, Slot_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_slot_info_t anInfo = OCCTL_PRIM_SLOT_INFO_INIT;
  anInfo.length                 = 10.0;
  anInfo.width                  = 4.0;
  occtl_node_id_t aWire         = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_slot(nullptr, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_slot(myGraph, nullptr, &aWire), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_slot(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimConvenienceTest, Slot_VersionMismatch_Rejected)
{
  occtl_prim_slot_info_t anInfo = OCCTL_PRIM_SLOT_INFO_INIT;
  anInfo.struct_version         = 0u;
  anInfo.length                 = 10.0;
  anInfo.width                  = 4.0;
  occtl_node_id_t aWire         = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_slot(myGraph, &anInfo, &aWire), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimConvenienceTest, Slot_NonNullPNext_ReturnsInvalidArgument)
{
  int                    aTag   = 0;
  occtl_prim_slot_info_t anInfo = OCCTL_PRIM_SLOT_INFO_INIT;
  anInfo.p_next                 = &aTag;
  anInfo.length                 = 10.0;
  anInfo.width                  = 4.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_slot(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimConvenienceTest, Slot_NanLength_ReturnsInvalidArgument)
{
  occtl_prim_slot_info_t anInfo = OCCTL_PRIM_SLOT_INFO_INIT;
  anInfo.length                 = std::numeric_limits<double>::quiet_NaN();
  anInfo.width                  = 4.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_slot(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

// ---- tube ----

TEST_F(PrimConvenienceTest, Tube_NominalDimensions_CreatesSolid)
{
  occtl_prim_tube_info_t anInfo = OCCTL_PRIM_TUBE_INFO_INIT;
  anInfo.outer_radius           = 5.0;
  anInfo.inner_radius           = 3.0;
  anInfo.height                 = 10.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_tube(myGraph, &anInfo, &aSolid), OCCTL_OK);
  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSolid, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_SOLID), 1u);
}

TEST_F(PrimConvenienceTest, Tube_InnerEqualOuter_GeometryInvalid)
{
  occtl_prim_tube_info_t anInfo = OCCTL_PRIM_TUBE_INFO_INIT;
  anInfo.outer_radius           = 5.0;
  anInfo.inner_radius           = 5.0;
  anInfo.height                 = 10.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_tube(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimConvenienceTest, Tube_ZeroInnerRadius_GeometryInvalid)
{
  occtl_prim_tube_info_t anInfo = OCCTL_PRIM_TUBE_INFO_INIT;
  anInfo.outer_radius           = 5.0;
  anInfo.inner_radius           = 0.0;
  anInfo.height                 = 10.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_tube(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimConvenienceTest, Tube_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_tube_info_t anInfo = OCCTL_PRIM_TUBE_INFO_INIT;
  anInfo.outer_radius           = 5.0;
  anInfo.inner_radius           = 3.0;
  anInfo.height                 = 10.0;
  occtl_node_id_t aSolid        = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_tube(nullptr, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_tube(myGraph, nullptr, &aSolid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_tube(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimConvenienceTest, Tube_VersionMismatch_Rejected)
{
  occtl_prim_tube_info_t anInfo = OCCTL_PRIM_TUBE_INFO_INIT;
  anInfo.struct_version         = 0u;
  anInfo.outer_radius           = 5.0;
  anInfo.inner_radius           = 3.0;
  anInfo.height                 = 10.0;
  occtl_node_id_t aSolid        = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_tube(myGraph, &anInfo, &aSolid), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimConvenienceTest, Tube_NonNullPNext_ReturnsInvalidArgument)
{
  int                    aTag   = 0;
  occtl_prim_tube_info_t anInfo = OCCTL_PRIM_TUBE_INFO_INIT;
  anInfo.p_next                 = &aTag;
  anInfo.outer_radius           = 5.0;
  anInfo.inner_radius           = 3.0;
  anInfo.height                 = 10.0;

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_tube(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimConvenienceTest, Tube_NanHeight_ReturnsInvalidArgument)
{
  occtl_prim_tube_info_t anInfo = OCCTL_PRIM_TUBE_INFO_INIT;
  anInfo.outer_radius           = 5.0;
  anInfo.inner_radius           = 3.0;
  anInfo.height                 = std::numeric_limits<double>::quiet_NaN();

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_tube(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

} // namespace
