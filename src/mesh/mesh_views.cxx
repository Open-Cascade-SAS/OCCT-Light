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

//! @file mesh_views.cxx
//! @brief Triangulation / polygon3D / polygon-on-tri view accessors.
//!
//! All public buffers are materialised into the per-graph #MeshCache on
//! first read so subsequent fetches return the same pointers without
//! re-copying. Triangle / node-index buffers are converted to 0-indexed
//! at materialisation time per the indexing contract documented in
//! @c occtl_mesh.h.

#include "MeshCache.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <occtl/occtl_mesh.h>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepGraph_ChildExplorer.hxx>
#include <BRepGraph_Iterator.hxx>
#include <BRepGraph_MeshView.hxx>
#include <BRepGraph_NodeId.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraph_UIDsView.hxx>
#include <BRepGraph_VersionStamp.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_FlatDataMap.hxx>
#include <NCollection_LinearVector.hxx>
#include <NCollection_Vec3.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <math_Jacobi.hxx>
#include <math_Matrix.hxx>
#include <math_SVD.hxx>
#include <math_Vector.hxx>

#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>

namespace
{

bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool IsZeroOrOne(const int32_t theValue) noexcept
{
  return theValue == 0 || theValue == 1;
}

//! Fetches (creating if needed) the per-graph mesh cache. Thread-safe.
OcctL::Mesh::MeshCache& cacheFor(const occtl_graph* const theGraph)
{
  std::lock_guard<std::mutex> aInitLock(theGraph->meshCacheInitMutex);
  if (!theGraph->meshCache)
  {
    theGraph->meshCache = std::make_unique<OcctL::Mesh::MeshCache>();
  }
  return *theGraph->meshCache;
}

//! Resets the buffers of @p theSlot but keeps the slot present in the
//! map. Used when the cached stamp went stale: we re-materialise into
//! the same slot to keep map iterators / map keys valid.
void resetFaceSlot(OcctL::Mesh::FaceMeshBuffers& theSlot) noexcept
{
  theSlot.myNodes.Clear();
  theSlot.myNormals.Clear();
  theSlot.myUVs.Clear();
  theSlot.myTriangles.Clear();
  theSlot.myDeflection = 0.0;
  theSlot.mySourceUid  = OCCTL_UID_INVALID;
  theSlot.myStamp      = BRepGraph_VersionStamp{};
}

void resetCoEdgeSlot(OcctL::Mesh::CoEdgeMeshBuffers& theSlot) noexcept
{
  theSlot.myNodeIndices.Clear();
  theSlot.myParameters.Clear();
  theSlot.myDeflection = 0.0;
  theSlot.mySourceUid  = OCCTL_UID_INVALID;
  theSlot.myStamp      = BRepGraph_VersionStamp{};
}

void resetEdgeSlot(OcctL::Mesh::EdgeMeshBuffers& theSlot) noexcept
{
  theSlot.myNodes.Clear();
  theSlot.myParameters.Clear();
  theSlot.myDeflection = 0.0;
  theSlot.mySourceUid  = OCCTL_UID_INVALID;
  theSlot.myStamp      = BRepGraph_VersionStamp{};
}

//! Returns the persistent UID for a NodeId living in the supplied graph.
occtl_uid_t uidFor(const occtl_graph* const theGraph, const BRepGraph_NodeId theNodeId)
{
  const BRepGraph_UID aUid = theGraph->graph.UIDs().Of(theNodeId);
  return OcctL::Topo::PackUID(aUid);
}

//! Validates a node id and resolves it to a typed BRepGraph id of the
//! requested kind. Returns the matching status code on failure (NULL
//! graph / out → INVALID_ARGUMENT, removed/invalid → NOT_FOUND, wrong
//! kind → WRONG_KIND).
template <typename TypedId>
occtl_status_t resolveTyped(const occtl_graph* const     theGraph,
                            const occtl_node_id_t        theAbiId,
                            const BRepGraph_NodeId::Kind theKind,
                            TypedId&                     theOut)
{
  const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(theAbiId);
  if (!aNodeId.IsValid())
  {
    return OCCTL_NOT_FOUND;
  }
  if (aNodeId.NodeKind != theKind)
  {
    return OCCTL_WRONG_KIND;
  }
  if (!theGraph->graph.UIDs().Of(aNodeId).IsValid())
  {
    return OCCTL_NOT_FOUND;
  }
  theOut = TypedId(aNodeId);
  return OCCTL_OK;
}

//! Materialises a triangulation into the supplied cache slot. Idempotent:
//! if the slot is already populated (myNodes non-empty), this is a no-op.
//! @return true if the slot now carries a valid mesh; false if the source
//!         handle was null or had no triangles.
bool materialiseFace(const occ::handle<Poly_Triangulation>& theTri,
                     const occtl_uid_t                      theSourceUid,
                     OcctL::Mesh::FaceMeshBuffers&          theSlot)
{
  if (theTri.IsNull())
  {
    return false;
  }
  if (!theSlot.myNodes.IsEmpty())
  {
    return true;
  }

  const int aNbNodes     = theTri->NbNodes();
  const int aNbTriangles = theTri->NbTriangles();
  if (aNbNodes <= 0 || aNbTriangles <= 0)
  {
    return false;
  }

  // Materialise nodes to double regardless of Poly_Triangulation's precision.
  for (int i = 1; i <= aNbNodes; ++i)
  {
    const gp_Pnt aP = theTri->Node(i);
    theSlot.myNodes.Append(aP.X());
    theSlot.myNodes.Append(aP.Y());
    theSlot.myNodes.Append(aP.Z());
  }

  if (theTri->HasNormals())
  {
    for (int i = 1; i <= aNbNodes; ++i)
    {
      NCollection_Vec3<float> aN;
      theTri->Normal(i, aN);
      theSlot.myNormals.Append(static_cast<double>(aN.x()));
      theSlot.myNormals.Append(static_cast<double>(aN.y()));
      theSlot.myNormals.Append(static_cast<double>(aN.z()));
    }
  }

  if (theTri->HasUVNodes())
  {
    for (int i = 1; i <= aNbNodes; ++i)
    {
      const gp_Pnt2d aUV = theTri->UVNode(i);
      theSlot.myUVs.Append(aUV.X());
      theSlot.myUVs.Append(aUV.Y());
    }
  }

  for (int i = 1; i <= aNbTriangles; ++i)
  {
    const Poly_Triangle& aTri = theTri->Triangle(i);
    int                  aA = 0, aB = 0, aC = 0;
    aTri.Get(aA, aB, aC);
    theSlot.myTriangles.Append(static_cast<uint32_t>(aA - 1));
    theSlot.myTriangles.Append(static_cast<uint32_t>(aB - 1));
    theSlot.myTriangles.Append(static_cast<uint32_t>(aC - 1));
  }

  theSlot.myDeflection = theTri->Deflection();
  theSlot.mySourceUid  = theSourceUid;
  return true;
}

//! Fills the public POD view from a populated cache slot.
void fillFaceView(const OcctL::Mesh::FaceMeshBuffers& theSlot,
                  occtl_triangulation_view_t* const   theOutView)
{
  theOutView->nodes          = theSlot.myNodes.IsEmpty() ? nullptr : &theSlot.myNodes[0];
  theOutView->node_count     = theSlot.myNodes.Size() / 3;
  theOutView->normals        = theSlot.myNormals.IsEmpty() ? nullptr : &theSlot.myNormals[0];
  theOutView->uvs            = theSlot.myUVs.IsEmpty() ? nullptr : &theSlot.myUVs[0];
  theOutView->triangles      = theSlot.myTriangles.IsEmpty() ? nullptr : &theSlot.myTriangles[0];
  theOutView->triangle_count = theSlot.myTriangles.Size() / 3;
  theOutView->deflection     = theSlot.myDeflection;
  theOutView->source_uid     = theSlot.mySourceUid;
}

void appendTriangulationSoup(const occ::handle<Poly_Triangulation>& theTri,
                             const TopLoc_Location&                 theLocation,
                             OcctL::Mesh::TriangleSoupBuffers&      theSlot)
{
  if (theTri.IsNull())
  {
    return;
  }

  const int aNbNodes     = theTri->NbNodes();
  const int aNbTriangles = theTri->NbTriangles();
  if (aNbNodes <= 0 || aNbTriangles <= 0)
  {
    return;
  }

  const uint32_t aBase = static_cast<uint32_t>(theSlot.myNodes.Size() / 3);
  const gp_Trsf  aTrsf = theLocation.Transformation();
  for (int i = 1; i <= aNbNodes; ++i)
  {
    const gp_Pnt aP =
      theLocation.IsIdentity() ? theTri->Node(i) : theTri->Node(i).Transformed(aTrsf);
    theSlot.myNodes.Append(aP.X());
    theSlot.myNodes.Append(aP.Y());
    theSlot.myNodes.Append(aP.Z());
  }

  for (int i = 1; i <= aNbTriangles; ++i)
  {
    const Poly_Triangle& aTri = theTri->Triangle(i);
    int                  aA = 0, aB = 0, aC = 0;
    aTri.Get(aA, aB, aC);
    theSlot.myTriangles.Append(aBase + static_cast<uint32_t>(aA - 1));
    theSlot.myTriangles.Append(aBase + static_cast<uint32_t>(aB - 1));
    theSlot.myTriangles.Append(aBase + static_cast<uint32_t>(aC - 1));
  }
  ++theSlot.myFaceCount;
}

void appendFaceSoup(const occtl_graph* const          theGraph,
                    const BRepGraph_FaceId            theFaceId,
                    const TopLoc_Location&            theLocation,
                    OcctL::Mesh::TriangleSoupBuffers& theSlot)
{
  const BRepGraph::MeshView::CacheView::FaceOps& aFaceOps = theGraph->graph.Mesh().Cache().Faces();
  const int aActiveIdx = aFaceOps.ActiveTriangulationIndex(theFaceId);
  if (aActiveIdx < 0)
  {
    return;
  }

  const occ::handle<Poly_Triangulation>& aTri =
    aFaceOps.Triangulation(theFaceId, static_cast<uint32_t>(aActiveIdx));
  appendTriangulationSoup(aTri, theLocation, theSlot);
}

void fillTriangleSoupView(const OcctL::Mesh::TriangleSoupBuffers&   theSlot,
                          occtl_mesh_triangle_buffers_view_t* const theOutView)
{
  theOutView->nodes          = theSlot.myNodes.IsEmpty() ? nullptr : &theSlot.myNodes[0];
  theOutView->node_count     = theSlot.myNodes.Size() / 3;
  theOutView->triangles      = theSlot.myTriangles.IsEmpty() ? nullptr : &theSlot.myTriangles[0];
  theOutView->triangle_count = theSlot.myTriangles.Size() / 3;
  theOutView->face_count     = theSlot.myFaceCount;
  theOutView->root           = theSlot.myRoot;
}

occtl_status_t populateTriangleSoup(const occtl_graph* const          theGraph,
                                    const occtl_node_id_t             theRoot,
                                    OcctL::Mesh::TriangleSoupBuffers& theSlot)
{
  theSlot.myRoot = theRoot;

  if (theRoot.bits == 0u)
  {
    for (BRepGraph_FaceIterator anIt(theGraph->graph); anIt.More(); anIt.Next())
    {
      appendFaceSoup(theGraph, BRepGraph_FaceId(anIt.CurrentId()), TopLoc_Location(), theSlot);
    }
  }
  else
  {
    const BRepGraph_NodeId aRootId = OcctL::Topo::UnpackNodeId(theRoot);
    if (!aRootId.IsValid() || theGraph->graph.Topo().Gen().IsRemoved(aRootId))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "root is invalid or removed");
      return OCCTL_NOT_FOUND;
    }

    if (aRootId.NodeKind == BRepGraph_NodeId::Kind::Face)
    {
      appendFaceSoup(theGraph, BRepGraph_FaceId(aRootId), TopLoc_Location(), theSlot);
    }
    else
    {
      BRepGraph_ChildExplorer::Config aConfig;
      aConfig.TargetKind            = BRepGraph_NodeId::Kind::Face;
      aConfig.AccumulateLocation    = true;
      aConfig.AccumulateOrientation = false;
      for (BRepGraph_ChildExplorer anIt(theGraph->graph, aRootId, aConfig); anIt.More();
           anIt.Next())
      {
        const BRepGraphInc::NodeInstance anInst = anIt.Current();
        appendFaceSoup(theGraph, BRepGraph_FaceId(anInst.DefId), anInst.Location, theSlot);
      }
    }
  }

  if (theSlot.myFaceCount == 0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "no cached face triangulations found");
    return OCCTL_NOT_FOUND;
  }
  return OCCTL_OK;
}

uint64_t edgeKey(const uint32_t theFirst, const uint32_t theSecond)
{
  const uint32_t aMin = std::min(theFirst, theSecond);
  const uint32_t aMax = std::max(theFirst, theSecond);
  return (static_cast<uint64_t>(aMin) << 32) | static_cast<uint64_t>(aMax);
}

