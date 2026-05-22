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

#include <cmath>
#include <limits>

namespace
{

occtl_node_id_t firstOccurrenceOfProduct(const occtl_graph_t* const theGraph,
                                         const occtl_node_id_t      theProduct)
{
  occtl_node_iter_t* anIter = nullptr;
  if (occtl_topo_occurrences_of_product_iter_create(theGraph, theProduct, &anIter) != OCCTL_OK)
  {
    return OCCTL_NODE_ID_INVALID;
  }

  occtl_node_id_t      anOccurrence = OCCTL_NODE_ID_INVALID;
  const occtl_status_t aNext        = occtl_node_iter_next(anIter, &anOccurrence);
  occtl_node_iter_free(anIter);
  return aNext == OCCTL_OK ? anOccurrence : OCCTL_NODE_ID_INVALID;
}

void expectTranslation(const occtl_transform_t& theTransform,
                       const double             theX,
                       const double             theY,
                       const double             theZ)
{
  EXPECT_NEAR(theTransform.m[3], theX, 1.0e-12);
  EXPECT_NEAR(theTransform.m[7], theY, 1.0e-12);
  EXPECT_NEAR(theTransform.m[11], theZ, 1.0e-12);
}

class GraphAssemblyTest : public ::testing::Test
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

TEST_F(GraphAssemblyTest, MakeProduct_EmptyProduct_ReturnsOk)
{
  occtl_topo_make_product_info_t anInfo   = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aProduct = {};
  EXPECT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_OK);
  EXPECT_NE(aProduct.bits, 0u);

  occtl_node_kind_t aKind = {};
  EXPECT_EQ(occtl_graph_node_kind(aGraph, aProduct, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_PRODUCT);
}

TEST_F(GraphAssemblyTest, MakeProduct_WithRoot_ReturnsOk)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_topo_make_product_info_t anInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  anInfo.root                           = aSolidId;

  occtl_node_id_t aProduct = {};
  EXPECT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_OK);
  EXPECT_NE(aProduct.bits, 0u);

  occtl_node_kind_t aKind = {};
  EXPECT_EQ(occtl_graph_node_kind(aGraph, aProduct, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_PRODUCT);
}

TEST_F(GraphAssemblyTest, MakeProduct_NullInfo_ReturnsInvalidArg)
{
  occtl_node_id_t aProduct = {};
  EXPECT_EQ(occtl_topo_make_product(aGraph, nullptr, &aProduct), OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphAssemblyTest, MakeProduct_InvalidInfoFields_ReturnExpectedStatus)
{
  occtl_topo_make_product_info_t anInfo   = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aProduct = {};

  int aTag      = 0;
  anInfo.p_next = &aTag;
  EXPECT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_INVALID_ARGUMENT);

  anInfo           = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  anInfo.root.bits = 1u;
  EXPECT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_NOT_FOUND);

  anInfo                 = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  anInfo.placement.m[11] = std::numeric_limits<double>::infinity();
  EXPECT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphAssemblyTest, LinkProductToTopology_EmptyProduct_ReturnsOk)
{
  occtl_topo_make_product_info_t anInfo   = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aProduct = {};
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_OK);

  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  const occtl_transform_t anIdTrsf = occtl_transform_identity();
  EXPECT_EQ(occtl_topo_link_product(aGraph, aProduct, aSolidId, anIdTrsf), OCCTL_OK);
}

TEST_F(GraphAssemblyTest, LinkProductToTopology_NonFiniteTransform_ReturnsInvalidArgument)
{
  occtl_topo_make_product_info_t anInfo   = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aProduct = {};
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_OK);

  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_transform_t aBadPlacement = occtl_transform_identity();
  aBadPlacement.m[0]              = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_topo_link_product(aGraph, aProduct, aSolidId, aBadPlacement),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphAssemblyTest, LinkProductToTopologyWithOccurrence_ReturnsOccurrence)
{
  occtl_topo_make_product_info_t anInfo   = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aProduct = {};
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_OK);

  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  const occtl_transform_t aPlacement   = occtl_transform_translation({3.0, 4.0, 5.0});
  occtl_node_id_t         anOccurrence = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(
    occtl_topo_link_product_occurrence(aGraph, aProduct, aSolidId, aPlacement, &anOccurrence),
    OCCTL_OK);
  ASSERT_NE(anOccurrence.bits, 0u);

  occtl_node_kind_t aKind = {};
  ASSERT_EQ(occtl_graph_node_kind(aGraph, anOccurrence, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_OCCURRENCE);

  occtl_transform_t aReadBack = occtl_transform_identity();
  ASSERT_EQ(occtl_topo_occurrence_transform(aGraph, anOccurrence, &aReadBack), OCCTL_OK);
  expectTranslation(aReadBack, 3.0, 4.0, 5.0);
}

