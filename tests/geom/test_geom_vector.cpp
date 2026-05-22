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
#include <cstring>

namespace
{

TEST(GeomVector3Test, Dot_OrthogonalVectors_ReturnsZero)
{
  const occtl_vector3_t aA = {1.0, 0.0, 0.0};
  const occtl_vector3_t aB = {0.0, 1.0, 0.0};
  EXPECT_NEAR(occtl_vector3_dot(aA, aB), 0.0, 1e-15);
}

TEST(GeomVector3Test, Dot_ParallelUnitVectors_ReturnsOne)
{
  const occtl_vector3_t aA = {1.0, 0.0, 0.0};
  EXPECT_NEAR(occtl_vector3_dot(aA, aA), 1.0, 1e-15);
}

TEST(GeomVector3Test, Cross_XY_ReturnsZ)
{
  const occtl_vector3_t aX = {1.0, 0.0, 0.0};
  const occtl_vector3_t aY = {0.0, 1.0, 0.0};
  const occtl_vector3_t aZ = occtl_vector3_cross(aX, aY);
  EXPECT_NEAR(aZ.x, 0.0, 1e-15);
  EXPECT_NEAR(aZ.y, 0.0, 1e-15);
  EXPECT_NEAR(aZ.z, 1.0, 1e-15);
}

TEST(GeomVector3Test, Magnitude_UnitX_ReturnsOne)
{
  const occtl_vector3_t aV = {1.0, 0.0, 0.0};
  EXPECT_NEAR(occtl_vector3_magnitude(aV), 1.0, 1e-15);
}

TEST(GeomVector3Test, Magnitude_ThreeFourZero_ReturnsFive)
{
  const occtl_vector3_t aV = {3.0, 4.0, 0.0};
  EXPECT_NEAR(occtl_vector3_magnitude(aV), 5.0, 1e-15);
}

TEST(GeomVector3Test, Normalized_UnitResult_MagnitudeIsOne)
{
  const occtl_vector3_t aV = {3.0, 4.0, 0.0};
  occtl_vector3_t       aResult{};
  ASSERT_EQ(occtl_vector3_normalized(aV, &aResult), OCCTL_OK);
  EXPECT_NEAR(occtl_vector3_magnitude(aResult), 1.0, 1e-15);
}

TEST(GeomVector3Test, Normalized_ZeroVector_ReturnsGeometryInvalid)
{
  const occtl_vector3_t aZero = {0.0, 0.0, 0.0};
  occtl_vector3_t       aResult{};
  EXPECT_EQ(occtl_vector3_normalized(aZero, &aResult), OCCTL_GEOMETRY_INVALID);
}

TEST(GeomVector3Test, Normalized_NullOut_ReturnsInvalidArgument)
{
  const occtl_vector3_t aV = {1.0, 0.0, 0.0};
  EXPECT_EQ(occtl_vector3_normalized(aV, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GeomVector3Test, Add_TwoVectors_CorrectSum)
{
  const occtl_vector3_t aA = {1.0, 2.0, 3.0};
  const occtl_vector3_t aB = {4.0, 5.0, 6.0};
  const occtl_vector3_t aR = occtl_vector3_add(aA, aB);
  EXPECT_NEAR(aR.x, 5.0, 1e-15);
  EXPECT_NEAR(aR.y, 7.0, 1e-15);
  EXPECT_NEAR(aR.z, 9.0, 1e-15);
}

TEST(GeomVector3Test, Sub_TwoVectors_CorrectDifference)
{
  const occtl_vector3_t aA = {4.0, 5.0, 6.0};
  const occtl_vector3_t aB = {1.0, 2.0, 3.0};
  const occtl_vector3_t aR = occtl_vector3_sub(aA, aB);
  EXPECT_NEAR(aR.x, 3.0, 1e-15);
  EXPECT_NEAR(aR.y, 3.0, 1e-15);
  EXPECT_NEAR(aR.z, 3.0, 1e-15);
}

TEST(GeomVector3Test, Scaled_ByTwo_DoublesComponents)
{
  const occtl_vector3_t aV = {1.0, 2.0, 3.0};
  const occtl_vector3_t aR = occtl_vector3_scaled(aV, 2.0);
  EXPECT_NEAR(aR.x, 2.0, 1e-15);
  EXPECT_NEAR(aR.y, 4.0, 1e-15);
  EXPECT_NEAR(aR.z, 6.0, 1e-15);
}

TEST(GeomVector3Test, Reversed_NegatesComponents)
{
  const occtl_vector3_t aV = {1.0, -2.0, 3.0};
  const occtl_vector3_t aR = occtl_vector3_reversed(aV);
  EXPECT_NEAR(aR.x, -1.0, 1e-15);
  EXPECT_NEAR(aR.y, 2.0, 1e-15);
  EXPECT_NEAR(aR.z, -3.0, 1e-15);
}

TEST(GeomVector3Test, Angle_OrthogonalVectors_ReturnsHalfPi)
{
  const occtl_vector3_t aX   = {1.0, 0.0, 0.0};
  const occtl_vector3_t aY   = {0.0, 1.0, 0.0};
  double                aRad = 0.0;
  ASSERT_EQ(occtl_vector3_angle(aX, aY, &aRad), OCCTL_OK);
  EXPECT_NEAR(aRad, M_PI / 2.0, 1e-10);
}

TEST(GeomVector3Test, Angle_ParallelVectors_ReturnsZero)
{
  const occtl_vector3_t aV   = {1.0, 0.0, 0.0};
  double                aRad = 999.0;
  ASSERT_EQ(occtl_vector3_angle(aV, aV, &aRad), OCCTL_OK);
  EXPECT_NEAR(aRad, 0.0, 1e-10);
}

TEST(GeomVector3Test, Angle_ZeroVector_ReturnsGeometryInvalid)
{
  const occtl_vector3_t aZero = {0.0, 0.0, 0.0};
  const occtl_vector3_t aV    = {1.0, 0.0, 0.0};
  double                aRad  = 0.0;
  EXPECT_EQ(occtl_vector3_angle(aZero, aV, &aRad), OCCTL_GEOMETRY_INVALID);
}

TEST(GeomVector3Test, Angle_NullOut_ReturnsInvalidArgument)
{
  const occtl_vector3_t aV = {1.0, 0.0, 0.0};
  EXPECT_EQ(occtl_vector3_angle(aV, aV, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GeomVector2Test, Dot_OrthogonalVectors_ReturnsZero)
{
  const occtl_vector2_t aA = {1.0, 0.0};
  const occtl_vector2_t aB = {0.0, 1.0};
  EXPECT_NEAR(occtl_vector2_dot(aA, aB), 0.0, 1e-15);
}

TEST(GeomVector2Test, Cross_XY_ReturnsPositiveOne)
{
  const occtl_vector2_t aX = {1.0, 0.0};
  const occtl_vector2_t aY = {0.0, 1.0};
  EXPECT_NEAR(occtl_vector2_cross(aX, aY), 1.0, 1e-15);
}

TEST(GeomVector2Test, Magnitude_ThreeFour_ReturnsFive)
{
  const occtl_vector2_t aV = {3.0, 4.0};
  EXPECT_NEAR(occtl_vector2_magnitude(aV), 5.0, 1e-15);
}

TEST(GeomVector2Test, Normalized_ZeroVector_ReturnsGeometryInvalid)
{
  const occtl_vector2_t aZero = {0.0, 0.0};
  occtl_vector2_t       aResult{};
  EXPECT_EQ(occtl_vector2_normalized(aZero, &aResult), OCCTL_GEOMETRY_INVALID);
}

TEST(GeomVector2Test, Normalized_UnitResult_MagnitudeIsOne)
{
  const occtl_vector2_t aV = {3.0, 4.0};
  occtl_vector2_t       aResult{};
  ASSERT_EQ(occtl_vector2_normalized(aV, &aResult), OCCTL_OK);
  EXPECT_NEAR(occtl_vector2_magnitude(aResult), 1.0, 1e-15);
}

TEST(GeomVector3Test, Normalized_ZeroVector_PopulatesErrorState)
{
  const occtl_vector3_t aZero = {0.0, 0.0, 0.0};
  occtl_vector3_t       aResult{};
  ASSERT_EQ(occtl_vector3_normalized(aZero, &aResult), OCCTL_GEOMETRY_INVALID);
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_EQ(anErr->status, OCCTL_GEOMETRY_INVALID);
  ASSERT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST(GeomVector3Test, Angle_NullOut_PopulatesErrorState)
{
  const occtl_vector3_t aV = {1.0, 0.0, 0.0};
  ASSERT_EQ(occtl_vector3_angle(aV, aV, nullptr), OCCTL_INVALID_ARGUMENT);
  const occtl_error_t* anErr = occtl_error_last();
  ASSERT_NE(anErr, nullptr);
  EXPECT_EQ(anErr->status, OCCTL_INVALID_ARGUMENT);
}

TEST(GeomVeneerVector3Test, Normalized_ZeroVector_ThrowsError)
{
  const occtl::Vector3 aZero(0.0, 0.0, 0.0);
  EXPECT_THROW(aZero.normalized(), occtl::Error);
}

TEST(GeomVeneerVector3Test, CrossAndMagnitude_UnitVectors_ReturnsOne)
{
  const occtl::Vector3 aX(1.0, 0.0, 0.0);
  const occtl::Vector3 aY(0.0, 1.0, 0.0);
  EXPECT_NEAR(aX.cross(aY).magnitude(), 1.0, 1e-15);
}

TEST(GeomVeneerVector3Test, DotAndAngle_KnownVectors_Consistent)
{
  const occtl::Vector3 aA(1.0, 0.0, 0.0);
  const occtl::Vector3 aB(1.0, 1.0, 0.0);
  EXPECT_NEAR(aA.dot(aB), 1.0, 1e-15);
  EXPECT_NEAR(aA.angle(aB), M_PI / 4.0, 1e-12);
}

TEST(GeomVeneerVector3Test, Operators_AddSubScale_Compose)
{
  const occtl::Vector3 aA(1.0, 2.0, 3.0);
  const occtl::Vector3 aB(4.0, 5.0, 6.0);
  const occtl::Vector3 aSum  = aA + aB;
  const occtl::Vector3 aDiff = aB - aA;
  const occtl::Vector3 aTwoA = aA * 2.0;
  EXPECT_NEAR(aSum.x(), 5.0, 1e-15);
  EXPECT_NEAR(aDiff.x(), 3.0, 1e-15);
  EXPECT_NEAR(aTwoA.x(), 2.0, 1e-15);
  EXPECT_NEAR(aA.reversed().x(), -1.0, 1e-15);
}

TEST(GeomVeneerVector2Test, Normalized_NonZero_HasUnitMagnitude)
{
  const occtl::Vector2 aV(3.0, 4.0);
  const occtl::Vector2 aNorm = aV.normalized();
  EXPECT_NEAR(aNorm.magnitude(), 1.0, 1e-15);
}

} // namespace
