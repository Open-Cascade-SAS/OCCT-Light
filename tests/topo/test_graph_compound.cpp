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

#include "test_helpers_internal.hxx"

namespace
{

class TopoCompoundTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);
    ASSERT_NE(myGraph, nullptr);
    loadBox(myGraph);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(TopoCompoundTest, CompoundNbChildren_Empty_ReturnsZero)
{
  occtl_topo_make_compound_info_t aInfo = OCCTL_TOPO_MAKE_COMPOUND_INFO_INIT;
  aInfo.children                        = nullptr;
  aInfo.child_count                     = 0;

  occtl_node_id_t aCompound;
  ASSERT_EQ(occtl_topo_make_compound(myGraph, &aInfo, &aCompound), OCCTL_OK);

  uint32_t aCount = 999;
  EXPECT_EQ(occtl_topo_compound_child_count(myGraph, aCompound, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 0u);
}

TEST_F(TopoCompoundTest, CompoundNbChildren_WithChild_ReturnsOne)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_oriented_node_t           aChildren[1] = {{aFaceId, OCCTL_ORIENTATION_FORWARD}};
  occtl_topo_make_compound_info_t aInfo        = OCCTL_TOPO_MAKE_COMPOUND_INFO_INIT;
  aInfo.children                               = aChildren;
  aInfo.child_count                            = 1;

  occtl_node_id_t aCompound;
  ASSERT_EQ(occtl_topo_make_compound(myGraph, &aInfo, &aCompound), OCCTL_OK);

  uint32_t aCount = 0;
  EXPECT_EQ(occtl_topo_compound_child_count(myGraph, aCompound, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);
}

TEST_F(TopoCompoundTest, CompoundNbChildren_NotFound)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_topo_compound_child_count(myGraph, OCCTL_NODE_ID_INVALID, &aCount),
            OCCTL_NOT_FOUND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoCompoundTest, CompoundNbChildren_WrongKind)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  uint32_t aCount = 0;
  EXPECT_EQ(occtl_topo_compound_child_count(myGraph, aSolidId, &aCount), OCCTL_WRONG_KIND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoCompoundTest, CompoundNbChildren_NullOut)
{
  occtl_topo_make_compound_info_t aInfo = OCCTL_TOPO_MAKE_COMPOUND_INFO_INIT;
  aInfo.children                        = nullptr;
  aInfo.child_count                     = 0;
  occtl_node_id_t aCompound;
  ASSERT_EQ(occtl_topo_make_compound(myGraph, &aInfo, &aCompound), OCCTL_OK);

  EXPECT_EQ(occtl_topo_compound_child_count(myGraph, aCompound, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

} // anonymous namespace
