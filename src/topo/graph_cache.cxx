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
#include "GraphGeometryKind.hxx"
#include "GraphGeometryKindCache.hxx"
#include "GraphHandle.hxx"
#include "GraphMeasureCache.hxx"
#include "GraphPairDistanceCache.hxx"
#include "TopoMath.hxx"

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <BRepBndLib.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepGraphAlgo_BndLib.hxx>
#include <BRepGraphAlgo_UVBounds.hxx>
#include <BRepGraph_ChildExplorer.hxx>
#include <BRepGraph_Iterator.hxx>
#include <BRepGraph_RelatedIterator.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_TopoView.hxx>
#include <NCollection_FlatDataMap.hxx>
#include <NCollection_FlatMap.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Mat.hxx>
#include <gp_Pnt.hxx>
#include <gp_XYZ.hxx>
#include <Bnd_Box.hxx>
#include <Bnd_OBB.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>

#include <NCollection_LinearVector.hxx>
#include <cmath>
#include <cstring>

namespace
{

bool IsFiniteValue(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool isActiveNode(const BRepGraph& theGraph, const BRepGraph_NodeId theNode)
{
  return theNode.IsValid() && !theGraph.Topo().Gen().IsRemoved(theNode);
}

bool isMeasureKindApplicable(const BRepGraph_NodeId            theNode,
                             const occtl_select_measure_kind_t theKind)
{
  switch (theKind)
  {
    case OCCTL_SELECT_MEASURE_EDGE_LENGTH:
      return theNode.NodeKind == BRepGraph_NodeId::Kind::Edge;
    case OCCTL_SELECT_MEASURE_WIRE_LENGTH:
      return theNode.NodeKind == BRepGraph_NodeId::Kind::Wire;
    case OCCTL_SELECT_MEASURE_FACE_AREA:
      return theNode.NodeKind == BRepGraph_NodeId::Kind::Face;
    case OCCTL_SELECT_MEASURE_SURFACE_AREA:
    case OCCTL_SELECT_MEASURE_VOLUME:
      return true;
    case OCCTL_SELECT_MEASURE_KIND_RESERVED_FUTURE:
      return false;
  }
  return false;
}

bool isKnownMeasureKind(const occtl_select_measure_kind_t theKind)
{
  switch (theKind)
  {
    case OCCTL_SELECT_MEASURE_EDGE_LENGTH:
    case OCCTL_SELECT_MEASURE_WIRE_LENGTH:
    case OCCTL_SELECT_MEASURE_FACE_AREA:
    case OCCTL_SELECT_MEASURE_SURFACE_AREA:
    case OCCTL_SELECT_MEASURE_VOLUME:
      return true;
    case OCCTL_SELECT_MEASURE_KIND_RESERVED_FUTURE:
      return false;
  }
  return false;
}

occtl_status_t resolveNode(const occtl_graph_t* const theGraph,
                           const occtl_node_id_t      theNode,
                           BRepGraph_NodeId&          theOutNode)
{
  if (theGraph == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }

  theOutNode = OcctL::Topo::UnpackNodeId(theNode);
  if (!isActiveNode(theGraph->graph, theOutNode))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "theNode is invalid or removed");
    return OCCTL_NOT_FOUND;
  }
  return OCCTL_OK;
}

bool computeObb(BRepGraph& theGraph, const BRepGraph_NodeId theNode, Bnd_OBB& theOutObb)
{
  const TopoDS_Shape aShape = theGraph.Shapes().Shape(theNode);
  if (aShape.IsNull())
  {
    return false;
  }

  BRepBndLib::AddOBB(aShape, theOutObb, true, false, true);
  return !theOutObb.IsVoid();
}

bool computedObb(BRepGraph& theGraph, const BRepGraph_NodeId theNode, Bnd_OBB& theOutObb)
{
  return computeObb(theGraph, theNode, theOutObb);
}

occtl_direction3_t fromDirection(const gp_XYZ& theDirection)
{
  return {theDirection.X(), theDirection.Y(), theDirection.Z()};
}

void fillObb(const Bnd_OBB& theObb, occtl_graph_obb_t& theOutObb)
{
  const gp_XYZ aCenter  = theObb.Center();
  theOutObb.center      = {aCenter.X(), aCenter.Y(), aCenter.Z()};
  theOutObb.x_direction = fromDirection(theObb.XDirection());
  theOutObb.y_direction = fromDirection(theObb.YDirection());
  theOutObb.z_direction = fromDirection(theObb.ZDirection());
  theOutObb.x_half_size = theObb.XHSize();
  theOutObb.y_half_size = theObb.YHSize();
  theOutObb.z_half_size = theObb.ZHSize();
}

