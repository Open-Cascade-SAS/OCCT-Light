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
#include <occtl/occtl_io_gltf.h>
#include <occtl/occtl_mesh.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <string>

namespace
{

class IoGltfTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    myGltfPath = std::filesystem::temp_directory_path() / "occtl-iogltf-test.gltf";
    myGlbPath  = std::filesystem::temp_directory_path() / "occtl-iogltf-test.glb";
    std::filesystem::remove(myGltfPath);
    std::filesystem::remove(myGlbPath);
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
    std::filesystem::remove(myGltfPath, aEc);
    std::filesystem::remove(myGlbPath, aEc);
  }

  occtl_graph_t*        myGraph = nullptr;
  occtl_node_id_t       myRoot{};
  std::filesystem::path myGltfPath;
  std::filesystem::path myGlbPath;
};

TEST_F(IoGltfTest, ReadOptionsInit_HasV1Defaults)
{
  occtl_io_gltf_read_options_t aOpts{};
  occtl_io_gltf_read_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_GLTF_READ_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.load_all_scenes, 0);
  EXPECT_EQ(aOpts.skip_empty_nodes, 1);
  EXPECT_EQ(aOpts.use_mesh_name_as_fallback, 1);
  EXPECT_EQ(aOpts.apply_scale, 1);
  EXPECT_EQ(aOpts.parallel, 0);
  EXPECT_EQ(aOpts.single_precision, 1);
  EXPECT_EQ(aOpts.fill_incomplete, 1);
  EXPECT_EQ(aOpts.memory_limit_mib, -1);
}

TEST_F(IoGltfTest, WriteOptionsInit_HasV1Defaults)
{
  occtl_io_gltf_write_options_t aOpts{};
  occtl_io_gltf_write_options_init(&aOpts);
  EXPECT_EQ(aOpts.struct_version, OCCTL_IO_GLTF_WRITE_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.transform_format, OCCTL_IO_GLTF_TRANSFORM_COMPACT);
  EXPECT_EQ(aOpts.force_uv_export, 0);
  EXPECT_EQ(aOpts.embed_textures_in_glb, 1);
  EXPECT_EQ(aOpts.merge_faces, 0);
  EXPECT_EQ(aOpts.split_indices_16, 0);
  EXPECT_EQ(aOpts.parallel, 0);
}

TEST_F(IoGltfTest, RoundTrip_GltfBox)
{
  ASSERT_EQ(occtl_io_gltf_write(myGraph, myRoot, myGltfPath.string().c_str(), nullptr), OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myGltfPath));

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_gltf_read(myGltfPath.string().c_str(), &aBack, &aBackRoot, nullptr), OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{0});

  occtl_graph_free(aBack);
}

TEST_F(IoGltfTest, RoundTrip_GlbBox)
{
  occtl_io_gltf_write_options_t aWriteOpts;
  occtl_io_gltf_write_options_init(&aWriteOpts);
  aWriteOpts.transform_format = OCCTL_IO_GLTF_TRANSFORM_TRS;

  ASSERT_EQ(occtl_io_gltf_write(myGraph, myRoot, myGlbPath.string().c_str(), &aWriteOpts),
            OCCTL_OK);
  ASSERT_TRUE(std::filesystem::exists(myGlbPath));

  occtl_io_gltf_read_options_t aReadOpts;
  occtl_io_gltf_read_options_init(&aReadOpts);
  aReadOpts.load_all_scenes = 1;

  occtl_graph_t*  aBack     = nullptr;
  occtl_node_id_t aBackRoot = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_io_gltf_read(myGlbPath.string().c_str(), &aBack, &aBackRoot, &aReadOpts),
            OCCTL_OK);
  ASSERT_NE(aBack, nullptr);
  EXPECT_NE(aBackRoot.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, aBack), size_t{0});

  occtl_graph_free(aBack);
}

