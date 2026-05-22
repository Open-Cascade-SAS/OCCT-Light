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
    Napi::TypeError::New(env, "de_write_memory: expected (graph, root, formatId)").ThrowAsJavaScriptException();
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

  if (!args.Get(2u).IsString()) {
    Napi::TypeError::New(env, "de_write_memory: formatId must be a string").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  std::string format_id = args.Get(2u).ToString().Utf8Value();

  size_t required = 0;
  occtl_status_t status =
    occtl_de_write_memory(graph, root, format_id.c_str(), nullptr, 0, &required);
  if (status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }

  Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::New(env, required);
  size_t count = required;
  status = occtl_de_write_memory(graph,
                                 root,
                                 format_id.c_str(),
                                 buffer.Data(),
                                 buffer.Length(),
                                 &count);
  if (status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }
  return buffer;
