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

//! @file extern_c.cxx
//! @brief Public C ABI entry points for the heal module.

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <occtl/occtl_heal.h>

#include <BRepBuilderAPI_Copy.hxx>
#include <BRepGraph_NodeId.hxx>
#include <BRepGraph_ShapesView.hxx>

#include <ShapeFix_Face.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>

#include <Standard_ErrorHandler.hxx>
#include <TopoDS_Shape.hxx>
#include <Precision.hxx>

#include <cmath>

namespace
{

bool IsZeroOrOne(const int32_t theValue)
{
  return theValue == 0 || theValue == 1;
}

bool IsFiniteValue(const double theValue)
{
  return !std::isnan(theValue) && !Precision::IsInfinite(theValue);
}

bool ResolveShape(const occtl_graph_t* const theGraph,
                  const occtl_node_id_t      theNodeId,
                  TopoDS_Shape&              theOutShape)
{
  const BRepGraph_NodeId aNode = OcctL::Topo::UnpackNodeId(theNodeId);
  if (!aNode.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND, "node_id is invalid or removed");
    return false;
  }

  theOutShape = theGraph->graph.Shapes().Shape(aNode);
  if (theOutShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                           "node_id could not be reconstructed as TopoDS shape");
    return false;
  }
  return true;
}

occtl_status_t AddShapeToGraph(occtl_graph_t* const theGraph,
                               const TopoDS_Shape&  theShape,
                               occtl_node_id_t*     theOutRoot)
{
  if (theShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_TOPOLOGY_INVALID, "result shape is null");
    return OCCTL_TOPOLOGY_INVALID;
  }

  BRepBuilderAPI_Copy aCopier(theShape);
  const TopoDS_Shape  aCopy = aCopier.Shape();

  BRepGraph::ShapesView::Options anBuildOpts;
  anBuildOpts.CreateAutoProduct = false;

  const BRepGraph::ShapesView::Result aBuildRes = theGraph->graph.Shapes().Add(aCopy, anBuildOpts);
  if (!aBuildRes.Ok)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      "BRepGraph::ShapesView::Add failed to ingest result shape");
    return OCCTL_TOPOLOGY_INVALID;
  }

  if (theOutRoot != nullptr)
  {
    *theOutRoot = OcctL::Topo::PackNodeId(aBuildRes.TopologyRoot);
  }
  return OCCTL_OK;
}

occtl_status_t ValidateHealOptions(const occtl_heal_options_t* const theOptions)
{
  if (theOptions == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOptions->struct_version != OCCTL_HEAL_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH, "unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOptions->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "options->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOptions->mode != OCCTL_HEAL_MODE_BASIC && theOptions->mode != OCCTL_HEAL_MODE_STANDARD
      && theOptions->mode != OCCTL_HEAL_MODE_FULL)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE, "options->mode is out of range");
    return OCCTL_OUT_OF_RANGE;
  }
  if (!IsFiniteValue(theOptions->tolerance) || theOptions->tolerance < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "options->tolerance must be finite and non-negative");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOptions->fix_same_parameter) || !IsZeroOrOne(theOptions->fix_small_edges)
      || !IsZeroOrOne(theOptions->fix_face_orient) || !IsZeroOrOne(theOptions->fix_missing_seam))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "options fix flags must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

