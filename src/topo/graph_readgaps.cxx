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
#include "TopoMath.hxx"

#include <BRepGraphInc_Definition.hxx>
#include <BRepGraphInc_Instance.hxx>
#include <BRepGraph_ChildExplorer.hxx>
#include <BRepGraph_MeshView.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_TopoView.hxx>
#include <GeomAdaptor_TransformedSurface.hxx>

#include <occtl/occtl_geom.h>
#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include "../geom/GeomMath.hxx"

#include <GeomAbs_Shape.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopLoc_Location.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <optional>

namespace
{

bool isActiveNode(const BRepGraph& theGraph, const BRepGraph_NodeId theNode)
{
  return theNode.IsValid() && !theGraph.Topo().Gen().IsRemoved(theNode);
}

bool findVertexUsageUnderParent(const BRepGraph&              theGraph,
                                const BRepGraph_VertexId      theVertex,
                                const BRepGraph_NodeId        theParent,
                                BRepGraphInc::VertexInstance& theOutUsage)
{
  if (theParent == BRepGraph_NodeId(theVertex))
  {
    theOutUsage = BRepGraphInc::VertexInstance{theVertex, TopLoc_Location(), TopAbs_FORWARD};
    return true;
  }

  BRepGraph_ChildExplorer::Config aConfig = {BRepGraph_ChildExplorer::TraversalMode::Recursive,
                                             BRepGraph_NodeId::Kind::Vertex,
                                             std::nullopt,
                                             false,
                                             true,
                                             true,
                                             TopLoc_Location(),
                                             TopAbs_FORWARD};

  for (BRepGraph_ChildExplorer anExplorer(theGraph, theParent, aConfig); anExplorer.More();
       anExplorer.Next())
  {
    const BRepGraphInc::NodeInstance aInst = anExplorer.Current();
    if (aInst.DefId != BRepGraph_NodeId(theVertex))
    {
      continue;
    }

    theOutUsage = BRepGraphInc::VertexInstance{theVertex, aInst.Location, aInst.Orientation};
    return true;
  }

  return false;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_continuity(const occtl_graph_t* const      theGraph,
                             const occtl_node_id_t           theEdge,
                             const occtl_node_id_t           theFaceA,
                             const occtl_node_id_t           theFaceB,
                             occtl_shape_continuity_t* const theOutContinuity)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutContinuity == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_continuity is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    BRepGraph_FaceId aFaceAId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFaceA, BRepGraph_NodeId::Kind::Face, aFaceAId))
    {
      return aStatus;
    }

    BRepGraph_FaceId aFaceBId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFaceB, BRepGraph_NodeId::Kind::Face, aFaceBId))
    {
      return aStatus;
    }

    const GeomAbs_Shape aCont =
      BRepGraph_Tool::Edge::Continuity(theGraph->graph, anEdgeId, aFaceAId, aFaceBId);
    *theOutContinuity = static_cast<occtl_shape_continuity_t>(aCont);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_has_continuity(const occtl_graph_t* const theGraph,
                                 const occtl_node_id_t      theEdge,
                                 const occtl_node_id_t      theFaceA,
                                 const occtl_node_id_t      theFaceB,
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

    BRepGraph_FaceId aFaceAId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFaceA, BRepGraph_NodeId::Kind::Face, aFaceAId))
    {
      return aStatus;
    }

    BRepGraph_FaceId aFaceBId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFaceB, BRepGraph_NodeId::Kind::Face, aFaceBId))
    {
      return aStatus;
    }

    *theOutFlag =
      BRepGraph_Tool::Edge::HasContinuity(theGraph->graph, anEdgeId, aFaceAId, aFaceBId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_max_continuity(const occtl_graph_t* const      theGraph,
                                 const occtl_node_id_t           theEdge,
                                 occtl_shape_continuity_t* const theOutContinuity)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutContinuity == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_continuity is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdgeId))
    {
      return aStatus;
    }

    const GeomAbs_Shape aCont = BRepGraph_Tool::Edge::MaxContinuity(theGraph->graph, anEdgeId);
    *theOutContinuity         = static_cast<occtl_shape_continuity_t>(aCont);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_orientation(const occtl_graph_t* const theGraph,
                                const occtl_node_id_t      theCoedge,
                                occtl_orientation_t* const theOutOrientation)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutOrientation == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_orientation is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aStatus;
    }

    const TopAbs_Orientation anOri =
      BRepGraph_Tool::CoEdge::Orientation(theGraph->graph, aCoEdgeId);
    *theOutOrientation = OcctL::Topo::FromOcctOrientation(anOri);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_find_coedge_on_face(const occtl_graph_t* const theGraph,
                                      const occtl_node_id_t      theEdge,
                                      const occtl_node_id_t      theFace,
                                      occtl_node_id_t* const     theOutCoedge)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCoedge == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_coedge is NULL");
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

    const BRepGraph_CoEdgeId aCoId = theGraph->graph.Topo().Edges().FindCoEdgeId(anEdgeId, aFaceId);
    if (!aCoId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "no coedge found for edge-face pair");
      return OCCTL_NOT_FOUND;
    }
    *theOutCoedge = OcctL::Topo::PackNodeId(aCoId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_find_coedge_on_face_oriented(const occtl_graph_t* const theGraph,
                                               const occtl_node_id_t      theEdge,
                                               const occtl_node_id_t      theFace,
                                               const occtl_orientation_t  theOrientation,
                                               occtl_node_id_t* const     theOutCoedge)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCoedge == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_coedge is NULL");
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

    const TopAbs_Orientation anOri = OcctL::Topo::ToOcctOrientation(theOrientation);
    const BRepGraph_CoEdgeId aCoId =
      theGraph->graph.Topo().Edges().FindCoEdgeId(anEdgeId, aFaceId, anOri);
    if (!aCoId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "no coedge found for edge-face-orientation triple");
      return OCCTL_NOT_FOUND;
    }
    *theOutCoedge = OcctL::Topo::PackNodeId(aCoId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_vertex_point_in_usage(const occtl_graph_t* const theGraph,
                                   const occtl_node_id_t      theVertex,
                                   const occtl_node_id_t      theParent,
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

    const BRepGraph_NodeId aParentNodeId = OcctL::Topo::UnpackNodeId(theParent);
    if (!isActiveNode(theGraph->graph, aParentNodeId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "parent NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    BRepGraphInc::VertexInstance aUsage;
    if (!findVertexUsageUnderParent(theGraph->graph, aVertId, aParentNodeId, aUsage))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "parent does not reference vertex");
      return OCCTL_NOT_FOUND;
    }

    const gp_Pnt aPnt = BRepGraph_Tool::Vertex::Pnt(theGraph->graph, aUsage);
    theOutPoint->x    = aPnt.X();
    theOutPoint->y    = aPnt.Y();
    theOutPoint->z    = aPnt.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_vertex_pcurve_parameter(const occtl_graph_t* const theGraph,
                                     const occtl_node_id_t      theVertex,
                                     const occtl_node_id_t      theCoedge,
                                     double* const              theOutU)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutU == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_u is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_VertexId aVertId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theVertex, BRepGraph_NodeId::Kind::Vertex, aVertId))
    {
      return aStatus;
    }

    BRepGraph_CoEdgeId aCoId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theCoedge, BRepGraph_NodeId::Kind::CoEdge, aCoId))
    {
      return aStatus;
    }

    *theOutU = BRepGraph_Tool::Vertex::PCurveParameter(theGraph->graph, aVertId, aCoId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edge_has_polygon3d(const occtl_graph_t* const theGraph,
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

    *theOutFlag = theGraph->graph.Mesh().Effective().Edges().Has(anEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_coedge_has_polygon_on_surface(const occtl_graph_t* const theGraph,
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

    *theOutFlag = theGraph->graph.Mesh().Effective().CoEdges().Has(aCoEdgeId) ? 1 : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_uv_bounds_restricted(const occtl_graph_t* const theGraph,
                                       const occtl_node_id_t      theFace,
                                       const double               theUMin,
                                       const double               theUMax,
                                       const double               theVMin,
                                       const double               theVMax,
                                       const double               theU,
                                       const double               theV,
                                       occtl_point3_t* const      theOutPoint,
                                       occtl_vector3_t* const     theOutD1U,
                                       occtl_vector3_t* const     theOutD1V)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutPoint == nullptr || theOutD1U == nullptr || theOutD1V == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_point, out_d1u, or out_d1v is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    GeomAdaptor_TransformedSurface anAdaptor = BRepGraph_Tool::Face::SurfaceAdaptor(theGraph->graph,
                                                                                    aFaceId,
                                                                                    theUMin,
                                                                                    theUMax,
                                                                                    theVMin,
                                                                                    theVMax);
    const Geom_Surface::ResD1      aRes      = anAdaptor.EvalD1(theU, theV);
    theOutPoint->x                           = aRes.Point.X();
    theOutPoint->y                           = aRes.Point.Y();
    theOutPoint->z                           = aRes.Point.Z();
    theOutD1U->x                             = aRes.D1U.X();
    theOutD1U->y                             = aRes.D1U.Y();
    theOutD1U->z                             = aRes.D1U.Z();
    theOutD1V->x                             = aRes.D1V.X();
    theOutD1V->y                             = aRes.D1V.Y();
    theOutD1V->z                             = aRes.D1V.Z();
    return OCCTL_OK;
  });
}

//==================================================================================================

} // extern "C"
