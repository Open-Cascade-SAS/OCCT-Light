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

#include "CurveMath.hxx"
#include "KindDetect.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"
#include "RepLookup.hxx"

#include <Geom2dAPI_ProjectPointOnCurve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom2dGcc_Circ2d2TanOn.hxx>
#include <Geom2dGcc_Circ2d2TanRad.hxx>
#include <Geom2dGcc_Circ2d3Tan.hxx>
#include <Geom2dGcc_Circ2dTanCen.hxx>
#include <Geom2dGcc_Circ2dTanOnRad.hxx>
#include <Geom2dGcc_Lin2d2Tan.hxx>
#include <Geom2dGcc_Lin2dTanObl.hxx>
#include <Geom2dGcc_QualifiedCurve.hxx>
#include <Geom2d_CartesianPoint.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_Hyperbola.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_Parabola.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <gp_Trsf2d.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GccEnt_Position.hxx>

#include <cmath>

namespace
{

bool ToGccQualifier(const occtl_curve2d_tangency_qualifier_t theQualifier,
                    GccEnt_Position&                         theOutQualifier)
{
  switch (theQualifier)
  {
    case OCCTL_GEOM_TANGENCY_UNQUALIFIED:
      theOutQualifier = GccEnt_unqualified;
      return true;
    case OCCTL_GEOM_TANGENCY_ENCLOSING:
      theOutQualifier = GccEnt_enclosing;
      return true;
    case OCCTL_GEOM_TANGENCY_ENCLOSED:
      theOutQualifier = GccEnt_enclosed;
      return true;
    case OCCTL_GEOM_TANGENCY_OUTSIDE:
      theOutQualifier = GccEnt_outside;
      return true;
    default:
      return false;
  }
}

void BlendArcRange(const double theFirst,
                   const double theSecond,
                   const bool   theLongArc,
                   double&      theOutFirst,
                   double&      theOutLast)
{
  const double aTwoPi       = 2.0 * OCCTL_PI;
  const double aPi          = OCCTL_PI;
  double       aForwardSpan = std::fmod(theSecond - theFirst, aTwoPi);
  if (aForwardSpan < 0.0)
  {
    aForwardSpan += aTwoPi;
  }
  if (aForwardSpan == 0.0)
  {
    aForwardSpan = aTwoPi;
  }

  const bool isForwardLong = aForwardSpan > aPi;
  if (isForwardLong == theLongArc)
  {
    theOutFirst = theFirst;
    theOutLast  = theFirst + aForwardSpan;
    return;
  }

  theOutFirst = theSecond;
  theOutLast  = theSecond + (aTwoPi - aForwardSpan);
}

} // namespace

extern "C"
{

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_to_two_radius_info_init(
  occtl_curve2d_circle_tangent_to_two_radius_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_curve2d_circle_tangent_to_two_radius_info_t anInit =
      OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_INIT;
    *theInfo = anInit;
  }
}

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_curve2d_blend_arc_info_init(occtl_curve2d_blend_arc_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_curve2d_blend_arc_info_t anInit = OCCTL_CURVE2D_BLEND_ARC_INFO_INIT;
    *theInfo                                    = anInit;
  }
}

//=================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_curve2d_line_tangent_to_two_info_init(occtl_curve2d_line_tangent_to_two_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_curve2d_line_tangent_to_two_info_t anInit =
      OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_INIT;
    *theInfo = anInit;
  }
}

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_curve2d_line_tangent_through_point_info_init(
  occtl_curve2d_line_tangent_through_point_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_curve2d_line_tangent_through_point_info_t anInit =
      OCCTL_CURVE2D_LINE_TANGENT_THROUGH_POINT_INFO_INIT;
    *theInfo = anInit;
  }
}

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_curve2d_line_tangent_with_angle_info_init(
  occtl_curve2d_line_tangent_with_angle_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_curve2d_line_tangent_with_angle_info_t anInit =
      OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_INIT;
    *theInfo = anInit;
  }
}

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_to_three_info_init(
  occtl_curve2d_circle_tangent_to_three_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_curve2d_circle_tangent_to_three_info_t anInit =
      OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_INIT;
    *theInfo = anInit;
  }
}

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_fixed_center_info_init(
  occtl_curve2d_circle_tangent_fixed_center_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_curve2d_circle_tangent_fixed_center_info_t anInit =
      OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_INIT;
    *theInfo = anInit;
  }
}

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_center_on_curve_info_init(
  occtl_curve2d_circle_tangent_center_on_curve_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_curve2d_circle_tangent_center_on_curve_info_t anInit =
      OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_INIT;
    *theInfo = anInit;
  }
}

