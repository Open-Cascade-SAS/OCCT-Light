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

#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include "test_helpers_internal.hxx"

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace
{

class GraphLayerTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
    ASSERT_NE(aGraph, nullptr);
    loadBox(aGraph);
  }

  void TearDown() override
  {
    occtl_graph_free(aGraph);
    aGraph = nullptr;
  }

  occtl_graph_t* aGraph = nullptr;
};

TEST_F(GraphLayerTest, ColorSetGetUnset_OnFace_ReturnsOk)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  const occtl_color_rgba_t aRed = {1.0f, 0.0f, 0.0f, 1.0f};
  EXPECT_EQ(occtl_graph_color_set(aGraph, aFaceId, aRed), OCCTL_OK);

  occtl_color_rgba_t anOut = {};
  EXPECT_EQ(occtl_graph_color_get(aGraph, aFaceId, &anOut), OCCTL_OK);
  EXPECT_FLOAT_EQ(anOut.r, 1.0f);
  EXPECT_FLOAT_EQ(anOut.g, 0.0f);
  EXPECT_FLOAT_EQ(anOut.b, 0.0f);
  EXPECT_FLOAT_EQ(anOut.a, 1.0f);

  EXPECT_EQ(occtl_graph_color_unset(aGraph, aFaceId), OCCTL_OK);

  occtl_color_rgba_t anOut2 = {};
  EXPECT_EQ(occtl_graph_color_get(aGraph, aFaceId, &anOut2), OCCTL_OK);
  EXPECT_FLOAT_EQ(anOut2.r, 1.0f);
  EXPECT_FLOAT_EQ(anOut2.g, 1.0f);
  EXPECT_FLOAT_EQ(anOut2.b, 1.0f);
  EXPECT_FLOAT_EQ(anOut2.a, 1.0f);
}

TEST_F(GraphLayerTest, ColorSet_NullGraph_ReturnsInvalidArg)
{
  const occtl_color_rgba_t aColor  = {0.5f, 0.5f, 0.5f, 1.0f};
  const occtl_status_t     aStatus = occtl_graph_color_set(nullptr, OCCTL_NODE_ID_INVALID, aColor);
  EXPECT_EQ(aStatus, OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(GraphLayerTest, ColorGet_NullOut_ReturnsInvalidArg)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  const occtl_status_t  aStatus = occtl_graph_color_get(aGraph, aFaceId, nullptr);
  EXPECT_EQ(aStatus, OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphLayerTest, ColorUnset_UnsetTwice_Noop)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  EXPECT_EQ(occtl_graph_color_unset(aGraph, aFaceId), OCCTL_OK);
  EXPECT_EQ(occtl_graph_color_unset(aGraph, aFaceId), OCCTL_OK);
}

TEST_F(GraphLayerTest, ColorSet_NonFiniteChannel_ReturnsInvalidArgument)
{
  const occtl_node_id_t    aFaceId   = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  const occtl_color_rgba_t aBadColor = {std::numeric_limits<float>::infinity(), 0.1f, 0.2f, 1.0f};
  EXPECT_EQ(occtl_graph_color_set(aGraph, aFaceId, aBadColor), OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphLayerTest, NameSetGet_TwoCallPattern_ReturnsOk)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  const char aName[] = "TestFace";
  EXPECT_EQ(occtl_graph_name_set(aGraph, aFaceId, aName, 8), OCCTL_OK);

  size_t aRequired = 0;
  EXPECT_EQ(occtl_graph_name_get(aGraph, aFaceId, nullptr, 0, &aRequired), OCCTL_OK);
  EXPECT_EQ(aRequired, 9u);

  char aBuf[16] = {};
  EXPECT_EQ(occtl_graph_name_get(aGraph, aFaceId, aBuf, sizeof(aBuf), &aRequired), OCCTL_OK);
  EXPECT_STREQ(aBuf, "TestFace");
}

TEST_F(GraphLayerTest, NameGet_BufferTooSmall_ReturnsBufferTooSmall)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  const char aName[] = "LongName";
  EXPECT_EQ(occtl_graph_name_set(aGraph, aFaceId, aName, 8), OCCTL_OK);

  char                 aBuf[2]   = {};
  size_t               aRequired = 0;
  const occtl_status_t aStatus   = occtl_graph_name_get(aGraph, aFaceId, aBuf, 2, &aRequired);
  EXPECT_EQ(aStatus, OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aRequired, 9u);
}

TEST_F(GraphLayerTest, NameSet_EmptyName_ClearsName)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  const char aName[] = "Temp";
  EXPECT_EQ(occtl_graph_name_set(aGraph, aFaceId, aName, 4), OCCTL_OK);
  EXPECT_EQ(occtl_graph_name_set(aGraph, aFaceId, nullptr, 0), OCCTL_OK);

  size_t aRequired = 0;
  EXPECT_EQ(occtl_graph_name_get(aGraph, aFaceId, nullptr, 0, &aRequired), OCCTL_OK);
  EXPECT_EQ(aRequired, 1u);
}

