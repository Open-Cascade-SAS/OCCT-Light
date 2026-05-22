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
#include "TopoMath.hxx"

#include <BRepGraph_DefsIterator.hxx>
#include <BRepGraph_Iterator.hxx>
#include <BRepGraph_RefsIterator.hxx>
#include <BRepGraph_TopoView.hxx>
#include <BRepGraph_UIDsView.hxx>

#include <occtl/occtl_topo.h>

#include <variant>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

namespace
{

// Visits every active node in the graph (across all 12 kinds), invoking
// theVisit(uid, nodeId) per node.  Order matches the iterator family.
template <typename Visit>
void ForEachActiveNode(const BRepGraph& theGraph, Visit theVisit)
{
  const auto aDo = [&](auto anIter) {
    for (; anIter.More(); anIter.Next())
    {
      const BRepGraph_NodeId aNodeId = anIter.CurrentId();
      const BRepGraph_UID    aUid    = theGraph.UIDs().Of(aNodeId);
      theVisit(aUid, aNodeId);
    }
  };
  aDo(BRepGraph_SolidIterator(theGraph));
  aDo(BRepGraph_ShellIterator(theGraph));
  aDo(BRepGraph_FaceIterator(theGraph));
  aDo(BRepGraph_WireIterator(theGraph));
  aDo(BRepGraph_EdgeIterator(theGraph));
  aDo(BRepGraph_VertexIterator(theGraph));
  aDo(BRepGraph_CompoundIterator(theGraph));
  aDo(BRepGraph_CompSolidIterator(theGraph));
  aDo(BRepGraph_CoEdgeIterator(theGraph));
  aDo(BRepGraph_ProductIterator(theGraph));
  aDo(BRepGraph_OccurrenceIterator(theGraph));
}

// Visits every active reference in the graph, invoking theVisit(refUid, refId).
template <typename Visit>
void ForEachActiveRef(const BRepGraph& theGraph, Visit theVisit)
{
  const auto aDo = [&](auto anIter) {
    for (; anIter.More(); anIter.Next())
    {
      const BRepGraph_RefId  aRefId = anIter.CurrentId();
      const BRepGraph_RefUID aUid   = theGraph.UIDs().Of(aRefId);
      theVisit(aUid, aRefId);
    }
  };
  aDo(BRepGraph_ShellRefIterator(theGraph));
  aDo(BRepGraph_FaceRefIterator(theGraph));
  aDo(BRepGraph_WireRefIterator(theGraph));
  aDo(BRepGraph_CoEdgeRefIterator(theGraph));
  aDo(BRepGraph_VertexRefIterator(theGraph));
  aDo(BRepGraph_SolidRefIterator(theGraph));
  aDo(BRepGraph_ChildRefIterator(theGraph));
  aDo(BRepGraph_OccurrenceRefIterator(theGraph));
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_node_id_from_uid(const occtl_graph_t* const theGraph,
                               const occtl_uid_t          theUid,
                               occtl_node_id_t* const     theOutNodeId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutNodeId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_UID aUid = OcctL::Topo::UnpackUID(theUid);
    if (!theGraph->graph.UIDs().Has(aUid))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "UID not found");
      return OCCTL_NOT_FOUND;
    }

