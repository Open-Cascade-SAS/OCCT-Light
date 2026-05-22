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

#ifndef OCCTL_CORE_RUNTIME_HXX
#define OCCTL_CORE_RUNTIME_HXX

#include <occtl/occtl_core.h>

namespace OcctL::Core
{

//! Process-wide runtime state. One instance, accessed via Instance().
class Runtime
{
public:
  static Runtime& Instance() noexcept;

  //! Initialises the runtime. Re-initialisation is rejected with
  //! OCCTL_INVALID_ARGUMENT, including when @p theInfo is nullptr.
  occtl_status_t Initialize(const occtl_runtime_init_info_t* theInfo) noexcept;

  //! Releases process-wide state. Safe to call repeatedly.
  void Shutdown() noexcept;

  bool IsInitialised() const noexcept { return myInitialised; }

private:
  Runtime() noexcept;

  bool myInitialised;
};

} // namespace OcctL::Core

#endif // OCCTL_CORE_RUNTIME_HXX
