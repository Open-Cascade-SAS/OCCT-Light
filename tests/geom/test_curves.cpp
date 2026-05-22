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
#include <occtl/occtl_curves.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{

// Helper: standard XY-plane axis2 placement (normal = Z, x-ref = X).
occtl_axis2_placement_t MakeXYPlaneAxis2()
{
  return {
    {0.0, 0.0, 0.0}, // location
    {0.0, 0.0, 1.0}, // x_dir = normal/Z
    {1.0, 0.0, 0.0}  // x_dir_ref = X
  };
}

// Builds a simple degree-3 clamped B-spline: 4 poles, 2 distinct knots (mult 4 each).
// n=4, d=3 -> flat_knot_count = 4+3+1 = 8 = 4+4
occtl_status_t MakeSimpleBSpline(occtl_graph_t* graph, occtl_rep_id_t* out_id)
{
  static const occtl_point3_t kPoles[4] = {{0.0, 0.0, 0.0},
                                           {1.0, 2.0, 0.0},
                                           {2.0, 2.0, 0.0},
                                           {3.0, 0.0, 0.0}};
  static const double         kKnots[2] = {0.0, 1.0};
  static const int32_t        kMults[2] = {4, 4};

  occtl_curve_bspline_create_info_t aInfo = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                             = kPoles;
  aInfo.pole_count                        = 4;
  aInfo.knots                             = kKnots;
  aInfo.multiplicities                    = kMults;
  aInfo.knot_count                        = 2;
  aInfo.degree                            = 3;

  return occtl_curve_create_bspline(graph, &aInfo, out_id);
}

// Builds a 4-pole rational degree-3 B-spline (clamped, knot multiplicities {4, 4}).
occtl_status_t MakeRationalBSpline(occtl_graph_t* graph, occtl_rep_id_t* out_id)
{
  static const occtl_point3_t kPoles[4]   = {{0.0, 0.0, 0.0},
                                             {1.0, 2.0, 0.0},
                                             {2.0, 2.0, 0.0},
                                             {3.0, 0.0, 0.0}};
  static const double         kWeights[4] = {1.0, 2.0, 0.5, 1.5};
  static const double         kKnots[2]   = {0.0, 1.0};
  static const int32_t        kMults[2]   = {4, 4};

  occtl_curve_bspline_create_info_t aInfo = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                             = kPoles;
  aInfo.pole_count                        = 4;
  aInfo.weights                           = kWeights;
  aInfo.knots                             = kKnots;
  aInfo.multiplicities                    = kMults;
  aInfo.knot_count                        = 2;
  aInfo.degree                            = 3;

  return occtl_curve_create_bspline(graph, &aInfo, out_id);
}

} // namespace

TEST(CurvesLineTest, Construct_KindIsLine)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_LINE, aKind);

  occtl_graph_free(aGraph);
}

TEST(CurvesLineTest, AsLine_RoundTripsLocationAndDirection)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{1.0, 2.0, 3.0}, {0.0, 1.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  occtl_geom_line_t aOut{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_as_line(aGraph, aId, &aOut));
  EXPECT_NEAR(aOut.location.x, 1.0, 1e-14);
  EXPECT_NEAR(aOut.location.y, 2.0, 1e-14);
  EXPECT_NEAR(aOut.location.z, 3.0, 1e-14);
  EXPECT_NEAR(aOut.direction.x, 0.0, 1e-14);
  EXPECT_NEAR(aOut.direction.y, 1.0, 1e-14);
  EXPECT_NEAR(aOut.direction.z, 0.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesCircleTest, Construct_KindIsCircle)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 5.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_CIRCLE, aKind);

  occtl_graph_free(aGraph);
}

TEST(CurvesCircleTest, Construct_IsPeriodic)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 3.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  int32_t aPeriodic;
  ASSERT_EQ(OCCTL_OK, occtl_curve_is_periodic(aGraph, aId, &aPeriodic));
  EXPECT_EQ(1, aPeriodic);

  occtl_graph_free(aGraph);
}

TEST(CurvesCircleTest, AsCircle_RoundTripsRadius)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 7.5};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_geom_circle_t aOut{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_as_circle(aGraph, aId, &aOut));
  EXPECT_NEAR(aOut.radius, 7.5, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesCircleTest, InvalidRadius_ReturnsGeometryInvalid)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), -1.0};
  occtl_rep_id_t            aId     = {};
  EXPECT_EQ(OCCTL_GEOMETRY_INVALID, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_graph_free(aGraph);
}

TEST(CurvesEllipseTest, Construct_KindIsEllipse)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_ellipse_t aEllipse = {MakeXYPlaneAxis2(), 5.0, 3.0};
  occtl_rep_id_t             aId      = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_ellipse(aGraph, aEllipse, &aId));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_ELLIPSE, aKind);

  occtl_graph_free(aGraph);
}

TEST(CurvesEllipseTest, AsEllipse_RoundTripsRadii)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_ellipse_t aEllipse = {MakeXYPlaneAxis2(), 6.0, 2.0};
  occtl_rep_id_t             aId      = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_ellipse(aGraph, aEllipse, &aId));

  occtl_geom_ellipse_t aOut{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_as_ellipse(aGraph, aId, &aOut));
  EXPECT_NEAR(aOut.major_radius, 6.0, 1e-14);
  EXPECT_NEAR(aOut.minor_radius, 2.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesHyperbolaTest, Construct_KindIsHyperbola)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_hyperbola_t aHypr = {MakeXYPlaneAxis2(), 4.0, 3.0};
  occtl_rep_id_t               aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_hyperbola(aGraph, aHypr, &aId));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_HYPERBOLA, aKind);

  occtl_graph_free(aGraph);
}

