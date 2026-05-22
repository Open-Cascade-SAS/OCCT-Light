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

class TopoExplorerTest : public ::testing::Test
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

TEST_F(TopoExplorerTest, ChildExplorer_SolidToFace_YieldsFaces)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_topo_explorer_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_child_explorer_create(myGraph, aSolidId, nullptr, &anIter), OCCTL_OK);
  ASSERT_NE(anIter, nullptr);

  int                 aCount = 0;
  occtl_node_id_t     aNode;
  occtl_transform_t   aTrsf;
  occtl_orientation_t aOri;
  while (occtl_topo_explorer_iter_next(anIter, &aNode, &aTrsf, &aOri) == OCCTL_OK)
  {
    EXPECT_NE(aNode.bits, 0u);
    occtl_node_kind_t aKind;
    ASSERT_EQ(occtl_graph_node_kind(myGraph, aNode, &aKind), OCCTL_OK);
    ++aCount;
  }
  EXPECT_GT(aCount, 0);
  occtl_topo_explorer_iter_free(anIter);
}

TEST_F(TopoExplorerTest, ChildExplorer_TargetKind_Face)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_topo_child_explorer_config_t aConfig = OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT;
  aConfig.target_kind                        = OCCTL_KIND_FACE;

  occtl_topo_explorer_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_child_explorer_create(myGraph, aSolidId, &aConfig, &anIter), OCCTL_OK);

  int                 aCount = 0;
  occtl_node_id_t     aNode;
  occtl_transform_t   aTrsf;
  occtl_orientation_t aOri;
  while (occtl_topo_explorer_iter_next(anIter, &aNode, &aTrsf, &aOri) == OCCTL_OK)
  {
    occtl_node_kind_t aKind;
    ASSERT_EQ(occtl_graph_node_kind(myGraph, aNode, &aKind), OCCTL_OK);
    EXPECT_EQ(aKind, OCCTL_KIND_FACE);
    ++aCount;
  }
  EXPECT_EQ(aCount, 6);
  occtl_topo_explorer_iter_free(anIter);
}

TEST_F(TopoExplorerTest, ChildExplorer_NullOutParam)
{
  occtl_topo_explorer_iter_t* anIter   = nullptr;
  const occtl_node_id_t       aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  ASSERT_EQ(occtl_topo_child_explorer_create(myGraph, aSolidId, nullptr, &anIter), OCCTL_OK);
  occtl_transform_t   aTrsf;
  occtl_orientation_t aOri;
  EXPECT_EQ(occtl_topo_explorer_iter_next(anIter, nullptr, &aTrsf, &aOri), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  occtl_topo_explorer_iter_free(anIter);
}

TEST_F(TopoExplorerTest, ParentExplorer_EdgeToSolid_YieldsParents)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_topo_explorer_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_parent_explorer_create(myGraph, anEdgeId, nullptr, &anIter), OCCTL_OK);

  int                 aCount = 0;
  occtl_node_id_t     aNode;
  occtl_transform_t   aTrsf;
  occtl_orientation_t aOri;
  while (occtl_topo_explorer_iter_next(anIter, &aNode, &aTrsf, &aOri) == OCCTL_OK)
  {
    EXPECT_NE(aNode.bits, 0u);
    ++aCount;
  }
  EXPECT_GT(aCount, 0);
  occtl_topo_explorer_iter_free(anIter);
}

TEST_F(TopoExplorerTest, ParentExplorer_LocationIsIdentity)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_topo_explorer_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_topo_parent_explorer_create(myGraph, aFaceId, nullptr, &anIter), OCCTL_OK);

  occtl_node_id_t     aNode;
  occtl_transform_t   aTrsf;
  occtl_orientation_t aOri;
  while (occtl_topo_explorer_iter_next(anIter, &aNode, &aTrsf, &aOri) == OCCTL_OK)
  {
    for (int aI = 0; aI < 12; ++aI)
    {
      if (aI == 0 || aI == 5 || aI == 10)
      {
        EXPECT_NEAR(aTrsf.m[aI], 1.0, 1e-9);
      }
      else
      {
        EXPECT_NEAR(aTrsf.m[aI], 0.0, 1e-9);
      }
    }
  }
  occtl_topo_explorer_iter_free(anIter);
}

