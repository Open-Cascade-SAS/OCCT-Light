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

#ifndef OCCTL_TOPO_GRAPH_HANDLE_HXX
#define OCCTL_TOPO_GRAPH_HANDLE_HXX

#include <BRepGraph.hxx>
#include <BRepGraph_EditorView.hxx>
#include <BRepGraph_TopoView.hxx>

#include <memory>
#include <mutex>

namespace OcctL::Mesh
{
class MeshCache;
} // namespace OcctL::Mesh

struct occtl_graph
{
  occtl_graph();
  ~occtl_graph();

  occtl_graph(const occtl_graph&)            = delete;
  occtl_graph& operator=(const occtl_graph&) = delete;

  BRepGraph graph;

  //! Lazily populated mesh view cache. Mutable so const view accessors
  //! can materialise per-face / per-coedge buffers on first read.
  //! Defined out of line in graph_lifecycle.cxx so the destructor sees
  //! the complete MeshCache definition without leaking it into every
  //! topo translation unit.
  mutable std::unique_ptr<OcctL::Mesh::MeshCache> meshCache;
  mutable std::mutex                              meshCacheInitMutex;
};

#endif // OCCTL_TOPO_GRAPH_HANDLE_HXX
