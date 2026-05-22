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

#include "handle_wrapper.h"

namespace occtl_node
{

// ===== GraphHandle ==========================================================

Napi::Function GraphHandle::GetClass(Napi::Env env)
{
  return DefineClass(env,
                     "Graph",
                     {
                       InstanceMethod("close", &GraphHandle::Close),
                       InstanceAccessor("disposed", &GraphHandle::Disposed, nullptr),
                     });
}

GraphHandle::GraphHandle(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<GraphHandle>(info)
{
  occtl_graph_t* graph = nullptr;
  occtl_status_t s     = occtl_graph_create(&graph);
  if (s != OCCTL_OK)
  {
    Napi::Error::New(info.Env(), "occtl_graph_create failed").ThrowAsJavaScriptException();
    return;
  }
  _ptr = graph;
}

GraphHandle::~GraphHandle()
{
  if (_ptr)
  {
    occtl_graph_free(_ptr);
    _ptr = nullptr;
  }
}

Napi::Value GraphHandle::Close(const Napi::CallbackInfo& info)
{
  if (_ptr)
  {
    occtl_graph_free(_ptr);
    _ptr = nullptr;
  }
  return info.Env().Undefined();
}

void GraphHandle::adopt(occtl_graph_t* graph)
{
  if (_ptr && _ptr != graph)
  {
    occtl_graph_free(_ptr);
  }
  _ptr = graph;
}

Napi::Value GraphHandle::Disposed(const Napi::CallbackInfo& info)
{
  return Napi::Boolean::New(info.Env(), _ptr == nullptr);
}

// ===== NodeIterHandle =======================================================

Napi::Function NodeIterHandle::GetClass(Napi::Env env)
{
  return DefineClass(env,
                     "NodeIter",
                     {
                       InstanceMethod("next", &NodeIterHandle::Next),
                       InstanceMethod("close", &NodeIterHandle::Close),
                       InstanceAccessor("disposed", &NodeIterHandle::Disposed, nullptr),
                     });
}

NodeIterHandle::NodeIterHandle(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NodeIterHandle>(info)
{
  // The iterator is constructed via a static factory in span.cc / iter.cc which
  // then calls adopt(). The default ctor leaves _ptr null and any next() call
  // returns OCCTL_NOT_FOUND-equivalent (JS null).
}

NodeIterHandle::~NodeIterHandle()
{
  if (_ptr)
  {
    occtl_node_iter_free(_ptr);
    _ptr = nullptr;
  }
}

Napi::Value NodeIterHandle::Next(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();
  if (!_ptr)
    return env.Null();
  occtl_node_id_t out = {0};
  occtl_status_t  s   = occtl_node_iter_next(_ptr, &out);
  if (s == OCCTL_NOT_FOUND)
    return env.Null();
  if (s != OCCTL_OK)
  {
    Napi::Error::New(env, occtl_status_to_string(s)).ThrowAsJavaScriptException();
    return env.Undefined();
  }
  return Napi::BigInt::New(env, (uint64_t)out.bits);
}

Napi::Value NodeIterHandle::Close(const Napi::CallbackInfo& info)
{
  if (_ptr)
  {
    occtl_node_iter_free(_ptr);
    _ptr = nullptr;
  }
  return info.Env().Undefined();
}

Napi::Value NodeIterHandle::Disposed(const Napi::CallbackInfo& info)
{
  return Napi::Boolean::New(info.Env(), _ptr == nullptr);
}

} // namespace occtl_node