TEST(CurvesHyperbolaTest, AsHyperbola_RoundTripsRadii)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_hyperbola_t aHypr = {MakeXYPlaneAxis2(), 4.0, 3.0};
  occtl_rep_id_t               aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_hyperbola(aGraph, aHypr, &aId));

  occtl_geom_hyperbola_t aOut{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_as_hyperbola(aGraph, aId, &aOut));
  EXPECT_NEAR(aOut.major_radius, 4.0, 1e-14);
  EXPECT_NEAR(aOut.minor_radius, 3.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesParabolaTest, Construct_KindIsParabola)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_parabola_t aParab = {MakeXYPlaneAxis2(), 2.0};
  occtl_rep_id_t              aId    = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_parabola(aGraph, aParab, &aId));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_PARABOLA, aKind);

  occtl_graph_free(aGraph);
}

TEST(CurvesWrongKindTest, AsCircle_OnLine_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  occtl_geom_circle_t aOut{};
  EXPECT_EQ(OCCTL_WRONG_KIND, occtl_curve_as_circle(aGraph, aId, &aOut));
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_NE(aErr->message, nullptr);
  EXPECT_GT(strlen(aErr->message), 0u);

  occtl_graph_free(aGraph);
}

TEST(CurvesWrongKindTest, AsLine_OnCircle_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 3.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_geom_line_t aOut{};
  EXPECT_EQ(OCCTL_WRONG_KIND, occtl_curve_as_line(aGraph, aId, &aOut));

  occtl_graph_free(aGraph);
}

TEST(CurvesParamRangeTest, Circle_FullPeriod)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 1.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  double aMin = 0.0, aMax = 0.0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_parameter_range(aGraph, aId, &aMin, &aMax));
  EXPECT_NEAR(aMin, 0.0, 1e-14);
  EXPECT_NEAR(aMax, 2.0 * M_PI, 1e-10);

  occtl_graph_free(aGraph);
}

TEST(CurvesCloneTest, SameKindAndRange)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 4.0};
  occtl_rep_id_t            aId1    = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId1));

  occtl_rep_id_t aId2 = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId2));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aId2, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_CIRCLE, aKind);

  double aMin1 = 0.0, aMax1 = 0.0, aMin2 = 0.0, aMax2 = 0.0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_parameter_range(aGraph, aId1, &aMin1, &aMax1));
  ASSERT_EQ(OCCTL_OK, occtl_curve_parameter_range(aGraph, aId2, &aMin2, &aMax2));
  EXPECT_NEAR(aMin1, aMin2, 1e-14);
  EXPECT_NEAR(aMax1, aMax2, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineTest, Construct_KindIsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_BSPLINE, aKind);

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineTest, ScalarQueries_MatchInput)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  int32_t aDegree;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_degree(aGraph, aId, &aDegree));
  EXPECT_EQ(3, aDegree);

  size_t aPoleCount;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_pole_count(aGraph, aId, &aPoleCount));
  EXPECT_EQ(4u, aPoleCount);

  size_t aKnotCount;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_knot_count(aGraph, aId, &aKnotCount));
  EXPECT_EQ(2u, aKnotCount);

  int32_t aRational;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_is_rational(aGraph, aId, &aRational));
  EXPECT_EQ(0, aRational);

  occtl_graph_free(aGraph);
}

TEST(CurvesBezierSegmentsTest, OptionsInit_HasDefaults)
{
  occtl_curve_bezier_segments_options_t aOptions{};
  occtl_curve_bezier_segments_options_init(&aOptions);
  EXPECT_EQ(aOptions.struct_version, OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_VERSION_1);
  EXPECT_EQ(aOptions.p_next, nullptr);
  EXPECT_EQ(aOptions.use_range, 0);
  EXPECT_NEAR(aOptions.parametric_tolerance, 1.0e-9, 1.0e-14);
}

TEST(CurvesBezierSegmentsTest, BSplineWithInteriorKnot_ReturnsTwoBezierCurves)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_point3_t aPoles[5] = {{0.0, 0.0, 0.0},
                                    {1.0, 2.0, 0.0},
                                    {2.0, 0.0, 0.0},
                                    {3.0, 2.0, 0.0},
                                    {4.0, 0.0, 0.0}};
  const double         aKnots[3] = {0.0, 0.5, 1.0};
  const int32_t        aMults[3] = {3, 2, 3};

  occtl_curve_bspline_create_info_t aInfo = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                             = aPoles;
  aInfo.pole_count                        = 5;
  aInfo.knots                             = aKnots;
  aInfo.multiplicities                    = aMults;
  aInfo.knot_count                        = 3;
  aInfo.degree                            = 2;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_bspline(aGraph, &aInfo, &aId));

  occtl_rep_id_t* aSegs  = nullptr;
  size_t          aCount = 0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_to_bezier_segments(aGraph, aId, nullptr, &aSegs, &aCount));
  ASSERT_EQ(aCount, 2u);
  ASSERT_NE(aSegs, nullptr);

  occtl_curve_kind_t aKind0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aSegs[0], &aKind0));
  EXPECT_EQ(OCCTL_CURVE_KIND_BEZIER, aKind0);
  occtl_curve_kind_t aKind1;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aSegs[1], &aKind1));
  EXPECT_EQ(OCCTL_CURVE_KIND_BEZIER, aKind1);

  occtl_point3_t aStart{};
  occtl_point3_t anEnd{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aSegs[0], 0.0, &aStart));
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aSegs[1], 1.0, &anEnd));
  EXPECT_NEAR(aStart.x, 0.0, 1.0e-12);
  EXPECT_NEAR(anEnd.x, 4.0, 1.0e-12);

  occtl_curve_free_bezier_segments(aSegs);
  occtl_graph_free(aGraph);
}

TEST(CurvesBezierSegmentsTest, LineWithoutRange_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  occtl_rep_id_t* aSegs  = nullptr;
  size_t          aCount = 0;
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT,
            occtl_curve_to_bezier_segments(aGraph, aId, nullptr, &aSegs, &aCount));
  EXPECT_EQ(aSegs, nullptr);
  EXPECT_EQ(aCount, 0u);

  occtl_graph_free(aGraph);
}

