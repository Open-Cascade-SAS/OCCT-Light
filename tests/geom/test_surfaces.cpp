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

#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <vector>

namespace
{

occtl_axis3_placement_t MakeStandardAxis3()
{
  return {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
}

occtl_rep_id_t MakeTrimmedLine(occtl_graph_t*            theGraph,
                               const occtl_point3_t&     theOrigin,
                               const occtl_direction3_t& theDirection,
                               const double              theFirst,
                               const double              theLast)
{
  const occtl_geom_line_t aLine  = {theOrigin, theDirection};
  occtl_rep_id_t          aBasis = {};
  EXPECT_EQ(occtl_curve_create_line(theGraph, aLine, &aBasis), OCCTL_OK);

  occtl_curve_trimmed_create_info_t aTrimInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aTrimInfo.basis                             = aBasis;
  aTrimInfo.u_first                           = theFirst;
  aTrimInfo.u_last                            = theLast;

  occtl_rep_id_t aTrimmed = {};
  EXPECT_EQ(occtl_curve_create_trimmed(theGraph, &aTrimInfo, &aTrimmed), OCCTL_OK);
  return aTrimmed;
}

occtl_rep_id_t MakeSimpleBSplineSurface(occtl_graph_t* theGraph)
{
  static const occtl_point3_t kPoles[4]  = {{0.0, 0.0, 0.0},
                                            {1.0, 0.0, 0.0},
                                            {0.0, 1.0, 0.0},
                                            {1.0, 1.0, 1.0}};
  static const double         kUKnots[2] = {0.0, 1.0};
  static const double         kVKnots[2] = {0.0, 1.0};
  static const int32_t        kUMults[2] = {2, 2};
  static const int32_t        kVMults[2] = {2, 2};

  occtl_surface_bspline_create_info_t aInfo = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                               = kPoles;
  aInfo.u_pole_count                        = 2;
  aInfo.v_pole_count                        = 2;
  aInfo.u_knots                             = kUKnots;
  aInfo.u_multiplicities                    = kUMults;
  aInfo.u_knot_count                        = 2;
  aInfo.v_knots                             = kVKnots;
  aInfo.v_multiplicities                    = kVMults;
  aInfo.v_knot_count                        = 2;
  aInfo.u_degree                            = 1;
  aInfo.v_degree                            = 1;

  occtl_rep_id_t aId = {};
  if (occtl_surface_create_bspline(theGraph, &aId, &aInfo) != OCCTL_OK)
  {
    return OCCTL_REP_ID_INVALID;
  }
  return aId;
}

occtl_rep_id_t MakeAsymmetricRationalBSplineSurface(occtl_graph_t* theGraph)
{
  static const occtl_point3_t kPoles[20] = {
    {0.0, 0.0, 0.0}, {0.0, 0.25, 0.1}, {0.0, 0.5, 0.1}, {0.0, 0.75, 0.1}, {0.0, 1.0, 0.0},
    {1.0, 0.0, 0.2}, {1.0, 0.25, 0.3}, {1.0, 0.5, 0.4}, {1.0, 0.75, 0.3}, {1.0, 1.0, 0.2},
    {2.0, 0.0, 0.2}, {2.0, 0.25, 0.5}, {2.0, 0.5, 0.6}, {2.0, 0.75, 0.5}, {2.0, 1.0, 0.2},
    {3.0, 0.0, 0.0}, {3.0, 0.25, 0.1}, {3.0, 0.5, 0.1}, {3.0, 0.75, 0.1}, {3.0, 1.0, 0.0}};
  static const double  kWeights[20] = {1.0, 0.8, 0.9, 0.8, 1.0, 1.2, 0.7, 0.7, 0.7, 1.2,
                                       1.2, 0.7, 0.7, 0.7, 1.2, 1.0, 0.8, 0.9, 0.8, 1.0};
  static const double  kUKnots[2]   = {0.0, 1.0};
  static const int32_t kUMults[2]   = {4, 4};
  static const double  kVKnots[3]   = {0.0, 0.5, 1.0};
  static const int32_t kVMults[3]   = {3, 2, 3};

  occtl_surface_bspline_create_info_t aInfo = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                               = kPoles;
  aInfo.u_pole_count                        = 4;
  aInfo.v_pole_count                        = 5;
  aInfo.weights                             = kWeights;
  aInfo.u_knots                             = kUKnots;
  aInfo.u_multiplicities                    = kUMults;
  aInfo.u_knot_count                        = 2;
  aInfo.v_knots                             = kVKnots;
  aInfo.v_multiplicities                    = kVMults;
  aInfo.v_knot_count                        = 3;
  aInfo.u_degree                            = 3;
  aInfo.v_degree                            = 2;

  occtl_rep_id_t aId = {};
  if (occtl_surface_create_bspline(theGraph, &aId, &aInfo) != OCCTL_OK)
  {
    return OCCTL_REP_ID_INVALID;
  }
  return aId;
}

} // namespace

TEST(SurfacesPlaneTest, Construct_KindIsPlane)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_PLANE);
  occtl_graph_free(aGraph);
}

TEST(SurfacesPlaneTest, AsPlane_RoundTripsPosition)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);
  occtl_geom_plane_t aOut{};
  ASSERT_EQ(occtl_surface_as_plane(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.position.location.x, 0.0, 1e-14);
  EXPECT_NEAR(aOut.position.location.y, 0.0, 1e-14);
  EXPECT_NEAR(aOut.position.location.z, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCylinderTest, Construct_KindIsCylindrical)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), 3.0};
  occtl_rep_id_t                         aId  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_CYLINDRICAL);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCylinderTest, AsCylinder_RoundTripsRadius)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), 7.0};
  occtl_rep_id_t                         aId  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_OK);
  occtl_geom_cylindrical_surface_t aOut{};
  ASSERT_EQ(occtl_surface_as_cylinder(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.radius, 7.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCylinderTest, InvalidRadius_ReturnsGeometryInvalid)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), -1.0};
  occtl_rep_id_t                         aId  = {};
  EXPECT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_GEOMETRY_INVALID);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesConeTest, Construct_KindIsConical)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_conical_surface_t aCone = {MakeStandardAxis3(), 0.5, 0.0};
  occtl_rep_id_t                     aId   = {};
  ASSERT_EQ(occtl_surface_create_cone(aGraph, &aId, aCone), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_CONICAL);
  occtl_graph_free(aGraph);
}

TEST(SurfacesConeTest, AsCone_RoundTripsSemiAngle)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_conical_surface_t aCone = {MakeStandardAxis3(), 0.3, 2.0};
  occtl_rep_id_t                     aId   = {};
  ASSERT_EQ(occtl_surface_create_cone(aGraph, &aId, aCone), OCCTL_OK);
  occtl_geom_conical_surface_t aOut{};
  ASSERT_EQ(occtl_surface_as_cone(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.semi_angle, 0.3, 1e-14);
  EXPECT_NEAR(aOut.radius, 2.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesSphereTest, Construct_KindIsSpherical)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_spherical_surface_t aSphere = {MakeStandardAxis3(), 5.0};
  occtl_rep_id_t                       aId     = {};
  ASSERT_EQ(occtl_surface_create_sphere(aGraph, &aId, aSphere), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_SPHERICAL);
  occtl_graph_free(aGraph);
}

TEST(SurfacesSphereTest, AsSphere_RoundTripsRadius)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_spherical_surface_t aSphere = {MakeStandardAxis3(), 3.5};
  occtl_rep_id_t                       aId     = {};
  ASSERT_EQ(occtl_surface_create_sphere(aGraph, &aId, aSphere), OCCTL_OK);
  occtl_geom_spherical_surface_t aOut{};
  ASSERT_EQ(occtl_surface_as_sphere(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.radius, 3.5, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesTorusTest, Construct_KindIsToroidal)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_toroidal_surface_t aTorus = {MakeStandardAxis3(), 5.0, 2.0};
  occtl_rep_id_t                      aId    = {};
  ASSERT_EQ(occtl_surface_create_torus(aGraph, &aId, aTorus), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_TOROIDAL);
  occtl_graph_free(aGraph);
}

