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

#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"
#include "CurveMath.hxx"
#include "GeomMath.hxx"
#include "KindDetect.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include <occtl/occtl_core.h>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepGProp.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_Array2.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <gp.hxx>

namespace OcctL::Geom
{

//================================================================================
//! Fills an aggregate view from the internal arrays of a Geom_BSplineSurface.
//! Caller is responsible for the WRONG_KIND / VERSION_MISMATCH / NULL guards;
//! this helper assumes both inputs are valid and the surface handle is non-null.
//!
//! Layout invariant exposed to the caller: @c poles and @c weights are
//! row-major with U as the major axis — element @c (u_index, v_index) lives
//! at offset @c u_index * v_pole_count + v_index.  This matches OCCT's own
//! @c NCollection_Array2 layout (row-major, row = U index) and the public
//! @c occtl_surface_bspline_create_info_t convention.
inline void FillBSplineSurfaceView(const occ::handle<Geom_BSplineSurface>& theBspl,
                                   occtl_surface_bspline_t&                theOut)
{
  theOut.u_degree      = theBspl->UDegree();
  theOut.v_degree      = theBspl->VDegree();
  theOut.is_rational   = (theBspl->IsURational() || theBspl->IsVRational()) ? 1 : 0;
  theOut.is_u_periodic = theBspl->IsUPeriodic() ? 1 : 0;
  theOut.is_v_periodic = theBspl->IsVPeriodic() ? 1 : 0;

  const NCollection_Array2<gp_Pnt>& aPoles   = theBspl->Poles();
  const NCollection_Array1<double>& aUKnots  = theBspl->UKnots();
  const NCollection_Array1<double>& aVKnots  = theBspl->VKnots();
  const NCollection_Array1<int>&    aUMults  = theBspl->UMultiplicities();
  const NCollection_Array1<int>&    aVMults  = theBspl->VMultiplicities();
  const NCollection_Array1<double>& aUFlatKn = theBspl->UKnotSequence();
  const NCollection_Array1<double>& aVFlatKn = theBspl->VKnotSequence();

  theOut.u_pole_count      = static_cast<size_t>(theBspl->NbUPoles());
  theOut.v_pole_count      = static_cast<size_t>(theBspl->NbVPoles());
  theOut.u_knot_count      = static_cast<size_t>(aUKnots.Size());
  theOut.v_knot_count      = static_cast<size_t>(aVKnots.Size());
  theOut.u_flat_knot_count = static_cast<size_t>(aUFlatKn.Size());
  theOut.v_flat_knot_count = static_cast<size_t>(aVFlatKn.Size());

  // NCollection_Array2 stores its elements contiguously in row-major order
  // (row = U): &Value(LowerRow, LowerCol) is the start of the flat buffer.
  // The layout-compat asserts in GeomMath.hxx for occtl_point3_t / gp_Pnt
  // cover both Array1 and Array2 since the element type is the same.
  theOut.poles =
    reinterpret_cast<const occtl_point3_t*>(&aPoles.Value(aPoles.LowerRow(), aPoles.LowerCol()));

  theOut.u_knots      = &aUKnots.First();
  theOut.v_knots      = &aVKnots.First();
  theOut.u_flat_knots = &aUFlatKn.First();
  theOut.v_flat_knots = &aVFlatKn.First();

  static_assert(sizeof(int) == sizeof(int32_t),
                "int and int32_t must have identical width for multiplicities aliasing");
  theOut.u_multiplicities = reinterpret_cast<const int32_t*>(&aUMults.First());
  theOut.v_multiplicities = reinterpret_cast<const int32_t*>(&aVMults.First());

  // Geom_BSplineSurface::Weights() returns NULL when the surface is non-rational.
  const NCollection_Array2<double>* aWeights = theBspl->Weights();
  theOut.weights =
    (aWeights != nullptr) ? &aWeights->Value(aWeights->LowerRow(), aWeights->LowerCol()) : nullptr;
}

} // namespace OcctL::Geom

namespace
{

occtl_status_t SurfaceFlatKnotsCommon(const occtl_graph_t* theGraph,
                                      occtl_rep_id_t       theSurfaceId,
                                      const bool           theIsU,
                                      double*              theOutBuf,
                                      const size_t         theCapacity,
                                      size_t*              theOutCount)
{
  if (theGraph == nullptr || theOutCount == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "graph and out_count must be non-NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theSurfaceId);
  if (!aRawId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep not found");
    return OCCTL_NOT_FOUND;
  }
  if (aRawId.RepKind != BRepGraph_RepId::Kind::Surface)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep is not a surface");
    return OCCTL_WRONG_KIND;
  }
  BRepGraph_SurfaceRepId           aSurfRepId(static_cast<uint32_t>(aRawId.Index));
  const occ::handle<Geom_Surface>& aSurface =
    theGraph->graph.Topo().Geometry().SurfaceRep(aSurfRepId).Surface;
  const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(aSurface);
  if (aBs.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
    return OCCTL_WRONG_KIND;
  }
  const int aNbKnots = theIsU ? aBs->NbUKnots() : aBs->NbVKnots();
  size_t    aFlat    = 0;
  for (int anI = 1; anI <= aNbKnots; ++anI)
  {
    aFlat += static_cast<size_t>(theIsU ? aBs->UMultiplicity(anI) : aBs->VMultiplicity(anI));
  }
  *theOutCount = aFlat;
  if (theOutBuf == nullptr)
  {
    return OCCTL_OK;
  }
  if (theCapacity < aFlat)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                           "buffer too small for bspline surface flat knots");
    return OCCTL_BUFFER_TOO_SMALL;
  }
  size_t aOut = 0;
  for (int anI = 1; anI <= aNbKnots; ++anI)
  {
    const double aV = theIsU ? aBs->UKnot(anI) : aBs->VKnot(anI);
    const int    aM = theIsU ? aBs->UMultiplicity(anI) : aBs->VMultiplicity(anI);
    for (int aR = 0; aR < aM; ++aR)
    {
      theOutBuf[aOut++] = aV;
    }
  }
  return OCCTL_OK;
}

