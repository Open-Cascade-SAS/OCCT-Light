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

/**
 * @file
 * @brief C++ veneer for the prim module.
 *
 * Provides builder helpers that mirror C info structs as PascalCase PODs and
 * return freshly built NodeId values. Failures translate to occtl::Error.
 */

#ifndef OCCTL_HPP_PRIM_HPP
#define OCCTL_HPP_PRIM_HPP

#include <occtl/occtl_prim.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/curves2d.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

namespace occtl::prim
{

/// @brief Options for building a Face from a surface rep.
using FaceFromSurfaceOptions = ::occtl_prim_face_from_surface_options_t;

/// @brief Options for building a Face from a point-grid surface.
using FaceFromPointGridOptions = ::occtl_prim_face_from_point_grid_options_t;

/// @brief Options for building a Face from boundary-curve surface filling.
using FaceFromBoundaryCurvesOptions = ::occtl_prim_face_from_boundary_curves_options_t;

/// @brief Options for building a Face from a curve-grid Gordon surface.
using FaceFromCurveGridOptions = ::occtl_prim_face_from_curve_grid_options_t;

/// @brief Brake-forming offset side.
using BrakeSide = ::occtl_prim_brake_side_t;

/// @brief Options for sheet-metal brake-formed solids.
using BrakeFormedOptions = ::occtl_prim_brake_formed_options_t;

namespace detail
{
/// @brief Default axis2 placement: origin + Z (main) + X.  Matches OCCT's @c gp::XOY().
inline ::occtl_axis2_placement_t default_ax2() noexcept
{
  return {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
}
} // namespace detail

/// @brief Builds a Box and returns its Solid NodeId.  Throws on failure.
/// @param[in,out] theGraph     graph receiving the box
/// @param[in]     theDx        edge size along X
/// @param[in]     theDy        edge size along Y
/// @param[in]     theDz        edge size along Z
/// @param[in]     thePlacement local frame; defaults to XOY
inline NodeId make_box(Graph&                           theGraph,
                       const double                     theDx,
                       const double                     theDy,
                       const double                     theDz,
                       const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_box_info_t anInfo = OCCTL_PRIM_BOX_INFO_INIT;
  anInfo.placement               = thePlacement;
  anInfo.dx                      = theDx;
  anInfo.dy                      = theDy;
  anInfo.dz                      = theDz;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_box(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a Face from a surface handle and optional boundary wires.
inline NodeId make_face_from_surface(Graph& theGraph, const FaceFromSurfaceOptions& theOptions)
{
  ::occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_face_from_surface(theGraph.get(), &theOptions, &aFace));
  return NodeId(aFace);
}

/// @brief Builds a Face directly from a point-grid B-spline surface.
inline NodeId make_face_from_point_grid(Graph& theGraph, const FaceFromPointGridOptions& theOptions)
{
  ::occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_face_from_point_grid(theGraph.get(), &theOptions, &aFace));
  return NodeId(aFace);
}

/// @brief Builds a Face directly from boundary-curve surface filling.
inline NodeId make_face_from_boundary_curves(Graph&                               theGraph,
                                             const FaceFromBoundaryCurvesOptions& theOptions)
{
  ::occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_face_from_boundary_curves(theGraph.get(), &theOptions, &aFace));
  return NodeId(aFace);
}

/// @brief Builds a Face directly from a curve-grid Gordon surface.
inline NodeId make_face_from_curve_grid(Graph& theGraph, const FaceFromCurveGridOptions& theOptions)
{
  ::occtl_node_id_t aFace = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_face_from_curve_grid(theGraph.get(), &theOptions, &aFace));
  return NodeId(aFace);
}

/// @brief Builds a sheet-metal brake-formed solid from a planar bend line.
inline std::pair<Graph, NodeId> make_brake_formed(const Graph&              theGraph,
                                                  const BrakeFormedOptions& theOptions)
{
  ::occtl_graph_t*  aOutGraph = nullptr;
  ::occtl_node_id_t aOutRoot  = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_brake_formed(theGraph.get(), &theOptions, &aOutGraph, &aOutRoot));
  return {Graph(aOutGraph), NodeId(aOutRoot)};
}

