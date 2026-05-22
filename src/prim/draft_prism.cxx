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

//! @file draft_prism.cxx
//! @brief Tapered prism wrapper around OCCT LocOpe_DPrism.

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/IdConvert.hxx"
#include "PrimMath.hxx"

#include <occtl/occtl_prim.h>

#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <LocOpe_DPrism.hxx>

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_draft_prism_info_init(occtl_prim_draft_prism_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_extrude_tapered_info_init(occtl_prim_extrude_tapered_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_EXTRUDE_TAPERED_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_draft_prism(occtl_graph_t* const                       theGraph,
                              const occtl_prim_draft_prism_info_t* const theInfo,
                              occtl_node_id_t* const                     theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_DRAFT_PRISM_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_DRAFT_PRISM_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    if (OcctL::Prim::CheckPNext(theInfo->p_next, "occtl_prim_draft_prism_info_t") != OCCTL_OK
        || OcctL::Prim::CheckPositive(theInfo->height, "height") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->taper_angle, "taper_angle") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    BRepGraph_NodeId aProfileId;
    if (const occtl_status_t aStatus = OcctL::Topo::ToTypedId(theGraph,
                                                              theInfo->profile,
                                                              BRepGraph_NodeId::Kind::Face,
                                                              aProfileId))
    {
      return aStatus;
    }

    const TopoDS_Shape aProfileShape = theGraph->graph.Shapes().Shape(aProfileId);
    if (aProfileShape.IsNull() || aProfileShape.ShapeType() != TopAbs_FACE)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "profile could not be reconstructed as TopoDS_Face");
      return OCCTL_NOT_FOUND;
    }

    LocOpe_DPrism aMaker(TopoDS::Face(aProfileShape), theInfo->height, theInfo->taper_angle);
    if (!aMaker.IsDone() || aMaker.Shape().IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "LocOpe_DPrism failed to build a shape");
      return OCCTL_GEOMETRY_INVALID;
    }

    return OcctL::Prim::AddTopologyRoot(theGraph, aMaker.Shape(), *theOutShape);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_extrude_tapered(occtl_graph_t* const                           theGraph,
                                  const occtl_prim_extrude_tapered_info_t* const theInfo,
                                  occtl_node_id_t* const                         theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;
    if (theInfo->struct_version != OCCTL_PRIM_EXTRUDE_TAPERED_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_EXTRUDE_TAPERED_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (OcctL::Prim::CheckPositive(theInfo->height, "height") != OCCTL_OK
        || OcctL::Prim::CheckFinite(theInfo->taper_angle, "taper_angle") != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }

    occtl_prim_draft_prism_info_t aDraftInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
    aDraftInfo.profile                       = theInfo->profile_face;
    aDraftInfo.height                        = theInfo->height;
    aDraftInfo.taper_angle                   = theInfo->taper_angle;
    return occtl_prim_make_draft_prism(theGraph, &aDraftInfo, theOutShape);
  });
}

} // extern "C"
