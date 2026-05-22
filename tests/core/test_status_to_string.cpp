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

TEST(StatusToStringTest, Ok_ReturnsCanonicalName)
{
  EXPECT_STREQ(occtl_status_to_string(OCCTL_OK), "OCCTL_OK");
}

TEST(StatusToStringTest, EveryDefinedCode_ReturnsNonEmptyString)
{
  const occtl_status_t aCodes[] = {
    OCCTL_OK,
    OCCTL_ERROR,
    OCCTL_INVALID_ARGUMENT,
    OCCTL_INVALID_HANDLE,
    OCCTL_NOT_FOUND,
    OCCTL_OUT_OF_MEMORY,
    OCCTL_OUT_OF_RANGE,
    OCCTL_NOT_DONE,
    OCCTL_GEOMETRY_INVALID,
    OCCTL_TOPOLOGY_INVALID,
    OCCTL_IO_ERROR,
    OCCTL_FORMAT_ERROR,
    OCCTL_UNSUPPORTED,
    OCCTL_CANCELLED,
    OCCTL_BUFFER_TOO_SMALL,
    OCCTL_VERSION_MISMATCH,
    OCCTL_INTERNAL,
  };
  for (occtl_status_t aCode : aCodes)
  {
    const char* aName = occtl_status_to_string(aCode);
    ASSERT_NE(aName, nullptr);
    EXPECT_GT(std::strlen(aName), 0u);
  }
}

TEST(StatusToStringTest, UnknownCode_ReturnsUnknownSentinel)
{
  const occtl_status_t aBogus = static_cast<occtl_status_t>(99999);
  EXPECT_STREQ(occtl_status_to_string(aBogus), "OCCTL_UNKNOWN");
}

TEST(StatusToStringTest, FailedAndSucceededMacros_AgreeWithCode)
{
  EXPECT_TRUE(OCCTL_SUCCEEDED(OCCTL_OK));
  EXPECT_FALSE(OCCTL_FAILED(OCCTL_OK));
  EXPECT_TRUE(OCCTL_FAILED(OCCTL_ERROR));
  EXPECT_FALSE(OCCTL_SUCCEEDED(OCCTL_ERROR));
}

} // namespace