//=================================================================================================

OCCTL_API void OCCTL_CALL occtl_curve2d_circle_tangent_on_curve_radius_info_init(
  occtl_curve2d_circle_tangent_on_curve_radius_info_t* theInfo)
{
  if (theInfo != nullptr)
  {
    const occtl_curve2d_circle_tangent_on_curve_radius_info_t anInit =
      OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_INIT;
    *theInfo = anInit;
  }
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_to_two_radius(
  occtl_graph_t*                                           theGraph,
  const occtl_curve2d_circle_tangent_to_two_radius_info_t* theInfo,
  size_t                                                   theSolutionIndex,
  occtl_geom2d_circle_t*                                   theOutCircle,
  size_t*                                                  theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "info and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCount = 0;
    if (theInfo->struct_version != OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported tangent-circle info version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || theInfo->curve_a.bits == 0 || theInfo->curve_b.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "p_next must be NULL and curves must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->radius <= 0.0 || theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "radius must be > 0 and tolerance must be >= 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    GccEnt_Position aQualifierA = GccEnt_unqualified;
    GccEnt_Position aQualifierB = GccEnt_unqualified;
    if (!ToGccQualifier(theInfo->qualifier_a, aQualifierA)
        || !ToGccQualifier(theInfo->qualifier_b, aQualifierB))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "invalid tangent qualifier");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      const Geom2dAdaptor_Curve anAdaptorA(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_a));
      const Geom2dAdaptor_Curve anAdaptorB(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_b));
      const Geom2dGcc_QualifiedCurve aQualifiedA(anAdaptorA, aQualifierA);
      const Geom2dGcc_QualifiedCurve aQualifiedB(anAdaptorB, aQualifierB);
      const Geom2dGcc_Circ2d2TanRad  aSolver(aQualifiedA,
                                             aQualifiedB,
                                             theInfo->radius,
                                             theInfo->tolerance);
      if (!aSolver.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "tangent-circle solver did not converge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const int aNbSolutions = aSolver.NbSolutions();
      *theOutCount           = static_cast<size_t>(aNbSolutions);
      if (theOutCircle == nullptr)
      {
        return OCCTL_OK;
      }
      if (theSolutionIndex >= *theOutCount)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "solution_index is outside the available range");
        return OCCTL_NOT_FOUND;
      }
      const int aSolution = static_cast<int>(theSolutionIndex) + 1;
      *theOutCircle       = OcctL::Geom::FromGpCirc2d(aSolver.ThisSolution(aSolution));
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
  occtl_curve2d_create_blend_arc(occtl_graph_t*                        theGraph,
                                 const occtl_curve2d_blend_arc_info_t* theInfo,
                                 size_t                                theSolutionIndex,
                                 occtl_rep_id_t*                       theOutCurve,
                                 size_t*                               theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "info and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCount = 0;
    if (theInfo->struct_version != OCCTL_CURVE2D_BLEND_ARC_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported blend-arc info version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || theInfo->curve_a.bits == 0 || theInfo->curve_b.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "p_next must be NULL and curves must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->radius <= 0.0 || theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "radius must be > 0 and tolerance must be >= 0");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->long_arc != 0 && theInfo->long_arc != 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "long_arc must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }

    GccEnt_Position aQualifierA = GccEnt_unqualified;
    GccEnt_Position aQualifierB = GccEnt_unqualified;
    if (!ToGccQualifier(theInfo->qualifier_a, aQualifierA)
        || !ToGccQualifier(theInfo->qualifier_b, aQualifierB))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "invalid tangent qualifier");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      const Geom2dAdaptor_Curve anAdaptorA(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_a));
      const Geom2dAdaptor_Curve anAdaptorB(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_b));
      const Geom2dGcc_QualifiedCurve aQualifiedA(anAdaptorA, aQualifierA);
      const Geom2dGcc_QualifiedCurve aQualifiedB(anAdaptorB, aQualifierB);
      const Geom2dGcc_Circ2d2TanRad  aSolver(aQualifiedA,
                                             aQualifiedB,
                                             theInfo->radius,
                                             theInfo->tolerance);
      if (!aSolver.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "blend-arc solver did not converge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const int aNbSolutions = aSolver.NbSolutions();
      *theOutCount           = static_cast<size_t>(aNbSolutions);
      if (theOutCurve == nullptr)
      {
        return OCCTL_OK;
      }
      if (theSolutionIndex >= *theOutCount)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "solution_index is outside the available range");
        return OCCTL_NOT_FOUND;
      }

      const int aSolution = static_cast<int>(theSolutionIndex) + 1;
      double    aParSol1  = 0.0;
      double    aParArg1  = 0.0;
      gp_Pnt2d  aPointSol1;
      double    aParSol2 = 0.0;
      double    aParArg2 = 0.0;
      gp_Pnt2d  aPointSol2;
      aSolver.Tangency1(aSolution, aParSol1, aParArg1, aPointSol1);
      aSolver.Tangency2(aSolution, aParSol2, aParArg2, aPointSol2);

      double aFirst = 0.0;
      double aLast  = 0.0;
      BlendArcRange(aParSol1, aParSol2, theInfo->long_arc != 0, aFirst, aLast);
      occ::handle<Geom2d_Circle>       aBasis = new Geom2d_Circle(aSolver.ThisSolution(aSolution));
      occ::handle<Geom2d_TrimmedCurve> aTrimmed =
        new Geom2d_TrimmedCurve(aBasis, aFirst, aLast, true);
      BRepGraph_Curve2DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aTrimmed);
      *theOutCurve                  = OcctL::Topo::PackRepId(aRepId);
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
  occtl_curve2d_create_tangent_line_to_two(occtl_graph_t*                                  theGraph,
                                           const occtl_curve2d_line_tangent_to_two_info_t* theInfo,
                                           size_t               theSolutionIndex,
                                           occtl_geom2d_line_t* theOutLine,
                                           size_t*              theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "info and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCount = 0;
    if (theInfo->struct_version != OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported tangent-line info version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || theInfo->curve_a.bits == 0 || theInfo->curve_b.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "p_next must be NULL and curves must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "tolerance must be >= 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    GccEnt_Position aQualifierA = GccEnt_unqualified;
    GccEnt_Position aQualifierB = GccEnt_unqualified;
    if (!ToGccQualifier(theInfo->qualifier_a, aQualifierA)
        || !ToGccQualifier(theInfo->qualifier_b, aQualifierB))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "invalid tangent qualifier");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      const Geom2dAdaptor_Curve anAdaptorA(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_a));
      const Geom2dAdaptor_Curve anAdaptorB(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_b));
      const Geom2dGcc_QualifiedCurve aQualifiedA(anAdaptorA, aQualifierA);
      const Geom2dGcc_QualifiedCurve aQualifiedB(anAdaptorB, aQualifierB);
      const Geom2dGcc_Lin2d2Tan      aSolver(aQualifiedA, aQualifiedB, theInfo->tolerance);
      if (!aSolver.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "tangent-line solver did not converge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const int aNbSolutions = aSolver.NbSolutions();
      *theOutCount           = static_cast<size_t>(aNbSolutions);
      if (theOutLine == nullptr)
      {
        return OCCTL_OK;
      }
      if (theSolutionIndex >= *theOutCount)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "solution_index is outside the available range");
        return OCCTL_NOT_FOUND;
      }
      const int aSolution = static_cast<int>(theSolutionIndex) + 1;
      *theOutLine         = OcctL::Geom::FromGpLin2d(aSolver.ThisSolution(aSolution));
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

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_line_through_point(
  occtl_graph_t*                                         theGraph,
  const occtl_curve2d_line_tangent_through_point_info_t* theInfo,
  size_t                                                 theSolutionIndex,
  occtl_geom2d_line_t*                                   theOutLine,
  size_t*                                                theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "info and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCount = 0;
    if (theInfo->struct_version != OCCTL_CURVE2D_LINE_TANGENT_THROUGH_POINT_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported tangent-through-point info version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || theInfo->curve.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "p_next must be NULL and curve must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "tolerance must be >= 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    GccEnt_Position aQualifier = GccEnt_unqualified;
    if (!ToGccQualifier(theInfo->qualifier, aQualifier))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "invalid tangent qualifier");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      const Geom2dAdaptor_Curve anAdaptor(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve));
      const Geom2dGcc_QualifiedCurve aQualified(anAdaptor, aQualifier);
      const Geom2dGcc_Lin2d2Tan      aSolver(aQualified,
                                             OcctL::Geom::ToGp(theInfo->point),
                                             theInfo->tolerance);
      if (!aSolver.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "tangent-line solver did not converge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const int aNbSolutions = aSolver.NbSolutions();
      *theOutCount           = static_cast<size_t>(aNbSolutions);
      if (theOutLine == nullptr)
      {
        return OCCTL_OK;
      }
      if (theSolutionIndex >= *theOutCount)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "solution_index is outside the available range");
        return OCCTL_NOT_FOUND;
      }
      const int aSolution = static_cast<int>(theSolutionIndex) + 1;
      *theOutLine         = OcctL::Geom::FromGpLin2d(aSolver.ThisSolution(aSolution));
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

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_line_with_angle(
  occtl_graph_t*                                      theGraph,
  const occtl_curve2d_line_tangent_with_angle_info_t* theInfo,
  size_t                                              theSolutionIndex,
  occtl_geom2d_line_t*                                theOutLine,
  size_t*                                             theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "info and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCount = 0;
    if (theInfo->struct_version != OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported tangent-angle info version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || theInfo->curve.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "p_next must be NULL and curve must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->use_initial_parameter != 0 && theInfo->use_initial_parameter != 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "use_initial_parameter must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "tolerance must be >= 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    GccEnt_Position aQualifier = GccEnt_unqualified;
    if (!ToGccQualifier(theInfo->qualifier, aQualifier))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "invalid tangent qualifier");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      const Geom2dAdaptor_Curve anAdaptor(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve));
      const Geom2dGcc_QualifiedCurve aQualified(anAdaptor, aQualifier);
      const gp_Lin2d              aReferenceLine = OcctL::Geom::ToGpLin2d(theInfo->reference_line);
      const Geom2dGcc_Lin2dTanObl aSolver = theInfo->use_initial_parameter != 0
                                              ? Geom2dGcc_Lin2dTanObl(aQualified,
                                                                      aReferenceLine,
                                                                      theInfo->tolerance,
                                                                      theInfo->initial_parameter,
                                                                      theInfo->angle_radians)
                                              : Geom2dGcc_Lin2dTanObl(aQualified,
                                                                      aReferenceLine,
                                                                      theInfo->tolerance,
                                                                      theInfo->angle_radians);
      if (!aSolver.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "tangent-angle solver did not converge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const int aNbSolutions = aSolver.NbSolutions();
      *theOutCount           = static_cast<size_t>(aNbSolutions);
      if (theOutLine == nullptr)
      {
        return OCCTL_OK;
      }
      if (theSolutionIndex >= *theOutCount)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "solution_index is outside the available range");
        return OCCTL_NOT_FOUND;
      }
      const int aSolution = static_cast<int>(theSolutionIndex) + 1;
      *theOutLine         = OcctL::Geom::FromGpLin2d(aSolver.ThisSolution(aSolution));
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

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_to_three(
  occtl_graph_t*                                      theGraph,
  const occtl_curve2d_circle_tangent_to_three_info_t* theInfo,
  size_t                                              theSolutionIndex,
  occtl_geom2d_circle_t*                              theOutCircle,
  size_t*                                             theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "info and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCount = 0;
    if (theInfo->struct_version != OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported tangent-three-circle info version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || theInfo->curve_a.bits == 0 || theInfo->curve_b.bits == 0
        || theInfo->curve_c.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "p_next must be NULL and all curves must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "tolerance must be >= 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    GccEnt_Position aQualifierA = GccEnt_unqualified;
    GccEnt_Position aQualifierB = GccEnt_unqualified;
    GccEnt_Position aQualifierC = GccEnt_unqualified;
    if (!ToGccQualifier(theInfo->qualifier_a, aQualifierA)
        || !ToGccQualifier(theInfo->qualifier_b, aQualifierB)
        || !ToGccQualifier(theInfo->qualifier_c, aQualifierC))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "invalid tangent qualifier");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      const Geom2dAdaptor_Curve anAdaptorA(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_a));
      const Geom2dAdaptor_Curve anAdaptorB(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_b));
      const Geom2dAdaptor_Curve anAdaptorC(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_c));
      const Geom2dGcc_QualifiedCurve aQualifiedA(anAdaptorA, aQualifierA);
      const Geom2dGcc_QualifiedCurve aQualifiedB(anAdaptorB, aQualifierB);
      const Geom2dGcc_QualifiedCurve aQualifiedC(anAdaptorC, aQualifierC);
      const Geom2dGcc_Circ2d3Tan     aSolver(aQualifiedA,
                                             aQualifiedB,
                                             aQualifiedC,
                                             theInfo->tolerance,
                                             theInfo->initial_parameter_a,
                                             theInfo->initial_parameter_b,
                                             theInfo->initial_parameter_c);
      if (!aSolver.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "tangent-three-circle solver did not converge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const int aNbSolutions = aSolver.NbSolutions();
      *theOutCount           = static_cast<size_t>(aNbSolutions);
      if (theOutCircle == nullptr)
      {
        return OCCTL_OK;
      }
      if (theSolutionIndex >= *theOutCount)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "solution_index is outside the available range");
        return OCCTL_NOT_FOUND;
      }
      const int aSolution = static_cast<int>(theSolutionIndex) + 1;
      *theOutCircle       = OcctL::Geom::FromGpCirc2d(aSolver.ThisSolution(aSolution));
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

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_fixed_center(
  occtl_graph_t*                                          theGraph,
  const occtl_curve2d_circle_tangent_fixed_center_info_t* theInfo,
  size_t                                                  theSolutionIndex,
  occtl_geom2d_circle_t*                                  theOutCircle,
  size_t*                                                 theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "info and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCount = 0;
    if (theInfo->struct_version != OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported tangent-fixed-center info version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || theInfo->curve.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "p_next must be NULL and curve must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "tolerance must be >= 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    GccEnt_Position aQualifier = GccEnt_unqualified;
    if (!ToGccQualifier(theInfo->qualifier, aQualifier))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "invalid tangent qualifier");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      const Geom2dAdaptor_Curve anAdaptor(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve));
      const Geom2dGcc_QualifiedCurve           aQualified(anAdaptor, aQualifier);
      const occ::handle<Geom2d_CartesianPoint> aCenter =
        new Geom2d_CartesianPoint(OcctL::Geom::ToGp(theInfo->center));
      const Geom2dGcc_Circ2dTanCen aSolver(aQualified, aCenter, theInfo->tolerance);
      if (!aSolver.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "tangent-fixed-center solver did not converge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const int aNbSolutions = aSolver.NbSolutions();
      *theOutCount           = static_cast<size_t>(aNbSolutions);
      if (theOutCircle == nullptr)
      {
        return OCCTL_OK;
      }
      if (theSolutionIndex >= *theOutCount)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "solution_index is outside the available range");
        return OCCTL_NOT_FOUND;
      }
      const int aSolution = static_cast<int>(theSolutionIndex) + 1;
      *theOutCircle       = OcctL::Geom::FromGpCirc2d(aSolver.ThisSolution(aSolution));
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

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_center_on_curve(
  occtl_graph_t*                                             theGraph,
  const occtl_curve2d_circle_tangent_center_on_curve_info_t* theInfo,
  size_t                                                     theSolutionIndex,
  occtl_geom2d_circle_t*                                     theOutCircle,
  size_t*                                                    theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "info and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCount = 0;
    if (theInfo->struct_version != OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported tangent-center-on-curve info version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || theInfo->curve_a.bits == 0 || theInfo->curve_b.bits == 0
        || theInfo->center_curve.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "p_next must be NULL and all curves must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "tolerance must be >= 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    GccEnt_Position aQualifierA = GccEnt_unqualified;
    GccEnt_Position aQualifierB = GccEnt_unqualified;
    if (!ToGccQualifier(theInfo->qualifier_a, aQualifierA)
        || !ToGccQualifier(theInfo->qualifier_b, aQualifierB))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "invalid tangent qualifier");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      const Geom2dAdaptor_Curve anAdaptorA(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_a));
      const Geom2dAdaptor_Curve anAdaptorB(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve_b));
      const Geom2dAdaptor_Curve anAdaptorCenter(
        OcctL::Geom::Curve2DFromRep(theGraph, theInfo->center_curve));
      const Geom2dGcc_QualifiedCurve aQualifiedA(anAdaptorA, aQualifierA);
      const Geom2dGcc_QualifiedCurve aQualifiedB(anAdaptorB, aQualifierB);
      const Geom2dGcc_Circ2d2TanOn   aSolver(aQualifiedA,
                                             aQualifiedB,
                                             anAdaptorCenter,
                                             theInfo->tolerance,
                                             theInfo->initial_parameter_a,
                                             theInfo->initial_parameter_b,
                                             theInfo->initial_parameter_center);
      if (!aSolver.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "tangent-center-on-curve solver did not converge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const int aNbSolutions = aSolver.NbSolutions();
      *theOutCount           = static_cast<size_t>(aNbSolutions);
      if (theOutCircle == nullptr)
      {
        return OCCTL_OK;
      }
      if (theSolutionIndex >= *theOutCount)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "solution_index is outside the available range");
        return OCCTL_NOT_FOUND;
      }
      const int aSolution = static_cast<int>(theSolutionIndex) + 1;
      *theOutCircle       = OcctL::Geom::FromGpCirc2d(aSolver.ThisSolution(aSolution));
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

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_tangent_circle_on_curve_radius(
  occtl_graph_t*                                             theGraph,
  const occtl_curve2d_circle_tangent_on_curve_radius_info_t* theInfo,
  size_t                                                     theSolutionIndex,
  occtl_geom2d_circle_t*                                     theOutCircle,
  size_t*                                                    theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "info and out_count must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCount = 0;
    if (theInfo->struct_version != OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported tangent-on-radius info version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr || theInfo->curve.bits == 0 || theInfo->center_curve.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "p_next must be NULL and curves must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->radius <= 0.0 || theInfo->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "radius must be > 0 and tolerance must be >= 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    GccEnt_Position aQualifier = GccEnt_unqualified;
    if (!ToGccQualifier(theInfo->qualifier, aQualifier))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "invalid tangent qualifier");
      return OCCTL_INVALID_ARGUMENT;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      const Geom2dAdaptor_Curve anAdaptor(OcctL::Geom::Curve2DFromRep(theGraph, theInfo->curve));
      const Geom2dAdaptor_Curve anAdaptorCenter(
        OcctL::Geom::Curve2DFromRep(theGraph, theInfo->center_curve));
      const Geom2dGcc_QualifiedCurve aQualified(anAdaptor, aQualifier);
      const Geom2dGcc_Circ2dTanOnRad aSolver(aQualified,
                                             anAdaptorCenter,
                                             theInfo->radius,
                                             theInfo->tolerance);
      if (!aSolver.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "tangent-on-radius solver did not converge");
        return OCCTL_GEOMETRY_INVALID;
      }

      const int aNbSolutions = aSolver.NbSolutions();
      *theOutCount           = static_cast<size_t>(aNbSolutions);
      if (theOutCircle == nullptr)
      {
        return OCCTL_OK;
      }
      if (theSolutionIndex >= *theOutCount)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "solution_index is outside the available range");
        return OCCTL_NOT_FOUND;
      }
      const int aSolution = static_cast<int>(theSolutionIndex) + 1;
      *theOutCircle       = OcctL::Geom::FromGpCirc2d(aSolver.ThisSolution(aSolution));
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
// 2D curve constructors — analytic (line, circle, ellipse, hyperbola, parabola)
//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_line(occtl_graph_t*      theGraph,
                                                              occtl_geom2d_line_t theLine,
                                                              occtl_rep_id_t*     theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occ::handle<Geom2d_Curve> aCurve = new Geom2d_Line(OcctL::Geom::ToGpLin2d(theLine));
    BRepGraph_Curve2DRepId    aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aCurve);
    *theOutId                        = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_circle(occtl_graph_t*        theGraph,
                                                                occtl_geom2d_circle_t theCircle,
                                                                occtl_rep_id_t*       theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theCircle.radius <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "circle radius must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom2d_Curve> aCurve = new Geom2d_Circle(OcctL::Geom::ToGpCirc2d(theCircle));
    BRepGraph_Curve2DRepId    aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aCurve);
    *theOutId                        = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_create_ellipse(occtl_graph_t*         theGraph,
                                                                 occtl_geom2d_ellipse_t theEllipse,
                                                                 occtl_rep_id_t*        theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theEllipse.minor_radius <= 0.0 || theEllipse.major_radius < theEllipse.minor_radius)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "ellipse: minor_radius > 0 and major >= minor required");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom2d_Curve> aCurve = new Geom2d_Ellipse(OcctL::Geom::ToGpElips2d(theEllipse));
    BRepGraph_Curve2DRepId    aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aCurve);
    *theOutId                        = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_create_hyperbola(occtl_graph_t*           theGraph,
                                 occtl_geom2d_hyperbola_t theHyperbola,
                                 occtl_rep_id_t*          theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theHyperbola.major_radius <= 0.0 || theHyperbola.minor_radius <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "hyperbola radii must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom2d_Curve> aCurve = new Geom2d_Hyperbola(OcctL::Geom::ToGpHypr2d(theHyperbola));
    BRepGraph_Curve2DRepId    aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aCurve);
    *theOutId                        = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_create_parabola(occtl_graph_t*          theGraph,
                                occtl_geom2d_parabola_t theParabola,
                                occtl_rep_id_t*         theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theParabola.focal_length <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "parabola focal_length must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom2d_Curve> aCurve = new Geom2d_Parabola(OcctL::Geom::ToGpParab2d(theParabola));
    BRepGraph_Curve2DRepId    aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aCurve);
    *theOutId                        = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================
