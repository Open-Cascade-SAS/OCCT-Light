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
 * @file occtl_prim_solid.h
 * @brief OCCT-Light: analytic primitive solid construction API.
 */

#ifndef OCCTL_PRIM_SOLID_H
#define OCCTL_PRIM_SOLID_H

#include "occtl_core.h"
#include "occtl_curves2d.h"
#include "occtl_geom.h"
#include "occtl_surfaces.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_PRIM_BOX_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_box.
 *
 * Constructs an axis-aligned parallelepiped of size @c dx by @c dy by @c dz
 * with one corner at @c placement.location and edges aligned to the frame
 * axes derived from @c placement. The default @c placement is the global
 * XOY frame (origin at @c (0,0,0), Z up, X along @c +x), so casual callers
 * can construct an axis-aligned box at the origin by editing only the three
 * dimensions.
 */
typedef struct occtl_prim_box_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_BOX_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Local frame; defaults to XOY. */
  double                  dx;             /**< Edge length along the frame X axis. */
  double                  dy;             /**< Edge length along the frame Y axis. */
  double                  dz;             /**< Edge length along the frame Z axis. */
} occtl_prim_box_info_t;

#define OCCTL_PRIM_BOX_INFO_INIT                                                                   \
  {OCCTL_PRIM_BOX_INFO_VERSION_1,                                                                  \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_box_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_BOX_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_box
 */
OCCTL_API void OCCTL_CALL occtl_prim_box_info_init(occtl_prim_box_info_t* info);

/**
 * Builds an axis-aligned box solid and inserts it into @p graph.
 *
 * The result is appended as a bare Solid root (no enclosing Product wrapper).
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version and strictly positive @c dx /
 *                          @c dy / @c dz.
 * @param[out]    out_solid Borrows it (caller-allocated slot). Must be
 *                          non-NULL. On success receives the new Solid
 *                          NodeId; on failure set to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL, @c p_next is non-NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Any dimension is non-positive or below
 *                                 the library's geometric precision.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No — mutates @p graph.
 *
 * @sa occtl_prim_make_sphere, occtl_prim_make_cylinder
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_box(occtl_graph_t*               graph,
                                                        const occtl_prim_box_info_t* info,
                                                        occtl_node_id_t*             out_solid);

#define OCCTL_PRIM_SPHERE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_sphere.
 *
 * A full sphere has @c angle1 = -pi/2, @c angle2 = +pi/2, and
 * @c angle = 2*pi (the defaults set by #OCCTL_PRIM_SPHERE_INFO_INIT).
 * Truncating @c angle1 / @c angle2 yields a spherical segment; truncating
 * @c angle yields a longitudinal wedge.
 */
typedef struct occtl_prim_sphere_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_SPHERE_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Local frame; centre = @c location, axis = Z. */
  double                  radius;         /**< Sphere radius; strictly positive. */
  double                  angle1;         /**< First latitude bound in radians; default @c -pi/2. */
  double                  angle2; /**< Second latitude bound in radians; default @c +pi/2. */
  double                  angle;  /**< Longitude span in radians; default @c 2*pi. */
} occtl_prim_sphere_info_t;

#define OCCTL_PRIM_SPHERE_INFO_INIT                                                                \
  {OCCTL_PRIM_SPHERE_INFO_VERSION_1,                                                               \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   -OCCTL_PI_OVER_TWO,                                                                             \
   OCCTL_PI_OVER_TWO,                                                                              \
   OCCTL_TWO_PI}

/**
 * Runtime initialiser for #occtl_prim_sphere_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_SPHERE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_sphere
 */
OCCTL_API void OCCTL_CALL occtl_prim_sphere_info_init(occtl_prim_sphere_info_t* info);

/**
 * Builds a sphere or spherical wedge solid and inserts it into @p graph.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_solid Borrows it. Must be non-NULL.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL, @c p_next is non-NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c radius is non-positive or angles are
 *                                 outside the legal range.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_sphere_info_init, occtl_prim_make_box
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_sphere(occtl_graph_t*                  graph,
                                                           const occtl_prim_sphere_info_t* info,
                                                           occtl_node_id_t* out_solid);

#define OCCTL_PRIM_CYLINDER_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_cylinder.
 *
 * The cylinder axis is the Z axis of @c placement; the base sits at
 * @c placement.location and the cap at @c placement.location +
 * @c height * Z. @c angle < @c 2*pi yields a partial cylinder.
 */
