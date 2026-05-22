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

  // Override: handle lifetime is owned by the JS wrapper (lib/handles.ts).
  // Calling *_free via .call() would double-free; reject instead.
  (void)args;
  Napi::Error e = Napi::Error::New(env, "handle disposal is owned by the JS wrapper; call handle.close() or let GC run");
  e.Set("name", Napi::String::New(env, "UnsupportedError"));
  e.Set("status", Napi::Number::New(env, (int)OCCTL_UNSUPPORTED));
  e.Set("source", Napi::BigInt::New(env, (uint64_t)0));
  e.Set("extended", Napi::Number::New(env, 0));
  e.ThrowAsJavaScriptException();
  return env.Undefined();
