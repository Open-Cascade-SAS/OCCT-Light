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
#include <occtl/occtl_curves2d.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{

occtl_axis2_placement2d_t MakeStandardAxis2d()
{
  return {{0.0, 0.0}, {1.0, 0.0}};
}

occtl_rep_id_t MakeSimple2DBSpline(occtl_graph_t* aGraph)
{
  static const occtl_point2_t kPoles[3] = {{0.0, 0.0}, {1.0, 2.0}, {2.0, 0.0}};
  static const double         kKnots[2] = {0.0, 1.0};
  static const int32_t        kMults[2] = {3, 3};

  occtl_curve2d_bspline_create_info_t aInfo = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                               = kPoles;
  aInfo.pole_count                          = 3;
  aInfo.knots                               = kKnots;
  aInfo.multiplicities                      = kMults;
  aInfo.knot_count                          = 2;
  aInfo.degree                              = 2;

  occtl_rep_id_t aId = {};
  if (occtl_curve2d_create_bspline(aGraph, &aInfo, &aId) != OCCTL_OK)
  {
    return occtl_rep_id_t{0};
  }
  return aId;
}

occtl_rep_id_t MakeRational2DBSpline(occtl_graph_t* aGraph)
{
  static const occtl_point2_t kPoles[4]   = {{0.0, 0.0}, {1.0, 2.0}, {2.0, 2.0}, {3.0, 0.0}};
  static const double         kWeights[4] = {1.0, 2.0, 0.5, 1.5};
  static const double         kKnots[2]   = {0.0, 1.0};
  static const int32_t        kMults[2]   = {4, 4};

  occtl_curve2d_bspline_create_info_t aInfo = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                               = kPoles;
  aInfo.pole_count                          = 4;
  aInfo.weights                             = kWeights;
  aInfo.knots                               = kKnots;
  aInfo.multiplicities                      = kMults;
  aInfo.knot_count                          = 2;
  aInfo.degree                              = 3;

  occtl_rep_id_t aId = {};
  if (occtl_curve2d_create_bspline(aGraph, &aInfo, &aId) != OCCTL_OK)
  {
    return occtl_rep_id_t{0};
  }
  return aId;
}

TEST(Curves2DLineTest, Construct_KindIsLine)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {MakeStandardAxis2d()};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_LINE);
  occtl_graph_free(aGraph);
}

TEST(Curves2DLineTest, AsLine_RoundTripsPosition)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_axis2_placement2d_t aAx   = {{1.0, 2.0}, {1.0, 0.0}};
  const occtl_geom2d_line_t       aLine = {aAx};
  occtl_rep_id_t                  aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);

  occtl_geom2d_line_t aOut{};
  ASSERT_EQ(occtl_curve2d_as_line(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.position.location.x, 1.0, 1e-14);
  EXPECT_NEAR(aOut.position.location.y, 2.0, 1e-14);
  EXPECT_NEAR(aOut.position.x_dir.x, 1.0, 1e-14);
  EXPECT_NEAR(aOut.position.x_dir.y, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curves2DCircleTest, Construct_KindIsCircle)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 5.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_CIRCLE);
  occtl_graph_free(aGraph);
}

TEST(Curves2DCircleTest, Construct_IsPeriodic)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 2.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);
  int32_t aPeriodic = 0;
  ASSERT_EQ(occtl_curve2d_is_periodic(aGraph, aId, &aPeriodic), OCCTL_OK);
  EXPECT_EQ(aPeriodic, 1);
  occtl_graph_free(aGraph);
}

