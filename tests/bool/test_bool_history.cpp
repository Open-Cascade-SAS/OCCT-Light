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

#include <cstdint>
#include <vector>

using bool_test::BoolFixture;

namespace
{

class BoolHistoryTest : public BoolFixture
{
protected:
  // Get one face UID off @p theSolid.  Caller-side pre-fuse capture so the
  // tests can query history.modified(faceUid) after the operation.
  occtl_uid_t firstFaceUidOf(occtl_node_id_t theSolid)
  {
    occtl_node_iter_t* aShellIt = nullptr;
    EXPECT_EQ(occtl_topo_shells_of_solid_iter_create(myGraph, theSolid, &aShellIt), OCCTL_OK);
    occtl_node_id_t aShell = OCCTL_NODE_ID_INVALID;
    EXPECT_EQ(occtl_node_iter_next(aShellIt, &aShell), OCCTL_OK);
    occtl_node_iter_free(aShellIt);

    occtl_node_iter_t* aFaceIt = nullptr;
    EXPECT_EQ(occtl_topo_faces_of_shell_iter_create(myGraph, aShell, &aFaceIt), OCCTL_OK);
    occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
    EXPECT_EQ(occtl_node_iter_next(aFaceIt, &aFace), OCCTL_OK);
    occtl_node_iter_free(aFaceIt);

    occtl_uid_t aUid{0};
    EXPECT_EQ(occtl_graph_uid_from_node_id(myGraph, aFace, &aUid), OCCTL_OK);
    return aUid;
  }

  void buildFuseWithHistory(occtl_uid_t& outFaceUid, occtl_node_id_t& outRoot)
  {
    const occtl_node_id_t aA = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 10.0, 10.0, 10.0);
    const occtl_node_id_t aB = bool_test::makeBoxAt(myGraph, 5.0, 0.0, 0.0, 10.0, 10.0, 10.0);
    outFaceUid               = firstFaceUidOf(aA);

    occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
    outRoot                     = OCCTL_NODE_ID_INVALID;
    ASSERT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOpts, &outRoot), OCCTL_OK);
  }
};

TEST_F(BoolHistoryTest, ModifiedAccessor_SizingThenFill)
{
  occtl_uid_t     aFaceUid{0};
  occtl_node_id_t aRoot;
  buildFuseWithHistory(aFaceUid, aRoot);

  std::size_t aCount = 9999;
  ASSERT_EQ(occtl_graph_history_modified(myGraph, aFaceUid, nullptr, 0, &aCount), OCCTL_OK);

  std::vector<occtl_uid_t> aBuf(aCount);
  if (aCount > 0)
  {
    std::size_t aRefill = 0;
    ASSERT_EQ(occtl_graph_history_modified(myGraph, aFaceUid, aBuf.data(), aBuf.size(), &aRefill),
              OCCTL_OK);
    EXPECT_EQ(aRefill, aCount);
  }
}

TEST_F(BoolHistoryTest, UnknownUid_ReturnsNotFound)
{
  occtl_uid_t     aFaceUid{0};
  occtl_node_id_t aRoot;
  buildFuseWithHistory(aFaceUid, aRoot);

  // A UID with a kind tag (Face) and a counter that is unlikely to have
  // been allocated yet.  This must surface as OCCTL_NOT_FOUND.
  occtl_uid_t aBogus{(static_cast<std::uint64_t>(OCCTL_KIND_FACE) << 56) | 0xFFFFFEull};
  std::size_t aCount = 0;
  EXPECT_EQ(occtl_graph_history_modified(myGraph, aBogus, nullptr, 0, &aCount), OCCTL_NOT_FOUND);
}

TEST_F(BoolHistoryTest, BufferTooSmall_ReturnsBufferTooSmall)
{
  occtl_uid_t     aFaceUid{0};
  occtl_node_id_t aRoot;
  buildFuseWithHistory(aFaceUid, aRoot);

  std::size_t aCount = 0;
  ASSERT_EQ(occtl_graph_history_modified(myGraph, aFaceUid, nullptr, 0, &aCount), OCCTL_OK);
  if (aCount >= 1)
  {
    std::vector<occtl_uid_t> aTooSmall(aCount == 0 ? 0 : aCount - 1);
    std::size_t              aWritten = 0;
    if (!aTooSmall.empty())
    {
      EXPECT_EQ(occtl_graph_history_modified(myGraph,
                                             aFaceUid,
                                             aTooSmall.data(),
                                             aTooSmall.size(),
                                             &aWritten),
                OCCTL_BUFFER_TOO_SMALL);
      EXPECT_EQ(aWritten, aCount);
    }
  }
}

TEST_F(BoolHistoryTest, DeletedAccessor_SizingWorks)
{
  occtl_uid_t     aFaceUid{0};
  occtl_node_id_t aRoot;
  buildFuseWithHistory(aFaceUid, aRoot);

  std::size_t aCount = 0;
  ASSERT_EQ(occtl_graph_history_deleted_all(myGraph, nullptr, 0, &aCount), OCCTL_OK);
  std::vector<occtl_uid_t> aBuf(aCount);
  std::size_t              aRefill = 0;
  if (aCount > 0)
  {
    ASSERT_EQ(occtl_graph_history_deleted_all(myGraph, aBuf.data(), aBuf.size(), &aRefill),
              OCCTL_OK);
    EXPECT_EQ(aRefill, aCount);
  }
  else
  {
    ASSERT_EQ(occtl_graph_history_deleted_all(myGraph, aBuf.data(), 0, &aRefill), OCCTL_OK);
  }
}

TEST_F(BoolHistoryTest, NullHistory_ReturnsInvalidArgument)
{
  occtl_uid_t aBogus{0};
  std::size_t aCount = 0;
  EXPECT_EQ(occtl_graph_history_modified(nullptr, aBogus, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_history_deleted_all(nullptr, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT); // must not crash
}

} // namespace
