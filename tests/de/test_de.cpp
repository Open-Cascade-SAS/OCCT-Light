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

#include <occtl/occtl_core.h>
#include <occtl/occtl_de.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <occtl-hpp/de.hpp>
#include <occtl-hpp/prim.hpp>

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace
{

const char* FormatIdForPath(const char* const thePath)
{
  const char* aId = nullptr;
  EXPECT_EQ(occtl_de_format_id_from_path(thePath, &aId),
            thePath == nullptr ? OCCTL_INVALID_ARGUMENT : OCCTL_OK);
  return aId;
}

class DeTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    myPath = std::filesystem::temp_directory_path() / "occtl-de-test.brep";
    std::filesystem::remove(myPath);
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
    aBox.dx                    = 1.0;
    aBox.dy                    = 1.0;
    aBox.dz                    = 1.0;
    ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &myRoot), OCCTL_OK);
  }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
    std::error_code aEc;
    std::filesystem::remove(myPath, aEc);
  }

  occtl_graph_t*        myGraph = nullptr;
  occtl_node_id_t       myRoot{};
  std::filesystem::path myPath;
};

TEST_F(DeTest, FormatIdForPath_BrepExtension)
{
  EXPECT_STREQ(FormatIdForPath("/tmp/box.brep"), "brep");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.BREP"), "brep");
}

#ifdef OCCTL_HAS_IO_STEP
TEST_F(DeTest, FormatIdForPath_StepExtensions)
{
  EXPECT_STREQ(FormatIdForPath("/tmp/box.stp"), "step");
  EXPECT_STREQ(FormatIdForPath("/tmp/box.step"), "step");
  EXPECT_STREQ(FormatIdForPath("/tmp/box.stpz"), "step");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.STEP"), "step");
}
#endif

#ifdef OCCTL_HAS_IO_IGES
TEST_F(DeTest, FormatIdForPath_IgesExtensions)
{
  EXPECT_STREQ(FormatIdForPath("/tmp/box.igs"), "iges");
  EXPECT_STREQ(FormatIdForPath("/tmp/box.iges"), "iges");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.IGES"), "iges");
}
#endif

#ifdef OCCTL_HAS_IO_STL
TEST_F(DeTest, FormatIdForPath_StlExtension)
{
  EXPECT_STREQ(FormatIdForPath("/tmp/box.stl"), "stl");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.STL"), "stl");
}
#endif

#ifdef OCCTL_HAS_IO_GLTF
TEST_F(DeTest, FormatIdForPath_GltfExtensions)
{
  EXPECT_STREQ(FormatIdForPath("/tmp/box.gltf"), "gltf");
  EXPECT_STREQ(FormatIdForPath("/tmp/box.glb"), "gltf");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.GLTF"), "gltf");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.GLB"), "gltf");
}
#endif

#ifdef OCCTL_HAS_IO_OBJ
TEST_F(DeTest, FormatIdForPath_ObjExtension)
{
  EXPECT_STREQ(FormatIdForPath("/tmp/box.obj"), "obj");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.OBJ"), "obj");
}
#endif

#ifdef OCCTL_HAS_IO_VRML
TEST_F(DeTest, FormatIdForPath_VrmlExtensions)
{
  EXPECT_STREQ(FormatIdForPath("/tmp/box.vrml"), "vrml");
  EXPECT_STREQ(FormatIdForPath("/tmp/box.wrl"), "vrml");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.VRML"), "vrml");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.WRL"), "vrml");
}
#endif

#ifdef OCCTL_HAS_IO_PLY
TEST_F(DeTest, FormatIdForPath_PlyExtension)
{
  EXPECT_STREQ(FormatIdForPath("/tmp/box.ply"), "ply");
  EXPECT_STREQ(FormatIdForPath("/tmp/BOX.PLY"), "ply");
}
#endif

TEST_F(DeTest, FormatIdForPath_UnknownExtension_ReturnsNull)
{
  EXPECT_EQ(FormatIdForPath("/tmp/box.unknown-occtl"), nullptr);
  EXPECT_EQ(FormatIdForPath("noextension"), nullptr);
  const char* aId = nullptr;
  EXPECT_EQ(occtl_de_format_id_from_path(nullptr, &aId), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_format_id_from_path("/tmp/box.brep", nullptr), OCCTL_INVALID_ARGUMENT);
}

#ifndef OCCTL_HAS_IO_PLY
TEST_F(DeTest, FormatIdForPath_PlyExtensionWithoutModule_ReturnsNull)
{
  EXPECT_EQ(FormatIdForPath("/tmp/box.ply"), nullptr);
  EXPECT_EQ(FormatIdForPath("/tmp/BOX.PLY"), nullptr);
}
#endif

