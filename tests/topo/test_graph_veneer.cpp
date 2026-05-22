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

#include <occtl-hpp/prim.hpp>
#include <occtl-hpp/topo.hpp>
#include <occtl/occtl_core.h>
#include <occtl/occtl_curves.h>
#include <occtl/occtl_geom.h>
#include <occtl/occtl_mesh.h>
#include <occtl/occtl_prim.h>
#include <occtl/occtl_surfaces.h>
#include <occtl/occtl_topo.h>

#include "test_helpers_internal.hxx"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

class TopoVeneerFixture : public ::testing::Test
{
protected:
  void SetUp() override
  {
    myGraph.reset(new occtl::Graph());
    loadBox(myGraph->get());
  }

  void TearDown() override { myGraph.reset(); }

  std::unique_ptr<occtl::Graph> myGraph;
};

occtl_rep_id_t makeTrimmedLine(occtl_graph_t* const      theGraph,
                               const occtl_point3_t&     theOrigin,
                               const occtl_direction3_t& theDirection,
                               const double              theFirst,
                               const double              theLast)
{
  const occtl_geom_line_t aLine  = {theOrigin, theDirection};
  occtl_rep_id_t          aBasis = {};
  EXPECT_EQ(::occtl_curve_create_line(theGraph, aLine, &aBasis), OCCTL_OK);

  occtl_curve_trimmed_create_info_t aTrimInfo = OCCTL_CURVE_TRIMMED_CREATE_INFO_INIT;
  aTrimInfo.basis                             = aBasis;
  aTrimInfo.u_first                           = theFirst;
  aTrimInfo.u_last                            = theLast;

  occtl_rep_id_t aTrimmed = {};
  EXPECT_EQ(::occtl_curve_create_trimmed(theGraph, &aTrimInfo, &aTrimmed), OCCTL_OK);
  return aTrimmed;
}

TEST_F(TopoVeneerFixture, Batch_GoesOutOfScopeWithoutCommit_DoesNotTerminate)
{
  // Open a batch, let it go out of scope without commit() — the destructor
  // must silently call occtl_batch_abort without throwing.  Survival of
  // this test proves the destructor does not call std::terminate.
  {
    occtl::Batch aBatch = myGraph->begin_batch();
    EXPECT_NE(aBatch.graph(), nullptr);
  }
  SUCCEED();
}

TEST_F(TopoVeneerFixture, Batch_DestructorDuringInFlightException_DoesNotTerminate)
{
  // Throw an exception while a Batch is alive on the stack.  The Batch's
  // destructor must not throw (would be std::terminate during unwind).
  try
  {
    occtl::Batch aBatch = myGraph->begin_batch();
    (void)aBatch;
    throw std::runtime_error("unwind trigger");
  }
  catch (const std::runtime_error&)
  {
    SUCCEED();
  }
}

TEST_F(TopoVeneerFixture, Batch_CommitTwice_SecondIsNoop)
{
  occtl::Batch aBatch = myGraph->begin_batch();
  EXPECT_NO_THROW(aBatch.commit());
  EXPECT_NO_THROW(aBatch.commit()); // already committed → no-op, must not throw
}

TEST_F(TopoVeneerFixture, NodeIter_RangeForOverSolids_CountsAtLeastOne)
{
  occtl::NodeIter aSolids = myGraph->solids();

  std::size_t aCount = 0;
  for (const occtl::NodeId& aId : aSolids)
  {
    EXPECT_NE(aId.get().bits, 0u);
    ++aCount;
  }
  EXPECT_GE(aCount, 1u);
}

TEST_F(TopoVeneerFixture, RefUidTable_RoundTripsRefs)
{
  const std::vector<std::pair<occtl::RefUID, occtl::RefId>> aRefs = myGraph->ref_uid_table();
  ASSERT_FALSE(aRefs.empty());

  const occtl::RefUID aUid = myGraph->ref_uid_from_ref_id(aRefs.front().second);
  EXPECT_EQ(aUid, aRefs.front().first);
  const std::array<std::uint8_t, OCCTL_REF_UID_WIRE_SIZE> aBytes = aUid.to_bytes();
  EXPECT_EQ(occtl::RefUID::from_bytes(aBytes), aUid);
  const occtl::RefId aRef = myGraph->ref_id_from_ref_uid(aUid);
  EXPECT_EQ(aRef, aRefs.front().second);
  EXPECT_EQ(myGraph->ref_uid_kind(aUid), myGraph->ref_id_kind(aRef));
}

TEST_F(TopoVeneerFixture, WireOrderEdges_BoxWire_ReturnsEdges)
{
  occtl::NodeId aWire = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->wires())
  {
    aWire = anId;
    break;
  }
  ASSERT_TRUE(aWire.is_valid());

  const std::vector<occtl::OrientedNode> anEdges = myGraph->wire_order_edges(aWire);
  EXPECT_EQ(anEdges.size(), 4u);
  for (const occtl::OrientedNode& anEntry : anEdges)
  {
    EXPECT_TRUE(occtl::NodeId(anEntry.id).is_valid());
  }
}

TEST_F(TopoVeneerFixture, EdgesToWires_UnorderedBoxFaceEdges_ReturnsWire)
{
  std::vector<::occtl_node_id_t> anEdges;
  for (const occtl::NodeId& anId : myGraph->edges())
  {
    anEdges.push_back(anId.get());
    if (anEdges.size() == 4u)
    {
      break;
    }
  }
  ASSERT_EQ(anEdges.size(), 4u);

  occtl::EdgesToWiresOptions anOptions = OCCTL_TOPO_EDGES_TO_WIRES_OPTIONS_INIT;
  anOptions.edges                      = anEdges.data();
  anOptions.edge_count                 = anEdges.size();

  const std::vector<occtl::NodeId> aWires = myGraph->edges_to_wires(anOptions);

  ASSERT_FALSE(aWires.empty());
  for (const occtl::NodeId& aWire : aWires)
  {
    EXPECT_TRUE(aWire.is_valid());
    EXPECT_GT(myGraph->wire_coedge_count(aWire), 0u);
  }
}

