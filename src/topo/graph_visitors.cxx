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

#include <occtl/occtl_topo.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "GraphHandle.hxx"
#include "TopoMath.hxx"

#include <BRepGraph_MeshView.hxx>
#include <BRepGraph_RefsIterator.hxx>
#include <BRepGraph_TopoView.hxx>

namespace
{

// Drives any §10.3 iterator factory through a §10.4 visitor.  The factory
// closure must hand back a newly-allocated occtl_node_iter_t* on OCCTL_OK
// and leave theOutIter untouched on failure (matching the existing factory
// contract).  Owns the iterator end-to-end so the caller never sees it.
template <typename Factory>
occtl_status_t DriveVisitor(Factory                    theFactory,
                            const occtl_node_visitor_t theFn,
                            void* const                theUserData)
{
  if (theFn == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "visitor fn is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  occtl_node_iter_t* anIter = nullptr;
  if (const occtl_status_t aErr = theFactory(&anIter); aErr != OCCTL_OK)
  {
    return aErr;
  }
  occtl_status_t aSt = OCCTL_OK;
  for (;;)
  {
    occtl_node_id_t anId;
    aSt = ::occtl_node_iter_next(anIter, &anId);
    if (aSt == OCCTL_NOT_FOUND)
    {
      aSt = OCCTL_OK;
      break;
    }
    if (aSt != OCCTL_OK)
    {
      break;
    }
    aSt = theFn(anId, theUserData);
    if (aSt == OCCTL_CANCELLED)
    {
      aSt = OCCTL_OK;
      break;
    }
    if (aSt != OCCTL_OK)
    {
      break;
    }
  }
  ::occtl_node_iter_free(anIter);
  return aSt;
}

} // namespace

extern "C"
{

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_for_each(const occtl_graph_t* const theGraph,
                                                         const uint64_t             theKindMask,
                                                         const occtl_node_visitor_t theVisitor,
                                                         void* const                theUserData)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theVisitor == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "visitor fn is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    using Factory = occtl_status_t (*)(const occtl_graph_t*, occtl_node_iter_t**);
    static const Factory theFactories[12] = {
      nullptr,                              //  0 = INVALID
      ::occtl_graph_solid_iter_create,      //  1 = SOLID
      ::occtl_graph_shell_iter_create,      //  2 = SHELL
      ::occtl_graph_face_iter_create,       //  3 = FACE
      ::occtl_graph_wire_iter_create,       //  4 = WIRE
      ::occtl_graph_edge_iter_create,       //  5 = EDGE
      ::occtl_graph_vertex_iter_create,     //  6 = VERTEX
      ::occtl_graph_compound_iter_create,   //  7 = COMPOUND
      ::occtl_graph_compsolid_iter_create,  //  8 = COMPSOLID
      ::occtl_graph_coedge_iter_create,     //  9 = COEDGE
      ::occtl_graph_product_iter_create,    // 10 = PRODUCT
      ::occtl_graph_occurrence_iter_create, // 11 = OCCURRENCE
    };
    for (uint32_t aKind = 1; aKind < 12; ++aKind)
    {
      if ((theKindMask & (1uLL << aKind)) == 0)
      {
        continue;
      }
      const occtl_status_t aSt = DriveVisitor(
        [&](occtl_node_iter_t** anOut) { return theFactories[aKind](theGraph, anOut); },
        theVisitor,
        theUserData);
      if (aSt != OCCTL_OK)
      {
        return aSt;
      }
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_for_each_ref(const occtl_graph_t* const theGraph,
                                                             const uint64_t theRefKindMask,
                                                             const occtl_ref_visitor_t theVisitor,
                                                             void* const               theUserData)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theVisitor == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "visitor fn is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph& aGraph = theGraph->graph;

#define OCCTL_FOR_EACH_REF_KIND(abiBit, occtType)                                                  \
  if (theRefKindMask & (1uLL << (abiBit)))                                                         \
  {                                                                                                \
    for (BRepGraph_RefsIterator::RefIterator<occtType> anIt(aGraph); anIt.More(); anIt.Next())     \
    {                                                                                              \
      const occtl_ref_id_t anId = OcctL::Topo::PackRefId(anIt.CurrentId());                        \
      const occtl_status_t aSt  = theVisitor(anId, theUserData);                                   \
      if (aSt == OCCTL_CANCELLED)                                                                  \
        return OCCTL_OK;                                                                           \
      if (aSt != OCCTL_OK)                                                                         \
        return aSt;                                                                                \
    }                                                                                              \
  }

    OCCTL_FOR_EACH_REF_KIND(OCCTL_REF_KIND_SHELL, BRepGraphInc::ShellRef)
    OCCTL_FOR_EACH_REF_KIND(OCCTL_REF_KIND_FACE, BRepGraphInc::FaceRef)
    OCCTL_FOR_EACH_REF_KIND(OCCTL_REF_KIND_WIRE, BRepGraphInc::WireRef)
    OCCTL_FOR_EACH_REF_KIND(OCCTL_REF_KIND_COEDGE, BRepGraphInc::CoEdgeRef)
    OCCTL_FOR_EACH_REF_KIND(OCCTL_REF_KIND_VERTEX, BRepGraphInc::VertexRef)
    OCCTL_FOR_EACH_REF_KIND(OCCTL_REF_KIND_SOLID, BRepGraphInc::SolidRef)
    OCCTL_FOR_EACH_REF_KIND(OCCTL_REF_KIND_CHILD, BRepGraphInc::ChildRef)
    OCCTL_FOR_EACH_REF_KIND(OCCTL_REF_KIND_OCCURRENCE, BRepGraphInc::OccurrenceRef)

#undef OCCTL_FOR_EACH_REF_KIND

    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_graph_for_each_rep(const occtl_graph_t* const theGraph,
                                                             const uint64_t theRepKindMask,
                                                             const occtl_rep_visitor_t theVisitor,
                                                             void* const               theUserData)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theVisitor == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "visitor fn is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const BRepGraph& aGraph = theGraph->graph;

#define OCCTL_FOR_EACH_REP_KIND(abiBit, repIdType, nbMethod, getMethod)                            \
  if (theRepKindMask & (1uLL << (abiBit)))                                                         \
  {                                                                                                \
    const int aCount = nbMethod;                                                                   \
    for (int anIdx = 0; anIdx < aCount; ++anIdx)                                                   \
    {                                                                                              \
      const repIdType anId(static_cast<uint32_t>(anIdx));                                          \
      if (getMethod(anId).IsRemoved)                                                               \
        continue;                                                                                  \
      const occtl_rep_id_t anAbiId = OcctL::Topo::PackRepId(anId);                                 \
      const occtl_status_t aSt     = theVisitor(anAbiId, theUserData);                             \
      if (aSt == OCCTL_CANCELLED)                                                                  \
        return OCCTL_OK;                                                                           \
      if (aSt != OCCTL_OK)                                                                         \
        return aSt;                                                                                \
    }                                                                                              \
  }

    // Geometry reps (via TopoView::Geometry)
    OCCTL_FOR_EACH_REP_KIND(OCCTL_REP_KIND_SURFACE,
                            BRepGraph_SurfaceRepId,
                            aGraph.Topo().Geometry().NbSurfaces(),
                            aGraph.Topo().Geometry().SurfaceRep)
    OCCTL_FOR_EACH_REP_KIND(OCCTL_REP_KIND_CURVE3D,
                            BRepGraph_Curve3DRepId,
                            aGraph.Topo().Geometry().NbCurves3D(),
                            aGraph.Topo().Geometry().Curve3DRep)
    OCCTL_FOR_EACH_REP_KIND(OCCTL_REP_KIND_CURVE2D,
                            BRepGraph_Curve2DRepId,
                            aGraph.Topo().Geometry().NbCurves2D(),
                            aGraph.Topo().Geometry().Curve2DRep)
    // Mesh reps (via MeshView::Poly)
    OCCTL_FOR_EACH_REP_KIND(OCCTL_REP_KIND_TRIANGULATION,
                            BRepGraph_TriangulationRepId,
                            aGraph.Mesh().Poly().NbTriangulations(),
                            aGraph.Mesh().Poly().TriangulationRep)
    OCCTL_FOR_EACH_REP_KIND(OCCTL_REP_KIND_POLYGON3D,
                            BRepGraph_Polygon3DRepId,
                            aGraph.Mesh().Poly().NbPolygons3D(),
                            aGraph.Mesh().Poly().Polygon3DRep)
    OCCTL_FOR_EACH_REP_KIND(OCCTL_REP_KIND_POLYGON2D,
                            BRepGraph_Polygon2DRepId,
                            aGraph.Mesh().Poly().NbPolygons2D(),
                            aGraph.Mesh().Poly().Polygon2DRep)
    OCCTL_FOR_EACH_REP_KIND(OCCTL_REP_KIND_POLYGON_ON_TRI,
                            BRepGraph_PolygonOnTriRepId,
                            aGraph.Mesh().Poly().NbPolygonsOnTri(),
                            aGraph.Mesh().Poly().PolygonOnTriRep)

#undef OCCTL_FOR_EACH_REP_KIND

    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_topo_for_each_related(const occtl_graph_t* const theGraph,
                              const occtl_node_id_t      theNode,
                              const occtl_node_visitor_t theVisitor,
                              void* const                theUserData)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theVisitor == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "visitor fn is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    occtl_topo_related_iter_t* anIter = nullptr;
    occtl_status_t aStatus = ::occtl_topo_related_iter_create(theGraph, theNode, &anIter);
    if (aStatus != OCCTL_OK)
    {
      return aStatus;
    }
    occtl_node_id_t aNode;
    while ((aStatus = ::occtl_topo_related_iter_next(anIter, &aNode, nullptr)) == OCCTL_OK)
    {
      aStatus = theVisitor(aNode, theUserData);
      if (aStatus == OCCTL_CANCELLED)
      {
        aStatus = OCCTL_OK;
        break;
      }
      if (aStatus != OCCTL_OK)
      {
        break;
      }
    }
    ::occtl_topo_related_iter_free(anIter);
    return aStatus == OCCTL_NOT_FOUND ? OCCTL_OK : aStatus;
  });
}

} // extern "C"
