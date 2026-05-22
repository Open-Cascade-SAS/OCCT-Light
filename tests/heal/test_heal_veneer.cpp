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

#include "test_heal_helpers.hxx"

#include <occtl-hpp/heal.hpp>
#include <occtl-hpp/topo.hpp>

#include <gtest/gtest.h>

#include <memory>

namespace
{

class HealVeneerTest : public ::testing::Test
{
protected:
  void SetUp() override { myGraphPtr = std::make_unique<occtl::Graph>(); }

  void TearDown() override { myGraphPtr.reset(); }

  occtl::NodeId makeBox(const double theX,
                        const double theY,
                        const double theZ,
                        const double theDx,
                        const double theDy,
                        const double theDz)
  {
    occtl_node_id_t       aRaw   = OCCTL_NODE_ID_INVALID;
    occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
    anInfo.placement.location    = {theX, theY, theZ};
    anInfo.dx                    = theDx;
    anInfo.dy                    = theDy;
    anInfo.dz                    = theDz;
    EXPECT_EQ(occtl_prim_make_box(myGraphPtr->get(), &anInfo, &aRaw), OCCTL_OK);
    return occtl::NodeId(aRaw);
  }

  std::unique_ptr<occtl::Graph> myGraphPtr;
};

TEST_F(HealVeneerTest, DefaultOptions_Succeeds)
{
  occtl::NodeId aNode = makeBox(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

  EXPECT_NO_THROW(occtl::heal::heal_shape(*myGraphPtr, aNode));
}

TEST_F(HealVeneerTest, FullMode_Succeeds)
{
  occtl::NodeId aNode = makeBox(0.0, 0.0, 0.0, 10.0, 20.0, 30.0);

  occtl::heal::Options anOpts{};
  anOpts.set_mode(OCCTL_HEAL_MODE_FULL).set_tolerance(1e-5);

  EXPECT_NO_THROW(occtl::heal::heal_shape(*myGraphPtr, aNode, anOpts));
}

TEST_F(HealVeneerTest, MovedFromGraph_ThrowsError)
{
  occtl::NodeId aNode = makeBox(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

  // Move the graph away; heal on the now-empty moved-from graph.
  occtl::Graph aMoved(std::move(*myGraphPtr));
  EXPECT_THROW(occtl::heal::heal_shape(*myGraphPtr, aNode), occtl::Error);
}

TEST_F(HealVeneerTest, InvalidNode_ThrowsError)
{
  occtl::NodeId aBad(occtl_node_id_t{0xdeadbeefull});

  EXPECT_THROW(occtl::heal::heal_shape(*myGraphPtr, aBad), occtl::Error);
}

TEST_F(HealVeneerTest, ChainedOptions_Succeeds)
{
  occtl::NodeId aNode = makeBox(0.0, 0.0, 0.0, 2.0, 3.0, 4.0);

  occtl::heal::Options anOpts{};
  anOpts.set_mode(OCCTL_HEAL_MODE_STANDARD)
    .set_fix_same_parameter(true)
    .set_fix_small_edges(true)
    .set_fix_face_orient(true)
    .set_fix_missing_seam(false);

  EXPECT_NO_THROW(occtl::heal::heal_shape(*myGraphPtr, aNode, anOpts));
}

TEST_F(HealVeneerTest, UnifySameDomainOptions_ReturnsNewRoot)
{
  occtl::NodeId aNode = makeBox(0.0, 0.0, 0.0, 2.0, 3.0, 4.0);

  occtl::heal::UnifySameDomainOptions anOpts{};
  anOpts.set_unify_edges(true).set_unify_faces(true).set_safe_input(true);

  const occtl::NodeId aRoot = occtl::heal::unify_same_domain(*myGraphPtr, aNode, anOpts);
  EXPECT_NE(aRoot.get().bits, OCCTL_NODE_ID_INVALID.bits);
}

} // namespace