//  Kind, periodicity, closure, continuity
//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_kind(const occtl_graph_t* theGraph,
                                                       occtl_rep_id_t       theCurveId,
                                                       occtl_curve_kind_t*  theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_kind is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    *theOutKind                             = OcctL::Geom::DetermineCurve2dKind(aCurve);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_is_periodic(const occtl_graph_t* theGraph,
                                                              occtl_rep_id_t       theCurveId,
                                                              int32_t*             theOutPeriodic)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutPeriodic == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_is_periodic is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    *theOutPeriodic                         = aCurve->IsPeriodic() ? 1 : 0;
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_is_closed(const occtl_graph_t* theGraph,
                                                            occtl_rep_id_t       theCurveId,
                                                            int32_t*             theOutClosed)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutClosed == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_is_closed is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    *theOutClosed                           = aCurve->IsClosed() ? 1 : 0;
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_continuity(occtl_graph_t*           theGraph,
                           occtl_rep_id_t           theCurveId,
                           occtl_geom_continuity_t* theOutContinuity)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutContinuity == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_continuity is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    *theOutContinuity =
      static_cast<occtl_geom_continuity_t>(static_cast<int>(aCurve->Continuity()));
    return OCCTL_OK;
  });
}

//=================================================================================================
//  Parameter range
//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_parameter_range(const occtl_graph_t* theGraph,
                                                                  occtl_rep_id_t       theCurveId,
                                                                  double*              theOutUMin,
                                                                  double*              theOutUMax)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    if (theOutUMin != nullptr)
    {
      *theOutUMin = aCurve->FirstParameter();
    }
    if (theOutUMax != nullptr)
    {
      *theOutUMax = aCurve->LastParameter();
    }
    return OCCTL_OK;
  });
}