occtl_status_t ValidateUnifyOptions(const occtl_heal_unify_same_domain_options_t* const theOptions)
{
  if (theOptions == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOptions->struct_version != OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH, "unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOptions->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "options->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOptions->unify_edges) || !IsZeroOrOne(theOptions->unify_faces)
      || !IsZeroOrOne(theOptions->concat_bspline) || !IsZeroOrOne(theOptions->allow_internal_edges)
      || !IsZeroOrOne(theOptions->safe_input))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "options boolean flags must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsFiniteValue(theOptions->linear_tolerance) || theOptions->linear_tolerance < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "options->linear_tolerance must be finite and non-negative");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsFiniteValue(theOptions->angular_tolerance) || theOptions->angular_tolerance < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "options->angular_tolerance must be finite and non-negative");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_heal_options_init(occtl_heal_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions = OCCTL_HEAL_OPTIONS_INIT;
  }
}

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_heal_unify_same_domain_options_init(
  occtl_heal_unify_same_domain_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions = OCCTL_HEAL_UNIFY_SAME_DOMAIN_OPTIONS_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_heal_shape(occtl_graph_t* const              theGraph,
                                                     const occtl_node_id_t             theNodeId,
                                                     const occtl_heal_options_t* const theOptions)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    // --- validate arguments -------------------------------------------------
    if (theGraph == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    occtl_heal_options_t aOpts;
    if (const occtl_status_t aStatus = ValidateHealOptions(theOptions))
    {
      return aStatus;
    }
    if (theOptions != nullptr)
    {
      aOpts = *theOptions;
    }
    else
    {
      occtl_heal_options_init(&aOpts);
    }

    TopoDS_Shape aShape;
    if (!ResolveShape(theGraph, theNodeId, aShape))
    {
      return OCCTL_NOT_FOUND;
    }

    OCC_CATCH_SIGNALS;

    // --- configure ShapeFix_Shape -------------------------------------------
    Handle(ShapeFix_Shape) aFixer = new ShapeFix_Shape(aShape);

    const double aPrecision = aOpts.tolerance > 0.0 ? aOpts.tolerance : Precision::Confusion();
    aFixer->SetPrecision(aPrecision);

    if (aOpts.mode >= OCCTL_HEAL_MODE_BASIC)
    {
      aFixer->FixWireTool()->FixReorderMode()   = 1;
      aFixer->FixWireTool()->FixConnectedMode() = 1;
      aFixer->FixFaceTool()->FixWireMode()      = 1;
    }

    if (aOpts.mode >= OCCTL_HEAL_MODE_STANDARD)
    {
      aFixer->FixFaceTool()->FixOrientationMode() = aOpts.fix_face_orient;
      aFixer->FixFreeShellMode()                  = 1;
      aFixer->FixSolidMode()                      = 1;
    }

    if (aOpts.mode >= OCCTL_HEAL_MODE_FULL)
    {
      aFixer->FixSameParameterMode()        = aOpts.fix_same_parameter;
      aFixer->FixWireTool()->FixSmallMode() = aOpts.fix_small_edges;
      aFixer->FixWireTool()->FixSeamMode()  = aOpts.fix_missing_seam;
    }

    // --- run healing --------------------------------------------------------
    aFixer->Perform();

    // --- ingest result back into graph --------------------------------------
    const TopoDS_Shape aResult = aFixer->Shape();
    return AddShapeToGraph(theGraph, aResult, nullptr);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_heal_unify_same_domain(occtl_graph_t* const                                theGraph,
                               const occtl_node_id_t                               theNodeId,
                               const occtl_heal_unify_same_domain_options_t* const theOptions,
                               occtl_node_id_t* const                              theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "graph or out_root is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    occtl_heal_unify_same_domain_options_t anOpts;
    if (const occtl_status_t aStatus = ValidateUnifyOptions(theOptions))
    {
      return aStatus;
    }
    if (theOptions != nullptr)
    {
      anOpts = *theOptions;
    }
    else
    {
      occtl_heal_unify_same_domain_options_init(&anOpts);
    }

    TopoDS_Shape aShape;
    if (!ResolveShape(theGraph, theNodeId, aShape))
    {
      return OCCTL_NOT_FOUND;
    }

    OCC_CATCH_SIGNALS;

    ShapeUpgrade_UnifySameDomain aUnifier(aShape,
                                          anOpts.unify_edges != 0,
                                          anOpts.unify_faces != 0,
                                          anOpts.concat_bspline != 0);
    aUnifier.AllowInternalEdges(anOpts.allow_internal_edges != 0);
    aUnifier.SetSafeInputMode(anOpts.safe_input != 0);
    if (anOpts.linear_tolerance > 0.0)
    {
      aUnifier.SetLinearTolerance(anOpts.linear_tolerance);
    }
    if (anOpts.angular_tolerance > 0.0)
    {
      aUnifier.SetAngularTolerance(anOpts.angular_tolerance);
    }

    aUnifier.Build();
    return AddShapeToGraph(theGraph, aUnifier.Shape(), theOutRoot);
  });
}

} // extern "C"