TEST_F(GraphLayerTest, MaterialSetGetUnset_TwoCallPattern_ReturnsOk)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_uid_t aFaceUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(aGraph, aFaceId, &aFaceUid), OCCTL_OK);

  occtl_material_info_t aMaterial = OCCTL_MATERIAL_INFO_INIT;
  aMaterial.name                  = "Aluminium";
  aMaterial.name_len              = 9;
  aMaterial.has_density           = 1;
  aMaterial.density               = 2700.0;
  aMaterial.has_diffuse_color     = 1;
  aMaterial.diffuse_color         = {0.8f, 0.8f, 0.75f, 1.0f};
  aMaterial.metadata_uid          = aFaceUid;
  ASSERT_EQ(occtl_graph_material_set(aGraph, aFaceId, &aMaterial), OCCTL_OK);

  occtl_material_info_t anOut     = OCCTL_MATERIAL_INFO_INIT;
  size_t                aRequired = 0;
  ASSERT_EQ(occtl_graph_material_get(aGraph, aFaceId, &anOut, nullptr, 0, &aRequired), OCCTL_OK);
  EXPECT_EQ(aRequired, 10u);
  EXPECT_EQ(anOut.name, nullptr);
  EXPECT_EQ(anOut.name_len, 9u);

  char aName[16] = {};
  ASSERT_EQ(occtl_graph_material_get(aGraph, aFaceId, &anOut, aName, sizeof(aName), &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aName, "Aluminium");
  EXPECT_EQ(anOut.name, aName);
  EXPECT_EQ(anOut.has_density, 1);
  EXPECT_DOUBLE_EQ(anOut.density, 2700.0);
  EXPECT_EQ(anOut.has_diffuse_color, 1);
  EXPECT_FLOAT_EQ(anOut.diffuse_color.r, 0.8f);
  EXPECT_FLOAT_EQ(anOut.diffuse_color.g, 0.8f);
  EXPECT_FLOAT_EQ(anOut.diffuse_color.b, 0.75f);
  EXPECT_FLOAT_EQ(anOut.diffuse_color.a, 1.0f);
  EXPECT_EQ(anOut.metadata_uid.bits, aFaceUid.bits);

  ASSERT_EQ(occtl_graph_material_unset(aGraph, aFaceId), OCCTL_OK);
  EXPECT_EQ(occtl_graph_material_get(aGraph, aFaceId, &anOut, nullptr, 0, &aRequired),
            OCCTL_NOT_FOUND);
}

TEST_F(GraphLayerTest, MaterialGet_BufferTooSmall_ReturnsBufferTooSmall)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_material_info_t aMaterial = OCCTL_MATERIAL_INFO_INIT;
  aMaterial.name                  = "Steel";
  aMaterial.name_len              = 5;
  ASSERT_EQ(occtl_graph_material_set(aGraph, aFaceId, &aMaterial), OCCTL_OK);

  occtl_material_info_t anOut     = OCCTL_MATERIAL_INFO_INIT;
  size_t                aRequired = 0;
  char                  aName[2]  = {};
  EXPECT_EQ(occtl_graph_material_get(aGraph, aFaceId, &anOut, aName, sizeof(aName), &aRequired),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aRequired, 6u);
}