typedef struct occtl_prim_cylinder_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_CYLINDER_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Local frame; axis = Z. */
  double                  radius;         /**< Radius; strictly positive. */
  double                  height;         /**< Height along Z; strictly positive. */
  double                  angle;          /**< Sweep angle in radians; default @c 2*pi. */
} occtl_prim_cylinder_info_t;

#define OCCTL_PRIM_CYLINDER_INFO_INIT                                                              \
  {OCCTL_PRIM_CYLINDER_INFO_VERSION_1,                                                             \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   OCCTL_TWO_PI}

/**
 * Runtime initialiser for #occtl_prim_cylinder_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_CYLINDER_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_cylinder
 */
OCCTL_API void OCCTL_CALL occtl_prim_cylinder_info_init(occtl_prim_cylinder_info_t* info);

/**
 * Builds a cylinder or partial-cylinder solid.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_solid Borrows it. Must be non-NULL. On success
 *                          receives the new Solid NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL, @c p_next is non-NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  @c radius or @c height is non-positive.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_cylinder_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_cylinder(occtl_graph_t* graph,
                                                             const occtl_prim_cylinder_info_t* info,
                                                             occtl_node_id_t* out_solid);

#define OCCTL_PRIM_CONE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_cone.
 *
 * Builds a truncated cone (frustum) of height @c height between two parallel
 * circular faces of radius @c r1 at @c z = 0 and @c r2 at @c z = height,
 * with both circles centred on the Z axis of @c placement. Setting one of
 * @c r1 / @c r2 to @c 0 produces a sharp cone. @c angle < @c 2*pi yields a
 * partial cone.
 */
typedef struct occtl_prim_cone_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_CONE_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Local frame; axis = Z. */
  double                  r1;             /**< Base radius (at @c z = 0); non-negative. */
  double                  r2;             /**< Cap  radius (at @c z = height); non-negative. */
  double                  height;         /**< Height along Z; strictly positive. */
  double                  angle;          /**< Sweep angle in radians; default @c 2*pi. */
} occtl_prim_cone_info_t;

#define OCCTL_PRIM_CONE_INFO_INIT                                                                  \
  {OCCTL_PRIM_CONE_INFO_VERSION_1,                                                                 \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   OCCTL_TWO_PI}

/**
 * Runtime initialiser for #occtl_prim_cone_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_CONE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_cone
 */
OCCTL_API void OCCTL_CALL occtl_prim_cone_info_init(occtl_prim_cone_info_t* info);

/**
 * Builds a cone or truncated cone solid.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_solid Borrows it. Must be non-NULL. On success
 *                          receives the new Solid NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Inputs are degenerate (e.g. @c r1 == @c r2 == 0,
 *                                 @c height non-positive, half-angle out of range).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_cone_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_cone(occtl_graph_t*                graph,
                                                         const occtl_prim_cone_info_t* info,
                                                         occtl_node_id_t*              out_solid);

#define OCCTL_PRIM_TORUS_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_torus.
 *
 * @c r1 is the major radius (centreline of the ring); @c r2 is the minor
 * radius (pipe). @c angle1 / @c angle2 trim the pipe in its v parameter
 * (defaulting to @c 0 / @c 2*pi — a full pipe); @c angle trims the ring in
 * its u parameter (defaulting to @c 2*pi — a full torus).
 */
typedef struct occtl_prim_torus_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_TORUS_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Local frame; ring axis = Z. */
  double                  r1;             /**< Major radius; strictly positive. */
  double                  r2;             /**< Minor radius (pipe); strictly positive, @c < r1. */
  double                  angle1;         /**< Pipe trim start in radians; default @c 0. */
  double                  angle2;         /**< Pipe trim end   in radians; default @c 2*pi. */
  double                  angle;          /**< Ring sweep angle in radians; default @c 2*pi. */
} occtl_prim_torus_info_t;

