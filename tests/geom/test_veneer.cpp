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

#include <occtl-hpp/curves.hpp>
#include <occtl-hpp/curves2d.hpp>
#include <occtl-hpp/surfaces.hpp>
#include <occtl-hpp/topo.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <utility>
#include <vector>

namespace
{

constexpr double THE_EPS = 1.0e-12;

occtl_axis2_placement_t MakeXYPlaneAxis2()
{
  return {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
}

occtl_axis3_placement_t MakeStdAxis3()
{
  return {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
}

occtl_axis2_placement2d_t MakeStdAxis2d()
{
  return {{0.0, 0.0}, {1.0, 0.0}};
}

occtl::Curve MakeTrimmedLineCurve(occtl_graph_t*            theGraph,
                                  const occtl_point3_t&     theOrigin,
                                  const occtl_direction3_t& theDirection,
                                  double                    theFirst,
                                  double                    theLast)
{
  occtl::Curve aBasis = occtl::Curve::from_line(theGraph, {theOrigin, theDirection});
  occtl_curve_trimmed_create_info_t aInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aInfo.basis                             = aBasis.id();
  aInfo.u_first                           = theFirst;
  aInfo.u_last                            = theLast;
  return occtl::Curve::from_trimmed(theGraph, aInfo);
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle / basics — create curves, verify kinds
// ---------------------------------------------------------------------------

TEST(VeneerLifecycleTest, CreateLine_KindIsLine)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aCurve = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});
    EXPECT_TRUE(static_cast<bool>(aCurve));
    EXPECT_EQ(aCurve.kind(), OCCTL_CURVE_KIND_LINE);
    EXPECT_NE(aCurve.id().bits, 0u);
    EXPECT_EQ(aCurve.graph(), aGraph);
  }
}

TEST(VeneerLifecycleTest, CreateCircle_KindIsCircle)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aCurve = occtl::Curve::from_circle(aGraph, {MakeXYPlaneAxis2(), 2.0});
    EXPECT_TRUE(static_cast<bool>(aCurve));
    EXPECT_EQ(aCurve.kind(), OCCTL_CURVE_KIND_CIRCLE);
  }
}

TEST(VeneerLifecycleTest, DefaultConstructed_IsFalse)
{
  occtl::Curve aCurve;
  EXPECT_FALSE(static_cast<bool>(aCurve));
}

// ---------------------------------------------------------------------------
// Exception translation
// ---------------------------------------------------------------------------

TEST(VeneerExceptionTest, NegativeCircleRadius_Throws)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    EXPECT_THROW(occtl::Curve::from_circle(aGraph, {MakeXYPlaneAxis2(), -1.0}), occtl::Error);
  }
}

TEST(VeneerExceptionTest, NegativeCylinderRadius_Throws)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    EXPECT_THROW(occtl::Surface::from_cylinder(aGraph, {MakeStdAxis3(), -1.0}), occtl::Error);
  }
}

TEST(VeneerExceptionTest, AsLineOnCircle_ThrowsWrongKind)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aCurve = occtl::Curve::from_circle(aGraph, {MakeXYPlaneAxis2(), 1.0});
    try
    {
      (void)aCurve.as_line();
      FAIL() << "expected occtl::Error";
    }
    catch (const occtl::Error& anErr)
    {
      EXPECT_EQ(anErr.code(), OCCTL_WRONG_KIND);
    }
  }
}

TEST(VeneerExceptionTest, AsRevolutionOnPlane_ThrowsWrongKind)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Surface aS = occtl::Surface::from_plane(aGraph, {MakeStdAxis3()});
    try
    {
      (void)aS.as_revolution();
      FAIL() << "expected occtl::Error";
    }
    catch (const occtl::Error& anErr)
    {
      EXPECT_EQ(anErr.code(), OCCTL_WRONG_KIND);
    }
  }
}

TEST(VeneerExceptionTest, AsRectangularTrimmedOnSphere_ThrowsWrongKind)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Surface aS = occtl::Surface::from_sphere(aGraph, {MakeStdAxis3(), 2.0});
    try
    {
      (void)aS.as_rectangular_trimmed();
      FAIL() << "expected occtl::Error";
    }
    catch (const occtl::Error& anErr)
    {
      EXPECT_EQ(anErr.code(), OCCTL_WRONG_KIND);
    }
  }
}

// ---------------------------------------------------------------------------
// Round-trip equivalence
// ---------------------------------------------------------------------------

TEST(VeneerRoundTripTest, Circle_ReconstructEquivalent)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_geom_circle_t aOrig{MakeXYPlaneAxis2(), 3.5};
    occtl::Curve              aC1  = occtl::Curve::from_circle(aGraph, aOrig);
    const occtl_geom_circle_t aOut = aC1.as_circle();
    occtl::Curve              aC2  = occtl::Curve::from_circle(aGraph, aOut);
    EXPECT_EQ(aC2.kind(), OCCTL_CURVE_KIND_CIRCLE);
    EXPECT_EQ(aC2.is_periodic(), 1);
    EXPECT_NEAR(aC2.as_circle().radius, aOrig.radius, THE_EPS);
  }
}

TEST(VeneerRoundTripTest, Cylinder_ReconstructEquivalent)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_geom_cylindrical_surface_t aOrig{MakeStdAxis3(), 4.2};
    occtl::Surface                         aS1  = occtl::Surface::from_cylinder(aGraph, aOrig);
    const occtl_geom_cylindrical_surface_t aOut = aS1.as_cylinder();
    occtl::Surface                         aS2  = occtl::Surface::from_cylinder(aGraph, aOut);
    EXPECT_EQ(aS2.kind(), OCCTL_SURFACE_KIND_CYLINDRICAL);
    EXPECT_NEAR(aS2.as_cylinder().radius, aOrig.radius, THE_EPS);
  }
}

// ---------------------------------------------------------------------------
// Continuity
// ---------------------------------------------------------------------------

TEST(VeneerContinuityTest, Line_IsCN)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});
    EXPECT_EQ(aLine.continuity(), OCCTL_GEOM_CONTINUITY_CN);
  }
}

TEST(VeneerContinuityTest, Circle_IsCN)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aC = occtl::Curve::from_circle(aGraph, {MakeXYPlaneAxis2(), 1.0});
    EXPECT_EQ(aC.continuity(), OCCTL_GEOM_CONTINUITY_CN);
  }
}

TEST(VeneerContinuityTest, Plane_IsCN)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Surface aS = occtl::Surface::from_plane(aGraph, {MakeStdAxis3()});
    EXPECT_EQ(aS.continuity(), OCCTL_GEOM_CONTINUITY_CN);
  }
}

// ---------------------------------------------------------------------------
// Parameter ranges
// ---------------------------------------------------------------------------

TEST(VeneerParameterRangeTest, Line_SpansOcctInfinite)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});
    double       aU0 = 0.0, aU1 = 0.0;
    aLine.parameter_range(aU0, aU1);
    EXPECT_LE(aU0, -1.0e99);
    EXPECT_GE(aU1, 1.0e99);
  }
}

TEST(VeneerParameterRangeTest, Circle_SpansTwoPi)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aC  = occtl::Curve::from_circle(aGraph, {MakeXYPlaneAxis2(), 1.0});
    double       aU0 = 0.0, aU1 = 0.0;
    aC.parameter_range(aU0, aU1);
    EXPECT_NEAR(aU0, 0.0, THE_EPS);
    EXPECT_NEAR(aU1, 2.0 * M_PI, THE_EPS);
  }
}

// ---------------------------------------------------------------------------
// Trimmed curves
// ---------------------------------------------------------------------------

