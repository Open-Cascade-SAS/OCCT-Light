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

extern "C"
{
#include <occtl/occtl_topo.h>
}

namespace occtl_node
{

void ThrowFromStatus(Napi::Env env, occtl_status_t status);

static Napi::Value MakeIterWith(Napi::Env env, Napi::Function nodeIterCtor, occtl_node_iter_t* raw)
{
  Napi::Object    obj = nodeIterCtor.New({});
  NodeIterHandle* h   = Napi::ObjectWrap<NodeIterHandle>::Unwrap(obj);
  if (h)
    h->adopt(raw);
  return obj;
}

Napi::Value GraphFaceIterCreate(const Napi::CallbackInfo& info)
{
  Napi::Env    env = info.Env();
  GraphHandle* gh  = Napi::ObjectWrap<GraphHandle>::Unwrap(info[0].As<Napi::Object>());
  if (!gh || gh->disposed())
  {
    Napi::TypeError::New(env, "graphFaceIterCreate: graph disposed").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  occtl_node_iter_t* it = nullptr;
  occtl_status_t     s  = occtl_graph_face_iter_create(gh->ptr(), &it);
  if (s != OCCTL_OK)
  {
    ThrowFromStatus(env, s);
    return env.Undefined();
  }
  Napi::Function ctor = NodeIterHandle::GetClass(env);
  return MakeIterWith(env, ctor, it);
}

Napi::Value GraphEdgeIterCreate(const Napi::CallbackInfo& info)
{
  Napi::Env    env = info.Env();
  GraphHandle* gh  = Napi::ObjectWrap<GraphHandle>::Unwrap(info[0].As<Napi::Object>());
  if (!gh || gh->disposed())
  {
    Napi::TypeError::New(env, "graphEdgeIterCreate: graph disposed").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  occtl_node_iter_t* it = nullptr;
  occtl_status_t     s  = occtl_graph_edge_iter_create(gh->ptr(), &it);
  if (s != OCCTL_OK)
  {
    ThrowFromStatus(env, s);
    return env.Undefined();
  }
  Napi::Function ctor = NodeIterHandle::GetClass(env);
  return MakeIterWith(env, ctor, it);
}

Napi::Value GraphVertexIterCreate(const Napi::CallbackInfo& info)
{
  Napi::Env    env = info.Env();
  GraphHandle* gh  = Napi::ObjectWrap<GraphHandle>::Unwrap(info[0].As<Napi::Object>());
  if (!gh || gh->disposed())
  {
    Napi::TypeError::New(env, "graphVertexIterCreate: graph disposed").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  occtl_node_iter_t* it = nullptr;
  occtl_status_t     s  = occtl_graph_vertex_iter_create(gh->ptr(), &it);
  if (s != OCCTL_OK)
  {
    ThrowFromStatus(env, s);
    return env.Undefined();
  }
  Napi::Function ctor = NodeIterHandle::GetClass(env);
  return MakeIterWith(env, ctor, it);
}

Napi::Value GraphSolidIterCreate(const Napi::CallbackInfo& info)
{
  Napi::Env    env = info.Env();
  GraphHandle* gh  = Napi::ObjectWrap<GraphHandle>::Unwrap(info[0].As<Napi::Object>());
  if (!gh || gh->disposed())
  {
    Napi::TypeError::New(env, "graphSolidIterCreate: graph disposed").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  occtl_node_iter_t* it = nullptr;
  occtl_status_t     s  = occtl_graph_solid_iter_create(gh->ptr(), &it);
  if (s != OCCTL_OK)
  {
    ThrowFromStatus(env, s);
    return env.Undefined();
  }
  Napi::Function ctor = NodeIterHandle::GetClass(env);
  return MakeIterWith(env, ctor, it);
}

} // namespace occtl_node
