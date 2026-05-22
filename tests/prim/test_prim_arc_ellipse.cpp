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

#include <cstring>

namespace
{

class PrimArcEllipseTest : public ::testing::Test
{
protected:
  void SetUp() override { ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK); }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(PrimArcEllipseTest, ArcEllipse_QuarterEllipse_CreatesWire)
{
  occtl_geom_ellipse_t anEllipse;
  anEllipse.position        = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  anEllipse.major_radius    = 3.0;
  anEllipse.minor_radius    = 1.5;
  occtl_rep_id_t aFullCurve = {};
  ASSERT_EQ(occtl_curve_create_ellipse(myGraph, anEllipse, &aFullCurve), OCCTL_OK);

  occtl_curve_trimmed_create_info_t aTrimInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aTrimInfo.basis                             = aFullCurve;
  aTrimInfo.u_first                           = 0.0;
  aTrimInfo.u_last                            = 1.5707963267948966; // pi/2
  aTrimInfo.sense                             = 1;
  occtl_rep_id_t aTrimmedCurve                = {};
  ASSERT_EQ(occtl_curve_create_trimmed(myGraph, &aTrimInfo, &aTrimmedCurve), OCCTL_OK);

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_curves_to_wire(myGraph, &aTrimmedCurve, 1, &aWire), OCCTL_OK);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
}

TEST_F(PrimArcEllipseTest, ArcEllipse_MinorAboveMajor_GeometryInvalid)
{
  occtl_geom_ellipse_t anEllipse;
  anEllipse.position     = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  anEllipse.major_radius = 1.0;
  anEllipse.minor_radius = 2.0;
  occtl_rep_id_t aCurve  = {};
  EXPECT_EQ(occtl_curve_create_ellipse(myGraph, anEllipse, &aCurve), OCCTL_GEOMETRY_INVALID);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(PrimArcEllipseTest, ArcEllipse_EndAngleNotGreater_GeometryInvalid)
{
  occtl_geom_ellipse_t anEllipse;
  anEllipse.position        = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  anEllipse.major_radius    = 3.0;
  anEllipse.minor_radius    = 1.5;
  occtl_rep_id_t aFullCurve = {};
  ASSERT_EQ(occtl_curve_create_ellipse(myGraph, anEllipse, &aFullCurve), OCCTL_OK);

  occtl_curve_trimmed_create_info_t aTrimInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aTrimInfo.basis                             = aFullCurve;
  aTrimInfo.u_first                           = 1.0;
  aTrimInfo.u_last                            = 1.0;
  aTrimInfo.sense                             = 1;
  occtl_rep_id_t aTrimmedCurve                = {};
  EXPECT_EQ(occtl_curve_create_trimmed(myGraph, &aTrimInfo, &aTrimmedCurve),
            OCCTL_GEOMETRY_INVALID);
}

TEST_F(PrimArcEllipseTest, ArcEllipse_NullOutCurve_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_curve_create_ellipse(myGraph, {}, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimArcEllipseTest, ArcEllipse_VersionMismatch_Rejected)
{
  occtl_geom_ellipse_t anEllipse;
  anEllipse.position        = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  anEllipse.major_radius    = 3.0;
  anEllipse.minor_radius    = 1.5;
  occtl_rep_id_t aFullCurve = {};
  ASSERT_EQ(occtl_curve_create_ellipse(myGraph, anEllipse, &aFullCurve), OCCTL_OK);

  occtl_curve_trimmed_create_info_t aTrimInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aTrimInfo.struct_version                    = 0u;
  aTrimInfo.basis                             = aFullCurve;
  aTrimInfo.u_first                           = 0.0;
  aTrimInfo.u_last                            = 1.0;
  aTrimInfo.sense                             = 1;
  occtl_rep_id_t aTrimmedCurve                = {};
  EXPECT_EQ(occtl_curve_create_trimmed(myGraph, &aTrimInfo, &aTrimmedCurve),
            OCCTL_VERSION_MISMATCH);
}

class PrimBezierTest : public ::testing::Test
{
protected:
  void SetUp() override { ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK); }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(PrimBezierTest, Bezier_CubicNonRational_CreatesWire)
{
  const occtl_point3_t             aPoles[4] = {{0, 0, 0}, {1, 2, 0}, {3, 2, 0}, {4, 0, 0}};
  occtl_curve_bezier_create_info_t aBezInfo  = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
  aBezInfo.poles                             = aPoles;
  aBezInfo.pole_count                        = 4;

  occtl_rep_id_t aCurve = {};
  ASSERT_EQ(occtl_curve_create_bezier(myGraph, &aBezInfo, &aCurve), OCCTL_OK);

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_curves_to_wire(myGraph, &aCurve, 1, &aWire), OCCTL_OK);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
}

TEST_F(PrimBezierTest, Bezier_Rational_CreatesWire)
{
  const occtl_point3_t             aPoles[3]   = {{0, 0, 0}, {1, 1, 0}, {2, 0, 0}};
  const double                     aWeights[3] = {1.0, 0.5, 1.0};
  occtl_curve_bezier_create_info_t aBezInfo    = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
  aBezInfo.poles                               = aPoles;
  aBezInfo.pole_count                          = 3;
  aBezInfo.weights                             = aWeights;

  occtl_rep_id_t aCurve = {};
  ASSERT_EQ(occtl_curve_create_bezier(myGraph, &aBezInfo, &aCurve), OCCTL_OK);

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_curves_to_wire(myGraph, &aCurve, 1, &aWire), OCCTL_OK);
}

TEST_F(PrimBezierTest, Bezier_TooFewPoles_InvalidArgument)
{
  const occtl_point3_t             aPoles[1] = {{0, 0, 0}};
  occtl_curve_bezier_create_info_t aBezInfo  = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
  aBezInfo.poles                             = aPoles;
  aBezInfo.pole_count                        = 1;
  occtl_rep_id_t aCurve                      = {};
  EXPECT_EQ(occtl_curve_create_bezier(myGraph, &aBezInfo, &aCurve), OCCTL_GEOMETRY_INVALID);
}

TEST_F(PrimBezierTest, Bezier_NullPointers_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_curve_create_bezier(nullptr, nullptr, nullptr), OCCTL_INVALID_ARGUMENT);
  occtl_curve_bezier_create_info_t aBezInfo = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
  EXPECT_EQ(occtl_curve_create_bezier(nullptr, &aBezInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimBezierTest, Bezier_VersionMismatch_Rejected)
{
  const occtl_point3_t             aPoles[2] = {{0, 0, 0}, {1, 0, 0}};
  occtl_curve_bezier_create_info_t aBezInfo  = OCCTL_CURVE_BEZIER_CREATE_INFO_INIT;
  aBezInfo.struct_version                    = 0u;
  aBezInfo.poles                             = aPoles;
  aBezInfo.pole_count                        = 2;
  occtl_rep_id_t aCurve                      = {};
  EXPECT_EQ(occtl_curve_create_bezier(myGraph, &aBezInfo, &aCurve), OCCTL_VERSION_MISMATCH);
}

} // namespace