TEST(VeneerTrimmedTest, AsTrimmed_ReturnsBasisAndBounds)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});

    occtl_curve_trimmed_create_info_t aInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
    aInfo.basis                             = aLine.id();
    aInfo.u_first                           = 1.0;
    aInfo.u_last                            = 5.0;
    occtl::Curve aTrim                      = occtl::Curve::from_trimmed(aGraph, aInfo);
    ASSERT_EQ(aTrim.kind(), OCCTL_CURVE_KIND_TRIMMED);

    const occtl::Curve::TrimmedView aView = aTrim.as_trimmed();
    EXPECT_NEAR(aView.u_first, 1.0, THE_EPS);
    EXPECT_NEAR(aView.u_last, 5.0, THE_EPS);
  }
}

TEST(VeneerTrimmedTest, TrimmedOfTrimmed_RecursiveExtractWorks)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});

    occtl_curve_trimmed_create_info_t aInfoOuter = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
    aInfoOuter.basis                             = aLine.id();
    aInfoOuter.u_first                           = 0.0;
    aInfoOuter.u_last                            = 10.0;
    occtl::Curve aOuter                          = occtl::Curve::from_trimmed(aGraph, aInfoOuter);

    occtl_curve_trimmed_create_info_t aInfoInner = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
    aInfoInner.basis                             = aOuter.id();
    aInfoInner.u_first                           = 2.0;
    aInfoInner.u_last                            = 7.0;
    occtl::Curve aInner                          = occtl::Curve::from_trimmed(aGraph, aInfoInner);

    ASSERT_EQ(aInner.kind(), OCCTL_CURVE_KIND_TRIMMED);
    const occtl::Curve::TrimmedView aView = aInner.as_trimmed();
    EXPECT_NEAR(aView.u_first, 2.0, THE_EPS);
    EXPECT_NEAR(aView.u_last, 7.0, THE_EPS);
  }
}

// ---------------------------------------------------------------------------
// Offset curves — 3D and 2D offset
// ---------------------------------------------------------------------------

TEST(VeneerOffsetTest, Curve3D_AsOffset_ReturnsBasisAndOffset)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});

    occtl_curve_offset_create_info_t aInfo = OCCTL_CURVE_OFFSET_CREATE_INFO_INIT;
    aInfo.basis                            = aLine.id();
    aInfo.offset_dir                       = {0.0, 1.0, 0.0};
    aInfo.offset                           = 2.5;
    occtl::Curve aOff                      = occtl::Curve::from_offset(aGraph, aInfo);
    ASSERT_EQ(aOff.kind(), OCCTL_CURVE_KIND_OFFSET);

    const occtl::Curve::OffsetView aView = aOff.as_offset();
    EXPECT_NEAR(aView.offset, 2.5, THE_EPS);
    EXPECT_NEAR(aView.offset_dir.y, 1.0, THE_EPS);
  }
}

TEST(VeneerOffsetTest, Curve2D_AsOffset_ReturnsBasisAndOffset)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aLine = occtl::Curve2d::from_line(aGraph, {MakeStdAxis2d()});

    occtl_curve2d_offset_create_info_t aInfo = OCCTL_CURVE2D_OFFSET_CREATE_INFO_INIT;
    aInfo.basis                              = aLine.id();
    aInfo.offset                             = 3.0;
    occtl::Curve2d aOff                      = occtl::Curve2d::from_offset(aGraph, aInfo);
    ASSERT_EQ(aOff.kind(), OCCTL_CURVE_KIND_OFFSET);

    const occtl::Curve2d::OffsetView aView = aOff.as_offset();
    EXPECT_NEAR(aView.offset, 3.0, THE_EPS);
  }
}

// ---------------------------------------------------------------------------
// Surface decomposition
// ---------------------------------------------------------------------------

TEST(VeneerSurfaceTest, AsRevolution_ReturnsBasisAndAxis)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}});

    occtl_surface_revolution_create_info_t aInfo = OCCTL_SURFACE_REVOLUTION_CREATE_INFO_INIT;
    aInfo.basis                                  = aLine.id();
    aInfo.axis                                   = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    occtl::Surface aRev                          = occtl::Surface::from_revolution(aGraph, aInfo);
    ASSERT_EQ(aRev.kind(), OCCTL_SURFACE_KIND_REVOLUTION);

    const occtl::Surface::RevolutionView aView = aRev.as_revolution();
    EXPECT_NEAR(aView.axis.direction.z, 1.0, THE_EPS);
  }
}

TEST(VeneerSurfaceTest, AsExtrusion_ReturnsBasisAndDirection)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});

    occtl_surface_extrusion_create_info_t aInfo = OCCTL_SURFACE_EXTRUSION_CREATE_INFO_INIT;
    aInfo.basis                                 = aLine.id();
    aInfo.direction                             = {0.0, 0.0, 1.0};
    occtl::Surface aExt                         = occtl::Surface::from_extrusion(aGraph, aInfo);
    ASSERT_EQ(aExt.kind(), OCCTL_SURFACE_KIND_EXTRUSION);

    const occtl::Surface::ExtrusionView aView = aExt.as_extrusion();
    EXPECT_NEAR(aView.direction.z, 1.0, THE_EPS);
  }
}

TEST(VeneerSurfaceTest, AsRectangularTrimmed_ReturnsBasisAndBounds)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Surface aPlane = occtl::Surface::from_plane(aGraph, {MakeStdAxis3()});

    occtl_surface_rectangular_trimmed_create_info_t aInfo =
      OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_INIT;
    aInfo.basis          = aPlane.id();
    aInfo.u_first        = -1.0;
    aInfo.u_last         = 3.0;
    aInfo.v_first        = -2.0;
    aInfo.v_last         = 4.0;
    occtl::Surface aTrim = occtl::Surface::from_rectangular_trimmed(aGraph, aInfo);
    ASSERT_EQ(aTrim.kind(), OCCTL_SURFACE_KIND_RECTANGULAR_TRIMMED);

    const occtl::Surface::RectangularTrimmedView aView = aTrim.as_rectangular_trimmed();
    EXPECT_NEAR(aView.u_first, -1.0, THE_EPS);
    EXPECT_NEAR(aView.u_last, 3.0, THE_EPS);
    EXPECT_NEAR(aView.v_first, -2.0, THE_EPS);
    EXPECT_NEAR(aView.v_last, 4.0, THE_EPS);
  }
}

TEST(VeneerSurfaceTest, AsOffset_ReturnsBasisAndOffset)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Surface aPlane = occtl::Surface::from_plane(aGraph, {MakeStdAxis3()});

    occtl_surface_offset_create_info_t aInfo = OCCTL_SURFACE_OFFSET_CREATE_INFO_INIT;
    aInfo.basis                              = aPlane.id();
    aInfo.offset                             = 5.0;
    occtl::Surface aOff                      = occtl::Surface::from_offset(aGraph, aInfo);
    ASSERT_EQ(aOff.kind(), OCCTL_SURFACE_KIND_OFFSET);

    const occtl::Surface::OffsetView aView = aOff.as_offset();
    EXPECT_NEAR(aView.offset, 5.0, THE_EPS);
  }
}

// ---------------------------------------------------------------------------
// Gordon / CurveGrid / BoundaryCurves / PointGrid surface construction
// ---------------------------------------------------------------------------