TEST_F(GraphLayerTest, MaterialSet_InvalidDensity_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_material_info_t aMaterial = OCCTL_MATERIAL_INFO_INIT;
  aMaterial.has_density           = 1;
  aMaterial.density               = -1.0;
  EXPECT_EQ(occtl_graph_material_set(aGraph, aFaceId, &aMaterial), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(GraphLayerTest, UnitsDefaultGet_ReturnsMeters)
{
  double aScale    = 0.0;
  size_t aRequired = 0;
  ASSERT_EQ(occtl_graph_units_get(aGraph, &aScale, nullptr, 0, &aRequired), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aScale, 1.0);
  EXPECT_EQ(aRequired, 2u);

  char aName[4] = {};
  ASSERT_EQ(occtl_graph_units_get(aGraph, &aScale, aName, sizeof(aName), &aRequired), OCCTL_OK);
  EXPECT_STREQ(aName, "m");
}

TEST_F(GraphLayerTest, UnitsSetGet_TwoCallPattern_ReturnsOk)
{
  ASSERT_EQ(occtl_graph_units_set(aGraph, 0.001, "mm", 2), OCCTL_OK);

  double aScale    = 0.0;
  size_t aRequired = 0;
  ASSERT_EQ(occtl_graph_units_get(aGraph, &aScale, nullptr, 0, &aRequired), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aScale, 0.001);
  EXPECT_EQ(aRequired, 3u);

  char aName[8] = {};
  ASSERT_EQ(occtl_graph_units_get(aGraph, &aScale, aName, sizeof(aName), &aRequired), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aScale, 0.001);
  EXPECT_STREQ(aName, "mm");
}

TEST_F(GraphLayerTest, UnitsGet_BufferTooSmall_ReturnsBufferTooSmall)
{
  ASSERT_EQ(occtl_graph_units_set(aGraph, 0.0254, "inch", 4), OCCTL_OK);

  double aScale    = 0.0;
  size_t aRequired = 0;
  char   aName[2]  = {};
  EXPECT_EQ(occtl_graph_units_get(aGraph, &aScale, aName, sizeof(aName), &aRequired),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aRequired, 5u);
}

TEST_F(GraphLayerTest, UnitsSet_InvalidScale_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_graph_units_set(aGraph, 0.0, "m", 1), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(GraphLayerTest, MetadataSetGetUnset_TwoCallPattern_ReturnsOk)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  ASSERT_EQ(occtl_graph_node_metadata_set(aGraph, aFaceId, "role", 4, "mount", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_node_metadata_set(aGraph, aFaceId, "owner", 5, "fixture", 7), OCCTL_OK);

  size_t aRequired = 0;
  ASSERT_EQ(occtl_graph_node_metadata_get(aGraph, aFaceId, "role", 4, nullptr, 0, &aRequired),
            OCCTL_OK);
  EXPECT_EQ(aRequired, 6u);

  char aValue[16] = {};
  ASSERT_EQ(
    occtl_graph_node_metadata_get(aGraph, aFaceId, "role", 4, aValue, sizeof(aValue), &aRequired),
    OCCTL_OK);
  EXPECT_STREQ(aValue, "mount");

  size_t aKeyCount = 0;
  ASSERT_EQ(occtl_graph_node_metadata_keys(aGraph, aFaceId, nullptr, 0, &aKeyCount), OCCTL_OK);
  EXPECT_EQ(aKeyCount, 2u);

  occtl_metadata_key_view_t aSmallKeys[1] = {};
  EXPECT_EQ(occtl_graph_node_metadata_keys(aGraph, aFaceId, aSmallKeys, 1, &aKeyCount),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aKeyCount, 2u);

  std::vector<occtl_metadata_key_view_t> aKeyViews(aKeyCount);
  ASSERT_EQ(
    occtl_graph_node_metadata_keys(aGraph, aFaceId, aKeyViews.data(), aKeyViews.size(), &aKeyCount),
    OCCTL_OK);
  ASSERT_EQ(aKeyCount, 2u);
  std::vector<std::string> aKeys;
  for (const occtl_metadata_key_view_t& aView : aKeyViews)
  {
    aKeys.emplace_back(aView.key, aView.key_len);
  }
  EXPECT_NE(std::find(aKeys.begin(), aKeys.end(), "owner"), aKeys.end());
  EXPECT_NE(std::find(aKeys.begin(), aKeys.end(), "role"), aKeys.end());

  ASSERT_EQ(occtl_graph_node_metadata_unset(aGraph, aFaceId, "role", 4), OCCTL_OK);
  EXPECT_EQ(occtl_graph_node_metadata_get(aGraph, aFaceId, "role", 4, nullptr, 0, &aRequired),
            OCCTL_NOT_FOUND);

  ASSERT_EQ(occtl_graph_node_metadata_keys(aGraph, aFaceId, nullptr, 0, &aKeyCount), OCCTL_OK);
  EXPECT_EQ(aKeyCount, 1u);
}

TEST_F(GraphLayerTest, GraphMetadataSetGetUnset_TwoCallPattern_ReturnsOk)
{
  ASSERT_EQ(occtl_graph_metadata_set(aGraph, "author", 6, "occt-light", 10), OCCTL_OK);
  ASSERT_EQ(occtl_graph_metadata_set(aGraph, "source", 6, "fixture", 7), OCCTL_OK);

  size_t aRequired = 0;
  ASSERT_EQ(occtl_graph_metadata_get(aGraph, "author", 6, nullptr, 0, &aRequired), OCCTL_OK);
  EXPECT_EQ(aRequired, 11u);

  char aValue[16] = {};
  ASSERT_EQ(occtl_graph_metadata_get(aGraph, "author", 6, aValue, sizeof(aValue), &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aValue, "occt-light");

  size_t aKeyCount = 0;
  ASSERT_EQ(occtl_graph_metadata_keys(aGraph, nullptr, 0, &aKeyCount), OCCTL_OK);
  EXPECT_EQ(aKeyCount, 2u);

  occtl_metadata_key_view_t aSmallKeys[1] = {};
  EXPECT_EQ(occtl_graph_metadata_keys(aGraph, aSmallKeys, 1, &aKeyCount), OCCTL_BUFFER_TOO_SMALL);

  std::vector<occtl_metadata_key_view_t> aKeys(aKeyCount);
  ASSERT_EQ(occtl_graph_metadata_keys(aGraph, aKeys.data(), aKeys.size(), &aKeyCount), OCCTL_OK);
  std::set<std::string> aNames;
  for (const occtl_metadata_key_view_t& aKey : aKeys)
  {
    aNames.emplace(aKey.key, aKey.key_len);
  }
  EXPECT_TRUE(aNames.count("author") != 0);
  EXPECT_TRUE(aNames.count("source") != 0);

  ASSERT_EQ(occtl_graph_metadata_unset(aGraph, "author", 6), OCCTL_OK);
  EXPECT_EQ(occtl_graph_metadata_get(aGraph, "author", 6, nullptr, 0, &aRequired), OCCTL_NOT_FOUND);
}

TEST_F(GraphLayerTest, GraphMetadataInvalidArguments_ReturnError)
{
  size_t aRequired = 0;
  EXPECT_EQ(occtl_graph_metadata_set(nullptr, "key", 3, "value", 5), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_metadata_set(aGraph, "", 0, "value", 5), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_metadata_set(aGraph, "key", 3, nullptr, 1), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_metadata_get(nullptr, "key", 3, nullptr, 0, &aRequired),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_metadata_get(aGraph, "key", 3, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_metadata_keys(nullptr, nullptr, 0, &aRequired), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_metadata_keys(aGraph, nullptr, 0, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_metadata_unset(nullptr, "key", 3), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_metadata_unset(aGraph, nullptr, 0), OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphLayerTest, MetadataGet_BufferTooSmall_ReturnsBufferTooSmall)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);
  ASSERT_EQ(occtl_graph_node_metadata_set(aGraph, aFaceId, "key", 3, "value", 5), OCCTL_OK);

  size_t aRequired = 0;
  char   aValue[2] = {};
  EXPECT_EQ(
    occtl_graph_node_metadata_get(aGraph, aFaceId, "key", 3, aValue, sizeof(aValue), &aRequired),
    OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aRequired, 6u);
}

TEST_F(GraphLayerTest, MetadataSet_EmptyKey_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  EXPECT_EQ(occtl_graph_node_metadata_set(aGraph, aFaceId, "", 0, "value", 5),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(GraphLayerTest, MetadataKeys_InvalidArguments_ReturnsError)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  size_t aKeyCount = 0;
  EXPECT_EQ(occtl_graph_node_metadata_keys(nullptr, aFaceId, nullptr, 0, &aKeyCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_node_metadata_keys(aGraph, aFaceId, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);

  const occtl_node_id_t anInvalidNode = {0u};
  EXPECT_EQ(occtl_graph_node_metadata_keys(aGraph, anInvalidNode, nullptr, 0, &aKeyCount),
            OCCTL_NOT_FOUND);
}

TEST_F(GraphLayerTest, TagAddListRemove_OnFace_ReturnsOk)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  ASSERT_EQ(occtl_graph_tag_add(aGraph, aFaceId, "mount", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_tag_add(aGraph, aFaceId, "fixture", 7), OCCTL_OK);
  ASSERT_EQ(occtl_graph_tag_add(aGraph, aFaceId, "mount", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_tag_add(aGraph, anEdgeId, "fixture", 7), OCCTL_OK);

  int32_t hasTag = 0;
  ASSERT_EQ(occtl_graph_tag_has(aGraph, aFaceId, "mount", 5, &hasTag), OCCTL_OK);
  EXPECT_EQ(hasTag, 1);
  ASSERT_EQ(occtl_graph_tag_has(aGraph, aFaceId, "missing", 7, &hasTag), OCCTL_OK);
  EXPECT_EQ(hasTag, 0);

  size_t aTagCount = 0;
  ASSERT_EQ(occtl_graph_tag_list(aGraph, aFaceId, nullptr, 0, &aTagCount), OCCTL_OK);
  EXPECT_EQ(aTagCount, 2u);

  occtl_tag_view_t aSmallTags[1] = {};
  EXPECT_EQ(occtl_graph_tag_list(aGraph, aFaceId, aSmallTags, 1, &aTagCount),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aTagCount, 2u);

  std::vector<occtl_tag_view_t> aTagViews(aTagCount);
  ASSERT_EQ(occtl_graph_tag_list(aGraph, aFaceId, aTagViews.data(), aTagViews.size(), &aTagCount),
            OCCTL_OK);
  std::vector<std::string> aTags;
  for (const occtl_tag_view_t& aView : aTagViews)
  {
    aTags.emplace_back(aView.tag, aView.tag_len);
  }
  EXPECT_NE(std::find(aTags.begin(), aTags.end(), "fixture"), aTags.end());
  EXPECT_NE(std::find(aTags.begin(), aTags.end(), "mount"), aTags.end());

  size_t aNodeCount = 0;
  ASSERT_EQ(occtl_graph_tag_nodes(aGraph, "fixture", 7, nullptr, 0, &aNodeCount), OCCTL_OK);
  EXPECT_EQ(aNodeCount, 2u);
  std::vector<occtl_node_id_t> aNodes(aNodeCount);
  ASSERT_EQ(occtl_graph_tag_nodes(aGraph, "fixture", 7, aNodes.data(), aNodes.size(), &aNodeCount),
            OCCTL_OK);
  EXPECT_EQ(aNodeCount, 2u);

  ASSERT_EQ(occtl_graph_tag_nodes(aGraph, nullptr, 0, nullptr, 0, &aNodeCount), OCCTL_OK);
  EXPECT_EQ(aNodeCount, 2u);

  ASSERT_EQ(occtl_graph_tag_remove(aGraph, aFaceId, "mount", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_tag_has(aGraph, aFaceId, "mount", 5, &hasTag), OCCTL_OK);
  EXPECT_EQ(hasTag, 0);
}

TEST_F(GraphLayerTest, TagInvalidArguments_ReturnsError)
{
  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  EXPECT_EQ(occtl_graph_tag_add(nullptr, aFaceId, "tag", 3), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_tag_add(aGraph, aFaceId, "", 0), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);

  int32_t hasTag = 0;
  EXPECT_EQ(occtl_graph_tag_has(aGraph, aFaceId, "tag", 3, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_tag_has(aGraph, OCCTL_NODE_ID_INVALID, "tag", 3, &hasTag), OCCTL_NOT_FOUND);

  size_t aCount = 0;
  EXPECT_EQ(occtl_graph_tag_list(aGraph, OCCTL_NODE_ID_INVALID, nullptr, 0, &aCount),
            OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_graph_tag_nodes(aGraph, nullptr, 1, nullptr, 0, &aCount), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_tag_nodes(aGraph, "tag", 0, nullptr, 0, &aCount), OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphLayerTest, JointInfoInit_DefaultsToRigidIdentity)
{
  occtl_joint_info_t anInfo;
  occtl_joint_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_JOINT_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.id.bits, 0u);
  EXPECT_EQ(anInfo.kind, OCCTL_JOINT_RIGID);
  EXPECT_EQ(anInfo.node_a.bits, 0u);
  EXPECT_EQ(anInfo.node_b.bits, 0u);
  EXPECT_DOUBLE_EQ(anInfo.frame_a.m[0], 1.0);
  EXPECT_DOUBLE_EQ(anInfo.frame_a.m[5], 1.0);
  EXPECT_DOUBLE_EQ(anInfo.frame_a.m[10], 1.0);
  EXPECT_EQ(anInfo.has_limit_min, 0);
  EXPECT_EQ(anInfo.has_limit_max, 0);
}

TEST_F(GraphLayerTest, JointCreateGetListRemove_ReturnsOk)
{
  const occtl_node_id_t aFaceId  = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aFaceId.bits, 0u);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_uid_t aFaceUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(aGraph, aFaceId, &aFaceUid), OCCTL_OK);

  occtl_joint_info_t anInfo = OCCTL_JOINT_INFO_INIT;
  anInfo.kind               = OCCTL_JOINT_REVOLUTE;
  anInfo.node_a             = aFaceId;
  anInfo.node_b             = anEdgeId;
  anInfo.frame_a.m[3]       = 10.0;
  anInfo.has_limit_min      = 1;
  anInfo.limit_min          = -0.25;
  anInfo.has_limit_max      = 1;
  anInfo.limit_max          = 0.75;
  anInfo.metadata_uid       = aFaceUid;

  occtl_joint_id_t aJoint = OCCTL_JOINT_ID_INVALID;
  ASSERT_EQ(occtl_joint_create(aGraph, &anInfo, &aJoint), OCCTL_OK);
  ASSERT_NE(aJoint.bits, 0u);

  occtl_joint_info_t anOut = OCCTL_JOINT_INFO_INIT;
  ASSERT_EQ(occtl_joint_get(aGraph, aJoint, &anOut), OCCTL_OK);
  EXPECT_EQ(anOut.id.bits, aJoint.bits);
  EXPECT_EQ(anOut.kind, OCCTL_JOINT_REVOLUTE);
  EXPECT_EQ(anOut.node_a.bits, aFaceId.bits);
  EXPECT_EQ(anOut.node_b.bits, anEdgeId.bits);
  EXPECT_DOUBLE_EQ(anOut.frame_a.m[3], 10.0);
  EXPECT_EQ(anOut.has_limit_min, 1);
  EXPECT_DOUBLE_EQ(anOut.limit_min, -0.25);
  EXPECT_EQ(anOut.has_limit_max, 1);
  EXPECT_DOUBLE_EQ(anOut.limit_max, 0.75);
  EXPECT_EQ(anOut.metadata_uid.bits, aFaceUid.bits);

  size_t aCount = 0;
  ASSERT_EQ(occtl_joint_list(aGraph, OCCTL_NODE_ID_INVALID, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 1u);
  occtl_joint_id_t aListed[1] = {};
  ASSERT_EQ(occtl_joint_list(aGraph, OCCTL_NODE_ID_INVALID, aListed, 1, &aCount), OCCTL_OK);
  EXPECT_EQ(aListed[0].bits, aJoint.bits);

  ASSERT_EQ(occtl_joint_list(aGraph, aFaceId, nullptr, 0, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 1u);

  ASSERT_EQ(occtl_joint_remove(aGraph, aJoint), OCCTL_OK);
  EXPECT_EQ(occtl_joint_get(aGraph, aJoint, &anOut), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_joint_remove(aGraph, aJoint), OCCTL_OK);
}

TEST_F(GraphLayerTest, JointCreate_InvalidLimits_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFaceId  = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aFaceId.bits, 0u);
  ASSERT_NE(anEdgeId.bits, 0u);

  occtl_joint_info_t anInfo = OCCTL_JOINT_INFO_INIT;
  anInfo.node_a             = aFaceId;
  anInfo.node_b             = anEdgeId;
  anInfo.has_limit_min      = 1;
  anInfo.limit_min          = 2.0;
  anInfo.has_limit_max      = 1;
  anInfo.limit_max          = 1.0;

  occtl_joint_id_t aJoint = OCCTL_JOINT_ID_INVALID;
  EXPECT_EQ(occtl_joint_create(aGraph, &anInfo, &aJoint), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST(GraphLayerStandaloneTest, Metadata_SurvivesCompactRemap)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_topo_make_vertex_info_t aInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aInfo.tolerance                     = 1e-6;

  occtl_node_id_t aRemovedVertex = OCCTL_NODE_ID_INVALID;
  aInfo.point                    = {0.0, 0.0, 0.0};
  ASSERT_EQ(occtl_topo_make_vertex(aGraph, &aInfo, &aRemovedVertex), OCCTL_OK);

  occtl_node_id_t aKeptVertex = OCCTL_NODE_ID_INVALID;
  aInfo.point                 = {1.0, 0.0, 0.0};
  ASSERT_EQ(occtl_topo_make_vertex(aGraph, &aInfo, &aKeptVertex), OCCTL_OK);

  occtl_uid_t aKeptUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(aGraph, aKeptVertex, &aKeptUid), OCCTL_OK);

  const occtl_color_rgba_t aColor = {0.25f, 0.50f, 0.75f, 1.0f};
  ASSERT_EQ(occtl_graph_color_set(aGraph, aKeptVertex, aColor), OCCTL_OK);
  ASSERT_EQ(occtl_graph_name_set(aGraph, aKeptVertex, "KeptVertex", 10), OCCTL_OK);
  occtl_material_info_t aMaterial = OCCTL_MATERIAL_INFO_INIT;
  aMaterial.name                  = "Copper";
  aMaterial.name_len              = 6;
  aMaterial.has_density           = 1;
  aMaterial.density               = 8960.0;
  ASSERT_EQ(occtl_graph_material_set(aGraph, aKeptVertex, &aMaterial), OCCTL_OK);
  ASSERT_EQ(occtl_graph_node_metadata_set(aGraph, aKeptVertex, "kind", 4, "datum", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_tag_add(aGraph, aKeptVertex, "datum", 5), OCCTL_OK);

  ASSERT_EQ(occtl_topo_remove(aGraph, aRemovedVertex), OCCTL_OK);
  ASSERT_EQ(occtl_graph_compact(aGraph), OCCTL_OK);

  occtl_node_id_t aRemappedVertex = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_graph_node_id_from_uid(aGraph, aKeptUid, &aRemappedVertex), OCCTL_OK);
  ASSERT_NE(aRemappedVertex.bits, 0u);

  occtl_color_rgba_t anOutColor = {};
  ASSERT_EQ(occtl_graph_color_get(aGraph, aRemappedVertex, &anOutColor), OCCTL_OK);
  EXPECT_FLOAT_EQ(anOutColor.r, aColor.r);
  EXPECT_FLOAT_EQ(anOutColor.g, aColor.g);
  EXPECT_FLOAT_EQ(anOutColor.b, aColor.b);
  EXPECT_FLOAT_EQ(anOutColor.a, aColor.a);

  size_t aRequired = 0;
  ASSERT_EQ(occtl_graph_name_get(aGraph, aRemappedVertex, nullptr, 0, &aRequired), OCCTL_OK);
  ASSERT_EQ(aRequired, 11u);
  char aName[16] = {};
  ASSERT_EQ(occtl_graph_name_get(aGraph, aRemappedVertex, aName, sizeof(aName), &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aName, "KeptVertex");

  occtl_material_info_t anOutMaterial = OCCTL_MATERIAL_INFO_INIT;
  ASSERT_EQ(
    occtl_graph_material_get(aGraph, aRemappedVertex, &anOutMaterial, nullptr, 0, &aRequired),
    OCCTL_OK);
  ASSERT_EQ(aRequired, 7u);
  char aMaterialName[16] = {};
  ASSERT_EQ(occtl_graph_material_get(aGraph,
                                     aRemappedVertex,
                                     &anOutMaterial,
                                     aMaterialName,
                                     sizeof(aMaterialName),
                                     &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aMaterialName, "Copper");
  EXPECT_EQ(anOutMaterial.has_density, 1);
  EXPECT_DOUBLE_EQ(anOutMaterial.density, 8960.0);

  ASSERT_EQ(
    occtl_graph_node_metadata_get(aGraph, aRemappedVertex, "kind", 4, nullptr, 0, &aRequired),
    OCCTL_OK);
  ASSERT_EQ(aRequired, 6u);
  char aValue[16] = {};
  ASSERT_EQ(occtl_graph_node_metadata_get(aGraph,
                                          aRemappedVertex,
                                          "kind",
                                          4,
                                          aValue,
                                          sizeof(aValue),
                                          &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aValue, "datum");

  int32_t hasTag = 0;
  ASSERT_EQ(occtl_graph_tag_has(aGraph, aRemappedVertex, "datum", 5, &hasTag), OCCTL_OK);
  EXPECT_EQ(hasTag, 1);

  occtl_graph_free(aGraph);
}

TEST(GraphLayerStandaloneTest, MetadataAndUnits_SurviveGraphClone)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  loadBox(aGraph);

  const occtl_node_id_t aFaceId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFaceId.bits, 0u);

  occtl_uid_t aFaceUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(aGraph, aFaceId, &aFaceUid), OCCTL_OK);

  const occtl_color_rgba_t aColor = {0.1f, 0.2f, 0.3f, 0.4f};
  ASSERT_EQ(occtl_graph_color_set(aGraph, aFaceId, aColor), OCCTL_OK);
  ASSERT_EQ(occtl_graph_name_set(aGraph, aFaceId, "CloneFace", 9), OCCTL_OK);
  occtl_material_info_t aMaterial = OCCTL_MATERIAL_INFO_INIT;
  aMaterial.name                  = "Steel";
  aMaterial.name_len              = 5;
  aMaterial.has_density           = 1;
  aMaterial.density               = 7850.0;
  ASSERT_EQ(occtl_graph_material_set(aGraph, aFaceId, &aMaterial), OCCTL_OK);
  ASSERT_EQ(occtl_graph_node_metadata_set(aGraph, aFaceId, "material", 8, "steel", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_tag_add(aGraph, aFaceId, "mount", 5), OCCTL_OK);
  ASSERT_EQ(occtl_graph_units_set(aGraph, 0.001, "mm", 2), OCCTL_OK);
  ASSERT_EQ(occtl_graph_metadata_set(aGraph, "author", 6, "unit-test", 9), OCCTL_OK);

  size_t anEntryCount = 0;
  ASSERT_EQ(occtl_graph_color_entries(aGraph, nullptr, nullptr, 0, &anEntryCount), OCCTL_OK);
  ASSERT_EQ(anEntryCount, 1u);
  occtl_node_id_t    aColorNode   = OCCTL_NODE_ID_INVALID;
  occtl_color_rgba_t aListedColor = {};
  ASSERT_EQ(occtl_graph_color_entries(aGraph, &aColorNode, &aListedColor, 1, &anEntryCount),
            OCCTL_OK);
  EXPECT_EQ(aColorNode.bits, aFaceId.bits);
  EXPECT_FLOAT_EQ(aListedColor.r, aColor.r);
  EXPECT_FLOAT_EQ(aListedColor.g, aColor.g);
  EXPECT_FLOAT_EQ(aListedColor.b, aColor.b);
  EXPECT_FLOAT_EQ(aListedColor.a, aColor.a);

  ASSERT_EQ(occtl_graph_name_nodes(aGraph, nullptr, 0, &anEntryCount), OCCTL_OK);
  ASSERT_EQ(anEntryCount, 1u);
  occtl_node_id_t aListedNode = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_graph_name_nodes(aGraph, &aListedNode, 1, &anEntryCount), OCCTL_OK);
  EXPECT_EQ(aListedNode.bits, aFaceId.bits);

  ASSERT_EQ(occtl_graph_material_nodes(aGraph, nullptr, 0, &anEntryCount), OCCTL_OK);
  ASSERT_EQ(anEntryCount, 1u);
  ASSERT_EQ(occtl_graph_material_nodes(aGraph, &aListedNode, 1, &anEntryCount), OCCTL_OK);
  EXPECT_EQ(aListedNode.bits, aFaceId.bits);

  ASSERT_EQ(occtl_graph_node_metadata_nodes(aGraph, nullptr, 0, &anEntryCount), OCCTL_OK);
  ASSERT_EQ(anEntryCount, 1u);
  ASSERT_EQ(occtl_graph_node_metadata_nodes(aGraph, &aListedNode, 1, &anEntryCount), OCCTL_OK);
  EXPECT_EQ(aListedNode.bits, aFaceId.bits);

  ASSERT_EQ(occtl_graph_tag_nodes(aGraph, nullptr, 0, &aListedNode, 1, &anEntryCount), OCCTL_OK);
  ASSERT_EQ(anEntryCount, 1u);
  EXPECT_EQ(aListedNode.bits, aFaceId.bits);

  EXPECT_EQ(occtl_graph_color_entries(aGraph, &aColorNode, nullptr, 1, &anEntryCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_name_nodes(aGraph, &aListedNode, 0, &anEntryCount), OCCTL_BUFFER_TOO_SMALL);

  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);
  occtl_joint_info_t aJointInfo = OCCTL_JOINT_INFO_INIT;
  aJointInfo.kind               = OCCTL_JOINT_LINEAR;
  aJointInfo.node_a             = aFaceId;
  aJointInfo.node_b             = anEdgeId;
  aJointInfo.has_limit_min      = 1;
  aJointInfo.limit_min          = 0.0;
  aJointInfo.has_limit_max      = 1;
  aJointInfo.limit_max          = 4.0;
  occtl_joint_id_t aJoint       = OCCTL_JOINT_ID_INVALID;
  ASSERT_EQ(occtl_joint_create(aGraph, &aJointInfo, &aJoint), OCCTL_OK);
  ASSERT_NE(aJoint.bits, 0u);

  occtl_graph_t* aClone = nullptr;
  ASSERT_EQ(occtl_graph_clone(aGraph, &aClone), OCCTL_OK);
  ASSERT_NE(aClone, nullptr);

  occtl_node_id_t aCloneFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_graph_node_id_from_uid(aClone, aFaceUid, &aCloneFace), OCCTL_OK);
  ASSERT_NE(aCloneFace.bits, 0u);

  occtl_color_rgba_t anOutColor = {};
  ASSERT_EQ(occtl_graph_color_get(aClone, aCloneFace, &anOutColor), OCCTL_OK);
  EXPECT_FLOAT_EQ(anOutColor.r, aColor.r);
  EXPECT_FLOAT_EQ(anOutColor.g, aColor.g);
  EXPECT_FLOAT_EQ(anOutColor.b, aColor.b);
  EXPECT_FLOAT_EQ(anOutColor.a, aColor.a);

  size_t aRequired = 0;
  ASSERT_EQ(occtl_graph_name_get(aClone, aCloneFace, nullptr, 0, &aRequired), OCCTL_OK);
  ASSERT_EQ(aRequired, 10u);
  char aName[16] = {};
  ASSERT_EQ(occtl_graph_name_get(aClone, aCloneFace, aName, sizeof(aName), &aRequired), OCCTL_OK);
  EXPECT_STREQ(aName, "CloneFace");

  occtl_material_info_t anOutMaterial = OCCTL_MATERIAL_INFO_INIT;
  ASSERT_EQ(occtl_graph_material_get(aClone, aCloneFace, &anOutMaterial, nullptr, 0, &aRequired),
            OCCTL_OK);
  ASSERT_EQ(aRequired, 6u);
  char aMaterialName[16] = {};
  ASSERT_EQ(occtl_graph_material_get(aClone,
                                     aCloneFace,
                                     &anOutMaterial,
                                     aMaterialName,
                                     sizeof(aMaterialName),
                                     &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aMaterialName, "Steel");
  EXPECT_EQ(anOutMaterial.has_density, 1);
  EXPECT_DOUBLE_EQ(anOutMaterial.density, 7850.0);

  ASSERT_EQ(
    occtl_graph_node_metadata_get(aClone, aCloneFace, "material", 8, nullptr, 0, &aRequired),
    OCCTL_OK);
  ASSERT_EQ(aRequired, 6u);
  char aValue[16] = {};
  ASSERT_EQ(occtl_graph_node_metadata_get(aClone,
                                          aCloneFace,
                                          "material",
                                          8,
                                          aValue,
                                          sizeof(aValue),
                                          &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aValue, "steel");

  int32_t hasTag = 0;
  ASSERT_EQ(occtl_graph_tag_has(aClone, aCloneFace, "mount", 5, &hasTag), OCCTL_OK);
  EXPECT_EQ(hasTag, 1);

  double aScale = 0.0;
  ASSERT_EQ(occtl_graph_units_get(aClone, &aScale, nullptr, 0, &aRequired), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aScale, 0.001);
  ASSERT_EQ(aRequired, 3u);
  char aUnitName[8] = {};
  ASSERT_EQ(occtl_graph_units_get(aClone, &aScale, aUnitName, sizeof(aUnitName), &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aUnitName, "mm");

  ASSERT_EQ(occtl_graph_metadata_get(aClone, "author", 6, nullptr, 0, &aRequired), OCCTL_OK);
  ASSERT_EQ(aRequired, 10u);
  char aGraphMetadata[16] = {};
  ASSERT_EQ(occtl_graph_metadata_get(aClone,
                                     "author",
                                     6,
                                     aGraphMetadata,
                                     sizeof(aGraphMetadata),
                                     &aRequired),
            OCCTL_OK);
  EXPECT_STREQ(aGraphMetadata, "unit-test");

  occtl_joint_info_t anOutJoint = OCCTL_JOINT_INFO_INIT;
  ASSERT_EQ(occtl_joint_get(aClone, aJoint, &anOutJoint), OCCTL_OK);
  EXPECT_EQ(anOutJoint.kind, OCCTL_JOINT_LINEAR);
  EXPECT_NE(anOutJoint.node_a.bits, 0u);
  EXPECT_NE(anOutJoint.node_b.bits, 0u);
  EXPECT_EQ(anOutJoint.has_limit_min, 1);
  EXPECT_DOUBLE_EQ(anOutJoint.limit_min, 0.0);
  EXPECT_EQ(anOutJoint.has_limit_max, 1);
  EXPECT_DOUBLE_EQ(anOutJoint.limit_max, 4.0);

  size_t aJointCount = 0;
  ASSERT_EQ(occtl_joint_list(aClone, aCloneFace, nullptr, 0, &aJointCount), OCCTL_OK);
  EXPECT_EQ(aJointCount, 1u);

  occtl_graph_free(aClone);
  occtl_graph_free(aGraph);
}

} // namespace
