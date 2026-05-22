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

#include <occtl-hpp/prim.hpp>

#include <cstring>

namespace
{

class PrimSketchesTest : public ::testing::Test
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

// ---- polyline ----

TEST_F(PrimSketchesTest, Polyline_OpenThreePoints_CreatesWire)
{
  const occtl_point3_t       aPts[3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
  occtl_prim_polyline_info_t anInfo  = OCCTL_PRIM_POLYLINE_INFO_INIT;
  anInfo.points                      = aPts;
  anInfo.point_count                 = 3;
  anInfo.closed                      = 0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_polyline(myGraph, &anInfo, &aWire), OCCTL_OK);
  ASSERT_NE(aWire.bits, 0u);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aWire, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_WIRE);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_VERTEX), 3u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 2u);
}

TEST_F(PrimSketchesTest, Polyline_ClosedTriangle_HasThreeEdges)
{
  const occtl_point3_t       aPts[3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  occtl_prim_polyline_info_t anInfo  = OCCTL_PRIM_POLYLINE_INFO_INIT;
  anInfo.points                      = aPts;
  anInfo.point_count                 = 3;
  anInfo.closed                      = 1;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_polyline(myGraph, &anInfo, &aWire), OCCTL_OK);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 3u);
}

TEST_F(PrimSketchesTest, Polyline_NullPointsArray_InvalidArgument)
{
  occtl_prim_polyline_info_t anInfo = OCCTL_PRIM_POLYLINE_INFO_INIT;
  anInfo.points                     = nullptr;
  anInfo.point_count                = 0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_polyline(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimSketchesTest, Polyline_VersionMismatch_Rejected)
{
  const occtl_point3_t       aPts[2] = {{0, 0, 0}, {1, 0, 0}};
  occtl_prim_polyline_info_t anInfo  = OCCTL_PRIM_POLYLINE_INFO_INIT;
  anInfo.struct_version              = 0u;
  anInfo.points                      = aPts;
  anInfo.point_count                 = 2;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_polyline(myGraph, &anInfo, &aWire), OCCTL_VERSION_MISMATCH);
}

// ---- regular polygon ----

TEST_F(PrimSketchesTest, RegularPolygon_Hexagon_HasSixEdges)
{
  occtl_prim_regular_polygon_info_t anInfo = OCCTL_PRIM_REGULAR_POLYGON_INFO_INIT;
  anInfo.sides                             = 6;
  anInfo.circumradius                      = 1.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_regular_polygon(myGraph, &anInfo, &aWire), OCCTL_OK);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 6u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_VERTEX), 6u);
}

TEST_F(PrimSketchesTest, RegularPolygon_FewerThanThreeSides_InvalidArgument)
{
  occtl_prim_regular_polygon_info_t anInfo = OCCTL_PRIM_REGULAR_POLYGON_INFO_INIT;
  anInfo.sides                             = 2;
  anInfo.circumradius                      = 1.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_regular_polygon(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
}

TEST_F(PrimSketchesTest, RegularPolygon_ZeroRadius_InvalidArgument)
{
  occtl_prim_regular_polygon_info_t anInfo = OCCTL_PRIM_REGULAR_POLYGON_INFO_INIT;
  anInfo.sides                             = 5;
  anInfo.circumradius                      = 0.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_regular_polygon(myGraph, &anInfo, &aWire), OCCTL_INVALID_ARGUMENT);
}

// ---- rectangle ----

TEST_F(PrimSketchesTest, Rectangle_AxisAligned_FourEdgesFourVertices)
{
  occtl_prim_rectangle_info_t anInfo = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  anInfo.width                       = 4.0;
  anInfo.height                      = 2.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &anInfo, &aWire), OCCTL_OK);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 4u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_VERTEX), 4u);
}

TEST_F(PrimSketchesTest, Rectangle_NegativeWidth_GeometryInvalid)
{
  occtl_prim_rectangle_info_t anInfo = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  anInfo.width                       = -1.0;
  anInfo.height                      = 2.0;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_rectangle(myGraph, &anInfo, &aWire), OCCTL_GEOMETRY_INVALID);

  const occtl_error_t* anErr = occtl_error_last();
  EXPECT_NE(anErr->message, nullptr);
  EXPECT_GT(std::strlen(anErr->message), 0u);
}