TEST(VeneerSurfaceTest, FromGordon_TwoByTwoLineNetwork_ReturnsBSpline)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aProfile0 =
      MakeTrimmedLineCurve(aGraph, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
    occtl::Curve aProfile1 =
      MakeTrimmedLineCurve(aGraph, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
    occtl::Curve aGuide0 = MakeTrimmedLineCurve(aGraph, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
    occtl::Curve aGuide1 = MakeTrimmedLineCurve(aGraph, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);

    std::vector<const occtl::Curve*> aProfiles = {&aProfile0, &aProfile1};
    std::vector<const occtl::Curve*> aGuides   = {&aGuide0, &aGuide1};

    occtl::Surface aSurface = occtl::Surface::from_gordon(aGraph, aProfiles, aGuides);
    EXPECT_EQ(aSurface.kind(), OCCTL_SURFACE_KIND_BSPLINE);
  }
}

TEST(VeneerSurfaceTest, FromCurveGrid_TwoByTwoLineNetwork_ReturnsBSpline)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aU0 = MakeTrimmedLineCurve(aGraph, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
    occtl::Curve aU1 = MakeTrimmedLineCurve(aGraph, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
    occtl::Curve aV0 = MakeTrimmedLineCurve(aGraph, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
    occtl::Curve aV1 = MakeTrimmedLineCurve(aGraph, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);

    std::vector<const occtl::Curve*> aUCurves = {&aU0, &aU1};
    std::vector<const occtl::Curve*> aVCurves = {&aV0, &aV1};

    occtl::Surface aSurface = occtl::Surface::from_curve_grid(aGraph, aUCurves, aVCurves);
    EXPECT_EQ(aSurface.kind(), OCCTL_SURFACE_KIND_BSPLINE);
  }
}

TEST(VeneerSurfaceTest, FromBoundaryCurves_FourLines_ReturnsBSpline)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aBottom = MakeTrimmedLineCurve(aGraph, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
    occtl::Curve aRight  = MakeTrimmedLineCurve(aGraph, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
    occtl::Curve aTop  = MakeTrimmedLineCurve(aGraph, {1.0, 1.0, 0.0}, {-1.0, 0.0, 0.0}, 0.0, 1.0);
    occtl::Curve aLeft = MakeTrimmedLineCurve(aGraph, {0.0, 1.0, 0.0}, {0.0, -1.0, 0.0}, 0.0, 1.0);

    std::vector<const occtl::Curve*> aCurves = {&aBottom, &aRight, &aTop, &aLeft};
    occtl::Surface aSurface = occtl::Surface::from_boundary_curves(aGraph, aCurves);
    EXPECT_EQ(aSurface.kind(), OCCTL_SURFACE_KIND_BSPLINE);
  }
}

TEST(VeneerSurfaceTest, FromPointGrid_ThreeByThreeGrid_ReturnsBSpline)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t aPoints[9] = {{0.0, 0.0, 0.0},
                                       {0.0, 1.0, 0.1},
                                       {0.0, 2.0, 0.0},
                                       {1.0, 0.0, 0.2},
                                       {1.0, 1.0, 0.6},
                                       {1.0, 2.0, 0.2},
                                       {2.0, 0.0, 0.0},
                                       {2.0, 1.0, 0.1},
                                       {2.0, 2.0, 0.0}};

    occtl_surface_point_grid_create_info_t aInfo = OCCTL_SURFACE_POINT_GRID_CREATE_INFO_INIT;
    aInfo.points                                 = aPoints;
    aInfo.u_point_count                          = 3;
    aInfo.v_point_count                          = 3;

    occtl::Surface aSurface = occtl::Surface::from_point_grid(aGraph, aInfo);
    EXPECT_EQ(aSurface.kind(), OCCTL_SURFACE_KIND_BSPLINE);
  }
}

// ---------------------------------------------------------------------------
// Bezier
// ---------------------------------------------------------------------------

TEST(VeneerBezierTest, Bezier_Degree_ReturnsPoleCountMinusOne)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t             aPoles[3] = {{0, 0, 0}, {1, 1, 0}, {2, 0, 0}};
    occtl_curve_bezier_create_info_t aInfo     = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
    aInfo.poles                                = aPoles;
    aInfo.pole_count                           = 3;
    occtl::Curve aBz                           = occtl::Curve::from_bezier(aGraph, aInfo);
    EXPECT_EQ(aBz.kind(), OCCTL_CURVE_KIND_BEZIER);
    EXPECT_EQ(aBz.bezier_degree(), 2);
    EXPECT_EQ(aBz.bezier_pole_count(), 3u);
    EXPECT_EQ(aBz.bezier_is_rational(), 0);
    EXPECT_THROW((void)aBz.bspline_degree(), occtl::Error);
  }
}

TEST(VeneerBezierSurfaceTest, FromBezierGrid_ReturnsBezierSurface)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t aPoles[4] = {{0.0, 0.0, 0.0},
                                      {0.0, 1.0, 0.0},
                                      {1.0, 0.0, 0.2},
                                      {1.0, 1.0, 0.0}};

    occtl_surface_bezier_create_info_t aInfo = OCCTL_SURFACE_BEZIER_CREATE_INFO_INIT;
    aInfo.poles                              = aPoles;
    aInfo.u_pole_count                       = 2;
    aInfo.v_pole_count                       = 2;

    occtl::Surface aSurface = occtl::Surface::from_bezier_grid(aGraph, aInfo);
    EXPECT_EQ(aSurface.kind(), OCCTL_SURFACE_KIND_BEZIER);
  }
}

// ---------------------------------------------------------------------------
// Flat knots — B-spline degree-1 polyline
// ---------------------------------------------------------------------------

TEST(VeneerFlatKnotsTest, Curve_DegreeOnePolyline_ExpandsKnots)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t              aPoles[3] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};
    const double                      aKnots[3] = {0.0, 1.0, 2.0};
    const int32_t                     aMults[3] = {2, 1, 2};
    occtl_curve_bspline_create_info_t aInfo     = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                 = aPoles;
    aInfo.pole_count                            = 3;
    aInfo.knots                                 = aKnots;
    aInfo.multiplicities                        = aMults;
    aInfo.knot_count                            = 3;
    aInfo.degree                                = 1;
    occtl::Curve aBs                            = occtl::Curve::from_bspline(aGraph, aInfo);

    std::vector<double> aFlat = aBs.bspline_flat_knots();
    ASSERT_EQ(aFlat.size(), 5u);
    EXPECT_NEAR(aFlat[0], 0.0, THE_EPS);
    EXPECT_NEAR(aFlat[1], 0.0, THE_EPS);
    EXPECT_NEAR(aFlat[2], 1.0, THE_EPS);
    EXPECT_NEAR(aFlat[3], 2.0, THE_EPS);
    EXPECT_NEAR(aFlat[4], 2.0, THE_EPS);
  }
}

// ---------------------------------------------------------------------------
// NACA 4-digit airfoil factory
// ---------------------------------------------------------------------------

TEST(VeneerCurveFactoryTest, AirfoilNaca4_ReturnsBSpline)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl_curve_airfoil_naca4_info_t aInfo = OCCTL_CURVE_AIRFOIL_NACA4_INFO_INIT;
    aInfo.max_camber                       = 0.02;
    aInfo.camber_position                  = 0.4;
    aInfo.thickness                        = 0.12;
    aInfo.point_count                      = 40;

    occtl::Curve anAirfoil = occtl::Curve::from_airfoil_naca4(aGraph, aInfo);
    EXPECT_EQ(anAirfoil.kind(), OCCTL_CURVE_KIND_BSPLINE);
    EXPECT_GT(anAirfoil.bspline_pole_count(), 0u);
  }
}

// ---------------------------------------------------------------------------
// ToBezierSegments (Curve2d only; Curve3D removed in new API)
// ---------------------------------------------------------------------------

TEST(VeneerCurveAlgoTest, Curve2dToBezierSegments_LineWithRange_ReturnsBezier)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aLine = occtl::Curve2d::from_line(aGraph, {MakeStdAxis2d()});

    occtl_curve_bezier_segments_options_t aOptions = OCCTL_CURVE_BEZIER_SEGMENTS_OPTIONS_INIT;
    aOptions.use_range                             = 1;
    aOptions.u_first                               = 0.0;
    aOptions.u_last                                = 3.0;

    std::vector<occtl::Curve2d> aSegments = aLine.to_bezier_segments(&aOptions);
    ASSERT_EQ(aSegments.size(), 1u);
    EXPECT_EQ(aSegments[0].kind(), OCCTL_CURVE_KIND_BEZIER);
    EXPECT_EQ(aSegments[0].bezier_degree(), 1);
  }
}

// ---------------------------------------------------------------------------
// BSplineView — zero-copy span accessors
// ---------------------------------------------------------------------------

