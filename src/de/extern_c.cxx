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

#include <occtl/occtl_de.h>

#include <DE_ConfigurationNode.hxx>
#include <DE_Wrapper.hxx>

#ifdef OCCTL_DE_HAS_IO_BREP
  #include <DEBREP_ConfigurationNode.hxx>
#endif
#ifdef OCCTL_DE_HAS_IO_STEP
  #include <DESTEP_ConfigurationNode.hxx>
#endif
#ifdef OCCTL_DE_HAS_IO_IGES
  #include <DEIGES_ConfigurationNode.hxx>
#endif
#ifdef OCCTL_DE_HAS_IO_STL
  #include <DESTL_ConfigurationNode.hxx>
#endif
#ifdef OCCTL_DE_HAS_IO_OBJ
  #include <DEOBJ_ConfigurationNode.hxx>
#endif
#ifdef OCCTL_DE_HAS_IO_GLTF
  #include <DEGLTF_ConfigurationNode.hxx>
#endif
#ifdef OCCTL_DE_HAS_IO_VRML
  #include <DEVRML_ConfigurationNode.hxx>
#endif
#ifdef OCCTL_DE_HAS_IO_PLY
  #include <DEPLY_ConfigurationNode.hxx>
#endif

#include <BRepGraph.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopoDS_Shape.hxx>
#include <Message_ProgressRange.hxx>

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../topo/GraphHandle.hxx"
#include "../topo/TopoMath.hxx"

#include <NCollection_LinearVector.hxx>
#include <cctype>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>

