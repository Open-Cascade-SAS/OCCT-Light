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

#include "GraphAdjacencyCache.hxx"
#include "GraphDescendantCache.hxx"
#include "GraphHandle.hxx"
#include "IdConvert.hxx"
#include "TopoMath.hxx"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGraphAlgo_SolidClassifier.hxx>
#include <BRepGraph_ChildExplorer.hxx>
#include <BRepGraph_RelatedIterator.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_TopoView.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <NCollection_FlatDataMap.hxx>
#include <NCollection_FlatMap.hxx>
#include <NCollection_LinearVector.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_State.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Precision.hxx>

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/GeomMath.hxx"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace
{

bool IsFiniteValue(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

struct TouchHitKey
{
  uint64_t SupportA = 0;
  uint64_t SupportB = 0;
  double   PointAX  = 0.0;
  double   PointAY  = 0.0;
  double   PointAZ  = 0.0;
  double   PointBX  = 0.0;
  double   PointBY  = 0.0;
  double   PointBZ  = 0.0;

  bool operator==(const TouchHitKey& theOther) const noexcept
  {
    return SupportA == theOther.SupportA && SupportB == theOther.SupportB
           && PointAX == theOther.PointAX && PointAY == theOther.PointAY
           && PointAZ == theOther.PointAZ && PointBX == theOther.PointBX
           && PointBY == theOther.PointBY && PointBZ == theOther.PointBZ;
  }
};

struct TouchHitKeyHasher
{
  size_t operator()(const TouchHitKey& theKey) const noexcept
  {
    size_t aHash = hashUint64(theKey.SupportA);
    aHash        = combine(aHash, hashUint64(theKey.SupportB));
    aHash        = combine(aHash, NCollection_DefaultHasher<double>{}(theKey.PointAX));
    aHash        = combine(aHash, NCollection_DefaultHasher<double>{}(theKey.PointAY));
    aHash        = combine(aHash, NCollection_DefaultHasher<double>{}(theKey.PointAZ));
    aHash        = combine(aHash, NCollection_DefaultHasher<double>{}(theKey.PointBX));
    aHash        = combine(aHash, NCollection_DefaultHasher<double>{}(theKey.PointBY));
    return combine(aHash, NCollection_DefaultHasher<double>{}(theKey.PointBZ));
  }

  bool operator()(const TouchHitKey& theLeft, const TouchHitKey& theRight) const noexcept
  {
    return theLeft == theRight;
  }

private:
  static size_t hashUint64(const uint64_t theValue) noexcept
  {
    return static_cast<size_t>(theValue ^ (theValue >> 32));
  }

  static size_t combine(const size_t theLeft, const size_t theRight) noexcept
  {
    return theLeft ^ (theRight + 0x9e3779b97f4a7c15ull + (theLeft << 6) + (theLeft >> 2));
  }
};

bool resolveShape(const occtl_graph_t* const theGraph,
                  const occtl_node_id_t      theNode,
                  TopoDS_Shape&              theShape)
{
  const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theNode);
  if (!aNodeId.IsValid())
  {
    return false;
  }

  theShape = theGraph->graph.Shapes().Shape(aNodeId);
  return !theShape.IsNull();
}

occtl_node_id_t supportNode(const BRepGraph& theGraph, const TopoDS_Shape& theShape)
{
  if (theShape.IsNull())
  {
    return OCCTL_NODE_ID_INVALID;
  }

  const BRepGraph_NodeId aNodeId = theGraph.Shapes().FindNode(theShape);
  if (!aNodeId.IsValid())
  {
    return OCCTL_NODE_ID_INVALID;
  }

  return OcctL::Topo::PackNodeId(aNodeId);
}

bool isLowerDimensionSupport(const occtl_node_id_t theNode)
{
  const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theNode);
  return aNodeId.NodeKind == BRepGraph_NodeId::Kind::Vertex
         || aNodeId.NodeKind == BRepGraph_NodeId::Kind::Edge
         || aNodeId.NodeKind == BRepGraph_NodeId::Kind::CoEdge;
}

bool isZeroOrOne(const int32_t theFlag) noexcept
{
  return theFlag == 0 || theFlag == 1;
}

occtl_status_t validateRelationOptions(const occtl_topo_relation_options_t& theOptions)
{
  if (theOptions.struct_version != OCCTL_TOPO_RELATION_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "options->struct_version is not OCCTL_TOPO_RELATION_OPTIONS_VERSION_1");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOptions.p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "options->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsFiniteValue(theOptions.tolerance) || theOptions.tolerance < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "options->tolerance is negative or non-finite");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!isZeroOrOne(theOptions.include_tangent_contacts) || !isZeroOrOne(theOptions.include_overlaps)
      || !isZeroOrOne(theOptions.include_lower_dimension_results))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "relation option flags must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

bool collectVertexNodes(BRepGraph&                                 theGraph,
                        const BRepGraph_NodeId                     theRoot,
                        NCollection_LinearVector<occtl_node_id_t>& theOutVertices)
{
  return OcctL::Topo::ComputeDescendantVertices(theGraph, theRoot, theOutVertices);
}

bool isActiveNode(const BRepGraph& theGraph, const BRepGraph_NodeId theNode)
{
  return theNode.IsValid() && !theGraph.Topo().Gen().IsRemoved(theNode);
}

void collectAdjacentEdges(BRepGraph&                                 theGraph,
                          const BRepGraph_EdgeId                     theEdge,
                          NCollection_LinearVector<occtl_node_id_t>& theOutEdges)
{
  if (!isActiveNode(theGraph, BRepGraph_NodeId(theEdge)))
  {
    return;
  }

  NCollection_FlatMap<uint64_t> aSeen;

  const auto addIncidentEdges = [&](const BRepGraph_VertexId theVertex) {
    const NCollection_DynamicArray<BRepGraph_EdgeId>& anEdges =
      theGraph.Topo().Vertices().Edges(theVertex);
    for (int anIndex = anEdges.Lower(); anIndex <= anEdges.Upper(); ++anIndex)
    {
      const BRepGraph_EdgeId anEdge = anEdges.Value(anIndex);
      if (anEdge == theEdge || !isActiveNode(theGraph, BRepGraph_NodeId(anEdge)))
      {
        continue;
      }

      const occtl_node_id_t anAbiEdge = OcctL::Topo::PackNodeId(anEdge);
      if (aSeen.Add(anAbiEdge.bits))
      {
        theOutEdges.Append(anAbiEdge);
      }
    }
  };

  addIncidentEdges(BRepGraph_Tool::Edge::StartVertexId(theGraph, theEdge));
  addIncidentEdges(BRepGraph_Tool::Edge::EndVertexId(theGraph, theEdge));
}

void collectAdjacentFaces(BRepGraph&                                 theGraph,
                          const BRepGraph_FaceId                     theFace,
                          NCollection_LinearVector<occtl_node_id_t>& theOutFaces)
{
  if (!isActiveNode(theGraph, BRepGraph_NodeId(theFace)))
  {
    return;
  }

  NCollection_FlatMap<uint64_t> aSeen;
  BRepGraph_RelatedIterator     anIt(theGraph, BRepGraph_NodeId(theFace));
  for (; anIt.More(); anIt.Next())
  {
    if (anIt.CurrentRelation() != BRepGraph_RelatedIterator::RelationKind::AdjacentFace)
    {
      continue;
    }

    const BRepGraph_NodeId aFace = anIt.Current();
    if (aFace.NodeKind != BRepGraph_NodeId::Kind::Face || !isActiveNode(theGraph, aFace))
    {
      continue;
    }

    const occtl_node_id_t anAbiFace = OcctL::Topo::PackNodeId(aFace);
    if (aSeen.Add(anAbiFace.bits))
    {
      theOutFaces.Append(anAbiFace);
    }
  }
}

