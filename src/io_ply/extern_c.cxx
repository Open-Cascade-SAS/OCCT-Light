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
#include <DEPLY_ConfigurationNode.hxx>
#include <DEPLY_Provider.hxx>

#include <occtl/occtl_io_ply.h>

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

RWMesh_CoordinateSystem ToOcctCoordinateSystem(const occtl_io_ply_coordinate_system_t theSystem,
                                               occtl_status_t&                        theStatus)
{
  theStatus = OCCTL_OK;
  switch (theSystem)
  {
    case OCCTL_IO_PLY_COORDINATE_SYSTEM_Y_UP:
      return RWMesh_CoordinateSystem_Yup;
    case OCCTL_IO_PLY_COORDINATE_SYSTEM_Z_UP:
      return RWMesh_CoordinateSystem_Zup;
    case OCCTL_IO_PLY_COORDINATE_SYSTEM_GLTF:
      return RWMesh_CoordinateSystem_glTF;
    default:
      theStatus = OCCTL_OUT_OF_RANGE;
      return RWMesh_CoordinateSystem_Zup;
  }
}

occtl_status_t ApplyWriteOptions(DEPLY_ConfigurationNode::RWPly_InternalSection& theParams,
                                 const occtl_io_ply_write_options_t* const       theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }

  if (theOpts->write_part_id != 0 && theOpts->write_face_id != 0)
  {
    return OCCTL_INVALID_ARGUMENT;
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
  theParams.WriteNormals   = theOpts->write_normals != 0;
  theParams.WriteColors    = theOpts->write_colors != 0;
  theParams.WriteTexCoords = theOpts->write_texcoords != 0;
  theParams.WritePartId    = theOpts->write_part_id != 0;
  theParams.WriteFaceId    = theOpts->write_face_id != 0;
  theParams.WriteComment   = theOpts->comment != nullptr ? TCollection_AsciiString(theOpts->comment)
                                                         : TCollection_AsciiString();
  theParams.WriteAuthor    = theOpts->author != nullptr ? TCollection_AsciiString(theOpts->author)
                                                        : TCollection_AsciiString();
  return OCCTL_OK;
}

occtl_status_t ValidateWriteOptions(const occtl_io_ply_write_options_t* const theOpts,
                                    const char* const                         theContext)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_PLY_WRITE_OPTIONS_VERSION_1)
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
  if (!IsZeroOrOne(theOpts->write_normals) || !IsZeroOrOne(theOpts->write_colors)
      || !IsZeroOrOne(theOpts->write_texcoords) || !IsZeroOrOne(theOpts->write_part_id)
      || !IsZeroOrOne(theOpts->write_face_id))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      static_cast<std::string_view>(
        TCollection_AsciiString(theContext)
        + ": write options flags "
          "write_normals/write_colors/write_texcoords/write_part_id/write_face_id must be 0 or 1"));
    return OCCTL_INVALID_ARGUMENT;
  }
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

occtl_status_t WriteShapeToPath(const TopoDS_Shape&                       theShape,
                                const char* const                         thePath,
                                const occtl_io_ply_write_options_t* const theOpts,
                                const char* const                         theContext)
{
  Handle(DEPLY_ConfigurationNode) aNode = new DEPLY_ConfigurationNode();
  const occtl_status_t aOptStatus       = ApplyWriteOptions(aNode->InternalParameters, theOpts);
  if (aOptStatus == OCCTL_INVALID_ARGUMENT)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_io_ply_write_options_t: write_part_id and write_face_id cannot both be enabled");
    return aOptStatus;
  }
  if (aOptStatus != OCCTL_OK)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_OUT_OF_RANGE,
      "occtl_io_ply_write_options_t: unsupported coordinate-system value");
    return aOptStatus;
  }

  DEPLY_Provider aProvider(aNode);
  try
  {
    OCC_CATCH_SIGNALS;
    if (!aProvider.Write(TCollection_AsciiString(thePath), theShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DEPLY_Provider::Write failed"));
      return OCCTL_IO_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_IO_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DEPLY_Provider::Write threw exception"));
    return OCCTL_IO_ERROR;
  }
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_ply_write_options_init(occtl_io_ply_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_ply_write_options_t aInit = OCCTL_IO_PLY_WRITE_OPTIONS_INIT;
  *theOpts                                 = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_ply_write(const occtl_graph_t* const                theGraph,
                     const occtl_node_id_t                     theRoot,
                     const char* const                         thePath,
                     const occtl_io_ply_write_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "path is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    if (const occtl_status_t aStatus = ValidateWriteOptions(theOpts, "occtl_io_ply_write"))
    {
      return aStatus;
    }
    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          ResolveRootShape(theGraph, theRoot, "occtl_io_ply_write", aShape))
    {
      return aStatus;
    }

    return WriteShapeToPath(aShape, thePath, theOpts, "occtl_io_ply_write");
  });
}

} // extern "C"
