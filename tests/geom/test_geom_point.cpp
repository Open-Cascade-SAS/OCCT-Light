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

#include <occtl-hpp/geom.hpp>
#include <occtl/occtl_geom.h>

#include <gtest/gtest.h>

#include <cmath>

namespace
{

TEST(GeomPoint3Test, Distance_SamePoint_ReturnsZero)
{
  const occtl_point3_t aP = {1.0, 2.0, 3.0};
  EXPECT_NEAR(occtl_point3_distance(aP, aP), 0.0, 1e-15);
}

TEST(GeomPoint3Test, Distance_KnownPair_ReturnsCorrectLength)
{
  const occtl_point3_t aA = {0.0, 0.0, 0.0};
  const occtl_point3_t aB = {3.0, 4.0, 0.0};
  EXPECT_NEAR(occtl_point3_distance(aA, aB), 5.0, 1e-15);
}

TEST(GeomPoint3Test, Midpoint_SymmetricPoints_ReturnsOrigin)
{
  const occtl_point3_t aA   = {-1.0, -2.0, -3.0};
  const occtl_point3_t aB   = {1.0, 2.0, 3.0};
  const occtl_point3_t aMid = occtl_point3_midpoint(aA, aB);
  EXPECT_NEAR(aMid.x, 0.0, 1e-15);
  EXPECT_NEAR(aMid.y, 0.0, 1e-15);
  EXPECT_NEAR(aMid.z, 0.0, 1e-15);
}

TEST(GeomPoint3Test, Translate_ByZeroVector_ReturnsSamePoint)
{
  const occtl_point3_t  aP = {1.0, 2.0, 3.0};
  const occtl_vector3_t aV = {0.0, 0.0, 0.0};
  const occtl_point3_t  aR = occtl_point3_translate(aP, aV);
  EXPECT_NEAR(aR.x, aP.x, 1e-15);
  EXPECT_NEAR(aR.y, aP.y, 1e-15);
  EXPECT_NEAR(aR.z, aP.z, 1e-15);
}

TEST(GeomPoint3Test, Translate_ByVector_CorrectResult)
{
  const occtl_point3_t  aP = {1.0, 2.0, 3.0};
  const occtl_vector3_t aV = {10.0, 20.0, 30.0};
  const occtl_point3_t  aR = occtl_point3_translate(aP, aV);
  EXPECT_NEAR(aR.x, 11.0, 1e-15);
  EXPECT_NEAR(aR.y, 22.0, 1e-15);
  EXPECT_NEAR(aR.z, 33.0, 1e-15);
}

TEST(GeomPoint3Test, TransformApply_IdentityTransform_ReturnsSamePoint)
{
  const occtl_transform_t aI = occtl_transform_identity();
  const occtl_point3_t    aP = {5.0, 6.0, 7.0};
  const occtl_point3_t    aR = occtl_transform_apply_point3(aI, aP);
  EXPECT_NEAR(aR.x, aP.x, 1e-15);
  EXPECT_NEAR(aR.y, aP.y, 1e-15);
  EXPECT_NEAR(aR.z, aP.z, 1e-15);
}

TEST(GeomPoint3Test, TransformApply_TranslationTransform_TranslatesPoint)
{
  const occtl_vector3_t   aV = {1.0, 2.0, 3.0};
  const occtl_transform_t aT = occtl_transform_translation(aV);
  const occtl_point3_t    aP = {0.0, 0.0, 0.0};
  const occtl_point3_t    aR = occtl_transform_apply_point3(aT, aP);
  EXPECT_NEAR(aR.x, 1.0, 1e-15);
  EXPECT_NEAR(aR.y, 2.0, 1e-15);
  EXPECT_NEAR(aR.z, 3.0, 1e-15);
}

TEST(GeomPoint2Test, Distance_SamePoint_ReturnsZero)
{
  const occtl_point2_t aP = {3.0, 4.0};
  EXPECT_NEAR(occtl_point2_distance(aP, aP), 0.0, 1e-15);
}

TEST(GeomPoint2Test, Distance_KnownPair_ReturnsCorrectLength)
{
  const occtl_point2_t aA = {0.0, 0.0};
  const occtl_point2_t aB = {3.0, 4.0};
  EXPECT_NEAR(occtl_point2_distance(aA, aB), 5.0, 1e-15);
}

TEST(GeomPoint2Test, Midpoint_TwoPoints_CorrectMidpoint)
{
  const occtl_point2_t aA   = {0.0, 0.0};
  const occtl_point2_t aB   = {2.0, 4.0};
  const occtl_point2_t aMid = occtl_point2_midpoint(aA, aB);
  EXPECT_NEAR(aMid.x, 1.0, 1e-15);
  EXPECT_NEAR(aMid.y, 2.0, 1e-15);
}

TEST(GeomVeneerPoint3Test, DistanceTo_SamePoint_ReturnsZero)
{
  const occtl::Point3 aP(1.0, 2.0, 3.0);
  EXPECT_NEAR(aP.distance_to(aP), 0.0, 1e-15);
}

TEST(GeomVeneerPoint3Test, Midpoint_ReturnsCorrectResult)
{
  const occtl::Point3 aA(0.0, 0.0, 0.0);
  const occtl::Point3 aB(2.0, 4.0, 6.0);
  const occtl::Point3 aMid = aA.midpoint(aB);
  EXPECT_NEAR(aMid.x(), 1.0, 1e-15);
  EXPECT_NEAR(aMid.y(), 2.0, 1e-15);
  EXPECT_NEAR(aMid.z(), 3.0, 1e-15);
}

TEST(GeomVeneerPoint3Test, DistanceTo_KnownPair_ReturnsFive)
{
  const occtl::Point3 aA(0.0, 0.0, 0.0);
  const occtl::Point3 aB(3.0, 4.0, 0.0);
  EXPECT_NEAR(aA.distance_to(aB), 5.0, 1e-15);
}

TEST(GeomVeneerPoint2Test, Midpoint_ReturnsCorrectResult)
{
  const occtl::Point2 aA(0.0, 0.0);
  const occtl::Point2 aB(2.0, 4.0);
  const occtl::Point2 aMid = aA.midpoint(aB);
  EXPECT_NEAR(aMid.x(), 1.0, 1e-15);
  EXPECT_NEAR(aMid.y(), 2.0, 1e-15);
}

} // namespace
