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

#ifndef OCCTL_CORE_ERROR_STATE_HXX
#define OCCTL_CORE_ERROR_STATE_HXX

#include <occtl/occtl_core.h>

#include <NCollection_Array1.hxx>

#include <string_view>

namespace OcctL::Core
{

//! Per-thread error state. Holds the most recent failure produced by
//! any extern "C" entry point on this thread. Returned by reference to
//! callers via occtl_error_last(); ownership stays with the thread.
class ErrorState
{
public:
  //! @return reference to the calling thread's error slot
  static ErrorState& Current() noexcept;

  //! Resets the slot to a clean OCCTL_OK state.
  void Clear() noexcept;

  //! Records a failure on this thread.
  //! @param[in] theStatus    the status code; must not be OCCTL_OK
  //! @param[in] theMessage   UTF-8 message, copied internally; may be empty
  //! @param[in] theSourceUid optional UID of the offending entity
  //! @param[in] theExtended  optional SQLite-style extended subcode
  void Set(const occtl_status_t theStatus,
           std::string_view     theMessage,
           const occtl_uid_t&   theSourceUid = occtl_uid_t{0},
           const uint32_t       theExtended  = 0) noexcept;

  //! Exposes the slot in the C ABI shape. Returns a borrowed pointer
  //! valid until this thread issues another OCCT-Light call.
  const occtl_error_t* AsCPointer() noexcept;

  ErrorState() noexcept;

public:
  static constexpr int THE_MAX_ERROR_MSG = 2048;

private:
  occtl_error_t            myView; //!< view returned to callers; @c message points into myMessage
  char                     myBuffer[THE_MAX_ERROR_MSG]; //!< owned stack buffer
  NCollection_Array1<char> myMessage; //!< wraps myBuffer as a bounds-checked array
  int                      myLength;  //!< actual message length excluding NUL; 0 = empty
};

} // namespace OcctL::Core

#endif // OCCTL_CORE_ERROR_STATE_HXX