TEST(VeneerPolesViewTest, Curve_ZeroCopy_MatchesCopiedPoles)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t              aPoles[4] = {{0, 0, 0}, {1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    const double                      aKnots[2] = {0.0, 1.0};
    const int32_t                     aMults[2] = {4, 4};
    occtl_curve_bspline_create_info_t aInfo     = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                 = aPoles;
    aInfo.pole_count                            = 4;
    aInfo.knots                                 = aKnots;
    aInfo.multiplicities                        = aMults;
    aInfo.knot_count                            = 2;
    aInfo.degree                                = 3;
    occtl::Curve aBs                            = occtl::Curve::from_bspline(aGraph, aInfo);

    size_t                aN    = 0;
    const occtl_point3_t* aView = aBs.bspline_poles_view(aN);
    ASSERT_EQ(aN, 4u);
    ASSERT_NE(aView, nullptr);
    for (size_t anI = 0; anI < aN; ++anI)
    {
      EXPECT_NEAR(aView[anI].x, aPoles[anI].x, THE_EPS);
      EXPECT_NEAR(aView[anI].y, aPoles[anI].y, THE_EPS);
      EXPECT_NEAR(aView[anI].z, aPoles[anI].z, THE_EPS);
    }
  }
}

TEST(VeneerPolesViewTest, Curve2D_ZeroCopy_MatchesCopiedPoles)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point2_t                aPoles[3] = {{0, 0}, {1, 2}, {3, 4}};
    const double                        aKnots[2] = {0.0, 1.0};
    const int32_t                       aMults[2] = {3, 3};
    occtl_curve2d_bspline_create_info_t aInfo     = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                   = aPoles;
    aInfo.pole_count                              = 3;
    aInfo.knots                                   = aKnots;
    aInfo.multiplicities                          = aMults;
    aInfo.knot_count                              = 2;
    aInfo.degree                                  = 2;
    occtl::Curve2d aBs                            = occtl::Curve2d::from_bspline(aGraph, aInfo);

    size_t                aN    = 0;
    const occtl_point2_t* aView = aBs.bspline_poles_view(aN);
    ASSERT_EQ(aN, 3u);
    for (size_t anI = 0; anI < aN; ++anI)
    {
      EXPECT_NEAR(aView[anI].x, aPoles[anI].x, THE_EPS);
      EXPECT_NEAR(aView[anI].y, aPoles[anI].y, THE_EPS);
    }
  }
}

TEST(VeneerPeriodicTest, BsplineCurve_PeriodicReportsTrue)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t              aPoles[3] = {{1, 0, 0}, {0, 1, 0}, {-1, 0, 0}};
    const double                      aKnots[4] = {0.0, 1.0, 2.0, 3.0};
    const int32_t                     aMults[4] = {1, 1, 1, 1};
    occtl_curve_bspline_create_info_t aInfo     = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                 = aPoles;
    aInfo.pole_count                            = 3;
    aInfo.knots                                 = aKnots;
    aInfo.multiplicities                        = aMults;
    aInfo.knot_count                            = 4;
    aInfo.degree                                = 2;
    aInfo.is_periodic                           = 1;
    occtl::Curve aBs                            = occtl::Curve::from_bspline(aGraph, aInfo);
    EXPECT_EQ(aBs.is_periodic(), 1);
  }
}

// ---------------------------------------------------------------------------
// Rational surface
// ---------------------------------------------------------------------------

TEST(VeneerRationalSurfaceTest, BsplineSurface_RationalReportsAndExtractsWeights)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t                aPoles[9]   = {{0, 0, 0},
                                                       {1, 0, 0},
                                                       {2, 0, 0},
                                                       {0, 1, 0},
                                                       {1, 1, 1},
                                                       {2, 1, 0},
                                                       {0, 2, 0},
                                                       {1, 2, 0},
                                                       {2, 2, 0}};
    const double                        aWeights[9] = {1, 1, 1, 1, 2, 1, 1, 1, 1};
    const double                        aUKnots[2]  = {0.0, 1.0};
    const int32_t                       aUMults[2]  = {3, 3};
    const double                        aVKnots[2]  = {0.0, 1.0};
    const int32_t                       aVMults[2]  = {3, 3};
    occtl_surface_bspline_create_info_t aInfo       = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                     = aPoles;
    aInfo.u_pole_count                              = 3;
    aInfo.v_pole_count                              = 3;
    aInfo.weights                                   = aWeights;
    aInfo.u_knots                                   = aUKnots;
    aInfo.u_multiplicities                          = aUMults;
    aInfo.u_knot_count                              = 2;
    aInfo.v_knots                                   = aVKnots;
    aInfo.v_multiplicities                          = aVMults;
    aInfo.v_knot_count                              = 2;
    aInfo.u_degree                                  = 2;
    aInfo.v_degree                                  = 2;
    occtl::Surface aS                               = occtl::Surface::from_bspline(aGraph, aInfo);

    EXPECT_EQ(aS.bspline_is_rational(), 1);
    std::vector<double> aOut = aS.bspline_weights();
    ASSERT_EQ(aOut.size(), 9u);
    EXPECT_NEAR(aOut[4], 2.0, THE_EPS);
    for (size_t anI : {0u, 1u, 2u, 3u, 5u, 6u, 7u, 8u})
    {
      EXPECT_NEAR(aOut[anI], 1.0, THE_EPS);
    }
  }
}

TEST(VeneerSurfacePolesViewTest, Surface_ZeroCopy_MatchesCopiedPoles)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t                aPoles[4]  = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}};
    const double                        aUKnots[2] = {0.0, 1.0};
    const int32_t                       aUMults[2] = {2, 2};
    const double                        aVKnots[2] = {0.0, 1.0};
    const int32_t                       aVMults[2] = {2, 2};
    occtl_surface_bspline_create_info_t aInfo      = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                    = aPoles;
    aInfo.u_pole_count                             = 2;
    aInfo.v_pole_count                             = 2;
    aInfo.u_knots                                  = aUKnots;
    aInfo.u_multiplicities                         = aUMults;
    aInfo.u_knot_count                             = 2;
    aInfo.v_knots                                  = aVKnots;
    aInfo.v_multiplicities                         = aVMults;
    aInfo.v_knot_count                             = 2;
    aInfo.u_degree                                 = 1;
    aInfo.v_degree                                 = 1;
    occtl::Surface aS                              = occtl::Surface::from_bspline(aGraph, aInfo);

    size_t                aNbU = 0, aNbV = 0;
    const occtl_point3_t* aView = aS.bspline_poles_view(aNbU, aNbV);
    ASSERT_EQ(aNbU, 2u);
    ASSERT_EQ(aNbV, 2u);
    ASSERT_NE(aView, nullptr);
    for (size_t aU = 0; aU < aNbU; ++aU)
    {
      for (size_t aV = 0; aV < aNbV; ++aV)
      {
        const occtl_point3_t& aP = aView[aU * aNbV + aV];
        const occtl_point3_t& aQ = aPoles[aU * aNbV + aV];
        EXPECT_NEAR(aP.x, aQ.x, THE_EPS);
        EXPECT_NEAR(aP.y, aQ.y, THE_EPS);
        EXPECT_NEAR(aP.z, aQ.z, THE_EPS);
      }
    }

    std::vector<double> aFU = aS.bspline_u_flat_knots();
    std::vector<double> aFV = aS.bspline_v_flat_knots();
    ASSERT_EQ(aFU.size(), 4u);
    ASSERT_EQ(aFV.size(), 4u);
    EXPECT_NEAR(aFU[0], 0.0, THE_EPS);
    EXPECT_NEAR(aFU[1], 0.0, THE_EPS);
    EXPECT_NEAR(aFU[2], 1.0, THE_EPS);
    EXPECT_NEAR(aFU[3], 1.0, THE_EPS);
  }
}

// ---------------------------------------------------------------------------
// Curve2d solvers
// ---------------------------------------------------------------------------

