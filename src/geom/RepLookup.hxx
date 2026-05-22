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

#ifndef OCCTL_GEOM_REPLOOKUP_HXX
#define OCCTL_GEOM_REPLOOKUP_HXX

#include "../core/ErrorState.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <Geom2d_Curve.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>

namespace OcctL::Geom
{

inline occ::handle<Geom_Curve> CurveFromRep(const occtl_graph_t* theGraph, occtl_rep_id_t theId)
{
  const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theId);
  if (theGraph == nullptr || !aRawId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve rep id is invalid");
    return occ::handle<Geom_Curve>();
  }
  if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve3D)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve3D");
    return occ::handle<Geom_Curve>();
  }
  BRepGraph_Curve3DRepId aCurveId(static_cast<uint32_t>(aRawId.Index));
  return theGraph->graph.Topo().Geometry().Curve3DRep(aCurveId).Curve;
}

inline occ::handle<Geom_Surface> SurfaceFromRep(const occtl_graph_t* theGraph, occtl_rep_id_t theId)
{
  const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theId);
  if (theGraph == nullptr || !aRawId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "surface rep id is invalid");
    return occ::handle<Geom_Surface>();
  }
  if (aRawId.RepKind != BRepGraph_RepId::Kind::Surface)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Surface");
    return occ::handle<Geom_Surface>();
  }
  BRepGraph_SurfaceRepId aSurfId(static_cast<uint32_t>(aRawId.Index));
  return theGraph->graph.Topo().Geometry().SurfaceRep(aSurfId).Surface;
}

inline occ::handle<Geom2d_Curve> Curve2DFromRep(const occtl_graph_t* theGraph, occtl_rep_id_t theId)
{
  const BRepGraph_RepId aRawId = OcctL::Topo::UnpackRepId(theId);
  if (theGraph == nullptr || !aRawId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "curve2d rep id is invalid");
    return occ::handle<Geom2d_Curve>();
  }
  if (aRawId.RepKind != BRepGraph_RepId::Kind::Curve2D)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "rep id is not a Curve2D");
    return occ::handle<Geom2d_Curve>();
  }
  BRepGraph_Curve2DRepId aCurveId(static_cast<uint32_t>(aRawId.Index));
  return theGraph->graph.Topo().Geometry().Curve2DRep(aCurveId).Curve;
}

} // namespace OcctL::Geom

#endif // OCCTL_GEOM_REPLOOKUP_HXX
