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

class PrimFeatPrismTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    // 10x10x10 base box on which to add / cut features.
    occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
    aBox.dx                    = 10.0;
    aBox.dy                    = 10.0;
    aBox.dz                    = 10.0;
    ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &myBox), OCCTL_OK);

    // Build a small rectangle sketch wire on the box's top face.
    occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
    aRect.placement.location          = {5.0, 5.0, 10.0}; // centred on top face
    aRect.width                       = 2.0;
    aRect.height                      = 2.0;
    occtl_node_id_t aRectWire         = OCCTL_NODE_ID_INVALID;
    ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aRectWire), OCCTL_OK);
    occtl_prim_planar_face_info_t aFi = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
    aFi.outer_wire                    = aRectWire;
    ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFi, &myProfile), OCCTL_OK);

    // Pick the first face of the box as the sketch_face host.  These tests
    // validate the wiring/error matrix rather than feature-on-base correctness.
    occtl_node_iter_t* anIter = nullptr;
    ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &anIter), OCCTL_OK);
    ASSERT_EQ(occtl_node_iter_next(anIter, &mySketchFace), OCCTL_OK);
    occtl_node_iter_free(anIter);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t*  myGraph      = nullptr;
  occtl_node_id_t myBox        = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t myProfile    = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t mySketchFace = OCCTL_NODE_ID_INVALID;
};

TEST_F(PrimFeatPrismTest, FeatPrism_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  occtl_node_id_t              aShape = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_feat_prism(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimFeatPrismTest, FeatPrism_VersionMismatch_Rejected)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  anInfo.struct_version               = 0u;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimFeatPrismTest, FeatPrism_ZeroDirection_InvalidArgument)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  anInfo.base_shape                   = myBox;
  anInfo.profile                      = myProfile;
  anInfo.sketch_face                  = mySketchFace;
  anInfo.direction                    = {0.0, 0.0, 0.0};
  anInfo.until_kind                   = OCCTL_UNTIL_LENGTH;
  anInfo.length                       = 5.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimFeatPrismTest, FeatPrism_ZeroLength_InvalidArgument)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  anInfo.base_shape                   = myBox;
  anInfo.profile                      = myProfile;
  anInfo.sketch_face                  = mySketchFace;
  anInfo.direction                    = {0.0, 0.0, 1.0};
  anInfo.until_kind                   = OCCTL_UNTIL_LENGTH;
  anInfo.length                       = 0.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimFeatPrismTest, FeatPrism_NanDirection_InvalidArgument)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  anInfo.base_shape                   = myBox;
  anInfo.profile                      = myProfile;
  anInfo.sketch_face                  = mySketchFace;
  anInfo.direction                    = {0.0, std::numeric_limits<double>::quiet_NaN(), 1.0};
  anInfo.until_kind                   = OCCTL_UNTIL_LENGTH;
  anInfo.length                       = 5.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimFeatPrismTest, FeatPrism_BadModifyBoolean_InvalidArgument)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  anInfo.base_shape                   = myBox;
  anInfo.profile                      = myProfile;
  anInfo.sketch_face                  = mySketchFace;
  anInfo.direction                    = {0.0, 0.0, 1.0};
  anInfo.until_kind                   = OCCTL_UNTIL_LENGTH;
  anInfo.length                       = 5.0;
  anInfo.modify                       = 2;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimFeatPrismTest, FeatPrism_NonFaceSketch_WrongKind)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  anInfo.base_shape                   = myBox;
  anInfo.profile                      = myProfile;
  anInfo.sketch_face                  = myBox; // a Solid, not a Face
  anInfo.direction                    = {0.0, 0.0, 1.0};
  anInfo.until_kind                   = OCCTL_UNTIL_LENGTH;
  anInfo.length                       = 5.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

TEST_F(PrimFeatPrismTest, FeatPrism_InvalidBase_NotFound)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  anInfo.base_shape                   = OCCTL_NODE_ID_INVALID;
  anInfo.profile                      = myProfile;
  anInfo.sketch_face                  = mySketchFace;
  anInfo.direction                    = {0.0, 0.0, 1.0};
  anInfo.until_kind                   = OCCTL_UNTIL_LENGTH;
  anInfo.length                       = 5.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, &aShape), OCCTL_NOT_FOUND);
}

TEST_F(PrimFeatPrismTest, FeatPrism_ShapeModeMissingUntil_NotFound)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  anInfo.base_shape                   = myBox;
  anInfo.profile                      = myProfile;
  anInfo.sketch_face                  = mySketchFace;
  anInfo.direction                    = {0.0, 0.0, 1.0};
  anInfo.until_kind                   = OCCTL_UNTIL_SHAPE;
  anInfo.until_shape                  = OCCTL_NODE_ID_INVALID;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, &aShape), OCCTL_NOT_FOUND);
}