TEST(CurvesBezierSegmentsTest, LineWithRange_ReturnsOneBezierCurve)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  occtl_curve_bezier_segments_options_t aOptions = OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_INIT;
  aOptions.use_range                             = 1;
  aOptions.u_first                               = 0.0;
  aOptions.u_last                                = 5.0;

  occtl_rep_id_t* aSegs  = nullptr;
  size_t          aCount = 0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_to_bezier_segments(aGraph, aId, &aOptions, &aSegs, &aCount));
  ASSERT_EQ(aCount, 1u);
  ASSERT_NE(aSegs, nullptr);

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aSegs[0], &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_BEZIER, aKind);

  occtl_point3_t anEnd{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aSegs[0], 1.0, &anEnd));
  EXPECT_NEAR(anEnd.x, 5.0, 1.0e-12);

  occtl_curve_free_bezier_segments(aSegs);
  occtl_graph_free(aGraph);
}

TEST(CurvesBezierSegmentsTest, BadVersion_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  occtl_curve_bezier_segments_options_t aOptions = OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_INIT;
  aOptions.struct_version                        = 99u;
  occtl_rep_id_t* aSegs                          = nullptr;
  size_t          aCount                         = 0;
  EXPECT_EQ(OCCTL_VERSION_MISMATCH,
            occtl_curve_to_bezier_segments(aGraph, aId, &aOptions, &aSegs, &aCount));

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineTest, TwoCallPoles_RoundTrip)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  size_t aNb = 0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_poles(aGraph, aId, nullptr, 0, &aNb));
  ASSERT_EQ(aNb, 4u);

  occtl_point3_t aPoles[4]{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_poles(aGraph, aId, aPoles, 4, &aNb));
  EXPECT_NEAR(aPoles[0].x, 0.0, 1e-14);
  EXPECT_NEAR(aPoles[1].x, 1.0, 1e-14);
  EXPECT_NEAR(aPoles[2].x, 2.0, 1e-14);
  EXPECT_NEAR(aPoles[3].x, 3.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineTest, TwoCallKnots_RoundTrip)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  size_t  aNb = 0;
  double  aKnots[2]{};
  int32_t aMults[2]{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_knots(aGraph, aId, aKnots, 2, &aNb));
  ASSERT_EQ(aNb, 2u);
  EXPECT_NEAR(aKnots[0], 0.0, 1e-14);
  EXPECT_NEAR(aKnots[1], 1.0, 1e-14);

  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_multiplicities(aGraph, aId, aMults, 2, &aNb));
  EXPECT_EQ(aMults[0], 4);
  EXPECT_EQ(aMults[1], 4);

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineTest, WeightsOnNonRational_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  size_t aNb = 0;
  EXPECT_EQ(OCCTL_WRONG_KIND, occtl_curve_bspline_weights(aGraph, aId, nullptr, 0, &aNb));
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_GT(strlen(aErr->message), 0u);

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineTest, PolesView_DataMatchesTwoCall)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  const occtl_point3_t* aData = nullptr;
  size_t                aNb   = 0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_poles_view(aGraph, aId, &aData, &aNb));
  ASSERT_NE(aData, nullptr);
  ASSERT_EQ(aNb, 4u);
  EXPECT_NEAR(aData[0].x, 0.0, 1e-14);
  EXPECT_NEAR(aData[3].x, 3.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineRationalTest, IsRational_ReturnsOne)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  static const occtl_point3_t kPoles[3]   = {{1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}};
  static const double         kWeights[3] = {1.0, 0.70710678118, 1.0};
  static const double         kKnots[2]   = {0.0, 1.0};
  static const int32_t        kMults[2]   = {3, 3};

  occtl_curve_bspline_create_info_t aInfo = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                             = kPoles;
  aInfo.pole_count                        = 3;
  aInfo.weights                           = kWeights;
  aInfo.knots                             = kKnots;
  aInfo.multiplicities                    = kMults;
  aInfo.knot_count                        = 2;
  aInfo.degree                            = 2;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_bspline(aGraph, &aInfo, &aId));

  int32_t aRational;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_is_rational(aGraph, aId, &aRational));
  EXPECT_EQ(1, aRational);

  size_t aNb = 0;
  double aWeightsOut[3]{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_weights(aGraph, aId, aWeightsOut, 3, &aNb));
  ASSERT_EQ(aNb, 3u);
  EXPECT_NEAR(aWeightsOut[0], 1.0, 1e-12);
  EXPECT_NEAR(aWeightsOut[1], 0.70710678118, 1e-12);
  EXPECT_NEAR(aWeightsOut[2], 1.0, 1e-12);

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineTest, VersionMismatch_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_curve_bspline_create_info_t aInfo = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
  aInfo.struct_version                    = 99u;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(OCCTL_VERSION_MISMATCH, occtl_curve_create_bspline(aGraph, &aInfo, &aId));

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineTest, PNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_curve_bspline_create_info_t aInfo = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
  aInfo.p_next                            = reinterpret_cast<const void*>(0x1);

  occtl_rep_id_t aId = {};
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_create_bspline(aGraph, &aInfo, &aId));

  occtl_graph_free(aGraph);
}

TEST(CurvesBSplineTest, InvalidPeriodicFlag_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_point3_t kPoles[4] = {{0.0, 0.0, 0.0},
                                    {1.0, 2.0, 0.0},
                                    {2.0, 2.0, 0.0},
                                    {3.0, 0.0, 0.0}};
  const double         kKnots[2] = {0.0, 1.0};
  const int32_t        kMults[2] = {4, 4};

  occtl_curve_bspline_create_info_t aInfo = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                             = kPoles;
  aInfo.pole_count                        = 4;
  aInfo.knots                             = kKnots;
  aInfo.multiplicities                    = kMults;
  aInfo.knot_count                        = 2;
  aInfo.degree                            = 3;
  aInfo.is_periodic                       = 2;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_create_bspline(aGraph, &aInfo, &aId));

  occtl_graph_free(aGraph);
}

