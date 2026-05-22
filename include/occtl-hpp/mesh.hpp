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

/**
 * @file
 * @brief C++ veneer for the mesh module.
 *
 * Provides idiomatic options, borrowed span views, and mesh generation
 * helpers. Failures translate to occtl::Error via check().
 */

#ifndef OCCTL_HPP_MESH_HPP
#define OCCTL_HPP_MESH_HPP

#include <occtl/occtl_mesh.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/geom.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
  #include <span>
  #ifndef OCCTL_HPP_HAS_SPAN
    #define OCCTL_HPP_HAS_SPAN 1
  #endif
#else
  #ifndef OCCTL_HPP_HAS_SPAN
    #define OCCTL_HPP_HAS_SPAN 0
  #endif
#endif

namespace occtl::mesh
{

/// @brief Axis-aligned bounding box.  Aggregate over the C POD.
struct AABB3
{
  Point3 min;
  Point3 max;
};

/// @brief Idiomatic mirror of @c ::occtl_mesh_options_t.
///
/// Defaults are the conservative tessellation knobs; supply an @c AABB3
/// in @c bbox to switch the algorithm to bbox-derived parameter mode.
struct Options
{
  double deflection                     = 0.001;
  double angle                          = 0.5;
  double deflection_interior            = -1.0;
  double angle_interior                 = -1.0;
  double min_size                       = -1.0;
  bool   in_parallel                    = false;
  bool   relative                       = false;
  bool   internal_vertices_mode         = true;
  bool   control_surface_deflection     = true;
  bool   control_surface_deflection_all = false;
  bool   clean_model                    = true;
  bool   adjust_min_size                = false;
  bool   force_face_deflection          = false;
  bool   allow_quality_decrease         = false;

  std::optional<AABB3> bbox;
  double               deviation_coefficient = 0.001;
  double               deviation_angle       = 0.3490658503988659; // 20°

  /// @brief Project into the C ABI options struct.
  [[nodiscard]] ::occtl_mesh_options_t to_c() const noexcept
  {
    ::occtl_mesh_options_t aOpts         = OCCTL_MESH_OPTIONS_INIT;
    aOpts.deflection                     = deflection;
    aOpts.angle                          = angle;
    aOpts.deflection_interior            = deflection_interior;
    aOpts.angle_interior                 = angle_interior;
    aOpts.min_size                       = min_size;
    aOpts.in_parallel                    = in_parallel ? 1 : 0;
    aOpts.relative                       = relative ? 1 : 0;
    aOpts.internal_vertices_mode         = internal_vertices_mode ? 1 : 0;
    aOpts.control_surface_deflection     = control_surface_deflection ? 1 : 0;
    aOpts.control_surface_deflection_all = control_surface_deflection_all ? 1 : 0;
    aOpts.clean_model                    = clean_model ? 1 : 0;
    aOpts.adjust_min_size                = adjust_min_size ? 1 : 0;
    aOpts.force_face_deflection          = force_face_deflection ? 1 : 0;
    aOpts.allow_quality_decrease         = allow_quality_decrease ? 1 : 0;
    aOpts.deviation_coefficient          = deviation_coefficient;
    aOpts.deviation_angle                = deviation_angle;
    if (bbox.has_value())
    {
      aOpts.use_bbox = 1;
      aOpts.bbox.min = bbox->min.c_type();
      aOpts.bbox.max = bbox->max.c_type();
    }
    return aOpts;
  }
};

/// @brief Input buffers for creating a graph-owned triangulated face.
using FromBuffersOptions = ::occtl_mesh_from_buffers_options_t;

/// @brief Borrowed view of a face triangulation.  Lifetime tied to the source graph.
class TriangulationView
{
public:
  TriangulationView() noexcept = default;

  explicit TriangulationView(const ::occtl_triangulation_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const double> nodes() const noexcept
  {
    return {myC.nodes, myC.node_count * 3u};
  }

  [[nodiscard]] std::span<const double> normals() const noexcept
  {
    return myC.normals != nullptr ? std::span<const double>{myC.normals, myC.node_count * 3u}
                                  : std::span<const double>{};
  }

  [[nodiscard]] std::span<const double> uvs() const noexcept
  {
    return myC.uvs != nullptr ? std::span<const double>{myC.uvs, myC.node_count * 2u}
                              : std::span<const double>{};
  }

  [[nodiscard]] std::span<const uint32_t> triangles() const noexcept
  {
    return {myC.triangles, myC.triangle_count * 3u};
  }
#else
  [[nodiscard]] const double* nodes() const noexcept { return myC.nodes; }

  [[nodiscard]] const double* normals() const noexcept { return myC.normals; }

  [[nodiscard]] const double* uvs() const noexcept { return myC.uvs; }

  [[nodiscard]] const uint32_t* triangles() const noexcept { return myC.triangles; }
#endif

  [[nodiscard]] std::size_t node_count() const noexcept { return myC.node_count; }

  [[nodiscard]] std::size_t triangle_count() const noexcept { return myC.triangle_count; }

  [[nodiscard]] double deflection() const noexcept { return myC.deflection; }

  [[nodiscard]] UID source_uid() const noexcept { return UID(myC.source_uid); }

  [[nodiscard]] bool has_normals() const noexcept { return myC.normals != nullptr; }

  [[nodiscard]] bool has_uvs() const noexcept { return myC.uvs != nullptr; }

  [[nodiscard]] const ::occtl_triangulation_view_t& c() const noexcept { return myC; }

private:
  ::occtl_triangulation_view_t myC{};
};

/// @brief Borrowed view of a 3D polyline cached on an edge.
class Polygon3DView
{
public:
  Polygon3DView() noexcept = default;