TEST_F(TopoVeneerFixture, WireOffset2d_RectangleWire_ReturnsWire)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 4.0;
  aRect.height                      = 2.0;

  ::occtl_node_id_t aWireRaw = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_prim_make_rectangle(myGraph->get(), &aRect, &aWireRaw), OCCTL_OK);
  ASSERT_NE(aWireRaw.bits, 0u);

  occtl::WireOffset2dOptions anOptions = OCCTL_TOPO_WIRE_OFFSET_2D_OPTIONS_INIT;
  anOptions.wire                       = aWireRaw;
  anOptions.distance                   = 0.25;

  const occtl::NodeId aOffsetWire = myGraph->wire_offset_2d(anOptions);
  EXPECT_TRUE(aOffsetWire.is_valid());
  EXPECT_GT(myGraph->wire_coedge_count(aOffsetWire), 0u);
}

TEST_F(TopoVeneerFixture, MakeFaceFromPointGrid_ApproximateGrid_ReturnsFace)
{
  const occtl_point3_t aPoints[9] = {{0.0, 0.0, 0.0},
                                     {0.0, 1.0, 0.1},
                                     {0.0, 2.0, 0.0},
                                     {1.0, 0.0, 0.3},
                                     {1.0, 1.0, 0.7},
                                     {1.0, 2.0, 0.3},
                                     {2.0, 0.0, 0.0},
                                     {2.0, 1.0, 0.1},
                                     {2.0, 2.0, 0.0}};

  occtl_prim_face_from_point_grid_options_t anOptions =
    OCCTL_PRIM_FACE_FROM_POINT_GRID_OPTIONS_INIT;
  anOptions.surface.points        = aPoints;
  anOptions.surface.u_point_count = 3;
  anOptions.surface.v_point_count = 3;

  const occtl::NodeId aFace = occtl::prim::make_face_from_point_grid(*myGraph, anOptions);
  EXPECT_TRUE(aFace.is_valid());
  EXPECT_EQ(myGraph->node_id_kind(aFace), OCCTL_KIND_FACE);
}

TEST_F(TopoVeneerFixture, MakeFaceFromBoundaryCurves_FourLines_ReturnsFace)
{
  const occtl_rep_id_t aBottom =
    makeTrimmedLine(myGraph->get(), {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0.0, 1.0);
  const occtl_rep_id_t aRight =
    makeTrimmedLine(myGraph->get(), {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 0.0, 1.0);
  const occtl_rep_id_t aTop =
    makeTrimmedLine(myGraph->get(), {1.0, 1.0, 0.0}, {-1.0, 0.0, 0.0}, 0.0, 1.0);
  const occtl_rep_id_t aLeft =
    makeTrimmedLine(myGraph->get(), {0.0, 1.0, 0.0}, {0.0, -1.0, 0.0}, 0.0, 1.0);

  const occtl_rep_id_t                           aCurves[4] = {aBottom, aRight, aTop, aLeft};
  occtl_prim_face_from_boundary_curves_options_t anOptions =
    OCCTL_PRIM_FACE_FROM_BOUNDARY_CURVES_OPTIONS_INIT;
  anOptions.surface.curves      = aCurves;
  anOptions.surface.curve_count = 4;

  const occtl::NodeId aFace = occtl::prim::make_face_from_boundary_curves(*myGraph, anOptions);
  EXPECT_TRUE(aFace.is_valid());
  EXPECT_EQ(myGraph->node_id_kind(aFace), OCCTL_KIND_FACE);
}

TEST_F(TopoVeneerFixture, WireFixDegenerateEdges_OpenWire_ReturnsRemovedCount)
{
  ::occtl_topo_make_vertex_info_t aVertInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
  aVertInfo.tolerance                       = 1.0e-7;

  ::occtl_node_id_t aVertices[3] = {OCCTL_NODE_ID_INVALID,
                                    OCCTL_NODE_ID_INVALID,
                                    OCCTL_NODE_ID_INVALID};
  const double      aX[3]        = {0.0, 1.0e-5, 1.0};
  for (int anI = 0; anI < 3; ++anI)
  {
    aVertInfo.point = {aX[anI], 0.0, 0.0};
    ASSERT_EQ(::occtl_topo_make_vertex(myGraph->get(), &aVertInfo, &aVertices[anI]), OCCTL_OK);
  }

  ::occtl_topo_make_edge_info_t anEdgeInfo = OCCTL_TOPO_MAKE_EDGE_INFO_INIT;
  anEdgeInfo.tolerance                     = 1.0e-7;

  ::occtl_geom_line_t aLineData = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  ::occtl_rep_id_t    aCurve    = {};
  ASSERT_EQ(::occtl_curve_create_line(myGraph->get(), aLineData, &aCurve), OCCTL_OK);
  ASSERT_NE(aCurve.bits, 0u);
  anEdgeInfo.curve = aCurve;

  ::occtl_oriented_node_t anEdges[2];
  for (int anI = 0; anI < 2; ++anI)
  {
    anEdgeInfo.start_vertex = aVertices[anI];
    anEdgeInfo.end_vertex   = aVertices[anI + 1];
    anEdgeInfo.first        = 0.0;
    anEdgeInfo.last         = aX[anI + 1] - aX[anI];

    ::occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
    ASSERT_EQ(::occtl_topo_make_edge(myGraph->get(), &anEdgeInfo, &anEdge), OCCTL_OK);
    anEdges[anI] = {anEdge, OCCTL_ORIENTATION_FORWARD};
  }

  ::occtl_topo_make_wire_info_t aWireInfo = OCCTL_TOPO_MAKE_WIRE_INFO_INIT;
  aWireInfo.edges                         = anEdges;
  aWireInfo.edge_count                    = 2;

  ::occtl_node_id_t aWireRaw = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_topo_make_wire(myGraph->get(), &aWireInfo, &aWireRaw), OCCTL_OK);

  occtl::WireFixDegenerateEdgesOptions anOptions =
    OCCTL_TOPO_WIRE_FIX_DEGENERATE_EDGES_OPTIONS_INIT;
  anOptions.wire       = aWireRaw;
  anOptions.min_length = 1.0e-4;

  EXPECT_EQ(myGraph->wire_fix_degenerate_edges(anOptions), 1u);
  EXPECT_EQ(myGraph->wire_coedge_count(occtl::NodeId(aWireRaw)), 1u);
}

TEST_F(TopoVeneerFixture, OccurrenceTransforms_ProductRoot_ReturnTransforms)
{
  occtl::NodeId aSolid = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->solids())
  {
    aSolid = anId;
    break;
  }
  ASSERT_TRUE(aSolid.is_valid());

  ::occtl_topo_make_product_info_t anInfo = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  anInfo.root                             = aSolid.get();
  anInfo.placement                        = ::occtl_transform_translation({1.0, 2.0, 3.0});

  const occtl::NodeId aProduct = myGraph->make_product(anInfo);
  ASSERT_TRUE(aProduct.is_valid());

  occtl::NodeId anOccurrence = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->occurrences_of_product(aProduct))
  {
    anOccurrence = anId;
    break;
  }
  ASSERT_TRUE(anOccurrence.is_valid());

  const ::occtl_transform_t aLocal = myGraph->occurrence_transform_get(anOccurrence);
  EXPECT_NEAR(aLocal.m[3], 1.0, 1.0e-12);
  EXPECT_NEAR(aLocal.m[7], 2.0, 1.0e-12);
  EXPECT_NEAR(aLocal.m[11], 3.0, 1.0e-12);

  const ::occtl_transform_t aNewLocal = ::occtl_transform_translation({4.0, 5.0, 6.0});
  myGraph->occurrence_transform_set(anOccurrence, aNewLocal);

  const ::occtl_transform_t aWorld = myGraph->occurrence_world_transform(aProduct, anOccurrence);
  EXPECT_NEAR(aWorld.m[3], 4.0, 1.0e-12);
  EXPECT_NEAR(aWorld.m[7], 5.0, 1.0e-12);
  EXPECT_NEAR(aWorld.m[11], 6.0, 1.0e-12);
}

