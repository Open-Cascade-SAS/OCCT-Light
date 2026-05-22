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

#ifndef OCCTL_GEOM_KINDDETECT_HXX
#define OCCTL_GEOM_KINDDETECT_HXX

#include <occtl/occtl_curves.h>
#include <occtl/occtl_curves2d.h>
#include <occtl/occtl_surfaces.h>

#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_Hyperbola.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_OffsetCurve.hxx>
#include <Geom2d_Parabola.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_Circle.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_Curve.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_Line.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_Plane.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_Surface.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Standard_Type.hxx>

namespace OcctL::Geom
{

inline occtl_curve_kind_t DetermineCurveKind(const occ::handle<Geom_Curve>& theCurve) noexcept
{
  const occ::handle<Standard_Type>& aType = theCurve->DynamicType();
  if (aType->SubType(STANDARD_TYPE(Geom_TrimmedCurve)))
  {
    return OCCTL_CURVE_KIND_TRIMMED;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_OffsetCurve)))
  {
    return OCCTL_CURVE_KIND_OFFSET;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_BSplineCurve)))
  {
    return OCCTL_CURVE_KIND_BSPLINE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_BezierCurve)))
  {
    return OCCTL_CURVE_KIND_BEZIER;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_Line)))
  {
    return OCCTL_CURVE_KIND_LINE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_Circle)))
  {
    return OCCTL_CURVE_KIND_CIRCLE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_Ellipse)))
  {
    return OCCTL_CURVE_KIND_ELLIPSE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_Hyperbola)))
  {
    return OCCTL_CURVE_KIND_HYPERBOLA;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_Parabola)))
  {
    return OCCTL_CURVE_KIND_PARABOLA;
  }
  return OCCTL_CURVE_KIND_UNDEFINED;
}

inline occtl_curve_kind_t DetermineCurve2dKind(const occ::handle<Geom2d_Curve>& theCurve) noexcept
{
  const occ::handle<Standard_Type>& aType = theCurve->DynamicType();
  if (aType->SubType(STANDARD_TYPE(Geom2d_TrimmedCurve)))
  {
    return OCCTL_CURVE_KIND_TRIMMED;
  }
  if (aType->SubType(STANDARD_TYPE(Geom2d_OffsetCurve)))
  {
    return OCCTL_CURVE_KIND_OFFSET;
  }
  if (aType->SubType(STANDARD_TYPE(Geom2d_BSplineCurve)))
  {
    return OCCTL_CURVE_KIND_BSPLINE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom2d_BezierCurve)))
  {
    return OCCTL_CURVE_KIND_BEZIER;
  }
  if (aType->SubType(STANDARD_TYPE(Geom2d_Line)))
  {
    return OCCTL_CURVE_KIND_LINE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom2d_Circle)))
  {
    return OCCTL_CURVE_KIND_CIRCLE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom2d_Ellipse)))
  {
    return OCCTL_CURVE_KIND_ELLIPSE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom2d_Hyperbola)))
  {
    return OCCTL_CURVE_KIND_HYPERBOLA;
  }
  if (aType->SubType(STANDARD_TYPE(Geom2d_Parabola)))
  {
    return OCCTL_CURVE_KIND_PARABOLA;
  }
  return OCCTL_CURVE_KIND_UNDEFINED;
}

inline occtl_surface_kind_t DetermineSurfaceKind(
  const occ::handle<Geom_Surface>& theSurface) noexcept
{
  const occ::handle<Standard_Type>& aType = theSurface->DynamicType();
  if (aType->SubType(STANDARD_TYPE(Geom_RectangularTrimmedSurface)))
  {
    return OCCTL_SURFACE_KIND_RECTANGULAR_TRIMMED;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_OffsetSurface)))
  {
    return OCCTL_SURFACE_KIND_OFFSET;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_SurfaceOfRevolution)))
  {
    return OCCTL_SURFACE_KIND_REVOLUTION;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_SurfaceOfLinearExtrusion)))
  {
    return OCCTL_SURFACE_KIND_EXTRUSION;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_BSplineSurface)))
  {
    return OCCTL_SURFACE_KIND_BSPLINE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_BezierSurface)))
  {
    return OCCTL_SURFACE_KIND_BEZIER;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_Plane)))
  {
    return OCCTL_SURFACE_KIND_PLANE;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_CylindricalSurface)))
  {
    return OCCTL_SURFACE_KIND_CYLINDRICAL;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_ConicalSurface)))
  {
    return OCCTL_SURFACE_KIND_CONICAL;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_SphericalSurface)))
  {
    return OCCTL_SURFACE_KIND_SPHERICAL;
  }
  if (aType->SubType(STANDARD_TYPE(Geom_ToroidalSurface)))
  {
    return OCCTL_SURFACE_KIND_TOROIDAL;
  }
  return OCCTL_SURFACE_KIND_UNDEFINED;
}

} // namespace OcctL::Geom

#endif // OCCTL_GEOM_KINDDETECT_HXX
