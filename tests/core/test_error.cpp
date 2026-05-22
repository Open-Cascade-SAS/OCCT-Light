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

#include <gtest/gtest.h>

#include <cstring>

namespace
{

TEST(ErrorTest, Last_OnFreshThread_StatusIsOk)
{
  occtl_error_clear();
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_EQ(anErr->status, OCCTL_OK);
  EXPECT_NE(anErr->message, nullptr);
}

TEST(ErrorTest, Clear_AfterFailure_ResetsStatus)
{
  occtl_runtime_init_info_t anInfo = OCCTL_RUNTIME_INIT_INFO_INIT;
  anInfo.struct_version            = 0xdeadbeefu;
  occtl_status_t aStatus           = occtl_runtime_init(&anInfo);
  EXPECT_EQ(aStatus, OCCTL_VERSION_MISMATCH);

  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_EQ(anErr->status, OCCTL_VERSION_MISMATCH);
  ASSERT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);

  occtl_error_clear();
  anErr = occtl_error_last();
  EXPECT_EQ(anErr->status, OCCTL_OK);
}

TEST(ErrorTest, NullInfoToInfoInit_ReturnsInvalidArgument)
{
  occtl_status_t aStatus = occtl_runtime_init_info_init(nullptr);
  EXPECT_EQ(aStatus, OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_EQ(anErr->status, OCCTL_INVALID_ARGUMENT);
  ASSERT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

} // namespace