    const BRepGraph_NodeId aNodeId = theGraph->graph.UIDs().NodeIdFrom(aUid);
    *theOutNodeId                  = OcctL::Topo::PackNodeId(aNodeId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_uid_from_node_id(const occtl_graph_t* const theGraph,
                               const occtl_node_id_t      theId,
                               occtl_uid_t* const         theOutUid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutUid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theId);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "invalid NodeId");
      return OCCTL_NOT_FOUND;
    }

    const BRepGraph_UID aUid = theGraph->graph.UIDs().Of(aNodeId);
    *theOutUid               = OcctL::Topo::PackUID(aUid);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_kind(const occtl_graph_t* const theGraph,
                                                          const occtl_node_id_t      theId,
                                                          occtl_node_kind_t* const   theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theId);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "invalid NodeId");
      return OCCTL_NOT_FOUND;
    }

    *theOutKind = OcctL::Topo::ToAbiNodeKind(aNodeId.NodeKind);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_uid_kind(const occtl_graph_t* const theGraph,
                                                         const occtl_uid_t          theUid,
                                                         occtl_node_kind_t* const   theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_UID aUid = OcctL::Topo::UnpackUID(theUid);
    if (!aUid.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "invalid UID");
      return OCCTL_NOT_FOUND;
    }

    *theOutKind = OcctL::Topo::ToAbiNodeKind(aUid.Kind());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_ref_kind(const occtl_graph_t* const theGraph,
                                                         const occtl_ref_id_t       theId,
                                                         occtl_ref_kind_t* const    theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RefId aRefId = OcctL::Topo::UnpackRefId(theId);
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "invalid RefId");
      return OCCTL_NOT_FOUND;
    }

    *theOutKind = OcctL::Topo::ToAbiRefKind(aRefId.RefKind);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_ref_uid_kind(const occtl_graph_t* const theGraph,
                                                             const occtl_ref_uid_t      theUid,
                                                             occtl_ref_kind_t* const    theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RefUID aUid = OcctL::Topo::UnpackRefUID(theUid);
    if (!theGraph->graph.UIDs().Has(aUid))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "RefUID not found");
      return OCCTL_NOT_FOUND;
    }

    *theOutKind = OcctL::Topo::ToAbiRefKind(aUid.Kind());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_rep_kind(const occtl_graph_t* const theGraph,
                                                         const occtl_rep_id_t       theId,
                                                         occtl_rep_kind_t* const    theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RepId aRepId = OcctL::Topo::UnpackRepId(theId);
    if (!aRepId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "invalid RepId");
      return OCCTL_NOT_FOUND;
    }

    *theOutKind = OcctL::Topo::ToAbiRepKind(aRepId.RepKind);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_ref_id_from_ref_uid(const occtl_graph_t* const theGraph,
                                  const occtl_ref_uid_t      theUid,
                                  occtl_ref_id_t* const      theOutRefId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutRefId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RefUID aUid = OcctL::Topo::UnpackRefUID(theUid);
    if (!theGraph->graph.UIDs().Has(aUid))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "RefUID not found");
      return OCCTL_NOT_FOUND;
    }

    const BRepGraph_RefId aRefId = theGraph->graph.UIDs().RefIdFrom(aUid);
    *theOutRefId                 = OcctL::Topo::PackRefId(aRefId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_ref_uid_from_ref_id(const occtl_graph_t* const theGraph,
                                  const occtl_ref_id_t       theId,
                                  occtl_ref_uid_t* const     theOutUid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutUid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RefId aRefId = OcctL::Topo::UnpackRefId(theId);
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "invalid RefId");
      return OCCTL_NOT_FOUND;
    }

    const BRepGraph_RefUID aUid = theGraph->graph.UIDs().Of(aRefId);
    if (!aUid.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "RefId not found");
      return OCCTL_NOT_FOUND;
    }

    *theOutUid = OcctL::Topo::PackRefUID(aUid);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_rep_id_from_rep_uid(const occtl_graph_t* const theGraph,
                                  const occtl_rep_uid_t      theUid,
                                  occtl_rep_id_t* const      theOutRepId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutRepId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RepUID aUid = OcctL::Topo::UnpackRepUID(theUid);
    if (!theGraph->graph.UIDs().Has(aUid))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "RepUID not found");
      return OCCTL_NOT_FOUND;
    }

    const BRepGraph_RepId aRepId = theGraph->graph.UIDs().RepIdFrom(aUid);
    *theOutRepId                 = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_rep_uid_from_rep_id(const occtl_graph_t* const theGraph,
                                  const occtl_rep_id_t       theId,
                                  occtl_rep_uid_t* const     theOutUid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutUid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "null argument");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RepId aRepId = OcctL::Topo::UnpackRepId(theId);
    if (!aRepId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "invalid RepId");
      return OCCTL_NOT_FOUND;
    }

    const BRepGraph_RepUID aUid = theGraph->graph.UIDs().Of(aRepId);
    if (!aUid.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "RepId not found");
      return OCCTL_NOT_FOUND;
    }

    *theOutUid = OcctL::Topo::PackRepUID(aUid);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_ref_uid_to_bytes(const occtl_ref_uid_t theUid,
                                                           uint8_t* const        theOutBytes)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutBytes == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_bytes is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    for (unsigned anIndex = 0; anIndex < 8u; ++anIndex)
    {
      theOutBytes[anIndex] = static_cast<uint8_t>((theUid.bits >> (56u - 8u * anIndex)) & 0xffu);
    }
    for (unsigned anIndex = 8u; anIndex < OCCTL_REF_UID_WIRE_SIZE; ++anIndex)
    {
      theOutBytes[anIndex] = 0u;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_ref_uid_from_bytes(const uint8_t* const   theInBytes,
                                                             occtl_ref_uid_t* const theOutUid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theInBytes == nullptr || theOutUid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theInBytes ? "out_ref_uid is NULL"
                                                        : "in_bytes is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    for (unsigned anIndex = 8u; anIndex < OCCTL_REF_UID_WIRE_SIZE; ++anIndex)
    {
      if (theInBytes[anIndex] != 0u)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_FORMAT_ERROR,
          "occtl_ref_uid_from_bytes: reserved bytes must be zero");
        return OCCTL_FORMAT_ERROR;
      }
    }

    uint64_t aBits = 0u;
    for (unsigned anIndex = 0; anIndex < 8u; ++anIndex)
    {
      aBits = (aBits << 8u) | static_cast<uint64_t>(theInBytes[anIndex]);
    }
    theOutUid->bits = aBits;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_rep_uid_to_bytes(const occtl_rep_uid_t theUid,
                                                           uint8_t* const        theOutBytes)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutBytes == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_bytes is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    for (unsigned anIndex = 0; anIndex < 8u; ++anIndex)
    {
      theOutBytes[anIndex] = static_cast<uint8_t>((theUid.bits >> (56u - 8u * anIndex)) & 0xffu);
    }
    for (unsigned anIndex = 8u; anIndex < OCCTL_REP_UID_WIRE_SIZE; ++anIndex)
    {
      theOutBytes[anIndex] = 0u;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_rep_uid_from_bytes(const uint8_t* const   theInBytes,
                                                             occtl_rep_uid_t* const theOutUid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theInBytes == nullptr || theOutUid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theInBytes ? "out_rep_uid is NULL"
                                                        : "in_bytes is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    for (unsigned anIndex = 8u; anIndex < OCCTL_REP_UID_WIRE_SIZE; ++anIndex)
    {
      if (theInBytes[anIndex] != 0u)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_FORMAT_ERROR,
          "occtl_rep_uid_from_bytes: reserved bytes must be zero");
        return OCCTL_FORMAT_ERROR;
      }
    }

    uint64_t aBits = 0u;
    for (unsigned anIndex = 0; anIndex < 8u; ++anIndex)
    {
      aBits = (aBits << 8u) | static_cast<uint64_t>(theInBytes[anIndex]);
    }
    theOutUid->bits = aBits;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_uid_table(const occtl_graph_t* const theGraph,
                                                          occtl_uid_t* const         theOutUids,
                                                          occtl_node_id_t* const     theOutNodes,
                                                          const size_t               theCap,
                                                          size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_count is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    // Sizing-only call: both output arrays NULL.
    if (theOutUids == nullptr && theOutNodes == nullptr)
    {
      size_t aCount = 0;
      ForEachActiveNode(theGraph->graph,
                        [&](const BRepGraph_UID&, const BRepGraph_NodeId&) { ++aCount; });
      *theOutCount = aCount;
      return OCCTL_OK;
    }

    // Refill: first measure, then either fail-fast or write.
    size_t aTotal = 0;
    ForEachActiveNode(theGraph->graph,
                      [&](const BRepGraph_UID&, const BRepGraph_NodeId&) { ++aTotal; });
    *theOutCount = aTotal;
    if (theCap < aTotal)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "occtl_graph_uid_table: cap < total UID count");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    size_t aIdx = 0;
    ForEachActiveNode(theGraph->graph,
                      [&](const BRepGraph_UID& theUid, const BRepGraph_NodeId& theNode) {
                        if (theOutUids != nullptr)
                        {
                          theOutUids[aIdx] = OcctL::Topo::PackUID(theUid);
                        }
                        if (theOutNodes != nullptr)
                        {
                          theOutNodes[aIdx] = OcctL::Topo::PackNodeId(theNode);
                        }
                        ++aIdx;
                      });
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_ref_uid_table(const occtl_graph_t* const theGraph,
                                                              occtl_ref_uid_t* const     theOutUids,
                                                              occtl_ref_id_t* const      theOutRefs,
                                                              const size_t               theCap,
                                                              size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_count is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if ((theOutUids == nullptr) != (theOutRefs == nullptr))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_graph_ref_uid_table: output arrays must both be NULL or both be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    size_t aTotal = 0;
    ForEachActiveRef(theGraph->graph,
                     [&](const BRepGraph_RefUID&, const BRepGraph_RefId&) { ++aTotal; });
    *theOutCount = aTotal;
    if (theOutUids == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aTotal)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "occtl_graph_ref_uid_table: cap < total RefUID count");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    size_t aIdx = 0;
    ForEachActiveRef(theGraph->graph,
                     [&](const BRepGraph_RefUID& theUid, const BRepGraph_RefId& theRef) {
                       theOutUids[aIdx] = OcctL::Topo::PackRefUID(theUid);
                       theOutRefs[aIdx] = OcctL::Topo::PackRefId(theRef);
                       ++aIdx;
                     });
    return OCCTL_OK;
  });
}

} // extern "C"
