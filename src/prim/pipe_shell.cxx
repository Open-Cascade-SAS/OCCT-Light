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

//! @file pipe_shell.cxx
//! @brief Pipe shell sweep implementation with trihedron, corner-transition,
//!        contact, correction, and law controls for non-trivial sweeps.

#include "PrimMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/CurveMath.hxx"
#include "../topo/IdConvert.hxx"

#include <occtl/occtl_prim.h>

#include <BRepBuilderAPI_TransitionMode.hxx>
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <NCollection_Array1.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <Law_Interpol.hxx>
#include <Law_Linear.hxx>

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt2d.hxx>

#include <cmath>
#include <limits>

namespace
{

inline bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

inline BRepBuilderAPI_TransitionMode toOcctTransition(const occtl_prim_pipe_transition_t theMode)
{
  switch (theMode)
  {
    case OCCTL_PIPE_TRANSITION_RIGHT_CORNER:
      return BRepBuilderAPI_RightCorner;
    case OCCTL_PIPE_TRANSITION_ROUND_CORNER:
      return BRepBuilderAPI_RoundCorner;
    default:
      return BRepBuilderAPI_Transformed;
  }
}

occtl_status_t configurePipeShellMode(BRepOffsetAPI_MakePipeShell&   theMaker,
                                      const occtl_prim_pipe_mode_t   theMode,
                                      const occtl_axis2_placement_t& theModeAxis,
                                      const occtl_direction3_t&      theModeBinormal)
{
  switch (theMode)
  {
    case OCCTL_PIPE_MODE_CORRECTED_FRENET:
      theMaker.SetMode(/* IsFrenet */ false);
      return OCCTL_OK;
    case OCCTL_PIPE_MODE_FRENET:
      theMaker.SetMode(/* IsFrenet */ true);
      return OCCTL_OK;
    case OCCTL_PIPE_MODE_DISCRETE:
      theMaker.SetDiscreteMode();
      return OCCTL_OK;
    case OCCTL_PIPE_MODE_CONSTANT_AXIS:
      if (OcctL::Prim::CheckFinitePlacement(theModeAxis, "mode_axis") != OCCTL_OK)
      {
        return OCCTL_INVALID_ARGUMENT;
      }
      theMaker.SetMode(OcctL::Geom::ToGpAx2(theModeAxis));
      return OCCTL_OK;
    case OCCTL_PIPE_MODE_CONSTANT_BINORMAL: {
      if (OcctL::Prim::CheckFiniteVec3(theModeBinormal, "mode_binormal") != OCCTL_OK)
      {
        return OCCTL_INVALID_ARGUMENT;
      }
      if (const occtl_status_t aStatus =
            OcctL::Prim::CheckDirection(theModeBinormal, "mode_binormal"))
      {
        return aStatus;
      }

      theMaker.SetMode(gp_Dir(theModeBinormal.x, theModeBinormal.y, theModeBinormal.z));
      return OCCTL_OK;
    }
    default:
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "unknown pipe mode");
      return OCCTL_INVALID_ARGUMENT;
  }
}

occtl_status_t resolveSpineWire(occtl_graph_t* const  theGraph,
                                const occtl_node_id_t theSpineWire,
                                TopoDS_Wire&          theWire)
{
  BRepGraph_NodeId aSpineId;
  if (const occtl_status_t aStatus =
        OcctL::Topo::ToTypedId(theGraph, theSpineWire, BRepGraph_NodeId::Kind::Wire, aSpineId))
  {
    return aStatus;
  }

  const TopoDS_Shape aSpineShape = theGraph->graph.Shapes().Shape(aSpineId);
  if (aSpineShape.IsNull() || aSpineShape.ShapeType() != TopAbs_WIRE)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                           "spine_wire could not be reconstructed as TopoDS_Wire");
    return OCCTL_NOT_FOUND;
  }

  theWire = TopoDS::Wire(aSpineShape);
  return OCCTL_OK;
}

occtl_status_t addPipeShellResult(occtl_graph_t* const         theGraph,
                                  BRepOffsetAPI_MakePipeShell& theMaker,
                                  const int32_t                theMakeSolid,
                                  occtl_node_id_t&             theOutShape)
{
  theMaker.Build();
  if (!theMaker.IsDone())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "BRepOffsetAPI_MakePipeShell reported IsDone()==false");
    return OCCTL_GEOMETRY_INVALID;
  }

  if (theMakeSolid != 0)
  {
    if (!theMaker.MakeSolid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                             "MakeSolid() failed (result shell is not closed)");
      return OCCTL_GEOMETRY_INVALID;
    }
  }
  return OcctL::Prim::AddTopologyRoot(theGraph, theMaker.Shape(), theOutShape);
}

