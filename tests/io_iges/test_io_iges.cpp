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

#include <occtl-hpp/io_iges.hpp>
#include <occtl/occtl_core.h>
#include <occtl/occtl_io_iges.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace
{

class IoIgesTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    myPath = std::filesystem::temp_directory_path() / "occtl-ioiges-test.igs";
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

TEST_F(IoIgesTest, ReadOptionsInit_HasV1)
{
  occtl_io_iges_read_options_t aOpts{};
  occtl_io_iges_read_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_IGES_READ_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.read_color, 1);
  EXPECT_EQ(aOpts.read_name, 1);
}

TEST_F(IoIgesTest, WriteOptionsInit_HasV1)
{
  occtl_io_iges_write_options_t aOpts{};
  occtl_io_iges_write_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_IGES_WRITE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.write_brep, 0);
}

TEST_F(IoIgesTest, RoundTrip_BoxPreservesCounts)
{
  occtl_io_iges_write_options_t aOpts;
  occtl_io_iges_write_options_init(&aOpts);
  aOpts.write_brep = 1;
  ASSERT_EQ(occtl_io_iges_write(myGraph, myRoot, myPath.string().c_str(), &aOpts), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myPath));

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_iges_read(myPath.string().c_str(), &aBack, &aBackRoot, nullptr), OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);

  EXPECT_EQ(occtl_graph_count_value(occtl_graph_solid_count, aBack), size_t{1});
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{6});

  occtl_graph_free(aBack);
}

TEST_F(IoIgesTest, Read_NullPath_ReturnsInvalidArgument)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_iges_read(nullptr, &aG, &aR, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoIgesTest, Read_MissingFile_ReturnsIoError)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_iges_read("/tmp/this-file-does-not-exist-occtl.igs", &aG, &aR, nullptr),
            OCCTL_IO_ERROR);
}

TEST_F(IoIgesTest, Write_NullGraph_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_io_iges_write(nullptr, myRoot, myPath.string().c_str(), nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoIgesTest, Write_InvalidRoot_ReturnsNotFound)
{
  occtl_node_id_t aBad{};
  aBad.bits = 0xdeadbeefdeadbeefULL;
  EXPECT_EQ(occtl_io_iges_write(myGraph, aBad, myPath.string().c_str(), nullptr), OCCTL_NOT_FOUND);
}

TEST_F(IoIgesTest, Read_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_iges_read_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_graph_t*  aG   = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_iges_read("/tmp/any.igs", &aG, &aR, &aOpts), OCCTL_VERSION_MISMATCH);
}

TEST_F(IoIgesTest, Read_BadFlag_ReturnsInvalidArgument)
{
  occtl_io_iges_read_options_t aOpts;
  occtl_io_iges_read_options_init(&aOpts);
  aOpts.read_color   = 7;
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_iges_read("/tmp/any.igs", &aG, &aR, &aOpts), OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoIgesTest, Write_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_iges_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  EXPECT_EQ(occtl_io_iges_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_VERSION_MISMATCH);
}

TEST_F(IoIgesTest, Write_BadFlag_ReturnsInvalidArgument)
{
  occtl_io_iges_write_options_t aOpts;
  occtl_io_iges_write_options_init(&aOpts);
  aOpts.write_brep = -2;
  EXPECT_EQ(occtl_io_iges_write(myGraph, myRoot, myPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoIgesTest, ErrorAfterFailure_HasMessage)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  occtl_io_iges_read("/tmp/missing.igs", &aG, &aR, nullptr);
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_NE(aErr->message, nullptr);
  EXPECT_GT(std::strlen(aErr->message), 0u);
}

TEST_F(IoIgesTest, Write_BRepMode_WritesFile)
{
  occtl_io_iges_write_options_t aOpts;
  occtl_io_iges_write_options_init(&aOpts);
  aOpts.write_brep = 1;
  ASSERT_EQ(occtl_io_iges_write(myGraph, myRoot, myPath.string().c_str(), &aOpts), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myPath));

  occtl_graph_t*  aBack = nullptr;
  occtl_node_id_t aBackR{};
  ASSERT_EQ(occtl_io_iges_read(myPath.string().c_str(), &aBack, &aBackR, nullptr), OCCTL_OK);
  EXPECT_GE(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{1});
  occtl_graph_free(aBack);
}

} // namespace