TEST(SurfacesTorusTest, AsTorus_RoundTripsRadii)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_toroidal_surface_t aTorus = {MakeStandardAxis3(), 8.0, 2.0};
  occtl_rep_id_t                      aId    = {};
  ASSERT_EQ(occtl_surface_create_torus(aGraph, &aId, aTorus), OCCTL_OK);
  occtl_geom_toroidal_surface_t aOut{};
  ASSERT_EQ(occtl_surface_as_torus(aGraph, aId, &aOut), OCCTL_OK);
  EXPECT_NEAR(aOut.major_radius, 8.0, 1e-14);
  EXPECT_NEAR(aOut.minor_radius, 2.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesWrongKindTest, AsPlane_OnCylinder_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), 3.0};
  occtl_rep_id_t                         aId  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_OK);
  occtl_geom_plane_t aOut{};
  EXPECT_EQ(occtl_surface_as_plane(aGraph, aId, &aOut), OCCTL_WRONG_KIND);
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_GT(strlen(aErr->message), 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesWrongKindTest, AsCylinder_OnSphere_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_spherical_surface_t aSphere = {MakeStandardAxis3(), 4.0};
  occtl_rep_id_t                       aId     = {};
  ASSERT_EQ(occtl_surface_create_sphere(aGraph, &aId, aSphere), OCCTL_OK);
  occtl_geom_cylindrical_surface_t aOut{};
  EXPECT_EQ(occtl_surface_as_cylinder(aGraph, aId, &aOut), OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(SurfacesTransformTest, Transformed_SameKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);
  occtl_transform_t aTransform   = {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0}};
  occtl_rep_id_t    aTransformed = {};
  ASSERT_EQ(occtl_surface_transformed(aGraph, aId, aTransform, &aTransformed), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aTransformed, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_PLANE);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, Construct_KindIsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeSimpleBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_BSPLINE);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBezierGridTest, Construct_KindIsBezier)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoles[4] = {{0.0, 0.0, 0.0},
                                    {0.0, 1.0, 0.0},
                                    {1.0, 0.0, 0.2},
                                    {1.0, 1.0, 0.0}};

  occtl_surface_bezier_create_info_t anInfo = OCCTL_SURFACE_BEZIER_CREATE_INFO_INIT;
  anInfo.poles                              = aPoles;
  anInfo.u_pole_count                       = 2;
  anInfo.v_pole_count                       = 2;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_bezier_grid(aGraph, &aId, &anInfo), OCCTL_OK);
  ASSERT_NE(aId.bits, 0u);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_BEZIER);
  size_t aNbU = 0, aNbV = 0;
  ASSERT_EQ(occtl_surface_bezier_u_pole_count(aGraph, aId, &aNbU), OCCTL_OK);
  EXPECT_EQ(aNbU, 2u);
  ASSERT_EQ(occtl_surface_bezier_v_pole_count(aGraph, aId, &aNbV), OCCTL_OK);
  EXPECT_EQ(aNbV, 2u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBezierGridTest, BadVersion_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoles[1] = {{0.0, 0.0, 0.0}};

  occtl_surface_bezier_create_info_t anInfo = OCCTL_SURFACE_BEZIER_CREATE_INFO_INIT;
  anInfo.struct_version                     = 999u;
  anInfo.poles                              = aPoles;
  anInfo.u_pole_count                       = 1;
  anInfo.v_pole_count                       = 1;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_bezier_grid(aGraph, &aId, &anInfo), OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBezierGridTest, PNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoles[1] = {{0.0, 0.0, 0.0}};

  occtl_surface_bezier_create_info_t anInfo = OCCTL_SURFACE_BEZIER_CREATE_INFO_INIT;
  anInfo.p_next                             = reinterpret_cast<const void*>(0x1);
  anInfo.poles                              = aPoles;
  anInfo.u_pole_count                       = 1;
  anInfo.v_pole_count                       = 1;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_bezier_grid(aGraph, &aId, &anInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesPointGridTest, InterpolateInfoInit_HasDefaults)
{
  occtl_surface_interpolated_info_t anInfo{};
  occtl_surface_interpolated_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_SURFACE_INTERPOLATED_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.points, nullptr);
  EXPECT_EQ(anInfo.u_point_count, 0u);
  EXPECT_EQ(anInfo.v_point_count, 0u);
  EXPECT_EQ(anInfo.is_u_periodic, 0);
}

TEST(SurfacesPointGridTest, Interpolate_ThreeByThreeGrid_ReturnsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoints[9] = {{0.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.2},
                                     {0.0, 2.0, 0.0},
                                     {1.0, 0.0, 0.4},
                                     {1.0, 1.0, 1.0},
                                     {1.0, 2.0, 0.4},
                                     {2.0, 0.0, 0.0},
                                     {2.0, 1.0, 0.2},
                                     {2.0, 2.0, 0.0}};

  occtl_surface_interpolated_info_t anInfo = OCCTL_SURFACE_INTERPOLATED_INFO_INIT;
  anInfo.points                            = aPoints;
  anInfo.u_point_count                     = 3;
  anInfo.v_point_count                     = 3;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_interpolated(aGraph, &anInfo, &aId), OCCTL_OK);
  ASSERT_NE(aId.bits, 0u);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_BSPLINE);
  size_t aNbU = 0, aNbV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_pole_count(aGraph, aId, &aNbU), OCCTL_OK);
  EXPECT_GE(aNbU, 3u);
  ASSERT_EQ(occtl_surface_bspline_v_pole_count(aGraph, aId, &aNbV), OCCTL_OK);
  EXPECT_GE(aNbV, 3u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesPointGridTest, Interpolate_InvalidPeriodicFlag_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoints[4] = {{0.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.0},
                                     {1.0, 0.0, 0.0},
                                     {1.0, 1.0, 0.0}};

  occtl_surface_interpolated_info_t anInfo = OCCTL_SURFACE_INTERPOLATED_INFO_INIT;
  anInfo.points                            = aPoints;
  anInfo.u_point_count                     = 2;
  anInfo.v_point_count                     = 2;
  anInfo.is_u_periodic                     = 2;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_interpolated(aGraph, &anInfo, &aId), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesPointGridTest, ApproximateInfoInit_HasDefaults)
{
  occtl_surface_approximated_info_t anInfo{};
  occtl_surface_approximated_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_SURFACE_APPROXIMATED_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.points, nullptr);
  EXPECT_EQ(anInfo.u_point_count, 0u);
  EXPECT_EQ(anInfo.v_point_count, 0u);
  EXPECT_EQ(anInfo.degree_min, 1);
  EXPECT_EQ(anInfo.degree_max, 3);
  EXPECT_DOUBLE_EQ(anInfo.tolerance, 1.0e-3);
}

TEST(SurfacesPointGridTest, Approximate_ThreeByThreeGrid_ReturnsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoints[9] = {{0.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.1},
                                     {0.0, 2.0, 0.0},
                                     {1.0, 0.0, 0.2},
                                     {1.0, 1.0, 0.6},
                                     {1.0, 2.0, 0.2},
                                     {2.0, 0.0, 0.0},
                                     {2.0, 1.0, 0.1},
                                     {2.0, 2.0, 0.0}};

  occtl_surface_approximated_info_t anInfo = OCCTL_SURFACE_APPROXIMATED_INFO_INIT;
  anInfo.points                            = aPoints;
  anInfo.u_point_count                     = 3;
  anInfo.v_point_count                     = 3;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_approximated(aGraph, &anInfo, &aId), OCCTL_OK);
  ASSERT_NE(aId.bits, 0u);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_BSPLINE);
  int32_t aDegU = 0, aDegV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_degree(aGraph, aId, &aDegU), OCCTL_OK);
  EXPECT_GE(aDegU, 1);
  ASSERT_EQ(occtl_surface_bspline_v_degree(aGraph, aId, &aDegV), OCCTL_OK);
  EXPECT_GE(aDegV, 1);
  occtl_graph_free(aGraph);
}

TEST(SurfacesPointGridTest, Approximate_InvalidDegreeRange_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoints[4] = {{0.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.0},
                                     {1.0, 0.0, 0.0},
                                     {1.0, 1.0, 0.0}};

  occtl_surface_approximated_info_t anInfo = OCCTL_SURFACE_APPROXIMATED_INFO_INIT;
  anInfo.points                            = aPoints;
  anInfo.u_point_count                     = 2;
  anInfo.v_point_count                     = 2;
  anInfo.degree_max                        = 0;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_approximated(aGraph, &anInfo, &aId), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesPointGridTest, PointGridInfoInit_HasDefaults)
{
  occtl_surface_point_grid_create_info_t anInfo{};
  occtl_surface_point_grid_create_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_SURFACE_POINT_GRID_CREATE_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.points, nullptr);
  EXPECT_EQ(anInfo.u_point_count, 0u);
  EXPECT_EQ(anInfo.v_point_count, 0u);
  EXPECT_EQ(anInfo.mode, OCCTL_SURFACE_POINT_GRID_MODE_APPROXIMATE);
  EXPECT_EQ(anInfo.degree_min, 1);
  EXPECT_EQ(anInfo.degree_max, 3);
  EXPECT_EQ(anInfo.is_u_periodic, 0);
  EXPECT_DOUBLE_EQ(anInfo.tolerance, 1.0e-3);
}

TEST(SurfacesPointGridTest, PointGridApproximate_ThreeByThreeGrid_ReturnsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoints[9] = {{0.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.1},
                                     {0.0, 2.0, 0.0},
                                     {1.0, 0.0, 0.2},
                                     {1.0, 1.0, 0.6},
                                     {1.0, 2.0, 0.2},
                                     {2.0, 0.0, 0.0},
                                     {2.0, 1.0, 0.1},
                                     {2.0, 2.0, 0.0}};

  occtl_surface_point_grid_create_info_t anInfo = OCCTL_SURFACE_POINT_GRID_CREATE_INFO_INIT;
  anInfo.points                                 = aPoints;
  anInfo.u_point_count                          = 3;
  anInfo.v_point_count                          = 3;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_from_point_grid(aGraph, &aId, &anInfo), OCCTL_OK);
  ASSERT_NE(aId.bits, 0u);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_BSPLINE);
  int32_t aDegU = 0, aDegV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_degree(aGraph, aId, &aDegU), OCCTL_OK);
  EXPECT_GE(aDegU, 1);
  ASSERT_EQ(occtl_surface_bspline_v_degree(aGraph, aId, &aDegV), OCCTL_OK);
  EXPECT_GE(aDegV, 1);
  occtl_graph_free(aGraph);
}

TEST(SurfacesPointGridTest, PointGridInterpolate_ThreeByThreeGrid_ReturnsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoints[9] = {{0.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.2},
                                     {0.0, 2.0, 0.0},
                                     {1.0, 0.0, 0.4},
                                     {1.0, 1.0, 1.0},
                                     {1.0, 2.0, 0.4},
                                     {2.0, 0.0, 0.0},
                                     {2.0, 1.0, 0.2},
                                     {2.0, 2.0, 0.0}};

  occtl_surface_point_grid_create_info_t anInfo = OCCTL_SURFACE_POINT_GRID_CREATE_INFO_INIT;
  anInfo.points                                 = aPoints;
  anInfo.u_point_count                          = 3;
  anInfo.v_point_count                          = 3;
  anInfo.mode                                   = OCCTL_SURFACE_POINT_GRID_MODE_INTERPOLATE;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_from_point_grid(aGraph, &aId, &anInfo), OCCTL_OK);
  ASSERT_NE(aId.bits, 0u);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_BSPLINE);
  size_t aNbU = 0, aNbV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_pole_count(aGraph, aId, &aNbU), OCCTL_OK);
  EXPECT_GE(aNbU, 3u);
  ASSERT_EQ(occtl_surface_bspline_v_pole_count(aGraph, aId, &aNbV), OCCTL_OK);
  EXPECT_GE(aNbV, 3u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesPointGridTest, PointGridInvalidMode_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoints[4] = {{0.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.0},
                                     {1.0, 0.0, 0.0},
                                     {1.0, 1.0, 0.0}};

  occtl_surface_point_grid_create_info_t anInfo = OCCTL_SURFACE_POINT_GRID_CREATE_INFO_INIT;
  anInfo.points                                 = aPoints;
  anInfo.u_point_count                          = 2;
  anInfo.v_point_count                          = 2;
  anInfo.mode                                   = OCCTL_SURFACE_POINT_GRID_MODE_RESERVED_FUTURE;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_from_point_grid(aGraph, &aId, &anInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesGordonTest, GordonInfoInit_HasDefaults)
{
  occtl_surface_gordon_create_info_t anInfo{};
  occtl_surface_gordon_create_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_SURFACE_GORDON_CREATE_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.profiles, nullptr);
  EXPECT_EQ(anInfo.profile_count, 0u);
  EXPECT_EQ(anInfo.guides, nullptr);
  EXPECT_EQ(anInfo.guide_count, 0u);
  EXPECT_DOUBLE_EQ(anInfo.tolerance, 1.0e-7);
  EXPECT_EQ(anInfo.parallel, 0);
}

