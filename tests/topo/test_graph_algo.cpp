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

#include <occtl/occtl_curves.h>
#include <occtl/occtl_mesh.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_surfaces.h>
#include <occtl/occtl_topo.h>
#include <occtl/occtl_topo_algo.h>

#include "test_helpers_internal.hxx"

#include <BRepAdaptor_Surface.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <TopoDS.hxx>

#include "../src/topo/IdConvert.hxx"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace
{

class TopoAlgoTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);
    ASSERT_NE(myGraph, nullptr);
    loadBox(myGraph);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t* myGraph = nullptr;
};

occtl_node_id_t makeRectangle(occtl_graph_t* const theGraph,
                              const double         theZ,
                              const double         theWidth,
                              const double         theHeight)
{
  occtl_prim_rectangle_info_t anInfo = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  anInfo.placement.location          = {0.0, 0.0, theZ};
  anInfo.width                       = theWidth;
  anInfo.height                      = theHeight;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_rectangle(theGraph, &anInfo, &aWire), OCCTL_OK);
  return aWire;
}

occtl_node_id_t makePlanarFace(occtl_graph_t* const theGraph, const occtl_node_id_t theWire)
{
  occtl_prim_planar_face_info_t anInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  anInfo.outer_wire                    = theWire;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_planar_face(theGraph, &anInfo, &aFace), OCCTL_OK);
  return aFace;
}

occtl_rep_id_t makeTrimmedLine(occtl_graph_t* const      theGraph,
                               const occtl_point3_t&     theOrigin,
                               const occtl_direction3_t& theDirection,
                               const double              theFirst,
                               const double              theLast)
{
  occtl_rep_id_t aBasis = {};
  EXPECT_EQ(occtl_curve_create_line(theGraph, {theOrigin, theDirection}, &aBasis), OCCTL_OK);

  occtl_curve_trimmed_create_info_t aTrimInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aTrimInfo.basis                             = aBasis;
  aTrimInfo.u_first                           = theFirst;
  aTrimInfo.u_last                            = theLast;

  occtl_rep_id_t aTrimmed = {};
  EXPECT_EQ(occtl_curve_create_trimmed(theGraph, &aTrimInfo, &aTrimmed), OCCTL_OK);
  return aTrimmed;
}

std::vector<occtl_node_id_t> childEdges(occtl_graph_t* const  theGraph,
                                        const occtl_node_id_t theRoot)
{
  occtl_topo_child_explorer_config_t aConfig = OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT;
  aConfig.mode                               = OCCTL_TOPO_EXPLORER_RECURSIVE;
  aConfig.target_kind                        = OCCTL_KIND_EDGE;

  occtl_topo_explorer_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_child_explorer_create(theGraph, theRoot, &aConfig, &anIter), OCCTL_OK);

  std::vector<occtl_node_id_t> anEdges;
  occtl_node_id_t              anEdge        = OCCTL_NODE_ID_INVALID;
  occtl_transform_t            aTransform    = occtl_transform_identity();
  occtl_orientation_t          anOrientation = OCCTL_ORIENTATION_FORWARD;
  while (occtl_topo_explorer_iter_next(anIter, &anEdge, &aTransform, &anOrientation) == OCCTL_OK)
  {
    anEdges.push_back(anEdge);
  }
  occtl_topo_explorer_iter_free(anIter);
  return anEdges;
}

std::vector<occtl_node_id_t> childFaces(occtl_graph_t* const  theGraph,
                                        const occtl_node_id_t theRoot)
{
  occtl_topo_child_explorer_config_t aConfig = OCCTL_TOPO_CHILD_EXPLORER_CONFIG_INIT;
  aConfig.mode                               = OCCTL_TOPO_EXPLORER_RECURSIVE;
  aConfig.target_kind                        = OCCTL_KIND_FACE;

  occtl_topo_explorer_iter_t* anIter = nullptr;
  EXPECT_EQ(occtl_topo_child_explorer_create(theGraph, theRoot, &aConfig, &anIter), OCCTL_OK);

  std::vector<occtl_node_id_t> aFaces;
  occtl_node_id_t              aFace         = OCCTL_NODE_ID_INVALID;
  occtl_transform_t            aTransform    = occtl_transform_identity();
  occtl_orientation_t          anOrientation = OCCTL_ORIENTATION_FORWARD;
  while (occtl_topo_explorer_iter_next(anIter, &aFace, &aTransform, &anOrientation) == OCCTL_OK)
  {
    aFaces.push_back(aFace);
  }
  occtl_topo_explorer_iter_free(anIter);
  return aFaces;
}

occtl_node_id_t firstCylindricalFace(occtl_graph_t* const theGraph, const occtl_node_id_t theRoot)
{
  const std::vector<occtl_node_id_t> aFaces = childFaces(theGraph, theRoot);
  for (const occtl_node_id_t aFace : aFaces)
  {
    const BRepGraph_NodeId aNodeId = OcctL::Topo::UnpackNodeId(aFace);
    if (!aNodeId.IsValid())
    {
      continue;
    }

    const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(aNodeId);
    if (aShape.IsNull() || aShape.ShapeType() != TopAbs_FACE)
    {
      continue;
    }

    BRepAdaptor_Surface aSurface{TopoDS::Face(aShape)};
    if (aSurface.GetType() == GeomAbs_Cylinder)
    {
      return aFace;
    }
  }
  return OCCTL_NODE_ID_INVALID;
}

TEST_F(TopoAlgoTest, SewOptions_Init_HasV1AndDefaults)
{
  occtl_topo_sew_options_t aOpts{};
  occtl_topo_sew_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_SEW_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.cutting, 1);
  EXPECT_EQ(aOpts.same_parameter_mode, 1);
  EXPECT_EQ(aOpts.face_analysis, 1);
}

TEST_F(TopoAlgoTest, Sew_AlreadyClosedBox_IsDoneAndZeroSewn)
{
  occtl_topo_sew_options_t aOpts;
  occtl_topo_sew_options_init(&aOpts);
  occtl_topo_sew_result_t aRes;
  occtl_topo_sew_result_init(&aRes);
  ASSERT_EQ(occtl_topo_sew(myGraph, &aOpts, &aRes), OCCTL_OK);
  EXPECT_EQ(aRes.is_done, 1);
  EXPECT_EQ(aRes.sewn_edge_count, 0u);
  EXPECT_EQ(aRes.free_edge_count_after, 0u);
}

