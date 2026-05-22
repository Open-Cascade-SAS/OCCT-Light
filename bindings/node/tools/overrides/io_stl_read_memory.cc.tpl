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

  if (args.Length() < 1) {
    Napi::TypeError::New(env, "io_stl_read_memory: expected (buffer)").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const uint8_t* data = nullptr;
  size_t size = 0;
  Napi::Value vv = args.Get(0u);
  if (vv.IsBuffer()) {
    Napi::Buffer<uint8_t> buffer = vv.As<Napi::Buffer<uint8_t>>();
    data = buffer.Data();
    size = buffer.Length();
  } else if (vv.IsTypedArray()) {
    Napi::TypedArray array = vv.As<Napi::TypedArray>();
    if (array.TypedArrayType() != napi_uint8_array && array.TypedArrayType() != napi_uint8_clamped_array) {
      Napi::TypeError::New(env, "io_stl_read_memory: typed array must be Uint8Array").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    Napi::ArrayBuffer ab = array.ArrayBuffer();
    data = static_cast<const uint8_t*>(ab.Data()) + array.ByteOffset();
    size = array.ByteLength();
  } else if (vv.IsArrayBuffer()) {
    Napi::ArrayBuffer ab = vv.As<Napi::ArrayBuffer>();
    data = static_cast<const uint8_t*>(ab.Data());
    size = ab.ByteLength();
  } else {
    Napi::TypeError::New(env, "io_stl_read_memory: expected Buffer, Uint8Array, or ArrayBuffer").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  occtl_graph_t* out_graph = nullptr;
  occtl_node_id_t out_root = OCCTL_NODE_ID_INVALID;
  occtl_status_t status = occtl_io_stl_read_memory(data, size, &out_graph, &out_root);
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
