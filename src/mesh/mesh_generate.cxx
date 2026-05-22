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

//! @file mesh_generate.cxx
//! @brief BRepGraphMesh_IncrementalMesh dispatch.
//!
//! Three modes folded into one entry point via the (nodes, n_nodes) pair:
//!   * 0 nodes      → whole-graph dispatch
//!   * 1 node       → single-root subtree dispatch
//!   * N>1 nodes    → multi-node dispatch
//! Bbox-derived parameter mode (use_bbox != 0) goes through the bbox
//! overload of BRepGraphMesh_IncrementalMesh.

#include "MeshCache.hxx"
#include "MeshMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <occtl/occtl_mesh.h>

#include <BRepGraphMesh_IncrementalMesh.hxx>
#include <BRepGraph_NodeId.hxx>
#include <NCollection_DynamicArray.hxx>
#include <Precision.hxx>

#include <cmath>
#include <mutex>

namespace
{

//! Drops every entry in the per-graph mesh view cache. Pre-Perform
//! invalidation guarantees that a partial failure (Perform returns false
//! after touching some faces) does not leave the cache torn — every face
//! re-materialises from the current Poly_Triangulation handle on the
//! next view fetch. The per-slot version stamps in #FaceMeshBuffers /
//! #CoEdgeMeshBuffers handle the post-success path too (every mutation
//! bumps the stamp; stale slots self-evict on the next read), but a
//! single Clear() here is cheaper than walking every slot.
void invalidateCache(occtl_graph_t* const theGraph) noexcept
{
  if (theGraph == nullptr || !theGraph->meshCache)
  {
    return;
  }
  std::lock_guard<std::mutex> aLock(theGraph->meshCache->Mutex());
  theGraph->meshCache->Invalidate();
}

bool isBool01(const int32_t theValue) noexcept
{
  return theValue == 0 || theValue == 1;
}

bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool isFinitePoint(const occtl_point3_t& thePoint) noexcept
{
  return IsFiniteValue(thePoint.x) && IsFiniteValue(thePoint.y) && IsFiniteValue(thePoint.z);
}

occtl_status_t reject(const char* const theMessage)
{
  OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, theMessage);
  return OCCTL_INVALID_ARGUMENT;
}

occtl_status_t validateOptions(const occtl_mesh_options_t& theOptions)
{
  if (!OcctL::Mesh::IsKnownVersion(theOptions))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                           "unsupported mesh options struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOptions.p_next != nullptr)
  {
    return reject("mesh options p_next must be NULL");
  }
  if (!IsFiniteValue(theOptions.deflection) || theOptions.deflection <= 0.0)
  {
    return reject("mesh deflection must be finite and positive");
  }
  if (!IsFiniteValue(theOptions.angle) || theOptions.angle <= 0.0)
  {
    return reject("mesh angle must be finite and positive");
  }
  if (!IsFiniteValue(theOptions.deflection_interior) || theOptions.deflection_interior < -1.0
      || theOptions.deflection_interior == 0.0)
  {
    return reject("mesh interior deflection must be -1 or finite and positive");
  }
  if (!IsFiniteValue(theOptions.angle_interior) || theOptions.angle_interior < -1.0
      || theOptions.angle_interior == 0.0)
  {
    return reject("mesh interior angle must be -1 or finite and positive");
  }
  if (!IsFiniteValue(theOptions.min_size) || theOptions.min_size < -1.0)
  {
    return reject("mesh min_size must be -1 or finite and non-negative");
  }

  if (!isBool01(theOptions.in_parallel) || !isBool01(theOptions.relative)
      || !isBool01(theOptions.internal_vertices_mode)
      || !isBool01(theOptions.control_surface_deflection)
      || !isBool01(theOptions.control_surface_deflection_all) || !isBool01(theOptions.clean_model)
      || !isBool01(theOptions.adjust_min_size) || !isBool01(theOptions.force_face_deflection)
      || !isBool01(theOptions.allow_quality_decrease) || !isBool01(theOptions.use_bbox))
  {
    return reject("mesh boolean options must be 0 or 1");
  }

  if (!IsFiniteValue(theOptions.deviation_coefficient) || theOptions.deviation_coefficient <= 0.0)
  {
    return reject("mesh deviation_coefficient must be finite and positive");
  }
  if (!IsFiniteValue(theOptions.deviation_angle) || theOptions.deviation_angle <= 0.0)
  {
    return reject("mesh deviation_angle must be finite and positive");
  }
  if (theOptions.use_bbox != 0)
  {
    if (!isFinitePoint(theOptions.bbox.min) || !isFinitePoint(theOptions.bbox.max))
    {
      return reject("mesh bbox coordinates must be finite");
    }
    if (theOptions.bbox.min.x > theOptions.bbox.max.x
        || theOptions.bbox.min.y > theOptions.bbox.max.y
        || theOptions.bbox.min.z > theOptions.bbox.max.z)
    {
      return reject("mesh bbox min must not exceed max");
    }
  }

  return OCCTL_OK;
}

} // unnamed namespace

