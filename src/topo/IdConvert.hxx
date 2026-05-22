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

//! @file IdConvert.hxx
//! @brief Typed-ID validation and orientation-conversion helpers for the topo module.
//!
//! Extracted from graph_geom.cxx so graph_iterators.cxx and graph_mutation.cxx
//! can reuse the same validation logic without duplication.

#ifndef OCCTL_TOPO_ID_CONVERT_HXX
#define OCCTL_TOPO_ID_CONVERT_HXX

#include "GraphHandle.hxx"
#include "TopoMath.hxx"

#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"

#include <TopAbs_Orientation.hxx>

namespace OcctL::Topo
{

//! Validates the graph pointer, unpacks @p theAbiId, checks that it has
//! kind @p TheKind (using BRepGraph_NodeId::Kind), and on success
//! returns the typed ID through @p theOutId.  On failure the thread-local
//! error state is populated and a non-OK status is returned.
//!
//! Call pattern:
//!   BRepGraph_VertexId aVertId;
//!   if (const auto aErr = ToTypedId(theGraph, theAbiVertex,
//!                                    BRepGraph_NodeId::Kind::Vertex, aVertId))
//!     return aErr;
template <typename TheTypedId>
inline occtl_status_t ToTypedId(const occtl_graph_t* const   theGraph,
                                occtl_node_id_t              theAbiId,
                                const BRepGraph_NodeId::Kind theKind,
                                TheTypedId&                  theOutId) noexcept
{
  if (theGraph == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }

  const BRepGraph_NodeId aNodeId = UnpackNodeId(theAbiId);
  if (!aNodeId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "NodeId is invalid or removed");
    return OCCTL_NOT_FOUND;
  }
  if (aNodeId.NodeKind != theKind)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "NodeId has wrong kind");
    return OCCTL_WRONG_KIND;
  }

  theOutId = TheTypedId(aNodeId);
  return OCCTL_OK;
}

//! Simpler overload: validates the graph, unpacks, checks kind, and
//! returns the typed ID on success.  On failure the error state is
//! populated with an appropriate message via ToTypedId above.
template <typename TheTypedId>
inline occtl_status_t ToTypedId(const occtl_graph_t* const   theGraph,
                                const occtl_node_id_t        theAbiId,
                                const BRepGraph_NodeId::Kind theKind,
                                BRepGraph_NodeId&            theOutNodeId) noexcept
{
  return ToTypedId<BRepGraph_NodeId>(theGraph, theAbiId, theKind, theOutNodeId);
}

//! Converts an ABI orientation to OCCT's TopAbs_Orientation.
//! ABI values mirror TopAbs_Orientation 1:1 in numeric order.
inline TopAbs_Orientation ToOcctOrientation(const occtl_orientation_t theOri) noexcept
{
  return static_cast<TopAbs_Orientation>(theOri);
}

//! Converts OCCT's TopAbs_Orientation to the ABI orientation.
inline occtl_orientation_t FromOcctOrientation(const TopAbs_Orientation theOri) noexcept
{
  return static_cast<occtl_orientation_t>(theOri);
}

} // namespace OcctL::Topo

#endif //!< OCCTL_TOPO_ID_CONVERT_HXX
