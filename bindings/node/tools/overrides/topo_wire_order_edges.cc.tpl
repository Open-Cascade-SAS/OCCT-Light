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

  if (args.Length() < 2) {
    Napi::TypeError::New(env, "topo_wire_order_edges: expected (graph, wire)").ThrowAsJavaScriptException();
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

  occtl_node_id_t wire = {0};
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
    wire.bits = bits;
  }

  size_t count = 0;
  occtl_status_t status = occtl_topo_wire_order_edges(graph, wire, nullptr, 0, &count);
  if (status != OCCTL_OK && status != OCCTL_BUFFER_TOO_SMALL) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }

  std::vector<occtl_oriented_node_t> ordered(count);
  status = occtl_topo_wire_order_edges(graph,
                                       wire,
                                       ordered.empty() ? nullptr : ordered.data(),
                                       ordered.size(),
                                       &count);
  if (status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }

  Napi::Array out = Napi::Array::New(env, count);
  for (size_t i = 0; i < count; ++i) {
    Napi::Object item = Napi::Object::New(env);
    item.Set("id", Napi::BigInt::New(env, (uint64_t)ordered[i].id.bits));
    item.Set("orientation", Napi::Number::New(env, (int)ordered[i].orientation));
    out.Set((uint32_t)i, item);
  }
  return out;