TEST(Curves2DCircleTest, AsCircle_RoundTripsRadius)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 4.5};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);

  occtl_geom2d_circle_t aOut{};
  ASSERT_EQ(occtl_curve2d_as_circle(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.radius, 4.5, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curves2DCircleTest, InvalidRadius_ReturnsGeometryInvalid)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 0.0};
  occtl_rep_id_t              aId     = {};
  EXPECT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_GEOMETRY_INVALID);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, InfoInit_HasDefaults)
{
  occtl_curve2d_circle_tangent_to_two_radius_info_t aInfo{};
  occtl_curve2d_circle_tangent_to_two_radius_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_EQ(aInfo.curve_a.bits, 0u);
  EXPECT_EQ(aInfo.curve_b.bits, 0u);
  EXPECT_EQ(aInfo.qualifier_a, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_EQ(aInfo.qualifier_b, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_NEAR(aInfo.tolerance, 1.0e-9, 1.0e-16);
}

TEST(Curves2DTangentCircleTest, BlendArcInfoInit_HasDefaults)
{
  occtl_curve2d_blend_arc_info_t aInfo{};
  occtl_curve2d_blend_arc_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE2D_BLEND_ARC_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_EQ(aInfo.curve_a.bits, 0u);
  EXPECT_EQ(aInfo.curve_b.bits, 0u);
  EXPECT_EQ(aInfo.qualifier_a, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_EQ(aInfo.qualifier_b, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_EQ(aInfo.long_arc, 0);
  EXPECT_NEAR(aInfo.tolerance, 1.0e-9, 1.0e-16);
}

TEST(Curves2DTangentCircleTest, TangentToThreeInfoInit_HasDefaults)
{
  occtl_curve2d_circle_tangent_to_three_info_t aInfo{};
  occtl_curve2d_circle_tangent_to_three_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_EQ(aInfo.curve_a.bits, 0u);
  EXPECT_EQ(aInfo.curve_b.bits, 0u);
  EXPECT_EQ(aInfo.curve_c.bits, 0u);
  EXPECT_EQ(aInfo.qualifier_a, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_EQ(aInfo.qualifier_b, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_EQ(aInfo.qualifier_c, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_NEAR(aInfo.initial_parameter_a, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.initial_parameter_b, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.initial_parameter_c, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.tolerance, 1.0e-9, 1.0e-16);
}

TEST(Curves2DTangentCircleTest, TangentFixedCenterInfoInit_HasDefaults)
{
  occtl_curve2d_circle_tangent_fixed_center_info_t aInfo{};
  occtl_curve2d_circle_tangent_fixed_center_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_EQ(aInfo.curve.bits, 0u);
  EXPECT_EQ(aInfo.qualifier, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_NEAR(aInfo.center.x, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.center.y, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.tolerance, 1.0e-9, 1.0e-16);
}

TEST(Curves2DTangentCircleTest, TangentCenterOnCurveInfoInit_HasDefaults)
{
  occtl_curve2d_circle_tangent_center_on_curve_info_t aInfo{};
  occtl_curve2d_circle_tangent_center_on_curve_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_EQ(aInfo.curve_a.bits, 0u);
  EXPECT_EQ(aInfo.curve_b.bits, 0u);
  EXPECT_EQ(aInfo.center_curve.bits, 0u);
  EXPECT_EQ(aInfo.qualifier_a, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_EQ(aInfo.qualifier_b, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_NEAR(aInfo.initial_parameter_a, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.initial_parameter_b, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.initial_parameter_center, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.tolerance, 1.0e-9, 1.0e-16);
}

TEST(Curves2DTangentCircleTest, TangentOnCurveRadiusInfoInit_HasDefaults)
{
  occtl_curve2d_circle_tangent_on_curve_radius_info_t aInfo{};
  occtl_curve2d_circle_tangent_on_curve_radius_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_EQ(aInfo.curve.bits, 0u);
  EXPECT_EQ(aInfo.center_curve.bits, 0u);
  EXPECT_EQ(aInfo.qualifier, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_NEAR(aInfo.radius, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.tolerance, 1.0e-9, 1.0e-16);
}

TEST(Curves2DTangentCircleTest, TwoPerpendicularLines_ReturnsCandidateCircles)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t       aLineX = {MakeStandardAxis2d()};
  const occtl_axis2_placement2d_t aYAxis = {{0.0, 0.0}, {0.0, 1.0}};
  const occtl_geom2d_line_t       aLineY = {aYAxis};

  occtl_rep_id_t aIdX = {};
  occtl_rep_id_t aIdY = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineX, &aIdX), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineY, &aIdY), OCCTL_OK);

  occtl_curve2d_circle_tangent_to_two_radius_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_INIT;
  aInfo.curve_a = aIdX;
  aInfo.curve_b = aIdY;
  aInfo.radius  = 2.0;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_tangent_circle_to_two_radius(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  std::vector<occtl_geom2d_circle_t> aCircles(aCount);
  for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
  {
    ASSERT_EQ(occtl_curve2d_create_tangent_circle_to_two_radius(aGraph,
                                                                &aInfo,
                                                                anIndex,
                                                                &aCircles[anIndex],
                                                                &aCount),
              OCCTL_OK);
  }
  for (const occtl_geom2d_circle_t& aCircle : aCircles)
  {
    EXPECT_NEAR(aCircle.radius, 2.0, 1.0e-9);
  }

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, BlendArcTwoPerpendicularLines_ReturnsTrimmedArc)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLineX = {MakeStandardAxis2d()};
  const occtl_geom2d_line_t aLineY = {{{0.0, 0.0}, {0.0, 1.0}}};

  occtl_rep_id_t aIdX = {};
  occtl_rep_id_t aIdY = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineX, &aIdX), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineY, &aIdY), OCCTL_OK);

  occtl_curve2d_blend_arc_info_t aInfo = OCCTL_CURVE2D_BLEND_ARC_INFO_INIT;
  aInfo.curve_a                        = aIdX;
  aInfo.curve_b                        = aIdY;
  aInfo.radius                         = 2.0;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_blend_arc(aGraph, &aInfo, 0, nullptr, &aCount), OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_rep_id_t anId = {};
  ASSERT_EQ(occtl_curve2d_create_blend_arc(aGraph, &aInfo, 0, &anId, &aCount), OCCTL_OK);
  ASSERT_NE(anId.bits, 0u);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, anId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_TRIMMED);

  double aFirst = 0.0;
  double aLast  = 0.0;
  ASSERT_EQ(occtl_curve2d_as_trimmed(aGraph, anId, &aFirst, &aLast), OCCTL_OK);
  EXPECT_GT(aLast, aFirst);
  EXPECT_LT(aLast - aFirst, 3.1415926535897932384626433832795);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, BlendArcLongArc_ReturnsComplementaryTrim)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLineX = {MakeStandardAxis2d()};
  const occtl_geom2d_line_t aLineY = {{{0.0, 0.0}, {0.0, 1.0}}};

  occtl_rep_id_t aIdX = {};
  occtl_rep_id_t aIdY = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineX, &aIdX), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineY, &aIdY), OCCTL_OK);

  occtl_curve2d_blend_arc_info_t aInfo = OCCTL_CURVE2D_BLEND_ARC_INFO_INIT;
  aInfo.curve_a                        = aIdX;
  aInfo.curve_b                        = aIdY;
  aInfo.radius                         = 2.0;
  aInfo.long_arc                       = 1;

  size_t         aCount = 0;
  occtl_rep_id_t anId   = {};
  ASSERT_EQ(occtl_curve2d_create_blend_arc(aGraph, &aInfo, 0, &anId, &aCount), OCCTL_OK);

  double aFirst = 0.0;
  double aLast  = 0.0;
  ASSERT_EQ(occtl_curve2d_as_trimmed(aGraph, anId, &aFirst, &aLast), OCCTL_OK);
  EXPECT_GT(aLast - aFirst, 3.1415926535897932384626433832795);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, ThreeCircles_ReturnsCandidateCircles)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t     aCircleA = {MakeStandardAxis2d(), 2.0};
  const occtl_axis2_placement2d_t aAxisB   = {{10.0, 0.0}, {1.0, 0.0}};
  const occtl_axis2_placement2d_t aAxisC   = {{5.0, 8.0}, {1.0, 0.0}};
  const occtl_geom2d_circle_t     aCircleB = {aAxisB, 2.0};
  const occtl_geom2d_circle_t     aCircleC = {aAxisC, 2.0};

  occtl_rep_id_t aIdA = {};
  occtl_rep_id_t aIdB = {};
  occtl_rep_id_t aIdC = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleA, &aIdA), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleB, &aIdB), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleC, &aIdC), OCCTL_OK);

  occtl_curve2d_circle_tangent_to_three_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_INIT;
  aInfo.curve_a = aIdA;
  aInfo.curve_b = aIdB;
  aInfo.curve_c = aIdC;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_tangent_circle_to_three(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_circle_t aCircle{};
  ASSERT_EQ(occtl_curve2d_create_tangent_circle_to_three(aGraph, &aInfo, 0, &aCircle, &aCount),
            OCCTL_OK);
  EXPECT_GT(aCircle.radius, 0.0);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, CircleWithFixedCenter_ReturnsCandidateCircles)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aSource = {MakeStandardAxis2d(), 1.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aSource, &aId), OCCTL_OK);

  occtl_curve2d_circle_tangent_fixed_center_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_INIT;
  aInfo.curve  = aId;
  aInfo.center = {3.0, 0.0};

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_tangent_circle_fixed_center(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_circle_t aCircle{};
  ASSERT_EQ(occtl_curve2d_create_tangent_circle_fixed_center(aGraph, &aInfo, 0, &aCircle, &aCount),
            OCCTL_OK);
  EXPECT_NEAR(aCircle.position.location.x, 3.0, 1.0e-12);
  EXPECT_NEAR(aCircle.position.location.y, 0.0, 1.0e-12);
  EXPECT_GT(aCircle.radius, 0.0);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, TwoLinesCenterOnCurve_ReturnsCandidateCircles)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLineA      = {MakeStandardAxis2d()};
  const occtl_geom2d_line_t aLineB      = {{{0.0, 4.0}, {1.0, 0.0}}};
  const occtl_geom2d_line_t aCenterLine = {{{0.0, 0.0}, {0.0, 1.0}}};

  occtl_rep_id_t aIdA      = {};
  occtl_rep_id_t aIdB      = {};
  occtl_rep_id_t aCenterId = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineA, &aIdA), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineB, &aIdB), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aCenterLine, &aCenterId), OCCTL_OK);

  occtl_curve2d_circle_tangent_center_on_curve_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_INIT;
  aInfo.curve_a      = aIdA;
  aInfo.curve_b      = aIdB;
  aInfo.center_curve = aCenterId;

  size_t aCount = 0;
  ASSERT_EQ(
    occtl_curve2d_create_tangent_circle_center_on_curve(aGraph, &aInfo, 0, nullptr, &aCount),
    OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_circle_t aCircle{};
  ASSERT_EQ(
    occtl_curve2d_create_tangent_circle_center_on_curve(aGraph, &aInfo, 0, &aCircle, &aCount),
    OCCTL_OK);
  EXPECT_NEAR(aCircle.position.location.x, 0.0, 1.0e-9);
  EXPECT_GT(aCircle.radius, 0.0);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, LineCenterOnCurveRadius_ReturnsCandidateCircles)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aTangentLine = {MakeStandardAxis2d()};
  const occtl_geom2d_line_t aCenterLine  = {{{0.0, 0.0}, {0.0, 1.0}}};

  occtl_rep_id_t aTangentId = {};
  occtl_rep_id_t aCenterId  = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aTangentLine, &aTangentId), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aCenterLine, &aCenterId), OCCTL_OK);

  occtl_curve2d_circle_tangent_on_curve_radius_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_INIT;
  aInfo.curve        = aTangentId;
  aInfo.center_curve = aCenterId;
  aInfo.radius       = 2.0;

  size_t aCount = 0;
  ASSERT_EQ(
    occtl_curve2d_create_tangent_circle_on_curve_radius(aGraph, &aInfo, 0, nullptr, &aCount),
    OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_circle_t aCircle{};
  ASSERT_EQ(
    occtl_curve2d_create_tangent_circle_on_curve_radius(aGraph, &aInfo, 0, &aCircle, &aCount),
    OCCTL_OK);
  EXPECT_NEAR(aCircle.position.location.x, 0.0, 1.0e-9);
  EXPECT_NEAR(aCircle.radius, 2.0, 1.0e-12);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, LineCenterOnCurveRadius_InvalidRadius)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aTangentLine = {MakeStandardAxis2d()};
  const occtl_geom2d_line_t aCenterLine  = {{{0.0, 0.0}, {0.0, 1.0}}};

  occtl_rep_id_t aTangentId = {};
  occtl_rep_id_t aCenterId  = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aTangentLine, &aTangentId), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aCenterLine, &aCenterId), OCCTL_OK);

  occtl_curve2d_circle_tangent_on_curve_radius_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_INIT;
  aInfo.curve        = aTangentId;
  aInfo.center_curve = aCenterId;

  size_t aCount = 0;
  EXPECT_EQ(
    occtl_curve2d_create_tangent_circle_on_curve_radius(aGraph, &aInfo, 0, nullptr, &aCount),
    OCCTL_INVALID_ARGUMENT);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, CenterOnCurveNullCenter_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLineA = {MakeStandardAxis2d()};
  const occtl_geom2d_line_t aLineB = {{{0.0, 4.0}, {1.0, 0.0}}};

  occtl_rep_id_t aIdA = {};
  occtl_rep_id_t aIdB = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineA, &aIdA), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineB, &aIdB), OCCTL_OK);

  occtl_curve2d_circle_tangent_center_on_curve_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_INIT;
  aInfo.curve_a = aIdA;
  aInfo.curve_b = aIdB;

  size_t aCount = 0;
  EXPECT_EQ(
    occtl_curve2d_create_tangent_circle_center_on_curve(aGraph, &aInfo, 0, nullptr, &aCount),
    OCCTL_INVALID_ARGUMENT);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, FixedCenterBadVersion_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aSource = {MakeStandardAxis2d(), 1.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aSource, &aId), OCCTL_OK);

  occtl_curve2d_circle_tangent_fixed_center_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_INIT;
  aInfo.struct_version = 99;
  aInfo.curve          = aId;

  size_t aCount = 0;
  EXPECT_EQ(occtl_curve2d_create_tangent_circle_fixed_center(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_VERSION_MISMATCH);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, ThreeCircleOutOfRange_ReturnsNotFoundWithCount)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t     aCircleA = {MakeStandardAxis2d(), 2.0};
  const occtl_axis2_placement2d_t aAxisB   = {{10.0, 0.0}, {1.0, 0.0}};
  const occtl_axis2_placement2d_t aAxisC   = {{5.0, 8.0}, {1.0, 0.0}};
  const occtl_geom2d_circle_t     aCircleB = {aAxisB, 2.0};
  const occtl_geom2d_circle_t     aCircleC = {aAxisC, 2.0};

  occtl_rep_id_t aIdA = {};
  occtl_rep_id_t aIdB = {};
  occtl_rep_id_t aIdC = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleA, &aIdA), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleB, &aIdB), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleC, &aIdC), OCCTL_OK);

  occtl_curve2d_circle_tangent_to_three_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_INIT;
  aInfo.curve_a = aIdA;
  aInfo.curve_b = aIdB;
  aInfo.curve_c = aIdC;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_tangent_circle_to_three(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_circle_t aCircle{};
  EXPECT_EQ(occtl_curve2d_create_tangent_circle_to_three(aGraph, &aInfo, aCount, &aCircle, &aCount),
            OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, OutOfRangeIndex_ReturnsNotFoundWithCount)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t       aLineX = {MakeStandardAxis2d()};
  const occtl_axis2_placement2d_t aYAxis = {{0.0, 0.0}, {0.0, 1.0}};
  const occtl_geom2d_line_t       aLineY = {aYAxis};

  occtl_rep_id_t aIdX = {};
  occtl_rep_id_t aIdY = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineX, &aIdX), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineY, &aIdY), OCCTL_OK);

  occtl_curve2d_circle_tangent_to_two_radius_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_INIT;
  aInfo.curve_a = aIdX;
  aInfo.curve_b = aIdY;
  aInfo.radius  = 2.0;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_tangent_circle_to_two_radius(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_circle_t aCircle{};
  EXPECT_EQ(
    occtl_curve2d_create_tangent_circle_to_two_radius(aGraph, &aInfo, aCount, &aCircle, &aCount),
    OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, BlendArcOutOfRange_ReturnsNotFoundWithCount)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLineX = {MakeStandardAxis2d()};
  const occtl_geom2d_line_t aLineY = {{{0.0, 0.0}, {0.0, 1.0}}};

  occtl_rep_id_t aIdX = {};
  occtl_rep_id_t aIdY = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineX, &aIdX), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLineY, &aIdY), OCCTL_OK);

  occtl_curve2d_blend_arc_info_t aInfo = OCCTL_CURVE2D_BLEND_ARC_INFO_INIT;
  aInfo.curve_a                        = aIdX;
  aInfo.curve_b                        = aIdY;
  aInfo.radius                         = 2.0;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_blend_arc(aGraph, &aInfo, 0, nullptr, &aCount), OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_rep_id_t anId = {};
  EXPECT_EQ(occtl_curve2d_create_blend_arc(aGraph, &aInfo, aCount, &anId, &aCount),
            OCCTL_NOT_FOUND);
  EXPECT_EQ(anId.bits, 0u);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, BlendArcBadLongArcFlag_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {MakeStandardAxis2d()};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);

  occtl_curve2d_blend_arc_info_t aInfo = OCCTL_CURVE2D_BLEND_ARC_INFO_INIT;
  aInfo.curve_a                        = aId;
  aInfo.curve_b                        = aId;
  aInfo.radius                         = 2.0;
  aInfo.long_arc                       = 2;
  size_t aCount                        = 0;
  EXPECT_EQ(occtl_curve2d_create_blend_arc(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_INVALID_ARGUMENT);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, BadVersion_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_curve2d_circle_tangent_to_two_radius_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_INIT;
  aInfo.struct_version = 99u;
  size_t aCount        = 0;
  EXPECT_EQ(occtl_curve2d_create_tangent_circle_to_two_radius(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentCircleTest, NoQualifierInput_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {MakeStandardAxis2d()};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);

  occtl_curve2d_circle_tangent_to_two_radius_info_t aInfo =
    OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_INIT;
  aInfo.curve_a     = aId;
  aInfo.curve_b     = aId;
  aInfo.qualifier_a = OCCTL_GEOM_TANGENCY_NO_QUALIFIER;
  aInfo.radius      = 1.0;

  size_t aCount = 0;
  EXPECT_EQ(occtl_curve2d_create_tangent_circle_to_two_radius(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_INVALID_ARGUMENT);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentLineTest, LineTangentToTwoInfoInit_HasDefaults)
{
  occtl_curve2d_line_tangent_to_two_info_t aInfo{};
  occtl_curve2d_line_tangent_to_two_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_EQ(aInfo.curve_a.bits, 0u);
  EXPECT_EQ(aInfo.curve_b.bits, 0u);
  EXPECT_EQ(aInfo.qualifier_a, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_EQ(aInfo.qualifier_b, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_NEAR(aInfo.tolerance, 1.0e-9, 1.0e-16);
}

TEST(Curves2DTangentLineTest, LineTangentThroughPointInfoInit_HasDefaults)
{
  occtl_curve2d_line_tangent_through_point_info_t aInfo{};
  occtl_curve2d_line_tangent_through_point_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE2D_LINE_TANGENT_THROUGH_POINT_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_EQ(aInfo.curve.bits, 0u);
  EXPECT_EQ(aInfo.qualifier, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_NEAR(aInfo.point.x, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.point.y, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.tolerance, 1.0e-9, 1.0e-16);
}

TEST(Curves2DTangentLineTest, LineTangentWithAngleInfoInit_HasDefaults)
{
  occtl_curve2d_line_tangent_with_angle_info_t aInfo{};
  occtl_curve2d_line_tangent_with_angle_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_EQ(aInfo.curve.bits, 0u);
  EXPECT_EQ(aInfo.qualifier, OCCTL_GEOM_TANGENCY_UNQUALIFIED);
  EXPECT_NEAR(aInfo.reference_line.position.location.x, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.reference_line.position.location.y, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.reference_line.position.x_dir.x, 1.0, 1.0e-16);
  EXPECT_NEAR(aInfo.reference_line.position.x_dir.y, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.angle_radians, 0.0, 1.0e-16);
  EXPECT_EQ(aInfo.use_initial_parameter, 0);
  EXPECT_NEAR(aInfo.initial_parameter, 0.0, 1.0e-16);
  EXPECT_NEAR(aInfo.tolerance, 1.0e-9, 1.0e-16);
}

TEST(Curves2DTangentLineTest, TwoCircles_ReturnsCandidateLines)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t     aCircleA = {MakeStandardAxis2d(), 1.0};
  const occtl_axis2_placement2d_t aAxisB   = {{4.0, 0.0}, {1.0, 0.0}};
  const occtl_geom2d_circle_t     aCircleB = {aAxisB, 1.0};

  occtl_rep_id_t aIdA = {};
  occtl_rep_id_t aIdB = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleA, &aIdA), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleB, &aIdB), OCCTL_OK);

  occtl_curve2d_line_tangent_to_two_info_t aInfo = OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_INIT;
  aInfo.curve_a                                  = aIdA;
  aInfo.curve_b                                  = aIdB;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_tangent_line_to_two(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_line_t aLine{};
  ASSERT_EQ(occtl_curve2d_create_tangent_line_to_two(aGraph, &aInfo, 0, &aLine, &aCount), OCCTL_OK);
  const double aDirLen = std::hypot(aLine.position.x_dir.x, aLine.position.x_dir.y);
  EXPECT_NEAR(aDirLen, 1.0, 1.0e-12);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentLineTest, CircleThroughPoint_ReturnsCandidateLines)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 1.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);

  occtl_curve2d_line_tangent_through_point_info_t aInfo =
    OCCTL_CURVE2D_LINE_TANGENT_THROUGH_POINT_INFO_INIT;
  aInfo.curve = aId;
  aInfo.point = {3.0, 0.0};

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_tangent_line_through_point(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_line_t aLine{};
  ASSERT_EQ(occtl_curve2d_create_tangent_line_through_point(aGraph, &aInfo, 0, &aLine, &aCount),
            OCCTL_OK);
  const double aDirLen = std::hypot(aLine.position.x_dir.x, aLine.position.x_dir.y);
  EXPECT_NEAR(aDirLen, 1.0, 1.0e-12);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentLineTest, CircleWithZeroAngleToLine_ReturnsCandidateLines)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 1.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);

  occtl_curve2d_line_tangent_with_angle_info_t aInfo =
    OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_INIT;
  aInfo.curve = aId;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_tangent_line_with_angle(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_line_t aLine{};
  ASSERT_EQ(occtl_curve2d_create_tangent_line_with_angle(aGraph, &aInfo, 0, &aLine, &aCount),
            OCCTL_OK);
  const double aDirLen = std::hypot(aLine.position.x_dir.x, aLine.position.x_dir.y);
  EXPECT_NEAR(aDirLen, 1.0, 1.0e-12);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentLineTest, TangentWithAngleInvalidFlag_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 1.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);

  occtl_curve2d_line_tangent_with_angle_info_t aInfo =
    OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_INIT;
  aInfo.curve                 = aId;
  aInfo.use_initial_parameter = 7;

  size_t aCount = 0;
  EXPECT_EQ(occtl_curve2d_create_tangent_line_with_angle(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_INVALID_ARGUMENT);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTangentLineTest, OutOfRangeLineIndex_ReturnsNotFound)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t     aCircleA = {MakeStandardAxis2d(), 1.0};
  const occtl_axis2_placement2d_t aAxisB   = {{4.0, 0.0}, {1.0, 0.0}};
  const occtl_geom2d_circle_t     aCircleB = {aAxisB, 1.0};

  occtl_rep_id_t aIdA = {};
  occtl_rep_id_t aIdB = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleA, &aIdA), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircleB, &aIdB), OCCTL_OK);

  occtl_curve2d_line_tangent_to_two_info_t aInfo = OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_INIT;
  aInfo.curve_a                                  = aIdA;
  aInfo.curve_b                                  = aIdB;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_tangent_line_to_two(aGraph, &aInfo, 0, nullptr, &aCount),
            OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_geom2d_line_t aLine{};
  EXPECT_EQ(occtl_curve2d_create_tangent_line_to_two(aGraph, &aInfo, aCount, &aLine, &aCount),
            OCCTL_NOT_FOUND);

  occtl_graph_free(aGraph);
}

