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
#include <RWMesh_CoordinateSystem.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopoDS_Shape.hxx>
#include <DEVRML_ConfigurationNode.hxx>
#include <DEVRML_Provider.hxx>
#include <Precision.hxx>

#include <occtl/occtl_io_vrml.h>

#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <cmath>
#include <cstring>
#include <memory>
#include <sstream>

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

RWMesh_CoordinateSystem ToOcctCoordinateSystem(const occtl_io_vrml_coordinate_system_t theSystem,
                                               occtl_status_t&                         theStatus)
{
  theStatus = OCCTL_OK;
  switch (theSystem)
  {
    case OCCTL_IO_VRML_COORDINATE_SYSTEM_Y_UP:
      return RWMesh_CoordinateSystem_Yup;
    case OCCTL_IO_VRML_COORDINATE_SYSTEM_Z_UP:
      return RWMesh_CoordinateSystem_Zup;
    case OCCTL_IO_VRML_COORDINATE_SYSTEM_GLTF:
      return RWMesh_CoordinateSystem_glTF;
    default:
      theStatus = OCCTL_OUT_OF_RANGE;
      return RWMesh_CoordinateSystem_Zup;
  }
}

occtl_status_t ApplyReadOptions(DEVRML_ConfigurationNode::Vrml_InternalSection& theParams,
                                const occtl_io_vrml_read_options_t* const       theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }

  occtl_status_t aStatus = OCCTL_OK;
  theParams.ReadFileUnit = theOpts->file_length_unit_m;
  theParams.ReadSystemCoordinateSys =
    ToOcctCoordinateSystem(theOpts->system_coordinate_system, aStatus);
  if (aStatus != OCCTL_OK)
  {
    return aStatus;
  }
  theParams.ReadFileCoordinateSys =
    ToOcctCoordinateSystem(theOpts->file_coordinate_system, aStatus);
  if (aStatus != OCCTL_OK)
  {
    return aStatus;
  }
  theParams.ReadFillIncomplete = theOpts->fill_incomplete != 0;
  return OCCTL_OK;
}

occtl_status_t ApplyWriteOptions(DEVRML_ConfigurationNode::Vrml_InternalSection& theParams,
                                 const occtl_io_vrml_write_options_t* const      theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }

  switch (theOpts->writer_version)
  {
    case OCCTL_IO_VRML_WRITER_VERSION_1:
      theParams.WriterVersion = DEVRML_ConfigurationNode::WriteMode_WriterVersion_1;
      break;
    case OCCTL_IO_VRML_WRITER_VERSION_2:
      theParams.WriterVersion = DEVRML_ConfigurationNode::WriteMode_WriterVersion_2;
      break;
    default:
      return OCCTL_OUT_OF_RANGE;
  }

  switch (theOpts->representation)
  {
    case OCCTL_IO_VRML_REPRESENTATION_SHADED:
      theParams.WriteRepresentationType =
        DEVRML_ConfigurationNode::WriteMode_RepresentationType_Shaded;
      break;
    case OCCTL_IO_VRML_REPRESENTATION_WIREFRAME:
      theParams.WriteRepresentationType =
        DEVRML_ConfigurationNode::WriteMode_RepresentationType_Wireframe;
      break;
    case OCCTL_IO_VRML_REPRESENTATION_BOTH:
      theParams.WriteRepresentationType =
        DEVRML_ConfigurationNode::WriteMode_RepresentationType_Both;
      break;
    default:
      return OCCTL_OUT_OF_RANGE;
  }

  return OCCTL_OK;
}

occtl_status_t ValidateReadOptions(const occtl_io_vrml_read_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_VRML_READ_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_io_vrml_read_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "occtl_io_vrml_read_options_t: p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsFiniteValue(theOpts->file_length_unit_m) || theOpts->file_length_unit_m <= 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_io_vrml_read_options_t: file_length_unit_m must be finite and positive");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOpts->fill_incomplete))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_io_vrml_read_options_t: fill_incomplete must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

