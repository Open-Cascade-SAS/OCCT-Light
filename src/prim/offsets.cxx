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

//! @file offsets.cxx
//! @brief Offset operations: occtl_prim_make_offset_shape (generic offset on
//!        Wire/Face/Shell/Solid via
//!        BRepOffsetAPI_MakeOffsetShape::PerformByJoin) and
//!        occtl_prim_make_thick_solid (the canonical "shell" CAD op via
//!        BRepOffsetAPI_MakeThickSolid::MakeThickSolidByJoin).

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/IdConvert.hxx"

#include <occtl/occtl_prim.h>

#include <BRepOffsetAPI_MakeOffsetShape.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepOffset_Mode.hxx>
#include <GeomAbs_JoinType.hxx>
#include <NCollection_List.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

namespace
{
inline BRepOffset_Mode toOcctMode(const occtl_prim_offset_mode_t theMode)
{
  switch (theMode)
  {
    case OCCTL_OFFSET_MODE_PIPE:
      return BRepOffset_Pipe;
    case OCCTL_OFFSET_MODE_RECTO_VERSO:
      return BRepOffset_RectoVerso;
    default:
      return BRepOffset_Skin;
  }
}

inline GeomAbs_JoinType toOcctJoin(const occtl_offset_join_type_t theJoin)
{
  switch (theJoin)
  {
    case OCCTL_OFFSET_JOIN_TANGENT:
      return GeomAbs_Tangent;
    case OCCTL_OFFSET_JOIN_INTERSECTION:
      return GeomAbs_Intersection;
    default:
      return GeomAbs_Arc;
  }
}

inline occtl_status_t checkOffsetMode(const occtl_prim_offset_mode_t theMode)
{
  switch (theMode)
  {
    case OCCTL_OFFSET_MODE_SKIN:
    case OCCTL_OFFSET_MODE_PIPE:
    case OCCTL_OFFSET_MODE_RECTO_VERSO:
      return OCCTL_OK;
    default:
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "mode is not a valid offset mode");
      return OCCTL_INVALID_ARGUMENT;
  }
}

inline occtl_status_t checkJoinType(const occtl_offset_join_type_t theJoin)
{
  switch (theJoin)
  {
    case OCCTL_OFFSET_JOIN_ARC:
    case OCCTL_OFFSET_JOIN_TANGENT:
    case OCCTL_OFFSET_JOIN_INTERSECTION:
      return OCCTL_OK;
    default:
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "join is not a valid offset join type");
      return OCCTL_INVALID_ARGUMENT;
  }
}
} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_offset_shape_info_init(occtl_prim_offset_shape_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_thick_solid_info_init(occtl_prim_thick_solid_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_offset_shape(occtl_graph_t* const                        theGraph,
                               const occtl_prim_offset_shape_info_t* const theInfo,
                               occtl_node_id_t* const                      theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_OFFSET_SHAPE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_OFFSET_SHAPE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_offset_shape_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->offset, "offset") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->tolerance, "tolerance") != OCCTL_OK
        || checkOffsetMode(theInfo->mode) != OCCTL_OK || checkJoinType(theInfo->join) != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->intersection, "intersection") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->self_intersection, "self_intersection") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->remove_internal_edges, "remove_internal_edges")
             != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->shape, "shape", aShape))
    {
      return aStatus;
    }

    BRepOffsetAPI_MakeOffsetShape aMaker;
    aMaker.PerformByJoin(aShape,
                         theInfo->offset,
                         theInfo->tolerance,
                         toOcctMode(theInfo->mode),
                         theInfo->intersection != 0,
                         theInfo->self_intersection != 0,
                         toOcctJoin(theInfo->join),
                         theInfo->remove_internal_edges != 0);
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "BRepOffsetAPI_MakeOffsetShape reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutShape);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_thick_solid(occtl_graph_t* const                       theGraph,
                              const occtl_prim_thick_solid_info_t* const theInfo,
                              occtl_node_id_t* const                     theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_THICK_SOLID_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_THICK_SOLID_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_thick_solid_info_t") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->offset, "offset") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->tolerance, "tolerance") != OCCTL_OK
        || checkOffsetMode(theInfo->mode) != OCCTL_OK || checkJoinType(theInfo->join) != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->intersection, "intersection") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->self_intersection, "self_intersection") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->remove_internal_edges, "remove_internal_edges")
             != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->closing_face_count > 0 && theInfo->closing_faces == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "closing_faces is NULL while closing_face_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutSolid = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId aSolidId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theInfo->solid, BRepGraph_NodeId::Kind::Solid, aSolidId))
    {
      return aStatus;
    }

    const TopoDS_Shape aSolidShape = theGraph->graph.Shapes().Shape(aSolidId);
    if (aSolidShape.IsNull() || aSolidShape.ShapeType() != TopAbs_SOLID)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "solid could not be reconstructed as TopoDS_Solid");
      return OCCTL_NOT_FOUND;
    }

    NCollection_List<TopoDS_Shape> aClosing;
    for (size_t anI = 0; anI < theInfo->closing_face_count; ++anI)
    {
      BRepGraph_NodeId aFaceId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theInfo->closing_faces[anI],
                                                                BRepGraph_NodeId::Kind::Face,
                                                                aFaceId))
      {
        return aStatus;
      }

      const TopoDS_Shape aFaceShape = theGraph->graph.Shapes().Shape(aFaceId);
      if (aFaceShape.IsNull() || aFaceShape.ShapeType() != TopAbs_FACE)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_NOT_FOUND,
          "closing face could not be reconstructed as TopoDS_Face");
        return OCCTL_NOT_FOUND;
      }
      aClosing.Append(aFaceShape);
    }

    BRepOffsetAPI_MakeThickSolid aMaker;
    aMaker.MakeThickSolidByJoin(aSolidShape,
                                aClosing,
                                theInfo->offset,
                                theInfo->tolerance,
                                toOcctMode(theInfo->mode),
                                theInfo->intersection != 0,
                                theInfo->self_intersection != 0,
                                toOcctJoin(theInfo->join),
                                theInfo->remove_internal_edges != 0);
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "BRepOffsetAPI_MakeThickSolid reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutSolid);
  });
}

} // extern "C"