TEST_F(TopoAlgoTest, Sew_NullGraph_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_topo_sew(nullptr, nullptr, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

TEST_F(TopoAlgoTest, Sew_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_topo_sew_options_t aOpts{};
  aOpts.struct_version = 999u;
  EXPECT_EQ(occtl_topo_sew(myGraph, &aOpts, nullptr), OCCTL_VERSION_MISMATCH);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

TEST_F(TopoAlgoTest, RecomputeSameParameter_NoFallbacksOnBox)
{
  occtl_topo_same_parameter_options_t aOpts;
  occtl_topo_same_parameter_options_init(&aOpts);
  uint32_t aC0 = 999u, aAp = 999u;
  EXPECT_EQ(occtl_topo_recompute_same_parameter(myGraph, &aOpts, &aC0, &aAp), OCCTL_OK);
  EXPECT_EQ(aC0, 0u);
  EXPECT_EQ(aAp, 0u);
}

TEST_F(TopoAlgoTest, RecomputeSameParameter_NullOptsAllowed)
{
  EXPECT_EQ(occtl_topo_recompute_same_parameter(myGraph, nullptr, nullptr, nullptr), OCCTL_OK);
}

TEST_F(TopoAlgoTest, RecomputeSameParameter_BadVersion_ReturnsVersionMismatch)
{
  occtl_topo_same_parameter_options_t aOpts{};
  aOpts.struct_version = 0u;
  EXPECT_EQ(occtl_topo_recompute_same_parameter(myGraph, &aOpts, nullptr, nullptr),
            OCCTL_VERSION_MISMATCH);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

TEST_F(TopoAlgoTest, EdgeBlendOptions_Init_HasV1AndDefaults)
{
  occtl_topo_edge_blend_options_t aOpts{};
  occtl_topo_edge_blend_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_EDGE_BLEND_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.root.bits, 0u);
  EXPECT_EQ(aOpts.edges, nullptr);
  EXPECT_EQ(aOpts.edge_count, 0u);
  EXPECT_EQ(aOpts.radius, 1.0);
  EXPECT_EQ(aOpts.chamfer_mode, 0);
}

TEST_F(TopoAlgoTest, MaxFilletRadiusOptions_Init_HasV1AndDefaults)
{
  occtl_topo_max_fillet_radius_options_t aOpts{};
  occtl_topo_max_fillet_radius_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.root.bits, 0u);
  EXPECT_EQ(aOpts.edges, nullptr);
  EXPECT_EQ(aOpts.edge_count, 0u);
  EXPECT_EQ(aOpts.min_radius, 1.0e-6);
  EXPECT_EQ(aOpts.max_radius, 0.0);
  EXPECT_EQ(aOpts.tolerance, 1.0e-4);
  EXPECT_EQ(aOpts.max_iterations, 24);
}

TEST_F(TopoAlgoTest, TransformCopy_Box_ReturnsNewGraphWithSolidRoot)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aRoot.bits, 0u);

  const occtl_transform_t aTrsf     = occtl_transform_translation({1.0, 2.0, 3.0});
  occtl_graph_t*          aOutGraph = nullptr;
  occtl_node_id_t         aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_transformed(myGraph, aRoot, aTrsf, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aOutGraph), size_t{1});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, TransformCopy_InvalidRoot_ReturnsNotFound)
{
  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_transformed(myGraph,
                                   OCCTL_NODE_ID_INVALID,
                                   occtl_transform_identity(),
                                   &aOutGraph,
                                   &aOutRoot),
            OCCTL_NOT_FOUND);
  EXPECT_EQ(aOutGraph, nullptr);
  EXPECT_EQ(aOutRoot.bits, 0u);
}

TEST_F(TopoAlgoTest, MaxFilletRadius_BoxEdge_ReturnsUsableRadius)
{
  const occtl_node_id_t aRoot  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_topo_max_fillet_radius_options_t aOpts = OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_INIT;
  aOpts.root                                   = aRoot;
  aOpts.edges                                  = &anEdge;
  aOpts.edge_count                             = 1;
  aOpts.max_radius                             = 2.0;
  aOpts.tolerance                              = 1.0e-3;

  double aRadius = 0.0;
  ASSERT_EQ(occtl_topo_max_fillet_radius(myGraph, &aOpts, &aRadius), OCCTL_OK);
  EXPECT_GT(aRadius, 0.1);
  EXPECT_LE(aRadius, aOpts.max_radius);

  occtl_topo_edge_blend_options_t aBlendOpts = OCCTL_TOPO_EDGE_BLEND_OPTIONS_INIT;
  aBlendOpts.root                            = aRoot;
  aBlendOpts.edges                           = &anEdge;
  aBlendOpts.edge_count                      = 1;
  aBlendOpts.radius                          = aRadius * 0.5;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_blend_edges(myGraph, &aBlendOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, MaxFilletRadius_InvalidOptions_ReturnInvalidArgument)
{
  const occtl_node_id_t aRoot  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(anEdge.bits, 0u);

  double                                 aRadius = 0.0;
  occtl_topo_max_fillet_radius_options_t aOpts   = OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_INIT;
  aOpts.root                                     = aRoot;
  aOpts.edges                                    = &anEdge;
  aOpts.edge_count                               = 1;
  aOpts.min_radius                               = -1.0;

  EXPECT_EQ(occtl_topo_max_fillet_radius(myGraph, &aOpts, &aRadius), OCCTL_INVALID_ARGUMENT);

  aOpts                = OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_INIT;
  aOpts.root           = aRoot;
  aOpts.edges          = &anEdge;
  aOpts.edge_count     = 1;
  aOpts.struct_version = 999u;
  EXPECT_EQ(occtl_topo_max_fillet_radius(myGraph, &aOpts, &aRadius), OCCTL_VERSION_MISMATCH);

  aOpts      = OCCTL_TOPO_MAX_FILLET_RADIUS_OPTIONS_INIT;
  aOpts.root = aRoot;
  EXPECT_EQ(occtl_topo_max_fillet_radius(myGraph, &aOpts, &aRadius), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_max_fillet_radius(nullptr, &aOpts, &aRadius), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_max_fillet_radius(myGraph, nullptr, &aRadius), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_topo_max_fillet_radius(myGraph, &aOpts, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoAlgoTest, BlendEdges_SelectedFillet_ReturnsNewGraph)
{
  const occtl_node_id_t aRoot  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_topo_edge_blend_options_t aOpts;
  occtl_topo_edge_blend_options_init(&aOpts);
  aOpts.root       = aRoot;
  aOpts.edges      = &anEdge;
  aOpts.edge_count = 1;
  aOpts.radius     = 0.5;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_blend_edges(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_edge_count, aOutGraph),
            occtl_graph_count_value(occtl_graph_edge_count, myGraph));
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, BlendEdgesWithHistory_SelectedFillet_ReportsEdgeChange)
{
  const occtl_node_id_t aRoot  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_uid_t anEdgeUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(myGraph, anEdge, &anEdgeUid), OCCTL_OK);

  occtl_topo_edge_blend_options_t aOpts;
  occtl_topo_edge_blend_options_init(&aOpts);
  aOpts.root       = aRoot;
  aOpts.edges      = &anEdge;
  aOpts.edge_count = 1;
  aOpts.radius     = 0.5;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_blend_edges(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);

  size_t aModifiedCount  = 0;
  size_t aGeneratedCount = 0;
  size_t aDeletedCount   = 0;
  ASSERT_EQ(occtl_graph_history_modified(aOutGraph, anEdgeUid, nullptr, 0, &aModifiedCount),
            OCCTL_OK);
  ASSERT_EQ(occtl_graph_history_generated(aOutGraph, anEdgeUid, nullptr, 0, &aGeneratedCount),
            OCCTL_OK);
  ASSERT_EQ(occtl_graph_history_deleted_all(aOutGraph, nullptr, 0, &aDeletedCount), OCCTL_OK);
  EXPECT_GT(aModifiedCount + aGeneratedCount + aDeletedCount, 0u);
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, BlendEdges_SelectedChamfer_ReturnsNewGraph)
{
  const occtl_node_id_t aRoot  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_topo_edge_blend_options_t aOpts;
  occtl_topo_edge_blend_options_init(&aOpts);
  aOpts.root          = aRoot;
  aOpts.edges         = &anEdge;
  aOpts.edge_count    = 1;
  aOpts.chamfer_mode  = 1;
  aOpts.chamfer_dist1 = 0.5;
  aOpts.chamfer_dist2 = 0.5;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_blend_edges(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, BlendEdges_EmptySelection_ReturnsInvalidArgument)
{
  occtl_topo_edge_blend_options_t aOpts;
  occtl_topo_edge_blend_options_init(&aOpts);
  aOpts.root = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_blend_edges(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, BlendEdges_BadVersion_ReturnsVersionMismatch)
{
  occtl_topo_edge_blend_options_t aOpts{};
  aOpts.struct_version = 999u;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_blend_edges(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, ProjectOnFace_RectangleWire_ReturnsProjectedWireGraph)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t aTargetWire = makeRectangle(aGraph, 0.0, 6.0, 4.0);
  const occtl_node_id_t aTargetFace = makePlanarFace(aGraph, aTargetWire);
  const occtl_node_id_t aSourceWire = makeRectangle(aGraph, 2.0, 2.0, 1.0);

  occtl_topo_project_on_face_options_t aOpts = OCCTL_TOPO_PROJECT_ON_FACE_OPTIONS_INIT;
  aOpts.source                               = aSourceWire;
  aOpts.face                                 = aTargetFace;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_project_on_face(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_edge_count, aOutGraph), 0u);

  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, ProjectFaceAlongDirection_RectangleOntoFace_ReturnsFaceGraph)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t aTargetWire = makeRectangle(aGraph, 0.0, 6.0, 4.0);
  const occtl_node_id_t aTargetFace = makePlanarFace(aGraph, aTargetWire);
  const occtl_node_id_t aSourceWire = makeRectangle(aGraph, 2.0, 2.0, 1.0);
  const occtl_node_id_t aSourceFace = makePlanarFace(aGraph, aSourceWire);

  occtl_topo_project_face_direction_options_t aOpts =
    OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_INIT;
  aOpts.source_face = aSourceFace;
  aOpts.target      = aTargetFace;
  aOpts.direction   = {0.0, 0.0, -1.0};

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_project_face_along_direction(aGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_GE(occtl_graph_count_value(occtl_graph_face_count, aOutGraph), size_t{1});

  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, ProjectFaceAlongDirectionOptions_Init_HasDefaults)
{
  occtl_topo_project_face_direction_options_t aOpts{};
  occtl_topo_project_face_direction_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.source_face.bits, 0u);
  EXPECT_EQ(aOpts.target.bits, 0u);
  EXPECT_DOUBLE_EQ(aOpts.direction.x, 0.0);
  EXPECT_DOUBLE_EQ(aOpts.direction.y, 0.0);
  EXPECT_DOUBLE_EQ(aOpts.direction.z, -1.0);
  EXPECT_DOUBLE_EQ(aOpts.max_distance, -1.0);
  EXPECT_EQ(aOpts.copy_source, 1);
}

TEST_F(TopoAlgoTest, ProjectFaceAlongDirection_WireSource_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t aTargetWire = makeRectangle(aGraph, 0.0, 6.0, 4.0);
  const occtl_node_id_t aTargetFace = makePlanarFace(aGraph, aTargetWire);
  const occtl_node_id_t aSourceWire = makeRectangle(aGraph, 2.0, 2.0, 1.0);

  occtl_topo_project_face_direction_options_t aOpts =
    OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_INIT;
  aOpts.source_face = aSourceWire;
  aOpts.target      = aTargetFace;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_project_face_along_direction(aGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_WRONG_KIND);
  EXPECT_EQ(aOutGraph, nullptr);

  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, ProjectFaceAlongDirection_ZeroDirection_ReturnsInvalidArgument)
{
  occtl_topo_project_face_direction_options_t aOpts =
    OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_INIT;
  aOpts.direction = {0.0, 0.0, 0.0};

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_project_face_along_direction(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, FaceToArcs_RectangleFace_ReturnsFaceGraph)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t aWire = makeRectangle(aGraph, 0.0, 4.0, 3.0);
  const occtl_node_id_t aFace = makePlanarFace(aGraph, aWire);

  occtl_topo_face_to_arcs_options_t aOpts = OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_INIT;
  aOpts.source                            = aFace;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_face_to_arcs(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aOutGraph), size_t{1});
  EXPECT_GE(occtl_graph_count_value(occtl_graph_edge_count, aOutGraph), size_t{4});

  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, FaceToArcs_ClosedWire_ReturnsWireGraph)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t aWire = makeRectangle(aGraph, 0.0, 4.0, 3.0);

  occtl_topo_face_to_arcs_options_t aOpts = OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_INIT;
  aOpts.source                            = aWire;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_face_to_arcs(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_wire_count, aOutGraph), size_t{1});
  EXPECT_GE(occtl_graph_count_value(occtl_graph_edge_count, aOutGraph), size_t{4});

  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, WrapOnFace_RectangleFaceOntoCylinder_ReturnsFaceGraph)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_prim_cylinder_info_t aCylinderInfo = OCCTL_PRIM_CYLINDER_INFO_INIT;
  aCylinderInfo.radius                     = 5.0;
  aCylinderInfo.height                     = 8.0;
  occtl_node_id_t aCylinder                = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_cylinder(aGraph, &aCylinderInfo, &aCylinder), OCCTL_OK);

  const occtl_node_id_t aTargetFace = firstCylindricalFace(aGraph, aCylinder);
  ASSERT_NE(aTargetFace.bits, 0u);

  const occtl_node_id_t aWire = makeRectangle(aGraph, 0.0, 1.2, 1.0);
  const occtl_node_id_t aFace = makePlanarFace(aGraph, aWire);

  occtl_topo_wrap_on_face_options_t aOpts = OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_INIT;
  aOpts.source                            = aFace;
  aOpts.target_face                       = aTargetFace;
  aOpts.surface_location.location         = {5.0, 0.0, 4.0};
  aOpts.surface_location.x_dir            = {0.0, 0.0, 1.0};
  aOpts.surface_location.y_dir            = {0.0, 1.0, 0.0};
  aOpts.surface_location.z_dir            = {1.0, 0.0, 0.0};

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_wrap_on_face(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aOutGraph), size_t{1});
  EXPECT_GE(occtl_graph_count_value(occtl_graph_edge_count, aOutGraph), size_t{4});

  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, WrapOnFaceOptions_Init_HasDefaults)
{
  occtl_topo_wrap_on_face_options_t aOpts{};
  occtl_topo_wrap_on_face_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.source.bits, 0u);
  EXPECT_DOUBLE_EQ(aOpts.tolerance, 1.0e-3);
  EXPECT_EQ(aOpts.initial_subdivisions, 4);
}

