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

//! @file feat_prism.cxx
//! @brief Feature prism implementation for profile extrusion on an existing
//!        body, with termination controlled by target shape or height.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/IdConvert.hxx"

#include <occtl/occtl_prim.h>

#include <BRepFeat_MakeDPrism.hxx>
#include <BRepFeat_MakePrism.hxx>
#include <BRepGraph_UIDsView.hxx>
#include <BRepTools_History.hxx>
#include <NCollection_List.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <gp_Dir.hxx>

#include <TCollection_AsciiString.hxx>
#include <cmath>
#include <memory>

namespace
{

bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_feat_prism_info_init(occtl_prim_feat_prism_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_feat_draft_prism_info_init(occtl_prim_feat_draft_prism_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_extrude_until_info_init(occtl_prim_extrude_until_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_EXTRUDE_UNTIL_INFO_INIT;
  }
}

//==================================================================================================

static occtl_status_t combineToOcct(const occtl_prim_feat_combine_t theCombine, int& theOutFuse)
{
  switch (theCombine)
  {
    case OCCTL_FEAT_SEPARATE:
      theOutFuse = -1;
      return OCCTL_OK;
    case OCCTL_FEAT_CUT:
      theOutFuse = 0;
      return OCCTL_OK;
    case OCCTL_FEAT_FUSE:
      theOutFuse = 1;
      return OCCTL_OK;
    default:
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "unknown combine mode");
      return OCCTL_INVALID_ARGUMENT;
  }
}

//==================================================================================================

static occtl_status_t resolveFace(occtl_graph_t* const  theGraph,
                                  const occtl_node_id_t theNode,
                                  const char* const     theName,
                                  TopoDS_Face&          theOutFace)
{
  BRepGraph_NodeId aFaceId;
  if (const occtl_status_t aStatus =
        OcctL::Topo::ToTypedId(theGraph, theNode, BRepGraph_NodeId::Kind::Face, aFaceId))
  {
    return aStatus;
  }

  const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(aFaceId);
  if (aShape.IsNull() || aShape.ShapeType() != TopAbs_FACE)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theName)
                                    + " could not be reconstructed as TopoDS_Face"));
    return OCCTL_NOT_FOUND;
  }
  theOutFace = TopoDS::Face(aShape);
  return OCCTL_OK;
}

//==================================================================================================

static occtl_status_t finishFeatureResult(
  occtl_graph_t* const                                                                theGraph,
  const TopoDS_Shape&                                                                 theShape,
  const NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>& theInputs,
  const occ::handle<BRepTools_History>& theSourceHistory,
  const TCollection_AsciiString&        theOpLabel,
  occtl_node_id_t* const                theOutShape)
{
  const BRepGraph::ShapesView::Result aResult =
    theGraph->graph.Shapes().AddWithHistory(theShape, theInputs, theSourceHistory, theOpLabel);
  if (!aResult.Ok)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      "BRepGraph::ShapesView::AddWithHistory failed to ingest feature result");
    return OCCTL_TOPOLOGY_INVALID;
  }

  *theOutShape = OcctL::Topo::PackNodeId(aResult.TopologyRoot);
  return OCCTL_OK;
}

//==================================================================================================

static void bindInputShape(
  occtl_graph_t* const                                                          theGraph,
  const TopoDS_Shape&                                                           theRoot,
  NCollection_List<TopoDS_Shape>&                                               theArgs,
  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>& theInputs)
{
  if (theRoot.IsNull())
  {
    return;
  }

  theArgs.Append(theRoot);

  auto bind = [&](const TopoDS_Shape& theSubShape) {
    if (theSubShape.IsNull() || theInputs.IsBound(theSubShape))
    {
      return;
    }
    const BRepGraph_NodeId aNodeId = theGraph->graph.Shapes().FindNode(theSubShape);
    if (aNodeId.IsValid())
    {
      theInputs.Bind(theSubShape, aNodeId);
    }
  };

  bind(theRoot);
  static const TopAbs_ShapeEnum aKinds[] = {TopAbs_COMPOUND,
                                            TopAbs_COMPSOLID,
                                            TopAbs_SOLID,
                                            TopAbs_SHELL,
                                            TopAbs_FACE,
                                            TopAbs_WIRE,
                                            TopAbs_EDGE,
                                            TopAbs_VERTEX};
  for (const TopAbs_ShapeEnum aKind : aKinds)
  {
    for (TopExp_Explorer anExp(theRoot, aKind); anExp.More(); anExp.Next())
    {
      bind(anExp.Current());
    }
  }
}

