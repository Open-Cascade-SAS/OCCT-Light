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

namespace
{

TEST(GraphLifecycleTest, Create_Default_ReturnsOk)
{
  occtl_graph_t* aGraph = nullptr;
  EXPECT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  EXPECT_NE(aGraph, nullptr);
  occtl_graph_free(aGraph);
}

TEST(GraphLifecycleTest, Create_NullOut_ReturnsInvalidArg)
{
  const occtl_status_t aStatus = occtl_graph_create(nullptr);
  EXPECT_EQ(aStatus, OCCTL_INVALID_ARGUMENT);
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_NE(anErr->message, nullptr);
}

TEST(GraphLifecycleTest, Free_Null_Noop)
{
  occtl_graph_free(nullptr);
}

TEST(GraphLifecycleTest, Free_MultipleCycles_Noop)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_graph_free(aGraph);

  occtl_graph_t* aGraph2 = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph2), OCCTL_OK);
  occtl_graph_free(aGraph2);

  occtl_graph_free(nullptr);
}

TEST(GraphLifecycleTest, Clone_FromFilledGraph_ReturnsValidClone)
{
  occtl_graph_t* aSource = nullptr;
  ASSERT_EQ(occtl_graph_create(&aSource), OCCTL_OK);
  loadBox(aSource);

  occtl_graph_t* aClone = nullptr;
  ASSERT_EQ(occtl_graph_clone(aSource, &aClone), OCCTL_OK);
  ASSERT_NE(aClone, nullptr);

  EXPECT_GE(occtl_graph_count_value(occtl_graph_node_count, aClone), 1u);

  occtl_graph_free(aClone);
  occtl_graph_free(aSource);
}

TEST(GraphLifecycleTest, Clone_NullSource_ReturnsInvalidArg)
{
  occtl_graph_t*       aClone  = nullptr;
  const occtl_status_t aStatus = occtl_graph_clone(nullptr, &aClone);
  EXPECT_EQ(aStatus, OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aClone, nullptr);
}

TEST(GraphLifecycleTest, Clone_NullOut_ReturnsInvalidArg)
{
  occtl_graph_t* aSource = nullptr;
  ASSERT_EQ(occtl_graph_create(&aSource), OCCTL_OK);
  const occtl_status_t aStatus = occtl_graph_clone(aSource, nullptr);
  EXPECT_EQ(aStatus, OCCTL_INVALID_ARGUMENT);
  occtl_graph_free(aSource);
}

TEST(GraphLifecycleTest, Compact_AfterLoad_ReturnsOk)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  loadBox(aGraph);

  const size_t aNbBefore = occtl_graph_count_value(occtl_graph_node_count, aGraph);
  EXPECT_EQ(occtl_graph_compact(aGraph), OCCTL_OK);
  const size_t aNbAfter = occtl_graph_count_value(occtl_graph_node_count, aGraph);
  EXPECT_EQ(aNbAfter, aNbBefore);

  occtl_graph_free(aGraph);
}

TEST(GraphLifecycleTest, Compact_NullGraph_ReturnsInvalidArg)
{
  EXPECT_EQ(occtl_graph_compact(nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GraphLifecycleTest, RemoveWithReplacement_ValidEdge_ReturnsOk)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  loadBox(aGraph);

  const occtl_node_id_t anEdge0 = firstAbiNodeOfKind(aGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge0.bits, 0u);

  BRepGraph_EdgeIterator anIt(aGraph->graph);
  ASSERT_TRUE(anIt.More());
  anIt.Next();
  ASSERT_TRUE(anIt.More());
  const occtl_node_id_t anEdge1 = OcctL::Topo::PackNodeId(anIt.CurrentId());

  EXPECT_EQ(occtl_topo_remove_with_replacement(aGraph, anEdge0, anEdge1), OCCTL_OK);
  occtl_graph_free(aGraph);
}

TEST(GraphLifecycleTest, RemoveWithReplacement_NullGraph_ReturnsInvalidArg)
{
  EXPECT_EQ(
    occtl_topo_remove_with_replacement(nullptr, OCCTL_NODE_ID_INVALID, OCCTL_NODE_ID_INVALID),
    OCCTL_INVALID_ARGUMENT);
}

TEST(GraphLifecycleTest, RemoveRef_InvalidRefId_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  EXPECT_EQ(occtl_topo_remove_ref(aGraph, OCCTL_REF_ID_INVALID), OCCTL_NOT_FOUND);
  occtl_graph_free(aGraph);
}

TEST(GraphLifecycleTest, RemoveRep_InvalidRepId_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  EXPECT_EQ(occtl_topo_remove_rep(aGraph, OCCTL_REP_ID_INVALID), OCCTL_NOT_FOUND);
  occtl_graph_free(aGraph);
}

TEST(GraphLifecycleTest, CleanupRemovedRefs_Idempotent_ReturnsOk)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  EXPECT_EQ(occtl_topo_cleanup_removed_refs(aGraph), OCCTL_OK);
  EXPECT_EQ(occtl_topo_cleanup_removed_refs(aGraph), OCCTL_OK);
  occtl_graph_free(aGraph);
}

} // namespace