#define OCCTL_PRIM_TORUS_INFO_INIT                                                                 \
  {OCCTL_PRIM_TORUS_INFO_VERSION_1,                                                                \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   OCCTL_TWO_PI,                                                                                   \
   OCCTL_TWO_PI}

/**
 * Runtime initialiser for #occtl_prim_torus_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_TORUS_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_torus
 */
OCCTL_API void OCCTL_CALL occtl_prim_torus_info_init(occtl_prim_torus_info_t* info);

/**
 * Builds a torus or torus segment solid.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_solid Borrows it. Must be non-NULL. On success
 *                          receives the new Solid NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Radii are degenerate (@c r2 >= @c r1 or non-positive),
 *                                 or angles are outside the legal range.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_torus_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_torus(occtl_graph_t*                 graph,
                                                          const occtl_prim_torus_info_t* info,
                                                          occtl_node_id_t*               out_solid);

#define OCCTL_PRIM_WEDGE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_wedge.
 *
 * Builds a STEP right-angular wedge — a box of size @c dx by @c dy by
 * @c dz with the @c +y face shrunk in X from @c dx to @c ltx. @c ltx must
 * be in @c [0, dx].
 */
typedef struct occtl_prim_wedge_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_WEDGE_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Local frame; defaults to XOY. */
  double                  dx;             /**< X size of the base face; strictly positive. */
  double                  dy;  /**< Y depth between the two trapezoidal faces; positive. */
  double                  dz;  /**< Z height of the base face; strictly positive. */
  double                  ltx; /**< X size of the top face; in @c [0, dx]. */
} occtl_prim_wedge_info_t;

#define OCCTL_PRIM_WEDGE_INFO_INIT                                                                 \
  {OCCTL_PRIM_WEDGE_INFO_VERSION_1,                                                                \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0,                                                                                            \
   0.0}

/**
 * Runtime initialiser for #occtl_prim_wedge_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_WEDGE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_wedge
 */
OCCTL_API void OCCTL_CALL occtl_prim_wedge_info_init(occtl_prim_wedge_info_t* info);

/**
 * Builds a right-angular wedge solid.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_solid Borrows it. Must be non-NULL. On success
 *                          receives the new Solid NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Inputs are degenerate (any size non-positive,
 *                                 @c ltx outside @c [0, dx]).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_wedge_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_wedge(occtl_graph_t*                 graph,
                                                          const occtl_prim_wedge_info_t* info,
                                                          occtl_node_id_t*               out_solid);

#define OCCTL_PRIM_HALFSPACE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_halfspace.
 *
 * A half-space is an infinite solid bounded by one face. Given a face
 * @c face from the graph and a reference point @c reference_point not on
 * the face's surface, the half-space is the side of the face containing
 * @c reference_point. Typical use is as a cutting tool for Booleans.
 */
typedef struct occtl_prim_halfspace_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_HALFSPACE_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_node_id_t face; /**< Borrows it. Bounding face; must be of kind #OCCTL_KIND_FACE. */
  occtl_point3_t  reference_point; /**< A point on the side of @c face where the matter lies. */
} occtl_prim_halfspace_info_t;

#define OCCTL_PRIM_HALFSPACE_INFO_INIT                                                             \
  {OCCTL_PRIM_HALFSPACE_INFO_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, {0.0, 0.0, 0.0}}

/**
 * Runtime initialiser for #occtl_prim_halfspace_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_HALFSPACE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_halfspace
 */
OCCTL_API void OCCTL_CALL occtl_prim_halfspace_info_init(occtl_prim_halfspace_info_t* info);

/**
 * Builds a half-space solid bounded by @c info->face on the side of @c info->reference_point.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_solid Borrows it. Must be non-NULL. On success
 *                          receives the new Solid NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL, @c p_next is non-NULL,
 *                                 or @c reference_point contains a non-finite component.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c face refers to a removed or absent node.
 * @retval OCCTL_WRONG_KIND        @c face is not of kind #OCCTL_KIND_FACE.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed (reference point on the face surface, etc.).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_halfspace_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_halfspace(occtl_graph_t*                     graph,
                            const occtl_prim_halfspace_info_t* info,
                            occtl_node_id_t*                   out_solid);

#ifdef __cplusplus
}
#endif

#endif /* OCCTL_PRIM_SOLID_H */
