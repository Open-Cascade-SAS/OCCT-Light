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

#include <cstdint>
#include <napi.h>

namespace occtl_node
{

/** Wraps `count` doubles starting at `ptr` in a Float64Array. The parent_ref is
 *  a JS Object held strong until the buffer is collected. */
Napi::Value WrapFloat64Span(Napi::Env env, const double* ptr, size_t count, Napi::Object parent_ref)
{
  if (!ptr || count == 0)
  {
    return Napi::Float64Array::New(env, 0);
  }
  // Persist the parent reference via napi_ref so its lifetime extends the view's.
  napi_ref*   parent = new napi_ref;
  napi_status s      = napi_create_reference(env, parent_ref, 1, parent);
  if (s != napi_ok)
  {
    delete parent;
    Napi::Error::New(env, "WrapFloat64Span: failed to create parent reference")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Buffer<double> buf = Napi::Buffer<double>::New(
    env,
    const_cast<double*>(ptr),
    count,
    [](Napi::Env e, double* /*data*/, napi_ref* parent_ptr) {
      if (parent_ptr)
      {
        uint32_t refcount = 0;
        napi_reference_unref(e, *parent_ptr, &refcount);
        napi_delete_reference(e, *parent_ptr);
        delete parent_ptr;
      }
    },
    parent);
  return Napi::Float64Array::New(env, count, buf.ArrayBuffer(), 0);
}

} // namespace occtl_node