TEST_F(GraphAssemblyTest, LinkProducts_TwoProducts_ReturnsOk)
{
  occtl_topo_make_product_info_t anEmptyInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aParent     = {};
  occtl_node_id_t                aChild      = {};
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anEmptyInfo, &aParent), OCCTL_OK);
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anEmptyInfo, &aChild), OCCTL_OK);

  const occtl_transform_t anIdTrsf = occtl_transform_identity();
  const occtl_node_id_t   aNullOcc = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_link_products(aGraph, aParent, aChild, anIdTrsf, aNullOcc), OCCTL_OK);
}

TEST_F(GraphAssemblyTest, LinkProducts_NonFiniteTransform_ReturnsInvalidArgument)
{
  occtl_topo_make_product_info_t anEmptyInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aParent     = {};
  occtl_node_id_t                aChild      = {};
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anEmptyInfo, &aParent), OCCTL_OK);
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anEmptyInfo, &aChild), OCCTL_OK);

  occtl_transform_t aBadPlacement = occtl_transform_identity();
  aBadPlacement.m[3]              = std::numeric_limits<double>::infinity();
  EXPECT_EQ(occtl_topo_link_products(aGraph, aParent, aChild, aBadPlacement, OCCTL_NODE_ID_INVALID),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphAssemblyTest, LinkProductsWithOccurrence_TwoProducts_ReturnsOccurrence)
{
  occtl_topo_make_product_info_t anEmptyInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aParent     = {};
  occtl_node_id_t                aChild      = {};
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anEmptyInfo, &aParent), OCCTL_OK);
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anEmptyInfo, &aChild), OCCTL_OK);

  const occtl_transform_t aPlacement   = occtl_transform_translation({-1.0, 2.0, 8.0});
  occtl_node_id_t         anOccurrence = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_link_products_occurrence(aGraph,
                                                aParent,
                                                aChild,
                                                aPlacement,
                                                OCCTL_NODE_ID_INVALID,
                                                &anOccurrence),
            OCCTL_OK);
  ASSERT_NE(anOccurrence.bits, 0u);

  occtl_node_kind_t aKind = {};
  ASSERT_EQ(occtl_graph_node_kind(aGraph, anOccurrence, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_OCCURRENCE);

  occtl_transform_t aReadBack = occtl_transform_identity();
  ASSERT_EQ(occtl_topo_occurrence_transform(aGraph, anOccurrence, &aReadBack), OCCTL_OK);
  expectTranslation(aReadBack, -1.0, 2.0, 8.0);
}

TEST_F(GraphAssemblyTest, LinkProductsWithOccurrence_NullOut_ReturnsInvalidArgument)
{
  occtl_topo_make_product_info_t anEmptyInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aParent     = {};
  occtl_node_id_t                aChild      = {};
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anEmptyInfo, &aParent), OCCTL_OK);
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anEmptyInfo, &aChild), OCCTL_OK);

  const occtl_transform_t anIdTrsf = occtl_transform_identity();
  EXPECT_EQ(occtl_topo_link_products_occurrence(aGraph,
                                                aParent,
                                                aChild,
                                                anIdTrsf,
                                                OCCTL_NODE_ID_INVALID,
                                                nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphAssemblyTest, RemoveOccurrence_ValidRef_ReturnsOk)
{
  occtl_topo_make_product_info_t anInfo   = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aProduct = {};
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_OK);

  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  const occtl_transform_t anIdTrsf = occtl_transform_identity();
  ASSERT_EQ(occtl_topo_link_product(aGraph, aProduct, aSolidId, anIdTrsf), OCCTL_OK);
}