TEST(VeneerCurve2dSolverTest, TangentCircleToTwoLines_ReturnsAllCandidateCircles)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aLineX = occtl::Curve2d::from_line(aGraph, {MakeStdAxis2d()});
    occtl::Curve2d aLineY = occtl::Curve2d::from_line(aGraph, {{{0.0, 0.0}, {0.0, 1.0}}});

    occtl_curve2d_circle_tangent_to_two_radius_info_t aInfo =
      OCCTL_CURVE2D_CIRCLE_TANGENT_TO_TWO_RADIUS_INFO_INIT;
    aInfo.curve_a = aLineX.id();
    aInfo.curve_b = aLineY.id();
    aInfo.radius  = 2.0;

    const std::vector<occtl_geom2d_circle_t> aCircles =
      occtl::Curve2d::circles_tangent_to_two_radius(aGraph, aInfo);
    ASSERT_GT(aCircles.size(), 0u);
    for (const occtl_geom2d_circle_t& aCircle : aCircles)
    {
      EXPECT_NEAR(aCircle.radius, 2.0, 1.0e-9);
    }
  }
}

TEST(VeneerCurve2dSolverTest, BlendArcs_ReturnsTrimmedCurves)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aLineX = occtl::Curve2d::from_line(aGraph, {MakeStdAxis2d()});
    occtl::Curve2d aLineY = occtl::Curve2d::from_line(aGraph, {{{0.0, 0.0}, {0.0, 1.0}}});

    occtl_curve2d_blend_arc_info_t aInfo = OCCTL_CURVE2D_BLEND_ARC_INFO_INIT;
    aInfo.curve_a                        = aLineX.id();
    aInfo.curve_b                        = aLineY.id();
    aInfo.radius                         = 2.0;

    std::vector<occtl::Curve2d> anArcs = occtl::Curve2d::blend_arcs(aGraph, aInfo);
    ASSERT_GT(anArcs.size(), 0u);
    EXPECT_EQ(anArcs.front().kind(), OCCTL_CURVE_KIND_TRIMMED);

    const occtl::Curve2d::TrimmedView aView = anArcs.front().as_trimmed();
    EXPECT_GT(aView.u_last, aView.u_first);
  }
}

TEST(VeneerCurve2dSolverTest, TangentCircleToThreeCircles_ReturnsCandidates)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aCircleA = occtl::Curve2d::from_circle(aGraph, {MakeStdAxis2d(), 2.0});
    occtl::Curve2d aCircleB = occtl::Curve2d::from_circle(aGraph, {{{10.0, 0.0}, {1.0, 0.0}}, 2.0});
    occtl::Curve2d aCircleC = occtl::Curve2d::from_circle(aGraph, {{{5.0, 8.0}, {1.0, 0.0}}, 2.0});

    occtl_curve2d_circle_tangent_to_three_info_t aInfo =
      OCCTL_CURVE2D_CIRCLE_TANGENT_TO_THREE_INFO_INIT;
    aInfo.curve_a = aCircleA.id();
    aInfo.curve_b = aCircleB.id();
    aInfo.curve_c = aCircleC.id();

    const std::vector<occtl_geom2d_circle_t> aCircles =
      occtl::Curve2d::circles_tangent_to_three(aGraph, aInfo);
    ASSERT_GT(aCircles.size(), 0u);
    EXPECT_GT(aCircles.front().radius, 0.0);
  }
}

TEST(VeneerCurve2dSolverTest, TangentCircleFixedCenter_ReturnsCandidates)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aCircle = occtl::Curve2d::from_circle(aGraph, {MakeStdAxis2d(), 1.0});

    occtl_curve2d_circle_tangent_fixed_center_info_t aInfo =
      OCCTL_CURVE2D_CIRCLE_TANGENT_FIXED_CENTER_INFO_INIT;
    aInfo.curve  = aCircle.id();
    aInfo.center = {3.0, 0.0};

    const std::vector<occtl_geom2d_circle_t> aCircles =
      occtl::Curve2d::circles_tangent_fixed_center(aGraph, aInfo);
    ASSERT_GT(aCircles.size(), 0u);
    EXPECT_NEAR(aCircles.front().position.location.x, 3.0, 1.0e-12);
    EXPECT_GT(aCircles.front().radius, 0.0);
  }
}

TEST(VeneerCurve2dSolverTest, TangentCircleCenterOnCurve_ReturnsCandidates)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aLineA      = occtl::Curve2d::from_line(aGraph, {MakeStdAxis2d()});
    occtl::Curve2d aLineB      = occtl::Curve2d::from_line(aGraph, {{{0.0, 4.0}, {1.0, 0.0}}});
    occtl::Curve2d aCenterLine = occtl::Curve2d::from_line(aGraph, {{{0.0, 0.0}, {0.0, 1.0}}});

    occtl_curve2d_circle_tangent_center_on_curve_info_t aInfo =
      OCCTL_CURVE2D_CIRCLE_TANGENT_CENTER_ON_CURVE_INFO_INIT;
    aInfo.curve_a      = aLineA.id();
    aInfo.curve_b      = aLineB.id();
    aInfo.center_curve = aCenterLine.id();

    const std::vector<occtl_geom2d_circle_t> aCircles =
      occtl::Curve2d::circles_tangent_center_on_curve(aGraph, &aInfo);
    ASSERT_GT(aCircles.size(), 0u);
    EXPECT_NEAR(aCircles.front().position.location.x, 0.0, 1.0e-9);
    EXPECT_GT(aCircles.front().radius, 0.0);
  }
}

TEST(VeneerCurve2dSolverTest, TangentCircleOnCurveRadius_ReturnsCandidates)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aLine       = occtl::Curve2d::from_line(aGraph, {MakeStdAxis2d()});
    occtl::Curve2d aCenterLine = occtl::Curve2d::from_line(aGraph, {{{0.0, 0.0}, {0.0, 1.0}}});

    occtl_curve2d_circle_tangent_on_curve_radius_info_t aInfo =
      OCCTL_CURVE2D_CIRCLE_TANGENT_ON_CURVE_RADIUS_INFO_INIT;
    aInfo.curve        = aLine.id();
    aInfo.center_curve = aCenterLine.id();
    aInfo.radius       = 2.0;

    const std::vector<occtl_geom2d_circle_t> aCircles =
      occtl::Curve2d::circles_tangent_on_curve_radius(aGraph, &aInfo);
    ASSERT_GT(aCircles.size(), 0u);
    EXPECT_NEAR(aCircles.front().position.location.x, 0.0, 1.0e-9);
    EXPECT_NEAR(aCircles.front().radius, 2.0, 1.0e-12);
  }
}

TEST(VeneerCurve2dSolverTest, TangentLinesToTwoCircles_ReturnsCandidates)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aCircleA = occtl::Curve2d::from_circle(aGraph, {MakeStdAxis2d(), 1.0});
    occtl::Curve2d aCircleB = occtl::Curve2d::from_circle(aGraph, {{{4.0, 0.0}, {1.0, 0.0}}, 1.0});

    occtl_curve2d_line_tangent_to_two_info_t aInfo = OCCTL_CURVE2D_LINE_TANGENT_TO_TWO_INFO_INIT;
    aInfo.curve_a                                  = aCircleA.id();
    aInfo.curve_b                                  = aCircleB.id();

    const std::vector<occtl_geom2d_line_t> aLines =
      occtl::Curve2d::lines_tangent_to_two(aGraph, aInfo);
    ASSERT_GT(aLines.size(), 0u);
    const double aDirLen =
      std::hypot(aLines.front().position.x_dir.x, aLines.front().position.x_dir.y);
    EXPECT_NEAR(aDirLen, 1.0, 1.0e-12);
  }
}