TEST_F(TopoVeneerFixture, LinkProductsWithOccurrence_ReturnsOccurrence)
{
  ::occtl_topo_make_product_info_t anInfo  = OCCTL_TOPO_MAKE_PRODUCT_INFO_INIT;
  const occtl::NodeId              aParent = myGraph->make_product(anInfo);
  const occtl::NodeId              aChild  = myGraph->make_product(anInfo);
  ASSERT_TRUE(aParent.is_valid());
  ASSERT_TRUE(aChild.is_valid());

  const ::occtl_transform_t aPlacement = ::occtl_transform_translation({9.0, 1.0, -2.0});
  const occtl::NodeId       anOccurrence =
    myGraph->link_products_with_occurrence(aParent, aChild, aPlacement);
  ASSERT_TRUE(anOccurrence.is_valid());
  EXPECT_EQ(myGraph->node_id_kind(anOccurrence), OCCTL_KIND_OCCURRENCE);

  const ::occtl_transform_t aLocal = myGraph->occurrence_transform_get(anOccurrence);
  EXPECT_NEAR(aLocal.m[3], 9.0, 1.0e-12);
  EXPECT_NEAR(aLocal.m[7], 1.0, 1.0e-12);
  EXPECT_NEAR(aLocal.m[11], -2.0, 1.0e-12);
}

TEST_F(TopoVeneerFixture, FaceChamfer2d_RectangleFace_ReturnsFace)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 10.0;
  aRect.height                      = 10.0;

  ::occtl_node_id_t aRectWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_prim_make_rectangle(myGraph->get(), &aRect, &aRectWire), OCCTL_OK);

  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = aRectWire;

  ::occtl_node_id_t aFaceRaw = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_prim_make_planar_face(myGraph->get(), &aFaceInfo, &aFaceRaw), OCCTL_OK);
  ASSERT_NE(aFaceRaw.bits, 0u);

  occtl::FaceChamfer2dOptions anOptions = OCCTL_TOPO_FACE_CHAMFER_2D_OPTIONS_INIT;
  anOptions.face                        = aFaceRaw;
  anOptions.distance1                   = 1.0;
  anOptions.distance2                   = 1.0;

  const occtl::NodeId aChamferedFace = myGraph->face_chamfer_2d(anOptions);
  EXPECT_TRUE(aChamferedFace.is_valid());
  EXPECT_EQ(myGraph->face_wire_count(aChamferedFace), 1u);
}

TEST_F(TopoVeneerFixture, MakeFaceFromWiresAuto_RectangleWire_ReturnsFace)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 4.0;
  aRect.height                      = 2.0;

  ::occtl_node_id_t aWireRaw = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_prim_make_rectangle(myGraph->get(), &aRect, &aWireRaw), OCCTL_OK);
  ASSERT_NE(aWireRaw.bits, 0u);

  ::occtl_geom_plane_t aPlane = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  ::occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(::occtl_surface_create_plane(myGraph->get(), &aSurfaceId, aPlane), OCCTL_OK);
  ASSERT_NE(aSurfaceId.bits, 0u);

  occtl::MakeFaceFromWiresAutoOptions anOptions = OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_INIT;
  anOptions.surface                             = aSurfaceId;
  anOptions.wires                               = &aWireRaw;
  anOptions.wire_count                          = 1;

  const occtl::NodeId aFace = myGraph->make_face_from_wires_auto(anOptions);
  EXPECT_TRUE(aFace.is_valid());
}

TEST_F(TopoVeneerFixture, FaceRemoveHoles_FaceWithHole_RemovesInnerWire)
{
  occtl_prim_rectangle_info_t anOuter = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  anOuter.width                       = 8.0;
  anOuter.height                      = 8.0;
  ::occtl_node_id_t anOuterWire       = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_prim_make_rectangle(myGraph->get(), &anOuter, &anOuterWire), OCCTL_OK);

  occtl_prim_rectangle_info_t anInner = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  anInner.width                       = 2.0;
  anInner.height                      = 2.0;
  ::occtl_node_id_t anInnerWire       = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_prim_make_rectangle(myGraph->get(), &anInner, &anInnerWire), OCCTL_OK);

  const ::occtl_node_id_t aWires[2] = {anInnerWire, anOuterWire};
  ::occtl_geom_plane_t    aPlane    = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  ::occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(::occtl_surface_create_plane(myGraph->get(), &aSurfaceId, aPlane), OCCTL_OK);
  ASSERT_NE(aSurfaceId.bits, 0u);

  occtl::MakeFaceFromWiresAutoOptions anOptions = OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_INIT;
  anOptions.surface                             = aSurfaceId;
  anOptions.wires                               = aWires;
  anOptions.wire_count                          = 2;

  const occtl::NodeId aFace = myGraph->make_face_from_wires_auto(anOptions);
  ASSERT_EQ(myGraph->face_wire_count(aFace), 2u);

  myGraph->face_remove_holes(aFace, {occtl::NodeId(anInnerWire)});
  EXPECT_EQ(myGraph->face_wire_count(aFace), 1u);
}