TEST_F(GraphAssemblyTest, OccurrenceTransformSetGet_UniqueOccurrence_ReturnsPlacement)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_topo_make_product_info_t anInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  anInfo.root                           = aSolidId;

  occtl_node_id_t aProduct = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_OK);

  const occtl_node_id_t anOccurrence = firstOccurrenceOfProduct(aGraph, aProduct);
  ASSERT_NE(anOccurrence.bits, 0u);

  const occtl_transform_t aPlacement = occtl_transform_translation({2.0, 3.0, 4.0});
  EXPECT_EQ(occtl_topo_occurrence_set_transform(aGraph, anOccurrence, aPlacement), OCCTL_OK);

  occtl_transform_t aReadBack = occtl_transform_identity();
  EXPECT_EQ(occtl_topo_occurrence_transform(aGraph, anOccurrence, &aReadBack), OCCTL_OK);
  expectTranslation(aReadBack, 2.0, 3.0, 4.0);
}

TEST_F(GraphAssemblyTest, OccurrenceTransformSet_NonFiniteTransform_ReturnsInvalidArgument)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_topo_make_product_info_t anInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  anInfo.root                           = aSolidId;

  occtl_node_id_t aProduct = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_OK);

  const occtl_node_id_t anOccurrence = firstOccurrenceOfProduct(aGraph, aProduct);
  ASSERT_NE(anOccurrence.bits, 0u);

  occtl_transform_t aBadTransform = occtl_transform_identity();
  aBadTransform.m[5]              = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_topo_occurrence_set_transform(aGraph, anOccurrence, aBadTransform),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(GraphAssemblyTest, OccurrenceWorldTransform_ProductRoot_ReturnsAccumulatedPlacement)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_topo_make_product_info_t anInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  anInfo.root                           = aSolidId;
  anInfo.placement                      = occtl_transform_translation({5.0, 6.0, 7.0});

  occtl_node_id_t aProduct = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anInfo, &aProduct), OCCTL_OK);

  const occtl_node_id_t anOccurrence = firstOccurrenceOfProduct(aGraph, aProduct);
  ASSERT_NE(anOccurrence.bits, 0u);

  occtl_transform_t aWorld = occtl_transform_identity();
  EXPECT_EQ(occtl_topo_occurrence_world_transform(aGraph, aProduct, anOccurrence, &aWorld),
            OCCTL_OK);
  expectTranslation(aWorld, 5.0, 6.0, 7.0);
}

TEST_F(GraphAssemblyTest, OccurrenceTransformGet_NonOccurrence_ReturnsWrongKind)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  occtl_transform_t aTransform = occtl_transform_identity();
  EXPECT_EQ(occtl_topo_occurrence_transform(aGraph, aSolidId, &aTransform), OCCTL_WRONG_KIND);
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_NE(anErr->message, nullptr);
}

TEST_F(GraphAssemblyTest, LinkProductToTopology_NonProductNode_ReturnsWrongKind)
{
  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  const occtl_transform_t anIdTrsf = occtl_transform_identity();
  EXPECT_EQ(occtl_topo_link_product(aGraph, aSolidId, aSolidId, anIdTrsf), OCCTL_WRONG_KIND);
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_NE(anErr->message, nullptr);
}

TEST_F(GraphAssemblyTest, LinkProducts_NonProductParent_ReturnsWrongKind)
{
  occtl_topo_make_product_info_t anEmptyInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  occtl_node_id_t                aChild      = {};
  ASSERT_EQ(occtl_topo_make_product(aGraph, &anEmptyInfo, &aChild), OCCTL_OK);

  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  const occtl_transform_t anIdTrsf = occtl_transform_identity();
  const occtl_node_id_t   aNullOcc = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_link_products(aGraph, aSolidId, aChild, anIdTrsf, aNullOcc),
            OCCTL_WRONG_KIND);
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_NE(anErr->message, nullptr);
}

TEST_F(GraphAssemblyTest, LinkProductToTopology_InvalidProduct_ReturnsNotFound)
{
  occtl_node_id_t aBadId = {};
  aBadId.bits            = 0xfeedfacefeedfaceULL;

  const occtl_node_id_t aSolidId = firstAbiNodeOfKind(aGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidId.bits, 0u);

  const occtl_transform_t anIdTrsf = occtl_transform_identity();
  EXPECT_EQ(occtl_topo_link_product(aGraph, aBadId, aSolidId, anIdTrsf), OCCTL_NOT_FOUND);
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_NE(anErr->message, nullptr);
}

} // namespace
