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

#include "test_bool_helpers.hxx"

#include <occtl-hpp/bool.hpp>
#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <vector>

namespace
{

class BoolVeneerTest : public ::testing::Test
{
protected:
  occtl::Graph make_graph()
  {
    ::occtl_graph_t* aRaw = nullptr;
    occtl::check(::occtl_graph_create(&aRaw));
    return occtl::Graph(aRaw);
  }
};

TEST_F(BoolVeneerTest, FuseTwoBoxes_ReturnsResult)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aA(bool_test::makeBoxAt(aGraph.get(), 0.0, 0.0, 0.0, 10.0, 10.0, 10.0));
  const occtl::NodeId aB(bool_test::makeBoxAt(aGraph.get(), 5.0, 0.0, 0.0, 10.0, 10.0, 10.0));

  const occtl::NodeId aRoot =
    occtl::bool_::fuse(aGraph, std::vector<occtl::NodeId>{aA}, std::vector<occtl::NodeId>{aB});
  EXPECT_TRUE(aRoot.is_valid());
  EXPECT_NO_THROW({ (void)aGraph.history_deleted_all(); });
}

TEST_F(BoolVeneerTest, FuseWithEmptyObjects_ThrowsError)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aB(bool_test::makeBoxAt(aGraph.get(), 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
  EXPECT_THROW(occtl::bool_::fuse(aGraph, {}, std::vector<occtl::NodeId>{aB}), occtl::Error);
}

TEST_F(BoolVeneerTest, GraphHistory_IsQueriedFromGraph)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aA(bool_test::makeBoxAt(aGraph.get(), 0.0, 0.0, 0.0, 10.0, 10.0, 10.0));
  const occtl::NodeId aB(bool_test::makeBoxAt(aGraph.get(), 5.0, 0.0, 0.0, 10.0, 10.0, 10.0));
  const occtl::NodeId aRoot =
    occtl::bool_::fuse(aGraph, std::vector<occtl::NodeId>{aA}, std::vector<occtl::NodeId>{aB});
  ASSERT_TRUE(aRoot.is_valid());
  EXPECT_NO_THROW({ (void)aGraph.history_deleted_all(); });
}

} // namespace
