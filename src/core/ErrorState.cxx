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

#include <algorithm>

namespace OcctL::Core
{

namespace
{

thread_local ErrorState* THE_TLS_ERROR_STATE = nullptr;

class TlsOwner
{
public:
  TlsOwner() noexcept = default;

  ~TlsOwner() noexcept { THE_TLS_ERROR_STATE = nullptr; }

  ErrorState& Get() noexcept
  {
    if (THE_TLS_ERROR_STATE == nullptr)
    {
      THE_TLS_ERROR_STATE = &myState;
    }
    return myState;
  }

private:
  ErrorState myState;
};

} // namespace

//==================================================================================================

ErrorState::ErrorState() noexcept
    : myView{OCCTL_OK, "", occtl_uid_t{0}, 0u},
      myBuffer{},
      myMessage(myBuffer, static_cast<size_t>(THE_MAX_ERROR_MSG)),
      myLength(0)
{
}

//==================================================================================================

ErrorState& ErrorState::Current() noexcept
{
  thread_local TlsOwner anOwner;
  return anOwner.Get();
}

//==================================================================================================

void ErrorState::Clear() noexcept
{
  myLength        = 0;
  myView.status   = OCCTL_OK;
  myView.message  = "";
  myView.source   = occtl_uid_t{0};
  myView.extended = 0u;
}

//==================================================================================================

void ErrorState::Set(const occtl_status_t theStatus,
                     std::string_view     theMessage,
                     const occtl_uid_t&   theSourceUid,
                     const uint32_t       theExtended) noexcept
{
  const int aInputLen = static_cast<int>(theMessage.size());
  const int aCopyLen  = std::min(aInputLen, THE_MAX_ERROR_MSG - 1);

  try
  {
    for (int anIdx = 0; anIdx < aCopyLen; ++anIdx)
    {
      myMessage.ChangeValue(anIdx) =
        static_cast<char>(theMessage.data()[static_cast<size_t>(anIdx)]);
    }
    myMessage.ChangeValue(aCopyLen) = '\0';
    myLength                        = aCopyLen;
  }
  catch (...)
  {
    myLength = 0;
  }

  myView.status   = theStatus;
  myView.message  = myLength > 0 ? &myMessage.ChangeValue(0) : "";
  myView.source   = theSourceUid;
  myView.extended = theExtended;
}

//==================================================================================================

const occtl_error_t* ErrorState::AsCPointer() noexcept
{
  myView.message = myLength > 0 ? &myMessage.ChangeValue(0) : "";
  return &myView;
}

} // namespace OcctL::Core
