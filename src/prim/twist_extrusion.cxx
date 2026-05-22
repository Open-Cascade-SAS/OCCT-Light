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

//! @file twist_extrusion.cxx
//! @brief Twisted extrusion primitive built from transformed Wire sections
//!        and OCCT through-sections.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/CurveMath.hxx"
#include "../topo/IdConvert.hxx"

#include <occtl/occtl_prim.h>

#include <BRepBuilderAPI_Transform.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>

#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_twist_extrusion_info_init(occtl_prim_twist_extrusion_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_extrude_twist_info_init(occtl_prim_extrude_twist_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_EXTRUDE_TWIST_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_twist_extrusion(occtl_graph_t* const                           theGraph,
                                  const occtl_prim_twist_extrusion_info_t* const theInfo,
                                  occtl_node_id_t* const                         theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_TWIST_EXTRUSION_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_TWIST_EXTRUSION_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckFiniteAxis1(theInfo->axis, "axis") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->height, "height") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->angle, "angle") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->pres3d, "pres3d") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->make_solid, "make_solid") != OCCTL_OK
        || OcctL::Prim::CheckBool(theInfo->ruled, "ruled") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->height == 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "height must be non-zero");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->section_count < 2)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "section_count must be at least 2");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus =
          OcctL::Prim::CheckDirection(theInfo->axis.direction, "axis direction"))
    {
      return aStatus;
    }

    BRepGraph_NodeId aProfileId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theInfo->profile_wire,
                                                              BRepGraph_NodeId::Kind::Wire,
                                                              aProfileId))
    {
      return aStatus;
    }

    const TopoDS_Shape aProfileShape = theGraph->graph.Shapes().Shape(aProfileId);
    if (aProfileShape.IsNull() || aProfileShape.ShapeType() != TopAbs_WIRE)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "profile_wire could not be reconstructed as TopoDS_Wire");
      return OCCTL_NOT_FOUND;
    }

    const occtl_direction3_t& anAxisDir = theInfo->axis.direction;
    const gp_Dir              aDir(anAxisDir.x, anAxisDir.y, anAxisDir.z);
    const gp_Ax1              anAxis(OcctL::Geom::ToGp(theInfo->axis.location), aDir);
    const gp_Vec              aTranslation(aDir.XYZ() * theInfo->height);

    BRepOffsetAPI_ThruSections aMaker(theInfo->make_solid != 0,
                                      theInfo->ruled != 0,
                                      theInfo->pres3d);

    for (int anI = 0; anI < theInfo->section_count; ++anI)
    {
      const double aT = static_cast<double>(anI) / static_cast<double>(theInfo->section_count - 1);

      gp_Trsf aTrsf;
      aTrsf.SetRotation(anAxis, theInfo->angle * aT);
      aTrsf.SetTranslationPart(aTranslation * aT);

      BRepBuilderAPI_Transform aTransform(aProfileShape, aTrsf, true);
      if (!aTransform.IsDone())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "OCCT failed to transform a twist section");
        return OCCTL_GEOMETRY_INVALID;
      }

      const TopoDS_Shape aSection = aTransform.Shape();
      if (aSection.IsNull() || aSection.ShapeType() != TopAbs_WIRE)
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                               "twist section is not a reconstructed Wire");
        return OCCTL_GEOMETRY_INVALID;
      }
      aMaker.AddWire(TopoDS::Wire(aSection));
    }

    aMaker.Build();
    if (!aMaker.IsDone() || aMaker.Shape().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "BRepOffsetAPI_ThruSections rejected twist sections");
      return OCCTL_GEOMETRY_INVALID;
    }
    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutShape);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_extrude_twist(occtl_graph_t* const                         theGraph,
                                const occtl_prim_extrude_twist_info_t* const theInfo,
                                occtl_node_id_t* const                       theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;
    if (theInfo->struct_version != OCCTL_PRIM_EXTRUDE_TWIST_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_EXTRUDE_TWIST_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    occtl_prim_twist_extrusion_info_t aTwistInfo = OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT;
    aTwistInfo.profile_wire                      = theInfo->profile_wire;
    aTwistInfo.axis                              = theInfo->axis;
    aTwistInfo.height                            = theInfo->height;
    aTwistInfo.angle                             = theInfo->angle;
    aTwistInfo.section_count                     = theInfo->section_count;
    aTwistInfo.make_solid                        = theInfo->make_solid;
    aTwistInfo.ruled                             = theInfo->ruled;
    aTwistInfo.pres3d                            = theInfo->pres3d;
    return occtl_prim_make_twist_extrusion(theGraph, &aTwistInfo, theOutShape);
  });
}

} // extern "C"