TEST(VeneerCurve2dSolverTest, TangentLinesThroughPoint_ReturnsCandidates)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aCircle = occtl::Curve2d::from_circle(aGraph, {MakeStdAxis2d(), 1.0});

    occtl_curve2d_line_tangent_through_point_info_t aInfo =
      OCCTL_CURVE2D_LINE_TANGENT_THROUGH_POINT_INFO_INIT;
    aInfo.curve = aCircle.id();
    aInfo.point = {3.0, 0.0};

    const std::vector<occtl_geom2d_line_t> aLines =
      occtl::Curve2d::lines_tangent_through_point(aGraph, aInfo);
    ASSERT_GT(aLines.size(), 0u);
    const double aDirLen =
      std::hypot(aLines.front().position.x_dir.x, aLines.front().position.x_dir.y);
    EXPECT_NEAR(aDirLen, 1.0, 1.0e-12);
  }
}

TEST(VeneerCurve2dSolverTest, TangentLinesWithAngle_ReturnsCandidates)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve2d aCircle = occtl::Curve2d::from_circle(aGraph, {MakeStdAxis2d(), 1.0});

    occtl_curve2d_line_tangent_with_angle_info_t aInfo =
      OCCTL_CURVE2D_LINE_TANGENT_WITH_ANGLE_INFO_INIT;
    aInfo.curve = aCircle.id();

    const std::vector<occtl_geom2d_line_t> aLines =
      occtl::Curve2d::lines_tangent_with_angle(aGraph, &aInfo);
    ASSERT_GT(aLines.size(), 0u);
    const double aDirLen =
      std::hypot(aLines.front().position.x_dir.x, aLines.front().position.x_dir.y);
    EXPECT_NEAR(aDirLen, 1.0, 1.0e-12);
  }
}

// ---------------------------------------------------------------------------
// Exception translation — wrong kind on accessors
// ---------------------------------------------------------------------------

TEST(VeneerExceptionTest, NonRationalSurfaceWeights_ThrowsWrongKind)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t                aPoles[4]  = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}};
    const double                        aUKnots[2] = {0.0, 1.0};
    const int32_t                       aUMults[2] = {2, 2};
    const double                        aVKnots[2] = {0.0, 1.0};
    const int32_t                       aVMults[2] = {2, 2};
    occtl_surface_bspline_create_info_t aInfo      = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                    = aPoles;
    aInfo.u_pole_count                             = 2;
    aInfo.v_pole_count                             = 2;
    aInfo.u_knots                                  = aUKnots;
    aInfo.u_multiplicities                         = aUMults;
    aInfo.u_knot_count                             = 2;
    aInfo.v_knots                                  = aVKnots;
    aInfo.v_multiplicities                         = aVMults;
    aInfo.v_knot_count                             = 2;
    aInfo.u_degree                                 = 1;
    aInfo.v_degree                                 = 1;
    occtl::Surface aS                              = occtl::Surface::from_bspline(aGraph, aInfo);

    EXPECT_EQ(aS.bspline_is_rational(), 0);
    try
    {
      (void)aS.bspline_weights();
      FAIL() << "expected occtl::Error";
    }
    catch (const occtl::Error& anErr)
    {
      EXPECT_EQ(anErr.code(), OCCTL_WRONG_KIND);
    }
  }
}

// ---------------------------------------------------------------------------
// Round-trip for revolution and extrusion
// ---------------------------------------------------------------------------

TEST(VeneerRoundTripTest, Revolution_ReconstructEquivalent)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}});

    occtl_surface_revolution_create_info_t aInfo = OCCTL_SURFACE_REVOLUTION_CREATE_INFO_INIT;
    aInfo.basis                                  = aLine.id();
    aInfo.axis                                   = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    occtl::Surface aS1                           = occtl::Surface::from_revolution(aGraph, aInfo);

    const occtl::Surface::RevolutionView aView = aS1.as_revolution();

    occtl_surface_revolution_create_info_t aInfo2 = OCCTL_SURFACE_REVOLUTION_CREATE_INFO_INIT;
    aInfo2.axis                                   = aView.axis;
    // Basis cannot be round-tripped through the view in the new API; create
    // a fresh line for verification that the surface is still a revolution.
    occtl::Curve aLine2 = occtl::Curve::from_line(aGraph, {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}});
    aInfo2.basis        = aLine2.id();
    occtl::Surface aS2  = occtl::Surface::from_revolution(aGraph, aInfo2);

    EXPECT_EQ(aS2.kind(), OCCTL_SURFACE_KIND_REVOLUTION);
    const occtl::Surface::RevolutionView aView2 = aS2.as_revolution();
    EXPECT_NEAR(aView2.axis.direction.x, aInfo.axis.direction.x, THE_EPS);
    EXPECT_NEAR(aView2.axis.direction.y, aInfo.axis.direction.y, THE_EPS);
    EXPECT_NEAR(aView2.axis.direction.z, aInfo.axis.direction.z, THE_EPS);
  }
}

TEST(VeneerRoundTripTest, Extrusion_ReconstructEquivalent)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});

    occtl_surface_extrusion_create_info_t aInfo = OCCTL_SURFACE_EXTRUSION_CREATE_INFO_INIT;
    aInfo.basis                                 = aLine.id();
    aInfo.direction                             = {0.0, 0.0, 1.0};
    occtl::Surface aS1                          = occtl::Surface::from_extrusion(aGraph, aInfo);

    const occtl::Surface::ExtrusionView aView = aS1.as_extrusion();

    // Basis cannot be round-tripped through view; create fresh line.
    occtl::Curve aLine2 = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});
    occtl_surface_extrusion_create_info_t aInfo2 = OCCTL_SURFACE_EXTRUSION_CREATE_INFO_INIT;
    aInfo2.basis                                 = aLine2.id();
    aInfo2.direction                             = aView.direction;
    occtl::Surface aS2                           = occtl::Surface::from_extrusion(aGraph, aInfo2);

    EXPECT_EQ(aS2.kind(), OCCTL_SURFACE_KIND_EXTRUSION);
    const occtl::Surface::ExtrusionView aView2 = aS2.as_extrusion();
    EXPECT_NEAR(aView2.direction.x, aInfo.direction.x, THE_EPS);
    EXPECT_NEAR(aView2.direction.y, aInfo.direction.y, THE_EPS);
    EXPECT_NEAR(aView2.direction.z, aInfo.direction.z, THE_EPS);
  }
}

// ---------------------------------------------------------------------------
// Evaluation — d0, d1, d2 for curve, curve2d, surface
// ---------------------------------------------------------------------------

TEST(VeneerEvalTest, Curve_D0_Line_ReturnsPointOnLine)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const auto aCurve = occtl::Curve::from_line(aGraph, {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}});
    const auto aP     = aCurve.eval_d0(3.0);
    EXPECT_NEAR(aP.x, 1.0, 1e-14);
    EXPECT_NEAR(aP.y, 3.0, 1e-14);
    EXPECT_NEAR(aP.z, 0.0, 1e-14);
  }
}

TEST(VeneerEvalTest, Curve_D1_Circle_ReturnsOrthogonalDerivatives)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const auto aCurve =
      occtl::Curve::from_circle(aGraph, {{{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}}, 3.0});
    const auto [aP, aD1] = aCurve.eval_d1(0.0);
    EXPECT_NEAR(aP.x, 3.0, 1e-14);
    EXPECT_NEAR(aD1.x, 0.0, 1e-14);
    EXPECT_NEAR(aD1.y, 3.0, 1e-14);
  }
}

TEST(VeneerEvalTest, Curve2d_D0_Circle_AtZeroIsOnXAxis)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const auto aCurve = occtl::Curve2d::from_circle(aGraph, {{{0.0, 0.0}, {1.0, 0.0}}, 2.0});
    const auto aP     = aCurve.eval_d0(0.0);
    EXPECT_NEAR(aP.x, 2.0, 1e-14);
    EXPECT_NEAR(aP.y, 0.0, 1e-14);
  }
}

