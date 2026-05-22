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

#include "test_prim_helpers.hxx"

#include <occtl-hpp/prim.hpp>
#include <occtl/occtl_curves.h>

#include <cstring>

namespace
{

class PrimPipeShellTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    // Profile: a small circle in XY at the origin.
    occtl_geom_circle_t aCircGeom;
    aCircGeom.position        = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
    aCircGeom.radius          = 0.5;
    occtl_rep_id_t aCircCurve = {};
    ASSERT_EQ(occtl_curve_create_circle(myGraph, aCircGeom, &aCircCurve), OCCTL_OK);
    ASSERT_EQ(occtl_topo_curves_to_wire(myGraph, &aCircCurve, 1, &myProfile), OCCTL_OK);

    // Spine: straight wire from (0,0,0) to (0,0,5) along +Z.
    occtl_node_id_t aV0 = OCCTL_NODE_ID_INVALID, aV1 = OCCTL_NODE_ID_INVALID;
    {
      occtl_topo_make_vertex_info_t aVi = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
      aVi.tolerance                     = 1e-6;
      aVi.point                         = {0.0, 0.0, 0.0};
      ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVi, &aV0), OCCTL_OK);
      aVi.point = {0.0, 0.0, 5.0};
      ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVi, &aV1), OCCTL_OK);
    }

    occtl_geom_line_t aLine  = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    occtl_rep_id_t    aCurve = {};
    ASSERT_EQ(occtl_curve_create_line(myGraph, aLine, &aCurve), OCCTL_OK);

    occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
    {
      occtl_topo_make_edge_info_t aEi = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
      aEi.start_vertex                = aV0;
      aEi.end_vertex                  = aV1;
      aEi.curve                       = aCurve;
      aEi.first                       = 0.0;
      aEi.last                        = 5.0;
      aEi.tolerance                   = 1e-6;
      ASSERT_EQ(occtl_topo_make_edge(myGraph, &aEi, &anEdge), OCCTL_OK);
    }

    occtl_oriented_node_t       aOe = {anEdge, OCCTL_ORIENTATION_FORWARD};
    occtl_topo_make_wire_info_t aWi = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
    aWi.edges                       = &aOe;
    aWi.edge_count                  = 1;
    ASSERT_EQ(occtl_topo_make_wire(myGraph, &aWi, &mySpine), OCCTL_OK);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t*  myGraph   = nullptr;
  occtl_node_id_t myProfile = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t mySpine   = OCCTL_NODE_ID_INVALID;
};

TEST_F(PrimPipeShellTest, MakePipeShell_DefaultFrenet_CreatesShell)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                   = mySpine;
  anInfo.profiles                     = &myProfile;
  anInfo.profile_count                = 1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimPipeShellTest, MakePipeShell_MakeSolid_ReturnsSolid)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                   = mySpine;
  anInfo.profiles                     = &myProfile;
  anInfo.profile_count                = 1;
  anInfo.make_solid                   = 1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, &aShape), OCCTL_OK);
  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimPipeShellTest, MakePipeShell_ConstantBinormalMode_CreatesShape)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                   = mySpine;
  anInfo.profiles                     = &myProfile;
  anInfo.profile_count                = 1;
  anInfo.mode                         = OCCTL_PIPE_MODE_CONSTANT_BINORMAL;
  anInfo.mode_binormal                = {1.0, 0.0, 0.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimPipeShellTest, MakePipeShell_ZeroBinormal_InvalidArgument)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                   = mySpine;
  anInfo.profiles                     = &myProfile;
  anInfo.profile_count                = 1;
  anInfo.mode                         = OCCTL_PIPE_MODE_CONSTANT_BINORMAL;
  anInfo.mode_binormal                = {0.0, 0.0, 0.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimPipeShellTest, MakePipeShell_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                   = mySpine;
  anInfo.profiles                     = &myProfile;
  anInfo.profile_count                = 1;
  occtl_node_id_t aShape              = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_pipe_shell(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_pipe_shell(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimPipeShellTest, MakePipeShell_NonNullPNext_ReturnsInvalidArgument)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                   = mySpine;
  anInfo.profiles                     = &myProfile;
  anInfo.profile_count                = 1;
  anInfo.p_next                       = &anInfo;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimPipeShellTest, MakePipeShell_VersionMismatch_Rejected)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.struct_version               = 0u;
  anInfo.spine_wire                   = mySpine;
  anInfo.profiles                     = &myProfile;
  anInfo.profile_count                = 1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimPipeShellTest, MakePipeShell_NoProfiles_InvalidArgument)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                   = mySpine;
  anInfo.profiles                     = nullptr;
  anInfo.profile_count                = 0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimPipeShellTest, MakePipeShell_BadBoolean_ReturnsInvalidArgument)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                   = mySpine;
  anInfo.profiles                     = &myProfile;
  anInfo.profile_count                = 1;
  anInfo.with_contact                 = 2;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimPipeShellTest, MakePipeShell_NonWireSpine_WrongKind)
{
  occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                   = myProfile; // a Wire actually, but let's substitute a face
  anInfo.profiles                     = &myProfile;
  anInfo.profile_count                = 1;

  // myProfile is itself a Wire (circle), so feed something non-Wire:
  occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
  aBox.dx                    = 1.0;
  aBox.dy                    = 1.0;
  aBox.dz                    = 1.0;
  occtl_node_id_t aSolid     = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &aSolid), OCCTL_OK);
  anInfo.spine_wire = aSolid; // Solid, not Wire

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

