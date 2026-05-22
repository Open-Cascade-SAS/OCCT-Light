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

//! @file history.cxx
//! @brief extern "C" shims for graph-owned topology history queries.

#include "GraphHandle.hxx"
#include "TopoMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <occtl/occtl_topo.h>

#include <BRepGraph_LayerHistory.hxx>
#include <BRepGraph_LayerRegistry.hxx>
#include <BRepGraph_UID.hxx>
#include <BRepGraph_UIDsView.hxx>

#include <NCollection_DynamicArray.hxx>

namespace
{

const BRepGraph_LayerHistory* findHistory(const BRepGraph& theGraph)
{
  const occ::handle<BRepGraph_LayerHistory> aHistory =
    theGraph.LayerRegistry().Find<BRepGraph_LayerHistory>();
  return aHistory.get();
}

occtl_status_t emitUidArray(const NCollection_DynamicArray<BRepGraph_UID>& theData,
                            occtl_uid_t* const                             theOutBuf,
                            const size_t                                   theCap,
                            size_t* const                                  theOutCount) noexcept
{
  const size_t aRequired = theData.Size();
  *theOutCount           = aRequired;

  if (theOutBuf == nullptr)
  {
    return OCCTL_OK;
  }
  if (theCap < aRequired)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                           "out_buf capacity is smaller than the required size");
    return OCCTL_BUFFER_TOO_SMALL;
  }

  size_t aWriteIdx = 0;
  for (const BRepGraph_UID& aUID : theData)
  {
    theOutBuf[aWriteIdx++] = OcctL::Topo::PackUID(aUID);
  }
  return OCCTL_OK;
}

occtl_status_t prepareGraphHistoryLookup(const occtl_graph_t* const     theGraph,
                                         const occtl_uid_t              theInputUid,
                                         size_t* const                  theOutCount,
                                         BRepGraph_UID&                 theOutUid,
                                         const BRepGraph_LayerHistory*& theOutHistory) noexcept
{
  if (theGraph == nullptr || theOutCount == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_count is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }

  theOutUid = OcctL::Topo::UnpackUID(theInputUid);
  if (!theOutUid.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "input_uid bit-pattern is malformed");
    return OCCTL_INVALID_ARGUMENT;
  }

  theOutHistory = findHistory(theGraph->graph);
  if (!theGraph->graph.UIDs().Has(theOutUid)
      && (theOutHistory == nullptr || !theOutHistory->HasKnownInput(theOutUid)))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                           "input_uid does not exist in graph or history");
    return OCCTL_NOT_FOUND;
  }

  if (theOutHistory == nullptr || !theOutHistory->HasKnownInput(theOutUid))
  {
    theOutHistory = nullptr;
  }
  return OCCTL_OK;
}

} // namespace

extern "C"
{

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_history_modified(const occtl_graph_t* const graph,
                                                                 const occtl_uid_t  input_uid,
                                                                 occtl_uid_t* const out_buf,
                                                                 const size_t       cap,
                                                                 size_t* const      out_count)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_UID                 aUid;
    const BRepGraph_LayerHistory* aHistory = nullptr;
    if (const occtl_status_t aStatus =
          prepareGraphHistoryLookup(graph, input_uid, out_count, aUid, aHistory))
    {
      return aStatus;
    }
    if (aHistory == nullptr)
    {
      const NCollection_DynamicArray<BRepGraph_UID> anEmpty;
      return emitUidArray(anEmpty, out_buf, cap, out_count);
    }
    const NCollection_DynamicArray<BRepGraph_UID> aImages =
      aHistory->FindModified(graph->graph, aUid);
    return emitUidArray(aImages, out_buf, cap, out_count);
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_history_generated(const occtl_graph_t* const graph,
                                                                  const occtl_uid_t  input_uid,
                                                                  occtl_uid_t* const out_buf,
                                                                  const size_t       cap,
                                                                  size_t* const      out_count)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_UID                 aUid;
    const BRepGraph_LayerHistory* aHistory = nullptr;
    if (const occtl_status_t aStatus =
          prepareGraphHistoryLookup(graph, input_uid, out_count, aUid, aHistory))
    {
      return aStatus;
    }
    if (aHistory == nullptr)
    {
      const NCollection_DynamicArray<BRepGraph_UID> anEmpty;
      return emitUidArray(anEmpty, out_buf, cap, out_count);
    }
    const NCollection_DynamicArray<BRepGraph_UID> aImages =
      aHistory->FindGenerated(graph->graph, aUid);
    return emitUidArray(aImages, out_buf, cap, out_count);
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_history_deleted_all(const occtl_graph_t* const graph,
                                  occtl_uid_t* const         out_buf,
                                  const size_t               cap,
                                  size_t* const              out_count)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_count == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_LayerHistory* aHistory = findHistory(graph->graph);
    if (aHistory == nullptr)
    {
      *out_count = 0;
      return OCCTL_OK;
    }
    const NCollection_DynamicArray<BRepGraph_UID> aImages = aHistory->DeletedUids(graph->graph);
    return emitUidArray(aImages, out_buf, cap, out_count);
  });
}

} // extern "C"
