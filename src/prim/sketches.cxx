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

//! @file sketches.cxx
//! @brief 2D sketch primitives: polyline / regular_polygon / rectangle /
//!        circle / ellipse wire builders and planar_face from wire(s).
//!        Each entry point constructs the topology via BRepBuilderAPI_Make*
//!        helpers and round-trips the result back into the graph via
//!        AddTopologyRoot.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/CurveMath.hxx"
#include "../topo/IdConvert.hxx"

#include <occtl/occtl_prim.h>

#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_PlanarConvexHull.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_JoinType.hxx>
#include <NCollection_Array1.hxx>
#include <TopAbs.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <Precision.hxx>

#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <cmath>

namespace
{

bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool isFinitePoint(const gp_Pnt& thePoint)
{
  return IsFiniteValue(thePoint.X()) && IsFiniteValue(thePoint.Y()) && IsFiniteValue(thePoint.Z());
}

GeomAbs_JoinType toOcctTraceJoin(const occtl_topo_wire_offset_2d_join_t theJoin)
{
  switch (theJoin)
  {
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_TANGENT:
      return GeomAbs_Tangent;
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION:
      return GeomAbs_Intersection;
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC:
      return GeomAbs_Arc;
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_RESERVED_FUTURE:
      break;
  }
  return GeomAbs_Arc;
}

bool isValidTraceJoin(const occtl_topo_wire_offset_2d_join_t theJoin)
{
  return theJoin == OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC
         || theJoin == OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_TANGENT
         || theJoin == OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION;
}

occtl_status_t makePlanarFaceFromTraceResult(const TopoDS_Shape& theResult,
                                             TopoDS_Shape&       theOutFace)
{
  TopoDS_Shape aCandidate = theResult;
  if (!aCandidate.IsNull() && aCandidate.ShapeType() == TopAbs_FACE)
  {
    theOutFace = aCandidate;
    return OCCTL_OK;
  }

  if (aCandidate.IsNull() || aCandidate.ShapeType() != TopAbs_WIRE)
  {
    for (TopExp_Explorer anExp(theResult, TopAbs_FACE); anExp.More(); anExp.Next())
    {
      theOutFace = anExp.Current();
      return OCCTL_OK;
    }

    for (TopExp_Explorer anExp(theResult, TopAbs_WIRE); anExp.More(); anExp.Next())
    {
      aCandidate = anExp.Current();
      break;
    }
  }

  if (aCandidate.IsNull() || aCandidate.ShapeType() != TopAbs_WIRE)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "trace offset did not produce a faceable wire");
    return OCCTL_GEOMETRY_INVALID;
  }

  BRepBuilderAPI_MakeFace aFaceMaker(TopoDS::Wire(aCandidate), /* OnlyPlane */ true);
  aFaceMaker.Build();
  if (!aFaceMaker.IsDone() || aFaceMaker.Face().IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "BRepBuilderAPI_MakeFace failed for trace contour");
    return OCCTL_GEOMETRY_INVALID;
  }

  theOutFace = aFaceMaker.Face();
  return OCCTL_OK;
}

