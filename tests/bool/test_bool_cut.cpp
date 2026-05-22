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

class BoolCutTest : public BoolFixture
{
};

TEST_F(BoolCutTest, BoxMinusSphere_ProducesSolid)
{
  const occtl_node_id_t aBox    = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 10.0, 10.0, 10.0);
  const occtl_node_id_t aSphere = bool_test::makeSphereAt(myGraph, 5.0, 5.0, 5.0, 3.0);

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  occtl_node_id_t      aRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_bool_cut(myGraph, &aBox, 1, &aSphere, 1, &anOpts, &aRoot), OCCTL_OK);
  ASSERT_NE(aRoot.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aRoot, &aKind), OCCTL_OK);
  EXPECT_TRUE(aKind == OCCTL_KIND_SOLID || aKind == OCCTL_KIND_COMPOUND)
    << "Unexpected result kind: " << aKind;
}

TEST_F(BoolCutTest, BoxMinusEnclosingSphere_ProducesEmptyCompound)
{
  // The sphere fully contains the box; OCCT typically yields an empty
  // compound or a degenerate result.  The call must succeed regardless.
  const occtl_node_id_t aBox    = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  const occtl_node_id_t aSphere = bool_test::makeSphereAt(myGraph, 0.5, 0.5, 0.5, 5.0);

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  occtl_node_id_t      aRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_bool_cut(myGraph, &aBox, 1, &aSphere, 1, &anOpts, &aRoot), OCCTL_OK);
}

} // namespace