occtl_status_t validateInterpolatedLawSamples(
  const occtl_prim_pipe_shell_interpolated_law_info_t& theInfo)
{
  if (theInfo.parameters == nullptr || theInfo.scales == nullptr || theInfo.sample_count < 2)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "interpolated law requires non-NULL parameters/scales and at least 2 samples");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theInfo.sample_count > static_cast<size_t>((std::numeric_limits<int>::max)()))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "interpolated law sample_count is too large");
    return OCCTL_INVALID_ARGUMENT;
  }

  double aPreviousParameter = theInfo.parameters[0];
  if (!IsFiniteValue(aPreviousParameter) || std::abs(aPreviousParameter) > 1.0e-12)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "first law parameter must be 0.0");
    return OCCTL_INVALID_ARGUMENT;
  }

  for (size_t anI = 0; anI < theInfo.sample_count; ++anI)
  {
    const double aParameter = theInfo.parameters[anI];
    const double aScale     = theInfo.scales[anI];
    if (!IsFiniteValue(aParameter) || !IsFiniteValue(aScale) || aScale <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "law parameters must be finite and law scales must be finite and positive");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (anI > 0 && aParameter <= aPreviousParameter)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "law parameters must be strictly increasing");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (aParameter < 0.0 || aParameter > 1.0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "law parameters must be normalized to [0, 1]");
      return OCCTL_INVALID_ARGUMENT;
    }
    aPreviousParameter = aParameter;
  }

  if (std::abs(theInfo.parameters[theInfo.sample_count - 1] - 1.0) > 1.0e-12)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "last law parameter must be 1.0");
    return OCCTL_INVALID_ARGUMENT;
  }

  return OCCTL_OK;
}

