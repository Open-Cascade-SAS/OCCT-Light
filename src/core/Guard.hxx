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

#ifndef OCCTL_CORE_GUARD_HXX
#define OCCTL_CORE_GUARD_HXX

#include "ErrorState.hxx"

#include <occtl/occtl_core.h>

#include <exception>
#include <typeinfo>
#include <utility>

namespace OcctL::Core
{

//! Translates a caught exception into a populated thread-local error state.
//! Used by Guard; exposed so other internal helpers can call it explicitly.
//! @param[in] theException pointer to the caught std::exception, or nullptr for an unknown throw
//! @return the translated status code; never OCCTL_OK
occtl_status_t TranslateException(const std::exception* theException) noexcept;

//! Wraps an extern "C" entry point body, catching every exception type and
//! translating it into a status code plus a populated thread-local error.
//! @param[in] theBody a callable returning occtl_status_t
//! @return the body's status, or a translated failure status if the body threw
//!
//! For finer-grained status codes (e.g. OCCTL_GEOMETRY_INVALID rather than
//! the default OCCTL_INTERNAL), catch the OCCT exception subclass
//! (Standard_ConstructionError, Standard_OutOfRange, ...) inside @p theBody
//! and call ErrorState::Set() before returning the desired status. Anything
//! that escapes ends up here and maps to OCCTL_INTERNAL.
template <typename TheBody>
occtl_status_t Guard(TheBody&& theBody) noexcept
{
  try
  {
    return std::forward<TheBody>(theBody)();
  }
  catch (const std::exception& anEx)
  {
    return TranslateException(&anEx);
  }
  catch (...)
  {
    return TranslateException(nullptr);
  }
}

//! Wraps an extern "C" entry point body that returns a non-status value
//! (e.g. `size_t` count queries), substituting @p theFallback on any
//! exception so nothing escapes across the C ABI boundary.
//! @param[in] theBody     callable returning TheValue
//! @param[in] theFallback value substituted on exception
//! @return the body's value, or @p theFallback if the body threw
template <typename TheValue, typename TheBody>
TheValue GuardValue(TheBody&& theBody, const TheValue theFallback) noexcept
{
  try
  {
    return std::forward<TheBody>(theBody)();
  }
  catch (const std::exception& anEx)
  {
    TranslateException(&anEx);
    return theFallback;
  }
  catch (...)
  {
    TranslateException(nullptr);
    return theFallback;
  }
}

} // namespace OcctL::Core

#endif // OCCTL_CORE_GUARD_HXX
