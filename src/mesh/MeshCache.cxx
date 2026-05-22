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

#include "MeshCache.hxx"

namespace OcctL::Mesh
{

void MeshCache::Invalidate()
{
  myFaces.Clear();
  myEdges.Clear();
  myCoEdges.Clear();
  myTriangleSoup       = TriangleSoupBuffers{};
  myTriangleAnalysis   = TriangleAnalysisBuffers{};
  myTriangleComponents = TriangleComponentBuffers{};
}

FaceMeshBuffers& MeshCache::FindOrCreateFaceSlot(const uint64_t theFaceUid, const uint32_t theIndex)
{
  const FaceMeshKey aKey{theFaceUid, theIndex};
  if (FaceMeshBuffers* aExisting = myFaces.ChangeSeek(aKey))
  {
    return *aExisting;
  }
  myFaces.Bind(aKey, FaceMeshBuffers{});
  return myFaces.ChangeFind(aKey);
}

EdgeMeshBuffers& MeshCache::FindOrCreateEdgeSlot(const uint64_t theEdgeUid)
{
  if (EdgeMeshBuffers* aExisting = myEdges.ChangeSeek(theEdgeUid))
  {
    return *aExisting;
  }
  myEdges.Bind(theEdgeUid, EdgeMeshBuffers{});
  return myEdges.ChangeFind(theEdgeUid);
}

CoEdgeMeshBuffers& MeshCache::FindOrCreateCoEdgeSlot(const uint64_t theCoEdgeUid)
{
  if (CoEdgeMeshBuffers* aExisting = myCoEdges.ChangeSeek(theCoEdgeUid))
  {
    return *aExisting;
  }
  myCoEdges.Bind(theCoEdgeUid, CoEdgeMeshBuffers{});
  return myCoEdges.ChangeFind(theCoEdgeUid);
}

TriangleSoupBuffers& MeshCache::TriangleSoupSlot()
{
  myTriangleSoup.myNodes.Clear();
  myTriangleSoup.myTriangles.Clear();
  myTriangleSoup.myRoot      = OCCTL_NODE_ID_INVALID;
  myTriangleSoup.myFaceCount = 0;
  return myTriangleSoup;
}

TriangleAnalysisBuffers& MeshCache::TriangleAnalysisSlot()
{
  myTriangleAnalysis.myNormals.Clear();
  myTriangleAnalysis.myAdjacency.Clear();
  myTriangleAnalysis.myRoot          = OCCTL_NODE_ID_INVALID;
  myTriangleAnalysis.myTriangleCount = 0;
  myTriangleAnalysis.myFaceCount     = 0;
  return myTriangleAnalysis;
}

TriangleComponentBuffers& MeshCache::TriangleComponentSlot()
{
  myTriangleComponents.myComponentIds.Clear();
  myTriangleComponents.myComponentSizes.Clear();
  myTriangleComponents.mySummaries.Clear();
  myTriangleComponents.myPlaneComponents.Clear();
  myTriangleComponents.mySphereComponents.Clear();
  myTriangleComponents.myCylinderComponents.Clear();
  myTriangleComponents.mySelectedTriangles.Clear();
  myTriangleComponents.mySelectedBoundaryEdges.Clear();
  myTriangleComponents.myOrderedBoundaryEdges.Clear();
  myTriangleComponents.myBoundaryChains.Clear();
  myTriangleComponents.myBoundaryPolylinePoints.Clear();
  myTriangleComponents.myBoundaryPolylines.Clear();
  myTriangleComponents.myRoot           = OCCTL_NODE_ID_INVALID;
  myTriangleComponents.myTriangleCount  = 0;
  myTriangleComponents.myComponentCount = 0;
  return myTriangleComponents;
}

} // namespace OcctL::Mesh