TEST_F(PrimPipeShellTest, PipeShellLinearLawInfoInit_Defaults)
{
  occtl_prim_pipe_shell_linear_law_info_t anInfo;
  anInfo.struct_version = 0u;

  occtl_prim_pipe_shell_linear_law_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.spine_wire.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(anInfo.profile.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anInfo.scale_first, 1.0);
  EXPECT_DOUBLE_EQ(anInfo.scale_last, 1.0);
}

TEST_F(PrimPipeShellTest, MakePipeShellLinearLaw_TaperedProfile_CreatesShape)
{
  occtl_prim_pipe_shell_linear_law_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT;
  anInfo.spine_wire                              = mySpine;
  anInfo.profile                                 = myProfile;
  anInfo.scale_first                             = 1.0;
  anInfo.scale_last                              = 2.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_pipe_shell_linear_law(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimPipeShellTest, MakePipeShellLinearLaw_MakeSolid_ReturnsSolid)
{
  occtl_prim_pipe_shell_linear_law_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT;
  anInfo.spine_wire                              = mySpine;
  anInfo.profile                                 = myProfile;
  anInfo.scale_first                             = 1.0;
  anInfo.scale_last                              = 0.5;
  anInfo.make_solid                              = 1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_pipe_shell_linear_law(myGraph, &anInfo, &aShape), OCCTL_OK);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimPipeShellTest, MakePipeShellLinearLaw_InvalidScale_ReturnsInvalidArgument)
{
  occtl_prim_pipe_shell_linear_law_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT;
  anInfo.spine_wire                              = mySpine;
  anInfo.profile                                 = myProfile;
  anInfo.scale_first                             = 0.0;
  anInfo.scale_last                              = 2.0;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell_linear_law(myGraph, &anInfo, &aShape),
            OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimPipeShellTest, MakePipeShellLinearLaw_VersionMismatch_Rejected)
{
  occtl_prim_pipe_shell_linear_law_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT;
  anInfo.struct_version                          = 0u;
  anInfo.spine_wire                              = mySpine;
  anInfo.profile                                 = myProfile;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell_linear_law(myGraph, &anInfo, &aShape),
            OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimPipeShellTest, MakePipeShellLinearLaw_BadBoolean_ReturnsInvalidArgument)
{
  occtl_prim_pipe_shell_linear_law_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT;
  anInfo.spine_wire                              = mySpine;
  anInfo.profile                                 = myProfile;
  anInfo.make_solid                              = -1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell_linear_law(myGraph, &anInfo, &aShape),
            OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST_F(PrimPipeShellTest, MakePipeShellLinearLaw_ConstantBinormalMode_CreatesShape)
{
  occtl_prim_pipe_shell_linear_law_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT;
  anInfo.spine_wire                              = mySpine;
  anInfo.profile                                 = myProfile;
  anInfo.scale_first                             = 1.0;
  anInfo.scale_last                              = 1.25;
  anInfo.mode                                    = OCCTL_PIPE_MODE_CONSTANT_BINORMAL;
  anInfo.mode_binormal                           = {1.0, 0.0, 0.0};

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_pipe_shell_linear_law(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimPipeShellTest, PipeShellInterpolatedLawInfoInit_Defaults)
{
  occtl_prim_pipe_shell_interpolated_law_info_t anInfo;
  anInfo.struct_version = 0u;

  occtl_prim_pipe_shell_interpolated_law_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.spine_wire.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(anInfo.profile.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(anInfo.parameters, nullptr);
  EXPECT_EQ(anInfo.scales, nullptr);
  EXPECT_EQ(anInfo.sample_count, 0u);
}

TEST_F(PrimPipeShellTest, MakePipeShellInterpolatedLaw_BulgedProfile_CreatesShape)
{
  const double aParameters[] = {0.0, 0.35, 0.7, 1.0};
  const double aScales[]     = {0.75, 1.8, 0.9, 1.25};

  occtl_prim_pipe_shell_interpolated_law_info_t anInfo =
    OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_INIT;
  anInfo.spine_wire   = mySpine;
  anInfo.profile      = myProfile;
  anInfo.parameters   = aParameters;
  anInfo.scales       = aScales;
  anInfo.sample_count = 4;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_pipe_shell_interpolated_law(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimPipeShellTest, MakePipeShellInterpolatedLaw_MakeSolid_ReturnsSolid)
{
  const double aParameters[] = {0.0, 0.5, 1.0};
  const double aScales[]     = {1.0, 1.7, 1.0};

  occtl_prim_pipe_shell_interpolated_law_info_t anInfo =
    OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_INIT;
  anInfo.spine_wire   = mySpine;
  anInfo.profile      = myProfile;
  anInfo.parameters   = aParameters;
  anInfo.scales       = aScales;
  anInfo.sample_count = 3;
  anInfo.make_solid   = 1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_pipe_shell_interpolated_law(myGraph, &anInfo, &aShape), OCCTL_OK);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aShape, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_SOLID);
}

TEST_F(PrimPipeShellTest,
       MakePipeShellInterpolatedLaw_NonIncreasingParameters_ReturnsInvalidArgument)
{
  const double aParameters[] = {0.0, 0.5, 0.4, 1.0};
  const double aScales[]     = {1.0, 1.5, 0.8, 1.0};

  occtl_prim_pipe_shell_interpolated_law_info_t anInfo =
    OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_INIT;
  anInfo.spine_wire   = mySpine;
  anInfo.profile      = myProfile;
  anInfo.parameters   = aParameters;
  anInfo.scales       = aScales;
  anInfo.sample_count = 4;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell_interpolated_law(myGraph, &anInfo, &aShape),
            OCCTL_INVALID_ARGUMENT);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimPipeShellTest, MakePipeShellInterpolatedLaw_BadEndpoint_ReturnsInvalidArgument)
{
  const double aParameters[] = {0.0, 0.5, 0.95};
  const double aScales[]     = {1.0, 1.5, 1.0};

  occtl_prim_pipe_shell_interpolated_law_info_t anInfo =
    OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_INIT;
  anInfo.spine_wire   = mySpine;
  anInfo.profile      = myProfile;
  anInfo.parameters   = aParameters;
  anInfo.scales       = aScales;
  anInfo.sample_count = 3;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell_interpolated_law(myGraph, &anInfo, &aShape),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimPipeShellTest, MakePipeShellInterpolatedLaw_BadBoolean_ReturnsInvalidArgument)
{
  const double aParameters[] = {0.0, 0.5, 1.0};
  const double aScales[]     = {1.0, 1.5, 1.0};

  occtl_prim_pipe_shell_interpolated_law_info_t anInfo =
    OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_INIT;
  anInfo.spine_wire      = mySpine;
  anInfo.profile         = myProfile;
  anInfo.parameters      = aParameters;
  anInfo.scales          = aScales;
  anInfo.sample_count    = 3;
  anInfo.with_correction = 3;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_pipe_shell_interpolated_law(myGraph, &anInfo, &aShape),
            OCCTL_INVALID_ARGUMENT);
  expectLastErrorMessage();
}

