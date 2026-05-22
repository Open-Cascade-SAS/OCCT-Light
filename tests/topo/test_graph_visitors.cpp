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
#include <occtl/occtl_topo.h>

#include "test_helpers_internal.hxx"

#include <cstring>

namespace
{

class TopoVisitorTest : public ::testing::Test
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

  static occtl_status_t OCCTL_CALL countNodeVisitor(occtl_node_id_t /*theId*/, void* theUser)
  {
    ++*static_cast<uint32_t*>(theUser);
    return OCCTL_OK;
  }

  static occtl_status_t OCCTL_CALL countRefVisitor(occtl_ref_id_t /*theId*/, void* theUser)
  {
    ++*static_cast<uint32_t*>(theUser);
    return OCCTL_OK;
  }

  static occtl_status_t OCCTL_CALL countRepVisitor(occtl_rep_id_t /*theId*/, void* theUser)
  {
    ++*static_cast<uint32_t*>(theUser);
    return OCCTL_OK;
  }

  static occtl_status_t OCCTL_CALL cancelAfterTwo(occtl_node_id_t /*theId*/, void* theUser)
  {
    uint32_t& aCount = *static_cast<uint32_t*>(theUser);
    ++aCount;
    return aCount >= 2 ? OCCTL_CANCELLED : OCCTL_OK;
  }

  static occtl_status_t OCCTL_CALL failingVisitor(occtl_node_id_t /*theId*/, void* /*theUser*/)
  {
    return OCCTL_GEOMETRY_INVALID;
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(TopoVisitorTest, ForEachKindMask_VisitsFaces)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each(myGraph, 1uLL << OCCTL_KIND_FACE, &countNodeVisitor, &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 6u);
}

TEST_F(TopoVisitorTest, ForEachKindMask_VisitsEdges)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each(myGraph, 1uLL << OCCTL_KIND_EDGE, &countNodeVisitor, &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 12u);
}

TEST_F(TopoVisitorTest, ForEachKindMask_VisitsVertices)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each(myGraph, 1uLL << OCCTL_KIND_VERTEX, &countNodeVisitor, &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 8u);
}

TEST_F(TopoVisitorTest, ForEachKindMask_VisitsSolids)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each(myGraph, 1uLL << OCCTL_KIND_SOLID, &countNodeVisitor, &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 1u);
}

TEST_F(TopoVisitorTest, ForEachKindMask_MultiMask)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each(myGraph,
                                 (1uLL << OCCTL_KIND_FACE) | (1uLL << OCCTL_KIND_EDGE),
                                 &countNodeVisitor,
                                 &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 18u);
}

TEST_F(TopoVisitorTest, CancelledEarlyReturnsOk_AndStopsVisits)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each(myGraph, 1uLL << OCCTL_KIND_FACE, &cancelAfterTwo, &aCount),
            OCCTL_OK);
  EXPECT_EQ(aCount, 2u);
}

TEST_F(TopoVisitorTest, NonOkPropagates)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each(myGraph, 1uLL << OCCTL_KIND_FACE, &failingVisitor, &aCount),
            OCCTL_GEOMETRY_INVALID);
}

TEST_F(TopoVisitorTest, NullFn_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_graph_for_each(myGraph, 1uLL << OCCTL_KIND_FACE, nullptr, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

TEST_F(TopoVisitorTest, NullGraph_ReturnsInvalidArgument)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each(nullptr, 1uLL << OCCTL_KIND_FACE, &countNodeVisitor, &aCount),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoVisitorTest, ForEachRef_Basic)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each_ref(myGraph,
                                     OCCTL_REF_KIND_FACE | OCCTL_REF_KIND_WIRE,
                                     &countRefVisitor,
                                     &aCount),
            OCCTL_OK);
  EXPECT_GT(aCount, 0u);
}

TEST_F(TopoVisitorTest, ForEachRef_NullFn)
{
  EXPECT_EQ(occtl_graph_for_each_ref(myGraph, OCCTL_REF_KIND_FACE, nullptr, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoVisitorTest, ForEachRef_NullGraph)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each_ref(nullptr, OCCTL_REF_KIND_FACE, &countRefVisitor, &aCount),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoVisitorTest, ForEachRep_Basic)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each_rep(myGraph,
                                     OCCTL_REP_KIND_SURFACE | OCCTL_REP_KIND_CURVE3D,
                                     &countRepVisitor,
                                     &aCount),
            OCCTL_OK);
  EXPECT_GT(aCount, 0u);
}

TEST_F(TopoVisitorTest, ForEachRep_NullFn)
{
  EXPECT_EQ(occtl_graph_for_each_rep(myGraph, OCCTL_REP_KIND_SURFACE, nullptr, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
}

TEST_F(TopoVisitorTest, ForEachRep_NullGraph)
{
  uint32_t aCount = 0;
  EXPECT_EQ(occtl_graph_for_each_rep(nullptr, OCCTL_REP_KIND_SURFACE, &countRepVisitor, &aCount),
            OCCTL_INVALID_ARGUMENT);
}

} // namespace