//! Helper: unpacks a surface rep id and returns the Geom_Surface handle.
//! On failure sets the error state and returns nullptr; the caller must
//! propagate the status stored via @p theOutStatus.
inline const occ::handle<Geom_Surface>* UnpackSurface(const occtl_graph_t* theGraph,
                                                      occtl_rep_id_t       theSurfaceId,
                                                      occtl_status_t&      theOutStatus)
{
  const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theSurfaceId);
  if (!aRawId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep not found");
    theOutStatus = OCCTL_NOT_FOUND;
    return nullptr;
  }
  if (aRawId.RepKind != BRepGraph_RepId::Kind::Surface)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep is not a surface");
    theOutStatus = OCCTL_WRONG_KIND;
    return nullptr;
  }
  BRepGraph_SurfaceRepId           aSurfRepId(static_cast<uint32_t>(aRawId.Index));
  const occ::handle<Geom_Surface>& aSurface =
    theGraph->graph.Topo().Geometry().SurfaceRep(aSurfRepId).Surface;
  theOutStatus = OCCTL_OK;
  return &aSurface;
}

} // namespace

extern "C"
{

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_create_plane(occtl_graph_t*     theGraph,
                                                               occtl_rep_id_t*    theOutId,
                                                               occtl_geom_plane_t thePlane)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occ::handle<Geom_Plane> aPlane = new Geom_Plane(OcctL::Geom::ToGpPln(thePlane));
    BRepGraph_SurfaceRepId  aRepId = theGraph->graph.Editor().Reps().CreateSurface(aPlane);
    *theOutId                      = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_cylinder(occtl_graph_t*                   theGraph,
                                occtl_rep_id_t*                  theOutId,
                                occtl_geom_cylindrical_surface_t theCylinder)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theCylinder.radius <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "cylinder radius must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom_CylindricalSurface> aCyl =
      new Geom_CylindricalSurface(OcctL::Geom::ToGpCylinder(theCylinder));
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aCyl);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_create_cone(occtl_graph_t*               theGraph,
                                                              occtl_rep_id_t*              theOutId,
                                                              occtl_geom_conical_surface_t theCone)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theCone.semi_angle <= 0.0 || theCone.semi_angle >= OCCTL_PI_OVER_TWO)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "cone semi_angle must be in (0, pi/2)");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom_ConicalSurface> aCone =
      new Geom_ConicalSurface(OcctL::Geom::ToGpCone(theCone));
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aCone);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_sphere(occtl_graph_t*                 theGraph,
                              occtl_rep_id_t*                theOutId,
                              occtl_geom_spherical_surface_t theSphere)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theSphere.radius <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "sphere radius must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom_SphericalSurface> aSphere =
      new Geom_SphericalSurface(OcctL::Geom::ToGpSphere(theSphere));
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aSphere);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_torus(occtl_graph_t*                theGraph,
                             occtl_rep_id_t*               theOutId,
                             occtl_geom_toroidal_surface_t theTorus)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theTorus.minor_radius <= 0.0 || theTorus.major_radius <= theTorus.minor_radius)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "torus: minor_radius > 0 and major > minor required");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom_ToroidalSurface> aTorus =
      new Geom_ToroidalSurface(OcctL::Geom::ToGpTorus(theTorus));
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aTorus);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_revolution_create_info_init(occtl_surface_revolution_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  theInfo->struct_version = OCCTL_SURFACE_REVOLUTION_CREATE_INFO_VERSION_1;
  theInfo->p_next         = nullptr;
  theInfo->basis          = OCCTL_REP_ID_INVALID;
  theInfo->axis           = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_revolution(occtl_graph_t*                                theGraph,
                                  occtl_rep_id_t*                               theOutId,
                                  const occtl_surface_revolution_create_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_REVOLUTION_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "unsupported struct_version in revolution create_info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const gp_Vec aAxisDirection(theInfo->axis.direction.x,
                                theInfo->axis.direction.y,
                                theInfo->axis.direction.z);
    if (aAxisDirection.SquareMagnitude() <= Precision::SquareConfusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "revolution axis direction must have non-zero length");
      return OCCTL_GEOMETRY_INVALID;
    }
    const BRepGraph_RepId aRawBasis = OcctL::Topo::UnpackRepId(theInfo->basis);
    if (!aRawBasis.IsValid() || aRawBasis.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "basis curve rep is invalid or not a Curve3D");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_Curve3DRepId         aBasisId(static_cast<uint32_t>(aRawBasis.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aBasisId).Curve;
    const gp_Ax1                          anAxis(OcctL::Geom::ToGp(theInfo->axis.location),
                                                 OcctL::Geom::ToGp(theInfo->axis.direction));
    occ::handle<Geom_SurfaceOfRevolution> aRev = new Geom_SurfaceOfRevolution(aCurve, anAxis);
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aRev);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_extrusion_create_info_init(occtl_surface_extrusion_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  theInfo->struct_version = OCCTL_SURFACE_EXTRUSION_CREATE_INFO_VERSION_1;
  theInfo->p_next         = nullptr;
  theInfo->basis          = OCCTL_REP_ID_INVALID;
  theInfo->direction      = {0.0, 0.0, 1.0};
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_extrusion(occtl_graph_t*                               theGraph,
                                 occtl_rep_id_t*                              theOutId,
                                 const occtl_surface_extrusion_create_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_EXTRUSION_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in extrusion create_info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawBasis = OcctL::Topo::UnpackRepId(theInfo->basis);
    if (!aRawBasis.IsValid() || aRawBasis.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "basis curve rep is invalid or not a Curve3D");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_Curve3DRepId         aBasisId(static_cast<uint32_t>(aRawBasis.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aBasisId).Curve;
    const gp_Vec aDir = OcctL::Geom::ToGp(theInfo->direction);
    if (aDir.SquareMagnitude() <= Precision::SquareConfusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "extrusion direction must have non-zero length");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom_SurfaceOfLinearExtrusion> anExtrusion =
      new Geom_SurfaceOfLinearExtrusion(aCurve, gp_Dir(aDir));
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(anExtrusion);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API void OCCTL_CALL occtl_surface_rectangular_trimmed_create_info_init(
  occtl_surface_rectangular_trimmed_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  theInfo->struct_version = OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_VERSION_1;
  theInfo->p_next         = nullptr;
  theInfo->basis          = OCCTL_REP_ID_INVALID;
  theInfo->u_first        = 0.0;
  theInfo->u_last         = 1.0;
  theInfo->v_first        = 0.0;
  theInfo->v_last         = 1.0;
  theInfo->u_sense        = 1;
  theInfo->v_sense        = 1;
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_create_rectangular_trimmed(
  occtl_graph_t*                                         theGraph,
  occtl_rep_id_t*                                        theOutId,
  const occtl_surface_rectangular_trimmed_create_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "unsupported struct_version in rect_trimmed create_info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawBasis = OcctL::Topo::UnpackRepId(theInfo->basis);
    if (!aRawBasis.IsValid() || aRawBasis.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "basis surface rep is invalid or not a surface");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_SurfaceRepId           aBasisSurfId(static_cast<uint32_t>(aRawBasis.Index));
    const occ::handle<Geom_Surface>& aBasisSurface =
      theGraph->graph.Topo().Geometry().SurfaceRep(aBasisSurfId).Surface;
    if (theInfo->u_last <= theInfo->u_first || theInfo->v_last <= theInfo->v_first)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "rect_trimmed: u_last > u_first and v_last > v_first required");
      return OCCTL_GEOMETRY_INVALID;
    }
    if ((theInfo->u_sense != 1 && theInfo->u_sense != -1)
        || (theInfo->v_sense != 1 && theInfo->v_sense != -1))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "rect_trimmed: u_sense and v_sense must each be 1 or -1");
      return OCCTL_INVALID_ARGUMENT;
    }
    occ::handle<Geom_RectangularTrimmedSurface> aTrim =
      new Geom_RectangularTrimmedSurface(aBasisSurface,
                                         theInfo->u_first,
                                         theInfo->u_last,
                                         theInfo->v_first,
                                         theInfo->v_last,
                                         theInfo->u_sense == 1,
                                         theInfo->v_sense == 1);
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aTrim);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_offset_create_info_init(occtl_surface_offset_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  theInfo->struct_version = OCCTL_SURFACE_OFFSET_CREATE_INFO_VERSION_1;
  theInfo->p_next         = nullptr;
  theInfo->basis          = OCCTL_REP_ID_INVALID;
  theInfo->offset         = 0.0;
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_offset(occtl_graph_t*                            theGraph,
                              occtl_rep_id_t*                           theOutId,
                              const occtl_surface_offset_create_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_OFFSET_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in offset create_info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawBasis = OcctL::Topo::UnpackRepId(theInfo->basis);
    if (!aRawBasis.IsValid() || aRawBasis.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "basis surface rep is invalid or not a surface");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_SurfaceRepId           aBasisSurfId(static_cast<uint32_t>(aRawBasis.Index));
    const occ::handle<Geom_Surface>& aBasisSurface =
      theGraph->graph.Topo().Geometry().SurfaceRep(aBasisSurfId).Surface;
    occ::handle<Geom_OffsetSurface> anOffset =
      new Geom_OffsetSurface(aBasisSurface, theInfo->offset);
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(anOffset);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_bspline_create_info_init(occtl_surface_bspline_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  theInfo->struct_version   = OCCTL_SURFACE_BSPLINE_CREATE_INFO_VERSION_1;
  theInfo->p_next           = nullptr;
  theInfo->poles            = nullptr;
  theInfo->u_pole_count     = 0;
  theInfo->v_pole_count     = 0;
  theInfo->weights          = nullptr;
  theInfo->u_knots          = nullptr;
  theInfo->u_multiplicities = nullptr;
  theInfo->u_knot_count     = 0;
  theInfo->v_knots          = nullptr;
  theInfo->v_multiplicities = nullptr;
  theInfo->v_knot_count     = 0;
  theInfo->u_degree         = 0;
  theInfo->v_degree         = 0;
  theInfo->is_u_periodic    = 0;
  theInfo->is_v_periodic    = 0;
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_bspline(occtl_graph_t*                             theGraph,
                               occtl_rep_id_t*                            theOutId,
                               const occtl_surface_bspline_create_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_BSPLINE_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in bspline create_info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->poles == nullptr || theInfo->u_knots == nullptr || theInfo->v_knots == nullptr
        || theInfo->u_multiplicities == nullptr || theInfo->v_multiplicities == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "poles, knots, and multiplicities must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if ((theInfo->is_u_periodic != 0 && theInfo->is_u_periodic != 1)
        || (theInfo->is_v_periodic != 0 && theInfo->is_v_periodic != 1))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "periodic flags must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->u_pole_count == 0 || theInfo->v_pole_count == 0 || theInfo->u_knot_count == 0
        || theInfo->v_knot_count == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "u/v pole and knot counts must be greater than zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->u_degree < 1 || theInfo->v_degree < 1
        || static_cast<size_t>(theInfo->u_degree) > theInfo->u_pole_count - 1
        || static_cast<size_t>(theInfo->v_degree) > theInfo->v_pole_count - 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "u/v degrees must be >= 1 and <= pole_count - 1");
      return OCCTL_INVALID_ARGUMENT;
    }

    const int aNbU = static_cast<int>(theInfo->u_pole_count);
    const int aNbV = static_cast<int>(theInfo->v_pole_count);

    NCollection_Array2<gp_Pnt> aPoles(1, aNbU, 1, aNbV);
    for (int aU = 1; aU <= aNbU; ++aU)
    {
      for (int aV = 1; aV <= aNbV; ++aV)
      {
        const occtl_point3_t& aP = theInfo->poles[(aU - 1) * aNbV + (aV - 1)];
        aPoles.SetValue(aU, aV, OcctL::Geom::ToGp(aP));
      }
    }

    NCollection_Array1<double> aUKnots(1, static_cast<int>(theInfo->u_knot_count));
    NCollection_Array1<int>    aUMults(1, static_cast<int>(theInfo->u_knot_count));
    for (int anI = 1; anI <= static_cast<int>(theInfo->u_knot_count); ++anI)
    {
      aUKnots.SetValue(anI, theInfo->u_knots[anI - 1]);
      aUMults.SetValue(anI, theInfo->u_multiplicities[anI - 1]);
    }

    NCollection_Array1<double> aVKnots(1, static_cast<int>(theInfo->v_knot_count));
    NCollection_Array1<int>    aVMults(1, static_cast<int>(theInfo->v_knot_count));
    for (int anI = 1; anI <= static_cast<int>(theInfo->v_knot_count); ++anI)
    {
      aVKnots.SetValue(anI, theInfo->v_knots[anI - 1]);
      aVMults.SetValue(anI, theInfo->v_multiplicities[anI - 1]);
    }

    occ::handle<Geom_BSplineSurface> aBspline;
    if (theInfo->weights != nullptr)
    {
      NCollection_Array2<double> aWeights(1, aNbU, 1, aNbV);
      for (int aU = 1; aU <= aNbU; ++aU)
      {
        for (int aV = 1; aV <= aNbV; ++aV)
        {
          aWeights.SetValue(aU, aV, theInfo->weights[(aU - 1) * aNbV + (aV - 1)]);
        }
      }
      aBspline = new Geom_BSplineSurface(aPoles,
                                         aWeights,
                                         aUKnots,
                                         aVKnots,
                                         aUMults,
                                         aVMults,
                                         theInfo->u_degree,
                                         theInfo->v_degree,
                                         theInfo->is_u_periodic != 0,
                                         theInfo->is_v_periodic != 0);
    }
    else
    {
      aBspline = new Geom_BSplineSurface(aPoles,
                                         aUKnots,
                                         aVKnots,
                                         aUMults,
                                         aVMults,
                                         theInfo->u_degree,
                                         theInfo->v_degree,
                                         theInfo->is_u_periodic != 0,
                                         theInfo->is_v_periodic != 0);
    }

    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aBspline);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_kind(const occtl_graph_t*  theGraph,
                                                       occtl_rep_id_t        theSurfaceId,
                                                       occtl_surface_kind_t* theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_kind is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    *theOutKind = OcctL::Geom::DetermineSurfaceKind(*aSurface);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_is_u_periodic(const occtl_graph_t* theGraph,
                                                                occtl_rep_id_t       theSurfaceId,
                                                                int32_t* theOutIsPeriodic)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIsPeriodic == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_is_periodic is NULL"
                                                      : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    *theOutIsPeriodic = (*aSurface)->IsUPeriodic() ? 1 : 0;
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_is_v_periodic(const occtl_graph_t* theGraph,
                                                                occtl_rep_id_t       theSurfaceId,
                                                                int32_t* theOutIsPeriodic)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIsPeriodic == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_is_periodic is NULL"
                                                      : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    *theOutIsPeriodic = (*aSurface)->IsVPeriodic() ? 1 : 0;
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_is_closed(const occtl_graph_t* theGraph,
                                                            occtl_rep_id_t       theSurfaceId,
                                                            int32_t*             theOutIsClosed)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIsClosed == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_is_closed is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    *theOutIsClosed = ((*aSurface)->IsUClosed() && (*aSurface)->IsVClosed()) ? 1 : 0;
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_is_periodic(const occtl_graph_t* theGraph,
                                                              occtl_rep_id_t       theSurfaceId,
                                                              int32_t*             theOutIsPeriodic)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIsPeriodic == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_is_periodic is NULL"
                                                      : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    *theOutIsPeriodic = ((*aSurface)->IsUPeriodic() || (*aSurface)->IsVPeriodic()) ? 1 : 0;
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_continuity(occtl_graph_t*           theGraph,
                           occtl_rep_id_t           theSurfaceId,
                           occtl_geom_continuity_t* theOutContinuity)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutContinuity == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_continuity is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    *theOutContinuity =
      static_cast<occtl_geom_continuity_t>(static_cast<int>((*aSurface)->Continuity()));
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_parameter_range(const occtl_graph_t* theGraph,
                                                                  occtl_rep_id_t       theSurfaceId,
                                                                  double*              theOutUMin,
                                                                  double*              theOutUMax,
                                                                  double*              theOutVMin,
                                                                  double*              theOutVMax)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    double aU1 = 0.0, aU2 = 0.0, aV1 = 0.0, aV2 = 0.0;
    (*aSurface)->Bounds(aU1, aU2, aV1, aV2);
    if (theOutUMin != nullptr)
    {
      *theOutUMin = aU1;
    }
    if (theOutUMax != nullptr)
    {
      *theOutUMax = aU2;
    }
    if (theOutVMin != nullptr)
    {
      *theOutVMin = aV1;
    }
    if (theOutVMax != nullptr)
    {
      *theOutVMax = aV2;
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_plane(const occtl_graph_t* theGraph,
                                                           occtl_rep_id_t       theSurfaceId,
                                                           occtl_geom_plane_t*  theOutPlane)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutPlane == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_plane is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_Plane> aPlane = occ::down_cast<Geom_Plane>(*aSurface);
    if (aPlane.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a plane");
      return OCCTL_WRONG_KIND;
    }
    *theOutPlane = OcctL::Geom::FromGpPln(aPlane->Pln());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_as_cylinder(occtl_graph_t*                    theGraph,
                            occtl_rep_id_t                    theSurfaceId,
                            occtl_geom_cylindrical_surface_t* theOutCylinder)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCylinder == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_cylinder is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_CylindricalSurface> aCyl =
      occ::down_cast<Geom_CylindricalSurface>(*aSurface);
    if (aCyl.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "surface is not a cylindrical surface");
      return OCCTL_WRONG_KIND;
    }
    *theOutCylinder = OcctL::Geom::FromGpCylinder(aCyl->Cylinder());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_cone(const occtl_graph_t* theGraph,
                                                          occtl_rep_id_t       theSurfaceId,
                                                          occtl_geom_conical_surface_t* theOutCone)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCone == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_cone is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_ConicalSurface> aCone = occ::down_cast<Geom_ConicalSurface>(*aSurface);
    if (aCone.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a conical surface");
      return OCCTL_WRONG_KIND;
    }
    *theOutCone = OcctL::Geom::FromGpCone(aCone->Cone());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_as_sphere(occtl_graph_t*                  theGraph,
                          occtl_rep_id_t                  theSurfaceId,
                          occtl_geom_spherical_surface_t* theOutSphere)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutSphere == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_sphere is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_SphericalSurface> aSphere =
      occ::down_cast<Geom_SphericalSurface>(*aSurface);
    if (aSphere.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "surface is not a spherical surface");
      return OCCTL_WRONG_KIND;
    }
    *theOutSphere = OcctL::Geom::FromGpSphere(aSphere->Sphere());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_torus(const occtl_graph_t* graph,
                                                           occtl_rep_id_t       surface_id,
                                                           occtl_geom_toroidal_surface_t* out_torus)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_torus == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             graph ? "out_torus is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(graph, surface_id, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_ToroidalSurface> aTorus =
      occ::down_cast<Geom_ToroidalSurface>(*aSurface);
    if (aTorus.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a toroidal surface");
      return OCCTL_WRONG_KIND;
    }
    *out_torus = OcctL::Geom::FromGpTorus(aTorus->Torus());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_revolution(const occtl_graph_t* theGraph,
                                                                occtl_rep_id_t       theSurfaceId,
                                                                occtl_axis1_placement_t* theOutAxis)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_SurfaceOfRevolution> aRev =
      occ::down_cast<Geom_SurfaceOfRevolution>(*aSurface);
    if (aRev.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "surface is not a surface of revolution");
      return OCCTL_WRONG_KIND;
    }
    if (theOutAxis != nullptr)
    {
      const gp_Ax1& aAx     = aRev->Axis();
      theOutAxis->location  = OcctL::Geom::FromGp(aAx.Location());
      theOutAxis->direction = OcctL::Geom::FromGp(aAx.Direction());
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_extrusion(const occtl_graph_t* theGraph,
                                                               occtl_rep_id_t       theSurfaceId,
                                                               occtl_vector3_t*     theOutDirection)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_SurfaceOfLinearExtrusion> anExt =
      occ::down_cast<Geom_SurfaceOfLinearExtrusion>(*aSurface);
    if (anExt.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a linear extrusion");
      return OCCTL_WRONG_KIND;
    }
    if (theOutDirection != nullptr)
    {
      const gp_Dir& aD = anExt->Direction();
      *theOutDirection = {aD.X(), aD.Y(), aD.Z()};
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_as_rectangular_trimmed(const occtl_graph_t* theGraph,
                                       occtl_rep_id_t       theSurfaceId,
                                       double*              theOutUFirst,
                                       double*              theOutULast,
                                       double*              theOutVFirst,
                                       double*              theOutVLast)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_RectangularTrimmedSurface> aTrim =
      occ::down_cast<Geom_RectangularTrimmedSurface>(*aSurface);
    if (aTrim.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "surface is not a rectangular trimmed surface");
      return OCCTL_WRONG_KIND;
    }
    double aU1 = 0.0, aU2 = 0.0, aV1 = 0.0, aV2 = 0.0;
    aTrim->Bounds(aU1, aU2, aV1, aV2);
    if (theOutUFirst != nullptr)
    {
      *theOutUFirst = aU1;
    }
    if (theOutULast != nullptr)
    {
      *theOutULast = aU2;
    }
    if (theOutVFirst != nullptr)
    {
      *theOutVFirst = aV1;
    }
    if (theOutVLast != nullptr)
    {
      *theOutVLast = aV2;
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_offset(const occtl_graph_t* theGraph,
                                                            occtl_rep_id_t       theSurfaceId,
                                                            double*              theOutOffset)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_OffsetSurface> anOffset = occ::down_cast<Geom_OffsetSurface>(*aSurface);
    if (anOffset.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not an offset surface");
      return OCCTL_WRONG_KIND;
    }
    if (theOutOffset != nullptr)
    {
      *theOutOffset = anOffset->Offset();
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_u_degree(const occtl_graph_t* theGraph,
                                                                   occtl_rep_id_t theSurfaceId,
                                                                   int32_t*       theOutDegree)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutDegree == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_degree is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutDegree = aBs->UDegree();
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_v_degree(const occtl_graph_t* theGraph,
                                                                   occtl_rep_id_t theSurfaceId,
                                                                   int32_t*       theOutDegree)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutDegree == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_degree is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutDegree = aBs->VDegree();
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_u_pole_count(const occtl_graph_t* theGraph,
                                     occtl_rep_id_t       theSurfaceId,
                                     size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_count is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutCount = static_cast<size_t>(aBs->NbUPoles());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_v_pole_count(const occtl_graph_t* theGraph,
                                     occtl_rep_id_t       theSurfaceId,
                                     size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_count is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutCount = static_cast<size_t>(aBs->NbVPoles());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_u_knot_count(const occtl_graph_t* theGraph,
                                     occtl_rep_id_t       theSurfaceId,
                                     size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_count is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutCount = static_cast<size_t>(aBs->NbUKnots());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_v_knot_count(const occtl_graph_t* theGraph,
                                     occtl_rep_id_t       theSurfaceId,
                                     size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_count is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutCount = static_cast<size_t>(aBs->NbVKnots());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_is_rational(const occtl_graph_t* theGraph,
                                                                      occtl_rep_id_t theSurfaceId,
                                                                      int32_t*       theOutRational)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutRational == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_is_rational is NULL"
                                                      : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutRational = (aBs->IsURational() || aBs->IsVRational()) ? 1 : 0;
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_u_degree(const occtl_graph_t* theGraph,
                                                                  occtl_rep_id_t       theSurfaceId,
                                                                  int32_t*             theOutDegree)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutDegree == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_degree is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BezierSurface> aBz = occ::down_cast<Geom_BezierSurface>(*aSurface);
    if (aBz.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a Bezier");
      return OCCTL_WRONG_KIND;
    }
    *theOutDegree = aBz->UDegree();
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_v_degree(const occtl_graph_t* theGraph,
                                                                  occtl_rep_id_t       theSurfaceId,
                                                                  int32_t*             theOutDegree)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutDegree == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_degree is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BezierSurface> aBz = occ::down_cast<Geom_BezierSurface>(*aSurface);
    if (aBz.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a Bezier");
      return OCCTL_WRONG_KIND;
    }
    *theOutDegree = aBz->VDegree();
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_u_pole_count(const occtl_graph_t* theGraph,
                                                                      occtl_rep_id_t theSurfaceId,
                                                                      size_t*        theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_count is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BezierSurface> aBz = occ::down_cast<Geom_BezierSurface>(*aSurface);
    if (aBz.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a Bezier");
      return OCCTL_WRONG_KIND;
    }
    *theOutCount = static_cast<size_t>(aBz->NbUPoles());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_v_pole_count(const occtl_graph_t* theGraph,
                                                                      occtl_rep_id_t theSurfaceId,
                                                                      size_t*        theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_count is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BezierSurface> aBz = occ::down_cast<Geom_BezierSurface>(*aSurface);
    if (aBz.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a Bezier");
      return OCCTL_WRONG_KIND;
    }
    *theOutCount = static_cast<size_t>(aBz->NbVPoles());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bezier_is_rational(const occtl_graph_t* theGraph,
                                                                     occtl_rep_id_t theSurfaceId,
                                                                     int32_t*       theOutRational)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutRational == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_is_rational is NULL"
                                                      : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BezierSurface> aBz = occ::down_cast<Geom_BezierSurface>(*aSurface);
    if (aBz.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a Bezier");
      return OCCTL_WRONG_KIND;
    }
    *theOutRational = (aBz->IsURational() || aBz->IsVRational()) ? 1 : 0;
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_poles(const occtl_graph_t* theGraph,
                                                                occtl_rep_id_t       theSurfaceId,
                                                                occtl_point3_t*      theOutBuf,
                                                                size_t               theCapacity,
                                                                size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNbU = static_cast<size_t>(aBs->NbUPoles());
    const size_t aNbV = static_cast<size_t>(aBs->NbVPoles());
    const size_t aNb  = aNbU * aNbV;
    *theOutCount      = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline surface poles");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int aU = 1; aU <= static_cast<int>(aNbU); ++aU)
    {
      for (int aV = 1; aV <= static_cast<int>(aNbV); ++aV)
      {
        const gp_Pnt& aP                                        = aBs->Pole(aU, aV);
        theOutBuf[(aU - 1) * static_cast<int>(aNbV) + (aV - 1)] = {aP.X(), aP.Y(), aP.Z()};
      }
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_u_knots(const occtl_graph_t* theGraph,
                                                                  occtl_rep_id_t       theSurfaceId,
                                                                  double*              theOutBuf,
                                                                  size_t               theCapacity,
                                                                  size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNb = static_cast<size_t>(aBs->NbUKnots());
    *theOutCount     = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline U knots");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int anI = 1; anI <= static_cast<int>(aNb); ++anI)
    {
      theOutBuf[anI - 1] = aBs->UKnot(anI);
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_v_knots(const occtl_graph_t* theGraph,
                                                                  occtl_rep_id_t       theSurfaceId,
                                                                  double*              theOutBuf,
                                                                  size_t               theCapacity,
                                                                  size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNb = static_cast<size_t>(aBs->NbVKnots());
    *theOutCount     = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline V knots");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int anI = 1; anI <= static_cast<int>(aNb); ++anI)
    {
      theOutBuf[anI - 1] = aBs->VKnot(anI);
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_u_multiplicities(occtl_graph_t* theGraph,
                                         occtl_rep_id_t theSurfaceId,
                                         int32_t*       theOutBuf,
                                         size_t         theCapacity,
                                         size_t*        theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNb = static_cast<size_t>(aBs->NbUKnots());
    *theOutCount     = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline U multiplicities");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int anI = 1; anI <= static_cast<int>(aNb); ++anI)
    {
      theOutBuf[anI - 1] = static_cast<int32_t>(aBs->UMultiplicity(anI));
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_v_multiplicities(occtl_graph_t* theGraph,
                                         occtl_rep_id_t theSurfaceId,
                                         int32_t*       theOutBuf,
                                         size_t         theCapacity,
                                         size_t*        theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNb = static_cast<size_t>(aBs->NbVKnots());
    *theOutCount     = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline V multiplicities");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int anI = 1; anI <= static_cast<int>(aNb); ++anI)
    {
      theOutBuf[anI - 1] = static_cast<int32_t>(aBs->VMultiplicity(anI));
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_bspline_weights(const occtl_graph_t* theGraph,
                                                                  occtl_rep_id_t       theSurfaceId,
                                                                  double*              theOutBuf,
                                                                  size_t               theCapacity,
                                                                  size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    if (!(aBs->IsURational() || aBs->IsVRational()))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "B-spline surface is non-rational; no weights");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNbU = static_cast<size_t>(aBs->NbUPoles());
    const size_t aNbV = static_cast<size_t>(aBs->NbVPoles());
    const size_t aNb  = aNbU * aNbV;
    *theOutCount      = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline surface weights");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int aU = 1; aU <= static_cast<int>(aNbU); ++aU)
    {
      for (int aV = 1; aV <= static_cast<int>(aNbV); ++aV)
      {
        theOutBuf[(aU - 1) * static_cast<int>(aNbV) + (aV - 1)] = aBs->Weight(aU, aV);
      }
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_u_flat_knots(const occtl_graph_t* theGraph,
                                     occtl_rep_id_t       theSurfaceId,
                                     double*              theOutBuf,
                                     size_t               theCapacity,
                                     size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return SurfaceFlatKnotsCommon(theGraph,
                                  theSurfaceId,
                                  true,
                                  theOutBuf,
                                  theCapacity,
                                  theOutCount);
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_v_flat_knots(const occtl_graph_t* theGraph,
                                     occtl_rep_id_t       theSurfaceId,
                                     double*              theOutBuf,
                                     size_t               theCapacity,
                                     size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return SurfaceFlatKnotsCommon(theGraph,
                                  theSurfaceId,
                                  false,
                                  theOutBuf,
                                  theCapacity,
                                  theOutCount);
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_bspline_poles_view(occtl_graph_t*         theGraph,
                                   occtl_rep_id_t         theSurfaceId,
                                   const occtl_point3_t** theOutData,
                                   size_t*                theOutNbU,
                                   size_t*                theOutNbV)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutData == nullptr || theOutNbU == nullptr
        || theOutNbV == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "graph, out_data, out_u_count, out_v_pole_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const NCollection_Array2<gp_Pnt>& aPoles = aBs->Poles();
    if (aBs->NbVPoles() >= 2)
    {
      const gp_Pnt* aP00 = &aPoles.Value(aPoles.LowerRow(), aPoles.LowerCol());
      const gp_Pnt* aP01 = &aPoles.Value(aPoles.LowerRow(), aPoles.LowerCol() + 1);
      if (aP01 - aP00 != 1)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_INTERNAL,
          "OCCT NCollection_Array2 layout is not contiguous row-major; poles_view unsupported");
        return OCCTL_INTERNAL;
      }
    }
    *theOutData =
      reinterpret_cast<const occtl_point3_t*>(&aPoles.Value(aPoles.LowerRow(), aPoles.LowerCol()));
    *theOutNbU = static_cast<size_t>(aBs->NbUPoles());
    *theOutNbV = static_cast<size_t>(aBs->NbVPoles());
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API void OCCTL_CALL occtl_surface_bspline_init(occtl_surface_bspline_t* theOut)
{
  if (theOut == nullptr)
  {
    return;
  }
  theOut->struct_version    = OCCTL_SURFACE_BSPLINE_VERSION_1;
  theOut->p_next            = nullptr;
  theOut->u_degree          = 0;
  theOut->v_degree          = 0;
  theOut->u_pole_count      = 0;
  theOut->v_pole_count      = 0;
  theOut->is_rational       = 0;
  theOut->is_u_periodic     = 0;
  theOut->is_v_periodic     = 0;
  theOut->u_knot_count      = 0;
  theOut->v_knot_count      = 0;
  theOut->u_flat_knot_count = 0;
  theOut->v_flat_knot_count = 0;
  theOut->poles             = nullptr;
  theOut->weights           = nullptr;
  theOut->u_knots           = nullptr;
  theOut->u_multiplicities  = nullptr;
  theOut->u_flat_knots      = nullptr;
  theOut->v_knots           = nullptr;
  theOut->v_multiplicities  = nullptr;
  theOut->v_flat_knots      = nullptr;
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_as_bspline(const occtl_graph_t*     theGraph,
                                                             occtl_rep_id_t           theSurfaceId,
                                                             occtl_surface_bspline_t* theOut)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOut == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOut->struct_version != OCCTL_SURFACE_BSPLINE_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in surface_bspline view");
      return OCCTL_VERSION_MISMATCH;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    const occ::handle<Geom_BSplineSurface> aBs = occ::down_cast<Geom_BSplineSurface>(*aSurface);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    OcctL::Geom::FillBSplineSurfaceView(aBs, *theOut);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_bezier_create_info_init(occtl_surface_bezier_create_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_SURFACE_BEZIER_CREATE_INFO_INIT;
  }
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_bezier(occtl_graph_t*                            theGraph,
                              occtl_rep_id_t*                           theOutId,
                              const occtl_surface_bezier_create_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_BEZIER_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH, "unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->poles == nullptr || theInfo->u_pole_count < 1 || theInfo->v_pole_count < 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "poles is NULL or u_pole_count/v_pole_count < 1");
      return OCCTL_INVALID_ARGUMENT;
    }
    const int                  aNbU = static_cast<int>(theInfo->u_pole_count);
    const int                  aNbV = static_cast<int>(theInfo->v_pole_count);
    NCollection_Array2<gp_Pnt> aPoles(1, aNbU, 1, aNbV);
    for (int aU = 1; aU <= aNbU; ++aU)
    {
      for (int aV = 1; aV <= aNbV; ++aV)
      {
        aPoles.SetValue(aU, aV, OcctL::Geom::ToGp(theInfo->poles[(aU - 1) * aNbV + (aV - 1)]));
      }
    }
    occ::handle<Geom_BezierSurface> aBezier;
    if (theInfo->weights != nullptr)
    {
      NCollection_Array2<double> aWeights(1, aNbU, 1, aNbV);
      for (int aU = 1; aU <= aNbU; ++aU)
      {
        for (int aV = 1; aV <= aNbV; ++aV)
        {
          aWeights.SetValue(aU, aV, theInfo->weights[(aU - 1) * aNbV + (aV - 1)]);
        }
      }
      aBezier = new Geom_BezierSurface(aPoles, aWeights);
    }
    else
    {
      aBezier = new Geom_BezierSurface(aPoles);
    }
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aBezier);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_bezier_grid(occtl_graph_t*                            theGraph,
                                   occtl_rep_id_t*                           theOutId,
                                   const occtl_surface_bezier_create_info_t* theInfo)
{
  return OcctL::Core::Guard(
    [&]() -> occtl_status_t { return occtl_surface_create_bezier(theGraph, theOutId, theInfo); });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_reverse(occtl_graph_t*  theGraph,
                                                          occtl_rep_id_t  theSurfaceId,
                                                          occtl_rep_id_t* theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    occ::handle<Geom_Surface> aReversed = (*aSurface)->UReversed();
    BRepGraph_SurfaceRepId    aRepId    = theGraph->graph.Editor().Reps().CreateSurface(aReversed);
    *theOutId                           = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_transformed(occtl_graph_t*    theGraph,
                                                              occtl_rep_id_t    theSurfaceId,
                                                              occtl_transform_t theTransform,
                                                              occtl_rep_id_t*   theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    gp_Trsf                   aTrsf = OcctL::Geom::ToGpTrsf(theTransform);
    occ::handle<Geom_Surface> aTransformed =
      occ::handle<Geom_Surface>::DownCast((*aSurface)->Transformed(aTrsf));
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aTransformed);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_translated(occtl_graph_t*  theGraph,
                                                             occtl_rep_id_t  theSurfaceId,
                                                             occtl_vector3_t theDelta,
                                                             occtl_rep_id_t* theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    gp_Trsf aTrsf;
    aTrsf.SetTranslation(OcctL::Geom::ToGp(theDelta));
    occ::handle<Geom_Surface> aTranslated =
      occ::handle<Geom_Surface>::DownCast((*aSurface)->Transformed(aTrsf));
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aTranslated);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_rotated(occtl_graph_t*          theGraph,
                                                          occtl_rep_id_t          theSurfaceId,
                                                          occtl_axis1_placement_t theAxis,
                                                          double                  theAngle,
                                                          occtl_rep_id_t*         theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    gp_Trsf aTrsf;
    aTrsf.SetRotation(OcctL::Geom::ToGpAx1(theAxis), theAngle);
    occ::handle<Geom_Surface> aRotated =
      occ::handle<Geom_Surface>::DownCast((*aSurface)->Transformed(aTrsf));
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aRotated);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_scaled(occtl_graph_t*  theGraph,
                                                         occtl_rep_id_t  theSurfaceId,
                                                         occtl_point3_t  theOrigin,
                                                         double          theFactor,
                                                         occtl_rep_id_t* theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_id is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theFactor == 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "scale factor is zero");
      return OCCTL_GEOMETRY_INVALID;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    gp_Trsf aTrsf;
    aTrsf.SetScale(OcctL::Geom::ToGp(theOrigin), theFactor);
    occ::handle<Geom_Surface> aScaled =
      occ::handle<Geom_Surface>::DownCast((*aSurface)->Transformed(aTrsf));
    BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aScaled);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_area(const occtl_graph_t* theGraph,
                                                       occtl_rep_id_t       theSurfaceId,
                                                       double*              theOutArea)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutArea == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "out_area is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    GeomAdaptor_Surface anAdaptor(*aSurface);
    const double        aU1 = anAdaptor.FirstUParameter();
    const double        aU2 = anAdaptor.LastUParameter();
    const double        aV1 = anAdaptor.FirstVParameter();
    const double        aV2 = anAdaptor.LastVParameter();

    if (Precision::IsInfinite(aU1) || Precision::IsInfinite(aU2) || Precision::IsInfinite(aV1)
        || Precision::IsInfinite(aV2))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "surface is unbounded; cannot compute area over an infinite parameter domain");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepBuilderAPI_MakeFace aFaceMaker(*aSurface, aU1, aU2, aV1, aV2, Precision::Confusion());
    if (!aFaceMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "surface cannot be converted to a bounded face");
      return OCCTL_GEOMETRY_INVALID;
    }

    GProp_GProps aProps;
    BRepGProp::SurfaceProperties(aFaceMaker.Face(), aProps);
    *theOutArea = aProps.Mass();
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_project_point(const occtl_graph_t* theGraph,
                                                                occtl_rep_id_t       theSurfaceId,
                                                                occtl_point3_t       thePoint,
                                                                double*              theOutU,
                                                                double*              theOutV,
                                                                double*              theOutDistance)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutU == nullptr || theOutV == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_u, out_v must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_status_t                   aStatus;
    const occ::handle<Geom_Surface>* aSurface = UnpackSurface(theGraph, theSurfaceId, aStatus);
    if (aSurface == nullptr)
    {
      return aStatus;
    }
    GeomAPI_ProjectPointOnSurf aProj(OcctL::Geom::ToGp(thePoint), *aSurface);
    if (aProj.NbPoints() < 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "projection failed");
      return OCCTL_GEOMETRY_INVALID;
    }
    aProj.LowerDistanceParameters(*theOutU, *theOutV);
    if (theOutDistance != nullptr)
    {
      *theOutDistance = aProj.LowerDistance();
    }
    return OCCTL_OK;
  });
}

//================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_uv_of_point(const occtl_graph_t* theGraph,
                                                              occtl_rep_id_t       theSurfaceId,
                                                              occtl_point3_t       thePoint,
                                                              double*              theOutU,
                                                              double*              theOutV)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return occtl_surface_project_point(theGraph, theSurfaceId, thePoint, theOutU, theOutV, nullptr);
  });
}

} // extern "C"