bool isFiniteUvBounds(const BRepGraphAlgo_UVBounds::CachedData& theData)
{
  return theData.IsValid && IsFiniteValue(theData.UMin) && IsFiniteValue(theData.UMax)
         && IsFiniteValue(theData.VMin) && IsFiniteValue(theData.VMax);
}

void fillUvBounds(const BRepGraphAlgo_UVBounds::CachedData& theData,
                  occtl_graph_uv_bounds_t&                  theOutUvBounds)
{
  theOutUvBounds.u_min                  = theData.UMin;
  theOutUvBounds.u_max                  = theData.UMax;
  theOutUvBounds.v_min                  = theData.VMin;
  theOutUvBounds.v_max                  = theData.VMax;
  theOutUvBounds.is_natural_restriction = theData.IsNaturalRestriction ? 1 : 0;
}

bool isFiniteMassProperties(const occtl_graph_mass_properties_t& theProperties)
{
  if (!IsFiniteValue(theProperties.linear_length) || !IsFiniteValue(theProperties.surface_area)
      || !IsFiniteValue(theProperties.volume) || !IsFiniteValue(theProperties.mass)
      || !IsFiniteValue(theProperties.centre_of_mass.x)
      || !IsFiniteValue(theProperties.centre_of_mass.y)
      || !IsFiniteValue(theProperties.centre_of_mass.z))
  {
    return false;
  }
  for (const double aValue : theProperties.inertia)
  {
    if (!IsFiniteValue(aValue))
    {
      return false;
    }
  }
  return true;
}

void fillMassProperties(const GProp_GProps&            theProps,
                        const double                   theLinearLength,
                        const double                   theSurfaceArea,
                        const double                   theVolume,
                        occtl_graph_mass_properties_t& theOutProperties)
{
  const gp_Pnt aCentre   = theProps.CentreOfMass();
  const gp_Mat anInertia = theProps.MatrixOfInertia();

  theOutProperties.linear_length  = theLinearLength;
  theOutProperties.surface_area   = theSurfaceArea;
  theOutProperties.volume         = theVolume;
  theOutProperties.mass           = theProps.Mass();
  theOutProperties.centre_of_mass = {aCentre.X(), aCentre.Y(), aCentre.Z()};
  for (int aRow = 1; aRow <= 3; ++aRow)
  {
    for (int aCol = 1; aCol <= 3; ++aCol)
    {
      theOutProperties.inertia[(aRow - 1) * 3 + (aCol - 1)] = anInertia.Value(aRow, aCol);
    }
  }
}

bool computeMassProperties(BRepGraph&                     theGraph,
                           const BRepGraph_NodeId         theNode,
                           occtl_graph_mass_properties_t& theOutProperties)
{
  const TopoDS_Shape aShape = theGraph.Shapes().Shape(theNode);
  if (aShape.IsNull())
  {
    return false;
  }

  GProp_GProps aLinearProps;
  GProp_GProps aSurfaceProps;
  GProp_GProps aVolumeProps;
  BRepGProp::LinearProperties(aShape, aLinearProps);
  BRepGProp::SurfaceProperties(aShape, aSurfaceProps);
  BRepGProp::VolumeProperties(aShape, aVolumeProps);

  const double aLinearLength = aLinearProps.Mass();
  const double aSurfaceArea  = aSurfaceProps.Mass();
  const double aVolume       = aVolumeProps.Mass();

  const GProp_GProps& aBestProps =
    aVolume != 0.0 ? aVolumeProps : (aSurfaceArea != 0.0 ? aSurfaceProps : aLinearProps);
  fillMassProperties(aBestProps, aLinearLength, aSurfaceArea, aVolume, theOutProperties);
  return isFiniteMassProperties(theOutProperties);
}

bool computedMassProperties(BRepGraph&                     theGraph,
                            const BRepGraph_NodeId         theNode,
                            occtl_graph_mass_properties_t& theOutProperties)
{
  return computeMassProperties(theGraph, theNode, theOutProperties);
}

