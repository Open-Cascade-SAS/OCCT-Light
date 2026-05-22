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
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopoDS_Shape.hxx>
#include <DEIGES_ConfigurationNode.hxx>
#include <DEIGES_Provider.hxx>

#include <occtl/occtl_io_iges.h>

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

occtl_status_t ValidateReadOptions(const occtl_io_iges_read_options_t* const theOpts,
                                   const char* const                         theContext)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_IGES_READ_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": unsupported read options struct_version"));
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": read options p_next must be NULL"));
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOpts->read_color) || !IsZeroOrOne(theOpts->read_name))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": read options flags read_color/read_name must be 0 or 1"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

occtl_status_t ValidateWriteOptions(const occtl_io_iges_write_options_t* const theOpts,
                                    const char* const                          theContext)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_IGES_WRITE_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": unsupported write options struct_version"));
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": write options p_next must be NULL"));
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOpts->write_brep))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": write_brep must be 0 or 1"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

Handle(DEIGES_ConfigurationNode) MakeReadNode(const occtl_io_iges_read_options_t* const theOpts)
{
  Handle(DEIGES_ConfigurationNode) aNode      = new DEIGES_ConfigurationNode();
  const bool                       aReadColor = theOpts == nullptr || theOpts->read_color != 0;
  const bool                       aReadName  = theOpts == nullptr || theOpts->read_name != 0;
  aNode->InternalParameters.ReadColor         = aReadColor;
  aNode->InternalParameters.ReadName          = aReadName;
  return aNode;
}

Handle(DEIGES_ConfigurationNode) MakeWriteNode(const occtl_io_iges_write_options_t* const theOpts)
{
  Handle(DEIGES_ConfigurationNode) aNode = new DEIGES_ConfigurationNode();
  if (theOpts != nullptr && theOpts->write_brep)
  {
    aNode->InternalParameters.WriteBRepMode = DEIGES_Parameters::WriteMode_BRep_BRep;
  }
  return aNode;
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
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": provider returned null shape"));
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
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": shape did not ingest into a graph"));
    return OCCTL_TOPOLOGY_INVALID;
  }

  *theOutGraph = aGraph.release();
  *theOutRoot  = OcctL::Topo::PackNodeId(aRes.TopologyRoot);
  return OCCTL_OK;
}

occtl_status_t ReadPathToGraph(const char* const                         thePath,
                               occtl_graph_t** const                     theOutGraph,
                               occtl_node_id_t* const                    theOutRoot,
                               const occtl_io_iges_read_options_t* const theOpts,
                               const char* const                         theContext)
{
  DEIGES_Provider aProvider(MakeReadNode(theOpts));
  TopoDS_Shape    aShape;
  try
  {
    OCC_CATCH_SIGNALS;
    if (!aProvider.Read(TCollection_AsciiString(thePath), aShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DEIGES_Provider::Read failed"));
      return OCCTL_IO_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_IO_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DEIGES_Provider::Read threw exception"));
    return OCCTL_IO_ERROR;
  }
  return AddShapeToFreshGraph(aShape, theOutGraph, theOutRoot, theContext);
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
      static_cast<std::string_view>(TCollection_AsciiString(theContext) + ": graph is NULL"));
    return OCCTL_INVALID_ARGUMENT;
  }
  const BRepGraph_NodeId aRootId = OcctL::Topo::UnpackNodeId(theRoot);
  if (!aRootId.IsValid())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": root NodeId is invalid"));
    return OCCTL_NOT_FOUND;
  }
  theOutShape = theGraph->graph.Shapes().Shape(aRootId);
  if (theOutShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_NOT_FOUND,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": root resolved to a null shape"));
    return OCCTL_NOT_FOUND;
  }
  return OCCTL_OK;
}

occtl_status_t WriteShapeToPath(const TopoDS_Shape&                        theShape,
                                const char* const                          thePath,
                                const occtl_io_iges_write_options_t* const theOpts,
                                const char* const                          theContext)
{
  DEIGES_Provider aProvider(MakeWriteNode(theOpts));
  try
  {
    OCC_CATCH_SIGNALS;
    if (!aProvider.Write(TCollection_AsciiString(thePath), theShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DEIGES_Provider::Write failed"));
      return OCCTL_IO_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_IO_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DEIGES_Provider::Write threw exception"));
    return OCCTL_IO_ERROR;
  }
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_iges_read_options_init(occtl_io_iges_read_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_iges_read_options_t aInit = OCCTL_IO_IGES_READ_OPTIONS_INIT;
  *theOpts                                 = aInit;
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_iges_write_options_init(occtl_io_iges_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_iges_write_options_t aInit = OCCTL_IO_IGES_WRITE_OPTIONS_INIT;
  *theOpts                                  = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_iges_read(const char* const                         thePath,
                     occtl_graph_t** const                     theOutGraph,
                     occtl_node_id_t* const                    theOutRoot,
                     const occtl_io_iges_read_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_io_iges_read: NULL argument");
      if (theOutGraph != nullptr)
      {
        *theOutGraph = nullptr;
      }
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    if (const occtl_status_t aStatus = ValidateReadOptions(theOpts, "occtl_io_iges_read"))
    {
      return aStatus;
    }

    return ReadPathToGraph(thePath, theOutGraph, theOutRoot, theOpts, "occtl_io_iges_read");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_iges_write(const occtl_graph_t* const                 theGraph,
                      const occtl_node_id_t                      theRoot,
                      const char* const                          thePath,
                      const occtl_io_iges_write_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || thePath == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "path is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = ValidateWriteOptions(theOpts, "occtl_io_iges_write"))
    {
      return aStatus;
    }
    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          ResolveRootShape(theGraph, theRoot, "occtl_io_iges_write", aShape))
    {
      return aStatus;
    }

    return WriteShapeToPath(aShape, thePath, theOpts, "occtl_io_iges_write");
  });
}

} // extern "C"