TEST_F(TopoAlgoTest, WrapOnFace_SolidSource_ReturnsWrongKind)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aSolid.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  occtl_topo_wrap_on_face_options_t aOpts = OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_INIT;
  aOpts.source                            = aSolid;
  aOpts.target_face                       = aFace;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_wrap_on_face(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_WRONG_KIND);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, WrapOnFace_InvalidTolerance_ReturnsInvalidArgument)
{
  occtl_topo_wrap_on_face_options_t aOpts = OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_INIT;
  aOpts.tolerance                         = 0.0;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_wrap_on_face(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, FaceToArcsOptions_Init_HasDefaults)
{
  occtl_topo_face_to_arcs_options_t aOpts{};
  occtl_topo_face_to_arcs_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.source.bits, 0u);
  EXPECT_DOUBLE_EQ(aOpts.angular_tolerance, 1.0e-3);
}

TEST_F(TopoAlgoTest, FaceToArcs_SolidSource_ReturnsWrongKind)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_topo_face_to_arcs_options_t aOpts = OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_INIT;
  aOpts.source                            = aSolid;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_face_to_arcs(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_WRONG_KIND);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, FaceToArcs_NonPositiveTolerance_ReturnsInvalidArgument)
{
  occtl_topo_face_to_arcs_options_t aOpts = OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_INIT;
  aOpts.angular_tolerance                 = 0.0;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_face_to_arcs(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, HlrProject_Box_ReturnsVisibleEdgeGraph)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_topo_hlr_options_t aOpts = OCCTL_TOPO_HLR_OPTIONS_INIT;
  aOpts.root                     = aSolid;
  aOpts.include_hidden           = 0;
  aOpts.mode                     = OCCTL_TOPO_HLR_BREP;

  occtl_topo_hlr_result_t aResult = OCCTL_TOPO_HLR_RESULT_INIT;
  ASSERT_EQ(occtl_topo_make_hlr_projection(myGraph, &aOpts, &aResult), OCCTL_OK);
  ASSERT_NE(aResult.graph, nullptr);
  EXPECT_NE(aResult.visible_sharp.bits | aResult.visible_outline.bits, 0u);
  EXPECT_EQ(aResult.hidden_sharp.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_edge_count, aResult.graph), size_t{0});

  occtl_graph_free(aResult.graph);
}

TEST_F(TopoAlgoTest, HlrOptions_Init_HasDefaults)
{
  occtl_topo_hlr_options_t aOpts{};
  occtl_topo_hlr_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_HLR_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.root.bits, 0u);
  EXPECT_DOUBLE_EQ(aOpts.focus, 0.0);
  EXPECT_EQ(aOpts.include_hidden, 1);
  EXPECT_EQ(aOpts.mode, OCCTL_TOPO_HLR_BREP);
}

TEST_F(TopoAlgoTest, HlrProject_InvalidFrame_ReturnsInvalidArgument)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_topo_hlr_options_t aOpts = OCCTL_TOPO_HLR_OPTIONS_INIT;
  aOpts.root                     = aSolid;
  aOpts.projection_frame.x_dir   = {0.0, 0.0, 1.0};
  aOpts.projection_frame.z_dir   = {0.0, 0.0, 1.0};

  occtl_topo_hlr_result_t aResult = OCCTL_TOPO_HLR_RESULT_INIT;
  EXPECT_EQ(occtl_topo_make_hlr_projection(myGraph, &aOpts, &aResult), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aResult.graph, nullptr);
}