TEST_F(TopoVeneerFixture, FaceAddHoles_OuterFace_AddsInnerWire)
{
  occtl_prim_rectangle_info_t anOuter = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  anOuter.width                       = 8.0;
  anOuter.height                      = 8.0;
  ::occtl_node_id_t anOuterWire       = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_prim_make_rectangle(myGraph->get(), &anOuter, &anOuterWire), OCCTL_OK);

  occtl_prim_rectangle_info_t anInner = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  anInner.width                       = 2.0;
  anInner.height                      = 2.0;
  ::occtl_node_id_t anInnerWire       = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_prim_make_rectangle(myGraph->get(), &anInner, &anInnerWire), OCCTL_OK);

  ::occtl_geom_plane_t aPlane = {
    {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
  ::occtl_rep_id_t aSurfaceId = {};
  ASSERT_EQ(::occtl_surface_create_plane(myGraph->get(), &aSurfaceId, aPlane), OCCTL_OK);
  ASSERT_NE(aSurfaceId.bits, 0u);

  occtl::MakeFaceFromWiresAutoOptions anOptions = OCCTL_TOPO_MAKE_FACE_FROM_WIRES_AUTO_OPTIONS_INIT;
  anOptions.surface                             = aSurfaceId;
  anOptions.wires                               = &anOuterWire;
  anOptions.wire_count                          = 1;

  const occtl::NodeId aFace = myGraph->make_face_from_wires_auto(anOptions);
  ASSERT_EQ(myGraph->face_wire_count(aFace), 1u);

  myGraph->face_add_holes(aFace, {occtl::NodeId(anInnerWire)});
  EXPECT_EQ(myGraph->face_wire_count(aFace), 2u);
}

TEST_F(TopoVeneerFixture, GraphUnits_SetGet_ReturnsUnits)
{
  myGraph->graph_units_set(0.001, "mm", 2);

  const occtl::GraphUnits aUnits = myGraph->graph_units_get();
  EXPECT_DOUBLE_EQ(aUnits.length_unit_to_meter, 0.001);
  EXPECT_EQ(aUnits.name, "mm");
}

TEST_F(TopoVeneerFixture, GraphMetadata_SetGetUnset_ReturnsValue)
{
  myGraph->graph_metadata_set("author", 6, "unit-test", 9);
  myGraph->graph_metadata_set("source", 6, "script", 6);

  EXPECT_EQ(myGraph->graph_metadata_get("author", 6), "unit-test");

  const std::vector<std::string> aKeys = myGraph->graph_metadata_keys();
  EXPECT_NE(std::find(aKeys.begin(), aKeys.end(), "author"), aKeys.end());
  EXPECT_NE(std::find(aKeys.begin(), aKeys.end(), "source"), aKeys.end());

  myGraph->graph_metadata_unset("author", 6);
  EXPECT_THROW(myGraph->graph_metadata_get("author", 6), occtl::Error);
}

TEST_F(TopoVeneerFixture, LayerMaterial_SetGetUnset_ReturnsMaterial)
{
  const occtl::NodeId aFace(firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_FACE));
  ASSERT_TRUE(aFace.is_valid());

  myGraph->color_set(aFace, {0.1f, 0.2f, 0.3f, 1.0f});
  const std::vector<std::pair<occtl::NodeId, ::occtl_color_rgba_t>> aColorEntries =
    myGraph->color_entries();
  ASSERT_EQ(aColorEntries.size(), 1u);
  EXPECT_EQ(aColorEntries[0].first, aFace);
  EXPECT_FLOAT_EQ(aColorEntries[0].second.g, 0.2f);

  occtl::MaterialInfo aMaterial = OCCTL_MATERIAL_INFO_INIT;
  aMaterial.name                = "Steel";
  aMaterial.name_len            = 5;
  aMaterial.has_density         = 1;
  aMaterial.density             = 7850.0;
  myGraph->material_set(aFace, aMaterial);

  const occtl::GraphMaterial anOut = myGraph->material_get(aFace);
  EXPECT_EQ(anOut.name, "Steel");
  EXPECT_EQ(anOut.has_density, 1);
  EXPECT_DOUBLE_EQ(anOut.density, 7850.0);

  const std::vector<occtl::NodeId> aMaterialNodes = myGraph->material_nodes();
  ASSERT_EQ(aMaterialNodes.size(), 1u);
  EXPECT_EQ(aMaterialNodes[0], aFace);

  myGraph->material_unset(aFace);
  EXPECT_THROW(myGraph->material_get(aFace), occtl::Error);
}

TEST_F(TopoVeneerFixture, LayerMetadata_SetGetUnset_ReturnsValue)
{
  const occtl::NodeId aFace(firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_FACE));
  ASSERT_TRUE(aFace.is_valid());

  myGraph->node_metadata_set(aFace, "role", 4, "mount", 5);
  myGraph->node_metadata_set(aFace, "owner", 5, "fixture", 7);
  myGraph->name_set(aFace, "MountFace", 9);
  EXPECT_EQ(myGraph->node_metadata_get(aFace, "role", 4), "mount");

  const std::vector<occtl::NodeId> aNameNodes = myGraph->name_nodes();
  ASSERT_EQ(aNameNodes.size(), 1u);
  EXPECT_EQ(aNameNodes[0], aFace);

  const std::vector<occtl::NodeId> aMetadataNodes = myGraph->node_metadata_nodes();
  ASSERT_EQ(aMetadataNodes.size(), 1u);
  EXPECT_EQ(aMetadataNodes[0], aFace);

  const std::vector<std::string> aKeys = myGraph->node_metadata_keys(aFace);
  EXPECT_NE(std::find(aKeys.begin(), aKeys.end(), "owner"), aKeys.end());
  EXPECT_NE(std::find(aKeys.begin(), aKeys.end(), "role"), aKeys.end());

  myGraph->node_metadata_unset(aFace, "role", 4);
  EXPECT_THROW(myGraph->node_metadata_get(aFace, "role", 4), occtl::Error);
}

