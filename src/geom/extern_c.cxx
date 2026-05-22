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

#include "GeomMath.hxx"

#include <occtl/occtl_geom.h>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Mat.hxx>
#include <gp_XY.hxx>
#include <Precision.hxx>
#include <gp.hxx>

extern "C"
{

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_point3_distance(occtl_point3_t theA, occtl_point3_t theB)
{
  return OcctL::Geom::ToGp(theA).Distance(OcctL::Geom::ToGp(theB));
}

//=================================================================================================

OCCTL_API occtl_point3_t OCCTL_CALL occtl_point3_midpoint(occtl_point3_t theA, occtl_point3_t theB)
{
  gp_Pnt aMid = OcctL::Geom::ToGp(theA);
  aMid.BaryCenter(1.0, OcctL::Geom::ToGp(theB), 1.0);
  return OcctL::Geom::FromGp(aMid);
}

//=================================================================================================

OCCTL_API occtl_point3_t OCCTL_CALL occtl_point3_translate(occtl_point3_t  theP,
                                                           occtl_vector3_t theV)
{
  return OcctL::Geom::FromGp(OcctL::Geom::ToGp(theP).Translated(OcctL::Geom::ToGp(theV)));
}

//=================================================================================================

OCCTL_API occtl_point3_t OCCTL_CALL occtl_transform_apply_point3(occtl_transform_t theT,
                                                                 occtl_point3_t    theP)
{
  gp_XYZ aCoord = OcctL::Geom::ToGp(theP).XYZ();
  OcctL::Geom::ToGpGTrsf(theT).Transforms(aCoord);
  return {aCoord.X(), aCoord.Y(), aCoord.Z()};
}

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_point2_distance(occtl_point2_t theA, occtl_point2_t theB)
{
  return OcctL::Geom::ToGp(theA).Distance(OcctL::Geom::ToGp(theB));
}

//=================================================================================================

OCCTL_API occtl_point2_t OCCTL_CALL occtl_point2_midpoint(occtl_point2_t theA, occtl_point2_t theB)
{
  gp_XY aXY = OcctL::Geom::ToGp(theA).XY();
  aXY.Add(OcctL::Geom::ToGp(theB).XY());
  aXY.Multiply(0.5);
  return {aXY.X(), aXY.Y()};
}

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_vector3_dot(occtl_vector3_t theA, occtl_vector3_t theB)
{
  return OcctL::Geom::ToGp(theA).Dot(OcctL::Geom::ToGp(theB));
}

//=================================================================================================

OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_cross(occtl_vector3_t theA, occtl_vector3_t theB)
{
  return OcctL::Geom::FromGp(OcctL::Geom::ToGp(theA).Crossed(OcctL::Geom::ToGp(theB)));
}

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_vector3_magnitude(occtl_vector3_t theV)
{
  return OcctL::Geom::ToGp(theV).Magnitude();
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_vector3_normalized(occtl_vector3_t  theV,
                                                             occtl_vector3_t* theOutResult)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutResult == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_result is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (OcctL::Geom::ToGp(theV).SquareMagnitude() <= Precision::SquareConfusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "cannot normalise a zero-length vector");
      return OCCTL_GEOMETRY_INVALID;
    }
    gp_Vec aV = OcctL::Geom::ToGp(theV);
    aV.Normalize();
    *theOutResult = OcctL::Geom::FromGp(aV);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_vector3_angle(occtl_vector3_t theA,
                                                        occtl_vector3_t theB,
                                                        double*         theOutRadians)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutRadians == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_radians is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const gp_Vec aA = OcctL::Geom::ToGp(theA);
    const gp_Vec aB = OcctL::Geom::ToGp(theB);
    if (aA.SquareMagnitude() <= Precision::SquareConfusion()
        || aB.SquareMagnitude() <= Precision::SquareConfusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "angle requires non-zero vectors");
      return OCCTL_GEOMETRY_INVALID;
    }
    *theOutRadians = aA.Angle(aB);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_add(occtl_vector3_t theA, occtl_vector3_t theB)
{
  return OcctL::Geom::FromGp(OcctL::Geom::ToGp(theA) + OcctL::Geom::ToGp(theB));
}

//=================================================================================================

OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_sub(occtl_vector3_t theA, occtl_vector3_t theB)
{
  return OcctL::Geom::FromGp(OcctL::Geom::ToGp(theA) - OcctL::Geom::ToGp(theB));
}

//=================================================================================================

OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_scaled(occtl_vector3_t theV, double theS)
{
  return OcctL::Geom::FromGp(OcctL::Geom::ToGp(theV) * theS);
}

//=================================================================================================

