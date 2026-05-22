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

#include "test_mesh_helpers.hxx"

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/mesh.hpp>
#include <occtl-hpp/topo.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace
{

class MeshVeneerTest : public ::testing::Test
{
protected:
  occtl::Graph make_graph()
  {
    ::occtl_graph_t* aRaw = nullptr;
    occtl::check(::occtl_graph_create(&aRaw));
    return occtl::Graph(aRaw);
  }

  occtl::NodeId make_box(occtl::Graph& theGraph,
                         const double  theDx,
                         const double  theDy,
                         const double  theDz)
  {
    const ::occtl_node_id_t aId = mesh_test::makeBox(theGraph.get(), theDx, theDy, theDz);
    return occtl::NodeId(aId);
  }

  occtl::NodeId make_sphere(occtl::Graph& theGraph, const double theRadius)
  {
    const ::occtl_node_id_t aId = mesh_test::makeSphere(theGraph.get(), theRadius);
    return occtl::NodeId(aId);
  }

  occtl::NodeId make_cylinder(occtl::Graph& theGraph,
                              const double  theRadius,
                              const double  theHeight)
  {
    const ::occtl_node_id_t aId = mesh_test::makeCylinder(theGraph.get(), theRadius, theHeight);
    return occtl::NodeId(aId);
  }

  occtl::NodeId first_face(occtl::Graph& theGraph)
  {
    ::occtl_node_iter_t* aIter   = nullptr;
    ::occtl_node_id_t    aFaceId = OCCTL_NODE_ID_INVALID;
    EXPECT_EQ(::occtl_graph_face_iter_create(theGraph.get(), &aIter), OCCTL_OK);
    ::occtl_node_iter_next(aIter, &aFaceId);
    ::occtl_node_iter_free(aIter);
    return occtl::NodeId(aFaceId);
  }
};

TEST_F(MeshVeneerTest, OptionsDefaults_MatchCInit)
{
  occtl::mesh::Options   aOpts;
  ::occtl_mesh_options_t aC = aOpts.to_c();
  EXPECT_EQ(aC.struct_version, OCCTL_MESH_OPTIONS_VERSION_1);
  EXPECT_EQ(aC.use_bbox, 0);
  EXPECT_DOUBLE_EQ(aC.deflection, 0.001);
}

TEST_F(MeshVeneerTest, OptionsToC_AllFieldsForwarded)
{
  // Set every field of the C++ aggregate to a distinctive non-default
  // value, then verify each lands on the projected C struct. Catches
  // additions to Options that forget to mirror to_c().
  occtl::mesh::Options aOpts;
  aOpts.deflection                     = 0.5;
  aOpts.angle                          = 0.6;
  aOpts.deflection_interior            = 0.7;
  aOpts.angle_interior                 = 0.8;
  aOpts.min_size                       = 0.001;
  aOpts.in_parallel                    = true;
  aOpts.relative                       = true;
  aOpts.internal_vertices_mode         = false;
  aOpts.control_surface_deflection     = false;
  aOpts.control_surface_deflection_all = true;
  aOpts.clean_model                    = false;
  aOpts.adjust_min_size                = true;
  aOpts.force_face_deflection          = true;
  aOpts.allow_quality_decrease         = true;
  aOpts.deviation_coefficient          = 0.42;
  aOpts.deviation_angle                = 0.43;
  aOpts.bbox = occtl::mesh::AABB3{occtl::Point3{1.0, 2.0, 3.0}, occtl::Point3{4.0, 5.0, 6.0}};

  const ::occtl_mesh_options_t aC = aOpts.to_c();
  EXPECT_DOUBLE_EQ(aC.deflection, 0.5);
  EXPECT_DOUBLE_EQ(aC.angle, 0.6);
  EXPECT_DOUBLE_EQ(aC.deflection_interior, 0.7);
  EXPECT_DOUBLE_EQ(aC.angle_interior, 0.8);
  EXPECT_DOUBLE_EQ(aC.min_size, 0.001);
  EXPECT_EQ(aC.in_parallel, 1);
  EXPECT_EQ(aC.relative, 1);
  EXPECT_EQ(aC.internal_vertices_mode, 0);
  EXPECT_EQ(aC.control_surface_deflection, 0);
  EXPECT_EQ(aC.control_surface_deflection_all, 1);
  EXPECT_EQ(aC.clean_model, 0);
  EXPECT_EQ(aC.adjust_min_size, 1);
  EXPECT_EQ(aC.force_face_deflection, 1);
  EXPECT_EQ(aC.allow_quality_decrease, 1);
  EXPECT_DOUBLE_EQ(aC.deviation_coefficient, 0.42);
  EXPECT_DOUBLE_EQ(aC.deviation_angle, 0.43);
  EXPECT_EQ(aC.use_bbox, 1);
  EXPECT_DOUBLE_EQ(aC.bbox.min.x, 1.0);
  EXPECT_DOUBLE_EQ(aC.bbox.max.z, 6.0);
}

TEST_F(MeshVeneerTest, ModelMetadata_SetGetUnset_ReturnsValue)
{
  occtl::Graph aGraph = make_graph();

  occtl::mesh::model_metadata_set(aGraph, "author", 6, "unit-test", 9);
  occtl::mesh::model_metadata_set(aGraph, "source", 6, "mesh", 4);

  EXPECT_EQ(occtl::mesh::model_metadata_get(aGraph, "author", 6), "unit-test");

  const std::vector<std::string> aKeys = occtl::mesh::model_metadata_keys(aGraph);
  EXPECT_NE(std::find(aKeys.begin(), aKeys.end(), "author"), aKeys.end());
  EXPECT_NE(std::find(aKeys.begin(), aKeys.end(), "source"), aKeys.end());

  occtl::mesh::model_metadata_unset(aGraph, "author", 6);
  EXPECT_THROW((void)occtl::mesh::model_metadata_get(aGraph, "author", 6), occtl::Error);
}

TEST_F(MeshVeneerTest, TriangulationView_SpanSize_MatchesCount)
{
  occtl::Graph aGraph = make_graph();
  ASSERT_TRUE(make_box(aGraph, 10.0, 10.0, 10.0).is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::NodeId            aFace = first_face(aGraph);
  occtl::mesh::TriangulationView aView = occtl::mesh::face_triangulation(aGraph, aFace);

#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.nodes().size(), aView.node_count() * 3u);
  EXPECT_EQ(aView.triangles().size(), aView.triangle_count() * 3u);
#else
  // C++17 fallback returns raw pointers; just confirm they are non-null.
  EXPECT_NE(aView.nodes(), nullptr);
  EXPECT_NE(aView.triangles(), nullptr);
#endif
}

TEST_F(MeshVeneerTest, GenerateBoxAndFetchTriangulationView_Roundtrips)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  EXPECT_TRUE(aBox.is_valid());

  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::NodeId            aFace = first_face(aGraph);
  occtl::mesh::TriangulationView aView = occtl::mesh::face_triangulation(aGraph, aFace);

  EXPECT_GT(aView.node_count(), 0u);
  EXPECT_GT(aView.triangle_count(), 0u);
  EXPECT_NE(aView.nodes(), nullptr);
  EXPECT_NE(aView.triangles(), nullptr);

  // 0-indexed contract — every triangle vertex must be in [0, node_count).
  const std::size_t aNbTriIndices = aView.triangle_count() * 3;
  for (std::size_t i = 0; i < aNbTriIndices; ++i)
  {
    EXPECT_LT(aView.triangles()[i], static_cast<uint32_t>(aView.node_count()));
  }
}

TEST_F(MeshVeneerTest, TriangleBuffers_BoxGraph_ReturnsSoupView)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::mesh::TriangleBuffersView aView = occtl::mesh::triangle_buffers(aGraph);

  EXPECT_EQ(aView.face_count(), 6u);
  EXPECT_EQ(aView.node_count(), 24u);
  EXPECT_EQ(aView.triangle_count(), 12u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.nodes().size(), aView.node_count() * 3u);
  EXPECT_EQ(aView.triangles().size(), aView.triangle_count() * 3u);
#else
  EXPECT_NE(aView.nodes(), nullptr);
  EXPECT_NE(aView.triangles(), nullptr);
#endif
}