TEST_F(TopoVeneerFixture, JointLayer_CreateListGetRemove_ReturnsJoint)
{
  const occtl::NodeId aFace(firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_FACE));
  const occtl::NodeId anEdge(firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_EDGE));
  ASSERT_TRUE(aFace.is_valid());
  ASSERT_TRUE(anEdge.is_valid());

  occtl::JointInfo anInfo = OCCTL_JOINT_INFO_INIT;
  anInfo.kind             = OCCTL_JOINT_CYLINDRICAL;
  anInfo.node_a           = aFace.get();
  anInfo.node_b           = anEdge.get();
  anInfo.frame_b.m[7]     = 2.0;

  const occtl::JointId aJoint = myGraph->joint_create(anInfo);
  ASSERT_TRUE(aJoint.is_valid());

  const std::vector<occtl::JointId> aJoints = myGraph->joint_list(aFace);
  ASSERT_EQ(aJoints.size(), 1u);
  EXPECT_EQ(aJoints[0], aJoint);

  const occtl::JointInfo anOut = myGraph->joint_get(aJoint);
  EXPECT_EQ(anOut.kind, OCCTL_JOINT_CYLINDRICAL);
  EXPECT_EQ(anOut.node_a.bits, aFace.get().bits);
  EXPECT_EQ(anOut.node_b.bits, anEdge.get().bits);
  EXPECT_DOUBLE_EQ(anOut.frame_b.m[7], 2.0);

  myGraph->joint_remove(aJoint);
  EXPECT_THROW(myGraph->joint_get(aJoint), occtl::Error);
}

TEST_F(TopoVeneerFixture, SelectIter_ByKindCountsFaces)
{
  occtl::SelectOptions aOptions = OCCTL_SELECT_OPTIONS_INIT;
  aOptions.kind_mask            = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);

  std::size_t aCount = 0;
  for (const occtl::NodeId& aId : myGraph->select(aOptions))
  {
    EXPECT_NE(aId.get().bits, 0u);
    ++aCount;
  }
  EXPECT_EQ(aCount, 6u);
}

TEST_F(TopoVeneerFixture, SelectGroupIter_ByKindCountsFaceGroup)
{
  occtl::SelectOptions aSelectOptions = OCCTL_SELECT_OPTIONS_INIT;
  aSelectOptions.kind_mask            = uint64_t(1) << static_cast<unsigned int>(OCCTL_KIND_FACE);
  occtl::SelectGroupOptions aGroupOptions = OCCTL_SELECT_GROUP_OPTIONS_INIT;
  aGroupOptions.key                       = OCCTL_SELECT_GROUP_KIND;

  std::size_t aGroupCount = 0;
  for (const occtl::SelectGroupView& aView : myGraph->select_groups(aSelectOptions, aGroupOptions))
  {
    EXPECT_EQ(aView.key, OCCTL_SELECT_GROUP_KIND);
    EXPECT_EQ(aView.node_kind, OCCTL_KIND_FACE);
    EXPECT_EQ(aView.node_count, 6u);
    ++aGroupCount;
  }
  EXPECT_EQ(aGroupCount, 1u);
}

TEST_F(TopoVeneerFixture, DistancePair_TwoBoxVertices_ReturnsFiniteDistance)
{
  auto aVertices = myGraph->vertices();
  auto anIt      = aVertices.begin();
  ASSERT_NE(anIt, aVertices.end());
  const occtl::NodeId aVertexA = *anIt;
  ++anIt;
  ASSERT_NE(anIt, aVertices.end());
  const occtl::NodeId aVertexB = *anIt;

  const occtl::DistancePair aPair = myGraph->distance_pair(aVertexA, aVertexB);

  EXPECT_GT(aPair.distance, 0.0);
  EXPECT_NE(aPair.support_a.bits, 0u);
  EXPECT_NE(aPair.support_b.bits, 0u);
  EXPECT_GE(aPair.solution_count, 1);
}

TEST_F(TopoVeneerFixture, Touches_SameVertex_ReturnsContactHit)
{
  auto aVertices = myGraph->vertices();
  auto anIt      = aVertices.begin();
  ASSERT_NE(anIt, aVertices.end());
  const occtl::NodeId aVertex = *anIt;

  std::vector<occtl::TouchHit> aHits;
  for (const occtl::TouchHit& aHit : myGraph->touches(aVertex, aVertex))
  {
    aHits.push_back(aHit);
  }

  ASSERT_GT(aHits.size(), 0u);
  EXPECT_EQ(aHits.front().node_a.bits, aVertex.get().bits);
  EXPECT_EQ(aHits.front().node_b.bits, aVertex.get().bits);
  EXPECT_NEAR(aHits.front().distance, 0.0, 1e-12);
}

TEST_F(TopoVeneerFixture, Intersections_OverlappingBoxes_ReturnsGeneratedNodes)
{
  occtl_prim_box_info_t aBox = OCCTL_PRIM_BOX_INFO_INIT;
  aBox.placement.location    = {5.0, 5.0, 5.0};
  aBox.dx                    = 10.0;
  aBox.dy                    = 10.0;
  aBox.dz                    = 10.0;
  occtl_node_id_t aBoxB      = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_box(myGraph->get(), &aBox, &aBoxB), OCCTL_OK);

  occtl::NodeId aBoxA = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->solids())
  {
    if (anId.get().bits != aBoxB.bits)
    {
      aBoxA = anId;
      break;
    }
  }
  ASSERT_TRUE(aBoxA.is_valid());

  occtl::RelationOptions anOptions = OCCTL_TOPO_RELATION_OPTIONS_INIT;
  anOptions.include_overlaps       = 0;
  size_t aCount                    = 0;
  for (const occtl::NodeId& aNode : myGraph->intersections(aBoxA, occtl::NodeId(aBoxB), &anOptions))
  {
    const occtl_node_kind_t aKind = myGraph->node_id_kind(aNode);
    EXPECT_TRUE(aKind == OCCTL_KIND_EDGE || aKind == OCCTL_KIND_VERTEX);
    ++aCount;
  }
  EXPECT_GT(aCount, 0u);
}

TEST_F(TopoVeneerFixture, CommonVertices_SolidAndFace_ReturnsFaceVertices)
{
  occtl::NodeId aSolid = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->solids())
  {
    aSolid = anId;
    break;
  }
  ASSERT_TRUE(aSolid.is_valid());

  occtl::NodeId aFace = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->faces())
  {
    aFace = anId;
    break;
  }
  ASSERT_TRUE(aFace.is_valid());

  const std::vector<occtl::NodeId> aCommon = myGraph->common_vertices(aSolid, aFace);

  EXPECT_EQ(aCommon.size(), 4u);
  for (const occtl::NodeId& anId : aCommon)
  {
    EXPECT_TRUE(anId.is_valid());
  }
}

