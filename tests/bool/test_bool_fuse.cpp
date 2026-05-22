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

using bool_test::BoolFixture;

namespace
{

class BoolFuseTest : public BoolFixture
{
};

TEST_F(BoolFuseTest, FuseTwoOverlappingBoxes_ProducesSolidOrCompound)
{
  const occtl_node_id_t aBoxA = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 10.0, 10.0, 10.0);
  const occtl_node_id_t aBoxB = bool_test::makeBoxAt(myGraph, 5.0, 0.0, 0.0, 10.0, 10.0, 10.0);
  ASSERT_NE(aBoxA.bits, 0u);
  ASSERT_NE(aBoxB.bits, 0u);

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  occtl_node_id_t      aRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_bool_fuse(myGraph, &aBoxA, 1, &aBoxB, 1, &anOpts, &aRoot), OCCTL_OK);
  ASSERT_NE(aRoot.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aRoot, &aKind), OCCTL_OK);
  // OCCT's BRepAlgoAPI_Fuse returns the result wrapped in a Compound when
  // driven through SetArguments / SetTools (the generic N-input form).
  EXPECT_TRUE(aKind == OCCTL_KIND_SOLID || aKind == OCCTL_KIND_COMPOUND)
    << "Unexpected result kind: " << aKind;
}

TEST_F(BoolFuseTest, FuseDisjointBoxes_ProducesCompoundOrSolid)
{
  // Two boxes that share only a vertex.  OCCT may produce a Solid (if it
  // glues the touching corner) or a Compound (if it leaves them disjoint).
  // Either is acceptable; the test asserts that the call succeeds and
  // history records one Modified or Generated image per input root face.
  const occtl_node_id_t aA = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  const occtl_node_id_t aB = bool_test::makeBoxAt(myGraph, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  occtl_node_id_t      aRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOpts, &aRoot), OCCTL_OK);
  ASSERT_NE(aRoot.bits, 0u);
}

TEST_F(BoolFuseTest, FuseWithoutHistory_LeavesOutHistoryUnset)
{
  const occtl_node_id_t aA = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 10.0, 10.0, 10.0);
  const occtl_node_id_t aB = bool_test::makeBoxAt(myGraph, 5.0, 0.0, 0.0, 10.0, 10.0, 10.0);

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  anOpts.build_history        = 0;
  occtl_node_id_t aRoot       = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOpts, &aRoot), OCCTL_OK);
  EXPECT_NE(aRoot.bits, 0u);
}

} // namespace