  explicit Polygon3DView(const ::occtl_polygon3d_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const double> nodes() const noexcept
  {
    return {myC.nodes, myC.node_count * 3u};
  }

  [[nodiscard]] std::span<const double> parameters() const noexcept
  {
    return myC.parameters != nullptr ? std::span<const double>{myC.parameters, myC.node_count}
                                     : std::span<const double>{};
  }
#else
  [[nodiscard]] const double* nodes() const noexcept { return myC.nodes; }

  [[nodiscard]] const double* parameters() const noexcept { return myC.parameters; }
#endif

  [[nodiscard]] std::size_t node_count() const noexcept { return myC.node_count; }

  [[nodiscard]] double deflection() const noexcept { return myC.deflection; }

  [[nodiscard]] UID source_uid() const noexcept { return UID(myC.source_uid); }

  [[nodiscard]] bool has_parameters() const noexcept { return myC.parameters != nullptr; }

  [[nodiscard]] const ::occtl_polygon3d_view_t& c() const noexcept { return myC; }

private:
  ::occtl_polygon3d_view_t myC{};
};

/// @brief Borrowed view of a coedge polyline indexed onto its parent face's triangulation.
class PolygonOnTriView
{
public:
  PolygonOnTriView() noexcept = default;

  explicit PolygonOnTriView(const ::occtl_polygon_on_tri_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const uint32_t> node_indices() const noexcept
  {
    return {myC.node_indices, myC.node_count};
  }

  [[nodiscard]] std::span<const double> parameters() const noexcept
  {
    return myC.parameters != nullptr ? std::span<const double>{myC.parameters, myC.node_count}
                                     : std::span<const double>{};
  }
#else
  [[nodiscard]] const uint32_t* node_indices() const noexcept { return myC.node_indices; }

  [[nodiscard]] const double* parameters() const noexcept { return myC.parameters; }
#endif

  [[nodiscard]] std::size_t node_count() const noexcept { return myC.node_count; }

  [[nodiscard]] double deflection() const noexcept { return myC.deflection; }

  [[nodiscard]] UID source_uid() const noexcept { return UID(myC.source_uid); }

  [[nodiscard]] bool has_parameters() const noexcept { return myC.parameters != nullptr; }

  [[nodiscard]] const ::occtl_polygon_on_tri_view_t& c() const noexcept { return myC; }

private:
  ::occtl_polygon_on_tri_view_t myC{};
};

/// @brief Borrowed aggregate triangle-soup buffers for a graph/root.
class TriangleBuffersView
{
public:
  TriangleBuffersView() noexcept = default;

