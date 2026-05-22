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

#include "test_heal_helpers.hxx"

using heal_test::HealFixture;

namespace
{

class HealBasicTest : public HealFixture
{
};

TEST_F(HealBasicTest, StandardMode_SucceedsOnBox)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 10.0, 20.0, 30.0);

  occtl_heal_options_t anOpts = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.mode                 = OCCTL_HEAL_MODE_STANDARD;

  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_OK);

  // The healed shape was ingested back — we should see more nodes
  EXPECT_GT(occtl_graph_count_value(occtl_graph_solid_count, myGraph), 1u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, myGraph), 6u);
}

TEST_F(HealBasicTest, DefaultOptions_SucceedsOnBox)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, nullptr), OCCTL_OK);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_solid_count, myGraph), 1u);
}

TEST_F(HealBasicTest, BasicMode_SucceedsOnBox)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 5.0, 5.0, 5.0);

  occtl_heal_options_t anOpts = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.mode                 = OCCTL_HEAL_MODE_BASIC;

  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_OK);
}

TEST_F(HealBasicTest, FullMode_SucceedsOnBox)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 3.0, 3.0, 3.0);

  occtl_heal_options_t anOpts = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.mode                 = OCCTL_HEAL_MODE_FULL;

  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_OK);
}

TEST_F(HealBasicTest, CustomTolerance_Succeeds)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

  occtl_heal_options_t anOpts = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.tolerance            = 1e-4;

  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_OK);
}

TEST_F(HealBasicTest, UnifySameDomain_DefaultOptions_SucceedsOnBox)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 3.0, 4.0, 5.0);

  occtl_node_id_t aRoot = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_heal_unify_same_domain(myGraph, aSolid, nullptr, &aRoot), OCCTL_OK);
  EXPECT_NE(aRoot.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_solid_count, myGraph), 1u);
}

TEST_F(HealBasicTest, UnifySameDomain_Init_FillsDefaults)
{
  occtl_heal_unify_same_domain_options_t anOpts{};
  occtl_heal_unify_same_domain_options_init(&anOpts);

  EXPECT_EQ(anOpts.struct_version, OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_VERSION_1);
  EXPECT_EQ(anOpts.p_next, nullptr);
  EXPECT_EQ(anOpts.unify_edges, 1);
  EXPECT_EQ(anOpts.unify_faces, 1);
  EXPECT_EQ(anOpts.safe_input, 1);
}

TEST_F(HealBasicTest, UnifySameDomain_NullArguments_ReturnInvalidArgument)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aRoot  = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_heal_unify_same_domain(nullptr, aSolid, nullptr, &aRoot), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_heal_unify_same_domain(myGraph, aSolid, nullptr, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(HealBasicTest, UnifySameDomain_BadVersion_ReturnsVersionMismatch)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

  occtl_heal_unify_same_domain_options_t anOpts = OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_INIT;
  anOpts.struct_version                         = 0u;

  occtl_node_id_t aRoot = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_heal_unify_same_domain(myGraph, aSolid, &anOpts, &aRoot), OCCTL_VERSION_MISMATCH);
}

TEST_F(HealBasicTest, SomeFixesDisabled_Succeeds)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 2.0, 2.0, 2.0);

  occtl_heal_options_t anOpts = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.mode                 = OCCTL_HEAL_MODE_FULL;
  anOpts.fix_same_parameter   = 0;
  anOpts.fix_small_edges      = 0;
  anOpts.fix_face_orient      = 0;

  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_OK);
}

} // namespace
