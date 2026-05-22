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

#ifndef OCCTL_GEOM_GEOMMATH_HXX
#define OCCTL_GEOM_GEOMMATH_HXX

#include <occtl/occtl_geom.h>

#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Dir2d.hxx>
#include <gp_GTrsf.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Vec2d.hxx>

#include <cstddef>

//
// Several extractors expose the OCCT internal pole / weight arrays as
// borrowed spans of the public POD types (e.g. occtl_curve_bspline_poles_view
// and the §10.5 aggregate views) by reinterpret_cast'ing &Array.First() (or
// &Array2.Value(LowerRow, LowerCol) for 2D pole grids).  The cast is only
// sound when our POD layout exactly matches the OCCT class layout (size,
// alignment, leading offsets).  gp_Pnt / gp_Pnt2d each hold a single
// gp_XYZ / gp_XY of double members in declaration order, with no virtuals —
// so the layouts coincide.  These asserts pin the assumption and apply to
// both NCollection_Array1<gp_Pnt> and NCollection_Array2<gp_Pnt> use sites,
// since the per-element layout is what matters; both array kinds store
// elements contiguously in their declared type.

static_assert(sizeof(occtl_point3_t) == sizeof(gp_Pnt),
              "occtl_point3_t / gp_Pnt size mismatch — borrowed-span casts unsafe");
static_assert(alignof(occtl_point3_t) == alignof(gp_Pnt),
              "occtl_point3_t / gp_Pnt alignment mismatch — borrowed-span casts unsafe");
static_assert(offsetof(occtl_point3_t, x) == 0,
              "occtl_point3_t::x must be at offset 0 to alias gp_Pnt's first coordinate");
static_assert(offsetof(occtl_point3_t, y) == sizeof(double),
              "occtl_point3_t::y must follow x with no padding");
static_assert(offsetof(occtl_point3_t, z) == 2 * sizeof(double),
              "occtl_point3_t::z must follow y with no padding");

static_assert(sizeof(occtl_point2_t) == sizeof(gp_Pnt2d),
              "occtl_point2_t / gp_Pnt2d size mismatch — borrowed-span casts unsafe");
static_assert(alignof(occtl_point2_t) == alignof(gp_Pnt2d),
              "occtl_point2_t / gp_Pnt2d alignment mismatch — borrowed-span casts unsafe");
static_assert(offsetof(occtl_point2_t, x) == 0,
              "occtl_point2_t::x must be at offset 0 to alias gp_Pnt2d's first coordinate");
static_assert(offsetof(occtl_point2_t, y) == sizeof(double),
              "occtl_point2_t::y must follow x with no padding");

namespace OcctL::Geom
{

//! Convert an ABI 3D point to gp_Pnt.
inline gp_Pnt ToGp(const occtl_point3_t& theP) noexcept
{
  return {theP.x, theP.y, theP.z};
}

//! Convert an ABI 2D point to gp_Pnt2d.
inline gp_Pnt2d ToGp(const occtl_point2_t& theP) noexcept
{
  return {theP.x, theP.y};
}

//! Convert an ABI 3D free vector to gp_Vec.
inline gp_Vec ToGp(const occtl_vector3_t& theV) noexcept
{
  return {theV.x, theV.y, theV.z};
}

//! Convert an ABI 2D free vector to gp_Vec2d.
inline gp_Vec2d ToGp(const occtl_vector2_t& theV) noexcept
{
  return {theV.x, theV.y};
}

//! Convert an ABI 3D unit direction to gp_Dir. Caller guarantees unit norm.
inline gp_Dir ToGp(const occtl_direction3_t& theD) noexcept
{
  return {theD.x, theD.y, theD.z};
}

//! Convert an ABI 2D unit direction to gp_Dir2d. Caller guarantees unit norm.
inline gp_Dir2d ToGp(const occtl_direction2_t& theD) noexcept
{
  return {theD.x, theD.y};
}

//! Converts our general 3×4 row-major matrix to a gp_GTrsf.
//! Calling SetValue for any matrix column forces gp_Other form, so
//! gp_GTrsf::Transforms() always uses the explicit-matrix path.
inline gp_GTrsf ToGpGTrsf(const occtl_transform_t& theT) noexcept
{
  gp_GTrsf aGT;
  for (int aRow = 1; aRow <= 3; ++aRow)
  {
    for (int aCol = 1; aCol <= 4; ++aCol)
    {
      aGT.SetValue(aRow, aCol, theT.m[(aRow - 1) * 4 + (aCol - 1)]);
    }
  }
  return aGT;
}

inline occtl_point3_t FromGp(const gp_Pnt& theP) noexcept
{
  return {theP.X(), theP.Y(), theP.Z()};
}

inline occtl_point2_t FromGp(const gp_Pnt2d& theP) noexcept
{
  return {theP.X(), theP.Y()};
}

inline occtl_vector3_t FromGp(const gp_Vec& theV) noexcept
{
  return {theV.X(), theV.Y(), theV.Z()};
}

inline occtl_vector2_t FromGp(const gp_Vec2d& theV) noexcept
{
  return {theV.X(), theV.Y()};
}

inline occtl_direction3_t FromGp(const gp_Dir& theD) noexcept
{
  return {theD.X(), theD.Y(), theD.Z()};
}

inline occtl_direction2_t FromGp(const gp_Dir2d& theD) noexcept
{
  return {theD.X(), theD.Y()};
}

//! Converts our 3×4 row-major matrix to a gp_Trsf.
inline gp_Trsf ToGpTrsf(const occtl_transform_t& theT) noexcept
{
  gp_Trsf aTrsf;
  aTrsf.SetValues(theT.m[0],
                  theT.m[1],
                  theT.m[2],
                  theT.m[3],
                  theT.m[4],
                  theT.m[5],
                  theT.m[6],
                  theT.m[7],
                  theT.m[8],
                  theT.m[9],
                  theT.m[10],
                  theT.m[11]);
  return aTrsf;
}

//! Converts an occtl_axis1_placement_t to gp_Ax1.
inline gp_Ax1 ToGpAx1(const occtl_axis1_placement_t& theAxis) noexcept
{
  return gp_Ax1(ToGp(theAxis.location), ToGp(theAxis.direction));
}

//! Extracts a gp_Trsf into our 3×4 row-major matrix.
//! gp_Trsf::Value(r, c) returns scale*R[r][c] for cols 1-3 and loc[r] for col 4,
//! which matches our layout exactly.
inline occtl_transform_t FromGp(const gp_Trsf& theT) noexcept
{
  occtl_transform_t aResult{};
  for (int aRow = 1; aRow <= 3; ++aRow)
  {
    for (int aCol = 1; aCol <= 4; ++aCol)
    {
      aResult.m[(aRow - 1) * 4 + (aCol - 1)] = theT.Value(aRow, aCol);
    }
  }
  return aResult;
}

//! Extracts a gp_GTrsf into our 3×4 row-major matrix.
inline occtl_transform_t FromGp(const gp_GTrsf& theT) noexcept
{
  occtl_transform_t aResult{};
  for (int aRow = 1; aRow <= 3; ++aRow)
  {
    for (int aCol = 1; aCol <= 4; ++aCol)
    {
      aResult.m[(aRow - 1) * 4 + (aCol - 1)] = theT.Value(aRow, aCol);
    }
  }
  return aResult;
}

} // namespace OcctL::Geom

#endif // OCCTL_GEOM_GEOMMATH_HXX
