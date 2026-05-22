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

#include "GraphGeometryKind.hxx"
#include "GraphGeometryKindCache.hxx"
#include "GraphHandle.hxx"
#include "GraphLayers.hxx"
#include "GraphMeasureCache.hxx"
#include "GraphPairDistanceCache.hxx"
#include "TopoMath.hxx"

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <BRepGraphAlgo_BndLib.hxx>
#include <BRepGraph_ChildExplorer.hxx>
#include <BRepGraph_Iterator.hxx>
#include <BRepGraph_Tool.hxx>
#include <BRepGraph_UID.hxx>
#include <BRepGraph_UIDsView.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <Quantity_ColorRGBA.hxx>

#include <NCollection_FlatMap.hxx>
#include <NCollection_LinearVector.hxx>
#include <TCollection_AsciiString.hxx>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

struct occtl_select_iter
{
  NCollection_LinearVector<occtl_node_id_t> nodes;
  size_t                                    index = 0;
};

struct occtl_select_group_iter
{
  struct Group
  {
    occtl_select_group_view_t                 view = OCCTL_SELECT_GROUP_VIEW_INIT;
    TCollection_AsciiString                   name;
    NCollection_LinearVector<occtl_node_id_t> nodes;
  };

  NCollection_LinearVector<Group> groups;
  size_t                          index = 0;
};