extern "C"
{

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_generate(occtl_graph_t* const              graph,
                                                        const occtl_node_id_t* const      nodes,
                                                        const size_t                      n_nodes,
                                                        const occtl_mesh_options_t* const options)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (graph == nullptr || options == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             graph == nullptr ? "graph is NULL"
                                                              : "options is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (nodes == nullptr && n_nodes != 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "nodes is NULL but n_nodes is non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }

    const occtl_status_t aValidationStatus = validateOptions(*options);
    if (aValidationStatus != OCCTL_OK)
    {
      return aValidationStatus;
    }

    const IMeshTools_Parameters aParams  = OcctL::Mesh::ToParameters(*options);
    const bool                  aUseBbox = options->use_bbox != 0;
    const Bnd_Box               aBox = aUseBbox ? OcctL::Mesh::ToBndBox(options->bbox) : Bnd_Box();
    const double                aDevCoef    = options->deviation_coefficient;
    const double                aDevAngle   = options->deviation_angle;
    const bool                  aInParallel = options->in_parallel != 0;

    BRepGraph_NodeId                           aSingleRoot;
    NCollection_DynamicArray<BRepGraph_NodeId> aNodeIds;
    if (n_nodes == 1)
    {
      const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(nodes[0]);
      if (!aNodeId.IsValid())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                               "mesh root node is invalid or removed");
        return OCCTL_NOT_FOUND;
      }
      aSingleRoot = aNodeId;
    }
    else if (n_nodes > 1)
    {
      for (size_t i = 0; i < n_nodes; ++i)
      {
        const BRepGraph_NodeId aId = OcctL::Topo::UnpackNodeId(nodes[i]);
        if (!aId.IsValid())
        {
          OcctL::Core::ErrorState::Current().Set(
            OCCTL_NOT_FOUND,
            "mesh node list contains an invalid or removed node");
          return OCCTL_NOT_FOUND;
        }
        aNodeIds.Append(aId);
      }
    }

    // Invalidate up-front so a partial-failure Perform does not leave a
    // half-cached state visible to the next view fetch. Stamps would
    // catch this anyway on a per-slot basis, but eager clearing avoids
    // walking the cache on every subsequent read until the stamp catches
    // up.
    invalidateCache(graph);

    bool aOk = false;
    if (n_nodes == 0)
    {
      aOk = aUseBbox ? BRepGraphMesh_IncrementalMesh::Perform(graph->graph,
                                                              aBox,
                                                              aDevCoef,
                                                              aDevAngle,
                                                              aInParallel)
                     : BRepGraphMesh_IncrementalMesh::Perform(graph->graph, aParams);
    }
    else if (n_nodes == 1)
    {
      aOk = aUseBbox ? BRepGraphMesh_IncrementalMesh::Perform(graph->graph,
                                                              aSingleRoot,
                                                              aBox,
                                                              aDevCoef,
                                                              aDevAngle,
                                                              aInParallel)
                     : BRepGraphMesh_IncrementalMesh::Perform(graph->graph, aSingleRoot, aParams);
    }
    else
    {
      aOk = aUseBbox ? BRepGraphMesh_IncrementalMesh::Perform(graph->graph,
                                                              aNodeIds,
                                                              aBox,
                                                              aDevCoef,
                                                              aDevAngle,
                                                              aInParallel)
                     : BRepGraphMesh_IncrementalMesh::Perform(graph->graph, aNodeIds, aParams);
    }

    return aOk ? OCCTL_OK : OCCTL_NOT_DONE;
  });
}

} // extern "C"
