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

//! @file convenience.cxx
//! @brief Convenience 2D primitives: Plane (rect face directly), Disk
//!        (circle face directly), Slot (stadium wire — two parallel lines
//!        connected by two semicircles), and Tube (hollow cylinder solid).

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/CurveMath.hxx"

#include <occtl/occtl_prim.h>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <GC_MakeArcOfCircle.hxx>

#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_slot_info_init(occtl_prim_slot_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_SLOT_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_tube_info_init(occtl_prim_tube_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_TUBE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_slot(occtl_graph_t* const                theGraph,
                       const occtl_prim_slot_info_t* const theInfo,
                       occtl_node_id_t* const              theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_SLOT_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_SLOT_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_slot_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinitePlacement(theInfo->placement, "slot") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->width, "width") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->length, "length") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->length <= theInfo->width)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "slot requires width > 0 and length > width");
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Ax2 anAxes = OcctL::Geom::ToGpAx2(theInfo->placement);
    const gp_Pnt aC     = anAxes.Location();
    const gp_Vec aX     = gp_Vec(anAxes.XDirection());
    const gp_Vec aY     = gp_Vec(anAxes.YDirection());
    const double aHalfL = 0.5 * theInfo->length;
    const double aR     = 0.5 * theInfo->width;

    const gp_Pnt aP1       = aC.Translated(aX * (-aHalfL + aR) + aY * (aR));
    const gp_Pnt aP2       = aC.Translated(aX * (aHalfL - aR) + aY * (aR));
    const gp_Pnt aP3       = aC.Translated(aX * (aHalfL - aR) + aY * (-aR));
    const gp_Pnt aP4       = aC.Translated(aX * (-aHalfL + aR) + aY * (-aR));
    const gp_Pnt aRightTip = aC.Translated(aX * (aHalfL));
    const gp_Pnt aLeftTip  = aC.Translated(aX * (-aHalfL));

    GC_MakeArcOfCircle aRightArc(aP2, aRightTip, aP3);
    GC_MakeArcOfCircle aLeftArc(aP4, aLeftTip, aP1);
    if (!aRightArc.IsDone() || !aLeftArc.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "Slot cap-arc construction failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepBuilderAPI_MakeWire aWireMaker;
    aWireMaker.Add(BRepBuilderAPI_MakeEdge(aP1, aP2).Edge());
    aWireMaker.Add(BRepBuilderAPI_MakeEdge(aRightArc.Value()).Edge());
    aWireMaker.Add(BRepBuilderAPI_MakeEdge(aP3, aP4).Edge());
    aWireMaker.Add(BRepBuilderAPI_MakeEdge(aLeftArc.Value()).Edge());
    aWireMaker.Build();
    if (!aWireMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepBuilderAPI_MakeWire failed assembling slot wire");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aWireMaker.Wire(), *theOutWire);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_tube(occtl_graph_t* const                theGraph,
                       const occtl_prim_tube_info_t* const theInfo,
                       occtl_node_id_t* const              theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_TUBE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_TUBE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutSolid = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_tube_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinitePlacement(theInfo->placement, "tube") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->inner_radius, "inner_radius") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->outer_radius, "outer_radius") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->height, "height") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->outer_radius <= theInfo->inner_radius)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "outer_radius must be greater than inner_radius");
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Ax2 anAxes  = OcctL::Geom::ToGpAx2(theInfo->placement);
    const gp_Pnt aOrigin = anAxes.Location();
    const gp_Vec aX      = gp_Vec(anAxes.XDirection());
    const gp_Vec aZ      = gp_Vec(anAxes.Direction());

    const gp_Pnt aP0 = aOrigin.Translated(aX * theInfo->inner_radius);
    const gp_Pnt aP1 = aOrigin.Translated(aX * theInfo->outer_radius);
    const gp_Pnt aP2 = aOrigin.Translated(aX * theInfo->outer_radius + aZ * theInfo->height);
    const gp_Pnt aP3 = aOrigin.Translated(aX * theInfo->inner_radius + aZ * theInfo->height);

    BRepBuilderAPI_MakePolygon aPoly(aP0, aP1, aP2, aP3, /* Close */ true);
    aPoly.Build();
    if (!aPoly.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "Failed building tube cross-section polygon");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepBuilderAPI_MakeFace aFace(aPoly.Wire(), /* OnlyPlane */ true);
    if (!aFace.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "Failed building tube cross-section face");
      return OCCTL_GEOMETRY_INVALID;
    }

    const gp_Ax1          aRevAxis(aOrigin, anAxes.Direction());
    BRepPrimAPI_MakeRevol aRevol(aFace.Face(), aRevAxis);
    aRevol.Build();
    if (!aRevol.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepPrimAPI_MakeRevol failed for tube");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aRevol.Shape(), *theOutSolid);
  });
}

} // extern "C"