TEST(CurvesAirfoilNaca4Test, InfoInit_HasDefaults)
{
  occtl_curve_airfoil_naca4_info_t aInfo{};
  occtl_curve_airfoil_naca4_info_init(&aInfo);
  EXPECT_EQ(aInfo.struct_version, OCCTL_CURVE_AIRFOIL_NACA4_INFO_VERSION_1);
  EXPECT_EQ(aInfo.p_next, nullptr);
  EXPECT_NEAR(aInfo.max_camber, 0.0, 1.0e-14);
  EXPECT_NEAR(aInfo.camber_position, 0.0, 1.0e-14);
  EXPECT_NEAR(aInfo.thickness, 0.12, 1.0e-14);
  EXPECT_NEAR(aInfo.chord_length, 1.0, 1.0e-14);
  EXPECT_EQ(aInfo.point_count, 50u);
  EXPECT_EQ(aInfo.finite_trailing_edge, 0);
}

TEST(CurvesAirfoilNaca4Test, Make2412_ReturnsBSplineProfile)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_curve_airfoil_naca4_info_t aInfo = OCCTL_CURVE_AIRFOIL_NACA4_INFO_INIT;
  aInfo.max_camber                       = 0.02;
  aInfo.camber_position                  = 0.4;
  aInfo.thickness                        = 0.12;
  aInfo.chord_length                     = 2.0;
  aInfo.point_count                      = 40;
  aInfo.tolerance                        = 1.0e-6;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_airfoil_naca4(aGraph, &aInfo, &aId));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_BSPLINE, aKind);

  size_t aPoleCount;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_pole_count(aGraph, aId, &aPoleCount));
  EXPECT_GT(aPoleCount, 0u);

  double aMin = 0.0;
  double aMax = 0.0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_parameter_range(aGraph, aId, &aMin, &aMax));
  EXPECT_GT(aMax, aMin);

  occtl_point3_t aStart{};
  occtl_point3_t aEnd{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aId, aMin, &aStart));
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aId, aMax, &aEnd));
  EXPECT_NEAR(aStart.x, 2.0, 5.0e-3);
  EXPECT_NEAR(aEnd.x, 2.0, 5.0e-3);
  EXPECT_NEAR(aStart.z, 0.0, 1.0e-12);
  EXPECT_NEAR(aEnd.z, 0.0, 1.0e-12);

  occtl_graph_free(aGraph);
}

TEST(CurvesAirfoilNaca4Test, BadFlag_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_curve_airfoil_naca4_info_t aInfo = OCCTL_CURVE_AIRFOIL_NACA4_INFO_INIT;
  aInfo.finite_trailing_edge             = 2;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_create_airfoil_naca4(aGraph, &aInfo, &aId));

  occtl_graph_free(aGraph);
}

TEST(CurvesTrimmedTest, Construct_KindIsTrimmed)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine    = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aBasisId = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aBasisId));

  occtl_curve_trimmed_create_info_t aInfo;
  occtl_curve_trimmed_create_info_init(&aInfo);
  aInfo.basis   = aBasisId;
  aInfo.u_first = 0.0;
  aInfo.u_last  = 5.0;

  occtl_rep_id_t aTrimmedId = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_trimmed(aGraph, &aInfo, &aTrimmedId));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aTrimmedId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_TRIMMED, aKind);

  double aMin = 0.0, aMax = 0.0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_parameter_range(aGraph, aTrimmedId, &aMin, &aMax));
  EXPECT_NEAR(aMin, 0.0, 1e-14);
  EXPECT_NEAR(aMax, 5.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesTrimmedTest, PNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine    = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aBasisId = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aBasisId));

  occtl_curve_trimmed_create_info_t aInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aInfo.p_next                            = reinterpret_cast<const void*>(0x1);
  aInfo.basis                             = aBasisId;
  aInfo.u_first                           = 0.0;
  aInfo.u_last                            = 1.0;

  occtl_rep_id_t aTrimmedId = {};
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_create_trimmed(aGraph, &aInfo, &aTrimmedId));

  occtl_graph_free(aGraph);
}

TEST(CurvesBezierTest, Construct_KindIsBezier)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_point3_t             aPoles[] = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  occtl_curve_bezier_create_info_t aInfo    = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
  aInfo.poles                               = aPoles;
  aInfo.pole_count                          = 3;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_bezier(aGraph, &aInfo, &aId));

  occtl_curve_kind_t aKind;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, aId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_BEZIER, aKind);

  int32_t aDegree;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bezier_degree(aGraph, aId, &aDegree));
  EXPECT_EQ(2, aDegree);

  size_t aPoleCount;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bezier_pole_count(aGraph, aId, &aPoleCount));
  EXPECT_EQ(3u, aPoleCount);

  int32_t aRational;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bezier_is_rational(aGraph, aId, &aRational));
  EXPECT_EQ(0, aRational);

  occtl_graph_free(aGraph);
}

TEST(CurvesBezierTest, Rational_ReportsRational)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_point3_t             aPoles[]   = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  const double                     aWeights[] = {1.0, 2.0};
  occtl_curve_bezier_create_info_t aInfo      = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
  aInfo.poles                                 = aPoles;
  aInfo.weights                               = aWeights;
  aInfo.pole_count                            = 2;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_bezier(aGraph, &aInfo, &aId));

  int32_t aRational;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bezier_is_rational(aGraph, aId, &aRational));
  EXPECT_EQ(1, aRational);

  occtl_graph_free(aGraph);
}

