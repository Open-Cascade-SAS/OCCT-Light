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

//! @file thru_sections.cxx
//! @brief Loft (through-sections) primitive: skin a sequence of Wire /
//!        Vertex section nodes into a shell or solid via
//!        BRepOffsetAPI_ThruSections.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/IdConvert.hxx"

#include <occtl/occtl_prim.h>

#include <BRepOffsetAPI_ThruSections.hxx>
#include <Standard_ErrorHandler.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_loft(occtl_graph_t* const                theGraph,
                       const occtl_prim_loft_info_t* const theInfo,
                       occtl_node_id_t* const              theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_LOFT_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_LOFT_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (const occtl_status_t aStatus =
          OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_loft_info_t"))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus = OcctL::Prim::CheckBool(theInfo->is_solid, "is_solid"))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus = OcctL::Prim::CheckBool(theInfo->ruled, "ruled"))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus = OcctL::Prim::CheckPositive(theInfo->pres3d, "pres3d"))
    {
      return aStatus;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (theInfo->sections == nullptr || theInfo->section_count < 2)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "loft requires at least 2 sections in a non-NULL array");
      return OCCTL_INVALID_ARGUMENT;
    }

    OCC_CATCH_SIGNALS;
    BRepOffsetAPI_ThruSections aMaker(theInfo->is_solid != 0, theInfo->ruled != 0, theInfo->pres3d);

    for (size_t anI = 0; anI < theInfo->section_count; ++anI)
    {
      const occtl_node_id_t         aSectionAbi = theInfo->sections[anI];
      const BRepGraph_NodeId        aSectionId  = OcctL::Topo::UnpackNodeId(aSectionAbi);
      const TCollection_AsciiString aSectionLabel =
        TCollection_AsciiString("section[") + TCollection_AsciiString(static_cast<int>(anI)) + "]";
      if (!aSectionId.IsValid())
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_NOT_FOUND,
          static_cast<std::string_view>(aSectionLabel + " NodeId is invalid or removed"));
        return OCCTL_NOT_FOUND;
      }

      const bool anIsVertex = aSectionId.NodeKind == BRepGraph_NodeId::Kind::Vertex;
      const bool anIsWire   = aSectionId.NodeKind == BRepGraph_NodeId::Kind::Wire;
      if (!anIsVertex && !anIsWire)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_WRONG_KIND,
          static_cast<std::string_view>(aSectionLabel + " must be a Wire or Vertex"));
        return OCCTL_WRONG_KIND;
      }
      if (anIsVertex && anI != 0 && anI != theInfo->section_count - 1)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_WRONG_KIND,
          static_cast<std::string_view>(TCollection_AsciiString("Vertex ") + aSectionLabel
                                        + " may only appear at the first or last position"));
        return OCCTL_WRONG_KIND;
      }

      const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(aSectionId);
      if (aShape.IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_NOT_FOUND,
          static_cast<std::string_view>(aSectionLabel
                                        + " could not be reconstructed as TopoDS shape"));
        return OCCTL_NOT_FOUND;
      }
      if (anIsWire)
      {
        aMaker.AddWire(TopoDS::Wire(aShape));
      }
      else
      {
        aMaker.AddVertex(TopoDS::Vertex(aShape));
      }
    }

    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepOffsetAPI_ThruSections reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutShape);
  });
}

} // extern "C"
