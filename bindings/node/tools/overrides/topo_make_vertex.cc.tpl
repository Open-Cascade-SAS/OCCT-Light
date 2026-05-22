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

  // Override: ergonomic call form used by smoke tests: (graph, x, y, z).
  // and lib/handles.ts. The generic generator would expect (graph, info_object).
  if (args.Length() < 2) {
    Napi::TypeError::New(env, "topo_make_vertex: need (graph, info|x, [y, z])").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  GraphHandle* gh = Napi::ObjectWrap<GraphHandle>::Unwrap(args.Get(0u).As<Napi::Object>());
  occtl_graph_t* graph = gh ? gh->ptr() : nullptr;
  if (!graph) {
    Napi::TypeError::New(env, "topo_make_vertex: graph is disposed").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  occtl_topo_make_vertex_info_t vertex_info = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  if (args.Length() >= 4 && args.Get(1u).IsNumber()) {
    vertex_info.point.x = args.Get(1u).ToNumber().DoubleValue();
    vertex_info.point.y = args.Get(2u).ToNumber().DoubleValue();
    vertex_info.point.z = args.Get(3u).ToNumber().DoubleValue();
  } else if (args.Get(1u).IsObject()) {
    Napi::Object info_obj = args.Get(1u).As<Napi::Object>();
    if (info_obj.Has("point")) {
      Napi::Object p = info_obj.Get("point").As<Napi::Object>();
      vertex_info.point.x = p.Get("x").ToNumber().DoubleValue();
      vertex_info.point.y = p.Get("y").ToNumber().DoubleValue();
      vertex_info.point.z = p.Get("z").ToNumber().DoubleValue();
    } else {
      if (info_obj.Has("x")) vertex_info.point.x = info_obj.Get("x").ToNumber().DoubleValue();
      if (info_obj.Has("y")) vertex_info.point.y = info_obj.Get("y").ToNumber().DoubleValue();
      if (info_obj.Has("z")) vertex_info.point.z = info_obj.Get("z").ToNumber().DoubleValue();
    }
    if (info_obj.Has("tolerance")) vertex_info.tolerance = info_obj.Get("tolerance").ToNumber().DoubleValue();
  } else {
    Napi::TypeError::New(env, "topo_make_vertex: arg 1 must be a number or object").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  occtl_node_id_t out_node = {0};
  occtl_status_t status = occtl_topo_make_vertex(graph, &vertex_info, &out_node);
  if (status != OCCTL_OK) { ThrowFromStatus(env, status); return env.Undefined(); }
  return Napi::BigInt::New(env, (uint64_t)out_node.bits);
