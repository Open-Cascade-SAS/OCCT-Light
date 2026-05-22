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
 * @brief C++ veneer for surface representations in the topology graph.
 *
 * Non-owning graph-rep reference over @c occtl_rep_id_t inside an
 * @c occtl_graph_t* with STL-shaped accessors.
 * Local identifiers follow OCCT style.
 */

#ifndef OCCTL_HPP_SURFACES_HPP
#define OCCTL_HPP_SURFACES_HPP

#include <occtl/occtl_surfaces.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/curves.hpp>
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

/// @brief Non-owning reference to a geometric surface stored in a graph.
///
/// Wraps one @c occtl_rep_id_t inside an @c occtl_graph_t*.  Copying is
/// shallow — both copies refer to the same graph representation.
class Surface
{
public:
  /// @brief Constructs a null reference (no graph, invalid id).
  Surface(occtl_graph_t* theGraph = nullptr, occtl_rep_id_t theId = occtl_rep_id_t{0}) noexcept
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

  /// @brief Returns a U-reversed copy of this surface stored in the same graph.
  /// @throws Error when the underlying call fails.
  Surface reversed() const
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_reverse(myGraph, myId, &aId));
    return Surface(myGraph, aId);
  }

  /// @brief Returns a transformed copy of this surface stored in the same graph.
  /// @throws Error when the underlying call fails.
  Surface transformed(const Transform& theTrsf) const
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_transformed(myGraph, myId, theTrsf.c_type(), &aId));
    return Surface(myGraph, aId);
  }

  /// @brief Returns a translated copy of this surface stored in the same graph.
  /// @throws Error when the underlying call fails.
  Surface translated(const Vector3& theDelta) const
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_translated(myGraph, myId, theDelta.c_type(), &aId));
    return Surface(myGraph, aId);
  }

  /// @brief Returns a rotated copy of this surface stored in the same graph.
  /// @throws Error when the underlying call fails.
  Surface rotated(const Axis1Placement& theAxis, double theAngle) const
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_rotated(myGraph, myId, theAxis.c_type(), theAngle, &aId));
    return Surface(myGraph, aId);
  }

  /// @brief Returns a scaled copy of this surface stored in the same graph.
  /// @throws Error when the underlying call fails.
  Surface scaled(const Point3& theOrigin, double theFactor) const
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_scaled(myGraph, myId, theOrigin.c_type(), theFactor, &aId));
    return Surface(myGraph, aId);
  }

  /// @brief Returns the surface area over its full UV domain.
  /// @throws Error when the underlying call fails.
  double area() const
  {
    double anArea = 0.0;
    check(::occtl_surface_area(myGraph, myId, &anArea));
    return anArea;
  }

  /// @brief Projects a point onto the surface.
  /// @returns A tuple of (u, v, distance).
  /// @throws Error when the underlying call fails.
  std::tuple<double, double, double> project_point(const Point3& thePoint) const
  {
    double aU = 0.0, aV = 0.0, aDist = 0.0;
    check(::occtl_surface_project_point(myGraph, myId, thePoint.c_type(), &aU, &aV, &aDist));
    return {aU, aV, aDist};
  }

  /// @brief Returns the UV coordinates on the surface nearest to a point.
  /// @returns A pair of (u, v).
  /// @throws Error when the underlying call fails.
  std::pair<double, double> uv_of_point(const Point3& thePoint) const
  {
    double aU = 0.0, aV = 0.0;
    check(::occtl_surface_uv_of_point(myGraph, myId, thePoint.c_type(), &aU, &aV));
    return {aU, aV};
  }

  /// @brief Returns the kind of surface this handle holds.
  /// @throws Error when the underlying call fails.
  occtl_surface_kind_t kind() const
  {
    occtl_surface_kind_t aKind{};
    check(::occtl_surface_kind(myGraph, myId, &aKind));
    return aKind;
  }

  /// @brief Returns 1 when the surface is periodic in the U direction.
  /// @throws Error when the underlying call fails.
  int32_t is_u_periodic() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_is_u_periodic(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns 1 when the surface is periodic in the V direction.
  /// @throws Error when the underlying call fails.
  int32_t is_v_periodic() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_is_v_periodic(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns 1 when the surface is closed in both U and V directions.
  /// @throws Error when the underlying call fails.
  int32_t is_closed() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_is_closed(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns 1 when the surface is periodic in either U or V direction.
  /// @throws Error when the underlying call fails.
  int32_t is_periodic() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_is_periodic(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the geometric continuity class.
  /// @throws Error when the underlying call fails.
  occtl_geom_continuity_t continuity() const
  {
    occtl_geom_continuity_t aCont{};
    check(::occtl_surface_continuity(myGraph, myId, &aCont));
    return aCont;
  }

  /// @brief Returns the parameter range via out-parameters.
  /// @throws Error when the underlying call fails.
  void parameter_range(double& theUMin, double& theUMax, double& theVMin, double& theVMax) const
  {
    check(::occtl_surface_parameter_range(myGraph, myId, &theUMin, &theUMax, &theVMin, &theVMax));
  }

  /// @brief Extracts the underlying plane.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_SURFACE_KIND_PLANE.
  occtl_geom_plane_t as_plane() const
  {
    occtl_geom_plane_t aOut{};
    check(::occtl_surface_as_plane(myGraph, myId, &aOut));
    return aOut;
  }

  /// @brief Extracts the underlying cylinder.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_SURFACE_KIND_CYLINDRICAL.
  occtl_geom_cylindrical_surface_t as_cylinder() const
  {
    occtl_geom_cylindrical_surface_t aOut{};
    check(::occtl_surface_as_cylinder(myGraph, myId, &aOut));
    return aOut;
  }

  /// @brief Extracts the underlying cone.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_SURFACE_KIND_CONICAL.
  occtl_geom_conical_surface_t as_cone() const
  {
    occtl_geom_conical_surface_t aOut{};
    check(::occtl_surface_as_cone(myGraph, myId, &aOut));
    return aOut;
  }

  /// @brief Extracts the underlying sphere.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_SURFACE_KIND_SPHERICAL.
  occtl_geom_spherical_surface_t as_sphere() const
  {
    occtl_geom_spherical_surface_t aOut{};
    check(::occtl_surface_as_sphere(myGraph, myId, &aOut));
    return aOut;
  }

  /// @brief Extracts the underlying torus.
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != @c OCCTL_SURFACE_KIND_TOROIDAL.
  occtl_geom_toroidal_surface_t as_torus() const
  {
    occtl_geom_toroidal_surface_t aOut{};
    check(::occtl_surface_as_torus(myGraph, myId, &aOut));
    return aOut;
  }

  struct RevolutionView;
  struct ExtrusionView;
  struct RectangularTrimmedView;
  struct OffsetView;
  struct BSplineView;

  inline RevolutionView         as_revolution() const;
  inline ExtrusionView          as_extrusion() const;
  inline RectangularTrimmedView as_rectangular_trimmed() const;
  inline OffsetView             as_offset() const;

  /// @brief Returns a aggregate inspection view of a B-spline surface.
  ///
  /// The returned view's pointer fields borrow from this surface's graph rep
  /// and remain valid until the rep is removed from the graph.
  ///
  /// @throws Error with @c OCCTL_WRONG_KIND when kind() != BSPLINE,
  ///         or @c OCCTL_VERSION_MISMATCH on unsupported layout version.
  inline BSplineView as_bspline() const;

  /// @brief Returns the U polynomial degree.
  /// @throws Error when kind != BSPLINE.
  int32_t bspline_u_degree() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_bspline_u_degree(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the V polynomial degree.
  /// @throws Error when kind != BSPLINE.
  int32_t bspline_v_degree() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_bspline_v_degree(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the number of poles in the U direction.
  /// @throws Error when kind != BSPLINE.
  size_t bspline_u_pole_count() const
  {
    size_t aVal = 0;
    check(::occtl_surface_bspline_u_pole_count(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the number of poles in the V direction.
  /// @throws Error when kind != BSPLINE.
  size_t bspline_v_pole_count() const
  {
    size_t aVal = 0;
    check(::occtl_surface_bspline_v_pole_count(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the number of distinct U knot values.
  /// @throws Error when kind != BSPLINE.
  size_t bspline_u_knot_count() const
  {
    size_t aVal = 0;
    check(::occtl_surface_bspline_u_knot_count(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the number of distinct V knot values.
  /// @throws Error when kind != BSPLINE.
  size_t bspline_v_knot_count() const
  {
    size_t aVal = 0;
    check(::occtl_surface_bspline_v_knot_count(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns 1 when the surface is rational in either U or V, 0 otherwise.
  /// @throws Error when kind != BSPLINE.
  int32_t bspline_is_rational() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_bspline_is_rational(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the U degree of a Bezier surface.
  /// @throws Error when kind != BEZIER.
  int32_t bezier_u_degree() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_bezier_u_degree(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the V degree of a Bezier surface.
  /// @throws Error when kind != BEZIER.
  int32_t bezier_v_degree() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_bezier_v_degree(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the number of poles in the U direction of a Bezier surface.
  /// @throws Error when kind != BEZIER.
  size_t bezier_u_pole_count() const
  {
    size_t aVal = 0;
    check(::occtl_surface_bezier_u_pole_count(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns the number of poles in the V direction of a Bezier surface.
  /// @throws Error when kind != BEZIER.
  size_t bezier_v_pole_count() const
  {
    size_t aVal = 0;
    check(::occtl_surface_bezier_v_pole_count(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Returns 1 when the Bezier surface is rational, 0 otherwise.
  /// @throws Error when kind != BEZIER.
  int32_t bezier_is_rational() const
  {
    int32_t aVal = 0;
    check(::occtl_surface_bezier_is_rational(myGraph, myId, &aVal));
    return aVal;
  }

  /// @brief Copies poles (row-major: u * v_pole_count + v) into a vector.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<occtl_point3_t> bspline_poles() const
  {
    size_t aNb = 0;
    check(::occtl_surface_bspline_poles(myGraph, myId, nullptr, 0, &aNb));
    std::vector<occtl_point3_t> aVec(aNb);
    check(::occtl_surface_bspline_poles(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies U knot values into a vector.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<double> bspline_u_knots() const
  {
    size_t aNb = 0;
    check(::occtl_surface_bspline_u_knots(myGraph, myId, nullptr, 0, &aNb));
    std::vector<double> aVec(aNb);
    check(::occtl_surface_bspline_u_knots(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies V knot values into a vector.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<double> bspline_v_knots() const
  {
    size_t aNb = 0;
    check(::occtl_surface_bspline_v_knots(myGraph, myId, nullptr, 0, &aNb));
    std::vector<double> aVec(aNb);
    check(::occtl_surface_bspline_v_knots(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies U knot multiplicities into a vector.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<int32_t> bspline_u_multiplicities() const
  {
    size_t aNb = 0;
    check(::occtl_surface_bspline_u_multiplicities(myGraph, myId, nullptr, 0, &aNb));
    std::vector<int32_t> aVec(aNb);
    check(::occtl_surface_bspline_u_multiplicities(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies V knot multiplicities into a vector.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<int32_t> bspline_v_multiplicities() const
  {
    size_t aNb = 0;
    check(::occtl_surface_bspline_v_multiplicities(myGraph, myId, nullptr, 0, &aNb));
    std::vector<int32_t> aVec(aNb);
    check(::occtl_surface_bspline_v_multiplicities(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies all weights (row-major u*v_pole_count + v) into a vector.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<double> bspline_weights() const
  {
    size_t aNb = 0;
    check(::occtl_surface_bspline_weights(myGraph, myId, nullptr, 0, &aNb));
    std::vector<double> aVec(aNb);
    check(::occtl_surface_bspline_weights(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies the expanded U flat knot sequence into a vector.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<double> bspline_u_flat_knots() const
  {
    size_t aNb = 0;
    check(::occtl_surface_bspline_u_flat_knots(myGraph, myId, nullptr, 0, &aNb));
    std::vector<double> aVec(aNb);
    check(::occtl_surface_bspline_u_flat_knots(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Copies the expanded V flat knot sequence into a vector.
  /// @throws Error when kind != BSPLINE or extraction fails.
  std::vector<double> bspline_v_flat_knots() const
  {
    size_t aNb = 0;
    check(::occtl_surface_bspline_v_flat_knots(myGraph, myId, nullptr, 0, &aNb));
    std::vector<double> aVec(aNb);
    check(::occtl_surface_bspline_v_flat_knots(myGraph, myId, aVec.data(), aNb, &aNb));
    return aVec;
  }

  /// @brief Returns a zero-copy view of the surface poles array (row-major u*v_pole_count + v).
  /// Valid until the graph rep is removed or a mutating call is made on it.
  /// @throws Error when kind != BSPLINE or view is unavailable.
  const occtl_point3_t* bspline_poles_view(size_t& theNbU, size_t& theNbV) const
  {
    const occtl_point3_t* aData = nullptr;
    check(::occtl_surface_bspline_poles_view(myGraph, myId, &aData, &theNbU, &theNbV));
    return aData;
  }

  /// @brief Constructs a surface from a plane.
  /// @throws Error on invalid geometry.
  static Surface from_plane(occtl_graph_t* graph, const occtl_geom_plane_t& thePlane)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_plane(graph, &aId, thePlane));
    return Surface(graph, aId);
  }

  /// @brief Constructs a surface from a cylinder.
  /// @throws Error on invalid geometry.
  static Surface from_cylinder(occtl_graph_t* graph, const occtl_geom_cylindrical_surface_t& theCyl)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_cylinder(graph, &aId, theCyl));
    return Surface(graph, aId);
  }

  /// @brief Constructs a surface from a cone.
  /// @throws Error on invalid geometry.
  static Surface from_cone(occtl_graph_t* graph, const occtl_geom_conical_surface_t& theCone)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_cone(graph, &aId, theCone));
    return Surface(graph, aId);
  }

  /// @brief Constructs a surface from a sphere.
  /// @throws Error on invalid geometry.
  static Surface from_sphere(occtl_graph_t* graph, const occtl_geom_spherical_surface_t& theSphere)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_sphere(graph, &aId, theSphere));
    return Surface(graph, aId);
  }

  /// @brief Constructs a surface from a torus.
  /// @throws Error on invalid geometry.
  static Surface from_torus(occtl_graph_t* graph, const occtl_geom_toroidal_surface_t& theTorus)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_torus(graph, &aId, theTorus));
    return Surface(graph, aId);
  }

  /// @brief Constructs a B-spline surface from the given create info.
  /// @throws Error on invalid geometry or version mismatch.
  static Surface from_bspline(occtl_graph_t*                             graph,
                              const occtl_surface_bspline_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_bspline(graph, &aId, &theInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs a Bezier surface from the given create info.
  /// @throws Error on invalid geometry or version mismatch.
  static Surface from_bezier(occtl_graph_t*                            graph,
                             const occtl_surface_bezier_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_bezier(graph, &aId, &theInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs a Bezier surface from a row-major pole grid.
  /// @throws Error on invalid geometry or version mismatch.
  static Surface from_bezier_grid(occtl_graph_t*                            graph,
                                  const occtl_surface_bezier_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_bezier_grid(graph, &aId, &theInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs a surface of revolution.
  /// @throws Error on invalid geometry or version mismatch.
  static Surface from_revolution(occtl_graph_t*                                graph,
                                 const occtl_surface_revolution_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_revolution(graph, &aId, &theInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs a surface of linear extrusion.
  /// @throws Error on invalid geometry or version mismatch.
  static Surface from_extrusion(occtl_graph_t*                               graph,
                                const occtl_surface_extrusion_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_extrusion(graph, &aId, &theInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs a rectangular-trimmed surface.
  static Surface from_rectangular_trimmed(
    occtl_graph_t*                                         graph,
    const occtl_surface_rectangular_trimmed_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_rectangular_trimmed(graph, &aId, &theInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs an offset surface.
  static Surface from_offset(occtl_graph_t*                            graph,
                             const occtl_surface_offset_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_offset(graph, &aId, &theInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs a surface by interpolating through the given point grid.
  /// @throws Error on invalid geometry or version mismatch.
  static Surface from_interpolation(occtl_graph_t*                           graph,
                                    const occtl_surface_interpolated_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_interpolated(graph, &theInfo, &aId));
    return Surface(graph, aId);
  }

  /// @brief Constructs a surface by approximating the given point grid.
  /// @throws Error on invalid geometry or version mismatch.
  static Surface from_approximation(occtl_graph_t*                           graph,
                                    const occtl_surface_approximated_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_approximated(graph, &theInfo, &aId));
    return Surface(graph, aId);
  }

  /// @brief Constructs a B-spline surface from a point grid.
  /// @throws Error on invalid geometry or version mismatch.
  static Surface from_point_grid(occtl_graph_t*                                graph,
                                 const occtl_surface_point_grid_create_info_t& theInfo)
  {
    occtl_rep_id_t aId{};
    check(::occtl_surface_create_from_point_grid(graph, &aId, &theInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs a B-spline surface from contiguous boundary curves.
  static Surface from_boundary_curves(
    occtl_graph_t*                   graph,
    const std::vector<const Curve*>& theCurves,
    ::occtl_surface_filling_style_t  theStyle = OCCTL_SURFACE_FILLING_STRETCH)
  {
    std::vector<occtl_rep_id_t> aRaw;
    aRaw.reserve(theCurves.size());
    for (const Curve* aCurve : theCurves)
    {
      aRaw.push_back(aCurve == nullptr ? occtl_rep_id_t{0} : aCurve->id());
    }

    occtl_surface_boundary_curves_create_info_t anInfo =
      OCCTL_SURFACE_BOUNDARY_CURVES_CREATE_INFO_INIT;
    anInfo.curves      = aRaw.empty() ? nullptr : aRaw.data();
    anInfo.curve_count = aRaw.size();
    anInfo.style       = theStyle;

    occtl_rep_id_t aId{};
    check(::occtl_surface_create_from_boundary_curves(graph, &aId, &anInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs a Gordon B-spline surface from profile and guide curves.
  static Surface from_gordon(occtl_graph_t*                   graph,
                             const std::vector<const Curve*>& theProfiles,
                             const std::vector<const Curve*>& theGuides,
                             const double                     theTolerance = 1.0e-7,
                             const bool                       theParallel  = false)
  {
    std::vector<occtl_rep_id_t> aProfileRaw;
    aProfileRaw.reserve(theProfiles.size());
    for (const Curve* aCurve : theProfiles)
    {
      aProfileRaw.push_back(aCurve == nullptr ? occtl_rep_id_t{0} : aCurve->id());
    }

    std::vector<occtl_rep_id_t> aGuideRaw;
    aGuideRaw.reserve(theGuides.size());
    for (const Curve* aCurve : theGuides)
    {
      aGuideRaw.push_back(aCurve == nullptr ? occtl_rep_id_t{0} : aCurve->id());
    }

    occtl_surface_gordon_create_info_t anInfo = OCCTL_SURFACE_GORDON_CREATE_INFO_INIT;
    anInfo.profiles                           = aProfileRaw.empty() ? nullptr : aProfileRaw.data();
    anInfo.profile_count                      = aProfileRaw.size();
    anInfo.guides                             = aGuideRaw.empty() ? nullptr : aGuideRaw.data();
    anInfo.guide_count                        = aGuideRaw.size();
    anInfo.tolerance                          = theTolerance;
    anInfo.parallel                           = theParallel ? 1 : 0;

    occtl_rep_id_t aId{};
    check(::occtl_surface_create_gordon(graph, &aId, &anInfo));
    return Surface(graph, aId);
  }

  /// @brief Constructs a B-spline surface from an intersecting U/V curve grid.
  static Surface from_curve_grid(occtl_graph_t*                   graph,
                                 const std::vector<const Curve*>& theUCurves,
                                 const std::vector<const Curve*>& theVCurves,
                                 const double                     theTolerance = 1.0e-7,
                                 const bool                       theParallel  = false)
  {
    std::vector<occtl_rep_id_t> aURaw;
    aURaw.reserve(theUCurves.size());
    for (const Curve* aCurve : theUCurves)
    {
      aURaw.push_back(aCurve == nullptr ? occtl_rep_id_t{0} : aCurve->id());
    }

    std::vector<occtl_rep_id_t> aVRaw;
    aVRaw.reserve(theVCurves.size());
    for (const Curve* aCurve : theVCurves)
    {
      aVRaw.push_back(aCurve == nullptr ? occtl_rep_id_t{0} : aCurve->id());
    }

    occtl_surface_curve_grid_create_info_t anInfo = OCCTL_SURFACE_CURVE_GRID_CREATE_INFO_INIT;
    anInfo.u_curves                               = aURaw.empty() ? nullptr : aURaw.data();
    anInfo.u_curve_count                          = aURaw.size();
    anInfo.v_curves                               = aVRaw.empty() ? nullptr : aVRaw.data();
    anInfo.v_curve_count                          = aVRaw.size();
    anInfo.tolerance                              = theTolerance;
    anInfo.parallel                               = theParallel ? 1 : 0;

    occtl_rep_id_t aId{};
    check(::occtl_surface_create_from_curve_grid(graph, &aId, &anInfo));
    return Surface(graph, aId);
  }

  /// @brief Computes intersection points with a curve.  Two-call pattern handled
  /// internally; returns a vector of intersection points.
  /// @throws Error on NULL handle or computation failure.
  std::vector<occtl_point3_t> intersect_curve(const Curve& theCurve) const
  {
    size_t aNb = 0;
    check(::occtl_surface_intersect_curve(myGraph, myId, theCurve.id(), nullptr, 0, &aNb));
    std::vector<occtl_point3_t> aResult(aNb);
    check(::occtl_surface_intersect_curve(myGraph, myId, theCurve.id(), aResult.data(), aNb, &aNb));
    return aResult;
  }

  /// @brief Computes intersection curves with another surface.
  /// Each result curve wraps a rep id in the same graph.
  /// @throws Error on NULL handle or computation failure.
  std::vector<Curve> intersect_surface(const Surface& theOther, double theTolerance = 1.0e-7) const
  {
    occtl_rep_id_t* aRaw = nullptr;
    size_t          aNb  = 0;
    check(
      ::occtl_surface_surface_intersect(myGraph, myId, theOther.myId, theTolerance, &aRaw, &aNb));
    std::vector<Curve> aResult;
    aResult.reserve(aNb);
    for (size_t anI = 0; anI < aNb; ++anI)
    {
      aResult.emplace_back(myGraph, aRaw[anI]);
    }
    std::free(aRaw);
    return aResult;
  }

  /// @brief Evaluates the surface point at (theU, theV).
  /// @throws Error on NULL graph or out-of-range.
  occtl_point3_t eval_d0(const double theU, const double theV) const
  {
    occtl_point3_t aP{};
    check(::occtl_surface_eval_d0(myGraph, myId, theU, theV, &aP));
    return aP;
  }

  /// @brief Evaluates point and first-order partial derivatives at (theU, theV).
  /// Returns (point, dS/du, dS/dv).
  /// @throws Error on NULL graph or out-of-range.
  std::tuple<occtl_point3_t, occtl_vector3_t, occtl_vector3_t> eval_d1(const double theU,
                                                                       const double theV) const
  {
    occtl_point3_t  aP{};
    occtl_vector3_t aD1U{}, aD1V{};
    check(::occtl_surface_eval_d1(myGraph, myId, theU, theV, &aP, &aD1U, &aD1V));
    return {aP, aD1U, aD1V};
  }

  /// @brief Evaluates point and first- and second-order partial derivatives
  /// at (theU, theV).
  /// Returns (point, dS/du, dS/dv, d²S/du², d²S/dv², d²S/dudv).
  /// @throws Error on NULL graph or out-of-range.
  std::tuple<occtl_point3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t>
    eval_d2(const double theU, const double theV) const
  {
    occtl_point3_t  aP{};
    occtl_vector3_t aD1U{}, aD1V{}, aD2U{}, aD2V{}, aD2UV{};
    check(
      ::occtl_surface_eval_d2(myGraph, myId, theU, theV, &aP, &aD1U, &aD1V, &aD2U, &aD2V, &aD2UV));
    return {aP, aD1U, aD1V, aD2U, aD2V, aD2UV};
  }

  /// @brief Evaluates point and first-, second-, and third-order partial
  /// derivatives at (theU, theV).
  /// @throws Error on NULL graph or out-of-range.
  std::tuple<occtl_point3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t,
             occtl_vector3_t>
    eval_d3(const double theU, const double theV) const
  {
    occtl_point3_t  aP{};
    occtl_vector3_t aD1U{}, aD1V{}, aD2U{}, aD2V{}, aD2UV{};
    occtl_vector3_t aD3U{}, aD3V{}, aD3UUV{}, aD3UVV{};
    check(::occtl_surface_eval_d3(myGraph,
                                  myId,
                                  theU,
                                  theV,
                                  &aP,
                                  &aD1U,
                                  &aD1V,
                                  &aD2U,
                                  &aD2V,
                                  &aD2UV,
                                  &aD3U,
                                  &aD3V,
                                  &aD3UUV,
                                  &aD3UVV));
    return {aP, aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV};
  }

  /// @brief Returns a partial derivative vector at (theU, theV).
  /// @param theNu  Derivative order in U (>= 0).
  /// @param theNv  Derivative order in V (>= 0).
  /// @throws Error on NULL graph or out-of-range.
  occtl_vector3_t eval_dn(const double  theU,
                          const double  theV,
                          const int32_t theNu,
                          const int32_t theNv) const
  {
    occtl_vector3_t aD{};
    check(::occtl_surface_eval_dn(myGraph, myId, theU, theV, theNu, theNv, &aD));
    return aD;
  }

private:
  occtl_graph_t* myGraph;
  occtl_rep_id_t myId;
};

struct Surface::RevolutionView
{
  occtl_axis1_placement_t axis;
};

struct Surface::ExtrusionView
{
  occtl_vector3_t direction;
};

struct Surface::RectangularTrimmedView
{
  double u_first;
  double u_last;
  double v_first;
  double v_last;
};

struct Surface::OffsetView
{
  double offset;
};

inline Surface::RevolutionView Surface::as_revolution() const
{
  occtl_axis1_placement_t aAxis{};
  check(::occtl_surface_as_revolution(myGraph, myId, &aAxis));
  return {aAxis};
}

inline Surface::ExtrusionView Surface::as_extrusion() const
{
  occtl_vector3_t aDir{};
  check(::occtl_surface_as_extrusion(myGraph, myId, &aDir));
  return {aDir};
}

inline Surface::RectangularTrimmedView Surface::as_rectangular_trimmed() const
{
  double aU0 = 0.0, aU1 = 0.0, aV0 = 0.0, aV1 = 0.0;
  check(::occtl_surface_as_rectangular_trimmed(myGraph, myId, &aU0, &aU1, &aV0, &aV1));
  return {aU0, aU1, aV0, aV1};
}

inline Surface::OffsetView Surface::as_offset() const
{
  double aOff = 0.0;
  check(::occtl_surface_as_offset(myGraph, myId, &aOff));
  return {aOff};
}

/// @brief Aggregate inspection view of a B-spline surface.
///
/// Holds the raw @c occtl_surface_bspline_t plus typed accessors.  Pointer
/// fields borrow from the parent @c Surface's graph rep and stay valid as
/// long as the rep remains in the graph.  The pole and weight grids are
/// exposed as a single span in row-major order with U as the major axis:
/// element @c (u, v) is at offset @c u * v_pole_count + v.
struct Surface::BSplineView
{
  occtl_surface_bspline_t raw;

  int32_t u_degree() const noexcept { return raw.u_degree; }

  int32_t v_degree() const noexcept { return raw.v_degree; }

  bool is_rational() const noexcept { return raw.is_rational != 0; }

  bool is_u_periodic() const noexcept { return raw.is_u_periodic != 0; }

  bool is_v_periodic() const noexcept { return raw.is_v_periodic != 0; }

  size_t u_pole_count() const noexcept { return raw.u_pole_count; }

  size_t v_pole_count() const noexcept { return raw.v_pole_count; }

  size_t u_knot_count() const noexcept { return raw.u_knot_count; }

  size_t v_knot_count() const noexcept { return raw.v_knot_count; }

  size_t u_flat_knot_count() const noexcept { return raw.u_flat_knot_count; }

  size_t v_flat_knot_count() const noexcept { return raw.v_flat_knot_count; }

#if OCCTL_HPP_HAS_SPAN
  std::span<const occtl_point3_t> poles() const noexcept
  {
    return {raw.poles, raw.u_pole_count * raw.v_pole_count};
  }

  std::span<const double> weights() const noexcept
  {
    return raw.weights ? std::span<const double>{raw.weights, raw.u_pole_count * raw.v_pole_count}
                       : std::span<const double>{};
  }

  std::span<const double> u_knots() const noexcept { return {raw.u_knots, raw.u_knot_count}; }

  std::span<const double> v_knots() const noexcept { return {raw.v_knots, raw.v_knot_count}; }

  std::span<const int32_t> u_multiplicities() const noexcept
  {
    return {raw.u_multiplicities, raw.u_knot_count};
  }

  std::span<const int32_t> v_multiplicities() const noexcept
  {
    return {raw.v_multiplicities, raw.v_knot_count};
  }

  std::span<const double> u_flat_knots() const noexcept
  {
    return {raw.u_flat_knots, raw.u_flat_knot_count};
  }

  std::span<const double> v_flat_knots() const noexcept
  {
    return {raw.v_flat_knots, raw.v_flat_knot_count};
  }
#else
  const occtl_point3_t* poles() const noexcept { return raw.poles; }

  const double* weights() const noexcept { return raw.weights; }

  const double* u_knots() const noexcept { return raw.u_knots; }

  const double* v_knots() const noexcept { return raw.v_knots; }

  const int32_t* u_multiplicities() const noexcept { return raw.u_multiplicities; }

  const int32_t* v_multiplicities() const noexcept { return raw.v_multiplicities; }

  const double* u_flat_knots() const noexcept { return raw.u_flat_knots; }

  const double* v_flat_knots() const noexcept { return raw.v_flat_knots; }
#endif
};

inline Surface::BSplineView Surface::as_bspline() const
{
  BSplineView aView;
  ::occtl_surface_bspline_init(&aView.raw);
  check(::occtl_surface_as_bspline(myGraph, myId, &aView.raw));
  return aView;
}

} // namespace occtl

#endif // OCCTL_HPP_SURFACES_HPP
