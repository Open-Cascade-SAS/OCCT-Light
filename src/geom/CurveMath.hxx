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

#ifndef OCCTL_GEOM_CURVEMATH_HXX
#define OCCTL_GEOM_CURVEMATH_HXX

#include "GeomMath.hxx"

#include <occtl/occtl_geom.h>

#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax22d.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Ax3.hxx>
#include <gp_Circ.hxx>
#include <gp_Circ2d.hxx>
#include <gp_Cone.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Elips.hxx>
#include <gp_Elips2d.hxx>
#include <gp_Hypr.hxx>
#include <gp_Hypr2d.hxx>
#include <gp_Lin.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Parab.hxx>
#include <gp_Parab2d.hxx>
#include <gp_Pln.hxx>
#include <gp_Sphere.hxx>
#include <gp_Torus.hxx>

// Layout assertions: gp_Pnt / gp_Pnt2d must be identical in size and layout to
// occtl_point3_t / occtl_point2_t so the *_poles_view families can hand out
// zero-copy pointers reinterpret-cast straight from OCCT's contiguous arrays.
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <type_traits>

static_assert(sizeof(gp_Pnt) == sizeof(occtl_point3_t),
              "gp_Pnt and occtl_point3_t must have identical size for poles_view");
static_assert(sizeof(gp_Pnt) == 3 * sizeof(double),
              "gp_Pnt must be exactly three contiguous doubles");
static_assert(sizeof(gp_Pnt2d) == sizeof(occtl_point2_t),
              "gp_Pnt2d and occtl_point2_t must have identical size for poles_view");
static_assert(sizeof(gp_Pnt2d) == 2 * sizeof(double),
              "gp_Pnt2d must be exactly two contiguous doubles");