TEST(SurfacesGordonTest, CreateGordon_TwoByTwoLineNetwork_ReturnsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aProfile0 = MakeTrimmedLine(aGraph, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
  occtl_rep_id_t aProfile1 = MakeTrimmedLine(aGraph, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
  occtl_rep_id_t aGuide0   = MakeTrimmedLine(aGraph, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
  occtl_rep_id_t aGuide1   = MakeTrimmedLine(aGraph, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
  ASSERT_NE(aProfile0.bits, 0u);
  ASSERT_NE(aProfile1.bits, 0u);
  ASSERT_NE(aGuide0.bits, 0u);
  ASSERT_NE(aGuide1.bits, 0u);

  const occtl_rep_id_t aProfiles[2] = {aProfile0, aProfile1};
  const occtl_rep_id_t aGuides[2]   = {aGuide0, aGuide1};

  occtl_surface_gordon_create_info_t anInfo = OCCTL_SURFACE_GORDON_CREATE_INFO_INIT;
  anInfo.profiles                           = aProfiles;
  anInfo.profile_count                      = 2;
  anInfo.guides                             = aGuides;
  anInfo.guide_count                        = 2;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_gordon(aGraph, &aId, &anInfo), OCCTL_OK);
  ASSERT_NE(aId.bits, 0u);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_BSPLINE);

  double aUMin = 0.0, aUMax = 0.0, aVMin = 0.0, aVMax = 0.0;
  ASSERT_EQ(occtl_surface_parameter_range(aGraph, aId, &aUMin, &aUMax, &aVMin, &aVMax), OCCTL_OK);
  occtl_point3_t aCorner{};
  ASSERT_EQ(occtl_surface_eval_d0(aGraph, aId, aUMin, aVMin, &aCorner), OCCTL_OK);
  EXPECT_NEAR(aCorner.x, 0.0, 1.0e-6);
  EXPECT_NEAR(aCorner.y, 0.0, 1.0e-6);
  EXPECT_NEAR(aCorner.z, 0.0, 1.0e-6);

  occtl_graph_free(aGraph);
}

TEST(SurfacesGordonTest, CreateGordon_InvalidArrays_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_surface_gordon_create_info_t anInfo = OCCTL_SURFACE_GORDON_CREATE_INFO_INIT;
  occtl_rep_id_t                     aId    = {};

  EXPECT_EQ(occtl_surface_create_gordon(aGraph, &aId, &anInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);

  anInfo.struct_version = 0u;
  EXPECT_EQ(occtl_surface_create_gordon(aGraph, &aId, &anInfo), OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCurveGridTest, CurveGridInfoInit_HasDefaults)
{
  occtl_surface_curve_grid_create_info_t anInfo{};
  occtl_surface_curve_grid_create_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.u_curves, nullptr);
  EXPECT_EQ(anInfo.u_curve_count, 0u);
  EXPECT_EQ(anInfo.v_curves, nullptr);
  EXPECT_EQ(anInfo.v_curve_count, 0u);
  EXPECT_DOUBLE_EQ(anInfo.tolerance, 1.0e-7);
  EXPECT_EQ(anInfo.parallel, 0);
}

TEST(SurfacesCurveGridTest, CreateCurveGrid_TwoByTwoLineNetwork_ReturnsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aU0 = MakeTrimmedLine(aGraph, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
  occtl_rep_id_t aU1 = MakeTrimmedLine(aGraph, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
  occtl_rep_id_t aV0 = MakeTrimmedLine(aGraph, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
  occtl_rep_id_t aV1 = MakeTrimmedLine(aGraph, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
  ASSERT_NE(aU0.bits, 0u);
  ASSERT_NE(aU1.bits, 0u);
  ASSERT_NE(aV0.bits, 0u);
  ASSERT_NE(aV1.bits, 0u);

  const occtl_rep_id_t aUCurves[2] = {aU0, aU1};
  const occtl_rep_id_t aVCurves[2] = {aV0, aV1};

  occtl_surface_curve_grid_create_info_t anInfo = OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_INIT;
  anInfo.u_curves                               = aUCurves;
  anInfo.u_curve_count                          = 2;
  anInfo.v_curves                               = aVCurves;
  anInfo.v_curve_count                          = 2;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_from_curve_grid(aGraph, &aId, &anInfo), OCCTL_OK);
  ASSERT_NE(aId.bits, 0u);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_BSPLINE);

  double aUMin = 0.0, aUMax = 0.0, aVMin = 0.0, aVMax = 0.0;
  ASSERT_EQ(occtl_surface_parameter_range(aGraph, aId, &aUMin, &aUMax, &aVMin, &aVMax), OCCTL_OK);
  occtl_point3_t aCorner{};
  ASSERT_EQ(occtl_surface_eval_d0(aGraph, aId, aUMin, aVMin, &aCorner), OCCTL_OK);
  EXPECT_NEAR(aCorner.x, 0.0, 1.0e-6);
  EXPECT_NEAR(aCorner.y, 0.0, 1.0e-6);
  EXPECT_NEAR(aCorner.z, 0.0, 1.0e-6);

  occtl_graph_free(aGraph);
}

TEST(SurfacesCurveGridTest, CreateCurveGrid_InvalidInfo_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_surface_curve_grid_create_info_t anInfo = OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_INIT;
  occtl_rep_id_t                         aId    = {};

  EXPECT_EQ(occtl_surface_create_from_curve_grid(aGraph, &aId, &anInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);

  anInfo.parallel = 2;
  EXPECT_EQ(occtl_surface_create_from_curve_grid(aGraph, &aId, &anInfo), OCCTL_INVALID_ARGUMENT);

  anInfo.parallel       = 0;
  anInfo.struct_version = 0u;
  EXPECT_EQ(occtl_surface_create_from_curve_grid(aGraph, &aId, &anInfo), OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBoundaryCurvesTest, BoundaryCurvesInfoInit_HasDefaults)
{
  occtl_surface_boundary_curves_create_info_t anInfo{};
  occtl_surface_boundary_curves_create_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.curves, nullptr);
  EXPECT_EQ(anInfo.curve_count, 0u);
  EXPECT_EQ(anInfo.style, OCCTL_SURFACE_FILLING_STRETCH);
}

TEST(SurfacesBoundaryCurvesTest, BoundaryCurves_FourLines_ReturnsBSpline)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aBottom = MakeTrimmedLine(aGraph, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
  occtl_rep_id_t aRight  = MakeTrimmedLine(aGraph, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
  occtl_rep_id_t aTop    = MakeTrimmedLine(aGraph, {1.0, 1.0, 0.0}, {-1.0, 0.0, 0.0}, 0.0, 1.0);
  occtl_rep_id_t aLeft   = MakeTrimmedLine(aGraph, {0.0, 1.0, 0.0}, {0.0, -1.0, 0.0}, 0.0, 1.0);
  ASSERT_NE(aBottom.bits, 0u);
  ASSERT_NE(aRight.bits, 0u);
  ASSERT_NE(aTop.bits, 0u);
  ASSERT_NE(aLeft.bits, 0u);

  const occtl_rep_id_t                        aCurves[4] = {aBottom, aRight, aTop, aLeft};
  occtl_surface_boundary_curves_create_info_t anInfo =
    OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_INIT;
  anInfo.curves      = aCurves;
  anInfo.curve_count = 4;
  anInfo.style       = OCCTL_SURFACE_FILLING_STRETCH;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_from_boundary_curves(aGraph, &aId, &anInfo), OCCTL_OK);
  ASSERT_NE(aId.bits, 0u);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_BSPLINE);

  occtl_graph_free(aGraph);
}

TEST(SurfacesBoundaryCurvesTest, BoundaryCurves_InvalidInfo_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_surface_boundary_curves_create_info_t anInfo =
    OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_INIT;
  occtl_rep_id_t aId = {};

  EXPECT_EQ(occtl_surface_create_from_boundary_curves(aGraph, &aId, &anInfo),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);

  anInfo.struct_version = 0u;
  EXPECT_EQ(occtl_surface_create_from_boundary_curves(aGraph, &aId, &anInfo),
            OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, ScalarQueries_MatchInput)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeSimpleBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);
  int32_t aDegU = 0, aDegV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_degree(aGraph, aId, &aDegU), OCCTL_OK);
  EXPECT_EQ(aDegU, 1);
  ASSERT_EQ(occtl_surface_bspline_v_degree(aGraph, aId, &aDegV), OCCTL_OK);
  EXPECT_EQ(aDegV, 1);
  size_t aNbU = 0, aNbV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_pole_count(aGraph, aId, &aNbU), OCCTL_OK);
  EXPECT_EQ(aNbU, 2u);
  ASSERT_EQ(occtl_surface_bspline_v_pole_count(aGraph, aId, &aNbV), OCCTL_OK);
  EXPECT_EQ(aNbV, 2u);
  int32_t aRational = 0;
  ASSERT_EQ(occtl_surface_bspline_is_rational(aGraph, aId, &aRational), OCCTL_OK);
  EXPECT_EQ(aRational, 0);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, TwoCallPoles_RoundTrip)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeSimpleBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  size_t aNb = 0;
  ASSERT_EQ(occtl_surface_bspline_poles(aGraph, aId, nullptr, 0, &aNb), OCCTL_OK);
  ASSERT_EQ(aNb, 4u);

  occtl_point3_t aPoles[4]{};
  ASSERT_EQ(occtl_surface_bspline_poles(aGraph, aId, aPoles, 4, &aNb), OCCTL_OK);
  EXPECT_NEAR(aPoles[0].x, 0.0, 1e-14);
  EXPECT_NEAR(aPoles[0].y, 0.0, 1e-14);
  EXPECT_NEAR(aPoles[0].z, 0.0, 1e-14);
  EXPECT_NEAR(aPoles[3].x, 1.0, 1e-14);
  EXPECT_NEAR(aPoles[3].y, 1.0, 1e-14);
  EXPECT_NEAR(aPoles[3].z, 1.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, UKnots_RoundTrip)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeSimpleBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  size_t aNb = 0;
  double aKnots[2]{};
  ASSERT_EQ(occtl_surface_bspline_u_knots(aGraph, aId, aKnots, 2, &aNb), OCCTL_OK);
  ASSERT_EQ(aNb, 2u);
  EXPECT_NEAR(aKnots[0], 0.0, 1e-14);
  EXPECT_NEAR(aKnots[1], 1.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, VMultiplicities_RoundTrip)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeSimpleBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  size_t  aNb = 0;
  int32_t aMults[2]{};
  ASSERT_EQ(occtl_surface_bspline_v_multiplicities(aGraph, aId, aMults, 2, &aNb), OCCTL_OK);
  ASSERT_EQ(aNb, 2u);
  EXPECT_EQ(aMults[0], 2);
  EXPECT_EQ(aMults[1], 2);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, VersionMismatch_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_surface_bspline_create_info_t aInfo = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
  aInfo.struct_version                      = 99u;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_bspline(aGraph, &aId, &aInfo), OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, InvalidPeriodicFlag_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_point3_t kPoles[4]  = {{0.0, 0.0, 0.0},
                                     {1.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.0},
                                     {1.0, 1.0, 1.0}};
  const double         kUKnots[2] = {0.0, 1.0};
  const double         kVKnots[2] = {0.0, 1.0};
  const int32_t        kUMults[2] = {2, 2};
  const int32_t        kVMults[2] = {2, 2};

  occtl_surface_bspline_create_info_t aInfo = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                               = kPoles;
  aInfo.u_pole_count                        = 2;
  aInfo.v_pole_count                        = 2;
  aInfo.u_knots                             = kUKnots;
  aInfo.u_multiplicities                    = kUMults;
  aInfo.u_knot_count                        = 2;
  aInfo.v_knots                             = kVKnots;
  aInfo.v_multiplicities                    = kVMults;
  aInfo.v_knot_count                        = 2;
  aInfo.u_degree                            = 1;
  aInfo.v_degree                            = 1;
  aInfo.is_u_periodic                       = 2;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_bspline(aGraph, &aId, &aInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, PNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  occtl_surface_bspline_create_info_t aInfo = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
  aInfo.p_next                              = reinterpret_cast<const void*>(0x1);

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_bspline(aGraph, &aId, &aInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, ZeroCounts_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_point3_t kPoles[1]  = {{0.0, 0.0, 0.0}};
  const double         kUKnots[1] = {0.0};
  const double         kVKnots[1] = {0.0};
  const int32_t        kUMults[1] = {1};
  const int32_t        kVMults[1] = {1};

  occtl_surface_bspline_create_info_t aInfo = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                               = kPoles;
  aInfo.u_pole_count                        = 0;
  aInfo.v_pole_count                        = 1;
  aInfo.u_knots                             = kUKnots;
  aInfo.u_multiplicities                    = kUMults;
  aInfo.u_knot_count                        = 1;
  aInfo.v_knots                             = kVKnots;
  aInfo.v_multiplicities                    = kVMults;
  aInfo.v_knot_count                        = 1;
  aInfo.u_degree                            = 1;
  aInfo.v_degree                            = 1;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_bspline(aGraph, &aId, &aInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesBSplineTest, InvalidDegree_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_point3_t kPoles[4]  = {{0.0, 0.0, 0.0},
                                     {1.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.0},
                                     {1.0, 1.0, 1.0}};
  const double         kUKnots[2] = {0.0, 1.0};
  const double         kVKnots[2] = {0.0, 1.0};
  const int32_t        kUMults[2] = {2, 2};
  const int32_t        kVMults[2] = {2, 2};

  occtl_surface_bspline_create_info_t aInfo = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                               = kPoles;
  aInfo.u_pole_count                        = 2;
  aInfo.v_pole_count                        = 2;
  aInfo.u_knots                             = kUKnots;
  aInfo.u_multiplicities                    = kUMults;
  aInfo.u_knot_count                        = 2;
  aInfo.v_knots                             = kVKnots;
  aInfo.v_multiplicities                    = kVMults;
  aInfo.v_knot_count                        = 2;
  aInfo.u_degree                            = 2;
  aInfo.v_degree                            = 1;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_bspline(aGraph, &aId, &aInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(BSplineSurfaceViewTest, Init_PopulatesVersionAndZeros)
{
  occtl_surface_bspline_t aView{};
  aView.struct_version = 0xdeadbeef;
  aView.u_degree       = 9;
  aView.v_degree       = 7;
  aView.poles          = reinterpret_cast<const occtl_point3_t*>(0x1234);
  aView.weights        = reinterpret_cast<const double*>(0x5678);
  occtl_surface_bspline_init(&aView);
  EXPECT_EQ(aView.struct_version, OCCTL_SURFACE_BSPLINE_VERSION_1);
  EXPECT_EQ(aView.p_next, nullptr);
  EXPECT_EQ(aView.u_degree, 0);
  EXPECT_EQ(aView.v_degree, 0);
  EXPECT_EQ(aView.poles, nullptr);
  EXPECT_EQ(aView.weights, nullptr);
}

TEST(BSplineSurfaceViewTest, Init_NullTolerant)
{
  occtl_surface_bspline_init(nullptr);
}

TEST(BSplineSurfaceViewTest, Asymmetric_Rational_ScalarsMatchAtoms)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeAsymmetricRationalBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_surface_bspline_t aView = OCCTL_SURFACE_BSPLINE_INIT;
  ASSERT_EQ(occtl_surface_as_bspline(aGraph, aId, &aView), OCCTL_OK);

  int32_t aDegU = 0, aDegV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_degree(aGraph, aId, &aDegU), OCCTL_OK);
  EXPECT_EQ(aView.u_degree, aDegU);
  ASSERT_EQ(occtl_surface_bspline_v_degree(aGraph, aId, &aDegV), OCCTL_OK);
  EXPECT_EQ(aView.v_degree, aDegV);
  int32_t aRational = 0;
  ASSERT_EQ(occtl_surface_bspline_is_rational(aGraph, aId, &aRational), OCCTL_OK);
  EXPECT_EQ(aView.is_rational, aRational);
  int32_t aUPer = 0, aVPer = 0;
  ASSERT_EQ(occtl_surface_is_u_periodic(aGraph, aId, &aUPer), OCCTL_OK);
  EXPECT_EQ(aView.is_u_periodic, aUPer);
  ASSERT_EQ(occtl_surface_is_v_periodic(aGraph, aId, &aVPer), OCCTL_OK);
  EXPECT_EQ(aView.is_v_periodic, aVPer);
  size_t aNbU = 0, aNbV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_pole_count(aGraph, aId, &aNbU), OCCTL_OK);
  EXPECT_EQ(aView.u_pole_count, aNbU);
  ASSERT_EQ(occtl_surface_bspline_v_pole_count(aGraph, aId, &aNbV), OCCTL_OK);
  EXPECT_EQ(aView.v_pole_count, aNbV);
  size_t aUKC = 0, aVKC = 0;
  ASSERT_EQ(occtl_surface_bspline_u_knot_count(aGraph, aId, &aUKC), OCCTL_OK);
  EXPECT_EQ(aView.u_knot_count, aUKC);
  ASSERT_EQ(occtl_surface_bspline_v_knot_count(aGraph, aId, &aVKC), OCCTL_OK);
  EXPECT_EQ(aView.v_knot_count, aVKC);

  EXPECT_EQ(aView.u_degree, 3);
  EXPECT_EQ(aView.v_degree, 2);
  EXPECT_EQ(aView.is_rational, 1);
  EXPECT_EQ(aView.u_pole_count, 4u);
  EXPECT_EQ(aView.v_pole_count, 5u);
  EXPECT_EQ(aView.u_knot_count, 2u);
  EXPECT_EQ(aView.v_knot_count, 3u);

  size_t aFlatU = 0, aFlatV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_flat_knots(aGraph, aId, nullptr, 0, &aFlatU), OCCTL_OK);
  ASSERT_EQ(occtl_surface_bspline_v_flat_knots(aGraph, aId, nullptr, 0, &aFlatV), OCCTL_OK);
  EXPECT_EQ(aView.u_flat_knot_count, aFlatU);
  EXPECT_EQ(aView.v_flat_knot_count, aFlatV);
  EXPECT_EQ(aView.u_flat_knot_count, 8u);
  EXPECT_EQ(aView.v_flat_knot_count, 8u);

  occtl_graph_free(aGraph);
}

TEST(BSplineSurfaceViewTest, Asymmetric_Rational_PolesMatchAtomized_RowMajorU)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeAsymmetricRationalBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_surface_bspline_t aView = OCCTL_SURFACE_BSPLINE_INIT;
  ASSERT_EQ(occtl_surface_as_bspline(aGraph, aId, &aView), OCCTL_OK);

  size_t aNb = 0;
  ASSERT_EQ(occtl_surface_bspline_poles(aGraph, aId, nullptr, 0, &aNb), OCCTL_OK);
  ASSERT_EQ(aNb, aView.u_pole_count * aView.v_pole_count);
  ASSERT_EQ(aNb, 20u);
  std::vector<occtl_point3_t> aAtoms(aNb);
  ASSERT_EQ(occtl_surface_bspline_poles(aGraph, aId, aAtoms.data(), aNb, &aNb), OCCTL_OK);

  const occtl_point3_t* aViewPtr = nullptr;
  size_t                aNbU = 0, aNbV = 0;
  ASSERT_EQ(occtl_surface_bspline_poles_view(aGraph, aId, &aViewPtr, &aNbU, &aNbV), OCCTL_OK);
  ASSERT_EQ(aNbU, aView.u_pole_count);
  ASSERT_EQ(aNbV, aView.v_pole_count);

  for (size_t aU = 0; aU < aView.u_pole_count; ++aU)
  {
    for (size_t aV = 0; aV < aView.v_pole_count; ++aV)
    {
      const size_t anIdx = aU * aView.v_pole_count + aV;
      EXPECT_NEAR(aView.poles[anIdx].x, aAtoms[anIdx].x, 1e-14);
      EXPECT_NEAR(aView.poles[anIdx].y, aAtoms[anIdx].y, 1e-14);
      EXPECT_NEAR(aView.poles[anIdx].z, aAtoms[anIdx].z, 1e-14);
      EXPECT_NEAR(aView.poles[anIdx].x, aViewPtr[anIdx].x, 1e-14);
      EXPECT_NEAR(aView.poles[anIdx].y, aViewPtr[anIdx].y, 1e-14);
      EXPECT_NEAR(aView.poles[anIdx].z, aViewPtr[anIdx].z, 1e-14);
    }
  }

  occtl_graph_free(aGraph);
}

TEST(BSplineSurfaceViewTest, Asymmetric_Rational_KnotsMultsFlatWeightsMatchAtomized)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeAsymmetricRationalBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_surface_bspline_t aView = OCCTL_SURFACE_BSPLINE_INIT;
  ASSERT_EQ(occtl_surface_as_bspline(aGraph, aId, &aView), OCCTL_OK);

  size_t              aNb = 0;
  std::vector<double> aUKnots(aView.u_knot_count);
  ASSERT_EQ(occtl_surface_bspline_u_knots(aGraph, aId, aUKnots.data(), aUKnots.size(), &aNb),
            OCCTL_OK);
  for (size_t anI = 0; anI < aView.u_knot_count; ++anI)
  {
    EXPECT_NEAR(aView.u_knots[anI], aUKnots[anI], 1e-14);
  }

  std::vector<double> aVKnots(aView.v_knot_count);
  ASSERT_EQ(occtl_surface_bspline_v_knots(aGraph, aId, aVKnots.data(), aVKnots.size(), &aNb),
            OCCTL_OK);
  for (size_t anI = 0; anI < aView.v_knot_count; ++anI)
  {
    EXPECT_NEAR(aView.v_knots[anI], aVKnots[anI], 1e-14);
  }

  std::vector<int32_t> aUMults(aView.u_knot_count);
  ASSERT_EQ(
    occtl_surface_bspline_u_multiplicities(aGraph, aId, aUMults.data(), aUMults.size(), &aNb),
    OCCTL_OK);
  for (size_t anI = 0; anI < aView.u_knot_count; ++anI)
  {
    EXPECT_EQ(aView.u_multiplicities[anI], aUMults[anI]);
  }

  std::vector<int32_t> aVMults(aView.v_knot_count);
  ASSERT_EQ(
    occtl_surface_bspline_v_multiplicities(aGraph, aId, aVMults.data(), aVMults.size(), &aNb),
    OCCTL_OK);
  for (size_t anI = 0; anI < aView.v_knot_count; ++anI)
  {
    EXPECT_EQ(aView.v_multiplicities[anI], aVMults[anI]);
  }

  std::vector<double> aUFlat(aView.u_flat_knot_count);
  ASSERT_EQ(occtl_surface_bspline_u_flat_knots(aGraph, aId, aUFlat.data(), aUFlat.size(), &aNb),
            OCCTL_OK);
  for (size_t anI = 0; anI < aView.u_flat_knot_count; ++anI)
  {
    EXPECT_NEAR(aView.u_flat_knots[anI], aUFlat[anI], 1e-14);
  }

  std::vector<double> aVFlat(aView.v_flat_knot_count);
  ASSERT_EQ(occtl_surface_bspline_v_flat_knots(aGraph, aId, aVFlat.data(), aVFlat.size(), &aNb),
            OCCTL_OK);
  for (size_t anI = 0; anI < aView.v_flat_knot_count; ++anI)
  {
    EXPECT_NEAR(aView.v_flat_knots[anI], aVFlat[anI], 1e-14);
  }

  ASSERT_NE(aView.weights, nullptr);
  std::vector<double> aWeights(aView.u_pole_count * aView.v_pole_count);
  ASSERT_EQ(occtl_surface_bspline_weights(aGraph, aId, aWeights.data(), aWeights.size(), &aNb),
            OCCTL_OK);
  for (size_t anI = 0; anI < aWeights.size(); ++anI)
  {
    EXPECT_NEAR(aView.weights[anI], aWeights[anI], 1e-14);
  }

  occtl_graph_free(aGraph);
}