TEST(Curves2DEllipseTest, Construct_KindIsEllipse)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_ellipse_t aEllipse = {MakeStandardAxis2d(), 5.0, 2.0};
  occtl_rep_id_t               aId      = {};
  ASSERT_EQ(occtl_curve2d_create_ellipse(aGraph, aEllipse, &aId), OCCTL_OK);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_ELLIPSE);
  occtl_graph_free(aGraph);
}

TEST(Curves2DEllipseTest, AsEllipse_RoundTripsRadii)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_ellipse_t aEllipse = {MakeStandardAxis2d(), 6.0, 3.0};
  occtl_rep_id_t               aId      = {};
  ASSERT_EQ(occtl_curve2d_create_ellipse(aGraph, aEllipse, &aId), OCCTL_OK);

  occtl_geom2d_ellipse_t aOut{};
  ASSERT_EQ(occtl_curve2d_as_ellipse(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.major_radius, 6.0, 1e-14);
  EXPECT_NEAR(aOut.minor_radius, 3.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curves2DWrongKindTest, AsLine_OnCircle_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 3.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);

  occtl_geom2d_line_t aOut{};
  EXPECT_EQ(occtl_curve2d_as_line(aGraph, aId, &aOut), OCCTL_WRONG_KIND);
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_GT(strlen(aErr->message), 0u);
  occtl_graph_free(aGraph);
}

