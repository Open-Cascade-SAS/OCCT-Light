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
 * @brief C++ veneer for 2D curve representations in the topology graph.
 *
 * Non-owning graph-rep reference over @c occtl_rep_id_t inside an
 * @c occtl_graph_t* with STL-shaped accessors.
 * Local identifiers follow OCCT style.
 */

#ifndef OCCTL_HPP_CURVES2D_HPP
#define OCCTL_HPP_CURVES2D_HPP

#include <occtl/occtl_curves2d.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/geom.hpp>

#include <cstdint>
#include <cstdlib>
#include <tuple>
#include <utility>
#include <vector>

#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
  #include <span>
  #define OCCTL_HPP_HAS_SPAN 1
#else
  #define OCCTL_HPP_HAS_SPAN 0
#endif

namespace occtl
{

using GeomTangencyQualifier = occtl_curve2d_tangency_qualifier_t;

/// @brief Non-owning reference to a 2D geometric curve stored in a graph.
///
/// Wraps one @c occtl_rep_id_t inside an @c occtl_graph_t*.  Copying is
/// shallow — both copies refer to the same graph representation.
class Curve2d
{
public:
  /// @brief Constructs a null reference (no graph, invalid id).
  Curve2d(occtl_graph_t* theGraph = nullptr, occtl_rep_id_t theId = occtl_rep_id_t{0}) noexcept
      : myGraph(theGraph),
        myId(theId)
  {
  }

  /// @brief Returns true when the reference points into a valid graph.
  explicit operator bool() const noexcept { return myGraph != nullptr && myId.bits != 0; }

  /// @brief Borrows it — returns the graph pointer.
  occtl_graph_t* graph() const noexcept { return myGraph; }

  /// @brief Returns the representation id.
  occtl_rep_id_t id() const noexcept { return myId; }