TEST(BSplineSurfaceViewTest, NonRational_WeightsAreNull)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeSimpleBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_surface_bspline_t aView = OCCTL_SURFACE_BSPLINE_INIT;
  ASSERT_EQ(occtl_surface_as_bspline(aGraph, aId, &aView), OCCTL_OK);
  EXPECT_EQ(aView.is_rational, 0);
  EXPECT_EQ(aView.weights, nullptr);
  EXPECT_NE(aView.poles, nullptr);
  EXPECT_NE(aView.u_knots, nullptr);
  EXPECT_NE(aView.v_knots, nullptr);
  EXPECT_NE(aView.u_multiplicities, nullptr);
  EXPECT_NE(aView.v_multiplicities, nullptr);
  EXPECT_NE(aView.u_flat_knots, nullptr);
  EXPECT_NE(aView.v_flat_knots, nullptr);
  occtl_graph_free(aGraph);
}

TEST(BSplineSurfaceViewTest, Plane_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  occtl_surface_bspline_t aView = OCCTL_SURFACE_BSPLINE_INIT;
  EXPECT_EQ(occtl_surface_as_bspline(aGraph, aId, &aView), OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(BSplineSurfaceViewTest, Cylinder_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), 2.0};
  occtl_rep_id_t                         aId  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_OK);

  occtl_surface_bspline_t aView = OCCTL_SURFACE_BSPLINE_INIT;
  EXPECT_EQ(occtl_surface_as_bspline(aGraph, aId, &aView), OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(BSplineSurfaceViewTest, NullSurface_ReturnsInvalidArgument)
{
  occtl_surface_bspline_t aView = OCCTL_SURFACE_BSPLINE_INIT;
  EXPECT_EQ(occtl_surface_as_bspline(nullptr, OCCTL_REP_ID_INVALID, &aView),
            OCCTL_INVALID_ARGUMENT);
}

TEST(BSplineSurfaceViewTest, NullOut_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeSimpleBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);
  EXPECT_EQ(occtl_surface_as_bspline(aGraph, aId, nullptr), OCCTL_INVALID_ARGUMENT);
  occtl_graph_free(aGraph);
}

