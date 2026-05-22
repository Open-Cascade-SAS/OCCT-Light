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

#include "GeomMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <GeomAPI_AirfoilNACA4.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <GeomAPI_IntCS.hxx>
#include <GeomAPI_IntSS.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <GeomAPI_PointsToBSplineSurface.hxx>
#include <GeomAbs_Shape.hxx>
#include <GeomConvert.hxx>
#include <GeomConvert_BSplineCurveToBezierCurve.hxx>
#include <GeomFill_BSplineCurves.hxx>
#include <GeomFill_FillingStyle.hxx>
#include <GeomFill_Gordon.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_Array2.hxx>
#include <NCollection_HArray1.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <gp_Pnt.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Precision.hxx>

#include <cmath>
#include <cstdlib>
#include <memory>

#include <TCollection_AsciiString.hxx>

namespace
{

bool IsFinite(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool IsFiniteCurveExportRange(const double theValue)
{
  return IsFinite(theValue) && std::abs(theValue) < 1.0e90;
}

bool ToFillingStyle(const occtl_surface_filling_style_t theStyle,
                    GeomFill_FillingStyle&              theOutStyle)
{
  switch (theStyle)
  {
    case OCCTL_SURFACE_FILLING_STRETCH:
      theOutStyle = GeomFill_StretchStyle;
      return true;
    case OCCTL_SURFACE_FILLING_COONS:
      theOutStyle = GeomFill_CoonsStyle;
      return true;
    case OCCTL_SURFACE_FILLING_CURVED:
      theOutStyle = GeomFill_CurvedStyle;
      return true;
    default:
      return false;
  }
}

occtl_status_t BuildGordonSurface(occtl_graph_t* const        theGraph,
                                  occtl_rep_id_t* const       theOutId,
                                  const occtl_rep_id_t* const theProfiles,
                                  const size_t                theProfileCount,
                                  const char* const           theProfileLabel,
                                  const occtl_rep_id_t* const theGuides,
                                  const size_t                theGuideCount,
                                  const char* const           theGuideLabel,
                                  const double                theTolerance,
                                  const int32_t               theParallel)
{
  if (theOutId == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }

  if (theProfiles == nullptr || theGuides == nullptr || theProfileCount < 2 || theGuideCount < 2
      || theTolerance <= 0.0 || (theParallel != 0 && theParallel != 1))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "Gordon surface info has invalid arrays or tolerance");
    return OCCTL_INVALID_ARGUMENT;
  }

  const int                                   aProfileCount = static_cast<int>(theProfileCount);
  const int                                   aGuideCount   = static_cast<int>(theGuideCount);
  NCollection_Array1<occ::handle<Geom_Curve>> aProfiles(1, aProfileCount);
  NCollection_Array1<occ::handle<Geom_Curve>> aGuides(1, aGuideCount);

