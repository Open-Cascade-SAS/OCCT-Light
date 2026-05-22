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

/**
 * @file
 * @brief C++ veneer for the core module.
 *
 * Header-only RAII wrappers and exception translation over the C ABI.
 * The public API is STL-shaped while local identifiers follow OCCT style.
 */

#ifndef OCCTL_HPP_CORE_HPP
#define OCCTL_HPP_CORE_HPP

#include <occtl/occtl_core.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace occtl
{

/// @brief Exception thrown by the veneer on any non-OK status code.
///
/// Carries everything #occtl_error_t carries: status, UTF-8 message,
/// optional source UID, optional extended subcode. The message is
/// copied into the exception so it survives subsequent OCCT-Light calls.
class Error : public std::runtime_error
{
public:
  /// @brief Build from the current thread-local error state.
  static Error from_last(const ::occtl_status_t theFallback) noexcept
  {
    const ::occtl_error_t* anErr = ::occtl_error_last();
    if (anErr != nullptr && anErr->status != OCCTL_OK)
    {
      return Error(anErr->status,
                   anErr->message != nullptr ? anErr->message : "",
                   anErr->source.bits,
                   anErr->extended);
    }
    return Error(theFallback, ::occtl_status_to_string(theFallback), 0, 0);
  }

  Error(const ::occtl_status_t theCode,
        std::string            theMessage,
        const std::uint64_t    theSourceUidBits,
        const std::uint32_t    theExtended) noexcept
      : std::runtime_error(std::move(theMessage)),
        myCode(theCode),
        mySourceUidBits(theSourceUidBits),
        myExtended(theExtended)
  {
  }

  ::occtl_status_t code() const noexcept { return myCode; }

  std::uint64_t source_uid_bits() const noexcept { return mySourceUidBits; }

  std::uint32_t extended() const noexcept { return myExtended; }

private:
  ::occtl_status_t myCode;
  std::uint64_t    mySourceUidBits;
  std::uint32_t    myExtended;
};

/// @brief Throw on non-OK; otherwise a no-op.
inline void check(const ::occtl_status_t theStatus)
{
  if (theStatus != OCCTL_OK)
  {
    throw Error::from_last(theStatus);
  }
}

/// @brief Returns the runtime SemVer as a (major, minor, patch) tuple.
inline std::tuple<std::uint32_t, std::uint32_t, std::uint32_t> version() noexcept
{
  std::uint32_t aMajor = 0;
  std::uint32_t aMinor = 0;
  std::uint32_t aPatch = 0;
  ::occtl_runtime_version(&aMajor, &aMinor, &aPatch);
  return {aMajor, aMinor, aPatch};
}

/// @brief Returns the runtime ABI version.
inline std::uint32_t abi_version() noexcept
{
  return ::occtl_runtime_abi_version();
}

/// @brief Returns the OCCT version OCCT-Light was built against, as a string view.
inline std::string_view occt_version() noexcept
{
  const char* aStr = ::occtl_runtime_occt_version();
  return aStr != nullptr ? std::string_view{aStr} : std::string_view{};
}

/// @brief Initialises an #occtl_runtime_init_info_t to defaults and
/// throws on a non-OK status (which would mean @p theInfo was NULL).
inline void init_runtime_info(::occtl_runtime_init_info_t* theInfo)
{
  check(::occtl_runtime_init_info_init(theInfo));
}

/// @brief Tests two UIDs for bitwise equality.
/// @retval 1 when equal, 0 otherwise.
inline int32_t uid_equal(const ::occtl_uid_t theA, const ::occtl_uid_t theB) noexcept
{
  return ::occtl_uid_equal(theA, theB);
}

/// @brief RAII handle for the process-wide runtime.
///
/// Constructor calls #occtl_runtime_init; destructor calls
/// #occtl_runtime_shutdown. Move-only; constructing a second Runtime
/// while the first is alive is undefined.
class Runtime
{
public:
  /// @brief Initialise with defaults.
  Runtime()
  {
    check(::occtl_runtime_init(nullptr));
    myInitialised = true;
  }

  /// @brief Initialise with explicit options.
  explicit Runtime(const ::occtl_runtime_init_info_t& theInfo)
  {
    check(::occtl_runtime_init(&theInfo));
    myInitialised = true;
  }

  /// @brief Shuts down the runtime. Silently swallows any shutdown failure
  /// because this is called from destructor contexts (move-assignment, unwind)
  /// and must never throw.
  ~Runtime() noexcept
  {
    if (myInitialised)
    {
      ::occtl_runtime_shutdown();
    }
  }

  Runtime(const Runtime&)            = delete;
  Runtime& operator=(const Runtime&) = delete;

  Runtime(Runtime&& theOther) noexcept
      : myInitialised(theOther.myInitialised)
  {
    theOther.myInitialised = false;
  }

  Runtime& operator=(Runtime&& theOther) noexcept
  {
    if (this != &theOther)
    {
      if (myInitialised)
      {
        ::occtl_runtime_shutdown();
      }
      myInitialised          = theOther.myInitialised;
      theOther.myInitialised = false;
    }
    return *this;
  }

private:
  bool myInitialised = false;
};

} // namespace occtl

#endif // OCCTL_HPP_CORE_HPP