TEST(BSplineSurfaceViewTest, BogusVersion_ReturnsVersionMismatch)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeSimpleBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  occtl_surface_bspline_t aView = OCCTL_SURFACE_BSPLINE_INIT;
  aView.struct_version          = 999u;
  EXPECT_EQ(occtl_surface_as_bspline(aGraph, aId, &aView), OCCTL_VERSION_MISMATCH);
  occtl_graph_free(aGraph);
}

TEST(BSplinePolesViewTest, Asymmetric_Rational_MatchesAtomized_AndStablePointer)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeAsymmetricRationalBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  size_t aNbAtoms = 0;
  ASSERT_EQ(occtl_surface_bspline_poles(aGraph, aId, nullptr, 0, &aNbAtoms), OCCTL_OK);
  ASSERT_EQ(aNbAtoms, 20u);
  std::vector<occtl_point3_t> anAtoms(aNbAtoms);
  ASSERT_EQ(occtl_surface_bspline_poles(aGraph, aId, anAtoms.data(), aNbAtoms, &aNbAtoms),
            OCCTL_OK);

  const occtl_point3_t* aViewPtr = nullptr;
  size_t                aNbU = 0, aNbV = 0;
  ASSERT_EQ(occtl_surface_bspline_poles_view(aGraph, aId, &aViewPtr, &aNbU, &aNbV), OCCTL_OK);
  ASSERT_NE(aViewPtr, nullptr);
  ASSERT_EQ(aNbU, 4u);
  ASSERT_EQ(aNbV, 5u);
  ASSERT_EQ(aNbU * aNbV, aNbAtoms);

  for (size_t anI = 0; anI < aNbAtoms; ++anI)
  {
    EXPECT_NEAR(aViewPtr[anI].x, anAtoms[anI].x, 1e-14);
    EXPECT_NEAR(aViewPtr[anI].y, anAtoms[anI].y, 1e-14);
    EXPECT_NEAR(aViewPtr[anI].z, anAtoms[anI].z, 1e-14);
  }

  const occtl_point3_t* aViewPtr2 = nullptr;
  size_t                aNbU2 = 0, aNbV2 = 0;
  ASSERT_EQ(occtl_surface_bspline_poles_view(aGraph, aId, &aViewPtr2, &aNbU2, &aNbV2), OCCTL_OK);
  EXPECT_EQ(aViewPtr2, aViewPtr);
  EXPECT_EQ(aNbU2, aNbU);
  EXPECT_EQ(aNbV2, aNbV);

  int32_t aDegU = 0, aDegV = 0;
  ASSERT_EQ(occtl_surface_bspline_u_degree(aGraph, aId, &aDegU), OCCTL_OK);
  EXPECT_EQ(aDegU, 3);
  ASSERT_EQ(occtl_surface_bspline_v_degree(aGraph, aId, &aDegV), OCCTL_OK);
  EXPECT_EQ(aDegV, 2);
  int32_t aRational = 0;
  ASSERT_EQ(occtl_surface_bspline_is_rational(aGraph, aId, &aRational), OCCTL_OK);
  EXPECT_EQ(aRational, 1);
  for (size_t anI = 0; anI < aNbAtoms; ++anI)
  {
    EXPECT_NEAR(aViewPtr[anI].x, anAtoms[anI].x, 1e-14);
    EXPECT_NEAR(aViewPtr[anI].y, anAtoms[anI].y, 1e-14);
    EXPECT_NEAR(aViewPtr[anI].z, anAtoms[anI].z, 1e-14);
  }

  occtl_graph_free(aGraph);
}