void materialiseTriangleAnalysis(const OcctL::Mesh::TriangleSoupBuffers& theSoup,
                                 OcctL::Mesh::TriangleAnalysisBuffers&   theAnalysis)
{
  theAnalysis.myRoot          = theSoup.myRoot;
  theAnalysis.myTriangleCount = theSoup.myTriangles.Size() / 3;
  theAnalysis.myFaceCount     = theSoup.myFaceCount;

  for (size_t aTriIdx = 0; aTriIdx < theAnalysis.myTriangleCount; ++aTriIdx)
  {
    const uint32_t anA = theSoup.myTriangles[aTriIdx * 3u];
    const uint32_t aB  = theSoup.myTriangles[aTriIdx * 3u + 1u];
    const uint32_t aC  = theSoup.myTriangles[aTriIdx * 3u + 2u];

    const size_t anABase = static_cast<size_t>(anA) * 3u;
    const size_t aBBase  = static_cast<size_t>(aB) * 3u;
    const size_t aCBase  = static_cast<size_t>(aC) * 3u;
    const gp_Pnt aP0(theSoup.myNodes[anABase],
                     theSoup.myNodes[anABase + 1u],
                     theSoup.myNodes[anABase + 2u]);
    const gp_Pnt aP1(theSoup.myNodes[aBBase],
                     theSoup.myNodes[aBBase + 1u],
                     theSoup.myNodes[aBBase + 2u]);
    const gp_Pnt aP2(theSoup.myNodes[aCBase],
                     theSoup.myNodes[aCBase + 1u],
                     theSoup.myNodes[aCBase + 2u]);

    gp_Vec aNormal(gp_Vec(aP0, aP1).Crossed(gp_Vec(aP0, aP2)));
    if (aNormal.SquareMagnitude() > Precision::SquareConfusion())
    {
      aNormal.Normalize();
      theAnalysis.myNormals.Append(aNormal.X());
      theAnalysis.myNormals.Append(aNormal.Y());
      theAnalysis.myNormals.Append(aNormal.Z());
    }
    else
    {
      theAnalysis.myNormals.Append(0.0);
      theAnalysis.myNormals.Append(0.0);
      theAnalysis.myNormals.Append(0.0);
    }
  }

  for (size_t anIdx = 0; anIdx < theAnalysis.myTriangleCount * 3u; ++anIdx)
  {
    theAnalysis.myAdjacency.Append(OCCTL_MESH_TRIANGLE_ADJACENCY_BOUNDARY);
  }

  struct EdgeUse
  {
    uint32_t myTriangle;
    uint32_t myLocalEdge;
  };

  NCollection_FlatDataMap<uint64_t, EdgeUse> anEdges;
  anEdges.Reserve(theAnalysis.myTriangleCount * 3u);

  for (size_t aTriIdx = 0; aTriIdx < theAnalysis.myTriangleCount; ++aTriIdx)
  {
    const uint32_t aVerts[3] = {theSoup.myTriangles[aTriIdx * 3u],
                                theSoup.myTriangles[aTriIdx * 3u + 1u],
                                theSoup.myTriangles[aTriIdx * 3u + 2u]};
    for (uint32_t aLocal = 0; aLocal < 3u; ++aLocal)
    {
      const uint32_t aFirst  = aVerts[aLocal];
      const uint32_t aSecond = aVerts[(aLocal + 1u) % 3u];
      const uint64_t aKey    = edgeKey(aFirst, aSecond);
      const uint32_t aTri    = static_cast<uint32_t>(aTriIdx);
      if (anEdges.TryBind(aKey, EdgeUse{aTri, aLocal}))
      {
        continue;
      }

      const EdgeUse* anOther = anEdges.Seek(aKey);
      if (anOther == nullptr)
      {
        continue;
      }
      theAnalysis.myAdjacency[aTri * 3u + aLocal] = anOther->myTriangle;
      theAnalysis.myAdjacency[anOther->myTriangle * 3u + anOther->myLocalEdge] = aTri;
    }
  }
}

void fillTriangleAnalysisView(const OcctL::Mesh::TriangleAnalysisBuffers& theSlot,
                              occtl_mesh_triangle_analysis_view_t* const  theOutView)
{
  theOutView->triangle_normals = theSlot.myNormals.IsEmpty() ? nullptr : &theSlot.myNormals[0];
  theOutView->triangle_adjacency =
    theSlot.myAdjacency.IsEmpty() ? nullptr : &theSlot.myAdjacency[0];
  theOutView->triangle_count = theSlot.myTriangleCount;
  theOutView->face_count     = theSlot.myFaceCount;
  theOutView->root           = theSlot.myRoot;
}

void materialiseTriangleComponents(const OcctL::Mesh::TriangleAnalysisBuffers&     theAnalysis,
                                   const occtl_mesh_triangle_components_options_t& theOptions,
                                   OcctL::Mesh::TriangleComponentBuffers&          theComponents)
{
  theComponents.myRoot          = theAnalysis.myRoot;
  theComponents.myTriangleCount = theAnalysis.myTriangleCount;

  const double aPi = OCCTL_PI;
  if (theOptions.max_normal_angle >= aPi - 1.0e-12)
  {
    for (size_t anIdx = 0; anIdx < theComponents.myTriangleCount; ++anIdx)
    {
      theComponents.myComponentIds.Append(0u);
    }
    if (theComponents.myTriangleCount > 0u)
    {
      theComponents.myComponentSizes.Append(static_cast<uint32_t>(theComponents.myTriangleCount));
      theComponents.myComponentCount = 1u;
    }
    return;
  }

  for (size_t anIdx = 0; anIdx < theComponents.myTriangleCount; ++anIdx)
  {
    theComponents.myComponentIds.Append(std::numeric_limits<uint32_t>::max());
  }

  const double                       aCosLimit      = std::cos(theOptions.max_normal_angle);
  uint32_t                           aNextComponent = 0;
  NCollection_LinearVector<uint32_t> aQueue;

  for (uint32_t aSeed = 0; aSeed < static_cast<uint32_t>(theComponents.myTriangleCount); ++aSeed)
  {
    if (theComponents.myComponentIds[aSeed] != std::numeric_limits<uint32_t>::max())
    {
      continue;
    }

    uint32_t aSize                      = 0;
    theComponents.myComponentIds[aSeed] = aNextComponent;
    aQueue.Clear();
    aQueue.Append(aSeed);

    for (size_t aQueueHead = 0; aQueueHead < aQueue.Size(); ++aQueueHead)
    {
      const uint32_t aTri = aQueue.Value(aQueueHead);
      ++aSize;

      const gp_Vec aNormal(theAnalysis.myNormals[aTri * 3u],
                           theAnalysis.myNormals[aTri * 3u + 1u],
                           theAnalysis.myNormals[aTri * 3u + 2u]);
      for (uint32_t aLocal = 0; aLocal < 3u; ++aLocal)
      {
        const uint32_t aNeighbor = theAnalysis.myAdjacency[aTri * 3u + aLocal];
        if (aNeighbor == OCCTL_MESH_TRIANGLE_ADJACENCY_BOUNDARY
            || aNeighbor >= theComponents.myTriangleCount
            || theComponents.myComponentIds[aNeighbor] != std::numeric_limits<uint32_t>::max())
        {
          continue;
        }

        const gp_Vec aNeighborNormal(theAnalysis.myNormals[aNeighbor * 3u],
                                     theAnalysis.myNormals[aNeighbor * 3u + 1u],
                                     theAnalysis.myNormals[aNeighbor * 3u + 2u]);
        double       aDot = aNormal.Dot(aNeighborNormal);
        if (theOptions.include_opposite_normals != 0)
        {
          aDot = std::abs(aDot);
        }
        if (aDot < aCosLimit)
        {
          continue;
        }

        theComponents.myComponentIds[aNeighbor] = aNextComponent;
        aQueue.Append(aNeighbor);
      }
    }

    theComponents.myComponentSizes.Append(aSize);
    ++aNextComponent;
  }

  theComponents.myComponentCount = aNextComponent;
}

void materialiseTriangleComponentSummaries(const OcctL::Mesh::TriangleSoupBuffers& theSoup,
                                           OcctL::Mesh::TriangleComponentBuffers&  theComponents)
{
  theComponents.mySummaries.Clear();
  for (uint32_t aComponent = 0; aComponent < theComponents.myComponentCount; ++aComponent)
  {
    occtl_mesh_triangle_component_summary_t aSummary{};
    aSummary.component_id   = aComponent;
    aSummary.triangle_count = aComponent < theComponents.myComponentSizes.Size()
                                ? theComponents.myComponentSizes[aComponent]
                                : 0u;
    aSummary.bounds.min.x   = std::numeric_limits<double>::infinity();
    aSummary.bounds.min.y   = std::numeric_limits<double>::infinity();
    aSummary.bounds.min.z   = std::numeric_limits<double>::infinity();
    aSummary.bounds.max.x   = -std::numeric_limits<double>::infinity();
    aSummary.bounds.max.y   = -std::numeric_limits<double>::infinity();
    aSummary.bounds.max.z   = -std::numeric_limits<double>::infinity();
    theComponents.mySummaries.Append(aSummary);
  }

  NCollection_Array1<double> aCentroidX(theComponents.myComponentCount);
  NCollection_Array1<double> aCentroidY(theComponents.myComponentCount);
  NCollection_Array1<double> aCentroidZ(theComponents.myComponentCount);
  NCollection_Array1<gp_Vec> aNormalSums(theComponents.myComponentCount);
  for (size_t aComponent = 0; aComponent < theComponents.myComponentCount; ++aComponent)
  {
    aCentroidX.ChangeAt(aComponent)  = 0.0;
    aCentroidY.ChangeAt(aComponent)  = 0.0;
    aCentroidZ.ChangeAt(aComponent)  = 0.0;
    aNormalSums.ChangeAt(aComponent) = gp_Vec(0.0, 0.0, 0.0);
  }

  for (uint32_t aTri = 0; aTri < static_cast<uint32_t>(theComponents.myTriangleCount); ++aTri)
  {
    const uint32_t aComponent = theComponents.myComponentIds[aTri];
    if (aComponent >= theComponents.myComponentCount)
    {
      continue;
    }

    const uint32_t anA     = theSoup.myTriangles[aTri * 3u];
    const uint32_t aB      = theSoup.myTriangles[aTri * 3u + 1u];
    const uint32_t aC      = theSoup.myTriangles[aTri * 3u + 2u];
    const size_t   anABase = static_cast<size_t>(anA) * 3u;
    const size_t   aBBase  = static_cast<size_t>(aB) * 3u;
    const size_t   aCBase  = static_cast<size_t>(aC) * 3u;
    const gp_Pnt   aP0(theSoup.myNodes[anABase],
                       theSoup.myNodes[anABase + 1u],
                       theSoup.myNodes[anABase + 2u]);
    const gp_Pnt   aP1(theSoup.myNodes[aBBase],
                       theSoup.myNodes[aBBase + 1u],
                       theSoup.myNodes[aBBase + 2u]);
    const gp_Pnt   aP2(theSoup.myNodes[aCBase],
                       theSoup.myNodes[aCBase + 1u],
                       theSoup.myNodes[aCBase + 2u]);

    gp_Vec                                   aCross   = gp_Vec(aP0, aP1).Crossed(gp_Vec(aP0, aP2));
    const double                             anArea   = 0.5 * aCross.Magnitude();
    occtl_mesh_triangle_component_summary_t& aSummary = theComponents.mySummaries[aComponent];
    aSummary.area += anArea;
    aCentroidX.ChangeAt(aComponent) += anArea * (aP0.X() + aP1.X() + aP2.X()) / 3.0;
    aCentroidY.ChangeAt(aComponent) += anArea * (aP0.Y() + aP1.Y() + aP2.Y()) / 3.0;
    aCentroidZ.ChangeAt(aComponent) += anArea * (aP0.Z() + aP1.Z() + aP2.Z()) / 3.0;
    aNormalSums.ChangeAt(aComponent) += aCross;

    const gp_Pnt aPoints[3] = {aP0, aP1, aP2};
    for (const gp_Pnt& aPoint : aPoints)
    {
      aSummary.bounds.min.x = std::min(aSummary.bounds.min.x, aPoint.X());
      aSummary.bounds.min.y = std::min(aSummary.bounds.min.y, aPoint.Y());
      aSummary.bounds.min.z = std::min(aSummary.bounds.min.z, aPoint.Z());
      aSummary.bounds.max.x = std::max(aSummary.bounds.max.x, aPoint.X());
      aSummary.bounds.max.y = std::max(aSummary.bounds.max.y, aPoint.Y());
      aSummary.bounds.max.z = std::max(aSummary.bounds.max.z, aPoint.Z());
    }
  }

  for (uint32_t aComponent = 0; aComponent < theComponents.myComponentCount; ++aComponent)
  {
    occtl_mesh_triangle_component_summary_t& aSummary = theComponents.mySummaries[aComponent];
    if (aSummary.area > Precision::Confusion())
    {
      aSummary.centroid.x = aCentroidX.At(aComponent) / aSummary.area;
      aSummary.centroid.y = aCentroidY.At(aComponent) / aSummary.area;
      aSummary.centroid.z = aCentroidZ.At(aComponent) / aSummary.area;
    }
    if (aNormalSums.At(aComponent).SquareMagnitude() > Precision::SquareConfusion())
    {
      aNormalSums.ChangeAt(aComponent).Normalize();
      aSummary.normal.x = aNormalSums.At(aComponent).X();
      aSummary.normal.y = aNormalSums.At(aComponent).Y();
      aSummary.normal.z = aNormalSums.At(aComponent).Z();
    }
  }
}