// ---- circle ----

TEST_F(PrimSketchesTest, Circle_FullCircle_SingleEdge)
{
  occtl_prim_circle_info_t anInfo = OCCTL_PRIM_CIRCLE_INFO_INIT;
  anInfo.radius                   = 2.5;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_circle(myGraph, &anInfo, &aWire), OCCTL_OK);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
}

TEST_F(PrimSketchesTest, Circle_NegativeRadius_GeometryInvalid)
{
  occtl_prim_circle_info_t anInfo = OCCTL_PRIM_CIRCLE_INFO_INIT;
  anInfo.radius                   = -1.0;
  occtl_node_id_t aWire           = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_circle(myGraph, &anInfo, &aWire), OCCTL_GEOMETRY_INVALID);
}

// ---- ellipse ----

TEST_F(PrimSketchesTest, Ellipse_Full_SingleEdge)
{
  occtl_prim_ellipse_info_t anInfo = OCCTL_PRIM_ELLIPSE_INFO_INIT;
  anInfo.major                     = 3.0;
  anInfo.minor                     = 1.5;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_ellipse(myGraph, &anInfo, &aWire), OCCTL_OK);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
}

TEST_F(PrimSketchesTest, Ellipse_MinorGreaterThanMajor_GeometryInvalid)
{
  occtl_prim_ellipse_info_t anInfo = OCCTL_PRIM_ELLIPSE_INFO_INIT;
  anInfo.major                     = 1.0;
  anInfo.minor                     = 2.0;
  occtl_node_id_t aWire            = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_ellipse(myGraph, &anInfo, &aWire), OCCTL_GEOMETRY_INVALID);
}

// ---- planar face ----

TEST_F(PrimSketchesTest, PlanarFace_FromRectangleWire_HasOneFace)
{
  occtl_prim_rectangle_info_t aRectInfo = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRectInfo.width                       = 4.0;
  aRectInfo.height                      = 2.0;
  occtl_node_id_t aWire                 = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRectInfo, &aWire), OCCTL_OK);

  occtl_prim_planar_face_info_t anInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  anInfo.outer_wire                    = aWire;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &anInfo, &aFace), OCCTL_OK);

  occtl_node_kind_t aKind;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(PrimSketchesTest, PlanarFace_WithHole_PreservesInnerWires)
{
  // Outer 10x10 rectangle, inner circle hole at the centre.
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 10.0;
  aRect.height                      = 10.0;
  occtl_node_id_t anOuter           = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &anOuter), OCCTL_OK);

  occtl_geom_circle_t aCircGeom;
  aCircGeom.position        = {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
  aCircGeom.radius          = 1.0;
  occtl_rep_id_t aCircCurve = {};
  ASSERT_EQ(occtl_curve_create_circle(myGraph, aCircGeom, &aCircCurve), OCCTL_OK);
  occtl_node_id_t anInner = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_topo_curves_to_wire(myGraph, &aCircCurve, 1, &anInner), OCCTL_OK);

  occtl_prim_planar_face_info_t anInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  anInfo.outer_wire                    = anOuter;
  anInfo.inner_wires                   = &anInner;
  anInfo.inner_wire_count              = 1;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &anInfo, &aFace), OCCTL_OK);
  EXPECT_NE(aFace.bits, 0u);
}

TEST_F(PrimSketchesTest, PlanarFace_NonWireOuter_WrongKind)
{
  // Build a face, then attempt to use it as the outer wire.
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 1.0;
  aRect.height                      = 1.0;
  occtl_node_id_t aWire             = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aWire), OCCTL_OK);
  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = aWire;
  occtl_node_id_t aFace                   = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &aFace), OCCTL_OK);

  // Now feed a Face as the outer wire — should reject.
  occtl_prim_planar_face_info_t aBad = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aBad.outer_wire                    = aFace;
  occtl_node_id_t aResult            = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_planar_face(myGraph, &aBad, &aResult), OCCTL_WRONG_KIND);
}

