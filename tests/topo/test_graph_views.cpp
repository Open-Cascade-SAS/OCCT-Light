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

#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>

#include "test_helpers_internal.hxx"

#include <cstring>

namespace
{

class TopoViewTest : public ::testing::Test
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

TEST_F(TopoViewTest, EdgeViewInit_SetsVersionAndZeroes)
{
  occtl_edge_view_t aView;
  occtl_edge_view_init(&aView);
  EXPECT_EQ(aView.struct_version, OCCTL_EDGE_VIEW_VERSION_1);
  EXPECT_EQ(aView.p_next, nullptr);
  EXPECT_EQ(aView.start_vertex.bits, 0u);
  EXPECT_EQ(aView.face_count, 0u);
  EXPECT_EQ(aView.has_curve, 0);
}

TEST_F(TopoViewTest, EdgeView_FillsExpectedScalarsForBoxEdge)
{
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_edge_view_t aView;
  occtl_edge_view_init(&aView);
  ASSERT_EQ(occtl_topo_edge_view(myGraph, anEdge, &aView), OCCTL_OK);

  // Box edges have a 3D curve, two distinct end vertices, two incident faces,
  // SameParameter+SameRange set by BRepBuilderAPI_MakeEdge.
  EXPECT_EQ(aView.has_curve, 1);
  EXPECT_EQ(aView.is_degenerated, 0);
  EXPECT_EQ(aView.is_closed, 0);
  EXPECT_EQ(aView.same_parameter, 1);
  EXPECT_EQ(aView.same_range, 1);
  EXPECT_NE(aView.start_vertex.bits, 0u);
  EXPECT_NE(aView.end_vertex.bits, 0u);
  EXPECT_NE(aView.start_vertex.bits, aView.end_vertex.bits);
  EXPECT_EQ(aView.internal_vertex_count, 0u);
  EXPECT_EQ(aView.face_count, 2u);
  EXPECT_LT(aView.t_min, aView.t_max);
  EXPECT_GT(aView.tolerance, 0.0);
}

TEST_F(TopoViewTest, EdgeView_VersionMismatch_ReturnsVersionMismatch)
{
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);
  occtl_edge_view_t aView{};
  aView.struct_version = 0u;
  EXPECT_EQ(occtl_topo_edge_view(myGraph, anEdge, &aView), OCCTL_VERSION_MISMATCH);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

TEST_F(TopoViewTest, EdgeView_NullView_ReturnsInvalidArgument)
{
  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  EXPECT_EQ(occtl_topo_edge_view(myGraph, anEdge, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(TopoViewTest, EdgeView_WrongKind_ReturnsWrongKind)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  occtl_edge_view_t     aView;
  occtl_edge_view_init(&aView);
  EXPECT_EQ(occtl_topo_edge_view(myGraph, aFace, &aView), OCCTL_WRONG_KIND);
}

TEST_F(TopoViewTest, CoEdgeView_FillsExpectedScalars)
{
  const occtl_node_id_t aCoEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_COEDGE);
  ASSERT_NE(aCoEdge.bits, 0u);

  occtl_coedge_view_t aView;
  occtl_coedge_view_init(&aView);
  ASSERT_EQ(occtl_topo_coedge_view(myGraph, aCoEdge, &aView), OCCTL_OK);

  EXPECT_NE(aView.edge_of.bits, 0u);
  EXPECT_NE(aView.face_of.bits, 0u);
  EXPECT_LT(aView.t_min, aView.t_max);
  // Box coedges carry a pcurve on their parent planar face.
  EXPECT_EQ(aView.has_pcurve, 1);
}

TEST_F(TopoViewTest, FaceView_FillsExpectedScalarsForBoxFace)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  ASSERT_NE(aFace.bits, 0u);

  occtl_face_view_t aView;
  occtl_face_view_init(&aView);
  ASSERT_EQ(occtl_topo_face_view(myGraph, aFace, &aView), OCCTL_OK);

  EXPECT_EQ(aView.has_surface, 1);
  EXPECT_EQ(aView.has_triangulation, 0); // box has no mesh yet
  EXPECT_EQ(aView.wire_count, 1u);
  EXPECT_NE(aView.outer_wire.bits, 0u);
  EXPECT_LT(aView.u_min, aView.u_max);
  EXPECT_LT(aView.v_min, aView.v_max);
  EXPECT_GT(aView.tolerance, 0.0);
}

TEST_F(TopoViewTest, FaceView_VersionMismatch_ReturnsVersionMismatch)
{
  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  occtl_face_view_t     aView{};
  aView.struct_version = 999u;
  EXPECT_EQ(occtl_topo_face_view(myGraph, aFace, &aView), OCCTL_VERSION_MISMATCH);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

TEST_F(TopoViewTest, AggregateViews_RejectNonNullPNext)
{
  int aTag = 1;

  const occtl_node_id_t anEdge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  occtl_edge_view_t     anEdgeView;
  occtl_edge_view_init(&anEdgeView);
  anEdgeView.p_next = &aTag;
  EXPECT_EQ(occtl_topo_edge_view(myGraph, anEdge, &anEdgeView), OCCTL_INVALID_ARGUMENT);

  const occtl_node_id_t aCoedge = firstAbiNodeOfKind(myGraph, OCCTL_KIND_COEDGE);
  occtl_coedge_view_t   aCoedgeView;
  occtl_coedge_view_init(&aCoedgeView);
  aCoedgeView.p_next = &aTag;
  EXPECT_EQ(occtl_topo_coedge_view(myGraph, aCoedge, &aCoedgeView), OCCTL_INVALID_ARGUMENT);

  const occtl_node_id_t aFace = firstAbiNodeOfKind(myGraph, OCCTL_KIND_FACE);
  occtl_face_view_t     aFaceView;
  occtl_face_view_init(&aFaceView);
  aFaceView.p_next = &aTag;
  EXPECT_EQ(occtl_topo_face_view(myGraph, aFace, &aFaceView), OCCTL_INVALID_ARGUMENT);

  const occtl_node_id_t aVertex = firstAbiNodeOfKind(myGraph, OCCTL_KIND_VERTEX);
  occtl_vertex_view_t   aVertexView;
  occtl_vertex_view_init(&aVertexView);
  aVertexView.p_next = &aTag;
  EXPECT_EQ(occtl_topo_vertex_view(myGraph, aVertex, &aVertexView), OCCTL_INVALID_ARGUMENT);

  const occtl_node_id_t aWire = firstAbiNodeOfKind(myGraph, OCCTL_KIND_WIRE);
  occtl_wire_view_t     aWireView;
  occtl_wire_view_init(&aWireView);
  aWireView.p_next = &aTag;
  EXPECT_EQ(occtl_topo_wire_view(myGraph, aWire, &aWireView), OCCTL_INVALID_ARGUMENT);

  const occtl_node_id_t aShell = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SHELL);
  occtl_shell_view_t    aShellView;
  occtl_shell_view_init(&aShellView);
  aShellView.p_next = &aTag;
  EXPECT_EQ(occtl_topo_shell_view(myGraph, aShell, &aShellView), OCCTL_INVALID_ARGUMENT);

  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph, OCCTL_KIND_SOLID);
  occtl_solid_view_t    aSolidView;
  occtl_solid_view_init(&aSolidView);
  aSolidView.p_next = &aTag;
  EXPECT_EQ(occtl_topo_solid_view(myGraph, aSolid, &aSolidView), OCCTL_INVALID_ARGUMENT);

  const occtl_node_id_t aCompound = firstAbiNodeOfKind(myGraph, OCCTL_KIND_COMPOUND);
  occtl_compound_view_t aCompoundView;
  occtl_compound_view_init(&aCompoundView);
  aCompoundView.p_next = &aTag;
  EXPECT_EQ(occtl_topo_compound_view(myGraph, aCompound, &aCompoundView), OCCTL_INVALID_ARGUMENT);
}

} // namespace