void fillTriangleComponentsView(const OcctL::Mesh::TriangleComponentBuffers& theSlot,
                                occtl_mesh_triangle_components_view_t* const theOutView)
{
  theOutView->triangle_component_ids =
    theSlot.myComponentIds.IsEmpty() ? nullptr : &theSlot.myComponentIds[0];
  theOutView->triangle_count = theSlot.myTriangleCount;
  theOutView->component_sizes =
    theSlot.myComponentSizes.IsEmpty() ? nullptr : &theSlot.myComponentSizes[0];
  theOutView->component_count = theSlot.myComponentCount;
  theOutView->root            = theSlot.myRoot;
}

void materialiseSelectedComponentTriangles(OcctL::Mesh::TriangleComponentBuffers& theComponents,
                                           const uint32_t                         theComponentId)
{
  theComponents.mySelectedTriangles.Clear();
  for (uint32_t aTri = 0; aTri < static_cast<uint32_t>(theComponents.myTriangleCount); ++aTri)
  {
    if (theComponents.myComponentIds[aTri] == theComponentId)
    {
      theComponents.mySelectedTriangles.Append(aTri);
    }
  }
}

void fillComponentTrianglesView(const OcctL::Mesh::TriangleComponentBuffers& theSlot,
                                const uint32_t                               theComponentId,
                                occtl_mesh_triangle_component_triangles_view_t* const theOutView)
{
  theOutView->triangles =
    theSlot.mySelectedTriangles.IsEmpty() ? nullptr : &theSlot.mySelectedTriangles[0];
  theOutView->triangle_count = theSlot.mySelectedTriangles.Size();
  theOutView->component_id   = theComponentId;
  theOutView->root           = theSlot.myRoot;
}

void materialiseSelectedComponentBoundary(const OcctL::Mesh::TriangleSoupBuffers&     theSoup,
                                          const OcctL::Mesh::TriangleAnalysisBuffers& theAnalysis,
                                          OcctL::Mesh::TriangleComponentBuffers&      theComponents,
                                          const uint32_t theComponentId)
{
  theComponents.mySelectedBoundaryEdges.Clear();
  for (uint32_t aTri = 0; aTri < static_cast<uint32_t>(theComponents.myTriangleCount); ++aTri)
  {
    if (theComponents.myComponentIds[aTri] != theComponentId)
    {
      continue;
    }

    const uint32_t aVerts[3] = {theSoup.myTriangles[aTri * 3u],
                                theSoup.myTriangles[aTri * 3u + 1u],
                                theSoup.myTriangles[aTri * 3u + 2u]};

    for (uint32_t aLocal = 0; aLocal < 3u; ++aLocal)
    {
      const uint32_t aNeighbor = theAnalysis.myAdjacency[aTri * 3u + aLocal];
      if (aNeighbor != OCCTL_MESH_TRIANGLE_ADJACENCY_BOUNDARY
          && aNeighbor < theComponents.myTriangleCount
          && theComponents.myComponentIds[aNeighbor] == theComponentId)
      {
        continue;
      }

      occtl_mesh_triangle_component_boundary_edge_t anEdge{};
      anEdge.triangle          = aTri;
      anEdge.local_edge        = aLocal;
      anEdge.node0             = aVerts[aLocal];
      anEdge.node1             = aVerts[(aLocal + 1u) % 3u];
      anEdge.adjacent_triangle = aNeighbor;
      theComponents.mySelectedBoundaryEdges.Append(anEdge);
    }
  }
}

void fillComponentBoundaryView(const OcctL::Mesh::TriangleComponentBuffers&         theSlot,
                               const uint32_t                                       theComponentId,
                               occtl_mesh_triangle_component_boundary_view_t* const theOutView)
{
  theOutView->edges =
    theSlot.mySelectedBoundaryEdges.IsEmpty() ? nullptr : &theSlot.mySelectedBoundaryEdges[0];
  theOutView->edge_count   = theSlot.mySelectedBoundaryEdges.Size();
  theOutView->component_id = theComponentId;
  theOutView->root         = theSlot.myRoot;
}

occtl_mesh_triangle_component_boundary_edge_t reversedBoundaryEdge(
  const occtl_mesh_triangle_component_boundary_edge_t& theEdge)
{
  occtl_mesh_triangle_component_boundary_edge_t aReversed = theEdge;
  std::swap(aReversed.node0, aReversed.node1);
  return aReversed;
}

void materialiseSelectedComponentBoundaryChains(
  OcctL::Mesh::TriangleComponentBuffers& theComponents)
{
  theComponents.myOrderedBoundaryEdges.Clear();
  theComponents.myBoundaryChains.Clear();

  const uint32_t aNbEdges = static_cast<uint32_t>(theComponents.mySelectedBoundaryEdges.Size());
  if (aNbEdges == 0u)
  {
    return;
  }

  NCollection_FlatDataMap<uint32_t, NCollection_LinearVector<uint32_t>> aNodeToEdges;
  aNodeToEdges.Reserve(aNbEdges * 2u);
  for (uint32_t anEdgeIdx = 0u; anEdgeIdx < aNbEdges; ++anEdgeIdx)
  {
    const occtl_mesh_triangle_component_boundary_edge_t& anEdge =
      theComponents.mySelectedBoundaryEdges[anEdgeIdx];
    aNodeToEdges.TryBound(anEdge.node0, NCollection_LinearVector<uint32_t>()).Append(anEdgeIdx);
    aNodeToEdges.TryBound(anEdge.node1, NCollection_LinearVector<uint32_t>()).Append(anEdgeIdx);
  }

  NCollection_LinearVector<unsigned char> aVisited;
  aVisited.Reserve(aNbEdges);
  for (uint32_t anEdgeIdx = 0u; anEdgeIdx < aNbEdges; ++anEdgeIdx)
  {
    aVisited.Append(0u);
  }
  for (uint32_t aSeedEdge = 0u; aSeedEdge < aNbEdges; ++aSeedEdge)
  {
    if (aVisited[aSeedEdge] != 0u)
    {
      continue;
    }

    const occtl_mesh_triangle_component_boundary_edge_t& aSeed =
      theComponents.mySelectedBoundaryEdges[aSeedEdge];
    uint32_t                                  aStartNode  = aSeed.node0;
    const NCollection_LinearVector<uint32_t>* aNode0Edges = aNodeToEdges.Seek(aSeed.node0);
    const NCollection_LinearVector<uint32_t>* aNode1Edges = aNodeToEdges.Seek(aSeed.node1);
    if (aNode0Edges != nullptr && aNode0Edges->Size() == 1u)
    {
      aStartNode = aSeed.node0;
    }
    else if (aNode1Edges != nullptr && aNode1Edges->Size() == 1u)
    {
      aStartNode = aSeed.node1;
    }

    const uint32_t aFirstOrdered =
      static_cast<uint32_t>(theComponents.myOrderedBoundaryEdges.Size());
    uint32_t aCurrentNode = aStartNode;
    uint32_t aCurrentEdge = aSeedEdge;
    int32_t  isClosed     = 0;

    for (;;)
    {
      if (aVisited[aCurrentEdge] != 0u)
      {
        break;
      }
      aVisited[aCurrentEdge] = 1u;

      const occtl_mesh_triangle_component_boundary_edge_t& aCurrent =
        theComponents.mySelectedBoundaryEdges[aCurrentEdge];
      occtl_mesh_triangle_component_boundary_edge_t anOriented = aCurrent;
      if (aCurrent.node1 == aCurrentNode)
      {
        anOriented = reversedBoundaryEdge(aCurrent);
      }
      else if (aCurrent.node0 != aCurrentNode)
      {
        aCurrentNode = aCurrent.node0;
      }

      theComponents.myOrderedBoundaryEdges.Append(anOriented);
      const uint32_t aNextNode = anOriented.node1;

      uint32_t                                  aNextEdge  = std::numeric_limits<uint32_t>::max();
      const NCollection_LinearVector<uint32_t>* aNextEdges = aNodeToEdges.Seek(aNextNode);
      if (aNextEdges != nullptr)
      {
        for (const uint32_t aCandidate : *aNextEdges)
        {
          if (aVisited[aCandidate] == 0u)
          {
            aNextEdge = aCandidate;
            break;
          }
        }
      }

      if (aNextEdge == std::numeric_limits<uint32_t>::max())
      {
        isClosed = (aNextNode == aStartNode) ? 1 : 0;
        break;
      }

      aCurrentNode = aNextNode;
      aCurrentEdge = aNextEdge;
    }

    occtl_mesh_triangle_component_boundary_chain_t aChain{};
    aChain.first_edge = aFirstOrdered;
    aChain.edge_count =
      static_cast<uint32_t>(theComponents.myOrderedBoundaryEdges.Size()) - aFirstOrdered;
    aChain.is_closed = isClosed;
    if (aChain.edge_count > 0u)
    {
      theComponents.myBoundaryChains.Append(aChain);
    }
  }
}

void fillComponentBoundaryChainsView(
  const OcctL::Mesh::TriangleComponentBuffers&                theSlot,
  const uint32_t                                              theComponentId,
  occtl_mesh_triangle_component_boundary_chains_view_t* const theOutView)
{
  theOutView->edges =
    theSlot.myOrderedBoundaryEdges.IsEmpty() ? nullptr : &theSlot.myOrderedBoundaryEdges[0];
  theOutView->edge_count = theSlot.myOrderedBoundaryEdges.Size();
  theOutView->chains = theSlot.myBoundaryChains.IsEmpty() ? nullptr : &theSlot.myBoundaryChains[0];
  theOutView->chain_count  = theSlot.myBoundaryChains.Size();
  theOutView->component_id = theComponentId;
  theOutView->root         = theSlot.myRoot;
}

occtl_point3_t pointAtNode(const OcctL::Mesh::TriangleSoupBuffers& theSoup, const uint32_t theNode)
{
  const size_t   aBase = static_cast<size_t>(theNode) * 3u;
  occtl_point3_t aPoint{};
  aPoint.x = theSoup.myNodes[aBase];
  aPoint.y = theSoup.myNodes[aBase + 1u];
  aPoint.z = theSoup.myNodes[aBase + 2u];
  return aPoint;
}

void materialiseSelectedComponentBoundaryPolylines(
  const OcctL::Mesh::TriangleSoupBuffers& theSoup,
  OcctL::Mesh::TriangleComponentBuffers&  theComponents)
{
  theComponents.myBoundaryPolylinePoints.Clear();
  theComponents.myBoundaryPolylines.Clear();

  for (uint32_t aChainIdx = 0u;
       aChainIdx < static_cast<uint32_t>(theComponents.myBoundaryChains.Size());
       ++aChainIdx)
  {
    const occtl_mesh_triangle_component_boundary_chain_t& aChain =
      theComponents.myBoundaryChains[aChainIdx];
    if (aChain.edge_count == 0u)
    {
      continue;
    }

    occtl_mesh_component_boundary_polyline_t aPolyline{};
    aPolyline.first_point = static_cast<uint32_t>(theComponents.myBoundaryPolylinePoints.Size());
    aPolyline.is_closed   = aChain.is_closed;

    const uint32_t aFirstEdge = aChain.first_edge;
    const uint32_t anEndEdge  = aChain.first_edge + aChain.edge_count;
    const occtl_mesh_triangle_component_boundary_edge_t& aFirst =
      theComponents.myOrderedBoundaryEdges[aFirstEdge];
    theComponents.myBoundaryPolylinePoints.Append(pointAtNode(theSoup, aFirst.node0));
    for (uint32_t anEdgeIdx = aFirstEdge; anEdgeIdx < anEndEdge; ++anEdgeIdx)
    {
      const occtl_mesh_triangle_component_boundary_edge_t& anEdge =
        theComponents.myOrderedBoundaryEdges[anEdgeIdx];
      theComponents.myBoundaryPolylinePoints.Append(pointAtNode(theSoup, anEdge.node1));
    }

    aPolyline.point_count =
      static_cast<uint32_t>(theComponents.myBoundaryPolylinePoints.Size()) - aPolyline.first_point;
    theComponents.myBoundaryPolylines.Append(aPolyline);
  }
}

void fillComponentBoundaryPolylinesView(
  const OcctL::Mesh::TriangleComponentBuffers&          theSlot,
  const uint32_t                                        theComponentId,
  occtl_mesh_component_boundary_polylines_view_t* const theOutView)
{
  theOutView->points =
    theSlot.myBoundaryPolylinePoints.IsEmpty() ? nullptr : &theSlot.myBoundaryPolylinePoints[0];
  theOutView->point_count = theSlot.myBoundaryPolylinePoints.Size();
  theOutView->polylines =
    theSlot.myBoundaryPolylines.IsEmpty() ? nullptr : &theSlot.myBoundaryPolylines[0];
  theOutView->polyline_count = theSlot.myBoundaryPolylines.Size();
  theOutView->component_id   = theComponentId;
  theOutView->root           = theSlot.myRoot;
}

