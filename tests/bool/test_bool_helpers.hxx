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

#ifndef OCCTL_TEST_BOOL_HELPERS_HXX
#define OCCTL_TEST_BOOL_HELPERS_HXX

#include <occtl/occtl_bool.h>
#include <occtl/occtl_core.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

#include <cstddef>

namespace bool_test
{

inline std::size_t countOfKind(const occtl_graph_t* const theGraph, const occtl_node_kind_t theKind)
{
  switch (theKind)
  {
    case OCCTL_KIND_SOLID:
      return occtl_graph_count_value(occtl_graph_solid_count, theGraph);
    case OCCTL_KIND_SHELL:
      return occtl_graph_count_value(occtl_graph_shell_count, theGraph);
    case OCCTL_KIND_FACE:
      return occtl_graph_count_value(occtl_graph_face_count, theGraph);
    case OCCTL_KIND_WIRE:
      return occtl_graph_count_value(occtl_graph_wire_count, theGraph);
    case OCCTL_KIND_EDGE:
      return occtl_graph_count_value(occtl_graph_edge_count, theGraph);
    case OCCTL_KIND_VERTEX:
      return occtl_graph_count_value(occtl_graph_vertex_count, theGraph);
    case OCCTL_KIND_COMPOUND:
      return occtl_graph_count_value(occtl_graph_compound_count, theGraph);
    default:
      return 0;
  }
}

inline occtl_node_id_t makeBoxAt(occtl_graph_t* const theGraph,
                                 const double         theX,
                                 const double         theY,
                                 const double         theZ,
                                 const double         theDx,
                                 const double         theDy,
                                 const double         theDz)
{
  occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.placement.location    = {theX, theY, theZ};
  anInfo.dx                    = theDx;
  anInfo.dy                    = theDy;
  anInfo.dz                    = theDz;
  occtl_node_id_t aSolid       = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_box(theGraph, &anInfo, &aSolid), OCCTL_OK);
  return aSolid;
}

inline occtl_node_id_t makeSphereAt(occtl_graph_t* const theGraph,
                                    const double         theX,
                                    const double         theY,
                                    const double         theZ,
                                    const double         theRadius)
{
  occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.placement.location       = {theX, theY, theZ};
  anInfo.radius                   = theRadius;
  occtl_node_id_t aSolid          = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_sphere(theGraph, &anInfo, &aSolid), OCCTL_OK);
  return aSolid;
}

class BoolFixture : public ::testing::Test
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

} // namespace bool_test

#endif // OCCTL_TEST_BOOL_HELPERS_HXX
