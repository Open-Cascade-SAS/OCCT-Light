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

//! @file MeshCache.hxx
//! @brief Per-graph cache of materialised mesh buffers backing the public
//! triangulation / polygon3D / polygon-on-tri views.
//!
//! The public ABI promises @c const double* xyz interleaved buffers and
//! 0-indexed triangle / polygon-on-tri vertex indices. OCCT's
//! @c Poly_Triangulation can store nodes in either double (gp_Pnt) or
//! single-precision (NCollection_Vec3<float>) layouts, and indices are
//! 1-indexed (Poly_Triangle / Poly_PolygonOnTriangulation::Nodes()). On
//! first view fetch the shim materialises all the public-facing buffers
//! here once, then subsequent fetches return the same pointers without
//! re-copying.
//!
//! Each slot remembers the source node's @c BRepGraph_VersionStamp; on
//! every fetch the shim compares the cached stamp against the current
//! one. A bump (any builder, any boolean op, any compact, any subsequent
//! mesh_generate) yields a stale slot that is cleared and re-materialised
//! transparently. This avoids instrumenting every mutation entry point
//! with explicit invalidation calls and keeps the public lifetime
//! contract honest.

#ifndef OCCTL_MESH_MESHCACHE_HXX
#define OCCTL_MESH_MESHCACHE_HXX

#include <occtl/occtl_core.h>
#include <occtl/occtl_mesh.h>
#include <occtl/occtl_topo.h>

#include <BRepGraph_VersionStamp.hxx>
#include <NCollection_FlatDataMap.hxx>
#include <NCollection_LinearVector.hxx>

#include <cstddef>
#include <cstdint>
#include <mutex>

namespace OcctL::Mesh
{

//! Per-face triangulation cache slot (one per (face uid, lod index) pair).
//! Empty NCollection_LinearVectors signal "not yet materialised"; the
//! mesh_views shim populates them on first read.
struct FaceMeshBuffers
{
  NCollection_LinearVector<double>   myNodes;   //!< xyz, 3*NbNodes; always materialised to double.
  NCollection_LinearVector<double>   myNormals; //!< xyz, 3*NbNodes or empty when absent.
  NCollection_LinearVector<double>   myUVs;     //!< uv,  2*NbNodes or empty when absent.
  NCollection_LinearVector<uint32_t> myTriangles; //!< 0-indexed triplets, 3*NbTriangles.

  BRepGraph_VersionStamp
              myStamp; //!< Captured at materialisation; compared against current on read.
  double      myDeflection = 0.0;
  occtl_uid_t mySourceUid  = OCCTL_UID_INVALID;
};

//! Per-coedge polygon-on-triangulation cache slot.
struct CoEdgeMeshBuffers
{
  NCollection_LinearVector<uint32_t> myNodeIndices; //!< 0-indexed into parent face triangulation.
  NCollection_LinearVector<double>   myParameters;  //!< Length NbNodes or empty when absent.

  BRepGraph_VersionStamp
              myStamp; //!< Captured at materialisation; compared against current on read.
  double      myDeflection = 0.0;
  occtl_uid_t mySourceUid  = OCCTL_UID_INVALID;
};

//! Per-edge 3D polyline cache slot.
struct EdgeMeshBuffers
{
  NCollection_LinearVector<double> myNodes;      //!< xyz interleaved doubles, length 3*NbNodes.
  NCollection_LinearVector<double> myParameters; //!< Length NbNodes or empty when absent.

  BRepGraph_VersionStamp
              myStamp; //!< Captured at materialisation; compared against current on read.
  double      myDeflection = 0.0;
  occtl_uid_t mySourceUid  = OCCTL_UID_INVALID;
};

//! Per-graph aggregate triangle-soup cache slot.
struct TriangleSoupBuffers
{
  NCollection_LinearVector<double>   myNodes;     //!< xyz, 3*NbNodes.
  NCollection_LinearVector<uint32_t> myTriangles; //!< 0-indexed triplets.

  occtl_node_id_t myRoot      = OCCTL_NODE_ID_INVALID;
  size_t          myFaceCount = 0;
};

//! Per-graph triangle analysis cache slot.
struct TriangleAnalysisBuffers
{
  NCollection_LinearVector<double> myNormals; //!< xyz, 3*NbTriangles.
  NCollection_LinearVector<uint32_t>
    myAdjacency; //!< 3 neighbours per triangle; UINT32_MAX for boundary.