bool computePairDistance(BRepGraph&             theGraph,
                         const BRepGraph_NodeId theFirst,
                         const BRepGraph_NodeId theSecond,
                         double&                theOutDistance)
{
  if (theFirst == theSecond)
  {
    theOutDistance = 0.0;
    return true;
  }

  const TopoDS_Shape aFirstShape  = theGraph.Shapes().Shape(theFirst);
  const TopoDS_Shape aSecondShape = theGraph.Shapes().Shape(theSecond);
  if (aFirstShape.IsNull() || aSecondShape.IsNull())
  {
    return false;
  }

  BRepExtrema_DistShapeShape aDistance(aFirstShape, aSecondShape);
  aDistance.Perform();
  if (!aDistance.IsDone())
  {
    return false;
  }

  theOutDistance = aDistance.Value();
  return IsFiniteValue(theOutDistance) && theOutDistance >= 0.0;
}

bool computedPairDistance(BRepGraph&             theGraph,
                          const BRepGraph_NodeId theFirst,
                          const BRepGraph_NodeId theSecond,
                          double&                theOutDistance)
{
  return computePairDistance(theGraph, theFirst, theSecond, theOutDistance);
}

bool computedEdgeCurveKind(BRepGraph&             theGraph,
                           const BRepGraph_EdgeId theEdge,
                           occtl_curve_kind_t&    theOutKind)
{
  theOutKind = OcctL::Topo::EdgeCurveKind(theGraph, theEdge);
  return OcctL::Topo::IsKnownCurveKind(theOutKind);
}

bool computedFaceSurfaceKind(BRepGraph&             theGraph,
                             const BRepGraph_FaceId theFace,
                             occtl_surface_kind_t&  theOutKind)
{
  theOutKind = OcctL::Topo::FaceSurfaceKind(theGraph, theFace);
  return OcctL::Topo::IsKnownSurfaceKind(theOutKind);
}

bool computeDescendantNodes(BRepGraph&                                 theGraph,
                            const BRepGraph_NodeId                     theRoot,
                            const BRepGraph_NodeId::Kind               theKind,
                            NCollection_LinearVector<occtl_node_id_t>& theOutNodes)
{
  if (!isActiveNode(theGraph, theRoot))
  {
    return false;
  }

  NCollection_FlatMap<uint64_t> aSeen;
  const auto                    addNode = [&](const BRepGraph_NodeId theNode) {
    if (!theNode.IsValid())
    {
      return;
    }
    const occtl_node_id_t anAbiNode = OcctL::Topo::PackNodeId(theNode);
    if (aSeen.Add(anAbiNode.bits))
    {
      theOutNodes.Append(anAbiNode);
    }
  };

  if (theRoot.NodeKind == theKind)
  {
    addNode(theRoot);
  }

  BRepGraph_ChildExplorer anExplorer(theGraph, theRoot, theKind);
  for (; anExplorer.More(); anExplorer.Next())
  {
    addNode(anExplorer.Current().DefId);
  }
  return true;
}

bool computeDescendantVertices(BRepGraph&                                 theGraph,
                               const BRepGraph_NodeId                     theRoot,
                               NCollection_LinearVector<occtl_node_id_t>& theOutVertices)
{
  return computeDescendantNodes(theGraph, theRoot, BRepGraph_NodeId::Kind::Vertex, theOutVertices);
}

bool computedDescendantVertices(BRepGraph&                                 theGraph,
                                const BRepGraph_NodeId                     theRoot,
                                NCollection_LinearVector<occtl_node_id_t>& theOutVertices)
{
  return computeDescendantVertices(theGraph, theRoot, theOutVertices);
}

bool computedDescendantNodes(BRepGraph&                                 theGraph,
                             const BRepGraph_NodeId                     theRoot,
                             const BRepGraph_NodeId::Kind               theKind,
                             NCollection_LinearVector<occtl_node_id_t>& theOutNodes)
{
  return computeDescendantNodes(theGraph, theRoot, theKind, theOutNodes);
}

bool computedDescendantEdges(BRepGraph&                                 theGraph,
                             const BRepGraph_NodeId                     theRoot,
                             NCollection_LinearVector<occtl_node_id_t>& theOutEdges)
{
  return computedDescendantNodes(theGraph, theRoot, BRepGraph_NodeId::Kind::Edge, theOutEdges);
}

bool computedDescendantFaces(BRepGraph&                                 theGraph,
                             const BRepGraph_NodeId                     theRoot,
                             NCollection_LinearVector<occtl_node_id_t>& theOutFaces)
{
  return computedDescendantNodes(theGraph, theRoot, BRepGraph_NodeId::Kind::Face, theOutFaces);
}