TEST_F(PrimFeatPrismTest, FeatDraftPrismInfoInit_HasDefaults)
{
  occtl_prim_feat_draft_prism_info_t anInfo{};
  occtl_prim_feat_draft_prism_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_VERSION_1);
  EXPECT_EQ(anInfo.base_shape.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(anInfo.profile_face.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(anInfo.sketch_face.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anInfo.taper_angle, 0.0);
  EXPECT_EQ(anInfo.combine, OCCTL_FEAT_FUSE);
  EXPECT_EQ(anInfo.modify, 1);
  EXPECT_EQ(anInfo.until_kind, OCCTL_UNTIL_LENGTH);
}

TEST_F(PrimFeatPrismTest, FeatDraftPrism_LengthMode_ReturnsSolid)
{
  occtl_prim_feat_draft_prism_info_t anInfo = OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_INIT;
  anInfo.base_shape                         = myBox;
  anInfo.profile_face                       = mySketchFace;
  anInfo.sketch_face                        = mySketchFace;
  anInfo.taper_angle                        = 0.05;
  anInfo.until_kind                         = OCCTL_UNTIL_LENGTH;
  anInfo.length                             = 1.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_feat_draft_prism(myGraph, &anInfo, &aShape), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_TRUE(aKind == OCCTL_KIND_SOLID || aKind == OCCTL_KIND_COMPOUND);
}

TEST_F(PrimFeatPrismTest, FeatDraftPrismWithHistory_LengthMode_ReturnsHistory)
{
  occtl_uid_t aProfileUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(myGraph, mySketchFace, &aProfileUid), OCCTL_OK);

  occtl_prim_feat_draft_prism_info_t anInfo = OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_INIT;
  anInfo.base_shape                         = myBox;
  anInfo.profile_face                       = mySketchFace;
  anInfo.sketch_face                        = mySketchFace;
  anInfo.taper_angle                        = 0.05;
  anInfo.until_kind                         = OCCTL_UNTIL_LENGTH;
  anInfo.length                             = 1.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_feat_draft_prism(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);

  size_t aModifiedCount  = 0;
  size_t aGeneratedCount = 0;
  size_t aDeletedCount   = 0;
  EXPECT_EQ(occtl_graph_history_modified(myGraph, aProfileUid, nullptr, 0, &aModifiedCount),
            OCCTL_OK);
  EXPECT_EQ(aModifiedCount, 0u);
  EXPECT_EQ(occtl_graph_history_generated(myGraph, aProfileUid, nullptr, 0, &aGeneratedCount),
            OCCTL_OK);
  EXPECT_EQ(aGeneratedCount, 0u);
  EXPECT_EQ(occtl_graph_history_deleted_all(myGraph, nullptr, 0, &aDeletedCount), OCCTL_OK);
}

TEST_F(PrimFeatPrismTest, FeatPrism_MissingBase_ReturnsNotFound)
{
  occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  occtl_node_id_t              aShape = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_feat_prism(myGraph, &anInfo, &aShape), OCCTL_NOT_FOUND);
}

