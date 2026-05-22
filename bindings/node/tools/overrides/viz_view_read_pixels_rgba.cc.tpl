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

  if (args.Length() < 1 || args.Length() > 2) {
    Napi::TypeError::New(env, "viz_view_read_pixels_rgba: expected (view[, buffer])").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  occtl_viz_view_t* view = nullptr;
  {
    Napi::Value vv = args.Get(0u);
    if (vv.IsExternal()) {
      view = vv.As<Napi::External<occtl_viz_view_t>>().Data();
    }
  }

  size_t required = 0;
  occtl_status_t status = occtl_viz_view_read_pixels_rgba(view, nullptr, 0, &required);
  if (status != OCCTL_BUFFER_TOO_SMALL && status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }

  Napi::Buffer<uint8_t> buffer =
    args.Length() == 2 && args.Get(1u).IsBuffer()
      ? args.Get(1u).As<Napi::Buffer<uint8_t>>()
      : Napi::Buffer<uint8_t>::New(env, required);
  size_t count = buffer.Length();
  status = occtl_viz_view_read_pixels_rgba(view,
                                           buffer.Data(),
                                           buffer.Length(),
                                           &count);
  if (status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }

  Napi::Object out = Napi::Object::New(env);
  out.Set("data", buffer);
  out.Set("count", Napi::Number::New(env, static_cast<double>(count)));
  return out;
