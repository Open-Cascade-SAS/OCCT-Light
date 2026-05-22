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
#include <occtl/occtl_io_obj.h>
#include <occtl/occtl_mesh.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

namespace
{

class IoObjTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    myObjPath    = std::filesystem::temp_directory_path() / "occtl-ioobj-test.obj";
    myBadObjPath = std::filesystem::temp_directory_path() / "occtl-ioobj-bad.obj";
    myImportPath = std::filesystem::temp_directory_path() / "occtl-ioobj-import.obj";
    std::filesystem::remove(myObjPath);
    std::filesystem::remove(myBadObjPath);
    std::filesystem::remove(myImportPath);
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
    std::filesystem::remove(myObjPath, aEc);
    std::filesystem::remove(myBadObjPath, aEc);
    std::filesystem::remove(myImportPath, aEc);
  }

  void WriteSimpleObj(const std::filesystem::path& thePath)
  {
    std::ofstream aFile(thePath);
    aFile << "o tri\n";
    aFile << "v 0 0 0\n";
    aFile << "v 1 0 0\n";
    aFile << "v 0 1 0\n";
    aFile << "f 1 2 3\n";
  }

  occtl_graph_t*        myGraph = nullptr;
  occtl_node_id_t       myRoot{};
  std::filesystem::path myObjPath;
  std::filesystem::path myBadObjPath;
  std::filesystem::path myImportPath;
};

TEST_F(IoObjTest, ReadOptionsInit_HasV1Defaults)
{
  occtl_io_obj_read_options_t aOpts{};
  occtl_io_obj_read_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_OBJ_READ_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.file_length_unit_m, 1.0);
  EXPECT_EQ(aOpts.system_coordinate_system, OCCTL_IO_OBJ_COORDINATE_SYSTEM_Z_UP);
  EXPECT_EQ(aOpts.file_coordinate_system, OCCTL_IO_OBJ_COORDINATE_SYSTEM_Y_UP);
  EXPECT_EQ(aOpts.single_precision, 0);
  EXPECT_EQ(aOpts.create_shapes, 0);
  EXPECT_EQ(aOpts.fill_incomplete, 1);
  EXPECT_EQ(aOpts.memory_limit_mib, -1);
  EXPECT_EQ(aOpts.root_prefix, nullptr);
}

TEST_F(IoObjTest, WriteOptionsInit_HasV1Defaults)
{
  occtl_io_obj_write_options_t aOpts{};
  occtl_io_obj_write_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_OBJ_WRITE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.system_coordinate_system, OCCTL_IO_OBJ_COORDINATE_SYSTEM_Z_UP);
  EXPECT_EQ(aOpts.file_coordinate_system, OCCTL_IO_OBJ_COORDINATE_SYSTEM_Y_UP);
  EXPECT_EQ(aOpts.comment, nullptr);
  EXPECT_EQ(aOpts.author, nullptr);
}

TEST_F(IoObjTest, Import_SimpleTriangle)
{
  WriteSimpleObj(myImportPath);

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_obj_read(myImportPath.string().c_str(), &aBack, &aBackRoot, nullptr),
            OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{0});

  occtl_graph_free(aBack);
}

TEST_F(IoObjTest, RoundTrip_Box)
{
  occtl_io_obj_write_options_t aWriteOpts;
  occtl_io_obj_write_options_init(&aWriteOpts);
  aWriteOpts.comment = "occt-light test";
  aWriteOpts.author  = "occt-light";

  ASSERT_EQ(occtl_io_obj_write(myGraph, myRoot, myObjPath.string().c_str(), &aWriteOpts), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myObjPath));

  occtl_io_obj_read_options_t aReadOpts;
  occtl_io_obj_read_options_init(&aReadOpts);
  aReadOpts.root_prefix = "roundtrip";

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_obj_read(myObjPath.string().c_str(), &aBack, &aBackRoot, &aReadOpts),
            OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{0});

  occtl_graph_free(aBack);
}

