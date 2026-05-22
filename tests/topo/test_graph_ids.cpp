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

#include "test_helpers_internal.hxx"

#include <array>
#include <cstring>
#include <vector>

namespace
{

TEST(GraphIdsTest, NodeId_InvalidSentinel_BitsZero)
{
  EXPECT_EQ(OCCTL_NODE_ID_INVALID.bits, 0ull);
}

TEST(GraphIdsTest, RefId_InvalidSentinel_BitsZero)
{
  EXPECT_EQ(OCCTL_REF_ID_INVALID.bits, 0ull);
}

TEST(GraphIdsTest, RefUid_InvalidSentinel_BitsZero)
{
  EXPECT_EQ(OCCTL_REF_UID_INVALID.bits, 0ull);
}

TEST(GraphIdsTest, RepId_InvalidSentinel_BitsZero)
{
  EXPECT_EQ(OCCTL_REP_ID_INVALID.bits, 0ull);
}

TEST(GraphIdsTest, Uid_InvalidSentinel_BitsZero)
{
  EXPECT_EQ(OCCTL_UID_INVALID.bits, 0ull);
}

TEST(GraphIdsTest, Uid_FromInvalidNodeId_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_uid_t aUid = OCCTL_UID_INVALID;
  EXPECT_EQ(occtl_graph_uid_from_node_id(aGraph, OCCTL_NODE_ID_INVALID, &aUid), OCCTL_NOT_FOUND);
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_NE(anErr->message, nullptr);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, NodeId_FromInvalidUid_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_node_id_t aNodeId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_graph_node_id_from_uid(aGraph, OCCTL_UID_INVALID, &aNodeId), OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, NodeId_FromGarbageNonZeroUid_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  // Construct a non-zero UID that doesn't exist in the empty graph.
  // Bits[63:56] = OCCTL_KIND_FACE, payload = counter 42.
  occtl_uid_t     aGarbage{(static_cast<uint64_t>(OCCTL_KIND_FACE) << 56) | 42ull};
  occtl_node_id_t aNodeId = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_graph_node_id_from_uid(aGraph, aGarbage, &aNodeId), OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, GraphNodeIdKind_InvalidId_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  EXPECT_EQ(occtl_graph_node_kind(aGraph, OCCTL_NODE_ID_INVALID, &aKind), OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, GraphUidKind_InvalidId_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  EXPECT_EQ(occtl_graph_uid_kind(aGraph, OCCTL_UID_INVALID, &aKind), OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, GraphRefIdKind_InvalidId_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_ref_kind_t aKind = OCCTL_REF_KIND_INVALID;
  EXPECT_EQ(occtl_graph_ref_kind(aGraph, OCCTL_REF_ID_INVALID, &aKind), OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, GraphRefUidKind_InvalidId_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_ref_kind_t aKind = OCCTL_REF_KIND_INVALID;
  EXPECT_EQ(occtl_graph_ref_uid_kind(aGraph, OCCTL_REF_UID_INVALID, &aKind), OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, GraphRepIdKind_InvalidId_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_rep_kind_t aKind = OCCTL_REP_KIND_INVALID;
  EXPECT_EQ(occtl_graph_rep_kind(aGraph, OCCTL_REP_ID_INVALID, &aKind), OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

// -- UID wire format --------------------------------------------------------

TEST(GraphIdsTest, UidToBytes_NullOutBytes_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_uid_to_bytes(OCCTL_UID_INVALID, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GraphIdsTest, RefUidToBytes_NullOutBytes_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_ref_uid_to_bytes(OCCTL_REF_UID_INVALID, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GraphIdsTest, UidFromBytes_NullArg_ReturnsInvalidArgument)
{
  uint8_t     aBuf[OCCTL_UID_WIRE_SIZE] = {0};
  occtl_uid_t aUid;
  EXPECT_EQ(occtl_uid_from_bytes(nullptr, &aUid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_uid_from_bytes(aBuf, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GraphIdsTest, RefUidFromBytes_NullArg_ReturnsInvalidArgument)
{
  uint8_t         aBuf[OCCTL_REF_UID_WIRE_SIZE] = {0};
  occtl_ref_uid_t aUid                          = OCCTL_REF_UID_INVALID;
  EXPECT_EQ(occtl_ref_uid_from_bytes(nullptr, &aUid), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_ref_uid_from_bytes(aBuf, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GraphIdsTest, UidWireFormat_InvalidRoundTrip_ZeroBytes)
{
  uint8_t aBuf[OCCTL_UID_WIRE_SIZE];
  ASSERT_EQ(occtl_uid_to_bytes(OCCTL_UID_INVALID, aBuf), OCCTL_OK);
  for (unsigned i = 0; i < OCCTL_UID_WIRE_SIZE; ++i)
  {
    EXPECT_EQ(aBuf[i], 0u);
  }
  occtl_uid_t aBack;
  ASSERT_EQ(occtl_uid_from_bytes(aBuf, &aBack), OCCTL_OK);
  EXPECT_EQ(aBack.bits, 0ull);
}

TEST(GraphIdsTest, RefUidWireFormat_InvalidRoundTrip_ZeroBytes)
{
  uint8_t aBuf[OCCTL_REF_UID_WIRE_SIZE];
  ASSERT_EQ(occtl_ref_uid_to_bytes(OCCTL_REF_UID_INVALID, aBuf), OCCTL_OK);
  for (unsigned anIndex = 0; anIndex < OCCTL_REF_UID_WIRE_SIZE; ++anIndex)
  {
    EXPECT_EQ(aBuf[anIndex], 0u);
  }

  occtl_ref_uid_t aBack = OCCTL_REF_UID_INVALID;
  ASSERT_EQ(occtl_ref_uid_from_bytes(aBuf, &aBack), OCCTL_OK);
  EXPECT_EQ(aBack.bits, 0ull);
}

TEST(GraphIdsTest, UidFromBytes_NonZeroReserved_ReturnsFormatError)
{
  uint8_t aBuf[OCCTL_UID_WIRE_SIZE] = {0};
  aBuf[8]                           = 0x42; // reserved byte
  occtl_uid_t aUid;
  EXPECT_EQ(occtl_uid_from_bytes(aBuf, &aUid), OCCTL_FORMAT_ERROR);
}

TEST(GraphIdsTest, RefUidFromBytes_NonZeroReserved_ReturnsFormatError)
{
  uint8_t aBuf[OCCTL_REF_UID_WIRE_SIZE] = {0};
  aBuf[8]                               = 0x42;
  occtl_ref_uid_t aUid                  = OCCTL_REF_UID_INVALID;
  EXPECT_EQ(occtl_ref_uid_from_bytes(aBuf, &aUid), OCCTL_FORMAT_ERROR);
}

TEST(GraphIdsTest, UidWireFormat_RoundTripsBoxUids)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  loadBox(aGraph);

  // Sizing call.
  size_t aCount = 0;
  ASSERT_EQ(occtl_graph_uid_table(aGraph, nullptr, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  std::vector<occtl_uid_t>     aUids(aCount);
  std::vector<occtl_node_id_t> aNodes(aCount);
  size_t                       aCount2 = 0;
  ASSERT_EQ(occtl_graph_uid_table(aGraph, aUids.data(), aNodes.data(), aCount, &aCount2), OCCTL_OK);
  EXPECT_EQ(aCount2, aCount);

  // Round-trip every UID through the wire format.
  for (size_t i = 0; i < aCount; ++i)
  {
    uint8_t aBuf[OCCTL_UID_WIRE_SIZE];
    ASSERT_EQ(occtl_uid_to_bytes(aUids[i], aBuf), OCCTL_OK);
    occtl_uid_t aBack;
    ASSERT_EQ(occtl_uid_from_bytes(aBuf, &aBack), OCCTL_OK);
    EXPECT_EQ(aBack.bits, aUids[i].bits);
  }

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, UidTable_BufferTooSmall_ReturnsBufferTooSmall)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  loadBox(aGraph);

  size_t aCount = 0;
  ASSERT_EQ(occtl_graph_uid_table(aGraph, nullptr, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_GT(aCount, 1u);

  std::vector<occtl_uid_t>     aUids(1);
  std::vector<occtl_node_id_t> aNodes(1);
  size_t                       aCount2 = 0;
  EXPECT_EQ(occtl_graph_uid_table(aGraph, aUids.data(), aNodes.data(), 1, &aCount2),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_EQ(aCount2, aCount);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, RefUidTable_RoundTripsBoxRefs)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  loadBox(aGraph);

  size_t aCount = 0;
  ASSERT_EQ(occtl_graph_ref_uid_table(aGraph, nullptr, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  std::vector<occtl_ref_uid_t> aUids(aCount);
  std::vector<occtl_ref_id_t>  aRefs(aCount);
  size_t                       aCount2 = 0;
  ASSERT_EQ(occtl_graph_ref_uid_table(aGraph, aUids.data(), aRefs.data(), aCount, &aCount2),
            OCCTL_OK);
  EXPECT_EQ(aCount2, aCount);

  for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
  {
    occtl_ref_uid_t aUid = OCCTL_REF_UID_INVALID;
    ASSERT_EQ(occtl_graph_ref_uid_from_ref_id(aGraph, aRefs[anIndex], &aUid), OCCTL_OK);
    EXPECT_EQ(aUid.bits, aUids[anIndex].bits);

    uint8_t aBytes[OCCTL_REF_UID_WIRE_SIZE] = {0};
    ASSERT_EQ(occtl_ref_uid_to_bytes(aUid, aBytes), OCCTL_OK);
    occtl_ref_uid_t aBack = OCCTL_REF_UID_INVALID;
    ASSERT_EQ(occtl_ref_uid_from_bytes(aBytes, &aBack), OCCTL_OK);
    EXPECT_EQ(aBack.bits, aUid.bits);

    occtl_ref_id_t aRef = OCCTL_REF_ID_INVALID;
    ASSERT_EQ(occtl_graph_ref_id_from_ref_uid(aGraph, aUids[anIndex], &aRef), OCCTL_OK);
    EXPECT_EQ(aRef.bits, aRefs[anIndex].bits);

    occtl_ref_kind_t aKindFromId  = OCCTL_REF_KIND_INVALID;
    occtl_ref_kind_t aKindFromUid = OCCTL_REF_KIND_INVALID;
    ASSERT_EQ(occtl_graph_ref_kind(aGraph, aRefs[anIndex], &aKindFromId), OCCTL_OK);
    ASSERT_EQ(occtl_graph_ref_uid_kind(aGraph, aUids[anIndex], &aKindFromUid), OCCTL_OK);
    EXPECT_EQ(aKindFromUid, aKindFromId);
  }

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, RefUidTable_InvalidArgs_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  loadBox(aGraph);

  size_t          aCount   = 0;
  occtl_ref_uid_t aUids[1] = {};
  occtl_ref_id_t  aRefs[1] = {};
  EXPECT_EQ(occtl_graph_ref_uid_table(nullptr, nullptr, nullptr, 0, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_ref_uid_table(aGraph, aUids, nullptr, 1, &aCount), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_ref_uid_table(aGraph, nullptr, aRefs, 1, &aCount), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_graph_ref_uid_table(aGraph, nullptr, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, RefUidTable_BufferTooSmall_ReturnsBufferTooSmall)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  loadBox(aGraph);

  size_t aCount = 0;
  ASSERT_EQ(occtl_graph_ref_uid_table(aGraph, nullptr, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_GT(aCount, 1u);

  occtl_ref_uid_t aUids[1] = {};
  occtl_ref_id_t  aRefs[1] = {};
  EXPECT_EQ(occtl_graph_ref_uid_table(aGraph, aUids, aRefs, 1, &aCount), OCCTL_BUFFER_TOO_SMALL);

  occtl_graph_free(aGraph);
}

TEST(GraphIdsTest, UidBytes_SurviveCompact)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  loadBox(aGraph);

  // Snapshot every UID byte-encoded.
  size_t aCount = 0;
  ASSERT_EQ(occtl_graph_uid_table(aGraph, nullptr, nullptr, 0, &aCount), OCCTL_OK);
  std::vector<occtl_uid_t>     aUidsBefore(aCount);
  std::vector<occtl_node_id_t> aNodesBefore(aCount);
  size_t                       aTmp = 0;
  ASSERT_EQ(occtl_graph_uid_table(aGraph, aUidsBefore.data(), aNodesBefore.data(), aCount, &aTmp),
            OCCTL_OK);

  std::vector<std::array<uint8_t, OCCTL_UID_WIRE_SIZE>> aBytesBefore(aCount);
  for (size_t i = 0; i < aCount; ++i)
  {
    ASSERT_EQ(occtl_uid_to_bytes(aUidsBefore[i], aBytesBefore[i].data()), OCCTL_OK);
  }

  ASSERT_EQ(occtl_graph_compact(aGraph), OCCTL_OK);

  // After compact: NodeIds invalidated, but UIDs should resolve to a new NodeId.
  for (size_t i = 0; i < aCount; ++i)
  {
    occtl_uid_t aBack;
    ASSERT_EQ(occtl_uid_from_bytes(aBytesBefore[i].data(), &aBack), OCCTL_OK);
    EXPECT_EQ(aBack.bits, aUidsBefore[i].bits);
    occtl_node_id_t aNewNode;
    EXPECT_EQ(occtl_graph_node_id_from_uid(aGraph, aBack, &aNewNode), OCCTL_OK);
    EXPECT_NE(aNewNode.bits, 0u);
  }

  occtl_graph_free(aGraph);
}

} // namespace
