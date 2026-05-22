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
    Napi::TypeError::New(env, "curve2d_to_bezier_segments: expected (graph, curveId[, options])").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  occtl_graph_t* graph = nullptr;
  {
    Napi::Value vv = args.Get(0u);
    if (vv.IsObject()) {
      GraphHandle* h = Napi::ObjectWrap<GraphHandle>::Unwrap(vv.As<Napi::Object>());
      if (h) graph = h->ptr();
    }
  }
  if (!graph) {
    Napi::Error::New(env, "curve2d_to_bezier_segments: first argument must be a Graph").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  occtl_rep_id_t curve_id = {};
  {
    Napi::Value vv = args.Get(1u);
    if (vv.IsBigInt()) {
      bool lossless = false;
      curve_id.bits = (uint64_t)vv.As<Napi::BigInt>().Uint64Value(&lossless);
    }
  }

  occtl_curve_bezier_segments_options_t options = OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_INIT;
  const occtl_curve_bezier_segments_options_t* options_ptr = nullptr;
  if (args.Length() >= 3 && args.Get(2u).IsObject()) {
    options_ptr = &options;
    Napi::Object obj = args.Get(2u).As<Napi::Object>();
    if (obj.Has("struct_version")) {
      options.struct_version = (uint32_t)obj.Get("struct_version").ToNumber().Int64Value();
    }
    if (obj.Has("use_range")) {
      options.use_range = (int32_t)obj.Get("use_range").ToNumber().Int64Value();
    }
    if (obj.Has("u_first")) {
      options.u_first = obj.Get("u_first").ToNumber().DoubleValue();
    }
    if (obj.Has("u_last")) {
      options.u_last = obj.Get("u_last").ToNumber().DoubleValue();
    }
    if (obj.Has("parametric_tolerance")) {
      options.parametric_tolerance = obj.Get("parametric_tolerance").ToNumber().DoubleValue();
    }
  }

  occtl_rep_id_t* segments = nullptr;
  size_t count = 0;
  occtl_status_t status =
    occtl_curve2d_to_bezier_segments(graph, curve_id, options_ptr, &segments, &count);
  if (status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }

  Napi::Array out = Napi::Array::New(env, count);
  for (size_t i = 0; i < count; ++i) {
    out.Set((uint32_t)i, Napi::BigInt::New(env, (uint64_t)segments[i].bits));
  }
  std::free(segments);
  return out;
