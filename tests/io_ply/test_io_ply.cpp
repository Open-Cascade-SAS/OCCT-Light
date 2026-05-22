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

#include <occtl-hpp/io_ply.hpp>
#include <occtl/occtl_core.h>
#include <occtl/occtl_io_ply.h>
#include <occtl/occtl_mesh.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace
{

class IoPlyTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    myPlyPath = std::filesystem::temp_directory_path() / "occtl-ioply-test.ply";
    std::filesystem::remove(myPlyPath);
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
    std::filesystem::remove(myPlyPath, aEc);
  }

  occtl_graph_t*        myGraph = nullptr;
  occtl_node_id_t       myRoot{};
  std::filesystem::path myPlyPath;
};

TEST_F(IoPlyTest, WriteOptionsInit_HasV1Defaults)
{
  occtl_io_ply_write_options_t aOpts{};
  occtl_io_ply_write_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_PLY_WRITE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.system_coordinate_system, OCCTL_IO_PLY_COORDINATE_SYSTEM_Z_UP);
  EXPECT_EQ(aOpts.file_coordinate_system, OCCTL_IO_PLY_COORDINATE_SYSTEM_Y_UP);
  EXPECT_EQ(aOpts.write_normals, 1);
  EXPECT_EQ(aOpts.write_colors, 1);
  EXPECT_EQ(aOpts.write_texcoords, 0);
  EXPECT_EQ(aOpts.write_part_id, 1);
  EXPECT_EQ(aOpts.write_face_id, 0);
  EXPECT_EQ(aOpts.comment, nullptr);
  EXPECT_EQ(aOpts.author, nullptr);
}

TEST_F(IoPlyTest, Write_Box)
{
  occtl_io_ply_write_options_t aOpts;
  occtl_io_ply_write_options_init(&aOpts);
  aOpts.comment = "occt-light test";
  aOpts.author  = "occt-light";

  ASSERT_EQ(occtl_io_ply_write(myGraph, myRoot, myPlyPath.string().c_str(), &aOpts), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myPlyPath));
  EXPECT_GT(std::filesystem::file_size(myPlyPath), uintmax_t{0});
}

TEST_F(IoPlyTest, Write_NullGraph_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_io_ply_write(nullptr, myRoot, myPlyPath.string().c_str(), nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoPlyTest, Write_InvalidRoot_ReturnsNotFound)
{
  occtl_node_id_t aBad{};
  aBad.bits = 0xdeadbeefdeadbeefULL;
  EXPECT_EQ(occtl_io_ply_write(myGraph, aBad, myPlyPath.string().c_str(), nullptr),
            OCCTL_NOT_FOUND);
}

TEST_F(IoPlyTest, Write_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_ply_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  EXPECT_EQ(occtl_io_ply_write(myGraph, myRoot, myPlyPath.string().c_str(), &aOpts),
            OCCTL_VERSION_MISMATCH);
}

TEST_F(IoPlyTest, Write_BadCoordinateSystem_ReturnsOutOfRange)
{
  occtl_io_ply_write_options_t aOpts;
  occtl_io_ply_write_options_init(&aOpts);
  aOpts.file_coordinate_system = static_cast<occtl_io_ply_coordinate_system_t>(0x1234);
  EXPECT_EQ(occtl_io_ply_write(myGraph, myRoot, myPlyPath.string().c_str(), &aOpts),
            OCCTL_OUT_OF_RANGE);
}

TEST_F(IoPlyTest, Write_PartIdAndFaceId_ReturnsInvalidArgument)
{
  occtl_io_ply_write_options_t aOpts;
  occtl_io_ply_write_options_init(&aOpts);
  aOpts.write_face_id = 1;
  EXPECT_EQ(occtl_io_ply_write(myGraph, myRoot, myPlyPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoPlyTest, Write_InvalidOptionsFields_ReturnsInvalidArgument)
{
  occtl_io_ply_write_options_t aOpts;
  occtl_io_ply_write_options_init(&aOpts);
  aOpts.p_next = &aOpts;
  EXPECT_EQ(occtl_io_ply_write(myGraph, myRoot, myPlyPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_ply_write_options_init(&aOpts);
  aOpts.write_normals = 2;
  EXPECT_EQ(occtl_io_ply_write(myGraph, myRoot, myPlyPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoPlyTest, ErrorAfterFailure_HasMessage)
{
  occtl_io_ply_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_io_ply_write(myGraph, myRoot, myPlyPath.string().c_str(), &aOpts);
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_NE(aErr->message, nullptr);
  EXPECT_GT(std::strlen(aErr->message), 0u);
}

} // namespace