TEST(Curves2DWrongKindTest, AsCircle_OnEllipse_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_ellipse_t aEllipse = {MakeStandardAxis2d(), 5.0, 2.0};
  occtl_rep_id_t               aId      = {};
  ASSERT_EQ(occtl_curve2d_create_ellipse(aGraph, aEllipse, &aId), OCCTL_OK);

  occtl_geom2d_circle_t aOut{};
  EXPECT_EQ(occtl_curve2d_as_circle(aGraph, aId, &aOut), OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBSplineTest, Construct_KindIsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeSimple2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_BSPLINE);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBSplineTest, ScalarQueries_MatchInput)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeSimple2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);
  int32_t aDegree = 0;
  ASSERT_EQ(occtl_curve2d_bspline_degree(aGraph, aId, &aDegree), OCCTL_OK);
  EXPECT_EQ(aDegree, 2);
  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_bspline_pole_count(aGraph, aId, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 3u);
  ASSERT_EQ(occtl_curve2d_bspline_knot_count(aGraph, aId, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 2u);
  int32_t aRational = 0;
  ASSERT_EQ(occtl_curve2d_bspline_is_rational(aGraph, aId, &aRational), OCCTL_OK);
  EXPECT_EQ(aRational, 0);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBezierSegmentsTest, OptionsInit_HasDefaults)
{
  occtl_curve_bezier_segments_options_t aOptions{};
  occtl_curve_bezier_segments_options_init(&aOptions);
  EXPECT_EQ(aOptions.struct_version, OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_VERSION_1);
  EXPECT_EQ(aOptions.p_next, nullptr);
  EXPECT_EQ(aOptions.use_range, 0);
  EXPECT_NEAR(aOptions.parametric_tolerance, 1.0e-9, 1.0e-14);
}