TEST_F(PrimSketchesTest, PlanarFace_InvalidOuter_NotFound)
{
  occtl_prim_planar_face_info_t anInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  anInfo.outer_wire                    = OCCTL_NODE_ID_INVALID;
  occtl_node_id_t aFace                = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_planar_face(myGraph, &anInfo, &aFace), OCCTL_NOT_FOUND);
}

TEST_F(PrimSketchesTest, Plane_RectangularFace)
{
  occtl_prim_plane_info_t anInfo = OCCTL_PRIM_PLANE_INFO_INIT;
  anInfo.width                   = 4.0;
  anInfo.height                  = 2.0;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_plane(myGraph, &anInfo, &aFace), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(PrimSketchesTest, Disk_CircularFace)
{
  occtl_prim_disk_info_t anInfo = OCCTL_PRIM_DISK_INFO_INIT;
  anInfo.radius                 = 1.0;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_disk(myGraph, &anInfo, &aFace), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

// ---- convex hull 2d ----

TEST_F(PrimSketchesTest, ConvexHull2d_PointsWithInteriorPoint_CreatesWire)
{
  const occtl_point3_t aPts[5] = {{0.0, 0.0, 0.0},
                                  {2.0, 0.0, 0.0},
                                  {2.0, 1.0, 0.0},
                                  {0.0, 1.0, 0.0},
                                  {1.0, 0.5, 0.0}};

  occtl_prim_convex_hull_2d_info_t anInfo = OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT;
  anInfo.points                           = aPts;
  anInfo.point_count                      = 5;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_convex_hull_2d(myGraph, &anInfo, &aWire), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aWire, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_WIRE);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 4u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_VERTEX), 4u);
}

TEST_F(PrimSketchesTest, ConvexHull2d_VertexInputs_CreatesFace)
{
  const occtl_point3_t aPts[4]      = {{0.0, 0.0, 0.0},
                                       {3.0, 0.0, 0.0},
                                       {0.0, 2.0, 0.0},
                                       {1.0, 0.5, 0.0}};
  occtl_node_id_t      aVertices[4] = {};
  for (size_t anIdx = 0; anIdx < 4; ++anIdx)
  {
    occtl_topo_make_vertex_info_t aVertexInfo = OCCTL_TOPO_MAKE_VERTEX_INFO_INIT;
    aVertexInfo.point                         = aPts[anIdx];
    ASSERT_EQ(occtl_topo_make_vertex(myGraph, &aVertexInfo, &aVertices[anIdx]), OCCTL_OK);
  }

  occtl_prim_convex_hull_2d_info_t anInfo = OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT;
  anInfo.vertices                         = aVertices;
  anInfo.vertex_count                     = 4;
  anInfo.make_face                        = 1;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_convex_hull_2d(myGraph, &anInfo, &aFace), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
  EXPECT_GE(countOfKind(myGraph, OCCTL_KIND_EDGE), 3u);
}

TEST_F(PrimSketchesTest, ConvexHull2dInfoInit_HasDefaults)
{
  occtl_prim_convex_hull_2d_info_t anInfo;
  occtl_prim_convex_hull_2d_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_CONVEX_HULL_2D_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.points, nullptr);
  EXPECT_EQ(anInfo.point_count, 0u);
  EXPECT_EQ(anInfo.vertices, nullptr);
  EXPECT_EQ(anInfo.vertex_count, 0u);
  EXPECT_DOUBLE_EQ(anInfo.tolerance, 1.0e-7);
  EXPECT_EQ(anInfo.make_face, 0);
}

TEST_F(PrimSketchesTest, ConvexHull2d_BadArguments_ReturnExpectedStatuses)
{
  const occtl_point3_t             aPts[3] = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  occtl_prim_convex_hull_2d_info_t anInfo  = OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT;
  anInfo.points                            = aPts;
  anInfo.point_count                       = 3;
  occtl_node_id_t aNode                    = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_convex_hull_2d(nullptr, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_convex_hull_2d(myGraph, nullptr, &aNode), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_convex_hull_2d(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);

  anInfo.struct_version = 0u;
  EXPECT_EQ(occtl_prim_make_convex_hull_2d(myGraph, &anInfo, &aNode), OCCTL_VERSION_MISMATCH);

  anInfo = OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT;
  EXPECT_EQ(occtl_prim_make_convex_hull_2d(myGraph, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);

  anInfo.points      = aPts;
  anInfo.point_count = 3;
  anInfo.tolerance   = 0.0;
  EXPECT_EQ(occtl_prim_make_convex_hull_2d(myGraph, &anInfo, &aNode), OCCTL_INVALID_ARGUMENT);

  const occtl_point3_t aLinePts[3] = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}};
  anInfo                           = OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT;
  anInfo.points                    = aLinePts;
  anInfo.point_count               = 3;
  EXPECT_EQ(occtl_prim_make_convex_hull_2d(myGraph, &anInfo, &aNode), OCCTL_GEOMETRY_INVALID);
}