#ifdef OCCTL_HAS_MESH
TEST_F(TopoAlgoTest, HlrProject_PolyMode_ReturnsVisibleEdgeGraph)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_topo_hlr_options_t aOpts = OCCTL_TOPO_HLR_OPTIONS_INIT;
  aOpts.root                     = aSolid;
  aOpts.include_hidden           = 0;
  aOpts.mode                     = OCCTL_TOPO_HLR_POLY;

  occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(occtl_mesh_generate(myGraph, &aSolid, 1, &aMeshOpts), OCCTL_OK);

  occtl_topo_hlr_result_t aResult = OCCTL_TOPO_HLR_RESULT_INIT;
  ASSERT_EQ(occtl_topo_make_hlr_projection(myGraph, &aOpts, &aResult), OCCTL_OK);
  ASSERT_NE(aResult.graph, nullptr);
  EXPECT_NE(aResult.visible_sharp.bits | aResult.visible_outline.bits, 0u);
  EXPECT_EQ(aResult.hidden_sharp.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_edge_count, aResult.graph), size_t{0});

  occtl_graph_free(aResult.graph);
}
#endif

TEST_F(TopoAlgoTest, HlrProject_InvalidMode_ReturnsInvalidArgument)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  occtl_topo_hlr_options_t aOpts = OCCTL_TOPO_HLR_OPTIONS_INIT;
  aOpts.root                     = aSolid;
  aOpts.mode                     = static_cast<occtl_topo_hlr_mode_t>(99);

  occtl_topo_hlr_result_t aResult = OCCTL_TOPO_HLR_RESULT_INIT;
  EXPECT_EQ(occtl_topo_make_hlr_projection(myGraph, &aOpts, &aResult), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aResult.graph, nullptr);
}

TEST_F(TopoAlgoTest, FaceFromSurface_PlaneWithRectangleWire_ReturnsFace)
{
  const occtl_node_id_t aWire = makeRectangle(myGraph, 0.0, 4.0, 3.0);
  ASSERT_NE(aWire.bits, 0u);

  occtl_geom_plane_t aPlaneData = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(occtl_surface_create_plane(myGraph, &aSurfaceId, aPlaneData), OCCTL_OK);
  ASSERT_NE(aSurfaceId.bits, 0u);

  occtl_prim_face_from_surface_options_t aOpts = OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_INIT;
  aOpts.surface_id                             = aSurfaceId;
  aOpts.outer_wire                             = aWire;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_face_from_surface(myGraph, &aOpts, &aFace), OCCTL_OK);
  EXPECT_NE(aFace.bits, 0u);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);

  int32_t aHasSurface = 0;
  ASSERT_EQ(occtl_topo_face_has_surface(myGraph, aFace, &aHasSurface), OCCTL_OK);
  EXPECT_EQ(aHasSurface, 1);
}

TEST_F(TopoAlgoTest, FaceFromSurface_Init_HasDefaults)
{
  occtl_prim_face_from_surface_options_t aOpts{};
  occtl_prim_face_from_surface_options_init(&aOpts);

  EXPECT_EQ(aOpts.struct_version, OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.p_next, nullptr);
  EXPECT_EQ(aOpts.surface_id.bits, 0u);
  EXPECT_EQ(aOpts.outer_wire.bits, 0u);
  EXPECT_EQ(aOpts.inner_wires, nullptr);
  EXPECT_EQ(aOpts.inner_wire_count, 0u);
  EXPECT_DOUBLE_EQ(aOpts.tolerance, 1.0e-6);
}

TEST_F(TopoAlgoTest, FaceFromSurface_InnerWithoutOuter_ReturnsInvalidArgument)
{
  const occtl_node_id_t aWire = makeRectangle(myGraph, 0.0, 2.0, 2.0);
  ASSERT_NE(aWire.bits, 0u);

  occtl_geom_plane_t aPlaneData = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(occtl_surface_create_plane(myGraph, &aSurfaceId, aPlaneData), OCCTL_OK);
  ASSERT_NE(aSurfaceId.bits, 0u);

  occtl_prim_face_from_surface_options_t aOpts = OCCTL_PRIM_FACE_FROM_SURFACE_OPTIONS_INIT;
  aOpts.surface_id                             = aSurfaceId;
  aOpts.inner_wires                            = &aWire;
  aOpts.inner_wire_count                       = 1;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_face_from_surface(myGraph, &aOpts, &aFace), OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoAlgoTest, FaceFromPointGrid_ApproximateGrid_ReturnsFace)
{
  const occtl_point3_t aPoints[9] = {{0.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.2},
                                     {0.0, 2.0, 0.0},
                                     {1.0, 0.0, 0.4},
                                     {1.0, 1.0, 0.8},
                                     {1.0, 2.0, 0.4},
                                     {2.0, 0.0, 0.0},
                                     {2.0, 1.0, 0.2},
                                     {2.0, 2.0, 0.0}};

  occtl_prim_face_from_point_grid_options_t aOpts = OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_INIT;
  aOpts.surface.points                            = aPoints;
  aOpts.surface.u_point_count                     = 3;
  aOpts.surface.v_point_count                     = 3;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_face_from_point_grid(myGraph, &aOpts, &aFace), OCCTL_OK);
  EXPECT_NE(aFace.bits, 0u);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);

  int32_t aHasSurface = 0;
  ASSERT_EQ(occtl_topo_face_has_surface(myGraph, aFace, &aHasSurface), OCCTL_OK);
  EXPECT_EQ(aHasSurface, 1);
}

TEST_F(TopoAlgoTest, FaceFromPointGrid_Init_HasDefaults)
{
  occtl_prim_face_from_point_grid_options_t aOpts{};
  occtl_prim_face_from_point_grid_options_init(&aOpts);

  EXPECT_EQ(aOpts.struct_version, OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.p_next, nullptr);
  EXPECT_EQ(aOpts.surface.struct_version, OCCTL_SURFACE_POINT_GRID_CREATE_INFO_VERSION_1);
  EXPECT_EQ(aOpts.surface.points, nullptr);
  EXPECT_EQ(aOpts.surface.mode, OCCTL_SURFACE_POINT_GRID_MODE_APPROXIMATE);
  EXPECT_DOUBLE_EQ(aOpts.tolerance, 1.0e-6);
}

TEST_F(TopoAlgoTest, FaceFromPointGrid_BadOuterVersion_ReturnsVersionMismatch)
{
  occtl_prim_face_from_point_grid_options_t aOpts = OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_INIT;
  aOpts.struct_version                            = 0u;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_face_from_point_grid(myGraph, &aOpts, &aFace), OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aFace.bits, 0u);
}

TEST_F(TopoAlgoTest, FaceFromPointGrid_BadSurfaceOptions_ReturnsInvalidArgument)
{
  occtl_prim_face_from_point_grid_options_t aOpts = OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_INIT;
  aOpts.surface.u_point_count                     = 3;
  aOpts.surface.v_point_count                     = 3;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_face_from_point_grid(myGraph, &aOpts, &aFace), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aFace.bits, 0u);
}

