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

//! @file BoolMath.hxx
//! @brief Internal helpers shared by the bool module's five entry points.
//!
//! Three routines:
//!   - ResolveShapeList    — translate an ABI NodeId array into a TopoDS
//!                           shape list and a (shape -> NodeId) lookup
//!                           map covering every subshape of every input.
//!   - AddResultToGraph    — wrap BRepGraph::ShapesView::AddWithHistory
//!                           so graph layers receive operation history.

#ifndef OCCTL_BOOL_BOOL_MATH_HXX
#define OCCTL_BOOL_BOOL_MATH_HXX

#include "../core/ErrorState.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>

#include <occtl/occtl_bool.h>
#include <BRepAlgoAPI_BuilderAlgo.hxx>
#include <BRepGraph.hxx>
#include <BRepGraph_NodeId.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_UID.hxx>
#include <BRepGraph_UIDsView.hxx>
#include <BRepTools_History.hxx>
#include <NCollection_DataMap.hxx>
#include <NCollection_List.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shape.hxx>
#include <TopTools_ShapeMapHasher.hxx>

#include <Standard_Handle.hxx>
#include <TCollection_AsciiString.hxx>
#include <Precision.hxx>

#include <cmath>
#include <cstdint>
#include <memory>

namespace OcctL::Bool
{

inline bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

inline bool IsZeroOrOne(const int32_t theValue) noexcept
{
  return theValue == 0 || theValue == 1;
}

//! Resolve an array of @p theIds (already living in @p theGraph) to:
//!   - a NCollection_List<TopoDS_Shape> suitable for BRepAlgoAPI's SetArguments /
//!     SetTools call,
//!   - an input-shape map mapping every input subshape (recursive) to its
//!     BRepGraph_NodeId, used by AddWithHistory to translate
//!     BRepTools_History's TopoDS_Shape keys into NodeIds.
//!
//! @param[in]  theGraph graph holding the input NodeIds
//! @param[in]  theIds   array of ABI NodeIds to resolve
//! @param[in]  theN     number of entries in @p theIds
//! @param[in]  theLabel short caller-supplied tag for the error message
//!                      ("objects" / "tools")
//! @param[out] theOutList    list of root TopoDS shapes (one per input)
//! @param[out] theOutInputs  shape -> NodeId map covering all subshapes
//! @return OCCTL_OK on success; OCCTL_INVALID_ARGUMENT for NULL @p theIds
//!         with non-zero @p theN; OCCTL_NOT_FOUND for an invalid NodeId
//!         or one that cannot be reconstructed as a TopoDS shape.
inline occtl_status_t ResolveShapeList(
  const occtl_graph_t* const      theGraph,
  const occtl_node_id_t* const    theIds,
  const size_t                    theN,
  const char* const               theLabel,
  NCollection_List<TopoDS_Shape>& theOutList,
  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>&
    theOutInputs) noexcept
{
  if (theN == 0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " list must contain at least one entry"));
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theIds == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel) + " pointer is NULL"));
    return OCCTL_INVALID_ARGUMENT;
  }

  for (size_t anIdx = 0; anIdx < theN; ++anIdx)
  {
    const BRepGraph_NodeId aNode = OcctL::Topo::UnpackNodeId(theIds[anIdx]);
    if (!aNode.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                      + " entry has invalid or removed NodeId"));
      return OCCTL_NOT_FOUND;
    }

    const TopoDS_Shape aRoot = theGraph->graph.Shapes().Shape(aNode);
    if (aRoot.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                      + " entry could not be reconstructed as TopoDS shape"));
      return OCCTL_NOT_FOUND;
    }

    theOutList.Append(aRoot);

    // Walk every subshape of the input root and record (shape -> NodeId)
    // when the subshape resolves back to a node.  This is the lookup used
    // by AddWithHistory to translate BRepTools_History's TopoDS_Shape
    // results into NodeIds and UIDs.
    auto bind = [&](const TopoDS_Shape& theSub) {
      if (theSub.IsNull() || theOutInputs.IsBound(theSub))
      {
        return;
      }
      const BRepGraph_NodeId aSubNode = theGraph->graph.Shapes().FindNode(theSub);
      if (aSubNode.IsValid())
      {
        theOutInputs.Bind(theSub, aSubNode);
      }
    };

    bind(aRoot);
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
      for (TopExp_Explorer anExp(aRoot, aKind); anExp.More(); anExp.Next())
      {
        bind(anExp.Current());
      }
    }
  }

  return OCCTL_OK;
}

//! Merge an OCCT-produced @p theShape back into @p theGraph as a new topology root.
//! When history is requested, absorb algorithm history through AddWithHistory.
inline occtl_status_t AddResultToGraph(
  occtl_graph_t* const                                                                theGraph,
  const TopoDS_Shape&                                                                 theShape,
  occtl_node_id_t&                                                                    theOutId,
  const NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>& theInputs,
  const occ::handle<BRepTools_History>&                                               theHistory,
  const TCollection_AsciiString&                                                      theOpLabel,
  const bool theBuildHistory) noexcept
{
  if (theShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "boolean result shape is null");
    return OCCTL_GEOMETRY_INVALID;
  }

  BRepGraph::ShapesView::Options anOpts;
  anOpts.CreateAutoProduct = false;
  anOpts.TrackAddedNodes   = false;

  const BRepGraph::ShapesView::Result aRes =
    theBuildHistory
      ? theGraph->graph.Shapes().AddWithHistory(theShape, theInputs, theHistory, theOpLabel)
      : theGraph->graph.Shapes().Add(theShape, anOpts);
  if (!aRes.Ok)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      "the boolean result topology could not be merged into the graph");
    return OCCTL_TOPOLOGY_INVALID;
  }

  theOutId = OcctL::Topo::PackNodeId(aRes.TopologyRoot);
  return OCCTL_OK;
}