TEST(Curves2DBezierSegmentsTest, BSplineWithInteriorKnot_ReturnsTwoBezierCurves)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_point2_t aPoles[5] = {{0.0, 0.0}, {1.0, 2.0}, {2.0, 0.0}, {3.0, 2.0}, {4.0, 0.0}};
  const double         aKnots[3] = {0.0, 0.5, 1.0};
  const int32_t        aMults[3] = {3, 2, 3};

  occtl_curve2d_bspline_create_info_t aInfo = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                               = aPoles;
  aInfo.pole_count                          = 5;
  aInfo.knots                               = aKnots;
  aInfo.multiplicities                      = aMults;
  aInfo.knot_count                          = 3;
  aInfo.degree                              = 2;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_curve2d_create_bspline(aGraph, &aInfo, &aId), OCCTL_OK);

  occtl_rep_id_t* aSegments = nullptr;
  size_t          aCount    = 0;
  ASSERT_EQ(occtl_curve2d_to_bezier_segments(aGraph, aId, nullptr, &aSegments, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, 2u);
  ASSERT_NE(aSegments, nullptr);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aSegments[0], &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_BEZIER);
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aSegments[1], &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_BEZIER);

  occtl_curve2d_free_bezier_segments(aSegments);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBezierSegmentsTest, LineWithoutRange_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {MakeStandardAxis2d()};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);

  occtl_rep_id_t* aSegments = nullptr;
  size_t          aCount    = 0;
  EXPECT_EQ(occtl_curve2d_to_bezier_segments(aGraph, aId, nullptr, &aSegments, &aCount),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aSegments, nullptr);
  EXPECT_EQ(aCount, 0u);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBezierSegmentsTest, LineWithRange_ReturnsOneBezierCurve)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {MakeStandardAxis2d()};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);

  occtl_curve_bezier_segments_options_t aOptions = OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_INIT;
  aOptions.use_range                             = 1;
  aOptions.u_first                               = 0.0;
  aOptions.u_last                                = 5.0;

  occtl_rep_id_t* aSegments = nullptr;
  size_t          aCount    = 0;
  ASSERT_EQ(occtl_curve2d_to_bezier_segments(aGraph, aId, &aOptions, &aSegments, &aCount),
            OCCTL_OK);
  ASSERT_EQ(aCount, 1u);
  ASSERT_NE(aSegments, nullptr);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aSegments[0], &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_BEZIER);

  occtl_point2_t anEnd{};
  ASSERT_EQ(occtl_curve2d_eval_d0(aGraph, aSegments[0], 1.0, &anEnd), OCCTL_OK);
  EXPECT_NEAR(anEnd.x, 5.0, 1.0e-12);

  occtl_curve2d_free_bezier_segments(aSegments);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBezierSegmentsTest, BadVersion_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeSimple2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_curve_bezier_segments_options_t aOptions = OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_INIT;
  aOptions.struct_version                        = 99u;
  occtl_rep_id_t* aSegments                      = nullptr;
  size_t          aCount                         = 0;
  EXPECT_EQ(occtl_curve2d_to_bezier_segments(aGraph, aId, &aOptions, &aSegments, &aCount),
            OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBezierSegmentsTest, Free_Nullptr_IsNoop)
{
  occtl_curve2d_free_bezier_segments(nullptr);
}

TEST(Curves2DBSplineTest, TwoCallPoles_RoundTrip)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeSimple2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);

  size_t aNb = 0;
  ASSERT_EQ(occtl_curve2d_bspline_poles(aGraph, aId, nullptr, 0, &aNb), OCCTL_OK);
  ASSERT_EQ(aNb, 3u);

  occtl_point2_t aPoles[3]{};
  ASSERT_EQ(occtl_curve2d_bspline_poles(aGraph, aId, aPoles, 3, &aNb), OCCTL_OK);
  EXPECT_NEAR(aPoles[0].x, 0.0, 1e-14);
  EXPECT_NEAR(aPoles[0].y, 0.0, 1e-14);
  EXPECT_NEAR(aPoles[1].x, 1.0, 1e-14);
  EXPECT_NEAR(aPoles[1].y, 2.0, 1e-14);
  EXPECT_NEAR(aPoles[2].x, 2.0, 1e-14);
  EXPECT_NEAR(aPoles[2].y, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBSplineTest, WeightsOnNonRational_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeSimple2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);

  size_t aNb = 0;
  EXPECT_EQ(occtl_curve2d_bspline_weights(aGraph, aId, nullptr, 0, &aNb), OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBSplineTest, VersionMismatch_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_curve2d_bspline_create_info_t aInfo = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
  aInfo.struct_version                      = 99u;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_curve2d_create_bspline(aGraph, &aInfo, &aId), OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBSplineTest, PNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_curve2d_bspline_create_info_t aInfo = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
  aInfo.p_next                              = reinterpret_cast<const void*>(0x1);

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_curve2d_create_bspline(aGraph, &aInfo, &aId), OCCTL_INVALID_ARGUMENT);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBSplineTest, InvalidPeriodicFlag_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));

  const occtl_point2_t kPoles[3] = {{0.0, 0.0}, {1.0, 2.0}, {2.0, 0.0}};
  const double         kKnots[2] = {0.0, 1.0};
  const int32_t        kMults[2] = {3, 3};

  occtl_curve2d_bspline_create_info_t aInfo = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                               = kPoles;
  aInfo.pole_count                          = 3;
  aInfo.knots                               = kKnots;
  aInfo.multiplicities                      = kMults;
  aInfo.knot_count                          = 2;
  aInfo.degree                              = 2;
  aInfo.is_periodic                         = 2;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_curve2d_create_bspline(aGraph, &aInfo, &aId), OCCTL_INVALID_ARGUMENT);
  occtl_graph_free(aGraph);
}