//=================================================================================================
//  Analytic-type extraction (as_line, as_circle, as_ellipse, …)
//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_as_line(const occtl_graph_t* theGraph,
                                                          occtl_rep_id_t       theCurveId,
                                                          occtl_geom2d_line_t* theOutLine)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutLine == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_line is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    const occ::handle<Geom2d_Line>   aLine  = occ::down_cast<Geom2d_Line>(aCurve);
    if (aLine.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a 2D line");
      return OCCTL_WRONG_KIND;
    }
    *theOutLine = OcctL::Geom::FromGpLin2d(aLine->Lin2d());
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_as_circle(const occtl_graph_t*   theGraph,
                                                            occtl_rep_id_t         theCurveId,
                                                            occtl_geom2d_circle_t* theOutCircle)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutCircle == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_circle is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve  = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    const occ::handle<Geom2d_Circle> aCircle = occ::down_cast<Geom2d_Circle>(aCurve);
    if (aCircle.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a 2D circle");
      return OCCTL_WRONG_KIND;
    }
    *theOutCircle = OcctL::Geom::FromGpCirc2d(aCircle->Circ2d());
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_as_ellipse(const occtl_graph_t*    theGraph,
                                                             occtl_rep_id_t          theCurveId,
                                                             occtl_geom2d_ellipse_t* theOutEllipse)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutEllipse == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_ellipse is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>&  aCurve    = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    const occ::handle<Geom2d_Ellipse> anEllipse = occ::down_cast<Geom2d_Ellipse>(aCurve);
    if (anEllipse.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a 2D ellipse");
      return OCCTL_WRONG_KIND;
    }
    *theOutEllipse = OcctL::Geom::FromGpElips2d(anEllipse->Elips2d());
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_as_hyperbola(occtl_graph_t*            theGraph,
                             occtl_rep_id_t            theCurveId,
                             occtl_geom2d_hyperbola_t* theOutHyperbola)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutHyperbola == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_hyperbola is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>&    aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    const occ::handle<Geom2d_Hyperbola> aHypr  = occ::down_cast<Geom2d_Hyperbola>(aCurve);
    if (aHypr.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a 2D hyperbola");
      return OCCTL_WRONG_KIND;
    }
    *theOutHyperbola = OcctL::Geom::FromGpHypr2d(aHypr->Hypr2d());
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve2d_as_parabola(occtl_graph_t*           theGraph,
                            occtl_rep_id_t           theCurveId,
                            occtl_geom2d_parabola_t* theOutParabola)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutParabola == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_parabola is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>&   aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    const occ::handle<Geom2d_Parabola> aParab = occ::down_cast<Geom2d_Parabola>(aCurve);
    if (aParab.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a 2D parabola");
      return OCCTL_WRONG_KIND;
    }
    *theOutParabola = OcctL::Geom::FromGpParab2d(aParab->Parab2d());
    return OCCTL_OK;
  });
}

