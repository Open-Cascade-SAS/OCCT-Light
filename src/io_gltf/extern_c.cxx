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
#include <RWGltf_WriterTrsfFormat.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Shape.hxx>
#include <DEGLTF_ConfigurationNode.hxx>
#include <DEGLTF_Provider.hxx>

#include <occtl/occtl_io_gltf.h>

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

RWGltf_WriterTrsfFormat ToOcctTransformFormat(const occtl_io_gltf_transform_format_t theFormat,
                                              occtl_status_t&                        theStatus)
{
  theStatus = OCCTL_OK;
  switch (theFormat)
  {
    case OCCTL_IO_GLTF_TRANSFORM_COMPACT:
      return RWGltf_WriterTrsfFormat_Compact;
    case OCCTL_IO_GLTF_TRANSFORM_MAT4:
      return RWGltf_WriterTrsfFormat_Mat4;
    case OCCTL_IO_GLTF_TRANSFORM_TRS:
      return RWGltf_WriterTrsfFormat_TRS;
    default:
      theStatus = OCCTL_OUT_OF_RANGE;
      return RWGltf_WriterTrsfFormat_Compact;
  }
}

occtl_status_t ValidateReadOptions(const occtl_io_gltf_read_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_GLTF_READ_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_io_gltf_read_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "occtl_io_gltf_read_options_t: p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOpts->load_all_scenes) || !IsZeroOrOne(theOpts->skip_empty_nodes)
      || !IsZeroOrOne(theOpts->use_mesh_name_as_fallback) || !IsZeroOrOne(theOpts->apply_scale)
      || !IsZeroOrOne(theOpts->parallel) || !IsZeroOrOne(theOpts->single_precision)
      || !IsZeroOrOne(theOpts->fill_incomplete))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_io_gltf_read_options_t: read flags must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->memory_limit_mib < -1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_io_gltf_read_options_t: memory_limit_mib must be -1 or non-negative");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

occtl_status_t ValidateWriteOptions(const occtl_io_gltf_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return OCCTL_OK;
  }
  if (theOpts->struct_version != OCCTL_IO_GLTF_WRITE_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_io_gltf_write_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theOpts->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "occtl_io_gltf_write_options_t: p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(theOpts->force_uv_export) || !IsZeroOrOne(theOpts->embed_textures_in_glb)
      || !IsZeroOrOne(theOpts->merge_faces) || !IsZeroOrOne(theOpts->split_indices_16)
      || !IsZeroOrOne(theOpts->parallel))
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "occtl_io_gltf_write_options_t: write flags must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theOpts->parallel != 0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "occtl_io_gltf_write_options_t: parallel must be 0");
    return OCCTL_INVALID_ARGUMENT;
  }
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_gltf_read_options_init(occtl_io_gltf_read_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_gltf_read_options_t aInit = OCCTL_IO_GLTF_READ_OPTIONS_INIT;
  *theOpts                                 = aInit;
}

//==================================================================================================

