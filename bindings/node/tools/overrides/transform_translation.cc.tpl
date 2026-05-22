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

  if (args.Length() < 1 || !args.Get(0u).IsObject()) {
    Napi::TypeError::New(env, "transform_translation: expected ({x,y,z})").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Object obj = args.Get(0u).As<Napi::Object>();
  occtl_vector3_t delta{};
  delta.x = obj.Has("x") ? obj.Get("x").ToNumber().DoubleValue() : 0.0;
  delta.y = obj.Has("y") ? obj.Get("y").ToNumber().DoubleValue() : 0.0;
  delta.z = obj.Has("z") ? obj.Get("z").ToNumber().DoubleValue() : 0.0;

  const occtl_transform_t transform = occtl_transform_translation(delta);
  Napi::Array m = Napi::Array::New(env, 12);
  for (uint32_t i = 0; i < 12; ++i) {
    m.Set(i, Napi::Number::New(env, transform.m[i]));
  }
  Napi::Object out = Napi::Object::New(env);
  out.Set("m", m);
  return out;
