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

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/text.hpp>
#include <occtl-hpp/topo.hpp>

#include <occtl/occtl_core.h>
#include <occtl/occtl_text.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <string>

namespace
{

class TextFacesTest : public ::testing::Test
{
protected:
  void SetUp() override { ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK); }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t* myGraph = nullptr;
};

// Helper: try to resolve a default sans-serif family via the system font
// registry.  Used by tests that need a real font but should not hard-fail
// on machines that lack a specific family name.
static bool tryFindAnyFont(std::string& theNameOut)
{
  static const char* const aCandidates[] =
    {"Arial", "Helvetica", "DejaVu Sans", "Liberation Sans", "Verdana", "FreeSans", "Noto Sans"};
  for (const char* aCand : aCandidates)
  {
    occtl_text_info_t aProbe = OCCTL_TEXT_INFO_INIT;
    aProbe.utf8_text         = "A";
    aProbe.font_family       = aCand;
    aProbe.height            = 1.0;

    occtl_graph_t* aProbeGraph = nullptr;
    if (occtl_graph_create(&aProbeGraph) != OCCTL_OK)
    {
      continue;
    }
    occtl_node_id_t      aProbeId = OCCTL_NODE_ID_INVALID;
    const occtl_status_t aSt      = occtl_text_make_faces(aProbeGraph, &aProbe, &aProbeId);
    occtl_graph_free(aProbeGraph);
    if (aSt == OCCTL_OK)
    {
      theNameOut = aCand;
      return true;
    }
  }
  return false;
}

TEST_F(TextFacesTest, MakeTextFaces_NullPointers_ReturnsInvalidArgument)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = "Arial";
  anInfo.height            = 1.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_text_make_faces(nullptr, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_text_make_faces(myGraph, nullptr, &aNode), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_text_make_faces(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(TextFacesTest, MakeTextFaces_VersionMismatch_Rejected)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.struct_version    = 0u;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = "Arial";
  anInfo.height            = 1.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aNode), OCCTL_VERSION_MISMATCH);
  EXPECT_EQ(aNode.bits, 0u);
}

TEST_F(TextFacesTest, MakeTextFaces_EmptyText_ReturnsInvalidArgument)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "";
  anInfo.font_family       = "Arial";
  anInfo.height            = 1.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aNode.bits, 0u);
}

TEST_F(TextFacesTest, MakeTextFaces_NonPositiveHeight_ReturnsInvalidArgument)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = "Arial";
  anInfo.height            = 0.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);

  anInfo.height = -1.0;
  EXPECT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);
}

TEST_F(TextFacesTest, MakeTextFaces_NoFontSelector_ReturnsInvalidArgument)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.height            = 1.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);
}

TEST_F(TextFacesTest, MakeTextFaces_BothFontSelectors_ReturnsInvalidArgument)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = "Arial";
  anInfo.font_path         = "/nonexistent/path/to/font.ttf";
  anInfo.height            = 1.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(aNode.bits, 0u);
}

TEST_F(TextFacesTest, MakeTextFaces_BadFontPath_ReturnsGeometryInvalid)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_path         = "/nonexistent/path/to/font.ttf";
  anInfo.height            = 1.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aNode), OCCTL_GEOMETRY_INVALID);
  EXPECT_EQ(aNode.bits, 0u);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

TEST_F(TextFacesTest, MakeTextFaces_AnyFont_CreatesCompound)
{
  std::string aFontName;
  if (!tryFindAnyFont(aFontName))
  {
    GTEST_SKIP() << "No common sans-serif font installed on this host";
  }

  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = aFontName.c_str();
  anInfo.height            = 1.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aNode), OCCTL_OK);
  EXPECT_NE(aNode.bits, 0u);

  occtl_node_kind_t aKind = OCCTL_KIND_COMPOUND;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aNode, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_COMPOUND);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, myGraph), 0u);
}

TEST_F(TextFacesTest, MakeTextFaces_Multiline_ProducesMoreFacesThanSingleLine)
{
  std::string aFontName;
  if (!tryFindAnyFont(aFontName))
  {
    GTEST_SKIP() << "No common sans-serif font installed on this host";
  }

  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.font_family       = aFontName.c_str();
  anInfo.height            = 1.0;

  occtl_graph_t* aSingle = nullptr;
  ASSERT_EQ(occtl_graph_create(&aSingle), OCCTL_OK);
  occtl_node_id_t aSingleId = OCCTL_NODE_ID_INVALID;
  anInfo.utf8_text          = "A";
  ASSERT_EQ(occtl_text_make_faces(aSingle, &anInfo, &aSingleId), OCCTL_OK);
  const std::size_t aSingleFaces = occtl_graph_count_value(occtl_graph_face_count, aSingle);
  occtl_graph_free(aSingle);

  occtl_node_id_t aMultiId = OCCTL_NODE_ID_INVALID;
  anInfo.utf8_text         = "A\nB";
  ASSERT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aMultiId), OCCTL_OK);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, myGraph), aSingleFaces);
}