TEST_F(TopoExplorerTest, ParentExplorer_NotFound)
{
  occtl_topo_explorer_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_parent_explorer_create(myGraph, OCCTL_NODE_ID_INVALID, nullptr, &anIter),
            OCCTL_NOT_FOUND);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoExplorerTest, ConfigInit_Defaults)
{
  occtl_topo_child_explorer_config_t aChildCfg;
  occtl_topo_child_explorer_config_init(&aChildCfg);
  EXPECT_EQ(aChildCfg.struct_version, OCCTL_TOPO_CHILD_EXPLORER_CONFIG_VERSION_1);
  EXPECT_EQ(aChildCfg.p_next, nullptr);
  EXPECT_EQ(aChildCfg.accumulate_location, 1);
  EXPECT_EQ(aChildCfg.accumulate_orientation, 1);

  occtl_topo_parent_explorer_config_t aParentCfg;
  occtl_topo_parent_explorer_config_init(&aParentCfg);
  EXPECT_EQ(aParentCfg.struct_version, OCCTL_TOPO_PARENT_EXPLORER_CONFIG_VERSION_1);
  EXPECT_EQ(aParentCfg.p_next, nullptr);
}

TEST_F(TopoExplorerTest, ChildExplorer_InvalidConfigFields_ReturnExpectedStatus)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_topo_child_explorer_config_t aCfg   = OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT;
  occtl_topo_explorer_iter_t*        anIter = nullptr;
  int                                aTag   = 1;

  aCfg.p_next = &aTag;
  EXPECT_EQ(occtl_topo_child_explorer_create(myGraph, aSolidId, &aCfg, &anIter),
            OCCTL_INVALID_ARGUMENT);

  aCfg      = OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT;
  aCfg.mode = static_cast<occtl_topo_explorer_traversal_t>(999);
  EXPECT_EQ(occtl_topo_child_explorer_create(myGraph, aSolidId, &aCfg, &anIter),
            OCCTL_OUT_OF_RANGE);

  aCfg             = OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT;
  aCfg.target_kind = static_cast<occtl_node_kind_t>(999);
  EXPECT_EQ(occtl_topo_child_explorer_create(myGraph, aSolidId, &aCfg, &anIter),
            OCCTL_OUT_OF_RANGE);

  aCfg            = OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT;
  aCfg.avoid_kind = static_cast<occtl_node_kind_t>(999);
  EXPECT_EQ(occtl_topo_child_explorer_create(myGraph, aSolidId, &aCfg, &anIter),
            OCCTL_OUT_OF_RANGE);
}

TEST_F(TopoExplorerTest, ParentExplorer_InvalidConfigFields_ReturnExpectedStatus)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_topo_parent_explorer_config_t aCfg   = OCCTL_TOPO_PARENT_EXPLORER_CONFIG_INIT;
  occtl_topo_explorer_iter_t*         anIter = nullptr;
  int                                 aTag   = 1;

  aCfg.p_next = &aTag;
  EXPECT_EQ(occtl_topo_parent_explorer_create(myGraph, anEdgeId, &aCfg, &anIter),
            OCCTL_INVALID_ARGUMENT);

  aCfg      = OCCTL_TOPO_PARENT_EXPLORER_CONFIG_INIT;
  aCfg.mode = static_cast<occtl_topo_explorer_traversal_t>(999);
  EXPECT_EQ(occtl_topo_parent_explorer_create(myGraph, anEdgeId, &aCfg, &anIter),
            OCCTL_OUT_OF_RANGE);

  aCfg             = OCCTL_TOPO_PARENT_EXPLORER_CONFIG_INIT;
  aCfg.target_kind = static_cast<occtl_node_kind_t>(999);
  EXPECT_EQ(occtl_topo_parent_explorer_create(myGraph, anEdgeId, &aCfg, &anIter),
            OCCTL_OUT_OF_RANGE);

  aCfg            = OCCTL_TOPO_PARENT_EXPLORER_CONFIG_INIT;
  aCfg.avoid_kind = static_cast<occtl_node_kind_t>(999);
  EXPECT_EQ(occtl_topo_parent_explorer_create(myGraph, anEdgeId, &aCfg, &anIter),
            OCCTL_OUT_OF_RANGE);
}

} // anonymous namespace