TEST_F(TopoAlgoTest, FaceFromBoundaryCurves_FourLines_ReturnsFace)
{
  const occtl_rep_id_t aBottom =
    makeTrimmedLine(myGraph, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
  const occtl_rep_id_t aRight =
    makeTrimmedLine(myGraph, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
  const occtl_rep_id_t aTop = makeTrimmedLine(myGraph, {1.0, 1.0, 0.0}, {-1.0, 0.0, 0.0}, 0.0, 1.0);
  const occtl_rep_id_t aLeft =
    makeTrimmedLine(myGraph, {0.0, 1.0, 0.0}, {0.0, -1.0, 0.0}, 0.0, 1.0);

  const occtl_rep_id_t                           aCurves[4] = {aBottom, aRight, aTop, aLeft};
  occtl_prim_face_from_boundary_curves_options_t aOpts =
    OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_INIT;
  aOpts.surface.curves      = aCurves;
  aOpts.surface.curve_count = 4;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_face_from_boundary_curves(myGraph, &aOpts, &aFace), OCCTL_OK);
  EXPECT_NE(aFace.bits, 0u);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(TopoAlgoTest, FaceFromBoundaryCurves_BadOptions_ReturnsInvalidArgument)
{
  occtl_prim_face_from_boundary_curves_options_t aOpts =
    OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_INIT;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_face_from_boundary_curves(myGraph, &aOpts, &aFace),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aFace.bits, 0u);
}

TEST_F(TopoAlgoTest, FaceFromCurveGrid_TwoByTwoLines_ReturnsFace)
{
  const occtl_rep_id_t aU0 = makeTrimmedLine(myGraph, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
  const occtl_rep_id_t aU1 = makeTrimmedLine(myGraph, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
  const occtl_rep_id_t aV0 = makeTrimmedLine(myGraph, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
  const occtl_rep_id_t aV1 = makeTrimmedLine(myGraph, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);

  const occtl_rep_id_t                      aUCurves[2] = {aU0, aU1};
  const occtl_rep_id_t                      aVCurves[2] = {aV0, aV1};
  occtl_prim_face_from_curve_grid_options_t aOpts = OCCTL_PRIM_FACE_FROM_CURVE_GRID_OPTIONS_INIT;
  aOpts.surface.u_curves                          = aUCurves;
  aOpts.surface.u_curve_count                     = 2;
  aOpts.surface.v_curves                          = aVCurves;
  aOpts.surface.v_curve_count                     = 2;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_face_from_curve_grid(myGraph, &aOpts, &aFace), OCCTL_OK);
  EXPECT_NE(aFace.bits, 0u);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(TopoAlgoTest, FaceFromCurveGrid_BadOptions_ReturnsInvalidArgument)
{
  occtl_prim_face_from_curve_grid_options_t aOpts = OCCTL_PRIM_FACE_FROM_CURVE_GRID_OPTIONS_INIT;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_face_from_curve_grid(myGraph, &aOpts, &aFace), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aFace.bits, 0u);
}

TEST_F(TopoAlgoTest, DraftFaces_SelectedFace_ReturnsNewGraph)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  occtl_topo_draft_faces_options_t aOpts = OCCTL_TOPO_DRAFT_FACES_OPTIONS_INIT;
  aOpts.root                             = aRoot;
  aOpts.faces                            = &aFace;
  aOpts.face_count                       = 1;
  aOpts.angle                            = 0.05;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_draft_faces(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aOutGraph), size_t{1});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, DraftFacesWithHistory_SelectedFace_ReportsImages)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  occtl_uid_t aFaceUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(myGraph, aFace, &aFaceUid), OCCTL_OK);

  occtl_topo_draft_faces_options_t aOpts = OCCTL_TOPO_DRAFT_FACES_OPTIONS_INIT;
  aOpts.root                             = aRoot;
  aOpts.faces                            = &aFace;
  aOpts.face_count                       = 1;
  aOpts.angle                            = 0.05;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_draft_faces(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);

  size_t aModifiedCount  = 0;
  size_t aGeneratedCount = 0;
  ASSERT_EQ(occtl_graph_history_modified(aOutGraph, aFaceUid, nullptr, 0, &aModifiedCount),
            OCCTL_OK);
  ASSERT_EQ(occtl_graph_history_generated(aOutGraph, aFaceUid, nullptr, 0, &aGeneratedCount),
            OCCTL_OK);
  EXPECT_GT(aModifiedCount + aGeneratedCount, 0u);
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, RemoveFeaturesOptions_Init_HasV1AndDefaults)
{
  occtl_topo_defeature_options_t aOpts{};
  occtl_topo_defeature_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_DEFEATURE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.root.bits, 0u);
  EXPECT_EQ(aOpts.selections, nullptr);
  EXPECT_EQ(aOpts.selection_count, 0u);
  EXPECT_EQ(aOpts.parallel, 0);
}