bool isTopologicalDistanceKind(const BRepGraph_NodeId::Kind theKind)
{
  return theKind == BRepGraph_NodeId::Kind::Vertex || theKind == BRepGraph_NodeId::Kind::Edge
         || theKind == BRepGraph_NodeId::Kind::Wire || theKind == BRepGraph_NodeId::Kind::Face
         || theKind == BRepGraph_NodeId::Kind::Shell || theKind == BRepGraph_NodeId::Kind::Solid;
}

void collectPeerNodes(const BRepGraph&                            theGraph,
                      const BRepGraph_NodeId                      theRoot,
                      const BRepGraph_NodeId::Kind                theKind,
                      NCollection_LinearVector<BRepGraph_NodeId>& theOutPeers)
{
  NCollection_FlatMap<uint64_t> aSeen;
  if (theRoot.NodeKind == theKind && isActiveNode(theGraph, theRoot))
  {
    theOutPeers.Append(theRoot);
    aSeen.Add(OcctL::Topo::PackNodeId(theRoot).bits);
  }

  BRepGraph_ChildExplorer anExplorer(theGraph, theRoot, theKind);
  for (; anExplorer.More(); anExplorer.Next())
  {
    const BRepGraph_NodeId aNode = anExplorer.Current().DefId;
    if (!isActiveNode(theGraph, aNode))
    {
      continue;
    }

    const uint64_t aBits = OcctL::Topo::PackNodeId(aNode).bits;
    if (aSeen.Add(aBits))
    {
      theOutPeers.Append(aNode);
    }
  }
}

bool collectDescendantFaces(const BRepGraph&                           theGraph,
                            const BRepGraph_NodeId                     theRoot,
                            NCollection_LinearVector<occtl_node_id_t>& theOutFaces)
{
  if (!isActiveNode(theGraph, theRoot))
  {
    return false;
  }

  NCollection_FlatMap<uint64_t> aSeen;
  if (theRoot.NodeKind == BRepGraph_NodeId::Kind::Face)
  {
    const occtl_node_id_t anAbiFace = OcctL::Topo::PackNodeId(theRoot);
    theOutFaces.Append(anAbiFace);
    aSeen.Add(anAbiFace.bits);
  }

  BRepGraph_ChildExplorer anExplorer(theGraph, theRoot, BRepGraph_NodeId::Kind::Face);
  for (; anExplorer.More(); anExplorer.Next())
  {
    const BRepGraph_NodeId aFace = anExplorer.Current().DefId;
    if (!isActiveNode(theGraph, aFace))
    {
      continue;
    }

    const occtl_node_id_t anAbiFace = OcctL::Topo::PackNodeId(aFace);
    if (aSeen.Add(anAbiFace.bits))
    {
      theOutFaces.Append(anAbiFace);
    }
  }
  return true;
}

void addIfPeer(const BRepGraph_NodeId&                     theNode,
               const NCollection_FlatMap<uint64_t>&        thePeerBits,
               NCollection_LinearVector<BRepGraph_NodeId>& theOut)
{
  if (theNode.IsValid() && thePeerBits.Seek(OcctL::Topo::PackNodeId(theNode).bits) != nullptr)
  {
    theOut.Append(theNode);
  }
}

void buildSharedDescendantMap(
  BRepGraph&                                                                     theGraph,
  const NCollection_LinearVector<BRepGraph_NodeId>&                              thePeers,
  const bool                                                                     theUseVertices,
  NCollection_FlatDataMap<uint64_t, NCollection_LinearVector<BRepGraph_NodeId>>& theOutMap)
{
  for (size_t aPeerIndex = 0; aPeerIndex < thePeers.Size(); ++aPeerIndex)
  {
    const BRepGraph_NodeId                    aPeer = thePeers.Value(aPeerIndex);
    NCollection_LinearVector<occtl_node_id_t> aConnectors;
    if (theUseVertices)
    {
      (void)collectVertexNodes(theGraph, aPeer, aConnectors);
    }
    else
    {
      (void)collectDescendantFaces(theGraph, aPeer, aConnectors);
    }

    for (size_t aConnectorIndex = 0; aConnectorIndex < aConnectors.Size(); ++aConnectorIndex)
    {
      const occtl_node_id_t aConnector = aConnectors.Value(aConnectorIndex);
      theOutMap.TryBound(aConnector.bits, NCollection_LinearVector<BRepGraph_NodeId>())
        .Append(aPeer);
    }
  }
}

void collectTopologicalDistanceNeighbours(
  BRepGraph&                           theGraph,
  const BRepGraph_NodeId               theNode,
  const NCollection_FlatMap<uint64_t>& thePeerBits,
  const NCollection_FlatDataMap<uint64_t, NCollection_LinearVector<BRepGraph_NodeId>>&
                                              theSharedConnectorMap,
  NCollection_LinearVector<BRepGraph_NodeId>& theOutNeighbours)
{
  if (theNode.NodeKind == BRepGraph_NodeId::Kind::Vertex)
  {
    const BRepGraph_VertexId aVertex = BRepGraph_VertexId::FromNodeId(theNode);
    const NCollection_DynamicArray<BRepGraph_EdgeId>& anEdges =
      theGraph.Topo().Vertices().Edges(aVertex);
    for (int anI = anEdges.Lower(); anI <= anEdges.Upper(); ++anI)
    {
      const BRepGraph_EdgeId   anEdge = anEdges.Value(anI);
      const BRepGraph_VertexId aStart = BRepGraph_Tool::Edge::StartVertexId(theGraph, anEdge);
      const BRepGraph_VertexId anEnd  = BRepGraph_Tool::Edge::EndVertexId(theGraph, anEdge);
      if (BRepGraph_NodeId(aStart) != theNode)
      {
        addIfPeer(BRepGraph_NodeId(aStart), thePeerBits, theOutNeighbours);
      }
      if (BRepGraph_NodeId(anEnd) != theNode)
      {
        addIfPeer(BRepGraph_NodeId(anEnd), thePeerBits, theOutNeighbours);
      }
    }
    return;
  }

  if (theNode.NodeKind == BRepGraph_NodeId::Kind::Edge)
  {
    NCollection_LinearVector<occtl_node_id_t> anAdjacentEdges;
    (void)OcctL::Topo::ComputeAdjacentEdges(theGraph,
                                            BRepGraph_EdgeId::FromNodeId(theNode),
                                            anAdjacentEdges);
    for (size_t anEdgeIndex = 0; anEdgeIndex < anAdjacentEdges.Size(); ++anEdgeIndex)
    {
      const occtl_node_id_t anAdjacentEdge = anAdjacentEdges.Value(anEdgeIndex);
      addIfPeer(OcctL::Topo::UnpackNodeId(anAdjacentEdge), thePeerBits, theOutNeighbours);
    }
    return;
  }

  if (theNode.NodeKind == BRepGraph_NodeId::Kind::Face)
  {
    NCollection_LinearVector<occtl_node_id_t> aAdjacentFaces;
    (void)OcctL::Topo::ComputeAdjacentFaces(theGraph,
                                            BRepGraph_FaceId::FromNodeId(theNode),
                                            aAdjacentFaces);
    for (size_t aFaceIndex = 0; aFaceIndex < aAdjacentFaces.Size(); ++aFaceIndex)
    {
      const occtl_node_id_t anAdjacentFace = aAdjacentFaces.Value(aFaceIndex);
      addIfPeer(OcctL::Topo::UnpackNodeId(anAdjacentFace), thePeerBits, theOutNeighbours);
    }
    return;
  }

  NCollection_LinearVector<occtl_node_id_t> aConnectors;
  if (theNode.NodeKind == BRepGraph_NodeId::Kind::Wire)
  {
    (void)collectVertexNodes(theGraph, theNode, aConnectors);
  }
  else
  {
    (void)collectDescendantFaces(theGraph, theNode, aConnectors);
  }

  for (size_t aConnectorIndex = 0; aConnectorIndex < aConnectors.Size(); ++aConnectorIndex)
  {
    const occtl_node_id_t aConnector = aConnectors.Value(aConnectorIndex);
    const NCollection_LinearVector<BRepGraph_NodeId>* aFound =
      theSharedConnectorMap.Seek(aConnector.bits);
    if (aFound == nullptr)
    {
      continue;
    }
    for (size_t aNeighbourIndex = 0; aNeighbourIndex < aFound->Size(); ++aNeighbourIndex)
    {
      const BRepGraph_NodeId aNeighbour = aFound->Value(aNeighbourIndex);
      if (aNeighbour != theNode)
      {
        addIfPeer(aNeighbour, thePeerBits, theOutNeighbours);
      }
    }
  }
}