TEST_F(IoObjTest, Read_NullPath_ReturnsInvalidArgument)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_obj_read(nullptr, &aG, &aR, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoObjTest, Read_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_obj_read_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_graph_t*  aG   = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_obj_read(myObjPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoObjTest, Read_OptionsPNext_ReturnsInvalidArgument)
{
  occtl_io_obj_read_options_t aOpts;
  occtl_io_obj_read_options_init(&aOpts);
  aOpts.p_next       = &aOpts;
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_obj_read(myObjPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoObjTest, Read_OptionsInvalidValues_ReturnsInvalidArgument)
{
  occtl_io_obj_read_options_t aOpts;
  occtl_io_obj_read_options_init(&aOpts);
  aOpts.single_precision = 2;
  occtl_graph_t*  aG     = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_obj_read(myObjPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_obj_read_options_init(&aOpts);
  aOpts.file_length_unit_m = std::numeric_limits<double>::infinity();
  EXPECT_EQ(occtl_io_obj_read(myObjPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_obj_read_options_init(&aOpts);
  aOpts.memory_limit_mib = -2;
  EXPECT_EQ(occtl_io_obj_read(myObjPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoObjTest, Read_BadCoordinateSystem_ReturnsOutOfRange)
{
  occtl_io_obj_read_options_t aOpts;
  occtl_io_obj_read_options_init(&aOpts);
  aOpts.file_coordinate_system = static_cast<occtl_io_obj_coordinate_system_t>(0x1234);
  occtl_graph_t*  aG           = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_obj_read(myObjPath.string().c_str(), &aG, &aR, &aOpts), OCCTL_OUT_OF_RANGE);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoObjTest, Read_MissingFile_ReturnsIoError)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_obj_read("/tmp/this-file-does-not-exist-occtl.obj", &aG, &aR, nullptr),
            OCCTL_IO_ERROR);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoObjTest, Read_MalformedFile_ReturnsFailure)
{
  {
    std::ofstream aFile(myBadObjPath);
    aFile << "v not-a-number 0 0\n";
    aFile << "f 1 2 3\n";
  }
  occtl_graph_t*              aG = nullptr;
  occtl_node_id_t             aR{};
  occtl_io_obj_read_options_t aOpts;
  occtl_io_obj_read_options_init(&aOpts);
  aOpts.fill_incomplete        = 0;
  const occtl_status_t aStatus = occtl_io_obj_read(myBadObjPath.string().c_str(), &aG, &aR, &aOpts);
  EXPECT_TRUE(aStatus == OCCTL_IO_ERROR || aStatus == OCCTL_FORMAT_ERROR);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoObjTest, ErrorAfterFailure_HasMessage)
{
  occtl_io_obj_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_io_obj_write(myGraph, myRoot, myObjPath.string().c_str(), &aOpts);
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_NE(aErr->message, nullptr);
  EXPECT_GT(std::strlen(aErr->message), 0u);
}

TEST_F(IoObjTest, Write_NullGraph_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_io_obj_write(nullptr, myRoot, myObjPath.string().c_str(), nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoObjTest, Write_InvalidRoot_ReturnsNotFound)
{
  occtl_node_id_t aBad{};
  aBad.bits = 0xdeadbeefdeadbeefULL;
  EXPECT_EQ(occtl_io_obj_write(myGraph, aBad, myObjPath.string().c_str(), nullptr),
            OCCTL_NOT_FOUND);
}

TEST_F(IoObjTest, Write_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_obj_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  EXPECT_EQ(occtl_io_obj_write(myGraph, myRoot, myObjPath.string().c_str(), &aOpts),
            OCCTL_VERSION_MISMATCH);
}

TEST_F(IoObjTest, Write_OptionsPNext_ReturnsInvalidArgument)
{
  occtl_io_obj_write_options_t aOpts;
  occtl_io_obj_write_options_init(&aOpts);
  aOpts.p_next = &aOpts;
  EXPECT_EQ(occtl_io_obj_write(myGraph, myRoot, myObjPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoObjTest, Write_BadCoordinateSystem_ReturnsOutOfRange)
{
  occtl_io_obj_write_options_t aOpts;
  occtl_io_obj_write_options_init(&aOpts);
  aOpts.file_coordinate_system = static_cast<occtl_io_obj_coordinate_system_t>(0x1234);
  EXPECT_EQ(occtl_io_obj_write(myGraph, myRoot, myObjPath.string().c_str(), &aOpts),
            OCCTL_OUT_OF_RANGE);
}

} // namespace
