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

//! @file fillet_2d.cxx
//! @brief 2D fillet wire: rounds the corners of a planar Face to a uniform
//!        radius.  Wraps BRepFilletAPI_MakeFillet2d, adding TKFillet to
//!        occtl-prim's link surface.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/IdConvert.hxx"

#include <occtl/occtl_prim.h>

#include <BRepFilletAPI_MakeFillet2d.hxx>
#include <BRepGProp.hxx>
#include <NCollection_IndexedMap.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <GProp_GProps.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <gp.hxx>

#include <algorithm>
#include <cmath>

namespace
{

bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool buildFullRoundFace(const TopoDS_Face&   theFace,
                        const TopoDS_Vertex& theFirstVertex,
                        const TopoDS_Vertex& theLastVertex,
                        const double         theRadius,
                        TopoDS_Shape&        theOutShape)
{
  if (!IsFiniteValue(theRadius) || theRadius <= 0.0)
  {
    return false;
  }

  BRepFilletAPI_MakeFillet2d aMaker(theFace);
  aMaker.AddFillet(theFirstVertex, theRadius);
  aMaker.AddFillet(theLastVertex, theRadius);
  aMaker.Build();
  if (!aMaker.IsDone() || aMaker.Shape().IsNull())
  {
    return false;
  }

  theOutShape = aMaker.Shape();
  return true;
}

double edgeLength(const TopoDS_Edge& theEdge)
{
  GProp_GProps aProps;
  BRepGProp::LinearProperties(theEdge, aProps);
  return aProps.Mass();
}

bool autoFullRoundRadius(const TopoDS_Face&   theFace,
                         const TopoDS_Edge&   theEdge,
                         const TopoDS_Vertex& theFirstVertex,
                         const TopoDS_Vertex& theLastVertex,
                         const uint32_t       theSearchSteps,
                         double&              theOutRadius,
                         TopoDS_Shape&        theOutShape)
{
  const double anEdgeLength = edgeLength(theEdge);
  if (!IsFiniteValue(anEdgeLength) || anEdgeLength <= gp::Resolution())
  {
    return false;
  }

  double aLow    = 0.0;
  double aHigh   = 0.5 * anEdgeLength;
  bool   isFound = false;

  const uint32_t aSteps = theSearchSteps == 0u ? 32u : theSearchSteps;
  for (uint32_t anIdx = 0; anIdx < aSteps; ++anIdx)
  {
    const double aRadius = 0.5 * (aLow + aHigh);
    TopoDS_Shape aCandidate;
    if (buildFullRoundFace(theFace, theFirstVertex, theLastVertex, aRadius, aCandidate))
    {
      aLow         = aRadius;
      theOutRadius = aRadius;
      theOutShape  = aCandidate;
      isFound      = true;
    }
    else
    {
      aHigh = aRadius;
    }
  }
  return isFound;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_fillet_2d_info_init(occtl_prim_fillet_2d_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_full_round_2d_info_init(occtl_prim_full_round_2d_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_FULL_ROUND_2D_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_fillet_2d(occtl_graph_t* const                     theGraph,
                            const occtl_prim_fillet_2d_info_t* const theInfo,
                            occtl_node_id_t* const                   theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_FILLET_2D_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_FILLET_2D_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutFace = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_fillet_2d_info_t") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->radius, "radius") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->vertex_count > 0 && theInfo->vertices == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "vertices is NULL while vertex_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theInfo->face, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    const TopoDS_Shape aFaceShape = theGraph->graph.Shapes().Shape(aFaceId);
    if (aFaceShape.IsNull() || aFaceShape.ShapeType() != TopAbs_FACE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "face could not be reconstructed as TopoDS_Face");
      return OCCTL_NOT_FOUND;
    }
    const TopoDS_Face& aFace = TopoDS::Face(aFaceShape);

    BRepFilletAPI_MakeFillet2d aMaker(aFace);

    if (theInfo->vertex_count == 0)
    {
      // Deduplicate by TShape identity so seam vertices aren't filleted twice.
      NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> aVertexMap;
      TopExp::MapShapes(aFace, TopAbs_VERTEX, aVertexMap);
      if (aVertexMap.Extent() == 0)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "face has no vertices to fillet");
        return OCCTL_GEOMETRY_INVALID;
      }
      for (int anI = 1; anI <= aVertexMap.Extent(); ++anI)
      {
        const TopoDS_Vertex& aV = TopoDS::Vertex(aVertexMap.FindKey(anI));
        aMaker.AddFillet(aV, theInfo->radius);
      }
    }
    else
    {
      for (size_t anI = 0; anI < theInfo->vertex_count; ++anI)
      {
        BRepGraph_NodeId aVId;
        if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                  theInfo->vertices[anI],
                                                                  BRepGraph_NodeId::Kind::Vertex,
                                                                  aVId))
        {
          return aStatus;
        }

        const TopoDS_Shape aVShape = theGraph->graph.Shapes().Shape(aVId);
        if (aVShape.IsNull() || aVShape.ShapeType() != TopAbs_VERTEX)
        {
          OcctL::Core::ErrorState::Current().Set(
            OCCTL_NOT_FOUND,
            "vertex could not be reconstructed as TopoDS_Vertex");
          return OCCTL_NOT_FOUND;
        }
        aMaker.AddFillet(TopoDS::Vertex(aVShape), theInfo->radius);
      }
    }

    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepFilletAPI_MakeFillet2d reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutFace);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_full_round_2d(occtl_graph_t* const                         theGraph,
                                const occtl_prim_full_round_2d_info_t* const theInfo,
                                occtl_node_id_t* const                       theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutFace = OCCTL_NODE_ID_INVALID;

    if (theInfo->struct_version != OCCTL_PRIM_FULL_ROUND_2D_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_FULL_ROUND_2D_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theInfo->radius) || theInfo->radius < 0.0 || theInfo->search_steps > 128u)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "full-round radius must be finite/non-negative and search_steps <= 128");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theInfo->face, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    BRepGraph_NodeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theInfo->edge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    const TopoDS_Shape aFaceShape = theGraph->graph.Shapes().Shape(aFaceId);
    if (aFaceShape.IsNull() || aFaceShape.ShapeType() != TopAbs_FACE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "face could not be reconstructed as TopoDS_Face");
      return OCCTL_NOT_FOUND;
    }

    const TopoDS_Shape anEdgeShape = theGraph->graph.Shapes().Shape(anEdgeId);
    if (anEdgeShape.IsNull() || anEdgeShape.ShapeType() != TopAbs_EDGE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "edge could not be reconstructed as TopoDS_Edge");
      return OCCTL_NOT_FOUND;
    }

    const TopoDS_Face& aFace  = TopoDS::Face(aFaceShape);
    const TopoDS_Edge& anEdge = TopoDS::Edge(anEdgeShape);

    TopoDS_Vertex aFirstVertex;
    TopoDS_Vertex aLastVertex;
    TopExp::Vertices(anEdge, aFirstVertex, aLastVertex);
    if (aFirstVertex.IsNull() || aLastVertex.IsNull() || aFirstVertex.IsSame(aLastVertex))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "full-round edge must have two distinct vertices");
      return OCCTL_GEOMETRY_INVALID;
    }

    TopoDS_Shape aResult;
    if (theInfo->radius > 0.0)
    {
      if (!buildFullRoundFace(aFace, aFirstVertex, aLastVertex, theInfo->radius, aResult))
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_GEOMETRY_INVALID,
          "BRepFilletAPI_MakeFillet2d failed for requested full-round radius");
        return OCCTL_GEOMETRY_INVALID;
      }
    }
    else
    {
      double aRadius = 0.0;
      if (!autoFullRoundRadius(aFace,
                               anEdge,
                               aFirstVertex,
                               aLastVertex,
                               theInfo->search_steps,
                               aRadius,
                               aResult))
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_GEOMETRY_INVALID,
          "OCCT could not find a valid automatic full-round radius");
        return OCCTL_GEOMETRY_INVALID;
      }
    }

    return OcctL::Prim::AddTopologyRoot(theGraph, aResult, *theOutFace);
  });
}

} // extern "C"