TEST_F(TopoAlgoTest, RemoveFeatures_CylindricalHoleSideFace_ReturnsNewSolidGraph)
{
  occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
  aBox.dx                    = 10.0;
  aBox.dy                    = 10.0;
  aBox.dz                    = 10.0;

  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_node_id_t aBoxRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_box(aGraph, &aBox, &aBoxRoot), OCCTL_OK);

  occtl_prim_cylindrical_hole_info_t aHoleInfo = OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_INIT;
  aHoleInfo.base_shape                         = aBoxRoot;
  aHoleInfo.axis.location                      = {5.0, 5.0, -1.0};
  aHoleInfo.axis.direction                     = {0.0, 0.0, 1.0};
  aHoleInfo.radius                             = 1.0;

  occtl_node_id_t aHoleRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_cylindrical_hole(aGraph, &aHoleInfo, &aHoleRoot), OCCTL_OK);

  const occtl_node_id_t aHoleFace = firstCylindricalFace(aGraph, aHoleRoot);
  ASSERT_NE(aHoleFace.bits, 0u);

  occtl_topo_defeature_options_t aOpts = OCCTL_TOPO_DEFEATURE_OPTIONS_INIT;
  aOpts.root                           = aHoleRoot;
  aOpts.selections                     = &aHoleFace;
  aOpts.selection_count                = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_defeature(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aOutGraph), size_t{1});
  EXPECT_LT(occtl_graph_count_value(occtl_graph_face_count, aOutGraph),
            childFaces(aGraph, aHoleRoot).size());

  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, RemoveFeaturesWithHistory_CylindricalHoleSideFace_ReportsDeletedUid)
{
  occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
  aBox.dx                    = 10.0;
  aBox.dy                    = 10.0;
  aBox.dz                    = 10.0;

  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_node_id_t aBoxRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_box(aGraph, &aBox, &aBoxRoot), OCCTL_OK);

  occtl_prim_cylindrical_hole_info_t aHoleInfo = OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_INIT;
  aHoleInfo.base_shape                         = aBoxRoot;
  aHoleInfo.axis.location                      = {5.0, 5.0, -1.0};
  aHoleInfo.axis.direction                     = {0.0, 0.0, 1.0};
  aHoleInfo.radius                             = 1.0;

  occtl_node_id_t aHoleRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_cylindrical_hole(aGraph, &aHoleInfo, &aHoleRoot), OCCTL_OK);

  const occtl_node_id_t aHoleFace = firstCylindricalFace(aGraph, aHoleRoot);
  ASSERT_NE(aHoleFace.bits, 0u);

  occtl_uid_t aHoleFaceUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(aGraph, aHoleFace, &aHoleFaceUid), OCCTL_OK);

  occtl_topo_defeature_options_t aOpts = OCCTL_TOPO_DEFEATURE_OPTIONS_INIT;
  aOpts.root                           = aHoleRoot;
  aOpts.selections                     = &aHoleFace;
  aOpts.selection_count                = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_defeature(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);

  size_t aDeletedCount = 0;
  ASSERT_EQ(occtl_graph_history_deleted_all(aOutGraph, nullptr, 0, &aDeletedCount), OCCTL_OK);
  ASSERT_GT(aDeletedCount, 0u);

  std::vector<occtl_uid_t> aDeleted(aDeletedCount);
  ASSERT_EQ(
    occtl_graph_history_deleted_all(aOutGraph, aDeleted.data(), aDeleted.size(), &aDeletedCount),
    OCCTL_OK);
  EXPECT_NE(
    std::find_if(aDeleted.begin(),
                 aDeleted.end(),
                 [&](const occtl_uid_t theUid) { return theUid.bits == aHoleFaceUid.bits; }),
    aDeleted.end());
  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, RemoveFeatures_EmptySelection_ReturnsInvalidArgument)
{
  occtl_topo_defeature_options_t aOpts = OCCTL_TOPO_DEFEATURE_OPTIONS_INIT;
  aOpts.root                           = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_defeature(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, RemoveFeatures_BadVersion_ReturnsVersionMismatch)
{
  occtl_topo_defeature_options_t aOpts = OCCTL_TOPO_DEFEATURE_OPTIONS_INIT;
  aOpts.struct_version                 = 999u;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_defeature(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, RemoveFeatures_WrongKindSelection_ReturnsWrongKind)
{
  const occtl_node_id_t aRoot  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_topo_defeature_options_t aOpts = OCCTL_TOPO_DEFEATURE_OPTIONS_INIT;
  aOpts.root                           = aRoot;
  aOpts.selections                     = &anEdge;
  aOpts.selection_count                = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_defeature(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_WRONG_KIND);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, OffsetFeaturesOptions_Init_HasV1AndDefaults)
{
  occtl_topo_offset_features_options_t aOpts{};
  occtl_topo_offset_features_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.root.bits, 0u);
  EXPECT_EQ(aOpts.selections, nullptr);
  EXPECT_EQ(aOpts.selection_count, 0u);
  EXPECT_EQ(aOpts.base_offset, 0.0);
  EXPECT_EQ(aOpts.selection_offset, 1.0);
  EXPECT_EQ(aOpts.tolerance, 1.0e-3);
  EXPECT_EQ(aOpts.join, OCCTL_OFFSET_JOIN_ARC);
}

TEST_F(TopoAlgoTest, OffsetFeatures_SelectedBoxFace_ReturnsNewGraph)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  occtl_topo_offset_features_options_t aOpts = OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT;
  aOpts.root                                 = aRoot;
  aOpts.selections                           = &aFace;
  aOpts.selection_count                      = 1;
  aOpts.selection_offset                     = 0.25;
  aOpts.join                                 = OCCTL_OFFSET_JOIN_INTERSECTION;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_offset_features(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, aOutGraph), size_t{0});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, OffsetFeaturesWithHistory_SelectedBoxFace_ReportsImages)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(aFace.bits, 0u);

  occtl_uid_t aFaceUid = OCCTL_UID_INVALID;
  ASSERT_EQ(occtl_graph_uid_from_node_id(myGraph, aFace, &aFaceUid), OCCTL_OK);

  occtl_topo_offset_features_options_t aOpts = OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT;
  aOpts.root                                 = aRoot;
  aOpts.selections                           = &aFace;
  aOpts.selection_count                      = 1;
  aOpts.selection_offset                     = 0.25;
  aOpts.join                                 = OCCTL_OFFSET_JOIN_INTERSECTION;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_offset_features(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);

  size_t aModifiedCount  = 0;
  size_t aGeneratedCount = 0;
  ASSERT_EQ(occtl_graph_history_modified(aOutGraph, aFaceUid, nullptr, 0, &aModifiedCount),
            OCCTL_OK);
  ASSERT_EQ(occtl_graph_history_generated(aOutGraph, aFaceUid, nullptr, 0, &aGeneratedCount),
            OCCTL_OK);
  EXPECT_GT(aModifiedCount + aGeneratedCount, 0u);
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, OffsetFeatures_EmptySelection_ReturnsInvalidArgument)
{
  occtl_topo_offset_features_options_t aOpts = OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT;
  aOpts.root                                 = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_offset_features(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, OffsetFeatures_BadVersion_ReturnsVersionMismatch)
{
  occtl_topo_offset_features_options_t aOpts = OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT;
  aOpts.struct_version                       = 999u;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_offset_features(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, OffsetFeatures_WrongKindSelection_ReturnsWrongKind)
{
  const occtl_node_id_t aRoot  = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(aRoot.bits, 0u);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_topo_offset_features_options_t aOpts = OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT;
  aOpts.root                                 = aRoot;
  aOpts.selections                           = &anEdge;
  aOpts.selection_count                      = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_offset_features(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_WRONG_KIND);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, MakeFilling_RectangleBoundary_ReturnsFaceGraph)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t              aWire   = makeRectangle(aGraph, 0.0, 3.0, 2.0);
  const std::vector<occtl_node_id_t> anEdges = childEdges(aGraph, aWire);
  ASSERT_EQ(anEdges.size(), size_t{4});

  occtl_topo_filling_options_t aOpts = OCCTL_TOPO_FILLING_OPTIONS_INIT;
  aOpts.edges                        = anEdges.data();
  aOpts.edge_count                   = anEdges.size();

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_filling(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aOutGraph), size_t{1});

  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, MakeFillingWithHistory_RectangleBoundary_ReportsGeneratedFace)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t              aWire   = makeRectangle(aGraph, 0.0, 3.0, 2.0);
  const std::vector<occtl_node_id_t> anEdges = childEdges(aGraph, aWire);
  ASSERT_EQ(anEdges.size(), size_t{4});

  occtl_topo_filling_options_t aOpts = OCCTL_TOPO_FILLING_OPTIONS_INIT;
  aOpts.edges                        = anEdges.data();
  aOpts.edge_count                   = anEdges.size();

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_filling(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aOutGraph), size_t{1});

  size_t aTotalGenerated = 0;
  for (const occtl_node_id_t anEdge : anEdges)
  {
    occtl_uid_t anEdgeUid = OCCTL_UID_INVALID;
    ASSERT_EQ(occtl_graph_uid_from_node_id(aGraph, anEdge, &anEdgeUid), OCCTL_OK);
    size_t aGeneratedCount = 0;
    ASSERT_EQ(occtl_graph_history_generated(aOutGraph, anEdgeUid, nullptr, 0, &aGeneratedCount),
              OCCTL_OK);
    aTotalGenerated += aGeneratedCount;
  }
  EXPECT_GT(aTotalGenerated, size_t{0});
  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, MakeFilling_EmptySelection_ReturnsInvalidArgument)
{
  occtl_topo_filling_options_t aOpts = OCCTL_TOPO_FILLING_OPTIONS_INIT;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_filling(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, FillingPatch_RectangleBoundaryAndPoint_ReturnsFaceGraph)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t              aWire   = makeRectangle(aGraph, 0.0, 3.0, 2.0);
  const std::vector<occtl_node_id_t> anEdges = childEdges(aGraph, aWire);
  ASSERT_EQ(anEdges.size(), size_t{4});

  std::vector<occtl_topo_filling_patch_edge_t> aConstraints;
  aConstraints.reserve(anEdges.size());
  for (const occtl_node_id_t anEdge : anEdges)
  {
    occtl_topo_filling_patch_edge_t aConstraint{};
    aConstraint.edge         = anEdge;
    aConstraint.support_face = OCCTL_NODE_ID_INVALID;
    aConstraint.continuity   = OCCTL_TOPO_FILLING_C0;
    aConstraint.is_boundary  = 1;
    aConstraints.push_back(aConstraint);
  }

  const occtl_point3_t               aPoint = {0.0, 0.0, 0.2};
  occtl_topo_filling_patch_options_t aOpts  = OCCTL_TOPO_FILLING_PATCH_OPTIONS_INIT;
  aOpts.edges                               = aConstraints.data();
  aOpts.edge_count                          = aConstraints.size();
  aOpts.points                              = &aPoint;
  aOpts.point_count                         = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_filling_patch(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aOutGraph), size_t{1});

  occtl_graph_free(aOutGraph);
  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, FillingPatchOptions_Init_HasDefaults)
{
  occtl_topo_filling_patch_options_t aOpts{};
  occtl_topo_filling_patch_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_FILLING_PATCH_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.edges, nullptr);
  EXPECT_EQ(aOpts.edge_count, size_t{0});
  EXPECT_EQ(aOpts.points, nullptr);
  EXPECT_EQ(aOpts.point_count, size_t{0});
  EXPECT_EQ(aOpts.degree, 3);
  EXPECT_EQ(aOpts.point_count_on_curve, 15);
  EXPECT_EQ(aOpts.iteration_count, 2);
}

TEST_F(TopoAlgoTest, FillingPatch_BadVersion_ReturnsVersionMismatch)
{
  occtl_topo_filling_patch_options_t aOpts = OCCTL_TOPO_FILLING_PATCH_OPTIONS_INIT;
  aOpts.struct_version                     = 999u;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_filling_patch(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aOutGraph, nullptr);
}