namespace
{

static std::once_flag s_de_once;

struct FormatDescriptor
{
  const char*        id;
  const char*        label;
  const char* const* extensions;
  size_t             extensionCount;
  int32_t            canReadFile;
  int32_t            canWriteFile;
  int32_t            canReadMemory;
  int32_t            canWriteMemory;
};

#ifdef OCCTL_DE_HAS_IO_BREP
static constexpr const char* THE_BREP_EXTENSIONS[] = {".brep"};
#endif
#ifdef OCCTL_DE_HAS_IO_STEP
static constexpr const char* THE_STEP_EXTENSIONS[] = {".stp", ".step", ".stpz"};
#endif
#ifdef OCCTL_DE_HAS_IO_IGES
static constexpr const char* THE_IGES_EXTENSIONS[] = {".igs", ".iges"};
#endif
#ifdef OCCTL_DE_HAS_IO_STL
static constexpr const char* THE_STL_EXTENSIONS[] = {".stl"};
#endif
#ifdef OCCTL_DE_HAS_IO_OBJ
static constexpr const char* THE_OBJ_EXTENSIONS[] = {".obj"};
#endif
#ifdef OCCTL_DE_HAS_IO_GLTF
static constexpr const char* THE_GLTF_EXTENSIONS[] = {".gltf", ".glb"};
#endif
#ifdef OCCTL_DE_HAS_IO_VRML
static constexpr const char* THE_VRML_EXTENSIONS[] = {".vrml", ".wrl"};
#endif
#ifdef OCCTL_DE_HAS_IO_PLY
static constexpr const char* THE_PLY_EXTENSIONS[] = {".ply"};
#endif

const NCollection_LinearVector<FormatDescriptor>& SupportedFormatTable()
{
  static const NCollection_LinearVector<FormatDescriptor> s_formats = []() {
    NCollection_LinearVector<FormatDescriptor> aFormats;
#ifdef OCCTL_DE_HAS_IO_BREP
    aFormats.Append({"brep",
                     "Open CASCADE BRep",
                     THE_BREP_EXTENSIONS,
                     sizeof(THE_BREP_EXTENSIONS) / sizeof(THE_BREP_EXTENSIONS[0]),
                     1,
                     1,
                     0,
                     0});
#endif
#ifdef OCCTL_DE_HAS_IO_STEP
    aFormats.Append({"step",
                     "STEP",
                     THE_STEP_EXTENSIONS,
                     sizeof(THE_STEP_EXTENSIONS) / sizeof(THE_STEP_EXTENSIONS[0]),
                     1,
                     1,
                     1,
                     1});
#endif
#ifdef OCCTL_DE_HAS_IO_IGES
    aFormats.Append({"iges",
                     "IGES",
                     THE_IGES_EXTENSIONS,
                     sizeof(THE_IGES_EXTENSIONS) / sizeof(THE_IGES_EXTENSIONS[0]),
                     1,
                     1,
                     0,
                     0});
#endif
#ifdef OCCTL_DE_HAS_IO_STL
    aFormats.Append({"stl",
                     "STL",
                     THE_STL_EXTENSIONS,
                     sizeof(THE_STL_EXTENSIONS) / sizeof(THE_STL_EXTENSIONS[0]),
                     1,
                     1,
                     1,
                     1});
#endif
#ifdef OCCTL_DE_HAS_IO_OBJ
    aFormats.Append({"obj",
                     "Wavefront OBJ",
                     THE_OBJ_EXTENSIONS,
                     sizeof(THE_OBJ_EXTENSIONS) / sizeof(THE_OBJ_EXTENSIONS[0]),
                     1,
                     1,
                     0,
                     0});
#endif
#ifdef OCCTL_DE_HAS_IO_GLTF
    aFormats.Append({"gltf",
                     "glTF / GLB",
                     THE_GLTF_EXTENSIONS,
                     sizeof(THE_GLTF_EXTENSIONS) / sizeof(THE_GLTF_EXTENSIONS[0]),
                     1,
                     1,
                     0,
                     0});
#endif
#ifdef OCCTL_DE_HAS_IO_VRML
    aFormats.Append({"vrml",
                     "VRML",
                     THE_VRML_EXTENSIONS,
                     sizeof(THE_VRML_EXTENSIONS) / sizeof(THE_VRML_EXTENSIONS[0]),
                     1,
                     1,
                     1,
                     1});
#endif
#ifdef OCCTL_DE_HAS_IO_PLY
    aFormats.Append({"ply",
                     "PLY",
                     THE_PLY_EXTENSIONS,
                     sizeof(THE_PLY_EXTENSIONS) / sizeof(THE_PLY_EXTENSIONS[0]),
                     0,
                     1,
                     0,
                     0});
#endif
    return aFormats;
  }();
  return s_formats;
}

void EnsureDEInitialized()
{
  std::call_once(s_de_once, []() {
    Handle(DE_Wrapper) aWrapper = DE_Wrapper::GlobalWrapper();

#ifdef OCCTL_DE_HAS_IO_BREP
    aWrapper->Bind(new DEBREP_ConfigurationNode());
#endif
#ifdef OCCTL_DE_HAS_IO_STEP
    aWrapper->Bind(new DESTEP_ConfigurationNode());
#endif
#ifdef OCCTL_DE_HAS_IO_IGES
    aWrapper->Bind(new DEIGES_ConfigurationNode());
#endif
#ifdef OCCTL_DE_HAS_IO_STL
    aWrapper->Bind(new DESTL_ConfigurationNode());
#endif
#ifdef OCCTL_DE_HAS_IO_OBJ
    aWrapper->Bind(new DEOBJ_ConfigurationNode());
#endif
#ifdef OCCTL_DE_HAS_IO_GLTF
    aWrapper->Bind(new DEGLTF_ConfigurationNode());
#endif
#ifdef OCCTL_DE_HAS_IO_VRML
    aWrapper->Bind(new DEVRML_ConfigurationNode());
#endif
#ifdef OCCTL_DE_HAS_IO_PLY
    aWrapper->Bind(new DEPLY_ConfigurationNode());
#endif
  });
}

TCollection_AsciiString LowerCopy(const char* const theText)
{
  TCollection_AsciiString aLower =
    theText != nullptr ? TCollection_AsciiString(theText) : TCollection_AsciiString();
  aLower.LowerCase();
  return aLower;
}

const FormatDescriptor* FindFormatForId(const char* const theFormatId)
{
  if (theFormatId == nullptr)
  {
    return nullptr;
  }
  const TCollection_AsciiString aNeedle = LowerCopy(theFormatId);
  for (const FormatDescriptor& aFormat : SupportedFormatTable())
  {
    if (aNeedle.IsEqual(aFormat.id))
    {
      return &aFormat;
    }
  }
  return nullptr;
}

const FormatDescriptor* FindFormatForExtension(const char* const theExt)
{
  if (theExt == nullptr)
  {
    return nullptr;
  }
  const TCollection_AsciiString anExt = LowerCopy(theExt);
  for (const FormatDescriptor& aFormat : SupportedFormatTable())
  {
    for (size_t anIndex = 0; anIndex < aFormat.extensionCount; ++anIndex)
    {
      if (anExt.IsEqual(aFormat.extensions[anIndex]))
      {
        return &aFormat;
      }
    }
  }
  return nullptr;
}

void FillFormatInfo(const FormatDescriptor& theFormat, occtl_de_format_info_t& theOutInfo)
{
  theOutInfo.struct_version   = OCCTL_DE_FORMAT_INFO_VERSION_1;
  theOutInfo.p_next           = nullptr;
  theOutInfo.id               = theFormat.id;
  theOutInfo.label            = theFormat.label;
  theOutInfo.extension_count  = theFormat.extensionCount;
  theOutInfo.can_read_file    = theFormat.canReadFile;
  theOutInfo.can_write_file   = theFormat.canWriteFile;
  theOutInfo.can_read_memory  = theFormat.canReadMemory;
  theOutInfo.can_write_memory = theFormat.canWriteMemory;
}

occtl_status_t AddShapeToFreshGraph(const TopoDS_Shape&    theShape,
                                    occtl_graph_t** const  theOutGraph,
                                    occtl_node_id_t* const theOutRoot,
                                    const char* const      theContext,
                                    const occtl_status_t   theNullShapeStatus)
{
  if (theShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(
      theNullShapeStatus,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DE_Wrapper returned null shape"));
    return theNullShapeStatus;
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

occtl_status_t ReadPathToGraph(const char* const      thePath,
                               const char* const      theContext,
                               occtl_graph_t** const  theOutGraph,
                               occtl_node_id_t* const theOutRoot,
                               const occtl_status_t   theReadFailStatus)
{
  TopoDS_Shape aShape;
  try
  {
    OCC_CATCH_SIGNALS;
    if (!DE_Wrapper::GlobalWrapper()->Read(TCollection_AsciiString(thePath), aShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        theReadFailStatus,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DE_Wrapper::Read failed"));
      return theReadFailStatus;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      theReadFailStatus,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DE_Wrapper::Read threw exception"));
    return theReadFailStatus;
  }
  return AddShapeToFreshGraph(aShape, theOutGraph, theOutRoot, theContext, theReadFailStatus);
}

occtl_status_t WriteShapeToPath(const TopoDS_Shape& theShape,
                                const char* const   thePath,
                                const char* const   theContext)
{
  try
  {
    OCC_CATCH_SIGNALS;
    if (!DE_Wrapper::GlobalWrapper()->Write(TCollection_AsciiString(thePath), theShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DE_Wrapper::Write failed"));
      return OCCTL_IO_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_IO_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DE_Wrapper::Write threw exception"));
    return OCCTL_IO_ERROR;
  }
  return OCCTL_OK;
}

bool FormatSupportsMemoryStreams(const FormatDescriptor& theFormat)
{
  return std::strcmp(theFormat.id, "step") == 0 || std::strcmp(theFormat.id, "stl") == 0
         || std::strcmp(theFormat.id, "vrml") == 0;
}

const char* MemoryStreamPathForFormat(const FormatDescriptor& theFormat)
{
  if (std::strcmp(theFormat.id, "step") == 0)
  {
    return "memory.step";
  }
  if (std::strcmp(theFormat.id, "stl") == 0)
  {
    return "memory.stl";
  }
  return "memory.wrl";
}

occtl_status_t ReadStreamToGraph(const FormatDescriptor& theFormat,
                                 const uint8_t* const    theData,
                                 const size_t            theSize,
                                 const char* const       theContext,
                                 occtl_graph_t** const   theOutGraph,
                                 occtl_node_id_t* const  theOutRoot)
{
  std::basic_stringbuf<char> aBuffer(std::ios::in | std::ios::out | std::ios::binary);
  aBuffer.sputn(reinterpret_cast<const char*>(theData), static_cast<std::streamsize>(theSize));
  std::istream                aStream(&aBuffer);
  DE_Provider::ReadStreamList aStreams;
  aStreams.Append(DE_Provider::ReadStreamNode(MemoryStreamPathForFormat(theFormat), aStream));

  TopoDS_Shape aShape;
  try
  {
    OCC_CATCH_SIGNALS;
    if (!DE_Wrapper::GlobalWrapper()->Read(aStreams, aShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_FORMAT_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DE_Wrapper::Read stream failed"));
      return OCCTL_FORMAT_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_FORMAT_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DE_Wrapper::Read stream threw exception"));
    return OCCTL_FORMAT_ERROR;
  }
  return AddShapeToFreshGraph(aShape, theOutGraph, theOutRoot, theContext, OCCTL_FORMAT_ERROR);
}

occtl_status_t WriteShapeToStream(const FormatDescriptor& theFormat,
                                  const TopoDS_Shape&     theShape,
                                  const char* const       theContext,
                                  std::ostringstream&     theOutStream)
{
  DE_Provider::WriteStreamList aStreams;
  aStreams.Append(DE_Provider::WriteStreamNode(MemoryStreamPathForFormat(theFormat), theOutStream));

  try
  {
    OCC_CATCH_SIGNALS;
    if (!DE_Wrapper::GlobalWrapper()->Write(aStreams, theShape))
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_IO_ERROR,
        static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                      + ": DE_Wrapper::Write stream failed"));
      return OCCTL_IO_ERROR;
    }
  }
  catch (const Standard_Failure&)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_IO_ERROR,
      static_cast<std::string_view>(TCollection_AsciiString(theContext)
                                    + ": DE_Wrapper::Write stream threw exception"));
    return OCCTL_IO_ERROR;
  }
  return OCCTL_OK;
}

} // namespace

extern "C"
{

//==================================================================================================

OCCTL_API void OCCTL_CALL occtl_de_format_info_init(occtl_de_format_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_DE_FORMAT_INFO_INIT;
  }
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_id_from_path(const char* const  thePath,
                                                                 const char** const theOutFormatId)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr || theOutFormatId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "path and out_format_id must be non-NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    *theOutFormatId        = nullptr;
    const char* const aDot = std::strrchr(thePath, '.');
    if (aDot == nullptr)
    {
      return OCCTL_OK;
    }

    EnsureDEInitialized();

    const FormatDescriptor* const aFormat = FindFormatForExtension(aDot);
    if (aFormat == nullptr)
    {
      return OCCTL_OK;
    }
    *theOutFormatId = aFormat->id;
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_ids(const char** const theOutIds,
                                                        const size_t       theCap,
                                                        size_t* const      theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    EnsureDEInitialized();

    const NCollection_LinearVector<FormatDescriptor>& aFormats = SupportedFormatTable();
    *theOutCount                                               = aFormats.Size();

    if (theOutIds == nullptr)
    {
      return OCCTL_OK;
    }

    if (theCap < aFormats.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "occtl_de_format_ids: cap < total count");
      return OCCTL_BUFFER_TOO_SMALL;
    }

    for (size_t anIndex = 0; anIndex < aFormats.Size(); ++anIndex)
    {
      theOutIds[anIndex] = aFormats.Value(anIndex).id;
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_count(size_t* const theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    EnsureDEInitialized();
    *theOutCount = SupportedFormatTable().Size();
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_de_format_info_at(const size_t theIndex, occtl_de_format_info_t* const theOutInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "out_info is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    EnsureDEInitialized();
    const NCollection_LinearVector<FormatDescriptor>& aFormats = SupportedFormatTable();
    if (theIndex >= aFormats.Size())
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_OUT_OF_RANGE,
                                             "occtl_de_format_info_at: index is out of range");
      return OCCTL_OUT_OF_RANGE;
    }
    FillFormatInfo(aFormats.Value(theIndex), *theOutInfo);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_de_format_info_by_id(const char* const             theFormatId,
                             occtl_de_format_info_t* const theOutInfo)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theFormatId == nullptr || theOutInfo == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "format_id or out_info is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    EnsureDEInitialized();
    const FormatDescriptor* const aFormat = FindFormatForId(theFormatId);
    if (aFormat == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "occtl_de_format_info_by_id: format is not supported");
      return OCCTL_NOT_FOUND;
    }
    FillFormatInfo(*aFormat, *theOutInfo);
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_de_format_extensions(const char* const  theFormatId,
                                                               const char** const theOutExtensions,
                                                               const size_t       theCap,
                                                               size_t* const      theOutCount)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theFormatId == nullptr || theOutCount == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "format_id or out_count is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    EnsureDEInitialized();
    const FormatDescriptor* const aFormat = FindFormatForId(theFormatId);
    if (aFormat == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_NOT_FOUND,
                                             "occtl_de_format_extensions: format is not supported");
      return OCCTL_NOT_FOUND;
    }

    *theOutCount = aFormat->extensionCount;
    if (theOutExtensions == nullptr)
    {
      return OCCTL_OK;
    }
    if (theCap < aFormat->extensionCount)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_BUFFER_TOO_SMALL,
                                             "occtl_de_format_extensions: cap < extension count");
      return OCCTL_BUFFER_TOO_SMALL;
    }
    for (size_t anIndex = 0; anIndex < aFormat->extensionCount; ++anIndex)
    {
      theOutExtensions[anIndex] = aFormat->extensions[anIndex];
    }
    return OCCTL_OK;
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_de_read(const char* const      thePath,
                                                  occtl_graph_t** const  theOutGraph,
                                                  occtl_node_id_t* const theOutRoot)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (thePath == nullptr || theOutGraph == nullptr || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_de_read: NULL argument");
      if (theOutGraph != nullptr)
      {
        *theOutGraph = nullptr;
      }
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutGraph = nullptr;
    *theOutRoot  = OCCTL_NODE_ID_INVALID;

    EnsureDEInitialized();

    const char* const             aDot = std::strrchr(thePath, '.');
    const FormatDescriptor* const aFormat =
      aDot != nullptr ? FindFormatForExtension(aDot) : nullptr;
    if (aFormat == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_UNSUPPORTED,
                                             "occtl_de_read: unsupported file extension");
      return OCCTL_UNSUPPORTED;
    }
    if (aFormat->canReadFile == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_UNSUPPORTED,
                                             "occtl_de_read: format is write-only");
      return OCCTL_UNSUPPORTED;
    }

