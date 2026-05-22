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

#include "ErrorState.hxx"
#include "Guard.hxx"
#include "Runtime.hxx"

#include <occtl/occtl_core.h>

#include <cstring>

namespace
{

constexpr const char* THE_OCCT_VERSION_STRING =
#if defined(OCCT_VERSION_STRING_EXT)
  OCCT_VERSION_STRING_EXT
#elif defined(OCC_VERSION_COMPLETE)
  OCC_VERSION_COMPLETE
#else
  "unknown"
#endif
  ;

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API const char* OCCTL_CALL occtl_status_to_string(occtl_status_t theStatus)
{
  switch (theStatus)
  {
    case OCCTL_OK:
      return "OCCTL_OK";
    case OCCTL_ERROR:
      return "OCCTL_ERROR";
    case OCCTL_INVALID_ARGUMENT:
      return "OCCTL_INVALID_ARGUMENT";
    case OCCTL_INVALID_HANDLE:
      return "OCCTL_INVALID_HANDLE";
    case OCCTL_NOT_FOUND:
      return "OCCTL_NOT_FOUND";
    case OCCTL_OUT_OF_MEMORY:
      return "OCCTL_OUT_OF_MEMORY";
    case OCCTL_OUT_OF_RANGE:
      return "OCCTL_OUT_OF_RANGE";
    case OCCTL_NOT_DONE:
      return "OCCTL_NOT_DONE";
    case OCCTL_GEOMETRY_INVALID:
      return "OCCTL_GEOMETRY_INVALID";
    case OCCTL_TOPOLOGY_INVALID:
      return "OCCTL_TOPOLOGY_INVALID";
    case OCCTL_IO_ERROR:
      return "OCCTL_IO_ERROR";
    case OCCTL_FORMAT_ERROR:
      return "OCCTL_FORMAT_ERROR";
    case OCCTL_UNSUPPORTED:
      return "OCCTL_UNSUPPORTED";
    case OCCTL_CANCELLED:
      return "OCCTL_CANCELLED";
    case OCCTL_BUFFER_TOO_SMALL:
      return "OCCTL_BUFFER_TOO_SMALL";
    case OCCTL_VERSION_MISMATCH:
      return "OCCTL_VERSION_MISMATCH";
    case OCCTL_INTERNAL:
      return "OCCTL_INTERNAL";
    case OCCTL_WRONG_KIND:
      return "OCCTL_WRONG_KIND";
    case OCCTL_STATUS_RESERVED_FUTURE:
      break;
  }
  return "OCCTL_UNKNOWN";
}

//==================================================================================================

OCCTL_API const occtl_error_t* OCCTL_CALL occtl_error_last(void)
{
  return OcctL::Core::ErrorState::Current().AsCPointer();
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_error_clear(void)
{
  OcctL::Core::ErrorState::Current().Clear();
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_runtime_init(const occtl_runtime_init_info_t* theInfo)
{
  return OcctL::Core::Guard(
    [&]() -> occtl_status_t { return OcctL::Core::Runtime::Instance().Initialize(theInfo); });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_runtime_init_info_init(occtl_runtime_init_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_runtime_init_info_init: info is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    theInfo->struct_version = OCCTL_RUNTIME_INIT_INFO_VERSION_1;
    theInfo->p_next         = nullptr;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_runtime_shutdown(void)
{
  OcctL::Core::Runtime::Instance().Shutdown();
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_runtime_version(uint32_t* theOutMajor,
                                                uint32_t* theOutMinor,
                                                uint32_t* theOutPatch)
{
  if (theOutMajor != nullptr)
  {
    *theOutMajor = OCCTL_VERSION_MAJOR;
  }
  if (theOutMinor != nullptr)
  {
    *theOutMinor = OCCTL_VERSION_MINOR;
  }
  if (theOutPatch != nullptr)
  {
    *theOutPatch = OCCTL_VERSION_PATCH;
  }
}

//==================================================================================================

OCCTL_API uint32_t OCCTL_CALL occtl_runtime_abi_version(void)
{
  return static_cast<uint32_t>(OCCTL_ABI_VERSION);
}

//==================================================================================================

OCCTL_API const char* OCCTL_CALL occtl_runtime_occt_version(void)
{
  return THE_OCCT_VERSION_STRING;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_uid_to_bytes(const occtl_uid_t theUid,
                                                       uint8_t* const    theOutBytes)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutBytes == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_bytes is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    // Top 8 bytes: current 64-bit identity, big-endian.
    for (unsigned i = 0; i < 8u; ++i)
    {
      theOutBytes[i] = static_cast<uint8_t>((theUid.bits >> (56u - 8u * i)) & 0xffu);
    }
    // Bottom 8 bytes: reserved, always zero today.
    for (unsigned i = 8; i < OCCTL_UID_WIRE_SIZE; ++i)
    {
      theOutBytes[i] = 0u;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_uid_from_bytes(const uint8_t* const theInBytes,
                                                         occtl_uid_t* const   theOutUid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theInBytes == nullptr || theOutUid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theInBytes ? "out_uid is NULL" : "in_bytes is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    // Reserved bytes must be zero.
    for (unsigned i = 8; i < OCCTL_UID_WIRE_SIZE; ++i)
    {
      if (theInBytes[i] != 0u)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_FORMAT_ERROR,
                                               "occtl_uid_from_bytes: reserved bytes must be zero");
        return OCCTL_FORMAT_ERROR;
      }
    }
    uint64_t aBits = 0u;
    for (unsigned i = 0; i < 8u; ++i)
    {
      aBits = (aBits << 8u) | static_cast<uint64_t>(theInBytes[i]);
    }
    theOutUid->bits = aBits;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API int32_t OCCTL_CALL occtl_uid_equal(const occtl_uid_t theA, const occtl_uid_t theB)
{
  return (theA.bits == theB.bits) ? 1 : 0;
}

} // extern "C"