OCCTL_API occtl_vector3_t OCCTL_CALL occtl_vector3_reversed(occtl_vector3_t theV)
{
  return OcctL::Geom::FromGp(OcctL::Geom::ToGp(theV).Reversed());
}

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_vector2_dot(occtl_vector2_t theA, occtl_vector2_t theB)
{
  return OcctL::Geom::ToGp(theA).Dot(OcctL::Geom::ToGp(theB));
}

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_vector2_cross(occtl_vector2_t theA, occtl_vector2_t theB)
{
  return OcctL::Geom::ToGp(theA).Crossed(OcctL::Geom::ToGp(theB));
}

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_vector2_magnitude(occtl_vector2_t theV)
{
  return OcctL::Geom::ToGp(theV).Magnitude();
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_vector2_normalized(occtl_vector2_t  theV,
                                                             occtl_vector2_t* theOutResult)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutResult == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_result is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (OcctL::Geom::ToGp(theV).SquareMagnitude() <= Precision::SquareConfusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "cannot normalise a zero-length vector");
      return OCCTL_GEOMETRY_INVALID;
    }
    gp_Vec2d aV = OcctL::Geom::ToGp(theV);
    aV.Normalize();
    *theOutResult = OcctL::Geom::FromGp(aV);
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_direction3_from_vector(occtl_vector3_t theV, occtl_direction3_t* theOutDirection)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutDirection == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_direction is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (OcctL::Geom::ToGp(theV).SquareMagnitude() <= Precision::SquareConfusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "cannot build a direction from a zero-length vector");
      return OCCTL_GEOMETRY_INVALID;
    }
    *theOutDirection = OcctL::Geom::FromGp(gp_Dir(OcctL::Geom::ToGp(theV)));
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_direction3_dot(occtl_direction3_t theA, occtl_direction3_t theB)
{
  return OcctL::Geom::ToGp(theA).Dot(OcctL::Geom::ToGp(theB));
}

//=================================================================================================

OCCTL_API occtl_vector3_t OCCTL_CALL occtl_direction3_cross(occtl_direction3_t theA,
                                                            occtl_direction3_t theB)
{
  // Use gp_Vec for cross so the zero-vector result (parallel inputs) is valid.
  const gp_Vec aVA(OcctL::Geom::ToGp(theA));
  const gp_Vec aVB(OcctL::Geom::ToGp(theB));
  return OcctL::Geom::FromGp(aVA.Crossed(aVB));
}

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_direction3_angle(occtl_direction3_t theA, occtl_direction3_t theB)
{
  return OcctL::Geom::ToGp(theA).Angle(OcctL::Geom::ToGp(theB));
}

//=================================================================================================

OCCTL_API occtl_direction3_t OCCTL_CALL occtl_direction3_reversed(occtl_direction3_t theD)
{
  return OcctL::Geom::FromGp(OcctL::Geom::ToGp(theD).Reversed());
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_direction3_transform(occtl_direction3_t  theD,
                                                               occtl_transform_t   theT,
                                                               occtl_direction3_t* theOutDirection)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutDirection == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_direction is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const gp_Vec aDirection(OcctL::Geom::ToGp(theD).XYZ());
    const gp_Vec aXformedVec(
      aDirection.XYZ().Multiplied(OcctL::Geom::ToGpGTrsf(theT).VectorialPart()));
    if (aXformedVec.SquareMagnitude() <= Precision::SquareConfusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "transform collapses the direction to zero");
      return OCCTL_GEOMETRY_INVALID;
    }
    *theOutDirection = OcctL::Geom::FromGp(gp_Dir(aXformedVec));
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_direction2_from_vector(occtl_vector2_t theV, occtl_direction2_t* theOutDirection)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutDirection == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_direction is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (OcctL::Geom::ToGp(theV).SquareMagnitude() <= Precision::SquareConfusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "cannot build a direction from a zero-length vector");
      return OCCTL_GEOMETRY_INVALID;
    }
    *theOutDirection = OcctL::Geom::FromGp(gp_Dir2d(OcctL::Geom::ToGp(theV)));
    return OCCTL_OK;
  });
}

//=================================================================================================

OCCTL_API double OCCTL_CALL occtl_direction2_angle(occtl_direction2_t theA, occtl_direction2_t theB)
{
  // gp_Dir2d::Angle returns (-π, π]; take absolute value for [0, π].
  return std::fabs(OcctL::Geom::ToGp(theA).Angle(OcctL::Geom::ToGp(theB)));
}

//=================================================================================================

OCCTL_API occtl_transform_t OCCTL_CALL occtl_transform_identity(void)
{
  return OcctL::Geom::FromGp(gp_Trsf());
}

//=================================================================================================

