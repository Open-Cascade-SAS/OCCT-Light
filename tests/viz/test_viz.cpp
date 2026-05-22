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

#include <occtl/occtl_prim.h>
#include <occtl/occtl_viz.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace
{

class VizTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK);
    occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
    aBox.dx                    = 10.0;
    aBox.dy                    = 20.0;
    aBox.dz                    = 30.0;
    ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &myRoot), OCCTL_OK);
  }

  void TearDown() override
  {
    occtl_viz_presentable_free(myPresentable);
    occtl_viz_view_free(myView);
    occtl_viz_viewer_free(myViewer);
    occtl_viz_driver_free(myDriver);
    occtl_graph_free(myGraph);
  }

  void CreateViz()
  {
    occtl_viz_driver_options_t aDriverOptions;
    occtl_viz_driver_options_init(&aDriverOptions);
    ASSERT_EQ(occtl_viz_driver_create(&aDriverOptions, &myDriver), OCCTL_OK);
    ASSERT_EQ(occtl_viz_viewer_create(myDriver, &myViewer), OCCTL_OK);
    occtl_viz_view_options_t aViewOptions;
    occtl_viz_view_options_init(&aViewOptions);
    aViewOptions.width               = 128;
    aViewOptions.height              = 96;
    aViewOptions.offscreen           = 1;
    const occtl_status_t aViewStatus = occtl_viz_view_create(myViewer, &aViewOptions, &myView);
    if (aViewStatus == OCCTL_UNSUPPORTED || aViewStatus == OCCTL_INTERNAL)
    {
      GTEST_SKIP() << "Offscreen OpenGL view creation is unsupported in this environment";
    }
    ASSERT_EQ(aViewStatus, OCCTL_OK);
    ASSERT_EQ(occtl_viz_presentable_create(myViewer, myGraph, myRoot, &myPresentable), OCCTL_OK);
  }

  occtl_graph_t*           myGraph = nullptr;
  occtl_node_id_t          myRoot{};
  occtl_viz_driver_t*      myDriver      = nullptr;
  occtl_viz_viewer_t*      myViewer      = nullptr;
  occtl_viz_view_t*        myView        = nullptr;
  occtl_viz_presentable_t* myPresentable = nullptr;
};

} // namespace

TEST_F(VizTest, OptionsInit_HasDefaults)
{
  occtl_viz_driver_options_t aDriverOptions{};
  occtl_viz_driver_options_init(&aDriverOptions);
  EXPECT_EQ(aDriverOptions.struct_version, OCCTL_VIZ_DRIVER_OPTIONS_VERSION_1);
  EXPECT_EQ(aDriverOptions.enable_vbo, 1);
  EXPECT_EQ(aDriverOptions.enable_vsync, 1);

  occtl_viz_view_options_t aViewOptions{};
  occtl_viz_view_options_init(&aViewOptions);
  EXPECT_EQ(aViewOptions.struct_version, OCCTL_VIZ_VIEW_OPTIONS_VERSION_1);
  EXPECT_EQ(aViewOptions.width, 640);
  EXPECT_EQ(aViewOptions.height, 480);
  EXPECT_EQ(aViewOptions.native_handle, nullptr);
  EXPECT_EQ(aViewOptions.offscreen, 1);
}

TEST_F(VizTest, Create_NullArguments_ReturnInvalidArgument)
{
  EXPECT_EQ(occtl_viz_driver_create(nullptr, nullptr), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_viz_viewer_create(nullptr, &myViewer), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_viz_view_create(nullptr, nullptr, &myView), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_viz_presentable_create(nullptr, myGraph, myRoot, &myPresentable),
            OCCTL_INVALID_ARGUMENT);
}

TEST_F(VizTest, Create_BadVersions_ReturnVersionMismatch)
{
  occtl_viz_driver_options_t aDriverOptions = OCCTL_VIZ_DRIVER_OPTIONS_INIT;
  aDriverOptions.struct_version             = 999u;
  EXPECT_EQ(occtl_viz_driver_create(&aDriverOptions, &myDriver), OCCTL_VERSION_MISMATCH);

  ASSERT_EQ(occtl_viz_driver_create(nullptr, &myDriver), OCCTL_OK);
  ASSERT_EQ(occtl_viz_viewer_create(myDriver, &myViewer), OCCTL_OK);
  occtl_viz_view_options_t aViewOptions = OCCTL_VIZ_VIEW_OPTIONS_INIT;
  aViewOptions.struct_version           = 999u;
  EXPECT_EQ(occtl_viz_view_create(myViewer, &aViewOptions, &myView), OCCTL_VERSION_MISMATCH);
}