TEST_F(TopoAlgoTest, FillingPatch_G1WithoutSupport_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t              aWire   = makeRectangle(aGraph, 0.0, 3.0, 2.0);
  const std::vector<occtl_node_id_t> anEdges = childEdges(aGraph, aWire);
  ASSERT_EQ(anEdges.size(), size_t{4});

  std::vector<occtl_topo_filling_patch_edge_t> aConstraints;
  aConstraints.reserve(anEdges.size());
  for (const occtl_node_id_t anEdge : anEdges)
  {
    occtl_topo_filling_patch_edge_t aConstraint{};
    aConstraint.edge         = anEdge;
    aConstraint.support_face = OCCTL_NODE_ID_INVALID;
    aConstraint.continuity   = OCCTL_TOPO_FILLING_C0;
    aConstraint.is_boundary  = 1;
    aConstraints.push_back(aConstraint);
  }
  aConstraints[0].continuity = OCCTL_TOPO_FILLING_G1;

  occtl_topo_filling_patch_options_t aOpts = OCCTL_TOPO_FILLING_PATCH_OPTIONS_INIT;
  aOpts.edges                              = aConstraints.data();
  aOpts.edge_count                         = aConstraints.size();

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_filling_patch(aGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);

  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, FillingPatch_WrongKindSupport_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_node_id_t              aWire   = makeRectangle(aGraph, 0.0, 3.0, 2.0);
  const std::vector<occtl_node_id_t> anEdges = childEdges(aGraph, aWire);
  ASSERT_EQ(anEdges.size(), size_t{4});

  std::vector<occtl_topo_filling_patch_edge_t> aConstraints;
  aConstraints.reserve(anEdges.size());
  for (const occtl_node_id_t anEdge : anEdges)
  {
    occtl_topo_filling_patch_edge_t aConstraint{};
    aConstraint.edge         = anEdge;
    aConstraint.support_face = OCCTL_NODE_ID_INVALID;
    aConstraint.continuity   = OCCTL_TOPO_FILLING_C0;
    aConstraint.is_boundary  = 1;
    aConstraints.push_back(aConstraint);
  }
  aConstraints[0].support_face = aWire;

  occtl_topo_filling_patch_options_t aOpts = OCCTL_TOPO_FILLING_PATCH_OPTIONS_INIT;
  aOpts.edges                              = aConstraints.data();
  aOpts.edge_count                         = aConstraints.size();

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_filling_patch(aGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_WRONG_KIND);
  EXPECT_EQ(aOutGraph, nullptr);

  occtl_graph_free(aGraph);
}

TEST_F(TopoAlgoTest, SplitByPlaneOptions_Init_HasV1AndDefaults)
{
  occtl_topo_split_by_plane_options_t aOpts{};
  occtl_topo_split_by_plane_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.root.bits, 0u);
  EXPECT_DOUBLE_EQ(aOpts.point.x, 0.0);
  EXPECT_DOUBLE_EQ(aOpts.point.y, 0.0);
  EXPECT_DOUBLE_EQ(aOpts.point.z, 0.0);
  EXPECT_DOUBLE_EQ(aOpts.normal.x, 0.0);
  EXPECT_DOUBLE_EQ(aOpts.normal.y, 0.0);
  EXPECT_DOUBLE_EQ(aOpts.normal.z, 1.0);
  EXPECT_EQ(aOpts.keep, OCCTL_TOPO_SPLIT_KEEP_ALL);
}

TEST_F(TopoAlgoTest, SplitByPlane_BoxKeepAll_ReturnsSplitGraph)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aRoot.bits, 0u);

  occtl_topo_split_by_plane_options_t aOpts = OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_INIT;
  aOpts.root                                = aRoot;
  aOpts.point                               = {5.0, 0.0, 0.0};
  aOpts.normal                              = {1.0, 0.0, 0.0};

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_split_by_plane(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_GE(occtl_graph_count_value(occtl_graph_solid_count, aOutGraph), size_t{2});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, SplitByPlane_BoxKeepPositive_ReturnsGraph)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aRoot.bits, 0u);

  occtl_topo_split_by_plane_options_t aOpts = OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_INIT;
  aOpts.root                                = aRoot;
  aOpts.point                               = {5.0, 0.0, 0.0};
  aOpts.normal                              = {1.0, 0.0, 0.0};
  aOpts.keep                                = OCCTL_TOPO_SPLIT_KEEP_POSITIVE;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_split_by_plane(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_GE(occtl_graph_count_value(occtl_graph_solid_count, aOutGraph), size_t{1});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, SplitByPlane_BadVersion_ReturnsVersionMismatch)
{
  occtl_topo_split_by_plane_options_t aOpts = OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_INIT;
  aOpts.struct_version                      = 999u;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_split_by_plane(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aOutGraph, nullptr);
  EXPECT_EQ(aOutRoot.bits, 0u);
}

TEST_F(TopoAlgoTest, SplitByPlane_InvalidRoot_ReturnsNotFound)
{
  occtl_topo_split_by_plane_options_t aOpts = OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_INIT;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_split_by_plane(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_NOT_FOUND);
  EXPECT_EQ(aOutGraph, nullptr);
  EXPECT_EQ(aOutRoot.bits, 0u);
}

TEST_F(TopoAlgoTest, SectionByPlanesOptions_Init_HasV1AndDefaults)
{
  occtl_topo_section_by_planes_options_t aOpts{};
  occtl_topo_section_by_planes_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.root.bits, 0u);
  EXPECT_EQ(aOpts.planes, nullptr);
  EXPECT_EQ(aOpts.plane_count, 0u);
  EXPECT_EQ(aOpts.approximate, 1);
  EXPECT_EQ(aOpts.compute_pcurves_on_root, 0);
  EXPECT_EQ(aOpts.compute_pcurves_on_plane, 0);
}

TEST_F(TopoAlgoTest, SectionByPlanes_BoxOnePlane_ReturnsEdgesGraph)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aRoot.bits, 0u);

  const occtl_topo_section_plane_t       aPlane = {{5.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_topo_section_by_planes_options_t aOpts  = OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_INIT;
  aOpts.root                                    = aRoot;
  aOpts.planes                                  = &aPlane;
  aOpts.plane_count                             = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_sections_by_planes(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_edge_count, aOutGraph), size_t{0});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, SectionByPlanes_BoxTwoPlanes_ReturnsCompoundGraph)
{
  const occtl_node_id_t aRoot = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  ASSERT_NE(aRoot.bits, 0u);

  const occtl_topo_section_plane_t       aPlanes[2] = {{{5.0, 0.0, 0.0}, {1.0, 0.0, 0.0}},
                                                       {{0.0, 10.0, 0.0}, {0.0, 1.0, 0.0}}};
  occtl_topo_section_by_planes_options_t aOpts      = OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_INIT;
  aOpts.root                                        = aRoot;
  aOpts.planes                                      = aPlanes;
  aOpts.plane_count                                 = 2;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_sections_by_planes(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_GE(occtl_graph_count_value(occtl_graph_compound_count, aOutGraph), size_t{1});
  EXPECT_GT(occtl_graph_count_value(occtl_graph_edge_count, aOutGraph), size_t{0});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, SectionByPlanes_EmptyPlanes_ReturnsInvalidArgument)
{
  occtl_topo_section_by_planes_options_t aOpts = OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_INIT;
  aOpts.root                                   = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_sections_by_planes(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
  EXPECT_EQ(aOutRoot.bits, 0u);
}

TEST_F(TopoAlgoTest, SectionByPlanes_BadVersion_ReturnsVersionMismatch)
{
  const occtl_topo_section_plane_t       aPlane = {{5.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_topo_section_by_planes_options_t aOpts  = OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_INIT;
  aOpts.struct_version                          = 999u;
  aOpts.planes                                  = &aPlane;
  aOpts.plane_count                             = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_sections_by_planes(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aOutGraph, nullptr);
  EXPECT_EQ(aOutRoot.bits, 0u);
}

TEST_F(TopoAlgoTest, ExtrudeFacesOptions_Init_HasV1AndDefaults)
{
  occtl_topo_extrude_faces_options_t aOpts{};
  occtl_topo_extrude_faces_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.faces, nullptr);
  EXPECT_EQ(aOpts.face_count, 0u);
  EXPECT_DOUBLE_EQ(aOpts.thickness, 1.0);
  EXPECT_EQ(aOpts.both_sides, 0);
  EXPECT_EQ(aOpts.use_normal, 0);
  EXPECT_DOUBLE_EQ(aOpts.normal.x, 0.0);
  EXPECT_DOUBLE_EQ(aOpts.normal.y, 0.0);
  EXPECT_DOUBLE_EQ(aOpts.normal.z, 1.0);
  EXPECT_EQ(aOpts.copy, 1);
  EXPECT_EQ(aOpts.canonize, 1);
}

TEST_F(TopoAlgoTest, ExtrudeFaces_BoxFace_ReturnsSolidGraph)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  occtl_topo_extrude_faces_options_t aOpts = OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_INIT;
  aOpts.faces                              = &aFace;
  aOpts.face_count                         = 1;
  aOpts.thickness                          = 2.0;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_face_extrusion(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aOutGraph), size_t{1});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, ExtrudeFaces_TwoFacesBothSides_ReturnsCompoundGraph)
{
  const std::vector<occtl_node_id_t> aFaces =
    childFaces(myGraph, firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID));
  ASSERT_GE(aFaces.size(), 2u);
  const occtl_node_id_t aSelected[2] = {aFaces[0], aFaces[1]};

  occtl_topo_extrude_faces_options_t aOpts = OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_INIT;
  aOpts.faces                              = aSelected;
  aOpts.face_count                         = 2;
  aOpts.thickness                          = 1.0;
  aOpts.both_sides                         = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_face_extrusion(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_GE(occtl_graph_count_value(occtl_graph_compound_count, aOutGraph), size_t{1});
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aOutGraph), size_t{2});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, ExtrudeFaces_InvalidOptions_ReturnsInvalidArgument)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  occtl_topo_extrude_faces_options_t aOpts = OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_INIT;
  aOpts.faces                              = &aFace;
  aOpts.face_count                         = 1;
  aOpts.thickness                          = 0.0;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_face_extrusion(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
  EXPECT_EQ(aOutRoot.bits, 0u);
}

