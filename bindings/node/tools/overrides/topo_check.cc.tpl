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
    Napi::TypeError::New(env, "topo_check: expected (graph, capacity)").ThrowAsJavaScriptException();
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

  const size_t capacity = (size_t)args.Get(1u).ToNumber().Int64Value();
  std::vector<occtl_topo_check_issue_t> issues(capacity);
  size_t count = 0;
  occtl_status_t status =
    occtl_topo_check(graph, capacity ? issues.data() : nullptr, capacity, &count);
  if (status != OCCTL_OK) {
    ThrowFromStatus(env, status);
    return env.Undefined();
  }

  Napi::Object out = Napi::Object::New(env);
  out.Set("out_count", Napi::Number::New(env, (double)count));
  if (capacity > 0 && count > 0) {
    const size_t emitted = count < capacity ? count : capacity;
    Napi::Array jsIssues = Napi::Array::New(env, (uint32_t)emitted);
    for (size_t i = 0; i < emitted; ++i) {
      const occtl_topo_check_issue_t& issue = issues[i];
      Napi::Object jsIssue = Napi::Object::New(env);
      jsIssue.Set("node_id", Napi::BigInt::New(env, (uint64_t)issue.node_id.bits));
      jsIssue.Set("context_node_id", Napi::BigInt::New(env, (uint64_t)issue.context_node_id.bits));
      jsIssue.Set("status_bit", Napi::Number::New(env, (double)issue.status_bit));
      jsIssue.Set("severity", Napi::Number::New(env, (int)issue.severity));
      jsIssues.Set((uint32_t)i, jsIssue);
    }
    out.Set("out_issues", jsIssues);
  }
  return out;