TEST_F(TopoVeneerFixture, FacesIntersectedByAxis_BoxSolid_ReturnsRange)
{
  occtl::NodeId aSolid = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->solids())
  {
    aSolid = anId;
    break;
  }
  ASSERT_TRUE(aSolid.is_valid());

  const occtl::Axis1Placement anAxis(occtl::Point3(-5.0, 10.0, 15.0),
                                     occtl::Direction3(::occtl_direction3_t{1.0, 0.0, 0.0}));
  std::vector<occtl::AxisHit> aHits;
  for (const occtl::AxisHit& aHit :
       myGraph->faces_intersected_by_axis(aSolid, anAxis, 0.0, 20.0, 1e-7))
  {
    aHits.push_back(aHit);
  }

  ASSERT_EQ(aHits.size(), 2u);
  EXPECT_NEAR(aHits[0].parameter, 5.0, 1e-7);
  EXPECT_NEAR(aHits[1].parameter, 15.0, 1e-7);
  EXPECT_TRUE(occtl::NodeId(aHits[0].face).is_valid());
  EXPECT_TRUE(occtl::NodeId(aHits[1].face).is_valid());
  EXPECT_EQ(aHits[0].has_normal, 1);
  EXPECT_EQ(aHits[1].has_normal, 1);
}

TEST_F(TopoVeneerFixture, IsSameGeometry_SameFace_ReturnsTrue)
{
  occtl::NodeId aFace = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->faces())
  {
    aFace = anId;
    break;
  }
  ASSERT_TRUE(aFace.is_valid());

  EXPECT_TRUE(myGraph->is_same_geometry(aFace, aFace, 1e-7));
}

TEST_F(TopoVeneerFixture, ConnectedEdgesAndFaces_Box_ReturnsConnectedComponents)
{
  occtl::NodeId anEdge = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->edges())
  {
    anEdge = anId;
    break;
  }
  ASSERT_TRUE(anEdge.is_valid());

  occtl::NodeId aFace = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->faces())
  {
    aFace = anId;
    break;
  }
  ASSERT_TRUE(aFace.is_valid());

  EXPECT_EQ(myGraph->adjacent_edges(anEdge).size(), 4u);
  EXPECT_EQ(myGraph->adjacent_faces(aFace).size(), 4u);
  EXPECT_EQ(myGraph->connected_edges(anEdge).size(), 12u);
  EXPECT_EQ(myGraph->connected_faces(aFace).size(), 6u);
}

TEST_F(TopoVeneerFixture, GraphDistance_BoxFaces_ReturnsHopDistance)
{
  occtl::NodeId aSolid = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->solids())
  {
    aSolid = anId;
    break;
  }
  ASSERT_TRUE(aSolid.is_valid());

  occtl::NodeId aFace = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->faces())
  {
    aFace = anId;
    break;
  }
  ASSERT_TRUE(aFace.is_valid());

  const std::vector<occtl::NodeId> aFaces = myGraph->connected_faces(aFace);
  ASSERT_EQ(aFaces.size(), 6u);

  bool hasOppositeFace = false;
  for (const occtl::NodeId& aTarget : aFaces)
  {
    if (myGraph->graph_distance(aSolid, aFace, aTarget) == 2)
    {
      hasOppositeFace = true;
    }
  }
  EXPECT_TRUE(hasOppositeFace);
  EXPECT_EQ(myGraph->graph_distance(aSolid,
                                    std::vector<occtl::NodeId>{aFace, aFaces.back()},
                                    aFaces.back()),
            0);
}

TEST_F(TopoVeneerFixture, ClassifyPointAndIsInside_BoxSolid_ReturnExpectedStates)
{
  occtl::NodeId aSolid = occtl::NodeId::invalid();
  for (const occtl::NodeId& anId : myGraph->solids())
  {
    aSolid = anId;
    break;
  }
  ASSERT_TRUE(aSolid.is_valid());

  EXPECT_EQ(myGraph->classify_point(aSolid, occtl::Point3(5.0, 10.0, 15.0), 1e-7),
            OCCTL_TOPO_POINT_CLASS_IN);
  EXPECT_TRUE(myGraph->is_inside(aSolid, occtl::Point3(5.0, 10.0, 15.0), 1e-7));
  EXPECT_FALSE(myGraph->is_inside(aSolid, occtl::Point3(20.0, 40.0, 60.0), 1e-7));
}

TEST_F(TopoVeneerFixture, CheckIssues_BoxIsValid)
{
  const std::vector<::occtl_topo_check_issue_t> anIssues = myGraph->check_issues();
  EXPECT_TRUE(anIssues.empty());
  EXPECT_TRUE(myGraph->is_valid());
}

TEST_F(TopoVeneerFixture, InvalidNodeId_VertexPoint_ThrowsOcctlError)
{
  // A non-zero but never-issued node id should be rejected.
  ::occtl_node_id_t aBad{};
  aBad.bits = 0xdeadbeefdeadbeefULL;
  EXPECT_THROW(myGraph->vertex_point(occtl::NodeId(aBad)), occtl::Error);
}

TEST_F(TopoVeneerFixture, ShellAddFace_NonShell_ThrowsOcctlError)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidAbi.bits, 0u);
  const ::occtl_node_id_t aFaceAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_FACE);
  ASSERT_NE(aFaceAbi.bits, 0u);

  // Solid is not a shell — the veneer should translate the C status to an occtl::Error.
  EXPECT_THROW(myGraph->shell_add_face(occtl::NodeId(aSolidAbi),
                                       occtl::NodeId(aFaceAbi),
                                       OCCTL_ORIENTATION_FORWARD),
               occtl::Error);
}

TEST_F(TopoVeneerFixture, TransformCopy_Translated_ReturnsGraphAndRoot)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidAbi.bits, 0u);

  const auto aResult = myGraph->translated(occtl::NodeId(aSolidAbi), occtl::Vector3(1.0, 2.0, 3.0));
  EXPECT_NE(aResult.first.get(), nullptr);
  EXPECT_NE(aResult.second.get().bits, 0u);
}

