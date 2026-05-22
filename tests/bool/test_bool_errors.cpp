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

#include <cstring>
#include <limits>

using bool_test::BoolFixture;

namespace
{

class BoolErrorsTest : public BoolFixture
{
};

TEST_F(BoolErrorsTest, NullArguments_ReturnInvalidArgument)
{
  occtl_node_id_t      aA     = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t      aB     = bool_test::makeBoxAt(myGraph, 0.5, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  occtl_node_id_t      aRoot  = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_bool_fuse(nullptr, &aA, 1, &aB, 1, &anOpts, &aRoot), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, nullptr, &aRoot), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOpts, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(BoolErrorsTest, EmptyInputLists_ReturnInvalidArgument)
{
  occtl_node_id_t      aA     = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  occtl_node_id_t      aRoot  = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_bool_fuse(myGraph, nullptr, 0, &aA, 1, &anOpts, &aRoot), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, nullptr, 0, &anOpts, &aRoot), OCCTL_INVALID_ARGUMENT);
}

TEST_F(BoolErrorsTest, VersionMismatch_ReturnsVersionMismatch)
{
  occtl_node_id_t aA = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aB = bool_test::makeBoxAt(myGraph, 0.5, 0.0, 0.0, 1.0, 1.0, 1.0);

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  anOpts.struct_version       = 0u;
  occtl_node_id_t aRoot       = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOpts, &aRoot), OCCTL_VERSION_MISMATCH);

  anOpts.struct_version = 0xdeadbeefu;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOpts, &aRoot), OCCTL_VERSION_MISMATCH);
}

TEST_F(BoolErrorsTest, OptionsPNextMustBeNull_ReturnsInvalidArgument)
{
  occtl_node_id_t aA = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aB = bool_test::makeBoxAt(myGraph, 0.5, 0.0, 0.0, 1.0, 1.0, 1.0);

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  anOpts.p_next               = &anOpts;
  occtl_node_id_t aRoot       = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOpts, &aRoot), OCCTL_INVALID_ARGUMENT);
}

TEST_F(BoolErrorsTest, OptionsFlagsMustBeZeroOrOne_ReturnsInvalidArgument)
{
  occtl_node_id_t aA    = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aB    = bool_test::makeBoxAt(myGraph, 0.5, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aRoot = OCCTL_NODE_ID_INVALID;

  occtl_bool_options_t anOptsRunParallel = OCCTL_BOOL_OPTIONS_INIT;
  anOptsRunParallel.run_parallel         = 2;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOptsRunParallel, &aRoot),
            OCCTL_INVALID_ARGUMENT);

  occtl_bool_options_t anOptsSimplify = OCCTL_BOOL_OPTIONS_INIT;
  anOptsSimplify.simplify_result      = -1;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOptsSimplify, &aRoot),
            OCCTL_INVALID_ARGUMENT);

  occtl_bool_options_t anOptsHistory = OCCTL_BOOL_OPTIONS_INIT;
  anOptsHistory.build_history        = 5;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOptsHistory, &aRoot),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(BoolErrorsTest, OptionsNumericValidation_ReturnsInvalidArgument)
{
  occtl_node_id_t aA    = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aB    = bool_test::makeBoxAt(myGraph, 0.5, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aRoot = OCCTL_NODE_ID_INVALID;

  occtl_bool_options_t anOptsNegativeFuzzy = OCCTL_BOOL_OPTIONS_INIT;
  anOptsNegativeFuzzy.fuzzy_value          = -1.0;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOptsNegativeFuzzy, &aRoot),
            OCCTL_INVALID_ARGUMENT);

  occtl_bool_options_t anOptsInfiniteFuzzy = OCCTL_BOOL_OPTIONS_INIT;
  anOptsInfiniteFuzzy.fuzzy_value          = std::numeric_limits<double>::infinity();
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOptsInfiniteFuzzy, &aRoot),
            OCCTL_INVALID_ARGUMENT);

  occtl_bool_options_t anOptsBadAngular       = OCCTL_BOOL_OPTIONS_INIT;
  anOptsBadAngular.simplify_result            = 1;
  anOptsBadAngular.simplify_angular_tolerance = 0.0;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOptsBadAngular, &aRoot),
            OCCTL_INVALID_ARGUMENT);

  occtl_bool_options_t anOptsNanAngular       = OCCTL_BOOL_OPTIONS_INIT;
  anOptsNanAngular.simplify_result            = 1;
  anOptsNanAngular.simplify_angular_tolerance = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOptsNanAngular, &aRoot),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(BoolErrorsTest, BuildHistoryDisabled_ReturnsRootWithoutHistoryFailure)
{
  occtl_node_id_t aA = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aB = bool_test::makeBoxAt(myGraph, 0.5, 0.0, 0.0, 1.0, 1.0, 1.0);

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  anOpts.build_history        = 0;
  occtl_node_id_t aRoot       = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aB, 1, &anOpts, &aRoot), OCCTL_OK);
  EXPECT_NE(aRoot.bits, 0u);
}

TEST_F(BoolErrorsTest, InvalidInputNodeId_ReturnsNotFound)
{
  occtl_node_id_t aA = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_node_id_t aBad{0x12345678ull};

  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  occtl_node_id_t      aRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_bool_fuse(myGraph, &aA, 1, &aBad, 1, &anOpts, &aRoot), OCCTL_NOT_FOUND);
}

TEST_F(BoolErrorsTest, ErrorState_PopulatedAfterFailure)
{
  occtl_node_id_t      aA     = bool_test::makeBoxAt(myGraph, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  occtl_bool_options_t anOpts = OCCTL_BOOL_OPTIONS_INIT;
  occtl_node_id_t      aRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_bool_fuse(nullptr, &aA, 1, &aA, 1, &anOpts, &aRoot), OCCTL_INVALID_ARGUMENT);
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_NE(anErr->status, OCCTL_OK);
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_NE(std::strlen(anErr->message), 0u);
}

} // namespace
