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

#ifndef OCCTL_TOPO_GRAPH_GEOMETRY_KIND_HXX
#define OCCTL_TOPO_GRAPH_GEOMETRY_KIND_HXX

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepGraph.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_Tool.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

#include <occtl/occtl_curves.h>
#include <occtl/occtl_surfaces.h>

namespace OcctL::Topo
{

inline occtl_curve_kind_t ToAbiCurveKind(const GeomAbs_CurveType theType)
{
  switch (theType)
  {
    case GeomAbs_Line:
      return OCCTL_CURVE_KIND_LINE;
    case GeomAbs_Circle:
      return OCCTL_CURVE_KIND_CIRCLE;
    case GeomAbs_Ellipse:
      return OCCTL_CURVE_KIND_ELLIPSE;
    case GeomAbs_Hyperbola:
      return OCCTL_CURVE_KIND_HYPERBOLA;
    case GeomAbs_Parabola:
      return OCCTL_CURVE_KIND_PARABOLA;
    case GeomAbs_BSplineCurve:
      return OCCTL_CURVE_KIND_BSPLINE;
    case GeomAbs_BezierCurve:
      return OCCTL_CURVE_KIND_BEZIER;
    case GeomAbs_OffsetCurve:
      return OCCTL_CURVE_KIND_OFFSET;
    case GeomAbs_OtherCurve:
      return OCCTL_CURVE_KIND_UNDEFINED;
  }
  return OCCTL_CURVE_KIND_UNDEFINED;
}

inline occtl_surface_kind_t ToAbiSurfaceKind(const GeomAbs_SurfaceType theType)
{
  switch (theType)
  {
    case GeomAbs_Plane:
      return OCCTL_SURFACE_KIND_PLANE;
    case GeomAbs_Cylinder:
      return OCCTL_SURFACE_KIND_CYLINDRICAL;
    case GeomAbs_Cone:
      return OCCTL_SURFACE_KIND_CONICAL;
    case GeomAbs_Sphere:
      return OCCTL_SURFACE_KIND_SPHERICAL;
    case GeomAbs_Torus:
      return OCCTL_SURFACE_KIND_TOROIDAL;
    case GeomAbs_BSplineSurface:
      return OCCTL_SURFACE_KIND_BSPLINE;
    case GeomAbs_BezierSurface:
      return OCCTL_SURFACE_KIND_BEZIER;
    case GeomAbs_SurfaceOfRevolution:
      return OCCTL_SURFACE_KIND_REVOLUTION;
    case GeomAbs_SurfaceOfExtrusion:
      return OCCTL_SURFACE_KIND_EXTRUSION;
    case GeomAbs_OffsetSurface:
      return OCCTL_SURFACE_KIND_OFFSET;
    case GeomAbs_OtherSurface:
      return OCCTL_SURFACE_KIND_UNDEFINED;
  }
  return OCCTL_SURFACE_KIND_UNDEFINED;
}

inline bool IsKnownCurveKind(const occtl_curve_kind_t theKind)
{
  switch (theKind)
  {
    case OCCTL_CURVE_KIND_LINE:
    case OCCTL_CURVE_KIND_CIRCLE:
    case OCCTL_CURVE_KIND_ELLIPSE:
    case OCCTL_CURVE_KIND_HYPERBOLA:
    case OCCTL_CURVE_KIND_PARABOLA:
    case OCCTL_CURVE_KIND_BSPLINE:
    case OCCTL_CURVE_KIND_BEZIER:
    case OCCTL_CURVE_KIND_TRIMMED:
    case OCCTL_CURVE_KIND_OFFSET:
    case OCCTL_CURVE_KIND_UNDEFINED:
      return true;
    case OCCTL_CURVE_KIND_RESERVED_FUTURE:
      return false;
  }
  return false;
}

inline bool IsKnownSurfaceKind(const occtl_surface_kind_t theKind)
{
  switch (theKind)
  {
    case OCCTL_SURFACE_KIND_PLANE:
    case OCCTL_SURFACE_KIND_CYLINDRICAL:
    case OCCTL_SURFACE_KIND_CONICAL:
    case OCCTL_SURFACE_KIND_SPHERICAL:
    case OCCTL_SURFACE_KIND_TOROIDAL:
    case OCCTL_SURFACE_KIND_BSPLINE:
    case OCCTL_SURFACE_KIND_BEZIER:
    case OCCTL_SURFACE_KIND_REVOLUTION:
    case OCCTL_SURFACE_KIND_EXTRUSION:
    case OCCTL_SURFACE_KIND_RECTANGULAR_TRIMMED:
    case OCCTL_SURFACE_KIND_OFFSET:
    case OCCTL_SURFACE_KIND_UNDEFINED:
      return true;
    case OCCTL_SURFACE_KIND_RESERVED_FUTURE:
      return false;
  }
  return false;
}

inline occtl_curve_kind_t EdgeCurveKind(const BRepGraph& theGraph, const BRepGraph_EdgeId theEdge)
{
  if (!BRepGraph_Tool::Edge::HasCurve(theGraph, theEdge))
  {
    return OCCTL_CURVE_KIND_UNDEFINED;
  }

  const TopoDS_Shape anEdgeShape = theGraph.Shapes().Shape(theEdge);
  BRepAdaptor_Curve  anAdaptor(TopoDS::Edge(anEdgeShape));
  return ToAbiCurveKind(anAdaptor.GetType());
}

inline occtl_surface_kind_t FaceSurfaceKind(const BRepGraph&       theGraph,
                                            const BRepGraph_FaceId theFace)
{
  if (!BRepGraph_Tool::Face::HasSurface(theGraph, theFace))
  {
    return OCCTL_SURFACE_KIND_UNDEFINED;
  }

  const TopoDS_Shape  aFaceShape = theGraph.Shapes().Shape(theFace);
  BRepAdaptor_Surface anAdaptor(TopoDS::Face(aFaceShape));
  return ToAbiSurfaceKind(anAdaptor.GetType());
}

} // namespace OcctL::Topo

#endif // OCCTL_TOPO_GRAPH_GEOMETRY_KIND_HXX
