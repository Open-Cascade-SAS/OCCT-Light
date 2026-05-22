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
#include <Poly_Triangulation.hxx>
#include <RWMesh_CoordinateSystem.hxx>
#include <RWObj.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopoDS_Shape.hxx>
#include <DEOBJ_ConfigurationNode.hxx>
#include <DEOBJ_Provider.hxx>
#include <Precision.hxx>

#include <occtl/occtl_io_obj.h>

#include <cmath>
#include <memory>

#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"

#include <fstream>

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

RWMesh_CoordinateSystem ToOcctCoordinateSystem(const occtl_io_obj_coordinate_system_t theSystem,
                                               occtl_status_t&                        theStatus)
{
  theStatus = OCCTL_OK;
  switch (theSystem)
  {
    case OCCTL_IO_OBJ_COORDINATE_SYSTEM_Y_UP:
      return RWMesh_CoordinateSystem_Yup;
    case OCCTL_IO_OBJ_COORDINATE_SYSTEM_Z_UP:
      return RWMesh_CoordinateSystem_Zup;
    case OCCTL_IO_OBJ_COORDINATE_SYSTEM_GLTF:
      return RWMesh_CoordinateSystem_glTF;
    default:
      theStatus = OCCTL_OUT_OF_RANGE;
      return RWMesh_CoordinateSystem_Zup;
  }
}

occtl_status_t ApplyReadOptions(DEOBJ_ConfigurationNode::RWObj_InternalSection& theParams,
                                const occtl_io_obj_read_options_t* const        theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }

  occtl_status_t aStatus   = OCCTL_OK;
  theParams.FileLengthUnit = theOpts->file_length_unit_m;
  theParams.SystemCS       = ToOcctCoordinateSystem(theOpts->system_coordinate_system, aStatus);
  if (aStatus != OCCTL_OK)
  {
    return aStatus;
  }
  theParams.FileCS = ToOcctCoordinateSystem(theOpts->file_coordinate_system, aStatus);
  if (aStatus != OCCTL_OK)
  {
    return aStatus;
  }
  theParams.ReadSinglePrecision = theOpts->single_precision != 0;
  theParams.ReadCreateShapes    = theOpts->create_shapes != 0;
  theParams.ReadFillIncomplete  = theOpts->fill_incomplete != 0;
  theParams.ReadMemoryLimitMiB  = theOpts->memory_limit_mib;
  theParams.ReadRootPrefix      = theOpts->root_prefix != nullptr
                                    ? TCollection_AsciiString(theOpts->root_prefix)
                                    : TCollection_AsciiString();
  return OCCTL_OK;
}

occtl_status_t ApplyWriteOptions(DEOBJ_ConfigurationNode::RWObj_InternalSection& theParams,
                                 const occtl_io_obj_write_options_t* const       theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }

  occtl_status_t aStatus = OCCTL_OK;
  theParams.SystemCS     = ToOcctCoordinateSystem(theOpts->system_coordinate_system, aStatus);
  if (aStatus != OCCTL_OK)
  {
    return aStatus;
  }
  theParams.FileCS = ToOcctCoordinateSystem(theOpts->file_coordinate_system, aStatus);
  if (aStatus != OCCTL_OK)
  {
    return aStatus;
  }
  theParams.WriteComment = theOpts->comment != nullptr ? TCollection_AsciiString(theOpts->comment)
                                                       : TCollection_AsciiString();
  theParams.WriteAuthor  = theOpts->author != nullptr ? TCollection_AsciiString(theOpts->author)
                                                      : TCollection_AsciiString();
  return OCCTL_OK;
}

occtl_status_t ValidateReadOptions(const occtl_io_obj_read_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_OBJ_READ_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_io_obj_read_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "occtl_io_obj_read_options_t: p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsFiniteValue(theOpts->file_length_unit_m) || theOpts->file_length_unit_m <= 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_io_obj_read_options_t: file_length_unit_m must be finite and positive");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOpts->single_precision) || !IsZeroOrOne(theOpts->create_shapes)
      || !IsZeroOrOne(theOpts->fill_incomplete))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_io_obj_read_options_t: flags single_precision/create_shapes/fill_incomplete must be 0 "
      "or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->memory_limit_mib < -1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_io_obj_read_options_t: memory_limit_mib must be -1 or non-negative");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

