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
#include "IdConvert.hxx"

#include <BRepGraphInc_Definition.hxx>
#include <BRepGraph_TopoView.hxx>

#include <occtl/occtl_topo.h>

#include "../core/Guard.hxx"

namespace
{

template <typename TheQuery>
occtl_status_t Count(const occtl_graph_t* const theGraph,
                     size_t* const              theOutCount,
                     TheQuery&&                 theQuery) noexcept
{
  if (theGraph == nullptr || theOutCount == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "graph and out_count must be non-NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  *theOutCount = static_cast<size_t>(theQuery(theGraph->graph));
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_solid_count(const occtl_graph_t* const theGraph,
                                                            size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Solids().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_shell_count(const occtl_graph_t* const theGraph,
                                                            size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Shells().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_face_count(const occtl_graph_t* const theGraph,
                                                           size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Faces().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_wire_count(const occtl_graph_t* const theGraph,
                                                           size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Wires().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_edge_count(const occtl_graph_t* const theGraph,
                                                           size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Edges().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_vertex_count(const occtl_graph_t* const theGraph,
                                                             size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Vertices().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_compound_count(const occtl_graph_t* const theGraph,
                                                               size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Compounds().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_compsolid_count(const occtl_graph_t* const theGraph,
                                                                size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().CompSolids().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_coedge_count(const occtl_graph_t* const theGraph,
                                                             size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().CoEdges().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_product_count(const occtl_graph_t* const theGraph,
                                                              size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Products().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_occurrence_count(const occtl_graph_t* const theGraph, size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Occurrences().NbActive();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_node_count(const occtl_graph_t* const theGraph,
                                                           size_t* const              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return Count(theGraph, theOutCount, [](const BRepGraph& theBrep) {
      return theBrep.Topo().Gen().NbNodes();
    });
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_compound_child_count(const occtl_graph_t* const theGraph,
                                  const occtl_node_id_t      theCompound,
                                  uint32_t* const            theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_CompoundId aCompoundId;
    if (const occtl_status_t aErr = OcctL::Topo::ToTypedId(theGraph,
                                                           theCompound,
                                                           BRepGraph_NodeId::Kind::Compound,
                                                           aCompoundId))
    {
      return aErr;
    }
    *theOutCount = static_cast<uint32_t>(
      theGraph->graph.Topo().Compounds().Definition(aCompoundId).ChildRefIds.Size());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_compsolid_solid_count(const occtl_graph_t* const theGraph,
                                   const occtl_node_id_t      theCompSolid,
                                   uint32_t* const            theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_CompSolidId aCompSolidId;
    if (const occtl_status_t aErr = OcctL::Topo::ToTypedId(theGraph,
                                                           theCompSolid,
                                                           BRepGraph_NodeId::Kind::CompSolid,
                                                           aCompSolidId))
    {
      return aErr;
    }
    *theOutCount = static_cast<uint32_t>(
      theGraph->graph.Topo().CompSolids().Definition(aCompSolidId).SolidRefIds.Size());
    return OCCTL_OK;
  });
}

} // extern "C"