void fillTriangleComponentSummariesView(
  const OcctL::Mesh::TriangleComponentBuffers&          theSlot,
  occtl_mesh_triangle_component_summaries_view_t* const theOutView)
{
  theOutView->summaries       = theSlot.mySummaries.IsEmpty() ? nullptr : &theSlot.mySummaries[0];
  theOutView->component_count = theSlot.myComponentCount;
  theOutView->triangle_count  = theSlot.myTriangleCount;
  theOutView->root            = theSlot.myRoot;
}

occtl_status_t validateTriangleComponentsOptions(
  const occtl_mesh_triangle_components_options_t* const theOptions)
{
  if (theOptions == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "options is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOptions->struct_version != OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_mesh_triangle_components_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOptions->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_mesh_triangle_components_options_t::p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  const double aPi = OCCTL_PI;
  if (!IsFiniteValue(theOptions->max_normal_angle) || theOptions->max_normal_angle < 0.0
      || theOptions->max_normal_angle > aPi)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "max_normal_angle must be finite and in [0, pi]");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOptions->include_opposite_normals))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "include_opposite_normals must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

occtl_status_t validateTrianglePlaneComponentsOptions(
  const occtl_mesh_triangle_plane_components_options_t* const theOptions)
{
  if (theOptions == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "options is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOptions->struct_version != OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_mesh_triangle_plane_components_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOptions->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_mesh_triangle_plane_components_options_t::p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  const double aPi = OCCTL_PI;
  if (!IsFiniteValue(theOptions->max_normal_angle) || theOptions->max_normal_angle < 0.0
      || theOptions->max_normal_angle > aPi || !IsFiniteValue(theOptions->max_distance)
      || theOptions->max_distance < 0.0 || !IsFiniteValue(theOptions->min_area)
      || theOptions->min_area < 0.0 || theOptions->min_triangle_count == 0u)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "plane component options contain invalid numeric values");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOptions->include_opposite_normals))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "include_opposite_normals must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

occtl_status_t validateTriangleSphereComponentsOptions(
  const occtl_mesh_triangle_sphere_components_options_t* const theOptions)
{
  if (theOptions == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "options is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOptions->struct_version != OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_mesh_triangle_sphere_components_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOptions->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_mesh_triangle_sphere_components_options_t::p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  const double aPi = OCCTL_PI;
  if (!IsFiniteValue(theOptions->max_normal_angle) || theOptions->max_normal_angle < 0.0
      || theOptions->max_normal_angle > aPi || !IsFiniteValue(theOptions->max_distance)
      || theOptions->max_distance < 0.0 || !IsFiniteValue(theOptions->min_area)
      || theOptions->min_area < 0.0 || theOptions->min_triangle_count == 0u
      || !IsFiniteValue(theOptions->min_radius) || theOptions->min_radius < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "sphere component options contain invalid numeric values");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOptions->include_opposite_normals))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "include_opposite_normals must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

occtl_status_t validateTriangleCylinderComponentsOptions(
  const occtl_mesh_triangle_cylinder_components_options_t* const theOptions)
{
  if (theOptions == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "options is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOptions->struct_version != OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_mesh_triangle_cylinder_components_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOptions->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_mesh_triangle_cylinder_components_options_t::p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  const double aPi = OCCTL_PI;
  if (!IsFiniteValue(theOptions->max_normal_angle) || theOptions->max_normal_angle < 0.0
      || theOptions->max_normal_angle > aPi || !IsFiniteValue(theOptions->max_distance)
      || theOptions->max_distance < 0.0 || !IsFiniteValue(theOptions->min_area)
      || theOptions->min_area < 0.0 || theOptions->min_triangle_count == 0u
      || !IsFiniteValue(theOptions->min_radius) || theOptions->min_radius < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "cylinder component options contain invalid numeric values");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOptions->include_opposite_normals))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "include_opposite_normals must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

void appendUniqueComponentNodes(const OcctL::Mesh::TriangleSoupBuffers&      theSoup,
                                const OcctL::Mesh::TriangleComponentBuffers& theComponents,
                                const uint32_t                               theComponent,
                                NCollection_LinearVector<uint32_t>&          theNodes)
{
  theNodes.Clear();
  NCollection_Array1<unsigned char> aSeen(theSoup.myNodes.Size() / 3u);
  for (size_t aNodeIndex = 0; aNodeIndex < aSeen.Size(); ++aNodeIndex)
  {
    aSeen.ChangeAt(aNodeIndex) = 0u;
  }
  for (uint32_t aTri = 0; aTri < static_cast<uint32_t>(theComponents.myTriangleCount); ++aTri)
  {
    if (theComponents.myComponentIds[aTri] != theComponent)
    {
      continue;
    }

    for (uint32_t aLocal = 0; aLocal < 3u; ++aLocal)
    {
      const uint32_t aNode = theSoup.myTriangles[aTri * 3u + aLocal];
      if (static_cast<size_t>(aNode) < aSeen.Size() && aSeen.At(aNode) == 0u)
      {
        aSeen.ChangeAt(aNode) = 1u;
        theNodes.Append(aNode);
      }
    }
  }
}

bool fitComponentSphere(const OcctL::Mesh::TriangleSoupBuffers&   theSoup,
                        const NCollection_LinearVector<uint32_t>& theNodes,
                        gp_Pnt&                                   theCenter,
                        double&                                   theRadius)
{
  if (theNodes.Size() < 4u)
  {
    return false;
  }

  math_Matrix aMatrix(1, static_cast<int>(theNodes.Size()), 1, 4, 0.0);
  math_Vector aRhs(1, static_cast<int>(theNodes.Size()), 0.0);
  for (int anIdx = 1; anIdx <= static_cast<int>(theNodes.Size()); ++anIdx)
  {
    const uint32_t aNode = theNodes.Value(static_cast<size_t>(anIdx - 1));
    const size_t   aBase = static_cast<size_t>(aNode) * 3u;
    const double   anX   = theSoup.myNodes[aBase];
    const double   aY    = theSoup.myNodes[aBase + 1u];
    const double   aZ    = theSoup.myNodes[aBase + 2u];
    aMatrix(anIdx, 1)    = anX;
    aMatrix(anIdx, 2)    = aY;
    aMatrix(anIdx, 3)    = aZ;
    aMatrix(anIdx, 4)    = 1.0;
    aRhs(anIdx)          = -(anX * anX + aY * aY + aZ * aZ);
  }

  math_SVD aSolver(aMatrix);
  if (!aSolver.IsDone())
  {
    return false;
  }

  math_Vector aSolution(1, 4, 0.0);
  aSolver.Solve(aRhs, aSolution);
  const double aCx = -0.5 * aSolution(1);
  const double aCy = -0.5 * aSolution(2);
  const double aCz = -0.5 * aSolution(3);
  const gp_Vec aCenterVec(aCx, aCy, aCz);
  const double aRadiusSquared = aCenterVec.SquareMagnitude() - aSolution(4);
  if (!IsFiniteValue(aRadiusSquared) || aRadiusSquared <= Precision::SquareConfusion())
  {
    return false;
  }

  theCenter = gp_Pnt(aCx, aCy, aCz);
  theRadius = std::sqrt(aRadiusSquared);
  return IsFiniteValue(theRadius);
}

bool fitComponentCylinder(const OcctL::Mesh::TriangleSoupBuffers&      theSoup,
                          const OcctL::Mesh::TriangleAnalysisBuffers&  theAnalysis,
                          const OcctL::Mesh::TriangleComponentBuffers& theComponents,
                          const uint32_t                               theComponent,
                          gp_Pnt&                                      theAxisOrigin,
                          gp_Dir&                                      theAxisDirection,
                          double&                                      theRadius,
                          double&                                      theHeightMin,
                          double&                                      theHeightMax,
                          double&                                      theMaxDistance)
{
  if (theComponents.myComponentSizes[theComponent] < 4u)
  {
    return false;
  }

  math_Matrix aCovariance(1, 3, 1, 3, 0.0);
  uint32_t    aNormalCount = 0u;
  for (uint32_t aTri = 0u; aTri < static_cast<uint32_t>(theComponents.myTriangleCount); ++aTri)
  {
    if (theComponents.myComponentIds[aTri] != theComponent)
    {
      continue;
    }

    gp_Vec aNormal(theAnalysis.myNormals[aTri * 3u],
                   theAnalysis.myNormals[aTri * 3u + 1u],
                   theAnalysis.myNormals[aTri * 3u + 2u]);
    if (aNormal.SquareMagnitude() <= Precision::SquareConfusion())
    {
      continue;
    }
    aNormal.Normalize();

    aCovariance(1, 1) += aNormal.X() * aNormal.X();
    aCovariance(1, 2) += aNormal.X() * aNormal.Y();
    aCovariance(1, 3) += aNormal.X() * aNormal.Z();
    aCovariance(2, 1) += aNormal.Y() * aNormal.X();
    aCovariance(2, 2) += aNormal.Y() * aNormal.Y();
    aCovariance(2, 3) += aNormal.Y() * aNormal.Z();
    aCovariance(3, 1) += aNormal.Z() * aNormal.X();
    aCovariance(3, 2) += aNormal.Z() * aNormal.Y();
    aCovariance(3, 3) += aNormal.Z() * aNormal.Z();
    ++aNormalCount;
  }
  if (aNormalCount < 4u)
  {
    return false;
  }

  math_Jacobi anEigen(aCovariance);
  if (!anEigen.IsDone())
  {
    return false;
  }

  int    aSmallestIndex = 1;
  double aSmallestValue = anEigen.Value(1);
  for (int anIdx = 2; anIdx <= 3; ++anIdx)
  {
    if (anEigen.Value(anIdx) < aSmallestValue)
    {
      aSmallestValue = anEigen.Value(anIdx);
      aSmallestIndex = anIdx;
    }
  }

  math_Vector anAxisVector(1, 3, 0.0);
  anEigen.Vector(aSmallestIndex, anAxisVector);
  gp_Vec anAxis(anAxisVector(1), anAxisVector(2), anAxisVector(3));
  if (anAxis.SquareMagnitude() <= Precision::SquareConfusion())
  {
    return false;
  }
  anAxis.Normalize();
  theAxisDirection = gp_Dir(anAxis);

  math_Matrix aPointMatrix(1, static_cast<int>(aNormalCount), 1, 3, 0.0);
  math_Vector aRhs(1, static_cast<int>(aNormalCount), 0.0);
  int         aRow = 1;
  for (uint32_t aTri = 0u; aTri < static_cast<uint32_t>(theComponents.myTriangleCount); ++aTri)
  {
    if (theComponents.myComponentIds[aTri] != theComponent)
    {
      continue;
    }

    gp_Vec aNormal(theAnalysis.myNormals[aTri * 3u],
                   theAnalysis.myNormals[aTri * 3u + 1u],
                   theAnalysis.myNormals[aTri * 3u + 2u]);
    if (aNormal.SquareMagnitude() <= Precision::SquareConfusion())
    {
      continue;
    }
    aNormal.Normalize();

    const uint32_t anA = theSoup.myTriangles[aTri * 3u];
    const uint32_t aB  = theSoup.myTriangles[aTri * 3u + 1u];
    const uint32_t aC  = theSoup.myTriangles[aTri * 3u + 2u];
    const gp_Pnt   aP0 = gp_Pnt(theSoup.myNodes[static_cast<size_t>(anA) * 3u],
                                theSoup.myNodes[static_cast<size_t>(anA) * 3u + 1u],
                                theSoup.myNodes[static_cast<size_t>(anA) * 3u + 2u]);
    const gp_Pnt   aP1 = gp_Pnt(theSoup.myNodes[static_cast<size_t>(aB) * 3u],
                                theSoup.myNodes[static_cast<size_t>(aB) * 3u + 1u],
                                theSoup.myNodes[static_cast<size_t>(aB) * 3u + 2u]);
    const gp_Pnt   aP2 = gp_Pnt(theSoup.myNodes[static_cast<size_t>(aC) * 3u],
                                theSoup.myNodes[static_cast<size_t>(aC) * 3u + 1u],
                                theSoup.myNodes[static_cast<size_t>(aC) * 3u + 2u]);
    const gp_Pnt   aCentroid((aP0.X() + aP1.X() + aP2.X()) / 3.0,
                             (aP0.Y() + aP1.Y() + aP2.Y()) / 3.0,
                             (aP0.Z() + aP1.Z() + aP2.Z()) / 3.0);

    aPointMatrix(aRow, 1) = aNormal.X();
    aPointMatrix(aRow, 2) = aNormal.Y();
    aPointMatrix(aRow, 3) = aNormal.Z();
    aRhs(aRow) =
      aNormal.X() * aCentroid.X() + aNormal.Y() * aCentroid.Y() + aNormal.Z() * aCentroid.Z();
    ++aRow;
  }

  math_SVD aSolver(aPointMatrix);
  if (!aSolver.IsDone())
  {
    return false;
  }

  math_Vector anOriginSolution(1, 3, 0.0);
  aSolver.Solve(aRhs, anOriginSolution);
  theAxisOrigin = gp_Pnt(anOriginSolution(1), anOriginSolution(2), anOriginSolution(3));

  NCollection_LinearVector<uint32_t> aNodes;
  appendUniqueComponentNodes(theSoup, theComponents, theComponent, aNodes);
  if (aNodes.Size() < 4u)
  {
    return false;
  }

  const gp_Lin anAxisLine(theAxisOrigin, theAxisDirection);
  double       aRadiusSum = 0.0;
  for (size_t aNodeIndex = 0; aNodeIndex < aNodes.Size(); ++aNodeIndex)
  {
    const uint32_t aNode = aNodes.Value(aNodeIndex);
    const size_t   aBase = static_cast<size_t>(aNode) * 3u;
    const gp_Pnt   aPoint(theSoup.myNodes[aBase],
                          theSoup.myNodes[aBase + 1u],
                          theSoup.myNodes[aBase + 2u]);
    aRadiusSum += anAxisLine.Distance(aPoint);
  }
  theRadius = aRadiusSum / static_cast<double>(aNodes.Size());
  if (!IsFiniteValue(theRadius) || theRadius <= Precision::Confusion())
  {
    return false;
  }

  theHeightMin   = std::numeric_limits<double>::infinity();
  theHeightMax   = -std::numeric_limits<double>::infinity();
  theMaxDistance = 0.0;
  for (size_t aNodeIndex = 0; aNodeIndex < aNodes.Size(); ++aNodeIndex)
  {
    const uint32_t aNode = aNodes.Value(aNodeIndex);
    const size_t   aBase = static_cast<size_t>(aNode) * 3u;
    const gp_Pnt   aPoint(theSoup.myNodes[aBase],
                          theSoup.myNodes[aBase + 1u],
                          theSoup.myNodes[aBase + 2u]);
    const double   aDistance = std::abs(anAxisLine.Distance(aPoint) - theRadius);
    theMaxDistance           = std::max(theMaxDistance, aDistance);
    const double aProjection = gp_Vec(theAxisOrigin, aPoint).Dot(gp_Vec(theAxisDirection));
    theHeightMin             = std::min(theHeightMin, aProjection);
    theHeightMax             = std::max(theHeightMax, aProjection);
  }

  return IsFiniteValue(theHeightMin) && IsFiniteValue(theHeightMax);
}

