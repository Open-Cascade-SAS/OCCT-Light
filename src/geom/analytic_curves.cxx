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
#include "GeomMath.hxx"
#include "KindDetect.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_Line.hxx>
#include <Geom_Parabola.hxx>
#include <gp_Trsf.hxx>
#include <GCPnts_AbscissaPoint.hxx>

extern "C"
{

//==================================================================================================
// 3D curve constructors — analytic (line, circle, ellipse, hyperbola, parabola)
//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_line(occtl_graph_t*    theGraph,
                                                            occtl_geom_line_t theLine,
                                                            occtl_rep_id_t*   theOutId)
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
    occ::handle<Geom_Curve> aCurve = new Geom_Line(OcctL::Geom::ToGpLin(theLine));
    BRepGraph_Curve3DRepId  aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aCurve);
    *theOutId                      = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_circle(occtl_graph_t*      theGraph,
                                                              occtl_geom_circle_t theCircle,
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
    if (theCircle.radius <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "circle radius must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom_Curve> aCurve = new Geom_Circle(OcctL::Geom::ToGpCirc(theCircle));
    BRepGraph_Curve3DRepId  aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aCurve);
    *theOutId                      = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_ellipse(occtl_graph_t*       theGraph,
                                                               occtl_geom_ellipse_t theEllipse,
                                                               occtl_rep_id_t*      theOutId)
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
    occ::handle<Geom_Curve> aCurve = new Geom_Ellipse(OcctL::Geom::ToGpElips(theEllipse));
    BRepGraph_Curve3DRepId  aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aCurve);
    *theOutId                      = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_create_hyperbola(occtl_graph_t*         theGraph,
                               occtl_geom_hyperbola_t theHyperbola,
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
    if (theHyperbola.major_radius <= 0.0 || theHyperbola.minor_radius <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "hyperbola radii must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom_Curve> aCurve = new Geom_Hyperbola(OcctL::Geom::ToGpHypr(theHyperbola));
    BRepGraph_Curve3DRepId  aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aCurve);
    *theOutId                      = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_create_parabola(occtl_graph_t*        theGraph,
                                                                occtl_geom_parabola_t theParabola,
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
    if (theParabola.focal_length <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "parabola focal_length must be > 0");
      return OCCTL_GEOMETRY_INVALID;
    }
    occ::handle<Geom_Curve> aCurve = new Geom_Parabola(OcctL::Geom::ToGpParab(theParabola));
    BRepGraph_Curve3DRepId  aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aCurve);
    *theOutId                      = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================
//  Kind, periodicity, closure, continuity
//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_kind(const occtl_graph_t* theGraph,
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    *theOutKind = OcctL::Geom::DetermineCurveKind(aCurve);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_is_periodic(const occtl_graph_t* theGraph,
                                                            occtl_rep_id_t       theCurveId,
                                                            int32_t*             theOutIsPeriodic)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutIsPeriodic == nullptr)
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    *theOutIsPeriodic = aCurve->IsPeriodic() ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_is_closed(const occtl_graph_t* theGraph,
                                                          occtl_rep_id_t       theCurveId,
                                                          int32_t*             theOutIsClosed)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutIsClosed == nullptr)
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    *theOutIsClosed = aCurve->IsClosed() ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_continuity(const occtl_graph_t*     theGraph,
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    *theOutContinuity =
      static_cast<occtl_geom_continuity_t>(static_cast<int>(aCurve->Continuity()));
    return OCCTL_OK;
  });
}

//==================================================================================================
//  Parameter range
//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_parameter_range(const occtl_graph_t* theGraph,
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
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

//==================================================================================================
//  Analytic-type extraction (as_line, as_circle, as_ellipse, …)
//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_line(const occtl_graph_t* theGraph,
                                                        occtl_rep_id_t       theCurveId,
                                                        occtl_geom_line_t*   theOutLine)
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    const occ::handle<Geom_Line> aLine = occ::down_cast<Geom_Line>(aCurve);
    if (aLine.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a line");
      return OCCTL_WRONG_KIND;
    }
    *theOutLine = OcctL::Geom::FromGpLin(aLine->Lin());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_circle(const occtl_graph_t* theGraph,
                                                          occtl_rep_id_t       theCurveId,
                                                          occtl_geom_circle_t* theOutCircle)
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    const occ::handle<Geom_Circle> aCircle = occ::down_cast<Geom_Circle>(aCurve);
    if (aCircle.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a circle");
      return OCCTL_WRONG_KIND;
    }
    *theOutCircle = OcctL::Geom::FromGpCirc(aCircle->Circ());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_ellipse(const occtl_graph_t*  theGraph,
                                                           occtl_rep_id_t        theCurveId,
                                                           occtl_geom_ellipse_t* theOutEllipse)
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    const occ::handle<Geom_Ellipse> anEllipse = occ::down_cast<Geom_Ellipse>(aCurve);
    if (anEllipse.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not an ellipse");
      return OCCTL_WRONG_KIND;
    }
    *theOutEllipse = OcctL::Geom::FromGpElips(anEllipse->Elips());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_curve_as_hyperbola(const occtl_graph_t*    theGraph,
                           occtl_rep_id_t          theCurveId,
                           occtl_geom_hyperbola_t* theOutHyperbola)
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    const occ::handle<Geom_Hyperbola> aHypr = occ::down_cast<Geom_Hyperbola>(aCurve);
    if (aHypr.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a hyperbola");
      return OCCTL_WRONG_KIND;
    }
    *theOutHyperbola = OcctL::Geom::FromGpHypr(aHypr->Hypr());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_as_parabola(const occtl_graph_t*   theGraph,
                                                            occtl_rep_id_t         theCurveId,
                                                            occtl_geom_parabola_t* theOutParabola)
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    const occ::handle<Geom_Parabola> aParab = occ::down_cast<Geom_Parabola>(aCurve);
    if (aParab.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "curve is not a parabola");
      return OCCTL_WRONG_KIND;
    }
    *theOutParabola = OcctL::Geom::FromGpParab(aParab->Parab());
    return OCCTL_OK;
  });
}