TEST(CurvesBezierTest, WrongKindOnNonBezier_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 1.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  int32_t aDegree;
  EXPECT_EQ(OCCTL_WRONG_KIND, occtl_curve_bezier_degree(aGraph, aId, &aDegree));
  size_t aCount;
  EXPECT_EQ(OCCTL_WRONG_KIND, occtl_curve_bezier_pole_count(aGraph, aId, &aCount));
  int32_t aRational;
  EXPECT_EQ(OCCTL_WRONG_KIND, occtl_curve_bezier_is_rational(aGraph, aId, &aRational));

  occtl_graph_free(aGraph);
}

TEST(CurvesBezierTest, VersionMismatch_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_curve_bezier_create_info_t aInfo = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
  aInfo.struct_version                   = 99u;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(OCCTL_VERSION_MISMATCH, occtl_curve_create_bezier(aGraph, &aInfo, &aId));

  occtl_graph_free(aGraph);
}

TEST(CurvesBezierTest, PNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_curve_bezier_create_info_t aInfo = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
  aInfo.p_next                           = reinterpret_cast<const void*>(0x1);

  occtl_rep_id_t aId = {};
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_create_bezier(aGraph, &aInfo, &aId));

  occtl_graph_free(aGraph);
}

TEST(CurvesContinuityTest, Circle_IsCN)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 1.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_geom_continuity_t aCont;
  ASSERT_EQ(OCCTL_OK, occtl_curve_continuity(aGraph, aId, &aCont));
  EXPECT_EQ(OCCTL_GEOM_CONTINUITY_CN, aCont);

  occtl_graph_free(aGraph);
}

TEST(CurvesEvalTest, D0_Line_AtOriginReturnsLocation)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{1.0, 2.0, 3.0}, {0.0, 1.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  occtl_point3_t aP{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aId, 0.0, &aP));
  EXPECT_NEAR(aP.x, 1.0, 1e-14);
  EXPECT_NEAR(aP.y, 2.0, 1e-14);
  EXPECT_NEAR(aP.z, 3.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesEvalTest, D1_Line_TangentIsDirection)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  occtl_point3_t  aP{};
  occtl_vector3_t aD1{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d1(aGraph, aId, 5.0, &aP, &aD1));
  EXPECT_NEAR(aP.x, 5.0, 1e-14);
  EXPECT_NEAR(aD1.x, 1.0, 1e-14);
  EXPECT_NEAR(aD1.y, 0.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesEvalTest, D0_Circle_AtZeroIsOnXAxis)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 3.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_point3_t aP{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aId, 0.0, &aP));
  EXPECT_NEAR(aP.x, 3.0, 1e-14);
  EXPECT_NEAR(aP.y, 0.0, 1e-14);
  EXPECT_NEAR(aP.z, 0.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesEvalTest, D0_Circle_AtHalfPiIsOnYAxis)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 3.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_point3_t aP{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aId, M_PI_2, &aP));
  EXPECT_NEAR(aP.x, 0.0, 1e-14);
  EXPECT_NEAR(aP.y, 3.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesEvalTest, D2_Circle_SecondDerivativePointsInward)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 2.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_point3_t  aP{};
  occtl_vector3_t aD1{}, aD2{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d2(aGraph, aId, 0.0, &aP, &aD1, &aD2));
  EXPECT_NEAR(aP.x, 2.0, 1e-14);
  EXPECT_NEAR(aD1.y, 2.0, 1e-14);
  EXPECT_NEAR(aD2.x, -2.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesEvalTest, DN_Line_SecondDerivativeIsZero)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  occtl_vector3_t aD2{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_dn(aGraph, aId, 0.0, 2, &aD2));
  EXPECT_NEAR(aD2.x, 0.0, 1e-14);
  EXPECT_NEAR(aD2.y, 0.0, 1e-14);
  EXPECT_NEAR(aD2.z, 0.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesEvalTest, NullOutPoint_Succeeds)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 1.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aId, 0.0, nullptr));

  occtl_graph_free(aGraph);
}

TEST(CurvesEvalTest, NullGraph_ReturnsInvalidArgument)
{
  occtl_point3_t aP{};
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_eval_d0(nullptr, {0}, 0.0, &aP));
}

TEST(CurvesEvalTest, BSpline_EvaluationSucceeds)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_point3_t              aPoles[] = {{0.0, 0.0, 0.0},
                                                {1.0, 1.0, 0.0},
                                                {2.0, 0.0, 0.0},
                                                {3.0, 1.0, 0.0}};
  const double                      aKnots[] = {0.0, 1.0};
  const int32_t                     aMults[] = {4, 4};
  occtl_curve_bspline_create_info_t aInfo    = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                                = aPoles;
  aInfo.pole_count                           = 4;
  aInfo.knots                                = aKnots;
  aInfo.multiplicities                       = aMults;
  aInfo.knot_count                           = 2;
  aInfo.degree                               = 3;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_bspline(aGraph, &aInfo, &aId));

  occtl_point3_t aP{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aId, 0.0, &aP));
  EXPECT_NEAR(aP.x, 0.0, 1e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesIsClosedTest, Line_NotClosed)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  int32_t aClosed;
  ASSERT_EQ(OCCTL_OK, occtl_curve_is_closed(aGraph, aId, &aClosed));
  EXPECT_EQ(0, aClosed);

  occtl_graph_free(aGraph);
}

TEST(CurvesIsClosedTest, Circle_IsClosed)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 1.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  int32_t aClosed;
  ASSERT_EQ(OCCTL_OK, occtl_curve_is_closed(aGraph, aId, &aClosed));
  EXPECT_EQ(1, aClosed);

  occtl_graph_free(aGraph);
}

TEST(CurvesIsClosedTest, NullGraph_ReturnsInvalidArgument)
{
  int32_t aClosed = 1;
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_is_closed(nullptr, {0}, &aClosed));
}