TEST_F(PrimSketchesTest, ConvexHull2d_NonVertexInput_ReturnsWrongKind)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 1.0;
  aRect.height                      = 1.0;
  occtl_node_id_t aWire             = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aWire), OCCTL_OK);

  const occtl_point3_t             aPts[3] = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  occtl_prim_convex_hull_2d_info_t anInfo  = OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT;
  anInfo.points                            = aPts;
  anInfo.point_count                       = 3;
  anInfo.vertices                          = &aWire;
  anInfo.vertex_count                      = 1;

  occtl_node_id_t aNode = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_convex_hull_2d(myGraph, &anInfo, &aNode), OCCTL_WRONG_KIND);
}

TEST(PrimConvexHull2dVeneerTest, MakeConvexHull2d_Points_ReturnsFace)
{
  occtl::Graph                      aGraph;
  const std::vector<occtl_point3_t> aPts = {{0.0, 0.0, 0.0},
                                            {2.0, 0.0, 0.0},
                                            {0.0, 1.0, 0.0},
                                            {0.5, 0.25, 0.0}};

  occtl::prim::ConvexHull2dOptions anOptions;
  anOptions.make_face       = true;
  const occtl::NodeId aFace = occtl::prim::make_convex_hull_2d(aGraph, aPts, {}, anOptions);

  EXPECT_EQ(aGraph.node_id_kind(aFace), OCCTL_KIND_FACE);
}

// ---- trace ----

TEST_F(PrimSketchesTest, Trace_OpenPolylineWire_CreatesFace)
{
  const occtl_point3_t       aPts[3] = {{0, 0, 0}, {3, 0, 0}, {3, 2, 0}};
  occtl_prim_polyline_info_t aLine   = OCCTL_PRIM_POLYLINE_INFO_INIT;
  aLine.points                       = aPts;
  aLine.point_count                  = 3;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_polyline(myGraph, &aLine, &aWire), OCCTL_OK);

  occtl_prim_trace_info_t anInfo = OCCTL_PRIM_TRACE_INFO_INIT;
  anInfo.path                    = aWire;
  anInfo.width                   = 0.4;
  anInfo.join                    = OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION;
  anInfo.approximate             = 1;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_trace(myGraph, &anInfo, &aFace), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
  EXPECT_GE(countOfKind(myGraph, OCCTL_KIND_EDGE), 6u);
}

TEST_F(PrimSketchesTest, Trace_EdgePath_CreatesFace)
{
  const occtl_point3_t       aPts[2] = {{0, 0, 0}, {4, 0, 0}};
  occtl_prim_polyline_info_t aLine   = OCCTL_PRIM_POLYLINE_INFO_INIT;
  aLine.points                       = aPts;
  aLine.point_count                  = 2;

  occtl_node_id_t aWire = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_polyline(myGraph, &aLine, &aWire), OCCTL_OK);

  const occtl_node_id_t anEdge = firstNodeOfKind(myGraph, OCCTL_KIND_EDGE);
  ASSERT_NE(anEdge.bits, 0u);

  occtl_prim_trace_info_t anInfo = OCCTL_PRIM_TRACE_INFO_INIT;
  anInfo.path                    = anEdge;
  anInfo.width                   = 0.5;

  occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_trace(myGraph, &anInfo, &aFace), OCCTL_OK);

  occtl_node_kind_t aKind = OCCTL_KIND_INVALID;
  ASSERT_EQ(occtl_graph_node_kind(myGraph, aFace, &aKind), OCCTL_OK);
  EXPECT_EQ(aKind, OCCTL_KIND_FACE);
}

