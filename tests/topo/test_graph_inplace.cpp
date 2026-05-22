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

#include <occtl/occtl_core.h>
#include <occtl/occtl_geom.h>
#include <occtl/occtl_topo.h>

#include "test_helpers_internal.hxx"

#include <limits>

namespace
{

class TopoInplaceTest : public ::testing::Test
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

  occtl_node_id_t firstAbiNodeOfKind(occtl_node_kind_t theKind) const
  {
    return ::firstAbiNodeOfKind(myGraph, theKind);
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(TopoInplaceTest, SetVertexPoint_UpdatesAndReadsBack)
{
  const occtl_node_id_t aVertexId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertexId.bits, 0u);

  const occtl_point3_t aNewPt = {1.5, 2.5, 3.5};
  ASSERT_EQ(occtl_topo_set_vertex_point(myGraph, aVertexId, aNewPt), OCCTL_OK);

  occtl_point3_t aReadPt;
  ASSERT_EQ(occtl_topo_vertex_point(myGraph, aVertexId, &aReadPt), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aReadPt.x, aNewPt.x);
  EXPECT_DOUBLE_EQ(aReadPt.y, aNewPt.y);
  EXPECT_DOUBLE_EQ(aReadPt.z, aNewPt.z);
}

TEST_F(TopoInplaceTest, SetVertexTolerance_UpdatesAndReadsBack)
{
  const occtl_node_id_t aVertexId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertexId.bits, 0u);

  const double aNewTol = 0.00123;
  ASSERT_EQ(occtl_topo_set_vertex_tolerance(myGraph, aVertexId, aNewTol), OCCTL_OK);

  double aReadTol = 0.0;
  ASSERT_EQ(occtl_topo_vertex_tolerance(myGraph, aVertexId, &aReadTol), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aReadTol, aNewTol);
}

TEST_F(TopoInplaceTest, SetEdgeParamRange_UpdatesAndReadsBack)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  ASSERT_EQ(occtl_topo_set_edge_param_range(myGraph, anEdgeId, 0.5, 3.5), OCCTL_OK);

  double aFirst = 0.0, aLast = 0.0;
  ASSERT_EQ(occtl_topo_edge_range(myGraph, anEdgeId, &aFirst, &aLast), OCCTL_OK);
  EXPECT_DOUBLE_EQ(aFirst, 0.5);
  EXPECT_DOUBLE_EQ(aLast, 3.5);
}

TEST_F(TopoInplaceTest, SetEdgeSameParameter_SetsAndReadsBack)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  ASSERT_EQ(occtl_topo_set_edge_same_parameter(myGraph, anEdgeId, 1), OCCTL_OK);

  int32_t aFlag = 0;
  ASSERT_EQ(occtl_topo_edge_same_parameter(myGraph, anEdgeId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 1);

  ASSERT_EQ(occtl_topo_set_edge_same_parameter(myGraph, anEdgeId, 0), OCCTL_OK);
  ASSERT_EQ(occtl_topo_edge_same_parameter(myGraph, anEdgeId, &aFlag), OCCTL_OK);
  EXPECT_EQ(aFlag, 0);
}

TEST_F(TopoInplaceTest, SetVertexPoint_NullGraph_ReturnsInvalidArgument)
{
  const occtl_node_id_t aVertexId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  const occtl_point3_t  aPt       = {0.0, 0.0, 0.0};
  EXPECT_EQ(occtl_topo_set_vertex_point(nullptr, aVertexId, aPt), OCCTL_INVALID_ARGUMENT);

  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoInplaceTest, SetVertexTolerance_NegativeOrNonFinite_ReturnsInvalidArgument)
{
  const occtl_node_id_t aVertexId = firstAbiNodeOfKind(OCCTL_KIND_VERTEX);
  ASSERT_NE(aVertexId.bits, 0u);

  EXPECT_EQ(occtl_topo_set_vertex_tolerance(myGraph, aVertexId, -1.0), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(
    occtl_topo_set_vertex_tolerance(myGraph, aVertexId, std::numeric_limits<double>::infinity()),
    OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoInplaceTest, SetEdgeParamRange_InvalidInput_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdgeId = firstAbiNodeOfKind(OCCTL_KIND_EDGE);
  ASSERT_NE(anEdgeId.bits, 0u);

  EXPECT_EQ(occtl_topo_set_edge_param_range(myGraph, anEdgeId, 2.0, 1.0), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_set_edge_param_range(myGraph,
                                            anEdgeId,
                                            0.0,
                                            std::numeric_limits<double>::infinity()),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoInplaceTest, SetCoedgeParamRange_InvalidInput_ReturnsInvalidArgument)
{
  const occtl_node_id_t aCoedgeId = firstAbiNodeOfKind(OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoedgeId.bits, 0u);

  EXPECT_EQ(occtl_topo_set_coedge_param_range(myGraph, aCoedgeId, 3.0, 2.0),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_set_coedge_param_range(myGraph,
                                              aCoedgeId,
                                              std::numeric_limits<double>::infinity(),
                                              1.0),
            OCCTL_INVALID_ARGUMENT);
}

} // namespace