namespace
{

bool IsFiniteValue(const double theValue)
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

struct SelectConfig
{
  occtl_select_options_t               Options            = OCCTL_SELECT_OPTIONS_INIT;
  occtl_select_metadata_filter_t       MetadataFilter     = OCCTL_SELECT_METADATA_FILTER_INIT;
  occtl_select_distance_to_node_sort_t DistanceToNodeSort = OCCTL_SELECT_DISTANCE_TO_NODE_SORT_INIT;
  TCollection_AsciiString              TagFilter;
  bool                                 HasMetadataFilter     = false;
  bool                                 HasDistanceToNodeSort = false;
  bool                                 HasTagFilter          = false;
  bool                                 HasRoot               = false;
};

bool isKindEnabled(const uint64_t theMask, const BRepGraph_NodeId theNode)
{
  if (theMask == 0)
  {
    return true;
  }
  const occtl_node_kind_t aKind = OcctL::Topo::ToAbiNodeKind(theNode.NodeKind);
  if (aKind == OCCTL_KIND_INVALID || static_cast<unsigned int>(aKind) >= 64u)
  {
    return false;
  }
  return (theMask & (uint64_t(1) << static_cast<unsigned int>(aKind))) != 0;
}

bool bboxFromAbi(const occtl_select_bbox_t& theIn, Bnd_Box& theOut)
{
  if (theIn.min.x > theIn.max.x || theIn.min.y > theIn.max.y || theIn.min.z > theIn.max.z)
  {
    return false;
  }
  theOut.Add(gp_Pnt(theIn.min.x, theIn.min.y, theIn.min.z));
  theOut.Add(gp_Pnt(theIn.max.x, theIn.max.y, theIn.max.z));
  return true;
}

bool pointInsideBox(const gp_Pnt& thePoint, const occtl_select_bbox_t& theBox)
{
  return thePoint.X() >= theBox.min.x && thePoint.X() <= theBox.max.x
         && thePoint.Y() >= theBox.min.y && thePoint.Y() <= theBox.max.y
         && thePoint.Z() >= theBox.min.z && thePoint.Z() <= theBox.max.z;
}

double coordinateOf(const gp_Pnt& thePoint, const occtl_select_axis_t theAxis)
{
  switch (theAxis)
  {
    case OCCTL_SELECT_AXIS_X:
      return thePoint.X();
    case OCCTL_SELECT_AXIS_Y:
      return thePoint.Y();
    case OCCTL_SELECT_AXIS_Z:
      return thePoint.Z();
    case OCCTL_SELECT_AXIS_RESERVED_FUTURE:
      break;
  }
  return thePoint.Z();
}

bool isKnownAxis(const occtl_select_axis_t theAxis)
{
  switch (theAxis)
  {
    case OCCTL_SELECT_AXIS_X:
    case OCCTL_SELECT_AXIS_Y:
    case OCCTL_SELECT_AXIS_Z:
      return true;
    case OCCTL_SELECT_AXIS_RESERVED_FUTURE:
      return false;
  }
  return false;
}

bool isKnownAxisPosition(const occtl_select_axis_position_t thePosition)
{
  switch (thePosition)
  {
    case OCCTL_SELECT_AXIS_POSITION_MIN:
    case OCCTL_SELECT_AXIS_POSITION_MAX:
    case OCCTL_SELECT_AXIS_POSITION_CENTER:
      return true;
    case OCCTL_SELECT_AXIS_POSITION_RESERVED_FUTURE:
      return false;
  }
  return false;
}

bool isKnownNormalMode(const occtl_select_normal_mode_t theMode)
{
  switch (theMode)
  {
    case OCCTL_SELECT_NORMAL_PARALLEL:
    case OCCTL_SELECT_NORMAL_ANTIPARALLEL:
    case OCCTL_SELECT_NORMAL_EITHER:
      return true;
    case OCCTL_SELECT_NORMAL_MODE_RESERVED_FUTURE:
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

bool isKnownSortKey(const occtl_select_sort_key_t theKey)
{
  switch (theKey)
  {
    case OCCTL_SELECT_SORT_NONE:
    case OCCTL_SELECT_SORT_AXIS_COORDINATE:
    case OCCTL_SELECT_SORT_MEASURE:
    case OCCTL_SELECT_SORT_DISTANCE_TO_POINT:
    case OCCTL_SELECT_SORT_NAME:
    case OCCTL_SELECT_SORT_UID:
    case OCCTL_SELECT_SORT_DISTANCE_TO_NODE:
      return true;
    case OCCTL_SELECT_SORT_KEY_RESERVED_FUTURE:
      return false;
  }
  return false;
}

bool isKnownSortDirection(const occtl_select_sort_direction_t theDirection)
{
  switch (theDirection)
  {
    case OCCTL_SELECT_SORT_ASCENDING:
    case OCCTL_SELECT_SORT_DESCENDING:
      return true;
    case OCCTL_SELECT_SORT_DIRECTION_RESERVED_FUTURE:
      return false;
  }
  return false;
}

bool isKnownGroupKey(const occtl_select_group_key_t theKey)
{
  switch (theKey)
  {
    case OCCTL_SELECT_GROUP_KIND:
    case OCCTL_SELECT_GROUP_AXIS_COORDINATE:
    case OCCTL_SELECT_GROUP_CURVE_KIND:
    case OCCTL_SELECT_GROUP_SURFACE_KIND:
    case OCCTL_SELECT_GROUP_NAME:
    case OCCTL_SELECT_GROUP_COLOR:
      return true;
    case OCCTL_SELECT_GROUP_KEY_RESERVED_FUTURE:
      return false;
  }
  return false;
}

bool directionToVector(const occtl_direction3_t& theDirection, gp_Vec& theOut)
{
  theOut                         = gp_Vec(theDirection.x, theDirection.y, theDirection.z);
  const double aMagnitudeSquared = theOut.SquareMagnitude();
  if (!IsFiniteValue(aMagnitudeSquared) || aMagnitudeSquared <= Precision::SquareConfusion())
  {
    return false;
  }
  theOut.Normalize();
  return true;
}

bool boxInsideBox(const Bnd_Box& theCandidate, const Bnd_Box& theFilter)
{
  if (theCandidate.IsVoid())
  {
    return false;
  }
  const gp_Pnt aMin = theCandidate.CornerMin();
  const gp_Pnt aMax = theCandidate.CornerMax();
  return !theFilter.IsOut(aMin) && !theFilter.IsOut(aMax);
}

bool candidateCenter(BRepGraph& theGraph, const BRepGraph_NodeId theNode, gp_Pnt& theOutCenter)
{
  const Bnd_Box aBox = BRepGraphAlgo_BndLib::AddCached(theGraph,
                                                       theNode,
                                                       BRepGraphAlgo_BndLib::Precision::Standard,
                                                       true);
  if (aBox.IsVoid())
  {
    return false;
  }

  const gp_Pnt aMin = aBox.CornerMin();
  const gp_Pnt aMax = aBox.CornerMax();
  theOutCenter =
    gp_Pnt((aMin.X() + aMax.X()) * 0.5, (aMin.Y() + aMax.Y()) * 0.5, (aMin.Z() + aMax.Z()) * 0.5);
  return true;
}

bool measureMatches(BRepGraph&                    theGraph,
                    const BRepGraph_NodeId        theNode,
                    const occtl_select_options_t& theOptions)
{
  if (theOptions.use_measure == 0)
  {
    return true;
  }

  double aValue = 0.0;
  if (!OcctL::Topo::ComputeMeasureValue(theGraph, theNode, theOptions.measure_kind, aValue))
  {
    return false;
  }
  return aValue >= theOptions.measure_min && aValue <= theOptions.measure_max;
}

bool bboxMatches(BRepGraph&                    theGraph,
                 const BRepGraph_NodeId        theNode,
                 const occtl_select_options_t& theOptions)
{
  if (theOptions.use_bbox == 0)
  {
    return true;
  }

  Bnd_Box aFilter;
  if (!bboxFromAbi(theOptions.bbox, aFilter))
  {
    return false;
  }

  const Bnd_Box aCandidate =
    BRepGraphAlgo_BndLib::AddCached(theGraph,
                                    theNode,
                                    BRepGraphAlgo_BndLib::Precision::Standard,
                                    true);
  if (aCandidate.IsVoid())
  {
    return false;
  }

  switch (theOptions.bbox_mode)
  {
    case OCCTL_SELECT_BBOX_INSIDE:
      return boxInsideBox(aCandidate, aFilter);
    case OCCTL_SELECT_BBOX_CONTAINS_CENTER: {
      const gp_Pnt aMin = aCandidate.CornerMin();
      const gp_Pnt aMax = aCandidate.CornerMax();
      const gp_Pnt aCenter((aMin.X() + aMax.X()) * 0.5,
                           (aMin.Y() + aMax.Y()) * 0.5,
                           (aMin.Z() + aMax.Z()) * 0.5);
      return pointInsideBox(aCenter, theOptions.bbox);
    }
    case OCCTL_SELECT_BBOX_INTERSECTS:
    default:
      return !aCandidate.IsOut(aFilter);
  }
}

bool candidateCenterCoordinate(BRepGraph&                theGraph,
                               const occtl_node_id_t     theNode,
                               const occtl_select_axis_t theAxis,
                               double&                   theOutValue)
{
  const BRepGraph_NodeId aNode = OcctL::Topo::UnpackNodeId(theNode);
  if (!aNode.IsValid())
  {
    return false;
  }

  gp_Pnt aCenter;
  if (!candidateCenter(theGraph, aNode, aCenter))
  {
    return false;
  }

  theOutValue = coordinateOf(aCenter, theAxis);
  return true;
}

void filterAxisPosition(BRepGraph&                                 theGraph,
                        const occtl_select_options_t&              theOptions,
                        NCollection_LinearVector<occtl_node_id_t>& theNodes)
{
  if (theOptions.use_axis_position == 0 || theNodes.IsEmpty())
  {
    return;
  }

  NCollection_LinearVector<double> aValues;
  double                           aMin = std::numeric_limits<double>::infinity();
  double                           aMax = -std::numeric_limits<double>::infinity();

  for (size_t anIndex = 0; anIndex < theNodes.Size(); ++anIndex)
  {
    double aValue = 0.0;
    if (!candidateCenterCoordinate(theGraph, theNodes.Value(anIndex), theOptions.axis, aValue))
    {
      aValues.Append(std::numeric_limits<double>::quiet_NaN());
      continue;
    }
    aValues.Append(aValue);
    aMin = std::min(aMin, aValue);
    aMax = std::max(aMax, aValue);
  }

  if (!IsFiniteValue(aMin) || !IsFiniteValue(aMax))
  {
    theNodes.Clear();
    return;
  }

  double aTarget = aMax;
  switch (theOptions.axis_position)
  {
    case OCCTL_SELECT_AXIS_POSITION_MIN:
      aTarget = aMin;
      break;
    case OCCTL_SELECT_AXIS_POSITION_CENTER:
      aTarget = (aMin + aMax) * 0.5;
      break;
    case OCCTL_SELECT_AXIS_POSITION_MAX:
    default:
      aTarget = aMax;
      break;
  }

  const double                              aTolerance = std::max(0.0, theOptions.axis_tolerance);
  NCollection_LinearVector<occtl_node_id_t> aFiltered;
  for (size_t anIndex = 0; anIndex < theNodes.Size(); ++anIndex)
  {
    const double aValue = aValues.Value(anIndex);
    if (IsFiniteValue(aValue) && std::fabs(aValue - aTarget) <= aTolerance)
    {
      aFiltered.Append(theNodes.Value(anIndex));
    }
  }
  theNodes = aFiltered;
}

struct SortEntry
{
  occtl_node_id_t         Node   = OCCTL_NODE_ID_INVALID;
  double                  Number = 0.0;
  TCollection_AsciiString Text;
  uint64_t                Bits   = 0;
  bool                    HasKey = false;
};

bool sortNumber(BRepGraph&                    theGraph,
                const BRepGraph_NodeId        theNode,
                const occtl_select_options_t& theOptions,
                double&                       theOutValue)
{
  switch (theOptions.sort_key)
  {
    case OCCTL_SELECT_SORT_AXIS_COORDINATE: {
      gp_Pnt aCenter;
      if (!candidateCenter(theGraph, theNode, aCenter))
      {
        return false;
      }
      theOutValue = coordinateOf(aCenter, theOptions.sort_axis);
      return IsFiniteValue(theOutValue);
    }
    case OCCTL_SELECT_SORT_MEASURE:
      return OcctL::Topo::ComputeMeasureValue(theGraph,
                                              theNode,
                                              theOptions.sort_measure_kind,
                                              theOutValue);
    case OCCTL_SELECT_SORT_DISTANCE_TO_POINT: {
      gp_Pnt aCenter;
      if (!candidateCenter(theGraph, theNode, aCenter))
      {
        return false;
      }
      const gp_Pnt aPoint(theOptions.sort_point.x,
                          theOptions.sort_point.y,
                          theOptions.sort_point.z);
      theOutValue = aCenter.Distance(aPoint);
      return IsFiniteValue(theOutValue);
    }
    case OCCTL_SELECT_SORT_NONE:
    case OCCTL_SELECT_SORT_NAME:
    case OCCTL_SELECT_SORT_UID:
    case OCCTL_SELECT_SORT_DISTANCE_TO_NODE:
    case OCCTL_SELECT_SORT_KEY_RESERVED_FUTURE:
      break;
  }
  return false;
}

bool sortDistanceToNode(BRepGraph&             theGraph,
                        const BRepGraph_NodeId theNode,
                        const BRepGraph_NodeId theTarget,
                        double&                theOutValue)
{
  if (!theTarget.IsValid())
  {
    return false;
  }
  return OcctL::Topo::ComputePairDistance(theGraph, theNode, theTarget, theOutValue);
}

SortEntry makeSortEntry(BRepGraph&            theGraph,
                        const occtl_node_id_t theNode,
                        const SelectConfig&   theConfig)
{
  SortEntry anEntry;
  anEntry.Node = theNode;

  const BRepGraph_NodeId aNode = OcctL::Topo::UnpackNodeId(theNode);
  if (!aNode.IsValid())
  {
    return anEntry;
  }

  switch (theConfig.Options.sort_key)
  {
    case OCCTL_SELECT_SORT_AXIS_COORDINATE:
    case OCCTL_SELECT_SORT_MEASURE:
    case OCCTL_SELECT_SORT_DISTANCE_TO_POINT:
      anEntry.HasKey = sortNumber(theGraph, aNode, theConfig.Options, anEntry.Number);
      break;
    case OCCTL_SELECT_SORT_DISTANCE_TO_NODE: {
      const BRepGraph_NodeId aTarget =
        OcctL::Topo::UnpackNodeId(theConfig.DistanceToNodeSort.target);
      anEntry.HasKey = sortDistanceToNode(theGraph, aNode, aTarget, anEntry.Number);
      break;
    }
    case OCCTL_SELECT_SORT_NAME:
      anEntry.HasKey = OcctL::Topo::FindBuiltinName(theGraph, aNode, anEntry.Text);
      break;
    case OCCTL_SELECT_SORT_UID: {
      const BRepGraph_UID aUid  = theGraph.UIDs().Of(aNode);
      const occtl_uid_t   anUid = OcctL::Topo::PackUID(aUid);
      anEntry.Bits              = anUid.bits;
      anEntry.HasKey            = anUid.bits != 0;
      break;
    }
    case OCCTL_SELECT_SORT_NONE:
    case OCCTL_SELECT_SORT_KEY_RESERVED_FUTURE:
      break;
  }
  return anEntry;
}

void sortSelection(BRepGraph&                                 theGraph,
                   const SelectConfig&                        theConfig,
                   NCollection_LinearVector<occtl_node_id_t>& theNodes)
{
  const occtl_select_options_t& theOptions = theConfig.Options;
  if (theOptions.sort_key == OCCTL_SELECT_SORT_NONE || theNodes.Size() < 2)
  {
    return;
  }

  NCollection_LinearVector<SortEntry> anEntries;
  for (size_t aNodeIndex = 0; aNodeIndex < theNodes.Size(); ++aNodeIndex)
  {
    const occtl_node_id_t aNode = theNodes.Value(aNodeIndex);
    anEntries.Append(makeSortEntry(theGraph, aNode, theConfig));
  }

  const bool isDescending = theOptions.sort_direction == OCCTL_SELECT_SORT_DESCENDING;
  std::stable_sort(
    anEntries.begin(),
    anEntries.end(),
    [&](const SortEntry& theLeft, const SortEntry& theRight) {
      if (theLeft.HasKey != theRight.HasKey)
      {
        return theLeft.HasKey;
      }
      if (!theLeft.HasKey)
      {
        return false;
      }

      int aCompare = 0;
      switch (theOptions.sort_key)
      {
        case OCCTL_SELECT_SORT_AXIS_COORDINATE:
        case OCCTL_SELECT_SORT_MEASURE:
        case OCCTL_SELECT_SORT_DISTANCE_TO_POINT:
        case OCCTL_SELECT_SORT_DISTANCE_TO_NODE:
          aCompare = (theLeft.Number < theRight.Number)   ? -1
                     : (theRight.Number < theLeft.Number) ? 1
                                                          : 0;
          break;
        case OCCTL_SELECT_SORT_NAME:
          aCompare =
            std::strcmp(theLeft.Text.ToCString(), theRight.Text.ToCString()) < 0
              ? -1
              : (std::strcmp(theRight.Text.ToCString(), theLeft.Text.ToCString()) < 0 ? 1 : 0);
          break;
        case OCCTL_SELECT_SORT_UID:
          aCompare = (theLeft.Bits < theRight.Bits) ? -1 : (theRight.Bits < theLeft.Bits) ? 1 : 0;
          break;
        case OCCTL_SELECT_SORT_NONE:
        case OCCTL_SELECT_SORT_KEY_RESERVED_FUTURE:
          break;
      }
      if (aCompare == 0)
      {
        return false;
      }
      return isDescending ? aCompare > 0 : aCompare < 0;
    });

  for (size_t anIndex = 0; anIndex < anEntries.Size(); ++anIndex)
  {
    theNodes.ChangeValue(anIndex) = anEntries.Value(anIndex).Node;
  }
}

struct GroupKeyData
{
  occtl_select_group_key_t Key         = OCCTL_SELECT_GROUP_KIND;
  occtl_node_kind_t        NodeKind    = OCCTL_KIND_INVALID;
  occtl_curve_kind_t       CurveKind   = OCCTL_CURVE_KIND_UNDEFINED;
  occtl_surface_kind_t     SurfaceKind = OCCTL_SURFACE_KIND_UNDEFINED;
  double                   NumericKey  = 0.0;
  TCollection_AsciiString  Name;
  Quantity_ColorRGBA       Color;
  bool                     HasColor = false;
  bool                     HasKey   = false;
};

double bucketNumeric(const double theValue, const double theTolerance)
{
  if (theTolerance <= 0.0)
  {
    return theValue;
  }
  return std::round(theValue / theTolerance) * theTolerance;
}

float bucketColorChannel(const float theValue, const float theTolerance)
{
  if (theTolerance <= 0.0f)
  {
    return theValue;
  }
  return std::round(theValue / theTolerance) * theTolerance;
}

bool groupKeyData(BRepGraph&                          theGraph,
                  const occtl_node_id_t               theNode,
                  const occtl_select_group_options_t& theOptions,
                  GroupKeyData&                       theOut)
{
  const BRepGraph_NodeId aNode = OcctL::Topo::UnpackNodeId(theNode);
  if (!aNode.IsValid())
  {
    return false;
  }

  theOut.Key = theOptions.key;
  switch (theOptions.key)
  {
    case OCCTL_SELECT_GROUP_KIND:
      theOut.NodeKind = OcctL::Topo::ToAbiNodeKind(aNode.NodeKind);
      theOut.HasKey   = theOut.NodeKind != OCCTL_KIND_INVALID;
      return theOut.HasKey;
    case OCCTL_SELECT_GROUP_AXIS_COORDINATE: {
      gp_Pnt aCenter;
      if (!candidateCenter(theGraph, aNode, aCenter))
      {
        return false;
      }
      theOut.NumericKey =
        bucketNumeric(coordinateOf(aCenter, theOptions.axis), theOptions.tolerance);
      theOut.HasKey = IsFiniteValue(theOut.NumericKey);
      return theOut.HasKey;
    }
    case OCCTL_SELECT_GROUP_CURVE_KIND:
      if (aNode.NodeKind != BRepGraph_NodeId::Kind::Edge)
      {
        return false;
      }
      if (!OcctL::Topo::ComputeEdgeCurveKind(theGraph, BRepGraph_EdgeId(aNode), theOut.CurveKind))
      {
        return false;
      }
      theOut.HasKey = true;
      return true;
    case OCCTL_SELECT_GROUP_SURFACE_KIND:
      if (aNode.NodeKind != BRepGraph_NodeId::Kind::Face)
      {
        return false;
      }
      if (!OcctL::Topo::ComputeFaceSurfaceKind(theGraph,
                                               BRepGraph_FaceId(aNode),
                                               theOut.SurfaceKind))
      {
        return false;
      }
      theOut.HasKey = true;
      return true;
    case OCCTL_SELECT_GROUP_NAME:
      theOut.HasKey = OcctL::Topo::FindBuiltinName(theGraph, aNode, theOut.Name);
      return theOut.HasKey;
    case OCCTL_SELECT_GROUP_COLOR: {
      Quantity_ColorRGBA aColor;
      if (!OcctL::Topo::FindBuiltinColor(theGraph, aNode, aColor))
      {
        return false;
      }
      const float aTol = std::max(0.0f, theOptions.color_tolerance);
      theOut.Color =
        Quantity_ColorRGBA(bucketColorChannel(static_cast<float>(aColor.GetRGB().Red()), aTol),
                           bucketColorChannel(static_cast<float>(aColor.GetRGB().Green()), aTol),
                           bucketColorChannel(static_cast<float>(aColor.GetRGB().Blue()), aTol),
                           bucketColorChannel(static_cast<float>(aColor.Alpha()), aTol));
      theOut.HasColor = true;
      theOut.HasKey   = true;
      return true;
    }
    case OCCTL_SELECT_GROUP_KEY_RESERVED_FUTURE:
      break;
  }
  return false;
}

bool sameGroup(const occtl_select_group_view_t&    theView,
               const GroupKeyData&                 theKey,
               const occtl_select_group_options_t& theOptions)
{
  if (theView.key != theKey.Key)
  {
    return false;
  }

  switch (theKey.Key)
  {
    case OCCTL_SELECT_GROUP_KIND:
      return theView.node_kind == theKey.NodeKind;
    case OCCTL_SELECT_GROUP_AXIS_COORDINATE:
      return std::fabs(theView.numeric_key - theKey.NumericKey)
             <= std::max(0.0, theOptions.tolerance);
    case OCCTL_SELECT_GROUP_CURVE_KIND:
      return theView.curve_kind == theKey.CurveKind;
    case OCCTL_SELECT_GROUP_SURFACE_KIND:
      return theView.surface_kind == theKey.SurfaceKind;
    case OCCTL_SELECT_GROUP_NAME:
      return theView.name != nullptr
             && theView.name_len == static_cast<size_t>(theKey.Name.Length())
             && std::memcmp(theKey.Name.ToCString(), theView.name, theView.name_len) == 0;
    case OCCTL_SELECT_GROUP_COLOR:
      return theView.has_color != 0
             && std::fabs(theView.color.r - static_cast<float>(theKey.Color.GetRGB().Red()))
                  <= theOptions.color_tolerance
             && std::fabs(theView.color.g - static_cast<float>(theKey.Color.GetRGB().Green()))
                  <= theOptions.color_tolerance
             && std::fabs(theView.color.b - static_cast<float>(theKey.Color.GetRGB().Blue()))
                  <= theOptions.color_tolerance
             && std::fabs(theView.color.a - static_cast<float>(theKey.Color.Alpha()))
                  <= theOptions.color_tolerance;
    case OCCTL_SELECT_GROUP_KEY_RESERVED_FUTURE:
      break;
  }
  return false;
}

void initialiseGroupView(occtl_select_group_iter::Group& theGroup, const GroupKeyData& theKey)
{
  occtl_select_group_view_init(&theGroup.view);
  theGroup.view.key          = theKey.Key;
  theGroup.view.node_kind    = theKey.NodeKind;
  theGroup.view.curve_kind   = theKey.CurveKind;
  theGroup.view.surface_kind = theKey.SurfaceKind;
  theGroup.view.numeric_key  = theKey.NumericKey;
  theGroup.view.has_color    = theKey.HasColor ? 1 : 0;
  theGroup.view.color        = {static_cast<float>(theKey.Color.GetRGB().Red()),
                                static_cast<float>(theKey.Color.GetRGB().Green()),
                                static_cast<float>(theKey.Color.GetRGB().Blue()),
                                static_cast<float>(theKey.Color.Alpha())};
  theGroup.name              = theKey.Name;
  theGroup.view.name         = theGroup.name.IsEmpty() ? nullptr : theGroup.name.ToCString();
  theGroup.view.name_len     = static_cast<size_t>(theGroup.name.Length());
}

void groupSelection(BRepGraph&                                                theGraph,
                    const occtl_select_group_options_t&                       theOptions,
                    const NCollection_LinearVector<occtl_node_id_t>&          theNodes,
                    NCollection_LinearVector<occtl_select_group_iter::Group>& theGroups)
{
  for (size_t aNodeIndex = 0; aNodeIndex < theNodes.Size(); ++aNodeIndex)
  {
    const occtl_node_id_t aNode = theNodes.Value(aNodeIndex);
    GroupKeyData          aKey;
    if (!groupKeyData(theGraph, aNode, theOptions, aKey))
    {
      if (theOptions.include_missing == 0)
      {
        continue;
      }
      aKey.Key    = theOptions.key;
      aKey.HasKey = true;
    }

    bool isAdded = false;
    for (occtl_select_group_iter::Group& aGroup : theGroups)
    {
      if (sameGroup(aGroup.view, aKey, theOptions))
      {
        aGroup.nodes.Append(aNode);
        isAdded = true;
        break;
      }
    }

    if (!isAdded)
    {
      occtl_select_group_iter::Group aGroup;
      initialiseGroupView(aGroup, aKey);
      aGroup.nodes.Append(aNode);
      theGroups.Append(std::move(aGroup));
    }
  }

  for (occtl_select_group_iter::Group& aGroup : theGroups)
  {
    aGroup.view.nodes      = aGroup.nodes.IsEmpty() ? nullptr : aGroup.nodes.Data();
    aGroup.view.node_count = aGroup.nodes.Size();
    aGroup.view.name       = aGroup.name.IsEmpty() ? nullptr : aGroup.name.ToCString();
  }
}

bool curveKindMatches(BRepGraph&                    theGraph,
                      const BRepGraph_NodeId        theNode,
                      const occtl_select_options_t& theOptions)
{
  if (theOptions.use_curve_kind == 0)
  {
    return true;
  }
  if (theNode.NodeKind != BRepGraph_NodeId::Kind::Edge)
  {
    return false;
  }

  occtl_curve_kind_t aKind = OCCTL_CURVE_KIND_UNDEFINED;
  return OcctL::Topo::ComputeEdgeCurveKind(theGraph, BRepGraph_EdgeId(theNode), aKind)
         && aKind == theOptions.curve_kind;
}

bool surfaceKindMatches(BRepGraph&                    theGraph,
                        const BRepGraph_NodeId        theNode,
                        const occtl_select_options_t& theOptions)
{
  if (theOptions.use_surface_kind == 0)
  {
    return true;
  }
  if (theNode.NodeKind != BRepGraph_NodeId::Kind::Face)
  {
    return false;
  }

  occtl_surface_kind_t aKind = OCCTL_SURFACE_KIND_UNDEFINED;
  return OcctL::Topo::ComputeFaceSurfaceKind(theGraph, BRepGraph_FaceId(theNode), aKind)
         && aKind == theOptions.surface_kind;
}

bool faceNormal(BRepGraph& theGraph, const BRepGraph_FaceId theFace, gp_Vec& theOutNormal)
{
  double aUMin = 0.0;
  double aUMax = 0.0;
  double aVMin = 0.0;
  double aVMax = 0.0;
  BRepGraph_Tool::Face::Bounds(theGraph, theFace, aUMin, aUMax, aVMin, aVMax);

  GeomAdaptor_TransformedSurface anAdaptor =
    BRepGraph_Tool::Face::SurfaceAdaptor(theGraph, theFace);
  const Geom_Surface::ResD1 aRes = anAdaptor.EvalD1((aUMin + aUMax) * 0.5, (aVMin + aVMax) * 0.5);
  theOutNormal                   = aRes.D1U.Crossed(aRes.D1V);
  if (theOutNormal.SquareMagnitude() <= Precision::SquareConfusion())
  {
    return false;
  }
  theOutNormal.Normalize();

  const TopoDS_Shape       aFaceShape    = theGraph.Shapes().Shape(theFace);
  const TopAbs_Orientation anOrientation = TopoDS::Face(aFaceShape).Orientation();
  if (anOrientation == TopAbs_REVERSED)
  {
    theOutNormal.Reverse();
  }
  return true;
}

bool normalMatches(BRepGraph&                    theGraph,
                   const BRepGraph_NodeId        theNode,
                   const occtl_select_options_t& theOptions)
{
  if (theOptions.use_normal == 0)
  {
    return true;
  }
  if (theNode.NodeKind != BRepGraph_NodeId::Kind::Face)
  {
    return false;
  }

  gp_Vec aWanted;
  if (!directionToVector(theOptions.normal, aWanted))
  {
    return false;
  }

  gp_Vec aNormal;
  if (!faceNormal(theGraph, BRepGraph_FaceId(theNode), aNormal))
  {
    return false;
  }

  const double aDot = aNormal.Dot(aWanted);
  const double aCos = std::cos(theOptions.normal_angle_tolerance);
  switch (theOptions.normal_mode)
  {
    case OCCTL_SELECT_NORMAL_ANTIPARALLEL:
      return aDot <= -aCos;
    case OCCTL_SELECT_NORMAL_EITHER:
      return std::fabs(aDot) >= aCos;
    case OCCTL_SELECT_NORMAL_PARALLEL:
    default:
      return aDot >= aCos;
  }
}

bool colorMatches(const BRepGraph&              theGraph,
                  const BRepGraph_NodeId        theNode,
                  const occtl_select_options_t& theOptions)
{
  if (theOptions.use_color == 0)
  {
    return true;
  }

  Quantity_ColorRGBA aColor;
  if (!OcctL::Topo::FindBuiltinColor(theGraph, theNode, aColor))
  {
    return false;
  }

  const float aTol = std::max(0.0f, theOptions.color_tolerance);
  return std::fabs(static_cast<float>(aColor.GetRGB().Red()) - theOptions.color.r) <= aTol
         && std::fabs(static_cast<float>(aColor.GetRGB().Green()) - theOptions.color.g) <= aTol
         && std::fabs(static_cast<float>(aColor.GetRGB().Blue()) - theOptions.color.b) <= aTol
         && std::fabs(static_cast<float>(aColor.Alpha()) - theOptions.color.a) <= aTol;
}

bool nameMatches(const BRepGraph&              theGraph,
                 const BRepGraph_NodeId        theNode,
                 const occtl_select_options_t& theOptions)
{
  if (theOptions.name_len == 0)
  {
    return true;
  }

  TCollection_AsciiString aName;
  if (!OcctL::Topo::FindBuiltinName(theGraph, theNode, aName))
  {
    return false;
  }

  return static_cast<size_t>(aName.Length()) == theOptions.name_len
         && std::memcmp(aName.ToCString(), theOptions.name, theOptions.name_len) == 0;
}

bool metadataMatches(const BRepGraph&                            theGraph,
                     const BRepGraph_NodeId                      theNode,
                     const occtl_select_metadata_filter_t* const theFilter)
{
  if (theFilter == nullptr || theFilter->key_len == 0)
  {
    return true;
  }

  const TCollection_AsciiString aKey(theFilter->key, static_cast<int>(theFilter->key_len));
  TCollection_AsciiString       aValue;
  if (!OcctL::Topo::FindBuiltinMetadata(theGraph, theNode, aKey, aValue))
  {
    return false;
  }

  if (theFilter->match_value == 0)
  {
    return true;
  }
  return static_cast<size_t>(aValue.Length()) == theFilter->value_len
         && (aValue.IsEmpty()
             || std::memcmp(aValue.ToCString(), theFilter->value, theFilter->value_len) == 0);
}

bool tagMatches(const BRepGraph&                     theGraph,
                const BRepGraph_NodeId               theNode,
                const TCollection_AsciiString* const theTag)
{
  return theTag == nullptr || OcctL::Topo::HasBuiltinTag(theGraph, theNode, *theTag);
}

bool nodeMatches(BRepGraph&                                  theGraph,
                 const BRepGraph_NodeId                      theNode,
                 const occtl_select_options_t&               theOptions,
                 const occtl_select_metadata_filter_t* const theMetadataFilter,
                 const TCollection_AsciiString* const        theTagFilter)
{
  return theNode.IsValid() && isKindEnabled(theOptions.kind_mask, theNode)
         && nameMatches(theGraph, theNode, theOptions)
         && metadataMatches(theGraph, theNode, theMetadataFilter)
         && tagMatches(theGraph, theNode, theTagFilter)
         && colorMatches(theGraph, theNode, theOptions)
         && bboxMatches(theGraph, theNode, theOptions)
         && curveKindMatches(theGraph, theNode, theOptions)
         && surfaceKindMatches(theGraph, theNode, theOptions)
         && normalMatches(theGraph, theNode, theOptions)
         && measureMatches(theGraph, theNode, theOptions);
}

void appendNode(BRepGraph&                                  theGraph,
                const BRepGraph_NodeId                      theNode,
                const occtl_select_options_t&               theOptions,
                const occtl_select_metadata_filter_t* const theMetadataFilter,
                const TCollection_AsciiString* const        theTagFilter,
                NCollection_FlatMap<uint64_t>&              theSeen,
                NCollection_LinearVector<occtl_node_id_t>&  theOut)
{
  const occtl_node_id_t anAbiNode = OcctL::Topo::PackNodeId(theNode);
  if (anAbiNode.bits == 0 || !theSeen.Add(anAbiNode.bits))
  {
    return;
  }
  if (nodeMatches(theGraph, theNode, theOptions, theMetadataFilter, theTagFilter))
  {
    theOut.Append(anAbiNode);
  }
}

template <typename TIterator>
void collectKind(BRepGraph&                                  theGraph,
                 const occtl_select_options_t&               theOptions,
                 const occtl_select_metadata_filter_t* const theMetadataFilter,
                 const TCollection_AsciiString* const        theTagFilter,
                 NCollection_FlatMap<uint64_t>&              theSeen,
                 NCollection_LinearVector<occtl_node_id_t>&  theOut)
{
  TIterator anIt(theGraph);
  for (; anIt.More(); anIt.Next())
  {
    appendNode(theGraph,
               anIt.CurrentId(),
               theOptions,
               theMetadataFilter,
               theTagFilter,
               theSeen,
               theOut);
  }
}

void collectWholeGraph(BRepGraph&                                  theGraph,
                       const occtl_select_options_t&               theOptions,
                       const occtl_select_metadata_filter_t* const theMetadataFilter,
                       const TCollection_AsciiString* const        theTagFilter,
                       NCollection_LinearVector<occtl_node_id_t>&  theOut)
{
  NCollection_FlatMap<uint64_t> aSeen;
  collectKind<BRepGraph_SolidIterator>(theGraph,
                                       theOptions,
                                       theMetadataFilter,
                                       theTagFilter,
                                       aSeen,
                                       theOut);
  collectKind<BRepGraph_ShellIterator>(theGraph,
                                       theOptions,
                                       theMetadataFilter,
                                       theTagFilter,
                                       aSeen,
                                       theOut);
  collectKind<BRepGraph_FaceIterator>(theGraph,
                                      theOptions,
                                      theMetadataFilter,
                                      theTagFilter,
                                      aSeen,
                                      theOut);
  collectKind<BRepGraph_WireIterator>(theGraph,
                                      theOptions,
                                      theMetadataFilter,
                                      theTagFilter,
                                      aSeen,
                                      theOut);
  collectKind<BRepGraph_EdgeIterator>(theGraph,
                                      theOptions,
                                      theMetadataFilter,
                                      theTagFilter,
                                      aSeen,
                                      theOut);
  collectKind<BRepGraph_VertexIterator>(theGraph,
                                        theOptions,
                                        theMetadataFilter,
                                        theTagFilter,
                                        aSeen,
                                        theOut);
  collectKind<BRepGraph_CompoundIterator>(theGraph,
                                          theOptions,
                                          theMetadataFilter,
                                          theTagFilter,
                                          aSeen,
                                          theOut);
  collectKind<BRepGraph_CompSolidIterator>(theGraph,
                                           theOptions,
                                           theMetadataFilter,
                                           theTagFilter,
                                           aSeen,
                                           theOut);
  collectKind<BRepGraph_CoEdgeIterator>(theGraph,
                                        theOptions,
                                        theMetadataFilter,
                                        theTagFilter,
                                        aSeen,
                                        theOut);
  collectKind<BRepGraph_ProductIterator>(theGraph,
                                         theOptions,
                                         theMetadataFilter,
                                         theTagFilter,
                                         aSeen,
                                         theOut);
  collectKind<BRepGraph_OccurrenceIterator>(theGraph,
                                            theOptions,
                                            theMetadataFilter,
                                            theTagFilter,
                                            aSeen,
                                            theOut);
}

void collectRooted(BRepGraph&                                  theGraph,
                   const BRepGraph_NodeId                      theRoot,
                   const occtl_select_options_t&               theOptions,
                   const occtl_select_metadata_filter_t* const theMetadataFilter,
                   const TCollection_AsciiString* const        theTagFilter,
                   NCollection_LinearVector<occtl_node_id_t>&  theOut)
{
  NCollection_FlatMap<uint64_t> aSeen;
  if (theOptions.include_root != 0)
  {
    appendNode(theGraph, theRoot, theOptions, theMetadataFilter, theTagFilter, aSeen, theOut);
  }
  else
  {
    const occtl_node_id_t anAbiRoot = OcctL::Topo::PackNodeId(theRoot);
    if (anAbiRoot.bits != 0)
    {
      aSeen.Add(anAbiRoot.bits);
    }
  }

  BRepGraph_ChildExplorer::Config aConfig;
  aConfig.Mode = BRepGraph_ChildExplorer::TraversalMode::Recursive;
  BRepGraph_ChildExplorer anExplorer(theGraph, theRoot, aConfig);
  for (; anExplorer.More(); anExplorer.Next())
  {
    appendNode(theGraph,
               anExplorer.Current().DefId,
               theOptions,
               theMetadataFilter,
               theTagFilter,
               aSeen,
               theOut);
  }
}

occtl_status_t parseMetadataExtension(const void* const thePNext, SelectConfig& theConfig)
{
  if (thePNext == nullptr)
  {
    return OCCTL_OK;
  }

  const occtl_select_metadata_filter_t* const aMetadata =
    static_cast<const occtl_select_metadata_filter_t*>(thePNext);
  if (aMetadata->struct_version != OCCTL_SELECT_METADATA_FILTER_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                           "unsupported select p_next extension version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (aMetadata->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "select metadata filter p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }

  theConfig.MetadataFilter    = *aMetadata;
  theConfig.HasMetadataFilter = true;
  return OCCTL_OK;
}

occtl_status_t parseSelectExtensions(const void* const thePNext, SelectConfig& theConfig)
{
  if (theConfig.Options.sort_key == OCCTL_SELECT_SORT_DISTANCE_TO_NODE)
  {
    if (thePNext == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "distance-to-node sort requires occtl_select_distance_to_node_sort_t");
      return OCCTL_INVALID_ARGUMENT;
    }

    const occtl_select_distance_to_node_sort_t* const aSort =
      static_cast<const occtl_select_distance_to_node_sort_t*>(thePNext);
    if (aSort->struct_version != OCCTL_SELECT_DISTANCE_TO_NODE_SORT_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported select distance-to-node sort version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (aSort->target.bits == 0u)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "distance-to-node sort target is invalid");
      return OCCTL_INVALID_ARGUMENT;
    }

    theConfig.DistanceToNodeSort    = *aSort;
    theConfig.HasDistanceToNodeSort = true;
    return parseMetadataExtension(aSort->p_next, theConfig);
  }

  return parseMetadataExtension(thePNext, theConfig);
}

bool validateMetadataFilter(const occtl_select_metadata_filter_t& theFilter)
{
  if (theFilter.key_len > 0 && theFilter.key == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "metadata key is NULL with non-zero key_len");
    return false;
  }
  if (theFilter.match_value != 0 && theFilter.match_value != 1)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "metadata match_value must be 0 or 1");
    return false;
  }
  if (theFilter.match_value != 0 && theFilter.key_len == 0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "metadata match_value requires key");
    return false;
  }
  if (theFilter.match_value != 0 && theFilter.value_len > 0 && theFilter.value == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "metadata value is NULL with non-zero value_len");
    return false;
  }
  return true;
}