//==================================================================================================

static occtl_status_t makeFeatPrismImpl(occtl_graph_t* const                      theGraph,
                                        const occtl_prim_feat_prism_info_t* const theInfo,
                                        occtl_node_id_t* const                    theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_FEAT_PRISM_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_FEAT_PRISM_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckFiniteVec3(theInfo->direction, "direction") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->modify, "modify") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->length, "length") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = OcctL::Prim::CheckDirection(theInfo->direction, "direction"))
    {
      return aStatus;
    }

    const occtl_direction3_t& aDir = theInfo->direction;
    const gp_Dir              anOcctDir(aDir.x, aDir.y, aDir.z);

    TopoDS_Shape aBaseShape;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->base_shape, "base_shape", aBaseShape))
    {
      return aStatus;
    }

    TopoDS_Shape aProfileShape;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->profile, "profile", aProfileShape))
    {
      return aStatus;
    }

    BRepGraph_NodeId aSketchFaceId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theInfo->sketch_face,
                                                              BRepGraph_NodeId::Kind::Face,
                                                              aSketchFaceId))
    {
      return aStatus;
    }

    const TopoDS_Shape aSketchShape = theGraph->graph.Shapes().Shape(aSketchFaceId);
    if (aSketchShape.IsNull() || aSketchShape.ShapeType() != TopAbs_FACE)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "sketch_face could not be reconstructed as TopoDS_Face");
      return OCCTL_NOT_FOUND;
    }
    const TopoDS_Face& aSketchFace = TopoDS::Face(aSketchShape);

    NCollection_List<TopoDS_Shape>                                               anInputShapes;
    NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anInputNodes;
    bindInputShape(theGraph, aBaseShape, anInputShapes, anInputNodes);
    bindInputShape(theGraph, aProfileShape, anInputShapes, anInputNodes);
    bindInputShape(theGraph, aSketchFace, anInputShapes, anInputNodes);

    int aFuse = 1;
    if (const occtl_status_t aStatus = combineToOcct(theInfo->combine, aFuse))
    {
      return aStatus;
    }

    BRepFeat_MakePrism aMaker(aBaseShape,
                              aProfileShape,
                              aSketchFace,
                              anOcctDir,
                              aFuse,
                              theInfo->modify != 0);

    switch (theInfo->until_kind)
    {
      case OCCTL_UNTIL_LENGTH: {
        if (theInfo->length <= 0.0)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                                 "LENGTH mode requires length > 0");
          return OCCTL_INVALID_ARGUMENT;
        }
        aMaker.Perform(theInfo->length);
        break;
      }
      case OCCTL_UNTIL_SHAPE: {
        TopoDS_Shape anUntil;
        if (const occtl_status_t aStatus = OcctL::Prim::ResolveProfileShape(theGraph,
                                                                            theInfo->until_shape,
                                                                            "until_shape",
                                                                            anUntil))
        {
          return aStatus;
        }
        bindInputShape(theGraph, anUntil, anInputShapes, anInputNodes);
        aMaker.Perform(anUntil);
        break;
      }
      case OCCTL_UNTIL_THRU_ALL: {
        aMaker.PerformThruAll();
        break;
      }
      case OCCTL_UNTIL_HEIGHT: {
        if (theInfo->length <= 0.0)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                                 "HEIGHT mode requires length > 0");
          return OCCTL_INVALID_ARGUMENT;
        }
        TopoDS_Shape anUntil;
        if (const occtl_status_t aStatus = OcctL::Prim::ResolveProfileShape(theGraph,
                                                                            theInfo->until_shape,
                                                                            "until_shape",
                                                                            anUntil))
        {
          return aStatus;
        }
        bindInputShape(theGraph, anUntil, anInputShapes, anInputNodes);
        aMaker.PerformUntilHeight(anUntil, theInfo->length);
        break;
      }
      default:
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "unknown until_kind");
        return OCCTL_INVALID_ARGUMENT;
    }

    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepFeat_MakePrism reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }

    occ::handle<BRepTools_History> aSourceHistory = new BRepTools_History(anInputShapes, aMaker);
    return finishFeatureResult(theGraph,
                               aMaker.Shape(),
                               anInputNodes,
                               aSourceHistory,
                               TCollection_AsciiString("feature prism"),
                               theOutShape);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_feat_prism(occtl_graph_t* const                      theGraph,
                             const occtl_prim_feat_prism_info_t* const theInfo,
                             occtl_node_id_t* const                    theOutShape)
{
  return OcctL::Core::Guard(
    [&]() -> occtl_status_t { return makeFeatPrismImpl(theGraph, theInfo, theOutShape); });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_extrude_until(occtl_graph_t* const                         theGraph,
                                const occtl_prim_extrude_until_info_t* const theInfo,
                                occtl_node_id_t* const                       theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;
    if (theInfo->struct_version != OCCTL_PRIM_EXTRUDE_UNTIL_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_EXTRUDE_UNTIL_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (OcctL::Prim::CheckBool(theInfo->modify, "modify") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theInfo->limit) || theInfo->limit < 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "limit must be finite and non-negative");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (OcctL::Prim::CheckFiniteVec3(theInfo->direction, "direction") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = OcctL::Prim::CheckDirection(theInfo->direction, "direction"))
    {
      return aStatus;
    }

    occtl_direction3_t aDirection = theInfo->direction;
    switch (theInfo->side)
    {
      case OCCTL_EXTRUDE_UNTIL_NEXT:
      case OCCTL_EXTRUDE_UNTIL_LAST:
        break;
      case OCCTL_EXTRUDE_UNTIL_PREVIOUS:
      case OCCTL_EXTRUDE_UNTIL_FIRST:
        aDirection.x = -aDirection.x;
        aDirection.y = -aDirection.y;
        aDirection.z = -aDirection.z;
        break;
      default:
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                               "unknown extrude-until side");
        return OCCTL_INVALID_ARGUMENT;
    }

    occtl_prim_feat_prism_info_t aFeatInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
    aFeatInfo.base_shape                   = theInfo->base_shape;
    aFeatInfo.profile                      = theInfo->profile;
    aFeatInfo.sketch_face                  = theInfo->sketch_face;
    aFeatInfo.direction                    = aDirection;
    aFeatInfo.combine                      = theInfo->combine;
    aFeatInfo.modify                       = theInfo->modify;
    aFeatInfo.until_shape                  = theInfo->target_shape;
    if (theInfo->limit > 0.0)
    {
      aFeatInfo.until_kind = OCCTL_UNTIL_HEIGHT;
      aFeatInfo.length     = theInfo->limit;
    }
    else
    {
      aFeatInfo.until_kind = OCCTL_UNTIL_SHAPE;
    }

    return makeFeatPrismImpl(theGraph, &aFeatInfo, theOutShape);
  });
}