TEST_F(MeshVeneerTest, TriangleAnalysis_BoxGraph_ReturnsAnalysisView)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::mesh::TriangleAnalysisView aView = occtl::mesh::triangle_analysis(aGraph);

  EXPECT_EQ(aView.face_count(), 6u);
  EXPECT_EQ(aView.triangle_count(), 12u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.triangle_normals().size(), aView.triangle_count() * 3u);
  EXPECT_EQ(aView.triangle_adjacency().size(), aView.triangle_count() * 3u);
#else
  EXPECT_NE(aView.triangle_normals(), nullptr);
  EXPECT_NE(aView.triangle_adjacency(), nullptr);
#endif
}

TEST_F(MeshVeneerTest, TriangleComponents_BoxGraph_ReturnsComponentLabels)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::mesh::TriangleComponentsView aView = occtl::mesh::triangle_components(aGraph);

  EXPECT_EQ(aView.triangle_count(), 12u);
  EXPECT_EQ(aView.component_count(), 6u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.triangle_component_ids().size(), aView.triangle_count());
  EXPECT_EQ(aView.component_sizes().size(), aView.component_count());
#else
  EXPECT_NE(aView.triangle_component_ids(), nullptr);
  EXPECT_NE(aView.component_sizes(), nullptr);
#endif
}

