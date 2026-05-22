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
#include <occtl/occtl_io_step.h>
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

class IoStepTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    myPath = std::filesystem::temp_directory_path() / "occtl-iostep-test.stp";
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

TEST_F(IoStepTest, ReadOptionsInit_HasV1)
{
  occtl_io_step_read_options_t aOpts{};
  occtl_io_step_read_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_STEP_READ_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.read_color, 1);
  EXPECT_EQ(aOpts.read_name, 1);
  EXPECT_EQ(aOpts.read_layer, 1);
}

TEST_F(IoStepTest, WriteOptionsInit_HasV1)
{
  occtl_io_step_write_options_t aOpts{};
  occtl_io_step_write_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_STEP_WRITE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.unit, OCCTL_IO_STEP_UNIT_MM);
  EXPECT_EQ(aOpts.schema, OCCTL_IO_STEP_SCHEMA_AP242);
}

TEST_F(IoStepTest, RoundTrip_BoxPreservesCounts)
{
  ASSERT_EQ(occtl_io_step_write(myGraph, myRoot, myPath.string().c_str(), nullptr), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myPath));

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_step_read(myPath.string().c_str(), &aBack, &aBackRoot, nullptr), OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);

  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aBack), size_t{1});
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{6});

  occtl_graph_free(aBack);
}

TEST_F(IoStepTest, RoundTripMemory_BoxPreservesCounts)
{
  size_t aSize = 0;
  ASSERT_EQ(occtl_io_step_write_memory(myGraph, myRoot, nullptr, nullptr, 0, &aSize), OCCTL_OK);
  ASSERT_GT(aSize, 1u);

  std::vector<uint8_t> aData(aSize);
  ASSERT_EQ(
    occtl_io_step_write_memory(myGraph, myRoot, nullptr, aData.data(), aData.size(), &aSize),
    OCCTL_OK);
  ASSERT_GT(aSize, 1u);
  aData.resize(aSize);

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_step_read_memory(aData.data(), aData.size(), &aBack, &aBackRoot, nullptr),
            OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aBack), size_t{1});
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{6});
  occtl_graph_free(aBack);
}

