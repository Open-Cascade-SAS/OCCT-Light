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

class PrimCylindricalHoleTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
    aBox.dx                    = 10.0;
    aBox.dy                    = 10.0;
    aBox.dz                    = 10.0;
    ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &myBox), OCCTL_OK);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_prim_cylindrical_hole_info_t baseInfo() const
  {
    occtl_prim_cylindrical_hole_info_t anInfo = OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_INIT;
    anInfo.base_shape                         = myBox;
    anInfo.axis.location                      = {5.0, 5.0, -1.0};
    anInfo.axis.direction                     = {0.0, 0.0, 1.0};
    anInfo.radius                             = 1.0;
    return anInfo;
  }

  occtl_graph_t*  myGraph = nullptr;
  occtl_node_id_t myBox   = OCCTL_NODE_ID_INVALID;
};

TEST_F(PrimCylindricalHoleTest, CylindricalHole_ThroughAllBox_CreatesSolidWithMoreFaces)
{
  const std::size_t aFacesBefore  = countOfKind(myGraph, OCCTL_KIND_FACE);
  const std::size_t aSolidsBefore = countOfKind(myGraph, OCCTL_KIND_SOLID);

  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  occtl_node_id_t                    aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
  EXPECT_GT(countOfKind(myGraph, OCCTL_KIND_SOLID), aSolidsBefore);
  EXPECT_GT(countOfKind(myGraph, OCCTL_KIND_FACE), aFacesBefore);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_BetweenParams_CreatesShape)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  anInfo.kind                               = OCCTL_CYLINDRICAL_HOLE_BETWEEN_PARAMS;
  anInfo.p_from                             = 0.0;
  anInfo.p_to                               = 12.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_NullPointers_ReturnInvalidArgument)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  occtl_node_id_t                    aShape = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_cylindrical_hole(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_VersionMismatch_Rejected)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  anInfo.struct_version                     = 0u;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_InvalidBase_NotFound)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  anInfo.base_shape                         = OCCTL_NODE_ID_INVALID;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_NOT_FOUND);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_NonPositiveRadius_InvalidArgument)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  anInfo.radius                             = 0.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_ZeroAxisDirection_InvalidArgument)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  anInfo.axis.direction                     = {0.0, 0.0, 0.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_InvalidBetweenParams_InvalidArgument)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  anInfo.kind                               = OCCTL_CYLINDRICAL_HOLE_BETWEEN_PARAMS;
  anInfo.p_from                             = 3.0;
  anInfo.p_to                               = 3.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_InvalidBlindLength_InvalidArgument)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  anInfo.kind                               = OCCTL_CYLINDRICAL_HOLE_BLIND;
  anInfo.length                             = 0.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_UnknownKind_InvalidArgument)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  anInfo.kind                               = static_cast<occtl_prim_cylindrical_hole_kind_t>(99);

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimCylindricalHoleTest, CylindricalHole_InvalidInfoFields_ReturnInvalidArgument)
{
  occtl_prim_cylindrical_hole_info_t anInfo = baseInfo();
  occtl_node_id_t                    aShape = OCCTL_NODE_ID_INVALID;

  int aTag      = 0;
  anInfo.p_next = &aTag;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo              = baseInfo();
  anInfo.with_control = 2;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo                 = baseInfo();
  anInfo.axis.location.x = std::numeric_limits<double>::infinity();
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo        = baseInfo();
  anInfo.kind   = OCCTL_CYLINDRICAL_HOLE_BETWEEN_PARAMS;
  anInfo.p_from = std::numeric_limits<double>::quiet_NaN();
  anInfo.p_to   = 12.0;
  EXPECT_EQ(occtl_prim_make_cylindrical_hole(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST(PrimCylindricalHoleVeneerTest, MakeCylindricalHole_ThroughAll_ReturnsSolid)
{
  occtl::Graph        aGraph;
  const occtl::NodeId aBox = occtl::prim::make_box(aGraph, 10.0, 10.0, 10.0);

  const occtl_axis1_placement_t anAxis = {{5.0, 5.0, -1.0}, {0.0, 0.0, 1.0}};
  const occtl::NodeId aCut = occtl::prim::make_cylindrical_hole(aGraph, aBox, anAxis, 1.0);

  EXPECT_TRUE(aCut.is_valid());
  EXPECT_EQ(aGraph.node_id_kind(aCut), OCCTL_KIND_SOLID);
}

} // namespace
