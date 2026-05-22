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

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include "../geom/RepLookup.hxx"

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepFilletAPI_MakeFillet2d.hxx>
#include <BRepGProp.hxx>
#include <BRepGraph_EditorView.hxx>
#include <BRepGraph_RefsIterator.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_WireExplorer.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>

#include <GeomAbs_JoinType.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_DynamicArray.hxx>
#include <NCollection_IndexedMap.hxx>
#include <NCollection_LinearVector.hxx>
#include <ChFi2d.hxx>
#include <ChFi2d_ConstructionError.hxx>
#include <GProp_GProps.hxx>

#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_ShapeMapHasher.hxx>

#include <gp_Pnt.hxx>
#include <Precision.hxx>

#include <cmath>
#include <utility>

namespace
{

bool IsFiniteValue(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

struct EdgeWireRecord
{
  BRepGraph_EdgeId   EdgeId;
  occtl_node_id_t    AbiId;
  BRepGraph_VertexId StartId;
  BRepGraph_VertexId EndId;
  gp_Pnt             StartPoint;
  gp_Pnt             EndPoint;
  bool               Used = false;
};

struct WireBuildRecord
{
  NCollection_LinearVector<occtl_oriented_node_t> Edges;
  BRepGraph_VertexId                              FirstId;
  BRepGraph_VertexId                              LastId;
  gp_Pnt                                          FirstPoint;
  gp_Pnt                                          LastPoint;
  bool                                            Closed = false;
};

void prependOrientedNode(NCollection_LinearVector<occtl_oriented_node_t>& theNodes,
                         const occtl_oriented_node_t&                     theNode)
{
  NCollection_LinearVector<occtl_oriented_node_t> aPrepended;
  aPrepended.Append(theNode);
  for (size_t anIndex = 0; anIndex < theNodes.Size(); ++anIndex)
  {
    aPrepended.Append(theNodes.Value(anIndex));
  }
  theNodes = aPrepended;
}

bool endpointsMatch(const BRepGraph_VertexId theIdA,
                    const gp_Pnt&            thePointA,
                    const BRepGraph_VertexId theIdB,
                    const gp_Pnt&            thePointB,
                    const double             theTolerance)
{
  if (theIdA == theIdB)
  {
    return true;
  }
  if (theTolerance <= 0.0)
  {
    return false;
  }
  return thePointA.SquareDistance(thePointB) <= theTolerance * theTolerance;
}

bool tryAppendEdge(NCollection_LinearVector<EdgeWireRecord>& theEdges,
                   WireBuildRecord&                          theWire,
                   const double                              theTolerance)
{
  for (EdgeWireRecord& anEdge : theEdges)
  {
    if (anEdge.Used)
    {
      continue;
    }

    if (endpointsMatch(theWire.LastId,
                       theWire.LastPoint,
                       anEdge.StartId,
                       anEdge.StartPoint,
                       theTolerance))
    {
      anEdge.Used = true;
      theWire.Edges.Append({anEdge.AbiId, OCCTL_ORIENTATION_FORWARD});
      theWire.LastId    = anEdge.EndId;
      theWire.LastPoint = anEdge.EndPoint;
      return true;
    }

    if (endpointsMatch(theWire.LastId,
                       theWire.LastPoint,
                       anEdge.EndId,
                       anEdge.EndPoint,
                       theTolerance))
    {
      anEdge.Used = true;
      theWire.Edges.Append({anEdge.AbiId, OCCTL_ORIENTATION_REVERSED});
      theWire.LastId    = anEdge.StartId;
      theWire.LastPoint = anEdge.StartPoint;
      return true;
    }
  }
  return false;
}

bool tryPrependEdge(NCollection_LinearVector<EdgeWireRecord>& theEdges,
                    WireBuildRecord&                          theWire,
                    const double                              theTolerance)
{
  for (EdgeWireRecord& anEdge : theEdges)
  {
    if (anEdge.Used)
    {
      continue;
    }

    if (endpointsMatch(theWire.FirstId,
                       theWire.FirstPoint,
                       anEdge.EndId,
                       anEdge.EndPoint,
                       theTolerance))
    {
      anEdge.Used = true;
      prependOrientedNode(theWire.Edges, {anEdge.AbiId, OCCTL_ORIENTATION_FORWARD});
      theWire.FirstId    = anEdge.StartId;
      theWire.FirstPoint = anEdge.StartPoint;
      return true;
    }

    if (endpointsMatch(theWire.FirstId,
                       theWire.FirstPoint,
                       anEdge.StartId,
                       anEdge.StartPoint,
                       theTolerance))
    {
      anEdge.Used = true;
      prependOrientedNode(theWire.Edges, {anEdge.AbiId, OCCTL_ORIENTATION_REVERSED});
      theWire.FirstId    = anEdge.EndId;
      theWire.FirstPoint = anEdge.EndPoint;
      return true;
    }
  }
  return false;
}

GeomAbs_JoinType toOcctWireOffsetJoin(const occtl_topo_wire_offset_2d_join_t theJoin)
{
  switch (theJoin)
  {
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_TANGENT:
      return GeomAbs_Tangent;
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION:
      return GeomAbs_Intersection;
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC:
      return GeomAbs_Arc;
    case OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_RESERVED_FUTURE:
      break;
  }
  return GeomAbs_Arc;
}

bool isValidWireOffsetJoin(const occtl_topo_wire_offset_2d_join_t theJoin)
{
  return theJoin == OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC
         || theJoin == OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_TANGENT
         || theJoin == OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION;
}

double edgeEndpointDistance(const BRepGraph& theGraph, const BRepGraph_EdgeId theEdge)
{
  const BRepGraph_VertexId aStartId = BRepGraph_Tool::Edge::StartVertexId(theGraph, theEdge);
  const BRepGraph_VertexId anEndId  = BRepGraph_Tool::Edge::EndVertexId(theGraph, theEdge);
  if (!aStartId.IsValid() || !anEndId.IsValid())
  {
    return 0.0;
  }

  return BRepGraph_Tool::Vertex::Pnt(theGraph, aStartId)
    .Distance(BRepGraph_Tool::Vertex::Pnt(theGraph, anEndId));
}

double edgeLengthForDegenerateFix(const BRepGraph& theGraph, const BRepGraph_EdgeId theEdge)
{
  if (BRepGraph_Tool::Edge::Degenerated(theGraph, theEdge))
  {
    return 0.0;
  }

  const TopoDS_Shape anEdgeShape = theGraph.Shapes().Shape(BRepGraph_NodeId(theEdge));
  if (!anEdgeShape.IsNull() && anEdgeShape.ShapeType() == TopAbs_EDGE)
  {
    GProp_GProps aProps;
    BRepGProp::LinearProperties(TopoDS::Edge(anEdgeShape), aProps);
    const double aLength = std::fabs(aProps.Mass());
    if (IsFiniteValue(aLength))
    {
      return aLength;
    }
  }

  return edgeEndpointDistance(theGraph, theEdge);
}

bool wireIsClosedAfterEdits(const BRepGraph& theGraph, const BRepGraph_WireId theWire)
{
  BRepGraph_WireExplorer anExplorer(theGraph, theWire);
  if (!anExplorer.More())
  {
    return false;
  }

  const BRepGraph_CoEdgeId aFirstCoEdge = anExplorer.CurrentCoEdgeId();
  BRepGraph_CoEdgeId       aLastCoEdge  = aFirstCoEdge;
  for (; anExplorer.More(); anExplorer.Next())
  {
    aLastCoEdge = anExplorer.CurrentCoEdgeId();
  }

  const BRepGraph_EdgeId aFirstEdge = BRepGraph_Tool::CoEdge::EdgeOf(theGraph, aFirstCoEdge);
  const BRepGraph_EdgeId aLastEdge  = BRepGraph_Tool::CoEdge::EdgeOf(theGraph, aLastCoEdge);
  if (!aFirstEdge.IsValid() || !aLastEdge.IsValid())
  {
    return false;
  }

  const TopAbs_Orientation aFirstOrientation =
    BRepGraph_Tool::CoEdge::Orientation(theGraph, aFirstCoEdge);
  const TopAbs_Orientation aLastOrientation =
    BRepGraph_Tool::CoEdge::Orientation(theGraph, aLastCoEdge);
  const BRepGraph_VertexId aFirstStart =
    (aFirstOrientation == TopAbs_FORWARD)
      ? BRepGraph_Tool::Edge::StartVertexId(theGraph, aFirstEdge)
      : BRepGraph_Tool::Edge::EndVertexId(theGraph, aFirstEdge);
  const BRepGraph_VertexId aLastEnd = (aLastOrientation == TopAbs_FORWARD)
                                        ? BRepGraph_Tool::Edge::EndVertexId(theGraph, aLastEdge)
                                        : BRepGraph_Tool::Edge::StartVertexId(theGraph, aLastEdge);
  return aFirstStart.IsValid() && aLastEnd.IsValid() && aFirstStart == aLastEnd;
}

const char* chamfer2dStatusName(const ChFi2d_ConstructionError theStatus)
{
  switch (theStatus)
  {
    case ChFi2d_NotPlanar:
      return "ChFi2d_NotPlanar";
    case ChFi2d_NoFace:
      return "ChFi2d_NoFace";
    case ChFi2d_InitialisationError:
      return "ChFi2d_InitialisationError";
    case ChFi2d_ParametersError:
      return "ChFi2d_ParametersError";
    case ChFi2d_Ready:
      return "ChFi2d_Ready";
    case ChFi2d_IsDone:
      return "ChFi2d_IsDone";
    case ChFi2d_ComputationError:
      return "ChFi2d_ComputationError";
    case ChFi2d_ConnexionError:
      return "ChFi2d_ConnexionError";
    case ChFi2d_TangencyError:
      return "ChFi2d_TangencyError";
    case ChFi2d_FirstEdgeDegenerated:
      return "ChFi2d_FirstEdgeDegenerated";
    case ChFi2d_LastEdgeDegenerated:
      return "ChFi2d_LastEdgeDegenerated";
    case ChFi2d_BothEdgesDegenerated:
      return "ChFi2d_BothEdgesDegenerated";
    case ChFi2d_NotAuthorized:
      return "ChFi2d_NotAuthorized";
  }
  return "ChFi2d_Unknown";
}

occtl_status_t addChamferAtVertex(BRepFilletAPI_MakeFillet2d& theMaker,
                                  const TopoDS_Face&          theFace,
                                  const TopoDS_Vertex&        theVertex,
                                  const double                theDistance1,
                                  const double                theDistance2)
{
  TopoDS_Edge                    anEdge1;
  TopoDS_Edge                    anEdge2;
  const ChFi2d_ConstructionError aFindStatus =
    ChFi2d::FindConnectedEdges(theFace, theVertex, anEdge1, anEdge2);
  if (aFindStatus != ChFi2d_IsDone)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_GEOMETRY_INVALID,
      "ChFi2d::FindConnectedEdges failed while locating a chamfer vertex");
    return OCCTL_GEOMETRY_INVALID;
  }

