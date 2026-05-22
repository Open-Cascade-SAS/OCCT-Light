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

#ifndef OCCTL_TOPO_GRAPH_MEASURE_CACHE_HXX
#define OCCTL_TOPO_GRAPH_MEASURE_CACHE_HXX

#include <occtl/occtl_topo.h>

#include <BRepGProp.hxx>
#include <BRepGraph.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_TopoView.hxx>
#include <TopoDS_Shape.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>

#include <cmath>

namespace OcctL::Topo
{

inline bool IsFiniteValue(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

inline bool ComputeMeasureValue(BRepGraph&                        theGraph,
                                const BRepGraph_NodeId            theNode,
                                const occtl_select_measure_kind_t theKind,
                                double&                           theOutValue)
{
  if (!theNode.IsValid() || theGraph.Topo().Gen().IsRemoved(theNode))
  {
    return false;
  }

  const TopoDS_Shape aShape = theGraph.Shapes().Shape(theNode);
  if (aShape.IsNull())
  {
    return false;
  }

  GProp_GProps aProps;
  switch (theKind)
  {
    case OCCTL_SELECT_MEASURE_EDGE_LENGTH:
      if (theNode.NodeKind != BRepGraph_NodeId::Kind::Edge)
      {
        return false;
      }
      BRepGProp::LinearProperties(aShape, aProps);
      break;
    case OCCTL_SELECT_MEASURE_WIRE_LENGTH:
      if (theNode.NodeKind != BRepGraph_NodeId::Kind::Wire)
      {
        return false;
      }
      BRepGProp::LinearProperties(aShape, aProps);
      break;
    case OCCTL_SELECT_MEASURE_FACE_AREA:
      if (theNode.NodeKind != BRepGraph_NodeId::Kind::Face)
      {
        return false;
      }
      BRepGProp::SurfaceProperties(aShape, aProps);
      break;
    case OCCTL_SELECT_MEASURE_SURFACE_AREA:
      BRepGProp::SurfaceProperties(aShape, aProps);
      break;
    case OCCTL_SELECT_MEASURE_VOLUME:
      BRepGProp::VolumeProperties(aShape, aProps);
      break;
    case OCCTL_SELECT_MEASURE_KIND_RESERVED_FUTURE:
      return false;
  }

  theOutValue = aProps.Mass();
  return IsFiniteValue(theOutValue);
}

} // namespace OcctL::Topo

#endif // OCCTL_TOPO_GRAPH_MEASURE_CACHE_HXX
