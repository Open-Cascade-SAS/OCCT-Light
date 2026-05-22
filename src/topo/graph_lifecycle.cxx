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

#include "GraphHandle.hxx"
#include "GraphLayers.hxx"
#include "IdConvert.hxx"
#include "TopoMath.hxx"

#include "../mesh/MeshCache.hxx"

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <BRepGraph_Compact.hxx>
#include <BRepGraph_Copy.hxx>
#include <BRepGraph_EditorView.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_TopoView.hxx>

#include <TopLoc_Location.hxx>

#include <mutex>

namespace
{

void invalidateMeshCache(occtl_graph_t* const theGraph) noexcept
{
#ifdef OCCTL_HAS_MESH
  if (theGraph == nullptr || !theGraph->meshCache)
  {
    return;
  }
  std::lock_guard<std::mutex> aLock(theGraph->meshCache->Mutex());
  theGraph->meshCache->Invalidate();
#else
  (void)theGraph;
#endif
}

} // namespace

//! Out-of-line ctor / dtor for occtl_graph so the unique_ptr<MeshCache>
//! member sees the complete MeshCache definition. Defined here in topo
//! because graph lifecycle is owned by topo.
occtl_graph::occtl_graph()
{
  OcctL::Topo::EnsureBuiltinLayers(graph);
}

occtl_graph::~occtl_graph() = default;

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_create(occtl_graph_t** const theOutGraph)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutGraph = nullptr;
    *theOutGraph = new occtl_graph();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_graph_free(occtl_graph_t* const theGraph)
{
  delete theGraph;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_clone(const occtl_graph_t* const theSource,
                                                      occtl_graph_t** const      theOutGraph)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theSource == nullptr || theOutGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theSource or theOutGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutGraph = nullptr;

    BRepGraph aCopy = BRepGraph_Copy::Perform(theSource->graph, true);
    if (!aCopy.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR,
                                             "BRepGraph_Copy::Perform returned !IsDone");
      return OCCTL_ERROR;
    }

    occtl_graph_t* const aNewGraph = new occtl_graph();
    aNewGraph->graph               = std::move(aCopy);
    OcctL::Topo::CopyBuiltinLayers(theSource->graph, aNewGraph->graph);
    *theOutGraph = aNewGraph;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_compact(occtl_graph_t* const theGraph)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    (void)BRepGraph_Compact::Perform(theGraph->graph);
    invalidateMeshCache(theGraph);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_remove_with_replacement(occtl_graph_t* const  theGraph,
                                     const occtl_node_id_t theNode,
                                     const occtl_node_id_t theReplacement)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theNode);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theNode is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const BRepGraph_NodeId aReplId = OcctL::Topo::UnpackNodeId(theReplacement);
    if (!aReplId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "theReplacement is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    theGraph->graph.Editor().Gen().ReplaceNode(aNodeId, aReplId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove_ref(occtl_graph_t* const theGraph,
                                                          const occtl_ref_id_t theRefId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RefId aRefId = OcctL::Topo::UnpackRefId(theRefId);
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theRefId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    const bool anOk = theGraph->graph.Editor().Gen().RemoveRef(aRefId);
    if (!anOk)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theRefId is already removed");
      return OCCTL_NOT_FOUND;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove_rep(occtl_graph_t* const theGraph,
                                                          const occtl_rep_id_t theRepId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RepId aRepId = OcctL::Topo::UnpackRepId(theRepId);
    if (!aRepId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theRepId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    theGraph->graph.Editor().Gen().RemoveRep(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_cleanup_removed_refs(occtl_graph_t* const theGraph)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    theGraph->graph.Editor().CommitMutation();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_set_edge_start_vertex(occtl_graph_t* const  theGraph,
                                   const occtl_node_id_t theEdge,
                                   const occtl_node_id_t theVertex)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    const BRepGraphInc::EdgeDef& anEdgeDef  = theGraph->graph.Topo().Edges().Definition(anEdgeId);
    const BRepGraph_VertexRefId  anOldRefId = anEdgeDef.StartVertexRefId;

    if (!anOldRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "edge has no start vertex ref");
      return OCCTL_NOT_FOUND;
    }

    theGraph->graph.Editor().Vertices().SetRefVertexDefId(anOldRefId, aVertId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_end_vertex(occtl_graph_t* const  theGraph,
                                                                   const occtl_node_id_t theEdge,
                                                                   const occtl_node_id_t theVertex)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    const BRepGraphInc::EdgeDef& anEdgeDef  = theGraph->graph.Topo().Edges().Definition(anEdgeId);
    const BRepGraph_VertexRefId  anOldRefId = anEdgeDef.EndVertexRefId;

    if (!anOldRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "edge has no end vertex ref");
      return OCCTL_NOT_FOUND;
    }

    theGraph->graph.Editor().Vertices().SetRefVertexDefId(anOldRefId, aVertId);
    return OCCTL_OK;
  });
}

} // extern "C"