TEST_F(TopoVeneerFixture, BlendEdges_FilletEdges_ReturnsGraphAndRoot)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  const ::occtl_node_id_t anEdgeAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_EDGE);
  ASSERT_NE(aSolidAbi.bits, 0u);
  ASSERT_NE(anEdgeAbi.bits, 0u);

  const auto aResult = myGraph->fillet_edges(occtl::NodeId(aSolidAbi),
                                             std::vector<occtl::NodeId>{occtl::NodeId(anEdgeAbi)},
                                             0.5);
  EXPECT_NE(aResult.first.get(), nullptr);
  EXPECT_NE(aResult.second.get().bits, 0u);
}

TEST_F(TopoVeneerFixture, DraftFaces_ReturnsGraphAndRoot)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  const ::occtl_node_id_t aFaceAbi  = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_FACE);
  ASSERT_NE(aSolidAbi.bits, 0u);
  ASSERT_NE(aFaceAbi.bits, 0u);

  ::occtl_topo_draft_faces_options_t anOpts = OCCTL_TOPO_DRAFT_FACES_OPTIONS_INIT;
  anOpts.root                               = aSolidAbi;
  anOpts.faces                              = &aFaceAbi;
  anOpts.face_count                         = 1;
  anOpts.angle                              = 0.05;

  const auto aResult = myGraph->draft_faces(anOpts);
  EXPECT_NE(aResult.first.get(), nullptr);
  EXPECT_NE(aResult.second.get().bits, 0u);
}

TEST_F(TopoVeneerFixture, SplitByPlane_ReturnsGraphAndRoot)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidAbi.bits, 0u);

  ::occtl_topo_split_by_plane_options_t anOpts = OCCTL_TOPO_SPLIT_BY_PLANE_OPTIONS_INIT;
  anOpts.root                                  = aSolidAbi;
  anOpts.point                                 = {5.0, 0.0, 0.0};
  anOpts.normal                                = {1.0, 0.0, 0.0};
  anOpts.keep                                  = OCCTL_TOPO_SPLIT_KEEP_NEGATIVE;

  const auto aResult = myGraph->split_by_plane(anOpts);
  EXPECT_NE(aResult.first.get(), nullptr);
  EXPECT_NE(aResult.second.get().bits, 0u);
}

TEST_F(TopoVeneerFixture, SectionByPlanes_ReturnsGraphAndRoot)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  ASSERT_NE(aSolidAbi.bits, 0u);

  const ::occtl_topo_section_plane_t       aPlane = {{5.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  ::occtl_topo_section_by_planes_options_t anOpts = OCCTL_TOPO_SECTION_BY_PLANES_OPTIONS_INIT;
  anOpts.root                                     = aSolidAbi;
  anOpts.planes                                   = &aPlane;
  anOpts.plane_count                              = 1;

  const auto aResult = myGraph->section_by_planes(anOpts);
  EXPECT_NE(aResult.first.get(), nullptr);
  EXPECT_NE(aResult.second.get().bits, 0u);
}

TEST_F(TopoVeneerFixture, ExtrudeFacesToSolids_ReturnsGraphAndRoot)
{
  const ::occtl_node_id_t aFaceAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_FACE);
  ASSERT_NE(aFaceAbi.bits, 0u);

  ::occtl_topo_extrude_faces_options_t anOpts = OCCTL_TOPO_EXTRUDE_FACES_OPTIONS_INIT;
  anOpts.faces                                = &aFaceAbi;
  anOpts.face_count                           = 1;
  anOpts.thickness                            = 2.0;

  const auto aResult = myGraph->extrude_faces_to_solids(anOpts);
  EXPECT_NE(aResult.first.get(), nullptr);
  EXPECT_NE(aResult.second.get().bits, 0u);
}

TEST_F(TopoVeneerFixture, MakeBrakeFormed_ReturnsGraphAndRoot)
{
  const ::occtl_point3_t       aPoints[3] = {{0.0, 0.0, 0.0}, {5.0, 0.0, 0.0}, {5.0, 3.0, 0.0}};
  ::occtl_prim_polyline_info_t aLineInfo  = OCCTL_PRIM_POLYLINE_INFO_INIT;
  aLineInfo.points                        = aPoints;
  aLineInfo.point_count                   = 3;

  ::occtl_node_id_t aLine = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(::occtl_prim_make_polyline(myGraph->get(), &aLineInfo, &aLine), OCCTL_OK);

  const double                        aWidth = 2.0;
  ::occtl_prim_brake_formed_options_t anOpts = OCCTL_PRIM_BRAKE_FORMED_OPTIONS_INIT;
  anOpts.line                                = aLine;
  anOpts.thickness                           = 0.25;
  anOpts.station_widths                      = &aWidth;
  anOpts.station_width_count                 = 1;

  const auto aResult = occtl::prim::make_brake_formed(*myGraph, anOpts);
  EXPECT_NE(aResult.first.get(), nullptr);
  EXPECT_NE(aResult.second.get().bits, 0u);
}

TEST_F(TopoVeneerFixture, BlendEdgesWithHistory_WrongKindSelection_ThrowsOcctlError)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  const ::occtl_node_id_t aFaceAbi  = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_FACE);
  ASSERT_NE(aSolidAbi.bits, 0u);
  ASSERT_NE(aFaceAbi.bits, 0u);

  ::occtl_topo_edge_blend_options_t anOpts = OCCTL_TOPO_EDGE_BLEND_OPTIONS_INIT;
  anOpts.root                              = aSolidAbi;
  anOpts.edges                             = &aFaceAbi;
  anOpts.edge_count                        = 1;

  ::occtl_graph_t*       aOutGraph = nullptr;
  ::occtl_node_id_t      aOutRoot{};
  const ::occtl_status_t aStatus =
    ::occtl_topo_blend_edges(myGraph->get(), &anOpts, &aOutGraph, &aOutRoot);
  EXPECT_EQ(aStatus, OCCTL_WRONG_KIND);
  if (aOutGraph != nullptr)
  {
    ::occtl_graph_free(aOutGraph);
  }
}

TEST_F(TopoVeneerFixture, DraftFacesWithHistory_WrongKindSelection_ThrowsOcctlError)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  const ::occtl_node_id_t anEdgeAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_EDGE);
  ASSERT_NE(aSolidAbi.bits, 0u);
  ASSERT_NE(anEdgeAbi.bits, 0u);

  ::occtl_topo_draft_faces_options_t anOpts = OCCTL_TOPO_DRAFT_FACES_OPTIONS_INIT;
  anOpts.root                               = aSolidAbi;
  anOpts.faces                              = &anEdgeAbi;
  anOpts.face_count                         = 1;
  anOpts.angle                              = 0.05;

  EXPECT_THROW(myGraph->draft_faces(anOpts), occtl::Error);
}

