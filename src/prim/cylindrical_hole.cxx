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

//! @file cylindrical_hole.cxx
//! @brief Cylindrical-hole feature wrapper around BRepFeat_MakeCylindricalHole.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/GeomMath.hxx"

#include <occtl/occtl_prim.h>

#include <BRepFeat_MakeCylindricalHole.hxx>
#include <BRepFeat_Status.hxx>
#include <Standard_ErrorHandler.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shape.hxx>

#include <TCollection_AsciiString.hxx>

namespace
{

const char* statusName(const BRepFeat_Status theStatus) noexcept
{
  switch (theStatus)
  {
    case BRepFeat_NoError:
      return "NoError";
    case BRepFeat_InvalidPlacement:
      return "InvalidPlacement";
    case BRepFeat_HoleTooLong:
      return "HoleTooLong";
  }
  return "Unknown";
}

TopoDS_Shape unwrapSingleChildCompound(const TopoDS_Shape& theShape)
{
  if (theShape.IsNull() || theShape.ShapeType() != TopAbs_COMPOUND)
  {
    return theShape;
  }

  TopoDS_Shape aChild;
  int          aChildCount = 0;
  for (TopoDS_Iterator anIt(theShape); anIt.More(); anIt.Next())
  {
    aChild = anIt.Value();
    ++aChildCount;
    if (aChildCount > 1)
    {
      return theShape;
    }
  }
  return aChildCount == 1 ? aChild : theShape;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_cylindrical_hole_info_init(occtl_prim_cylindrical_hole_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_cylindrical_hole(occtl_graph_t* const                            theGraph,
                                   const occtl_prim_cylindrical_hole_info_t* const theInfo,
                                   occtl_node_id_t* const                          theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (const occtl_status_t aStatus =
          OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_cylindrical_hole_info_t"))
    {
      return aStatus;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (const occtl_status_t aStatus = OcctL::Prim::CheckFiniteAxis1(theInfo->axis, "axis"))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus = OcctL::Prim::CheckPositive(theInfo->radius, "radius"))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus =
          OcctL::Prim::CheckBool(theInfo->with_control, "with_control"))
    {
      return aStatus;
    }
    if (const occtl_status_t aStatus =
          OcctL::Prim::CheckDirection(theInfo->axis.direction, "axis.direction"))
    {
      return aStatus;
    }

    TopoDS_Shape aBaseShape;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->base_shape, "base_shape", aBaseShape))
    {
      return aStatus;
    }
    OCC_CATCH_SIGNALS;

    BRepFeat_MakeCylindricalHole aMaker;
    aMaker.Init(aBaseShape, OcctL::Geom::ToGpAx1(theInfo->axis));

    const bool toControl = theInfo->with_control != 0;
    switch (theInfo->kind)
    {
      case OCCTL_CYLINDRICAL_HOLE_THROUGH_ALL: {
        aMaker.Perform(theInfo->radius);
        break;
      }
      case OCCTL_CYLINDRICAL_HOLE_BETWEEN_PARAMS: {
        if (const occtl_status_t aStatus = OcctL::Prim::CheckFinite(theInfo->p_from, "p_from"))
        {
          return aStatus;
        }
        if (const occtl_status_t aStatus = OcctL::Prim::CheckFinite(theInfo->p_to, "p_to"))
        {
          return aStatus;
        }
        if (theInfo->p_to <= theInfo->p_from)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                                 "BETWEEN_PARAMS mode requires p_to > p_from");
          return OCCTL_INVALID_ARGUMENT;
        }
        aMaker.Perform(theInfo->radius, theInfo->p_from, theInfo->p_to, toControl);
        break;
      }
      case OCCTL_CYLINDRICAL_HOLE_THRU_NEXT: {
        aMaker.PerformThruNext(theInfo->radius, toControl);
        break;
      }
      case OCCTL_CYLINDRICAL_HOLE_UNTIL_END: {
        aMaker.PerformUntilEnd(theInfo->radius, toControl);
        break;
      }
      case OCCTL_CYLINDRICAL_HOLE_BLIND: {
        if (const occtl_status_t aStatus = OcctL::Prim::CheckPositive(theInfo->length, "length"))
        {
          return aStatus;
        }
        aMaker.PerformBlind(theInfo->radius, theInfo->length, toControl);
        break;
      }
      default: {
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "unknown hole kind");
        return OCCTL_INVALID_ARGUMENT;
      }
    }

    aMaker.Build();

    const BRepFeat_Status aStatus = aMaker.Status();
    if (aStatus != BRepFeat_NoError || aMaker.HasErrors() || aMaker.Shape().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        static_cast<std::string_view>(
          TCollection_AsciiString("BRepFeat_MakeCylindricalHole failed with status ")
          + statusName(aStatus)));
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph,
                                        unwrapSingleChildCompound(aMaker.Shape()),
                                        *theOutShape);
  });
}

} // extern "C"