TEST_F(PrimSketchesTest, TraceInfoInit_HasDefaults)
{
  occtl_prim_trace_info_t anInfo;
  occtl_prim_trace_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_TRACE_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.path.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_DOUBLE_EQ(anInfo.width, 1.0);
  EXPECT_DOUBLE_EQ(anInfo.normal.x, 0.0);
  EXPECT_DOUBLE_EQ(anInfo.normal.y, 0.0);
  EXPECT_DOUBLE_EQ(anInfo.normal.z, 1.0);
  EXPECT_EQ(anInfo.join, OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC);
  EXPECT_EQ(anInfo.approximate, 0);
}

TEST_F(PrimSketchesTest, Trace_BadArguments_ReturnExpectedStatuses)
{
  occtl_prim_trace_info_t anInfo = OCCTL_PRIM_TRACE_INFO_INIT;
  occtl_node_id_t         aFace  = OCCTL_NODE_ID_INVALID;

  EXPECT_EQ(occtl_prim_make_trace(nullptr, &anInfo, &aFace), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_trace(myGraph, nullptr, &aFace), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_trace(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);

  anInfo.struct_version = 0u;
  EXPECT_EQ(occtl_prim_make_trace(myGraph, &anInfo, &aFace), OCCTL_VERSION_MISMATCH);

  anInfo       = OCCTL_PRIM_TRACE_INFO_INIT;
  anInfo.width = -1.0;
  EXPECT_EQ(occtl_prim_make_trace(myGraph, &anInfo, &aFace), OCCTL_INVALID_ARGUMENT);

  anInfo      = OCCTL_PRIM_TRACE_INFO_INIT;
  anInfo.join = OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_RESERVED_FUTURE;
  EXPECT_EQ(occtl_prim_make_trace(myGraph, &anInfo, &aFace), OCCTL_INVALID_ARGUMENT);

  anInfo        = OCCTL_PRIM_TRACE_INFO_INIT;
  anInfo.normal = {0.0, 0.0, 0.0};
  EXPECT_EQ(occtl_prim_make_trace(myGraph, &anInfo, &aFace), OCCTL_INVALID_ARGUMENT);

  anInfo = OCCTL_PRIM_TRACE_INFO_INIT;
  EXPECT_EQ(occtl_prim_make_trace(myGraph, &anInfo, &aFace), OCCTL_NOT_FOUND);
}

TEST_F(PrimSketchesTest, Trace_WrongKind_ReturnsWrongKind)
{
  occtl_prim_rectangle_info_t aRect = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  aRect.width                       = 1.0;
  aRect.height                      = 1.0;
  occtl_node_id_t aWire             = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_rectangle(myGraph, &aRect, &aWire), OCCTL_OK);

  occtl_prim_planar_face_info_t aFaceInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  aFaceInfo.outer_wire                    = aWire;
  occtl_node_id_t aFace                   = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_planar_face(myGraph, &aFaceInfo, &aFace), OCCTL_OK);

  occtl_prim_trace_info_t anInfo = OCCTL_PRIM_TRACE_INFO_INIT;
  anInfo.path                    = aFace;

  occtl_node_id_t aTrace = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_trace(myGraph, &anInfo, &aTrace), OCCTL_WRONG_KIND);
}

TEST(PrimTraceVeneerTest, MakeTrace_OpenPolyline_ReturnsFace)
{
  occtl::Graph                      aGraph;
  const std::vector<occtl_point3_t> aPts  = {{0, 0, 0}, {2, 0, 0}, {2, 1, 0}};
  const occtl::NodeId               aWire = occtl::prim::make_polyline(aGraph, aPts);

  occtl::prim::TraceOptions anOptions;
  anOptions.join            = OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_INTERSECTION;
  const occtl::NodeId aFace = occtl::prim::make_trace(aGraph, aWire, 0.25, anOptions);

  EXPECT_EQ(aGraph.node_id_kind(aFace), OCCTL_KIND_FACE);
}

// ---- constrained curve edge ----

occtl_axis2_placement2d_t standardAxis2d()
{
  return {{0.0, 0.0}, {1.0, 0.0}};
}