TEST(BSplinePolesViewTest, NullSurface_ReturnsInvalidArgument)
{
  const occtl_point3_t* aViewPtr = nullptr;
  size_t                aNbU = 0, aNbV = 0;
  EXPECT_EQ(
    occtl_surface_bspline_poles_view(nullptr, OCCTL_REP_ID_INVALID, &aViewPtr, &aNbU, &aNbV),
    OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aViewPtr, nullptr);
}

TEST(BSplinePolesViewTest, NullOutPointers_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  occtl_rep_id_t aId = MakeSimpleBSplineSurface(aGraph);
  ASSERT_NE(aId.bits, 0u);

  const occtl_point3_t* aViewPtr = nullptr;
  size_t                aNbU = 0, aNbV = 0;
  EXPECT_EQ(occtl_surface_bspline_poles_view(aGraph, aId, nullptr, &aNbU, &aNbV),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_surface_bspline_poles_view(aGraph, aId, &aViewPtr, nullptr, &aNbV),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_surface_bspline_poles_view(aGraph, aId, &aViewPtr, &aNbU, nullptr),
            OCCTL_INVALID_ARGUMENT);
  occtl_graph_free(aGraph);
}

TEST(BSplinePolesViewTest, Plane_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  const occtl_point3_t* aViewPtr = nullptr;
  size_t                aNbU = 0, aNbV = 0;
  EXPECT_EQ(occtl_surface_bspline_poles_view(aGraph, aId, &aViewPtr, &aNbU, &aNbV),
            OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(BSplinePolesViewTest, Cylinder_ReturnsWrongKind)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), 2.0};
  occtl_rep_id_t                         aId  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_OK);

  const occtl_point3_t* aViewPtr = nullptr;
  size_t                aNbU = 0, aNbV = 0;
  EXPECT_EQ(occtl_surface_bspline_poles_view(aGraph, aId, &aViewPtr, &aNbU, &aNbV),
            OCCTL_WRONG_KIND);
  occtl_graph_free(aGraph);
}

TEST(SurfacesInvalidParamTest, Sphere_NegativeRadius_ReturnsGeometryInvalid)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_spherical_surface_t aSphere = {MakeStandardAxis3(), -1.0};
  occtl_rep_id_t                       aId     = {};
  EXPECT_EQ(occtl_surface_create_sphere(aGraph, &aId, aSphere), OCCTL_GEOMETRY_INVALID);
  occtl_graph_free(aGraph);
}

TEST(SurfacesInvalidParamTest, Cone_NegativeSemiAngle_ReturnsGeometryInvalid)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_conical_surface_t aCone = {MakeStandardAxis3(), -0.5, 2.0};
  occtl_rep_id_t                     aId   = {};
  EXPECT_EQ(occtl_surface_create_cone(aGraph, &aId, aCone), OCCTL_GEOMETRY_INVALID);
  occtl_graph_free(aGraph);
}

TEST(SurfacesInvalidParamTest, Torus_MinorGreaterThanMajor_ReturnsGeometryInvalid)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_toroidal_surface_t aTorus = {MakeStandardAxis3(), 1.0, 2.0};
  occtl_rep_id_t                      aId    = {};
  EXPECT_EQ(occtl_surface_create_torus(aGraph, &aId, aTorus), OCCTL_GEOMETRY_INVALID);
  occtl_graph_free(aGraph);
}

TEST(SurfacesParameterRangeTest, Cylinder_URangeIsFullCircle)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), 2.0};
  occtl_rep_id_t                         aId  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_OK);
  double aUMin = 0.0, aUMax = 0.0;
  ASSERT_EQ(occtl_surface_parameter_range(aGraph, aId, &aUMin, &aUMax, nullptr, nullptr), OCCTL_OK);
  EXPECT_NEAR(aUMin, 0.0, 1e-14);
  EXPECT_NEAR(aUMax, 2.0 * M_PI, 1e-12);
  occtl_graph_free(aGraph);
}

TEST(SurfacesParameterRangeTest, Cone_URangeIsFullCircle)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_conical_surface_t aCone = {MakeStandardAxis3(), 0.5, 2.0};
  occtl_rep_id_t                     aId   = {};
  ASSERT_EQ(occtl_surface_create_cone(aGraph, &aId, aCone), OCCTL_OK);
  double aUMin = 0.0, aUMax = 0.0;
  ASSERT_EQ(occtl_surface_parameter_range(aGraph, aId, &aUMin, &aUMax, nullptr, nullptr), OCCTL_OK);
  EXPECT_NEAR(aUMin, 0.0, 1e-14);
  EXPECT_NEAR(aUMax, 2.0 * M_PI, 1e-12);
  occtl_graph_free(aGraph);
}

TEST(SurfacesParameterRangeTest, Sphere_URangeIsFullCircle_VRangeIsHalfCircle)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_spherical_surface_t aSphere = {MakeStandardAxis3(), 3.0};
  occtl_rep_id_t                       aId     = {};
  ASSERT_EQ(occtl_surface_create_sphere(aGraph, &aId, aSphere), OCCTL_OK);
  double aVMin = 0.0, aVMax = 0.0;
  ASSERT_EQ(occtl_surface_parameter_range(aGraph, aId, nullptr, nullptr, &aVMin, &aVMax), OCCTL_OK);
  EXPECT_NEAR(aVMin, -M_PI / 2.0, 1e-12);
  EXPECT_NEAR(aVMax, M_PI / 2.0, 1e-12);
  occtl_graph_free(aGraph);
}

TEST(SurfacesContinuityTest, Plane_IsCN)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);
  occtl_geom_continuity_t aCont;
  ASSERT_EQ(occtl_surface_continuity(aGraph, aId, &aCont), OCCTL_OK);
  EXPECT_EQ(aCont, OCCTL_GEOM_CONTINUITY_CN);
  occtl_graph_free(aGraph);
}

TEST(SurfacesContinuityTest, Cylinder_IsCN)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), 2.0};
  occtl_rep_id_t                         aId  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_OK);
  occtl_geom_continuity_t aCont;
  ASSERT_EQ(occtl_surface_continuity(aGraph, aId, &aCont), OCCTL_OK);
  EXPECT_EQ(aCont, OCCTL_GEOM_CONTINUITY_CN);
  occtl_graph_free(aGraph);
}

TEST(SurfacesContinuityTest, Sphere_IsCN)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_spherical_surface_t aSphere = {MakeStandardAxis3(), 3.0};
  occtl_rep_id_t                       aId     = {};
  ASSERT_EQ(occtl_surface_create_sphere(aGraph, &aId, aSphere), OCCTL_OK);
  occtl_geom_continuity_t aCont;
  ASSERT_EQ(occtl_surface_continuity(aGraph, aId, &aCont), OCCTL_OK);
  EXPECT_EQ(aCont, OCCTL_GEOM_CONTINUITY_CN);
  occtl_graph_free(aGraph);
}

TEST(SurfacesContinuityTest, Torus_IsCN)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_toroidal_surface_t aTorus = {MakeStandardAxis3(), 5.0, 2.0};
  occtl_rep_id_t                      aId    = {};
  ASSERT_EQ(occtl_surface_create_torus(aGraph, &aId, aTorus), OCCTL_OK);
  occtl_geom_continuity_t aCont;
  ASSERT_EQ(occtl_surface_continuity(aGraph, aId, &aCont), OCCTL_OK);
  EXPECT_EQ(aCont, OCCTL_GEOM_CONTINUITY_CN);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCompoundTest, Revolution_KindIsRevolution)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_line_t aLine  = {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
  occtl_rep_id_t          aBasis = {};
  ASSERT_EQ(occtl_curve_create_line(aGraph, aLine, &aBasis), OCCTL_OK);

  occtl_surface_revolution_create_info_t aInfo;
  occtl_surface_revolution_create_info_init(&aInfo);
  aInfo.basis = aBasis;
  aInfo.axis  = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_revolution(aGraph, &aId, &aInfo), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_REVOLUTION);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCompoundTest, RevolutionZeroAxisDirection_ReturnsGeometryInvalid)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_line_t aLine  = {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
  occtl_rep_id_t          aBasis = {};
  ASSERT_EQ(occtl_curve_create_line(aGraph, aLine, &aBasis), OCCTL_OK);

  occtl_surface_revolution_create_info_t aInfo = OCCTL_SURFACE_REVOLUTION_CREATE_INFO_INIT;
  aInfo.basis                                  = aBasis;
  aInfo.axis                                   = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_revolution(aGraph, &aId, &aInfo), OCCTL_GEOMETRY_INVALID);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCompoundTest, RevolutionPNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_line_t aLine  = {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
  occtl_rep_id_t          aBasis = {};
  ASSERT_EQ(occtl_curve_create_line(aGraph, aLine, &aBasis), OCCTL_OK);

  occtl_surface_revolution_create_info_t aInfo = OCCTL_SURFACE_REVOLUTION_CREATE_INFO_INIT;
  aInfo.p_next                                 = reinterpret_cast<const void*>(0x1);
  aInfo.basis                                  = aBasis;
  aInfo.axis                                   = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_revolution(aGraph, &aId, &aInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCompoundTest, Extrusion_KindIsExtrusion)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_line_t aLine  = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aBasis = {};
  ASSERT_EQ(occtl_curve_create_line(aGraph, aLine, &aBasis), OCCTL_OK);

  occtl_surface_extrusion_create_info_t aInfo;
  occtl_surface_extrusion_create_info_init(&aInfo);
  aInfo.basis     = aBasis;
  aInfo.direction = {0.0, 0.0, 1.0};

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_extrusion(aGraph, &aId, &aInfo), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_EXTRUSION);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCompoundTest, ExtrusionPNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_line_t aLine  = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_rep_id_t          aBasis = {};
  ASSERT_EQ(occtl_curve_create_line(aGraph, aLine, &aBasis), OCCTL_OK);

  occtl_surface_extrusion_create_info_t aInfo = OCCTL_SURFACE_EXTRUSION_CREATE_INFO_INIT;
  aInfo.p_next                                = reinterpret_cast<const void*>(0x1);
  aInfo.basis                                 = aBasis;
  aInfo.direction                             = {0.0, 0.0, 1.0};

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_extrusion(aGraph, &aId, &aInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCompoundTest, RectangularTrimmed_KindIsRectangularTrimmed)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aBasis = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aBasis, aPlane), OCCTL_OK);

  occtl_surface_rectangular_trimmed_create_info_t aInfo;
  occtl_surface_rectangular_trimmed_create_info_init(&aInfo);
  aInfo.basis   = aBasis;
  aInfo.u_first = -1.0;
  aInfo.u_last  = 1.0;
  aInfo.v_first = -2.0;
  aInfo.v_last  = 2.0;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_rectangular_trimmed(aGraph, &aId, &aInfo), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_RECTANGULAR_TRIMMED);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCompoundTest, RectangularTrimmedPNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aBasis = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aBasis, aPlane), OCCTL_OK);

  occtl_surface_rectangular_trimmed_create_info_t aInfo =
    OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_INIT;
  aInfo.p_next  = reinterpret_cast<const void*>(0x1);
  aInfo.basis   = aBasis;
  aInfo.u_first = -1.0;
  aInfo.u_last  = 1.0;
  aInfo.v_first = -2.0;
  aInfo.v_last  = 2.0;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_rectangular_trimmed(aGraph, &aId, &aInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCompoundTest, Offset_KindIsOffset)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aBasis = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aBasis, aPlane), OCCTL_OK);

  occtl_surface_offset_create_info_t aInfo;
  occtl_surface_offset_create_info_init(&aInfo);
  aInfo.basis  = aBasis;
  aInfo.offset = 5.0;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_offset(aGraph, &aId, &aInfo), OCCTL_OK);
  occtl_surface_kind_t aKind;
  ASSERT_EQ(occtl_surface_kind(aGraph, aId, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_SURFACE_KIND_OFFSET);
  occtl_graph_free(aGraph);
}