  occtl_node_id_t myRoot          = OCCTL_NODE_ID_INVALID;
  size_t          myTriangleCount = 0;
  size_t          myFaceCount     = 0;
};

//! Per-graph normal-connected triangle component cache slot.
struct TriangleComponentBuffers
{
  NCollection_LinearVector<uint32_t> myComponentIds;   //!< Length NbTriangles.
  NCollection_LinearVector<uint32_t> myComponentSizes; //!< Length NbComponents.
  NCollection_LinearVector<occtl_mesh_triangle_component_summary_t>
    mySummaries; //!< Length NbComponents.
  NCollection_LinearVector<occtl_mesh_triangle_plane_component_t>
    myPlaneComponents; //!< Plane-like components.
  NCollection_LinearVector<occtl_mesh_triangle_sphere_component_t>
    mySphereComponents; //!< Sphere-like components.
  NCollection_LinearVector<occtl_mesh_triangle_cylinder_component_t>
    myCylinderComponents; //!< Cylinder-like components.
  NCollection_LinearVector<uint32_t>
    mySelectedTriangles; //!< Triangles belonging to one selected component.
  NCollection_LinearVector<occtl_mesh_triangle_component_boundary_edge_t>
    mySelectedBoundaryEdges; //!< Boundary edges of one selected component.
  NCollection_LinearVector<occtl_mesh_triangle_component_boundary_edge_t>
    myOrderedBoundaryEdges; //!< Boundary edges ordered into chains.
  NCollection_LinearVector<occtl_mesh_triangle_component_boundary_chain_t>
    myBoundaryChains; //!< Ordered boundary chain descriptors.
  NCollection_LinearVector<occtl_point3_t>
    myBoundaryPolylinePoints; //!< Ordered boundary polyline points.
  NCollection_LinearVector<occtl_mesh_component_boundary_polyline_t>
    myBoundaryPolylines; //!< Boundary polyline descriptors.

  occtl_node_id_t myRoot           = OCCTL_NODE_ID_INVALID;
  size_t          myTriangleCount  = 0;
  size_t          myComponentCount = 0;
};

//! Composite key for face cache lookups.
struct FaceMeshKey
{
  uint64_t myUid;
  uint32_t myIndex;

  bool operator==(const FaceMeshKey& theOther) const noexcept
  {
    return myUid == theOther.myUid && myIndex == theOther.myIndex;
  }
};

//! Hasher conforming to NCollection's HashCode/IsEqual contract.
struct FaceMeshKeyHasher
{
  size_t operator()(const FaceMeshKey& theKey) const noexcept
  {
    // FNV-style fold; uid dominates, index salts the upper 32 bits.
    return static_cast<size_t>(theKey.myUid ^ (static_cast<uint64_t>(theKey.myIndex) << 32));
  }

  bool operator()(const FaceMeshKey& theLhs, const FaceMeshKey& theRhs) const noexcept
  {
    return theLhs == theRhs;
  }
};

class MeshCache
{
public:
  MeshCache()  = default;
  ~MeshCache() = default;

  MeshCache(const MeshCache&)            = delete;
  MeshCache& operator=(const MeshCache&) = delete;

  //! Mutex protecting the maps. View fetchers in mesh_views.cxx lock
  //! this for the entire stamp-check + materialisation block; concurrent
  //! readers of disjoint slots serialise but never tear.
  [[nodiscard]] std::mutex& Mutex() const noexcept { return myMutex; }

  //! Drops every cached entry. Called proactively from #occtl_mesh_generate
  //! before Perform() so a partial failure does not leave torn slots, and
  //! again after a successful Perform() as a defensive belt-and-suspenders.
  //! Stamps still self-evict on any other mutation path. Caller must
  //! hold #Mutex().
  void Invalidate();

  //! Returns (and lazily creates) the buffer slot for (face, index).
  //! Caller compares @c myStamp against the current graph stamp; on
  //! mismatch the slot is reset and re-materialised. Caller must hold
  //! #Mutex().
  FaceMeshBuffers&          FindOrCreateFaceSlot(uint64_t theFaceUid, uint32_t theIndex);
  EdgeMeshBuffers&          FindOrCreateEdgeSlot(uint64_t theEdgeUid);
  CoEdgeMeshBuffers&        FindOrCreateCoEdgeSlot(uint64_t theCoEdgeUid);
  TriangleSoupBuffers&      TriangleSoupSlot();
  TriangleAnalysisBuffers&  TriangleAnalysisSlot();
  TriangleComponentBuffers& TriangleComponentSlot();

private:
  mutable std::mutex                                                       myMutex;
  NCollection_FlatDataMap<FaceMeshKey, FaceMeshBuffers, FaceMeshKeyHasher> myFaces;
  NCollection_FlatDataMap<uint64_t, EdgeMeshBuffers>                       myEdges;
  NCollection_FlatDataMap<uint64_t, CoEdgeMeshBuffers>                     myCoEdges;
  TriangleSoupBuffers                                                      myTriangleSoup;
  TriangleAnalysisBuffers                                                  myTriangleAnalysis;
  TriangleComponentBuffers                                                 myTriangleComponents;
};

} // namespace OcctL::Mesh

#endif // OCCTL_MESH_MESHCACHE_HXX