    return ReadPathToGraph(thePath, "occtl_de_read", theOutGraph, theOutRoot, OCCTL_IO_ERROR);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_de_read_memory(const char* const      theFormatId,
                                                         const uint8_t* const   theData,
                                                         const size_t           theSize,
                                                         occtl_graph_t** const  theOutGraph,
                                                         occtl_node_id_t* const theOutRoot)
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
    if (theFormatId == nullptr || theData == nullptr || theSize == 0 || theOutGraph == nullptr
        || theOutRoot == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_de_read_memory: NULL argument or empty buffer");
      return OCCTL_INVALID_ARGUMENT;
    }

    EnsureDEInitialized();

    const FormatDescriptor* const aFormat = FindFormatForId(theFormatId);
    if (aFormat == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_UNSUPPORTED,
                                             "occtl_de_read_memory: format is not supported");
      return OCCTL_UNSUPPORTED;
    }
    if (aFormat->canReadMemory == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_UNSUPPORTED,
                                             "occtl_de_read_memory: format has no memory read API");
      return OCCTL_UNSUPPORTED;
    }
    if (!FormatSupportsMemoryStreams(*aFormat))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_UNSUPPORTED,
                                             "occtl_de_read_memory: format has no stream API");
      return OCCTL_UNSUPPORTED;
    }

    return ReadStreamToGraph(*aFormat,
                             theData,
                             theSize,
                             "occtl_de_read_memory",
                             theOutGraph,
                             theOutRoot);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_de_write(const occtl_graph_t* const theGraph,
                                                   const occtl_node_id_t      theRoot,
                                                   const char* const          thePath)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theGraph == nullptr || thePath == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             theGraph ? "path is NULL" : "graph is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    EnsureDEInitialized();

    const char* const             aDot = std::strrchr(thePath, '.');
    const FormatDescriptor* const aFormat =
      aDot != nullptr ? FindFormatForExtension(aDot) : nullptr;
    if (aFormat == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_UNSUPPORTED,
                                             "occtl_de_write: unsupported file extension");
      return OCCTL_UNSUPPORTED;
    }
    if (aFormat->canWriteFile == 0)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_UNSUPPORTED,
                                             "occtl_de_write: format is read-only");
      return OCCTL_UNSUPPORTED;
    }

    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          ResolveRootShape(theGraph, theRoot, "occtl_de_write", aShape))
    {
      return aStatus;
    }

    return WriteShapeToPath(aShape, thePath, "occtl_de_write");
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_de_write_memory(const occtl_graph_t* const theGraph,
                                                          const occtl_node_id_t      theRoot,
                                                          const char* const          theFormatId,
                                                          uint8_t* const             theOutData,
                                                          const size_t               theCapacity,
                                                          size_t* const              theOutSize)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    if (theOutSize == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_de_write_memory: out_size is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutSize = 0;
    if (theFormatId == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "occtl_de_write_memory: format_id is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    EnsureDEInitialized();

    const FormatDescriptor* const aFormat = FindFormatForId(theFormatId);
    if (aFormat == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_UNSUPPORTED,
                                             "occtl_de_write_memory: format is not supported");
      return OCCTL_UNSUPPORTED;
    }
    if (aFormat->canWriteMemory == 0)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_UNSUPPORTED,
        "occtl_de_write_memory: format has no memory write API");
      return OCCTL_UNSUPPORTED;
    }
    if (!FormatSupportsMemoryStreams(*aFormat))
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_UNSUPPORTED,
                                             "occtl_de_write_memory: format has no stream API");
      return OCCTL_UNSUPPORTED;
    }

    TopoDS_Shape aShape;
    if (const occtl_status_t aStatus =
          ResolveRootShape(theGraph, theRoot, "occtl_de_write_memory", aShape))
    {
      return aStatus;
    }

    std::ostringstream aPayload(std::ios::out | std::ios::binary);
    if (const occtl_status_t aStatus =
          WriteShapeToStream(*aFormat, aShape, "occtl_de_write_memory", aPayload))
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
                                             "occtl_de_write_memory: output buffer too small");
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
