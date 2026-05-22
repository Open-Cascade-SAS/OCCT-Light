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

  if (args.Length() < 3) {
    Napi::TypeError::New(env, "topo_transformed: expected (graph, root, transform)").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  occtl_graph_t* graph = nullptr;
  {
    Napi::Value vv = args.Get(0u);
    if (vv.IsExternal()) {
      graph = vv.As<Napi::External<occtl_graph_t>>().Data();
    } else if (vv.IsObject()) {
      GraphHandle* h = Napi::ObjectWrap<GraphHandle>::Unwrap(vv.As<Napi::Object>());
      if (h) graph = h->ptr();
    }
  }

  occtl_node_id_t root = {0};
  {
    Napi::Value vv = args.Get(1u);
    uint64_t bits = 0;
    if (vv.IsBigInt()) {
      bool lossless = false;
      bits = vv.As<Napi::BigInt>().Uint64Value(&lossless);
    } else if (vv.IsNumber()) {
      bits = (uint64_t)vv.ToNumber().Int64Value();
    } else if (vv.IsObject() && vv.As<Napi::Object>().Has("bits")) {
      Napi::Value b = vv.As<Napi::Object>().Get("bits");
      if (b.IsBigInt()) {
        bool lossless = false;
        bits = b.As<Napi::BigInt>().Uint64Value(&lossless);
      } else {
        bits = (uint64_t)b.ToNumber().Int64Value();
      }
    }
    root.bits = bits;
  }

  occtl_transform_t transform{};
  if (args.Get(2u).IsObject()) {
    Napi::Object obj = args.Get(2u).As<Napi::Object>();
    if (obj.Has("m")) {
      Napi::Array m = obj.Get("m").As<Napi::Array>();
      for (uint32_t i = 0; i < 12 && i < m.Length(); ++i) {
        transform.m[i] = m.Get(i).ToNumber().DoubleValue();
      }
    }
  }

  occtl_graph_t* out_graph = nullptr;
  occtl_node_id_t out_root = {0};
  occtl_status_t status =
    occtl_topo_transformed(graph, root, transform, &out_graph, &out_root);
  if (status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }

  Napi::Object out = Napi::Object::New(env);
  out.Set("out_graph", [&]() -> Napi::Value {
    if (!out_graph) return env.Null();
    Napi::Function ctor = GraphHandle::GetClass(env);
    Napi::Object obj = ctor.New({});
    GraphHandle* h = Napi::ObjectWrap<GraphHandle>::Unwrap(obj);
    if (h) h->adopt(out_graph);
    return obj;
  }());
  out.Set("out_root", Napi::BigInt::New(env, (uint64_t)out_root.bits));
  return out;
