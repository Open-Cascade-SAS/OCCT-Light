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

#include "test_prim_helpers.hxx"

#include <occtl/occtl_curves.h>

#include <limits>

namespace
{

class PrimLoftTest : public ::testing::Test
{
protected:
  // Build a closed square wire at z = theZ with edge length aLen, using real
  // line curves so the resulting wire has a 3D parametrisation.
  occtl_node_id_t buildSquareWire(const double theZ, const double theLen)
  {
    const occtl_point3_t aP[4] = {
      {0.0, 0.0, theZ},
      {theLen, 0.0, theZ},
      {theLen, theLen, theZ},
      {0.0, theLen, theZ},
    };

    occtl_node_id_t aV[4];
    for (int anI = 0; anI < 4; ++anI)
    {
      occtl_topo_make_vertex_info_t aVi = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
      aVi.tolerance                     = 1e-6;
      aVi.point                         = aP[anI];
      EXPECT_EQ(occtl_topo_make_vertex(myGraph, &aVi, &aV[anI]), OCCTL_OK);
    }

    occtl_node_id_t aE[4];
    for (int anI = 0; anI < 4; ++anI)
    {
      const occtl_point3_t& aStart = aP[anI];
      const occtl_point3_t& anEnd  = aP[(anI + 1) % 4];

      const double      dx    = anEnd.x - aStart.x;
      const double      dy    = anEnd.y - aStart.y;
      const double      dz    = anEnd.z - aStart.z;
      const double      aL    = theLen;
      occtl_geom_line_t aLine = {aStart, {dx / aL, dy / aL, dz / aL}};

      occtl_rep_id_t aCurve = {};
      EXPECT_EQ(occtl_curve_create_line(myGraph, aLine, &aCurve), OCCTL_OK);

      occtl_topo_make_edge_info_t aEi = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
      aEi.start_vertex                = aV[anI];
      aEi.end_vertex                  = aV[(anI + 1) % 4];
      aEi.curve                       = aCurve;
      aEi.first                       = 0.0;
      aEi.last                        = aL;
      aEi.tolerance                   = 1e-6;
      EXPECT_EQ(occtl_topo_make_edge(myGraph, &aEi, &aE[anI]), OCCTL_OK);
    }

    occtl_oriented_node_t aOe[4] = {
      {aE[0], OCCTL_ORIENTATION_FORWARD},
      {aE[1], OCCTL_ORIENTATION_FORWARD},
      {aE[2], OCCTL_ORIENTATION_FORWARD},
      {aE[3], OCCTL_ORIENTATION_FORWARD},
    };
    occtl_topo_make_wire_info_t aWi = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
    aWi.edges                       = aOe;
    aWi.edge_count                  = 4;
    occtl_node_id_t aWire           = OCCTL_NODE_ID_INVALID;
    EXPECT_EQ(occtl_topo_make_wire(myGraph, &aWi, &aWire), OCCTL_OK);
    return aWire;
  }

  void SetUp() override { ASSERT_EQ(occtl_graph_create(&myGraph), OCCTL_OK); }

  void TearDown() override
  {
    occtl_graph_free(myGraph);
    myGraph = nullptr;
  }

  occtl_graph_t* myGraph = nullptr;
};

TEST_F(PrimLoftTest, MakeLoft_TwoSquares_CreatesShape)
{
  const occtl_node_id_t aWires[2] = {
    buildSquareWire(0.0, 1.0),
    buildSquareWire(5.0, 1.0),
  };

  occtl_prim_loft_info_t anInfo = OCCTL_PRIM_LOFT_INFO_INIT;
  anInfo.sections               = aWires;
  anInfo.section_count          = 2;
  anInfo.is_solid               = 1;
  anInfo.ruled                  = 1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_loft(myGraph, &anInfo, &aShape), OCCTL_OK);
  EXPECT_NE(aShape.bits, 0u);
}

TEST_F(PrimLoftTest, MakeLoft_NullPointers_ReturnsInvalidArgument)
{
  occtl_prim_loft_info_t anInfo = OCCTL_PRIM_LOFT_INFO_INIT;
  occtl_node_id_t        aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_loft(nullptr, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_loft(myGraph, nullptr, &aShape), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_loft(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimLoftTest, MakeLoft_VersionMismatch_Rejected)
{
  occtl_prim_loft_info_t anInfo = OCCTL_PRIM_LOFT_INFO_INIT;
  anInfo.struct_version         = 0u;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_loft(myGraph, &anInfo, &aShape), OCCTL_VERSION_MISMATCH);
}

TEST_F(PrimLoftTest, MakeLoft_TooFewSections_InvalidArgument)
{
  const occtl_node_id_t aSingle[1] = {buildSquareWire(0.0, 1.0)};

  occtl_prim_loft_info_t anInfo = OCCTL_PRIM_LOFT_INFO_INIT;
  anInfo.sections               = aSingle;
  anInfo.section_count          = 1;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_loft(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimLoftTest, MakeLoft_NonWireSection_WrongKind)
{
  occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
  aBox.dx                    = 1.0;
  aBox.dy                    = 1.0;
  aBox.dz                    = 1.0;
  occtl_node_id_t aSolid     = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_box(myGraph, &aBox, &aSolid), OCCTL_OK);

  occtl_node_iter_t* anIter = nullptr;
  ASSERT_EQ(occtl_graph_face_iter_create(myGraph, &anIter), OCCTL_OK);
  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_node_iter_next(anIter, &aFace), OCCTL_OK);
  occtl_node_iter_free(anIter);

  const occtl_node_id_t  aSecs[2] = {aFace, aFace};
  occtl_prim_loft_info_t anInfo   = OCCTL_PRIM_LOFT_INFO_INIT;
  anInfo.sections                 = aSecs;
  anInfo.section_count            = 2;

  occtl_node_id_t aShape = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_loft(myGraph, &anInfo, &aShape), OCCTL_WRONG_KIND);
}

TEST_F(PrimLoftTest, MakeLoft_InvalidInfoFields_ReturnInvalidArgument)
{
  const occtl_node_id_t aWires[2] = {
    buildSquareWire(0.0, 1.0),
    buildSquareWire(5.0, 1.0),
  };

  occtl_prim_loft_info_t anInfo = OCCTL_PRIM_LOFT_INFO_INIT;
  anInfo.sections               = aWires;
  anInfo.section_count          = 2;
  occtl_node_id_t aShape        = OCCTL_NODE_ID_INVALID;

  int aTag      = 0;
  anInfo.p_next = &aTag;
  EXPECT_EQ(occtl_prim_make_loft(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo               = OCCTL_PRIM_LOFT_INFO_INIT;
  anInfo.sections      = aWires;
  anInfo.section_count = 2;
  anInfo.is_solid      = 2;
  EXPECT_EQ(occtl_prim_make_loft(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo               = OCCTL_PRIM_LOFT_INFO_INIT;
  anInfo.sections      = aWires;
  anInfo.section_count = 2;
  anInfo.ruled         = -1;
  EXPECT_EQ(occtl_prim_make_loft(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);

  anInfo               = OCCTL_PRIM_LOFT_INFO_INIT;
  anInfo.sections      = aWires;
  anInfo.section_count = 2;
  anInfo.pres3d        = std::numeric_limits<double>::infinity();
  EXPECT_EQ(occtl_prim_make_loft(myGraph, &anInfo, &aShape), OCCTL_INVALID_ARGUMENT);
}

} // namespace
