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

//! @file swept_solids.cxx
//! @brief Swept primitives: Prism (linear extrusion), Revol (revolution), and
//!        Pipe (sweep along a spine wire).  Each entry point resolves its
//!        input profile (and, for Pipe, the spine wire) back to a
//!        TopoDS_Shape via BRepGraph::Shapes(), runs the OCCT sweep
//!        algorithm, and merges the result back into the same graph.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/CurveMath.hxx"
#include "../topo/IdConvert.hxx"

#include <occtl/occtl_prim.h>

#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>

#include <gp_Ax1.hxx>
#include <gp_Vec.hxx>

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_prism(occtl_graph_t* const                 theGraph,
                        const occtl_prim_prism_info_t* const theInfo,
                        occtl_node_id_t* const               theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_PRISM_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_PRISM_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_prism_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFiniteVec3(theInfo->direction, "direction") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->copy, "copy") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->canonize, "canonize") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = OcctL::Prim::CheckProfileKind(theInfo->profile, "profile"))
    {
      return aStatus;
    }

    if (const occtl_status_t aStatus = OcctL::Prim::CheckDirection(theInfo->direction, "direction"))
    {
      return aStatus;
    }

    TopoDS_Shape aProfile;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->profile, "profile", aProfile))
    {
      return aStatus;
    }

    const gp_Vec          aVec = OcctL::Geom::ToGp(theInfo->direction);
    BRepPrimAPI_MakePrism aMaker(aProfile, aVec, theInfo->copy != 0, theInfo->canonize != 0);
    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepPrimAPI_MakePrism reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutShape);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_revol(occtl_graph_t* const                 theGraph,
                        const occtl_prim_revol_info_t* const theInfo,
                        occtl_node_id_t* const               theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_REVOL_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_REVOL_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_revol_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFiniteAxis1(theInfo->axis, "axis") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle, "angle") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->copy, "copy") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = OcctL::Prim::CheckProfileKind(theInfo->profile, "profile"))
    {
      return aStatus;
    }

    if (const occtl_status_t aStatus =
          OcctL::Prim::CheckDirection(theInfo->axis.direction, "axis direction"))
    {
      return aStatus;
    }

    const occtl_direction3_t& aDir = theInfo->axis.direction;
    const gp_Ax1 anAxis(OcctL::Geom::ToGp(theInfo->axis.location), gp_Dir(aDir.x, aDir.y, aDir.z));

    TopoDS_Shape aProfile;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->profile, "profile", aProfile))
    {
      return aStatus;
    }

    BRepPrimAPI_MakeRevol aMaker(aProfile, anAxis, theInfo->angle, theInfo->copy != 0);
    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepPrimAPI_MakeRevol reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutShape);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_pipe(occtl_graph_t* const                theGraph,
                       const occtl_prim_pipe_info_t* const theInfo,
                       occtl_node_id_t* const              theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_PIPE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_PIPE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_pipe_info_t") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = OcctL::Prim::CheckProfileKind(theInfo->profile, "profile"))
    {
      return aStatus;
    }

    BRepGraph_NodeId aSpineId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theInfo->spine_wire,
                                                              BRepGraph_NodeId::Kind::Wire,
                                                              aSpineId))
    {
      return aStatus;
    }

    const TopoDS_Shape aSpineShape = theGraph->graph.Shapes().Shape(aSpineId);
    if (aSpineShape.IsNull() || aSpineShape.ShapeType() != TopAbs_WIRE)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "spine_wire NodeId could not be reconstructed as TopoDS_Wire");
      return OCCTL_NOT_FOUND;
    }
    const TopoDS_Wire& aSpine = TopoDS::Wire(aSpineShape);

    TopoDS_Shape aProfile;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->profile, "profile", aProfile))
    {
      return aStatus;
    }

    BRepOffsetAPI_MakePipe aMaker(aSpine, aProfile);
    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepOffsetAPI_MakePipe reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutShape);
  });
}

} // extern "C"