//==================================================================================================

static occtl_status_t makeFeatDraftPrismImpl(
  occtl_graph_t* const                            theGraph,
  const occtl_prim_feat_draft_prism_info_t* const theInfo,
  occtl_node_id_t* const                          theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theInfo->taper_angle))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "taper_angle must be finite");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (OcctL::Prim::CheckBool(theInfo->modify, "modify") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->length, "length") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    TopoDS_Shape aBaseShape;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->base_shape, "base_shape", aBaseShape))
    {
      return aStatus;
    }

    TopoDS_Face aProfileFace;
    if (const occtl_status_t aStatus =
          resolveFace(theGraph, theInfo->profile_face, "profile_face", aProfileFace))
    {
      return aStatus;
    }

    TopoDS_Face aSketchFace;
    if (const occtl_status_t aStatus =
          resolveFace(theGraph, theInfo->sketch_face, "sketch_face", aSketchFace))
    {
      return aStatus;
    }

    NCollection_List<TopoDS_Shape>                                               anInputShapes;
    NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher> anInputNodes;
    bindInputShape(theGraph, aBaseShape, anInputShapes, anInputNodes);
    bindInputShape(theGraph, aProfileFace, anInputShapes, anInputNodes);
    bindInputShape(theGraph, aSketchFace, anInputShapes, anInputNodes);

    int aFuse = 1;
    if (const occtl_status_t aStatus = combineToOcct(theInfo->combine, aFuse))
    {
      return aStatus;
    }

    BRepFeat_MakeDPrism aMaker(aBaseShape,
                               aProfileFace,
                               aSketchFace,
                               theInfo->taper_angle,
                               aFuse,
                               theInfo->modify != 0);

    switch (theInfo->until_kind)
    {
      case OCCTL_UNTIL_LENGTH: {
        if (theInfo->length <= 0.0)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                                 "LENGTH mode requires length > 0");
          return OCCTL_INVALID_ARGUMENT;
        }
        aMaker.Perform(theInfo->length);
        break;
      }
      case OCCTL_UNTIL_SHAPE: {
        TopoDS_Shape anUntil;
        if (const occtl_status_t aStatus = OcctL::Prim::ResolveProfileShape(theGraph,
                                                                            theInfo->until_shape,
                                                                            "until_shape",
                                                                            anUntil))
        {
          return aStatus;
        }
        bindInputShape(theGraph, anUntil, anInputShapes, anInputNodes);
        aMaker.Perform(anUntil);
        break;
      }
      case OCCTL_UNTIL_THRU_ALL: {
        aMaker.PerformThruAll();
        break;
      }
      case OCCTL_UNTIL_HEIGHT: {
        if (theInfo->length <= 0.0)
        {
          OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                                 "HEIGHT mode requires length > 0");
          return OCCTL_INVALID_ARGUMENT;
        }
        TopoDS_Shape anUntil;
        if (const occtl_status_t aStatus = OcctL::Prim::ResolveProfileShape(theGraph,
                                                                            theInfo->until_shape,
                                                                            "until_shape",
                                                                            anUntil))
        {
          return aStatus;
        }
        bindInputShape(theGraph, anUntil, anInputShapes, anInputNodes);
        aMaker.PerformUntilHeight(anUntil, theInfo->length);
        break;
      }
      default:
        OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "unknown until_kind");
        return OCCTL_INVALID_ARGUMENT;
    }

    if (!aMaker.IsDone())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepFeat_MakeDPrism reported IsDone()==false");
      return OCCTL_GEOMETRY_INVALID;
    }

    occ::handle<BRepTools_History> aSourceHistory = new BRepTools_History(anInputShapes, aMaker);
    return finishFeatureResult(theGraph,
                               aMaker.Shape(),
                               anInputNodes,
                               aSourceHistory,
                               TCollection_AsciiString("feature draft prism"),
                               theOutShape);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_feat_draft_prism(occtl_graph_t* const                            theGraph,
                                   const occtl_prim_feat_draft_prism_info_t* const theInfo,
                                   occtl_node_id_t* const                          theOutShape)
{
  return OcctL::Core::Guard(
    [&]() -> occtl_status_t { return makeFeatDraftPrismImpl(theGraph, theInfo, theOutShape); });
}

//==================================================================================================

} // extern "C"