TEST_F(MeshVeneerTest, TriangleComponentTriangles_BoxGraph_ReturnsTriangles)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::mesh::TriangleComponentTrianglesView aView =
    occtl::mesh::triangle_component_triangles(aGraph, 0u);

  EXPECT_EQ(aView.component_id(), 0u);
  EXPECT_EQ(aView.triangle_count(), 2u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.triangles().size(), 2u);
#else
  EXPECT_NE(aView.triangles(), nullptr);
#endif
}

TEST_F(MeshVeneerTest, TriangleComponentBoundary_BoxGraph_ReturnsEdges)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::mesh::TriangleComponentBoundaryView aView =
    occtl::mesh::triangle_component_boundary(aGraph, 0u);

  EXPECT_EQ(aView.component_id(), 0u);
  EXPECT_EQ(aView.edge_count(), 4u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.edges().size(), 4u);
#else
  EXPECT_NE(aView.edges(), nullptr);
#endif
}

TEST_F(MeshVeneerTest, TriangleComponentBoundaryChains_BoxGraph_ReturnsChain)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::mesh::TriangleComponentBoundaryChainsView aView =
    occtl::mesh::triangle_component_boundary_chains(aGraph, 0u);

  EXPECT_EQ(aView.component_id(), 0u);
  EXPECT_EQ(aView.edge_count(), 4u);
  EXPECT_EQ(aView.chain_count(), 1u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.edges().size(), 4u);
  EXPECT_EQ(aView.chains().size(), 1u);
  EXPECT_EQ(aView.chains()[0].is_closed, 1);
#else
  EXPECT_NE(aView.edges(), nullptr);
  EXPECT_NE(aView.chains(), nullptr);
  EXPECT_EQ(aView.chains()[0].is_closed, 1);
#endif
}

TEST_F(MeshVeneerTest, TriangleComponentBoundaryPolylines_BoxGraph_ReturnsPolyline)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::mesh::TriangleComponentBoundaryPolylinesView aView =
    occtl::mesh::triangle_component_boundary_polylines(aGraph, 0u);

  EXPECT_EQ(aView.component_id(), 0u);
  EXPECT_EQ(aView.point_count(), 5u);
  EXPECT_EQ(aView.polyline_count(), 1u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.points().size(), 5u);
  EXPECT_EQ(aView.polylines().size(), 1u);
  EXPECT_EQ(aView.polylines()[0].is_closed, 1);