namespace OcctL::Geom
{

inline gp_Ax2 ToGpAx2(const occtl_axis2_placement_t& theA) noexcept
{
  return gp_Ax2(ToGp(theA.location),
                gp_Dir(theA.x_dir.x, theA.x_dir.y, theA.x_dir.z),
                gp_Dir(theA.x_dir_ref.x, theA.x_dir_ref.y, theA.x_dir_ref.z));
}

inline occtl_axis2_placement_t FromGpAx2(const gp_Ax2& theA) noexcept
{
  return {FromGp(theA.Location()), FromGp(theA.Direction()), FromGp(theA.XDirection())};
}

inline gp_Ax3 ToGpAx3(const occtl_axis3_placement_t& theA) noexcept
{
  return gp_Ax3(ToGp(theA.location),
                gp_Dir(theA.z_dir.x, theA.z_dir.y, theA.z_dir.z),
                gp_Dir(theA.x_dir.x, theA.x_dir.y, theA.x_dir.z));
}

inline occtl_axis3_placement_t FromGpAx3(const gp_Ax3& theA) noexcept
{
  return {FromGp(theA.Location()),
          FromGp(theA.XDirection()),
          FromGp(theA.YDirection()),
          FromGp(theA.Direction())};
}

inline gp_Ax2d ToGpAx2d(const occtl_axis2_placement2d_t& theA) noexcept
{
  return gp_Ax2d(ToGp(theA.location), ToGp(theA.x_dir));
}

inline occtl_axis2_placement2d_t FromGpAx2d(const gp_Ax2d& theA) noexcept
{
  return {FromGp(theA.Location()), FromGp(theA.Direction())};
}

// gp_Ax22d overload — used by gp_Circ2d::Axis(), gp_Elips2d::Axis(), etc.
inline occtl_axis2_placement2d_t FromGpAx2d(const gp_Ax22d& theA) noexcept
{
  return {FromGp(theA.Location()), FromGp(theA.XDirection())};
}

inline gp_Lin ToGpLin(const occtl_geom_line_t& theL) noexcept
{
  return gp_Lin(
    gp_Ax1(ToGp(theL.location), gp_Dir(theL.direction.x, theL.direction.y, theL.direction.z)));
}

inline occtl_geom_line_t FromGpLin(const gp_Lin& theL) noexcept
{
  return {FromGp(theL.Location()), FromGp(theL.Direction())};
}

inline gp_Circ ToGpCirc(const occtl_geom_circle_t& theC) noexcept
{
  return gp_Circ(ToGpAx2(theC.position), theC.radius);
}

inline occtl_geom_circle_t FromGpCirc(const gp_Circ& theC) noexcept
{
  return {FromGpAx2(theC.Position()), theC.Radius()};
}

inline gp_Elips ToGpElips(const occtl_geom_ellipse_t& theE) noexcept
{
  return gp_Elips(ToGpAx2(theE.position), theE.major_radius, theE.minor_radius);
}

inline occtl_geom_ellipse_t FromGpElips(const gp_Elips& theE) noexcept
{
  return {FromGpAx2(theE.Position()), theE.MajorRadius(), theE.MinorRadius()};
}

inline gp_Hypr ToGpHypr(const occtl_geom_hyperbola_t& theH) noexcept
{
  return gp_Hypr(ToGpAx2(theH.position), theH.major_radius, theH.minor_radius);
}

inline occtl_geom_hyperbola_t FromGpHypr(const gp_Hypr& theH) noexcept
{
  return {FromGpAx2(theH.Position()), theH.MajorRadius(), theH.MinorRadius()};
}

inline gp_Parab ToGpParab(const occtl_geom_parabola_t& theP) noexcept
{
  return gp_Parab(ToGpAx2(theP.position), theP.focal_length);
}

inline occtl_geom_parabola_t FromGpParab(const gp_Parab& theP) noexcept
{
  return {FromGpAx2(theP.Position()), theP.Focal()};
}

inline gp_Lin2d ToGpLin2d(const occtl_geom2d_line_t& theL) noexcept
{
  return gp_Lin2d(ToGpAx2d(theL.position));
}

inline occtl_geom2d_line_t FromGpLin2d(const gp_Lin2d& theL) noexcept
{
  return {FromGpAx2d(theL.Position())};
}

inline gp_Circ2d ToGpCirc2d(const occtl_geom2d_circle_t& theC) noexcept
{
  return gp_Circ2d(ToGpAx2d(theC.position), theC.radius);
}

inline occtl_geom2d_circle_t FromGpCirc2d(const gp_Circ2d& theC) noexcept
{
  return {FromGpAx2d(theC.Axis()), theC.Radius()};
}

inline gp_Elips2d ToGpElips2d(const occtl_geom2d_ellipse_t& theE) noexcept
{
  return gp_Elips2d(ToGpAx2d(theE.position), theE.major_radius, theE.minor_radius);
}

inline occtl_geom2d_ellipse_t FromGpElips2d(const gp_Elips2d& theE) noexcept
{
  return {FromGpAx2d(theE.Axis()), theE.MajorRadius(), theE.MinorRadius()};
}

inline gp_Hypr2d ToGpHypr2d(const occtl_geom2d_hyperbola_t& theH) noexcept
{
  return gp_Hypr2d(ToGpAx2d(theH.position), theH.major_radius, theH.minor_radius);
}

inline occtl_geom2d_hyperbola_t FromGpHypr2d(const gp_Hypr2d& theH) noexcept
{
  return {FromGpAx2d(theH.Axis()), theH.MajorRadius(), theH.MinorRadius()};
}

inline gp_Parab2d ToGpParab2d(const occtl_geom2d_parabola_t& theP) noexcept
{
  return gp_Parab2d(ToGpAx2d(theP.position), theP.focal_length);
}

inline occtl_geom2d_parabola_t FromGpParab2d(const gp_Parab2d& theP) noexcept
{
  return {FromGpAx2d(theP.Axis()), theP.Focal()};
}

inline gp_Pln ToGpPln(const occtl_geom_plane_t& theP) noexcept
{
  return gp_Pln(ToGpAx3(theP.position));
}

inline occtl_geom_plane_t FromGpPln(const gp_Pln& theP) noexcept
{
  return {FromGpAx3(theP.Position())};
}

inline gp_Cylinder ToGpCylinder(const occtl_geom_cylindrical_surface_t& theC) noexcept
{
  return gp_Cylinder(ToGpAx3(theC.position), theC.radius);
}

inline occtl_geom_cylindrical_surface_t FromGpCylinder(const gp_Cylinder& theC) noexcept
{
  return {FromGpAx3(theC.Position()), theC.Radius()};
}

inline gp_Sphere ToGpSphere(const occtl_geom_spherical_surface_t& theS) noexcept
{
  return gp_Sphere(ToGpAx3(theS.position), theS.radius);
}

inline occtl_geom_spherical_surface_t FromGpSphere(const gp_Sphere& theS) noexcept
{
  return {FromGpAx3(theS.Position()), theS.Radius()};
}

inline gp_Cone ToGpCone(const occtl_geom_conical_surface_t& theC) noexcept
{
  return gp_Cone(ToGpAx3(theC.position), theC.semi_angle, theC.radius);
}

inline occtl_geom_conical_surface_t FromGpCone(const gp_Cone& theC) noexcept
{
  return {FromGpAx3(theC.Position()), theC.SemiAngle(), theC.RefRadius()};
}

inline gp_Torus ToGpTorus(const occtl_geom_toroidal_surface_t& theT) noexcept
{
  return gp_Torus(ToGpAx3(theT.position), theT.major_radius, theT.minor_radius);
}

inline occtl_geom_toroidal_surface_t FromGpTorus(const gp_Torus& theT) noexcept
{
  return {FromGpAx3(theT.Position()), theT.MajorRadius(), theT.MinorRadius()};
}

} // namespace OcctL::Geom

#endif // OCCTL_GEOM_CURVEMATH_HXX