TEST_F(VizTest, Create_PNextMustBeNull_ReturnInvalidArgument)
{
  occtl_viz_driver_options_t aDriverOptions = OCCTL_VIZ_DRIVER_OPTIONS_INIT;
  const uint32_t             aDummy         = 1u;
  aDriverOptions.p_next                     = &aDummy;
  EXPECT_EQ(occtl_viz_driver_create(&aDriverOptions, &myDriver), OCCTL_INVALID_ARGUMENT);

  ASSERT_EQ(occtl_viz_driver_create(nullptr, &myDriver), OCCTL_OK);
  ASSERT_EQ(occtl_viz_viewer_create(myDriver, &myViewer), OCCTL_OK);

  occtl_viz_view_options_t aViewOptions = OCCTL_VIZ_VIEW_OPTIONS_INIT;
  aViewOptions.p_next                   = &aDummy;
  EXPECT_EQ(occtl_viz_view_create(myViewer, &aViewOptions, &myView), OCCTL_INVALID_ARGUMENT);
}

TEST_F(VizTest, Create_OffscreenZeroWithoutNativeHandle_ReturnInvalidArgument)
{
  ASSERT_EQ(occtl_viz_driver_create(nullptr, &myDriver), OCCTL_OK);
  ASSERT_EQ(occtl_viz_viewer_create(myDriver, &myViewer), OCCTL_OK);

  occtl_viz_view_options_t aViewOptions = OCCTL_VIZ_VIEW_OPTIONS_INIT;
  aViewOptions.offscreen                = 0;
  aViewOptions.native_handle            = nullptr;
  EXPECT_EQ(occtl_viz_view_create(myViewer, &aViewOptions, &myView), OCCTL_INVALID_ARGUMENT);
}

TEST_F(VizTest, Presentable_DisplayModesAndSynchronize)
{
  CreateViz();
  if (myView == nullptr)
  {
    GTEST_SKIP() << "Offscreen OpenGL view creation is unsupported in this environment";
  }
  ASSERT_EQ(occtl_viz_presentable_set_display_mode(myPresentable, OCCTL_VIZ_DISPLAY_WIREFRAME),
            OCCTL_OK);
  ASSERT_EQ(occtl_viz_presentable_set_display_mode(myPresentable, OCCTL_VIZ_DISPLAY_SHADED),
            OCCTL_OK);
  ASSERT_EQ(
    occtl_viz_presentable_set_display_mode(myPresentable, OCCTL_VIZ_DISPLAY_SHADED_WITH_EDGES),
    OCCTL_OK);
  EXPECT_EQ(
    occtl_viz_presentable_set_display_mode(myPresentable, OCCTL_VIZ_DISPLAY_RESERVED_FUTURE),
    OCCTL_OUT_OF_RANGE);

  int32_t aChanged = 0;
  EXPECT_EQ(occtl_viz_presentable_synchronize(myPresentable, &aChanged), OCCTL_OK);
  EXPECT_EQ(occtl_viz_presentable_invalidate(myPresentable), OCCTL_OK);
}

TEST_F(VizTest, View_CameraResizeAndDisplay)
{
  CreateViz();
  if (myView == nullptr)
  {
    GTEST_SKIP() << "Offscreen OpenGL view creation is unsupported in this environment";
  }
  ASSERT_EQ(occtl_viz_view_display(myView, myPresentable), OCCTL_OK);
  EXPECT_EQ(occtl_viz_view_fit_all(myView), OCCTL_OK);
  EXPECT_EQ(occtl_viz_view_resize(myView, 160, 120), OCCTL_OK);
  EXPECT_EQ(occtl_viz_view_set_standard_view(myView, OCCTL_VIZ_VIEW_ISO), OCCTL_OK);

  occtl_viz_camera_t aCamera{};
  ASSERT_EQ(occtl_viz_view_get_camera(myView, &aCamera), OCCTL_OK);
  EXPECT_EQ(occtl_viz_view_set_camera(myView, &aCamera), OCCTL_OK);
}

TEST_F(VizTest, OffscreenReadPixels_SizingWorksAndRenderIsSupportedOrReported)
{
  CreateViz();
  if (myView == nullptr)
  {
    GTEST_SKIP() << "Offscreen OpenGL view creation is unsupported in this environment";
  }
  ASSERT_EQ(occtl_viz_view_display(myView, myPresentable), OCCTL_OK);
  ASSERT_EQ(occtl_viz_view_fit_all(myView), OCCTL_OK);

  size_t aCount = 0;
  ASSERT_EQ(occtl_viz_view_read_pixels_rgba(myView, nullptr, 0, &aCount), OCCTL_OK);
  ASSERT_EQ(aCount, size_t{128 * 96 * 4});
  std::vector<uint8_t> aPixels(aCount);
  const occtl_status_t aStatus =
    occtl_viz_view_read_pixels_rgba(myView, aPixels.data(), aPixels.size(), &aCount);
  if (aStatus == OCCTL_UNSUPPORTED)
  {
    GTEST_SKIP() << "Offscreen OpenGL rendering is unsupported in this environment";
  }
  ASSERT_EQ(aStatus, OCCTL_OK);
}