#else
  EXPECT_NE(aView.points(), nullptr);
  EXPECT_NE(aView.polylines(), nullptr);
  EXPECT_EQ(aView.polylines()[0].is_closed, 1);
#endif
}

TEST_F(MeshVeneerTest, TriangleComponentSummaries_BoxGraph_ReturnsSummaries)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::mesh::TriangleComponentSummariesView aView =
    occtl::mesh::triangle_component_summaries(aGraph);

  EXPECT_EQ(aView.triangle_count(), 12u);
  EXPECT_EQ(aView.component_count(), 6u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.summaries().size(), aView.component_count());
  EXPECT_EQ(aView.summaries()[0].triangle_count, 2u);
#else
  EXPECT_NE(aView.summaries(), nullptr);
  EXPECT_EQ(aView.summaries()[0].triangle_count, 2u);
#endif
}

TEST_F(MeshVeneerTest, TrianglePlaneComponents_BoxGraph_ReturnsPlanes)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::mesh::TrianglePlaneComponentsView aView =
    occtl::mesh::triangle_plane_components(aGraph);

  EXPECT_EQ(aView.triangle_count(), 12u);
  EXPECT_EQ(aView.component_count(), 6u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.components().size(), aView.component_count());
  EXPECT_EQ(aView.components()[0].triangle_count, 2u);
#else
  EXPECT_NE(aView.components(), nullptr);
  EXPECT_EQ(aView.components()[0].triangle_count, 2u);
#endif
}

TEST_F(MeshVeneerTest, TriangleSphereComponents_SphereGraph_ReturnsSphere)
{
  occtl::Graph        aGraph  = make_graph();
  const occtl::NodeId aSphere = make_sphere(aGraph, 5.0);
  ASSERT_TRUE(aSphere.is_valid());
  occtl::mesh::Options aMeshOptions{};
  aMeshOptions.deflection = 0.2;
  occtl::mesh::generate(aGraph, aMeshOptions);

  occtl::mesh::TriangleSphereComponentsOptions anOptions =
    OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  anOptions.max_normal_angle   = OCCTL_PI;
  anOptions.max_distance       = 0.25;
  anOptions.min_triangle_count = 8u;

  const occtl::mesh::TriangleSphereComponentsView aView =
    occtl::mesh::triangle_sphere_components(aGraph, anOptions);

  ASSERT_EQ(aView.component_count(), 1u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.components().size(), 1u);
  EXPECT_NEAR(aView.components()[0].radius, 5.0, 0.25);
#else
  ASSERT_NE(aView.components(), nullptr);
  EXPECT_NEAR(aView.components()[0].radius, 5.0, 0.25);
#endif
}

TEST_F(MeshVeneerTest, TriangleCylinderComponents_CylinderGraph_ReturnsCylinder)
{
  occtl::Graph        aGraph    = make_graph();
  const occtl::NodeId aCylinder = make_cylinder(aGraph, 3.0, 7.0);
  ASSERT_TRUE(aCylinder.is_valid());
  occtl::mesh::Options aMeshOptions{};
  aMeshOptions.deflection = 0.1;
  occtl::mesh::generate(aGraph, aMeshOptions);

  occtl::mesh::TriangleCylinderComponentsOptions anOptions =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  anOptions.max_distance       = 0.15;
  anOptions.min_triangle_count = 8u;

  const occtl::mesh::TriangleCylinderComponentsView aView =
    occtl::mesh::triangle_cylinder_components(aGraph, anOptions);

  ASSERT_EQ(aView.component_count(), 1u);
#if OCCTL_HPP_HAS_SPAN
  EXPECT_EQ(aView.components().size(), 1u);
  EXPECT_NEAR(aView.components()[0].radius, 3.0, 0.15);
#else
  ASSERT_NE(aView.components(), nullptr);
  EXPECT_NEAR(aView.components()[0].radius, 3.0, 0.15);
#endif
}

