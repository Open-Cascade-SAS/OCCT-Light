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

class PrimOffsetTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    // A unit box for the basis of every test in this suite.
    occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
    aBox.dx                    = 10.0;
    aBox.dy                    = 10.0;
    aBox.dz                    = 10.0;
    ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &myBox), OCCTL_OK);

    // Pick the first face we find — used as a closing face.
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

  occtl_graph_t*  myGraph = nullptr;
  occtl_node_id_t myBox   = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t myFace  = OCCTL_NODE_ID_INVALID;
};

// ---- offset_shape ----

TEST_F(PrimOffsetTest, OffsetShape_FacePositiveOffset_CreatesShape)
{
  occtl_prim_offset_shape_info_t anInfo = OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT;
  anInfo.shape                          = myFace;
  anInfo.offset                         = 0.5;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_offset_shape(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimOffsetTest, OffsetShape_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_offset_shape_info_t anInfo = OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT;
  anInfo.shape                          = myFace;
  anInfo.offset                         = 0.5;
  occtl_node_id_t aShape                = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_offset_shape(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_offset_shape(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_offset_shape(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimOffsetTest, OffsetShape_VersionMismatch_Rejected)
{
  occtl_prim_offset_shape_info_t anInfo = OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT;
  anInfo.struct_version                 = 0u;
  anInfo.shape                          = myFace;
  anInfo.offset                         = 0.5;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_offset_shape(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimOffsetTest, OffsetShape_NonNullPNext_ReturnsInvalidArgument)
{
  int                            aTag   = 0;
  occtl_prim_offset_shape_info_t anInfo = OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT;
  anInfo.p_next                         = &aTag;
  anInfo.shape                          = myFace;
  anInfo.offset                         = 0.5;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_offset_shape(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimOffsetTest, OffsetShape_NanOffset_ReturnsInvalidArgument)
{
  occtl_prim_offset_shape_info_t anInfo = OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT;
  anInfo.shape                          = myFace;
  anInfo.offset                         = std::numeric_limits<double>::quiet_NaN();

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_offset_shape(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimOffsetTest, OffsetShape_BadBoolean_ReturnsInvalidArgument)
{
  occtl_prim_offset_shape_info_t anInfo = OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT;
  anInfo.shape                          = myFace;
  anInfo.offset                         = 0.5;
  anInfo.intersection                   = 2;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_offset_shape(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimOffsetTest, OffsetShape_InvalidShape_NotFound)
{
  occtl_prim_offset_shape_info_t anInfo = OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT;
  anInfo.shape                          = OCCTL_NODE_ID_INVALID;
  anInfo.offset                         = 0.5;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_offset_shape(myGraph, &anInfo, &aShape), OCCTL_NOT_FOUND);
}

// ---- thick_solid ----

TEST_F(PrimOffsetTest, ThickSolid_HollowsBox_CreatesSolid)
{
  occtl_prim_thick_solid_info_t anInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  anInfo.solid                         = myBox;
  anInfo.closing_faces                 = &myFace;
  anInfo.closing_face_count            = 1;
  anInfo.offset                        = -1.0; // hollow inward
  anInfo.intersection                  = 1;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_thick_solid(myGraph, &anInfo, &aResult), OCCTL_OK);
  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aResult, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimOffsetTest, ThickSolid_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_thick_solid_info_t anInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  anInfo.solid                         = myBox;
  anInfo.closing_faces                 = &myFace;
  anInfo.closing_face_count            = 1;
  anInfo.offset                        = -1.0;
  occtl_node_id_t aResult              = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_thick_solid(nullptr, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_thick_solid(myGraph, nullptr, &aResult), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_thick_solid(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimOffsetTest, ThickSolid_VersionMismatch_Rejected)
{
  occtl_prim_thick_solid_info_t anInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  anInfo.struct_version                = 0u;
  anInfo.solid                         = myBox;
  anInfo.offset                        = -1.0;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_thick_solid(myGraph, &anInfo, &aResult), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimOffsetTest, ThickSolid_NonNullPNext_ReturnsInvalidArgument)
{
  int                           aTag   = 0;
  occtl_prim_thick_solid_info_t anInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  anInfo.p_next                        = &aTag;
  anInfo.solid                         = myBox;
  anInfo.offset                        = -1.0;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_thick_solid(myGraph, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimOffsetTest, ThickSolid_NanTolerance_ReturnsInvalidArgument)
{
  occtl_prim_thick_solid_info_t anInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  anInfo.solid                         = myBox;
  anInfo.offset                        = -1.0;
  anInfo.tolerance                     = std::numeric_limits<double>::quiet_NaN();

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_thick_solid(myGraph, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimOffsetTest, ThickSolid_BadBoolean_ReturnsInvalidArgument)
{
  occtl_prim_thick_solid_info_t anInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  anInfo.solid                         = myBox;
  anInfo.offset                        = -1.0;
  anInfo.remove_internal_edges         = 7;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_thick_solid(myGraph, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimOffsetTest, ThickSolid_NonSolidInput_WrongKind)
{
  occtl_prim_thick_solid_info_t anInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  anInfo.solid                         = myFace; // a Face, not a Solid
  anInfo.offset                        = -1.0;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_thick_solid(myGraph, &anInfo, &aResult), OCCTL_WRONG_KIND);
}

TEST_F(PrimOffsetTest, ThickSolid_NullClosingArrayWithCount_InvalidArgument)
{
  occtl_prim_thick_solid_info_t anInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  anInfo.solid                         = myBox;
  anInfo.closing_faces                 = nullptr;
  anInfo.closing_face_count            = 1;
  anInfo.offset                        = -1.0;

  occtl_node_id_t aResult = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_thick_solid(myGraph, &anInfo, &aResult), OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

} // namespace
