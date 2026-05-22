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

TEST(GeomTransformTest, Identity_ApplyPoint_ReturnsSamePoint)
{
  const occtl_transform_t aI = occtl_transform_identity();
  const occtl_point3_t    aP = {1.0, 2.0, 3.0};
  const occtl_point3_t    aR = occtl_transform_apply_point3(aI, aP);
  EXPECT_NEAR(aR.x, 1.0, 1e-15);
  EXPECT_NEAR(aR.y, 2.0, 1e-15);
  EXPECT_NEAR(aR.z, 3.0, 1e-15);
}

TEST(GeomTransformTest, Identity_ApplyVector_ReturnsSameVector)
{
  const occtl_transform_t aI = occtl_transform_identity();
  const occtl_vector3_t   aV = {5.0, 6.0, 7.0};
  const occtl_vector3_t   aR = occtl_transform_apply_vector3(aI, aV);
  EXPECT_NEAR(aR.x, 5.0, 1e-15);
  EXPECT_NEAR(aR.y, 6.0, 1e-15);
  EXPECT_NEAR(aR.z, 7.0, 1e-15);
}

TEST(GeomTransformTest, Translation_ApplyPoint_TranslatesCorrectly)
{
  const occtl_vector3_t   aV = {10.0, 20.0, 30.0};
  const occtl_transform_t aT = occtl_transform_translation(aV);
  const occtl_point3_t    aP = {1.0, 2.0, 3.0};
  const occtl_point3_t    aR = occtl_transform_apply_point3(aT, aP);
  EXPECT_NEAR(aR.x, 11.0, 1e-15);
  EXPECT_NEAR(aR.y, 22.0, 1e-15);
  EXPECT_NEAR(aR.z, 33.0, 1e-15);
}

TEST(GeomTransformTest, Translation_ApplyVector_DoesNotTranslateVector)
{
  const occtl_vector3_t   aV   = {10.0, 20.0, 30.0};
  const occtl_transform_t aT   = occtl_transform_translation(aV);
  const occtl_vector3_t   aVec = {1.0, 2.0, 3.0};
  const occtl_vector3_t   aR   = occtl_transform_apply_vector3(aT, aVec);
  EXPECT_NEAR(aR.x, 1.0, 1e-15);
  EXPECT_NEAR(aR.y, 2.0, 1e-15);
  EXPECT_NEAR(aR.z, 3.0, 1e-15);
}

TEST(GeomTransformTest, Rotation_90DegAboutZ_RotatesXtoY)
{
  const occtl_point3_t          aOrigin = {0.0, 0.0, 0.0};
  const occtl_direction3_t      aZDir   = {0.0, 0.0, 1.0};
  const occtl_axis1_placement_t aAxis   = {aOrigin, aZDir};
  occtl_transform_t             aRot{};
  ASSERT_EQ(occtl_transform_rotation(aAxis, M_PI / 2.0, &aRot), OCCTL_OK);

  const occtl_point3_t aP = {1.0, 0.0, 0.0};
  const occtl_point3_t aR = occtl_transform_apply_point3(aRot, aP);
  EXPECT_NEAR(aR.x, 0.0, 1e-10);
  EXPECT_NEAR(aR.y, 1.0, 1e-10);
  EXPECT_NEAR(aR.z, 0.0, 1e-10);
}