occtl_status_t ValidateWriteOptions(const occtl_io_obj_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_OBJ_WRITE_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_io_obj_write_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "occtl_io_obj_write_options_t: p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_io_obj_read_options_init(occtl_io_obj_read_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_obj_read_options_t aInit = OCCTL_IO_OBJ_READ_OPTIONS_INIT;
  *theOpts                                = aInit;
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_obj_write_options_init(occtl_io_obj_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_obj_write_options_t aInit = OCCTL_IO_OBJ_WRITE_OPTIONS_INIT;
  *theOpts                                 = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_obj_read(const char* const                        thePath,
                    occtl_graph_t** const                    theOutGraph,
                    occtl_node_id_t* const                   theOutRoot,
                    const occtl_io_obj_read_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_io_obj_read: NULL argument");
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

    Handle(DEOBJ_ConfigurationNode) aNode = new DEOBJ_ConfigurationNode();
    const occtl_status_t aOptStatus       = ApplyReadOptions(aNode->InternalParameters, theOpts);
    if (aOptStatus != OCCTL_OK)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_io_obj_read_options_t: unsupported coordinate-system value");
      return aOptStatus;
    }

    try
    {
      OCC_CATCH_SIGNALS;
      std::ifstream aInput(thePath);
      if (!aInput.good())
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_IO_ERROR,
                                               "occtl_io_obj_read: file is not readable");
        return OCCTL_IO_ERROR;
      }
      const Handle(Poly_Triangulation) aProbe = RWObj::ReadFile(thePath);
      if (aProbe.IsNull())
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_FORMAT_ERROR,
          "occtl_io_obj_read: OBJ triangulation validation failed");
        return OCCTL_FORMAT_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_FORMAT_ERROR,
        "occtl_io_obj_read: OBJ triangulation validation threw exception");
      return OCCTL_FORMAT_ERROR;
    }

    DEOBJ_Provider aProvider(aNode);
    TopoDS_Shape   aShape;
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Read(TCollection_AsciiString(thePath), aShape))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_IO_ERROR,
                                               "occtl_io_obj_read: DEOBJ_Provider::Read failed");
        return OCCTL_IO_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        "occtl_io_obj_read: DEOBJ_Provider::Read threw exception");
      return OCCTL_IO_ERROR;
    }
    if (aShape.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_FORMAT_ERROR,
                                             "occtl_io_obj_read: Provider returned null shape");
      return OCCTL_FORMAT_ERROR;
    }

    std::unique_ptr<occtl_graph_t> aGraph = std::make_unique<occtl_graph_t>();
    BRepGraph::ShapesView::Options anOpts;
    anOpts.CreateAutoProduct                 = false;
    anOpts.TrackAddedNodes                   = false;
    const BRepGraph::ShapesView::Result aRes = aGraph->graph.Shapes().Add(aShape, anOpts);
    if (!aRes.Ok)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_TOPOLOGY_INVALID,
        "occtl_io_obj_read: shape did not ingest into a graph");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutGraph = aGraph.release();
    *theOutRoot  = OcctL::Topo::PackNodeId(aRes.TopologyRoot);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_obj_write(const occtl_graph_t* const                theGraph,
                     const occtl_node_id_t                     theRoot,
                     const char* const                         thePath,
                     const occtl_io_obj_write_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || thePath == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "path is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = ValidateWriteOptions(theOpts))
    {
      return aStatus;
    }
    const BRepGraph_NodeId aRootId = OcctL::Topo::UnpackNodeId(theRoot);
    if (!aRootId.IsValid())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "occtl_io_obj_write: root NodeId is invalid");
      return OCCTL_NOT_FOUND;
    }
    const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(aRootId);
    if (aShape.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "occtl_io_obj_write: root resolved to a null shape (removed?)");
      return OCCTL_NOT_FOUND;
    }

    Handle(DEOBJ_ConfigurationNode) aNode = new DEOBJ_ConfigurationNode();
    const occtl_status_t aOptStatus       = ApplyWriteOptions(aNode->InternalParameters, theOpts);
    if (aOptStatus != OCCTL_OK)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_OUT_OF_RANGE,
        "occtl_io_obj_write_options_t: unsupported coordinate-system value");
      return aOptStatus;
    }

    DEOBJ_Provider aProvider(aNode);
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Write(TCollection_AsciiString(thePath), aShape))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_IO_ERROR,
                                               "occtl_io_obj_write: DEOBJ_Provider::Write failed");
        return OCCTL_IO_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        "occtl_io_obj_write: DEOBJ_Provider::Write threw exception");
      return OCCTL_IO_ERROR;
    }
    return OCCTL_OK;
  });
}

} // extern "C"