TEST_F(PrimSketchesTest, ConstrainedCurveEdgeInfoInit_HasDefaults)
{
  occtl_prim_constrained_edge_info_t anInfo;
  occtl_prim_constrained_edge_info_init(&anInfo);

  EXPECT_EQ(anInfo.struct_version, OCCTL_PRIM_CONSTRAINED_EDGE_INFO_VERSION_1);
  EXPECT_EQ(anInfo.p_next, nullptr);
  EXPECT_EQ(anInfo.curve.bits, 0u);
  EXPECT_EQ(anInfo.use_parameter_range, 0);
  EXPECT_DOUBLE_EQ(anInfo.first_parameter, 0.0);
  EXPECT_DOUBLE_EQ(anInfo.last_parameter, 0.0);
}

TEST_F(PrimSketchesTest, ConstrainedCurveEdge_LineWithRange_CreatesEdge)
{
  const occtl_geom2d_line_t aLine  = {standardAxis2d()};
  occtl_rep_id_t            aCurve = {};
  ASSERT_EQ(occtl_curve2d_create_line(myGraph, aLine, &aCurve), OCCTL_OK);

  occtl_prim_constrained_edge_info_t anInfo = OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT;
  anInfo.curve                              = aCurve;
  anInfo.use_parameter_range                = 1;
  anInfo.first_parameter                    = 0.0;
  anInfo.last_parameter                     = 5.0;

  occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_constrained_edge(myGraph, &anInfo, &anEdge), OCCTL_OK);
  EXPECT_NE(anEdge.bits, OCCTL_NODE_ID_INVALID.bits);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_VERTEX), 2u);
}

TEST_F(PrimSketchesTest, ConstrainedCurveEdge_BlendArcSolution_CreatesEdge)
{
  const occtl_geom2d_line_t aLineX = {standardAxis2d()};
  const occtl_geom2d_line_t aLineY = {{{0.0, 0.0}, {0.0, 1.0}}};

  occtl_rep_id_t aCurveX = {};
  occtl_rep_id_t aCurveY = {};
  ASSERT_EQ(occtl_curve2d_create_line(myGraph, aLineX, &aCurveX), OCCTL_OK);
  ASSERT_EQ(occtl_curve2d_create_line(myGraph, aLineY, &aCurveY), OCCTL_OK);

  occtl_curve2d_blend_arc_info_t aBlend = OCCTL_CURVE2D_BLEND_ARC_INFO_INIT;
  aBlend.curve_a                        = aCurveX;
  aBlend.curve_b                        = aCurveY;
  aBlend.radius                         = 1.0;

  size_t aCount = 0;
  ASSERT_EQ(occtl_curve2d_create_blend_arc(myGraph, &aBlend, 0, nullptr, &aCount), OCCTL_OK);
  ASSERT_GT(aCount, 0u);

  occtl_rep_id_t anArc = {};
  ASSERT_EQ(occtl_curve2d_create_blend_arc(myGraph, &aBlend, 0, &anArc, &aCount), OCCTL_OK);

  occtl_prim_constrained_edge_info_t anInfo = OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT;
  anInfo.curve                              = anArc;

  occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
  ASSERT_EQ(occtl_prim_make_constrained_edge(myGraph, &anInfo, &anEdge), OCCTL_OK);
  EXPECT_EQ(countOfKind(myGraph, OCCTL_KIND_EDGE), 1u);
}