bool hasSubshapeOfKind(const TopoDS_Shape& theShape, const TopAbs_ShapeEnum theKind)
{
  if (theShape.IsNull())
  {
    return false;
  }
  if (theShape.ShapeType() == theKind)
  {
    return true;
  }
  for (TopExp_Explorer anExp(theShape, theKind); anExp.More(); anExp.Next())
  {
    return true;
  }
  return false;
}

bool hasIntersectionTopology(const TopoDS_Shape& theShape,
                             const bool          theIncludeLowerDimension,
                             const bool          theIncludeFaces)
{
  if (theIncludeFaces && hasSubshapeOfKind(theShape, TopAbs_FACE))
  {
    return true;
  }
  if (theIncludeLowerDimension)
  {
    return hasSubshapeOfKind(theShape, TopAbs_EDGE) || hasSubshapeOfKind(theShape, TopAbs_VERTEX);
  }
  return false;
}

void collectAddedIntersectionNodes(
  const NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>& theAdded,
  const bool                                 theIncludeLowerDimension,
  const bool                                 theIncludeFaces,
  NCollection_LinearVector<occtl_node_id_t>& theOutNodes,
  NCollection_FlatMap<uint64_t>&             theSeen)
{
  for (NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>::Iterator anIt(
         theAdded);
       anIt.More();
       anIt.Next())
  {
    const BRepGraph_NodeId aNode            = anIt.Value();
    const bool             isLowerDimension = aNode.NodeKind == BRepGraph_NodeId::Kind::Edge
                                              || aNode.NodeKind == BRepGraph_NodeId::Kind::Vertex;
    const bool             isFace           = aNode.NodeKind == BRepGraph_NodeId::Kind::Face;
    if ((!theIncludeLowerDimension || !isLowerDimension) && (!theIncludeFaces || !isFace))
    {
      continue;
    }

    const occtl_node_id_t anAbiNode = OcctL::Topo::PackNodeId(aNode);
    if (theSeen.Add(anAbiNode.bits))
    {
      theOutNodes.Append(anAbiNode);
    }
  }
}

occtl_status_t addIntersectionResult(occtl_graph_t* const theGraph,
                                     const TopoDS_Shape&  theShape,
                                     const bool           theIncludeLowerDimension,
                                     const bool           theIncludeFaces,
                                     NCollection_LinearVector<occtl_node_id_t>& theOutNodes,
                                     NCollection_FlatMap<uint64_t>&             theSeen)
{
  if (!hasIntersectionTopology(theShape, theIncludeLowerDimension, theIncludeFaces))
  {
    return OCCTL_OK;
  }

  BRepGraph::ShapesView::Options anOptions;
  anOptions.CreateAutoProduct = false;
  anOptions.TrackAddedNodes   = true;

  const BRepGraph::ShapesView::Result aResult = theGraph->graph.Shapes().Add(theShape, anOptions);
  if (!aResult.Ok)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      "intersection result topology could not be merged into the graph");
    return OCCTL_TOPOLOGY_INVALID;
  }

  collectAddedIntersectionNodes(aResult.AddedNodes,
                                theIncludeLowerDimension,
                                theIncludeFaces,
                                theOutNodes,
                                theSeen);
  return OCCTL_OK;
}

occtl_status_t fillNodeBuffer(const NCollection_LinearVector<occtl_node_id_t>& theNodes,
                              occtl_node_id_t* const                           theOutBuf,
                              const size_t                                     theCap,
                              size_t* const                                    theOutCount)
{
  *theOutCount = theNodes.Size();
  if (theOutBuf != nullptr && theCap < theNodes.Size())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL, "out_buf capacity is too small");
    return OCCTL_BUFFER_TOO_SMALL;
  }

  if (theOutBuf != nullptr)
  {
    for (size_t anI = 0; anI < theNodes.Size(); ++anI)
    {
      theOutBuf[anI] = theNodes.Value(anI);
    }
  }
  return OCCTL_OK;
}

occtl_topo_point_class_t toPointClass(const TopAbs_State theState)
{
  switch (theState)
  {
    case TopAbs_IN:
      return OCCTL_TOPO_POINT_CLASS_IN;
    case TopAbs_OUT:
      return OCCTL_TOPO_POINT_CLASS_OUT;
    case TopAbs_ON:
      return OCCTL_TOPO_POINT_CLASS_ON;
    default:
      return OCCTL_TOPO_POINT_CLASS_UNKNOWN;
  }
}

bool axisDirectionIsValid(const occtl_axis1_placement_t& theAxis)
{
  const gp_Vec aDirection(theAxis.direction.x, theAxis.direction.y, theAxis.direction.z);
  return IsFiniteValue(theAxis.location.x) && IsFiniteValue(theAxis.location.y)
         && IsFiniteValue(theAxis.location.z) && IsFiniteValue(theAxis.direction.x)
         && IsFiniteValue(theAxis.direction.y) && IsFiniteValue(theAxis.direction.z)
         && aDirection.SquareMagnitude() > Precision::SquareConfusion();
}

bool faceNormalAt(const TopoDS_Face&  theFace,
                  const double        theU,
                  const double        theV,
                  occtl_direction3_t& theOutNormal)
{
  BRepAdaptor_Surface anAdaptor(theFace);
  gp_Pnt              aPoint;
  gp_Vec              aDU;
  gp_Vec              aDV;
  anAdaptor.D1(theU, theV, aPoint, aDU, aDV);

  gp_Vec aNormal = aDU.Crossed(aDV);
  if (aNormal.SquareMagnitude() <= Precision::SquareConfusion())
  {
    return false;
  }
  if (theFace.Orientation() == TopAbs_REVERSED)
  {
    aNormal.Reverse();
  }
  theOutNormal = OcctL::Geom::FromGp(gp_Dir(aNormal));
  return true;
}

bool sameFaceByGraphRelation(const BRepGraph&       theGraph,
                             const BRepGraph_FaceId theFaceA,
                             const BRepGraph_FaceId theFaceB)
{
  if (theFaceA == theFaceB)
  {
    return true;
  }

  const BRepGraph_SurfaceRepId aRepA = theGraph.Topo().Faces().SurfaceRepId(theFaceA);
  const BRepGraph_SurfaceRepId aRepB = theGraph.Topo().Faces().SurfaceRepId(theFaceB);
  if (aRepA.IsValid() && aRepB.IsValid() && aRepA == aRepB)
  {
    return true;
  }

  const NCollection_DynamicArray<BRepGraph_FaceId> aSameDomain =
    theGraph.Topo().Faces().SameDomain(theFaceA, nullptr);
  for (int anI = aSameDomain.Lower(); anI <= aSameDomain.Upper(); ++anI)
  {
    if (aSameDomain.Value(anI) == theFaceB)
    {
      return true;
    }
  }
  return false;
}

