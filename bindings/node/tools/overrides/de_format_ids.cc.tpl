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

  if (args.Length() != 0) {
    Napi::TypeError::New(env, "de_format_ids: expected no arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  size_t count = 0;
  occtl_status_t status = occtl_de_format_ids(nullptr, 0, &count);
  if (status != OCCTL_OK) { ThrowFromStatus(env, status); return env.Undefined(); }
  std::vector<const char*> ids(count, nullptr);
  status = occtl_de_format_ids(ids.empty() ? nullptr : ids.data(), count, &count);
  if (status != OCCTL_OK) { ThrowFromStatus(env, status); return env.Undefined(); }
  Napi::Array out = Napi::Array::New(env, count);
  for (size_t i = 0; i < count; ++i) {
    out.Set(static_cast<uint32_t>(i), Napi::String::New(env, ids[i] ? ids[i] : ""));
  }
  return out;