TEST(BSplineViewTest, Init_PopulatesVersionAndZeros)
{
  occtl_curve_bspline_t aView{};
  aView.struct_version = 0xdeadbeef;
  aView.degree         = 7;
  aView.poles          = reinterpret_cast<const occtl_point3_t*>(0x1234);
  occtl_curve_bspline_init(&aView);
  EXPECT_EQ(aView.struct_version, OCCTL_CURVE_BSPLINE_VERSION_1);
  EXPECT_EQ(aView.p_next, nullptr);
  EXPECT_EQ(aView.degree, 0);
  EXPECT_EQ(aView.poles, nullptr);
  EXPECT_EQ(aView.weights, nullptr);
}

TEST(BSplineViewTest, Init_NullTolerant)
{
  occtl_curve_bspline_init(nullptr);
}

TEST(BSplineViewTest, RationalBSpline_ScalarsMatchAtoms)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeRationalBSpline(aGraph, &aId));

  occtl_curve_bspline_t aView = OCCTL_CURVE_BSPLINE_INIT;
  ASSERT_EQ(OCCTL_OK, occtl_curve_as_bspline(aGraph, aId, &aView));

  int32_t aDegree;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_degree(aGraph, aId, &aDegree));
  EXPECT_EQ(aView.degree, aDegree);

  int32_t aRational;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_is_rational(aGraph, aId, &aRational));
  EXPECT_EQ(aView.is_rational, aRational);

  int32_t aPeriodic;
  ASSERT_EQ(OCCTL_OK, occtl_curve_is_periodic(aGraph, aId, &aPeriodic));
  EXPECT_EQ(aView.is_periodic, aPeriodic);

  int32_t aClosed;
  ASSERT_EQ(OCCTL_OK, occtl_curve_is_closed(aGraph, aId, &aClosed));
  EXPECT_EQ(aView.is_closed, aClosed);

  occtl_geom_continuity_t aCont;
  ASSERT_EQ(OCCTL_OK, occtl_curve_continuity(aGraph, aId, &aCont));
  EXPECT_EQ(aView.continuity, static_cast<int32_t>(aCont));

  size_t aPoleCnt;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_pole_count(aGraph, aId, &aPoleCnt));
  EXPECT_EQ(aView.pole_count, aPoleCnt);

  size_t aKnotCnt;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_knot_count(aGraph, aId, &aKnotCnt));
  EXPECT_EQ(aView.knot_count, aKnotCnt);

  EXPECT_EQ(aView.is_rational, 1);
  EXPECT_EQ(aView.pole_count, 4u);
  EXPECT_EQ(aView.knot_count, 2u);

  size_t aFlatExpected = 0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_flat_knots(aGraph, aId, nullptr, 0, &aFlatExpected));
  EXPECT_EQ(aView.flat_knot_count, aFlatExpected);

  occtl_graph_free(aGraph);
}

TEST(BSplineViewTest, RationalBSpline_PolesMatchAtomized)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeRationalBSpline(aGraph, &aId));

  occtl_curve_bspline_t aView = OCCTL_CURVE_BSPLINE_INIT;
  ASSERT_EQ(OCCTL_OK, occtl_curve_as_bspline(aGraph, aId, &aView));

  occtl_point3_t aAtoms[4]{};
  size_t         aNb = 0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_poles(aGraph, aId, aAtoms, 4, &aNb));
  ASSERT_EQ(aNb, aView.pole_count);

  for (size_t anI = 0; anI < aView.pole_count; ++anI)
  {
    EXPECT_NEAR(aView.poles[anI].x, aAtoms[anI].x, 1e-14);
    EXPECT_NEAR(aView.poles[anI].y, aAtoms[anI].y, 1e-14);
    EXPECT_NEAR(aView.poles[anI].z, aAtoms[anI].z, 1e-14);
  }

  occtl_graph_free(aGraph);
}

TEST(BSplineViewTest, RationalBSpline_KnotsMultsFlatWeightsMatchAtomized)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeRationalBSpline(aGraph, &aId));

  occtl_curve_bspline_t aView = OCCTL_CURVE_BSPLINE_INIT;
  ASSERT_EQ(OCCTL_OK, occtl_curve_as_bspline(aGraph, aId, &aView));

  double  aKnotsAtomic[2]{};
  int32_t aMultsAtomic[2]{};
  size_t  aNb = 0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_knots(aGraph, aId, aKnotsAtomic, 2, &aNb));
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_multiplicities(aGraph, aId, aMultsAtomic, 2, &aNb));
  for (size_t anI = 0; anI < aView.knot_count; ++anI)
  {
    EXPECT_NEAR(aView.knots[anI], aKnotsAtomic[anI], 1e-14);
    EXPECT_EQ(aView.multiplicities[anI], aMultsAtomic[anI]);
  }

  std::vector<double> aFlatAtomic(aView.flat_knot_count);
  ASSERT_EQ(
    OCCTL_OK,
    occtl_curve_bspline_flat_knots(aGraph, aId, aFlatAtomic.data(), aFlatAtomic.size(), &aNb));
  for (size_t anI = 0; anI < aView.flat_knot_count; ++anI)
  {
    EXPECT_NEAR(aView.flat_knots[anI], aFlatAtomic[anI], 1e-14);
  }

  ASSERT_NE(aView.weights, nullptr);
  double aWeightsAtomic[4]{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_bspline_weights(aGraph, aId, aWeightsAtomic, 4, &aNb));
  for (size_t anI = 0; anI < aView.pole_count; ++anI)
  {
    EXPECT_NEAR(aView.weights[anI], aWeightsAtomic[anI], 1e-14);
  }

  occtl_graph_free(aGraph);
}

TEST(BSplineViewTest, NonRational_WeightsAreNull)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  occtl_curve_bspline_t aView = OCCTL_CURVE_BSPLINE_INIT;
  ASSERT_EQ(OCCTL_OK, occtl_curve_as_bspline(aGraph, aId, &aView));
  EXPECT_EQ(aView.is_rational, 0);
  EXPECT_EQ(aView.weights, nullptr);
  EXPECT_NE(aView.poles, nullptr);
  EXPECT_NE(aView.knots, nullptr);
  EXPECT_NE(aView.multiplicities, nullptr);
  EXPECT_NE(aView.flat_knots, nullptr);

  occtl_graph_free(aGraph);
}

