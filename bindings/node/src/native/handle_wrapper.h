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

#pragma once

#include <cstdint>
#include <napi.h>

extern "C"
{
#include <occtl/occtl_core.h>
#include <occtl/occtl_curves.h>
#include <occtl/occtl_curves2d.h>
#include <occtl/occtl_geom.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_surfaces.h>
#include <occtl/occtl_text.h>
#include <occtl/occtl_topo.h>
}

namespace occtl_node
{

template <typename TPtr>
class TypedHandle
{
public:
  TPtr pointer() const { return _ptr; }

  bool disposed() const { return _ptr == nullptr; }

protected:
  TPtr _ptr = nullptr;
};

class GraphHandle : public Napi::ObjectWrap<GraphHandle>, public TypedHandle<occtl_graph_t*>
{
public:
  static Napi::Function GetClass(Napi::Env env);
  GraphHandle(const Napi::CallbackInfo& info);
  ~GraphHandle() override;

  Napi::Value Close(const Napi::CallbackInfo& info);
  Napi::Value Disposed(const Napi::CallbackInfo& info);

  void adopt(occtl_graph_t* graph);

  occtl_graph_t* ptr() const { return _ptr; }
};

class NodeIterHandle : public Napi::ObjectWrap<NodeIterHandle>
{
public:
  static Napi::Function GetClass(Napi::Env env);
  NodeIterHandle(const Napi::CallbackInfo& info);
  ~NodeIterHandle() override;

  Napi::Value Next(const Napi::CallbackInfo& info);
  Napi::Value Close(const Napi::CallbackInfo& info);
  Napi::Value Disposed(const Napi::CallbackInfo& info);

  void adopt(occtl_node_iter_t* it) { _ptr = it; }

  occtl_node_iter_t* ptr() const { return _ptr; }

private:
  occtl_node_iter_t* _ptr = nullptr;
};

} // namespace occtl_node
