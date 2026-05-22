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

class PrimHalfspaceTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    // Build a small box to harvest a face for use as the half-space boundary.
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

TEST_F(PrimHalfspaceTest, MakeHalfspace_FaceFromBox_CreatesSolid)
{
  occtl_prim_halfspace_info_t anInfo = OCCTL_PRIM_HALFSPACE_INFO_INIT;
  anInfo.face                        = myFace;
  anInfo.reference_point             = {0.5, 0.5, 5.0}; // well off the face

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_halfspace(myGraph, &anInfo, &aSolid), OCCTL_OK);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aSolid, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimHalfspaceTest, MakeHalfspace_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_halfspace_info_t anInfo = OCCTL_PRIM_HALFSPACE_INFO_INIT;
  anInfo.face                        = myFace;
  anInfo.reference_point             = {0.5, 0.5, 5.0};
  occtl_node_id_t aSolid             = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_halfspace(nullptr, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_halfspace(myGraph, nullptr, &aSolid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_halfspace(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimHalfspaceTest, MakeHalfspace_VersionMismatch_Rejected)
{
  occtl_prim_halfspace_info_t anInfo = OCCTL_PRIM_HALFSPACE_INFO_INIT;
  anInfo.struct_version              = 99u;
  anInfo.face                        = myFace;
  anInfo.reference_point             = {0.5, 0.5, 5.0};

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_halfspace(myGraph, &anInfo, &aSolid), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimHalfspaceTest, MakeHalfspace_NonFaceNode_WrongKind)
{
  // Use the box solid as the face — should be rejected with WRONG_KIND.
  occtl_prim_halfspace_info_t anInfo = OCCTL_PRIM_HALFSPACE_INFO_INIT;
  anInfo.face                        = myBoxSolid;
  anInfo.reference_point             = {0.5, 0.5, 5.0};

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_halfspace(myGraph, &anInfo, &aSolid), OCCTL_WRONG_KIND);
}

TEST_F(PrimHalfspaceTest, MakeHalfspace_InvalidFaceNode_NotFound)
{
  occtl_prim_halfspace_info_t anInfo = OCCTL_PRIM_HALFSPACE_INFO_INIT;
  anInfo.face                        = OCCTL_NODE_ID_INVALID;
  anInfo.reference_point             = {0.5, 0.5, 5.0};

  occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_halfspace(myGraph, &anInfo, &aSolid), OCCTL_NOT_FOUND);
}

TEST_F(PrimHalfspaceTest, MakeHalfspace_InvalidInfoFields_ReturnInvalidArgument)
{
  occtl_prim_halfspace_info_t anInfo = OCCTL_PRIM_HALFSPACE_INFO_INIT;
  anInfo.face                        = myFace;
  anInfo.reference_point             = {0.5, 0.5, 5.0};
  occtl_node_id_t aSolid             = OCCTL_NODE_ID_INVALID;

  int aTag      = 0;
  anInfo.p_next = &aTag;
  EXPECT_EQ(occtl_prim_make_halfspace(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);

  anInfo.p_next            = nullptr;
  anInfo.reference_point.y = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_prim_make_halfspace(myGraph, &anInfo, &aSolid), OCCTL_INVALID_ARGUMENT);
}

} // namespace