TEST_F(TopoVeneerFixture, RemoveFeatures_WrongKindSelection_ThrowsOcctlError)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  const ::occtl_node_id_t anEdgeAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_EDGE);
  ASSERT_NE(aSolidAbi.bits, 0u);
  ASSERT_NE(anEdgeAbi.bits, 0u);

  EXPECT_THROW(myGraph->remove_features(occtl::NodeId(aSolidAbi),
                                        std::vector<occtl::NodeId>{occtl::NodeId(anEdgeAbi)}),
               occtl::Error);
}

TEST_F(TopoVeneerFixture, OffsetFeatures_WrongKindSelection_ThrowsOcctlError)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  const ::occtl_node_id_t anEdgeAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_EDGE);
  ASSERT_NE(aSolidAbi.bits, 0u);
  ASSERT_NE(anEdgeAbi.bits, 0u);

  EXPECT_THROW(myGraph->offset_features(occtl::NodeId(aSolidAbi),
                                        std::vector<occtl::NodeId>{occtl::NodeId(anEdgeAbi)},
                                        0.5),
               occtl::Error);
}

TEST_F(TopoVeneerFixture, OffsetFeaturesWithHistory_WrongKindSelection_ThrowsOcctlError)
{
  const ::occtl_node_id_t aSolidAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  const ::occtl_node_id_t anEdgeAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_EDGE);
  ASSERT_NE(aSolidAbi.bits, 0u);
  ASSERT_NE(anEdgeAbi.bits, 0u);

  ::occtl_topo_offset_features_options_t anOpts = OCCTL_TOPO_OFFSET_FEATURES_OPTIONS_INIT;
  anOpts.root                                   = aSolidAbi;
  anOpts.selections                             = &anEdgeAbi;
  anOpts.selection_count                        = 1;

  EXPECT_THROW(myGraph->offset_features(anOpts), occtl::Error);
}

TEST_F(TopoVeneerFixture, MakeFillingWithHistory_WrongKindSelection_ThrowsOcctlError)
{
  const ::occtl_node_id_t aFaceAbi = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_FACE);
  ASSERT_NE(aFaceAbi.bits, 0u);

  const ::occtl_node_id_t        anEdges[] = {aFaceAbi, aFaceAbi};
  ::occtl_topo_filling_options_t anOpts    = OCCTL_TOPO_FILLING_OPTIONS_INIT;
  anOpts.edges                             = anEdges;
  anOpts.edge_count                        = 2;

  EXPECT_THROW(myGraph->make_filling(anOpts), occtl::Error);
}

TEST_F(TopoVeneerFixture, MakeFillingPatch_EmptyConstraints_ThrowsOcctlError)
{
  ::occtl_topo_filling_patch_options_t anOpts = OCCTL_TOPO_FILLING_PATCH_OPTIONS_INIT;

  EXPECT_THROW(myGraph->make_filling_patch(anOpts), occtl::Error);
}

TEST_F(TopoVeneerFixture, ProjectFaceAlongDirection_InvalidDirection_ThrowsOcctlError)
{
  ::occtl_topo_project_face_direction_options_t anOpts =
    OCCTL_TOPO_PROJECT_FACE_DIRECTION_OPTIONS_INIT;
  anOpts.direction = {0.0, 0.0, 0.0};

  EXPECT_THROW(myGraph->project_face_along_direction(anOpts), occtl::Error);
}

TEST_F(TopoVeneerFixture, FaceToArcs_InvalidTolerance_ThrowsOcctlError)
{
  ::occtl_topo_face_to_arcs_options_t anOpts = OCCTL_TOPO_FACE_TO_ARCS_OPTIONS_INIT;
  anOpts.angular_tolerance                   = 0.0;

  EXPECT_THROW(myGraph->face_to_arcs(anOpts), occtl::Error);
}

TEST_F(TopoVeneerFixture, HlrProject_Box_ReturnsGraphAndRoots)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  ::occtl_topo_hlr_options_t anOpts = OCCTL_TOPO_HLR_OPTIONS_INIT;
  anOpts.root                       = aSolid;
  anOpts.include_hidden             = 0;

  auto [aGraph, aRoots] = myGraph->hlr_project(anOpts);
  EXPECT_GT(aGraph.edge_count(), size_t{0});
  EXPECT_TRUE(aRoots.visible_sharp.is_valid() || aRoots.visible_outline.is_valid());
  EXPECT_FALSE(aRoots.hidden_sharp.is_valid());
}

#ifdef OCCTL_HAS_MESH
TEST_F(TopoVeneerFixture, HlrProject_PolyMode_ReturnsGraphAndRoots)
{
  const occtl_node_id_t aSolid = firstAbiNodeOfKind(myGraph->get(), OCCTL_KIND_SOLID);
  ASSERT_NE(aSolid.bits, 0u);

  ::occtl_topo_hlr_options_t anOpts = OCCTL_TOPO_HLR_OPTIONS_INIT;
  anOpts.root                       = aSolid;
  anOpts.include_hidden             = 0;
  anOpts.mode                       = OCCTL_TOPO_HLR_POLY;

  ::occtl_mesh_options_t aMeshOpts = OCCTL_MESH_OPTIONS_INIT;
  ASSERT_EQ(::occtl_mesh_generate(myGraph->get(), &aSolid, 1, &aMeshOpts), OCCTL_OK);

  auto [aGraph, aRoots] = myGraph->hlr_project(anOpts);
  EXPECT_GT(aGraph.edge_count(), size_t{0});
  EXPECT_TRUE(aRoots.visible_sharp.is_valid() || aRoots.visible_outline.is_valid());
  EXPECT_FALSE(aRoots.hidden_sharp.is_valid());
}
#endif

TEST_F(TopoVeneerFixture, WrapOnFace_InvalidTolerance_ThrowsOcctlError)
{
  ::occtl_topo_wrap_on_face_options_t anOpts = OCCTL_TOPO_WRAP_ON_FACE_OPTIONS_INIT;
  anOpts.tolerance                           = 0.0;

  EXPECT_THROW(myGraph->wrap_on_face(anOpts), occtl::Error);
}

} // namespace
