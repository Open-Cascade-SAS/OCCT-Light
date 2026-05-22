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

#include <occtl-hpp/io_stl.hpp>
#include <occtl-hpp/prim.hpp>
#include <occtl/occtl_core.h>
#include <occtl/occtl_io_stl.h>
#include <occtl/occtl_mesh.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace
{

class IoStlTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    myPath = std::filesystem::temp_directory_path() / "occtl-iostl-test.stl";
    std::filesystem::remove(myPath);
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);

    occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
    aBox.dx                    = 1.0;
    aBox.dy                    = 1.0;
    aBox.dz                    = 1.0;
    ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &myRoot), OCCTL_OK);

    occtl_mesh_options_t aMeshOpts;
    occtl_mesh_options_init(&aMeshOpts);
    ASSERT_EQ(occtl_mesh_generate(myGraph, &myRoot, 1, &aMeshOpts), OCCTL_OK);
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

TEST_F(IoStlTest, WriteOptionsInit_HasV1)
{
  occtl_io_stl_write_options_t aOpts{};
  occtl_io_stl_write_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_STL_WRITE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.ascii_mode, 0);
}

TEST_F(IoStlTest, RoundTrip_BinaryBox)
{
  ASSERT_EQ(occtl_io_stl_write(myGraph, myRoot, myPath.string().c_str(), nullptr), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myPath));

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_stl_read(myPath.string().c_str(), &aBack, &aBackRoot), OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);

  occtl_graph_free(aBack);
}

TEST_F(IoStlTest, RoundTrip_AsciiBox)
{
  occtl_io_stl_write_options_t aOpts;
  occtl_io_stl_write_options_init(&aOpts);
  aOpts.ascii_mode = 1;
  ASSERT_EQ(occtl_io_stl_write(myGraph, myRoot, myPath.string().c_str(), &aOpts), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myPath));

  occtl_graph_t*  aBack = nullptr;
  occtl_node_id_t aBackR{};
  ASSERT_EQ(occtl_io_stl_read(myPath.string().c_str(), &aBack, &aBackR), OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  occtl_graph_free(aBack);
}

TEST_F(IoStlTest, MemoryRoundTrip_BinaryBox)
{
  size_t aSize = 0;
  ASSERT_EQ(occtl_io_stl_write_memory(myGraph, myRoot, nullptr, nullptr, 0, &aSize), OCCTL_OK);
  ASSERT_GT(aSize, 0u);
  std::vector<uint8_t> aBytes(aSize);
  ASSERT_EQ(
    occtl_io_stl_write_memory(myGraph, myRoot, nullptr, aBytes.data(), aBytes.size(), &aSize),
    OCCTL_OK);
  ASSERT_GT(aSize, 0u);

  occtl_graph_t*  aBack = nullptr;
  occtl_node_id_t aBackR{};
  ASSERT_EQ(occtl_io_stl_read_memory(aBytes.data(), aSize, &aBack, &aBackR), OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackR.bits, 0u);
  occtl_graph_free(aBack);
}

TEST_F(IoStlTest, MemoryRoundTrip_AsciiBox)
{
  occtl_io_stl_write_options_t aOpts;
  occtl_io_stl_write_options_init(&aOpts);
  aOpts.ascii_mode = 1;

  size_t aSize = 0;
  ASSERT_EQ(occtl_io_stl_write_memory(myGraph, myRoot, &aOpts, nullptr, 0, &aSize), OCCTL_OK);
  ASSERT_GT(aSize, 0u);
  std::vector<uint8_t> aBytes(aSize);
  ASSERT_EQ(
    occtl_io_stl_write_memory(myGraph, myRoot, &aOpts, aBytes.data(), aBytes.size(), &aSize),
    OCCTL_OK);
  ASSERT_GT(aSize, 0u);
  ASSERT_NE(std::string(reinterpret_cast<const char*>(aBytes.data()), 5), std::string());

  occtl_graph_t*  aBack = nullptr;
  occtl_node_id_t aBackR{};
  ASSERT_EQ(occtl_io_stl_read_memory(aBytes.data(), aSize, &aBack, &aBackR), OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  occtl_graph_free(aBack);
}

TEST_F(IoStlTest, WriteMemory_BufferTooSmall_ReturnsRequiredSize)
{
  size_t aSize = 0;
  ASSERT_EQ(occtl_io_stl_write_memory(myGraph, myRoot, nullptr, nullptr, 0, &aSize), OCCTL_OK);
  ASSERT_GT(aSize, 1u);
  std::vector<uint8_t> aBytes(aSize - 1u);
  EXPECT_EQ(
    occtl_io_stl_write_memory(myGraph, myRoot, nullptr, aBytes.data(), aBytes.size(), &aSize),
    OCCTL_BUFFER_TOO_SMALL);
  EXPECT_GT(aSize, aBytes.size());
  uint8_t aByte = 0;
  EXPECT_EQ(occtl_io_stl_write_memory(myGraph, myRoot, nullptr, &aByte, 0, &aSize),
            OCCTL_BUFFER_TOO_SMALL);
}

TEST_F(IoStlTest, ReadMemory_InvalidPayload_ReturnsFormatError)
{
  const uint8_t   aBad[] = {1u, 2u, 3u, 4u, 5u};
  occtl_graph_t*  aBack  = nullptr;
  occtl_node_id_t aBackR{};
  EXPECT_EQ(occtl_io_stl_read_memory(aBad, sizeof(aBad), &aBack, &aBackR), OCCTL_FORMAT_ERROR);
  EXPECT_EQ(aBack, nullptr);
}

TEST_F(IoStlTest, Read_NullPath_ReturnsInvalidArgument)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_stl_read(nullptr, &aG, &aR), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoStlTest, Read_MissingFile_ReturnsOK)
{
  // DESTL_Provider returns OK with empty shape for missing files
  // (different from DESTEP_Provider/DEBREP_Provider).
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_stl_read("/tmp/this-file-does-not-exist-occtl.stl", &aG, &aR), OCCTL_OK);
  if (aG != nullptr)
  {
    occtl_graph_free(aG);
  }
}

