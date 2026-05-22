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

#include "Runtime.hxx"

#include "ErrorState.hxx"

namespace OcctL::Core
{

//==================================================================================================

Runtime::Runtime() noexcept
    : myInitialised(false)
{
}

//==================================================================================================

Runtime& Runtime::Instance() noexcept
{
  static Runtime THE_INSTANCE;
  return THE_INSTANCE;
}

//==================================================================================================

occtl_status_t Runtime::Initialize(const occtl_runtime_init_info_t* theInfo) noexcept
{
  if (myInitialised)
  {
    ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "occtl_runtime_init: already initialised");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (theInfo != nullptr)
  {
    if (theInfo->struct_version != OCCTL_RUNTIME_INIT_INFO_VERSION_1)
    {
      ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                "occtl_runtime_init: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                "occtl_runtime_init: p_next must be NULL in v1");
      return OCCTL_INVALID_ARGUMENT;
    }
  }

  myInitialised = true;
  return OCCTL_OK;
}

//==================================================================================================

void Runtime::Shutdown() noexcept
{
  myInitialised = false;
}

} // namespace OcctL::Core
