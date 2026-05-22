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
#include <occtl/occtl_io_vrml.h>
#include <occtl/occtl_mesh.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

namespace
{

class IoVrmlTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    myVrmlPath    = std::filesystem::temp_directory_path() / "occtl-iovrml-test.wrl";
    myBadVrmlPath = std::filesystem::temp_directory_path() / "occtl-iovrml-bad.wrl";
    std::filesystem::remove(myVrmlPath);
    std::filesystem::remove(myBadVrmlPath);
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
    std::filesystem::remove(myVrmlPath, aEc);
    std::filesystem::remove(myBadVrmlPath, aEc);
  }

  occtl_graph_t*        myGraph = nullptr;
  occtl_node_id_t       myRoot{};
  std::filesystem::path myVrmlPath;
  std::filesystem::path myBadVrmlPath;
};

TEST_F(IoVrmlTest, ReadOptionsInit_HasV1Defaults)
{
  occtl_io_vrml_read_options_t aOpts{};
  occtl_io_vrml_read_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_VRML_READ_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.file_length_unit_m, 1.0);
  EXPECT_EQ(aOpts.system_coordinate_system, OCCTL_IO_VRML_COORDINATE_SYSTEM_Z_UP);
  EXPECT_EQ(aOpts.file_coordinate_system, OCCTL_IO_VRML_COORDINATE_SYSTEM_Y_UP);
  EXPECT_EQ(aOpts.fill_incomplete, 1);
}

TEST_F(IoVrmlTest, WriteOptionsInit_HasV1Defaults)
{
  occtl_io_vrml_write_options_t aOpts{};
  occtl_io_vrml_write_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_VRML_WRITE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.writer_version, OCCTL_IO_VRML_WRITER_VERSION_2);
  EXPECT_EQ(aOpts.representation, OCCTL_IO_VRML_REPRESENTATION_WIREFRAME);
}

TEST_F(IoVrmlTest, RoundTrip_Box)
{
  occtl_io_vrml_write_options_t aWriteOpts;
  occtl_io_vrml_write_options_init(&aWriteOpts);
  aWriteOpts.representation = OCCTL_IO_VRML_REPRESENTATION_BOTH;

  ASSERT_EQ(occtl_io_vrml_write(myGraph, myRoot, myVrmlPath.string().c_str(), &aWriteOpts),
            OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myVrmlPath));

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_vrml_read(myVrmlPath.string().c_str(), &aBack, &aBackRoot, nullptr), OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{0});

  occtl_graph_free(aBack);
}

TEST_F(IoVrmlTest, RoundTripMemory_Box)
{
  occtl_io_vrml_write_options_t aWriteOpts;
  occtl_io_vrml_write_options_init(&aWriteOpts);
  aWriteOpts.representation = OCCTL_IO_VRML_REPRESENTATION_BOTH;

  size_t aSize = 0;
  ASSERT_EQ(occtl_io_vrml_write_memory(myGraph, myRoot, &aWriteOpts, nullptr, 0, &aSize), OCCTL_OK);
  ASSERT_GT(aSize, 1u);

  std::vector<uint8_t> aData(aSize);
  ASSERT_EQ(
    occtl_io_vrml_write_memory(myGraph, myRoot, &aWriteOpts, aData.data(), aData.size(), &aSize),
    OCCTL_OK);
  ASSERT_GT(aSize, 1u);
  aData.resize(aSize);

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_vrml_read_memory(aData.data(), aData.size(), &aBack, &aBackRoot, nullptr),
            OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{0});
  occtl_graph_free(aBack);
}

TEST_F(IoVrmlTest, WriteMemory_BufferTooSmall_ReturnsRequiredSize)
{
  size_t aSize = 0;
  ASSERT_EQ(occtl_io_vrml_write_memory(myGraph, myRoot, nullptr, nullptr, 0, &aSize), OCCTL_OK);
  ASSERT_GT(aSize, 1u);

  uint8_t aByte = 0;
  EXPECT_EQ(occtl_io_vrml_write_memory(myGraph, myRoot, nullptr, &aByte, 1, &aSize),
            OCCTL_BUFFER_TOO_SMALL);
  EXPECT_GT(aSize, 1u);
  EXPECT_EQ(occtl_io_vrml_write_memory(myGraph, myRoot, nullptr, &aByte, 0, &aSize),
            OCCTL_BUFFER_TOO_SMALL);
}

TEST_F(IoVrmlTest, ReadMemory_InvalidPayload_ReturnsFormatError)
{
  const uint8_t        aData[4] = {'n', 'o', 'p', 'e'};
  occtl_graph_t*       aGraph   = nullptr;
  occtl_node_id_t      aRoot    = OCCTL_NODE_ID_INVALID;
  const occtl_status_t aStatus =
    occtl_io_vrml_read_memory(aData, sizeof(aData), &aGraph, &aRoot, nullptr);
  EXPECT_TRUE(aStatus == OCCTL_FORMAT_ERROR || aStatus == OCCTL_IO_ERROR);
  EXPECT_EQ(aGraph, nullptr);
}