//==================================================================================================
//  Curve transforms — reverse, transform, translate, rotate, scale
//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_reverse(occtl_graph_t*  theGraph,
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    occ::handle<Geom_Curve> aReversed = aCurve->Reversed();
    BRepGraph_Curve3DRepId  aRepId    = theGraph->graph.Editor().Reps().CreateCurve3D(aReversed);
    *theOutId                         = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_transformed(occtl_graph_t*    theGraph,
                                                            occtl_rep_id_t    theCurveId,
                                                            occtl_transform_t theTransform,
                                                            occtl_rep_id_t*   theOutId)
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    gp_Trsf                 aTrsf = OcctL::Geom::ToGpTrsf(theTransform);
    occ::handle<Geom_Curve> aTransformed =
      occ::handle<Geom_Curve>::DownCast(aCurve->Transformed(aTrsf));
    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aTransformed);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_translated(occtl_graph_t*  theGraph,
                                                           occtl_rep_id_t  theCurveId,
                                                           occtl_vector3_t theDelta,
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    gp_Trsf aTrsf;
    aTrsf.SetTranslation(OcctL::Geom::ToGp(theDelta));
    occ::handle<Geom_Curve> aTranslated =
      occ::handle<Geom_Curve>::DownCast(aCurve->Transformed(aTrsf));
    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aTranslated);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_rotated(occtl_graph_t*          theGraph,
                                                        occtl_rep_id_t          theCurveId,
                                                        occtl_axis1_placement_t theAxis,
                                                        double                  theAngle,
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
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theCurveId);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    gp_Trsf aTrsf;
    aTrsf.SetRotation(OcctL::Geom::ToGpAx1(theAxis), theAngle);
    occ::handle<Geom_Curve> aRotated =
      occ::handle<Geom_Curve>::DownCast(aCurve->Transformed(aTrsf));
    BRepGraph_Curve3DRepId aRepId = theGraph->graph.Editor().Reps().CreateCurve3D(aRotated);
    *theOutId                     = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_scaled(occtl_graph_t*  theGraph,
                                                       occtl_rep_id_t  theCurveId,
                                                       occtl_point3_t  theOrigin,
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    gp_Trsf aTrsf;
    aTrsf.SetScale(OcctL::Geom::ToGp(theOrigin), theFactor);
    occ::handle<Geom_Curve> aScaled = occ::handle<Geom_Curve>::DownCast(aCurve->Transformed(aTrsf));
    BRepGraph_Curve3DRepId  aRepId  = theGraph->graph.Editor().Reps().CreateCurve3D(aScaled);
    *theOutId                       = OcctL::Topo::PackRepId(aRepId);
    return OCCTL_OK;
  });
}

//==================================================================================================
//  Curve length, projection
//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_length(const occtl_graph_t* theGraph,
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    GeomAdaptor_Curve anAdaptor(aCurve);
    *theOutLength = GCPnts_AbscissaPoint::Length(anAdaptor);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_project_point(const occtl_graph_t* theGraph,
                                                              occtl_rep_id_t       theCurveId,
                                                              occtl_point3_t       thePoint,
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
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "id is not a Curve3D rep");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve3DRepId         aCurveId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
    GeomAPI_ProjectPointOnCurve aProj(OcctL::Geom::ToGp(thePoint), aCurve);
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

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_parameter_of_point(const occtl_graph_t* theGraph,
                                                                   occtl_rep_id_t       theCurveId,
                                                                   occtl_point3_t       thePoint,
                                                                   double*              theOutParam)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return occtl_curve_project_point(theGraph, theCurveId, thePoint, theOutParam, nullptr);
  });
}

} // extern "C"