  explicit TriangleBuffersView(const ::occtl_mesh_triangle_buffers_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const double> nodes() const noexcept
  {
    return {myC.nodes, myC.node_count * 3u};
  }

  [[nodiscard]] std::span<const uint32_t> triangles() const noexcept
  {
    return {myC.triangles, myC.triangle_count * 3u};
  }
#else
  [[nodiscard]] const double* nodes() const noexcept { return myC.nodes; }

  [[nodiscard]] const uint32_t* triangles() const noexcept { return myC.triangles; }
#endif

  [[nodiscard]] std::size_t node_count() const noexcept { return myC.node_count; }

  [[nodiscard]] std::size_t triangle_count() const noexcept { return myC.triangle_count; }

  [[nodiscard]] std::size_t face_count() const noexcept { return myC.face_count; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_buffers_view_t& c() const noexcept { return myC; }

private:
  ::occtl_mesh_triangle_buffers_view_t myC{};
};

/// @brief Borrowed per-triangle normals and adjacency for a graph/root.
class TriangleAnalysisView
{
public:
  TriangleAnalysisView() noexcept = default;

  explicit TriangleAnalysisView(const ::occtl_mesh_triangle_analysis_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const double> triangle_normals() const noexcept
  {
    return {myC.triangle_normals, myC.triangle_count * 3u};
  }

  [[nodiscard]] std::span<const uint32_t> triangle_adjacency() const noexcept
  {
    return {myC.triangle_adjacency, myC.triangle_count * 3u};
  }
#else
  [[nodiscard]] const double* triangle_normals() const noexcept { return myC.triangle_normals; }

  [[nodiscard]] const uint32_t* triangle_adjacency() const noexcept
  {
    return myC.triangle_adjacency;
  }
#endif

  [[nodiscard]] std::size_t triangle_count() const noexcept { return myC.triangle_count; }

  [[nodiscard]] std::size_t face_count() const noexcept { return myC.face_count; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_analysis_view_t& c() const noexcept { return myC; }

private:
  ::occtl_mesh_triangle_analysis_view_t myC{};
};

using TriangleComponentsOptions = ::occtl_mesh_triangle_components_options_t;

/// @brief Borrowed normal-connected triangle component labels.
class TriangleComponentsView
{
public:
  TriangleComponentsView() noexcept = default;

  explicit TriangleComponentsView(const ::occtl_mesh_triangle_components_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const uint32_t> triangle_component_ids() const noexcept
  {
    return {myC.triangle_component_ids, myC.triangle_count};
  }

  [[nodiscard]] std::span<const uint32_t> component_sizes() const noexcept
  {
    return {myC.component_sizes, myC.component_count};
  }
#else
  [[nodiscard]] const uint32_t* triangle_component_ids() const noexcept
  {
    return myC.triangle_component_ids;
  }

  [[nodiscard]] const uint32_t* component_sizes() const noexcept { return myC.component_sizes; }
#endif

  [[nodiscard]] std::size_t triangle_count() const noexcept { return myC.triangle_count; }

  [[nodiscard]] std::size_t component_count() const noexcept { return myC.component_count; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_components_view_t& c() const noexcept { return myC; }

private:
  ::occtl_mesh_triangle_components_view_t myC{};
};

/// @brief Borrowed aggregate triangle indices belonging to one component.
class TriangleComponentTrianglesView
{
public:
  TriangleComponentTrianglesView() noexcept = default;

  explicit TriangleComponentTrianglesView(
    const ::occtl_mesh_triangle_component_triangles_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const uint32_t> triangles() const noexcept
  {
    return {myC.triangles, myC.triangle_count};
  }
#else
  [[nodiscard]] const uint32_t* triangles() const noexcept { return myC.triangles; }
#endif

  [[nodiscard]] std::size_t triangle_count() const noexcept { return myC.triangle_count; }

  [[nodiscard]] uint32_t component_id() const noexcept { return myC.component_id; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_component_triangles_view_t& c() const noexcept
  {
    return myC;
  }

private:
  ::occtl_mesh_triangle_component_triangles_view_t myC{};
};

/// @brief Borrowed boundary edges belonging to one triangle component.
class TriangleComponentBoundaryView
{
public:
  TriangleComponentBoundaryView() noexcept = default;

  explicit TriangleComponentBoundaryView(
    const ::occtl_mesh_triangle_component_boundary_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const ::occtl_mesh_triangle_component_boundary_edge_t> edges()
    const noexcept
  {
    return {myC.edges, myC.edge_count};
  }
#else
  [[nodiscard]] const ::occtl_mesh_triangle_component_boundary_edge_t* edges() const noexcept
  {
    return myC.edges;
  }
#endif

  [[nodiscard]] std::size_t edge_count() const noexcept { return myC.edge_count; }

  [[nodiscard]] uint32_t component_id() const noexcept { return myC.component_id; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_component_boundary_view_t& c() const noexcept
  {
    return myC;
  }

private:
  ::occtl_mesh_triangle_component_boundary_view_t myC{};
};

/// @brief Borrowed ordered boundary chains belonging to one triangle component.
class TriangleComponentBoundaryChainsView
{
public:
  TriangleComponentBoundaryChainsView() noexcept = default;

  explicit TriangleComponentBoundaryChainsView(
    const ::occtl_mesh_triangle_component_boundary_chains_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const ::occtl_mesh_triangle_component_boundary_edge_t> edges()
    const noexcept
  {
    return {myC.edges, myC.edge_count};
  }

  [[nodiscard]] std::span<const ::occtl_mesh_triangle_component_boundary_chain_t> chains()
    const noexcept
  {
    return {myC.chains, myC.chain_count};
  }
#else
  [[nodiscard]] const ::occtl_mesh_triangle_component_boundary_edge_t* edges() const noexcept
  {
    return myC.edges;
  }

  [[nodiscard]] const ::occtl_mesh_triangle_component_boundary_chain_t* chains() const noexcept
  {
    return myC.chains;
  }
#endif

  [[nodiscard]] std::size_t edge_count() const noexcept { return myC.edge_count; }

  [[nodiscard]] std::size_t chain_count() const noexcept { return myC.chain_count; }

  [[nodiscard]] uint32_t component_id() const noexcept { return myC.component_id; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_component_boundary_chains_view_t& c() const noexcept
  {
    return myC;
  }

private:
  ::occtl_mesh_triangle_component_boundary_chains_view_t myC{};
};

/// @brief Borrowed ordered boundary polylines belonging to one triangle component.
class TriangleComponentBoundaryPolylinesView
{
public:
  TriangleComponentBoundaryPolylinesView() noexcept = default;

  explicit TriangleComponentBoundaryPolylinesView(
    const ::occtl_mesh_component_boundary_polylines_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const ::occtl_point3_t> points() const noexcept
  {
    return {myC.points, myC.point_count};
  }

  [[nodiscard]] std::span<const ::occtl_mesh_component_boundary_polyline_t> polylines()
    const noexcept
  {
    return {myC.polylines, myC.polyline_count};
  }
#else
  [[nodiscard]] const ::occtl_point3_t* points() const noexcept { return myC.points; }

  [[nodiscard]] const ::occtl_mesh_component_boundary_polyline_t* polylines() const noexcept
  {
    return myC.polylines;
  }
#endif

  [[nodiscard]] std::size_t point_count() const noexcept { return myC.point_count; }

  [[nodiscard]] std::size_t polyline_count() const noexcept { return myC.polyline_count; }

  [[nodiscard]] uint32_t component_id() const noexcept { return myC.component_id; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_component_boundary_polylines_view_t& c() const noexcept
  {
    return myC;
  }

private:
  ::occtl_mesh_component_boundary_polylines_view_t myC{};
};

/// @brief Borrowed summaries for normal-connected triangle components.
class TriangleComponentSummariesView
{
public:
  TriangleComponentSummariesView() noexcept = default;

  explicit TriangleComponentSummariesView(
    const ::occtl_mesh_triangle_component_summaries_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const ::occtl_mesh_triangle_component_summary_t> summaries()
    const noexcept
  {
    return {myC.summaries, myC.component_count};
  }
#else
  [[nodiscard]] const ::occtl_mesh_triangle_component_summary_t* summaries() const noexcept
  {
    return myC.summaries;
  }
#endif

  [[nodiscard]] std::size_t component_count() const noexcept { return myC.component_count; }

  [[nodiscard]] std::size_t triangle_count() const noexcept { return myC.triangle_count; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_component_summaries_view_t& c() const noexcept
  {
    return myC;
  }

private:
  ::occtl_mesh_triangle_component_summaries_view_t myC{};
};

using TrianglePlaneComponentsOptions = ::occtl_mesh_triangle_plane_components_options_t;

/// @brief Borrowed plane-like normal-connected triangle components.
class TrianglePlaneComponentsView
{
public:
  TrianglePlaneComponentsView() noexcept = default;

  explicit TrianglePlaneComponentsView(
    const ::occtl_mesh_triangle_plane_components_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const ::occtl_mesh_triangle_plane_component_t> components() const noexcept
  {
    return {myC.components, myC.component_count};
  }
#else
  [[nodiscard]] const ::occtl_mesh_triangle_plane_component_t* components() const noexcept
  {
    return myC.components;
  }
#endif

  [[nodiscard]] std::size_t component_count() const noexcept { return myC.component_count; }

  [[nodiscard]] std::size_t triangle_count() const noexcept { return myC.triangle_count; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_plane_components_view_t& c() const noexcept
  {
    return myC;
  }

private:
  ::occtl_mesh_triangle_plane_components_view_t myC{};
};

using TriangleSphereComponentsOptions = ::occtl_mesh_triangle_sphere_components_options_t;

/// @brief Borrowed sphere-like normal-connected triangle components.
class TriangleSphereComponentsView
{
public:
  TriangleSphereComponentsView() noexcept = default;

  explicit TriangleSphereComponentsView(
    const ::occtl_mesh_triangle_sphere_components_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const ::occtl_mesh_triangle_sphere_component_t> components()
    const noexcept
  {
    return {myC.components, myC.component_count};
  }
#else
  [[nodiscard]] const ::occtl_mesh_triangle_sphere_component_t* components() const noexcept
  {
    return myC.components;
  }
#endif

  [[nodiscard]] std::size_t component_count() const noexcept { return myC.component_count; }

  [[nodiscard]] std::size_t triangle_count() const noexcept { return myC.triangle_count; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_sphere_components_view_t& c() const noexcept
  {
    return myC;
  }

private:
  ::occtl_mesh_triangle_sphere_components_view_t myC{};
};

using TriangleCylinderComponentsOptions = ::occtl_mesh_triangle_cylinder_components_options_t;

/// @brief Borrowed cylinder-like normal-connected triangle components.
class TriangleCylinderComponentsView
{
public:
  TriangleCylinderComponentsView() noexcept = default;

  explicit TriangleCylinderComponentsView(
    const ::occtl_mesh_triangle_cylinder_components_view_t& theC) noexcept
      : myC(theC)
  {
  }

#if OCCTL_HPP_HAS_SPAN
  [[nodiscard]] std::span<const ::occtl_mesh_triangle_cylinder_component_t> components()
    const noexcept
  {
    return {myC.components, myC.component_count};
  }
#else
  [[nodiscard]] const ::occtl_mesh_triangle_cylinder_component_t* components() const noexcept
  {
    return myC.components;
  }
#endif

  [[nodiscard]] std::size_t component_count() const noexcept { return myC.component_count; }

  [[nodiscard]] std::size_t triangle_count() const noexcept { return myC.triangle_count; }

  [[nodiscard]] NodeId root() const noexcept { return NodeId(myC.root); }

  [[nodiscard]] const ::occtl_mesh_triangle_cylinder_components_view_t& c() const noexcept
  {
    return myC;
  }

private:
  ::occtl_mesh_triangle_cylinder_components_view_t myC{};
};

/// @brief Tessellate every active face and free edge in @p theGraph.
inline void generate(Graph& theGraph, const Options& theOpts)
{
  ::occtl_mesh_options_t aOpts = theOpts.to_c();
  check(::occtl_mesh_generate(theGraph.get(), nullptr, 0, &aOpts));
}

/// @brief Tessellate the subtree(s) rooted at the supplied nodes.
inline void generate(Graph& theGraph, const std::vector<NodeId>& theNodes, const Options& theOpts)
{
  std::vector<::occtl_node_id_t> aIds;
  aIds.reserve(theNodes.size());
  for (const NodeId& aId : theNodes)
  {
    aIds.emplace_back(aId.get());
  }

  ::occtl_mesh_options_t aOpts = theOpts.to_c();
  check(::occtl_mesh_generate(theGraph.get(),
                              aIds.empty() ? nullptr : aIds.data(),
                              aIds.size(),
                              &aOpts));
}

#if OCCTL_HPP_HAS_SPAN
/// @brief Tessellate the subtree(s) rooted at the supplied nodes (span overload).
inline void generate(Graph& theGraph, std::span<const NodeId> theNodes, const Options& theOpts)
{
  std::vector<::occtl_node_id_t> aIds;
  aIds.reserve(theNodes.size());
  for (const NodeId& aId : theNodes)
    aIds.emplace_back(aId.get());

  ::occtl_mesh_options_t aOpts = theOpts.to_c();
  check(::occtl_mesh_generate(theGraph.get(),
                              aIds.empty() ? nullptr : aIds.data(),
                              aIds.size(),
                              &aOpts));
}
#endif

/// @brief Creates a new graph containing one triangulated Face from caller buffers.
[[nodiscard]] inline std::pair<Graph, NodeId> from_buffers(const FromBuffersOptions& theOptions)
{
  ::occtl_graph_t*  aGraph = nullptr;
  ::occtl_node_id_t aRoot  = OCCTL_NODE_ID_INVALID;
  check(::occtl_mesh_from_buffers(&theOptions, &aGraph, &aRoot));
  return {Graph(aGraph), NodeId(aRoot)};
}

/// @brief Sets mesh-model metadata on the graph metadata storage.
inline void model_metadata_set(Graph&            theGraph,
                               const char* const theKey,
                               const std::size_t theKeyLen,
                               const char* const theValue,
                               const std::size_t theValueLen)
{
  check(::occtl_mesh_model_metadata_set(theGraph.get(), theKey, theKeyLen, theValue, theValueLen));
}

/// @brief Retrieves mesh-model metadata from the graph metadata storage.
[[nodiscard]] inline std::string model_metadata_get(const Graph&      theGraph,
                                                    const char* const theKey,
                                                    const std::size_t theKeyLen)
{
  std::size_t aRequired = 0;
  check(::occtl_mesh_model_metadata_get(theGraph.get(), theKey, theKeyLen, nullptr, 0, &aRequired));
  if (aRequired <= 1)
  {
    return {};
  }
  std::string aResult(aRequired - 1, '\0');
  check(::occtl_mesh_model_metadata_get(theGraph.get(),
                                        theKey,
                                        theKeyLen,
                                        aResult.data(),
                                        aResult.size() + 1,
                                        &aRequired));
  return aResult;
}

/// @brief Lists mesh-model metadata keys from the graph metadata storage.
[[nodiscard]] inline std::vector<std::string> model_metadata_keys(const Graph& theGraph)
{
  std::size_t aCount = 0;
  check(::occtl_mesh_model_metadata_keys(theGraph.get(), nullptr, 0, &aCount));

  std::vector<::occtl_metadata_key_view_t> aViews(aCount);
  if (!aViews.empty())
  {
    check(::occtl_mesh_model_metadata_keys(theGraph.get(), aViews.data(), aViews.size(), &aCount));
  }

  std::vector<std::string> aKeys;
  aKeys.reserve(aCount);
  for (const ::occtl_metadata_key_view_t& aView : aViews)
  {
    aKeys.emplace_back(aView.key, aView.key_len);
  }
  return aKeys;
}

/// @brief Removes one mesh-model metadata key. Idempotent.
inline void model_metadata_unset(Graph&            theGraph,
                                 const char* const theKey,
                                 const std::size_t theKeyLen)
{
  check(::occtl_mesh_model_metadata_unset(theGraph.get(), theKey, theKeyLen));
}

/// @brief Active cached triangulation of @p theFace.
[[nodiscard]] inline TriangulationView face_triangulation(const Graph& theGraph,
                                                          const NodeId theFace)
{
  ::occtl_triangulation_view_t aView{};
  check(::occtl_mesh_face_triangulation(theGraph.get(), theFace.get(), &aView));
  return TriangulationView(aView);
}

/// @brief Number of cached triangulations on @p theFace.
[[nodiscard]] inline uint32_t face_triangulation_count(const Graph& theGraph, const NodeId theFace)
{
  uint32_t aCount = 0;
  check(::occtl_mesh_face_triangulation_count(theGraph.get(), theFace.get(), &aCount));
  return aCount;
}

/// @brief @p theIndex-th cached triangulation of @p theFace.
[[nodiscard]] inline TriangulationView face_triangulation_at(const Graph&   theGraph,
                                                             const NodeId   theFace,
                                                             const uint32_t theIndex)
{
  ::occtl_triangulation_view_t aView{};
  check(::occtl_mesh_face_triangulation_indexed(theGraph.get(), theFace.get(), theIndex, &aView));
  return TriangulationView(aView);
}

/// @brief Cached 3D polyline on @p theEdge.
[[nodiscard]] inline Polygon3DView edge_polygon3d(const Graph& theGraph, const NodeId theEdge)
{
  ::occtl_polygon3d_view_t aView{};
  check(::occtl_mesh_edge_polygon3d(theGraph.get(), theEdge.get(), &aView));
  return Polygon3DView(aView);
}

/// @brief Cached polygon-on-triangulation for @p theCoEdge.
[[nodiscard]] inline PolygonOnTriView coedge_polygon_on_tri(const Graph& theGraph,
                                                            const NodeId theCoEdge)
{
  ::occtl_polygon_on_tri_view_t aView{};
  check(::occtl_mesh_coedge_polygon_on_tri(theGraph.get(), theCoEdge.get(), &aView));
  return PolygonOnTriView(aView);
}

/// @brief Aggregate cached triangulations into graph-owned triangle-soup buffers.
[[nodiscard]] inline TriangleBuffersView triangle_buffers(const Graph& theGraph,
                                                          const NodeId theRoot = NodeId::invalid())
{
  ::occtl_mesh_triangle_buffers_view_t aView{};
  check(::occtl_mesh_triangle_buffers(theGraph.get(), theRoot.get(), &aView));
  return TriangleBuffersView(aView);
}

/// @brief Cached per-triangle normals and adjacency for graph-owned mesh buffers.
[[nodiscard]] inline TriangleAnalysisView triangle_analysis(
  const Graph& theGraph,
  const NodeId theRoot = NodeId::invalid())
{
  ::occtl_mesh_triangle_analysis_view_t aView{};
  check(::occtl_mesh_triangle_analysis(theGraph.get(), theRoot.get(), &aView));
  return TriangleAnalysisView(aView);
}

/// @brief Normal-connected triangle components for graph-owned mesh buffers.
[[nodiscard]] inline TriangleComponentsView triangle_components(
  const Graph&                     theGraph,
  const TriangleComponentsOptions& theOptions)
{
  ::occtl_mesh_triangle_components_view_t aView{};
  check(::occtl_mesh_triangle_components(theGraph.get(), &theOptions, &aView));
  return TriangleComponentsView(aView);
}

/// @brief Normal-connected triangle components with default tolerance.
[[nodiscard]] inline TriangleComponentsView triangle_components(
  const Graph& theGraph,
  const NodeId theRoot = NodeId::invalid())
{
  TriangleComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                      = theRoot.get();
  return triangle_components(theGraph, anOptions);
}

/// @brief Summaries for normal-connected triangle components.
[[nodiscard]] inline TriangleComponentSummariesView triangle_component_summaries(
  const Graph&                     theGraph,
  const TriangleComponentsOptions& theOptions)
{
  ::occtl_mesh_triangle_component_summaries_view_t aView{};
  check(::occtl_mesh_triangle_component_summaries(theGraph.get(), &theOptions, &aView));
  return TriangleComponentSummariesView(aView);
}

/// @brief Summaries for normal-connected triangle components with default tolerance.
[[nodiscard]] inline TriangleComponentSummariesView triangle_component_summaries(
  const Graph& theGraph,
  const NodeId theRoot = NodeId::invalid())
{
  TriangleComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                      = theRoot.get();
  return triangle_component_summaries(theGraph, anOptions);
}

/// @brief Aggregate triangle indices belonging to one component.
[[nodiscard]] inline TriangleComponentTrianglesView triangle_component_triangles(
  const Graph&                     theGraph,
  const TriangleComponentsOptions& theOptions,
  const uint32_t                   theComponentId)
{
  ::occtl_mesh_triangle_component_triangles_view_t aView{};
  check(
    ::occtl_mesh_triangle_component_triangles(theGraph.get(), &theOptions, theComponentId, &aView));
  return TriangleComponentTrianglesView(aView);
}

/// @brief Aggregate triangle indices for one component with default tolerance.
[[nodiscard]] inline TriangleComponentTrianglesView triangle_component_triangles(
  const Graph&   theGraph,
  const uint32_t theComponentId,
  const NodeId   theRoot = NodeId::invalid())
{
  TriangleComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                      = theRoot.get();
  return triangle_component_triangles(theGraph, anOptions, theComponentId);
}

/// @brief Boundary edges belonging to one triangle component.
[[nodiscard]] inline TriangleComponentBoundaryView triangle_component_boundary(
  const Graph&                     theGraph,
  const TriangleComponentsOptions& theOptions,
  const uint32_t                   theComponentId)
{
  ::occtl_mesh_triangle_component_boundary_view_t aView{};
  check(
    ::occtl_mesh_triangle_component_boundary(theGraph.get(), &theOptions, theComponentId, &aView));
  return TriangleComponentBoundaryView(aView);
}

/// @brief Boundary edges for one component with default tolerance.
[[nodiscard]] inline TriangleComponentBoundaryView triangle_component_boundary(
  const Graph&   theGraph,
  const uint32_t theComponentId,
  const NodeId   theRoot = NodeId::invalid())
{
  TriangleComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                      = theRoot.get();
  return triangle_component_boundary(theGraph, anOptions, theComponentId);
}

/// @brief Ordered boundary chains belonging to one triangle component.
[[nodiscard]] inline TriangleComponentBoundaryChainsView triangle_component_boundary_chains(
  const Graph&                     theGraph,
  const TriangleComponentsOptions& theOptions,
  const uint32_t                   theComponentId)
{
  ::occtl_mesh_triangle_component_boundary_chains_view_t aView{};
  check(::occtl_mesh_triangle_component_boundary_chains(theGraph.get(),
                                                        &theOptions,
                                                        theComponentId,
                                                        &aView));
  return TriangleComponentBoundaryChainsView(aView);
}

/// @brief Ordered boundary chains for one component with default tolerance.
[[nodiscard]] inline TriangleComponentBoundaryChainsView triangle_component_boundary_chains(
  const Graph&   theGraph,
  const uint32_t theComponentId,
  const NodeId   theRoot = NodeId::invalid())
{
  TriangleComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                      = theRoot.get();
  return triangle_component_boundary_chains(theGraph, anOptions, theComponentId);
}

/// @brief Ordered boundary polylines belonging to one triangle component.
[[nodiscard]] inline TriangleComponentBoundaryPolylinesView triangle_component_boundary_polylines(
  const Graph&                     theGraph,
  const TriangleComponentsOptions& theOptions,
  const uint32_t                   theComponentId)
{
  ::occtl_mesh_component_boundary_polylines_view_t aView{};
  check(
    ::occtl_mesh_component_boundary_polylines(theGraph.get(), &theOptions, theComponentId, &aView));
  return TriangleComponentBoundaryPolylinesView(aView);
}

/// @brief Ordered boundary polylines for one component with default tolerance.
[[nodiscard]] inline TriangleComponentBoundaryPolylinesView triangle_component_boundary_polylines(
  const Graph&   theGraph,
  const uint32_t theComponentId,
  const NodeId   theRoot = NodeId::invalid())
{
  TriangleComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                      = theRoot.get();
  return triangle_component_boundary_polylines(theGraph, anOptions, theComponentId);
}

/// @brief Plane-like normal-connected triangle components.
[[nodiscard]] inline TrianglePlaneComponentsView triangle_plane_components(
  const Graph&                          theGraph,
  const TrianglePlaneComponentsOptions& theOptions)
{
  ::occtl_mesh_triangle_plane_components_view_t aView{};
  check(::occtl_mesh_triangle_plane_components(theGraph.get(), &theOptions, &aView));
  return TrianglePlaneComponentsView(aView);
}

/// @brief Plane-like normal-connected triangle components with default tolerance.
[[nodiscard]] inline TrianglePlaneComponentsView triangle_plane_components(
  const Graph& theGraph,
  const NodeId theRoot = NodeId::invalid())
{
  TrianglePlaneComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                           = theRoot.get();
  return triangle_plane_components(theGraph, anOptions);
}

/// @brief Sphere-like normal-connected triangle components.
[[nodiscard]] inline TriangleSphereComponentsView triangle_sphere_components(
  const Graph&                           theGraph,
  const TriangleSphereComponentsOptions& theOptions)
{
  ::occtl_mesh_triangle_sphere_components_view_t aView{};
  check(::occtl_mesh_triangle_sphere_components(theGraph.get(), &theOptions, &aView));
  return TriangleSphereComponentsView(aView);
}

/// @brief Sphere-like normal-connected triangle components with default tolerance.
[[nodiscard]] inline TriangleSphereComponentsView triangle_sphere_components(
  const Graph& theGraph,
  const NodeId theRoot = NodeId::invalid())
{
  TriangleSphereComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                            = theRoot.get();
  return triangle_sphere_components(theGraph, anOptions);
}

/// @brief Cylinder-like normal-connected triangle components.
[[nodiscard]] inline TriangleCylinderComponentsView triangle_cylinder_components(
  const Graph&                             theGraph,
  const TriangleCylinderComponentsOptions& theOptions)
{
  ::occtl_mesh_triangle_cylinder_components_view_t aView{};
  check(::occtl_mesh_triangle_cylinder_components(theGraph.get(), &theOptions, &aView));
  return TriangleCylinderComponentsView(aView);
}

/// @brief Cylinder-like normal-connected triangle components with default tolerance.
[[nodiscard]] inline TriangleCylinderComponentsView triangle_cylinder_components(
  const Graph& theGraph,
  const NodeId theRoot = NodeId::invalid())
{
  TriangleCylinderComponentsOptions anOptions =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  anOptions.root = theRoot.get();
  return triangle_cylinder_components(theGraph, anOptions);
}

/// @brief Rebuild one sphere-like triangle component as an analytic Solid root.
[[nodiscard]] inline NodeId make_sphere_component_solid(
  Graph&                                 theGraph,
  const TriangleSphereComponentsOptions& theOptions,
  const uint32_t                         theComponentId)
{
  ::occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  check(
    ::occtl_mesh_make_sphere_component_solid(theGraph.get(), &theOptions, theComponentId, &aSolid));
  return NodeId(aSolid);
}

/// @brief Rebuild one sphere-like triangle component as an analytic Solid root with default
/// tolerance.
[[nodiscard]] inline NodeId make_sphere_component_solid(Graph&         theGraph,
                                                        const uint32_t theComponentId,
                                                        const NodeId   theRoot = NodeId::invalid())
{
  TriangleSphereComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                            = theRoot.get();
  return make_sphere_component_solid(theGraph, anOptions, theComponentId);
}

/// @brief Rebuild one cylinder-like triangle component as an analytic Solid root.
[[nodiscard]] inline NodeId make_cylinder_component_solid(
  Graph&                                   theGraph,
  const TriangleCylinderComponentsOptions& theOptions,
  const uint32_t                           theComponentId)
{
  ::occtl_node_id_t aSolid = OCCTL_NODE_ID_INVALID;
  check(::occtl_mesh_make_cylinder_component_solid(theGraph.get(),
                                                   &theOptions,
                                                   theComponentId,
                                                   &aSolid));
  return NodeId(aSolid);
}

/// @brief Rebuild one cylinder-like triangle component as an analytic Solid root with default
/// tolerance.
[[nodiscard]] inline NodeId make_cylinder_component_solid(Graph&         theGraph,
                                                          const uint32_t theComponentId,
                                                          const NodeId theRoot = NodeId::invalid())
{
  TriangleCylinderComponentsOptions anOptions =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  anOptions.root = theRoot.get();
  return make_cylinder_component_solid(theGraph, anOptions, theComponentId);
}

/// @brief Rebuild all sphere-like triangle components as analytic Solid roots.
[[nodiscard]] inline std::vector<NodeId> make_sphere_component_solids(
  Graph&                                 theGraph,
  const TriangleSphereComponentsOptions& theOptions)
{
  size_t aCount = 0u;
  check(
    ::occtl_mesh_make_sphere_component_solids(theGraph.get(), &theOptions, nullptr, 0u, &aCount));

  std::vector<::occtl_node_id_t> aCSolids(aCount, OCCTL_NODE_ID_INVALID);
  if (aCount != 0u)
  {
    check(::occtl_mesh_make_sphere_component_solids(theGraph.get(),
                                                    &theOptions,
                                                    aCSolids.data(),
                                                    aCSolids.size(),
                                                    &aCount));
  }

  std::vector<NodeId> aSolids;
  aSolids.reserve(aCount);
  for (const ::occtl_node_id_t& aSolid : aCSolids)
  {
    aSolids.emplace_back(aSolid);
  }
  return aSolids;
}

/// @brief Rebuild all sphere-like triangle components as analytic Solid roots with default
/// tolerance.
[[nodiscard]] inline std::vector<NodeId> make_sphere_component_solids(
  Graph&       theGraph,
  const NodeId theRoot = NodeId::invalid())
{
  TriangleSphereComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                            = theRoot.get();
  return make_sphere_component_solids(theGraph, anOptions);
}

/// @brief Rebuild all cylinder-like triangle components as analytic Solid roots.
[[nodiscard]] inline std::vector<NodeId> make_cylinder_component_solids(
  Graph&                                   theGraph,
  const TriangleCylinderComponentsOptions& theOptions)
{
  size_t aCount = 0u;
  check(
    ::occtl_mesh_make_cylinder_component_solids(theGraph.get(), &theOptions, nullptr, 0u, &aCount));

  std::vector<::occtl_node_id_t> aCSolids(aCount, OCCTL_NODE_ID_INVALID);
  if (aCount != 0u)
  {
    check(::occtl_mesh_make_cylinder_component_solids(theGraph.get(),
                                                      &theOptions,
                                                      aCSolids.data(),
                                                      aCSolids.size(),
                                                      &aCount));
  }

  std::vector<NodeId> aSolids;
  aSolids.reserve(aCount);
  for (const ::occtl_node_id_t& aSolid : aCSolids)
  {
    aSolids.emplace_back(aSolid);
  }
  return aSolids;
}

/// @brief Rebuild all cylinder-like triangle components as analytic Solid roots with default
/// tolerance.
[[nodiscard]] inline std::vector<NodeId> make_cylinder_component_solids(
  Graph&       theGraph,
  const NodeId theRoot = NodeId::invalid())
{
  TriangleCylinderComponentsOptions anOptions =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  anOptions.root = theRoot.get();
  return make_cylinder_component_solids(theGraph, anOptions);
}

/// @brief Rebuild one plane-like triangle component as a planar Face root.
[[nodiscard]] inline NodeId make_plane_component_face(
  Graph&                                theGraph,
  const TrianglePlaneComponentsOptions& theOptions,
  const uint32_t                        theComponentId)
{
  ::occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  check(
    ::occtl_mesh_make_plane_component_face(theGraph.get(), &theOptions, theComponentId, &aFace));
  return NodeId(aFace);
}

/// @brief Rebuild one plane-like triangle component as a planar Face root with default tolerance.
[[nodiscard]] inline NodeId make_plane_component_face(Graph&         theGraph,
                                                      const uint32_t theComponentId,
                                                      const NodeId   theRoot = NodeId::invalid())
{
  TrianglePlaneComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                           = theRoot.get();
  return make_plane_component_face(theGraph, anOptions, theComponentId);
}

/// @brief Rebuild all plane-like triangle components as planar Face roots.
[[nodiscard]] inline std::vector<NodeId> make_plane_component_faces(
  Graph&                                theGraph,
  const TrianglePlaneComponentsOptions& theOptions)
{
  size_t aCount = 0u;
  check(::occtl_mesh_make_plane_component_faces(theGraph.get(), &theOptions, nullptr, 0u, &aCount));

  std::vector<::occtl_node_id_t> aCFaces(aCount, OCCTL_NODE_ID_INVALID);
  if (aCount != 0u)
  {
    check(::occtl_mesh_make_plane_component_faces(theGraph.get(),
                                                  &theOptions,
                                                  aCFaces.data(),
                                                  aCFaces.size(),
                                                  &aCount));
  }

  std::vector<NodeId> aFaces;
  aFaces.reserve(aCount);
  for (const ::occtl_node_id_t& aFace : aCFaces)
  {
    aFaces.emplace_back(aFace);
  }
  return aFaces;
}

/// @brief Rebuild all plane-like triangle components as planar Face roots with default tolerance.
[[nodiscard]] inline std::vector<NodeId> make_plane_component_faces(
  Graph&       theGraph,
  const NodeId theRoot = NodeId::invalid())
{
  TrianglePlaneComponentsOptions anOptions = OCCTL_MESH_TRIANGLE_PLANE_COMPONENTS_OPTIONS_INIT;
  anOptions.root                           = theRoot.get();
  return make_plane_component_faces(theGraph, anOptions);
}

} // namespace occtl::mesh

#endif // OCCTL_HPP_MESH_HPP
