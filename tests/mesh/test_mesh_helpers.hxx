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

#ifndef OCCTL_TEST_MESH_HELPERS_HXX
#define OCCTL_TEST_MESH_HELPERS_HXX

#include <occtl/occtl_core.h>
#include <occtl/occtl_mesh.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_topo.h>

#include <gtest/gtest.h>

namespace mesh_test
{

inline occtl_node_id_t makeBox(occtl_graph_t* const theGraph,
                               const double         theDx,
                               const double         theDy,
                               const double         theDz)
{
  occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.dx                    = theDx;
  anInfo.dy                    = theDy;
  anInfo.dz                    = theDz;
  occtl_node_id_t aSolid       = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_box(theGraph, &anInfo, &aSolid), OCCTL_OK);
  return aSolid;
}

inline occtl_node_id_t makeSphere(occtl_graph_t* const theGraph, const double theRadius)
{
  occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.radius                   = theRadius;
  occtl_node_id_t aSolid          = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_sphere(theGraph, &anInfo, &aSolid), OCCTL_OK);
  return aSolid;
}

inline occtl_node_id_t makeCylinder(occtl_graph_t* const theGraph,
                                    const double         theRadius,
                                    const double         theHeight)
{
  occtl_prim_cylinder_info_t anInfo = OCCTL_PRIM_CYLINDER_INFO_INIT;
  anInfo.radius                     = theRadius;
  anInfo.height                     = theHeight;
  occtl_node_id_t aSolid            = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_cylinder(theGraph, &anInfo, &aSolid), OCCTL_OK);
  return aSolid;
}

class MeshFixture : public ::testing::Test
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

} // namespace mesh_test

#endif // OCCTL_TEST_MESH_HELPERS_HXX