OCCTL_API occtl_transform_t OCCTL_CALL occtl_transform_translation(occtl_vector3_t theV)
{
  gp_Trsf aT;
  aT.SetTranslation(OcctL::Geom::ToGp(theV));
  return OcctL::Geom::FromGp(aT);
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_rotation(occtl_axis1_placement_t theAxis,
                                                             double                  theAngle,
                                                             occtl_transform_t* theOutTransform)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutTransform == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_transform is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    try
    {
      OCC_CATCH_SIGNALS;
      const gp_Ax1 anAx1(OcctL::Geom::ToGp(theAxis.location), OcctL::Geom::ToGp(theAxis.direction));
      gp_Trsf      aT;
      aT.SetRotation(anAx1, theAngle);
      *theOutTransform = OcctL::Geom::FromGp(aT);
      return OCCTL_OK;
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "rotation axis has zero length");
      return OCCTL_GEOMETRY_INVALID;
    }
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_scale(occtl_point3_t     theCenter,
                                                          double             theS,
                                                          occtl_transform_t* theOutTransform)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutTransform == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_transform is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    try
    {
      OCC_CATCH_SIGNALS;
      gp_Trsf aT;
      aT.SetScale(OcctL::Geom::ToGp(theCenter), theS);
      *theOutTransform = OcctL::Geom::FromGp(aT);
      return OCCTL_OK;
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID, "scale factor is zero");
      return OCCTL_GEOMETRY_INVALID;
    }
  });
}

//=================================================================================================

OCCTL_API occtl_transform_t OCCTL_CALL occtl_transform_compose(occtl_transform_t theFirst,
                                                               occtl_transform_t theSecond)
{
  // second.Multiplied(first) = second * first = "apply first, then second".
  return OcctL::Geom::FromGp(
    OcctL::Geom::ToGpGTrsf(theSecond).Multiplied(OcctL::Geom::ToGpGTrsf(theFirst)));
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_inverted(occtl_transform_t  theT,
                                                             occtl_transform_t* theOutTransform)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutTransform == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_transform is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    try
    {
      OCC_CATCH_SIGNALS;
      *theOutTransform = OcctL::Geom::FromGp(OcctL::Geom::ToGpGTrsf(theT).Inverted());
      return OCCTL_OK;
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "transform is singular (determinant near zero)");
      return OCCTL_GEOMETRY_INVALID;
    }
  });
}

//=================================================================================================

OCCTL_API occtl_vector3_t OCCTL_CALL occtl_transform_apply_vector3(occtl_transform_t theT,
                                                                   occtl_vector3_t   theV)
{
  // VectorialPart() drops the translation column so vectors transform without offset.
  const gp_XYZ aResult =
    OcctL::Geom::ToGp(theV).XYZ().Multiplied(OcctL::Geom::ToGpGTrsf(theT).VectorialPart());
  return {aResult.X(), aResult.Y(), aResult.Z()};
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_from_axis2(occtl_axis2_placement_t theFrame,
                                                               occtl_transform_t* theOutTransform)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutTransform == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_transform is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    try
    {
      OCC_CATCH_SIGNALS;
      const gp_Dir aXDir = OcctL::Geom::ToGp(theFrame.x_dir);
      const gp_Dir aXRef = OcctL::Geom::ToGp(theFrame.x_dir_ref);
      if (aXDir.IsParallel(aXRef, Precision::Angular()))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "axis2 frame: x_dir and x_dir_ref are parallel");
        return OCCTL_GEOMETRY_INVALID;
      }
      const gp_Ax2 anAx2(OcctL::Geom::ToGp(theFrame.location), aXDir, aXRef);
      gp_Trsf      aT;
      aT.SetTransformation(anAx2);
      *theOutTransform = OcctL::Geom::FromGp(aT);
      return OCCTL_OK;
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "axis2 frame has zero-length axes");
      return OCCTL_GEOMETRY_INVALID;
    }
  });
}

//=================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_transform_from_axis3(occtl_axis3_placement_t theFrame,
                                                               occtl_transform_t* theOutTransform)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutTransform == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_transform is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    const gp_Mat aLinear(theFrame.x_dir.x,
                         theFrame.y_dir.x,
                         theFrame.z_dir.x,
                         theFrame.x_dir.y,
                         theFrame.y_dir.y,
                         theFrame.z_dir.y,
                         theFrame.x_dir.z,
                         theFrame.y_dir.z,
                         theFrame.z_dir.z);
    if (std::fabs(aLinear.Determinant()) < Precision::Confusion())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "axis3 frame is degenerate (linearly dependent axes)");
      return OCCTL_GEOMETRY_INVALID;
    }
    gp_GTrsf aGT;
    for (int aRow = 1; aRow <= 3; ++aRow)
    {
      for (int aCol = 1; aCol <= 3; ++aCol)
      {
        aGT.SetValue(aRow, aCol, aLinear.Value(aRow, aCol));
      }
    }
    aGT.SetValue(1, 4, theFrame.location.x);
    aGT.SetValue(2, 4, theFrame.location.y);
    aGT.SetValue(3, 4, theFrame.location.z);
    *theOutTransform = OcctL::Geom::FromGp(aGT);
    return OCCTL_OK;
  });
}

} // extern "C"