TEST(SurfacesCompoundTest, OffsetPNextNotNull_ReturnsInvalidArgument)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aBasis = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aBasis, aPlane), OCCTL_OK);

  occtl_surface_offset_create_info_t aInfo = OCCTL_SURFACE_OFFSET_CREATE_INFO_INIT;
  aInfo.p_next                             = reinterpret_cast<const void*>(0x1);
  aInfo.basis                              = aBasis;
  aInfo.offset                             = 5.0;

  occtl_rep_id_t aId = {};
  EXPECT_EQ(occtl_surface_create_offset(aGraph, &aId, &aInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aId.bits, 0u);
  occtl_graph_free(aGraph);
}

TEST(SurfacesIntrospectionTest, Cylinder_IsUPeriodic)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), 2.0};
  occtl_rep_id_t                         aId  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_OK);
  int32_t aVal = 0;
  ASSERT_EQ(occtl_surface_is_u_periodic(aGraph, aId, &aVal), OCCTL_OK);
  EXPECT_EQ(aVal, 1);
  occtl_graph_free(aGraph);
}

TEST(SurfacesIntrospectionTest, Plane_ParameterRangeIsInfinite)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);
  int32_t aUPer = 0, aVPer = 0;
  ASSERT_EQ(occtl_surface_is_u_periodic(aGraph, aId, &aUPer), OCCTL_OK);
  EXPECT_EQ(aUPer, 0);
  ASSERT_EQ(occtl_surface_is_v_periodic(aGraph, aId, &aVPer), OCCTL_OK);
  EXPECT_EQ(aVPer, 0);
  occtl_graph_free(aGraph);
}

TEST(SurfacesAreaTest, Sphere_UsesBoundedDomain)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_spherical_surface_t aSphere = {MakeStandardAxis3(), 1.0};
  occtl_rep_id_t                       aId     = {};
  ASSERT_EQ(occtl_surface_create_sphere(aGraph, &aId, aSphere), OCCTL_OK);

  double anArea = 0.0;
  ASSERT_EQ(occtl_surface_area(aGraph, aId, &anArea), OCCTL_OK);
  EXPECT_NEAR(anArea, 4.0 * M_PI, 1.0e-9);
  occtl_graph_free(aGraph);
}

TEST(SurfacesAreaTest, TrimmedCylinder_ComputesSideArea)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const double                           aRadius = 2.0;
  const double                           aHeight = 3.0;
  const occtl_geom_cylindrical_surface_t aCyl    = {MakeStandardAxis3(), aRadius};
  occtl_rep_id_t                         aBasis  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aBasis, aCyl), OCCTL_OK);

  occtl_surface_rectangular_trimmed_create_info_t aInfo =
    OCCTL_SURFACE_RECTANGULAR_TRIMMED_CREATE_INFO_INIT;
  aInfo.basis   = aBasis;
  aInfo.u_first = 0.0;
  aInfo.u_last  = 2.0 * M_PI;
  aInfo.v_first = 0.0;
  aInfo.v_last  = aHeight;

  occtl_rep_id_t aTrimmed = {};
  ASSERT_EQ(occtl_surface_create_rectangular_trimmed(aGraph, &aTrimmed, &aInfo), OCCTL_OK);

  double anArea = 0.0;
  ASSERT_EQ(occtl_surface_area(aGraph, aTrimmed, &anArea), OCCTL_OK);
  EXPECT_NEAR(anArea, 2.0 * M_PI * aRadius * aHeight, 1.0e-9);
  occtl_graph_free(aGraph);
}

TEST(SurfacesAreaTest, Plane_ReturnsGeometryInvalid)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  double anArea = 0.0;
  EXPECT_EQ(occtl_surface_area(aGraph, aId, &anArea), OCCTL_GEOMETRY_INVALID);
  const occtl_error_t* const anError = occtl_error_last();
  ASSERT_NE(anError, nullptr);
  ASSERT_NE(anError->message, nullptr);
  EXPECT_NE(anError->message[0], '\0');
  occtl_graph_free(aGraph);
}

TEST(SurfacesEvalTest, D0_Plane_AtOriginReturnsLocation)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  occtl_point3_t aP{};
  ASSERT_EQ(occtl_surface_eval_d0(aGraph, aId, 0.0, 0.0, &aP), OCCTL_OK);
  EXPECT_NEAR(aP.x, 0.0, 1e-14);
  EXPECT_NEAR(aP.y, 0.0, 1e-14);
  EXPECT_NEAR(aP.z, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesEvalTest, D1_Plane_PartialsAreAxes)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  occtl_point3_t  aP{};
  occtl_vector3_t aD1U{}, aD1V{};
  ASSERT_EQ(occtl_surface_eval_d1(aGraph, aId, 0.0, 0.0, &aP, &aD1U, &aD1V), OCCTL_OK);
  EXPECT_NEAR(aD1U.x, 1.0, 1e-14);
  EXPECT_NEAR(aD1U.y, 0.0, 1e-14);
  EXPECT_NEAR(aD1V.x, 0.0, 1e-14);
  EXPECT_NEAR(aD1V.y, 1.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesEvalTest, D0_Cylinder_AtURadiusReturnsPointOnSurface)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_cylindrical_surface_t aCyl = {MakeStandardAxis3(), 2.0};
  occtl_rep_id_t                         aId  = {};
  ASSERT_EQ(occtl_surface_create_cylinder(aGraph, &aId, aCyl), OCCTL_OK);

  occtl_point3_t aP{};
  ASSERT_EQ(occtl_surface_eval_d0(aGraph, aId, 0.0, 0.0, &aP), OCCTL_OK);
  EXPECT_NEAR(aP.x, 2.0, 1e-14);
  EXPECT_NEAR(aP.y, 0.0, 1e-14);
  EXPECT_NEAR(aP.z, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesEvalTest, D0_Sphere_AtNorthPole)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_spherical_surface_t aSphere = {MakeStandardAxis3(), 1.0};
  occtl_rep_id_t                       aId     = {};
  ASSERT_EQ(occtl_surface_create_sphere(aGraph, &aId, aSphere), OCCTL_OK);

  occtl_point3_t aP{};
  ASSERT_EQ(occtl_surface_eval_d0(aGraph, aId, 0.0, M_PI_2, &aP), OCCTL_OK);
  EXPECT_NEAR(aP.x, 0.0, 1e-14);
  EXPECT_NEAR(aP.y, 0.0, 1e-14);
  EXPECT_NEAR(aP.z, 1.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesEvalTest, NullSurf_ReturnsInvalidArgument)
{
  occtl_point3_t aP{};
  EXPECT_EQ(occtl_surface_eval_d0(nullptr, OCCTL_REP_ID_INVALID, 0.0, 0.0, &aP),
            OCCTL_INVALID_ARGUMENT);
}

