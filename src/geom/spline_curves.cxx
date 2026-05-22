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
#include "GeomMath.hxx"
#include "RepLookup.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <NCollection_Array1.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>

namespace OcctL::Geom
{

//============================================================================

//! Fills an aggregate view from the internal arrays of a Geom_BSplineCurve.
//! Caller is responsible for the WRONG_KIND / VERSION_MISMATCH / NULL guards;
//! this helper assumes both inputs are valid and the curve handle is non-null.
inline void FillBSplineCurveView(const occ::handle<Geom_BSplineCurve>& theBspl,
                                 occtl_curve_bspline_t&                theOut)
{
  theOut.degree      = theBspl->Degree();
  theOut.is_rational = theBspl->IsRational() ? 1 : 0;
  theOut.is_periodic = theBspl->IsPeriodic() ? 1 : 0;
  theOut.is_closed   = theBspl->IsClosed() ? 1 : 0;
  theOut.continuity  = static_cast<int32_t>(static_cast<int>(theBspl->Continuity()));

  const NCollection_Array1<gp_Pnt>& aPoles    = theBspl->Poles();
  const NCollection_Array1<double>& aKnots    = theBspl->Knots();
  const NCollection_Array1<int>&    aMults    = theBspl->Multiplicities();
  const NCollection_Array1<double>& aFlatKnts = theBspl->KnotSequence();

  theOut.pole_count      = static_cast<size_t>(aPoles.Size());
  theOut.knot_count      = static_cast<size_t>(aKnots.Size());
  theOut.flat_knot_count = static_cast<size_t>(aFlatKnts.Size());

  theOut.poles      = reinterpret_cast<const occtl_point3_t*>(&aPoles.First());
  theOut.knots      = &aKnots.First();
  theOut.flat_knots = &aFlatKnts.First();

  // NCollection_Array1<int> stores int contiguously; int and int32_t share
  // size/alignment on every platform we target, so a reinterpret_cast
  // preserves layout and value.
  static_assert(sizeof(int) == sizeof(int32_t),
                "int and int32_t must have identical width for multiplicities aliasing");
  theOut.multiplicities = reinterpret_cast<const int32_t*>(&aMults.First());

  // Geom_BSplineCurve::Weights() returns NULL when the curve is non-rational.
  const NCollection_Array1<double>* aWeights = theBspl->Weights();
  theOut.weights                             = (aWeights != nullptr) ? &aWeights->First() : nullptr;
}

} // namespace OcctL::Geom