  /// @brief Returns a reversed copy of this curve stored in the same graph.
  /// @throws Error when the underlying call fails.
  Curve2d reversed() const
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_reverse(myGraph, myId, &aId));
    return Curve2d(myGraph, aId);
  }

  /// @brief Returns a transformed copy of this curve stored in the same graph.
  /// @throws Error when the underlying call fails.
  Curve2d transformed(double theTranslateX,
                      double theTranslateY,
                      double theRotateAngle,
                      double theScaleX,
                      double theScaleY) const
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_transformed(myGraph,
                                      myId,
                                      theTranslateX,
                                      theTranslateY,
                                      theRotateAngle,
                                      theScaleX,
                                      theScaleY,
                                      &aId));
    return Curve2d(myGraph, aId);
  }

  /// @brief Returns a translated copy of this curve stored in the same graph.
  /// @throws Error when the underlying call fails.
  Curve2d translated(const Vector2& theDelta) const
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_translated(myGraph, myId, theDelta.c_type(), &aId));
    return Curve2d(myGraph, aId);
  }

  /// @brief Returns a rotated copy of this curve stored in the same graph.
  /// @throws Error when the underlying call fails.
  Curve2d rotated(double theAngle) const
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_rotated(myGraph, myId, theAngle, &aId));
    return Curve2d(myGraph, aId);
  }

  /// @brief Returns a scaled copy of this curve stored in the same graph.
  /// @throws Error when the underlying call fails.
  Curve2d scaled(const Point2& theOrigin, double theFactor) const
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_scaled(myGraph, myId, theOrigin.c_type(), theFactor, &aId));
    return Curve2d(myGraph, aId);
  }

  /// @brief Returns the curve length over its full parameter range.
  /// @throws Error when the underlying call fails.
  double length() const
  {
    double aLen = 0.0;
    check(::occtl_curve2d_length(myGraph, myId, &aLen));
    return aLen;
  }

  /// @brief Projects a 2D point onto the curve.
  /// @returns A pair of (parameter, distance).
  /// @throws Error when the underlying call fails.
  std::pair<double, double> project_point(const Point2& thePoint) const
  {
    double aParam = 0.0;
    double aDist  = 0.0;
    check(::occtl_curve2d_project_point(myGraph, myId, thePoint.c_type(), &aParam, &aDist));
    return {aParam, aDist};
  }

  /// @brief Returns the parameter on the 2D curve nearest to a given point.
  /// @throws Error when the underlying call fails.
  double parameter_of_point(const Point2& thePoint) const
  {
    double aParam = 0.0;
    check(::occtl_curve2d_parameter_of_point(myGraph, myId, thePoint.c_type(), &aParam));
    return aParam;
  }

  /// @brief Returns the kind of geometry.
  /// @throws Error when the underlying call fails.
  occtl_curve_kind_t kind() const
  {
    occtl_curve_kind_t aKind{};
    check(::occtl_curve2d_kind(myGraph, myId, &aKind));
    return aKind;
  }

  /// @brief Returns 1 when the curve is periodic, 0 otherwise.
  /// @throws Error when the underlying call fails.
  int32_t is_periodic() const
  {
    int32_t aVal = 0;
    check(::occtl_curve2d_is_periodic(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns 1 when the curve is closed, 0 otherwise.
  /// @throws Error when the underlying call fails.
  int32_t is_closed() const
  {
    int32_t aVal = 0;
    check(::occtl_curve2d_is_closed(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the geometric continuity class.
  /// @throws Error when the underlying call fails.
  occtl_geom_continuity_t continuity() const
  {
    occtl_geom_continuity_t aCont{};
    check(::occtl_curve2d_continuity(myGraph, myId, &aCont));
    return aCont;
  }

  /// @brief Returns the parameter range [u_min, u_max] via out-parameters.
  /// @throws Error when the underlying call fails.
  void parameter_range(double& theUMin, double& theUMax) const
  {
    check(::occtl_curve2d_parameter_range(myGraph, myId, &theUMin, &theUMax));
  }

  /// @brief Extracts the underlying line.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_CURVE_KIND_LINE.
  occtl_geom2d_line_t as_line() const
  {
    occtl_geom2d_line_t aOut{};
    check(::occtl_curve2d_as_line(myGraph, myId, &aOut));
    return aOut;
  }

  /// @brief Extracts the underlying circle.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_CURVE_KIND_CIRCLE.
  occtl_geom2d_circle_t as_circle() const
  {
    occtl_geom2d_circle_t aOut{};
    check(::occtl_curve2d_as_circle(myGraph, myId, &aOut));
    return aOut;
  }

  /// @brief Extracts the underlying ellipse.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_CURVE_KIND_ELLIPSE.
  occtl_geom2d_ellipse_t as_ellipse() const
  {
    occtl_geom2d_ellipse_t aOut{};
    check(::occtl_curve2d_as_ellipse(myGraph, myId, &aOut));
    return aOut;
  }

  /// @brief Extracts the underlying hyperbola.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_CURVE_KIND_HYPERBOLA.
  occtl_geom2d_hyperbola_t as_hyperbola() const
  {
    occtl_geom2d_hyperbola_t aOut{};
    check(::occtl_curve2d_as_hyperbola(myGraph, myId, &aOut));
    return aOut;
  }

  /// @brief Extracts the underlying parabola.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_CURVE_KIND_PARABOLA.
  occtl_geom2d_parabola_t as_parabola() const
  {
    occtl_geom2d_parabola_t aOut{};
    check(::occtl_curve2d_as_parabola(myGraph, myId, &aOut));
    return aOut;
  }

  struct TrimmedView;
  struct OffsetView;
  struct BSplineView;

  /// @brief Extracts the parameter bounds from a trimmed curve.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != TRIMMED.
  inline TrimmedView as_trimmed() const;

  /// @brief Extracts the scalar offset of an offset curve.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != OFFSET.
  inline OffsetView as_offset() const;

  /// @brief Returns a §10.5 aggregate inspection view of a 2D B-spline curve.
  ///
  /// The returned view's pointer fields borrow from this curve's graph rep
  /// and remain valid until the rep is removed from the graph.
  ///
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != BSPLINE,
  ///         or @c OCCTL_VERSION_MISMATCH on unsupported layout version.
  inline BSplineView as_bspline() const;

  /// @brief Returns the polynomial degree.
  /// @throws Error when kind != BSPLINE.
  int32_t bspline_degree() const
  {
    int32_t aVal = 0;
    check(::occtl_curve2d_bspline_degree(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the number of poles.
  /// @throws Error when kind != BSPLINE.
  size_t bspline_pole_count() const
  {
    size_t aVal = 0;
    check(::occtl_curve2d_bspline_pole_count(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the number of distinct knot values.
  /// @throws Error when kind != BSPLINE.
  size_t bspline_knot_count() const
  {
    size_t aVal = 0;
    check(::occtl_curve2d_bspline_knot_count(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns 1 when the B-spline is rational (NURBS), 0 otherwise.
  /// @throws Error when kind != BSPLINE.
  int32_t bspline_is_rational() const
  {
    int32_t aVal = 0;
    check(::occtl_curve2d_bspline_is_rational(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the Bezier polynomial degree.
  /// @throws Error when kind != BEZIER.
  int32_t bezier_degree() const
  {
    int32_t aVal = 0;
    check(::occtl_curve2d_bezier_degree(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the number of Bezier poles.
  /// @throws Error when kind != BEZIER.
  size_t bezier_pole_count() const
  {
    size_t aVal = 0;
    check(::occtl_curve2d_bezier_pole_count(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns 1 when the Bezier curve is rational, 0 otherwise.
  /// @throws Error when kind != BEZIER.
  int32_t bezier_is_rational() const
  {
    int32_t aVal = 0;
    check(::occtl_curve2d_bezier_is_rational(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Copies all poles into a @c std::vector<occtl_point2_t>.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<occtl_point2_t> bspline_poles() const
  {
    size_t aNb = 0;
    check(::occtl_curve2d_bspline_poles(myGraph, myId, nullptr, 0, &aNb));
    std::vector<occtl_point2_t> aVec(aNb);
    check(::occtl_curve2d_bspline_poles(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies all distinct knot values into a @c std::vector<double>.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<double> bspline_knots() const
  {
    size_t aNb = 0;
    check(::occtl_curve2d_bspline_knots(myGraph, myId, nullptr, 0, &aNb));
    std::vector<double> aVec(aNb);
    check(::occtl_curve2d_bspline_knots(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies all knot multiplicities into a @c std::vector<int32_t>.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<int32_t> bspline_multiplicities() const
  {
    size_t aNb = 0;
    check(::occtl_curve2d_bspline_multiplicities(myGraph, myId, nullptr, 0, &aNb));
    std::vector<int32_t> aVec(aNb);
    check(::occtl_curve2d_bspline_multiplicities(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies all weights into a @c std::vector<double>.
  /// @throws Error with @c OCCTL_WRONG_KIND when the curve is non-rational.
  std::vector<double> bspline_weights() const
  {
    size_t aNb = 0;
    check(::occtl_curve2d_bspline_weights(myGraph, myId, nullptr, 0, &aNb));
    std::vector<double> aVec(aNb);
    check(::occtl_curve2d_bspline_weights(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies the expanded (flat) knot sequence into a vector.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<double> bspline_flat_knots() const
  {
    size_t aNb = 0;
    check(::occtl_curve2d_bspline_flat_knots(myGraph, myId, nullptr, 0, &aNb));
    std::vector<double> aVec(aNb);
    check(::occtl_curve2d_bspline_flat_knots(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Returns a zero-copy view of the poles array.
  ///
  /// Valid until the graph rep is removed or a mutating call is made on it.
  /// @throws Error when kind != BSPLINE or view is unavailable.
  const occtl_point2_t* bspline_poles_view(size_t& theCount) const
  {
    const occtl_point2_t* aData = nullptr;
    check(::occtl_curve2d_bspline_poles_view(myGraph, myId, &aData, &theCount));
    return aData;
  }

  /// @brief Constructs a 2D curve from a line.
  /// @throws Error on invalid geometry.
  static Curve2d from_line(occtl_graph_t* graph, const occtl_geom2d_line_t& theLine)
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_create_line(graph, theLine, &aId));
    return Curve2d(graph, aId);
  }

  /// @brief Constructs a 2D curve from a circle.
  /// @throws Error on invalid geometry.
  static Curve2d from_circle(occtl_graph_t* graph, const occtl_geom2d_circle_t& theCircle)
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_create_circle(graph, theCircle, &aId));
    return Curve2d(graph, aId);
  }

  /// @brief Computes all fixed-radius circles tangent to two 2D curves.
  /// @throws Error when the underlying solver call fails.
  static std::vector<occtl_geom2d_circle_t> circles_tangent_to_two_radius(
    occtl_graph_t*                                           graph,
    const occtl_curve2d_circle_tangent_to_two_radius_info_t& theInfo)
  {
    size_t aCount = 0;
    check(
      ::occtl_curve2d_create_tangent_circle_to_two_radius(graph, &theInfo, 0, nullptr, &aCount));
    std::vector<occtl_geom2d_circle_t> aCircles(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      check(::occtl_curve2d_create_tangent_circle_to_two_radius(graph,
                                                                &theInfo,
                                                                anIndex,
                                                                &aCircles[anIndex],
                                                                &aCount));
    }
    return aCircles;
  }

  /// @brief Computes all fixed-radius 2D blend arcs tangent to two 2D curves.
  /// @throws Error when the underlying solver call fails.
  static std::vector<Curve2d> blend_arcs(occtl_graph_t*                        graph,
                                         const occtl_curve2d_blend_arc_info_t& theInfo)
  {
    size_t aCount = 0;
    check(::occtl_curve2d_create_blend_arc(graph, &theInfo, 0, nullptr, &aCount));
    std::vector<Curve2d> anArcs;
    anArcs.reserve(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      occtl_rep_id_t anArc{};
      check(::occtl_curve2d_create_blend_arc(graph, &theInfo, anIndex, &anArc, &aCount));
      anArcs.emplace_back(graph, anArc);
    }
    return anArcs;
  }

  /// @brief Computes all 2D lines tangent to two 2D curves.
  /// @throws Error when the underlying solver call fails.
  static std::vector<occtl_geom2d_line_t> lines_tangent_to_two(
    occtl_graph_t*                                  graph,
    const occtl_curve2d_line_tangent_to_two_info_t& theInfo)
  {
    size_t aCount = 0;
    check(::occtl_curve2d_create_tangent_line_to_two(graph, &theInfo, 0, nullptr, &aCount));
    std::vector<occtl_geom2d_line_t> aLines(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      check(::occtl_curve2d_create_tangent_line_to_two(graph,
                                                       &theInfo,
                                                       anIndex,
                                                       &aLines[anIndex],
                                                       &aCount));
    }
    return aLines;
  }

  /// @brief Computes all 2D lines tangent to a 2D curve through a point.
  /// @throws Error when the underlying solver call fails.
  static std::vector<occtl_geom2d_line_t> lines_tangent_through_point(
    occtl_graph_t*                                         graph,
    const occtl_curve2d_line_tangent_through_point_info_t& theInfo)
  {
    size_t aCount = 0;
    check(::occtl_curve2d_create_tangent_line_through_point(graph, &theInfo, 0, nullptr, &aCount));
    std::vector<occtl_geom2d_line_t> aLines(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      check(::occtl_curve2d_create_tangent_line_through_point(graph,
                                                              &theInfo,
                                                              anIndex,
                                                              &aLines[anIndex],
                                                              &aCount));
    }
    return aLines;
  }

  /// @brief Computes all 2D lines tangent to a curve at a fixed angle to a line.
  /// @throws Error when the underlying solver call fails.
  static std::vector<occtl_geom2d_line_t> lines_tangent_with_angle(
    occtl_graph_t*                                      graph,
    const occtl_curve2d_line_tangent_with_angle_info_t* theInfo)
  {
    size_t aCount = 0;
    check(::occtl_curve2d_create_tangent_line_with_angle(graph, theInfo, 0, nullptr, &aCount));
    std::vector<occtl_geom2d_line_t> aLines(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      check(::occtl_curve2d_create_tangent_line_with_angle(graph,
                                                           theInfo,
                                                           anIndex,
                                                           &aLines[anIndex],
                                                           &aCount));
    }
    return aLines;
  }

  /// @brief Computes all 2D circles tangent to three 2D curves.
  /// @throws Error when the underlying solver call fails.
  static std::vector<occtl_geom2d_circle_t> circles_tangent_to_three(
    occtl_graph_t*                                      graph,
    const occtl_curve2d_circle_tangent_to_three_info_t& theInfo)
  {
    size_t aCount = 0;
    check(::occtl_curve2d_create_tangent_circle_to_three(graph, &theInfo, 0, nullptr, &aCount));
    std::vector<occtl_geom2d_circle_t> aCircles(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      check(::occtl_curve2d_create_tangent_circle_to_three(graph,
                                                           &theInfo,
                                                           anIndex,
                                                           &aCircles[anIndex],
                                                           &aCount));
    }
    return aCircles;
  }

  /// @brief Computes all 2D circles tangent to a curve with a fixed center.
  /// @throws Error when the underlying solver call fails.
  static std::vector<occtl_geom2d_circle_t> circles_tangent_fixed_center(
    occtl_graph_t*                                          graph,
    const occtl_curve2d_circle_tangent_fixed_center_info_t& theInfo)
  {
    size_t aCount = 0;
    check(::occtl_curve2d_create_tangent_circle_fixed_center(graph, &theInfo, 0, nullptr, &aCount));
    std::vector<occtl_geom2d_circle_t> aCircles(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      check(::occtl_curve2d_create_tangent_circle_fixed_center(graph,
                                                               &theInfo,
                                                               anIndex,
                                                               &aCircles[anIndex],
                                                               &aCount));
    }
    return aCircles;
  }

  /// @brief Computes all 2D circles tangent to two curves with center on a curve.
  /// @throws Error when the underlying solver call fails.
  static std::vector<occtl_geom2d_circle_t> circles_tangent_center_on_curve(
    occtl_graph_t*                                             graph,
    const occtl_curve2d_circle_tangent_center_on_curve_info_t* theInfo)
  {
    size_t aCount = 0;
    check(
      ::occtl_curve2d_create_tangent_circle_center_on_curve(graph, theInfo, 0, nullptr, &aCount));
    std::vector<occtl_geom2d_circle_t> aCircles(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      check(::occtl_curve2d_create_tangent_circle_center_on_curve(graph,
                                                                  theInfo,
                                                                  anIndex,
                                                                  &aCircles[anIndex],
                                                                  &aCount));
    }
    return aCircles;
  }

  /// @brief Computes fixed-radius 2D circles tangent to a curve with center on another curve.
  /// @throws Error when the underlying solver call fails.
  static std::vector<occtl_geom2d_circle_t> circles_tangent_on_curve_radius(
    occtl_graph_t*                                             graph,
    const occtl_curve2d_circle_tangent_on_curve_radius_info_t* theInfo)
  {
    size_t aCount = 0;
    check(
      ::occtl_curve2d_create_tangent_circle_on_curve_radius(graph, theInfo, 0, nullptr, &aCount));
    std::vector<occtl_geom2d_circle_t> aCircles(aCount);
    for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
    {
      check(::occtl_curve2d_create_tangent_circle_on_curve_radius(graph,
                                                                  theInfo,
                                                                  anIndex,
                                                                  &aCircles[anIndex],
                                                                  &aCount));
    }
    return aCircles;
  }

  /// @brief Constructs a 2D curve from an ellipse.
  /// @throws Error on invalid geometry.
  static Curve2d from_ellipse(occtl_graph_t* graph, const occtl_geom2d_ellipse_t& theEllipse)
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_create_ellipse(graph, theEllipse, &aId));
    return Curve2d(graph, aId);
  }

  /// @brief Constructs a 2D curve from a hyperbola.
  /// @throws Error on invalid geometry.
  static Curve2d from_hyperbola(occtl_graph_t* graph, const occtl_geom2d_hyperbola_t& theHyperbola)
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_create_hyperbola(graph, theHyperbola, &aId));
    return Curve2d(graph, aId);
  }

  /// @brief Constructs a 2D curve from a parabola.
  /// @throws Error on invalid geometry.
  static Curve2d from_parabola(occtl_graph_t* graph, const occtl_geom2d_parabola_t& theParabola)
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_create_parabola(graph, theParabola, &aId));
    return Curve2d(graph, aId);
  }

  /// @brief Constructs a 2D B-spline curve from the given create info.
  /// @throws Error on invalid geometry or version mismatch.
  static Curve2d from_bspline(occtl_graph_t*                             graph,
                              const occtl_curve2d_bspline_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_create_bspline(graph, &theInfo, &aId));
    return Curve2d(graph, aId);
  }

  /// @brief Constructs a 2D Bezier curve from the given create info.
  /// @throws Error on invalid geometry or version mismatch.
  static Curve2d from_bezier(occtl_graph_t*                            graph,
                             const occtl_curve2d_bezier_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_create_bezier(graph, &theInfo, &aId));
    return Curve2d(graph, aId);
  }

  /// @brief Constructs a trimmed 2D curve.
  /// @throws Error on invalid geometry or version mismatch.
  static Curve2d from_trimmed(occtl_graph_t*                           graph,
                              const occtl_curve_trimmed_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_create_trimmed(graph, &theInfo, &aId));
    return Curve2d(graph, aId);
  }

  /// @brief Constructs an offset 2D curve.
  /// @throws Error on invalid geometry or version mismatch.
  static Curve2d from_offset(occtl_graph_t*                            graph,
                             const occtl_curve2d_offset_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_curve2d_create_offset(graph, &theInfo, &aId));
    return Curve2d(graph, aId);
  }

  /// @brief Decomposes this 2D curve into adjacent Bezier curve segments.
  /// @throws Error on invalid options or conversion failure.
  std::vector<Curve2d> to_bezier_segments(
    const occtl_curve_bezier_segments_options_t* theOptions = nullptr) const;

  /// @brief Evaluates the 2D curve point at parameter @p theU.
  /// @throws Error on NULL graph or out-of-range.
  occtl_point2_t eval_d0(const double theU) const
  {
    occtl_point2_t aP{};
    check(::occtl_curve2d_eval_d0(myGraph, myId, theU, &aP));
    return aP;
  }

  /// @brief Evaluates point and first derivative at @p theU.
  /// @throws Error on NULL graph or out-of-range.
  std::pair<occtl_point2_t, occtl_vector2_t> eval_d1(const double theU) const
  {
    occtl_point2_t  aP{};
    occtl_vector2_t aD1{};
    check(::occtl_curve2d_eval_d1(myGraph, myId, theU, &aP, &aD1));
    return {aP, aD1};
  }

  /// @brief Evaluates point, first, and second derivatives at @p theU.
  /// @throws Error on NULL graph or out-of-range.
  std::tuple<occtl_point2_t, occtl_vector2_t, occtl_vector2_t> eval_d2(const double theU) const
  {
    occtl_point2_t  aP{};
    occtl_vector2_t aD1{}, aD2{};
    check(::occtl_curve2d_eval_d2(myGraph, myId, theU, &aP, &aD1, &aD2));
    return {aP, aD1, aD2};
  }

  /// @brief Evaluates point, first, second, and third derivatives at @p theU.
  /// @throws Error on NULL graph or out-of-range.
  std::tuple<occtl_point2_t, occtl_vector2_t, occtl_vector2_t, occtl_vector2_t> eval_d3(
    const double theU) const
  {
    occtl_point2_t  aP{};
    occtl_vector2_t aD1{}, aD2{}, aD3{};
    check(::occtl_curve2d_eval_d3(myGraph, myId, theU, &aP, &aD1, &aD2, &aD3));
    return {aP, aD1, aD2, aD3};
  }

  /// @brief Returns the N-th derivative vector at @p theU.
  /// @throws Error on NULL graph or out-of-range.
  occtl_vector2_t eval_dn(const double theU, const int32_t theN) const
  {
    occtl_vector2_t aDN{};
    check(::occtl_curve2d_eval_dn(myGraph, myId, theU, theN, &aDN));
    return aDN;
  }

private:
  occtl_graph_t* myGraph;
  occtl_rep_id_t myId;
};

struct Curve2d::TrimmedView
{
  double u_first;
  double u_last;
};

struct Curve2d::OffsetView
{
  double offset;
};

inline Curve2d::TrimmedView Curve2d::as_trimmed() const
{
  double aU0 = 0.0;
  double aU1 = 0.0;
  check(::occtl_curve2d_as_trimmed(myGraph, myId, &aU0, &aU1));
  return {aU0, aU1};
}

inline Curve2d::OffsetView Curve2d::as_offset() const
{
  double aOff = 0.0;
  check(::occtl_curve2d_as_offset(myGraph, myId, &aOff));
  return {aOff};
}

/// @brief Aggregate inspection view of a 2D B-spline curve (§10.5).
///
/// Holds the raw @c occtl_curve2d_bspline_t plus typed accessors.  Pointer
/// fields borrow from the parent @c Curve2d's graph rep and stay valid as
/// long as the rep remains in the graph.  Copying the view is cheap (POD
/// copy of pointers) but the borrowed lifetime is unchanged.
struct Curve2d::BSplineView
{
  occtl_curve2d_bspline_t raw;

  int32_t degree() const noexcept { return raw.degree; }

  bool is_rational() const noexcept { return raw.is_rational != 0; }

  bool is_periodic() const noexcept { return raw.is_periodic != 0; }

  bool is_closed() const noexcept { return raw.is_closed != 0; }

  occtl_geom_continuity_t continuity() const noexcept
  {
    return static_cast<occtl_geom_continuity_t>(raw.continuity);
  }

  size_t pole_count() const noexcept { return raw.pole_count; }

  size_t knot_count() const noexcept { return raw.knot_count; }

  size_t flat_knot_count() const noexcept { return raw.flat_knot_count; }

#if OCCTL_HPP_HAS_SPAN
  std::span<const occtl_point2_t> poles() const noexcept { return {raw.poles, raw.pole_count}; }

  std::span<const double> weights() const noexcept
  {
    return raw.weights ? std::span<const double>{raw.weights, raw.pole_count}
                       : std::span<const double>{};
  }

  std::span<const double> knots() const noexcept { return {raw.knots, raw.knot_count}; }

  std::span<const int32_t> multiplicities() const noexcept
  {
    return {raw.multiplicities, raw.knot_count};
  }

  std::span<const double> flat_knots() const noexcept
  {
    return {raw.flat_knots, raw.flat_knot_count};
  }
#else
  const occtl_point2_t* poles() const noexcept { return raw.poles; }

  const double* weights() const noexcept { return raw.weights; }

  const double* knots() const noexcept { return raw.knots; }

  const int32_t* multiplicities() const noexcept { return raw.multiplicities; }

  const double* flat_knots() const noexcept { return raw.flat_knots; }
#endif
};

inline Curve2d::BSplineView Curve2d::as_bspline() const
{
  BSplineView aView{};
  aView.raw = OCCTL_CURVE2D_BSPLINE_INIT;
  check(::occtl_curve2d_as_bspline(myGraph, myId, &aView.raw));
  return aView;
}

inline std::vector<Curve2d> Curve2d::to_bezier_segments(
  const occtl_curve_bezier_segments_options_t* theOptions) const
{
  occtl_rep_id_t* aIds = nullptr;
  size_t          aNb  = 0;
  check(::occtl_curve2d_to_bezier_segments(myGraph, myId, theOptions, &aIds, &aNb));
  std::vector<Curve2d> aResult;
  aResult.reserve(aNb);
  for (size_t anI = 0; anI < aNb; ++anI)
  {
    aResult.emplace_back(myGraph, aIds[anI]);
  }
  std::free(aIds);
  return aResult;
}

} // namespace occtl

#endif // OCCTL_HPP_CURVES2D_HPP