TEST_F(PrimFeatPrismTest, ExtrudeUntilInfoInit_HasDefaults)
{
  occtl_prim_extrude_until_info_t anInfo{};
  occtl_prim_extrude_until_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_EXTRUDE_UNTIL_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.base_shape.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(anInfo.profile.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(anInfo.sketch_face.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(anInfo.target_shape.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anInfo.direction.x, 0.0);
  EXPECT_DOUBLE_EQ(anInfo.direction.y, 0.0);
  EXPECT_DOUBLE_EQ(anInfo.direction.z, 1.0);
  EXPECT_EQ(anInfo.side, OCCTL_EXTRUDE_UNTIL_NEXT);
  EXPECT_EQ(anInfo.combine, OCCTL_FEAT_FUSE);
  EXPECT_EQ(anInfo.modify, 1);
  EXPECT_DOUBLE_EQ(anInfo.limit, 0.0);
}

TEST_F(PrimFeatPrismTest, ExtrudeUntil_InvalidOptions_ReturnInvalidArgument)
{
  occtl_prim_extrude_until_info_t anInfo = OCCTL_PRIM_EXTRUDE_UNTIL_INFO_INIT;
  anInfo.base_shape                      = myBox;
  anInfo.profile                         = mySketchFace;
  anInfo.sketch_face                     = mySketchFace;
  anInfo.target_shape                    = myBox;
  anInfo.limit                           = -1.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_extrude_until(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.limit = 0.0;
  anInfo.side  = OCCTL_EXTRUDE_UNTIL_RESERVED_FUTURE;
  EXPECT_EQ(occtl_prim_make_extrude_until(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.side  = OCCTL_EXTRUDE_UNTIL_NEXT;
  anInfo.limit = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_prim_make_extrude_until(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.limit  = 0.0;
  anInfo.modify = 2;
  EXPECT_EQ(occtl_prim_make_extrude_until(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimFeatPrismTest, ExtrudeUntil_NextWithLimit_ReturnsShape)
{
  occtl_prim_extrude_until_info_t anInfo = OCCTL_PRIM_EXTRUDE_UNTIL_INFO_INIT;
  anInfo.base_shape                      = myBox;
  anInfo.profile                         = mySketchFace;
  anInfo.sketch_face                     = mySketchFace;
  anInfo.target_shape                    = myBox;
  anInfo.direction                       = {0.0, 0.0, 1.0};
  anInfo.side                            = OCCTL_EXTRUDE_UNTIL_NEXT;
  anInfo.limit                           = 1.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_extrude_until(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimFeatPrismTest, FeatDraftPrism_InvalidOptions_ReturnInvalidArgument)
{
  occtl_prim_feat_draft_prism_info_t anInfo = OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_INIT;
  anInfo.base_shape                         = myBox;
  anInfo.profile_face                       = mySketchFace;
  anInfo.sketch_face                        = mySketchFace;
  anInfo.length                             = 1.0;
  anInfo.p_next                             = &anInfo;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_draft_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.p_next  = nullptr;
  anInfo.combine = OCCTL_FEAT_COMBINE_RESERVED_FUTURE;
  EXPECT_EQ(occtl_prim_make_feat_draft_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.combine = OCCTL_FEAT_FUSE;
  anInfo.length  = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_prim_make_feat_draft_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo.length = 1.0;
  anInfo.modify = 2;
  EXPECT_EQ(occtl_prim_make_feat_draft_prism(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimFeatPrismTest, FeatDraftPrism_NonFaceProfile_WrongKind)
{
  occtl_prim_feat_draft_prism_info_t anInfo = OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_INIT;
  anInfo.base_shape                         = myBox;
  anInfo.profile_face                       = myBox;
  anInfo.sketch_face                        = mySketchFace;
  anInfo.length                             = 1.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_feat_draft_prism(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

TEST_F(PrimFeatPrismTest, FeatDraftPrismVeneer_LengthMode_ReturnsSolid)
{
  occtl::Graph aGraph;

  const occtl::NodeId aBoxId = occtl::prim::make_box(aGraph, 10.0, 10.0, 10.0);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(aGraph.get(), &anIter), OCCTL_OK);
  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_node_iter_next(anIter, &aFace), OCCTL_OK);
  occtl_node_iter_free(anIter);

  occtl::prim::FeatDraftPrismOptions anOpts;
  anOpts.length                  = 1.0;
  const occtl::NodeId     aDraft = occtl::prim::make_feat_draft_prism(aGraph,
                                                                      aBoxId,
                                                                      occtl::NodeId(aFace),
                                                                      occtl::NodeId(aFace),
                                                                      0.05,
                                                                      anOpts);
  const occtl_node_kind_t aKind  = aGraph.node_id_kind(aDraft);
  EXPECT_TRUE(aKind == OCCTL_KIND_SOLID || aKind == OCCTL_KIND_COMPOUND);
}

TEST_F(PrimFeatPrismTest, ExtrudeUntilVeneer_NextWithLimit_ReturnsShape)
{
  occtl::Graph aGraph;

  const occtl::NodeId aBoxId = occtl::prim::make_box(aGraph, 10.0, 10.0, 10.0);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(aGraph.get(), &anIter), OCCTL_OK);
  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_node_iter_next(anIter, &aFace), OCCTL_OK);
  occtl_node_iter_free(anIter);

  occtl::prim::ExtrudeUntilOptions anOpts;
  anOpts.limit                  = 1.0;
  const occtl::NodeId aExtruded = occtl::prim::make_extrude_until(aGraph,
                                                                  aBoxId,
                                                                  occtl::NodeId(aFace),
                                                                  occtl::NodeId(aFace),
                                                                  aBoxId,
                                                                  {0.0, 0.0, 1.0},
                                                                  anOpts);
  EXPECT_TRUE(aExtruded.is_valid());
}

TEST_F(PrimFeatPrismTest, FeatDraftPrismVeneerWithHistory_LengthMode_ReturnsHistory)
{
  occtl::Graph aGraph;

  const occtl::NodeId aBoxId = occtl::prim::make_box(aGraph, 10.0, 10.0, 10.0);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(aGraph.get(), &anIter), OCCTL_OK);
  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_node_iter_next(anIter, &aFace), OCCTL_OK);
  occtl_node_iter_free(anIter);

  const occtl::UID aFaceUid = aGraph.uid_from_node_id(occtl::NodeId(aFace));

  occtl::prim::FeatDraftPrismOptions anOpts;
  anOpts.length = 1.0;
  auto aDraft   = occtl::prim::make_feat_draft_prism(aGraph,
                                                     aBoxId,
                                                     occtl::NodeId(aFace),
                                                     occtl::NodeId(aFace),
                                                     0.05,
                                                     anOpts);
  EXPECT_TRUE(aDraft.is_valid());
  EXPECT_TRUE(aGraph.history_modified(aFaceUid).empty());
  EXPECT_TRUE(aGraph.history_generated(aFaceUid).empty());
  EXPECT_NO_THROW((void)aGraph.history_deleted_all());
}

} // namespace