bool computedDescendantsByKind(BRepGraph&                                 theGraph,
                               const BRepGraph_NodeId                     theRoot,
                               const occtl_node_kind_t                    theKind,
                               NCollection_LinearVector<occtl_node_id_t>& theOutNodes)
{
  BRepGraph_NodeId::Kind anOcctKind;
  if (!OcctL::Topo::TryToOcctNodeKind(theKind, anOcctKind))
  {
    return false;
  }
  return computeDescendantNodes(theGraph, theRoot, anOcctKind, theOutNodes);
}

bool computeAdjacentFaces(BRepGraph&                                 theGraph,
                          const BRepGraph_FaceId                     theFace,
                          NCollection_LinearVector<occtl_node_id_t>& theOutFaces)
{
  if (!isActiveNode(theGraph, BRepGraph_NodeId(theFace)))
  {
    return false;
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
  return true;
}

bool computedAdjacentFaces(BRepGraph&                                 theGraph,
                           const BRepGraph_FaceId                     theFace,
                           NCollection_LinearVector<occtl_node_id_t>& theOutFaces)
{
  return computeAdjacentFaces(theGraph, theFace, theOutFaces);
}

bool computeAdjacentEdges(BRepGraph&                                 theGraph,
                          const BRepGraph_EdgeId                     theEdge,
                          NCollection_LinearVector<occtl_node_id_t>& theOutEdges)
{
  if (!isActiveNode(theGraph, BRepGraph_NodeId(theEdge)))
  {
    return false;
  }

  NCollection_FlatMap<uint64_t> aSeen;
  const BRepGraph_VertexId aVertices[2] = {BRepGraph_Tool::Edge::StartVertexId(theGraph, theEdge),
                                           BRepGraph_Tool::Edge::EndVertexId(theGraph, theEdge)};
  for (const BRepGraph_VertexId& aVertex : aVertices)
  {
    if (!isActiveNode(theGraph, BRepGraph_NodeId(aVertex)))
    {
      continue;
    }

    const NCollection_DynamicArray<BRepGraph_EdgeId>& anEdges =
      theGraph.Topo().Vertices().Edges(aVertex);
    for (int anI = anEdges.Lower(); anI <= anEdges.Upper(); ++anI)
    {
      const BRepGraph_NodeId anEdge(anEdges.Value(anI));
      if (anEdge == BRepGraph_NodeId(theEdge) || !isActiveNode(theGraph, anEdge))
      {
        continue;
      }

      const occtl_node_id_t anAbiEdge = OcctL::Topo::PackNodeId(anEdge);
      if (aSeen.Add(anAbiEdge.bits))
      {
        theOutEdges.Append(anAbiEdge);
      }
    }
  }
  return true;
}

bool computedAdjacentEdges(BRepGraph&                                 theGraph,
                           const BRepGraph_EdgeId                     theEdge,
                           NCollection_LinearVector<occtl_node_id_t>& theOutEdges)
{
  return computeAdjacentEdges(theGraph, theEdge, theOutEdges);
}

void copyNodeList(const NCollection_LinearVector<occtl_node_id_t>& theNodes,
                  occtl_node_id_t* const                           theOutBuf)
{
  for (size_t anIndex = 0; anIndex < theNodes.Size(); ++anIndex)
  {
    theOutBuf[anIndex] = theNodes.Value(anIndex);
  }
}

} // namespace