OCCTL_API void OCCTL_CALL
  occtl_io_gltf_write_options_init(occtl_io_gltf_write_options_t* const theOpts)
{
  if (theOpts == nullptr)
  {
    return;
  }
  const occtl_io_gltf_write_options_t aInit = OCCTL_IO_GLTF_WRITE_OPTIONS_INIT;
  *theOpts                                  = aInit;
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_gltf_read(const char* const                         thePath,
                     occtl_graph_t** const                     theOutGraph,
                     occtl_node_id_t* const                    theOutRoot,
                     const occtl_io_gltf_read_options_t* const theOpts)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_io_gltf_read: NULL argument");
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

    Handle(DEGLTF_ConfigurationNode) aNode = new DEGLTF_ConfigurationNode();
    if (theOpts != nullptr)
    {
      aNode->InternalParameters.ReadLoadAllScenes         = theOpts->load_all_scenes != 0;
      aNode->InternalParameters.ReadSkipEmptyNodes        = theOpts->skip_empty_nodes != 0;
      aNode->InternalParameters.ReadUseMeshNameAsFallback = theOpts->use_mesh_name_as_fallback != 0;
      aNode->InternalParameters.ReadApplyScale            = theOpts->apply_scale != 0;
      aNode->InternalParameters.ReadParallel              = theOpts->parallel != 0;
      aNode->InternalParameters.ReadSinglePrecision       = theOpts->single_precision != 0;
      aNode->InternalParameters.ReadFillIncomplete        = theOpts->fill_incomplete != 0;
      aNode->InternalParameters.ReadMemoryLimitMiB        = theOpts->memory_limit_mib;
    }

    DEGLTF_Provider aProvider(aNode);
    TopoDS_Shape    aShape;
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Read(TCollection_AsciiString(thePath), aShape))
      {
        OcctL::Core::ErrorState::Current().Set(OCCTL_IO_ERROR,
                                               "occtl_io_gltf_read: DEGLTF_Provider::Read failed");
        return OCCTL_IO_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        "occtl_io_gltf_read: DEGLTF_Provider::Read threw exception");
      return OCCTL_IO_ERROR;
    }
    if (aShape.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_FORMAT_ERROR,
                                             "occtl_io_gltf_read: Provider returned null shape");
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
        "occtl_io_gltf_read: shape did not ingest into a graph");
      return OCCTL_TOPOLOGY_INVALID;
    }

    *theOutGraph = aGraph.release();
    *theOutRoot  = OcctL::Topo::PackNodeId(aRes.TopologyRoot);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_io_gltf_write(const occtl_graph_t* const                 theGraph,
                      const occtl_node_id_t                      theRoot,
                      const char* const                          thePath,
                      const occtl_io_gltf_write_options_t* const theOpts)
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
                                             "occtl_io_gltf_write: root NodeId is invalid");
      return OCCTL_NOT_FOUND;
    }
    const TopoDS_Shape aShape = theGraph->graph.Shapes().Shape(aRootId);
    if (aShape.IsNull())
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_NOT_FOUND,
        "occtl_io_gltf_write: root resolved to a null shape (removed?)");
      return OCCTL_NOT_FOUND;
    }

    Handle(DEGLTF_ConfigurationNode) aNode = new DEGLTF_ConfigurationNode();
    if (theOpts != nullptr)
    {
      occtl_status_t aFormatStatus = OCCTL_OK;
      aNode->InternalParameters.WriteTrsfFormat =
        ToOcctTransformFormat(theOpts->transform_format, aFormatStatus);
      if (aFormatStatus != OCCTL_OK)
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_OUT_OF_RANGE,
          "occtl_io_gltf_write_options_t: unsupported transform_format");
        return OCCTL_OUT_OF_RANGE;
      }
      aNode->InternalParameters.WriteForcedUVExport     = theOpts->force_uv_export != 0;
      aNode->InternalParameters.WriteEmbedTexturesInGlb = theOpts->embed_textures_in_glb != 0;
      aNode->InternalParameters.WriteMergeFaces         = theOpts->merge_faces != 0;
      aNode->InternalParameters.WriteSplitIndices16     = theOpts->split_indices_16 != 0;
    }

    DEGLTF_Provider aProvider(aNode);
    try
    {
      OCC_CATCH_SIGNALS;
      if (!aProvider.Write(TCollection_AsciiString(thePath), aShape))
      {
        OcctL::Core::ErrorState::Current().Set(
          OCCTL_IO_ERROR,
          "occtl_io_gltf_write: DEGLTF_Provider::Write failed");
        return OCCTL_IO_ERROR;
      }
    }
    catch (const Standard_Failure&)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        "occtl_io_gltf_write: DEGLTF_Provider::Write threw exception");
      return OCCTL_IO_ERROR;
    }
    return OCCTL_OK;
  });
}

} // extern "C"