TEST_F(IoStlTest, ErrorAfterFailure_HasMessage)
{
  occtl_io_stl_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_io_stl_write(myGraph, myRoot, myPath.string().c_str(), &aOpts);
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_NE(aErr->message, nullptr);
  EXPECT_GT(std::strlen(aErr->message), 0u);
}

TEST_F(IoStlTest, Write_NullGraph_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_io_stl_write(nullptr, myRoot, myPath.string().c_str(), nullptr),
            OCCTL_INVALID_ARGUMENT);
  size_t aSize = 0;
  EXPECT_EQ(occtl_io_stl_write_memory(nullptr, myRoot, nullptr, nullptr, 0, &aSize),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoStlTest, Write_InvalidRoot_ReturnsNotFound)
{
  occtl_node_id_t aBad{};
  aBad.bits = 0xdeadbeefdeadbeefULL;
  EXPECT_EQ(occtl_io_stl_write(myGraph, aBad, myPath.string().c_str(), nullptr), OCCTL_NOT_FOUND);
}

TEST_F(IoStlTest, Write_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_stl_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  EXPECT_EQ(occtl_io_stl_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_VERSION_MISMATCH);
  size_t aSize = 0;
  EXPECT_EQ(occtl_io_stl_write_memory(myGraph, myRoot, &aOpts, nullptr, 0, &aSize),
            OCCTL_VERSION_MISMATCH);
}

TEST_F(IoStlTest, Write_InvalidOptionsFields_ReturnsInvalidArgument)
{
  occtl_io_stl_write_options_t aOpts;
  occtl_io_stl_write_options_init(&aOpts);
  aOpts.p_next = &aOpts;
  EXPECT_EQ(occtl_io_stl_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);
  size_t aSize = 0;
  EXPECT_EQ(occtl_io_stl_write_memory(myGraph, myRoot, &aOpts, nullptr, 0, &aSize),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_stl_write_options_init(&aOpts);
  aOpts.ascii_mode = 2;
  EXPECT_EQ(occtl_io_stl_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_stl_write_memory(myGraph, myRoot, &aOpts, nullptr, 0, &aSize),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoStlTest, ReadMemory_NullArgs_ReturnsInvalidArgument)
{
  occtl_graph_t*  aBack = reinterpret_cast<occtl_graph_t*>(0x1);
  occtl_node_id_t aBackR{};
  EXPECT_EQ(occtl_io_stl_read_memory(nullptr, 1, &aBack, &aBackR), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aBack, nullptr);
  const uint8_t aByte = 0u;
  EXPECT_EQ(occtl_io_stl_read_memory(&aByte, 1, nullptr, &aBackR), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_stl_read_memory(&aByte, 1, &aBack, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(IoStlVeneerTest, MemoryRoundTrip_Box)
{
  occtl::Graph         aGraph;
  const occtl::NodeId  aRoot = occtl::prim::make_box(aGraph, 1.0, 1.0, 1.0);
  occtl_mesh_options_t aMeshOpts;
  occtl_mesh_options_init(&aMeshOpts);
  occtl_node_id_t aRawRoot = aRoot.get();
  ASSERT_EQ(occtl_mesh_generate(aGraph.get(), &aRawRoot, 1, &aMeshOpts), OCCTL_OK);

  const std::vector<uint8_t> aBytes = occtl::io_stl::write_memory(aGraph, aRoot);
  ASSERT_GT(aBytes.size(), 0u);

  std::pair<occtl::Graph, occtl::NodeId> aBack = occtl::io_stl::read_memory(aBytes);
  EXPECT_NE(aBack.second.get().bits, 0u);
}

} // namespace