TEST(BSplineViewTest, Line_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aId   = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aId));

  occtl_curve_bspline_t aView = OCCTL_CURVE_BSPLINE_INIT;
  EXPECT_EQ(OCCTL_WRONG_KIND, occtl_curve_as_bspline(aGraph, aId, &aView));

  occtl_graph_free(aGraph);
}

TEST(BSplineViewTest, Circle_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 1.0};
  occtl_rep_id_t            aId     = {};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_curve_bspline_t aView = OCCTL_CURVE_BSPLINE_INIT;
  EXPECT_EQ(OCCTL_WRONG_KIND, occtl_curve_as_bspline(aGraph, aId, &aView));

  occtl_graph_free(aGraph);
}

TEST(BSplineViewTest, NullGraph_ReturnsInvalidArgument)
{
  occtl_curve_bspline_t aView = OCCTL_CURVE_BSPLINE_INIT;
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_as_bspline(nullptr, {0}, &aView));
}

TEST(BSplineViewTest, NullOut_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_as_bspline(aGraph, aId, nullptr));

  occtl_graph_free(aGraph);
}

TEST(BSplineViewTest, BogusVersion_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));
  ASSERT_NE(nullptr, aGraph);

  occtl_rep_id_t aId = {};
  ASSERT_EQ(OCCTL_OK, MakeSimpleBSpline(aGraph, &aId));

  occtl_curve_bspline_t aView = OCCTL_CURVE_BSPLINE_INIT;
  aView.struct_version        = 999u;
  EXPECT_EQ(OCCTL_VERSION_MISMATCH, occtl_curve_as_bspline(aGraph, aId, &aView));

  occtl_graph_free(aGraph);
}

TEST(CurvesMeasureTest, Line_LengthProjectAndParameter_AreConsistent)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aLineId{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aLineId));

  occtl_curve_trimmed_create_info_t aTrim = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aTrim.basis                             = aLineId;
  aTrim.u_first                           = 0.0;
  aTrim.u_last                            = 5.0;
  occtl_rep_id_t aTrimmed{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_trimmed(aGraph, &aTrim, &aTrimmed));

  double aLength = 0.0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_length(aGraph, aTrimmed, &aLength));
  EXPECT_NEAR(aLength, 5.0, 1.0e-12);

  double aParam    = 0.0;
  double aDistance = 0.0;
  ASSERT_EQ(OCCTL_OK,
            occtl_curve_project_point(aGraph, aTrimmed, {2.5, 3.0, 0.0}, &aParam, &aDistance));
  EXPECT_NEAR(aParam, 2.5, 1.0e-12);
  EXPECT_NEAR(aDistance, 3.0, 1.0e-12);

  double aNearest = 0.0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_parameter_of_point(aGraph, aTrimmed, {4.0, 0.0, 0.0}, &aNearest));
  EXPECT_NEAR(aNearest, 4.0, 1.0e-12);

  occtl_graph_free(aGraph);
}

TEST(CurvesTransformTest, ReverseTranslateRotateAndScale_UpdateEvaluatedLine)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));

  const occtl_geom_line_t aLine = {{1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aLineId{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aLineId));

  occtl_rep_id_t aReversed{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_reverse(aGraph, aLineId, &aReversed));
  occtl_vector3_t aD1{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d1(aGraph, aReversed, 0.0, nullptr, &aD1));
  EXPECT_NEAR(aD1.x, -1.0, 1.0e-14);

  occtl_rep_id_t aTranslated{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_translated(aGraph, aLineId, {0.0, 2.0, 0.0}, &aTranslated));
  occtl_point3_t aP{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aTranslated, 0.0, &aP));
  EXPECT_NEAR(aP.x, 1.0, 1.0e-14);
  EXPECT_NEAR(aP.y, 2.0, 1.0e-14);

  occtl_rep_id_t aRotated{};
  ASSERT_EQ(
    OCCTL_OK,
    occtl_curve_rotated(aGraph, aLineId, {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}}, M_PI_2, &aRotated));
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d1(aGraph, aRotated, 0.0, nullptr, &aD1));
  EXPECT_NEAR(aD1.x, 0.0, 1.0e-12);
  EXPECT_NEAR(aD1.y, 1.0, 1.0e-12);

  occtl_rep_id_t aScaled{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_scaled(aGraph, aLineId, {0.0, 0.0, 0.0}, 2.0, &aScaled));
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d0(aGraph, aScaled, 0.0, &aP));
  EXPECT_NEAR(aP.x, 2.0, 1.0e-14);

  occtl_graph_free(aGraph);
}