TEST_F(TextFacesTest, TextMeasure_AnyFont_ReturnsPositiveMetrics)
{
  std::string aFontName;
  if (!tryFindAnyFont(aFontName))
  {
    GTEST_SKIP() << "No common sans-serif font installed on this host";
  }

  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = aFontName.c_str();
  anInfo.height            = 1.0;

  occtl_text_metrics_t aMetrics = OCCTL_TEXT_METRICS_INIT;
  ASSERT_EQ(occtl_text_measure(&anInfo, &aMetrics), OCCTL_OK);
  EXPECT_GT(aMetrics.width, 0.0);
  EXPECT_GT(aMetrics.height, 0.0);
  EXPECT_GT(aMetrics.line_spacing, 0.0);
  EXPECT_GT(aMetrics.max_symbol_width, 0.0);
  EXPECT_LE(aMetrics.left, aMetrics.right);
  EXPECT_LE(aMetrics.bottom, aMetrics.top);
}

TEST_F(TextFacesTest, TextMeasure_NullPointers_ReturnsInvalidArgument)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = "Arial";
  anInfo.height            = 1.0;

  occtl_text_metrics_t aMetrics = OCCTL_TEXT_METRICS_INIT;
  EXPECT_EQ(occtl_text_measure(nullptr, &aMetrics), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_text_measure(&anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(TextFacesTest, TextMeasure_MetricsVersionMismatch_Rejected)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = "Arial";
  anInfo.height            = 1.0;

  occtl_text_metrics_t aMetrics = OCCTL_TEXT_METRICS_INIT;
  aMetrics.struct_version       = 0u;
  EXPECT_EQ(occtl_text_measure(&anInfo, &aMetrics), OCCTL_VERSION_MISMATCH);
}

TEST_F(TextFacesTest, TextMeasure_WrappingOptions_ReducesWidthAndIncreasesHeight)
{
  std::string aFontName;
  if (!tryFindAnyFont(aFontName))
  {
    GTEST_SKIP() << "No common sans-serif font installed on this host";
  }

  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "wide text wide text wide text";
  anInfo.font_family       = aFontName.c_str();
  anInfo.height            = 1.0;

  occtl_text_metrics_t anUnwrapped = OCCTL_TEXT_METRICS_INIT;
  ASSERT_EQ(occtl_text_measure(&anInfo, &anUnwrapped), OCCTL_OK);

  occtl_text_layout_options_t aLayout = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
  aLayout.wrapping_width              = anUnwrapped.width * 0.45;
  anInfo.p_next                       = &aLayout;

  occtl_text_metrics_t aWrapped = OCCTL_TEXT_METRICS_INIT;
  ASSERT_EQ(occtl_text_measure(&anInfo, &aWrapped), OCCTL_OK);
  EXPECT_LT(aWrapped.width, anUnwrapped.width);
  EXPECT_GT(aWrapped.height, anUnwrapped.height);
}

TEST_F(TextFacesTest, MakeTextFaces_WrappingOptions_CreatesCompound)
{
  std::string aFontName;
  if (!tryFindAnyFont(aFontName))
  {
    GTEST_SKIP() << "No common sans-serif font installed on this host";
  }

  occtl_text_layout_options_t aLayout = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
  aLayout.wrapping_width              = 5.0;

  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "wide text wide text";
  anInfo.font_family       = aFontName.c_str();
  anInfo.height            = 1.0;
  anInfo.p_next            = &aLayout;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_text_make_faces(myGraph, &anInfo, &aNode), OCCTL_OK);
  EXPECT_NE(aNode.bits, 0u);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_face_count, myGraph), 0u);
}

TEST_F(TextFacesTest, TextLayoutOptions_InvalidValues_AreRejected)
{
  occtl_text_layout_options_t aLayout = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
  aLayout.wrapping_width              = -1.0;

  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = "Arial";
  anInfo.height            = 1.0;
  anInfo.p_next            = &aLayout;

  occtl_text_metrics_t aMetrics = OCCTL_TEXT_METRICS_INIT;
  EXPECT_EQ(occtl_text_measure(&anInfo, &aMetrics), OCCTL_INVALID_ARGUMENT);

  aLayout                = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
  aLayout.struct_version = 0u;
  EXPECT_EQ(occtl_text_measure(&anInfo, &aMetrics), OCCTL_VERSION_MISMATCH);

  aLayout               = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
  aLayout.word_wrapping = 2;
  EXPECT_EQ(occtl_text_measure(&anInfo, &aMetrics), OCCTL_INVALID_ARGUMENT);
}

