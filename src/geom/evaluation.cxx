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

#include <Geom2d_Curve.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>

#include <occtl/occtl_curves.h>
#include <occtl/occtl_curves2d.h>
#include <occtl/occtl_surfaces.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"
#include "GeomMath.hxx"

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_d0(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        const double         theU,
                                                        occtl_point3_t*      theOutPoint)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
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
      graph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;
    const gp_Pnt aP = aCurve->EvalD0(theU);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aP);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_d1(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        const double         theU,
                                                        occtl_point3_t*      theOutPoint,
                                                        occtl_vector3_t*     theOutD1)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
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
      graph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;
    const Geom_Curve::ResD1 aRes = aCurve->EvalD1(theU);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aRes.Point);
    }
    if (theOutD1 != nullptr)
    {
      *theOutD1 = OcctL::Geom::FromGp(aRes.D1);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_d2(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        const double         theU,
                                                        occtl_point3_t*      theOutPoint,
                                                        occtl_vector3_t*     theOutD1,
                                                        occtl_vector3_t*     theOutD2)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
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
      graph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;
    const Geom_Curve::ResD2 aRes = aCurve->EvalD2(theU);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aRes.Point);
    }
    if (theOutD1 != nullptr)
    {
      *theOutD1 = OcctL::Geom::FromGp(aRes.D1);
    }
    if (theOutD2 != nullptr)
    {
      *theOutD2 = OcctL::Geom::FromGp(aRes.D2);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_d3(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        const double         theU,
                                                        occtl_point3_t*      theOutPoint,
                                                        occtl_vector3_t*     theOutD1,
                                                        occtl_vector3_t*     theOutD2,
                                                        occtl_vector3_t*     theOutD3)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
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
      graph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;
    const Geom_Curve::ResD3 aRes = aCurve->EvalD3(theU);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aRes.Point);
    }
    if (theOutD1 != nullptr)
    {
      *theOutD1 = OcctL::Geom::FromGp(aRes.D1);
    }
    if (theOutD2 != nullptr)
    {
      *theOutD2 = OcctL::Geom::FromGp(aRes.D2);
    }
    if (theOutD3 != nullptr)
    {
      *theOutD3 = OcctL::Geom::FromGp(aRes.D3);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve_eval_dn(const occtl_graph_t* graph,
                                                        occtl_rep_id_t       curve_id,
                                                        const double         theU,
                                                        const int32_t        theN,
                                                        occtl_vector3_t*     theOutDerivative)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
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
    if (theOutDerivative == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_derivative must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_Curve3DRepId         aCurve3DId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Curve>& aCurve =
      graph->graph.Topo().Geometry().Curve3DRep(aCurve3DId).Curve;
    const gp_Vec aDN  = aCurve->EvalDN(theU, theN);
    *theOutDerivative = OcctL::Geom::FromGp(aDN);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_d0(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          const double         theU,
                                                          occtl_point2_t*      theOutPoint)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve2d rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve2D");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve2DRepId           aCurve2DId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom2d_Curve>& aCurve =
      graph->graph.Topo().Geometry().Curve2DRep(aCurve2DId).Curve;
    const gp_Pnt2d aP = aCurve->EvalD0(theU);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aP);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_d1(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          const double         theU,
                                                          occtl_point2_t*      theOutPoint,
                                                          occtl_vector2_t*     theOutD1)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve2d rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve2D");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve2DRepId           aCurve2DId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom2d_Curve>& aCurve =
      graph->graph.Topo().Geometry().Curve2DRep(aCurve2DId).Curve;
    const Geom2d_Curve::ResD1 aRes = aCurve->EvalD1(theU);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aRes.Point);
    }
    if (theOutD1 != nullptr)
    {
      *theOutD1 = OcctL::Geom::FromGp(aRes.D1);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_d2(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          const double         theU,
                                                          occtl_point2_t*      theOutPoint,
                                                          occtl_vector2_t*     theOutD1,
                                                          occtl_vector2_t*     theOutD2)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve2d rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve2D");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve2DRepId           aCurve2DId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom2d_Curve>& aCurve =
      graph->graph.Topo().Geometry().Curve2DRep(aCurve2DId).Curve;
    const Geom2d_Curve::ResD2 aRes = aCurve->EvalD2(theU);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aRes.Point);
    }
    if (theOutD1 != nullptr)
    {
      *theOutD1 = OcctL::Geom::FromGp(aRes.D1);
    }
    if (theOutD2 != nullptr)
    {
      *theOutD2 = OcctL::Geom::FromGp(aRes.D2);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_d3(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          const double         theU,
                                                          occtl_point2_t*      theOutPoint,
                                                          occtl_vector2_t*     theOutD1,
                                                          occtl_vector2_t*     theOutD2,
                                                          occtl_vector2_t*     theOutD3)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve2d rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve2D");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_Curve2DRepId           aCurve2DId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom2d_Curve>& aCurve =
      graph->graph.Topo().Geometry().Curve2DRep(aCurve2DId).Curve;
    const Geom2d_Curve::ResD3 aRes = aCurve->EvalD3(theU);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aRes.Point);
    }
    if (theOutD1 != nullptr)
    {
      *theOutD1 = OcctL::Geom::FromGp(aRes.D1);
    }
    if (theOutD2 != nullptr)
    {
      *theOutD2 = OcctL::Geom::FromGp(aRes.D2);
    }
    if (theOutD3 != nullptr)
    {
      *theOutD3 = OcctL::Geom::FromGp(aRes.D3);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_curve2d_eval_dn(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       curve_id,
                                                          const double         theU,
                                                          const int32_t        theN,
                                                          occtl_vector2_t*     theOutDerivative)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(curve_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve2d rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve2D");
      return OCCTL_WRONG_KIND;
    }
    if (theOutDerivative == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_derivative must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_Curve2DRepId           aCurve2DId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom2d_Curve>& aCurve =
      graph->graph.Topo().Geometry().Curve2DRep(aCurve2DId).Curve;
    const gp_Vec2d aDN = aCurve->EvalDN(theU, theN);
    *theOutDerivative  = OcctL::Geom::FromGp(aDN);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_d0(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          const double         theU,
                                                          const double         theV,
                                                          occtl_point3_t*      theOutPoint)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(surface_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Surface");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_SurfaceRepId           aSurfId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Surface>& aSurface =
      graph->graph.Topo().Geometry().SurfaceRep(aSurfId).Surface;
    const gp_Pnt aP = aSurface->EvalD0(theU, theV);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aP);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_d1(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          const double         theU,
                                                          const double         theV,
                                                          occtl_point3_t*      theOutPoint,
                                                          occtl_vector3_t*     theOutD1U,
                                                          occtl_vector3_t*     theOutD1V)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(surface_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Surface");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_SurfaceRepId           aSurfId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Surface>& aSurface =
      graph->graph.Topo().Geometry().SurfaceRep(aSurfId).Surface;
    const Geom_Surface::ResD1 aRes = aSurface->EvalD1(theU, theV);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aRes.Point);
    }
    if (theOutD1U != nullptr)
    {
      *theOutD1U = OcctL::Geom::FromGp(aRes.D1U);
    }
    if (theOutD1V != nullptr)
    {
      *theOutD1V = OcctL::Geom::FromGp(aRes.D1V);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_d2(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          const double         theU,
                                                          const double         theV,
                                                          occtl_point3_t*      theOutPoint,
                                                          occtl_vector3_t*     theOutD1U,
                                                          occtl_vector3_t*     theOutD1V,
                                                          occtl_vector3_t*     theOutD2U,
                                                          occtl_vector3_t*     theOutD2V,
                                                          occtl_vector3_t*     theOutD2UV)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(surface_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Surface");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_SurfaceRepId           aSurfId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Surface>& aSurface =
      graph->graph.Topo().Geometry().SurfaceRep(aSurfId).Surface;
    const Geom_Surface::ResD2 aRes = aSurface->EvalD2(theU, theV);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aRes.Point);
    }
    if (theOutD1U != nullptr)
    {
      *theOutD1U = OcctL::Geom::FromGp(aRes.D1U);
    }
    if (theOutD1V != nullptr)
    {
      *theOutD1V = OcctL::Geom::FromGp(aRes.D1V);
    }
    if (theOutD2U != nullptr)
    {
      *theOutD2U = OcctL::Geom::FromGp(aRes.D2U);
    }
    if (theOutD2V != nullptr)
    {
      *theOutD2V = OcctL::Geom::FromGp(aRes.D2V);
    }
    if (theOutD2UV != nullptr)
    {
      *theOutD2UV = OcctL::Geom::FromGp(aRes.D2UV);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_d3(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          const double         theU,
                                                          const double         theV,
                                                          occtl_point3_t*      theOutPoint,
                                                          occtl_vector3_t*     theOutD1U,
                                                          occtl_vector3_t*     theOutD1V,
                                                          occtl_vector3_t*     theOutD2U,
                                                          occtl_vector3_t*     theOutD2V,
                                                          occtl_vector3_t*     theOutD2UV,
                                                          occtl_vector3_t*     theOutD3U,
                                                          occtl_vector3_t*     theOutD3V,
                                                          occtl_vector3_t*     theOutD3UUV,
                                                          occtl_vector3_t*     theOutD3UVV)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(surface_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Surface");
      return OCCTL_WRONG_KIND;
    }
    BRepGraph_SurfaceRepId           aSurfId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Surface>& aSurface =
      graph->graph.Topo().Geometry().SurfaceRep(aSurfId).Surface;
    const Geom_Surface::ResD3 aRes = aSurface->EvalD3(theU, theV);
    if (theOutPoint != nullptr)
    {
      *theOutPoint = OcctL::Geom::FromGp(aRes.Point);
    }
    if (theOutD1U != nullptr)
    {
      *theOutD1U = OcctL::Geom::FromGp(aRes.D1U);
    }
    if (theOutD1V != nullptr)
    {
      *theOutD1V = OcctL::Geom::FromGp(aRes.D1V);
    }
    if (theOutD2U != nullptr)
    {
      *theOutD2U = OcctL::Geom::FromGp(aRes.D2U);
    }
    if (theOutD2V != nullptr)
    {
      *theOutD2V = OcctL::Geom::FromGp(aRes.D2V);
    }
    if (theOutD2UV != nullptr)
    {
      *theOutD2UV = OcctL::Geom::FromGp(aRes.D2UV);
    }
    if (theOutD3U != nullptr)
    {
      *theOutD3U = OcctL::Geom::FromGp(aRes.D3U);
    }
    if (theOutD3V != nullptr)
    {
      *theOutD3V = OcctL::Geom::FromGp(aRes.D3V);
    }
    if (theOutD3UUV != nullptr)
    {
      *theOutD3UUV = OcctL::Geom::FromGp(aRes.D3UUV);
    }
    if (theOutD3UVV != nullptr)
    {
      *theOutD3UVV = OcctL::Geom::FromGp(aRes.D3UVV);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_surface_eval_dn(const occtl_graph_t* graph,
                                                          occtl_rep_id_t       surface_id,
                                                          const double         theU,
                                                          const double         theV,
                                                          const int32_t        theNu,
                                                          const int32_t        theNv,
                                                          occtl_vector3_t*     theOutDerivative)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(surface_id);
    if (!aRawId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep id is invalid");
      return OCCTL_NOT_FOUND;
    }
    if (aRawId.RepKind != BRepGraph_RepId::Kind::Surface)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Surface");
      return OCCTL_WRONG_KIND;
    }
    if (theOutDerivative == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_derivative must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    BRepGraph_SurfaceRepId           aSurfId(static_cast<uint32_t>(aRawId.Index));
    const occ::handle<Geom_Surface>& aSurface =
      graph->graph.Topo().Geometry().SurfaceRep(aSurfId).Surface;
    const gp_Vec aDN  = aSurface->EvalDN(theU, theV, theNu, theNv);
    *theOutDerivative = OcctL::Geom::FromGp(aDN);
    return OCCTL_OK;
  });
}
