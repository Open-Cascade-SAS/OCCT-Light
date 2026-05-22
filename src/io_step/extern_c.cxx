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
#include <DESTEP_ConfigurationNode.hxx>
#include <DESTEP_Provider.hxx>

#include <occtl/occtl_io_step.h>

#include <cstring>
#include <memory>
#include <sstream>

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

occtl_status_t ValidateReadOptions(const occtl_io_step_read_options_t* const theOpts,
                                   const char* const                         theContext)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_STEP_READ_OPTIONS_VERSION_1)
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
  if (!IsZeroOrOne(theOpts->read_color) || !IsZeroOrOne(theOpts->read_name)
      || !IsZeroOrOne(theOpts->read_layer))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(
        TCollection_AsciiString(theContext)
        + ": read options flags read_color/read_name/read_layer must be 0 or 1"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

occtl_status_t ValidateWriteOptions(const occtl_io_step_write_options_t* const theOpts,
                                    const char* const                          theContext)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_STEP_WRITE_OPTIONS_VERSION_1)
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
  if (theOpts->unit != OCCTL_IO_STEP_UNIT_MM && theOpts->unit != OCCTL_IO_STEP_UNIT_M
      && theOpts->unit != OCCTL_IO_STEP_UNIT_INCH)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_OUT_OF_RANGE,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": unsupported write options unit"));
    return OCCTL_OUT_OF_RANGE;
  }
  if (theOpts->schema != OCCTL_IO_STEP_SCHEMA_AP203 && theOpts->schema != OCCTL_IO_STEP_SCHEMA_AP214
      && theOpts->schema != OCCTL_IO_STEP_SCHEMA_AP242)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_OUT_OF_RANGE,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": unsupported write options schema"));
    return OCCTL_OUT_OF_RANGE;
  }
  if (!IsZeroOrOne(theOpts->write_surface_curves) || !IsZeroOrOne(theOpts->write_tessellated))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(
        TCollection_AsciiString(theContext)
        + ": write options flags write_surface_curves/write_tessellated must be 0 or 1"));
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

Handle(DESTEP_ConfigurationNode) MakeReadNode(const occtl_io_step_read_options_t* const theOpts)
{
  Handle(DESTEP_ConfigurationNode) aNode      = new DESTEP_ConfigurationNode();
  const bool                       aReadColor = theOpts == nullptr || theOpts->read_color != 0;
  const bool                       aReadName  = theOpts == nullptr || theOpts->read_name != 0;
  const bool                       aReadLayer = theOpts == nullptr || theOpts->read_layer != 0;
  aNode->InternalParameters.ReadColor         = aReadColor;
  aNode->InternalParameters.ReadName          = aReadName;
  aNode->InternalParameters.ReadLayer         = aReadLayer;
  return aNode;
}