TEST_F(IoGltfTest, Read_NullPath_ReturnsInvalidArgument)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_gltf_read(nullptr, &aG, &aR, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoGltfTest, Read_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_gltf_read_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_graph_t*  aG   = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_gltf_read(myGltfPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoGltfTest, Read_InvalidOptionsFields_ReturnsInvalidArgument)
{
  occtl_io_gltf_read_options_t aOpts;
  occtl_io_gltf_read_options_init(&aOpts);
  aOpts.p_next       = &aOpts;
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_gltf_read(myGltfPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_gltf_read_options_init(&aOpts);
  aOpts.fill_incomplete = 2;
  EXPECT_EQ(occtl_io_gltf_read(myGltfPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_gltf_read_options_init(&aOpts);
  aOpts.memory_limit_mib = -2;
  EXPECT_EQ(occtl_io_gltf_read(myGltfPath.string().c_str(), &aG, &aR, &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoGltfTest, Read_MissingFile_ReturnsIoError)
{
  occtl_graph_t*  aG = nullptr;
  occtl_node_id_t aR{};
  EXPECT_EQ(occtl_io_gltf_read("/tmp/this-file-does-not-exist-occtl.gltf", &aG, &aR, nullptr),
            OCCTL_IO_ERROR);
  EXPECT_EQ(aG, nullptr);
}

TEST_F(IoGltfTest, ErrorAfterFailure_HasMessage)
{
  occtl_io_gltf_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  occtl_io_gltf_write(myGraph, myRoot, myGltfPath.string().c_str(), &aOpts);
  const occtl_error_t* aErr = occtl_error_last();
  ASSERT_NE(aErr, nullptr);
  EXPECT_NE(aErr->message, nullptr);
  EXPECT_GT(std::strlen(aErr->message), 0u);
}

TEST_F(IoGltfTest, Write_NullGraph_ReturnsInvalidArgument)
{
  EXPECT_EQ(occtl_io_gltf_write(nullptr, myRoot, myGltfPath.string().c_str(), nullptr),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoGltfTest, Write_InvalidRoot_ReturnsNotFound)
{
  occtl_node_id_t aBad{};
  aBad.bits = 0xdeadbeefdeadbeefULL;
  EXPECT_EQ(occtl_io_gltf_write(myGraph, aBad, myGltfPath.string().c_str(), nullptr),
            OCCTL_NOT_FOUND);
}

TEST_F(IoGltfTest, Write_BadOptsVersion_ReturnsVersionMismatch)
{
  occtl_io_gltf_write_options_t aOpts{};
  aOpts.struct_version = 999u;
  EXPECT_EQ(occtl_io_gltf_write(myGraph, myRoot, myGltfPath.string().c_str(), &aOpts),
            OCCTL_VERSION_MISMATCH);
}

TEST_F(IoGltfTest, Write_InvalidOptionsFields_ReturnsInvalidArgument)
{
  occtl_io_gltf_write_options_t aOpts;
  occtl_io_gltf_write_options_init(&aOpts);
  aOpts.p_next = &aOpts;
  EXPECT_EQ(occtl_io_gltf_write(myGraph, myRoot, myGltfPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_gltf_write_options_init(&aOpts);
  aOpts.embed_textures_in_glb = 2;
  EXPECT_EQ(occtl_io_gltf_write(myGraph, myRoot, myGltfPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);

  occtl_io_gltf_write_options_init(&aOpts);
  aOpts.parallel = 1;
  EXPECT_EQ(occtl_io_gltf_write(myGraph, myRoot, myGltfPath.string().c_str(), &aOpts),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(IoGltfTest, Write_BadTransformFormat_ReturnsOutOfRange)
{
  occtl_io_gltf_write_options_t aOpts;
  occtl_io_gltf_write_options_init(&aOpts);
  aOpts.transform_format = static_cast<occtl_io_gltf_transform_format_t>(0x1234);
  EXPECT_EQ(occtl_io_gltf_write(myGraph, myRoot, myGltfPath.string().c_str(), &aOpts),
            OCCTL_OUT_OF_RANGE);
}

} // namespace
