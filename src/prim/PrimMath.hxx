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

//! @file PrimMath.hxx
//! @brief Internal helpers shared by the primitives module.
//!
//! Two routines that live here so every prim translation unit shares one ODR
//! version: AddTopologyRoot — wraps BRepGraph::ShapesView::Add with
//! CreateAutoProduct == false (we want bare topology, not auto-Products);
//! ResolveProfileShape — performs the Graph -> TopoDS round-trip via
//! BRepGraph::Shapes() and validates the resulting shape is non-null.

#ifndef OCCTL_PRIM_PRIM_MATH_HXX
#define OCCTL_PRIM_PRIM_MATH_HXX

#include "../core/ErrorState.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>

#include <BRepGraph.hxx>
#include <BRepGraph_NodeId.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <TopoDS_Shape.hxx>
#include <Precision.hxx>

#include <gp_Vec.hxx>
#include <gp.hxx>

#include <cmath>

#include <TCollection_AsciiString.hxx>

namespace OcctL::Prim
{

inline bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

//! Adds @p theShape to @p theGraph as a new topology root (no auto Product
//! wrapper).  On success packs the topology root NodeId into @p theOutId;
//! on failure populates the thread-local error state and returns a non-OK
//! status.
//! @param[in,out] theGraph graph receiving the topology
//! @param[in]     theShape TopoDS shape from BRepPrimAPI / BRepOffsetAPI
//! @param[out]    theOutId receives the new topology root NodeId on success
inline occtl_status_t AddTopologyRoot(occtl_graph_t* const theGraph,
                                      const TopoDS_Shape&  theShape,
                                      occtl_node_id_t&     theOutId) noexcept
{
  BRepGraph::ShapesView::Options anOpts;
  anOpts.CreateAutoProduct = false;

  const BRepGraph::ShapesView::Result aRes = theGraph->graph.Shapes().Add(theShape, anOpts);
  if (!aRes.Ok)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      "BRepGraph::ShapesView::Add failed to ingest primitive shape");
    return OCCTL_TOPOLOGY_INVALID;
  }

  theOutId = OcctL::Topo::PackNodeId(aRes.TopologyRoot);
  return OCCTL_OK;
}

//! Same as AddTopologyRoot(), additionally returning the BRepGraph::ShapesView's
//! TopoDS-shape to NodeId map for history harvesting.
inline occtl_status_t AddTopologyRootTracked(
  occtl_graph_t* const theGraph,
  const TopoDS_Shape&  theShape,
  occtl_node_id_t&     theOutId,
  NCollection_DataMap<TopoDS_Shape, BRepGraph_NodeId, TopTools_ShapeMapHasher>&
    theOutAdded) noexcept
{
  BRepGraph::ShapesView::Options anOpts;
  anOpts.CreateAutoProduct = false;
  anOpts.TrackAddedNodes   = true;

  const BRepGraph::ShapesView::Result aRes = theGraph->graph.Shapes().Add(theShape, anOpts);
  if (!aRes.Ok)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      "BRepGraph::ShapesView::Add failed to ingest primitive shape");
    return OCCTL_TOPOLOGY_INVALID;
  }

  theOutId    = OcctL::Topo::PackNodeId(aRes.TopologyRoot);
  theOutAdded = aRes.AddedNodes;
  return OCCTL_OK;
}

//! Resolves an ABI NodeId to a TopoDS_Shape via the graph's cached shapes
//! view.  Validates that the resulting shape is not null.  On failure
//! populates the thread-local error state and returns a non-OK status.
//! @param[in]  theGraph graph holding the topology
//! @param[in]  theAbiId ABI NodeId to resolve
//! @param[in]  theLabel short caller-supplied tag for the error message ("profile", "spine", ...)
//! @param[out] theShape receives the reconstructed TopoDS_Shape on success
inline occtl_status_t ResolveProfileShape(const occtl_graph_t* const theGraph,
                                          const occtl_node_id_t      theAbiId,
                                          const char* const          theLabel,
                                          TopoDS_Shape&              theShape) noexcept
{
  const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theAbiId);
  if (!aNodeId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " NodeId is invalid or removed"));
    return OCCTL_NOT_FOUND;
  }

  theShape = theGraph->graph.Shapes().Shape(aNodeId);
  if (theShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " could not be reconstructed as TopoDS shape"));
    return OCCTL_NOT_FOUND;
  }
  return OCCTL_OK;
}