void materialisePlaneComponents(const OcctL::Mesh::TriangleSoupBuffers&               theSoup,
                                const OcctL::Mesh::TriangleComponentBuffers&          theSource,
                                const occtl_mesh_triangle_plane_components_options_t& theOptions,
                                OcctL::Mesh::TriangleComponentBuffers&                theOut)
{
  theOut.myPlaneComponents.Clear();

  NCollection_Array1<double> aMaxDistance(theSource.myComponentCount);
  NCollection_Array1<int>    anAccepted(theSource.myComponentCount);
  for (size_t aComponent = 0; aComponent < theSource.myComponentCount; ++aComponent)
  {
    aMaxDistance.ChangeAt(aComponent) = 0.0;
    anAccepted.ChangeAt(aComponent)   = 0;
  }

  for (uint32_t aComponent = 0; aComponent < theSource.myComponentCount; ++aComponent)
  {
    const occtl_mesh_triangle_component_summary_t& aSummary = theSource.mySummaries[aComponent];
    if (aSummary.triangle_count < theOptions.min_triangle_count
        || aSummary.area < theOptions.min_area)
    {
      continue;
    }

    gp_Vec aNormal(aSummary.normal.x, aSummary.normal.y, aSummary.normal.z);
    if (aNormal.SquareMagnitude() <= Precision::SquareConfusion())
    {
      continue;
    }

    const gp_Pln aPlane(gp_Pnt(aSummary.centroid.x, aSummary.centroid.y, aSummary.centroid.z),
                        gp_Dir(aNormal));
    bool         anIsPlane = true;
    for (uint32_t aTri = 0; aTri < static_cast<uint32_t>(theSource.myTriangleCount); ++aTri)
    {
      if (theSource.myComponentIds[aTri] != aComponent)
      {
        continue;
      }

      for (uint32_t aLocal = 0; aLocal < 3u; ++aLocal)
      {
        const uint32_t aNode = theSoup.myTriangles[aTri * 3u + aLocal];
        const size_t   aBase = static_cast<size_t>(aNode) * 3u;
        const gp_Pnt   aPoint(theSoup.myNodes[aBase],
                              theSoup.myNodes[aBase + 1u],
                              theSoup.myNodes[aBase + 2u]);
        const double   aDistance          = aPlane.Distance(aPoint);
        aMaxDistance.ChangeAt(aComponent) = std::max(aMaxDistance.At(aComponent), aDistance);
        if (aDistance > theOptions.max_distance)
        {
          anIsPlane = false;
          break;
        }
      }
      if (!anIsPlane)
      {
        break;
      }
    }

    if (anIsPlane)
    {
      anAccepted.ChangeAt(aComponent) = 1;
    }
  }

  for (uint32_t aComponent = 0; aComponent < theSource.myComponentCount; ++aComponent)
  {
    if (anAccepted.At(aComponent) == 0)
    {
      continue;
    }
    const occtl_mesh_triangle_component_summary_t& aSummary = theSource.mySummaries[aComponent];
    occtl_mesh_triangle_plane_component_t          aPlaneComponent{};
    aPlaneComponent.component_id   = aSummary.component_id;
    aPlaneComponent.triangle_count = aSummary.triangle_count;
    aPlaneComponent.area           = aSummary.area;
    aPlaneComponent.origin         = aSummary.centroid;
    aPlaneComponent.normal         = aSummary.normal;
    aPlaneComponent.bounds         = aSummary.bounds;
    aPlaneComponent.max_distance   = aMaxDistance.At(aComponent);
    theOut.myPlaneComponents.Append(aPlaneComponent);
  }
}

void materialiseSphereComponents(const OcctL::Mesh::TriangleSoupBuffers&                theSoup,
                                 const OcctL::Mesh::TriangleComponentBuffers&           theSource,
                                 const occtl_mesh_triangle_sphere_components_options_t& theOptions,
                                 OcctL::Mesh::TriangleComponentBuffers&                 theOut)
{
  theOut.mySphereComponents.Clear();

  NCollection_LinearVector<uint32_t> aNodes;
  for (uint32_t aComponent = 0; aComponent < theSource.myComponentCount; ++aComponent)
  {
    const occtl_mesh_triangle_component_summary_t& aSummary = theSource.mySummaries[aComponent];
    if (aSummary.triangle_count < theOptions.min_triangle_count
        || aSummary.area < theOptions.min_area)
    {
      continue;
    }

    appendUniqueComponentNodes(theSoup, theSource, aComponent, aNodes);
    gp_Pnt aCenter;
    double aRadius = 0.0;
    if (!fitComponentSphere(theSoup, aNodes, aCenter, aRadius) || aRadius < theOptions.min_radius)
    {
      continue;
    }

    double aMaxDistance = 0.0;
    bool   anIsSphere   = true;
    for (size_t aNodeIndex = 0; aNodeIndex < aNodes.Size(); ++aNodeIndex)
    {
      const uint32_t aNode = aNodes.Value(aNodeIndex);
      const size_t   aBase = static_cast<size_t>(aNode) * 3u;
      const gp_Pnt   aPoint(theSoup.myNodes[aBase],
                            theSoup.myNodes[aBase + 1u],
                            theSoup.myNodes[aBase + 2u]);
      const double   aDistance = std::abs(aPoint.Distance(aCenter) - aRadius);
      aMaxDistance             = std::max(aMaxDistance, aDistance);
      if (aDistance > theOptions.max_distance)
      {
        anIsSphere = false;
        break;
      }
    }
    if (!anIsSphere)
    {
      continue;
    }

    occtl_mesh_triangle_sphere_component_t aSphereComponent{};
    aSphereComponent.component_id   = aSummary.component_id;
    aSphereComponent.triangle_count = aSummary.triangle_count;
    aSphereComponent.area           = aSummary.area;
    aSphereComponent.center         = {aCenter.X(), aCenter.Y(), aCenter.Z()};
    aSphereComponent.radius         = aRadius;
    aSphereComponent.bounds         = aSummary.bounds;
    aSphereComponent.max_distance   = aMaxDistance;
    theOut.mySphereComponents.Append(aSphereComponent);
  }
}

void materialiseCylinderComponents(
  const OcctL::Mesh::TriangleSoupBuffers&                  theSoup,
  const OcctL::Mesh::TriangleAnalysisBuffers&              theAnalysis,
  const OcctL::Mesh::TriangleComponentBuffers&             theSource,
  const occtl_mesh_triangle_cylinder_components_options_t& theOptions,
  OcctL::Mesh::TriangleComponentBuffers&                   theOut)
{
  theOut.myCylinderComponents.Clear();

  for (uint32_t aComponent = 0; aComponent < theSource.myComponentCount; ++aComponent)
  {
    const occtl_mesh_triangle_component_summary_t& aSummary = theSource.mySummaries[aComponent];
    if (aSummary.triangle_count < theOptions.min_triangle_count
        || aSummary.area < theOptions.min_area)
    {
      continue;
    }

    gp_Pnt aAxisOrigin;
    gp_Dir anAxisDirection(0.0, 0.0, 1.0);
    double aRadius      = 0.0;
    double aHeightMin   = 0.0;
    double aHeightMax   = 0.0;
    double aMaxDistance = 0.0;
    if (!fitComponentCylinder(theSoup,
                              theAnalysis,
                              theSource,
                              aComponent,
                              aAxisOrigin,
                              anAxisDirection,
                              aRadius,
                              aHeightMin,
                              aHeightMax,
                              aMaxDistance)
        || aRadius < theOptions.min_radius || aMaxDistance > theOptions.max_distance)
    {
      continue;
    }

    occtl_mesh_triangle_cylinder_component_t aCylinderComponent{};
    aCylinderComponent.component_id   = aSummary.component_id;
    aCylinderComponent.triangle_count = aSummary.triangle_count;
    aCylinderComponent.area           = aSummary.area;
    aCylinderComponent.axis_origin    = {aAxisOrigin.X(), aAxisOrigin.Y(), aAxisOrigin.Z()};
    aCylinderComponent.axis_direction = {anAxisDirection.X(),
                                         anAxisDirection.Y(),
                                         anAxisDirection.Z()};
    aCylinderComponent.radius         = aRadius;
    aCylinderComponent.height_min     = aHeightMin;
    aCylinderComponent.height_max     = aHeightMax;
    aCylinderComponent.bounds         = aSummary.bounds;
    aCylinderComponent.max_distance   = aMaxDistance;
    theOut.myCylinderComponents.Append(aCylinderComponent);
  }
}

void fillTrianglePlaneComponentsView(const OcctL::Mesh::TriangleComponentBuffers&       theSlot,
                                     occtl_mesh_triangle_plane_components_view_t* const theOutView)
{
  theOutView->components =
    theSlot.myPlaneComponents.IsEmpty() ? nullptr : &theSlot.myPlaneComponents[0];
  theOutView->component_count = theSlot.myPlaneComponents.Size();
  theOutView->triangle_count  = theSlot.myTriangleCount;
  theOutView->root            = theSlot.myRoot;
}

void fillTriangleSphereComponentsView(
  const OcctL::Mesh::TriangleComponentBuffers&        theSlot,
  occtl_mesh_triangle_sphere_components_view_t* const theOutView)
{
  theOutView->components =
    theSlot.mySphereComponents.IsEmpty() ? nullptr : &theSlot.mySphereComponents[0];
  theOutView->component_count = theSlot.mySphereComponents.Size();
  theOutView->triangle_count  = theSlot.myTriangleCount;
  theOutView->root            = theSlot.myRoot;
}

void fillTriangleCylinderComponentsView(
  const OcctL::Mesh::TriangleComponentBuffers&          theSlot,
  occtl_mesh_triangle_cylinder_components_view_t* const theOutView)
{
  theOutView->components =
    theSlot.myCylinderComponents.IsEmpty() ? nullptr : &theSlot.myCylinderComponents[0];
  theOutView->component_count = theSlot.myCylinderComponents.Size();
  theOutView->triangle_count  = theSlot.myTriangleCount;
  theOutView->root            = theSlot.myRoot;
}

const occtl_mesh_triangle_plane_component_t* findPlaneComponent(
  const OcctL::Mesh::TriangleComponentBuffers& theSlot,
  const uint32_t                               theComponentId)
{
  for (size_t anIdx = 0; anIdx < theSlot.myPlaneComponents.Size(); ++anIdx)
  {
    if (theSlot.myPlaneComponents[anIdx].component_id == theComponentId)
    {
      return &theSlot.myPlaneComponents[anIdx];
    }
  }
  return nullptr;
}