Handle(DESTEP_ConfigurationNode) MakeWriteNode(const occtl_io_step_write_options_t* const theOpts)
{
  Handle(DESTEP_ConfigurationNode) aNode = new DESTEP_ConfigurationNode();
  if (theOpts == nullptr)
  {
    return aNode;
  }

  aNode->InternalParameters.WriteUnit = static_cast<UnitsMethods_LengthUnit>(theOpts->unit);
  aNode->InternalParameters.WriteSurfaceCurMode = theOpts->write_surface_curves != 0;
  if (theOpts->write_tessellated)
  {
    aNode->InternalParameters.WriteTessellated = DESTEP_Parameters::RWMode_Tessellated_On;
  }
  else
  {
    aNode->InternalParameters.WriteTessellated = DESTEP_Parameters::RWMode_Tessellated_Off;
  }
  switch (theOpts->schema)
  {
    case OCCTL_IO_STEP_SCHEMA_AP203:
      aNode->InternalParameters.WriteSchema = DESTEP_Parameters::WriteMode_StepSchema_AP203;
      break;
    case OCCTL_IO_STEP_SCHEMA_AP214:
      aNode->InternalParameters.WriteSchema = DESTEP_Parameters::WriteMode_StepSchema_AP214IS;
      break;
    default:
      aNode->InternalParameters.WriteSchema = DESTEP_Parameters::WriteMode_StepSchema_AP242DIS;
      break;
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
                               const occtl_io_step_read_options_t* const theOpts,
                               const char* const                         theContext)
{
  DESTEP_Provider aProvider(MakeReadNode(theOpts));
  TopoDS_Shape    aShape;
  try
  {
    OCC_CATCH_SIGNALS;
    if (!aProvider.Read(TCollection_AsciiString(thePath), aShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DESTEP_Provider::Read failed"));
      return OCCTL_IO_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_IO_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DESTEP_Provider::Read threw exception"));
    return OCCTL_IO_ERROR;
  }
  return AddShapeToFreshGraph(aShape, theOutGraph, theOutRoot, theContext);
}

occtl_status_t ReadStreamToGraph(const uint8_t* const                      theData,
                                 const size_t                              theSize,
                                 occtl_graph_t** const                     theOutGraph,
                                 occtl_node_id_t* const                    theOutRoot,
                                 const occtl_io_step_read_options_t* const theOpts,
                                 const char* const                         theContext)
{
  std::basic_stringbuf<char> aBuffer(std::ios::in | std::ios::out | std::ios::binary);
  aBuffer.sputn(reinterpret_cast<const char*>(theData), static_cast<std::streamsize>(theSize));
  std::istream                aStream(&aBuffer);
  DE_Provider::ReadStreamList aStreams;
  aStreams.Append(DE_Provider::ReadStreamNode("memory.step", aStream));

  DESTEP_Provider aProvider(MakeReadNode(theOpts));
  TopoDS_Shape    aShape;
  try
  {
    OCC_CATCH_SIGNALS;
    if (!aProvider.Read(aStreams, aShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_FORMAT_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DESTEP_Provider::Read stream failed"));
      return OCCTL_FORMAT_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_FORMAT_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DESTEP_Provider::Read stream threw exception"));
    return OCCTL_FORMAT_ERROR;
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
                                const occtl_io_step_write_options_t* const theOpts,
                                const char* const                          theContext)
{
  DESTEP_Provider aProvider(MakeWriteNode(theOpts));
  try
  {
    OCC_CATCH_SIGNALS;
    if (!aProvider.Write(TCollection_AsciiString(thePath), theShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DESTEP_Provider::Write failed"));
      return OCCTL_IO_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_IO_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DESTEP_Provider::Write threw exception"));
    return OCCTL_IO_ERROR;
  }
  return OCCTL_OK;
}

occtl_status_t WriteShapeToStream(const TopoDS_Shape&                        theShape,
                                  const occtl_io_step_write_options_t* const theOpts,
                                  const char* const                          theContext,
                                  std::ostringstream&                        theOutStream)
{
  DE_Provider::WriteStreamList aStreams;
  aStreams.Append(DE_Provider::WriteStreamNode("memory.step", theOutStream));

  DESTEP_Provider aProvider(MakeWriteNode(theOpts));
  try
  {
    OCC_CATCH_SIGNALS;
    if (!aProvider.Write(aStreams, theShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DESTEP_Provider::Write stream failed"));
      return OCCTL_IO_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_IO_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DESTEP_Provider::Write stream threw exception"));
    return OCCTL_IO_ERROR;
  }
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_step_read_options_init(occtl_io_step_read_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_step_read_options_t aInit = OCCTL_IO_STEP_READ_OPTIONS_INIT;
  *theOpts                                 = aInit;
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_step_write_options_init(occtl_io_step_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_step_write_options_t aInit = OCCTL_IO_STEP_WRITE_OPTIONS_INIT;
  *theOpts                                  = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_step_read(const char* const                         thePath,
                     occtl_graph_t** const                     theOutGraph,
                     occtl_node_id_t* const                    theOutRoot,
                     const occtl_io_step_read_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_io_step_read: NULL argument");
      if (theOutGraph != nullptr)
      {
        *theOutGraph = nullptr;
      }
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    if (const occtl_status_t aStatus = ValidateReadOptions(theOpts, "occtl_io_step_read"))
    {
      return aStatus;
    }

    return ReadPathToGraph(thePath, theOutGraph, theOutRoot, theOpts, "occtl_io_step_read");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_step_read_memory(const uint8_t* const                      theData,
                            const size_t                              theSize,
                            occtl_graph_t** const                     theOutGraph,
                            occtl_node_id_t* const                    theOutRoot,
                            const occtl_io_step_read_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutGraph != nullptr)
    {
      *theOutGraph = nullptr;
    }
    if (theOutRoot != nullptr)
    {
      *theOutRoot = OCCTL_NODE_ID_INVALID;
    }
    if (theData == nullptr || theSize == 0 || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_INVALID_ARGUMENT,
        "occtl_io_step_read_memory: NULL argument or empty buffer");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = ValidateReadOptions(theOpts, "occtl_io_step_read_memory"))
    {
      return aStatus;
    }

    return ReadStreamToGraph(theData,
                             theSize,
                             theOutGraph,
                             theOutRoot,
                             theOpts,
                             "occtl_io_step_read_memory");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_step_write(const occtl_graph_t* const                 theGraph,
                      const occtl_node_id_t                      theRoot,
                      const char* const                          thePath,
                      const occtl_io_step_write_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || thePath == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "path is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = ValidateWriteOptions(theOpts, "occtl_io_step_write"))
    {
      return aStatus;
    }
    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          ResolveRootShape(theGraph, theRoot, "occtl_io_step_write", aShape))
    {
      return aStatus;
    }

    return WriteShapeToPath(aShape, thePath, theOpts, "occtl_io_step_write");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_step_write_memory(const occtl_graph_t* const                 theGraph,
                             const occtl_node_id_t                      theRoot,
                             const occtl_io_step_write_options_t* const theOpts,
                             uint8_t* const                             theOutData,
                             const size_t                               theCapacity,
                             size_t* const                              theOutSize)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutSize == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_io_step_write_memory: out_size is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutSize = 0;
    if (const occtl_status_t aStatus = ValidateWriteOptions(theOpts, "occtl_io_step_write_memory"))
    {
      return aStatus;
    }
    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          ResolveRootShape(theGraph, theRoot, "occtl_io_step_write_memory", aShape))
    {
      return aStatus;
    }

    std::ostringstream aPayload(std::ios::out | std::ios::binary);
    if (const occtl_status_t aStatus =
          WriteShapeToStream(aShape, theOpts, "occtl_io_step_write_memory", aPayload))
    {
      return aStatus;
    }

    const auto aBytes = aPayload.str();
    *theOutSize       = aBytes.size();
    if (theOutData == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aBytes.size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "occtl_io_step_write_memory: output buffer too small");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    if (!aBytes.empty())
    {
      std::memcpy(theOutData, aBytes.data(), aBytes.size());
    }
    return OCCTL_OK;
  });
}

} // extern "C"