bool sameEdgeByGraphRelation(const BRepGraph&       theGraph,
                             const BRepGraph_EdgeId theEdgeA,
                             const BRepGraph_EdgeId theEdgeB)
{
  if (theEdgeA == theEdgeB)
  {
    return true;
  }

  const BRepGraph_Curve3DRepId aRepA = theGraph.Topo().Edges().Curve3DRepId(theEdgeA);
  const BRepGraph_Curve3DRepId aRepB = theGraph.Topo().Edges().Curve3DRepId(theEdgeB);
  return aRepA.IsValid() && aRepB.IsValid() && aRepA == aRepB;
}

bool pointOnCurve(const gp_Pnt&                  thePoint,
                  const occ::handle<Geom_Curve>& theCurve,
                  const double                   theTolerance)
{
  if (theCurve.IsNull())
  {
    return false;
  }

  GeomAPI_ProjectPointOnCurve aProj(thePoint, theCurve);
  return aProj.NbPoints() > 0 && aProj.LowerDistance() <= theTolerance;
}

bool sameEdgeByProjection(const TopoDS_Edge& theEdgeA,
                          const TopoDS_Edge& theEdgeB,
                          const double       theTolerance)
{
  BRepAdaptor_Curve anAdaptorA(theEdgeA);
  BRepAdaptor_Curve anAdaptorB(theEdgeB);
  if (anAdaptorA.GetType() != anAdaptorB.GetType())
  {
    return false;
  }

  double                        aFirstA = 0.0;
  double                        aLastA  = 0.0;
  double                        aFirstB = 0.0;
  double                        aLastB  = 0.0;
  const occ::handle<Geom_Curve> aCurveA = BRep_Tool::Curve(theEdgeA, aFirstA, aLastA);
  const occ::handle<Geom_Curve> aCurveB = BRep_Tool::Curve(theEdgeB, aFirstB, aLastB);
  if (aCurveA.IsNull() || aCurveB.IsNull())
  {
    return false;
  }

  const gp_Pnt aA0 = anAdaptorA.Value(anAdaptorA.FirstParameter());
  const gp_Pnt aA1 = anAdaptorA.Value(anAdaptorA.LastParameter());
  const gp_Pnt aB0 = anAdaptorB.Value(anAdaptorB.FirstParameter());
  const gp_Pnt aB1 = anAdaptorB.Value(anAdaptorB.LastParameter());
  return pointOnCurve(aA0, aCurveB, theTolerance) && pointOnCurve(aA1, aCurveB, theTolerance)
         && pointOnCurve(aB0, aCurveA, theTolerance) && pointOnCurve(aB1, aCurveA, theTolerance);
}

bool pointOnSurface(const gp_Pnt&                    thePoint,
                    const occ::handle<Geom_Surface>& theSurface,
                    const double                     theTolerance)
{
  if (theSurface.IsNull())
  {
    return false;
  }

  GeomAPI_ProjectPointOnSurf aProj(thePoint, theSurface);
  return aProj.NbPoints() > 0 && aProj.LowerDistance() <= theTolerance;
}

bool sameFaceByProjection(const TopoDS_Face& theFaceA,
                          const TopoDS_Face& theFaceB,
                          const double       theTolerance)
{
  BRepAdaptor_Surface anAdaptorA(theFaceA);
  BRepAdaptor_Surface anAdaptorB(theFaceB);
  if (anAdaptorA.GetType() != anAdaptorB.GetType())
  {
    return false;
  }

  const occ::handle<Geom_Surface> aSurfaceA = BRep_Tool::Surface(theFaceA);
  const occ::handle<Geom_Surface> aSurfaceB = BRep_Tool::Surface(theFaceB);
  if (aSurfaceA.IsNull() || aSurfaceB.IsNull())
  {
    return false;
  }

  double aUMinA = 0.0;
  double aUMaxA = 0.0;
  double aVMinA = 0.0;
  double aVMaxA = 0.0;
  double aUMinB = 0.0;
  double aUMaxB = 0.0;
  double aVMinB = 0.0;
  double aVMaxB = 0.0;
  BRepTools::UVBounds(theFaceA, aUMinA, aUMaxA, aVMinA, aVMaxA);
  BRepTools::UVBounds(theFaceB, aUMinB, aUMaxB, aVMinB, aVMaxB);

  const gp_Pnt aA00 = anAdaptorA.Value(aUMinA, aVMinA);
  const gp_Pnt aA10 = anAdaptorA.Value(aUMaxA, aVMinA);
  const gp_Pnt aA01 = anAdaptorA.Value(aUMinA, aVMaxA);
  const gp_Pnt aA11 = anAdaptorA.Value(aUMaxA, aVMaxA);
  const gp_Pnt aB00 = anAdaptorB.Value(aUMinB, aVMinB);
  const gp_Pnt aB10 = anAdaptorB.Value(aUMaxB, aVMinB);
  const gp_Pnt aB01 = anAdaptorB.Value(aUMinB, aVMaxB);
  const gp_Pnt aB11 = anAdaptorB.Value(aUMaxB, aVMaxB);
  return pointOnSurface(aA00, aSurfaceB, theTolerance)
         && pointOnSurface(aA10, aSurfaceB, theTolerance)
         && pointOnSurface(aA01, aSurfaceB, theTolerance)
         && pointOnSurface(aA11, aSurfaceB, theTolerance)
         && pointOnSurface(aB00, aSurfaceA, theTolerance)
         && pointOnSurface(aB10, aSurfaceA, theTolerance)
         && pointOnSurface(aB01, aSurfaceA, theTolerance)
         && pointOnSurface(aB11, aSurfaceA, theTolerance);
}

occtl_status_t classifySolidPoint(const occtl_graph_t* const      theGraph,
                                  const occtl_node_id_t           theSolid,
                                  const occtl_point3_t            thePoint,
                                  const double                    theTolerance,
                                  occtl_topo_point_class_t* const theOutClass)
{
  if (theTolerance < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "tolerance is negative");
    return OCCTL_INVALID_ARGUMENT;
  }

  BRepGraph_SolidId aSolidId;
  if (const occtl_status_t aStatus =
        OcctL::Topo::ToTypedId(theGraph, theSolid, BRepGraph_NodeId::Kind::Solid, aSolidId))
  {
    return aStatus;
  }

  BRepGraphAlgo_SolidClassifier aClassifier(theGraph->graph, aSolidId);
  aClassifier.Perform(OcctL::Geom::ToGp(thePoint), theTolerance);
  *theOutClass = toPointClass(aClassifier.State());
  return OCCTL_OK;
}

} // anonymous namespace

struct occtl_topo_related_iter
{
  occtl_topo_related_iter(const BRepGraph& theGraph, const BRepGraph_NodeId theNode)
      : impl(theGraph, theNode)
  {
  }

  BRepGraph_RelatedIterator impl;
};

struct occtl_topo_axis_hit_iter
{
  NCollection_LinearVector<occtl_topo_axis_hit_t> hits;
  size_t                                          index = 0;
};

struct occtl_topo_touch_iter
{
  NCollection_LinearVector<occtl_topo_touch_hit_t> hits;
  size_t                                           index = 0;
};

