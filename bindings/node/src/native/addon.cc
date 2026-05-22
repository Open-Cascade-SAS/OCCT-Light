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

extern "C"
{
#include <occtl/occtl_core.h>
}

#include "handle_wrapper.h"

namespace occtl_node
{

void ThrowFromStatus(Napi::Env env, occtl_status_t status);

Napi::Value RawCall(const Napi::CallbackInfo& info);
Napi::Array ExportedFunctionsArray(Napi::Env env);

Napi::Value GraphFaceIterCreate(const Napi::CallbackInfo& info);
Napi::Value GraphEdgeIterCreate(const Napi::CallbackInfo& info);
Napi::Value GraphVertexIterCreate(const Napi::CallbackInfo& info);
Napi::Value GraphSolidIterCreate(const Napi::CallbackInfo& info);

static Napi::Value RuntimeAbiVersion(const Napi::CallbackInfo& info)
{
  return Napi::Number::New(info.Env(), occtl_runtime_abi_version());
}

static Napi::Value RuntimeInit(const Napi::CallbackInfo& info)
{
  occtl_status_t s = occtl_runtime_init(nullptr);
  if (s != OCCTL_OK && s != OCCTL_INVALID_ARGUMENT)
  {
    // OCCTL_INVALID_ARGUMENT here is "already initialised" — treat as success.
    ThrowFromStatus(info.Env(), s);
    return info.Env().Undefined();
  }
  return info.Env().Undefined();
}

static Napi::Value ErrorLast(const Napi::CallbackInfo& info)
{
  Napi::Env            env = info.Env();
  const occtl_error_t* err = occtl_error_last();
  Napi::Object         o   = Napi::Object::New(env);
  if (err)
  {
    o.Set("status", Napi::Number::New(env, (int)err->status));
    o.Set("message", Napi::String::New(env, err->message ? err->message : ""));
    o.Set("source", Napi::BigInt::New(env, (uint64_t)err->source.bits));
    o.Set("extended", Napi::Number::New(env, err->extended));
  }
  else
  {
    o.Set("status", Napi::Number::New(env, 0));
    o.Set("message", Napi::String::New(env, ""));
    o.Set("source", Napi::BigInt::New(env, (uint64_t)0));
    o.Set("extended", Napi::Number::New(env, 0));
  }
  return o;
}

static Napi::Value ErrorClear(const Napi::CallbackInfo& info)
{
  occtl_error_clear();
  return info.Env().Undefined();
}

static Napi::Value StatusToString(const Napi::CallbackInfo& info)
{
  int s = info.Length() > 0 ? info[0].ToNumber().Int32Value() : 0;
  return Napi::String::New(info.Env(), occtl_status_to_string((occtl_status_t)s));
}

Napi::Object InitAddon(Napi::Env env, Napi::Object exports)
{
  exports.Set("ABI_VERSION", Napi::Number::New(env, OCCTL_ABI_VERSION));
  exports.Set("EXPORTED_FUNCTIONS", ExportedFunctionsArray(env));

  exports.Set("runtimeAbiVersion", Napi::Function::New(env, RuntimeAbiVersion));
  exports.Set("runtimeInit", Napi::Function::New(env, RuntimeInit));
  exports.Set("errorLast", Napi::Function::New(env, ErrorLast));
  exports.Set("errorClear", Napi::Function::New(env, ErrorClear));
  exports.Set("statusToString", Napi::Function::New(env, StatusToString));

  exports.Set("call", Napi::Function::New(env, RawCall));

  exports.Set("Graph", GraphHandle::GetClass(env));
  exports.Set("NodeIter", NodeIterHandle::GetClass(env));

  exports.Set("graphFaceIterCreate", Napi::Function::New(env, GraphFaceIterCreate));
  exports.Set("graphEdgeIterCreate", Napi::Function::New(env, GraphEdgeIterCreate));
  exports.Set("graphVertexIterCreate", Napi::Function::New(env, GraphVertexIterCreate));
  exports.Set("graphSolidIterCreate", Napi::Function::New(env, GraphSolidIterCreate));

  return exports;
}

} // namespace occtl_node

static Napi::Object OcctlNodeInit(Napi::Env env, Napi::Object exports)
{
  return occtl_node::InitAddon(env, exports);
}

NODE_API_MODULE(occtl_node, OcctlNodeInit)