TEST(GeomTransformTest, Rotation_NullOut_ReturnsInvalidArgument)
{
  const occtl_point3_t          aOrigin = {0.0, 0.0, 0.0};
  const occtl_direction3_t      aDir    = {0.0, 0.0, 1.0};
  const occtl_axis1_placement_t aAxis   = {aOrigin, aDir};
  EXPECT_EQ(occtl_transform_rotation(aAxis, 0.0, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GeomTransformTest, Rotation_AxisPointOnAxis_IsFixed)
{
  // A point on the rotation axis must be unchanged by the rotation.
  const occtl_point3_t          aOrigin = {1.0, 2.0, 0.0};
  const occtl_direction3_t      aZDir   = {0.0, 0.0, 1.0};
  const occtl_axis1_placement_t aAxis   = {aOrigin, aZDir};
  occtl_transform_t             aRot{};
  ASSERT_EQ(occtl_transform_rotation(aAxis, M_PI / 3.0, &aRot), OCCTL_OK);

  const occtl_point3_t aR = occtl_transform_apply_point3(aRot, aOrigin);
  EXPECT_NEAR(aR.x, aOrigin.x, 1e-10);
  EXPECT_NEAR(aR.y, aOrigin.y, 1e-10);
  EXPECT_NEAR(aR.z, aOrigin.z, 1e-10);
}

TEST(GeomTransformTest, Scale_ByTwo_DoublesDistanceFromCenter)
{
  const occtl_point3_t aCenter = {0.0, 0.0, 0.0};
  occtl_transform_t    aS{};
  ASSERT_EQ(occtl_transform_scale(aCenter, 2.0, &aS), OCCTL_OK);

  const occtl_point3_t aP = {1.0, 0.0, 0.0};
  const occtl_point3_t aR = occtl_transform_apply_point3(aS, aP);
  EXPECT_NEAR(aR.x, 2.0, 1e-15);
  EXPECT_NEAR(aR.y, 0.0, 1e-15);
  EXPECT_NEAR(aR.z, 0.0, 1e-15);
}

TEST(GeomTransformTest, Scale_ZeroFactor_ReturnsGeometryInvalid)
{
  const occtl_point3_t aCenter = {0.0, 0.0, 0.0};
  occtl_transform_t    aS{};
  EXPECT_EQ(occtl_transform_scale(aCenter, 0.0, &aS), OCCTL_GEOMETRY_INVALID);
}

TEST(GeomTransformTest, Scale_NullOut_ReturnsInvalidArgument)
{
  const occtl_point3_t aCenter = {0.0, 0.0, 0.0};
  EXPECT_EQ(occtl_transform_scale(aCenter, 2.0, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GeomTransformTest, Scale_CenterIsFixed)
{
  const occtl_point3_t aCenter = {3.0, 4.0, 5.0};
  occtl_transform_t    aS{};
  ASSERT_EQ(occtl_transform_scale(aCenter, 3.0, &aS), OCCTL_OK);
  const occtl_point3_t aR = occtl_transform_apply_point3(aS, aCenter);
  EXPECT_NEAR(aR.x, aCenter.x, 1e-15);
  EXPECT_NEAR(aR.y, aCenter.y, 1e-15);
  EXPECT_NEAR(aR.z, aCenter.z, 1e-15);
}

TEST(GeomTransformTest, Compose_TwoTranslations_SumsTranslation)
{
  const occtl_transform_t aT1 = occtl_transform_translation({1.0, 0.0, 0.0});
  const occtl_transform_t aT2 = occtl_transform_translation({0.0, 2.0, 0.0});
  const occtl_transform_t aC  = occtl_transform_compose(aT1, aT2);

  const occtl_point3_t aP = {0.0, 0.0, 0.0};
  const occtl_point3_t aR = occtl_transform_apply_point3(aC, aP);
  EXPECT_NEAR(aR.x, 1.0, 1e-15);
  EXPECT_NEAR(aR.y, 2.0, 1e-15);
  EXPECT_NEAR(aR.z, 0.0, 1e-15);
}

TEST(GeomTransformTest, Inverted_ComposeWithInverse_GivesIdentity)
{
  const occtl_vector3_t   aV = {5.0, -3.0, 2.0};
  const occtl_transform_t aT = occtl_transform_translation(aV);
  occtl_transform_t       aInv{};
  ASSERT_EQ(occtl_transform_inverted(aT, &aInv), OCCTL_OK);

  const occtl_transform_t aResult = occtl_transform_compose(aT, aInv);
  const occtl_point3_t    aP      = {1.0, 2.0, 3.0};
  const occtl_point3_t    aR      = occtl_transform_apply_point3(aResult, aP);
  EXPECT_NEAR(aR.x, aP.x, 1e-12);
  EXPECT_NEAR(aR.y, aP.y, 1e-12);
  EXPECT_NEAR(aR.z, aP.z, 1e-12);
}

TEST(GeomTransformTest, Inverted_NullOut_ReturnsInvalidArgument)
{
  const occtl_transform_t aI = occtl_transform_identity();
  EXPECT_EQ(occtl_transform_inverted(aI, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GeomTransformTest, Inverted_SingularTransform_ReturnsGeometryInvalid)
{
  occtl_transform_t aSingular{}; // all zeros — det = 0
  occtl_transform_t aInv{};
  EXPECT_EQ(occtl_transform_inverted(aSingular, &aInv), OCCTL_GEOMETRY_INVALID);
}

TEST(GeomVeneerTransformTest, Identity_ApplyPoint_ReturnsSamePoint)
{
  const occtl::Transform aI = occtl::Transform::identity();
  const occtl::Point3    aP(1.0, 2.0, 3.0);
  const occtl::Point3    aR = aI.apply(aP);
  EXPECT_NEAR(aR.x(), 1.0, 1e-15);
  EXPECT_NEAR(aR.y(), 2.0, 1e-15);
  EXPECT_NEAR(aR.z(), 3.0, 1e-15);
}

TEST(GeomVeneerTransformTest, Inverted_SingularTransform_ThrowsError)
{
  occtl_transform_t      aSingular{};
  const occtl::Transform aT(aSingular);
  EXPECT_THROW(aT.inverted(), occtl::Error);
}

TEST(GeomVeneerTransformTest, Compose_ThenInvert_RoundTrip)
{
  const occtl::Transform aT   = occtl::Transform::translation(occtl::Vector3(7.0, 8.0, 9.0));
  const occtl::Transform aInv = aT.inverted();
  const occtl::Transform aId  = aT.compose(aInv);
  const occtl::Point3    aP(1.0, 2.0, 3.0);
  const occtl::Point3    aR = aId.apply(aP);
  EXPECT_NEAR(aR.x(), aP.x(), 1e-12);
  EXPECT_NEAR(aR.y(), aP.y(), 1e-12);
  EXPECT_NEAR(aR.z(), aP.z(), 1e-12);
}

TEST(GeomVeneerTransformTest, ApplyDirection3_Identity_ReturnsSameDirection)
{
  const occtl_direction3_t aCD = {1.0, 0.0, 0.0};
  const occtl::Direction3  aD(aCD);
  const occtl::Transform   aI = occtl::Transform::identity();
  const occtl::Direction3  aR = aI.apply(aD);
  EXPECT_NEAR(aR.x(), 1.0, 1e-15);
  EXPECT_NEAR(aR.y(), 0.0, 1e-15);
  EXPECT_NEAR(aR.z(), 0.0, 1e-15);
}

TEST(GeomTransformTest, FromAxis2_WorldFrame_IsIdentity)
{
  const occtl_axis2_placement_t aFrame = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  occtl_transform_t             aT{};
  ASSERT_EQ(occtl_transform_from_axis2(aFrame, &aT), OCCTL_OK);
  const occtl_point3_t aR = occtl_transform_apply_point3(aT, {0.0, 0.0, 0.0});
  EXPECT_NEAR(aR.x, 0.0, 1e-12);
  EXPECT_NEAR(aR.y, 0.0, 1e-12);
  EXPECT_NEAR(aR.z, 0.0, 1e-12);
}

TEST(GeomTransformTest, FromAxis2_ParallelDirs_ReturnsGeometryInvalid)
{
  const occtl_axis2_placement_t aFrame = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  occtl_transform_t             aT{};
  EXPECT_EQ(occtl_transform_from_axis2(aFrame, &aT), OCCTL_GEOMETRY_INVALID);
}

TEST(GeomTransformTest, FromAxis2_NullOut_ReturnsInvalidArgument)
{
  const occtl_axis2_placement_t aFrame = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  EXPECT_EQ(occtl_transform_from_axis2(aFrame, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(GeomTransformTest, FromAxis3_StandardFrame_MapsBasisVectors)
{
  const occtl_axis3_placement_t aFrame = {{0.0, 0.0, 0.0},
                                          {1.0, 0.0, 0.0},
                                          {0.0, 1.0, 0.0},
                                          {0.0, 0.0, 1.0}};
  occtl_transform_t             aT{};
  ASSERT_EQ(occtl_transform_from_axis3(aFrame, &aT), OCCTL_OK);
  const occtl_point3_t aR = occtl_transform_apply_point3(aT, {2.0, 3.0, 4.0});
  EXPECT_NEAR(aR.x, 2.0, 1e-12);
  EXPECT_NEAR(aR.y, 3.0, 1e-12);
  EXPECT_NEAR(aR.z, 4.0, 1e-12);
}

TEST(GeomTransformTest, FromAxis3_DegenerateFrame_ReturnsGeometryInvalid)
{
  const occtl_axis3_placement_t aFrame = {{0.0, 0.0, 0.0},
                                          {1.0, 0.0, 0.0},
                                          {1.0, 0.0, 0.0},
                                          {0.0, 0.0, 1.0}};
  occtl_transform_t             aT{};
  EXPECT_EQ(occtl_transform_from_axis3(aFrame, &aT), OCCTL_GEOMETRY_INVALID);
}

TEST(GeomVeneerTransformTest, FromAxis2_BuildsValidTransform)
{
  const occtl::Point3         aOrigin(1.0, 2.0, 3.0);
  const occtl::Direction3     aXDir(occtl_direction3_t{1.0, 0.0, 0.0});
  const occtl::Direction3     aRef(occtl_direction3_t{0.0, 1.0, 0.0});
  const occtl::Axis2Placement aFrame(aOrigin, aXDir, aRef);
  const occtl::Transform      aT = occtl::Transform::from_axis2(aFrame);
  // Frame origin maps to world origin under world→frame transform.
  const occtl::Point3 aR = aT.apply(aOrigin);
  EXPECT_NEAR(aR.x(), 0.0, 1e-12);
  EXPECT_NEAR(aR.y(), 0.0, 1e-12);
  EXPECT_NEAR(aR.z(), 0.0, 1e-12);
}

} // namespace
