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

//! @file ruled_surface.cxx
//! @brief Ruled Face / Shell wrapper around OCCT BRepFill.

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/IdConvert.hxx"
#include "PrimMath.hxx"

#include <occtl/occtl_prim.h>

#include <BRepFill.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>

#include <TCollection_AsciiString.hxx>

namespace
{

occtl_status_t resolveSection(const occtl_graph_t* const theGraph,
                              const occtl_node_id_t      theAbiId,
                              const char* const          theLabel,
                              BRepGraph_NodeId&          theOutNode,
                              TopoDS_Shape&              theOutShape)
{
  theOutNode = OcctL::Topo::UnpackNodeId(theAbiId);
  if (!theOutNode.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " NodeId is invalid or removed"));
    return OCCTL_NOT_FOUND;
  }
  if (theOutNode.NodeKind != BRepGraph_NodeId::Kind::Edge
      && theOutNode.NodeKind != BRepGraph_NodeId::Kind::Wire)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_WRONG_KIND,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " must be an Edge or Wire"));
    return OCCTL_WRONG_KIND;
  }

  theOutShape = theGraph->graph.Shapes().Shape(theOutNode);
  if (theOutShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " could not be reconstructed as TopoDS shape"));
    return OCCTL_NOT_FOUND;
  }
  return OCCTL_OK;
}

int countWireEdges(const TopoDS_Shape& theWire)
{
  int aCount = 0;
  for (TopExp_Explorer anExp(theWire, TopAbs_EDGE); anExp.More(); anExp.Next())
  {
    ++aCount;
  }
  return aCount;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_ruled_surface_info_init(occtl_prim_ruled_surface_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_RULED_SURFACE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_ruled_surface(occtl_graph_t* const                         theGraph,
                                const occtl_prim_ruled_surface_info_t* const theInfo,
                                occtl_node_id_t* const                       theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_RULED_SURFACE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_RULED_SURFACE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aNodeA;
    BRepGraph_NodeId aNodeB;
    TopoDS_Shape     aShapeA;
    TopoDS_Shape     aShapeB;
    if (const occtl_status_t aStatus =
          resolveSection(theGraph, theInfo->section_a, "section_a", aNodeA, aShapeA))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus =
          resolveSection(theGraph, theInfo->section_b, "section_b", aNodeB, aShapeB))
    {
      return aStatus;
    }

    if (aNodeA.NodeKind != aNodeB.NodeKind)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "section_a and section_b must have the same kind");
      return OCCTL_INVALID_ARGUMENT;
    }

    TopoDS_Shape aResult;
    if (aNodeA.NodeKind == BRepGraph_NodeId::Kind::Edge)
    {
      aResult = BRepFill::Face(TopoDS::Edge(aShapeA), TopoDS::Edge(aShapeB));
    }
    else
    {
      if (countWireEdges(aShapeA) != countWireEdges(aShapeB))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                               "wire sections must have the same edge count");
        return OCCTL_INVALID_ARGUMENT;
      }
      aResult = BRepFill::Shell(TopoDS::Wire(aShapeA), TopoDS::Wire(aShapeB));
    }

    if (aResult.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepFill failed to build a ruled surface");
      return OCCTL_GEOMETRY_INVALID;
    }

    return OcctL::Prim::AddTopologyRoot(theGraph, aResult, *theOutShape);
  });
}

} // extern "C"