TEST_F(IoVrmlTest, ReadMemory_NullArgs_ReturnsInvalidArgument)
{
  occtl_graph_t*  aGraph = nullptr;
  occtl_node_id_t aRoot  = OCCTL_NODE_ID_INVALID;
  const uint8_t   aByte  = 0;
  EXPECT_EQ(occtl_io_vrml_read_memory(nullptr, 1, &aGraph, &aRoot, nullptr),
            OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_vrml_read_memory(&aByte, 0, &aGraph, &aRoot, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_vrml_read_memory(&aByte, 1, nullptr, &aRoot, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_io_vrml_read_memory(&aByte, 1, &aGraph, nullptr, nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoVrmlTest, Read_NullPath_ReturnsInvalidArgument)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_vrml_read(nullptr, &aG, &aR, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoVrmlTest, Read_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_vrml_read_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_graph_t*  aG   = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_vrml_read(myVrmlPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoVrmlTest, Read_InvalidOptionsFields_ReturnsInvalidArgument)
{
  occtl_io_vrml_read_options_t aOpts;
  occtl_io_vrml_read_options_init(&aOpts);
  aOpts.p_next       = &aOpts;
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_vrml_read(myVrmlPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_vrml_read_options_init(&aOpts);
  aOpts.file_length_unit_m = std::numeric_limits<double>::infinity();
  EXPECT_EQ(occtl_io_vrml_read(myVrmlPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_vrml_read_options_init(&aOpts);
  aOpts.fill_incomplete = 2;
  EXPECT_EQ(occtl_io_vrml_read(myVrmlPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoVrmlTest, Read_BadCoordinateSystem_ReturnsOutOfRange)
{
  occtl_io_vrml_read_options_t aOpts;
  occtl_io_vrml_read_options_init(&aOpts);
  aOpts.file_coordinate_system = static_cast<occtl_io_vrml_coordinate_system_t>(0x1234);
  occtl_graph_t*  aG           = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_vrml_read(myVrmlPath.string().c_str(), &aG, &aR, &aOpts), OCCTL_OUT_OF_RANGE);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoVrmlTest, Read_MissingFile_ReturnsIoError)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_vrml_read("/tmp/this-file-does-not-exist-occtl.wrl", &aG, &aR, nullptr),
            OCCTL_IO_ERROR);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoVrmlTest, Read_MalformedFile_ReturnsFailure)
{
  {
    std::ofstream aFile(myBadVrmlPath);
    aFile << "#VRML V2.0 utf8\n";
    aFile << "Shape { geometry IndexedFaceSet { coord Coordinate { point [ broken ] } } }\n";
  }
  occtl_graph_t*               aG = nullptr;
  occtl_node_id_t              aR{};
  occtl_io_vrml_read_options_t aOpts;
  occtl_io_vrml_read_options_init(&aOpts);
  aOpts.fill_incomplete = 0;
  const occtl_status_t aStatus =
    occtl_io_vrml_read(myBadVrmlPath.string().c_str(), &aG, &aR, &aOpts);
  EXPECT_TRUE(aStatus == OCCTL_IO_ERROR || aStatus == OCCTL_FORMAT_ERROR);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoVrmlTest, Write_NullGraph_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_io_vrml_write(nullptr, myRoot, myVrmlPath.string().c_str(), nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoVrmlTest, Write_InvalidRoot_ReturnsNotFound)
{
  occtl_node_id_t aBad{};
  aBad.bits = 0xdeadbeefdeadbeefULL;
  EXPECT_EQ(occtl_io_vrml_write(myGraph, aBad, myVrmlPath.string().c_str(), nullptr),
            OCCTL_NOT_FOUND);
}

TEST_F(IoVrmlTest, Write_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_vrml_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  EXPECT_EQ(occtl_io_vrml_write(myGraph, myRoot, myVrmlPath.string().c_str(), &aOpts),
            OCCTL_VERSION_MISMATCH);
}

TEST_F(IoVrmlTest, Write_PNext_ReturnsInvalidArgument)
{
  occtl_io_vrml_write_options_t aOpts;
  occtl_io_vrml_write_options_init(&aOpts);
  aOpts.p_next = &aOpts;
  EXPECT_EQ(occtl_io_vrml_write(myGraph, myRoot, myVrmlPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoVrmlTest, Write_BadRepresentation_ReturnsOutOfRange)
{
  occtl_io_vrml_write_options_t aOpts;
  occtl_io_vrml_write_options_init(&aOpts);
  aOpts.representation = static_cast<occtl_io_vrml_representation_t>(0x1234);
  EXPECT_EQ(occtl_io_vrml_write(myGraph, myRoot, myVrmlPath.string().c_str(), &aOpts),
            OCCTL_OUT_OF_RANGE);
}

TEST_F(IoVrmlTest, ErrorAfterFailure_HasMessage)
{
  occtl_io_vrml_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_io_vrml_write(myGraph, myRoot, myVrmlPath.string().c_str(), &aOpts);
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_NE(aErr->message, nullptr);
  EXPECT_GT(std::strlen(aErr->message), 0u);
}

} // namespace