TEST(CurvesTransformTest, RoundTripIdentity_ForReverseTranslateRotateScale)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));

  const occtl_geom_line_t aLine = {{1.5, -2.0, 0.5}, {1.0, 1.0, 0.0}};
  occtl_rep_id_t          aLineId{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aLineId));

  occtl_point3_t  aBasePoint{};
  occtl_vector3_t aBaseD1{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d1(aGraph, aLineId, 2.25, &aBasePoint, &aBaseD1));

  const auto expectSameAtParameter = [&](const occtl_rep_id_t theId) {
    occtl_point3_t  aPoint{};
    occtl_vector3_t aD1{};
    ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d1(aGraph, theId, 2.25, &aPoint, &aD1));
    EXPECT_NEAR(aPoint.x, aBasePoint.x, 1.0e-12);
    EXPECT_NEAR(aPoint.y, aBasePoint.y, 1.0e-12);
    EXPECT_NEAR(aPoint.z, aBasePoint.z, 1.0e-12);
    EXPECT_NEAR(aD1.x, aBaseD1.x, 1.0e-12);
    EXPECT_NEAR(aD1.y, aBaseD1.y, 1.0e-12);
    EXPECT_NEAR(aD1.z, aBaseD1.z, 1.0e-12);
  };

  occtl_rep_id_t aReverse1{};
  occtl_rep_id_t aReverse2{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_reverse(aGraph, aLineId, &aReverse1));
  ASSERT_EQ(OCCTL_OK, occtl_curve_reverse(aGraph, aReverse1, &aReverse2));
  expectSameAtParameter(aReverse2);

  occtl_rep_id_t aTranslated{};
  occtl_rep_id_t aTranslatedBack{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_translated(aGraph, aLineId, {0.25, -0.5, 1.25}, &aTranslated));
  ASSERT_EQ(OCCTL_OK,
            occtl_curve_translated(aGraph, aTranslated, {-0.25, 0.5, -1.25}, &aTranslatedBack));
  expectSameAtParameter(aTranslatedBack);

  occtl_rep_id_t aRotated{};
  occtl_rep_id_t aRotatedBack{};
  ASSERT_EQ(OCCTL_OK,
            occtl_curve_rotated(aGraph,
                                aLineId,
                                {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}},
                                M_PI / 3.0,
                                &aRotated));
  ASSERT_EQ(OCCTL_OK,
            occtl_curve_rotated(aGraph,
                                aRotated,
                                {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}},
                                -M_PI / 3.0,
                                &aRotatedBack));
  expectSameAtParameter(aRotatedBack);

  occtl_rep_id_t aScaled{};
  occtl_rep_id_t aScaledBack{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_scaled(aGraph, aLineId, {1.0, 2.0, -1.0}, 2.5, &aScaled));
  ASSERT_EQ(OCCTL_OK, occtl_curve_scaled(aGraph, aScaled, {1.0, 2.0, -1.0}, 0.4, &aScaledBack));
  expectSameAtParameter(aScaledBack);

  occtl_graph_free(aGraph);
}

TEST(CurvesConstructionTest, InterpolatedAndOffset_CreateExpectedKinds)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));

  const occtl_point3_t            aPoints[3] = {{0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {2.0, 0.0, 0.0}};
  occtl_curve_interpolated_info_t anInterp   = OCCTL_CURVE_INTERPOLATED_INFO_INIT;
  anInterp.points                            = aPoints;
  anInterp.point_count                       = 3;
  occtl_rep_id_t anInterpolated{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_interpolated(aGraph, &anInterp, &anInterpolated));
  occtl_curve_kind_t aKind = OCCTL_CURVE_KIND_UNDEFINED;
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, anInterpolated, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_BSPLINE, aKind);

  occtl_curve_offset_create_info_t anOffset = OCCTL_CURVE_OFFSET_CREATE_INFO_INIT;
  anOffset.basis                            = anInterpolated;
  anOffset.offset_dir                       = {0.0, 0.0, 1.0};
  anOffset.offset                           = 0.5;
  occtl_rep_id_t anOffsetId{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_offset(aGraph, &anOffset, &anOffsetId));
  ASSERT_EQ(OCCTL_OK, occtl_curve_kind(aGraph, anOffsetId, &aKind));
  EXPECT_EQ(OCCTL_CURVE_KIND_OFFSET, aKind);

  occtl_graph_free(aGraph);
}

TEST(CurvesConstructionTest, OffsetCreateInfoPNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));

  const occtl_geom_line_t aLine = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aLineId{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, aLine, &aLineId));

  occtl_curve_offset_create_info_t anOffset = OCCTL_CURVE_OFFSET_CREATE_INFO_INIT;
  anOffset.p_next                           = reinterpret_cast<const void*>(0x1);
  anOffset.basis                            = aLineId;
  anOffset.offset_dir                       = {0.0, 0.0, 1.0};
  anOffset.offset                           = 0.25;

  occtl_rep_id_t anOffsetId{};
  EXPECT_EQ(OCCTL_INVALID_ARGUMENT, occtl_curve_create_offset(aGraph, &anOffset, &anOffsetId));

  occtl_graph_free(aGraph);
}

TEST(CurvesIntersectionTest, CrossingLines_ReturnsOwnedResultArray)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));

  occtl_rep_id_t aX{};
  occtl_rep_id_t aY{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}}, &aX));
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_line(aGraph, {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}}, &aY));

  const occtl_curve_intersection_point_t* aResults = nullptr;
  size_t                                  aCount   = 0;
  ASSERT_EQ(OCCTL_OK, occtl_curve_intersect(aGraph, aX, aY, &aResults, &aCount));
  ASSERT_NE(nullptr, aResults);
  ASSERT_GE(aCount, 1u);
  EXPECT_NEAR(aResults[0].point.x, 0.0, 1.0e-12);
  EXPECT_NEAR(aResults[0].point.y, 0.0, 1.0e-12);
  occtl_curve_free_intersection_points(const_cast<occtl_curve_intersection_point_t*>(aResults));
  occtl_curve_free_intersection_points(nullptr);
  occtl_curve_free_bezier_segments(nullptr);

  occtl_graph_free(aGraph);
}

TEST(CurvesEvalTest, D3_Circle_ThirdDerivativeFollowsAnalyticCircle)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(OCCTL_OK, occtl_graph_create(&aGraph));

  const occtl_geom_circle_t aCircle = {MakeXYPlaneAxis2(), 2.0};
  occtl_rep_id_t            aId{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_create_circle(aGraph, aCircle, &aId));

  occtl_point3_t  aP{};
  occtl_vector3_t aD1{}, aD2{}, aD3{};
  ASSERT_EQ(OCCTL_OK, occtl_curve_eval_d3(aGraph, aId, 0.0, &aP, &aD1, &aD2, &aD3));
  EXPECT_NEAR(aP.x, 2.0, 1.0e-14);
  EXPECT_NEAR(aD1.y, 2.0, 1.0e-14);
  EXPECT_NEAR(aD2.x, -2.0, 1.0e-14);
  EXPECT_NEAR(aD3.y, -2.0, 1.0e-14);

  occtl_graph_free(aGraph);
}