TEST(Curves2DHyperbolaTest, Construct_KindIsHyperbola)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_hyperbola_t aHyperbola = {MakeStandardAxis2d(), 3.0, 2.0};
  occtl_rep_id_t                 aId        = {};
  ASSERT_EQ(occtl_curve2d_create_hyperbola(aGraph, aHyperbola, &aId), OCCTL_OK);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_HYPERBOLA);
  occtl_graph_free(aGraph);
}

TEST(Curves2DHyperbolaTest, AsHyperbola_RoundTripsRadii)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_hyperbola_t aHyperbola = {MakeStandardAxis2d(), 3.0, 2.0};
  occtl_rep_id_t                 aId        = {};
  ASSERT_EQ(occtl_curve2d_create_hyperbola(aGraph, aHyperbola, &aId), OCCTL_OK);

  occtl_geom2d_hyperbola_t aOut{};
  ASSERT_EQ(occtl_curve2d_as_hyperbola(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.major_radius, 3.0, 1e-14);
  EXPECT_NEAR(aOut.minor_radius, 2.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curves2DHyperbolaTest, WrongKindOnEllipse_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_ellipse_t aEllipse = {MakeStandardAxis2d(), 2.0, 1.0};
  occtl_rep_id_t               aId      = {};
  ASSERT_EQ(occtl_curve2d_create_ellipse(aGraph, aEllipse, &aId), OCCTL_OK);

  occtl_geom2d_hyperbola_t aOut{};
  EXPECT_EQ(occtl_curve2d_as_hyperbola(aGraph, aId, &aOut), OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(Curves2DParabolaTest, Construct_KindIsParabola)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_parabola_t aParabola = {MakeStandardAxis2d(), 1.5};
  occtl_rep_id_t                aId       = {};
  ASSERT_EQ(occtl_curve2d_create_parabola(aGraph, aParabola, &aId), OCCTL_OK);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_PARABOLA);
  occtl_graph_free(aGraph);
}

TEST(Curves2DParabolaTest, AsParabola_RoundTripsFocalLength)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_parabola_t aParabola = {MakeStandardAxis2d(), 1.5};
  occtl_rep_id_t                aId       = {};
  ASSERT_EQ(occtl_curve2d_create_parabola(aGraph, aParabola, &aId), OCCTL_OK);

  occtl_geom2d_parabola_t aOut{};
  ASSERT_EQ(occtl_curve2d_as_parabola(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.focal_length, 1.5, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curves2DParabolaTest, WrongKindOnEllipse_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_ellipse_t aEllipse = {MakeStandardAxis2d(), 2.0, 1.0};
  occtl_rep_id_t               aId      = {};
  ASSERT_EQ(occtl_curve2d_create_ellipse(aGraph, aEllipse, &aId), OCCTL_OK);

  occtl_geom2d_parabola_t aOut{};
  EXPECT_EQ(occtl_curve2d_as_parabola(aGraph, aId, &aOut), OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBezierTest, Construct_KindIsBezier)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_point2_t               aPoles[] = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}};
  occtl_curve2d_bezier_create_info_t aInfo    = OCCTL_CURVE2D_BEZIER_CREATE_INFO_INIT;
  aInfo.poles                                 = aPoles;
  aInfo.pole_count                            = 3;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_curve2d_create_bezier(aGraph, &aInfo, &aId), OCCTL_OK);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_BEZIER);
  int32_t aDegree = 0;
  ASSERT_EQ(occtl_curve2d_bezier_degree(aGraph, aId, &aDegree), OCCTL_OK);
  EXPECT_EQ(aDegree, 2);
  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_bezier_pole_count(aGraph, aId, &aCount), OCCTL_OK);
  EXPECT_EQ(aCount, 3u);
  int32_t aRational = 0;
  ASSERT_EQ(occtl_curve2d_bezier_is_rational(aGraph, aId, &aRational), OCCTL_OK);
  EXPECT_EQ(aRational, 0);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBezierTest, Rational_ReportsRational)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_point2_t               aPoles[]   = {{0.0, 0.0}, {1.0, 0.0}};
  const double                       aWeights[] = {1.0, 3.0};
  occtl_curve2d_bezier_create_info_t aInfo      = OCCTL_CURVE2D_BEZIER_CREATE_INFO_INIT;
  aInfo.poles                                   = aPoles;
  aInfo.weights                                 = aWeights;
  aInfo.pole_count                              = 2;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_curve2d_create_bezier(aGraph, &aInfo, &aId), OCCTL_OK);
  int32_t aRational = 0;
  ASSERT_EQ(occtl_curve2d_bezier_is_rational(aGraph, aId, &aRational), OCCTL_OK);
  EXPECT_EQ(aRational, 1);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBezierTest, VersionMismatch_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_curve2d_bezier_create_info_t aInfo = OCCTL_CURVE2D_BEZIER_CREATE_INFO_INIT;
  aInfo.struct_version                     = 99u;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_curve2d_create_bezier(aGraph, &aInfo, &aId), OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(Curves2DBezierTest, PNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_curve2d_bezier_create_info_t aInfo = OCCTL_CURVE2D_BEZIER_CREATE_INFO_INIT;
  aInfo.p_next                             = reinterpret_cast<const void*>(0x1);

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_curve2d_create_bezier(aGraph, &aInfo, &aId), OCCTL_INVALID_ARGUMENT);
  occtl_graph_free(aGraph);
}

TEST(Curves2DTrimmedTest, Construct_KindIsTrimmed)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine    = {MakeStandardAxis2d()};
  occtl_rep_id_t            aBasisId = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aBasisId), OCCTL_OK);

  occtl_curve_trimmed_create_info_t aInfo;
  occtl_curve_trimmed_create_info_init(&aInfo);
  aInfo.basis   = aBasisId;
  aInfo.u_first = 0.0;
  aInfo.u_last  = 3.0;

  occtl_rep_id_t aTrimmedId = {};
  ASSERT_EQ(occtl_curve2d_create_trimmed(aGraph, &aInfo, &aTrimmedId), OCCTL_OK);
  occtl_curve_kind_t aKind;
  ASSERT_EQ(occtl_curve2d_kind(aGraph, aTrimmedId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_CURVE_KIND_TRIMMED);

  double aMin = 0.0, aMax = 0.0;
  ASSERT_EQ(occtl_curve2d_parameter_range(aGraph, aTrimmedId, &aMin, &aMax), OCCTL_OK);
  EXPECT_NEAR(aMax, 3.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(Curves2DTrimmedTest, PNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine    = {MakeStandardAxis2d()};
  occtl_rep_id_t            aBasisId = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aBasisId), OCCTL_OK);

  occtl_curve_trimmed_create_info_t aInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aInfo.p_next                            = reinterpret_cast<const void*>(0x1);
  aInfo.basis                             = aBasisId;
  aInfo.u_first                           = 0.0;
  aInfo.u_last                            = 1.0;

  occtl_rep_id_t aTrimmedId = {};
  EXPECT_EQ(occtl_curve2d_create_trimmed(aGraph, &aInfo, &aTrimmedId), OCCTL_INVALID_ARGUMENT);
  occtl_graph_free(aGraph);
}

