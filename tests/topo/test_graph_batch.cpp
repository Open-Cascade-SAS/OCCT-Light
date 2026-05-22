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

#include <gtest/gtest.h>

#include <BRepGraph.hxx>

namespace
{

class TopoBatchTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);
    ASSERT_NE(myGraph, nullptr);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(TopoBatchTest, BeginBatch_ValidGraph_ReturnsOk)
{
  occtl_batch_t* aBatch = nullptr;
  ASSERT_EQ(occtl_graph_begin_batch(myGraph, &aBatch), OCCTL_OK);
  ASSERT_NE(aBatch, nullptr);
  ASSERT_EQ(occtl_batch_commit(aBatch), OCCTL_OK);
}

TEST_F(TopoBatchTest, BeginBatch_NullGraph_InvalidArgument)
{
  occtl_batch_t* aBatch = nullptr;
  EXPECT_EQ(occtl_graph_begin_batch(nullptr, &aBatch), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoBatchTest, BeginBatch_NullOut_InvalidArgument)
{
  EXPECT_EQ(occtl_graph_begin_batch(myGraph, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_NE(occtl_error_last()->status, OCCTL_OK);
}

TEST_F(TopoBatchTest, Commit_NullBatch_Ok)
{
  EXPECT_EQ(occtl_batch_commit(nullptr), OCCTL_OK);
}

TEST_F(TopoBatchTest, Abort_NullBatch_Ok)
{
  EXPECT_EQ(occtl_batch_abort(nullptr), OCCTL_OK);
}

TEST_F(TopoBatchTest, Commit_FreesBatch)
{
  occtl_batch_t* aBatch = nullptr;
  ASSERT_EQ(occtl_graph_begin_batch(myGraph, &aBatch), OCCTL_OK);
  ASSERT_NE(aBatch, nullptr);
  EXPECT_EQ(occtl_batch_commit(aBatch), OCCTL_OK);
}

TEST_F(TopoBatchTest, Abort_FreesBatch)
{
  occtl_batch_t* aBatch = nullptr;
  ASSERT_EQ(occtl_graph_begin_batch(myGraph, &aBatch), OCCTL_OK);
  ASSERT_NE(aBatch, nullptr);
  EXPECT_EQ(occtl_batch_abort(aBatch), OCCTL_OK);
}

} // anonymous namespace