TEST(VeneerEvalTest, Surface_D0_Plane_ReturnsPoint)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl_axis3_placement_t aAx   = {{0.0, 0.0, 1.0},
                                     {1.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.0},
                                     {0.0, 0.0, 1.0}};
    const auto              aSurf = occtl::Surface::from_plane(aGraph, {aAx});
    const auto              aP    = aSurf.eval_d0(2.0, 3.0);
    EXPECT_NEAR(aP.x, 2.0, 1e-14);
    EXPECT_NEAR(aP.y, 3.0, 1e-14);
    EXPECT_NEAR(aP.z, 1.0, 1e-14);
  }
}

TEST(VeneerEvalTest, Surface_D1_Plane_PartialsAreAxes)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    occtl_axis3_placement_t aAx   = {{0.0, 0.0, 0.0},
                                     {1.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.0},
                                     {0.0, 0.0, 1.0}};
    const auto              aSurf = occtl::Surface::from_plane(aGraph, {aAx});
    const auto [aP, aD1U, aD1V]   = aSurf.eval_d1(0.0, 0.0);
    EXPECT_NEAR(aD1U.x, 1.0, 1e-14);
    EXPECT_NEAR(aD1U.y, 0.0, 1e-14);
    EXPECT_NEAR(aD1V.x, 0.0, 1e-14);
    EXPECT_NEAR(aD1V.y, 1.0, 1e-14);
  }
}

TEST(VeneerEvalTest, Curve_D2_D3_DN_CheckConsistency)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const auto aCurve       = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});
    const auto [aP1, aD1_1] = aCurve.eval_d1(0.0);
    const auto [aP2, aD1_2, aD2] = aCurve.eval_d2(0.0);
    const auto aDN               = aCurve.eval_dn(0.0, 2);
    EXPECT_NEAR(aD1_1.x, aD1_2.x, 1e-14);
    EXPECT_NEAR(aD2.x, 0.0, 1e-14);
    EXPECT_NEAR(aD2.x, aDN.x, 1e-14);
  }
}

// ---------------------------------------------------------------------------
// BSplineView — span accessors for rational BSpline curves
// ---------------------------------------------------------------------------

TEST(VeneerBSplineViewTest, RationalBSpline_ScalarsAndSpansMatchRaw)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t              aPoles[4]   = {{0, 0, 0}, {1, 2, 0}, {2, 2, 0}, {3, 0, 0}};
    const double                      aWeights[4] = {1.0, 2.0, 0.5, 1.5};
    const double                      aKnots[2]   = {0.0, 1.0};
    const int32_t                     aMults[2]   = {4, 4};
    occtl_curve_bspline_create_info_t aInfo       = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                   = aPoles;
    aInfo.pole_count                              = 4;
    aInfo.weights                                 = aWeights;
    aInfo.knots                                   = aKnots;
    aInfo.multiplicities                          = aMults;
    aInfo.knot_count                              = 2;
    aInfo.degree                                  = 3;
    const occtl::Curve aBs                        = occtl::Curve::from_bspline(aGraph, aInfo);

    const occtl::Curve::BSplineView aView = aBs.as_bspline();
    EXPECT_EQ(aView.degree(), 3);
    EXPECT_TRUE(aView.is_rational());
    EXPECT_EQ(aView.pole_count(), 4u);
    EXPECT_EQ(aView.knot_count(), 2u);

#if OCCTL_HPP_HAS_SPAN
    EXPECT_EQ(aView.poles().size(), aView.raw.pole_count);
    EXPECT_EQ(aView.weights().size(), aView.raw.pole_count);
    EXPECT_EQ(aView.knots().size(), aView.raw.knot_count);
    EXPECT_EQ(aView.multiplicities().size(), aView.raw.knot_count);
    EXPECT_EQ(aView.flat_knots().size(), aView.raw.flat_knot_count);
    for (size_t anI = 0; anI < aView.poles().size(); ++anI)
    {
      EXPECT_NEAR(aView.poles()[anI].x, aPoles[anI].x, THE_EPS);
      EXPECT_NEAR(aView.weights()[anI], aWeights[anI], THE_EPS);
    }
#else
    EXPECT_EQ(aView.poles(), aView.raw.poles);
    EXPECT_EQ(aView.weights(), aView.raw.weights);
#endif
  }
}

TEST(VeneerBSplineViewTest, NonRational_WeightsSpanIsEmpty)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point3_t              aPoles[4] = {{0, 0, 0}, {1, 2, 0}, {2, 2, 0}, {3, 0, 0}};
    const double                      aKnots[2] = {0.0, 1.0};
    const int32_t                     aMults[2] = {4, 4};
    occtl_curve_bspline_create_info_t aInfo     = OCCTL_CURVE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                 = aPoles;
    aInfo.pole_count                            = 4;
    aInfo.knots                                 = aKnots;
    aInfo.multiplicities                        = aMults;
    aInfo.knot_count                            = 2;
    aInfo.degree                                = 3;
    const occtl::Curve aBs                      = occtl::Curve::from_bspline(aGraph, aInfo);

    const occtl::Curve::BSplineView aView = aBs.as_bspline();
    EXPECT_FALSE(aView.is_rational());
    EXPECT_EQ(aView.raw.weights, nullptr);
#if OCCTL_HPP_HAS_SPAN
    EXPECT_TRUE(aView.weights().empty());
    EXPECT_EQ(aView.poles().size(), 4u);
#else
    EXPECT_EQ(aView.weights(), nullptr);
#endif
  }
}

TEST(VeneerBSplineViewTest, NonBSpline_ThrowsWrongKind)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl::Curve aLine = occtl::Curve::from_line(aGraph, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}});
    try
    {
      (void)aLine.as_bspline();
      FAIL() << "expected occtl::Error";
    }
    catch (const occtl::Error& anErr)
    {
      EXPECT_EQ(anErr.code(), OCCTL_WRONG_KIND);
    }
  }
}

TEST(VeneerBSpline2dViewTest, RationalBSpline_ScalarsAndSpansMatchRaw)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point2_t                aPoles[4]   = {{0, 0}, {1, 2}, {2, 2}, {3, 0}};
    const double                        aWeights[4] = {1.0, 2.0, 0.5, 1.5};
    const double                        aKnots[2]   = {0.0, 1.0};
    const int32_t                       aMults[2]   = {4, 4};
    occtl_curve2d_bspline_create_info_t aInfo       = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                     = aPoles;
    aInfo.pole_count                                = 4;
    aInfo.weights                                   = aWeights;
    aInfo.knots                                     = aKnots;
    aInfo.multiplicities                            = aMults;
    aInfo.knot_count                                = 2;
    aInfo.degree                                    = 3;
    const occtl::Curve2d aBs                        = occtl::Curve2d::from_bspline(aGraph, aInfo);

    const occtl::Curve2d::BSplineView aView = aBs.as_bspline();
    EXPECT_EQ(aView.degree(), 3);
    EXPECT_TRUE(aView.is_rational());
    EXPECT_EQ(aView.pole_count(), 4u);
    EXPECT_EQ(aView.knot_count(), 2u);

#if OCCTL_HPP_HAS_SPAN
    EXPECT_EQ(aView.poles().size(), aView.raw.pole_count);
    EXPECT_EQ(aView.weights().size(), aView.raw.pole_count);
    EXPECT_EQ(aView.knots().size(), aView.raw.knot_count);
    EXPECT_EQ(aView.multiplicities().size(), aView.raw.knot_count);
    EXPECT_EQ(aView.flat_knots().size(), aView.raw.flat_knot_count);
    for (size_t anI = 0; anI < aView.poles().size(); ++anI)
    {
      EXPECT_NEAR(aView.poles()[anI].x, aPoles[anI].x, THE_EPS);
      EXPECT_NEAR(aView.poles()[anI].y, aPoles[anI].y, THE_EPS);
      EXPECT_NEAR(aView.weights()[anI], aWeights[anI], THE_EPS);
    }
#else
    EXPECT_EQ(aView.poles(), aView.raw.poles);
    EXPECT_EQ(aView.weights(), aView.raw.weights);
#endif
  }
}