TEST_F(PrimSketchesTest, ConstrainedCurveEdge_BadArguments_ReturnExpectedStatuses)
{
  const occtl_geom2d_line_t aLine  = {standardAxis2d()};
  occtl_rep_id_t            aCurve = {};
  ASSERT_EQ(occtl_curve2d_create_line(myGraph, aLine, &aCurve), OCCTL_OK);

  occtl_prim_constrained_edge_info_t anInfo = OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT;
  anInfo.curve                              = aCurve;
  anInfo.use_parameter_range                = 1;
  anInfo.first_parameter                    = 0.0;
  anInfo.last_parameter                     = 1.0;

  occtl_node_id_t anEdge = OCCTL_NODE_ID_INVALID;
  EXPECT_EQ(occtl_prim_make_constrained_edge(nullptr, &anInfo, &anEdge), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_constrained_edge(myGraph, nullptr, &anEdge), OCCTL_INVALID_ARGUMENT);
  EXPECT_EQ(occtl_prim_make_constrained_edge(myGraph, &anInfo, nullptr), OCCTL_INVALID_ARGUMENT);

  anInfo.struct_version = 0u;
  EXPECT_EQ(occtl_prim_make_constrained_edge(myGraph, &anInfo, &anEdge), OCCTL_VERSION_MISMATCH);

  anInfo       = OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT;
  anInfo.curve = {};
  EXPECT_EQ(occtl_prim_make_constrained_edge(myGraph, &anInfo, &anEdge), OCCTL_INVALID_ARGUMENT);

  anInfo                     = OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT;
  anInfo.curve               = aCurve;
  anInfo.use_parameter_range = 1;
  anInfo.first_parameter     = 2.0;
  anInfo.last_parameter      = 1.0;
  EXPECT_EQ(occtl_prim_make_constrained_edge(myGraph, &anInfo, &anEdge), OCCTL_INVALID_ARGUMENT);

  anInfo                     = OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT;
  anInfo.curve               = aCurve;
  anInfo.placement.x_dir_ref = {0.0, 0.0, 2.0};
  EXPECT_EQ(occtl_prim_make_constrained_edge(myGraph, &anInfo, &anEdge), OCCTL_INVALID_ARGUMENT);
}

TEST(PrimConstrainedCurveEdgeVeneerTest, MakeConstrainedCurveEdge_Line_ReturnsEdge)
{
  occtl::Graph         aGraph;
  const occtl::Curve2d aCurve = occtl::Curve2d::from_line(aGraph.get(), {standardAxis2d()});

  occtl::prim::ConstrainedCurveEdgeOptions anOptions;
  anOptions.use_parameter_range = true;
  anOptions.first_parameter     = 0.0;
  anOptions.last_parameter      = 3.0;

  const occtl::NodeId anEdge = occtl::prim::make_constrained_curve_edge(aGraph, aCurve, anOptions);
  EXPECT_EQ(aGraph.node_id_kind(anEdge), OCCTL_KIND_EDGE);
}

TEST(PrimSketchVeneerTest, RestoredSketchBuilders_ReturnExpectedKinds)
{
  occtl::Graph aGraph;

  const occtl::NodeId aCircle = occtl::prim::make_circle(aGraph, 2.0);
  EXPECT_EQ(aGraph.node_id_kind(aCircle), OCCTL_KIND_WIRE);

  const occtl::NodeId anEllipse = occtl::prim::make_ellipse(aGraph, 3.0, 1.0);
  EXPECT_EQ(aGraph.node_id_kind(anEllipse), OCCTL_KIND_WIRE);

  const occtl::NodeId anArc3Pt =
    occtl::prim::make_arc_3pt(aGraph, {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {2.0, 0.0, 0.0});
  EXPECT_EQ(aGraph.node_id_kind(anArc3Pt), OCCTL_KIND_WIRE);

  const occtl::NodeId anArcCenter = occtl::prim::make_arc_center(aGraph, 1.0, 0.0, OCCTL_PI);
  EXPECT_EQ(aGraph.node_id_kind(anArcCenter), OCCTL_KIND_WIRE);

  const std::vector<occtl_point3_t> aPoints = {{0.0, 0.0, 0.0}, {1.0, 0.5, 0.0}, {2.0, 0.0, 0.0}};
  const occtl::NodeId               aSpline = occtl::prim::make_spline(aGraph, aPoints);
  EXPECT_EQ(aGraph.node_id_kind(aSpline), OCCTL_KIND_WIRE);

  const occtl::NodeId aPlane = occtl::prim::make_plane(aGraph, 4.0, 2.0);
  EXPECT_EQ(aGraph.node_id_kind(aPlane), OCCTL_KIND_FACE);

  const occtl::NodeId aDisk = occtl::prim::make_disk(aGraph, 2.0);
  EXPECT_EQ(aGraph.node_id_kind(aDisk), OCCTL_KIND_FACE);
}

} // namespace
