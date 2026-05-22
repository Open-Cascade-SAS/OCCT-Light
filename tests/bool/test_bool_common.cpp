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

class BoolCommonTest : public BoolFixture
{
};

TEST_F(BoolCommonTest, OverlappingBoxes_CommonProducesSolid)
{
  const occtl_node_id_t aA = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 10.0, 10.0, 10.0);
  const occtl_node_id_t aB = bool_test::makeBoxAt(myGraph, 5.0, 5.0, 0.0, 10.0, 10.0, 10.0);

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  occtl_node_id_t      aRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_bool_common(myGraph, &aA, 1, &aB, 1, &anOpts, &aRoot), OCCTL_OK);
  ASSERT_NE(aRoot.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aRoot, &aKind), OCCTL_OK);
  EXPECT_TRUE(aKind == OCCTL_KIND_SOLID || aKind == OCCTL_KIND_COMPOUND)
    << "Unexpected result kind: " << aKind;
}

} // namespace