#ifndef OCCTL_HAS_IO_VRML
TEST_F(DeTest, FormatIdForPath_VrmlExtensionsWithoutModule_ReturnsNull)
{
  EXPECT_EQ(FormatIdForPath("/tmp/box.vrml"), nullptr);
  EXPECT_EQ(FormatIdForPath("/tmp/box.wrl"), nullptr);
  EXPECT_EQ(FormatIdForPath("/tmp/BOX.VRML"), nullptr);
  EXPECT_EQ(FormatIdForPath("/tmp/BOX.WRL"), nullptr);
}
#endif

TEST_F(DeTest, SupportedFormats_IncludesBrep)
{
  size_t aCount = 0;
  ASSERT_EQ(occtl_de_format_ids(nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_GE(aCount, 1u);

  std::vector<const char*> aIds(aCount);
  size_t                   aGot = 0;
  ASSERT_EQ(occtl_de_format_ids(aIds.data(), aCount, &aGot), OCCTL_OK);
  EXPECT_EQ(aGot, aCount);

  bool aFoundBrep = false;
#ifdef OCCTL_HAS_IO_GLTF
  bool aFoundGltf = false;
#endif
#ifdef OCCTL_HAS_IO_OBJ
  bool aFoundObj = false;
#endif
#ifdef OCCTL_HAS_IO_VRML
  bool aFoundVrml = false;
#endif
#ifdef OCCTL_HAS_IO_PLY
  bool aFoundPly = false;
#endif
  for (size_t i = 0; i < aGot; ++i)
  {
    if (std::strcmp(aIds[i], "brep") == 0)
    {
      aFoundBrep = true;
    }
#ifdef OCCTL_HAS_IO_GLTF
    if (std::strcmp(aIds[i], "gltf") == 0)
    {
      aFoundGltf = true;
    }
#endif
#ifdef OCCTL_HAS_IO_OBJ
    if (std::strcmp(aIds[i], "obj") == 0)
    {
      aFoundObj = true;
    }
#endif
#ifdef OCCTL_HAS_IO_VRML
    if (std::strcmp(aIds[i], "vrml") == 0)
    {
      aFoundVrml = true;
    }
#endif
#ifdef OCCTL_HAS_IO_PLY
    if (std::strcmp(aIds[i], "ply") == 0)
    {
      aFoundPly = true;
    }
#endif
  }
  EXPECT_TRUE(aFoundBrep);
#ifdef OCCTL_HAS_IO_GLTF
  EXPECT_TRUE(aFoundGltf);
#endif
#ifdef OCCTL_HAS_IO_OBJ
  EXPECT_TRUE(aFoundObj);
#endif
#ifdef OCCTL_HAS_IO_VRML
  EXPECT_TRUE(aFoundVrml);
#endif
#ifdef OCCTL_HAS_IO_PLY
  EXPECT_TRUE(aFoundPly);
#endif
}

TEST_F(DeTest, SupportedFormats_NullCount_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_de_format_ids(nullptr, 0, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(DeTest, FormatMetadata_ListsIdsAndExtensions)
{
  size_t aCount = 0;
  ASSERT_EQ(occtl_de_format_count(&aCount), OCCTL_OK);
  ASSERT_GE(aCount, 1u);

  std::set<std::string> aIds;
  for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
  {
    occtl_de_format_info_t anInfo = OCCTL_DE_FORMAT_INFO_INIT;
    ASSERT_EQ(occtl_de_format_info_at(anIndex, &anInfo), OCCTL_OK);
    EXPECT_EQ(anInfo.struct_version, OCCTL_DE_FORMAT_INFO_VERSION_1);
    EXPECT_EQ(anInfo.p_next, nullptr);
    ASSERT_NE(anInfo.id, nullptr);
    ASSERT_NE(anInfo.label, nullptr);
    EXPECT_GT(std::strlen(anInfo.id), 0u);
    EXPECT_GT(std::strlen(anInfo.label), 0u);
    EXPECT_GT(anInfo.extension_count, 0u);
    if (std::strcmp(anInfo.id, "ply") == 0)
    {
      EXPECT_EQ(anInfo.can_read_file, 0);
      EXPECT_EQ(anInfo.can_write_file, 1);
      EXPECT_EQ(anInfo.can_read_memory, 0);
      EXPECT_EQ(anInfo.can_write_memory, 0);
    }
    else if (std::strcmp(anInfo.id, "step") == 0 || std::strcmp(anInfo.id, "stl") == 0
             || std::strcmp(anInfo.id, "vrml") == 0)
    {
      EXPECT_EQ(anInfo.can_read_file, 1);
      EXPECT_EQ(anInfo.can_write_file, 1);
      EXPECT_EQ(anInfo.can_read_memory, 1);
      EXPECT_EQ(anInfo.can_write_memory, 1);
    }
    else
    {
      EXPECT_EQ(anInfo.can_read_file, 1);
      EXPECT_EQ(anInfo.can_write_file, 1);
      EXPECT_EQ(anInfo.can_read_memory, 0);
      EXPECT_EQ(anInfo.can_write_memory, 0);
    }
    aIds.insert(anInfo.id);

    size_t anExtCount = 0;
    ASSERT_EQ(occtl_de_format_extensions(anInfo.id, nullptr, 0, &anExtCount), OCCTL_OK);
    EXPECT_EQ(anExtCount, anInfo.extension_count);
    std::vector<const char*> anExts(anExtCount);
    size_t                   anExtGot = 0;
    ASSERT_EQ(occtl_de_format_extensions(anInfo.id, anExts.data(), anExts.size(), &anExtGot),
              OCCTL_OK);
    EXPECT_EQ(anExtGot, anExtCount);
    for (const char* const anExt : anExts)
    {
      ASSERT_NE(anExt, nullptr);
      EXPECT_EQ(anExt[0], '.');
      EXPECT_STREQ(FormatIdForPath((std::string("/tmp/file") + anExt).c_str()), anInfo.id);
    }
  }

  EXPECT_TRUE(aIds.count("brep") != 0u);
#ifdef OCCTL_HAS_IO_STEP
  EXPECT_TRUE(aIds.count("step") != 0u);
#endif
#ifdef OCCTL_HAS_IO_IGES
  EXPECT_TRUE(aIds.count("iges") != 0u);
#endif
#ifdef OCCTL_HAS_IO_STL
  EXPECT_TRUE(aIds.count("stl") != 0u);
#endif
#ifdef OCCTL_HAS_IO_OBJ
  EXPECT_TRUE(aIds.count("obj") != 0u);
#endif
#ifdef OCCTL_HAS_IO_GLTF
  EXPECT_TRUE(aIds.count("gltf") != 0u);
#endif
#ifdef OCCTL_HAS_IO_VRML
  EXPECT_TRUE(aIds.count("vrml") != 0u);
#endif
#ifdef OCCTL_HAS_IO_PLY
  EXPECT_TRUE(aIds.count("ply") != 0u);
#endif
}

TEST_F(DeTest, FormatInfoInit_SetsDefaults)
{
  occtl_de_format_info_t anInfo{};
  anInfo.struct_version = 99u;
  anInfo.p_next         = reinterpret_cast<const void*>(0x1);
  anInfo.id             = "dirty";
  occtl_de_format_info_init(&anInfo);
  EXPECT_EQ(anInfo.struct_version, OCCTL_DE_FORMAT_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.id, nullptr);
  EXPECT_EQ(anInfo.label, nullptr);
  EXPECT_EQ(anInfo.extension_count, 0u);
  EXPECT_EQ(anInfo.can_read_file, 0);
  EXPECT_EQ(anInfo.can_write_file, 0);
  EXPECT_EQ(anInfo.can_read_memory, 0);
  EXPECT_EQ(anInfo.can_write_memory, 0);
  occtl_de_format_info_init(nullptr);
}

TEST_F(DeTest, FormatMetadata_ErrorCases)
{
  EXPECT_EQ(occtl_de_format_count(nullptr), OCCTL_INVALID_ARGUMENT);

  occtl_de_format_info_t anInfo = OCCTL_DE_FORMAT_INFO_INIT;
  EXPECT_EQ(occtl_de_format_info_at(static_cast<size_t>(-1), &anInfo), OCCTL_OUT_OF_RANGE);
  EXPECT_EQ(occtl_de_format_info_at(0, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_format_info_by_id(nullptr, &anInfo), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_format_info_by_id("missing", &anInfo), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_de_format_info_by_id("BREP", &anInfo), OCCTL_OK);
  EXPECT_EQ(anInfo.struct_version, OCCTL_DE_FORMAT_INFO_VERSION_1);
  EXPECT_STREQ(anInfo.id, "brep");

  size_t anExtCount = 0;
  EXPECT_EQ(occtl_de_format_extensions(nullptr, nullptr, 0, &anExtCount), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_format_extensions("missing", nullptr, 0, &anExtCount), OCCTL_NOT_FOUND);
  EXPECT_EQ(occtl_de_format_extensions("brep", nullptr, 0, nullptr), OCCTL_INVALID_ARGUMENT);
#ifdef OCCTL_HAS_IO_STEP
  ASSERT_EQ(occtl_de_format_extensions("step", nullptr, 0, &anExtCount), OCCTL_OK);
  ASSERT_GT(anExtCount, 1u);
  const char* anExt = nullptr;
  EXPECT_EQ(occtl_de_format_extensions("step", &anExt, 1u, &anExtCount), OCCTL_BUFFER_TOO_SMALL);
#endif
}

TEST_F(DeTest, RoundTrip_BrepBoxPreservesCounts)
{
  ASSERT_EQ(occtl_de_write(myGraph, myRoot, myPath.string().c_str()), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myPath));

  occtl_graph_t*  aBack  = nullptr;
  occtl_node_id_t aBackR = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_de_read(myPath.string().c_str(), &aBack, &aBackR), OCCTL_OK);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{6});
  occtl_graph_free(aBack);
}

TEST_F(DeTest, MemoryRoundTrip_StepBoxPreservesCounts)
{
  size_t aSize = 0;
  ASSERT_EQ(occtl_de_write_memory(myGraph, myRoot, "step", nullptr, 0, &aSize), OCCTL_OK);
  ASSERT_GT(aSize, 0u);

  std::vector<uint8_t> aBytes(aSize);
  ASSERT_EQ(occtl_de_write_memory(myGraph, myRoot, "step", aBytes.data(), aBytes.size(), &aSize),
            OCCTL_OK);

  occtl_graph_t*  aBack  = nullptr;
  occtl_node_id_t aBackR = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_de_read_memory("step", aBytes.data(), aSize, &aBack, &aBackR), OCCTL_OK);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{6});
  occtl_graph_free(aBack);
}