TEST(VeneerBSpline2dViewTest, NonRational_WeightsSpanIsEmpty)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_point2_t                aPoles[4] = {{0, 0}, {1, 2}, {2, 2}, {3, 0}};
    const double                        aKnots[2] = {0.0, 1.0};
    const int32_t                       aMults[2] = {4, 4};
    occtl_curve2d_bspline_create_info_t aInfo     = OCCTL_CURVE2D_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                   = aPoles;
    aInfo.pole_count                              = 4;
    aInfo.knots                                   = aKnots;
    aInfo.multiplicities                          = aMults;
    aInfo.knot_count                              = 2;
    aInfo.degree                                  = 3;
    const occtl::Curve2d aBs                      = occtl::Curve2d::from_bspline(aGraph, aInfo);

    const occtl::Curve2d::BSplineView aView = aBs.as_bspline();
    EXPECT_FALSE(aView.is_rational());
    EXPECT_EQ(aView.raw.weights, nullptr);
#if OCCTL_HPP_HAS_SPAN
    EXPECT_TRUE(aView.weights().empty());
    EXPECT_EQ(aView.poles().size(), 4u);
#else
    EXPECT_EQ(aView.weights(), nullptr);
#endif
  }
}

TEST(VeneerBSpline2dViewTest, NonBSpline_ThrowsWrongKind)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl::Curve2d aLine = occtl::Curve2d::from_line(aGraph, {{{0.0, 0.0}, {1.0, 0.0}}});
    try
    {
      (void)aLine.as_bspline();
      FAIL() << "expected occtl::Error";
    }
    catch (const occtl::Error& anErr)
    {
      EXPECT_EQ(anErr.code(), OCCTL_WRONG_KIND);
    }
  }
}

TEST(VeneerBSplineSurfaceViewTest, AsymmetricRational_ScalarsAndSpansMatchRaw)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    static const occtl_point3_t aPoles[20] = {
      {0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, {0, 4, 0}, {1, 0, 1}, {1, 1, 2},
      {1, 2, 1}, {1, 3, 2}, {1, 4, 1}, {2, 0, 1}, {2, 1, 2}, {2, 2, 1}, {2, 3, 2},
      {2, 4, 1}, {3, 0, 0}, {3, 1, 0}, {3, 2, 0}, {3, 3, 0}, {3, 4, 0}};
    static const double  aWeights[20] = {1.0, 0.8, 0.9, 0.8, 1.0, 1.2, 0.7, 0.7, 0.7, 1.2,
                                         1.2, 0.7, 0.7, 0.7, 1.2, 1.0, 0.8, 0.9, 0.8, 1.0};
    static const double  aUKnots[2]   = {0.0, 1.0};
    static const int32_t aUMults[2]   = {4, 4};
    static const double  aVKnots[3]   = {0.0, 0.5, 1.0};
    static const int32_t aVMults[3]   = {3, 2, 3};

    occtl_surface_bspline_create_info_t aInfo = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                               = aPoles;
    aInfo.u_pole_count                        = 4;
    aInfo.v_pole_count                        = 5;
    aInfo.weights                             = aWeights;
    aInfo.u_knots                             = aUKnots;
    aInfo.u_multiplicities                    = aUMults;
    aInfo.u_knot_count                        = 2;
    aInfo.v_knots                             = aVKnots;
    aInfo.v_multiplicities                    = aVMults;
    aInfo.v_knot_count                        = 3;
    aInfo.u_degree                            = 3;
    aInfo.v_degree                            = 2;
    const occtl::Surface aBs                  = occtl::Surface::from_bspline(aGraph, aInfo);

    const occtl::Surface::BSplineView aView = aBs.as_bspline();
    EXPECT_EQ(aView.u_degree(), 3);
    EXPECT_EQ(aView.v_degree(), 2);
    EXPECT_TRUE(aView.is_rational());
    EXPECT_EQ(aView.u_pole_count(), 4u);
    EXPECT_EQ(aView.v_pole_count(), 5u);
    EXPECT_EQ(aView.u_knot_count(), 2u);
    EXPECT_EQ(aView.v_knot_count(), 3u);

#if OCCTL_HPP_HAS_SPAN
    EXPECT_EQ(aView.poles().size(), aView.raw.u_pole_count * aView.raw.v_pole_count);
    EXPECT_EQ(aView.weights().size(), aView.raw.u_pole_count * aView.raw.v_pole_count);
    EXPECT_EQ(aView.u_knots().size(), aView.raw.u_knot_count);
    EXPECT_EQ(aView.v_knots().size(), aView.raw.v_knot_count);
    EXPECT_EQ(aView.u_multiplicities().size(), aView.raw.u_knot_count);
    EXPECT_EQ(aView.v_multiplicities().size(), aView.raw.v_knot_count);
    EXPECT_EQ(aView.u_flat_knots().size(), aView.raw.u_flat_knot_count);
    EXPECT_EQ(aView.v_flat_knots().size(), aView.raw.v_flat_knot_count);
    for (size_t aU = 0; aU < aView.u_pole_count(); ++aU)
    {
      for (size_t aV = 0; aV < aView.v_pole_count(); ++aV)
      {
        const size_t anIdx = aU * aView.v_pole_count() + aV;
        EXPECT_NEAR(aView.poles()[anIdx].x, aPoles[anIdx].x, THE_EPS);
        EXPECT_NEAR(aView.poles()[anIdx].y, aPoles[anIdx].y, THE_EPS);
        EXPECT_NEAR(aView.poles()[anIdx].z, aPoles[anIdx].z, THE_EPS);
        EXPECT_NEAR(aView.weights()[anIdx], aWeights[anIdx], THE_EPS);
      }
    }
#else
    EXPECT_EQ(aView.poles(), aView.raw.poles);
    EXPECT_EQ(aView.weights(), aView.raw.weights);
#endif
  }
}

TEST(VeneerBSplineSurfaceViewTest, NonRational_WeightsSpanIsEmpty)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    static const occtl_point3_t         aPoles[4]  = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 1}};
    static const double                 aUKnots[2] = {0.0, 1.0};
    static const double                 aVKnots[2] = {0.0, 1.0};
    static const int32_t                aUMults[2] = {2, 2};
    static const int32_t                aVMults[2] = {2, 2};
    occtl_surface_bspline_create_info_t aInfo      = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
    aInfo.poles                                    = aPoles;
    aInfo.u_pole_count                             = 2;
    aInfo.v_pole_count                             = 2;
    aInfo.u_knots                                  = aUKnots;
    aInfo.u_multiplicities                         = aUMults;
    aInfo.u_knot_count                             = 2;
    aInfo.v_knots                                  = aVKnots;
    aInfo.v_multiplicities                         = aVMults;
    aInfo.v_knot_count                             = 2;
    aInfo.u_degree                                 = 1;
    aInfo.v_degree                                 = 1;
    const occtl::Surface aBs                       = occtl::Surface::from_bspline(aGraph, aInfo);

    const occtl::Surface::BSplineView aView = aBs.as_bspline();
    EXPECT_FALSE(aView.is_rational());
    EXPECT_EQ(aView.raw.weights, nullptr);
#if OCCTL_HPP_HAS_SPAN
    EXPECT_TRUE(aView.weights().empty());
    EXPECT_EQ(aView.poles().size(), 4u);
#else
    EXPECT_EQ(aView.weights(), nullptr);
#endif
  }
}

TEST(VeneerBSplineSurfaceViewTest, NonBSpline_ThrowsWrongKind)
{
  occtl::Graph   aGraphOwner;
  occtl_graph_t* aGraph = aGraphOwner.get();
  {
    const occtl_geom_plane_t aPlane = {MakeStdAxis3()};
    const occtl::Surface     aSurf  = occtl::Surface::from_plane(aGraph, aPlane);
    try
    {
      (void)aSurf.as_bspline();
      FAIL() << "expected occtl::Error";
    }
    catch (const occtl::Error& anErr)
    {
      EXPECT_EQ(anErr.code(), OCCTL_WRONG_KIND);
    }
  }
}