extern "C"
{

//============================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve_bspline_create_info_init(occtl_curve_bspline_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  theInfo->struct_version = OCCTL_CURVE_BSPLINE_CREATE_INFO_VERSION_1;
  theInfo->p_next         = nullptr;
  theInfo->poles          = nullptr;
  theInfo->pole_count     = 0;
  theInfo->weights        = nullptr;
  theInfo->knots          = nullptr;
  theInfo->multiplicities = nullptr;
  theInfo->knot_count     = 0;
  theInfo->degree         = 0;
  theInfo->is_periodic    = 0;
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_bspline(occtl_graph_t*                           theGraph,
                             const occtl_curve_bspline_create_info_t* theInfo,
                             occtl_rep_id_t*                          theOutCurve)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCurve == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_curve, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_CURVE_BSPLINE_CREATE_INFO_VERSION_1)
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
    if (theInfo->poles == nullptr || theInfo->knots == nullptr
        || theInfo->multiplicities == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "poles, knots, and multiplicities must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->pole_count == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "pole_count must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    if (theInfo->knot_count == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "knot_count must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    if (theInfo->degree < 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "degree must be >= 1");
      return OCCTL_GEOMETRY_INVALID;
    }
    if (static_cast<int>(theInfo->degree) > static_cast<int>(theInfo->pole_count) - 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "degree must be <= pole_count - 1");
      return OCCTL_GEOMETRY_INVALID;
    }
    if (theInfo->is_periodic != 0 && theInfo->is_periodic != 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "is_periodic must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_Array1<gp_Pnt> aPoles(1, static_cast<int>(theInfo->pole_count));
    for (int anI = 1; anI <= static_cast<int>(theInfo->pole_count); ++anI)
    {
      const occtl_point3_t& aP = theInfo->poles[anI - 1];
      aPoles.SetValue(anI, OcctL::Geom::ToGp(aP));
    }

    NCollection_Array1<double> aKnots(1, static_cast<int>(theInfo->knot_count));
    NCollection_Array1<int>    aMults(1, static_cast<int>(theInfo->knot_count));
    for (int anI = 1; anI <= static_cast<int>(theInfo->knot_count); ++anI)
    {
      aKnots.SetValue(anI, theInfo->knots[anI - 1]);
      aMults.SetValue(anI, theInfo->multiplicities[anI - 1]);
    }

    occ::handle<Geom_BSplineCurve> aBspline;
    if (theInfo->weights != nullptr)
    {
      NCollection_Array1<double> aWeights(1, static_cast<int>(theInfo->pole_count));
      for (int anI = 1; anI <= static_cast<int>(theInfo->pole_count); ++anI)
      {
        aWeights.SetValue(anI, theInfo->weights[anI - 1]);
      }
      aBspline = new Geom_BSplineCurve(aPoles,
                                       aWeights,
                                       aKnots,
                                       aMults,
                                       theInfo->degree,
                                       theInfo->is_periodic != 0);
    }
    else
    {
      aBspline =
        new Geom_BSplineCurve(aPoles, aKnots, aMults, theInfo->degree, theInfo->is_periodic != 0);
    }

    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aBspline);
    *theOutCurve                  = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve_bezier_create_info_init(occtl_curve_bezier_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  theInfo->struct_version = OCCTL_CURVE_BEZIER_CREATE_INFO_VERSION_1;
  theInfo->p_next         = nullptr;
  theInfo->poles          = nullptr;
  theInfo->pole_count     = 0;
  theInfo->weights        = nullptr;
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_bezier(occtl_graph_t*                          theGraph,
                            const occtl_curve_bezier_create_info_t* theInfo,
                            occtl_rep_id_t*                         theOutCurve)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCurve == nullptr || theInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_curve, and info must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_CURVE_BEZIER_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in bezier create_info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->poles == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "poles must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->pole_count < 2)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "pole_count must be >= 2");
      return OCCTL_GEOMETRY_INVALID;
    }

    NCollection_Array1<gp_Pnt> aPoles(1, static_cast<int>(theInfo->pole_count));
    for (int anI = 1; anI <= static_cast<int>(theInfo->pole_count); ++anI)
    {
      const occtl_point3_t& aP = theInfo->poles[anI - 1];
      aPoles.SetValue(anI, OcctL::Geom::ToGp(aP));
    }

    occ::handle<Geom_BezierCurve> aBezier;
    if (theInfo->weights != nullptr)
    {
      NCollection_Array1<double> aWeights(1, static_cast<int>(theInfo->pole_count));
      for (int anI = 1; anI <= static_cast<int>(theInfo->pole_count); ++anI)
      {
        aWeights.SetValue(anI, theInfo->weights[anI - 1]);
      }
      aBezier = new Geom_BezierCurve(aPoles, aWeights);
    }
    else
    {
      aBezier = new Geom_BezierCurve(aPoles);
    }

    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aBezier);
    *theOutCurve                  = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve_trimmed_create_info_init(occtl_curve_trimmed_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  theInfo->struct_version = OCCTL_CURVE_TRIMMED_CREATE_INFO_VERSION_1;
  theInfo->p_next         = nullptr;
  theInfo->basis.bits     = 0;
  theInfo->u_first        = 0.0;
  theInfo->u_last         = 1.0;
  theInfo->sense          = 1;
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_trimmed(occtl_graph_t*                           theGraph,
                             const occtl_curve_trimmed_create_info_t* theInfo,
                             occtl_rep_id_t*                          theOutCurve)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCurve == nullptr || theInfo == nullptr
        || theInfo->basis.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_curve, info, and basis must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_CURVE_TRIMMED_CREATE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in trimmed create_info");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->u_last <= theInfo->u_first)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "trimmed curve: u_last must be > u_first");
      return OCCTL_GEOMETRY_INVALID;
    }
    if (theInfo->sense != 1 && theInfo->sense != -1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "trimmed curve sense must be 1 or -1");
      return OCCTL_INVALID_ARGUMENT;
    }
    const bool                     aSense = (theInfo->sense == 1);
    const occ::handle<Geom_Curve>& aBasisCurve =
      OcctL::Geom::CurveFromRep(theGraph, theInfo->basis);
    occ::handle<Geom_TrimmedCurve> aTrimmed =
      new Geom_TrimmedCurve(aBasisCurve, theInfo->u_first, theInfo->u_last, aSense);
    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aTrimmed);
    *theOutCurve                  = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_trimmed(const occtl_graph_t* theGraph,
                                                           occtl_rep_id_t       theCurve,
                                                           double*              theOutUFirst,
                                                           double*              theOutULast)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_TrimmedCurve> aTrim = occ::down_cast<Geom_TrimmedCurve>(aLocalCurve);
    if (aTrim.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not trimmed");
      return OCCTL_WRONG_KIND;
    }
    if (theOutUFirst != nullptr)
    {
      *theOutUFirst = aTrim->FirstParameter();
    }
    if (theOutULast != nullptr)
    {
      *theOutULast = aTrim->LastParameter();
    }
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve_offset_create_info_init(occtl_curve_offset_create_info_t* theInfo)
{
  if (theInfo == nullptr)
  {
    return;
  }
  theInfo->struct_version = OCCTL_CURVE_OFFSET_CREATE_INFO_VERSION_1;
  theInfo->p_next         = nullptr;
  theInfo->basis.bits     = 0;
  theInfo->offset_dir     = {0.0, 0.0, 1.0};
  theInfo->offset         = 0.0;
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_offset(occtl_graph_t*                          theGraph,
                            const occtl_curve_offset_create_info_t* theInfo,
                            occtl_rep_id_t*                         theOutCurve)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCurve == nullptr || theInfo == nullptr
        || theInfo->basis.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_curve, info, and basis must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_CURVE_OFFSET_CREATE_INFO_VERSION_1)
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
    const gp_Vec aDir = OcctL::Geom::ToGp(theInfo->offset_dir);
    if (aDir.SquareMagnitude() <= Precision::SquareConfusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "offset_dir must have non-zero length");
      return OCCTL_GEOMETRY_INVALID;
    }
    const occ::handle<Geom_Curve>& aBasisCurve =
      OcctL::Geom::CurveFromRep(theGraph, theInfo->basis);
    occ::handle<Geom_OffsetCurve> anOffset =
      new Geom_OffsetCurve(aBasisCurve, theInfo->offset, gp_Dir(aDir));
    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(anOffset);
    *theOutCurve                  = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_offset(const occtl_graph_t* theGraph,
                                                          occtl_rep_id_t       theCurve,
                                                          double*              theOutOffset,
                                                          occtl_vector3_t*     theOutOffsetDir)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>&      aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_OffsetCurve> anOffset    = occ::down_cast<Geom_OffsetCurve>(aLocalCurve);
    if (anOffset.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not an offset curve");
      return OCCTL_WRONG_KIND;
    }
    if (theOutOffset != nullptr)
    {
      *theOutOffset = anOffset->Offset();
    }
    if (theOutOffsetDir != nullptr)
    {
      const gp_Dir& aD = anOffset->Direction();
      *theOutOffsetDir = {aD.X(), aD.Y(), aD.Z()};
    }
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_degree(const occtl_graph_t* theGraph,
                                                               occtl_rep_id_t       theCurve,
                                                               int32_t*             theOutVal)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutVal == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_val must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutVal = aBs->Degree();
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_pole_count(const occtl_graph_t* theGraph,
                                                                   occtl_rep_id_t       theCurve,
                                                                   size_t*              theOutVal)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutVal == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_val must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutVal = static_cast<size_t>(aBs->NbPoles());
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_knot_count(const occtl_graph_t* theGraph,
                                                                   occtl_rep_id_t       theCurve,
                                                                   size_t*              theOutVal)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutVal == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_val must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutVal = static_cast<size_t>(aBs->NbKnots());
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_is_rational(const occtl_graph_t* theGraph,
                                                                    occtl_rep_id_t       theCurve,
                                                                    int32_t*             theOutVal)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutVal == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_val must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    *theOutVal = aBs->IsRational() ? 1 : 0;
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bezier_degree(const occtl_graph_t* theGraph,
                                                              occtl_rep_id_t       theCurve,
                                                              int32_t*             theOutVal)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutVal == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_val must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>&      aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BezierCurve> aBz         = occ::down_cast<Geom_BezierCurve>(aLocalCurve);
    if (aBz.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a Bezier");
      return OCCTL_WRONG_KIND;
    }
    *theOutVal = aBz->Degree();
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bezier_pole_count(const occtl_graph_t* theGraph,
                                                                  occtl_rep_id_t       theCurve,
                                                                  size_t*              theOutVal)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutVal == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_val must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>&      aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BezierCurve> aBz         = occ::down_cast<Geom_BezierCurve>(aLocalCurve);
    if (aBz.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a Bezier");
      return OCCTL_WRONG_KIND;
    }
    *theOutVal = static_cast<size_t>(aBz->NbPoles());
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bezier_is_rational(const occtl_graph_t* theGraph,
                                                                   occtl_rep_id_t       theCurve,
                                                                   int32_t*             theOutVal)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutVal == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out_val must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>&      aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BezierCurve> aBz         = occ::down_cast<Geom_BezierCurve>(aLocalCurve);
    if (aBz.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a Bezier");
      return OCCTL_WRONG_KIND;
    }
    *theOutVal = aBz->IsRational() ? 1 : 0;
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_poles(const occtl_graph_t* theGraph,
                                                              occtl_rep_id_t       theCurve,
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
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNb = static_cast<size_t>(aBs->NbPoles());
    *theOutCount     = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline poles");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int anI = 1; anI <= static_cast<int>(aNb); ++anI)
    {
      const gp_Pnt& aP   = aBs->Pole(anI);
      theOutBuf[anI - 1] = {aP.X(), aP.Y(), aP.Z()};
    }
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_knots(const occtl_graph_t* theGraph,
                                                              occtl_rep_id_t       theCurve,
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
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNb = static_cast<size_t>(aBs->NbKnots());
    *theOutCount     = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline knots");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int anI = 1; anI <= static_cast<int>(aNb); ++anI)
    {
      theOutBuf[anI - 1] = aBs->Knot(anI);
    }
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_bspline_multiplicities(const occtl_graph_t* theGraph,
                                     occtl_rep_id_t       theCurve,
                                     int32_t*             theOutBuf,
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
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNb = static_cast<size_t>(aBs->NbKnots());
    *theOutCount     = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline multiplicities");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int anI = 1; anI <= static_cast<int>(aNb); ++anI)
    {
      theOutBuf[anI - 1] = static_cast<int32_t>(aBs->Multiplicity(anI));
    }
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_weights(const occtl_graph_t* theGraph,
                                                                occtl_rep_id_t       theCurve,
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
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    if (!aBs->IsRational())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "B-spline curve is non-rational; no weights");
      return OCCTL_WRONG_KIND;
    }
    const size_t aNb = static_cast<size_t>(aBs->NbPoles());
    *theOutCount     = aNb;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aNb)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline weights");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (int anI = 1; anI <= static_cast<int>(aNb); ++anI)
    {
      theOutBuf[anI - 1] = aBs->Weight(anI);
    }
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_bspline_flat_knots(const occtl_graph_t* theGraph,
                                                                   occtl_rep_id_t       theCurve,
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
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    // Flat (expanded) knot count = sum of multiplicities.
    const NCollection_Array1<double>& aKnots = aBs->Knots();
    const NCollection_Array1<int>&    aMults = aBs->Multiplicities();
    size_t                            aFlat  = 0;
    for (int anI = aMults.Lower(); anI <= aMults.Upper(); ++anI)
    {
      aFlat += static_cast<size_t>(aMults(anI));
    }
    *theOutCount = aFlat;
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aFlat)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "buffer too small for bspline flat knots");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    size_t aOut = 0;
    for (int anI = aKnots.Lower(); anI <= aKnots.Upper(); ++anI)
    {
      const double aV = aKnots(anI);
      const int    aM = aMults(anI);
      for (int aR = 0; aR < aM; ++aR)
      {
        theOutBuf[aOut++] = aV;
      }
    }
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_bspline_poles_view(const occtl_graph_t*   theGraph,
                                 occtl_rep_id_t         theCurve,
                                 const occtl_point3_t** theOutData,
                                 size_t*                theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutData == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, out_data, and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    const NCollection_Array1<gp_Pnt>& aPoles = aBs->Poles();
    *theOutData  = reinterpret_cast<const occtl_point3_t*>(&aPoles.First());
    *theOutCount = static_cast<size_t>(aPoles.Size());
    return OCCTL_OK;
  });
}

//============================================================================

OCCTL_API void OCCTL_CALL occtl_curve_bspline_init(occtl_curve_bspline_t* theOut)
{
  if (theOut == nullptr)
  {
    return;
  }
  *theOut = OCCTL_CURVE_BSPLINE_INIT;
}

//============================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_bspline(const occtl_graph_t*   theGraph,
                                                           occtl_rep_id_t         theCurve,
                                                           occtl_curve_bspline_t* theOut)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOut == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph and out must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOut->struct_version != OCCTL_CURVE_BSPLINE_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported struct_version in curve_bspline view");
      return OCCTL_VERSION_MISMATCH;
    }
    const occ::handle<Geom_Curve>& aLocalCurve = OcctL::Geom::CurveFromRep(theGraph, theCurve);
    const occ::handle<Geom_BSplineCurve> aBs   = occ::down_cast<Geom_BSplineCurve>(aLocalCurve);
    if (aBs.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a B-spline");
      return OCCTL_WRONG_KIND;
    }
    OcctL::Geom::FillBSplineCurveView(aBs, *theOut);
    return OCCTL_OK;
  });
}

} // extern "C"
