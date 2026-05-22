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

#include "GraphHandle.hxx"
#include "IdConvert.hxx"

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/GeomMath.hxx"

#include <BRepGraph_EditorView.hxx>
#include <Precision.hxx>

#include <cmath>

namespace
{

bool IsFiniteValue(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_vertex_point(occtl_graph_t* const  theGraph,
                                                                const occtl_node_id_t theVertex,
                                                                const occtl_point3_t  thePoint)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Vertices().SetPoint(aVertId, OcctL::Geom::ToGp(thePoint));
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_vertex_tolerance(occtl_graph_t* const  theGraph,
                                                                    const occtl_node_id_t theVertex,
                                                                    const double          theTol)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (!IsFiniteValue(theTol) || theTol < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tol must be finite and non-negative");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Vertices().SetTolerance(aVertId, theTol);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_tolerance(occtl_graph_t* const  theGraph,
                                                                  const occtl_node_id_t theEdge,
                                                                  const double          theTol)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (!IsFiniteValue(theTol) || theTol < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tol must be finite and non-negative");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Edges().SetTolerance(anEdgeId, theTol);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_face_tolerance(occtl_graph_t* const  theGraph,
                                                                  const occtl_node_id_t theFace,
                                                                  const double          theTol)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (!IsFiniteValue(theTol) || theTol < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tol must be finite and non-negative");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Faces().SetTolerance(aFaceId, theTol);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_param_range(occtl_graph_t* const  theGraph,
                                                                    const occtl_node_id_t theEdge,
                                                                    const double          theFirst,
                                                                    const double          theLast)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (!IsFiniteValue(theFirst) || !IsFiniteValue(theLast) || theLast < theFirst)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "edge parameter range is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Edges().SetParamRange(anEdgeId, theFirst, theLast);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_set_edge_same_parameter(occtl_graph_t* const  theGraph,
                                     const occtl_node_id_t theEdge,
                                     const int32_t         theFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Edges().SetSameParameter(anEdgeId, theFlag != 0);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_same_range(occtl_graph_t* const  theGraph,
                                                                   const occtl_node_id_t theEdge,
                                                                   const int32_t         theFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Edges().SetSameRange(anEdgeId, theFlag != 0);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_is_degenerate(occtl_graph_t* const theGraph,
                                                                      const occtl_node_id_t theEdge,
                                                                      const int32_t         theFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Edges().SetDegenerate(anEdgeId, theFlag != 0);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_edge_is_closed(occtl_graph_t* const  theGraph,
                                                                  const occtl_node_id_t theEdge,
                                                                  const int32_t         theFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Edges().SetIsClosed(anEdgeId, theFlag != 0);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_wire_is_closed(occtl_graph_t* const  theGraph,
                                                                  const occtl_node_id_t theWire,
                                                                  const int32_t         theFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_WireId aWireId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Wires().SetIsClosed(aWireId, theFlag != 0);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_shell_is_closed(occtl_graph_t* const  theGraph,
                                                                   const occtl_node_id_t theShell,
                                                                   const int32_t         theFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_ShellId aShellId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theShell, BRepGraph_NodeId::Kind::Shell, aShellId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Shells().SetIsClosed(aShellId, theFlag != 0);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_set_face_natural_restriction(occtl_graph_t* const  theGraph,
                                          const occtl_node_id_t theFace,
                                          const int32_t         theFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().Faces().SetNaturalRestriction(aFaceId, theFlag != 0);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_set_coedge_param_range(occtl_graph_t* const  theGraph,
                                    const occtl_node_id_t theCoedge,
                                    const double          theFirst,
                                    const double          theLast)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (!IsFiniteValue(theFirst) || !IsFiniteValue(theLast) || theLast < theFirst)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "coedge parameter range is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().CoEdges().SetParamRange(aCoEdgeId, theFirst, theLast);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_set_coedge_uv_box(occtl_graph_t* const  theGraph,
                                                                 const occtl_node_id_t theCoedge,
                                                                 const occtl_point2_t  theUvLo,
                                                                 const occtl_point2_t  theUvHi)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    theGraph->graph.Editor().CoEdges().SetUVBox(aCoEdgeId,
                                                OcctL::Geom::ToGp(theUvLo),
                                                OcctL::Geom::ToGp(theUvHi));
    return OCCTL_OK;
  });
}

} // extern "C"