TEST(PrimPipeShellVeneerTest, MakePipeShellLinearLaw_CircleAlongLine_ReturnsShape)
{
  occtl::Graph aGraph;

  occtl_geom_circle_t aCircGeom;
  aCircGeom.position        = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  aCircGeom.radius          = 0.5;
  occtl_rep_id_t aCircCurve = {};
  ASSERT_EQ(occtl_curve_create_circle(aGraph.get(), aCircGeom, &aCircCurve), OCCTL_OK);
  occtl_node_id_t aCircle = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_curves_to_wire(aGraph.get(), &aCircCurve, 1, &aCircle), OCCTL_OK);

  occtl_node_id_t               aV0         = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t               aV1         = OCCTL_NODE_ID_INVALID;
  occtl_topo_make_vertex_info_t aVertexInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertexInfo.tolerance                     = 1.0e-6;
  aVertexInfo.point                         = {0.0, 0.0, 0.0};
  ASSERT_EQ(occtl_topo_make_vertex(aGraph.get(), &aVertexInfo, &aV0), OCCTL_OK);
  aVertexInfo.point = {0.0, 0.0, 5.0};
  ASSERT_EQ(occtl_topo_make_vertex(aGraph.get(), &aVertexInfo, &aV1), OCCTL_OK);

  occtl_geom_line_t aLine  = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
  occtl_rep_id_t    aCurve = {};
  ASSERT_EQ(occtl_curve_create_line(aGraph.get(), aLine, &aCurve), OCCTL_OK);

  occtl_node_id_t             anEdge     = OCCTL_NODE_ID_INVALID;
  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.start_vertex                = aV0;
  anEdgeInfo.end_vertex                  = aV1;
  anEdgeInfo.curve                       = aCurve;
  anEdgeInfo.first                       = 0.0;
  anEdgeInfo.last                        = 5.0;
  anEdgeInfo.tolerance                   = 1.0e-6;
  ASSERT_EQ(occtl_topo_make_edge(aGraph.get(), &anEdgeInfo, &anEdge), OCCTL_OK);

  occtl_oriented_node_t       anOrientedEdge = {anEdge, OCCTL_ORIENTATION_FORWARD};
  occtl_topo_make_wire_info_t aWireInfo      = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                            = &anOrientedEdge;
  aWireInfo.edge_count                       = 1;
  occtl_node_id_t aSpine                     = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_wire(aGraph.get(), &aWireInfo, &aSpine), OCCTL_OK);

  occtl::prim::PipeShellLinearLawOptions anOptions;
  anOptions.scale_last = 1.75;

  const occtl::NodeId aShape = occtl::prim::make_pipe_shell_linear_law(aGraph,
                                                                       occtl::NodeId(aSpine),
                                                                       occtl::NodeId(aCircle),
                                                                       anOptions);
  EXPECT_NE(aShape.get().bits, OCCTL_NODE_ID_INVALID.bits);
}

