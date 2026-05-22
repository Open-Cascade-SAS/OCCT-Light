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

#include <occtl-hpp/core.hpp>
#include <occtl/occtl_core.h>

#include <gtest/gtest.h>

#include <cstring>

namespace
{

TEST(RuntimeTest, Version_MatchesCompileTimeMacros)
{
  uint32_t aMajor = 0, aMinor = 0, aPatch = 0;
  occtl_runtime_version(&aMajor, &aMinor, &aPatch);
  EXPECT_EQ(aMajor, static_cast<uint32_t>(OCCTL_VERSION_MAJOR));
  EXPECT_EQ(aMinor, static_cast<uint32_t>(OCCTL_VERSION_MINOR));
  EXPECT_EQ(aPatch, static_cast<uint32_t>(OCCTL_VERSION_PATCH));
}

TEST(RuntimeTest, Version_NullOuts_DoesNotCrash)
{
  occtl_runtime_version(nullptr, nullptr, nullptr);
}

TEST(RuntimeTest, AbiVersion_MatchesMacro)
{
  EXPECT_EQ(occtl_runtime_abi_version(), static_cast<uint32_t>(OCCTL_ABI_VERSION));
}

TEST(RuntimeTest, OcctVersion_NonNullAndNonEmpty)
{
  const char* aVer = occtl_runtime_occt_version();
  ASSERT_NE(aVer, nullptr);
  EXPECT_GT(std::strlen(aVer), 0u);
}

TEST(RuntimeTest, InitInfoInit_ProducesDefaultsThatMatchMacro)
{
  occtl_runtime_init_info_t aFromFn;
  ASSERT_EQ(occtl_runtime_init_info_init(&aFromFn), OCCTL_OK);

  occtl_runtime_init_info_t aFromMacro = OCCTL_RUNTIME_INIT_INFO_INIT;
  EXPECT_EQ(aFromFn.struct_version, aFromMacro.struct_version);
  EXPECT_EQ(aFromFn.p_next, aFromMacro.p_next);
}

TEST(RuntimeTest, InitWithBadVersion_ReturnsVersionMismatch)
{
  occtl_runtime_shutdown();
  occtl_runtime_init_info_t anInfo = OCCTL_RUNTIME_INIT_INFO_INIT;
  anInfo.struct_version            = 0xdeadbeefu;
  EXPECT_EQ(occtl_runtime_init(&anInfo), OCCTL_VERSION_MISMATCH);
}

TEST(RuntimeTest, InitWithNonNullPNext_ReturnsInvalidArgument)
{
  occtl_runtime_shutdown();
  occtl_runtime_init_info_t anInfo = OCCTL_RUNTIME_INIT_INFO_INIT;
  int                       aDummy = 0;
  anInfo.p_next                    = &aDummy;
  EXPECT_EQ(occtl_runtime_init(&anInfo), OCCTL_INVALID_ARGUMENT);
}

TEST(RuntimeTest, InitWithDefaults_Succeeds)
{
  occtl_runtime_shutdown();
  EXPECT_EQ(occtl_runtime_init(nullptr), OCCTL_OK);
  occtl_runtime_shutdown();
}

TEST(RuntimeTest, DoubleInit_RejectsSecondCall)
{
  occtl_runtime_shutdown();
  occtl_runtime_init_info_t anInfo = OCCTL_RUNTIME_INIT_INFO_INIT;
  ASSERT_EQ(occtl_runtime_init(&anInfo), OCCTL_OK);
  EXPECT_EQ(occtl_runtime_init(&anInfo), OCCTL_INVALID_ARGUMENT);
  occtl_runtime_shutdown();
}

TEST(RuntimeTest, DoubleInitWithNullInfo_RejectsSecondCall)
{
  occtl_runtime_shutdown();
  ASSERT_EQ(occtl_runtime_init(nullptr), OCCTL_OK);
  EXPECT_EQ(occtl_runtime_init(nullptr), OCCTL_INVALID_ARGUMENT);
  occtl_runtime_shutdown();
}

TEST(VeneerTest, Runtime_RaiiInitAndShutdown)
{
  occtl_runtime_shutdown();
  {
    occtl::Runtime aRt;
    auto [aMajor, aMinor, aPatch] = occtl::version();
    EXPECT_EQ(aMajor, static_cast<uint32_t>(OCCTL_VERSION_MAJOR));
    EXPECT_EQ(aMinor, static_cast<uint32_t>(OCCTL_VERSION_MINOR));
    EXPECT_EQ(aPatch, static_cast<uint32_t>(OCCTL_VERSION_PATCH));
  }
}

TEST(VeneerTest, Check_OnFailure_ThrowsErrorWithCode)
{
  occtl_runtime_shutdown();
  occtl_runtime_init_info_t anInfo = OCCTL_RUNTIME_INIT_INFO_INIT;
  anInfo.struct_version            = 42u;
  try
  {
    occtl::check(occtl_runtime_init(&anInfo));
    FAIL() << "expected occtl::Error to be thrown";
  }
  catch (const occtl::Error& anErr)
  {
    EXPECT_EQ(anErr.code(), OCCTL_VERSION_MISMATCH);
  }
}

TEST(VeneerTest, AbiVersion_MatchesCApi)
{
  EXPECT_EQ(occtl::abi_version(), occtl_runtime_abi_version());
}

} // namespace