TEST_F(MeshVeneerTest, MakeSphereComponentSolid_SphereGraph_ReturnsSolid)
{
  occtl::Graph        aGraph  = make_graph();
  const occtl::NodeId aSphere = make_sphere(aGraph, 5.0);
  ASSERT_TRUE(aSphere.is_valid());
  occtl::mesh::Options aMeshOptions{};
  aMeshOptions.deflection = 0.2;
  occtl::mesh::generate(aGraph, aMeshOptions);

  occtl::mesh::TriangleSphereComponentsOptions anOptions =
    OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  anOptions.max_normal_angle   = OCCTL_PI;
  anOptions.max_distance       = 0.25;
  anOptions.min_triangle_count = 8u;

  const occtl::NodeId aSolid = occtl::mesh::make_sphere_component_solid(aGraph, anOptions, 0u);

  ASSERT_TRUE(aSolid.is_valid());
  EXPECT_EQ(aGraph.node_id_kind(aSolid), OCCTL_KIND_SOLID);
}

TEST_F(MeshVeneerTest, MakeCylinderComponentSolid_CylinderGraph_ReturnsSolid)
{
  occtl::Graph        aGraph    = make_graph();
  const occtl::NodeId aCylinder = make_cylinder(aGraph, 3.0, 7.0);
  ASSERT_TRUE(aCylinder.is_valid());
  occtl::mesh::Options aMeshOptions{};
  aMeshOptions.deflection = 0.1;
  occtl::mesh::generate(aGraph, aMeshOptions);

  occtl::mesh::TriangleCylinderComponentsOptions anOptions =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  anOptions.max_distance       = 0.15;
  anOptions.min_triangle_count = 8u;

  const occtl::NodeId aSolid = occtl::mesh::make_cylinder_component_solid(aGraph, anOptions, 0u);

  ASSERT_TRUE(aSolid.is_valid());
  EXPECT_EQ(aGraph.node_id_kind(aSolid), OCCTL_KIND_SOLID);
}

TEST_F(MeshVeneerTest, MakeSphereComponentSolids_SphereGraph_ReturnsSolids)
{
  occtl::Graph        aGraph  = make_graph();
  const occtl::NodeId aSphere = make_sphere(aGraph, 5.0);
  ASSERT_TRUE(aSphere.is_valid());
  occtl::mesh::Options aMeshOptions{};
  aMeshOptions.deflection = 0.2;
  occtl::mesh::generate(aGraph, aMeshOptions);

  occtl::mesh::TriangleSphereComponentsOptions anOptions =
    OCCTL_MESH_TRIANGLE_SPHERE_COMPONENTS_OPTIONS_INIT;
  anOptions.max_normal_angle   = OCCTL_PI;
  anOptions.max_distance       = 0.25;
  anOptions.min_triangle_count = 8u;

  const std::vector<occtl::NodeId> aSolids =
    occtl::mesh::make_sphere_component_solids(aGraph, anOptions);

  ASSERT_EQ(aSolids.size(), 1u);
  ASSERT_TRUE(aSolids[0].is_valid());
  EXPECT_EQ(aGraph.node_id_kind(aSolids[0]), OCCTL_KIND_SOLID);
}

TEST_F(MeshVeneerTest, MakeCylinderComponentSolids_CylinderGraph_ReturnsSolids)
{
  occtl::Graph        aGraph    = make_graph();
  const occtl::NodeId aCylinder = make_cylinder(aGraph, 3.0, 7.0);
  ASSERT_TRUE(aCylinder.is_valid());
  occtl::mesh::Options aMeshOptions{};
  aMeshOptions.deflection = 0.1;
  occtl::mesh::generate(aGraph, aMeshOptions);

  occtl::mesh::TriangleCylinderComponentsOptions anOptions =
    OCCTL_MESH_TRIANGLE_CYLINDER_COMPONENTS_OPTIONS_INIT;
  anOptions.max_distance       = 0.15;
  anOptions.min_triangle_count = 8u;

  const std::vector<occtl::NodeId> aSolids =
    occtl::mesh::make_cylinder_component_solids(aGraph, anOptions);

  ASSERT_EQ(aSolids.size(), 1u);
  ASSERT_TRUE(aSolids[0].is_valid());
  EXPECT_EQ(aGraph.node_id_kind(aSolids[0]), OCCTL_KIND_SOLID);
}