//=================================================================================================
//  Curve transforms — reverse, transform, translate, rotate, scale
//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_reverse(occtl_graph_t*  theGraph,
                                                          occtl_rep_id_t  theCurveId,
                                                          occtl_rep_id_t* theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve    = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    occ::handle<Geom2d_Curve>        aReversed = aCurve->Reversed();
    BRepGraph_Curve2DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aReversed);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_transformed(occtl_graph_t*  theGraph,
                                                              occtl_rep_id_t  theCurveId,
                                                              double          theTranslateX,
                                                              double          theTranslateY,
                                                              double          theRotateAngle,
                                                              double          theScaleX,
                                                              double          theScaleY,
                                                              occtl_rep_id_t* theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    const double                     aCos   = std::cos(theRotateAngle);
    const double                     aSin   = std::sin(theRotateAngle);
    gp_Trsf2d                        aTrsf;
    aTrsf.SetValues(theScaleX * aCos,
                    -theScaleY * aSin,
                    theTranslateX,
                    theScaleX * aSin,
                    theScaleY * aCos,
                    theTranslateY);
    occ::handle<Geom2d_Curve> aTransformed =
      occ::handle<Geom2d_Curve>::DownCast(aCurve->Transformed(aTrsf));
    BRepGraph_Curve2DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aTransformed);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_translated(occtl_graph_t*  theGraph,
                                                             occtl_rep_id_t  theCurveId,
                                                             occtl_vector2_t theDelta,
                                                             occtl_rep_id_t* theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    gp_Trsf2d                        aTrsf;
    aTrsf.SetTranslation(OcctL::Geom::ToGp(theDelta));
    occ::handle<Geom2d_Curve> aTranslated =
      occ::handle<Geom2d_Curve>::DownCast(aCurve->Transformed(aTrsf));
    BRepGraph_Curve2DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aTranslated);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_rotated(occtl_graph_t*  theGraph,
                                                          occtl_rep_id_t  theCurveId,
                                                          double          theAngle,
                                                          occtl_rep_id_t* theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    gp_Trsf2d                        aTrsf;
    aTrsf.SetRotation(gp::Origin2d(), theAngle);
    occ::handle<Geom2d_Curve> aRotated =
      occ::handle<Geom2d_Curve>::DownCast(aCurve->Transformed(aTrsf));
    BRepGraph_Curve2DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aRotated);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_scaled(occtl_graph_t*  theGraph,
                                                         occtl_rep_id_t  theCurveId,
                                                         occtl_point2_t  theOrigin,
                                                         double          theFactor,
                                                         occtl_rep_id_t* theOutId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theFactor == 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "scale factor is zero");
      return OCCTL_GEOMETRY_INVALID;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    gp_Trsf2d                        aTrsf;
    aTrsf.SetScale(OcctL::Geom::ToGp(theOrigin), theFactor);
    occ::handle<Geom2d_Curve> aScaled =
      occ::handle<Geom2d_Curve>::DownCast(aCurve->Transformed(aTrsf));
    BRepGraph_Curve2DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve2D(aScaled);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//=================================================================================================
