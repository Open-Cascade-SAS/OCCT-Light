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

#include <napi.h>
#include <string>

extern "C"
{
#include <occtl/occtl_core.h>
}

namespace occtl_node
{

const char* ClassNameForStatus(occtl_status_t s)
{
  switch (s)
  {
    case OCCTL_OK:
      return "OK";
    case OCCTL_ERROR:
      return "GenericError";
    case OCCTL_INVALID_ARGUMENT:
      return "InvalidArgumentError";
    case OCCTL_INVALID_HANDLE:
      return "InvalidHandleError";
    case OCCTL_NOT_FOUND:
      return "NotFoundError";
    case OCCTL_OUT_OF_MEMORY:
      return "OutOfMemoryError";
    case OCCTL_OUT_OF_RANGE:
      return "OutOfRangeError";
    case OCCTL_NOT_DONE:
      return "NotDoneError";
    case OCCTL_GEOMETRY_INVALID:
      return "GeometryInvalidError";
    case OCCTL_TOPOLOGY_INVALID:
      return "TopologyInvalidError";
    case OCCTL_IO_ERROR:
      return "IoError";
    case OCCTL_FORMAT_ERROR:
      return "FormatError";
    case OCCTL_UNSUPPORTED:
      return "UnsupportedError";
    case OCCTL_CANCELLED:
      return "CancelledError";
    case OCCTL_BUFFER_TOO_SMALL:
      return "BufferTooSmallError";
    case OCCTL_VERSION_MISMATCH:
      return "VersionMismatchError";
    case OCCTL_INTERNAL:
      return "InternalError";
    case OCCTL_WRONG_KIND:
      return "WrongKindError";
    default:
      return "OcctLError";
  }
}

void ThrowFromStatus(Napi::Env env, occtl_status_t status)
{
  if (status == OCCTL_OK)
    return;
  const occtl_error_t* err = occtl_error_last();
  std::string          msg = (err && err->message) ? err->message : occtl_status_to_string(status);

  Napi::Error e = Napi::Error::New(env, msg);
  e.Set("name", Napi::String::New(env, ClassNameForStatus(status)));
  e.Set("status", Napi::Number::New(env, static_cast<int>(status)));
  if (err)
  {
    e.Set("source", Napi::BigInt::New(env, (uint64_t)err->source.bits));
    e.Set("extended", Napi::Number::New(env, err->extended));
  }
  else
  {
    e.Set("source", Napi::BigInt::New(env, (uint64_t)0));
    e.Set("extended", Napi::Number::New(env, 0));
  }
  e.ThrowAsJavaScriptException();
}

} // namespace occtl_node