TEST(PrimPipeShellVeneerTest, MakePipeShellInterpolatedLaw_CircleAlongLine_ReturnsShape)
{
  occtl::Graph aGraph;

  occtl_geom_circle_t aCircGeom;
  aCircGeom.position        = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  aCircGeom.radius          = 0.5;
  occtl_rep_id_t aCircCurve = {};
  ASSERT_EQ(occtl_curve_create_circle(aGraph.get(), aCircGeom, &aCircCurve), OCCTL_OK);
  occtl_node_id_t aCircle = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_curves_to_wire(aGraph.get(), &aCircCurve, 1, &aCircle), OCCTL_OK);

  occtl_node_id_t               aV0         = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t               aV1         = OCCTL_NODE_ID_INVALID;
  occtl_topo_make_vertex_info_t aVertexInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertexInfo.tolerance                     = 1.0e-6;
  aVertexInfo.point                         = {0.0, 0.0, 0.0};
  ASSERT_EQ(occtl_topo_make_vertex(aGraph.get(), &aVertexInfo, &aV0), OCCTL_OK);
  aVertexInfo.point = {0.0, 0.0, 5.0};
  ASSERT_EQ(occtl_topo_make_vertex(aGraph.get(), &aVertexInfo, &aV1), OCCTL_OK);

  occtl_geom_line_t aLine  = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
  occtl_rep_id_t    aCurve = {};
  ASSERT_EQ(occtl_curve_create_line(aGraph.get(), aLine, &aCurve), OCCTL_OK);

  occtl_node_id_t             anEdge     = OCCTL_NODE_ID_INVALID;
  occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.start_vertex                = aV0;
  anEdgeInfo.end_vertex                  = aV1;
  anEdgeInfo.curve                       = aCurve;
  anEdgeInfo.first                       = 0.0;
  anEdgeInfo.last                        = 5.0;
  anEdgeInfo.tolerance                   = 1.0e-6;
  ASSERT_EQ(occtl_topo_make_edge(aGraph.get(), &anEdgeInfo, &anEdge), OCCTL_OK);

  occtl_oriented_node_t       anOrientedEdge = {anEdge, OCCTL_ORIENTATION_FORWARD};
  occtl_topo_make_wire_info_t aWireInfo      = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                            = &anOrientedEdge;
  aWireInfo.edge_count                       = 1;
  occtl_node_id_t aSpine                     = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_make_wire(aGraph.get(), &aWireInfo, &aSpine), OCCTL_OK);

  occtl::prim::PipeShellInterpolatedLawOptions anOptions;
  anOptions.parameters = {0.0, 0.25, 0.75, 1.0};
  anOptions.scales     = {1.0, 1.5, 0.8, 1.25};

  const occtl::NodeId aShape = occtl::prim::make_pipe_shell_interpolated_law(aGraph,
                                                                             occtl::NodeId(aSpine),
                                                                             occtl::NodeId(aCircle),
                                                                             anOptions);
  EXPECT_NE(aShape.get().bits, OCCTL_NODE_ID_INVALID.bits);
}

} // namespace