//  Curve length, projection
//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_length(const occtl_graph_t* theGraph,
                                                         occtl_rep_id_t       theCurveId,
                                                         double*              theOutLength)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutLength == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_length is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    Geom2dAdaptor_Curve              anAdaptor(aCurve);
    *theOutLength = GCPnts_AbscissaPoint::Length(anAdaptor);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_project_point(const occtl_graph_t* theGraph,
                                                                occtl_rep_id_t       theCurveId,
                                                                occtl_point2_t       thePoint,
                                                                double*              theOutParam,
                                                                double*              theOutDistance)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutParam == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_param is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve2D rep");
      return OCCTL_WRONG_KIND;
    }
    const occ::handle<Geom2d_Curve>& aCurve = OcctL::Geom::Curve2DFromRep(theGraph, theCurveId);
    Geom2dAPI_ProjectPointOnCurve    aProj(OcctL::Geom::ToGp(thePoint), aCurve);
    if (aProj.NbPoints() < 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "projection failed");
      return OCCTL_GEOMETRY_INVALID;
    }
    *theOutParam = aProj.LowerDistanceParameter();
    if (theOutDistance != nullptr)
    {
      *theOutDistance = aProj.LowerDistance();
    }
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_parameter_of_point(const occtl_graph_t* theGraph,
                                                                     occtl_rep_id_t theCurveId,
                                                                     occtl_point2_t thePoint,
                                                                     double*        theOutParam)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return occtl_curve2d_project_point(theGraph, theCurveId, thePoint, theOutParam, nullptr);
  });
}

} // extern "C"