TEST_F(MeshVeneerTest, MakePlaneComponentFace_BoxGraph_ReturnsFace)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const occtl::NodeId aFace = occtl::mesh::make_plane_component_face(aGraph, 0u);

  ASSERT_TRUE(aFace.is_valid());
  ::occtl_node_kind_t aKind = static_cast<::occtl_node_kind_t>(0);
  ASSERT_EQ(::occtl_graph_node_kind(aGraph.get(), aFace.get(), &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(MeshVeneerTest, MakePlaneComponentFaces_BoxGraph_ReturnsFaces)
{
  occtl::Graph        aGraph = make_graph();
  const occtl::NodeId aBox   = make_box(aGraph, 10.0, 10.0, 10.0);
  ASSERT_TRUE(aBox.is_valid());
  occtl::mesh::generate(aGraph, occtl::mesh::Options{});

  const std::vector<occtl::NodeId> aFaces = occtl::mesh::make_plane_component_faces(aGraph);

  ASSERT_EQ(aFaces.size(), 6u);
  for (const occtl::NodeId& aFace : aFaces)
  {
    ASSERT_TRUE(aFace.is_valid());
    ::occtl_node_kind_t aKind = static_cast<::occtl_node_kind_t>(0);
    ASSERT_EQ(::occtl_graph_node_kind(aGraph.get(), aFace.get(), &aKind), OCCTL_OK);
    EXPECT_EQ(aKind, OCCTL_KIND_FACE);
  }
}

TEST_F(MeshVeneerTest, FromBuffers_SingleTriangle_ReturnsTriangulatedFace)
{
  const double   aNodes[]     = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0};
  const uint32_t aTriangles[] = {0u, 1u, 2u};

  occtl::mesh::FromBuffersOptions anOptions = OCCTL_MESH_FROM_BUFFERS_OPTIONS_INIT;
  anOptions.nodes                           = aNodes;
  anOptions.node_count                      = 3u;
  anOptions.triangles                       = aTriangles;
  anOptions.triangle_count                  = 1u;

  auto [aGraph, aRoot] = occtl::mesh::from_buffers(anOptions);
  ASSERT_TRUE(aRoot.is_valid());

  occtl::mesh::TriangulationView aView = occtl::mesh::face_triangulation(aGraph, aRoot);
  EXPECT_EQ(aView.node_count(), 3u);
  EXPECT_EQ(aView.triangle_count(), 1u);
}

TEST_F(MeshVeneerTest, GenerateWithBadVersion_ThrowsError)
{
  occtl::Graph aGraph = make_graph();
  ASSERT_TRUE(make_box(aGraph, 5.0, 5.0, 5.0).is_valid());

  // Forge a bad-version options struct via the C ABI directly so we can
  // verify the veneer translates the resulting status into occtl::Error.
  ::occtl_mesh_options_t aBad = OCCTL_MESH_OPTIONS_INIT;
  aBad.struct_version         = 0xDEADBEEFu;
  EXPECT_EQ(::occtl_mesh_generate(aGraph.get(), nullptr, 0, &aBad), OCCTL_VERSION_MISMATCH);

  // The veneer's generate() always projects through Options::to_c() which
  // sets a known version, so the throwing path is exercised by feeding a
  // bad node ID instead.
  const std::vector<occtl::NodeId> aNodes{occtl::NodeId(::occtl_node_id_t{0xCAFE})};
  EXPECT_THROW(occtl::mesh::generate(aGraph, aNodes, occtl::mesh::Options{}), occtl::Error);
}

} // namespace