bool validateOptions(const SelectConfig& theConfig)
{
  const occtl_select_options_t& theOptions = theConfig.Options;
  if (theOptions.name_len > 0 && theOptions.name == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "name is NULL with non-zero name_len");
    return false;
  }
  if (theConfig.HasMetadataFilter && !validateMetadataFilter(theConfig.MetadataFilter))
  {
    return false;
  }
  if (theOptions.color_tolerance < 0.0f)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "color_tolerance is negative");
    return false;
  }
  if (theOptions.axis_tolerance < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "axis_tolerance is negative");
    return false;
  }
  if (theOptions.normal_angle_tolerance < 0.0
      || theOptions.normal_angle_tolerance > 3.14159265358979323846)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "normal_angle_tolerance is outside [0, pi]");
    return false;
  }
  if (theOptions.use_measure != 0)
  {
    if (!isKnownMeasureKind(theOptions.measure_kind))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "measure_kind is not recognised");
      return false;
    }
    if (!IsFiniteValue(theOptions.measure_min) || !IsFiniteValue(theOptions.measure_max)
        || theOptions.measure_min > theOptions.measure_max)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "measure range is invalid");
      return false;
    }
  }
  if (theOptions.use_normal != 0)
  {
    gp_Vec aNormal;
    if (!directionToVector(theOptions.normal, aNormal))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "normal is not a finite non-zero direction");
      return false;
    }
  }
  if (theOptions.use_bbox != 0)
  {
    Bnd_Box aBox;
    if (!bboxFromAbi(theOptions.bbox, aBox))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "bbox min corner is greater than max corner");
      return false;
    }
  }
  if (theOptions.use_normal != 0 && !isKnownNormalMode(theOptions.normal_mode))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "normal_mode is not recognised");
    return false;
  }
  if (theOptions.use_axis_position != 0 && !isKnownAxis(theOptions.axis))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "axis is not recognised");
    return false;
  }
  if (theOptions.use_axis_position != 0 && !isKnownAxisPosition(theOptions.axis_position))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "axis_position is not recognised");
    return false;
  }
  if (theOptions.use_curve_kind != 0 && !OcctL::Topo::IsKnownCurveKind(theOptions.curve_kind))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "curve_kind is not recognised");
    return false;
  }
  if (theOptions.use_surface_kind != 0 && !OcctL::Topo::IsKnownSurfaceKind(theOptions.surface_kind))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "surface_kind is not recognised");
    return false;
  }
  if (!isKnownSortKey(theOptions.sort_key))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "sort_key is not recognised");
    return false;
  }
  if (!isKnownSortDirection(theOptions.sort_direction))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "sort_direction is not recognised");
    return false;
  }
  if (theOptions.sort_key == OCCTL_SELECT_SORT_AXIS_COORDINATE
      && !isKnownAxis(theOptions.sort_axis))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "sort_axis is not recognised");
    return false;
  }
  if (theOptions.sort_key == OCCTL_SELECT_SORT_MEASURE
      && !isKnownMeasureKind(theOptions.sort_measure_kind))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "sort_measure_kind is not recognised");
    return false;
  }
  if (theOptions.sort_key == OCCTL_SELECT_SORT_DISTANCE_TO_POINT
      && (!IsFiniteValue(theOptions.sort_point.x) || !IsFiniteValue(theOptions.sort_point.y)
          || !IsFiniteValue(theOptions.sort_point.z)))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "sort_point is not finite");
    return false;
  }
  if (theOptions.sort_key == OCCTL_SELECT_SORT_DISTANCE_TO_NODE && !theConfig.HasDistanceToNodeSort)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "distance-to-node sort extension is missing");
    return false;
  }
  return true;
}

