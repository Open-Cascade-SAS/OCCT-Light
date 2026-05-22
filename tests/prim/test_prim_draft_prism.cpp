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

class PrimDraftPrismTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
    aRect.width                       = 4.0;
    aRect.height                      = 3.0;
    occtl_node_id_t aWire             = OCCTL_NODE_ID_INVALID;
    ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aWire), OCCTL_OK);

    occtl_prim_planar_face_info_t aFace = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
    aFace.outer_wire                    = aWire;
    ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFace, &myFace), OCCTL_OK);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t*  myGraph = nullptr;
  occtl_node_id_t myFace  = OCCTL_NODE_ID_INVALID;
};

TEST_F(PrimDraftPrismTest, DraftPrismInfoInit_HasDefaults)
{
  occtl_prim_draft_prism_info_t anInfo{};
  occtl_prim_draft_prism_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_DRAFT_PRISM_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.profile.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anInfo.height, 1.0);
  EXPECT_DOUBLE_EQ(anInfo.taper_angle, 0.0);
}

TEST_F(PrimDraftPrismTest, ExtrudeTaperedInfoInit_HasDefaults)
{
  occtl_prim_extrude_tapered_info_t anInfo{};
  occtl_prim_extrude_tapered_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_EXTRUDE_TAPERED_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.profile_face.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anInfo.height, 1.0);
  EXPECT_DOUBLE_EQ(anInfo.taper_angle, 0.0);
}

TEST_F(PrimDraftPrismTest, DraftPrism_RectangleFace_ReturnsSolid)
{
  occtl_prim_draft_prism_info_t anInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
  anInfo.profile                       = myFace;
  anInfo.height                        = 5.0;
  anInfo.taper_angle                   = 0.1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_draft_prism(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, OCCTL_NODE_ID_INVALID.bits);

  occtl_node_kind_t aKind = OCCTL_KIND_VERTEX;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimDraftPrismTest, ExtrudeTapered_RectangleFace_ReturnsSolid)
{
  occtl_prim_extrude_tapered_info_t anInfo = OCCTL_PRIM_EXTRUDE_TAPERED_INFO_INIT;
  anInfo.profile_face                      = myFace;
  anInfo.height                            = 5.0;
  anInfo.taper_angle                       = 0.1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_extrude_tapered(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, OCCTL_NODE_ID_INVALID.bits);

  occtl_node_kind_t aKind = OCCTL_KIND_VERTEX;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimDraftPrismTest, ExtrudeTapered_InvalidOptions_ReturnInvalidArgument)
{
  occtl_prim_extrude_tapered_info_t anInfo = OCCTL_PRIM_EXTRUDE_TAPERED_INFO_INIT;
  anInfo.profile_face                      = myFace;
  anInfo.height                            = 5.0;
  anInfo.p_next                            = &anInfo;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_extrude_tapered(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.p_next = nullptr;
  anInfo.height = 0.0;
  EXPECT_EQ(occtl_prim_make_extrude_tapered(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.height      = 5.0;
  anInfo.taper_angle = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_prim_make_extrude_tapered(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimDraftPrismTest, DraftPrism_NullPointers_ReturnInvalidArgument)
{
  occtl_prim_draft_prism_info_t anInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
  occtl_node_id_t               aShape = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_draft_prism(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_draft_prism(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_draft_prism(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimDraftPrismTest, DraftPrism_VersionMismatch_ReturnsVersionMismatch)
{
  occtl_prim_draft_prism_info_t anInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
  anInfo.struct_version                = 0u;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_draft_prism(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimDraftPrismTest, DraftPrism_NonNullPNext_ReturnsInvalidArgument)
{
  occtl_prim_draft_prism_info_t anInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
  anInfo.profile                       = myFace;
  anInfo.p_next                        = &anInfo;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_draft_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimDraftPrismTest, DraftPrism_NanTaperAngle_ReturnsInvalidArgument)
{
  occtl_prim_draft_prism_info_t anInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
  anInfo.profile                       = myFace;
  anInfo.taper_angle                   = std::numeric_limits<double>::quiet_NaN();

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_draft_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimDraftPrismTest, DraftPrism_NonPositiveHeight_ReturnsInvalidArgument)
{
  occtl_prim_draft_prism_info_t anInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
  anInfo.profile                       = myFace;
  anInfo.height                        = 0.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_draft_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimDraftPrismTest, DraftPrism_WrongKind_ReturnsWrongKind)
{
  occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
  aBox.dx                    = 2.0;
  aBox.dy                    = 2.0;
  aBox.dz                    = 2.0;
  occtl_node_id_t aSolid     = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &aSolid), OCCTL_OK);

  occtl_prim_draft_prism_info_t anInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
  anInfo.profile                       = aSolid;
  anInfo.height                        = 5.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_draft_prism(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

TEST(PrimDraftPrismVeneerTest, MakeDraftPrism_RectangleFace_ReturnsSolid)
{
  occtl::Graph aGraph;

  const occtl::NodeId aWire  = occtl::prim::make_rectangle(aGraph, 4.0, 3.0);
  const occtl::NodeId aFace  = occtl::prim::make_planar_face(aGraph, aWire);
  const occtl::NodeId aSolid = occtl::prim::make_draft_prism(aGraph, aFace, 5.0, 0.1);

  EXPECT_NE(aSolid.get().bits, OCCTL_NODE_ID_INVALID.bits);
}

TEST(PrimDraftPrismVeneerTest, MakeExtrudeTapered_RectangleFace_ReturnsSolid)
{
  occtl::Graph aGraph;

  const occtl::NodeId aWire  = occtl::prim::make_rectangle(aGraph, 4.0, 3.0);
  const occtl::NodeId aFace  = occtl::prim::make_planar_face(aGraph, aWire);
  const occtl::NodeId aSolid = occtl::prim::make_extrude_tapered(aGraph, aFace, 5.0, 0.1);

  EXPECT_NE(aSolid.get().bits, OCCTL_NODE_ID_INVALID.bits);
}

} // namespace