struct occtl_topo_intersection_iter
{
  NCollection_LinearVector<occtl_node_id_t> nodes;
  size_t                                    index = 0;
};

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_relation_options_init(occtl_topo_relation_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    const occtl_topo_relation_options_t anInit = OCCTL_TOPO_RELATION_OPTIONS_INIT;
    *theOptions                                = anInit;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_related_iter_create(const occtl_graph_t* const        theGraph,
                                 const occtl_node_id_t             theNode,
                                 occtl_topo_related_iter_t** const theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theNode);
    if (!aNodeId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "NodeId is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    occtl_topo_related_iter* anIter = new occtl_topo_related_iter(theGraph->graph, aNodeId);
    *theOutIter                     = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_distance_pair(const occtl_graph_t* const        theGraph,
                           const occtl_node_id_t             theNodeA,
                           const occtl_node_id_t             theNodeB,
                           occtl_topo_distance_pair_t* const theOutPair)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutPair == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_pair is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutPair = occtl_topo_distance_pair_t{};

    TopoDS_Shape aShapeA;
    TopoDS_Shape aShapeB;
    if (!resolveShape(theGraph, theNodeA, aShapeA) || !resolveShape(theGraph, theNodeB, aShapeB))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "node_a or node_b is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    BRepExtrema_DistShapeShape aDist(aShapeA, aShapeB);
    if (!aDist.IsDone() || aDist.NbSolution() <= 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not compute a closest solution");
      return OCCTL_GEOMETRY_INVALID;
    }

    theOutPair->distance       = aDist.Value();
    theOutPair->point_a        = OcctL::Geom::FromGp(aDist.PointOnShape1(1));
    theOutPair->point_b        = OcctL::Geom::FromGp(aDist.PointOnShape2(1));
    theOutPair->support_a      = supportNode(theGraph->graph, aDist.SupportOnShape1(1));
    theOutPair->support_b      = supportNode(theGraph->graph, aDist.SupportOnShape2(1));
    theOutPair->inner_solution = aDist.InnerSolution() ? 1 : 0;
    theOutPair->solution_count = static_cast<int32_t>(aDist.NbSolution());
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_closest_point_to_point(const occtl_graph_t* const theGraph,
                                    const occtl_node_id_t      theNode,
                                    const occtl_point3_t       thePoint,
                                    occtl_point3_t* const      theOutClosest,
                                    double* const              theOutDistance)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutClosest == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph or out_closest is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    TopoDS_Shape aShape;
    if (!resolveShape(theGraph, theNode, aShape))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "node is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    BRepBuilderAPI_MakeVertex aVertexMaker(OcctL::Geom::ToGp(thePoint));
    if (!aVertexMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not construct query vertex");
      return OCCTL_GEOMETRY_INVALID;
    }

    BRepExtrema_DistShapeShape aDist(aShape, aVertexMaker.Vertex());
    if (!aDist.IsDone() || aDist.NbSolution() <= 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not compute a closest solution");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutClosest = OcctL::Geom::FromGp(aDist.PointOnShape1(1));
    if (theOutDistance != nullptr)
    {
      *theOutDistance = aDist.Value();
    }

    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_axis_intersect_faces(const occtl_graph_t* const         theGraph,
                                  const occtl_node_id_t              theRoot,
                                  const occtl_axis1_placement_t      theAxis,
                                  const double                       theMinParameter,
                                  const double                       theMaxParameter,
                                  const double                       theTolerance,
                                  occtl_topo_axis_hit_iter_t** const theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutIter = nullptr;

    if (theTolerance < 0.0 || !IsFiniteValue(theTolerance))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tolerance is negative or non-finite");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theMinParameter) || !IsFiniteValue(theMaxParameter)
        || theMinParameter > theMaxParameter)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "axis parameter interval is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!axisDirectionIsValid(theAxis))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "axis direction is zero or non-finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    TopoDS_Shape aRootShape;
    if (!resolveShape(theGraph, theRoot, aRootShape))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "root is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    IntCurvesFace_ShapeIntersector anIntersector;
    anIntersector.Load(aRootShape, theTolerance);
    anIntersector.Perform(gp_Lin(OcctL::Geom::ToGpAx1(theAxis)), theMinParameter, theMaxParameter);
    if (!anIntersector.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT face intersector did not complete");
      return OCCTL_GEOMETRY_INVALID;
    }

    anIntersector.SortResult();
    occtl_topo_axis_hit_iter* anIter = new occtl_topo_axis_hit_iter;

    for (int anI = 1; anI <= anIntersector.NbPnt(); ++anI)
    {
      const TopoDS_Face     aFace     = anIntersector.Face(anI);
      const occtl_node_id_t aFaceNode = supportNode(theGraph->graph, aFace);
      if (aFaceNode.bits == 0u)
      {
        continue;
      }

      occtl_topo_axis_hit_t aHit{};
      aHit.face       = aFaceNode;
      aHit.point      = OcctL::Geom::FromGp(anIntersector.Pnt(anI));
      aHit.uv         = {anIntersector.UParameter(anI), anIntersector.VParameter(anI)};
      aHit.parameter  = anIntersector.WParameter(anI);
      aHit.location   = toPointClass(anIntersector.State(anI));
      aHit.has_normal = faceNormalAt(aFace, aHit.uv.x, aHit.uv.y, aHit.normal) ? 1 : 0;
      anIter->hits.Append(aHit);
    }

    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_touch_iter_create(const occtl_graph_t* const                 theGraph,
                               const occtl_node_id_t                      theNodeA,
                               const occtl_node_id_t                      theNodeB,
                               const occtl_topo_relation_options_t* const theOptions,
                               occtl_topo_touch_iter_t** const            theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutIter = nullptr;

    occtl_topo_relation_options_t anOptions = OCCTL_TOPO_RELATION_OPTIONS_INIT;
    if (theOptions != nullptr)
    {
      anOptions = *theOptions;
    }
    if (const occtl_status_t aStatus = validateRelationOptions(anOptions))
    {
      return aStatus;
    }

    TopoDS_Shape aShapeA;
    TopoDS_Shape aShapeB;
    if (!resolveShape(theGraph, theNodeA, aShapeA) || !resolveShape(theGraph, theNodeB, aShapeB))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "node_a or node_b is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    occtl_topo_touch_iter*     anIter = new occtl_topo_touch_iter;
    BRepExtrema_DistShapeShape aDist(aShapeA, aShapeB);
    if (!aDist.IsDone() || aDist.NbSolution() <= 0)
    {
      delete anIter;
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not compute contact solutions");
      return OCCTL_GEOMETRY_INVALID;
    }

    if (aDist.Value() > anOptions.tolerance)
    {
      *theOutIter = anIter;
      return OCCTL_OK;
    }

    const int32_t anInnerSolution = aDist.InnerSolution() ? 1 : 0;
    if ((anInnerSolution != 0 && anOptions.include_overlaps == 0)
        || (anInnerSolution == 0 && anOptions.include_tangent_contacts == 0))
    {
      *theOutIter = anIter;
      return OCCTL_OK;
    }

    NCollection_FlatMap<TouchHitKey, TouchHitKeyHasher> aSeen;
    for (int anI = 1; anI <= aDist.NbSolution(); ++anI)
    {
      const gp_Pnt aPointA   = aDist.PointOnShape1(anI);
      const gp_Pnt aPointB   = aDist.PointOnShape2(anI);
      const double aDistance = aPointA.Distance(aPointB);
      if (aDistance > anOptions.tolerance)
      {
        continue;
      }

      occtl_topo_touch_hit_t aHit{};
      aHit.node_a         = theNodeA;
      aHit.node_b         = theNodeB;
      aHit.support_a      = supportNode(theGraph->graph, aDist.SupportOnShape1(anI));
      aHit.support_b      = supportNode(theGraph->graph, aDist.SupportOnShape2(anI));
      aHit.point_a        = OcctL::Geom::FromGp(aPointA);
      aHit.point_b        = OcctL::Geom::FromGp(aPointB);
      aHit.distance       = aDistance;
      aHit.inner_solution = anInnerSolution;

      if (anOptions.include_lower_dimension_results == 0
          && (isLowerDimensionSupport(aHit.support_a) || isLowerDimensionSupport(aHit.support_b)))
      {
        continue;
      }

      TouchHitKey aKey;
      aKey.SupportA = aHit.support_a.bits;
      aKey.SupportB = aHit.support_b.bits;
      aKey.PointAX  = aHit.point_a.x;
      aKey.PointAY  = aHit.point_a.y;
      aKey.PointAZ  = aHit.point_a.z;
      aKey.PointBX  = aHit.point_b.x;
      aKey.PointBY  = aHit.point_b.y;
      aKey.PointBZ  = aHit.point_b.z;
      if (aSeen.Add(aKey))
      {
        anIter->hits.Append(aHit);
      }
    }

    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_intersection_iter_create(occtl_graph_t* const                       theGraph,
                                      const occtl_node_id_t                      theNodeA,
                                      const occtl_node_id_t                      theNodeB,
                                      const occtl_topo_relation_options_t* const theOptions,
                                      occtl_topo_intersection_iter_t** const     theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutIter = nullptr;

    occtl_topo_relation_options_t anOptions = OCCTL_TOPO_RELATION_OPTIONS_INIT;
    if (theOptions != nullptr)
    {
      anOptions = *theOptions;
    }
    if (const occtl_status_t aStatus = validateRelationOptions(anOptions))
    {
      return aStatus;
    }

    TopoDS_Shape aShapeA;
    TopoDS_Shape aShapeB;
    if (!resolveShape(theGraph, theNodeA, aShapeA) || !resolveShape(theGraph, theNodeB, aShapeB))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "node_a or node_b is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    occtl_topo_intersection_iter* anIter = new occtl_topo_intersection_iter;
    const bool anIncludeLowerDimension   = anOptions.include_lower_dimension_results != 0;
    NCollection_FlatMap<uint64_t> aSeen;

    BRepAlgoAPI_Section aSection(aShapeA, aShapeB, false);
    aSection.Approximation(true);
    aSection.Build();
    if (!aSection.IsDone())
    {
      delete anIter;
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT section intersection failed");
      return OCCTL_GEOMETRY_INVALID;
    }
    if (const occtl_status_t aStatus = addIntersectionResult(theGraph,
                                                             aSection.Shape(),
                                                             anIncludeLowerDimension,
                                                             false,
                                                             anIter->nodes,
                                                             aSeen))
    {
      delete anIter;
      return aStatus;
    }

    if (anOptions.include_overlaps != 0)
    {
      BRepAlgoAPI_Common aCommon(aShapeA, aShapeB);
      aCommon.Build();
      if (!aCommon.IsDone())
      {
        delete anIter;
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "OCCT common intersection failed");
        return OCCTL_GEOMETRY_INVALID;
      }
      if (const occtl_status_t aStatus = addIntersectionResult(theGraph,
                                                               aCommon.Shape(),
                                                               anIncludeLowerDimension,
                                                               true,
                                                               anIter->nodes,
                                                               aSeen))
      {
        delete anIter;
        return aStatus;
      }
    }

    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_is_same_geometry(const occtl_graph_t* const theGraph,
                                                                const occtl_node_id_t      theNodeA,
                                                                const occtl_node_id_t      theNodeB,
                                                                const double   theTolerance,
                                                                int32_t* const theOutFlag)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutFlag == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_flag is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutFlag = 0;

    if (theTolerance < 0.0 || !IsFiniteValue(theTolerance))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "tolerance is negative or non-finite");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeA = OcctL::Topo::UnpackNodeId(theNodeA);
    const BRepGraph_NodeId aNodeB = OcctL::Topo::UnpackNodeId(theNodeB);
    if (!aNodeA.IsValid() || !aNodeB.IsValid() || theGraph->graph.Topo().Gen().IsRemoved(aNodeA)
        || theGraph->graph.Topo().Gen().IsRemoved(aNodeB))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "node_a or node_b is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    if (aNodeA.NodeKind != aNodeB.NodeKind)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "nodes must have the same kind");
      return OCCTL_WRONG_KIND;
    }

    TopoDS_Shape aShapeA;
    TopoDS_Shape aShapeB;

    if (aNodeA.NodeKind == BRepGraph_NodeId::Kind::Edge)
    {
      const BRepGraph_EdgeId anEdgeA = BRepGraph_EdgeId::FromNodeId(aNodeA);
      const BRepGraph_EdgeId anEdgeB = BRepGraph_EdgeId::FromNodeId(aNodeB);
      if (sameEdgeByGraphRelation(theGraph->graph, anEdgeA, anEdgeB))
      {
        *theOutFlag = 1;
        return OCCTL_OK;
      }
      if (!resolveShape(theGraph, theNodeA, aShapeA) || !resolveShape(theGraph, theNodeB, aShapeB))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "node_a or node_b is invalid or removed");
        return OCCTL_NOT_FOUND;
      }
      const TopoDS_Edge anOcctEdgeA = TopoDS::Edge(aShapeA);
      const TopoDS_Edge anOcctEdgeB = TopoDS::Edge(aShapeB);
      if (BRepTools::Compare(anOcctEdgeA, anOcctEdgeB)
          || sameEdgeByProjection(anOcctEdgeA, anOcctEdgeB, theTolerance))
      {
        *theOutFlag = 1;
      }
      return OCCTL_OK;
    }

    if (aNodeA.NodeKind == BRepGraph_NodeId::Kind::Face)
    {
      const BRepGraph_FaceId aFaceA = BRepGraph_FaceId::FromNodeId(aNodeA);
      const BRepGraph_FaceId aFaceB = BRepGraph_FaceId::FromNodeId(aNodeB);
      if (sameFaceByGraphRelation(theGraph->graph, aFaceA, aFaceB))
      {
        *theOutFlag = 1;
        return OCCTL_OK;
      }
      if (!resolveShape(theGraph, theNodeA, aShapeA) || !resolveShape(theGraph, theNodeB, aShapeB))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "node_a or node_b is invalid or removed");
        return OCCTL_NOT_FOUND;
      }
      *theOutFlag =
        sameFaceByProjection(TopoDS::Face(aShapeA), TopoDS::Face(aShapeB), theTolerance) ? 1 : 0;
      return OCCTL_OK;
    }

    OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                           "nodes must both be edges or both be faces");
    return OCCTL_WRONG_KIND;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_common_vertices(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t      theNodeA,
                                                               const occtl_node_id_t      theNodeB,
                                                               occtl_node_id_t* const     theOutBuf,
                                                               const size_t               theCap,
                                                               size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aNodeA = OcctL::Topo::UnpackNodeId(theNodeA);
    const BRepGraph_NodeId aNodeB = OcctL::Topo::UnpackNodeId(theNodeB);

    NCollection_LinearVector<occtl_node_id_t> aVerticesA;
    NCollection_LinearVector<occtl_node_id_t> aVerticesB;
    BRepGraph&                                aGraph = const_cast<BRepGraph&>(theGraph->graph);
    if (!collectVertexNodes(aGraph, aNodeA, aVerticesA)
        || !collectVertexNodes(aGraph, aNodeB, aVerticesB))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "node_a or node_b is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    NCollection_LinearVector<occtl_node_id_t> aCommon;
    NCollection_FlatMap<uint64_t>             aBitsB;
    for (size_t aVertexBIndex = 0; aVertexBIndex < aVerticesB.Size(); ++aVertexBIndex)
    {
      const occtl_node_id_t aVertexB = aVerticesB.Value(aVertexBIndex);
      aBitsB.Add(aVertexB.bits);
    }
    for (size_t aVertexAIndex = 0; aVertexAIndex < aVerticesA.Size(); ++aVertexAIndex)
    {
      const occtl_node_id_t aVertexA = aVerticesA.Value(aVertexAIndex);
      if (aBitsB.Seek(aVertexA.bits) != nullptr)
      {
        aCommon.Append(aVertexA);
      }
    }

    return fillNodeBuffer(aCommon, theOutBuf, theCap, theOutCount);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_adjacent_edges(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theEdge,
                                                              occtl_node_id_t* const     theOutBuf,
                                                              const size_t               theCap,
                                                              size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId anEdge;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theEdge, BRepGraph_NodeId::Kind::Edge, anEdge))
    {
      return aStatus;
    }

    NCollection_LinearVector<occtl_node_id_t> anAdjacent;
    BRepGraph&                                aGraph = const_cast<BRepGraph&>(theGraph->graph);
    collectAdjacentEdges(aGraph, anEdge, anAdjacent);
    return fillNodeBuffer(anAdjacent, theOutBuf, theCap, theOutCount);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_adjacent_faces(const occtl_graph_t* const theGraph,
                                                              const occtl_node_id_t      theFace,
                                                              occtl_node_id_t* const     theOutBuf,
                                                              const size_t               theCap,
                                                              size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFace;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theFace, BRepGraph_NodeId::Kind::Face, aFace))
    {
      return aStatus;
    }

    NCollection_LinearVector<occtl_node_id_t> anAdjacent;
    BRepGraph&                                aGraph = const_cast<BRepGraph&>(theGraph->graph);
    collectAdjacentFaces(aGraph, aFace, anAdjacent);
    return fillNodeBuffer(anAdjacent, theOutBuf, theCap, theOutCount);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_connected_edges(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t  theSeedEdge,
                                                               occtl_node_id_t* const theOutBuf,
                                                               const size_t           theCap,
                                                               size_t* const          theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId aSeedEdge;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theSeedEdge, BRepGraph_NodeId::Kind::Edge, aSeedEdge))
    {
      return aStatus;
    }

    NCollection_LinearVector<BRepGraph_EdgeId> aQueue;
    NCollection_LinearVector<occtl_node_id_t>  aConnected;
    NCollection_FlatMap<uint64_t>              aSeen;

    const auto addEdge = [&](const BRepGraph_EdgeId theEdge) {
      if (!theEdge.IsValid() || theGraph->graph.Topo().Gen().IsRemoved(BRepGraph_NodeId(theEdge)))
      {
        return;
      }
      const occtl_node_id_t anAbiEdge = OcctL::Topo::PackNodeId(theEdge);
      if (aSeen.Add(anAbiEdge.bits))
      {
        aQueue.Append(theEdge);
        aConnected.Append(anAbiEdge);
      }
    };

    addEdge(aSeedEdge);
    for (size_t aHead = 0; aHead < aQueue.Size(); ++aHead)
    {
      BRepGraph&                                aGraph = const_cast<BRepGraph&>(theGraph->graph);
      NCollection_LinearVector<occtl_node_id_t> anAdjacentEdges;
      collectAdjacentEdges(aGraph, aQueue.Value(aHead), anAdjacentEdges);
      for (size_t anEdgeIndex = 0; anEdgeIndex < anAdjacentEdges.Size(); ++anEdgeIndex)
      {
        const occtl_node_id_t anAdjacentEdge = anAdjacentEdges.Value(anEdgeIndex);
        addEdge(BRepGraph_EdgeId::FromNodeId(OcctL::Topo::UnpackNodeId(anAdjacentEdge)));
      }
    }

    return fillNodeBuffer(aConnected, theOutBuf, theCap, theOutCount);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_connected_faces(const occtl_graph_t* const theGraph,
                                                               const occtl_node_id_t  theSeedFace,
                                                               occtl_node_id_t* const theOutBuf,
                                                               const size_t           theCap,
                                                               size_t* const          theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aSeedFace;
    if (const occtl_status_t aStatus =
          OcctL::Topo::ToTypedId(theGraph, theSeedFace, BRepGraph_NodeId::Kind::Face, aSeedFace))
    {
      return aStatus;
    }

    NCollection_LinearVector<BRepGraph_FaceId> aQueue;
    NCollection_LinearVector<occtl_node_id_t>  aConnected;
    NCollection_FlatMap<uint64_t>              aSeen;

    const auto addFace = [&](const BRepGraph_FaceId theFace) {
      if (!theFace.IsValid() || theGraph->graph.Topo().Gen().IsRemoved(BRepGraph_NodeId(theFace)))
      {
        return;
      }
      const occtl_node_id_t anAbiFace = OcctL::Topo::PackNodeId(theFace);
      if (aSeen.Add(anAbiFace.bits))
      {
        aQueue.Append(theFace);
        aConnected.Append(anAbiFace);
      }
    };

    addFace(aSeedFace);
    for (size_t aHead = 0; aHead < aQueue.Size(); ++aHead)
    {
      BRepGraph&                                aGraph = const_cast<BRepGraph&>(theGraph->graph);
      NCollection_LinearVector<occtl_node_id_t> aAdjacentFaces;
      collectAdjacentFaces(aGraph, aQueue.Value(aHead), aAdjacentFaces);
      for (size_t aFaceIndex = 0; aFaceIndex < aAdjacentFaces.Size(); ++aFaceIndex)
      {
        const occtl_node_id_t anAdjacentFace = aAdjacentFaces.Value(aFaceIndex);
        addFace(BRepGraph_FaceId::FromNodeId(OcctL::Topo::UnpackNodeId(anAdjacentFace)));
      }
    }

    return fillNodeBuffer(aConnected, theOutBuf, theCap, theOutCount);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_graph_distance(const occtl_graph_t* const   theGraph,
                            const occtl_node_id_t        theRoot,
                            const occtl_node_id_t* const theSources,
                            const size_t                 theSourceCount,
                            const occtl_node_id_t        theTarget,
                            int32_t* const               theOutDistance)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theSources == nullptr || theSourceCount == 0
        || theOutDistance == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "graph, sources, or out_distance is NULL, or source_count is zero");
      return OCCTL_INVALID_ARGUMENT;
    }

    const BRepGraph_NodeId aRoot   = OcctL::Topo::UnpackNodeId(theRoot);
    const BRepGraph_NodeId aTarget = OcctL::Topo::UnpackNodeId(theTarget);
    if (!isActiveNode(theGraph->graph, aRoot) || !isActiveNode(theGraph->graph, aTarget))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "root or target is invalid or removed");
      return OCCTL_NOT_FOUND;
    }
    if (!isTopologicalDistanceKind(aTarget.NodeKind))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "target kind is not supported for graph distance");
      return OCCTL_WRONG_KIND;
    }

    NCollection_LinearVector<BRepGraph_NodeId> aSources;
    for (size_t anIndex = 0; anIndex < theSourceCount; ++anIndex)
    {
      const BRepGraph_NodeId aSource = OcctL::Topo::UnpackNodeId(theSources[anIndex]);
      if (!isActiveNode(theGraph->graph, aSource))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "a source node is invalid or removed");
        return OCCTL_NOT_FOUND;
      }
      if (aSource.NodeKind != aTarget.NodeKind)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                               "sources and target must have the same kind");
        return OCCTL_WRONG_KIND;
      }
      aSources.Append(aSource);
    }

    BRepGraph&                                 aGraph = const_cast<BRepGraph&>(theGraph->graph);
    NCollection_LinearVector<BRepGraph_NodeId> aPeers;
    collectPeerNodes(aGraph, aRoot, aTarget.NodeKind, aPeers);

    NCollection_FlatMap<uint64_t> aPeerBits;
    for (size_t aPeerIndex = 0; aPeerIndex < aPeers.Size(); ++aPeerIndex)
    {
      const BRepGraph_NodeId aPeer = aPeers.Value(aPeerIndex);
      aPeerBits.Add(OcctL::Topo::PackNodeId(aPeer).bits);
    }

    const uint64_t aTargetBits = OcctL::Topo::PackNodeId(aTarget).bits;
    if (aPeerBits.Seek(aTargetBits) == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "target is not reachable from root");
      return OCCTL_NOT_FOUND;
    }

    NCollection_LinearVector<BRepGraph_NodeId> aQueue;
    NCollection_FlatDataMap<uint64_t, int32_t> aDistances;
    for (size_t aSourceIndex = 0; aSourceIndex < aSources.Size(); ++aSourceIndex)
    {
      const BRepGraph_NodeId aSource     = aSources.Value(aSourceIndex);
      const uint64_t         aSourceBits = OcctL::Topo::PackNodeId(aSource).bits;
      if (aPeerBits.Seek(aSourceBits) == nullptr)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "a source is not reachable from root");
        return OCCTL_NOT_FOUND;
      }
      if (aDistances.Seek(aSourceBits) == nullptr)
      {
        aDistances.Bind(aSourceBits, 0);
        aQueue.Append(aSource);
      }
    }

    NCollection_FlatDataMap<uint64_t, NCollection_LinearVector<BRepGraph_NodeId>>
      aSharedConnectorMap;
    if (aTarget.NodeKind == BRepGraph_NodeId::Kind::Wire
        || aTarget.NodeKind == BRepGraph_NodeId::Kind::Shell
        || aTarget.NodeKind == BRepGraph_NodeId::Kind::Solid)
    {
      buildSharedDescendantMap(aGraph,
                               aPeers,
                               aTarget.NodeKind == BRepGraph_NodeId::Kind::Wire,
                               aSharedConnectorMap);
    }

    *theOutDistance = -1;
    for (size_t aQueueHead = 0; aQueueHead < aQueue.Size(); ++aQueueHead)
    {
      const BRepGraph_NodeId aCurrent         = aQueue.Value(aQueueHead);
      const uint64_t         aCurrentBits     = OcctL::Topo::PackNodeId(aCurrent).bits;
      const int32_t*         aCurrentDistance = aDistances.Seek(aCurrentBits);
      if (aCurrentDistance == nullptr)
      {
        continue;
      }
      if (aCurrentBits == aTargetBits)
      {
        *theOutDistance = *aCurrentDistance;
        return OCCTL_OK;
      }

      NCollection_LinearVector<BRepGraph_NodeId> aNeighbours;
      collectTopologicalDistanceNeighbours(aGraph,
                                           aCurrent,
                                           aPeerBits,
                                           aSharedConnectorMap,
                                           aNeighbours);
      for (size_t aNeighbourIndex = 0; aNeighbourIndex < aNeighbours.Size(); ++aNeighbourIndex)
      {
        const BRepGraph_NodeId aNeighbour     = aNeighbours.Value(aNeighbourIndex);
        const uint64_t         aNeighbourBits = OcctL::Topo::PackNodeId(aNeighbour).bits;
        if (aDistances.Seek(aNeighbourBits) != nullptr)
        {
          continue;
        }
        aDistances.Bind(aNeighbourBits, *aCurrentDistance + 1);
        aQueue.Append(aNeighbour);
      }
    }

    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_classify_point(const occtl_graph_t* const      theGraph,
                            const occtl_node_id_t           theSolid,
                            const occtl_point3_t            thePoint,
                            const double                    theTolerance,
                            occtl_topo_point_class_t* const theOutClass)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutClass == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_class is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutClass = OCCTL_TOPO_POINT_CLASS_UNKNOWN;
    return classifySolidPoint(theGraph, theSolid, thePoint, theTolerance, theOutClass);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_topo_is_inside(const occtl_graph_t* const theGraph,
                                                         const occtl_node_id_t      theSolid,
                                                         const occtl_point3_t       thePoint,
                                                         const double               theTolerance,
                                                         const int32_t  theIncludeBoundary,
                                                         int32_t* const theOutIsInside)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIsInside == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph or out_is_inside is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theIncludeBoundary != 0 && theIncludeBoundary != 1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "include_boundary must be 0 or 1");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutIsInside                 = 0;
    occtl_topo_point_class_t aClass = OCCTL_TOPO_POINT_CLASS_UNKNOWN;
    const occtl_status_t     aStatus =
      classifySolidPoint(theGraph, theSolid, thePoint, theTolerance, &aClass);
    if (aStatus != OCCTL_OK)
    {
      return aStatus;
    }

    *theOutIsInside = (aClass == OCCTL_TOPO_POINT_CLASS_IN
                       || (theIncludeBoundary == 1 && aClass == OCCTL_TOPO_POINT_CLASS_ON))
                        ? 1
                        : 0;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_axis_hit_iter_next(occtl_topo_axis_hit_iter_t* const theIter,
                                occtl_topo_axis_hit_t* const      theOutHit)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theIter == nullptr || theOutHit == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "iter or out_hit is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutHit = occtl_topo_axis_hit_t{};
    if (theIter->index >= theIter->hits.Size())
    {
      return OCCTL_NOT_FOUND;
    }

    *theOutHit = theIter->hits.Value(theIter->index++);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_axis_hit_iter_free(occtl_topo_axis_hit_iter_t* const theIter)
{
  delete theIter;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_touch_iter_next(occtl_topo_touch_iter_t* const theIter,
                             occtl_topo_touch_hit_t* const  theOutHit)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theIter == nullptr || theOutHit == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "iter or out_hit is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutHit = occtl_topo_touch_hit_t{};
    if (theIter->index >= theIter->hits.Size())
    {
      return OCCTL_NOT_FOUND;
    }

    *theOutHit = theIter->hits.Value(theIter->index++);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_touch_iter_free(occtl_topo_touch_iter_t* const theIter)
{
  delete theIter;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_intersection_iter_next(occtl_topo_intersection_iter_t* const theIter,
                                    occtl_node_id_t* const                theOutNode)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theIter == nullptr || theOutNode == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "iter or out_node is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutNode = OCCTL_NODE_ID_INVALID;
    if (theIter->index >= theIter->nodes.Size())
    {
      return OCCTL_NOT_FOUND;
    }

    *theOutNode = theIter->nodes.Value(theIter->index++);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_topo_intersection_iter_free(occtl_topo_intersection_iter_t* const theIter)
{
  delete theIter;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_related_iter_next(occtl_topo_related_iter_t* const theIter,
                               occtl_node_id_t* const           theOutNode,
                               occtl_relation_kind_t* const     theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theIter == nullptr || theOutNode == nullptr || theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theIter ? "an out-param is NULL" : "iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!theIter->impl.More())
    {
      *theOutNode = OCCTL_NODE_ID_INVALID;
      *theOutKind = static_cast<occtl_relation_kind_t>(0);
      return OCCTL_NOT_FOUND;
    }

    *theOutNode = OcctL::Topo::PackNodeId(theIter->impl.Current());
    *theOutKind =
      static_cast<occtl_relation_kind_t>(static_cast<int>(theIter->impl.CurrentRelation()));
    theIter->impl.Next();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_topo_related_iter_free(occtl_topo_related_iter_t* const theIter)
{
  delete theIter;
}

} // extern "C"