bool validateGroupOptions(const occtl_select_group_options_t& theOptions)
{
  if (!isKnownGroupKey(theOptions.key))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "group key is not recognised");
    return false;
  }
  if (theOptions.key == OCCTL_SELECT_GROUP_AXIS_COORDINATE && !isKnownAxis(theOptions.axis))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "group axis is not recognised");
    return false;
  }
  if (!IsFiniteValue(theOptions.tolerance) || theOptions.tolerance < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "group tolerance is invalid");
    return false;
  }
  if (theOptions.color_tolerance < 0.0f)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "group color_tolerance is negative");
    return false;
  }
  return true;
}

occtl_status_t prepareSelection(BRepGraph&                                 theGraph,
                                const occtl_select_options_t* const        theOptions,
                                SelectConfig&                              theConfig,
                                NCollection_LinearVector<occtl_node_id_t>& theOutNodes)
{
  if (theOptions != nullptr)
  {
    if (theOptions->struct_version != OCCTL_SELECT_OPTIONS_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "unsupported select options version");
      return OCCTL_VERSION_MISMATCH;
    }
    theConfig.Options = *theOptions;
  }
  const occtl_status_t aPNextStatus = parseSelectExtensions(theConfig.Options.p_next, theConfig);
  if (aPNextStatus != OCCTL_OK)
  {
    return aPNextStatus;
  }
  if (!validateOptions(theConfig))
  {
    return OCCTL_INVALID_ARGUMENT;
  }

  const BRepGraph_NodeId aRoot = OcctL::Topo::UnpackNodeId(theConfig.Options.root);
  theConfig.HasRoot            = theConfig.Options.root.bits != 0;
  if (theConfig.HasRoot && !aRoot.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "root NodeId is invalid or removed");
    return OCCTL_NOT_FOUND;
  }

  if (theConfig.HasRoot)
  {
    collectRooted(theGraph,
                  aRoot,
                  theConfig.Options,
                  theConfig.HasMetadataFilter ? &theConfig.MetadataFilter : nullptr,
                  theConfig.HasTagFilter ? &theConfig.TagFilter : nullptr,
                  theOutNodes);
  }
  else
  {
    collectWholeGraph(theGraph,
                      theConfig.Options,
                      theConfig.HasMetadataFilter ? &theConfig.MetadataFilter : nullptr,
                      theConfig.HasTagFilter ? &theConfig.TagFilter : nullptr,
                      theOutNodes);
  }
  filterAxisPosition(theGraph, theConfig.Options, theOutNodes);
  if (theConfig.HasDistanceToNodeSort)
  {
    const BRepGraph_NodeId aTarget = OcctL::Topo::UnpackNodeId(theConfig.DistanceToNodeSort.target);
    if (!aTarget.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "distance-to-node sort target is invalid or removed");
      return OCCTL_NOT_FOUND;
    }
  }
  sortSelection(theGraph, theConfig, theOutNodes);
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_select_options_init(occtl_select_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    const occtl_select_options_t aDefaults = OCCTL_SELECT_OPTIONS_INIT;
    *theOptions                            = aDefaults;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_select_group_options_init(occtl_select_group_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    const occtl_select_group_options_t aDefaults = OCCTL_SELECT_GROUP_OPTIONS_INIT;
    *theOptions                                  = aDefaults;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_select_group_view_init(occtl_select_group_view_t* const theView)
{
  if (theView != nullptr)
  {
    const occtl_select_group_view_t aDefaults = OCCTL_SELECT_GROUP_VIEW_INIT;
    *theView                                  = aDefaults;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_select_iter_create(occtl_graph_t* const                theGraph,
                           const occtl_select_options_t* const theOptions,
                           occtl_select_iter_t** const         theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutIter = nullptr;

    SelectConfig         aConfig;
    occtl_select_iter*   anIter = new occtl_select_iter;
    const occtl_status_t aStatus =
      prepareSelection(theGraph->graph, theOptions, aConfig, anIter->nodes);
    if (aStatus != OCCTL_OK)
    {
      delete anIter;
      return aStatus;
    }

    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_select_tagged_iter_create(occtl_graph_t* const                theGraph,
                                  const occtl_select_options_t* const theOptions,
                                  const char* const                   theTag,
                                  const size_t                        theTagLen,
                                  occtl_select_iter_t** const         theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theTag == nullptr || theTagLen == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "tag is NULL or empty");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutIter = nullptr;

    SelectConfig aConfig;
    aConfig.HasTagFilter        = true;
    aConfig.TagFilter           = TCollection_AsciiString(theTag, static_cast<int>(theTagLen));
    occtl_select_iter*   anIter = new occtl_select_iter;
    const occtl_status_t aStatus =
      prepareSelection(theGraph->graph, theOptions, aConfig, anIter->nodes);
    if (aStatus != OCCTL_OK)
    {
      delete anIter;
      return aStatus;
    }

    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_select_iter_next(occtl_select_iter_t* const theIter,
                                                           occtl_node_id_t* const     theOutNode)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theIter == nullptr || theOutNode == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "iter or out_node is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theIter->index >= theIter->nodes.Size())
    {
      *theOutNode = OCCTL_NODE_ID_INVALID;
      return OCCTL_NOT_FOUND;
    }

    *theOutNode = theIter->nodes.Value(theIter->index);
    ++theIter->index;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_select_iter_free(occtl_select_iter_t* const theIter)
{
  delete theIter;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_select_group_iter_create(occtl_graph_t* const                      theGraph,
                                 const occtl_select_options_t* const       theSelectOptions,
                                 const occtl_select_group_options_t* const theGroupOptions,
                                 occtl_select_group_iter_t** const         theOutIter)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutIter == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_iter is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutIter = nullptr;

    occtl_select_group_options_t aGroupOptions = OCCTL_SELECT_GROUP_OPTIONS_INIT;
    if (theGroupOptions != nullptr)
    {
      if (theGroupOptions->struct_version != OCCTL_SELECT_GROUP_OPTIONS_VERSION_1)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                               "unsupported select group options version");
        return OCCTL_VERSION_MISMATCH;
      }
      aGroupOptions = *theGroupOptions;
    }
    if (!validateGroupOptions(aGroupOptions))
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    NCollection_LinearVector<occtl_node_id_t> aNodes;
    SelectConfig                              aConfig;
    const occtl_status_t                      aStatus =
      prepareSelection(theGraph->graph, theSelectOptions, aConfig, aNodes);
    if (aStatus != OCCTL_OK)
    {
      return aStatus;
    }

    occtl_select_group_iter* anIter = new occtl_select_group_iter;
    groupSelection(theGraph->graph, aGroupOptions, aNodes, anIter->groups);
    *theOutIter = anIter;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_select_group_iter_next(occtl_select_group_iter_t* const theIter,
                               occtl_select_group_view_t* const theOutView)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theIter == nullptr || theOutView == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "iter or out_view is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theOutView->struct_version != OCCTL_SELECT_GROUP_VIEW_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                             "out_view has unsupported struct_version");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theOutView->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_view p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (theIter->index >= theIter->groups.Size())
    {
      occtl_select_group_view_init(theOutView);
      return OCCTL_NOT_FOUND;
    }

    occtl_select_group_iter::Group& aGroup = theIter->groups.ChangeValue(theIter->index);
    aGroup.view.nodes                      = aGroup.nodes.IsEmpty() ? nullptr : aGroup.nodes.Data();
    aGroup.view.node_count                 = aGroup.nodes.Size();
    aGroup.view.name = aGroup.name.IsEmpty() ? nullptr : aGroup.name.ToCString();
    *theOutView      = aGroup.view;
    ++theIter->index;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_select_group_iter_free(occtl_select_group_iter_t* const theIter)
{
  delete theIter;
}

} // extern "C"