TEST_F(IoStepTest, WriteMemory_BufferTooSmall_ReturnsRequiredSize)
{
  size_t aRequired = 0;
  ASSERT_EQ(occtl_io_step_write_memory(myGraph, myRoot, nullptr, nullptr, 0, &aRequired), OCCTL_OK);
  ASSERT_GT(aRequired, 1u);

  uint8_t aByte = 0;
  EXPECT_EQ(occtl_io_step_write_memory(myGraph, myRoot, nullptr, &aByte, 1, &aRequired),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_GT(aRequired, 1u);
  EXPECT_EQ(occtl_io_step_write_memory(myGraph, myRoot, nullptr, &aByte, 0, &aRequired),
            OCCTL_BUFFER_TOO_SMALL);
}

TEST_F(IoStepTest, Read_NullPath_ReturnsInvalidArgument)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_step_read(nullptr, &aG, &aR, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoStepTest, ReadMemory_NullArgs_ReturnsInvalidArgument)
{
  occtl_graph_t*  aGraph = nullptr;
  occtl_node_id_t aRoot  = OCCTL_NODE_ID_INVALID;
  const uint8_t   aByte  = 0;
  EXPECT_EQ(occtl_io_step_read_memory(nullptr, 1, &aGraph, &aRoot, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_step_read_memory(&aByte, 0, &aGraph, &aRoot, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_step_read_memory(&aByte, 1, nullptr, &aRoot, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_step_read_memory(&aByte, 1, &aGraph, nullptr, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoStepTest, ReadMemory_InvalidPayload_ReturnsFormatError)
{
  const uint8_t   aData[4] = {'n', 'o', 'p', 'e'};
  occtl_graph_t*  aGraph   = nullptr;
  occtl_node_id_t aRoot    = OCCTL_NODE_ID_INVALID;
  EXPECT_NE(occtl_io_step_read_memory(aData, sizeof(aData), &aGraph, &aRoot, nullptr), OCCTL_OK);
  EXPECT_EQ(aGraph, nullptr);
  EXPECT_EQ(aRoot.bits, OCCTL_NODE_ID_INVALID.bits);
}

TEST_F(IoStepTest, Read_MissingFile_ReturnsIoError)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_step_read("/tmp/this-file-does-not-exist-occtl.stp", &aG, &aR, nullptr),
            OCCTL_IO_ERROR);
}

TEST_F(IoStepTest, Write_NullGraph_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_io_step_write(nullptr, myRoot, myPath.string().c_str(), nullptr),
            OCCTL_INVALID_ARGUMENT);
  size_t aSize = 0;
  EXPECT_EQ(occtl_io_step_write_memory(nullptr, myRoot, nullptr, nullptr, 0, &aSize),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_step_write_memory(myGraph, myRoot, nullptr, nullptr, 0, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoStepTest, Write_InvalidRoot_ReturnsNotFound)
{
  occtl_node_id_t aBad{};
  aBad.bits = 0xdeadbeefdeadbeefULL;
  EXPECT_EQ(occtl_io_step_write(myGraph, aBad, myPath.string().c_str(), nullptr), OCCTL_NOT_FOUND);
}

TEST_F(IoStepTest, Read_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_step_read_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_graph_t*  aG   = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_step_read("/tmp/any.stp", &aG, &aR, &aOpts), OCCTL_VERSION_MISMATCH);
}

TEST_F(IoStepTest, Read_InvalidOptionsFields_ReturnsInvalidArgument)
{
  occtl_io_step_read_options_t aOpts;
  occtl_io_step_read_options_init(&aOpts);
  aOpts.p_next       = &aOpts;
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_step_read("/tmp/any.stp", &aG, &aR, &aOpts), OCCTL_INVALID_ARGUMENT);

  occtl_io_step_read_options_init(&aOpts);
  aOpts.read_layer = 2;
  EXPECT_EQ(occtl_io_step_read("/tmp/any.stp", &aG, &aR, &aOpts), OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoStepTest, Write_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_step_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  EXPECT_EQ(occtl_io_step_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_VERSION_MISMATCH);
  size_t aSize = 0;
  EXPECT_EQ(occtl_io_step_write_memory(myGraph, myRoot, &aOpts, nullptr, 0, &aSize),
            OCCTL_VERSION_MISMATCH);
}

TEST_F(IoStepTest, Write_InvalidOptionsFields_ReturnsInvalidArgumentOrOutOfRange)
{
  occtl_io_step_write_options_t aOpts;
  occtl_io_step_write_options_init(&aOpts);
  aOpts.p_next = &aOpts;
  size_t aSize = 0;
  EXPECT_EQ(occtl_io_step_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_step_write_memory(myGraph, myRoot, &aOpts, nullptr, 0, &aSize),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_step_write_options_init(&aOpts);
  aOpts.write_surface_curves = 2;
  EXPECT_EQ(occtl_io_step_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_step_write_options_init(&aOpts);
  aOpts.unit = static_cast<occtl_io_step_length_unit_t>(0x1234);
  EXPECT_EQ(occtl_io_step_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_OUT_OF_RANGE);

  occtl_io_step_write_options_init(&aOpts);
  aOpts.schema = static_cast<occtl_io_step_schema_t>(0x1234);
  EXPECT_EQ(occtl_io_step_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_OUT_OF_RANGE);
}

TEST_F(IoStepTest, ErrorAfterFailure_HasMessage)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  occtl_io_step_read("/tmp/missing.stp", &aG, &aR, nullptr);
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_NE(aErr->message, nullptr);
  EXPECT_GT(std::strlen(aErr->message), 0u);
}

TEST_F(IoStepTest, Write_Ap203Schema_WritesFile)
{
  occtl_io_step_write_options_t aOpts;
  occtl_io_step_write_options_init(&aOpts);
  aOpts.schema = OCCTL_IO_STEP_SCHEMA_AP203;
  ASSERT_EQ(occtl_io_step_write(myGraph, myRoot, myPath.string().c_str(), &aOpts), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myPath));

  occtl_graph_t*  aBack = nullptr;
  occtl_node_id_t aBackR{};
  ASSERT_EQ(occtl_io_step_read(myPath.string().c_str(), &aBack, &aBackR, nullptr), OCCTL_OK);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{6});
  occtl_graph_free(aBack);
}

} // namespace
