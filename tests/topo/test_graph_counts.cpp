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

namespace
{

TEST(GraphCountsTest, AllCounts_EmptyGraph_ReturnZero)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_shell_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_wire_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_edge_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_vertex_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_compound_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_compsolid_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_coedge_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_product_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_occurrence_count, aGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_node_count, aGraph), 0u);

  occtl_graph_free(aGraph);
}

TEST(GraphCountsTest, NbNodes_Equals_SumOfPerKind_Empty)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const size_t aSum = occtl_graph_count_value(occtl_graph_solid_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_shell_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_face_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_wire_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_edge_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_vertex_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_compound_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_compsolid_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_coedge_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_product_count, aGraph)
                      + occtl_graph_count_value(occtl_graph_occurrence_count, aGraph);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_node_count, aGraph), aSum);

  occtl_graph_free(aGraph);
}

TEST(GraphCountsTest, Counts_NullGraph_ReturnZero)
{
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, nullptr), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, nullptr), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_node_count, nullptr), 0u);
}

TEST(GraphCountsTest, Counts_AcrossCreateFreeCycle)
{
  for (int aCycle = 0; aCycle < 3; ++aCycle)
  {
    occtl_graph_t* aGraph = nullptr;
    ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
    EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aGraph), 0u);
    EXPECT_EQ(occtl_graph_count_value(occtl_graph_node_count, aGraph), 0u);
    occtl_graph_free(aGraph);
  }
}

} // namespace