TEST(Curves2DTrimmedTest, OffsetCreateInfoPNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {MakeStandardAxis2d()};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);

  occtl_curve2d_offset_create_info_t anOffset = OCCTL_CURVE2D_OFFSET_CREATE_INFO_INIT;
  anOffset.p_next                             = reinterpret_cast<const void*>(0x1);
  anOffset.basis                              = aId;
  anOffset.offset                             = 0.25;

  occtl_rep_id_t anOffsetId = {};
  EXPECT_EQ(occtl_curve2d_create_offset(aGraph, &anOffset, &anOffsetId), OCCTL_INVALID_ARGUMENT);
  occtl_graph_free(aGraph);
}

TEST(Curves2DEvalTest, D0_Line_AtOriginReturnsLocation)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {{{1.0, 2.0}, {1.0, 0.0}}};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);

  occtl_point2_t aP{};
  ASSERT_EQ(occtl_curve2d_eval_d0(aGraph, aId, 0.0, &aP), OCCTL_OK);
  EXPECT_NEAR(aP.x, 1.0, 1e-14);
  EXPECT_NEAR(aP.y, 2.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curves2DEvalTest, D1_Line_TangentIsDirection)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {{{0.0, 0.0}, {1.0, 0.0}}};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);

  occtl_point2_t  aP{};
  occtl_vector2_t aD1{};
  ASSERT_EQ(occtl_curve2d_eval_d1(aGraph, aId, 3.0, &aP, &aD1), OCCTL_OK);
  EXPECT_NEAR(aP.x, 3.0, 1e-14);
  EXPECT_NEAR(aD1.x, 1.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curves2DEvalTest, D0_Circle_AtZeroIsOnXAxis)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 2.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);

  occtl_point2_t aP{};
  ASSERT_EQ(occtl_curve2d_eval_d0(aGraph, aId, 0.0, &aP), OCCTL_OK);
  EXPECT_NEAR(aP.x, 2.0, 1e-14);
  EXPECT_NEAR(aP.y, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curves2DEvalTest, NullGraph_ReturnsInvalidArgument)
{
  occtl_point2_t aP{};
  EXPECT_EQ(occtl_curve2d_eval_d0(nullptr, occtl_rep_id_t{0}, 0.0, &aP), OCCTL_INVALID_ARGUMENT);
}

TEST(Curves2DEvalTest, BSpline_EvaluationSucceeds)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_point2_t                aPoles[] = {{0.0, 0.0}, {1.0, 1.0}, {2.0, 0.0}, {3.0, 1.0}};
  const double                        aKnots[] = {0.0, 1.0};
  const int32_t                       aMults[] = {4, 4};
  occtl_curve2d_bspline_create_info_t aInfo    = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                                  = aPoles;
  aInfo.pole_count                             = 4;
  aInfo.knots                                  = aKnots;
  aInfo.multiplicities                         = aMults;
  aInfo.knot_count                             = 2;
  aInfo.degree                                 = 3;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_curve2d_create_bspline(aGraph, &aInfo, &aId), OCCTL_OK);

  occtl_point2_t aP{};
  ASSERT_EQ(occtl_curve2d_eval_d0(aGraph, aId, 0.0, &aP), OCCTL_OK);
  EXPECT_NEAR(aP.x, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(Curve2dIsClosedTest, Line_NotClosed)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {MakeStandardAxis2d()};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);
  int32_t aClosed = 0;
  ASSERT_EQ(occtl_curve2d_is_closed(aGraph, aId, &aClosed), OCCTL_OK);
  EXPECT_EQ(aClosed, 0);
  occtl_graph_free(aGraph);
}

TEST(Curve2dIsClosedTest, Circle_IsClosed)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 1.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);
  int32_t aClosed = 0;
  ASSERT_EQ(occtl_curve2d_is_closed(aGraph, aId, &aClosed), OCCTL_OK);
  EXPECT_EQ(aClosed, 1);
  occtl_graph_free(aGraph);
}

TEST(Curve2dIsClosedTest, NullGraph_ReturnsInvalidArgument)
{
  int32_t aVal = 0;
  EXPECT_EQ(occtl_curve2d_is_closed(nullptr, occtl_rep_id_t{0}, &aVal), OCCTL_INVALID_ARGUMENT);
}

TEST(BSpline2dViewTest, Init_PopulatesVersionAndZeros)
{
  occtl_curve2d_bspline_t aView{};
  aView.struct_version = 0xdeadbeef;
  aView.degree         = 7;
  aView.poles          = reinterpret_cast<const occtl_point2_t*>(0x1234);
  occtl_curve2d_bspline_init(&aView);
  EXPECT_EQ(aView.struct_version, OCCTL_CURVE2D_BSPLINE_VERSION_1);
  EXPECT_EQ(aView.p_next, nullptr);
  EXPECT_EQ(aView.degree, 0);
  EXPECT_EQ(aView.poles, nullptr);
  EXPECT_EQ(aView.weights, nullptr);
}

TEST(BSpline2dViewTest, Init_NullTolerant)
{
  occtl_curve2d_bspline_init(nullptr);
}

TEST(BSpline2dViewTest, RationalBSpline_ScalarsMatchAtoms)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeRational2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_curve2d_bspline_t aView = OCCTL_CURVE2D_BSPLINE_INIT;
  ASSERT_EQ(occtl_curve2d_as_bspline(aGraph, aId, &aView), OCCTL_OK);

  int32_t aDegree = 0;
  ASSERT_EQ(occtl_curve2d_bspline_degree(aGraph, aId, &aDegree), OCCTL_OK);
  EXPECT_EQ(aView.degree, aDegree);
  int32_t aRational = 0;
  ASSERT_EQ(occtl_curve2d_bspline_is_rational(aGraph, aId, &aRational), OCCTL_OK);
  EXPECT_EQ(aView.is_rational, aRational);
  int32_t aPeriodic = 0;
  ASSERT_EQ(occtl_curve2d_is_periodic(aGraph, aId, &aPeriodic), OCCTL_OK);
  EXPECT_EQ(aView.is_periodic, aPeriodic);
  int32_t aClosed = 0;
  ASSERT_EQ(occtl_curve2d_is_closed(aGraph, aId, &aClosed), OCCTL_OK);
  EXPECT_EQ(aView.is_closed, aClosed);
  occtl_geom_continuity_t aCont = {};
  ASSERT_EQ(occtl_curve2d_continuity(aGraph, aId, &aCont), OCCTL_OK);
  EXPECT_EQ(aView.continuity, static_cast<int32_t>(aCont));
  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_bspline_pole_count(aGraph, aId, &aCount), OCCTL_OK);
  EXPECT_EQ(aView.pole_count, aCount);
  ASSERT_EQ(occtl_curve2d_bspline_knot_count(aGraph, aId, &aCount), OCCTL_OK);
  EXPECT_EQ(aView.knot_count, aCount);
  EXPECT_EQ(aView.is_rational, 1);
  EXPECT_EQ(aView.pole_count, 4u);
  EXPECT_EQ(aView.knot_count, 2u);

  size_t aFlatExpected = 0;
  ASSERT_EQ(occtl_curve2d_bspline_flat_knots(aGraph, aId, nullptr, 0, &aFlatExpected), OCCTL_OK);
  EXPECT_EQ(aView.flat_knot_count, aFlatExpected);

  occtl_graph_free(aGraph);
}