  for (int anI = 1; anI <= aProfileCount; ++anI)
  {
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theProfiles[anI - 1]);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        static_cast<std::string_view>(TCollection_AsciiString(theProfileLabel)
                                      + " curve rep id is invalid"));
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_WRONG_KIND,
        static_cast<std::string_view>(TCollection_AsciiString(theProfileLabel)
                                      + " rep id is not a Curve3D"));
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurve3DId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;
    aProfiles.SetValue(anI, aCurve);
  }
  for (int anI = 1; anI <= aGuideCount; ++anI)
  {
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theGuides[anI - 1]);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        static_cast<std::string_view>(TCollection_AsciiString(theGuideLabel)
                                      + " curve rep id is invalid"));
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_WRONG_KIND,
        static_cast<std::string_view>(TCollection_AsciiString(theGuideLabel)
                                      + " rep id is not a Curve3D"));
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurve3DId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;
    aGuides.SetValue(anI, aCurve);
  }

  GeomFill_Gordon aGordon;
  try
  {
    OCC_CATCH_SIGNALS;
    aGordon.Init(aProfiles, aGuides, theTolerance);
    aGordon.SetParallelMode(theParallel != 0);
    aGordon.Perform();
  }
  catch (const Standard_Failure& anError)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_GEOMETRY_INVALID,
      static_cast<std::string_view>(TCollection_AsciiString("GeomFill_Gordon failed: ")
                                    + anError.what()));
    return OCCTL_GEOMETRY_INVALID;
  }

  if (!aGordon.IsDone() || aGordon.Surface().IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "GeomFill_Gordon did not produce a surface");
    return OCCTL_GEOMETRY_INVALID;
  }

  BRepGraph_SurfaceRepId aRepId = theGraph->graph.Editor().Reps().CreateSurface(aGordon.Surface());
  *theOutId                     = OcctL::Topo::PackRepId(aRepId);
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve_airfoil_naca4_info_init(occtl_curve_airfoil_naca4_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  *theInfo = OCCTL_CURVE_AIRFOIL_NACA4_INFO_INIT;
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_airfoil_naca4(occtl_graph_t*                          theGraph,
                                   const occtl_curve_airfoil_naca4_info_t* theInfo,
                                   occtl_rep_id_t*                         theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, and out_id must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_CURVE_AIRFOIL_NACA4_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in airfoil info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFinite(theInfo->max_camber) || !IsFinite(theInfo->camber_position)
        || !IsFinite(theInfo->thickness) || !IsFinite(theInfo->chord_length)
        || !IsFinite(theInfo->tolerance))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "airfoil numeric parameters must be finite");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->max_camber < 0.0 || theInfo->camber_position < 0.0
        || theInfo->camber_position > 1.0 || theInfo->thickness <= 0.0
        || theInfo->chord_length <= 0.0 || theInfo->point_count < 5 || theInfo->degree_min < 1
        || theInfo->degree_max < theInfo->degree_min || theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "invalid airfoil parameter range");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->finite_trailing_edge != 0 && theInfo->finite_trailing_edge != 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "finite_trailing_edge must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      GeomAPI_AirfoilNACA4 anAirfoil(theInfo->max_camber,
                                     theInfo->camber_position,
                                     theInfo->thickness,
                                     theInfo->chord_length,
                                     static_cast<int>(theInfo->point_count),
                                     theInfo->finite_trailing_edge != 0,
                                     static_cast<int>(theInfo->degree_min),
                                     static_cast<int>(theInfo->degree_max),
                                     theInfo->tolerance);
      if (!anAirfoil.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "GeomAPI_AirfoilNACA4 failed");
        return OCCTL_GEOMETRY_INVALID;
      }

      BRepGraph_Curve3DRepId aRepId =
        theGraph->graph.Editor().Reps().CreateCurve3D(anAirfoil.Curve());
      *theOutId = OcctL::Topo::PackRepId(aRepId);
      return OCCTL_OK;
    }
    catch (const Standard_Failure& anErr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, anErr.what());
      return OCCTL_GEOMETRY_INVALID;
    }
  });
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve_interpolated_info_init(occtl_curve_interpolated_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  *theInfo = OCCTL_CURVE_INTERPOLATED_INFO_INIT;
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_interpolated(occtl_graph_t*                         theGraph,
                                  const occtl_curve_interpolated_info_t* theInfo,
                                  occtl_rep_id_t*                        theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_CURVE_INTERPOLATED_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in interpolate info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->points == nullptr || theInfo->point_count < 2)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "at least 2 points are required for interpolation");
      return OCCTL_GEOMETRY_INVALID;
    }
    const int aNbPts = static_cast<int>(theInfo->point_count);

    occ::handle<NCollection_HArray1<gp_Pnt>> aHPts = new NCollection_HArray1<gp_Pnt>(1, aNbPts);
    for (int anI = 1; anI <= aNbPts; ++anI)
    {
      const occtl_point3_t& aP = theInfo->points[anI - 1];
      aHPts->SetValue(anI, OcctL::Geom::ToGp(aP));
    }

    const bool aPeriodic = (theInfo->is_periodic != 0);

    GeomAPI_Interpolate anInterp(aHPts, aPeriodic, theInfo->tolerance);
    anInterp.Perform();
    if (!anInterp.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "curve interpolation failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(anInterp.Curve());
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve_approximated_info_init(occtl_curve_approximated_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  *theInfo = OCCTL_CURVE_APPROXIMATED_INFO_INIT;
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_approximated(occtl_graph_t*                         theGraph,
                                  const occtl_curve_approximated_info_t* theInfo,
                                  occtl_rep_id_t*                        theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_CURVE_APPROXIMATED_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in approximate info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->points == nullptr || theInfo->point_count < 2)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "at least 2 points are required for approximation");
      return OCCTL_GEOMETRY_INVALID;
    }
    if (theInfo->degree_min < 1 || theInfo->degree_max < theInfo->degree_min)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "degree_min must be >= 1 and <= degree_max");
      return OCCTL_GEOMETRY_INVALID;
    }

    const int aNbPts = static_cast<int>(theInfo->point_count);

    NCollection_Array1<gp_Pnt> aPts(1, aNbPts);
    for (int anI = 1; anI <= aNbPts; ++anI)
    {
      const occtl_point3_t& aP = theInfo->points[anI - 1];
      aPts.SetValue(anI, OcctL::Geom::ToGp(aP));
    }

    GeomAPI_PointsToBSpline anApprox(aPts,
                                     static_cast<int>(theInfo->degree_min),
                                     static_cast<int>(theInfo->degree_max),
                                     GeomAbs_C2,
                                     theInfo->tolerance);
    if (!anApprox.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "curve approximation failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(anApprox.Curve());
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve_bezier_segments_options_init(occtl_curve_bezier_segments_options_t* theOptions)
{
  if (theOptions == nullptr)
  {
    return;
  }
  *theOptions = OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_INIT;
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_to_bezier_segments(occtl_graph_t*                               theGraph,
                                 const occtl_rep_id_t                         theCurveId,
                                 const occtl_curve_bezier_segments_options_t* theOptions,
                                 occtl_rep_id_t**                             theOutIds,
                                 size_t*                                      theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIds == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_ids, and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve3D");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurve3DId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;

    *theOutIds   = nullptr;
    *theOutCount = 0;

    occtl_curve_bezier_segments_options_t anOptions = OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_INIT;
    if (theOptions != nullptr)
    {
      if (theOptions->struct_version != OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_VERSION_1)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_VERSION_MISMATCH,
          "unsupported struct_version in Bezier segment options");
        return OCCTL_VERSION_MISMATCH;
      }
      anOptions = *theOptions;
    }

    if (anOptions.p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (anOptions.use_range != 0 && anOptions.use_range != 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "use_range must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFinite(anOptions.parametric_tolerance) || anOptions.parametric_tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "parametric_tolerance must be finite and non-negative");
      return OCCTL_INVALID_ARGUMENT;
    }

    const double aTol   = anOptions.parametric_tolerance > 0.0 ? anOptions.parametric_tolerance
                                                               : Precision::PConfusion();
    double       aFirst = 0.0;
    double       aLast  = 0.0;
    if (anOptions.use_range != 0)
    {
      if (!IsFinite(anOptions.u_first) || !IsFinite(anOptions.u_last)
          || std::abs(anOptions.u_last - anOptions.u_first) <= aTol)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_INVALID_ARGUMENT,
          "Bezier segment range must be finite and non-degenerate");
        return OCCTL_INVALID_ARGUMENT;
      }
      aFirst = anOptions.u_first;
      aLast  = anOptions.u_last;
    }
    else
    {
      aFirst = aCurve->FirstParameter();
      aLast  = aCurve->LastParameter();
      if (!IsFiniteCurveExportRange(aFirst) || !IsFiniteCurveExportRange(aLast)
          || std::abs(aLast - aFirst) <= aTol)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_INVALID_ARGUMENT,
          "unbounded or degenerate curves require an explicit finite range");
        return OCCTL_INVALID_ARGUMENT;
      }
    }

    try
    {
      OCC_CATCH_SIGNALS;
      occ::handle<Geom_Curve> aWorkCurve = aCurve;
      if (anOptions.use_range != 0)
      {
        aWorkCurve = new Geom_TrimmedCurve(aCurve, aFirst, aLast);
      }

      const occ::handle<Geom_BSplineCurve> aBSpline = GeomConvert::CurveToBSplineCurve(aWorkCurve);
      if (aBSpline.IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "OCCT could not convert curve to B-spline");
        return OCCTL_GEOMETRY_INVALID;
      }

      GeomConvert_BSplineCurveToBezierCurve aConverter(aBSpline);
      const int                             aNb = aConverter.NbArcs();
      if (aNb <= 0)
      {
        *theOutCount = 0;
        return OCCTL_OK;
      }
      const size_t aNbArcs = static_cast<size_t>(aNb);
      *theOutCount         = aNbArcs;

      occtl_rep_id_t* anIds =
        static_cast<occtl_rep_id_t*>(std::malloc(aNbArcs * sizeof(occtl_rep_id_t)));
      if (anIds == nullptr)
      {
        *theOutCount = 0;
        OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_MEMORY, "malloc failed");
        return OCCTL_OUT_OF_MEMORY;
      }

      for (int anI = 1; anI <= aNb; ++anI)
      {
        const occ::handle<Geom_BezierCurve> aBezier = aConverter.Arc(anI);
        BRepGraph_Curve3DRepId aSegId = theGraph->graph.Editor().Reps().CreateCurve3D(aBezier);
        anIds[anI - 1]                = OcctL::Topo::PackRepId(aSegId);
      }

      *theOutIds = anIds;

      return OCCTL_OK;
    }
    catch (const Standard_Failure& anError)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        static_cast<std::string_view>(TCollection_AsciiString("Bezier segment conversion failed: ")
                                      + anError.what()));
      *theOutCount = 0;
      return OCCTL_GEOMETRY_INVALID;
    }
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_intersect(occtl_graph_t*                           theGraph,
                        occtl_rep_id_t                           theCurveIdA,
                        occtl_rep_id_t                           theCurveIdB,
                        const occtl_curve_intersection_point_t** theOutResults,
                        size_t*                                  theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutResults == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_results, and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RepId aRawA = OcctL::Topo::UnpackRepId(theCurveIdA);
    if (!aRawA.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve_a rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawA.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve_a rep id is not a Curve3D");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurve3DIdA(static_cast<uint32_t>(aRawA.Index));
    const occ::handle<Geom_Curve>& aCurveA =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurve3DIdA).Curve;

    const BRepGraph_RepId aRawB = OcctL::Topo::UnpackRepId(theCurveIdB);
    if (!aRawB.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve_b rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawB.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve_b rep id is not a Curve3D");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurve3DIdB(static_cast<uint32_t>(aRawB.Index));
    const occ::handle<Geom_Curve>& aCurveB =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurve3DIdB).Curve;

    GeomAPI_ExtremaCurveCurve anExtrema(aCurveA, aCurveB);
    const int                 aNb = anExtrema.NbExtrema();

    if (aNb == 0)
    {
      *theOutResults = nullptr;
      *theOutCount   = 0;
      return OCCTL_OK;
    }

    occtl_curve_intersection_point_t* aResults = static_cast<occtl_curve_intersection_point_t*>(
      std::malloc(static_cast<size_t>(aNb) * sizeof(occtl_curve_intersection_point_t)));
    if (aResults == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_MEMORY,
                                             "failed to allocate intersection point array");
      return OCCTL_OUT_OF_MEMORY;
    }

    for (int anI = 1; anI <= aNb; ++anI)
    {
      gp_Pnt aP1, aP2;
      double aU1 = 0.0, aU2 = 0.0;
      anExtrema.Points(anI, aP1, aP2);
      anExtrema.Parameters(anI, aU1, aU2);
      const double aMidX        = 0.5 * (aP1.X() + aP2.X());
      const double aMidY        = 0.5 * (aP1.Y() + aP2.Y());
      const double aMidZ        = 0.5 * (aP1.Z() + aP2.Z());
      aResults[anI - 1].point   = {aMidX, aMidY, aMidZ};
      aResults[anI - 1].param_a = aU1;
      aResults[anI - 1].param_b = aU2;
    }

    *theOutResults = aResults;
    *theOutCount   = static_cast<size_t>(aNb);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_interpolated_info_init(occtl_surface_interpolated_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  *theInfo = OCCTL_SURFACE_INTERPOLATED_INFO_INIT;
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_interpolated(occtl_graph_t*                           theGraph,
                                    const occtl_surface_interpolated_info_t* theInfo,
                                    occtl_rep_id_t*                          theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_INTERPOLATED_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "unsupported struct_version in surface interpolate info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "surface interpolate info p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->points == nullptr || theInfo->u_point_count < 2 || theInfo->v_point_count < 2)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "at least 2 x 2 points are required for surface interpolation");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->is_u_periodic != 0 && theInfo->is_u_periodic != 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "is_u_periodic must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }
    const int aNbU = static_cast<int>(theInfo->u_point_count);
    const int aNbV = static_cast<int>(theInfo->v_point_count);

    NCollection_Array2<gp_Pnt> aPts(1, aNbU, 1, aNbV);
    for (int aU = 1; aU <= aNbU; ++aU)
    {
      for (int aV = 1; aV <= aNbV; ++aV)
      {
        const occtl_point3_t& aP = theInfo->points[(aU - 1) * aNbV + (aV - 1)];
        aPts.SetValue(aU, aV, OcctL::Geom::ToGp(aP));
      }
    }

    const bool aPeriodic = (theInfo->is_u_periodic != 0);

    GeomAPI_PointsToBSplineSurface aInterp;
    aInterp.Interpolate(aPts, aPeriodic);
    if (!aInterp.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "surface interpolation failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepGraph_SurfaceRepId aRepId =
      theGraph->graph.Editor().Reps().CreateSurface(aInterp.Surface());
    *theOutId = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_approximated_info_init(occtl_surface_approximated_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  *theInfo = OCCTL_SURFACE_APPROXIMATED_INFO_INIT;
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_approximated(occtl_graph_t*                           theGraph,
                                    const occtl_surface_approximated_info_t* theInfo,
                                    occtl_rep_id_t*                          theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_APPROXIMATED_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "unsupported struct_version in surface approximate info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "surface approximate info p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->points == nullptr || theInfo->u_point_count < 2 || theInfo->v_point_count < 2)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "at least 2 x 2 points are required for surface approximation");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->degree_min < 1 || theInfo->degree_max < theInfo->degree_min)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "degree ranges must satisfy 1 <= min <= max");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "surface approximation requires non-negative tolerance");
      return OCCTL_INVALID_ARGUMENT;
    }

    const int aNbU = static_cast<int>(theInfo->u_point_count);
    const int aNbV = static_cast<int>(theInfo->v_point_count);

    NCollection_Array2<gp_Pnt> aPts(1, aNbU, 1, aNbV);
    for (int aU = 1; aU <= aNbU; ++aU)
    {
      for (int aV = 1; aV <= aNbV; ++aV)
      {
        const occtl_point3_t& aP = theInfo->points[(aU - 1) * aNbV + (aV - 1)];
        aPts.SetValue(aU, aV, OcctL::Geom::ToGp(aP));
      }
    }

    GeomAPI_PointsToBSplineSurface anApprox(aPts,
                                            static_cast<int>(theInfo->degree_min),
                                            static_cast<int>(theInfo->degree_max),
                                            GeomAbs_C2,
                                            theInfo->tolerance);
    if (!anApprox.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "surface approximation failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepGraph_SurfaceRepId aRepId =
      theGraph->graph.Editor().Reps().CreateSurface(anApprox.Surface());
    *theOutId = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_point_grid_create_info_init(occtl_surface_point_grid_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  *theInfo = OCCTL_SURFACE_POINT_GRID_CREATE_INFO_INIT;
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_from_point_grid(occtl_graph_t*                                theGraph,
                                       occtl_rep_id_t*                               theOutId,
                                       const occtl_surface_point_grid_create_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_POINT_GRID_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "unsupported struct_version in point-grid surface info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "point-grid surface info p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->mode != OCCTL_SURFACE_POINT_GRID_MODE_APPROXIMATE
        && theInfo->mode != OCCTL_SURFACE_POINT_GRID_MODE_INTERPOLATE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "point-grid surface mode is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->is_u_periodic != 0 && theInfo->is_u_periodic != 1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "point-grid periodic flag is_u_periodic must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->mode == OCCTL_SURFACE_POINT_GRID_MODE_INTERPOLATE)
    {
      occtl_surface_interpolated_info_t anInterp = OCCTL_SURFACE_INTERPOLATED_INFO_INIT;
      anInterp.points                            = theInfo->points;
      anInterp.u_point_count                     = theInfo->u_point_count;
      anInterp.v_point_count                     = theInfo->v_point_count;
      anInterp.is_u_periodic                     = theInfo->is_u_periodic;
      return occtl_surface_create_interpolated(theGraph, &anInterp, theOutId);
    }

    occtl_surface_approximated_info_t anApprox = OCCTL_SURFACE_APPROXIMATED_INFO_INIT;
    anApprox.points                            = theInfo->points;
    anApprox.u_point_count                     = theInfo->u_point_count;
    anApprox.v_point_count                     = theInfo->v_point_count;
    anApprox.degree_min                        = theInfo->degree_min;
    anApprox.degree_max                        = theInfo->degree_max;
    anApprox.tolerance                         = theInfo->tolerance;
    return occtl_surface_create_approximated(theGraph, &anApprox, theOutId);
  });
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_gordon_create_info_init(occtl_surface_gordon_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  *theInfo = OCCTL_SURFACE_GORDON_CREATE_INFO_INIT;
}

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_surface_boundary_curves_create_info_init(
  occtl_surface_boundary_curves_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  *theInfo = OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_INIT;
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_create_from_boundary_curves(
  occtl_graph_t*                                     theGraph,
  occtl_rep_id_t*                                    theOutId,
  const occtl_surface_boundary_curves_create_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "unsupported struct_version in boundary-curves surface info");
      return OCCTL_VERSION_MISMATCH;
    }

    GeomFill_FillingStyle aStyle = GeomFill_CoonsStyle;
    if (theInfo->p_next != nullptr || theInfo->curves == nullptr || theInfo->curve_count < 2
        || theInfo->curve_count > 4 || !ToFillingStyle(theInfo->style, aStyle))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "boundary-curves surface info has invalid arrays or style");
      return OCCTL_INVALID_ARGUMENT;
    }

    occ::handle<Geom_BSplineCurve> aCurves[4];
    for (size_t anIndex = 0; anIndex < theInfo->curve_count; ++anIndex)
    {
      const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theInfo->curves[anIndex]);
      if (!aRawId.IsValid())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "boundary curve rep id is invalid");
        return OCCTL_NOT_FOUND;
      }
      if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                               "boundary curve rep id is not a Curve3D");
        return OCCTL_WRONG_KIND;
      }
      BRepGraph_Curve3DRepId         aCurve3DId(static_cast<uint32_t>(aRawId.Index));
      const occ::handle<Geom_Curve>& aCurve =
        theGraph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;

      try
      {
        OCC_CATCH_SIGNALS;
        aCurves[anIndex] = GeomConvert::CurveToBSplineCurve(aCurve, Convert_RationalC1);
      }
      catch (const Standard_Failure& anErr)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, anErr.what());
        return OCCTL_GEOMETRY_INVALID;
      }
      if (aCurves[anIndex].IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "OCCT could not convert boundary curve to B-spline");
        return OCCTL_GEOMETRY_INVALID;
      }
    }

    try
    {
      OCC_CATCH_SIGNALS;
      GeomFill_BSplineCurves aFill;
      if (theInfo->curve_count == 2)
      {
        aFill.Init(aCurves[0], aCurves[1], aStyle);
      }
      else if (theInfo->curve_count == 3)
      {
        aFill.Init(aCurves[0], aCurves[1], aCurves[2], aStyle);
      }
      else
      {
        aFill.Init(aCurves[0], aCurves[1], aCurves[2], aCurves[3], aStyle);
      }

      if (aFill.Surface().IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "boundary-curve filling failed");
        return OCCTL_GEOMETRY_INVALID;
      }
      BRepGraph_SurfaceRepId aRepId =
        theGraph->graph.Editor().Reps().CreateSurface(aFill.Surface());
      *theOutId = OcctL::Topo::PackRepId(aRepId);
      return OCCTL_OK;
    }
    catch (const Standard_Failure& anErr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, anErr.what());
      return OCCTL_GEOMETRY_INVALID;
    }
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_gordon(occtl_graph_t* const                            theGraph,
                              occtl_rep_id_t* const                           theOutId,
                              const occtl_surface_gordon_create_info_t* const theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_GORDON_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in Gordon surface info");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theInfo->p_next != nullptr || theInfo->profiles == nullptr || theInfo->guides == nullptr
        || theInfo->profile_count < 2 || theInfo->guide_count < 2 || theInfo->tolerance <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "Gordon surface info has invalid arrays or tolerance");
      return OCCTL_INVALID_ARGUMENT;
    }

    return BuildGordonSurface(theGraph,
                              theOutId,
                              theInfo->profiles,
                              theInfo->profile_count,
                              "profile",
                              theInfo->guides,
                              theInfo->guide_count,
                              "guide",
                              theInfo->tolerance,
                              theInfo->parallel);
  });
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_surface_curve_grid_create_info_init(occtl_surface_curve_grid_create_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_INIT;
  }
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_surface_create_from_curve_grid(occtl_graph_t*                                theGraph,
                                       occtl_rep_id_t*                               theOutId,
                                       const occtl_surface_curve_grid_create_info_t* theInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_id, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "unsupported struct_version in curve-grid surface info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "curve-grid surface info p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    return BuildGordonSurface(theGraph,
                              theOutId,
                              theInfo->v_curves,
                              theInfo->v_curve_count,
                              "v",
                              theInfo->u_curves,
                              theInfo->u_curve_count,
                              "u",
                              theInfo->tolerance,
                              theInfo->parallel);
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_intersect_curve(const occtl_graph_t* theGraph,
                                                                  occtl_rep_id_t       theSurfaceId,
                                                                  occtl_rep_id_t       theCurveId,
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

    const BRepGraph_RepId aRawCurve = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawCurve.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawCurve.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve rep id is not a Curve3D");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurve3DId(static_cast<uint32_t>(aRawCurve.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;

    const BRepGraph_RepId aRawSurf = OcctL::Topo::UnpackRepId(theSurfaceId);
    if (!aRawSurf.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawSurf.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface rep id is not a Surface");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_SurfaceRepId           aSurfId(static_cast<uint32_t>(aRawSurf.Index));
    const occ::handle<Geom_Surface>& aSurface =
      theGraph->graph.Topo().Geometry().SurfaceRep(aSurfId).Surface;

    GeomAPI_IntCS anIntersector(aCurve, aSurface);
    if (!anIntersector.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "curve-surface intersection computation failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    const int aNb = anIntersector.NbPoints();
    *theOutCount  = static_cast<size_t>(aNb);

    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }

    if (theCapacity < static_cast<size_t>(aNb))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_BUFFER_TOO_SMALL,
        "buffer too small for curve-surface intersection points");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    for (int anI = 1; anI <= aNb; ++anI)
    {
      const gp_Pnt& aP   = anIntersector.Point(anI);
      theOutBuf[anI - 1] = {aP.X(), aP.Y(), aP.Z()};
    }

    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_surface_intersect(occtl_graph_t*   theGraph,
                                                                    occtl_rep_id_t   theSurfaceA,
                                                                    occtl_rep_id_t   theSurfaceB,
                                                                    double           theTolerance,
                                                                    occtl_rep_id_t** theOutIds,
                                                                    size_t*          theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIds == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_ids, and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_RepId aRawA = OcctL::Topo::UnpackRepId(theSurfaceA);
    if (!aRawA.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface_a rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawA.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface_a rep id is not a Surface");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_SurfaceRepId           aSurfAId(static_cast<uint32_t>(aRawA.Index));
    const occ::handle<Geom_Surface>& aSurfaceA =
      theGraph->graph.Topo().Geometry().SurfaceRep(aSurfAId).Surface;

    const BRepGraph_RepId aRawB = OcctL::Topo::UnpackRepId(theSurfaceB);
    if (!aRawB.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface_b rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawB.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "surface_b rep id is not a Surface");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_SurfaceRepId           aSurfBId(static_cast<uint32_t>(aRawB.Index));
    const occ::handle<Geom_Surface>& aSurfaceB =
      theGraph->graph.Topo().Geometry().SurfaceRep(aSurfBId).Surface;

    GeomAPI_IntSS anIntersector(aSurfaceA, aSurfaceB, theTolerance);
    if (!anIntersector.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "surface-surface intersection computation failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    const int    aNb      = anIntersector.NbLines();
    const size_t aNbLines = static_cast<size_t>(aNb);
    *theOutCount          = aNbLines;

    if (aNb == 0)
    {
      *theOutIds = nullptr;
      return OCCTL_OK;
    }

    occtl_rep_id_t* anIds =
      static_cast<occtl_rep_id_t*>(std::malloc(aNbLines * sizeof(occtl_rep_id_t)));
    if (anIds == nullptr)
    {
      *theOutCount = 0;
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_MEMORY, "malloc failed");
      return OCCTL_OUT_OF_MEMORY;
    }

    for (int anI = 1; anI <= aNb; ++anI)
    {
      const occ::handle<Geom_Curve>& aLine = anIntersector.Line(anI);
      BRepGraph_Curve3DRepId aCurveId      = theGraph->graph.Editor().Reps().CreateCurve3D(aLine);
      anIds[anI - 1]                       = OcctL::Topo::PackRepId(aCurveId);
    }

    *theOutIds = anIds;

    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_arc_of_circle_3pt(occtl_graph_t*  theGraph,
                                                                         occtl_point3_t  theP1,
                                                                         occtl_point3_t  theP2,
                                                                         occtl_point3_t  theP3,
                                                                         occtl_rep_id_t* theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_id must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const gp_Pnt aP1(theP1.x, theP1.y, theP1.z);
    const gp_Pnt aP2(theP2.x, theP2.y, theP2.z);
    const gp_Pnt aP3(theP3.x, theP3.y, theP3.z);

    GC_MakeArcOfCircle aMaker(aP1, aP2, aP3);
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "GC_MakeArcOfCircle failed");
      return OCCTL_GEOMETRY_INVALID;
    }

    const Handle(Geom_TrimmedCurve) aTrimmed = aMaker.Value();
    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aTrimmed);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_curve_free_bezier_segments(occtl_rep_id_t* theIds)
{
  std::free(theIds);
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve_free_intersection_points(occtl_curve_intersection_point_t* theResults)
{
  std::free(theResults);
}

//=================================================================================================

} // extern "C"