const occtl_mesh_triangle_sphere_component_t* findSphereComponent(
  const OcctL::Mesh::TriangleComponentBuffers& theSlot,
  const uint32_t                               theComponentId)
{
  for (size_t anIdx = 0; anIdx < theSlot.mySphereComponents.Size(); ++anIdx)
  {
    if (theSlot.mySphereComponents[anIdx].component_id == theComponentId)
    {
      return &theSlot.mySphereComponents[anIdx];
    }
  }
  return nullptr;
}

const occtl_mesh_triangle_cylinder_component_t* findCylinderComponent(
  const OcctL::Mesh::TriangleComponentBuffers& theSlot,
  const uint32_t                               theComponentId)
{
  for (size_t anIdx = 0; anIdx < theSlot.myCylinderComponents.Size(); ++anIdx)
  {
    if (theSlot.myCylinderComponents[anIdx].component_id == theComponentId)
    {
      return &theSlot.myCylinderComponents[anIdx];
    }
  }
  return nullptr;
}

double signedPolylineAreaOnPlane(const OcctL::Mesh::TriangleComponentBuffers&    theSlot,
                                 const occtl_mesh_component_boundary_polyline_t& thePolyline,
                                 const gp_Pln&                                   thePlane)
{
  if (thePolyline.point_count < 3u)
  {
    return 0.0;
  }

  const gp_Ax3   anAxes   = thePlane.Position();
  const gp_Pnt   anOrigin = thePlane.Location();
  const gp_Vec   anXDir(anAxes.XDirection());
  const gp_Vec   aYDir(anAxes.YDirection());
  double         anArea = 0.0;
  const uint32_t anEnd  = thePolyline.first_point + thePolyline.point_count;
  for (uint32_t anIdx = thePolyline.first_point; anIdx < anEnd; ++anIdx)
  {
    const uint32_t        aNextIdx = (anIdx + 1u == anEnd) ? thePolyline.first_point : anIdx + 1u;
    const occtl_point3_t& aP0      = theSlot.myBoundaryPolylinePoints[anIdx];
    const occtl_point3_t& aP1      = theSlot.myBoundaryPolylinePoints[aNextIdx];
    const gp_Vec          aV0(anOrigin, gp_Pnt(aP0.x, aP0.y, aP0.z));
    const gp_Vec          aV1(anOrigin, gp_Pnt(aP1.x, aP1.y, aP1.z));
    const double          aU0  = aV0.Dot(anXDir);
    const double          aV0p = aV0.Dot(aYDir);
    const double          aU1  = aV1.Dot(anXDir);
    const double          aV1p = aV1.Dot(aYDir);
    anArea += aU0 * aV1p - aU1 * aV0p;
  }
  return 0.5 * anArea;
}

bool makeWireFromPolyline(const OcctL::Mesh::TriangleComponentBuffers&    theSlot,
                          const occtl_mesh_component_boundary_polyline_t& thePolyline,
                          TopoDS_Wire&                                    theOutWire)
{
  if (thePolyline.point_count < 3u)
  {
    return false;
  }

  BRepBuilderAPI_MakePolygon aPolygon;
  const uint32_t             anEnd = thePolyline.first_point + thePolyline.point_count;
  for (uint32_t anIdx = thePolyline.first_point; anIdx < anEnd; ++anIdx)
  {
    if (thePolyline.is_closed != 0 && anIdx + 1u == anEnd)
    {
      continue;
    }
    const occtl_point3_t& aPoint = theSlot.myBoundaryPolylinePoints[anIdx];
    aPolygon.Add(gp_Pnt(aPoint.x, aPoint.y, aPoint.z));
  }
  if (thePolyline.is_closed != 0)
  {
    aPolygon.Close();
  }
  if (!aPolygon.IsDone())
  {
    return false;
  }

  theOutWire = aPolygon.Wire();
  return !theOutWire.IsNull();
}

occtl_status_t addFaceToGraph(occtl_graph_t* const theGraph,
                              const TopoDS_Face&   theFace,
                              occtl_node_id_t*     theOutFace)
{
  BRepGraph::ShapesView::Options aBuildOptions;
  aBuildOptions.CreateAutoProduct = false;
  const BRepGraph::ShapesView::Result aBuildResult =
    theGraph->graph.Shapes().Add(theFace, aBuildOptions);
  if (!aBuildResult.Ok || !aBuildResult.TopologyRoot.IsValid()
      || aBuildResult.TopologyRoot.NodeKind != BRepGraph_NodeId::Kind::Face)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      "rebuilt plane component could not be ingested into BRepGraph as a Face");
    return OCCTL_TOPOLOGY_INVALID;
  }
  *theOutFace = OcctL::Topo::PackNodeId(aBuildResult.TopologyRoot);
  return OCCTL_OK;
}

occtl_status_t addSolidToGraph(occtl_graph_t* const theGraph,
                               const TopoDS_Shape&  theShape,
                               const char* const    theFailureMessage,
                               occtl_node_id_t*     theOutSolid)
{
  BRepGraph::ShapesView::Options aBuildOptions;
  aBuildOptions.CreateAutoProduct = false;
  const BRepGraph::ShapesView::Result aBuildResult =
    theGraph->graph.Shapes().Add(theShape, aBuildOptions);
  if (!aBuildResult.Ok || !aBuildResult.TopologyRoot.IsValid()
      || aBuildResult.TopologyRoot.NodeKind != BRepGraph_NodeId::Kind::Solid)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_TOPOLOGY_INVALID, theFailureMessage);
    return OCCTL_TOPOLOGY_INVALID;
  }
  *theOutSolid = OcctL::Topo::PackNodeId(aBuildResult.TopologyRoot);
  return OCCTL_OK;
}

occtl_status_t makeSphereComponentShape(
  const occtl_mesh_triangle_sphere_component_t& theSphereComponent,
  TopoDS_Shape&                                 theOutShape)
{
  BRepPrimAPI_MakeSphere aMaker(
    gp_Ax2(
      gp_Pnt(theSphereComponent.center.x, theSphereComponent.center.y, theSphereComponent.center.z),
      gp_Dir(0.0, 0.0, 1.0)),
    theSphereComponent.radius);
  aMaker.Build();
  if (!aMaker.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "OCCT could not build analytic sphere");
    return OCCTL_GEOMETRY_INVALID;
  }
  theOutShape = aMaker.Shape();
  return OCCTL_OK;
}

occtl_status_t makeCylinderComponentShape(
  const occtl_mesh_triangle_cylinder_component_t& theCylinderComponent,
  TopoDS_Shape&                                   theOutShape)
{
  const double aHeight = theCylinderComponent.height_max - theCylinderComponent.height_min;
  if (!IsFiniteValue(aHeight) || aHeight <= Precision::Confusion())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "detected cylinder height is degenerate");
    return OCCTL_GEOMETRY_INVALID;
  }

  const gp_Vec             anAxis(theCylinderComponent.axis_direction.x,
                                  theCylinderComponent.axis_direction.y,
                                  theCylinderComponent.axis_direction.z);
  const gp_Pnt             anOrigin(theCylinderComponent.axis_origin.x,
                                    theCylinderComponent.axis_origin.y,
                                    theCylinderComponent.axis_origin.z);
  const gp_Pnt             aBase = anOrigin.Translated(anAxis * theCylinderComponent.height_min);
  BRepPrimAPI_MakeCylinder aMaker(gp_Ax2(aBase, gp_Dir(anAxis)),
                                  theCylinderComponent.radius,
                                  aHeight);
  aMaker.Build();
  if (!aMaker.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "OCCT could not build analytic cylinder");
    return OCCTL_GEOMETRY_INVALID;
  }
  theOutShape = aMaker.Shape();
  return OCCTL_OK;
}

occtl_status_t makePlaneComponentTopoFace(
  const OcctL::Mesh::TriangleSoupBuffers&      theSoup,
  const OcctL::Mesh::TriangleAnalysisBuffers&  theAnalysis,
  OcctL::Mesh::TriangleComponentBuffers&       theComponents,
  const uint32_t                               theComponentId,
  const occtl_mesh_triangle_plane_component_t& thePlaneComponent,
  TopoDS_Face&                                 theOutFace)
{
  materialiseSelectedComponentBoundary(theSoup, theAnalysis, theComponents, theComponentId);
  materialiseSelectedComponentBoundaryChains(theComponents);
  materialiseSelectedComponentBoundaryPolylines(theSoup, theComponents);
  if (theComponents.myBoundaryPolylines.IsEmpty())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "plane component has no boundary polylines");
    return OCCTL_GEOMETRY_INVALID;
  }

  const gp_Pln aPlane(
    gp_Pnt(thePlaneComponent.origin.x, thePlaneComponent.origin.y, thePlaneComponent.origin.z),
    gp_Dir(
      gp_Vec(thePlaneComponent.normal.x, thePlaneComponent.normal.y, thePlaneComponent.normal.z)));

  uint32_t anOuterPolyline = std::numeric_limits<uint32_t>::max();
  double   anOuterArea     = -1.0;
  for (uint32_t aPolylineIdx = 0u;
       aPolylineIdx < static_cast<uint32_t>(theComponents.myBoundaryPolylines.Size());
       ++aPolylineIdx)
  {
    const occtl_mesh_component_boundary_polyline_t& aPolyline =
      theComponents.myBoundaryPolylines[aPolylineIdx];
    const double anArea = std::abs(signedPolylineAreaOnPlane(theComponents, aPolyline, aPlane));
    if (anArea > anOuterArea)
    {
      anOuterArea     = anArea;
      anOuterPolyline = aPolylineIdx;
    }
  }
  if (anOuterPolyline == std::numeric_limits<uint32_t>::max()
      || anOuterArea <= Precision::Confusion())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "plane component boundary area is degenerate");
    return OCCTL_GEOMETRY_INVALID;
  }

  TopoDS_Wire anOuterWire;
  if (!makeWireFromPolyline(theComponents,
                            theComponents.myBoundaryPolylines[anOuterPolyline],
                            anOuterWire))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "OCCT could not build outer boundary wire");
    return OCCTL_GEOMETRY_INVALID;
  }

  BRepBuilderAPI_MakeFace aFaceMaker(aPlane, anOuterWire, true);
  if (!aFaceMaker.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "OCCT could not build planar Face from outer boundary");
    return OCCTL_GEOMETRY_INVALID;
  }

  for (uint32_t aPolylineIdx = 0u;
       aPolylineIdx < static_cast<uint32_t>(theComponents.myBoundaryPolylines.Size());
       ++aPolylineIdx)
  {
    if (aPolylineIdx == anOuterPolyline)
    {
      continue;
    }
    TopoDS_Wire anInnerWire;
    if (!makeWireFromPolyline(theComponents,
                              theComponents.myBoundaryPolylines[aPolylineIdx],
                              anInnerWire))
    {
      continue;
    }
    aFaceMaker.Add(anInnerWire);
  }
  if (!aFaceMaker.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "OCCT could not add inner boundary wires");
    return OCCTL_GEOMETRY_INVALID;
  }

  theOutFace = aFaceMaker.Face();
  return OCCTL_OK;
}

} // unnamed namespace

