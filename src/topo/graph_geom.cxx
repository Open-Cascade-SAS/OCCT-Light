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

#include "IdConvert.hxx"
#include "TopoMath.hxx"

#include <BRepGraph_MeshView.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

#include <occtl/occtl_geom.h>
#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>
#include <gp_Vec2d.hxx>

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_point(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theVertex,
                                                            occtl_point3_t* const      theOutPoint)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutPoint == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_point is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    const gp_Pnt aPnt = BRepGraph_Tool::Vertex::Pnt(theGraph->graph, aVertId);
    theOutPoint->x    = aPnt.X();
    theOutPoint->y    = aPnt.Y();
    theOutPoint->z    = aPnt.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_tolerance(const occtl_graph_t* const theGraph,
                                                                const occtl_node_id_t theVertex,
                                                                double* const theOutTolerance)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutTolerance == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_tolerance is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    *theOutTolerance = BRepGraph_Tool::Vertex::Tolerance(theGraph->graph, aVertId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_vertex_edge_count(const occtl_graph_t* const theGraph,
                               const occtl_node_id_t      theVertex,
                               uint32_t* const            theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    *theOutCount = BRepGraph_Tool::Vertex::NbEdges(theGraph->graph, aVertId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_range(const occtl_graph_t* const theGraph,
                                                          const occtl_node_id_t      theEdge,
                                                          double* const              theOutFirst,
                                                          double* const              theOutLast)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFirst == nullptr || theOutLast == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_first or out_last is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    const std::pair<double, double> aRange = BRepGraph_Tool::Edge::Range(theGraph->graph, anEdgeId);
    *theOutFirst                           = aRange.first;
    *theOutLast                            = aRange.second;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_tolerance(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theEdge,
                                                              double* const theOutTolerance)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutTolerance == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_tolerance is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    *theOutTolerance = BRepGraph_Tool::Edge::Tolerance(theGraph->graph, anEdgeId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_is_degenerated(const occtl_graph_t* const theGraph,
                                 const occtl_node_id_t      theEdge,
                                 int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Edge::Degenerated(theGraph->graph, anEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_has_curve(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theEdge,
                                                              int32_t* const             theOutHas)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutHas == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_has is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    *theOutHas = BRepGraph_Tool::Edge::HasCurve(theGraph->graph, anEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_curve_kind(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t      theEdge,
                                                               occtl_curve_kind_t* const theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return occtl_graph_edge_curve_kind_get(const_cast<occtl_graph_t*>(theGraph),
                                           theEdge,
                                           theOutKind);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_start_vertex(const occtl_graph_t* const theGraph,
                               const occtl_node_id_t      theEdge,
                               occtl_node_id_t* const     theOutVertex)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutVertex == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_vertex is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    const BRepGraph_VertexId aVertexId =
      BRepGraph_Tool::Edge::StartVertexId(theGraph->graph, anEdgeId);
    *theOutVertex = OcctL::Topo::PackNodeId(aVertexId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_end_vertex(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t      theEdge,
                                                               occtl_node_id_t* const theOutVertex)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutVertex == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_vertex is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    const BRepGraph_VertexId aVertexId =
      BRepGraph_Tool::Edge::EndVertexId(theGraph->graph, anEdgeId);
    *theOutVertex = OcctL::Topo::PackNodeId(aVertexId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_is_seam(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theCoedge,
                                                              int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::CoEdge::IsSeam(theGraph->graph, aCoEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_edge_of(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theCoedge,
                                                              occtl_node_id_t* const     theOutEdge)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutEdge == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_edge is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    const BRepGraph_EdgeId aEdgeId = BRepGraph_Tool::CoEdge::EdgeOf(theGraph->graph, aCoEdgeId);
    *theOutEdge                    = OcctL::Topo::PackNodeId(aEdgeId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_face_of(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theCoedge,
                                                              occtl_node_id_t* const     theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    const BRepGraph_FaceId aFaceId = BRepGraph_Tool::CoEdge::FaceOf(theGraph->graph, aCoEdgeId);
    *theOutFace                    = OcctL::Topo::PackNodeId(aFaceId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_tolerance(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theFace,
                                                              double* const theOutTolerance)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutTolerance == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_tolerance is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    *theOutTolerance = BRepGraph_Tool::Face::Tolerance(theGraph->graph, aFaceId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_wire_count(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t      theFace,
                                                               uint32_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    *theOutCount = BRepGraph_Tool::Face::NbWires(theGraph->graph, aFaceId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_outer_wire(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t      theFace,
                                                               occtl_node_id_t* const theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    const BRepGraph_WireId aWireId = BRepGraph_Tool::Face::OuterWireId(theGraph->graph, aFaceId);
    if (!aWireId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "face has no outer wire");
      return OCCTL_NOT_FOUND;
    }
    *theOutWire = OcctL::Topo::PackNodeId(aWireId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_uv_bounds(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theFace,
                                                              double* const              theOutUmin,
                                                              double* const              theOutUmax,
                                                              double* const              theOutVmin,
                                                              double* const              theOutVmax)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutUmin == nullptr || theOutUmax == nullptr || theOutVmin == nullptr
        || theOutVmax == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "one or more out-params are NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    BRepGraph_Tool::Face::Bounds(theGraph->graph,
                                 aFaceId,
                                 *theOutUmin,
                                 *theOutUmax,
                                 *theOutVmin,
                                 *theOutVmax);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_has_surface(const occtl_graph_t* const theGraph,
                                                                const occtl_node_id_t      theFace,
                                                                int32_t* const theOutHas)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutHas == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_has is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    *theOutHas = BRepGraph_Tool::Face::HasSurface(theGraph->graph, aFaceId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_surface_kind(const occtl_graph_t* const  theGraph,
                               const occtl_node_id_t       theFace,
                               occtl_surface_kind_t* const theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return occtl_graph_face_surface_kind_get(const_cast<occtl_graph_t*>(theGraph),
                                             theFace,
                                             theOutKind);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_is_closed(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theWire,
                                                              int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Wire::IsClosed(theGraph->graph, aWireId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wire_coedge_count(const occtl_graph_t* const theGraph,
                               const occtl_node_id_t      theWire,
                               uint32_t* const            theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aStatus;
    }

    *theOutCount = BRepGraph_Tool::Wire::NbCoEdges(theGraph->graph, aWireId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_shell_is_closed(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t      theShell,
                                                               int32_t* const theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_ShellId aShellId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theShell, BRepGraph_NodeId::Kind::Shell, aShellId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Shell::IsClosed(theGraph->graph, aShellId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_shell_face_count(const occtl_graph_t* const theGraph,
                                                                const occtl_node_id_t      theShell,
                                                                uint32_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_ShellId aShellId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theShell, BRepGraph_NodeId::Kind::Shell, aShellId))
    {
      return aStatus;
    }

    *theOutCount = BRepGraph_Tool::Shell::NbFaces(theGraph->graph, aShellId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_parameter(const occtl_graph_t* const theGraph,
                                                                const occtl_node_id_t theVertex,
                                                                const occtl_node_id_t theEdge,
                                                                double* const theOutParameter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutParameter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_parameter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    if (BRepGraph_Tool::Vertex::HasParameter(theGraph->graph, aVertId, anEdgeId))
    {
      *theOutParameter = BRepGraph_Tool::Vertex::Parameter(theGraph->graph, aVertId, anEdgeId);
      return OCCTL_OK;
    }

    const TopoDS_Shape aVertexShape = theGraph->graph.Shapes().Shape(aVertId);
    const TopoDS_Shape anEdgeShape  = theGraph->graph.Shapes().Shape(anEdgeId);
    if (aVertexShape.IsNull() || aVertexShape.ShapeType() != TopAbs_VERTEX || anEdgeShape.IsNull()
        || anEdgeShape.ShapeType() != TopAbs_EDGE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "vertex or edge could not be reconstructed");
      return OCCTL_NOT_FOUND;
    }

    *theOutParameter =
      BRep_Tool::Parameter(TopoDS::Vertex(aVertexShape), TopoDS::Edge(anEdgeShape));
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_vertex_parameters(const occtl_graph_t* const theGraph,
                               const occtl_node_id_t      theVertex,
                               const occtl_node_id_t      theFace,
                               occtl_point2_t* const      theOutUv)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutUv == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_uv is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    if (BRepGraph_Tool::Vertex::HasParameters(theGraph->graph, aVertId, aFaceId))
    {
      const gp_Pnt2d aUv = BRepGraph_Tool::Vertex::Parameters(theGraph->graph, aVertId, aFaceId);
      theOutUv->x        = aUv.X();
      theOutUv->y        = aUv.Y();
      return OCCTL_OK;
    }

    const TopoDS_Shape aVertexShape = theGraph->graph.Shapes().Shape(aVertId);
    const TopoDS_Shape aFaceShape   = theGraph->graph.Shapes().Shape(aFaceId);
    if (aVertexShape.IsNull() || aVertexShape.ShapeType() != TopAbs_VERTEX || aFaceShape.IsNull()
        || aFaceShape.ShapeType() != TopAbs_FACE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "vertex or face could not be reconstructed");
      return OCCTL_NOT_FOUND;
    }

    const gp_Pnt2d aUv =
      BRep_Tool::Parameters(TopoDS::Vertex(aVertexShape), TopoDS::Face(aFaceShape));
    theOutUv->x = aUv.X();
    theOutUv->y = aUv.Y();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_same_parameter(const occtl_graph_t* const theGraph,
                                 const occtl_node_id_t      theEdge,
                                 int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Edge::SameParameter(theGraph->graph, anEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_same_range(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t      theEdge,
                                                               int32_t* const theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Edge::SameRange(theGraph->graph, anEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_is_manifold(const occtl_graph_t* const theGraph,
                                                                const occtl_node_id_t      theEdge,
                                                                int32_t* const theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Edge::IsManifold(theGraph->graph, anEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_is_boundary(const occtl_graph_t* const theGraph,
                                                                const occtl_node_id_t      theEdge,
                                                                int32_t* const theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Edge::IsBoundary(theGraph->graph, anEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_is_seam_on_face(const occtl_graph_t* const theGraph,
                                  const occtl_node_id_t      theEdge,
                                  const occtl_node_id_t      theFace,
                                  int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Edge::IsSeamOnFace(theGraph->graph, anEdgeId, aFaceId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_face_count(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t      theEdge,
                                                               uint32_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    *theOutCount = BRepGraph_Tool::Edge::NbFaces(theGraph->graph, anEdgeId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_is_reversed(const occtl_graph_t* const theGraph,
                                const occtl_node_id_t      theCoedge,
                                int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::CoEdge::IsReversed(theGraph->graph, aCoEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_has_pcurve(const occtl_graph_t* const theGraph,
                               const occtl_node_id_t      theCoedge,
                               int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::CoEdge::HasPCurve(theGraph->graph, aCoEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_pcurve_parameter(const occtl_graph_t* const theGraph,
                                     const occtl_node_id_t      theCoedge,
                                     const occtl_node_id_t      theVertex,
                                     double* const              theOutParameter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutParameter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_parameter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoId))
    {
      return aStatus;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    if (BRepGraph_Tool::Vertex::HasPCurveParameter(theGraph->graph, aVertId, aCoId))
    {
      *theOutParameter = BRepGraph_Tool::Vertex::PCurveParameter(theGraph->graph, aVertId, aCoId);
      return OCCTL_OK;
    }

    const BRepGraph_EdgeId anEdgeId     = BRepGraph_Tool::CoEdge::EdgeOf(theGraph->graph, aCoId);
    const BRepGraph_FaceId aFaceId      = BRepGraph_Tool::CoEdge::FaceOf(theGraph->graph, aCoId);
    const TopoDS_Shape     aVertexShape = theGraph->graph.Shapes().Shape(aVertId);
    const TopoDS_Shape     anEdgeShape  = theGraph->graph.Shapes().Shape(anEdgeId);
    const TopoDS_Shape     aFaceShape   = theGraph->graph.Shapes().Shape(aFaceId);
    if (aVertexShape.IsNull() || aVertexShape.ShapeType() != TopAbs_VERTEX || anEdgeShape.IsNull()
        || anEdgeShape.ShapeType() != TopAbs_EDGE || aFaceShape.IsNull()
        || aFaceShape.ShapeType() != TopAbs_FACE)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "vertex, coedge edge, or coedge face could not be reconstructed");
      return OCCTL_NOT_FOUND;
    }

    *theOutParameter = BRep_Tool::Parameter(TopoDS::Vertex(aVertexShape),
                                            TopoDS::Edge(anEdgeShape),
                                            TopoDS::Face(aFaceShape));
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_range(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theCoedge,
                                                            double* const              theOutFirst,
                                                            double* const              theOutLast)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFirst == nullptr || theOutLast == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_first or out_last is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    const std::pair<double, double> aRange =
      BRepGraph_Tool::CoEdge::Range(theGraph->graph, aCoEdgeId);
    *theOutFirst = aRange.first;
    *theOutLast  = aRange.second;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_uv_points(const occtl_graph_t* const theGraph,
                                                                const occtl_node_id_t theCoedge,
                                                                occtl_point2_t* const theOutUvStart,
                                                                occtl_point2_t* const theOutUvEnd)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    const std::pair<gp_Pnt2d, gp_Pnt2d> aUV =
      BRepGraph_Tool::CoEdge::UVPoints(theGraph->graph, aCoEdgeId);
    if (theOutUvStart != nullptr)
    {
      theOutUvStart->x = aUV.first.X();
      theOutUvStart->y = aUV.first.Y();
    }
    if (theOutUvEnd != nullptr)
    {
      theOutUvEnd->x = aUV.second.X();
      theOutUvEnd->y = aUV.second.Y();
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_seam_pair(const occtl_graph_t* const theGraph,
                                                                const occtl_node_id_t  theCoedge,
                                                                occtl_node_id_t* const theOutPair)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutPair == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_pair is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    const BRepGraph_CoEdgeId aPairId = BRepGraph_Tool::CoEdge::SeamPair(theGraph->graph, aCoEdgeId);
    *theOutPair                      = OcctL::Topo::PackNodeId(aPairId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_natural_restriction(const occtl_graph_t* const theGraph,
                                      const occtl_node_id_t      theFace,
                                      int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Face::NaturalRestriction(theGraph->graph, aFaceId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_has_triangulation(const occtl_graph_t* const theGraph,
                                    const occtl_node_id_t      theFace,
                                    int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    *theOutFlag = theGraph->graph.Mesh().Effective().Faces().Has(aFaceId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wire_distinct_edge_count(const occtl_graph_t* const theGraph,
                                      const occtl_node_id_t      theWire,
                                      uint32_t* const            theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aStatus;
    }

    *theOutCount = BRepGraph_Tool::Wire::NbDistinctEdges(theGraph->graph, aWireId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_face_of(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theWire,
                                                            occtl_node_id_t* const     theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aStatus;
    }

    const BRepGraph_FaceId aFaceId = BRepGraph_Tool::Wire::FaceOf(theGraph->graph, aWireId);
    if (!aFaceId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "wire has no parent face");
      return OCCTL_NOT_FOUND;
    }
    *theOutFace = OcctL::Topo::PackNodeId(aFaceId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_is_outer(const occtl_graph_t* const theGraph,
                                                             const occtl_node_id_t      theWire,
                                                             int32_t* const             theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aStatus;
    }

    *theOutFlag = BRepGraph_Tool::Wire::IsOuter(theGraph->graph, aWireId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval(const occtl_graph_t* const theGraph,
                                                         const occtl_node_id_t      theEdge,
                                                         const double               theU,
                                                         occtl_point3_t* const      theOutP)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutP == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_p is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedCurve anAdaptor =
      BRepGraph_Tool::Edge::CurveAdaptor(theGraph->graph, anEdgeId);
    const gp_Pnt aPnt = anAdaptor.EvalD0(theU);
    theOutP->x        = aPnt.X();
    theOutP->y        = aPnt.Y();
    theOutP->z        = aPnt.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval_d1(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theEdge,
                                                            const double               theU,
                                                            occtl_point3_t* const      theOutP,
                                                            occtl_vector3_t* const     theOutD1)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutP == nullptr || theOutD1 == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_p or out_d1 is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedCurve anAdaptor =
      BRepGraph_Tool::Edge::CurveAdaptor(theGraph->graph, anEdgeId);
    const Geom_Curve::ResD1 aRes = anAdaptor.EvalD1(theU);
    theOutP->x                   = aRes.Point.X();
    theOutP->y                   = aRes.Point.Y();
    theOutP->z                   = aRes.Point.Z();
    theOutD1->x                  = aRes.D1.X();
    theOutD1->y                  = aRes.D1.Y();
    theOutD1->z                  = aRes.D1.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval_d2(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theEdge,
                                                            const double               theU,
                                                            occtl_point3_t* const      theOutP,
                                                            occtl_vector3_t* const     theOutD1,
                                                            occtl_vector3_t* const     theOutD2)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutP == nullptr || theOutD1 == nullptr || theOutD2 == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_p, out_d1, or out_d2 is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedCurve anAdaptor =
      BRepGraph_Tool::Edge::CurveAdaptor(theGraph->graph, anEdgeId);
    const Geom_Curve::ResD2 aRes = anAdaptor.EvalD2(theU);
    theOutP->x                   = aRes.Point.X();
    theOutP->y                   = aRes.Point.Y();
    theOutP->z                   = aRes.Point.Z();
    theOutD1->x                  = aRes.D1.X();
    theOutD1->y                  = aRes.D1.Y();
    theOutD1->z                  = aRes.D1.Z();
    theOutD2->x                  = aRes.D2.X();
    theOutD2->y                  = aRes.D2.Y();
    theOutD2->z                  = aRes.D2.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval_d3(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theEdge,
                                                            const double               theU,
                                                            occtl_point3_t* const      theOutP,
                                                            occtl_vector3_t* const     theOutD1,
                                                            occtl_vector3_t* const     theOutD2,
                                                            occtl_vector3_t* const     theOutD3)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutP == nullptr || theOutD1 == nullptr || theOutD2 == nullptr || theOutD3 == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_p, out_d1, out_d2, or out_d3 is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedCurve anAdaptor =
      BRepGraph_Tool::Edge::CurveAdaptor(theGraph->graph, anEdgeId);
    const Geom_Curve::ResD3 aRes = anAdaptor.EvalD3(theU);
    theOutP->x                   = aRes.Point.X();
    theOutP->y                   = aRes.Point.Y();
    theOutP->z                   = aRes.Point.Z();
    theOutD1->x                  = aRes.D1.X();
    theOutD1->y                  = aRes.D1.Y();
    theOutD1->z                  = aRes.D1.Z();
    theOutD2->x                  = aRes.D2.X();
    theOutD2->y                  = aRes.D2.Y();
    theOutD2->z                  = aRes.D2.Z();
    theOutD3->x                  = aRes.D3.X();
    theOutD3->y                  = aRes.D3.Y();
    theOutD3->z                  = aRes.D3.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_eval_dn(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theEdge,
                                                            const double               theU,
                                                            const uint32_t             theN,
                                                            occtl_vector3_t* const     theOutDn)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutDn == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_dn is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedCurve anAdaptor =
      BRepGraph_Tool::Edge::CurveAdaptor(theGraph->graph, anEdgeId);
    const gp_Vec aDN = anAdaptor.EvalDN(theU, static_cast<int>(theN));
    theOutDn->x      = aDN.X();
    theOutDn->y      = aDN.Y();
    theOutDn->z      = aDN.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_pcurve_eval(const occtl_graph_t* const theGraph,
                                const occtl_node_id_t      theCoedge,
                                const double               theU,
                                occtl_point2_t* const      theOutUv)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutUv == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_uv is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoId))
    {
      return aStatus;
    }

    Geom2dAdaptor_Curve anAdaptor = BRepGraph_Tool::CoEdge::PCurveAdaptor(theGraph->graph, aCoId);
    const gp_Pnt2d      aPnt2d    = anAdaptor.EvalD0(theU);
    theOutUv->x                   = aPnt2d.X();
    theOutUv->y                   = aPnt2d.Y();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_pcurve_eval_d1(const occtl_graph_t* const theGraph,
                                   const occtl_node_id_t      theCoedge,
                                   const double               theU,
                                   occtl_point2_t* const      theOutUv,
                                   occtl_vector2_t* const     theOutD1)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutUv == nullptr || theOutD1 == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_uv or out_d1 is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoId))
    {
      return aStatus;
    }

    Geom2dAdaptor_Curve anAdaptor  = BRepGraph_Tool::CoEdge::PCurveAdaptor(theGraph->graph, aCoId);
    const Geom2d_Curve::ResD1 aRes = anAdaptor.EvalD1(theU);
    theOutUv->x                    = aRes.Point.X();
    theOutUv->y                    = aRes.Point.Y();
    theOutD1->x                    = aRes.D1.X();
    theOutD1->y                    = aRes.D1.Y();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_pcurve_eval_d2(const occtl_graph_t* const theGraph,
                                   const occtl_node_id_t      theCoedge,
                                   const double               theU,
                                   occtl_point2_t* const      theOutUv,
                                   occtl_vector2_t* const     theOutD1,
                                   occtl_vector2_t* const     theOutD2)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutUv == nullptr || theOutD1 == nullptr || theOutD2 == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_uv, out_d1, or out_d2 is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoId))
    {
      return aStatus;
    }

    Geom2dAdaptor_Curve anAdaptor  = BRepGraph_Tool::CoEdge::PCurveAdaptor(theGraph->graph, aCoId);
    const Geom2d_Curve::ResD2 aRes = anAdaptor.EvalD2(theU);
    theOutUv->x                    = aRes.Point.X();
    theOutUv->y                    = aRes.Point.Y();
    theOutD1->x                    = aRes.D1.X();
    theOutD1->y                    = aRes.D1.Y();
    theOutD2->x                    = aRes.D2.X();
    theOutD2->y                    = aRes.D2.Y();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_pcurve_eval_d3(const occtl_graph_t* const theGraph,
                                   const occtl_node_id_t      theCoedge,
                                   const double               theU,
                                   occtl_point2_t* const      theOutUv,
                                   occtl_vector2_t* const     theOutD1,
                                   occtl_vector2_t* const     theOutD2,
                                   occtl_vector2_t* const     theOutD3)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutUv == nullptr || theOutD1 == nullptr || theOutD2 == nullptr || theOutD3 == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_uv, out_d1, out_d2, or out_d3 is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoId))
    {
      return aStatus;
    }

    Geom2dAdaptor_Curve anAdaptor  = BRepGraph_Tool::CoEdge::PCurveAdaptor(theGraph->graph, aCoId);
    const Geom2d_Curve::ResD3 aRes = anAdaptor.EvalD3(theU);
    theOutUv->x                    = aRes.Point.X();
    theOutUv->y                    = aRes.Point.Y();
    theOutD1->x                    = aRes.D1.X();
    theOutD1->y                    = aRes.D1.Y();
    theOutD2->x                    = aRes.D2.X();
    theOutD2->y                    = aRes.D2.Y();
    theOutD3->x                    = aRes.D3.X();
    theOutD3->y                    = aRes.D3.Y();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_pcurve_eval_dn(const occtl_graph_t* const theGraph,
                                   const occtl_node_id_t      theCoedge,
                                   const double               theU,
                                   const uint32_t             theN,
                                   occtl_vector2_t* const     theOutDn)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutDn == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_dn is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoId))
    {
      return aStatus;
    }

    Geom2dAdaptor_Curve anAdaptor = BRepGraph_Tool::CoEdge::PCurveAdaptor(theGraph->graph, aCoId);
    const gp_Vec2d      aDN       = anAdaptor.EvalDN(theU, static_cast<int>(theN));
    theOutDn->x                   = aDN.X();
    theOutDn->y                   = aDN.Y();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval(const occtl_graph_t* const theGraph,
                                                         const occtl_node_id_t      theFace,
                                                         const double               theU,
                                                         const double               theV,
                                                         occtl_point3_t* const      theOutP)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutP == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_p is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedSurface anAdaptor =
      BRepGraph_Tool::Face::SurfaceAdaptor(theGraph->graph, aFaceId);
    const gp_Pnt aPnt = anAdaptor.EvalD0(theU, theV);
    theOutP->x        = aPnt.X();
    theOutP->y        = aPnt.Y();
    theOutP->z        = aPnt.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval_d1(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theFace,
                                                            const double               theU,
                                                            const double               theV,
                                                            occtl_point3_t* const      theOutP,
                                                            occtl_vector3_t* const     theOutD1U,
                                                            occtl_vector3_t* const     theOutD1V)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutP == nullptr || theOutD1U == nullptr || theOutD1V == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_p, out_d1u, or out_d1v is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedSurface anAdaptor =
      BRepGraph_Tool::Face::SurfaceAdaptor(theGraph->graph, aFaceId);
    const Geom_Surface::ResD1 aRes = anAdaptor.EvalD1(theU, theV);
    theOutP->x                     = aRes.Point.X();
    theOutP->y                     = aRes.Point.Y();
    theOutP->z                     = aRes.Point.Z();
    theOutD1U->x                   = aRes.D1U.X();
    theOutD1U->y                   = aRes.D1U.Y();
    theOutD1U->z                   = aRes.D1U.Z();
    theOutD1V->x                   = aRes.D1V.X();
    theOutD1V->y                   = aRes.D1V.Y();
    theOutD1V->z                   = aRes.D1V.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval_dn(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theFace,
                                                            const double               theU,
                                                            const double               theV,
                                                            const uint32_t             theNu,
                                                            const uint32_t             theNv,
                                                            occtl_vector3_t* const     theOutDn)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutDn == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_dn is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedSurface anAdaptor =
      BRepGraph_Tool::Face::SurfaceAdaptor(theGraph->graph, aFaceId);
    const gp_Vec aDN =
      anAdaptor.EvalDN(theU, theV, static_cast<int>(theNu), static_cast<int>(theNv));
    theOutDn->x = aDN.X();
    theOutDn->y = aDN.Y();
    theOutDn->z = aDN.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval_d2(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theFace,
                                                            const double               theU,
                                                            const double               theV,
                                                            occtl_point3_t* const      theOutP,
                                                            occtl_vector3_t* const     theOutD1U,
                                                            occtl_vector3_t* const     theOutD1V,
                                                            occtl_vector3_t* const     theOutD2U,
                                                            occtl_vector3_t* const     theOutD2V,
                                                            occtl_vector3_t* const     theOutD2UV)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutP == nullptr || theOutD1U == nullptr || theOutD1V == nullptr || theOutD2U == nullptr
        || theOutD2V == nullptr || theOutD2UV == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "an out-param is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedSurface anAdaptor =
      BRepGraph_Tool::Face::SurfaceAdaptor(theGraph->graph, aFaceId);
    const Geom_Surface::ResD2 aRes = anAdaptor.EvalD2(theU, theV);
    theOutP->x                     = aRes.Point.X();
    theOutP->y                     = aRes.Point.Y();
    theOutP->z                     = aRes.Point.Z();
    theOutD1U->x                   = aRes.D1U.X();
    theOutD1U->y                   = aRes.D1U.Y();
    theOutD1U->z                   = aRes.D1U.Z();
    theOutD1V->x                   = aRes.D1V.X();
    theOutD1V->y                   = aRes.D1V.Y();
    theOutD1V->z                   = aRes.D1V.Z();
    theOutD2U->x                   = aRes.D2U.X();
    theOutD2U->y                   = aRes.D2U.Y();
    theOutD2U->z                   = aRes.D2U.Z();
    theOutD2V->x                   = aRes.D2V.X();
    theOutD2V->y                   = aRes.D2V.Y();
    theOutD2V->z                   = aRes.D2V.Z();
    theOutD2UV->x                  = aRes.D2UV.X();
    theOutD2UV->y                  = aRes.D2UV.Y();
    theOutD2UV->z                  = aRes.D2UV.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_eval_d3(const occtl_graph_t* const theGraph,
                                                            const occtl_node_id_t      theFace,
                                                            const double               theU,
                                                            const double               theV,
                                                            occtl_point3_t* const      theOutP,
                                                            occtl_vector3_t* const     theOutD1U,
                                                            occtl_vector3_t* const     theOutD1V,
                                                            occtl_vector3_t* const     theOutD2U,
                                                            occtl_vector3_t* const     theOutD2V,
                                                            occtl_vector3_t* const     theOutD2UV,
                                                            occtl_vector3_t* const     theOutD3U,
                                                            occtl_vector3_t* const     theOutD3V,
                                                            occtl_vector3_t* const     theOutD3UUV,
                                                            occtl_vector3_t* const     theOutD3UVV)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutP == nullptr || theOutD1U == nullptr || theOutD1V == nullptr || theOutD2U == nullptr
        || theOutD2V == nullptr || theOutD2UV == nullptr || theOutD3U == nullptr
        || theOutD3V == nullptr || theOutD3UUV == nullptr || theOutD3UVV == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "an out-param is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedSurface anAdaptor =
      BRepGraph_Tool::Face::SurfaceAdaptor(theGraph->graph, aFaceId);
    const Geom_Surface::ResD3 aRes = anAdaptor.EvalD3(theU, theV);
    theOutP->x                     = aRes.Point.X();
    theOutP->y                     = aRes.Point.Y();
    theOutP->z                     = aRes.Point.Z();
    theOutD1U->x                   = aRes.D1U.X();
    theOutD1U->y                   = aRes.D1U.Y();
    theOutD1U->z                   = aRes.D1U.Z();
    theOutD1V->x                   = aRes.D1V.X();
    theOutD1V->y                   = aRes.D1V.Y();
    theOutD1V->z                   = aRes.D1V.Z();
    theOutD2U->x                   = aRes.D2U.X();
    theOutD2U->y                   = aRes.D2U.Y();
    theOutD2U->z                   = aRes.D2U.Z();
    theOutD2V->x                   = aRes.D2V.X();
    theOutD2V->y                   = aRes.D2V.Y();
    theOutD2V->z                   = aRes.D2V.Z();
    theOutD2UV->x                  = aRes.D2UV.X();
    theOutD2UV->y                  = aRes.D2UV.Y();
    theOutD2UV->z                  = aRes.D2UV.Z();
    theOutD3U->x                   = aRes.D3U.X();
    theOutD3U->y                   = aRes.D3U.Y();
    theOutD3U->z                   = aRes.D3U.Z();
    theOutD3V->x                   = aRes.D3V.X();
    theOutD3V->y                   = aRes.D3V.Y();
    theOutD3V->z                   = aRes.D3V.Z();
    theOutD3UUV->x                 = aRes.D3UUV.X();
    theOutD3UUV->y                 = aRes.D3UUV.Y();
    theOutD3UUV->z                 = aRes.D3UUV.Z();
    theOutD3UVV->x                 = aRes.D3UVV.X();
    theOutD3UVV->y                 = aRes.D3UVV.Y();
    theOutD3UVV->z                 = aRes.D3UVV.Z();
    return OCCTL_OK;
  });
}

} // extern "C"