  const TopoDS_Edge aChamfer = theMaker.AddChamfer(anEdge1, anEdge2, theDistance1, theDistance2);
  if (aChamfer.IsNull() || theMaker.Status() != ChFi2d_IsDone)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           chamfer2dStatusName(theMaker.Status()));
    return OCCTL_GEOMETRY_INVALID;
  }
  return OCCTL_OK;
}

bool measureFaceArea(const TopoDS_Face& theFace, double& theOutArea)
{
  GProp_GProps aProps;
  BRepGProp::SurfaceProperties(theFace, aProps);
  theOutArea = std::fabs(aProps.Mass());
  return IsFiniteValue(theOutArea);
}

bool measureWireAreaWithOcct(const BRepGraph&       theGraph,
                             const BRepGraph_WireId theWireId,
                             const TopoDS_Wire&     theWire,
                             double&                theOutArea)
{
  try
  {
    OCC_CATCH_SIGNALS;
    BRepBuilderAPI_MakeFace aFaceMaker(theWire, true);
    if (aFaceMaker.IsDone() && measureFaceArea(aFaceMaker.Face(), theOutArea))
    {
      return true;
    }
  }
  catch (const Standard_Failure&)
  {
  }

  BRepBuilderAPI_MakePolygon aPoly;
  bool                       aHasFirst = false;
  for (BRepGraph_WireExplorer anExplorer(theGraph, theWireId); anExplorer.More(); anExplorer.Next())
  {
    const BRepGraph_CoEdgeId aCoEdgeId = anExplorer.CurrentCoEdgeId();
    const BRepGraph_EdgeId   anEdgeId  = BRepGraph_Tool::CoEdge::EdgeOf(theGraph, aCoEdgeId);
    const TopAbs_Orientation anOrientation =
      BRepGraph_Tool::CoEdge::Orientation(theGraph, aCoEdgeId);

    BRepGraph_VertexId aFirstVertex = BRepGraph_Tool::Edge::StartVertexId(theGraph, anEdgeId);
    BRepGraph_VertexId aNextVertex  = BRepGraph_Tool::Edge::EndVertexId(theGraph, anEdgeId);
    if (anOrientation == TopAbs_REVERSED)
    {
      aFirstVertex = BRepGraph_Tool::Edge::EndVertexId(theGraph, anEdgeId);
      aNextVertex  = BRepGraph_Tool::Edge::StartVertexId(theGraph, anEdgeId);
    }

    if (!aFirstVertex.IsValid() || !aNextVertex.IsValid())
    {
      return false;
    }

    if (!aHasFirst)
    {
      aPoly.Add(BRepGraph_Tool::Vertex::Pnt(theGraph, aFirstVertex));
      aHasFirst = true;
    }
    aPoly.Add(BRepGraph_Tool::Vertex::Pnt(theGraph, aNextVertex));
  }

  if (!aHasFirst)
  {
    return false;
  }

  aPoly.Close();
  if (!aPoly.IsDone())
  {
    return false;
  }

  try
  {
    OCC_CATCH_SIGNALS;
    BRepBuilderAPI_MakeFace aFaceMaker(aPoly.Wire(), true);
    return aFaceMaker.IsDone() && measureFaceArea(aFaceMaker.Face(), theOutArea);
  }
  catch (const Standard_Failure&)
  {
    return false;
  }
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_make_vertex_info_init(occtl_topo_make_vertex_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_make_edge_info_init(occtl_topo_make_edge_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo                = {};
    theInfo->struct_version = OCCTL_TOPO_MAKE_EDGE_INFO_VERSION_1;
    theInfo->curve          = OCCTL_REP_ID_INVALID;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_make_face_info_init(occtl_topo_make_face_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo                = {};
    theInfo->struct_version = OCCTL_TOPO_MAKE_FACE_INFO_VERSION_1;
    theInfo->surface        = OCCTL_REP_ID_INVALID;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_make_face_from_wires_auto_options_init(
  occtl_topo_make_face_from_wires_auto_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions                = {};
    theOptions->struct_version = OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_VERSION_1;
    theOptions->surface        = OCCTL_REP_ID_INVALID;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_make_wire_info_init(occtl_topo_make_wire_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_edges_to_wires_options_init(occtl_topo_edges_to_wires_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions = OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_wire_offset_2d_options_init(occtl_topo_wire_offset_2d_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions = OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_wire_fix_degenerate_options_init(
  occtl_topo_wire_fix_degenerate_edges_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions = OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_face_chamfer_2d_options_init(occtl_topo_face_chamfer_2d_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions = OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_wire_chamfer_2d_options_init(occtl_topo_wire_chamfer_2d_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions = OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_INIT;
  }
}

OCCTL_API void OCCTL_CALL
  occtl_topo_make_shell_info_init(occtl_topo_make_shell_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_TOPO_MAKE_SHELL_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_make_solid_info_init(occtl_topo_make_solid_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_TOPO_MAKE_SOLID_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_make_compound_info_init(occtl_topo_make_compound_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_TOPO_MAKE_COMPOUND_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_make_compsolid_info_init(occtl_topo_make_compsolid_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_TOPO_MAKE_COMPSOLID_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_vertex(occtl_graph_t* const                       theGraph,
                         const occtl_topo_make_vertex_info_t* const theInfo,
                         occtl_node_id_t* const                     theOutVertex)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutVertex == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_vertex is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->struct_version != OCCTL_TOPO_MAKE_VERTEX_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_TOPO_MAKE_VERTEX_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    const gp_Pnt aPnt(theInfo->point.x, theInfo->point.y, theInfo->point.z);

    const BRepGraph_VertexId aVertId =
      theGraph->graph.Editor().Vertices().Add(aPnt, theInfo->tolerance);
    *theOutVertex = OcctL::Topo::PackNodeId(aVertId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_edge(occtl_graph_t* const                     theGraph,
                       const occtl_topo_make_edge_info_t* const theInfo,
                       occtl_node_id_t* const                   theOutEdge)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutEdge == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_edge is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->struct_version != OCCTL_TOPO_MAKE_EDGE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_TOPO_MAKE_EDGE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    BRepGraph_VertexId aStart;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theInfo->start_vertex,
                                                              BRepGraph_NodeId::Kind::Vertex,
                                                              aStart))
    {
      return aStatus;
    }

    BRepGraph_VertexId anEnd;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theInfo->end_vertex,
                                                              BRepGraph_NodeId::Kind::Vertex,
                                                              anEnd))
    {
      return aStatus;
    }

    occ::handle<Geom_Curve> aCurve;
    if (theInfo->curve.bits != 0)
    {
      aCurve = OcctL::Geom::CurveFromRep(theGraph, theInfo->curve);
    }

    const BRepGraph_EdgeId anEdgeId = theGraph->graph.Editor().Edges().Add(aStart,
                                                                           anEnd,
                                                                           aCurve,
                                                                           theInfo->first,
                                                                           theInfo->last,
                                                                           theInfo->tolerance);
    *theOutEdge                     = OcctL::Topo::PackNodeId(anEdgeId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_wire(occtl_graph_t* const                     theGraph,
                       const occtl_topo_make_wire_info_t* const theInfo,
                       occtl_node_id_t* const                   theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->struct_version != OCCTL_TOPO_MAKE_WIRE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_TOPO_MAKE_WIRE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theInfo->edge_count > 0 && theInfo->edges == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "edges is NULL when edge_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->edge_count == 0 && theInfo->edges != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "edges is non-NULL when edge_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_DynamicArray<std::pair<BRepGraph_EdgeId, TopAbs_Orientation>> aPairs;
    for (size_t anI = 0; anI < theInfo->edge_count; ++anI)
    {
      BRepGraph_EdgeId anEdgeId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theInfo->edges[anI].id,
                                                                BRepGraph_NodeId::Kind::Edge,
                                                                anEdgeId))
      {
        return aStatus;
      }

      aPairs.Append(
        std::make_pair(anEdgeId, OcctL::Topo::ToOcctOrientation(theInfo->edges[anI].orientation)));
    }

    const BRepGraph_WireId aWireId = theGraph->graph.Editor().Wires().Add(aPairs);
    *theOutWire                    = OcctL::Topo::PackNodeId(aWireId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_edges_to_wires(occtl_graph_t* const                             theGraph,
                            const occtl_topo_edges_to_wires_options_t* const theOptions,
                            occtl_node_id_t* const                           theOutWires,
                            const size_t                                     theCap,
                            size_t* const                                    theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOptions == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, or out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->struct_version != OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "options->struct_version is not OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theOptions->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "options->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->edge_count > 0 && theOptions->edges == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "edges is NULL when edge_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->edge_count == 0 && theOptions->edges != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "edges is non-NULL when edge_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!IsFiniteValue(theOptions->tolerance) || theOptions->tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tolerance must be finite and non-negative");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_LinearVector<EdgeWireRecord> anEdges;
    anEdges.Reserve(theOptions->edge_count);
    for (size_t anI = 0; anI < theOptions->edge_count; ++anI)
    {
      for (size_t aPrev = 0; aPrev < anI; ++aPrev)
      {
        if (theOptions->edges[aPrev].bits == theOptions->edges[anI].bits)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "duplicate edge id");
          return OCCTL_INVALID_ARGUMENT;
        }
      }

      BRepGraph_EdgeId anEdgeId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theOptions->edges[anI],
                                                                BRepGraph_NodeId::Kind::Edge,
                                                                anEdgeId))
      {
        return aStatus;
      }

      const BRepGraph_VertexId aStartId =
        BRepGraph_Tool::Edge::StartVertexId(theGraph->graph, anEdgeId);
      const BRepGraph_VertexId anEndId =
        BRepGraph_Tool::Edge::EndVertexId(theGraph->graph, anEdgeId);
      if (!aStartId.IsValid() || !anEndId.IsValid())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR, "edge has invalid endpoint vertex");
        return OCCTL_ERROR;
      }

      anEdges.Append({anEdgeId,
                      theOptions->edges[anI],
                      aStartId,
                      anEndId,
                      BRepGraph_Tool::Vertex::Pnt(theGraph->graph, aStartId),
                      BRepGraph_Tool::Vertex::Pnt(theGraph->graph, anEndId),
                      false});
    }

    NCollection_LinearVector<WireBuildRecord> aWires;
    for (size_t anI = 0; anI < anEdges.Size(); ++anI)
    {
      EdgeWireRecord& aSeed = anEdges.ChangeValue(anI);
      if (aSeed.Used)
      {
        continue;
      }

      aSeed.Used = true;
      WireBuildRecord aWire;
      aWire.Edges.Append({aSeed.AbiId, OCCTL_ORIENTATION_FORWARD});
      aWire.FirstId    = aSeed.StartId;
      aWire.LastId     = aSeed.EndId;
      aWire.FirstPoint = aSeed.StartPoint;
      aWire.LastPoint  = aSeed.EndPoint;

      bool aChanged = true;
      while (aChanged)
      {
        aChanged = tryAppendEdge(anEdges, aWire, theOptions->tolerance);
        if (!aChanged)
        {
          aChanged = tryPrependEdge(anEdges, aWire, theOptions->tolerance);
        }
      }

      aWire.Closed = endpointsMatch(aWire.FirstId,
                                    aWire.FirstPoint,
                                    aWire.LastId,
                                    aWire.LastPoint,
                                    theOptions->tolerance);
      if (!aWire.Closed && theOptions->allow_open == 0)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                               "input edges produce an open wire");
        return OCCTL_INVALID_ARGUMENT;
      }
      aWires.Append(std::move(aWire));
    }

    *theOutCount = aWires.Size();
    if (theOutWires != nullptr && theCap < aWires.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "out_wires capacity is too small");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    if (theOutWires != nullptr)
    {
      for (size_t anI = 0; anI < aWires.Size(); ++anI)
      {
        NCollection_DynamicArray<std::pair<BRepGraph_EdgeId, TopAbs_Orientation>> aPairs;
        const WireBuildRecord& aWire = aWires.Value(anI);
        for (size_t anEntryIndex = 0; anEntryIndex < aWire.Edges.Size(); ++anEntryIndex)
        {
          const occtl_oriented_node_t anEntry = aWire.Edges.Value(anEntryIndex);
          BRepGraph_EdgeId            anEdgeId;
          if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                    anEntry.id,
                                                                    BRepGraph_NodeId::Kind::Edge,
                                                                    anEdgeId))
          {
            return aStatus;
          }
          aPairs.Append(
            std::make_pair(anEdgeId, OcctL::Topo::ToOcctOrientation(anEntry.orientation)));
        }
        theOutWires[anI] = OcctL::Topo::PackNodeId(theGraph->graph.Editor().Wires().Add(aPairs));
      }
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wire_offset_2d(occtl_graph_t* const                             theGraph,
                            const occtl_topo_wire_offset_2d_options_t* const theOptions,
                            occtl_node_id_t* const                           theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOptions == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutWire = OCCTL_NODE_ID_INVALID;

    if (theOptions->struct_version != OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "options->struct_version is not OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theOptions->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "options->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!IsFiniteValue(theOptions->distance) || theOptions->distance == 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "distance must be finite and non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!isValidWireOffsetJoin(theOptions->join))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "unsupported wire offset join type");
      return OCCTL_INVALID_ARGUMENT;
    }

    if ((theOptions->open_result != 0 && theOptions->open_result != 1)
        || (theOptions->approximate != 0 && theOptions->approximate != 1))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "open_result and approximate must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theOptions->wire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aStatus;
    }

    const TopoDS_Shape aWireShape = theGraph->graph.Shapes().Shape(BRepGraph_NodeId(aWireId));
    if (aWireShape.IsNull() || aWireShape.ShapeType() != TopAbs_WIRE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "wire could not be reconstructed as TopoDS_Wire");
      return OCCTL_NOT_FOUND;
    }

    BRepOffsetAPI_MakeOffset aMaker(TopoDS::Wire(aWireShape),
                                    toOcctWireOffsetJoin(theOptions->join),
                                    theOptions->open_result != 0);
    aMaker.SetApprox(theOptions->approximate != 0);
    aMaker.Perform(theOptions->distance, 0.0);
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepOffsetAPI_MakeOffset reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }

    const TopoDS_Shape aResult = aMaker.Shape();
    if (aResult.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "wire offset result shape is null");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepGraph::ShapesView::Options aBuildOptions;
    aBuildOptions.CreateAutoProduct = false;
    const BRepGraph::ShapesView::Result aBuildResult =
      theGraph->graph.Shapes().Add(aResult, aBuildOptions);
    if (!aBuildResult.Ok || !aBuildResult.TopologyRoot.IsValid()
        || aBuildResult.TopologyRoot.NodeKind != BRepGraph_NodeId::Kind::Wire)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "wire offset result could not be ingested as graph Wire");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutWire = OcctL::Topo::PackNodeId(aBuildResult.TopologyRoot);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_wire_fix_degenerate(
  occtl_graph_t* const                                        theGraph,
  const occtl_topo_wire_fix_degenerate_edges_options_t* const theOptions,
  size_t* const                                               theOutRemoved)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOptions == nullptr || theOutRemoved == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, or out_removed is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutRemoved = 0;

    if (theOptions->struct_version != OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "options->struct_version is not OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theOptions->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "options->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!IsFiniteValue(theOptions->min_length) || theOptions->min_length < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "min_length must be finite and non-negative");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theOptions->wire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aStatus;
    }

    NCollection_LinearVector<BRepGraph_CoEdgeRefId> aRefsToRemove;
    for (BRepGraph_RefsCoEdgeOfWire anIter(theGraph->graph, aWireId); anIter.More(); anIter.Next())
    {
      const BRepGraph_CoEdgeRefId    aRefId = anIter.CurrentId();
      const BRepGraphInc::CoEdgeRef& aRef   = theGraph->graph.Refs().CoEdges().Entry(aRefId);
      if (!aRef.CoEdgeDefId.IsValid())
      {
        continue;
      }

      const BRepGraph_EdgeId anEdgeId =
        BRepGraph_Tool::CoEdge::EdgeOf(theGraph->graph, aRef.CoEdgeDefId);
      if (!anEdgeId.IsValid())
      {
        continue;
      }

      if (edgeLengthForDegenerateFix(theGraph->graph, anEdgeId) <= theOptions->min_length)
      {
        aRefsToRemove.Append(aRefId);
      }
    }

    BRepGraph::EditorView anEditor = theGraph->graph.Editor();
    for (size_t aRefIndex = 0; aRefIndex < aRefsToRemove.Size(); ++aRefIndex)
    {
      const BRepGraph_CoEdgeRefId aRefId = aRefsToRemove.Value(aRefIndex);
      if (anEditor.Wires().RemoveCoEdge(aWireId, aRefId))
      {
        ++(*theOutRemoved);
      }
    }

    if (*theOutRemoved != 0)
    {
      anEditor.Wires().SetIsClosed(aWireId, wireIsClosedAfterEdits(theGraph->graph, aWireId));
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_face_chamfer_2d(occtl_graph_t* const                              theGraph,
                             const occtl_topo_face_chamfer_2d_options_t* const theOptions,
                             occtl_node_id_t* const                            theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOptions == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutFace = OCCTL_NODE_ID_INVALID;

    if (theOptions->struct_version != OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "options->struct_version is not OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theOptions->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "options->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->vertex_count > 0 && theOptions->vertices == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "vertices is NULL when vertex_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->vertex_count == 0 && theOptions->vertices != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "vertices is non-NULL when vertex_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!IsFiniteValue(theOptions->distance1) || !IsFiniteValue(theOptions->distance2)
        || theOptions->distance1 <= 0.0 || theOptions->distance2 <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "distance1 and distance2 must be finite and positive");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theOptions->face, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aStatus;
    }

    const TopoDS_Shape aFaceShape = theGraph->graph.Shapes().Shape(BRepGraph_NodeId(aFaceId));
    if (aFaceShape.IsNull() || aFaceShape.ShapeType() != TopAbs_FACE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "face could not be reconstructed as TopoDS_Face");
      return OCCTL_NOT_FOUND;
    }

    const TopoDS_Face&         aFace = TopoDS::Face(aFaceShape);
    BRepFilletAPI_MakeFillet2d aMaker(aFace);

    if (theOptions->vertex_count == 0)
    {
      NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> aVertexMap;
      TopExp::MapShapes(aFace, TopAbs_VERTEX, aVertexMap);
      if (aVertexMap.Extent() == 0)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "face has no vertices to chamfer");
        return OCCTL_GEOMETRY_INVALID;
      }

      for (int anI = 1; anI <= aVertexMap.Extent(); ++anI)
      {
        const TopoDS_Vertex& aVertex = TopoDS::Vertex(aVertexMap.FindKey(anI));
        if (const occtl_status_t aStatus = addChamferAtVertex(aMaker,
                                                              aFace,
                                                              aVertex,
                                                              theOptions->distance1,
                                                              theOptions->distance2))
        {
          return aStatus;
        }
      }
    }
    else
    {
      for (size_t anI = 0; anI < theOptions->vertex_count; ++anI)
      {
        for (size_t aPrev = 0; aPrev < anI; ++aPrev)
        {
          if (theOptions->vertices[aPrev].bits == theOptions->vertices[anI].bits)
          {
            OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "duplicate vertex id");
            return OCCTL_INVALID_ARGUMENT;
          }
        }

        BRepGraph_VertexId aVertexId;
        if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                  theOptions->vertices[anI],
                                                                  BRepGraph_NodeId::Kind::Vertex,
                                                                  aVertexId))
        {
          return aStatus;
        }

        const TopoDS_Shape aVertexShape =
          theGraph->graph.Shapes().Shape(BRepGraph_NodeId(aVertexId));
        if (aVertexShape.IsNull() || aVertexShape.ShapeType() != TopAbs_VERTEX)
        {
          OcctL::Core::ErrorState::Current().Set(
            OCCTL_NOT_FOUND,
            "vertex could not be reconstructed as TopoDS_Vertex");
          return OCCTL_NOT_FOUND;
        }

        if (const occtl_status_t aStatus = addChamferAtVertex(aMaker,
                                                              aFace,
                                                              TopoDS::Vertex(aVertexShape),
                                                              theOptions->distance1,
                                                              theOptions->distance2))
        {
          return aStatus;
        }
      }
    }

    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             chamfer2dStatusName(aMaker.Status()));
      return OCCTL_GEOMETRY_INVALID;
    }

    const TopoDS_Shape aResult = aMaker.Shape();
    if (aResult.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "face chamfer result shape is null");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepGraph::ShapesView::Options aBuildOptions;
    aBuildOptions.CreateAutoProduct = false;
    const BRepGraph::ShapesView::Result aBuildResult =
      theGraph->graph.Shapes().Add(aResult, aBuildOptions);
    if (!aBuildResult.Ok || !aBuildResult.TopologyRoot.IsValid()
        || aBuildResult.TopologyRoot.NodeKind != BRepGraph_NodeId::Kind::Face)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "face chamfer result could not be ingested as graph Face");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutFace = OcctL::Topo::PackNodeId(aBuildResult.TopologyRoot);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_wire_chamfer_2d(occtl_graph_t* const                              theGraph,
                             const occtl_topo_wire_chamfer_2d_options_t* const theOptions,
                             occtl_node_id_t* const                            theOutWire)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOptions == nullptr || theOutWire == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, or out_wire is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutWire = OCCTL_NODE_ID_INVALID;

    if (theOptions->struct_version != OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "options->struct_version is not OCCTL_TOPO_WIRE_CHAMFER_2D_OPTIONS_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theOptions->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "options->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->vertex_count > 0 && theOptions->vertices == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "vertices is NULL when vertex_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->vertex_count == 0 && theOptions->vertices != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "vertices is non-NULL when vertex_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!IsFiniteValue(theOptions->distance1) || !IsFiniteValue(theOptions->distance2)
        || theOptions->distance1 <= 0.0 || theOptions->distance2 <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "distance1 and distance2 must be finite and positive");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId aWireId;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theOptions->wire, BRepGraph_NodeId::Kind::Wire, aWireId))
    {
      return aStatus;
    }

    const TopoDS_Shape aWireShape = theGraph->graph.Shapes().Shape(BRepGraph_NodeId(aWireId));
    if (aWireShape.IsNull() || aWireShape.ShapeType() != TopAbs_WIRE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "wire could not be reconstructed as TopoDS_Wire");
      return OCCTL_NOT_FOUND;
    }

    BRepBuilderAPI_MakeFace aFaceMaker(TopoDS::Wire(aWireShape), true);
    if (!aFaceMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        "wire could not be converted to a planar support face");
      return OCCTL_GEOMETRY_INVALID;
    }

    const TopoDS_Face          aFace = aFaceMaker.Face();
    BRepFilletAPI_MakeFillet2d aMaker(aFace);

    if (theOptions->vertex_count == 0)
    {
      NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> aVertexMap;
      TopExp::MapShapes(aFace, TopAbs_VERTEX, aVertexMap);
      if (aVertexMap.Extent() == 0)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "wire has no vertices to chamfer");
        return OCCTL_GEOMETRY_INVALID;
      }

      for (int anI = 1; anI <= aVertexMap.Extent(); ++anI)
      {
        const TopoDS_Vertex& aVertex = TopoDS::Vertex(aVertexMap.FindKey(anI));
        if (const occtl_status_t aStatus = addChamferAtVertex(aMaker,
                                                              aFace,
                                                              aVertex,
                                                              theOptions->distance1,
                                                              theOptions->distance2))
        {
          return aStatus;
        }
      }
    }
    else
    {
      for (size_t anI = 0; anI < theOptions->vertex_count; ++anI)
      {
        for (size_t aPrev = 0; aPrev < anI; ++aPrev)
        {
          if (theOptions->vertices[aPrev].bits == theOptions->vertices[anI].bits)
          {
            OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "duplicate vertex id");
            return OCCTL_INVALID_ARGUMENT;
          }
        }

        BRepGraph_VertexId aVertexId;
        if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                  theOptions->vertices[anI],
                                                                  BRepGraph_NodeId::Kind::Vertex,
                                                                  aVertexId))
        {
          return aStatus;
        }

        const TopoDS_Shape aVertexShape =
          theGraph->graph.Shapes().Shape(BRepGraph_NodeId(aVertexId));
        if (aVertexShape.IsNull() || aVertexShape.ShapeType() != TopAbs_VERTEX)
        {
          OcctL::Core::ErrorState::Current().Set(
            OCCTL_NOT_FOUND,
            "vertex could not be reconstructed as TopoDS_Vertex");
          return OCCTL_NOT_FOUND;
        }

        if (const occtl_status_t aStatus = addChamferAtVertex(aMaker,
                                                              aFace,
                                                              TopoDS::Vertex(aVertexShape),
                                                              theOptions->distance1,
                                                              theOptions->distance2))
        {
          return aStatus;
        }
      }
    }

    aMaker.Build();
    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             chamfer2dStatusName(aMaker.Status()));
      return OCCTL_GEOMETRY_INVALID;
    }

    const TopoDS_Shape aResult = aMaker.Shape();
    if (aResult.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "wire chamfer result shape is null");
      return OCCTL_GEOMETRY_INVALID;
    }

    TopExp_Explorer aWireExplorer(aResult, TopAbs_WIRE);
    if (!aWireExplorer.More())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "wire chamfer result has no outer wire");
      return OCCTL_GEOMETRY_INVALID;
    }

    const TopoDS_Shape             aResultWire = aWireExplorer.Current();
    BRepGraph::ShapesView::Options aBuildOptions;
    aBuildOptions.CreateAutoProduct = false;
    const BRepGraph::ShapesView::Result aBuildResult =
      theGraph->graph.Shapes().Add(aResultWire, aBuildOptions);
    if (!aBuildResult.Ok || !aBuildResult.TopologyRoot.IsValid()
        || aBuildResult.TopologyRoot.NodeKind != BRepGraph_NodeId::Kind::Wire)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "wire chamfer result could not be ingested as graph Wire");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutWire = OcctL::Topo::PackNodeId(aBuildResult.TopologyRoot);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_face(occtl_graph_t* const                     theGraph,
                       const occtl_topo_make_face_info_t* const theInfo,
                       occtl_node_id_t* const                   theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->struct_version != OCCTL_TOPO_MAKE_FACE_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_TOPO_MAKE_FACE_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theInfo->surface.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "surface is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->inner_wire_count > 0 && theInfo->inner_wires == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "inner_wires is NULL when inner_wire_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->inner_wire_count == 0 && theInfo->inner_wires != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "inner_wires is non-NULL when inner_wire_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_WireId anOuterWireId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theInfo->outer_wire,
                                                              BRepGraph_NodeId::Kind::Wire,
                                                              anOuterWireId))
    {
      return aStatus;
    }

    NCollection_DynamicArray<BRepGraph_WireId> anInnerWires;
    for (size_t anI = 0; anI < theInfo->inner_wire_count; ++anI)
    {
      BRepGraph_WireId aWireId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theInfo->inner_wires[anI],
                                                                BRepGraph_NodeId::Kind::Wire,
                                                                aWireId))
      {
        return aStatus;
      }

      anInnerWires.Append(aWireId);
    }

    const occ::handle<Geom_Surface> aSurface =
      OcctL::Geom::SurfaceFromRep(theGraph, theInfo->surface);

    const BRepGraph_FaceId aFaceId = theGraph->graph.Editor().Faces().Add(aSurface,
                                                                          anOuterWireId,
                                                                          anInnerWires,
                                                                          theInfo->tolerance);
    *theOutFace                    = OcctL::Topo::PackNodeId(aFaceId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_make_face_from_wires_auto(
  occtl_graph_t* const                                        theGraph,
  const occtl_topo_make_face_from_wires_auto_options_t* const theOptions,
  occtl_node_id_t* const                                      theOutFace)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOptions == nullptr || theOutFace == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, options, or out_face is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->struct_version != OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "options->struct_version is not OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theOptions->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "options->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->surface.bits == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "surface id is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->wire_count == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "wire_count is zero");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theOptions->wires == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "wires is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!IsFiniteValue(theOptions->tolerance) || theOptions->tolerance < 0.0
        || !IsFiniteValue(theOptions->area_tolerance) || theOptions->area_tolerance < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tolerances must be finite and non-negative");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_Array1<BRepGraph_WireId> aWireIds(theOptions->wire_count);
    NCollection_Array1<double>           anAreas(theOptions->wire_count);

    size_t anOuterIndex     = 0;
    double aBestArea        = -1.0;
    bool   anOuterAmbiguous = false;

    for (size_t anI = 0; anI < theOptions->wire_count; ++anI)
    {
      for (size_t aPrev = 0; aPrev < anI; ++aPrev)
      {
        if (theOptions->wires[aPrev].bits == theOptions->wires[anI].bits)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "duplicate wire id");
          return OCCTL_INVALID_ARGUMENT;
        }
      }

      BRepGraph_WireId aWireId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theOptions->wires[anI],
                                                                BRepGraph_NodeId::Kind::Wire,
                                                                aWireId))
      {
        return aStatus;
      }

      const TopoDS_Shape aWireShape = theGraph->graph.Shapes().Shape(BRepGraph_NodeId(aWireId));
      if (aWireShape.IsNull() || aWireShape.ShapeType() != TopAbs_WIRE)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_ERROR,
                                               "wire could not be reconstructed as TopoDS_Wire");
        return OCCTL_ERROR;
      }

      double anArea = 0.0;
      if (!measureWireAreaWithOcct(theGraph->graph, aWireId, TopoDS::Wire(aWireShape), anArea))
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_ERROR,
          "OCCT could not measure an intermediate face from a candidate wire");
        return OCCTL_ERROR;
      }
      aWireIds.ChangeAt(anI) = aWireId;
      anAreas.ChangeAt(anI)  = anArea;

      if (anArea > aBestArea + theOptions->area_tolerance)
      {
        aBestArea        = anArea;
        anOuterIndex     = anI;
        anOuterAmbiguous = false;
      }
      else if (std::fabs(anArea - aBestArea) <= theOptions->area_tolerance)
      {
        anOuterAmbiguous = true;
      }
    }

    if (anOuterAmbiguous)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "outer wire is ambiguous by area");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_DynamicArray<BRepGraph_WireId> anInnerWires;
    for (size_t anI = 0; anI < aWireIds.Size(); ++anI)
    {
      if (anI != anOuterIndex)
      {
        anInnerWires.Append(aWireIds.At(anI));
      }
    }

    const BRepGraph_FaceId aFaceId = theGraph->graph.Editor().Faces().Add(
      OcctL::Geom::SurfaceFromRep(theGraph, theOptions->surface),
      aWireIds.At(anOuterIndex),
      anInnerWires,
      theOptions->tolerance);
    *theOutFace = OcctL::Topo::PackNodeId(aFaceId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_shell(occtl_graph_t* const                      theGraph,
                        const occtl_topo_make_shell_info_t* const theInfo,
                        occtl_node_id_t* const                    theOutShell)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShell == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shell is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->struct_version != OCCTL_TOPO_MAKE_SHELL_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_TOPO_MAKE_SHELL_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theInfo->face_count > 0 && theInfo->faces == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "faces is NULL when face_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->face_count == 0 && theInfo->faces != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "faces is non-NULL when face_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph::EditorView   anEditor = theGraph->graph.Editor();
    const BRepGraph_ShellId aShellId = anEditor.Shells().Add();
    for (size_t anI = 0; anI < theInfo->face_count; ++anI)
    {
      BRepGraph_FaceId aFaceId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theInfo->faces[anI].id,
                                                                BRepGraph_NodeId::Kind::Face,
                                                                aFaceId))
      {
        return aStatus;
      }

      anEditor.Shells().AddFace(aShellId,
                                aFaceId,
                                OcctL::Topo::ToOcctOrientation(theInfo->faces[anI].orientation));
    }

    if (theInfo->is_closed)
    {
      anEditor.Shells().SetIsClosed(aShellId, true);
    }

    *theOutShell = OcctL::Topo::PackNodeId(aShellId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_solid(occtl_graph_t* const                      theGraph,
                        const occtl_topo_make_solid_info_t* const theInfo,
                        occtl_node_id_t* const                    theOutSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_solid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->struct_version != OCCTL_TOPO_MAKE_SOLID_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_TOPO_MAKE_SOLID_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theInfo->shell_count > 0 && theInfo->shells == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "shells is NULL when shell_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->shell_count == 0 && theInfo->shells != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "shells is non-NULL when shell_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph::EditorView   anEditor = theGraph->graph.Editor();
    const BRepGraph_SolidId aSolidId = anEditor.Solids().Add();
    for (size_t anI = 0; anI < theInfo->shell_count; ++anI)
    {
      BRepGraph_ShellId aShellId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theInfo->shells[anI].id,
                                                                BRepGraph_NodeId::Kind::Shell,
                                                                aShellId))
      {
        return aStatus;
      }

      anEditor.Solids().AddShell(aSolidId,
                                 aShellId,
                                 OcctL::Topo::ToOcctOrientation(theInfo->shells[anI].orientation));
    }

    *theOutSolid = OcctL::Topo::PackNodeId(aSolidId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_compound(occtl_graph_t* const                         theGraph,
                           const occtl_topo_make_compound_info_t* const theInfo,
                           occtl_node_id_t* const                       theOutCompound)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutCompound == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_compound is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->struct_version != OCCTL_TOPO_MAKE_COMPOUND_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_TOPO_MAKE_COMPOUND_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theInfo->child_count > 0 && theInfo->children == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "children is NULL when child_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->child_count == 0 && theInfo->children != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "children is non-NULL when child_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_DynamicArray<BRepGraph_NodeId> anEmpty;
    BRepGraph::EditorView                      anEditor = theGraph->graph.Editor();
    const BRepGraph_CompoundId                 aCompId  = anEditor.Compounds().Add(anEmpty);
    for (size_t anI = 0; anI < theInfo->child_count; ++anI)
    {
      const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theInfo->children[anI].id);
      if (!aNodeId.IsValid())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "compound child NodeId is invalid");
        return OCCTL_NOT_FOUND;
      }

      (void)anEditor.Compounds().AddChild(
        aCompId,
        aNodeId,
        OcctL::Topo::ToOcctOrientation(theInfo->children[anI].orientation));
    }

    *theOutCompound = OcctL::Topo::PackNodeId(aCompId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_make_compsolid(occtl_graph_t* const                          theGraph,
                            const occtl_topo_make_compsolid_info_t* const theInfo,
                            occtl_node_id_t* const                        theOutCompSolid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutCompSolid == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_compsolid is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->struct_version != OCCTL_TOPO_MAKE_COMPSOLID_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_TOPO_MAKE_COMPSOLID_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }

    if (theInfo->solid_count > 0 && theInfo->solids == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "solids is NULL when solid_count > 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theInfo->solid_count == 0 && theInfo->solids != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "solids is non-NULL when solid_count == 0");
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_DynamicArray<BRepGraph_SolidId> aSolidIds;
    for (size_t anI = 0; anI < theInfo->solid_count; ++anI)
    {
      BRepGraph_SolidId aSolidId;
      if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                                theInfo->solids[anI].id,
                                                                BRepGraph_NodeId::Kind::Solid,
                                                                aSolidId))
      {
        return aStatus;
      }

      (void)OcctL::Topo::ToOcctOrientation(theInfo->solids[anI].orientation);
      aSolidIds.Append(aSolidId);
    }

    const BRepGraph_CompSolidId aCompSolidId = theGraph->graph.Editor().CompSolids().Add(aSolidIds);
    *theOutCompSolid                         = OcctL::Topo::PackNodeId(aCompSolidId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove(occtl_graph_t* const  theGraph,
                                                      const occtl_node_id_t theId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theId);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    theGraph->graph.Editor().Gen().RemoveNode(aNodeId);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_remove_subgraph(occtl_graph_t* const  theGraph,
                                                               const occtl_node_id_t theId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theId);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    theGraph->graph.Editor().Gen().RemoveSubgraph(aNodeId);
    return OCCTL_OK;
  });
}

} // extern "C"
