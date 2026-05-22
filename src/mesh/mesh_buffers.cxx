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

//! @file mesh_buffers.cxx
//! @brief Mesh buffer import helpers.

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <occtl/occtl_mesh.h>

#include <BRepGraph_EditorView.hxx>
#include <BRepGraph_MeshView.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRep_Builder.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>

#include <cmath>
#include <limits>
#include <memory>

namespace
{

bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

occtl_status_t validateMeshFromBuffers(const occtl_mesh_from_buffers_options_t* const theOptions)
{
  if (theOptions == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "options is NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOptions->struct_version != OCCTL_MESH_FROM_BUFFERS_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "options->struct_version is not OCCTL_MESH_FROM_BUFFERS_OPTIONS_VERSION_1");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOptions->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "options->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOptions->nodes == nullptr || theOptions->triangles == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "nodes and triangles must be non-NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOptions->node_count == 0 || theOptions->triangle_count == 0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "node_count and triangle_count must be positive");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOptions->node_count > static_cast<size_t>(std::numeric_limits<int>::max())
      || theOptions->triangle_count > static_cast<size_t>(std::numeric_limits<int>::max()))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "node_count or triangle_count exceeds OCCT limits");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsFiniteValue(theOptions->deflection) || theOptions->deflection < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "deflection must be finite and non-negative");
    return OCCTL_INVALID_ARGUMENT;
  }

  for (size_t anIdx = 0; anIdx < theOptions->node_count * 3u; ++anIdx)
  {
    if (!IsFiniteValue(theOptions->nodes[anIdx]))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "node coordinate is not finite");
      return OCCTL_INVALID_ARGUMENT;
    }
  }

  for (size_t anIdx = 0; anIdx < theOptions->triangle_count * 3u; ++anIdx)
  {
    if (static_cast<size_t>(theOptions->triangles[anIdx]) >= theOptions->node_count)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "triangle index is out of range");
      return OCCTL_INVALID_ARGUMENT;
    }
  }

  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_mesh_from_buffers_options_init(occtl_mesh_from_buffers_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions = OCCTL_MESH_FROM_BUFFERS_OPTIONS_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_from_buffers(const occtl_mesh_from_buffers_options_t* const theOptions,
                          occtl_graph_t** const                          theOutGraph,
                          occtl_node_id_t* const                         theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "out_graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    if (const occtl_status_t aStatus = validateMeshFromBuffers(theOptions))
    {
      return aStatus;
    }

    occ::handle<Poly_Triangulation> aTri =
      new Poly_Triangulation(static_cast<int>(theOptions->node_count),
                             static_cast<int>(theOptions->triangle_count),
                             false);
    aTri->Deflection(theOptions->deflection);

    for (int anIdx = 1; anIdx <= static_cast<int>(theOptions->node_count); ++anIdx)
    {
      const size_t aBase = static_cast<size_t>(anIdx - 1) * 3u;
      aTri->SetNode(anIdx,
                    gp_Pnt(theOptions->nodes[aBase],
                           theOptions->nodes[aBase + 1u],
                           theOptions->nodes[aBase + 2u]));
    }

    for (int anIdx = 1; anIdx <= static_cast<int>(theOptions->triangle_count); ++anIdx)
    {
      const size_t aBase = static_cast<size_t>(anIdx - 1) * 3u;
      aTri->SetTriangle(anIdx,
                        Poly_Triangle(static_cast<int>(theOptions->triangles[aBase] + 1u),
                                      static_cast<int>(theOptions->triangles[aBase + 1u] + 1u),
                                      static_cast<int>(theOptions->triangles[aBase + 2u] + 1u)));
    }

    TopoDS_Face  aFace;
    BRep_Builder aBuilder;
    aBuilder.MakeFace(aFace, aTri);
    if (aFace.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "OCCT could not create a triangulated face");
      return OCCTL_GEOMETRY_INVALID;
    }

    std::unique_ptr<occtl_graph>   aGraph(new occtl_graph());
    BRepGraph::ShapesView::Options aBuildOptions;
    aBuildOptions.CreateAutoProduct = false;
    const BRepGraph::ShapesView::Result aBuildResult =
      aGraph->graph.Shapes().Add(aFace, aBuildOptions);
    if (!aBuildResult.Ok || !aBuildResult.TopologyRoot.IsValid()
        || aBuildResult.TopologyRoot.NodeKind != BRepGraph_NodeId::Kind::Face)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "triangulated face could not be ingested into BRepGraph as a Face");
      return OCCTL_TOPOLOGY_INVALID;
    }

    const BRepGraph_FaceId             aFaceId(aBuildResult.TopologyRoot);
    const BRepGraph_TriangulationRepId aTriRepId(
      aGraph->graph.Editor().Reps().CreateTriangulation(aTri));
    aGraph->graph.Editor().Faces().SetTriangulationRep(aFaceId, aTriRepId);
    aGraph->graph.Mesh().Editor().Faces().AppendCachedTriangulation(aFaceId, aTriRepId);
    aGraph->graph.Mesh().Editor().Faces().SetCachedActiveTriangulationIndex(aFaceId, 0);

    *theOutRoot  = OcctL::Topo::PackNodeId(aBuildResult.TopologyRoot);
    *theOutGraph = aGraph.release();
    return OCCTL_OK;
  });
}

//==================================================================================================

} // extern "C"