TEST_F(DeTest, WriteMemory_BufferTooSmall_ReturnsRequiredSize)
{
  size_t aSize = 0;
  ASSERT_EQ(occtl_de_write_memory(myGraph, myRoot, "step", nullptr, 0, &aSize), OCCTL_OK);
  ASSERT_GT(aSize, 1u);

  uint8_t aByte = 0;
  EXPECT_EQ(occtl_de_write_memory(myGraph, myRoot, "step", &aByte, 1, &aSize),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_GT(aSize, 1u);
  EXPECT_EQ(occtl_de_write_memory(myGraph, myRoot, "step", &aByte, 0, &aSize),
            OCCTL_BUFFER_TOO_SMALL);
}

TEST_F(DeTest, Memory_ErrorCases)
{
  size_t          aSize  = 0;
  uint8_t         aByte  = 0;
  occtl_graph_t*  aBack  = nullptr;
  occtl_node_id_t aBackR = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_de_read_memory(nullptr, &aByte, 1, &aBack, &aBackR), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_read_memory("step", nullptr, 1, &aBack, &aBackR), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_read_memory("step", &aByte, 0, &aBack, &aBackR), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_read_memory("missing", &aByte, 1, &aBack, &aBackR), OCCTL_UNSUPPORTED);
  EXPECT_EQ(occtl_de_read_memory("brep", &aByte, 1, &aBack, &aBackR), OCCTL_UNSUPPORTED);
  EXPECT_EQ(occtl_de_read_memory("step", &aByte, 1, nullptr, &aBackR), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_read_memory("step", &aByte, 1, &aBack, nullptr), OCCTL_INVALID_ARGUMENT);

  EXPECT_EQ(occtl_de_write_memory(myGraph, myRoot, nullptr, nullptr, 0, &aSize),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_write_memory(myGraph, myRoot, "missing", nullptr, 0, &aSize),
            OCCTL_UNSUPPORTED);
  EXPECT_EQ(occtl_de_write_memory(myGraph, myRoot, "brep", nullptr, 0, &aSize), OCCTL_UNSUPPORTED);
  EXPECT_EQ(occtl_de_write_memory(myGraph, myRoot, "step", nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_de_write_memory(nullptr, myRoot, "step", nullptr, 0, &aSize),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(DeTest, VeneerMemoryRoundTrip_StepBoxPreservesCounts)
{
  occtl::Graph        aGraph;
  const occtl::NodeId aRoot = occtl::prim::make_box(aGraph, 1.0, 1.0, 1.0);

  const std::vector<uint8_t> aBytes = occtl::de::write_memory(aGraph, aRoot, "step");
  ASSERT_GT(aBytes.size(), 0u);

  std::pair<occtl::Graph, occtl::NodeId> aBack = occtl::de::read_memory("step", aBytes);
  EXPECT_EQ(aBack.first.face_count(), size_t{6});
}

TEST_F(DeTest, Read_UnsupportedExtension_ReturnsUnsupported)
{
  occtl_graph_t*  aBack = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_de_read("/tmp/nope.xyz", &aBack, &aR), OCCTL_UNSUPPORTED);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

TEST_F(DeTest, Write_UnsupportedExtension_ReturnsUnsupported)
{
  EXPECT_EQ(occtl_de_write(myGraph, myRoot, "/tmp/nope.xyz"), OCCTL_UNSUPPORTED);
  EXPECT_NE(occtl_error_last()->message, nullptr);
  EXPECT_GT(std::strlen(occtl_error_last()->message), 0u);
}

} // namespace