occtl_status_t makeLinearEdgeTraceFace(const TopoDS_Edge&        theEdge,
                                       const occtl_direction3_t& theNormal,
                                       const double              theWidth,
                                       TopoDS_Shape&             theOutFace)
{
  const gp_Vec aNormal(theNormal.x, theNormal.y, theNormal.z);
  if (aNormal.SquareMagnitude() <= Precision::SquareConfusion())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "trace normal has zero length");
    return OCCTL_INVALID_ARGUMENT;
  }

  BRepAdaptor_Curve aCurve(theEdge);
  if (aCurve.GetType() != GeomAbs_Line)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "single Edge trace currently requires a linear edge");
    return OCCTL_GEOMETRY_INVALID;
  }

  const gp_Pnt aFirst = aCurve.Value(aCurve.FirstParameter());
  const gp_Pnt aLast  = aCurve.Value(aCurve.LastParameter());
  gp_Vec       aTangent(aFirst, aLast);
  if (aTangent.SquareMagnitude() <= Precision::SquareConfusion())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "trace edge has zero length");
    return OCCTL_GEOMETRY_INVALID;
  }

  gp_Vec aWidthVec = aNormal.Crossed(aTangent);
  if (aWidthVec.SquareMagnitude() <= Precision::SquareConfusion())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "trace normal is parallel to the edge");
    return OCCTL_INVALID_ARGUMENT;
  }
  aWidthVec.Normalize();
  aWidthVec *= theWidth;

  gp_Trsf aShift;
  aShift.SetTranslation(-0.5 * aWidthVec);
  BRepBuilderAPI_Transform aTransform(theEdge, aShift, true);
  aTransform.Build();
  if (!aTransform.IsDone() || aTransform.Shape().IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "BRepBuilderAPI_Transform failed for trace edge");
    return OCCTL_GEOMETRY_INVALID;
  }

  BRepPrimAPI_MakePrism aPrism(aTransform.Shape(),
                               aWidthVec,
                               /* Copy */ false,
                               /* Canonize */ true);
  aPrism.Build();
  if (!aPrism.IsDone() || aPrism.Shape().IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "BRepPrimAPI_MakePrism failed for trace edge");
    return OCCTL_GEOMETRY_INVALID;
  }

  return makePlanarFaceFromTraceResult(aPrism.Shape(), theOutFace);
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_polyline_info_init(occtl_prim_polyline_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_POLYLINE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_regular_polygon_info_init(occtl_prim_regular_polygon_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_REGULAR_POLYGON_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_rectangle_info_init(occtl_prim_rectangle_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_planar_face_info_init(occtl_prim_planar_face_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_circle_info_init(occtl_prim_circle_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_CIRCLE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_ellipse_info_init(occtl_prim_ellipse_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_ELLIPSE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_arc_3pt_info_init(occtl_prim_arc_3pt_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_ARC_3PT_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_arc_center_info_init(occtl_prim_arc_center_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_ARC_CENTER_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_spline_info_init(occtl_prim_spline_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_SPLINE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_plane_info_init(occtl_prim_plane_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_PLANE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_disk_info_init(occtl_prim_disk_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_DISK_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_convex_hull_2d_info_init(occtl_prim_convex_hull_2d_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_trace_info_init(occtl_prim_trace_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_TRACE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_polyline(occtl_graph_t* const                    theGraph,
                           const occtl_prim_polyline_info_t* const theInfo,
                           occtl_node_id_t* const                  theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_POLYLINE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_POLYLINE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->points == nullptr || theInfo->point_count < 2)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "polyline requires at least 2 points in a non-NULL array");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    BRepBuilderAPI_MakePolygon aPoly;
    for (size_t anI = 0; anI < theInfo->point_count; ++anI)
    {
      aPoly.Add(OcctL::Geom::ToGp(theInfo->points[anI]));
    }
    if (theInfo->closed != 0)
    {
      aPoly.Close();
    }
    aPoly.Build();
    if (!aPoly.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "BRepBuilderAPI_MakePolygon reported IsDone()==false (coincident points?)");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aPoly.Wire(), *theOutWire);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_regular_polygon(occtl_graph_t* const                           theGraph,
                                  const occtl_prim_regular_polygon_info_t* const theInfo,
                                  occtl_node_id_t* const                         theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_REGULAR_POLYGON_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_REGULAR_POLYGON_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->sides < 3 || theInfo->circumradius <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "regular polygon requires sides >= 3 and circumradius > 0");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    const gp_Ax2 anAxes  = OcctL::Geom::ToGpAx2(theInfo->placement);
    const gp_Pnt aCenter = anAxes.Location();
    const gp_Dir aXDir   = anAxes.XDirection();
    const gp_Dir aYDir   = anAxes.YDirection();
    const double aR      = theInfo->circumradius;
    const double aStep   = 2.0 * OCCTL_PI / static_cast<double>(theInfo->sides);

    BRepBuilderAPI_MakePolygon aPoly;
    for (int anI = 0; anI < theInfo->sides; ++anI)
    {
      const double aTheta = theInfo->rotation + static_cast<double>(anI) * aStep;
      const double aC     = aR * std::cos(aTheta);
      const double aS     = aR * std::sin(aTheta);
      gp_Pnt       aP     = aCenter;
      aP.Translate(gp_Vec(aXDir) * aC + gp_Vec(aYDir) * aS);
      aPoly.Add(aP);
    }
    aPoly.Close();
    aPoly.Build();
    if (!aPoly.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepBuilderAPI_MakePolygon reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aPoly.Wire(), *theOutWire);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_rectangle(occtl_graph_t* const                     theGraph,
                            const occtl_prim_rectangle_info_t* const theInfo,
                            occtl_node_id_t* const                   theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_RECTANGLE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_RECTANGLE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->width <= 0.0 || theInfo->height <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "rectangle width and height must be strictly positive");
      return OCCTL_GEOMETRY_INVALID;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    const gp_Ax2 anAxes  = OcctL::Geom::ToGpAx2(theInfo->placement);
    const gp_Pnt aCenter = anAxes.Location();
    const gp_Vec aX      = gp_Vec(anAxes.XDirection()) * (0.5 * theInfo->width);
    const gp_Vec aY      = gp_Vec(anAxes.YDirection()) * (0.5 * theInfo->height);

    const gp_Pnt aP0 = aCenter.Translated(-aX - aY);
    const gp_Pnt aP1 = aCenter.Translated(aX - aY);
    const gp_Pnt aP2 = aCenter.Translated(aX + aY);
    const gp_Pnt aP3 = aCenter.Translated(-aX + aY);

    BRepBuilderAPI_MakePolygon aPoly(aP0, aP1, aP2, aP3, /* Close */ true);
    aPoly.Build();
    if (!aPoly.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepBuilderAPI_MakePolygon reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aPoly.Wire(), *theOutWire);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_planar_face(occtl_graph_t* const                       theGraph,
                              const occtl_prim_planar_face_info_t* const theInfo,
                              occtl_node_id_t* const                     theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_PLANAR_FACE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_PLANAR_FACE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->inner_wire_count > 0 && theInfo->inner_wires == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "inner_wires is NULL while inner_wire_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutFace = OCCTL_NODE_ID_INVALID;

    BRepGraph_NodeId anOuterId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theInfo->outer_wire,
                                                              BRepGraph_NodeId::Kind::Wire,
                                                              anOuterId))
    {
      return aStatus;
    }

    const TopoDS_Shape anOuterShape = theGraph->graph.Shapes().Shape(anOuterId);
    if (anOuterShape.IsNull() || anOuterShape.ShapeType() != TopAbs_WIRE)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "outer_wire could not be reconstructed as TopoDS_Wire");
      return OCCTL_NOT_FOUND;
    }
    const TopoDS_Wire& anOuter = TopoDS::Wire(anOuterShape);

    BRepBuilderAPI_MakeFace aFaceMaker(anOuter, /* OnlyPlane */ true);
    for (size_t anI = 0; anI < theInfo->inner_wire_count; ++anI)
    {
      BRepGraph_NodeId anInnerId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theInfo->inner_wires[anI],
                                                                BRepGraph_NodeId::Kind::Wire,
                                                                anInnerId))
      {
        return aStatus;
      }

      const TopoDS_Shape anInnerShape = theGraph->graph.Shapes().Shape(anInnerId);
      if (anInnerShape.IsNull() || anInnerShape.ShapeType() != TopAbs_WIRE)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_NOT_FOUND,
          "inner wire could not be reconstructed as TopoDS_Wire");
        return OCCTL_NOT_FOUND;
      }
      aFaceMaker.Add(TopoDS::Wire(anInnerShape));
    }

    aFaceMaker.Build();
    if (!aFaceMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "BRepBuilderAPI_MakeFace reported IsDone()==false (non-planar wire?)");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aFaceMaker.Face(), *theOutFace);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_circle(occtl_graph_t* const                  theGraph,
                         const occtl_prim_circle_info_t* const theInfo,
                         occtl_node_id_t* const                theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_CIRCLE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_CIRCLE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    const occtl_geom_circle_t aCircle = {theInfo->placement, theInfo->radius};
    occtl_rep_id_t            aCurve  = OCCTL_REP_ID_INVALID;
    if (const occtl_status_t aStatus = occtl_curve_create_circle(theGraph, aCircle, &aCurve))
    {
      return aStatus;
    }
    return occtl_topo_curves_to_wire(theGraph, &aCurve, 1, theOutWire);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_ellipse(occtl_graph_t* const                   theGraph,
                          const occtl_prim_ellipse_info_t* const theInfo,
                          occtl_node_id_t* const                 theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_ELLIPSE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_ELLIPSE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    const occtl_geom_ellipse_t anEllipse = {theInfo->placement, theInfo->major, theInfo->minor};
    occtl_rep_id_t             aCurve    = OCCTL_REP_ID_INVALID;
    if (const occtl_status_t aStatus = occtl_curve_create_ellipse(theGraph, anEllipse, &aCurve))
    {
      return aStatus;
    }
    return occtl_topo_curves_to_wire(theGraph, &aCurve, 1, theOutWire);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_arc_3pt(occtl_graph_t* const                   theGraph,
                          const occtl_prim_arc_3pt_info_t* const theInfo,
                          occtl_node_id_t* const                 theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_ARC_3PT_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_ARC_3PT_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    occtl_rep_id_t aCurve = OCCTL_REP_ID_INVALID;
    if (const occtl_status_t aStatus = occtl_curve_create_arc_of_circle_3pt(theGraph,
                                                                            theInfo->start,
                                                                            theInfo->via,
                                                                            theInfo->end,
                                                                            &aCurve))
    {
      return aStatus;
    }
    return occtl_topo_curves_to_wire(theGraph, &aCurve, 1, theOutWire);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_arc_center(occtl_graph_t* const                      theGraph,
                             const occtl_prim_arc_center_info_t* const theInfo,
                             occtl_node_id_t* const                    theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_ARC_CENTER_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_ARC_CENTER_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    const occtl_geom_circle_t aCircle    = {theInfo->placement, theInfo->radius};
    occtl_rep_id_t            aFullCurve = OCCTL_REP_ID_INVALID;
    if (const occtl_status_t aStatus = occtl_curve_create_circle(theGraph, aCircle, &aFullCurve))
    {
      return aStatus;
    }

    occtl_curve_trimmed_create_info_t aTrim = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
    aTrim.basis                             = aFullCurve;
    aTrim.u_first                           = theInfo->start_angle;
    aTrim.u_last                            = theInfo->end_angle;
    aTrim.sense                             = 1;

    occtl_rep_id_t aTrimmedCurve = OCCTL_REP_ID_INVALID;
    if (const occtl_status_t aStatus = occtl_curve_create_trimmed(theGraph, &aTrim, &aTrimmedCurve))
    {
      return aStatus;
    }
    return occtl_topo_curves_to_wire(theGraph, &aTrimmedCurve, 1, theOutWire);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_spline(occtl_graph_t* const                  theGraph,
                         const occtl_prim_spline_info_t* const theInfo,
                         occtl_node_id_t* const                theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_SPLINE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_SPLINE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutWire = OCCTL_NODE_ID_INVALID;

    occtl_curve_approximated_info_t anInfo = OCCTL_CURVE_APPROXIMATED_INFO_INIT;
    anInfo.points                          = theInfo->points;
    anInfo.point_count                     = theInfo->point_count;
    anInfo.degree_min                      = theInfo->degree_min;
    anInfo.degree_max                      = theInfo->degree_max;
    anInfo.tolerance                       = theInfo->tolerance;

    occtl_rep_id_t aCurve = OCCTL_REP_ID_INVALID;
    if (const occtl_status_t aStatus = occtl_curve_create_approximated(theGraph, &anInfo, &aCurve))
    {
      return aStatus;
    }
    return occtl_topo_curves_to_wire(theGraph, &aCurve, 1, theOutWire);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_plane(occtl_graph_t* const                 theGraph,
                        const occtl_prim_plane_info_t* const theInfo,
                        occtl_node_id_t* const               theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_PLANE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_PLANE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutFace = OCCTL_NODE_ID_INVALID;

    occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
    aRect.placement                   = theInfo->placement;
    aRect.width                       = theInfo->width;
    aRect.height                      = theInfo->height;

    occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
    if (const occtl_status_t aStatus = occtl_prim_make_rectangle(theGraph, &aRect, &aWire))
    {
      return aStatus;
    }

    occtl_prim_planar_face_info_t aFace = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
    aFace.outer_wire                    = aWire;
    return occtl_prim_make_planar_face(theGraph, &aFace, theOutFace);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_disk(occtl_graph_t* const                theGraph,
                       const occtl_prim_disk_info_t* const theInfo,
                       occtl_node_id_t* const              theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_DISK_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_DISK_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutFace = OCCTL_NODE_ID_INVALID;

    occtl_prim_circle_info_t aCircle = OCCTL_PRIM_CIRCLE_INFO_INIT;
    aCircle.placement                = theInfo->placement;
    aCircle.radius                   = theInfo->radius;

    occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
    if (const occtl_status_t aStatus = occtl_prim_make_circle(theGraph, &aCircle, &aWire))
    {
      return aStatus;
    }

    occtl_prim_planar_face_info_t aFace = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
    aFace.outer_wire                    = aWire;
    return occtl_prim_make_planar_face(theGraph, &aFace, theOutFace);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_convex_hull_2d(occtl_graph_t* const                          theGraph,
                                 const occtl_prim_convex_hull_2d_info_t* const theInfo,
                                 occtl_node_id_t* const                        theOutNode)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutNode == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_node is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_CONVEX_HULL_2D_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_CONVEX_HULL_2D_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || !IsFiniteValue(theInfo->tolerance)
        || theInfo->tolerance <= 0.0 || (theInfo->point_count > 0 && theInfo->points == nullptr)
        || (theInfo->vertex_count > 0 && theInfo->vertices == nullptr)
        || (theInfo->point_count == 0 && theInfo->vertex_count == 0)
        || (theInfo->make_face != 0 && theInfo->make_face != 1))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "convex hull options are invalid");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutNode = OCCTL_NODE_ID_INVALID;

    const gp_Ax2               anAxes      = OcctL::Geom::ToGpAx2(theInfo->placement);
    const size_t               aTotalCount = theInfo->point_count + theInfo->vertex_count;
    NCollection_Array1<gp_Pnt> aPoints(aTotalCount);
    size_t                     aPointIndex = 0;
    for (size_t anIdx = 0; anIdx < theInfo->point_count; ++anIdx)
    {
      const gp_Pnt aPoint = OcctL::Geom::ToGp(theInfo->points[anIdx]);
      if (!isFinitePoint(aPoint))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                               "convex hull point is not finite");
        return OCCTL_INVALID_ARGUMENT;
      }
      aPoints.ChangeAt(aPointIndex++) = aPoint;
    }

    for (size_t anIdx = 0; anIdx < theInfo->vertex_count; ++anIdx)
    {
      BRepGraph_NodeId aVertexId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theInfo->vertices[anIdx],
                                                                BRepGraph_NodeId::Kind::Vertex,
                                                                aVertexId))
      {
        return aStatus;
      }

      const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(aVertexId);
      if (aShape.IsNull() || aShape.ShapeType() != TopAbs_VERTEX)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_NOT_FOUND,
          "vertex could not be reconstructed as TopoDS_Vertex");
        return OCCTL_NOT_FOUND;
      }
      const gp_Pnt aPoint = BRep_Tool::Pnt(TopoDS::Vertex(aShape));
      if (!isFinitePoint(aPoint))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "vertex point is not finite");
        return OCCTL_GEOMETRY_INVALID;
      }
      aPoints.ChangeAt(aPointIndex++) = aPoint;
    }

    // Delegate 2D convex hull computation to OCCT.
    GeomAPI_PlanarConvexHull aHullAlgo(aPoints, anAxes, theInfo->tolerance);
    if (!aHullAlgo.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "convex hull requires at least three non-collinear points");
      return OCCTL_GEOMETRY_INVALID;
    }

    const NCollection_Array1<size_t>& aIndices = aHullAlgo.Result();
    BRepBuilderAPI_MakePolygon        aPolygon;
    for (size_t anI = 0; anI < aIndices.Size(); ++anI)
    {
      aPolygon.Add(aPoints.At(aIndices.At(anI)));
    }
    aPolygon.Close();
    aPolygon.Build();
    if (!aPolygon.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepBuilderAPI_MakePolygon failed for convex hull");
      return OCCTL_GEOMETRY_INVALID;
    }

    if (theInfo->make_face == 0)
    {
      return OcctL::Prim::AddTopologyRoot(theGraph, aPolygon.Wire(), *theOutNode);
    }

    BRepBuilderAPI_MakeFace aFace(aPolygon.Wire(), /* OnlyPlane */ true);
    aFace.Build();
    if (!aFace.IsDone() || aFace.Face().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepBuilderAPI_MakeFace failed for convex hull");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aFace.Face(), *theOutNode);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_trace(occtl_graph_t* const                 theGraph,
                        const occtl_prim_trace_info_t* const theInfo,
                        occtl_node_id_t* const               theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutFace = OCCTL_NODE_ID_INVALID;

    if (theInfo->struct_version != OCCTL_PRIM_TRACE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_TRACE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theInfo->width) || theInfo->width <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "trace width must be finite and strictly positive");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = OcctL::Prim::CheckDirection(theInfo->normal, "trace normal"))
    {
      return aStatus;
    }
    if (!isValidTraceJoin(theInfo->join))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "unsupported trace join type");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->approximate != 0 && theInfo->approximate != 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "approximate must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aPathId = OcctL::Topo::UnpackNodeId(theInfo->path);
    if (!aPathId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "path NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }
    if (aPathId.NodeKind != BRepGraph_NodeId::Kind::Edge
        && aPathId.NodeKind != BRepGraph_NodeId::Kind::Wire)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "path must be an Edge or Wire");
      return OCCTL_WRONG_KIND;
    }

    const TopoDS_Shape aPathShape = theGraph->graph.Shapes().Shape(aPathId);
    if (aPathShape.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "path could not be reconstructed as TopoDS shape");
      return OCCTL_NOT_FOUND;
    }

    TopoDS_Wire aWire;
    if (aPathId.NodeKind == BRepGraph_NodeId::Kind::Edge)
    {
      if (aPathShape.ShapeType() != TopAbs_EDGE)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "path could not be reconstructed as TopoDS_Edge");
        return OCCTL_NOT_FOUND;
      }

      TopoDS_Shape aFace;
      if (const occtl_status_t aStatus = makeLinearEdgeTraceFace(TopoDS::Edge(aPathShape),
                                                                 theInfo->normal,
                                                                 theInfo->width,
                                                                 aFace))
      {
        return aStatus;
      }

      return OcctL::Prim::AddTopologyRoot(theGraph, aFace, *theOutFace);
    }
    else
    {
      if (aPathShape.ShapeType() != TopAbs_WIRE)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "path could not be reconstructed as TopoDS_Wire");
        return OCCTL_NOT_FOUND;
      }
      aWire = TopoDS::Wire(aPathShape);
    }

    BRepOffsetAPI_MakeOffset aMaker(aWire,
                                    toOcctTraceJoin(theInfo->join),
                                    /* IsOpenResult */ false);
    aMaker.SetApprox(theInfo->approximate != 0);
    aMaker.Perform(0.5 * theInfo->width, 0.0);
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepOffsetAPI_MakeOffset failed for trace path");
      return OCCTL_GEOMETRY_INVALID;
    }

    TopoDS_Shape aFace;
    if (const occtl_status_t aStatus = makePlanarFaceFromTraceResult(aMaker.Shape(), aFace))
    {
      return aStatus;
    }

    return OcctL::Prim::AddTopologyRoot(theGraph, aFace, *theOutFace);
  });
}

} // extern "C"
