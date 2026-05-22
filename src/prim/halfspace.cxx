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

//! @file halfspace.cxx
//! @brief HalfSpace primitive: an infinite solid bounded by a face on the
//!        side of a reference point.  Exercises the Graph -> TopoDS
//!        round-trip: reads the bounding Face out of the graph via
//!        BRepGraph::Shapes() and feeds it to BRepPrimAPI_MakeHalfSpace.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/GeomMath.hxx"
#include "../topo/IdConvert.hxx"

#include <occtl/occtl_prim.h>

#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <Standard_ErrorHandler.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <gp_Pnt.hxx>

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_halfspace(occtl_graph_t* const                     theGraph,
                            const occtl_prim_halfspace_info_t* const theInfo,
                            occtl_node_id_t* const                   theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_HALFSPACE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_HALFSPACE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (const occtl_status_t aStatus =
          OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_halfspace_info_t"))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus =
          OcctL::Prim::CheckFiniteVec3(theInfo->reference_point, "reference_point"))
    {
      return aStatus;
    }
    *theOutSolid = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theInfo->face, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    const TopoDS_Shape aFaceShape = theGraph->graph.Shapes().Shape(aFaceId);
    if (aFaceShape.IsNull() || aFaceShape.ShapeType() != TopAbs_FACE)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "face NodeId could not be reconstructed as TopoDS_Face");
      return OCCTL_NOT_FOUND;
    }
    const TopoDS_Face& aFace = TopoDS::Face(aFaceShape);
    const gp_Pnt       aRef  = OcctL::Geom::ToGp(theInfo->reference_point);

    OCC_CATCH_SIGNALS;
    BRepPrimAPI_MakeHalfSpace aMaker(aFace, aRef);
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepPrimAPI_MakeHalfSpace reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Solid(), *theOutSolid);
  });
}

} // extern "C"