extern "C"
{

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_face_triangulation(const occtl_graph_t* const        graph,
                                const occtl_node_id_t             face,
                                occtl_triangulation_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aErr =
          resolveTyped(graph, face, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aErr;
    }

    const BRepGraph::MeshView::CacheView::FaceOps& aFaceOps = graph->graph.Mesh().Cache().Faces();

    const int aActiveIdx = aFaceOps.ActiveTriangulationIndex(aFaceId);
    if (aActiveIdx < 0)
    {
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<Poly_Triangulation>& aTri =
      aFaceOps.Triangulation(aFaceId, static_cast<uint32_t>(aActiveIdx));
    if (aTri.IsNull())
    {
      return OCCTL_NOT_FOUND;
    }

    const occtl_uid_t            aFaceUid = uidFor(graph, aFaceId);
    const BRepGraph_VersionStamp aStamp   = graph->graph.UIDs().StampOf(aFaceId);

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::FaceMeshBuffers& aSlot =
      aCache.FindOrCreateFaceSlot(aFaceUid.bits, static_cast<uint32_t>(aActiveIdx));
    if (!aSlot.myNodes.IsEmpty() && aSlot.myStamp != aStamp)
    {
      resetFaceSlot(aSlot);
    }
    if (!materialiseFace(aTri, aFaceUid, aSlot))
    {
      return OCCTL_NOT_FOUND;
    }
    aSlot.myStamp = aStamp;

    fillFaceView(aSlot, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_face_triangulation_count(const occtl_graph_t* const graph,
                                      const occtl_node_id_t      face,
                                      uint32_t* const            out_count)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_count == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aErr =
          resolveTyped(graph, face, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aErr;
    }

    *out_count = graph->graph.Mesh().Cache().Faces().NbTriangulations(aFaceId);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_face_triangulation_indexed(const occtl_graph_t* const        graph,
                                        const occtl_node_id_t             face,
                                        const uint32_t                    index,
                                        occtl_triangulation_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_FaceId aFaceId;
    if (const occtl_status_t aErr =
          resolveTyped(graph, face, BRepGraph_NodeId::Kind::Face, aFaceId))
    {
      return aErr;
    }

    const BRepGraph::MeshView::CacheView::FaceOps& aFaceOps = graph->graph.Mesh().Cache().Faces();
    if (index >= aFaceOps.NbTriangulations(aFaceId))
    {
      return OCCTL_OUT_OF_RANGE;
    }

    const occ::handle<Poly_Triangulation>& aTri = aFaceOps.Triangulation(aFaceId, index);
    if (aTri.IsNull())
    {
      return OCCTL_NOT_FOUND;
    }

    const occtl_uid_t            aFaceUid = uidFor(graph, aFaceId);
    const BRepGraph_VersionStamp aStamp   = graph->graph.UIDs().StampOf(aFaceId);

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::FaceMeshBuffers& aSlot = aCache.FindOrCreateFaceSlot(aFaceUid.bits, index);
    if (!aSlot.myNodes.IsEmpty() && aSlot.myStamp != aStamp)
    {
      resetFaceSlot(aSlot);
    }
    if (!materialiseFace(aTri, aFaceUid, aSlot))
    {
      return OCCTL_NOT_FOUND;
    }
    aSlot.myStamp = aStamp;

    fillFaceView(aSlot, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_edge_polygon3d(const occtl_graph_t* const    graph,
                            const occtl_node_id_t         edge,
                            occtl_polygon3d_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_EdgeId aEdgeId;
    if (const occtl_status_t aErr =
          resolveTyped(graph, edge, BRepGraph_NodeId::Kind::Edge, aEdgeId))
    {
      return aErr;
    }

    const BRepGraph::MeshView::CacheView::EdgeOps& aEdgeOps = graph->graph.Mesh().Cache().Edges();
    if (!aEdgeOps.Has(aEdgeId))
    {
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<Poly_Polygon3D>& aPoly = aEdgeOps.Polygon3D(aEdgeId);
    if (aPoly.IsNull())
    {
      return OCCTL_NOT_FOUND;
    }

    const occtl_uid_t            anEdgeUid = uidFor(graph, aEdgeId);
    const BRepGraph_VersionStamp aStamp    = graph->graph.UIDs().StampOf(aEdgeId);

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::EdgeMeshBuffers& aSlot = aCache.FindOrCreateEdgeSlot(anEdgeUid.bits);
    if (!aSlot.myNodes.IsEmpty() && aSlot.myStamp != aStamp)
    {
      resetEdgeSlot(aSlot);
    }
    if (aSlot.myNodes.IsEmpty())
    {
      const NCollection_Array1<gp_Pnt>& aNodes   = aPoly->Nodes();
      const size_t                      aNbNodes = aNodes.Size();
      if (aNbNodes <= 0)
      {
        return OCCTL_NOT_FOUND;
      }

      for (size_t anIdx = 1u; anIdx <= aNbNodes; ++anIdx)
      {
        const gp_Pnt& aPoint = aNodes.Value(static_cast<int>(anIdx));
        aSlot.myNodes.Append(aPoint.X());
        aSlot.myNodes.Append(aPoint.Y());
        aSlot.myNodes.Append(aPoint.Z());
      }
      if (aPoly->HasParameters())
      {
        const NCollection_Array1<double>& aParameters = aPoly->Parameters();
        for (size_t anIdx = 1u; anIdx <= aNbNodes; ++anIdx)
        {
          aSlot.myParameters.Append(aParameters.Value(static_cast<int>(anIdx)));
        }
      }
      aSlot.myDeflection = aPoly->Deflection();
      aSlot.mySourceUid  = anEdgeUid;
      aSlot.myStamp      = aStamp;
    }

    out_view->nodes      = aSlot.myNodes.IsEmpty() ? nullptr : &aSlot.myNodes[0];
    out_view->node_count = aSlot.myNodes.Size() / 3u;
    out_view->parameters = aSlot.myParameters.IsEmpty() ? nullptr : &aSlot.myParameters[0];
    out_view->deflection = aSlot.myDeflection;
    out_view->source_uid = aSlot.mySourceUid;
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_coedge_polygon_on_tri(const occtl_graph_t* const         graph,
                                   const occtl_node_id_t              coedge,
                                   occtl_polygon_on_tri_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_CoEdgeId aCoEdgeId;
    if (const occtl_status_t aErr =
          resolveTyped(graph, coedge, BRepGraph_NodeId::Kind::CoEdge, aCoEdgeId))
    {
      return aErr;
    }

    const BRepGraph::MeshView::EffectiveView::CoEdgeOps& aEffective =
      graph->graph.Mesh().Effective().CoEdges();
    if (!aEffective.HasPolygonOnTriangulation(aCoEdgeId))
    {
      return OCCTL_NOT_FOUND;
    }

    const occ::handle<Poly_PolygonOnTriangulation>& aPoly =
      aEffective.PolygonOnTriangulation(aCoEdgeId);
    if (aPoly.IsNull())
    {
      return OCCTL_NOT_FOUND;
    }

    const occtl_uid_t            aCoEdgeUid = uidFor(graph, aCoEdgeId);
    const BRepGraph_VersionStamp aStamp     = graph->graph.UIDs().StampOf(aCoEdgeId);

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::CoEdgeMeshBuffers& aSlot = aCache.FindOrCreateCoEdgeSlot(aCoEdgeUid.bits);
    if (!aSlot.myNodeIndices.IsEmpty() && aSlot.myStamp != aStamp)
    {
      resetCoEdgeSlot(aSlot);
    }
    if (aSlot.myNodeIndices.IsEmpty())
    {
      const int aNb = aPoly->NbNodes();
      for (int i = 1; i <= aNb; ++i)
      {
        const int aIdx = aPoly->Node(i);
        aSlot.myNodeIndices.Append(static_cast<uint32_t>(aIdx - 1));
      }
      if (aPoly->HasParameters())
      {
        for (int i = 1; i <= aNb; ++i)
        {
          aSlot.myParameters.Append(aPoly->Parameter(i));
        }
      }
      aSlot.myDeflection = aPoly->Deflection();
      aSlot.mySourceUid  = aCoEdgeUid;
      aSlot.myStamp      = aStamp;
    }

    out_view->node_indices = aSlot.myNodeIndices.IsEmpty() ? nullptr : &aSlot.myNodeIndices[0];
    out_view->node_count   = aSlot.myNodeIndices.Size();
    out_view->parameters   = aSlot.myParameters.IsEmpty() ? nullptr : &aSlot.myParameters[0];
    out_view->deflection   = aSlot.myDeflection;
    out_view->source_uid   = aSlot.mySourceUid;
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_triangle_buffers(const occtl_graph_t* const                graph,
                              const occtl_node_id_t                     root,
                              occtl_mesh_triangle_buffers_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    OcctL::Mesh::MeshCache&           aCache = cacheFor(graph);
    std::lock_guard<std::mutex>       aLock(aCache.Mutex());
    OcctL::Mesh::TriangleSoupBuffers& aSlot = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, root, aSlot))
    {
      return aStatus;
    }

    fillTriangleSoupView(aSlot, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_triangle_analysis(const occtl_graph_t* const                 graph,
                               const occtl_node_id_t                      root,
                               occtl_mesh_triangle_analysis_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);
    fillTriangleAnalysisView(anAnalysis, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API void OCCTL_CALL occtl_mesh_triangle_components_options_init(
  occtl_mesh_triangle_components_options_t* const options)
{
  if (options != nullptr)
  {
    *options = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  }
}

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_triangle_components(const occtl_graph_t* const                            graph,
                                 const occtl_mesh_triangle_components_options_t* const options,
                                 occtl_mesh_triangle_components_view_t* const          out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = validateTriangleComponentsOptions(options))
    {
      return aStatus;
    }

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, *options, aComponents);
    fillTriangleComponentsView(aComponents, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_component_triangles(
  const occtl_graph_t* const                            graph,
  const occtl_mesh_triangle_components_options_t* const options,
  const uint32_t                                        component_id,
  occtl_mesh_triangle_component_triangles_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = validateTriangleComponentsOptions(options))
    {
      return aStatus;
    }

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, *options, aComponents);
    if (component_id >= aComponents.myComponentCount)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE, "component_id is out of range");
      return OCCTL_OUT_OF_RANGE;
    }

    materialiseSelectedComponentTriangles(aComponents, component_id);
    fillComponentTrianglesView(aComponents, component_id, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_component_boundary(
  const occtl_graph_t* const                            graph,
  const occtl_mesh_triangle_components_options_t* const options,
  const uint32_t                                        component_id,
  occtl_mesh_triangle_component_boundary_view_t* const  out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = validateTriangleComponentsOptions(options))
    {
      return aStatus;
    }

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, *options, aComponents);
    if (component_id >= aComponents.myComponentCount)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE, "component_id is out of range");
      return OCCTL_OUT_OF_RANGE;
    }

    materialiseSelectedComponentBoundary(aSoup, anAnalysis, aComponents, component_id);
    fillComponentBoundaryView(aComponents, component_id, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_component_boundary_chains(
  const occtl_graph_t* const                                  graph,
  const occtl_mesh_triangle_components_options_t* const       options,
  const uint32_t                                              component_id,
  occtl_mesh_triangle_component_boundary_chains_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = validateTriangleComponentsOptions(options))
    {
      return aStatus;
    }

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, *options, aComponents);
    if (component_id >= aComponents.myComponentCount)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE, "component_id is out of range");
      return OCCTL_OUT_OF_RANGE;
    }

    materialiseSelectedComponentBoundary(aSoup, anAnalysis, aComponents, component_id);
    materialiseSelectedComponentBoundaryChains(aComponents);
    fillComponentBoundaryChainsView(aComponents, component_id, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_component_boundary_polylines(
  const occtl_graph_t* const                            graph,
  const occtl_mesh_triangle_components_options_t* const options,
  const uint32_t                                        component_id,
  occtl_mesh_component_boundary_polylines_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = validateTriangleComponentsOptions(options))
    {
      return aStatus;
    }

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, *options, aComponents);
    if (component_id >= aComponents.myComponentCount)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE, "component_id is out of range");
      return OCCTL_OUT_OF_RANGE;
    }

    materialiseSelectedComponentBoundary(aSoup, anAnalysis, aComponents, component_id);
    materialiseSelectedComponentBoundaryChains(aComponents);
    materialiseSelectedComponentBoundaryPolylines(aSoup, aComponents);
    fillComponentBoundaryPolylinesView(aComponents, component_id, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_component_summaries(
  const occtl_graph_t* const                            graph,
  const occtl_mesh_triangle_components_options_t* const options,
  occtl_mesh_triangle_component_summaries_view_t* const out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = validateTriangleComponentsOptions(options))
    {
      return aStatus;
    }

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, *options, aComponents);
    materialiseTriangleComponentSummaries(aSoup, aComponents);
    fillTriangleComponentSummariesView(aComponents, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API void OCCTL_CALL occtl_mesh_triangle_plane_components_options_init(
  occtl_mesh_triangle_plane_components_options_t* const options)
{
  if (options != nullptr)
  {
    *options = OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;
  }
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_plane_components(
  const occtl_graph_t* const                                  graph,
  const occtl_mesh_triangle_plane_components_options_t* const options,
  occtl_mesh_triangle_plane_components_view_t* const          out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = validateTrianglePlaneComponentsOptions(options))
    {
      return aStatus;
    }

    occtl_mesh_triangle_components_options_t aComponentOptions =
      OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
    aComponentOptions.root                     = options->root;
    aComponentOptions.max_normal_angle         = options->max_normal_angle;
    aComponentOptions.include_opposite_normals = options->include_opposite_normals;

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, aComponentOptions, aComponents);
    materialiseTriangleComponentSummaries(aSoup, aComponents);
    materialisePlaneComponents(aSoup, aComponents, *options, aComponents);
    fillTrianglePlaneComponentsView(aComponents, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API void OCCTL_CALL occtl_mesh_triangle_sphere_components_options_init(
  occtl_mesh_triangle_sphere_components_options_t* const options)
{
  if (options != nullptr)
  {
    *options = OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  }
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_sphere_components(
  const occtl_graph_t* const                                   graph,
  const occtl_mesh_triangle_sphere_components_options_t* const options,
  occtl_mesh_triangle_sphere_components_view_t* const          out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = validateTriangleSphereComponentsOptions(options))
    {
      return aStatus;
    }

    occtl_mesh_triangle_components_options_t aComponentOptions =
      OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
    aComponentOptions.root                     = options->root;
    aComponentOptions.max_normal_angle         = options->max_normal_angle;
    aComponentOptions.include_opposite_normals = options->include_opposite_normals;

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, aComponentOptions, aComponents);
    materialiseTriangleComponentSummaries(aSoup, aComponents);
    materialiseSphereComponents(aSoup, aComponents, *options, aComponents);
    fillTriangleSphereComponentsView(aComponents, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API void OCCTL_CALL occtl_mesh_triangle_cylinder_components_options_init(
  occtl_mesh_triangle_cylinder_components_options_t* const options)
{
  if (options != nullptr)
  {
    *options = OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  }
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_triangle_cylinder_components(
  const occtl_graph_t* const                                     graph,
  const occtl_mesh_triangle_cylinder_components_options_t* const options,
  occtl_mesh_triangle_cylinder_components_view_t* const          out_view)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_view == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = validateTriangleCylinderComponentsOptions(options))
    {
      return aStatus;
    }

    occtl_mesh_triangle_components_options_t aComponentOptions =
      OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
    aComponentOptions.root                     = options->root;
    aComponentOptions.max_normal_angle         = options->max_normal_angle;
    aComponentOptions.include_opposite_normals = options->include_opposite_normals;

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, aComponentOptions, aComponents);
    materialiseTriangleComponentSummaries(aSoup, aComponents);
    materialiseCylinderComponents(aSoup, anAnalysis, aComponents, *options, aComponents);
    fillTriangleCylinderComponentsView(aComponents, out_view);
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_sphere_component_solid(
  occtl_graph_t* const                                         graph,
  const occtl_mesh_triangle_sphere_components_options_t* const options,
  const uint32_t                                               component_id,
  occtl_node_id_t* const                                       out_solid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_solid == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *out_solid = OCCTL_NODE_ID_INVALID;

    if (const occtl_status_t aStatus = validateTriangleSphereComponentsOptions(options))
    {
      return aStatus;
    }

    occtl_mesh_triangle_components_options_t aComponentOptions =
      OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
    aComponentOptions.root                     = options->root;
    aComponentOptions.max_normal_angle         = options->max_normal_angle;
    aComponentOptions.include_opposite_normals = options->include_opposite_normals;

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, aComponentOptions, aComponents);
    if (component_id >= aComponents.myComponentCount)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE, "component_id is out of range");
      return OCCTL_OUT_OF_RANGE;
    }

    materialiseTriangleComponentSummaries(aSoup, aComponents);
    materialiseSphereComponents(aSoup, aComponents, *options, aComponents);
    const occtl_mesh_triangle_sphere_component_t* const aSphereComponent =
      findSphereComponent(aComponents, component_id);
    if (aSphereComponent == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "component_id is not sphere-like");
      return OCCTL_NOT_FOUND;
    }

    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus = makeSphereComponentShape(*aSphereComponent, aShape))
    {
      return aStatus;
    }

    return addSolidToGraph(
      graph,
      aShape,
      "rebuilt sphere component could not be ingested into BRepGraph as a Solid",
      out_solid);
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_cylinder_component_solid(
  occtl_graph_t* const                                           graph,
  const occtl_mesh_triangle_cylinder_components_options_t* const options,
  const uint32_t                                                 component_id,
  occtl_node_id_t* const                                         out_solid)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_solid == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *out_solid = OCCTL_NODE_ID_INVALID;

    if (const occtl_status_t aStatus = validateTriangleCylinderComponentsOptions(options))
    {
      return aStatus;
    }

    occtl_mesh_triangle_components_options_t aComponentOptions =
      OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
    aComponentOptions.root                     = options->root;
    aComponentOptions.max_normal_angle         = options->max_normal_angle;
    aComponentOptions.include_opposite_normals = options->include_opposite_normals;

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, aComponentOptions, aComponents);
    if (component_id >= aComponents.myComponentCount)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE, "component_id is out of range");
      return OCCTL_OUT_OF_RANGE;
    }

    materialiseTriangleComponentSummaries(aSoup, aComponents);
    materialiseCylinderComponents(aSoup, anAnalysis, aComponents, *options, aComponents);
    const occtl_mesh_triangle_cylinder_component_t* const aCylinderComponent =
      findCylinderComponent(aComponents, component_id);
    if (aCylinderComponent == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "component_id is not cylinder-like");
      return OCCTL_NOT_FOUND;
    }

    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus = makeCylinderComponentShape(*aCylinderComponent, aShape))
    {
      return aStatus;
    }

    return addSolidToGraph(
      graph,
      aShape,
      "rebuilt cylinder component could not be ingested into BRepGraph as a Solid",
      out_solid);
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_sphere_component_solids(
  occtl_graph_t* const                                         graph,
  const occtl_mesh_triangle_sphere_components_options_t* const options,
  occtl_node_id_t* const                                       out_buf,
  const size_t                                                 cap,
  size_t* const                                                out_count)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_count == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *out_count = 0u;

    if (const occtl_status_t aStatus = validateTriangleSphereComponentsOptions(options))
    {
      return aStatus;
    }

    occtl_mesh_triangle_components_options_t aComponentOptions =
      OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
    aComponentOptions.root                     = options->root;
    aComponentOptions.max_normal_angle         = options->max_normal_angle;
    aComponentOptions.include_opposite_normals = options->include_opposite_normals;

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, aComponentOptions, aComponents);
    materialiseTriangleComponentSummaries(aSoup, aComponents);
    materialiseSphereComponents(aSoup, aComponents, *options, aComponents);

    const size_t aCount = aComponents.mySphereComponents.Size();
    *out_count          = aCount;
    if (out_buf == nullptr)
    {
      return OCCTL_OK;
    }
    if (cap < aCount)
    {
      return OCCTL_BUFFER_TOO_SMALL;
    }

    for (size_t anIdx = 0u; anIdx < aCount; ++anIdx)
    {
      out_buf[anIdx] = OCCTL_NODE_ID_INVALID;
    }

    NCollection_LinearVector<TopoDS_Shape> aShapes;
    aShapes.Reserve(aCount);
    for (size_t anIdx = 0u; anIdx < aCount; ++anIdx)
    {
      TopoDS_Shape aShape;
      if (const occtl_status_t aStatus =
            makeSphereComponentShape(aComponents.mySphereComponents[anIdx], aShape))
      {
        return aStatus;
      }
      aShapes.Append(aShape);
    }

    for (size_t anIdx = 0u; anIdx < aShapes.Size(); ++anIdx)
    {
      if (const occtl_status_t aStatus = addSolidToGraph(
            graph,
            aShapes.Value(anIdx),
            "rebuilt sphere component could not be ingested into BRepGraph as a Solid",
            &out_buf[anIdx]))
      {
        return aStatus;
      }
    }
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_cylinder_component_solids(
  occtl_graph_t* const                                           graph,
  const occtl_mesh_triangle_cylinder_components_options_t* const options,
  occtl_node_id_t* const                                         out_buf,
  const size_t                                                   cap,
  size_t* const                                                  out_count)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_count == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *out_count = 0u;

    if (const occtl_status_t aStatus = validateTriangleCylinderComponentsOptions(options))
    {
      return aStatus;
    }

    occtl_mesh_triangle_components_options_t aComponentOptions =
      OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
    aComponentOptions.root                     = options->root;
    aComponentOptions.max_normal_angle         = options->max_normal_angle;
    aComponentOptions.include_opposite_normals = options->include_opposite_normals;

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, aComponentOptions, aComponents);
    materialiseTriangleComponentSummaries(aSoup, aComponents);
    materialiseCylinderComponents(aSoup, anAnalysis, aComponents, *options, aComponents);

    const size_t aCount = aComponents.myCylinderComponents.Size();
    *out_count          = aCount;
    if (out_buf == nullptr)
    {
      return OCCTL_OK;
    }
    if (cap < aCount)
    {
      return OCCTL_BUFFER_TOO_SMALL;
    }

    for (size_t anIdx = 0u; anIdx < aCount; ++anIdx)
    {
      out_buf[anIdx] = OCCTL_NODE_ID_INVALID;
    }

    NCollection_LinearVector<TopoDS_Shape> aShapes;
    aShapes.Reserve(aCount);
    for (size_t anIdx = 0u; anIdx < aCount; ++anIdx)
    {
      TopoDS_Shape aShape;
      if (const occtl_status_t aStatus =
            makeCylinderComponentShape(aComponents.myCylinderComponents[anIdx], aShape))
      {
        return aStatus;
      }
      aShapes.Append(aShape);
    }

    for (size_t anIdx = 0u; anIdx < aShapes.Size(); ++anIdx)
    {
      if (const occtl_status_t aStatus = addSolidToGraph(
            graph,
            aShapes.Value(anIdx),
            "rebuilt cylinder component could not be ingested into BRepGraph as a Solid",
            &out_buf[anIdx]))
      {
        return aStatus;
      }
    }
    return OCCTL_OK;
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_plane_component_face(
  occtl_graph_t* const                                        graph,
  const occtl_mesh_triangle_plane_components_options_t* const options,
  const uint32_t                                              component_id,
  occtl_node_id_t* const                                      out_face)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_face == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *out_face = OCCTL_NODE_ID_INVALID;

    if (const occtl_status_t aStatus = validateTrianglePlaneComponentsOptions(options))
    {
      return aStatus;
    }

    occtl_mesh_triangle_components_options_t aComponentOptions =
      OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
    aComponentOptions.root                     = options->root;
    aComponentOptions.max_normal_angle         = options->max_normal_angle;
    aComponentOptions.include_opposite_normals = options->include_opposite_normals;

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, aComponentOptions, aComponents);
    if (component_id >= aComponents.myComponentCount)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE, "component_id is out of range");
      return OCCTL_OUT_OF_RANGE;
    }

    materialiseTriangleComponentSummaries(aSoup, aComponents);
    materialisePlaneComponents(aSoup, aComponents, *options, aComponents);
    const occtl_mesh_triangle_plane_component_t* const aPlaneComponent =
      findPlaneComponent(aComponents, component_id);
    if (aPlaneComponent == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "component_id is not plane-like");
      return OCCTL_NOT_FOUND;
    }

    TopoDS_Face aFace;
    if (const occtl_status_t aStatus = makePlaneComponentTopoFace(aSoup,
                                                                  anAnalysis,
                                                                  aComponents,
                                                                  component_id,
                                                                  *aPlaneComponent,
                                                                  aFace))
    {
      return aStatus;
    }

    return addFaceToGraph(graph, aFace, out_face);
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_make_plane_component_faces(
  occtl_graph_t* const                                        graph,
  const occtl_mesh_triangle_plane_components_options_t* const options,
  occtl_node_id_t* const                                      out_buf,
  const size_t                                                cap,
  size_t* const                                               out_count)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || out_count == nullptr)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *out_count = 0u;

    if (const occtl_status_t aStatus = validateTrianglePlaneComponentsOptions(options))
    {
      return aStatus;
    }

    occtl_mesh_triangle_components_options_t aComponentOptions =
      OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
    aComponentOptions.root                     = options->root;
    aComponentOptions.max_normal_angle         = options->max_normal_angle;
    aComponentOptions.include_opposite_normals = options->include_opposite_normals;

    OcctL::Mesh::MeshCache&     aCache = cacheFor(graph);
    std::lock_guard<std::mutex> aLock(aCache.Mutex());

    OcctL::Mesh::TriangleSoupBuffers& aSoup = aCache.TriangleSoupSlot();
    if (const occtl_status_t aStatus = populateTriangleSoup(graph, options->root, aSoup))
    {
      return aStatus;
    }

    OcctL::Mesh::TriangleAnalysisBuffers& anAnalysis = aCache.TriangleAnalysisSlot();
    materialiseTriangleAnalysis(aSoup, anAnalysis);

    OcctL::Mesh::TriangleComponentBuffers& aComponents = aCache.TriangleComponentSlot();
    materialiseTriangleComponents(anAnalysis, aComponentOptions, aComponents);
    materialiseTriangleComponentSummaries(aSoup, aComponents);
    materialisePlaneComponents(aSoup, aComponents, *options, aComponents);

    const size_t aCount = aComponents.myPlaneComponents.Size();
    *out_count          = aCount;
    if (out_buf == nullptr)
    {
      return OCCTL_OK;
    }
    if (cap < aCount)
    {
      return OCCTL_BUFFER_TOO_SMALL;
    }

    for (size_t anIdx = 0u; anIdx < aCount; ++anIdx)
    {
      out_buf[anIdx] = OCCTL_NODE_ID_INVALID;
    }

    NCollection_LinearVector<TopoDS_Face> aFaces;
    aFaces.Reserve(aCount);
    for (size_t anIdx = 0u; anIdx < aCount; ++anIdx)
    {
      const occtl_mesh_triangle_plane_component_t& aPlaneComponent =
        aComponents.myPlaneComponents[anIdx];
      TopoDS_Face aFace;
      if (const occtl_status_t aStatus = makePlaneComponentTopoFace(aSoup,
                                                                    anAnalysis,
                                                                    aComponents,
                                                                    aPlaneComponent.component_id,
                                                                    aPlaneComponent,
                                                                    aFace))
      {
        return aStatus;
      }
      aFaces.Append(aFace);
    }

    for (size_t anIdx = 0u; anIdx < aFaces.Size(); ++anIdx)
    {
      if (const occtl_status_t aStatus =
            addFaceToGraph(graph, aFaces.Value(anIdx), &out_buf[anIdx]))
      {
        return aStatus;
      }
    }
    return OCCTL_OK;
  });
}

} // extern "C"
