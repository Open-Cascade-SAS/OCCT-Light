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

//! @file helix.cxx
//! @brief Helix wire builder.  The helix is constructed as a straight 2D
//!        line on a Geom_CylindricalSurface in (u, v) parameter space,
//!        where u is the azimuth angle and v is the height; the resulting
//!        curve-on-surface is then lifted to a 3D edge via
//!        BRepLib::BuildCurve3d.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/CurveMath.hxx"

#include <occtl/occtl_prim.h>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepLib.hxx>
#include <Geom2d_Line.hxx>
#include <Geom_CylindricalSurface.hxx>

#include <gp_Ax3.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Pnt2d.hxx>

#include <cmath>

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_helix_info_init(occtl_prim_helix_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_helix(occtl_graph_t* const                 theGraph,
                        const occtl_prim_helix_info_t* const theInfo,
                        occtl_node_id_t* const               theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_HELIX_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_HELIX_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_helix_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinitePlacement(theInfo->placement, "helix") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->radius, "radius") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->pitch, "pitch") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->height, "height") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->left_handed, "left_handed") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Ax2 anAx2 = OcctL::Geom::ToGpAx2(theInfo->placement);
    const gp_Ax3 anAx3(anAx2);

    occ::handle<Geom_CylindricalSurface> aCyl = new Geom_CylindricalSurface(anAx3, theInfo->radius);

    const double   aSign  = (theInfo->left_handed != 0) ? -1.0 : 1.0;
    const double   aSlope = theInfo->pitch / (2.0 * OCCTL_PI);
    const gp_Dir2d aDir2d(aSign, aSlope);

    occ::handle<Geom2d_Line> aLine2d = new Geom2d_Line(gp_Pnt2d(0.0, 0.0), aDir2d);

    // Parameter end: gp_Dir2d normalises to unit length, so unit parameter
    // step traverses unit length on the line.  Unit step components in
    // (u, v) are aSign / sqrt(1 + slope^2)  and  aSlope / sqrt(1 + slope^2).
    // To reach v = height, the parameter must be
    //   t_end = height * sqrt(1 + slope^2) / aSlope
    const double aSlopeSq  = aSlope * aSlope;
    const double aParamEnd = theInfo->height * std::sqrt(1.0 + aSlopeSq) / aSlope;

    BRepBuilderAPI_MakeEdge anEdgeMaker(aLine2d, aCyl, 0.0, aParamEnd);
    anEdgeMaker.Build();
    if (!anEdgeMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepBuilderAPI_MakeEdge failed for helix");
      return OCCTL_GEOMETRY_INVALID;
    }

    TopoDS_Edge anEdge = anEdgeMaker.Edge();
    // Compute the 3D curve from the curve-on-surface; without this the
    // edge has only the pcurve, which trips up downstream consumers.
    if (!BRepLib::BuildCurve3d(anEdge))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepLib::BuildCurve3d failed for helix edge");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepBuilderAPI_MakeWire aWireMaker(anEdge);
    aWireMaker.Build();
    if (!aWireMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepBuilderAPI_MakeWire failed for helix");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aWireMaker.Wire(), *theOutWire);
  });
}

} // extern "C"
