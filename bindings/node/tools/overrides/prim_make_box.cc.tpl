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

  // Override: hand-tuned trampoline used directly by smoke and parity tests.
  if (args.Length() < 2) {
    Napi::TypeError::New(env, "prim_make_box: need (graph, info)").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  GraphHandle* gh = Napi::ObjectWrap<GraphHandle>::Unwrap(args.Get(0u).As<Napi::Object>());
  occtl_graph_t* graph = gh ? gh->ptr() : nullptr;
  if (!graph) {
    Napi::TypeError::New(env, "prim_make_box: graph is disposed").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  Napi::Object info_obj = args.Get(1u).As<Napi::Object>();
  occtl_prim_box_info_t box_info = OCCTL_PRIM_BOX_INFO_INIT;
  if (info_obj.Has("dx")) box_info.dx = info_obj.Get("dx").ToNumber().DoubleValue();
  if (info_obj.Has("dy")) box_info.dy = info_obj.Get("dy").ToNumber().DoubleValue();
  if (info_obj.Has("dz")) box_info.dz = info_obj.Get("dz").ToNumber().DoubleValue();
  if (info_obj.Has("placement")) {
    Napi::Object placement = info_obj.Get("placement").As<Napi::Object>();
    if (placement.Has("location")) {
      Napi::Object location = placement.Get("location").As<Napi::Object>();
      if (location.Has("x")) box_info.placement.location.x = location.Get("x").ToNumber().DoubleValue();
      if (location.Has("y")) box_info.placement.location.y = location.Get("y").ToNumber().DoubleValue();
      if (location.Has("z")) box_info.placement.location.z = location.Get("z").ToNumber().DoubleValue();
    }
    if (placement.Has("x_dir")) {
      Napi::Object x_dir = placement.Get("x_dir").As<Napi::Object>();
      if (x_dir.Has("x")) box_info.placement.x_dir.x = x_dir.Get("x").ToNumber().DoubleValue();
      if (x_dir.Has("y")) box_info.placement.x_dir.y = x_dir.Get("y").ToNumber().DoubleValue();
      if (x_dir.Has("z")) box_info.placement.x_dir.z = x_dir.Get("z").ToNumber().DoubleValue();
    }
    if (placement.Has("x_dir_ref")) {
      Napi::Object x_dir_ref = placement.Get("x_dir_ref").As<Napi::Object>();
      if (x_dir_ref.Has("x")) box_info.placement.x_dir_ref.x = x_dir_ref.Get("x").ToNumber().DoubleValue();
      if (x_dir_ref.Has("y")) box_info.placement.x_dir_ref.y = x_dir_ref.Get("y").ToNumber().DoubleValue();
      if (x_dir_ref.Has("z")) box_info.placement.x_dir_ref.z = x_dir_ref.Get("z").ToNumber().DoubleValue();
    }
  }
  occtl_node_id_t out_node = {0};
  occtl_status_t status = occtl_prim_make_box(graph, &box_info, &out_node);
  if (status != OCCTL_OK) { ThrowFromStatus(env, status); return env.Undefined(); }
  return Napi::BigInt::New(env, (uint64_t)out_node.bits);
