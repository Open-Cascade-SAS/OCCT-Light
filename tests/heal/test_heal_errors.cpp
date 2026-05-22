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

#include <occtl/occtl_core.h>

#include <cstring>
#include <limits>

using heal_test::HealFixture;

namespace
{

class HealErrorsTest : public HealFixture
{
};

TEST_F(HealErrorsTest, NullGraph_ReturnsInvalidArgument)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

  EXPECT_EQ(occtl_heal_shape(nullptr, aSolid, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(HealErrorsTest, InvalidNodeId_ReturnsNotFound)
{
  occtl_node_id_t aBad{0xdeadbeefull};

  EXPECT_EQ(occtl_heal_shape(myGraph, aBad, nullptr), OCCTL_NOT_FOUND);
}

TEST_F(HealErrorsTest, ZeroNodeId_ReturnsNotFound)
{
  occtl_node_id_t aBad = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_heal_shape(myGraph, aBad, nullptr), OCCTL_NOT_FOUND);
}

TEST_F(HealErrorsTest, VersionMismatch_ReturnsVersionMismatch)
{
  occtl_node_id_t      aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_heal_options_t anOpts = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.struct_version       = 0u;

  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_VERSION_MISMATCH);

  anOpts.struct_version = 99u;
  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_VERSION_MISMATCH);
}

TEST_F(HealErrorsTest, HealOptions_InvalidFields_ReturnExpectedStatus)
{
  occtl_node_id_t      aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_heal_options_t anOpts = OCCTL_HEAL_OPTIONS_INIT;

  anOpts.p_next = &anOpts;
  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_INVALID_ARGUMENT);

  anOpts      = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.mode = static_cast<occtl_heal_mode_t>(0x1234);
  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_OUT_OF_RANGE);

  anOpts           = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.tolerance = -1.0;
  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_INVALID_ARGUMENT);

  anOpts           = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.tolerance = std::numeric_limits<double>::infinity();
  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_INVALID_ARGUMENT);

  anOpts                    = OCCTL_HEAL_OPTIONS_INIT;
  anOpts.fix_same_parameter = 2;
  EXPECT_EQ(occtl_heal_shape(myGraph, aSolid, &anOpts), OCCTL_INVALID_ARGUMENT);
}

TEST_F(HealErrorsTest, UnifyOptions_InvalidFields_ReturnExpectedStatus)
{
  occtl_node_id_t aSolid = heal_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aRoot  = OCCTL_NODE_ID_INVALID;

  occtl_heal_unify_same_domain_options_t anOpts = OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_INIT;
  anOpts.p_next                                 = &anOpts;
  EXPECT_EQ(occtl_heal_unify_same_domain(myGraph, aSolid, &anOpts, &aRoot), OCCTL_INVALID_ARGUMENT);

  anOpts             = OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_INIT;
  anOpts.unify_edges = 2;
  EXPECT_EQ(occtl_heal_unify_same_domain(myGraph, aSolid, &anOpts, &aRoot), OCCTL_INVALID_ARGUMENT);

  anOpts                  = OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_INIT;
  anOpts.linear_tolerance = -1.0;
  EXPECT_EQ(occtl_heal_unify_same_domain(myGraph, aSolid, &anOpts, &aRoot), OCCTL_INVALID_ARGUMENT);

  anOpts                   = OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_INIT;
  anOpts.angular_tolerance = std::numeric_limits<double>::infinity();
  EXPECT_EQ(occtl_heal_unify_same_domain(myGraph, aSolid, &anOpts, &aRoot), OCCTL_INVALID_ARGUMENT);
}

TEST_F(HealErrorsTest, ErrorState_PopulatedAfterFailure)
{
  occtl_node_id_t aBad{0xdeadbeefull};

  ASSERT_EQ(occtl_heal_shape(myGraph, aBad, nullptr), OCCTL_NOT_FOUND);

  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_NE(anErr->status, OCCTL_OK);
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_NE(std::strlen(anErr->message), 0u);
}

} // namespace
