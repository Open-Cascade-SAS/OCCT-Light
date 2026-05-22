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

TEST(GeomDirection3Test, FromVector_UnitX_Succeeds)
{
  const occtl_vector3_t aV = {1.0, 0.0, 0.0};
  occtl_direction3_t    aD{};
  ASSERT_EQ(occtl_direction3_from_vector(aV, &aD), OCCTL_OK);
  EXPECT_NEAR(aD.x, 1.0, 1e-15);
  EXPECT_NEAR(aD.y, 0.0, 1e-15);
  EXPECT_NEAR(aD.z, 0.0, 1e-15);
}

TEST(GeomDirection3Test, FromVector_NonUnitVector_NormalisesResult)
{
  const occtl_vector3_t aV = {3.0, 4.0, 0.0};
  occtl_direction3_t    aD{};
  ASSERT_EQ(occtl_direction3_from_vector(aV, &aD), OCCTL_OK);
  const double aMag = occtl_vector3_magnitude({aD.x, aD.y, aD.z});
  EXPECT_NEAR(aMag, 1.0, 1e-15);
}

TEST(GeomDirection3Test, FromVector_ZeroVector_ReturnsGeometryInvalid)
{
  const occtl_vector3_t aZero = {0.0, 0.0, 0.0};
  occtl_direction3_t    aD{};
  EXPECT_EQ(occtl_direction3_from_vector(aZero, &aD), OCCTL_GEOMETRY_INVALID);
}

TEST(GeomDirection3Test, FromVector_NullOut_ReturnsInvalidArgument)
{
  const occtl_vector3_t aV = {1.0, 0.0, 0.0};
  EXPECT_EQ(occtl_direction3_from_vector(aV, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GeomDirection3Test, Dot_OrthogonalDirections_ReturnsZero)
{
  const occtl_direction3_t aX = {1.0, 0.0, 0.0};
  const occtl_direction3_t aY = {0.0, 1.0, 0.0};
  EXPECT_NEAR(occtl_direction3_dot(aX, aY), 0.0, 1e-15);
}

TEST(GeomDirection3Test, Dot_SameDirection_ReturnsOne)
{
  const occtl_direction3_t aX = {1.0, 0.0, 0.0};
  EXPECT_NEAR(occtl_direction3_dot(aX, aX), 1.0, 1e-15);
}

TEST(GeomDirection3Test, Cross_XY_ReturnsZ)
{
  const occtl_direction3_t aX = {1.0, 0.0, 0.0};
  const occtl_direction3_t aY = {0.0, 1.0, 0.0};
  const occtl_vector3_t    aZ = occtl_direction3_cross(aX, aY);
  EXPECT_NEAR(aZ.x, 0.0, 1e-15);
  EXPECT_NEAR(aZ.y, 0.0, 1e-15);
  EXPECT_NEAR(aZ.z, 1.0, 1e-15);
}

TEST(GeomDirection3Test, Angle_OrthogonalDirections_ReturnsHalfPi)
{
  const occtl_direction3_t aX = {1.0, 0.0, 0.0};
  const occtl_direction3_t aY = {0.0, 1.0, 0.0};
  EXPECT_NEAR(occtl_direction3_angle(aX, aY), M_PI / 2.0, 1e-10);
}

TEST(GeomDirection3Test, Angle_SameDirection_ReturnsZero)
{
  const occtl_direction3_t aX = {1.0, 0.0, 0.0};
  EXPECT_NEAR(occtl_direction3_angle(aX, aX), 0.0, 1e-10);
}

TEST(GeomDirection3Test, Angle_OppositeDirections_ReturnsPi)
{
  const occtl_direction3_t aP = {1.0, 0.0, 0.0};
  const occtl_direction3_t aN = {-1.0, 0.0, 0.0};
  EXPECT_NEAR(occtl_direction3_angle(aP, aN), M_PI, 1e-10);
}

TEST(GeomDirection3Test, Reversed_NegatesComponents)
{
  const occtl_direction3_t aD = {1.0, 0.0, 0.0};
  const occtl_direction3_t aR = occtl_direction3_reversed(aD);
  EXPECT_NEAR(aR.x, -1.0, 1e-15);
  EXPECT_NEAR(aR.y, 0.0, 1e-15);
  EXPECT_NEAR(aR.z, 0.0, 1e-15);
}

TEST(GeomDirection3Test, Transform_Identity_ReturnsSameDirection)
{
  const occtl_direction3_t aD = {1.0, 0.0, 0.0};
  const occtl_transform_t  aI = occtl_transform_identity();
  occtl_direction3_t       aR{};
  ASSERT_EQ(occtl_direction3_transform(aD, aI, &aR), OCCTL_OK);
  EXPECT_NEAR(aR.x, 1.0, 1e-15);
  EXPECT_NEAR(aR.y, 0.0, 1e-15);
  EXPECT_NEAR(aR.z, 0.0, 1e-15);
}

TEST(GeomDirection3Test, Transform_NullOut_ReturnsInvalidArgument)
{
  const occtl_direction3_t aD = {1.0, 0.0, 0.0};
  const occtl_transform_t  aI = occtl_transform_identity();
  EXPECT_EQ(occtl_direction3_transform(aD, aI, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GeomDirection2Test, FromVector_UnitX_Succeeds)
{
  const occtl_vector2_t aV = {1.0, 0.0};
  occtl_direction2_t    aD{};
  ASSERT_EQ(occtl_direction2_from_vector(aV, &aD), OCCTL_OK);
  EXPECT_NEAR(aD.x, 1.0, 1e-15);
  EXPECT_NEAR(aD.y, 0.0, 1e-15);
}

TEST(GeomDirection2Test, FromVector_ZeroVector_ReturnsGeometryInvalid)
{
  const occtl_vector2_t aZero = {0.0, 0.0};
  occtl_direction2_t    aD{};
  EXPECT_EQ(occtl_direction2_from_vector(aZero, &aD), OCCTL_GEOMETRY_INVALID);
}

TEST(GeomDirection2Test, Angle_OrthogonalDirections_ReturnsHalfPi)
{
  const occtl_direction2_t aX = {1.0, 0.0};
  const occtl_direction2_t aY = {0.0, 1.0};
  EXPECT_NEAR(occtl_direction2_angle(aX, aY), M_PI / 2.0, 1e-10);
}

TEST(GeomVeneerDirection3Test, FromVector_ZeroVector_ThrowsError)
{
  const occtl::Vector3 aZero(0.0, 0.0, 0.0);
  EXPECT_THROW(occtl::Direction3::from_vector(aZero), occtl::Error);
}

TEST(GeomVeneerDirection3Test, DotAndAngle_Orthogonal_Consistent)
{
  const occtl_direction3_t aCX = {1.0, 0.0, 0.0};
  const occtl_direction3_t aCY = {0.0, 1.0, 0.0};
  const occtl::Direction3  aX(aCX);
  const occtl::Direction3  aY(aCY);
  EXPECT_NEAR(aX.dot(aY), 0.0, 1e-15);
  EXPECT_NEAR(aX.angle(aY), M_PI / 2.0, 1e-10);
}

} // namespace