TEST(TextLayoutOptionsTest, Init_HasExpectedDefaults)
{
  occtl_text_layout_options_t anOptions{};
  occtl_text_layout_options_init(&anOptions);

  EXPECT_EQ(anOptions.struct_version, OCCTL_TEXT_LAYOUT_OPTIONS_VERSION_1);
  EXPECT_EQ(anOptions.p_next, nullptr);
  EXPECT_DOUBLE_EQ(anOptions.wrapping_width, 0.0);
  EXPECT_EQ(anOptions.word_wrapping, 1);

  occtl_text_layout_options_init(nullptr);
}

TEST(TextMetricsTest, Init_HasExpectedDefaults)
{
  occtl_text_metrics_t aMetrics{};
  occtl_text_metrics_init(&aMetrics);

  EXPECT_EQ(aMetrics.struct_version, OCCTL_TEXT_METRICS_VERSION_1);
  EXPECT_EQ(aMetrics.p_next, nullptr);
  EXPECT_DOUBLE_EQ(aMetrics.width, 0.0);
  EXPECT_DOUBLE_EQ(aMetrics.height, 0.0);

  occtl_text_metrics_init(nullptr);
}

TEST_F(TextFacesTest, MakeTextWires_AnyFont_CreatesCompoundOfWires)
{
  std::string aFontName;
  if (!tryFindAnyFont(aFontName))
  {
    GTEST_SKIP() << "No common sans-serif font installed on this host";
  }

  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = aFontName.c_str();
  anInfo.height            = 1.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_text_make_wires(myGraph, &anInfo, &aNode), OCCTL_OK);
  EXPECT_NE(aNode.bits, 0u);

  occtl_node_kind_t aKind = OCCTL_KIND_COMPOUND;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aNode, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_COMPOUND);
  EXPECT_GT(occtl_graph_count_value(occtl_graph_wire_count, myGraph), 0u);
  EXPECT_EQ(occtl_graph_count_value(occtl_graph_face_count, myGraph), 0u);
}

TEST_F(TextFacesTest, MakeTextWires_NullPointers_ReturnsInvalidArgument)
{
  occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.utf8_text         = "Hi";
  anInfo.font_family       = "Arial";
  anInfo.height            = 1.0;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_text_make_wires(nullptr, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_text_make_wires(myGraph, nullptr, &aNode), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_text_make_wires(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST(TextFacesVeneer, MakeFaces_BadFontPath_Throws)
{
  occtl::Graph aGraph;

  occtl::text::TextFacesOptions anOpts;
  anOpts.font_path = "/nonexistent/path/to/font.ttf";
  anOpts.height    = 1.0;

  EXPECT_THROW(occtl::text::make_faces(aGraph, "Hi", anOpts), occtl::Error);
}

TEST(TextFacesVeneer, MakeFaces_EmptyText_Throws)
{
  occtl::Graph aGraph;

  occtl::text::TextFacesOptions anOpts;
  anOpts.font_family = "Arial";
  anOpts.height      = 1.0;

  EXPECT_THROW(occtl::text::make_faces(aGraph, "", anOpts), occtl::Error);
}

TEST(TextFacesVeneer, Measure_AnyFont_ReturnsMetrics)
{
  std::string aFontName;
  if (!tryFindAnyFont(aFontName))
  {
    GTEST_SKIP() << "No common sans-serif font installed on this host";
  }

  occtl::text::TextFacesOptions anOpts;
  anOpts.font_family = aFontName;
  anOpts.height      = 1.0;

  const occtl::text::TextMetrics aMetrics = occtl::text::measure("Hi", anOpts);
  EXPECT_GT(aMetrics.width, 0.0);
  EXPECT_GT(aMetrics.height, 0.0);

  anOpts.wrapping_width                   = aMetrics.width * 0.6;
  const occtl::text::TextMetrics aWrapped = occtl::text::measure("Hi Hi Hi Hi", anOpts);
  EXPECT_GT(aWrapped.height, aMetrics.height);
}

TEST(TextFacesVeneer, MakeWires_AnyFont_CreatesWires)
{
  std::string aFontName;
  if (!tryFindAnyFont(aFontName))
  {
    GTEST_SKIP() << "No common sans-serif font installed on this host";
  }

  occtl::Graph aGraph;

  occtl::text::TextFacesOptions anOpts;
  anOpts.font_family = aFontName;
  anOpts.height      = 1.0;

  const occtl::NodeId aNode = occtl::text::make_wires(aGraph, "Hi", anOpts);
  EXPECT_NE(aNode.get().bits, 0u);
  EXPECT_GT(aGraph.wire_count(), 0u);
  EXPECT_EQ(aGraph.face_count(), 0u);
}

} // namespace