TEST(BSpline2dViewTest, RationalBSpline_PolesMatchAtomized)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeRational2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_curve2d_bspline_t aView = OCCTL_CURVE2D_BSPLINE_INIT;
  ASSERT_EQ(occtl_curve2d_as_bspline(aGraph, aId, &aView), OCCTL_OK);

  occtl_point2_t aAtoms[4]{};
  size_t         aNb = 0;
  ASSERT_EQ(occtl_curve2d_bspline_poles(aGraph, aId, aAtoms, 4, &aNb), OCCTL_OK);
  ASSERT_EQ(aNb, aView.pole_count);

  for (size_t anI = 0; anI < aView.pole_count; ++anI)
  {
    EXPECT_NEAR(aView.poles[anI].x, aAtoms[anI].x, 1e-14);
    EXPECT_NEAR(aView.poles[anI].y, aAtoms[anI].y, 1e-14);
  }
  occtl_graph_free(aGraph);
}

TEST(BSpline2dViewTest, RationalBSpline_KnotsMultsFlatWeightsMatchAtomized)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeRational2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_curve2d_bspline_t aView = OCCTL_CURVE2D_BSPLINE_INIT;
  ASSERT_EQ(occtl_curve2d_as_bspline(aGraph, aId, &aView), OCCTL_OK);

  double  aKnotsAtomic[2]{};
  int32_t aMultsAtomic[2]{};
  size_t  aNb = 0;
  ASSERT_EQ(occtl_curve2d_bspline_knots(aGraph, aId, aKnotsAtomic, 2, &aNb), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_bspline_multiplicities(aGraph, aId, aMultsAtomic, 2, &aNb), OCCTL_OK);
  for (size_t anI = 0; anI < aView.knot_count; ++anI)
  {
    EXPECT_NEAR(aView.knots[anI], aKnotsAtomic[anI], 1e-14);
    EXPECT_EQ(aView.multiplicities[anI], aMultsAtomic[anI]);
  }

  std::vector<double> aFlatAtomic(aView.flat_knot_count);
  ASSERT_EQ(
    occtl_curve2d_bspline_flat_knots(aGraph, aId, aFlatAtomic.data(), aFlatAtomic.size(), &aNb),
    OCCTL_OK);
  for (size_t anI = 0; anI < aView.flat_knot_count; ++anI)
  {
    EXPECT_NEAR(aView.flat_knots[anI], aFlatAtomic[anI], 1e-14);
  }

  ASSERT_NE(aView.weights, nullptr);
  double aWeightsAtomic[4]{};
  ASSERT_EQ(occtl_curve2d_bspline_weights(aGraph, aId, aWeightsAtomic, 4, &aNb), OCCTL_OK);
  for (size_t anI = 0; anI < aView.pole_count; ++anI)
  {
    EXPECT_NEAR(aView.weights[anI], aWeightsAtomic[anI], 1e-14);
  }

  occtl_graph_free(aGraph);
}

TEST(BSpline2dViewTest, NonRational_WeightsAreNull)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeSimple2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_curve2d_bspline_t aView = OCCTL_CURVE2D_BSPLINE_INIT;
  ASSERT_EQ(occtl_curve2d_as_bspline(aGraph, aId, &aView), OCCTL_OK);
  EXPECT_EQ(aView.is_rational, 0);
  EXPECT_EQ(aView.weights, nullptr);
  EXPECT_NE(aView.poles, nullptr);
  EXPECT_NE(aView.knots, nullptr);
  EXPECT_NE(aView.multiplicities, nullptr);
  EXPECT_NE(aView.flat_knots, nullptr);
  occtl_graph_free(aGraph);
}

TEST(BSpline2dViewTest, Line_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_line_t aLine = {MakeStandardAxis2d()};
  occtl_rep_id_t            aId   = {};
  ASSERT_EQ(occtl_curve2d_create_line(aGraph, aLine, &aId), OCCTL_OK);

  occtl_curve2d_bspline_t aView = OCCTL_CURVE2D_BSPLINE_INIT;
  EXPECT_EQ(occtl_curve2d_as_bspline(aGraph, aId, &aView), OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(BSpline2dViewTest, Circle_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 1.0};
  occtl_rep_id_t              aId     = {};
  ASSERT_EQ(occtl_curve2d_create_circle(aGraph, aCircle, &aId), OCCTL_OK);

  occtl_curve2d_bspline_t aView = OCCTL_CURVE2D_BSPLINE_INIT;
  EXPECT_EQ(occtl_curve2d_as_bspline(aGraph, aId, &aView), OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(BSpline2dViewTest, NullGraph_ReturnsInvalidArgument)
{
  occtl_curve2d_bspline_t aView = OCCTL_CURVE2D_BSPLINE_INIT;
  EXPECT_EQ(occtl_curve2d_as_bspline(nullptr, occtl_rep_id_t{0}, &aView), OCCTL_INVALID_ARGUMENT);
}

TEST(BSpline2dViewTest, NullOut_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeSimple2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);
  EXPECT_EQ(occtl_curve2d_as_bspline(aGraph, aId, nullptr), OCCTL_INVALID_ARGUMENT);
  occtl_graph_free(aGraph);
}

TEST(BSpline2dViewTest, BogusVersion_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  occtl_rep_id_t aId = MakeSimple2DBSpline(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_curve2d_bspline_t aView = OCCTL_CURVE2D_BSPLINE_INIT;
  aView.struct_version          = 999u;
  EXPECT_EQ(occtl_curve2d_as_bspline(aGraph, aId, &aView), OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(Curves2DEvalTest, D2D3AndDN_CircleFollowAnalyticCircle)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));

  const occtl_geom2d_circle_t aCircle = {MakeStandardAxis2d(), 2.0};
  occtl_rep_id_t              aId{};
  ASSERT_EQ(OCCTL_OK, occtl_curve2d_create_circle(aGraph, aCircle, &aId));

  occtl_point2_t  aP{};
  occtl_vector2_t aD1{}, aD2{}, aD3{};
  ASSERT_EQ(OCCTL_OK, occtl_curve2d_eval_d2(aGraph, aId, 0.0, &aP, &aD1, &aD2));
  EXPECT_NEAR(aP.x, 2.0, 1.0e-14);
  EXPECT_NEAR(aP.y, 0.0, 1.0e-14);
  EXPECT_NEAR(aD1.x, 0.0, 1.0e-14);
  EXPECT_NEAR(aD1.y, 2.0, 1.0e-14);
  EXPECT_NEAR(aD2.x, -2.0, 1.0e-14);
  EXPECT_NEAR(aD2.y, 0.0, 1.0e-14);

  ASSERT_EQ(OCCTL_OK, occtl_curve2d_eval_d3(aGraph, aId, 0.0, &aP, &aD1, &aD2, &aD3));
  EXPECT_NEAR(aD3.x, 0.0, 1.0e-14);
  EXPECT_NEAR(aD3.y, -2.0, 1.0e-14);

  occtl_vector2_t aDN{};
  ASSERT_EQ(OCCTL_OK, occtl_curve2d_eval_dn(aGraph, aId, 0.0, 2, &aDN));
  EXPECT_NEAR(aDN.x, aD2.x, 1.0e-14);
  EXPECT_NEAR(aDN.y, aD2.y, 1.0e-14);

  occtl_graph_free(aGraph);
}

} // namespace