//! Shared front-end of every bool entry point.  Validates pointers and
//! options versioning; resolves both input arrays; configures common
//! BRepAlgoAPI_BuilderAlgo settings (fuzzy, parallel, history).
//!
//! Templated on @p TAlgo so the same code drives the algos that don't
//! share a common base for SetArguments/SetTools (Fuse/Cut/Common/Section
//! derive from BRepAlgoAPI_BooleanOperation; Splitter derives directly
//! from BRepAlgoAPI_BuilderAlgo and adds its own SetTools).
template <typename TAlgo>
inline occtl_status_t Preflight(
  occtl_graph_t* const                                                          theGraph,
  const occtl_node_id_t* const                                                  theObjects,
  const size_t                                                                  theNObjects,
  const occtl_node_id_t* const                                                  theTools,
  const size_t                                                                  theNTools,
  const occtl_bool_options_t* const                                             theOpts,
  const occtl_node_id_t* const                                                  theOutRoot,
  NCollection_List<TopoDS_Shape>&                                               theObjList,
  NCollection_List<TopoDS_Shape>&                                               theToolList,
  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>& theInputsMap,
  TAlgo&                                                                        theAlgo) noexcept
{
  if (theGraph == nullptr || theOpts == nullptr || theOutRoot == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "graph, opts or out_root is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->struct_version != OCCTL_BOOL_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "opts->struct_version is not OCCTL_BOOL_OPTIONS_VERSION_1");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "opts->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsFiniteValue(theOpts->fuzzy_value) || theOpts->fuzzy_value < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "opts->fuzzy_value must be finite and non-negative");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOpts->run_parallel) || !IsZeroOrOne(theOpts->simplify_result)
      || !IsZeroOrOne(theOpts->build_history))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "opts flags run_parallel/simplify_result/build_history must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->simplify_result != 0
      && (!IsFiniteValue(theOpts->simplify_angular_tolerance)
          || theOpts->simplify_angular_tolerance <= 0.0))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "opts->simplify_angular_tolerance must be finite and "
                                           "positive when simplify_result is enabled");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (const occtl_status_t aStatus =
        ResolveShapeList(theGraph, theObjects, theNObjects, "objects", theObjList, theInputsMap))
  {
    return aStatus;
  }
  if (const occtl_status_t aStatus =
        ResolveShapeList(theGraph, theTools, theNTools, "tools", theToolList, theInputsMap))
  {
    return aStatus;
  }

  theAlgo.SetArguments(theObjList);
  theAlgo.SetTools(theToolList);
  if (theOpts->fuzzy_value > 0.0)
  {
    theAlgo.SetFuzzyValue(theOpts->fuzzy_value);
  }
  theAlgo.SetRunParallel(theOpts->run_parallel != 0);
  theAlgo.SetToFillHistory(theOpts->build_history != 0);
  return OCCTL_OK;
}

//! Shared back-end: run the OCCT algo, check IsDone, simplify if requested,
//! and merge the result with graph-owned history. @p theAlgo must already be configured by
//! Preflight.  The caller owns @p theInputsMap (used as the history input
//! lookup).
inline occtl_status_t RunAndCommit(
  occtl_graph_t* const                                                                theGraph,
  const occtl_bool_options_t* const                                                   theOpts,
  occtl_node_id_t* const                                                              theOutRoot,
  const char* const                                                                   theOpLabel,
  const NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>& theInputsMap,
  BRepAlgoAPI_BuilderAlgo& theAlgo) noexcept
{
  *theOutRoot = OCCTL_NODE_ID_INVALID;
  theAlgo.Build();
  if (!theAlgo.IsDone() || theAlgo.HasErrors())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_GEOMETRY_INVALID,
      static_cast<std::string_view>(TCollection_AsciiString("Boolean ") + theOpLabel
                                    + " failed to produce a valid result"));
    return OCCTL_GEOMETRY_INVALID;
  }

  if (theOpts->simplify_result != 0)
  {
    theAlgo.SimplifyResult(true, true, theOpts->simplify_angular_tolerance);
  }

  if (const occtl_status_t aStatus = AddResultToGraph(theGraph,
                                                      theAlgo.Shape(),
                                                      *theOutRoot,
                                                      theInputsMap,
                                                      theAlgo.History(),
                                                      TCollection_AsciiString(theOpLabel),
                                                      theOpts->build_history != 0))
  {
    return aStatus;
  }

  return OCCTL_OK;
}

} // namespace OcctL::Bool

#endif // OCCTL_BOOL_BOOL_MATH_HXX