namespace OcctL::Topo
{

bool ComputePairDistance(BRepGraph&             theGraph,
                         const BRepGraph_NodeId theFirst,
                         const BRepGraph_NodeId theSecond,
                         double&                theOutDistance)
{
  return computedPairDistance(theGraph, theFirst, theSecond, theOutDistance);
}

bool ComputeEdgeCurveKind(BRepGraph&             theGraph,
                          const BRepGraph_EdgeId theEdge,
                          occtl_curve_kind_t&    theOutKind)
{
  return computedEdgeCurveKind(theGraph, theEdge, theOutKind);
}

bool ComputeFaceSurfaceKind(BRepGraph&             theGraph,
                            const BRepGraph_FaceId theFace,
                            occtl_surface_kind_t&  theOutKind)
{
  return computedFaceSurfaceKind(theGraph, theFace, theOutKind);
}

bool ComputeDescendantVertices(BRepGraph&                                 theGraph,
                               const BRepGraph_NodeId                     theRoot,
                               NCollection_LinearVector<occtl_node_id_t>& theOutVertices)
{
  return computedDescendantVertices(theGraph, theRoot, theOutVertices);
}

bool ComputeDescendantEdges(BRepGraph&                                 theGraph,
                            const BRepGraph_NodeId                     theRoot,
                            NCollection_LinearVector<occtl_node_id_t>& theOutEdges)
{
  return computedDescendantEdges(theGraph, theRoot, theOutEdges);
}

bool ComputeDescendantFaces(BRepGraph&                                 theGraph,
                            const BRepGraph_NodeId                     theRoot,
                            NCollection_LinearVector<occtl_node_id_t>& theOutFaces)
{
  return computedDescendantFaces(theGraph, theRoot, theOutFaces);
}

bool ComputeDescendantsByKind(BRepGraph&                                 theGraph,
                              const BRepGraph_NodeId                     theRoot,
                              const occtl_node_kind_t                    theKind,
                              NCollection_LinearVector<occtl_node_id_t>& theOutNodes)
{
  return computedDescendantsByKind(theGraph, theRoot, theKind, theOutNodes);
}

bool ComputeAdjacentFaces(BRepGraph&                                 theGraph,
                          const BRepGraph_FaceId                     theFace,
                          NCollection_LinearVector<occtl_node_id_t>& theOutFaces)
{
  return computedAdjacentFaces(theGraph, theFace, theOutFaces);
}

bool ComputeAdjacentEdges(BRepGraph&                                 theGraph,
                          const BRepGraph_EdgeId                     theEdge,
                          NCollection_LinearVector<occtl_node_id_t>& theOutEdges)
{
  return computedAdjacentEdges(theGraph, theEdge, theOutEdges);
}

} // namespace OcctL::Topo

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_clear_cached(occtl_graph_t* const  theGraph,
                                                             const occtl_node_id_t theNode,
                                                             const occtl_ref_id_t  theRef)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theGraph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    const bool hasNode = theNode.bits != 0;
    const bool hasRef  = theRef.bits != 0;
    if (hasNode == hasRef)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "exactly one of theNode or theRef must be valid");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (hasNode)
    {
      BRepGraph_NodeId     aNode;
      const occtl_status_t aStatus = resolveNode(theGraph, theNode, aNode);
      if (aStatus != OCCTL_OK)
      {
        return aStatus;
      }

      theGraph->graph.Shapes().ClearCached(aNode);
    }
    else
    {
      occtl_ref_kind_t     aKind   = OCCTL_REF_KIND_INVALID;
      const occtl_status_t aStatus = occtl_graph_ref_kind(theGraph, theRef, &aKind);
      if (aStatus != OCCTL_OK)
      {
        return aStatus;
      }

      const BRepGraph_RefId aRef = OcctL::Topo::UnpackRefId(theRef);
      theGraph->graph.Shapes().ClearCached(aRef);
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_bbox_get(occtl_graph_t* const       theGraph,
                                                         const occtl_node_id_t      theNode,
                                                         occtl_select_bbox_t* const theOutBbox)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutBbox == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutBbox is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aNode;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theNode, aNode))
    {
      return aStatus;
    }

    const Bnd_Box aBox = BRepGraphAlgo_BndLib::AddCached(theGraph->graph,
                                                         aNode,
                                                         BRepGraphAlgo_BndLib::Precision::Standard,
                                                         true);
    if (aBox.IsVoid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not compute bounding box");
      return OCCTL_GEOMETRY_INVALID;
    }

    const gp_Pnt aMin = aBox.CornerMin();
    const gp_Pnt aMax = aBox.CornerMax();
    theOutBbox->min   = {aMin.X(), aMin.Y(), aMin.Z()};
    theOutBbox->max   = {aMax.X(), aMax.Y(), aMax.Z()};
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_obb_get(occtl_graph_t* const     theGraph,
                                                        const occtl_node_id_t    theNode,
                                                        occtl_graph_obb_t* const theOutObb)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutObb == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutObb is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aNode;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theNode, aNode))
    {
      return aStatus;
    }

    Bnd_OBB anObb;
    if (!computedObb(theGraph->graph, aNode, anObb))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not compute oriented bounding box");
      return OCCTL_GEOMETRY_INVALID;
    }

    fillObb(anObb, *theOutObb);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_face_uv_bounds_get(occtl_graph_t* const           theGraph,
                                 const occtl_node_id_t          theFace,
                                 occtl_graph_uv_bounds_t* const theOutUvBounds)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutUvBounds == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutUvBounds is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aFace;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theFace, aFace))
    {
      return aStatus;
    }
    if (aFace.NodeKind != BRepGraph_NodeId::Kind::Face)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "theFace is not a Face node");
      return OCCTL_WRONG_KIND;
    }

    const BRepGraphAlgo_UVBounds::CachedData aData =
      BRepGraphAlgo_UVBounds::AddCached(theGraph->graph, BRepGraph_FaceId::FromNodeId(aFace));
    if (!isFiniteUvBounds(aData))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not compute Face UV bounds");
      return OCCTL_GEOMETRY_INVALID;
    }

    fillUvBounds(aData, *theOutUvBounds);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_measure_get(occtl_graph_t* const              theGraph,
                          const occtl_node_id_t             theNode,
                          const occtl_select_measure_kind_t theKind,
                          double* const                     theOutValue)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutValue == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutValue is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aNode;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theNode, aNode))
    {
      return aStatus;
    }

    if (!isKnownMeasureKind(theKind))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theKind is not a public measure kind");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (!isMeasureKindApplicable(aNode, theKind))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND,
                                             "measure kind is not applicable to the node");
      return OCCTL_WRONG_KIND;
    }

    double aValue = 0.0;
    if (!OcctL::Topo::ComputeMeasureValue(theGraph->graph, aNode, theKind, aValue))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not compute measure value");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutValue = aValue;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_pair_distance_get(occtl_graph_t* const  theGraph,
                                                                  const occtl_node_id_t theFirst,
                                                                  const occtl_node_id_t theSecond,
                                                                  double* const theOutDistance)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutDistance == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutDistance is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aFirst;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theFirst, aFirst))
    {
      return aStatus;
    }
    BRepGraph_NodeId aSecond;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theSecond, aSecond))
    {
      return aStatus;
    }

    double aDistance = 0.0;
    if (!OcctL::Topo::ComputePairDistance(theGraph->graph, aFirst, aSecond, aDistance))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not compute pair distance");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutDistance = aDistance;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_mass_properties_get(occtl_graph_t* const                 theGraph,
                                  const occtl_node_id_t                theNode,
                                  occtl_graph_mass_properties_t* const theOutProperties)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutProperties == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutProperties is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aNode;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theNode, aNode))
    {
      return aStatus;
    }

    occtl_graph_mass_properties_t aProperties{};
    if (!computedMassProperties(theGraph->graph, aNode, aProperties))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not compute mass properties");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutProperties = aProperties;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_edge_curve_kind_get(occtl_graph_t* const      theGraph,
                                  const occtl_node_id_t     theEdge,
                                  occtl_curve_kind_t* const theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutKind is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId anEdge;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theEdge, anEdge))
    {
      return aStatus;
    }
    if (anEdge.NodeKind != BRepGraph_NodeId::Kind::Edge)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "theEdge is not an Edge node");
      return OCCTL_WRONG_KIND;
    }

    occtl_curve_kind_t aKind = OCCTL_CURVE_KIND_UNDEFINED;
    if (!computedEdgeCurveKind(theGraph->graph, BRepGraph_EdgeId(anEdge), aKind))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not classify Edge curve kind");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutKind = aKind;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_face_surface_kind_get(occtl_graph_t* const        theGraph,
                                    const occtl_node_id_t       theFace,
                                    occtl_surface_kind_t* const theOutKind)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutKind == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutKind is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aFace;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theFace, aFace))
    {
      return aStatus;
    }
    if (aFace.NodeKind != BRepGraph_NodeId::Kind::Face)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "theFace is not a Face node");
      return OCCTL_WRONG_KIND;
    }

    occtl_surface_kind_t aKind = OCCTL_SURFACE_KIND_UNDEFINED;
    if (!computedFaceSurfaceKind(theGraph->graph, BRepGraph_FaceId(aFace), aKind))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not classify Face surface kind");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutKind = aKind;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_descendant_vertices_get(occtl_graph_t* const   theGraph,
                                      const occtl_node_id_t  theNode,
                                      occtl_node_id_t* const theOutBuf,
                                      const size_t           theCap,
                                      size_t* const          theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aNode;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theNode, aNode))
    {
      return aStatus;
    }

    NCollection_LinearVector<occtl_node_id_t> aVertices;
    if (!computedDescendantVertices(theGraph->graph, aNode, aVertices))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepGraph could not compute descendant vertices");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutCount = aVertices.Size();
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aVertices.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for descendant vertex list");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    copyNodeList(aVertices, theOutBuf);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_descendant_edges_get(occtl_graph_t* const   theGraph,
                                   const occtl_node_id_t  theNode,
                                   occtl_node_id_t* const theOutBuf,
                                   const size_t           theCap,
                                   size_t* const          theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aNode;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theNode, aNode))
    {
      return aStatus;
    }

    NCollection_LinearVector<occtl_node_id_t> anEdges;
    if (!computedDescendantEdges(theGraph->graph, aNode, anEdges))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepGraph could not compute descendant edges");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutCount = anEdges.Size();
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < anEdges.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for descendant edge list");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    copyNodeList(anEdges, theOutBuf);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_descendant_faces_get(occtl_graph_t* const   theGraph,
                                   const occtl_node_id_t  theNode,
                                   occtl_node_id_t* const theOutBuf,
                                   const size_t           theCap,
                                   size_t* const          theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aNode;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theNode, aNode))
    {
      return aStatus;
    }

    NCollection_LinearVector<occtl_node_id_t> aFaces;
    if (!computedDescendantFaces(theGraph->graph, aNode, aFaces))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepGraph could not compute descendant faces");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutCount = aFaces.Size();
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aFaces.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for descendant face list");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    copyNodeList(aFaces, theOutBuf);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_graph_descendants_get(occtl_graph_t* const    theGraph,
                              const occtl_node_id_t   theNode,
                              const occtl_node_kind_t theDescendantKind,
                              occtl_node_id_t* const  theOutBuf,
                              const size_t            theCap,
                              size_t* const           theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId::Kind anOcctKind;
    if (!OcctL::Topo::TryToOcctNodeKind(theDescendantKind, anOcctKind))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "theDescendantKind is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }
    (void)anOcctKind;

    BRepGraph_NodeId aNode;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theNode, aNode))
    {
      return aStatus;
    }

    NCollection_LinearVector<occtl_node_id_t> aNodes;
    if (!computedDescendantsByKind(theGraph->graph, aNode, theDescendantKind, aNodes))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepGraph could not compute descendants");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutCount = aNodes.Size();
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aNodes.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for descendant node list");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    copyNodeList(aNodes, theOutBuf);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_adjacent_faces_get(occtl_graph_t* const   theGraph,
                                                                   const occtl_node_id_t  theFace,
                                                                   occtl_node_id_t* const theOutBuf,
                                                                   const size_t           theCap,
                                                                   size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aFace;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theFace, aFace))
    {
      return aStatus;
    }
    if (aFace.NodeKind != BRepGraph_NodeId::Kind::Face)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "theFace is not a Face node");
      return OCCTL_WRONG_KIND;
    }

    NCollection_LinearVector<occtl_node_id_t> aFaces;
    if (!computedAdjacentFaces(theGraph->graph, BRepGraph_FaceId::FromNodeId(aFace), aFaces))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepGraph could not compute adjacent faces");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutCount = aFaces.Size();
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aFaces.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for adjacent face list");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    copyNodeList(aFaces, theOutBuf);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_adjacent_edges_get(occtl_graph_t* const   theGraph,
                                                                   const occtl_node_id_t  theEdge,
                                                                   occtl_node_id_t* const theOutBuf,
                                                                   const size_t           theCap,
                                                                   size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "theOutCount is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId anEdge;
    if (const occtl_status_t aStatus = resolveNode(theGraph, theEdge, anEdge))
    {
      return aStatus;
    }
    if (anEdge.NodeKind != BRepGraph_NodeId::Kind::Edge)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_WRONG_KIND, "theEdge is not an Edge node");
      return OCCTL_WRONG_KIND;
    }

    NCollection_LinearVector<occtl_node_id_t> anEdges;
    if (!computedAdjacentEdges(theGraph->graph, BRepGraph_EdgeId::FromNodeId(anEdge), anEdges))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepGraph could not compute adjacent edges");
      return OCCTL_GEOMETRY_INVALID;
    }

    *theOutCount = anEdges.Size();
    if (theOutBuf == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < anEdges.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "theCap is too small for adjacent edge list");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    copyNodeList(anEdges, theOutBuf);
    return OCCTL_OK;
  });
}

} // extern "C"
