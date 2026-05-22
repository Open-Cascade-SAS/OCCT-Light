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

#include <BRepGraphInc_Definition.hxx>
#include <BRepGraph_MeshView.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_TopoView.hxx>

#include <gp_Pnt.hxx>

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_edge_view_init(occtl_edge_view_t* const theView)
{
  if (theView == nullptr)
  {
    return;
  }
  const occtl_edge_view_t aInit = OCCTL_EDGE_VIEW_INIT;
  *theView                      = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_edge_view(const occtl_graph_t* const theGraph,
                                                         const occtl_node_id_t      theEdge,
                                                         occtl_edge_view_t* const   theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "view is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theView->struct_version != OCCTL_EDGE_VIEW_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_edge_view_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theView->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_edge_view_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aErr;
    }

    const BRepGraph& aGraph = theGraph->graph;

    const std::pair<double, double> aRange = BRepGraph_Tool::Edge::Range(aGraph, anEdgeId);
    theView->t_min                         = aRange.first;
    theView->t_max                         = aRange.second;
    theView->tolerance                     = BRepGraph_Tool::Edge::Tolerance(aGraph, anEdgeId);
    theView->start_vertex =
      OcctL::Topo::PackNodeId(BRepGraph_Tool::Edge::StartVertexId(aGraph, anEdgeId));
    theView->end_vertex =
      OcctL::Topo::PackNodeId(BRepGraph_Tool::Edge::EndVertexId(aGraph, anEdgeId));

    const BRepGraphInc::EdgeDef& aDef = aGraph.Topo().Edges().Definition(anEdgeId);
    theView->internal_vertex_count    = static_cast<uint32_t>(aDef.InternalVertexRefIds.Size());

    theView->face_count     = BRepGraph_Tool::Edge::NbFaces(aGraph, anEdgeId);
    theView->has_curve      = BRepGraph_Tool::Edge::HasCurve(aGraph, anEdgeId) ? 1 : 0;
    theView->is_degenerated = BRepGraph_Tool::Edge::Degenerated(aGraph, anEdgeId) ? 1 : 0;
    theView->is_closed      = BRepGraph_Tool::Edge::IsClosed(aGraph, anEdgeId) ? 1 : 0;
    theView->same_parameter = BRepGraph_Tool::Edge::SameParameter(aGraph, anEdgeId) ? 1 : 0;
    theView->same_range     = BRepGraph_Tool::Edge::SameRange(aGraph, anEdgeId) ? 1 : 0;
    theView->is_manifold    = BRepGraph_Tool::Edge::IsManifold(aGraph, anEdgeId) ? 1 : 0;
    theView->is_boundary    = BRepGraph_Tool::Edge::IsBoundary(aGraph, anEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_coedge_view_init(occtl_coedge_view_t* const theView)
{
  if (theView == nullptr)
  {
    return;
  }
  const occtl_coedge_view_t aInit = OCCTL_COEDGE_VIEW_INIT;
  *theView                        = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_coedge_view(const occtl_graph_t* const theGraph,
                                                           const occtl_node_id_t      theCoEdge,
                                                           occtl_coedge_view_t* const theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "view is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theView->struct_version != OCCTL_COEDGE_VIEW_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_coedge_view_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theView->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_coedge_view_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theCoEdge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aErr;
    }

    const BRepGraph& aGraph = theGraph->graph;
    theView->edge_of = OcctL::Topo::PackNodeId(BRepGraph_Tool::CoEdge::EdgeOf(aGraph, aCoEdgeId));
    theView->face_of = OcctL::Topo::PackNodeId(BRepGraph_Tool::CoEdge::FaceOf(aGraph, aCoEdgeId));
    const TopAbs_Orientation aOri = BRepGraph_Tool::CoEdge::Orientation(aGraph, aCoEdgeId);
    theView->orientation          = static_cast<occtl_orientation_t>(aOri);

    const std::pair<double, double> aRange = BRepGraph_Tool::CoEdge::Range(aGraph, aCoEdgeId);
    theView->t_min                         = aRange.first;
    theView->t_max                         = aRange.second;

    theView->has_pcurve = BRepGraph_Tool::CoEdge::HasPCurve(aGraph, aCoEdgeId) ? 1 : 0;
    if (theView->has_pcurve)
    {
      const std::pair<gp_Pnt2d, gp_Pnt2d> anUV =
        BRepGraph_Tool::CoEdge::UVPoints(aGraph, aCoEdgeId);
      theView->uv_start = {anUV.first.X(), anUV.first.Y()};
      theView->uv_end   = {anUV.second.X(), anUV.second.Y()};
    }

    theView->is_seam     = BRepGraph_Tool::CoEdge::IsSeam(aGraph, aCoEdgeId) ? 1 : 0;
    theView->is_reversed = BRepGraph_Tool::CoEdge::IsReversed(aGraph, aCoEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_face_view_init(occtl_face_view_t* const theView)
{
  if (theView == nullptr)
  {
    return;
  }
  const occtl_face_view_t aInit = OCCTL_FACE_VIEW_INIT;
  *theView                      = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_face_view(const occtl_graph_t* const theGraph,
                                                         const occtl_node_id_t      theFace,
                                                         occtl_face_view_t* const   theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "view is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theView->struct_version != OCCTL_FACE_VIEW_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_face_view_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theView->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_face_view_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aErr;
    }

    const BRepGraph& aGraph = theGraph->graph;
    BRepGraph_Tool::Face::Bounds(aGraph,
                                 aFaceId,
                                 theView->u_min,
                                 theView->u_max,
                                 theView->v_min,
                                 theView->v_max);
    theView->tolerance = BRepGraph_Tool::Face::Tolerance(aGraph, aFaceId);
    theView->outer_wire =
      OcctL::Topo::PackNodeId(BRepGraph_Tool::Face::OuterWireId(aGraph, aFaceId));
    theView->wire_count        = BRepGraph_Tool::Face::NbWires(aGraph, aFaceId);
    theView->has_surface       = BRepGraph_Tool::Face::HasSurface(aGraph, aFaceId) ? 1 : 0;
    theView->has_triangulation = aGraph.Mesh().Effective().Faces().Has(aFaceId) ? 1 : 0;
    theView->natural_restriction =
      BRepGraph_Tool::Face::NaturalRestriction(aGraph, aFaceId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_vertex_view_init(occtl_vertex_view_t* const theView)
{
  if (theView == nullptr)
  {
    return;
  }
  const occtl_vertex_view_t aInit = OCCTL_VERTEX_VIEW_INIT;
  *theView                        = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_vertex_view(const occtl_graph_t* const theGraph,
                                                           const occtl_node_id_t      theVertex,
                                                           occtl_vertex_view_t* const theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "view is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theView->struct_version != OCCTL_VERTEX_VIEW_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_vertex_view_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theView->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_vertex_view_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_VertexId aVertexId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertexId))
    {
      return aErr;
    }

    const BRepGraph& aGraph = theGraph->graph;
    const gp_Pnt     aPnt   = BRepGraph_Tool::Vertex::Pnt(aGraph, aVertexId);
    theView->point          = {aPnt.X(), aPnt.Y(), aPnt.Z()};
    theView->tolerance      = BRepGraph_Tool::Vertex::Tolerance(aGraph, aVertexId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_wire_view_init(occtl_wire_view_t* const theView)
{
  if (theView == nullptr)
  {
    return;
  }
  const occtl_wire_view_t aInit = OCCTL_WIRE_VIEW_INIT;
  *theView                      = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_view(const occtl_graph_t* const theGraph,
                                                         const occtl_node_id_t      theWire,
                                                         occtl_wire_view_t* const   theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "view is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theView->struct_version != OCCTL_WIRE_VIEW_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_wire_view_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theView->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_wire_view_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theWire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aErr;
    }

    const BRepGraph& aGraph      = theGraph->graph;
    theView->is_closed           = BRepGraph_Tool::Wire::IsClosed(aGraph, aWireId) ? 1 : 0;
    theView->coedge_count        = BRepGraph_Tool::Wire::NbCoEdges(aGraph, aWireId);
    theView->distinct_edge_count = BRepGraph_Tool::Wire::NbDistinctEdges(aGraph, aWireId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_shell_view_init(occtl_shell_view_t* const theView)
{
  if (theView == nullptr)
  {
    return;
  }
  const occtl_shell_view_t aInit = OCCTL_SHELL_VIEW_INIT;
  *theView                       = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_shell_view(const occtl_graph_t* const theGraph,
                                                          const occtl_node_id_t      theShell,
                                                          occtl_shell_view_t* const  theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "view is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theView->struct_version != OCCTL_SHELL_VIEW_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_shell_view_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theView->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_shell_view_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_ShellId aShellId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theShell, BRepGraph_NodeId::Kind::Shell, aShellId))
    {
      return aErr;
    }

    const BRepGraph& aGraph = theGraph->graph;
    theView->is_closed      = BRepGraph_Tool::Shell::IsClosed(aGraph, aShellId) ? 1 : 0;
    theView->face_count     = BRepGraph_Tool::Shell::NbFaces(aGraph, aShellId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_solid_view_init(occtl_solid_view_t* const theView)
{
  if (theView == nullptr)
  {
    return;
  }
  const occtl_solid_view_t aInit = OCCTL_SOLID_VIEW_INIT;
  *theView                       = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_solid_view(const occtl_graph_t* const theGraph,
                                                          const occtl_node_id_t      theSolid,
                                                          occtl_solid_view_t* const  theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "view is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theView->struct_version != OCCTL_SOLID_VIEW_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_solid_view_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theView->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_solid_view_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_SolidId aSolidId;
    if (const occtl_status_t aErr =
          OcctL::Topo::ToTypedId(theGraph, theSolid, BRepGraph_NodeId::Kind::Solid, aSolidId))
    {
      return aErr;
    }

    const BRepGraph& aGraph = theGraph->graph;
    theView->shell_count =
      static_cast<size_t>(aGraph.Topo().Solids().Definition(aSolidId).ShellRefIds.Size());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_compound_view_init(occtl_compound_view_t* const theView)
{
  if (theView == nullptr)
  {
    return;
  }
  const occtl_compound_view_t aInit = OCCTL_COMPOUND_VIEW_INIT;
  *theView                          = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_compound_view(const occtl_graph_t* const theGraph,
                                                             const occtl_node_id_t      theCompound,
                                                             occtl_compound_view_t* const theView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "view is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theView->struct_version != OCCTL_COMPOUND_VIEW_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "occtl_compound_view_t: unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theView->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_compound_view_t::p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CompoundId aCompoundId;
    if (const occtl_status_t aErr = OcctL::Topo::ToTypedId(theGraph,
                                                           theCompound,
                                                           BRepGraph_NodeId::Kind::Compound,
                                                           aCompoundId))
    {
      return aErr;
    }

    const BRepGraph& aGraph = theGraph->graph;
    theView->child_count =
      static_cast<size_t>(aGraph.Topo().Compounds().Definition(aCompoundId).ChildRefIds.Size());
    return OCCTL_OK;
  });
}

} // extern "C"