occtl_status_t ValidateWriteOptions(const occtl_io_vrml_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_VRML_WRITE_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_io_vrml_write_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "occtl_io_vrml_write_options_t: p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
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

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_vrml_read_options_init(occtl_io_vrml_read_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_vrml_read_options_t aInit = OCCTL_IO_VRML_READ_OPTIONS_INIT;
  *theOpts                                 = aInit;
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_vrml_write_options_init(occtl_io_vrml_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_vrml_write_options_t aInit = OCCTL_IO_VRML_WRITE_OPTIONS_INIT;
  *theOpts                                  = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_vrml_read(const char* const                         thePath,
                     occtl_graph_t** const                     theOutGraph,
                     occtl_node_id_t* const                    theOutRoot,
                     const occtl_io_vrml_read_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_io_vrml_read: NULL argument");
      if (theOutGraph != nullptr)
      {
        *theOutGraph = nullptr;
      }
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    if (const occtl_status_t aStatus = ValidateReadOptions(theOpts))
    {
      return aStatus;
    }

    Handle(DEVRML_ConfigurationNode) aNode = new DEVRML_ConfigurationNode();
    const occtl_status_t aOptStatus        = ApplyReadOptions(aNode->InternalParameters, theOpts);
    if (aOptStatus != OCCTL_OK)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_io_vrml_read_options_t: unsupported coordinate-system value");
      return aOptStatus;
    }

    DEVRML_Provider aProvider(aNode);
    TopoDS_Shape    aShape;
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Read(TCollection_AsciiString(thePath), aShape))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_IO_ERROR,
                                               "occtl_io_vrml_read: DEVRML_Provider::Read failed");
        return OCCTL_IO_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        "occtl_io_vrml_read: DEVRML_Provider::Read threw exception");
      return OCCTL_IO_ERROR;
    }
    return AddShapeToFreshGraph(aShape, theOutGraph, theOutRoot, "occtl_io_vrml_read");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_vrml_write(const occtl_graph_t* const                 theGraph,
                      const occtl_node_id_t                      theRoot,
                      const char* const                          thePath,
                      const occtl_io_vrml_write_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_io_vrml_write: path is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = ValidateWriteOptions(theOpts))
    {
      return aStatus;
    }
    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          ResolveRootShape(theGraph, theRoot, "occtl_io_vrml_write", aShape))
    {
      return aStatus;
    }

    Handle(DEVRML_ConfigurationNode) aNode = new DEVRML_ConfigurationNode();
    const occtl_status_t aOptStatus        = ApplyWriteOptions(aNode->InternalParameters, theOpts);
    if (aOptStatus != OCCTL_OK)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_io_vrml_write_options_t: unsupported writer_version or representation");
      return aOptStatus;
    }

    DEVRML_Provider aProvider(aNode);
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Write(TCollection_AsciiString(thePath), aShape))
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_IO_ERROR,
          "occtl_io_vrml_write: DEVRML_Provider::Write failed");
        return OCCTL_IO_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        "occtl_io_vrml_write: DEVRML_Provider::Write threw exception");
      return OCCTL_IO_ERROR;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_vrml_read_memory(const uint8_t* const                      theData,
                            const size_t                              theSize,
                            occtl_graph_t** const                     theOutGraph,
                            occtl_node_id_t* const                    theOutRoot,
                            const occtl_io_vrml_read_options_t* const theOpts)
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
        "occtl_io_vrml_read_memory: NULL argument or empty buffer");
      return OCCTL_INVALID_ARGUMENT;
    }

    if (const occtl_status_t aStatus = ValidateReadOptions(theOpts))
    {
      return aStatus;
    }

    Handle(DEVRML_ConfigurationNode) aNode = new DEVRML_ConfigurationNode();
    const occtl_status_t aOptStatus        = ApplyReadOptions(aNode->InternalParameters, theOpts);
    if (aOptStatus != OCCTL_OK)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_io_vrml_read_options_t: unsupported coordinate-system value");
      return aOptStatus;
    }

    std::basic_stringbuf<char> aBuffer(std::ios::in | std::ios::out | std::ios::binary);
    aBuffer.sputn(reinterpret_cast<const char*>(theData), static_cast<std::streamsize>(theSize));
    std::istream                aStream(&aBuffer);
    DE_Provider::ReadStreamList aStreams;
    aStreams.Append(DE_Provider::ReadStreamNode("memory.wrl", aStream));

    DEVRML_Provider aProvider(aNode);
    TopoDS_Shape    aShape;
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Read(aStreams, aShape))
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_FORMAT_ERROR,
          "occtl_io_vrml_read_memory: DEVRML_Provider::Read stream failed");
        return OCCTL_FORMAT_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_FORMAT_ERROR,
        "occtl_io_vrml_read_memory: DEVRML_Provider::Read stream threw exception");
      return OCCTL_FORMAT_ERROR;
    }
    return AddShapeToFreshGraph(aShape, theOutGraph, theOutRoot, "occtl_io_vrml_read_memory");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_vrml_write_memory(const occtl_graph_t* const                 theGraph,
                             const occtl_node_id_t                      theRoot,
                             const occtl_io_vrml_write_options_t* const theOpts,
                             uint8_t* const                             theOutData,
                             const size_t                               theCapacity,
                             size_t* const                              theOutSize)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutSize == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_io_vrml_write_memory: out_size is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutSize = 0;
    if (const occtl_status_t aStatus = ValidateWriteOptions(theOpts))
    {
      return aStatus;
    }

    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          ResolveRootShape(theGraph, theRoot, "occtl_io_vrml_write_memory", aShape))
    {
      return aStatus;
    }

    Handle(DEVRML_ConfigurationNode) aNode = new DEVRML_ConfigurationNode();
    const occtl_status_t aOptStatus        = ApplyWriteOptions(aNode->InternalParameters, theOpts);
    if (aOptStatus != OCCTL_OK)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_io_vrml_write_options_t: unsupported writer_version or representation");
      return aOptStatus;
    }

    std::ostringstream           aStream(std::ios::out | std::ios::binary);
    DE_Provider::WriteStreamList aStreams;
    aStreams.Append(DE_Provider::WriteStreamNode("memory.wrl", aStream));

    DEVRML_Provider aProvider(aNode);
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Write(aStreams, aShape))
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_IO_ERROR,
          "occtl_io_vrml_write_memory: DEVRML_Provider::Write stream failed");
        return OCCTL_IO_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        "occtl_io_vrml_write_memory: DEVRML_Provider::Write stream threw exception");
      return OCCTL_IO_ERROR;
    }

    const auto aPayload = aStream.str();
    *theOutSize         = aPayload.size();
    if (theOutData == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCapacity < aPayload.size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "occtl_io_vrml_write_memory: output buffer too small");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    if (!aPayload.empty())
    {
      std::memcpy(theOutData, aPayload.data(), aPayload.size());
    }
    return OCCTL_OK;
  });
}

} // extern "C"
