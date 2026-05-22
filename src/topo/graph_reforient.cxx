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

#include "IdConvert.hxx"
#include "TopoMath.hxx"

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <BRepGraph_EditorView.hxx>

#include <TopLoc_Location.hxx>
#include <gp_Trsf.hxx>
#include <Precision.hxx>

#include <cmath>

namespace
{

bool IsFiniteValue(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool IsValidOrientation(const occtl_orientation_t theOrientation)
{
  return theOrientation == OCCTL_ORIENTATION_FORWARD || theOrientation == OCCTL_ORIENTATION_REVERSED
         || theOrientation == OCCTL_ORIENTATION_INTERNAL
         || theOrientation == OCCTL_ORIENTATION_EXTERNAL;
}

bool TransformIsFinite(const occtl_transform_t& theTransform)
{
  for (int anIdx = 0; anIdx < 12; ++anIdx)
  {
    if (!IsFiniteValue(theTransform.m[anIdx]))
    {
      return false;
    }
  }
  return true;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_set_ref_orientation(occtl_graph_t* const      theGraph,
                                 const occtl_ref_id_t      theRefId,
                                 const occtl_orientation_t theOrientation)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RefId aRefId = OcctL::Topo::UnpackRefId(theRefId);
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theRefId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }
    if (!IsValidOrientation(theOrientation))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theOrientation is out of range");
      return OCCTL_INVALID_ARGUMENT;
    }

    const TopAbs_Orientation anOri = OcctL::Topo::ToOcctOrientation(theOrientation);

    switch (aRefId.RefKind)
    {
      case BRepGraph_RefId::Kind::Shell:
        theGraph->graph.Editor().Shells().SetRefOrientation(BRepGraph_ShellRefId(aRefId), anOri);
        break;
      case BRepGraph_RefId::Kind::Face:
        theGraph->graph.Editor().Faces().SetRefOrientation(BRepGraph_FaceRefId(aRefId), anOri);
        break;
      case BRepGraph_RefId::Kind::Wire:
        theGraph->graph.Editor().Wires().SetRefOrientation(BRepGraph_WireRefId(aRefId), anOri);
        break;
      case BRepGraph_RefId::Kind::CoEdge:
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_WRONG_KIND,
          "CoEdge refs do not carry orientation; use CoEdgeDef");
        return OCCTL_WRONG_KIND;
      case BRepGraph_RefId::Kind::Vertex:
        theGraph->graph.Editor().Vertices().SetRefOrientation(BRepGraph_VertexRefId(aRefId), anOri);
        break;
      case BRepGraph_RefId::Kind::Solid:
        theGraph->graph.Editor().Solids().SetRefOrientation(BRepGraph_SolidRefId(aRefId), anOri);
        break;
      case BRepGraph_RefId::Kind::Child:
        theGraph->graph.Editor().Gen().SetChildRefOrientation(BRepGraph_ChildRefId(aRefId), anOri);
        break;
      default:
        OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                               "unsupported ref kind for orientation");
        return OCCTL_WRONG_KIND;
    }

    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_set_ref_location(occtl_graph_t* const    theGraph,
                              const occtl_ref_id_t    theRefId,
                              const occtl_transform_t theTransform)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RefId aRefId = OcctL::Topo::UnpackRefId(theRefId);
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theRefId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }
    if (!TransformIsFinite(theTransform))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theTransform contains non-finite values");
      return OCCTL_INVALID_ARGUMENT;
    }

    gp_Trsf aTrsf;
    aTrsf.SetValues(theTransform.m[0],
                    theTransform.m[1],
                    theTransform.m[2],
                    theTransform.m[3],
                    theTransform.m[4],
                    theTransform.m[5],
                    theTransform.m[6],
                    theTransform.m[7],
                    theTransform.m[8],
                    theTransform.m[9],
                    theTransform.m[10],
                    theTransform.m[11]);
    const TopLoc_Location aLoc(aTrsf);

    switch (aRefId.RefKind)
    {
      case BRepGraph_RefId::Kind::Shell:
        theGraph->graph.Editor().Shells().SetRefLocalLocation(BRepGraph_ShellRefId(aRefId), aLoc);
        break;
      case BRepGraph_RefId::Kind::Face:
        theGraph->graph.Editor().Faces().SetRefLocalLocation(BRepGraph_FaceRefId(aRefId), aLoc);
        break;
      case BRepGraph_RefId::Kind::Wire:
        theGraph->graph.Editor().Wires().SetRefLocalLocation(BRepGraph_WireRefId(aRefId), aLoc);
        break;
      case BRepGraph_RefId::Kind::CoEdge:
        theGraph->graph.Editor().CoEdges().SetRefLocalLocation(BRepGraph_CoEdgeRefId(aRefId), aLoc);
        break;
      case BRepGraph_RefId::Kind::Vertex:
        theGraph->graph.Editor().Vertices().SetRefLocalLocation(BRepGraph_VertexRefId(aRefId),
                                                                aLoc);
        break;
      case BRepGraph_RefId::Kind::Solid:
        theGraph->graph.Editor().Solids().SetRefLocalLocation(BRepGraph_SolidRefId(aRefId), aLoc);
        break;
      case BRepGraph_RefId::Kind::Child:
        theGraph->graph.Editor().Gen().SetChildRefLocalLocation(BRepGraph_ChildRefId(aRefId), aLoc);
        break;
      case BRepGraph_RefId::Kind::Occurrence:
        theGraph->graph.Editor().Occurrences().SetRefLocalLocation(
          BRepGraph_OccurrenceRefId(aRefId),
          aLoc);
        break;
      default:
        OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                               "unsupported ref kind for location");
        return OCCTL_WRONG_KIND;
    }

    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_wire_ref_is_outer(occtl_graph_t* const theGraph,
                                                                     const occtl_ref_id_t theRefId,
                                                                     const int32_t        theFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RefId aRefId = OcctL::Topo::UnpackRefId(theRefId);
    if (!aRefId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theRefId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    if (aRefId.RefKind != BRepGraph_RefId::Kind::Wire)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "theRefId is not a wire ref");
      return OCCTL_WRONG_KIND;
    }

    const BRepGraph_WireRefId aWireRefId(aRefId);
    theGraph->graph.Editor().Wires().SetRefIsOuter(aWireRefId, theFlag != 0);
    return OCCTL_OK;
  });
}

} // extern "C"