//! Validates that a NodeId's kind is one of the kinds accepted as a
//! sweep / loft profile (Vertex, Edge, Wire, Face, Shell, Compound).
//! Rejects Solid, CompSolid, CoEdge, Product, Occurrence with
//! OCCTL_WRONG_KIND.  Caller is responsible for the higher-level shape
//! reconstruction.
//! @param[in] theAbiId ABI NodeId to inspect
//! @param[in] theLabel short caller-supplied tag for the error message
inline occtl_status_t CheckProfileKind(const occtl_node_id_t theAbiId,
                                       const char* const     theLabel) noexcept
{
  const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theAbiId);
  if (!aNodeId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " NodeId is invalid or removed"));
    return OCCTL_NOT_FOUND;
  }
  switch (aNodeId.NodeKind)
  {
    case BRepGraph_NodeId::Kind::Vertex:
    case BRepGraph_NodeId::Kind::Edge:
    case BRepGraph_NodeId::Kind::Wire:
    case BRepGraph_NodeId::Kind::Face:
    case BRepGraph_NodeId::Kind::Shell:
    case BRepGraph_NodeId::Kind::Compound:
      return OCCTL_OK;
    default:
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_WRONG_KIND,
        static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                      + " must be a Vertex/Edge/Wire/Face/Shell/Compound"));
      return OCCTL_WRONG_KIND;
  }
}

//! Rejects extension chains on public ABI option/info structs.
inline occtl_status_t CheckPNext(const void* const thePNext,
                                 const char* const theStructName) noexcept
{
  if (thePNext != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theStructName)
                                    + " p_next must be NULL"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

//! Validates a scalar is finite before passing it into OCCT constructors.
inline occtl_status_t CheckFinite(const double theValue, const char* const theLabel) noexcept
{
  if (!IsFiniteValue(theValue))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel) + " must be finite"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

//! Validates a public int32_t boolean field is exactly 0 or 1.
inline occtl_status_t CheckBool(const int32_t theValue, const char* const theLabel) noexcept
{
  if (theValue != 0 && theValue != 1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel) + " must be 0 or 1"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

//! Validates a scalar is finite and strictly positive.
inline occtl_status_t CheckPositive(const double theValue, const char* const theLabel) noexcept
{
  if (!IsFiniteValue(theValue) || theValue <= 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " must be finite and positive"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

//! Validates a scalar is finite and non-negative.
inline occtl_status_t CheckNonNegative(const double theValue, const char* const theLabel) noexcept
{
  if (!IsFiniteValue(theValue) || theValue < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                    + " must be finite and non-negative"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

//! Validates the finite coordinates in an ABI placement. Direction degeneracy
//! is still left to the existing OCCT conversion path unless a caller needs a
//! stricter public-domain check.
inline occtl_status_t CheckFinitePlacement(const occtl_axis2_placement_t& thePlacement,
                                           const char* const              theLabel) noexcept
{
  const double aValues[] = {thePlacement.location.x,
                            thePlacement.location.y,
                            thePlacement.location.z,
                            thePlacement.x_dir.x,
                            thePlacement.x_dir.y,
                            thePlacement.x_dir.z,
                            thePlacement.x_dir_ref.x,
                            thePlacement.x_dir_ref.y,
                            thePlacement.x_dir_ref.z};
  for (const double aValue : aValues)
  {
    if (!IsFiniteValue(aValue))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        static_cast<std::string_view>(TCollection_AsciiString(theLabel)
                                      + " placement values must be finite"));
      return OCCTL_INVALID_ARGUMENT;
    }
  }
  return OCCTL_OK;
}

//! Validates all coordinates in a public 3D vector/direction are finite.
template <typename Vec3>
inline occtl_status_t CheckFiniteVec3(const Vec3& theVec, const char* const theLabel) noexcept
{
  if (!IsFiniteValue(theVec.x) || !IsFiniteValue(theVec.y) || !IsFiniteValue(theVec.z))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel) + " values must be finite"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

//! Validates the finite coordinates in an ABI axis placement.
inline occtl_status_t CheckFiniteAxis1(const occtl_axis1_placement_t& theAxis,
                                       const char* const              theLabel) noexcept
{
  if (CheckFiniteVec3(theAxis.location, theLabel) != OCCTL_OK
      || CheckFiniteVec3(theAxis.direction, theLabel) != OCCTL_OK)
  {
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

//! Validates that an ABI direction or vector is non-degenerate using OCCT's
//! own threshold (Precision::SquareConfusion()). Anything looser leaks sub-tolerance
//! inputs into gp_Dir(gp_Vec), which throws Standard_ConstructionError —
//! the Guard barrier would then translate it to OCCTL_INTERNAL instead of
//! the documented OCCTL_INVALID_ARGUMENT.
//! Templated so a single helper validates both occtl_direction3_t (e.g.
//! revolution axis) and occtl_vector3_t (e.g. prism extrusion vector).
//! @param[in] theVec   ABI direction or vector to test (must expose x/y/z)
//! @param[in] theLabel short caller-supplied tag for the error message
template <typename Vec3>
inline occtl_status_t CheckDirection(const Vec3& theVec, const char* const theLabel) noexcept
{
  const gp_Vec aVec(theVec.x, theVec.y, theVec.z);
  if (aVec.SquareMagnitude() <= Precision::SquareConfusion())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theLabel) + " has zero length"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

} // namespace OcctL::Prim

#endif // OCCTL_PRIM_PRIM_MATH_HXX
