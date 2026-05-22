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

//! @file constrained_curve_edge.cxx
//! @brief Inserts 2D constrained-curve solver results as graph Edge nodes.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/CurveMath.hxx"
#include "../geom/RepLookup.hxx"

#include <occtl/occtl_prim.h>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GeomAPI.hxx>
#include <Geom_Curve.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>

#include <cmath>

namespace
{

bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool isFinitePoint(const occtl_point3_t& thePoint) noexcept
{
  return IsFiniteValue(thePoint.x) && IsFiniteValue(thePoint.y) && IsFiniteValue(thePoint.z);
}

bool isFiniteDirection(const occtl_direction3_t& theDirection) noexcept
{
  return IsFiniteValue(theDirection.x) && IsFiniteValue(theDirection.y)
         && IsFiniteValue(theDirection.z);
}

occtl_status_t checkPlacement(const occtl_axis2_placement_t& thePlacement)
{
  if (!isFinitePoint(thePlacement.location) || !isFiniteDirection(thePlacement.x_dir)
      || !isFiniteDirection(thePlacement.x_dir_ref))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "placement contains a non-finite coordinate");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (const occtl_status_t aStatus =
        OcctL::Prim::CheckDirection(thePlacement.x_dir, "placement.x_dir"))
  {
    return aStatus;
  }
  if (const occtl_status_t aStatus =
        OcctL::Prim::CheckDirection(thePlacement.x_dir_ref, "placement.x_dir_ref"))
  {
    return aStatus;
  }

  const gp_Vec anAxis(thePlacement.x_dir.x, thePlacement.x_dir.y, thePlacement.x_dir.z);
  const gp_Vec aRef(thePlacement.x_dir_ref.x, thePlacement.x_dir_ref.y, thePlacement.x_dir_ref.z);
  if (anAxis.Crossed(aRef).SquareMagnitude() <= Precision::SquareConfusion())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "placement.x_dir_ref is parallel to placement.x_dir");
    return OCCTL_INVALID_ARGUMENT;
  }

  return OCCTL_OK;
}

} // namespace

extern "C"
{

OCCTL_API void OCCTL_CALL
  occtl_prim_constrained_edge_info_init(occtl_prim_constrained_edge_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_prim_constrained_edge_info_t anInit = OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT;
    *theInfo                                        = anInit;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_constrained_edge(occtl_graph_t* const                            theGraph,
                                   const occtl_prim_constrained_edge_info_t* const theInfo,
                                   occtl_node_id_t* const                          theOutEdge)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutEdge == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_edge is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutEdge = OCCTL_NODE_ID_INVALID;

    if (theInfo->struct_version != OCCTL_PRIM_CONSTRAINED_EDGE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_CONSTRAINED_EDGE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->curve.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->curve is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = checkPlacement(theInfo->placement))
    {
      return aStatus;
    }
    if (theInfo->use_parameter_range != 0 && theInfo->use_parameter_range != 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "use_parameter_range must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepBuilderAPI_MakeEdge          anEdgeMaker;
    const gp_Ax2                     aFrame = OcctL::Geom::ToGpAx2(theInfo->placement);
    const gp_Pln                     aPlane(aFrame);
    const occ::handle<Geom2d_Curve>& aCurve2d =
      OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve);
    const occ::handle<Geom_Curve> aCurve3d = GeomAPI::To3d(aCurve2d, aPlane);
    if (aCurve3d.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "GeomAPI::To3d returned a NULL curve");
      return OCCTL_GEOMETRY_INVALID;
    }

    if (theInfo->use_parameter_range != 0)
    {
      if (!IsFiniteValue(theInfo->first_parameter) || !IsFiniteValue(theInfo->last_parameter)
          || theInfo->last_parameter <= theInfo->first_parameter)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                               "parameter range must be finite and increasing");
        return OCCTL_INVALID_ARGUMENT;
      }
      anEdgeMaker =
        BRepBuilderAPI_MakeEdge(aCurve3d, theInfo->first_parameter, theInfo->last_parameter);
    }
    else
    {
      anEdgeMaker = BRepBuilderAPI_MakeEdge(aCurve3d);
    }

    anEdgeMaker.Build();
    if (!anEdgeMaker.IsDone() || anEdgeMaker.Edge().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "BRepBuilderAPI_MakeEdge failed for constrained 2D curve");
      return OCCTL_GEOMETRY_INVALID;
    }

    return OcctL::Prim::AddTopologyRoot(theGraph, anEdgeMaker.Edge(), *theOutEdge);
  });
}

} // extern "C"