TEST_F(TopoAlgoTest, ExtrudeFaces_WrongKind_ReturnsWrongKind)
{
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_topo_extrude_faces_options_t aOpts = OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_INIT;
  aOpts.faces                              = &anEdge;
  aOpts.face_count                         = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_topo_make_face_extrusion(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_WRONG_KIND);
  EXPECT_EQ(aOutGraph, nullptr);
  EXPECT_EQ(aOutRoot.bits, 0u);
}

TEST_F(TopoAlgoTest, BrakeFormedOptions_Init_HasV1AndDefaults)
{
  occtl_prim_brake_formed_options_t aOpts{};
  occtl_prim_brake_formed_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_PRIM_BRAKE_FORMED_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.p_next, nullptr);
  EXPECT_EQ(aOpts.line.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(aOpts.thickness, 1.0);
  EXPECT_EQ(aOpts.station_widths, nullptr);
  EXPECT_EQ(aOpts.station_width_count, 0u);
  EXPECT_EQ(aOpts.side, OCCTL_PRIM_BRAKE_SIDE_LEFT);
  EXPECT_EQ(aOpts.join, OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC);
  EXPECT_EQ(aOpts.approximate, 0);
  EXPECT_DOUBLE_EQ(aOpts.tolerance, 1.0e-6);
}

TEST_F(TopoAlgoTest, BrakeFormed_OpenPolylineSingleWidth_ReturnsSolidGraph)
{
  const occtl_point3_t       aPoints[3] = {{0.0, 0.0, 0.0}, {5.0, 0.0, 0.0}, {5.0, 3.0, 0.0}};
  occtl_prim_polyline_info_t aLineInfo  = OCCTL_PRIM_POLYLINE_INFO_INIT;
  aLineInfo.points                      = aPoints;
  aLineInfo.point_count                 = 3;

  occtl_node_id_t aLine = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_polyline(myGraph, &aLineInfo, &aLine), OCCTL_OK);

  const double                      aWidth = 2.0;
  occtl_prim_brake_formed_options_t aOpts  = OCCTL_PRIM_BRAKE_FORMED_OPTIONS_INIT;
  aOpts.line                               = aLine;
  aOpts.thickness                          = 0.25;
  aOpts.station_widths                     = &aWidth;
  aOpts.station_width_count                = 1;
  aOpts.side                               = OCCTL_PRIM_BRAKE_SIDE_RIGHT;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_brake_formed(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aOutGraph), size_t{1});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, BrakeFormed_PerStationWidths_ReturnsSolidGraph)
{
  const occtl_point3_t       aPoints[4] = {{0.0, 0.0, 0.0},
                                           {4.0, 0.0, 0.0},
                                           {4.0, 2.0, 0.0},
                                           {7.0, 2.0, 0.0}};
  occtl_prim_polyline_info_t aLineInfo  = OCCTL_PRIM_POLYLINE_INFO_INIT;
  aLineInfo.points                      = aPoints;
  aLineInfo.point_count                 = 4;

  occtl_node_id_t aLine = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_polyline(myGraph, &aLineInfo, &aLine), OCCTL_OK);

  const double                      aWidths[4] = {1.0, 1.5, 2.0, 1.25};
  occtl_prim_brake_formed_options_t aOpts      = OCCTL_PRIM_BRAKE_FORMED_OPTIONS_INIT;
  aOpts.line                                   = aLine;
  aOpts.thickness                              = 0.2;
  aOpts.station_widths                         = aWidths;
  aOpts.station_width_count                    = 4;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_brake_formed(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_OK);
  ASSERT_NE(aOutGraph, nullptr);
  EXPECT_NE(aOutRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aOutGraph), size_t{1});
  occtl_graph_free(aOutGraph);
}

TEST_F(TopoAlgoTest, BrakeFormed_BadWidthCount_ReturnsInvalidArgument)
{
  const occtl_point3_t       aPoints[3] = {{0.0, 0.0, 0.0}, {5.0, 0.0, 0.0}, {5.0, 3.0, 0.0}};
  occtl_prim_polyline_info_t aLineInfo  = OCCTL_PRIM_POLYLINE_INFO_INIT;
  aLineInfo.points                      = aPoints;
  aLineInfo.point_count                 = 3;

  occtl_node_id_t aLine = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_polyline(myGraph, &aLineInfo, &aLine), OCCTL_OK);

  const double                      aWidths[2] = {1.0, 2.0};
  occtl_prim_brake_formed_options_t aOpts      = OCCTL_PRIM_BRAKE_FORMED_OPTIONS_INIT;
  aOpts.line                                   = aLine;
  aOpts.thickness                              = 0.25;
  aOpts.station_widths                         = aWidths;
  aOpts.station_width_count                    = 2;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_brake_formed(myGraph, &aOpts, &aOutGraph, &aOutRoot),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aOutGraph, nullptr);
  EXPECT_EQ(aOutRoot.bits, 0u);
}

TEST_F(TopoAlgoTest, BrakeFormed_WrongKind_ReturnsWrongKind)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  const double                      aWidth = 2.0;
  occtl_prim_brake_formed_options_t aOpts  = OCCTL_PRIM_BRAKE_FORMED_OPTIONS_INIT;
  aOpts.line                               = aFace;
  aOpts.thickness                          = 0.25;
  aOpts.station_widths                     = &aWidth;
  aOpts.station_width_count                = 1;

  occtl_graph_t*  aOutGraph = nullptr;
  occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_brake_formed(myGraph, &aOpts, &aOutGraph, &aOutRoot), OCCTL_WRONG_KIND);
  EXPECT_EQ(aOutGraph, nullptr);
  EXPECT_EQ(aOutRoot.bits, 0u);
}

TEST_F(TopoAlgoTest, Check_BoxHasNoIssues)
{
  size_t aCount = 0xdeadu;
  EXPECT_EQ(occtl_topo_check(myGraph, nullptr, 0, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 0u);
}

TEST_F(TopoAlgoTest, Check_NullCount_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_topo_check(myGraph, nullptr, 0, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

TEST_F(TopoAlgoTest, Check_NullGraph_ReturnsInvalidArgument)
{
  size_t aCount = 0;
  EXPECT_EQ(occtl_topo_check(nullptr, nullptr, 0, &aCount), OCCTL_INVALID_ARGUMENT);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

} // namespace