occtl_status_t validatePipeShellBooleans(const int32_t theWithContact,
                                         const int32_t theWithCorrection,
                                         const int32_t theMakeSolid)
{
  if (OcctL::Prim::CheckBool(theWithContact, "with_contact") != OCCTL_OK
      || OcctL::Prim::CheckBool(theWithCorrection, "with_correction") != OCCTL_OK
      || OcctL::Prim::CheckBool(theMakeSolid, "make_solid") != OCCTL_OK)
  {
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}
} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_pipe_shell_info_init(occtl_prim_pipe_shell_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_prim_pipe_shell_linear_law_info_init(occtl_prim_pipe_shell_linear_law_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_prim_pipe_shell_interpolated_law_info_init(
  occtl_prim_pipe_shell_interpolated_law_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_pipe_shell(occtl_graph_t* const                      theGraph,
                             const occtl_prim_pipe_shell_info_t* const theInfo,
                             occtl_node_id_t* const                    theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_PIPE_SHELL_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_PIPE_SHELL_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->profiles == nullptr || theInfo->profile_count == 0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "pipe_shell requires at least 1 profile in a non-NULL array");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (validatePipeShellBooleans(theInfo->with_contact,
                                  theInfo->with_correction,
                                  theInfo->make_solid)
        != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    TopoDS_Wire aSpineWire;
    if (const occtl_status_t aStatus = resolveSpineWire(theGraph, theInfo->spine_wire, aSpineWire))
    {
      return aStatus;
    }

    BRepOffsetAPI_MakePipeShell aMaker(aSpineWire);
    if (const occtl_status_t aStatus =
          configurePipeShellMode(aMaker, theInfo->mode, theInfo->mode_axis, theInfo->mode_binormal))
    {
      return aStatus;
    }

    aMaker.SetTransitionMode(toOcctTransition(theInfo->transition));

    for (size_t anI = 0; anI < theInfo->profile_count; ++anI)
    {
      if (const occtl_status_t aStatus =
            OcctL::Prim::CheckProfileKind(theInfo->profiles[anI], "profile"))
      {
        return aStatus;
      }

      TopoDS_Shape aProfileShape;
      if (const occtl_status_t aStatus = OcctL::Prim::ResolveProfileShape(theGraph,
                                                                          theInfo->profiles[anI],
                                                                          "profile",
                                                                          aProfileShape))
      {
        return aStatus;
      }

      aMaker.Add(aProfileShape, theInfo->with_contact != 0, theInfo->with_correction != 0);
    }

    return addPipeShellResult(theGraph, aMaker, theInfo->make_solid, *theOutShape);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_pipe_shell_linear_law(
  occtl_graph_t* const                                 theGraph,
  const occtl_prim_pipe_shell_linear_law_info_t* const theInfo,
  occtl_node_id_t* const                               theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (!IsFiniteValue(theInfo->scale_first) || !IsFiniteValue(theInfo->scale_last)
        || theInfo->scale_first <= 0.0 || theInfo->scale_last <= 0.0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "scale_first and scale_last must be finite and positive");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (validatePipeShellBooleans(theInfo->with_contact,
                                  theInfo->with_correction,
                                  theInfo->make_solid)
        != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    TopoDS_Wire aSpineWire;
    if (const occtl_status_t aStatus = resolveSpineWire(theGraph, theInfo->spine_wire, aSpineWire))
    {
      return aStatus;
    }

    if (const occtl_status_t aStatus = OcctL::Prim::CheckProfileKind(theInfo->profile, "profile"))
    {
      return aStatus;
    }

    TopoDS_Shape aProfileShape;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->profile, "profile", aProfileShape))
    {
      return aStatus;
    }

    BRepOffsetAPI_MakePipeShell aMaker(aSpineWire);
    if (const occtl_status_t aStatus =
          configurePipeShellMode(aMaker, theInfo->mode, theInfo->mode_axis, theInfo->mode_binormal))
    {
      return aStatus;
    }

    aMaker.SetTransitionMode(toOcctTransition(theInfo->transition));

    occ::handle<Law_Linear> aLaw = new Law_Linear();
    aLaw->Set(0.0, theInfo->scale_first, 1.0, theInfo->scale_last);
    aMaker.SetLaw(aProfileShape, aLaw, theInfo->with_contact != 0, theInfo->with_correction != 0);

    return addPipeShellResult(theGraph, aMaker, theInfo->make_solid, *theOutShape);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_pipe_shell_interpolated_law(
  occtl_graph_t* const                                       theGraph,
  const occtl_prim_pipe_shell_interpolated_law_info_t* const theInfo,
  occtl_node_id_t* const                                     theOutShape)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theInfo == nullptr || theOutShape == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_shape is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (theInfo->struct_version != OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_VERSION_1)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_VERSION_MISMATCH,
        "info->struct_version is not OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_VERSION_1");
      return OCCTL_VERSION_MISMATCH;
    }
    if (theInfo->p_next != nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info->p_next must be NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = validateInterpolatedLawSamples(*theInfo))
    {
      return aStatus;
    }
    if (validatePipeShellBooleans(theInfo->with_contact,
                                  theInfo->with_correction,
                                  theInfo->make_solid)
        != OCCTL_OK)
    {
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutShape = OCCTL_NODE_ID_INVALID;

    TopoDS_Wire aSpineWire;
    if (const occtl_status_t aStatus = resolveSpineWire(theGraph, theInfo->spine_wire, aSpineWire))
    {
      return aStatus;
    }

    if (const occtl_status_t aStatus = OcctL::Prim::CheckProfileKind(theInfo->profile, "profile"))
    {
      return aStatus;
    }

    TopoDS_Shape aProfileShape;
    if (const occtl_status_t aStatus =
          OcctL::Prim::ResolveProfileShape(theGraph, theInfo->profile, "profile", aProfileShape))
    {
      return aStatus;
    }

    BRepOffsetAPI_MakePipeShell aMaker(aSpineWire);
    if (const occtl_status_t aStatus =
          configurePipeShellMode(aMaker, theInfo->mode, theInfo->mode_axis, theInfo->mode_binormal))
    {
      return aStatus;
    }

    aMaker.SetTransitionMode(toOcctTransition(theInfo->transition));

    NCollection_Array1<gp_Pnt2d> aSamples(1, static_cast<int>(theInfo->sample_count));
    for (int anI = 1; anI <= static_cast<int>(theInfo->sample_count); ++anI)
    {
      const size_t anIndex = static_cast<size_t>(anI - 1);
      aSamples.SetValue(anI, gp_Pnt2d(theInfo->parameters[anIndex], theInfo->scales[anIndex]));
    }

    occ::handle<Law_Interpol> aLaw = new Law_Interpol();
    aLaw->Set(aSamples, /* Periodic */ false);
    aMaker.SetLaw(aProfileShape, aLaw, theInfo->with_contact != 0, theInfo->with_correction != 0);

    return addPipeShellResult(theGraph, aMaker, theInfo->make_solid, *theOutShape);
  });
}

} // extern "C"
