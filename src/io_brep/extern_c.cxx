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

#include <BRepGraph.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_LinearVector.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopoDS_Shape.hxx>
#include <DEBREP_ConfigurationNode.hxx>
#include <DEBREP_Provider.hxx>

#include <occtl/occtl_io_brep.h>

#include <limits>
#include <memory>

#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

namespace
{

bool IsZeroOrOne(const int32_t theValue)
{
  return theValue == 0 || theValue == 1;
}

occtl_status_t AddShapeToFreshGraph(const TopoDS_Shape&    theShape,
                                    occtl_graph_t** const  theOutGraph,
                                    occtl_node_id_t* const theOutRoot,
                                    const char* const      theContext)
{
  if (theShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_FORMAT_ERROR,
      (TCollection_AsciiString(theContext) + ": null shape").ToCString());
    return OCCTL_FORMAT_ERROR;
  }

  std::unique_ptr<occtl_graph_t> aGraph = std::make_unique<occtl_graph_t>();
  BRepGraph::ShapesView::Options anOpts;
  anOpts.CreateAutoProduct                 = false;
  anOpts.TrackAddedNodes                   = false;
  const BRepGraph::ShapesView::Result aRes = aGraph->graph.Shapes().Add(theShape, anOpts);
  if (!aRes.Ok)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_TOPOLOGY_INVALID,
      (TCollection_AsciiString(theContext) + ": shape did not ingest into a graph").ToCString());
    return OCCTL_TOPOLOGY_INVALID;
  }

  *theOutGraph = aGraph.release();
  *theOutRoot  = OcctL::Topo::PackNodeId(aRes.TopologyRoot);
  return OCCTL_OK;
}

occtl_status_t ResolveRootShape(const occtl_graph_t* const theGraph,
                                const occtl_node_id_t      theRoot,
                                const char* const          theContext,
                                TopoDS_Shape&              theOutShape)
{
  if (theGraph == nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      (TCollection_AsciiString(theContext) + ": graph is NULL").ToCString());
    return OCCTL_INVALID_ARGUMENT;
  }
  const BRepGraph_NodeId aRootId = OcctL::Topo::UnpackNodeId(theRoot);
  if (!aRootId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      (TCollection_AsciiString(theContext) + ": root NodeId is invalid").ToCString());
    return OCCTL_NOT_FOUND;
  }
  theOutShape = theGraph->graph.Shapes().Shape(aRootId);
  if (theOutShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      (TCollection_AsciiString(theContext) + ": root resolved to a null shape").ToCString());
    return OCCTL_NOT_FOUND;
  }
  return OCCTL_OK;
}

occtl_status_t ValidateWriteOptions(const occtl_io_brep_write_options_t* const theOpts,
                                    const char* const                          theContext)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_BREP_WRITE_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      (TCollection_AsciiString(theContext) + ": unsupported write options struct_version")
        .ToCString());
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      (TCollection_AsciiString(theContext) + ": write options p_next must be NULL").ToCString());
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOpts->write_triangulation))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      (TCollection_AsciiString(theContext) + ": write_triangulation must be 0 or 1").ToCString());
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}
} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_brep_write_options_init(occtl_io_brep_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_brep_write_options_t aInit = OCCTL_IO_BREP_WRITE_OPTIONS_INIT;
  *theOpts                                  = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_io_brep_read(const char* const      thePath,
                                                       occtl_graph_t** const  theOutGraph,
                                                       occtl_node_id_t* const theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_io_brep_read: NULL argument");
      if (theOutGraph != nullptr)
      {
        *theOutGraph = nullptr;
      }
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    DEBREP_Provider aProvider;
    TopoDS_Shape    aShape;
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Read(TCollection_AsciiString(thePath), aShape))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_IO_ERROR,
                                               "occtl_io_brep_read: DEBREP_Provider::Read failed");
        return OCCTL_IO_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        "occtl_io_brep_read: DEBREP_Provider::Read threw exception");
      return OCCTL_IO_ERROR;
    }
    return AddShapeToFreshGraph(aShape, theOutGraph, theOutRoot, "occtl_io_brep_read");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_brep_write(const occtl_graph_t* const                 theGraph,
                      const occtl_node_id_t                      theRoot,
                      const char* const                          thePath,
                      const occtl_io_brep_write_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || thePath == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "path is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = ValidateWriteOptions(theOpts, "occtl_io_brep_write"))
    {
      return aStatus;
    }
    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          ResolveRootShape(theGraph, theRoot, "occtl_io_brep_write", aShape))
    {
      return aStatus;
    }

    Handle(DEBREP_ConfigurationNode) aNode = new DEBREP_ConfigurationNode();
    const bool aTris = theOpts == nullptr || theOpts->write_triangulation != 0;
    aNode->InternalParameters.WriteTriangles = aTris;

    DEBREP_Provider aProvider(aNode);
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Write(TCollection_AsciiString(thePath), aShape))
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_IO_ERROR,
          "occtl_io_brep_write: DEBREP_Provider::Write failed");
        return OCCTL_IO_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        "occtl_io_brep_write: DEBREP_Provider::Write threw exception");
      return OCCTL_IO_ERROR;
    }
    return OCCTL_OK;
  });
}

} // extern "C"