TEST(SurfacesEvalTest, BSpline_SurfaceEvaluationSucceeds)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_point3_t aPoles[] = {
    {0.0, 0.0, 0.0},
    {1.0, 0.0, 0.0},
    {2.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {1.0, 1.0, 1.0},
    {2.0, 1.0, 0.0},
  };
  const double                        aUKnots[] = {0.0, 1.0};
  const double                        aVKnots[] = {0.0, 1.0};
  const int32_t                       aUMults[] = {3, 3};
  const int32_t                       aVMults[] = {2, 2};
  occtl_surface_bspline_create_info_t aInfo     = OCCTL_SURFACE_BSPLINE_CREATE_INFO_INIT;
  aInfo.poles                                   = aPoles;
  aInfo.u_pole_count                            = 3;
  aInfo.v_pole_count                            = 2;
  aInfo.u_knots                                 = aUKnots;
  aInfo.u_multiplicities                        = aUMults;
  aInfo.u_knot_count                            = 2;
  aInfo.v_knots                                 = aVKnots;
  aInfo.v_multiplicities                        = aVMults;
  aInfo.v_knot_count                            = 2;
  aInfo.u_degree                                = 2;
  aInfo.v_degree                                = 1;

  occtl_rep_id_t aId = {};
  ASSERT_EQ(occtl_surface_create_bspline(aGraph, &aId, &aInfo), OCCTL_OK);

  occtl_point3_t aP{};
  ASSERT_EQ(occtl_surface_eval_d0(aGraph, aId, 0.0, 0.0, &aP), OCCTL_OK);
  EXPECT_NEAR(aP.x, 0.0, 1e-14);
  EXPECT_NEAR(aP.y, 0.0, 1e-14);
  EXPECT_NEAR(aP.z, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesEvalTest, D2_Plane_SecondDerivsAreZero)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  occtl_point3_t  aP{};
  occtl_vector3_t aD1U{}, aD1V{}, aD2U{}, aD2V{}, aD2UV{};
  ASSERT_EQ(occtl_surface_eval_d2(aGraph, aId, 0.0, 0.0, &aP, &aD1U, &aD1V, &aD2U, &aD2V, &aD2UV),
            OCCTL_OK);
  EXPECT_NEAR(aD2U.x, 0.0, 1e-14);
  EXPECT_NEAR(aD2U.y, 0.0, 1e-14);
  EXPECT_NEAR(aD2U.z, 0.0, 1e-14);
  EXPECT_NEAR(aD2V.x, 0.0, 1e-14);
  EXPECT_NEAR(aD2V.y, 0.0, 1e-14);
  EXPECT_NEAR(aD2V.z, 0.0, 1e-14);
  EXPECT_NEAR(aD2UV.x, 0.0, 1e-14);
  EXPECT_NEAR(aD2UV.y, 0.0, 1e-14);
  EXPECT_NEAR(aD2UV.z, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesEvalTest, D3_Plane_ThirdDerivsAreZero)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  occtl_point3_t  aP{};
  occtl_vector3_t aD1U{}, aD1V{}, aD2U{}, aD2V{}, aD2UV{};
  occtl_vector3_t aD3U{}, aD3V{}, aD3UUV{}, aD3UVV{};
  ASSERT_EQ(occtl_surface_eval_d3(aGraph,
                                  aId,
                                  0.0,
                                  0.0,
                                  &aP,
                                  &aD1U,
                                  &aD1V,
                                  &aD2U,
                                  &aD2V,
                                  &aD2UV,
                                  &aD3U,
                                  &aD3V,
                                  &aD3UUV,
                                  &aD3UVV),
            OCCTL_OK);
  EXPECT_NEAR(aD3U.x, 0.0, 1e-14);
  EXPECT_NEAR(aD3U.y, 0.0, 1e-14);
  EXPECT_NEAR(aD3U.z, 0.0, 1e-14);
  EXPECT_NEAR(aD3V.x, 0.0, 1e-14);
  EXPECT_NEAR(aD3V.y, 0.0, 1e-14);
  EXPECT_NEAR(aD3V.z, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesEvalTest, DN_Plane_CrossDerivativeIsZero)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);
  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId    = {};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  occtl_vector3_t aD{};
  ASSERT_EQ(occtl_surface_eval_dn(aGraph, aId, 0.0, 0.0, 1, 1, &aD), OCCTL_OK);
  EXPECT_NEAR(aD.x, 0.0, 1e-14);
  EXPECT_NEAR(aD.y, 0.0, 1e-14);
  EXPECT_NEAR(aD.z, 0.0, 1e-14);
  occtl_graph_free(aGraph);
}

TEST(SurfacesEvalTest, DN_NullSurf_ReturnsInvalidArgument)
{
  occtl_vector3_t aD{};
  EXPECT_EQ(occtl_surface_eval_dn(nullptr, OCCTL_REP_ID_INVALID, 0.0, 0.0, 0, 1, &aD),
            OCCTL_INVALID_ARGUMENT);
}

TEST(SurfacesEvalTest, D2_NullSurf_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_surface_eval_d2(nullptr,
                                  OCCTL_REP_ID_INVALID,
                                  0.0,
                                  0.0,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST(SurfacesProjectionTest, Plane_ProjectPointAndUVOfPoint_AreConsistent)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId{};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  double aU        = 0.0;
  double aV        = 0.0;
  double aDistance = 0.0;
  ASSERT_EQ(OCCTL_OK,
            occtl_surface_project_point(aGraph, aId, {1.0, 2.0, 5.0}, &aU, &aV, &aDistance));
  EXPECT_NEAR(aU, 1.0, 1.0e-12);
  EXPECT_NEAR(aV, 2.0, 1.0e-12);
  EXPECT_NEAR(aDistance, 5.0, 1.0e-12);

  double aNearestU = 0.0;
  double aNearestV = 0.0;
  ASSERT_EQ(OCCTL_OK,
            occtl_surface_uv_of_point(aGraph, aId, {3.0, 4.0, 0.0}, &aNearestU, &aNearestV));
  EXPECT_NEAR(aNearestU, 3.0, 1.0e-12);
  EXPECT_NEAR(aNearestV, 4.0, 1.0e-12);

  occtl_graph_free(aGraph);
}

TEST(SurfacesTransformTest, ReverseTranslateRotateAndScale_ReturnUsableSurfaces)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId{};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  occtl_rep_id_t aReversed{};
  ASSERT_EQ(OCCTL_OK, occtl_surface_reverse(aGraph, aId, &aReversed));
  occtl_point3_t aP{};
  ASSERT_EQ(OCCTL_OK, occtl_surface_eval_d0(aGraph, aReversed, 1.0, 2.0, &aP));
  EXPECT_NEAR(aP.x, -1.0, 1.0e-12);
  EXPECT_NEAR(aP.y, 2.0, 1.0e-12);

  occtl_rep_id_t aTranslated{};
  ASSERT_EQ(OCCTL_OK, occtl_surface_translated(aGraph, aId, {0.0, 0.0, 5.0}, &aTranslated));
  ASSERT_EQ(OCCTL_OK, occtl_surface_eval_d0(aGraph, aTranslated, 1.0, 2.0, &aP));
  EXPECT_NEAR(aP.x, 1.0, 1.0e-12);
  EXPECT_NEAR(aP.y, 2.0, 1.0e-12);
  EXPECT_NEAR(aP.z, 5.0, 1.0e-12);

  occtl_rep_id_t aRotated{};
  ASSERT_EQ(
    OCCTL_OK,
    occtl_surface_rotated(aGraph, aId, {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}}, M_PI_2, &aRotated));
  ASSERT_EQ(OCCTL_OK, occtl_surface_eval_d0(aGraph, aRotated, 1.0, 0.0, &aP));
  EXPECT_NEAR(aP.x, 0.0, 1.0e-12);
  EXPECT_NEAR(aP.y, 1.0, 1.0e-12);

  occtl_rep_id_t aScaled{};
  ASSERT_EQ(OCCTL_OK, occtl_surface_scaled(aGraph, aId, {0.0, 0.0, 0.0}, 2.0, &aScaled));
  occtl_surface_kind_t aKind = OCCTL_SURFACE_KIND_UNDEFINED;
  ASSERT_EQ(OCCTL_OK, occtl_surface_kind(aGraph, aScaled, &aKind));
  EXPECT_EQ(OCCTL_SURFACE_KIND_PLANE, aKind);

  occtl_graph_free(aGraph);
}

TEST(SurfacesTransformTest, RoundTripIdentity_ForReverseTranslateRotateScale)
{
  occtl_graph_t* aGraph = nullptr;
  ASSERT_EQ(occtl_graph_create(&aGraph), OCCTL_OK);

  const occtl_geom_plane_t aPlane = {MakeStandardAxis3()};
  occtl_rep_id_t           aId{};
  ASSERT_EQ(occtl_surface_create_plane(aGraph, &aId, aPlane), OCCTL_OK);

  occtl_point3_t aBasePoint{};
  ASSERT_EQ(OCCTL_OK, occtl_surface_eval_d0(aGraph, aId, 1.25, -0.75, &aBasePoint));

  const auto expectSameAtParameters = [&](const occtl_rep_id_t theId) {
    occtl_point3_t aPoint{};
    ASSERT_EQ(OCCTL_OK, occtl_surface_eval_d0(aGraph, theId, 1.25, -0.75, &aPoint));
    EXPECT_NEAR(aPoint.x, aBasePoint.x, 1.0e-12);
    EXPECT_NEAR(aPoint.y, aBasePoint.y, 1.0e-12);
    EXPECT_NEAR(aPoint.z, aBasePoint.z, 1.0e-12);
  };

  occtl_rep_id_t aReverse1{};
  occtl_rep_id_t aReverse2{};
  ASSERT_EQ(OCCTL_OK, occtl_surface_reverse(aGraph, aId, &aReverse1));
  ASSERT_EQ(OCCTL_OK, occtl_surface_reverse(aGraph, aReverse1, &aReverse2));
  expectSameAtParameters(aReverse2);

  occtl_rep_id_t aTranslated{};
  occtl_rep_id_t aTranslatedBack{};
  ASSERT_EQ(OCCTL_OK, occtl_surface_translated(aGraph, aId, {0.5, -1.0, 2.0}, &aTranslated));
  ASSERT_EQ(OCCTL_OK,
            occtl_surface_translated(aGraph, aTranslated, {-0.5, 1.0, -2.0}, &aTranslatedBack));
  expectSameAtParameters(aTranslatedBack);

  occtl_rep_id_t aRotated{};
  occtl_rep_id_t aRotatedBack{};
  ASSERT_EQ(
    OCCTL_OK,
    occtl_surface_rotated(aGraph, aId, {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}}, M_PI / 4.0, &aRotated));
  ASSERT_EQ(OCCTL_OK,
            occtl_surface_rotated(aGraph,
                                  aRotated,
                                  {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}},
                                  -M_PI / 4.0,
                                  &aRotatedBack));
  expectSameAtParameters(aRotatedBack);

  occtl_rep_id_t aScaled{};
  occtl_rep_id_t aScaledBack{};
  ASSERT_EQ(OCCTL_OK, occtl_surface_scaled(aGraph, aId, {1.0, -2.0, 0.5}, 3.0, &aScaled));
  ASSERT_EQ(OCCTL_OK,
            occtl_surface_scaled(aGraph, aScaled, {1.0, -2.0, 0.5}, 1.0 / 3.0, &aScaledBack));
  expectSameAtParameters(aScaledBack);

  occtl_graph_free(aGraph);
}