/// @brief Builds a Sphere and returns its Solid NodeId.  Throws on failure.
inline NodeId make_sphere(Graph&                           theGraph,
                          const double                     theRadius,
                          const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_sphere_info_t anInfo = OCCTL_PRIM_SPHERE_INFO_INIT;
  anInfo.placement                  = thePlacement;
  anInfo.radius                     = theRadius;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_sphere(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a Cylinder and returns its Solid NodeId.  Throws on failure.
inline NodeId make_cylinder(Graph&                           theGraph,
                            const double                     theRadius,
                            const double                     theHeight,
                            const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_cylinder_info_t anInfo = OCCTL_PRIM_CYLINDER_INFO_INIT;
  anInfo.placement                    = thePlacement;
  anInfo.radius                       = theRadius;
  anInfo.height                       = theHeight;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_cylinder(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a Cone and returns its Solid NodeId.  Throws on failure.
inline NodeId make_cone(Graph&                           theGraph,
                        const double                     theR1,
                        const double                     theR2,
                        const double                     theHeight,
                        const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_cone_info_t anInfo = OCCTL_PRIM_CONE_INFO_INIT;
  anInfo.placement                = thePlacement;
  anInfo.r1                       = theR1;
  anInfo.r2                       = theR2;
  anInfo.height                   = theHeight;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_cone(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a Torus and returns its Solid NodeId.  Throws on failure.
inline NodeId make_torus(Graph&                           theGraph,
                         const double                     theMajor,
                         const double                     theMinor,
                         const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_torus_info_t anInfo = OCCTL_PRIM_TORUS_INFO_INIT;
  anInfo.placement                 = thePlacement;
  anInfo.r1                        = theMajor;
  anInfo.r2                        = theMinor;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_torus(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a right-angular Wedge and returns its Solid NodeId.  Throws on failure.
inline NodeId make_wedge(Graph&                           theGraph,
                         const double                     theDx,
                         const double                     theDy,
                         const double                     theDz,
                         const double                     theLtx,
                         const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_wedge_info_t anInfo = OCCTL_PRIM_WEDGE_INFO_INIT;
  anInfo.placement                 = thePlacement;
  anInfo.dx                        = theDx;
  anInfo.dy                        = theDy;
  anInfo.dz                        = theDz;
  anInfo.ltx                       = theLtx;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_wedge(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a HalfSpace bounded by @p theFace on the side of @p theRefPoint.
inline NodeId make_halfspace(Graph&                  theGraph,
                             const NodeId&           theFace,
                             const ::occtl_point3_t& theRefPoint)
{
  ::occtl_prim_halfspace_info_t anInfo = OCCTL_PRIM_HALFSPACE_INFO_INIT;
  anInfo.face                          = theFace.get();
  anInfo.reference_point               = theRefPoint;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_halfspace(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a linear extrusion of @p theProfile by @p theDirection.
inline NodeId make_prism(Graph&                   theGraph,
                         const NodeId&            theProfile,
                         const ::occtl_vector3_t& theDirection,
                         const bool               theCopy     = false,
                         const bool               theCanonize = true)
{
  ::occtl_prim_prism_info_t anInfo = OCCTL_PRIM_PRISM_INFO_INIT;
  anInfo.profile                   = theProfile.get();
  anInfo.direction                 = theDirection;
  anInfo.copy                      = theCopy ? 1 : 0;
  anInfo.canonize                  = theCanonize ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_prism(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_twist_extrusion.
struct TwistExtrusionOptions
{
  int    section_count = 9;
  bool   make_solid    = true;
  bool   ruled         = true;
  double pres3d        = 1.0e-6;
};

/// @brief Builds a twisted extrusion of a closed Wire profile.
inline NodeId make_twist_extrusion(Graph&                           theGraph,
                                   const NodeId&                    theProfileWire,
                                   const ::occtl_axis1_placement_t& theAxis,
                                   const double                     theHeight,
                                   const double                     theAngle,
                                   const TwistExtrusionOptions&     theOptions = {})
{
  ::occtl_prim_twist_extrusion_info_t anInfo = OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT;
  anInfo.profile_wire                        = theProfileWire.get();
  anInfo.axis                                = theAxis;
  anInfo.height                              = theHeight;
  anInfo.angle                               = theAngle;
  anInfo.section_count                       = theOptions.section_count;
  anInfo.make_solid                          = theOptions.make_solid ? 1 : 0;
  anInfo.ruled                               = theOptions.ruled ? 1 : 0;
  anInfo.pres3d                              = theOptions.pres3d;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_twist_extrusion(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a twisted extrusion of a closed Wire profile.
inline NodeId make_extrude_twist(Graph&                           theGraph,
                                 const NodeId&                    theProfileWire,
                                 const ::occtl_axis1_placement_t& theAxis,
                                 const double                     theHeight,
                                 const double                     theAngle,
                                 const TwistExtrusionOptions&     theOptions = {})
{
  ::occtl_prim_extrude_twist_info_t anInfo = OCCTL_PRIM_EXTRUDE_TWIST_INFO_INIT;
  anInfo.profile_wire                      = theProfileWire.get();
  anInfo.axis                              = theAxis;
  anInfo.height                            = theHeight;
  anInfo.angle                             = theAngle;
  anInfo.section_count                     = theOptions.section_count;
  anInfo.make_solid                        = theOptions.make_solid ? 1 : 0;
  anInfo.ruled                             = theOptions.ruled ? 1 : 0;
  anInfo.pres3d                            = theOptions.pres3d;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_extrude_twist(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a revolution of @p theProfile around @p theAxis by @p theAngle radians.
inline NodeId make_revol(Graph&                           theGraph,
                         const NodeId&                    theProfile,
                         const ::occtl_axis1_placement_t& theAxis,
                         const double                     theAngle = 6.283185307179586,
                         const bool                       theCopy  = false)
{
  ::occtl_prim_revol_info_t anInfo = OCCTL_PRIM_REVOL_INFO_INIT;
  anInfo.profile                   = theProfile.get();
  anInfo.axis                      = theAxis;
  anInfo.angle                     = theAngle;
  anInfo.copy                      = theCopy ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_revol(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Sweeps @p theProfile along @p theSpineWire.
inline NodeId make_pipe(Graph& theGraph, const NodeId& theProfile, const NodeId& theSpineWire)
{
  ::occtl_prim_pipe_info_t anInfo = OCCTL_PRIM_PIPE_INFO_INIT;
  anInfo.profile                  = theProfile.get();
  anInfo.spine_wire               = theSpineWire.get();

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_pipe(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Lofts a sequence of Wire / Vertex sections into a shell or solid.
inline NodeId make_loft(Graph&                     theGraph,
                        const std::vector<NodeId>& theSections,
                        const bool                 theIsSolid = false,
                        const bool                 theRuled   = false,
                        const double               thePres3d  = 1.0e-6)
{
  std::vector<::occtl_node_id_t> aRaw;
  aRaw.reserve(theSections.size());
  for (const NodeId& aSec : theSections)
  {
    aRaw.push_back(aSec.get());
  }

  ::occtl_prim_loft_info_t anInfo = OCCTL_PRIM_LOFT_INFO_INIT;
  anInfo.sections                 = aRaw.empty() ? nullptr : aRaw.data();
  anInfo.section_count            = aRaw.size();
  anInfo.is_solid                 = theIsSolid ? 1 : 0;
  anInfo.ruled                    = theRuled ? 1 : 0;
  anInfo.pres3d                   = thePres3d;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_loft(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds an open or closed polyline Wire from @p thePoints.
inline NodeId make_polyline(Graph&                               theGraph,
                            const std::vector<::occtl_point3_t>& thePoints,
                            const bool                           theClosed = false)
{
  ::occtl_prim_polyline_info_t anInfo = OCCTL_PRIM_POLYLINE_INFO_INIT;
  anInfo.points                       = thePoints.empty() ? nullptr : thePoints.data();
  anInfo.point_count                  = thePoints.size();
  anInfo.closed                       = theClosed ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_polyline(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a regular-polygon Wire inscribed in a circle.
inline NodeId make_regular_polygon(
  Graph&                           theGraph,
  const int                        theSides,
  const double                     theCircumradius,
  const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2(),
  const double                     theRotation  = 0.0)
{
  ::occtl_prim_regular_polygon_info_t anInfo = OCCTL_PRIM_REGULAR_POLYGON_INFO_INIT;
  anInfo.placement                           = thePlacement;
  anInfo.circumradius                        = theCircumradius;
  anInfo.sides                               = theSides;
  anInfo.rotation                            = theRotation;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_regular_polygon(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a closed rectangle Wire centred on @p thePlacement.
inline NodeId make_rectangle(Graph&                           theGraph,
                             const double                     theWidth,
                             const double                     theHeight,
                             const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_rectangle_info_t anInfo = OCCTL_PRIM_RECTANGLE_INFO_INIT;
  anInfo.placement                     = thePlacement;
  anInfo.width                         = theWidth;
  anInfo.height                        = theHeight;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_rectangle(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a closed circular Wire.
inline NodeId make_circle(Graph&                           theGraph,
                          const double                     theRadius,
                          const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_circle_info_t anInfo = OCCTL_PRIM_CIRCLE_INFO_INIT;
  anInfo.placement                  = thePlacement;
  anInfo.radius                     = theRadius;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_circle(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a closed elliptical Wire.
inline NodeId make_ellipse(Graph&                           theGraph,
                           const double                     theMajor,
                           const double                     theMinor,
                           const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_ellipse_info_t anInfo = OCCTL_PRIM_ELLIPSE_INFO_INIT;
  anInfo.placement                   = thePlacement;
  anInfo.major                       = theMajor;
  anInfo.minor                       = theMinor;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_ellipse(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a circular-arc Wire through three points.
inline NodeId make_arc_3pt(Graph&                  theGraph,
                           const ::occtl_point3_t& theStart,
                           const ::occtl_point3_t& theVia,
                           const ::occtl_point3_t& theEnd)
{
  ::occtl_prim_arc_3pt_info_t anInfo = OCCTL_PRIM_ARC_3PT_INFO_INIT;
  anInfo.start                       = theStart;
  anInfo.via                         = theVia;
  anInfo.end                         = theEnd;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_arc_3pt(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a circular-arc Wire from a center frame, radius, and angle range.
inline NodeId make_arc_center(Graph&                           theGraph,
                              const double                     theRadius,
                              const double                     theStartAngle,
                              const double                     theEndAngle,
                              const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_arc_center_info_t anInfo = OCCTL_PRIM_ARC_CENTER_INFO_INIT;
  anInfo.placement                      = thePlacement;
  anInfo.radius                         = theRadius;
  anInfo.start_angle                    = theStartAngle;
  anInfo.end_angle                      = theEndAngle;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_arc_center(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a B-spline Wire approximating points.
inline NodeId make_spline(Graph&                               theGraph,
                          const std::vector<::occtl_point3_t>& thePoints,
                          const int                            theDegreeMin = 3,
                          const int                            theDegreeMax = 8,
                          const double                         theTolerance = 1.0e-3)
{
  ::occtl_prim_spline_info_t anInfo = OCCTL_PRIM_SPLINE_INFO_INIT;
  anInfo.points                     = thePoints.empty() ? nullptr : thePoints.data();
  anInfo.point_count                = thePoints.size();
  anInfo.degree_min                 = theDegreeMin;
  anInfo.degree_max                 = theDegreeMax;
  anInfo.tolerance                  = theTolerance;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_spline(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a rectangular planar Face.
inline NodeId make_plane(Graph&                           theGraph,
                         const double                     theWidth,
                         const double                     theHeight,
                         const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_plane_info_t anInfo = OCCTL_PRIM_PLANE_INFO_INIT;
  anInfo.placement                 = thePlacement;
  anInfo.width                     = theWidth;
  anInfo.height                    = theHeight;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_plane(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a circular planar Face.
inline NodeId make_disk(Graph&                           theGraph,
                        const double                     theRadius,
                        const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_disk_info_t anInfo = OCCTL_PRIM_DISK_INFO_INIT;
  anInfo.placement                = thePlacement;
  anInfo.radius                   = theRadius;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_disk(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a planar Face from a closed outer Wire and optional inner Wires (holes).
inline NodeId make_planar_face(Graph&                     theGraph,
                               const NodeId&              theOuter,
                               const std::vector<NodeId>& theInners = {})
{
  std::vector<::occtl_node_id_t> anInnerIds;
  anInnerIds.reserve(theInners.size());
  for (const NodeId& anInner : theInners)
  {
    anInnerIds.push_back(anInner.get());
  }

  ::occtl_prim_planar_face_info_t anInfo = OCCTL_PRIM_PLANAR_FACE_INFO_INIT;
  anInfo.outer_wire                      = theOuter.get();
  anInfo.inner_wires                     = anInnerIds.empty() ? nullptr : anInnerIds.data();
  anInfo.inner_wire_count                = anInnerIds.size();

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_planar_face(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_convex_hull_2d.
struct ConvexHull2dOptions
{
  ::occtl_axis2_placement_t placement = detail::default_ax2();
  double                    tolerance = 1.0e-7;
  bool                      make_face = false;
};

/// @brief Builds a convex-hull Wire or planar Face from points and Vertex nodes.
inline NodeId make_convex_hull_2d(Graph&                             theGraph,
                                  const std::vector<occtl_point3_t>& thePoints,
                                  const std::vector<NodeId>&         theVertices = {},
                                  const ConvexHull2dOptions&         theOptions  = {})
{
  std::vector<::occtl_node_id_t> aVertexIds;
  aVertexIds.reserve(theVertices.size());
  for (const NodeId& aVertex : theVertices)
  {
    aVertexIds.push_back(aVertex.get());
  }

  ::occtl_prim_convex_hull_2d_info_t anInfo = OCCTL_PRIM_CONVEX_HULL_2D_INFO_INIT;
  anInfo.placement                          = theOptions.placement;
  anInfo.points                             = thePoints.empty() ? nullptr : thePoints.data();
  anInfo.point_count                        = thePoints.size();
  anInfo.vertices                           = aVertexIds.empty() ? nullptr : aVertexIds.data();
  anInfo.vertex_count                       = aVertexIds.size();
  anInfo.tolerance                          = theOptions.tolerance;
  anInfo.make_face                          = theOptions.make_face ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_convex_hull_2d(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_trace.
struct TraceOptions
{
  ::occtl_direction3_t               normal      = {0.0, 0.0, 1.0};
  ::occtl_topo_wire_offset_2d_join_t join        = OCCTL_TOPO_WIRE_OFFSET_2D_JOIN_ARC;
  bool                               approximate = false;
};

/// @brief Builds a constant-width planar Face around an Edge or open Wire path.
inline NodeId make_trace(Graph&              theGraph,
                         const NodeId&       thePath,
                         const double        theWidth,
                         const TraceOptions& theOptions = {})
{
  ::occtl_prim_trace_info_t anInfo = OCCTL_PRIM_TRACE_INFO_INIT;
  anInfo.path                      = thePath.get();
  anInfo.width                     = theWidth;
  anInfo.normal                    = theOptions.normal;
  anInfo.join                      = theOptions.join;
  anInfo.approximate               = theOptions.approximate ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_trace(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_constrained_curve_edge.
struct ConstrainedCurveEdgeOptions
{
  ::occtl_axis2_placement_t placement           = detail::default_ax2();
  bool                      use_parameter_range = false;
  double                    first_parameter     = 0.0;
  double                    last_parameter      = 0.0;
};

/// @brief Places a 2D curve on a sketch plane and inserts the resulting Edge.
inline NodeId make_constrained_curve_edge(Graph&                             theGraph,
                                          const Curve2d&                     theCurve,
                                          const ConstrainedCurveEdgeOptions& theOptions = {})
{
  ::occtl_prim_constrained_edge_info_t anInfo = OCCTL_PRIM_CONSTRAINED_EDGE_INFO_INIT;
  anInfo.curve                                = theCurve.id();
  anInfo.placement                            = theOptions.placement;
  anInfo.use_parameter_range                  = theOptions.use_parameter_range ? 1 : 0;
  anInfo.first_parameter                      = theOptions.first_parameter;
  anInfo.last_parameter                       = theOptions.last_parameter;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_constrained_edge(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_pipe_shell.  Mirrors the C info struct in a more idiomatic form.
struct PipeShellOptions
{
  ::occtl_prim_pipe_mode_t       mode            = OCCTL_PIPE_MODE_CORRECTED_FRENET;
  ::occtl_axis2_placement_t      mode_axis       = detail::default_ax2();
  ::occtl_direction3_t           mode_binormal   = {0.0, 0.0, 1.0};
  ::occtl_prim_pipe_transition_t transition      = OCCTL_PIPE_TRANSITION_MODIFIED;
  bool                           with_contact    = false;
  bool                           with_correction = false;
  bool                           make_solid      = false;
};

/// @brief Rich pipe-shell sweep with explicit mode / transition / contact control.
inline NodeId make_pipe_shell(Graph&                     theGraph,
                              const NodeId&              theSpineWire,
                              const std::vector<NodeId>& theProfiles,
                              const PipeShellOptions&    theOptions = {})
{
  std::vector<::occtl_node_id_t> aProfiles;
  aProfiles.reserve(theProfiles.size());
  for (const NodeId& aP : theProfiles)
  {
    aProfiles.push_back(aP.get());
  }

  ::occtl_prim_pipe_shell_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_INFO_INIT;
  anInfo.spine_wire                     = theSpineWire.get();
  anInfo.profiles                       = aProfiles.empty() ? nullptr : aProfiles.data();
  anInfo.profile_count                  = aProfiles.size();
  anInfo.mode                           = theOptions.mode;
  anInfo.mode_axis                      = theOptions.mode_axis;
  anInfo.mode_binormal                  = theOptions.mode_binormal;
  anInfo.transition                     = theOptions.transition;
  anInfo.with_contact                   = theOptions.with_contact ? 1 : 0;
  anInfo.with_correction                = theOptions.with_correction ? 1 : 0;
  anInfo.make_solid                     = theOptions.make_solid ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_pipe_shell(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_pipe_shell_linear_law.
struct PipeShellLinearLawOptions
{
  double                         scale_first     = 1.0;
  double                         scale_last      = 1.0;
  ::occtl_prim_pipe_mode_t       mode            = OCCTL_PIPE_MODE_CORRECTED_FRENET;
  ::occtl_axis2_placement_t      mode_axis       = detail::default_ax2();
  ::occtl_direction3_t           mode_binormal   = {0.0, 0.0, 1.0};
  ::occtl_prim_pipe_transition_t transition      = OCCTL_PIPE_TRANSITION_MODIFIED;
  bool                           with_contact    = false;
  bool                           with_correction = false;
  bool                           make_solid      = false;
};

/// @brief Rich pipe-shell sweep with linear profile scaling along the spine.
inline NodeId make_pipe_shell_linear_law(Graph&                           theGraph,
                                         const NodeId&                    theSpineWire,
                                         const NodeId&                    theProfile,
                                         const PipeShellLinearLawOptions& theOptions = {})
{
  ::occtl_prim_pipe_shell_linear_law_info_t anInfo = OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT;
  anInfo.spine_wire                                = theSpineWire.get();
  anInfo.profile                                   = theProfile.get();
  anInfo.scale_first                               = theOptions.scale_first;
  anInfo.scale_last                                = theOptions.scale_last;
  anInfo.mode                                      = theOptions.mode;
  anInfo.mode_axis                                 = theOptions.mode_axis;
  anInfo.mode_binormal                             = theOptions.mode_binormal;
  anInfo.transition                                = theOptions.transition;
  anInfo.with_contact                              = theOptions.with_contact ? 1 : 0;
  anInfo.with_correction                           = theOptions.with_correction ? 1 : 0;
  anInfo.make_solid                                = theOptions.make_solid ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_pipe_shell_linear_law(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_pipe_shell_interpolated_law.
struct PipeShellInterpolatedLawOptions
{
  std::vector<double>            parameters;
  std::vector<double>            scales;
  ::occtl_prim_pipe_mode_t       mode            = OCCTL_PIPE_MODE_CORRECTED_FRENET;
  ::occtl_axis2_placement_t      mode_axis       = detail::default_ax2();
  ::occtl_direction3_t           mode_binormal   = {0.0, 0.0, 1.0};
  ::occtl_prim_pipe_transition_t transition      = OCCTL_PIPE_TRANSITION_MODIFIED;
  bool                           with_contact    = false;
  bool                           with_correction = false;
  bool                           make_solid      = false;
};

/// @brief Rich pipe-shell sweep with interpolated profile scaling along the spine.
inline NodeId make_pipe_shell_interpolated_law(Graph&                                 theGraph,
                                               const NodeId&                          theSpineWire,
                                               const NodeId&                          theProfile,
                                               const PipeShellInterpolatedLawOptions& theOptions)
{
  ::occtl_prim_pipe_shell_interpolated_law_info_t anInfo =
    OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_INIT;
  anInfo.spine_wire = theSpineWire.get();
  anInfo.profile    = theProfile.get();
  anInfo.parameters = theOptions.parameters.empty() ? nullptr : theOptions.parameters.data();
  anInfo.scales     = theOptions.scales.empty() ? nullptr : theOptions.scales.data();
  anInfo.sample_count =
    theOptions.parameters.size() == theOptions.scales.size() ? theOptions.parameters.size() : 0;
  anInfo.mode            = theOptions.mode;
  anInfo.mode_axis       = theOptions.mode_axis;
  anInfo.mode_binormal   = theOptions.mode_binormal;
  anInfo.transition      = theOptions.transition;
  anInfo.with_contact    = theOptions.with_contact ? 1 : 0;
  anInfo.with_correction = theOptions.with_correction ? 1 : 0;
  anInfo.make_solid      = theOptions.make_solid ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_pipe_shell_interpolated_law(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_offset_shape and #make_thick_solid.
struct OffsetOptions
{
  double                     tolerance             = 1.0e-3;
  ::occtl_prim_offset_mode_t mode                  = OCCTL_OFFSET_MODE_SKIN;
  ::occtl_offset_join_type_t join                  = OCCTL_OFFSET_JOIN_ARC;
  bool                       intersection          = false;
  bool                       self_intersection     = false;
  bool                       remove_internal_edges = false;
};

/// @brief Builds an offset of @p theShape (wire / face / shell / solid).
inline NodeId make_offset_shape(Graph&               theGraph,
                                const NodeId&        theShape,
                                const double         theOffset,
                                const OffsetOptions& theOptions = {})
{
  ::occtl_prim_offset_shape_info_t anInfo = OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT;
  anInfo.shape                            = theShape.get();
  anInfo.offset                           = theOffset;
  anInfo.tolerance                        = theOptions.tolerance;
  anInfo.mode                             = theOptions.mode;
  anInfo.join                             = theOptions.join;
  anInfo.intersection                     = theOptions.intersection ? 1 : 0;
  anInfo.self_intersection                = theOptions.self_intersection ? 1 : 0;
  anInfo.remove_internal_edges            = theOptions.remove_internal_edges ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_offset_shape(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Hollows @p theSolid into a thick-walled solid (the "shell" CAD op).
///        @p theClosingFaces are dropped to leave open ends.
inline NodeId make_thick_solid(Graph&                     theGraph,
                               const NodeId&              theSolid,
                               const std::vector<NodeId>& theClosingFaces,
                               const double               theOffset,
                               const OffsetOptions&       theOptions = {})
{
  std::vector<::occtl_node_id_t> aClosing;
  aClosing.reserve(theClosingFaces.size());
  for (const NodeId& aF : theClosingFaces)
  {
    aClosing.push_back(aF.get());
  }

  ::occtl_prim_thick_solid_info_t anInfo = OCCTL_PRIM_THICK_SOLID_INFO_INIT;
  anInfo.solid                           = theSolid.get();
  anInfo.closing_faces                   = aClosing.empty() ? nullptr : aClosing.data();
  anInfo.closing_face_count              = aClosing.size();
  anInfo.offset                          = theOffset;
  anInfo.tolerance                       = theOptions.tolerance;
  anInfo.mode                            = theOptions.mode;
  anInfo.join                            = theOptions.join;
  anInfo.intersection                    = theOptions.intersection ? 1 : 0;
  anInfo.self_intersection               = theOptions.self_intersection ? 1 : 0;
  anInfo.remove_internal_edges           = theOptions.remove_internal_edges ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_thick_solid(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a tapered prism solid from one Face profile.
inline NodeId make_draft_prism(Graph&        theGraph,
                               const NodeId& theProfile,
                               const double  theHeight,
                               const double  theTaperAngle)
{
  ::occtl_prim_draft_prism_info_t anInfo = OCCTL_PRIM_DRAFT_PRISM_INFO_INIT;
  anInfo.profile                         = theProfile.get();
  anInfo.height                          = theHeight;
  anInfo.taper_angle                     = theTaperAngle;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_draft_prism(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a tapered extrusion solid from one Face profile.
inline NodeId make_extrude_tapered(Graph&        theGraph,
                                   const NodeId& theProfileFace,
                                   const double  theHeight,
                                   const double  theTaperAngle)
{
  ::occtl_prim_extrude_tapered_info_t anInfo = OCCTL_PRIM_EXTRUDE_TAPERED_INFO_INIT;
  anInfo.profile_face                        = theProfileFace.get();
  anInfo.height                              = theHeight;
  anInfo.taper_angle                         = theTaperAngle;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_extrude_tapered(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a ruled Face or Shell between two Edge or Wire sections.
inline NodeId make_ruled_surface(Graph&        theGraph,
                                 const NodeId& theSectionA,
                                 const NodeId& theSectionB)
{
  ::occtl_prim_ruled_surface_info_t anInfo = OCCTL_PRIM_RULED_SURFACE_INFO_INIT;
  anInfo.section_a                         = theSectionA.get();
  anInfo.section_b                         = theSectionB.get();

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_ruled_surface(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_feat_prism.
struct FeatPrismOptions
{
  ::occtl_prim_feat_combine_t combine     = OCCTL_FEAT_FUSE;
  bool                        modify      = true;
  ::occtl_prim_until_kind_t   until_kind  = OCCTL_UNTIL_LENGTH;
  ::occtl_node_id_t           until_shape = OCCTL_NODE_ID_INVALID;
  double                      length      = 0.0;
};

/// @brief Builds a feature prism — extrude a profile on an existing body until a target / length /
/// through all.
inline NodeId make_feat_prism(Graph&                      theGraph,
                              const NodeId&               theBaseShape,
                              const NodeId&               theProfile,
                              const NodeId&               theSketchFace,
                              const ::occtl_direction3_t& theDirection,
                              const FeatPrismOptions&     theOptions = {})
{
  ::occtl_prim_feat_prism_info_t anInfo = OCCTL_PRIM_FEAT_PRISM_INFO_INIT;
  anInfo.base_shape                     = theBaseShape.get();
  anInfo.profile                        = theProfile.get();
  anInfo.sketch_face                    = theSketchFace.get();
  anInfo.direction                      = theDirection;
  anInfo.combine                        = theOptions.combine;
  anInfo.modify                         = theOptions.modify ? 1 : 0;
  anInfo.until_kind                     = theOptions.until_kind;
  anInfo.until_shape                    = theOptions.until_shape;
  anInfo.length                         = theOptions.length;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_feat_prism(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_extrude_until.
struct ExtrudeUntilOptions
{
  ::occtl_prim_extrude_until_side_t side    = OCCTL_EXTRUDE_UNTIL_NEXT;
  ::occtl_prim_feat_combine_t       combine = OCCTL_FEAT_FUSE;
  bool                              modify  = true;
  double                            limit   = 0.0;
};

/// @brief Builds a feature extrusion from a profile until a target shape.
inline NodeId make_extrude_until(Graph&                      theGraph,
                                 const NodeId&               theBaseShape,
                                 const NodeId&               theProfile,
                                 const NodeId&               theSketchFace,
                                 const NodeId&               theTargetShape,
                                 const ::occtl_direction3_t& theDirection,
                                 const ExtrudeUntilOptions&  theOptions = {})
{
  ::occtl_prim_extrude_until_info_t anInfo = OCCTL_PRIM_EXTRUDE_UNTIL_INFO_INIT;
  anInfo.base_shape                        = theBaseShape.get();
  anInfo.profile                           = theProfile.get();
  anInfo.sketch_face                       = theSketchFace.get();
  anInfo.target_shape                      = theTargetShape.get();
  anInfo.direction                         = theDirection;
  anInfo.side                              = theOptions.side;
  anInfo.combine                           = theOptions.combine;
  anInfo.modify                            = theOptions.modify ? 1 : 0;
  anInfo.limit                             = theOptions.limit;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_extrude_until(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_feat_draft_prism.
struct FeatDraftPrismOptions
{
  ::occtl_prim_feat_combine_t combine     = OCCTL_FEAT_FUSE;
  bool                        modify      = true;
  ::occtl_prim_until_kind_t   until_kind  = OCCTL_UNTIL_LENGTH;
  ::occtl_node_id_t           until_shape = OCCTL_NODE_ID_INVALID;
  double                      length      = 0.0;
};

/// @brief Builds a draft-prism feature on an existing body.
inline NodeId make_feat_draft_prism(Graph&                       theGraph,
                                    const NodeId&                theBaseShape,
                                    const NodeId&                theProfileFace,
                                    const NodeId&                theSketchFace,
                                    const double                 theTaperAngle,
                                    const FeatDraftPrismOptions& theOptions = {})
{
  ::occtl_prim_feat_draft_prism_info_t anInfo = OCCTL_PRIM_FEAT_DRAFT_PRISM_INFO_INIT;
  anInfo.base_shape                           = theBaseShape.get();
  anInfo.profile_face                         = theProfileFace.get();
  anInfo.sketch_face                          = theSketchFace.get();
  anInfo.taper_angle                          = theTaperAngle;
  anInfo.combine                              = theOptions.combine;
  anInfo.modify                               = theOptions.modify ? 1 : 0;
  anInfo.until_kind                           = theOptions.until_kind;
  anInfo.until_shape                          = theOptions.until_shape;
  anInfo.length                               = theOptions.length;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_feat_draft_prism(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_cylindrical_hole.
struct CylindricalHoleOptions
{
  ::occtl_prim_cylindrical_hole_kind_t kind         = OCCTL_CYLINDRICAL_HOLE_THROUGH_ALL;
  double                               p_from       = 0.0;
  double                               p_to         = 0.0;
  double                               length       = 0.0;
  bool                                 with_control = true;
};

/// @brief Cuts a cylindrical hole feature into @p theBaseShape.
inline NodeId make_cylindrical_hole(Graph&                           theGraph,
                                    const NodeId&                    theBaseShape,
                                    const ::occtl_axis1_placement_t& theAxis,
                                    const double                     theRadius,
                                    const CylindricalHoleOptions&    theOptions = {})
{
  ::occtl_prim_cylindrical_hole_info_t anInfo = OCCTL_PRIM_CYLINDRICAL_HOLE_INFO_INIT;
  anInfo.base_shape                           = theBaseShape.get();
  anInfo.axis                                 = theAxis;
  anInfo.radius                               = theRadius;
  anInfo.kind                                 = theOptions.kind;
  anInfo.p_from                               = theOptions.p_from;
  anInfo.p_to                                 = theOptions.p_to;
  anInfo.length                               = theOptions.length;
  anInfo.with_control                         = theOptions.with_control ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_cylindrical_hole(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a closed slot (stadium) Wire.
inline NodeId make_slot(Graph&                           theGraph,
                        const double                     theLength,
                        const double                     theWidth,
                        const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_slot_info_t anInfo = OCCTL_PRIM_SLOT_INFO_INIT;
  anInfo.placement                = thePlacement;
  anInfo.length                   = theLength;
  anInfo.width                    = theWidth;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_slot(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a hollow-cylinder (tube) Solid.
inline NodeId make_tube(Graph&                           theGraph,
                        const double                     theOuterRadius,
                        const double                     theInnerRadius,
                        const double                     theHeight,
                        const ::occtl_axis2_placement_t& thePlacement = detail::default_ax2())
{
  ::occtl_prim_tube_info_t anInfo = OCCTL_PRIM_TUBE_INFO_INIT;
  anInfo.placement                = thePlacement;
  anInfo.outer_radius             = theOuterRadius;
  anInfo.inner_radius             = theInnerRadius;
  anInfo.height                   = theHeight;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_tube(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a helical Wire.
inline NodeId make_helix(Graph&                           theGraph,
                         const double                     theRadius,
                         const double                     thePitch,
                         const double                     theHeight,
                         const ::occtl_axis2_placement_t& thePlacement  = detail::default_ax2(),
                         const bool                       theLeftHanded = false)
{
  ::occtl_prim_helix_info_t anInfo = OCCTL_PRIM_HELIX_INFO_INIT;
  anInfo.placement                 = thePlacement;
  anInfo.radius                    = theRadius;
  anInfo.pitch                     = thePitch;
  anInfo.height                    = theHeight;
  anInfo.left_handed               = theLeftHanded ? 1 : 0;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_helix(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Rounds every corner of a planar Face to a uniform radius.
inline NodeId make_fillet_2d(Graph&                     theGraph,
                             const NodeId&              theFace,
                             const double               theRadius,
                             const std::vector<NodeId>& theVertices = {})
{
  std::vector<::occtl_node_id_t> aVerts;
  aVerts.reserve(theVertices.size());
  for (const NodeId& aV : theVertices)
  {
    aVerts.push_back(aV.get());
  }

  ::occtl_prim_fillet_2d_info_t anInfo = OCCTL_PRIM_FILLET_2D_INFO_INIT;
  anInfo.face                          = theFace.get();
  anInfo.vertices                      = aVerts.empty() ? nullptr : aVerts.data();
  anInfo.vertex_count                  = aVerts.size();
  anInfo.radius                        = theRadius;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_fillet_2d(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Options for #make_full_round_2d.
struct FullRound2dOptions
{
  double   radius       = 0.0;
  uint32_t search_steps = 32u;
};

/// @brief Replaces a selected planar Face edge with a full-round arc.
inline NodeId make_full_round_2d(Graph&                    theGraph,
                                 const NodeId&             theFace,
                                 const NodeId&             theEdge,
                                 const FullRound2dOptions& theOptions = {})
{
  ::occtl_prim_full_round_2d_info_t anInfo = OCCTL_PRIM_FULL_ROUND_2D_INFO_INIT;
  anInfo.face                              = theFace.get();
  anInfo.edge                              = theEdge.get();
  anInfo.radius                            = theOptions.radius;
  anInfo.search_steps                      = theOptions.search_steps;

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_prim_make_full_round_2d(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

} // namespace occtl::prim

#endif // OCCTL_HPP_PRIM_HPP
