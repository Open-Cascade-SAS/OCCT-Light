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
 * @file occtl_prim_sweep.h
 * @brief OCCT-Light: sweep, loft, offset, and thickening construction API.
 */

#ifndef OCCTL_PRIM_SWEEP_H
#define OCCTL_PRIM_SWEEP_H

#include "occtl_core.h"
#include "occtl_curves2d.h"
#include "occtl_geom.h"
#include "occtl_surfaces.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OCCTL_PRIM_PRISM_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_prism.
 *
 * Linearly extrudes @c profile by vector @c direction. The profile may be
 * a Vertex, Edge, Wire, Face, or Shell of @p graph; the result kind is one
 * step higher in dimension (Vertex→Edge, Edge→Face, Wire→Shell, Face→Solid,
 * Shell→CompSolid).
 */
typedef struct occtl_prim_prism_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_PRISM_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_node_id_t profile;        /**< Borrows it. Vertex/Edge/Wire/Face/Shell to extrude. */
  occtl_vector3_t direction;      /**< Extrusion vector (length = sweep distance). */
  int32_t         copy;           /**< 0/1. When 1, the profile is duplicated rather than shared. */
  int32_t canonize; /**< 0/1. When 1, swept surfaces are canonised to simple types. Default 1. */
} occtl_prim_prism_info_t;

#define OCCTL_PRIM_PRISM_INFO_INIT                                                                 \
  {OCCTL_PRIM_PRISM_INFO_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, {0.0, 0.0, 0.0}, 0, 1}

/**
 * Runtime initialiser for #occtl_prim_prism_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_PRISM_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_prism
 */
OCCTL_API void OCCTL_CALL occtl_prim_prism_info_init(occtl_prim_prism_info_t* info);

/**
 * Builds a linear extrusion of @c info->profile by @c info->direction.
 *
 * The profile is resolved from its NodeId, extruded, and the resulting
 * topology appended to the same graph.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL, or
 *                                 @c direction has zero length.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c profile refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c profile is of a kind that cannot be extruded
 *                                 (Solid, CompSolid, Compound, Product, Occurrence, CoEdge).
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_revol, occtl_prim_make_pipe
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_prism(occtl_graph_t*                 graph,
                                                          const occtl_prim_prism_info_t* info,
                                                          occtl_node_id_t*               out_shape);

#define OCCTL_PRIM_TWIST_EXTRUSION_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_twist_extrusion.
 *
 * Builds a twisted extrusion from one closed Wire section along
 * @c axis.direction, rotating each generated section around @c axis.
 * Face profiles with inner wires should be decomposed before calling.
 */
typedef struct occtl_prim_twist_extrusion_info
{
  uint32_t        struct_version;        /**< Must be #OCCTL_PRIM_TWIST_EXTRUSION_INFO_VERSION_1. */
  const void*     p_next;                /**< Reserved; must be NULL. */
  occtl_node_id_t profile_wire;          /**< Borrows it. Closed Wire section to sweep. */
  occtl_axis1_placement_t axis;          /**< Twist axis and extrusion direction. */
  double                  height;        /**< Signed extrusion distance along @c axis.direction. */
  double                  angle;         /**< Total twist angle in radians. */
  int32_t                 section_count; /**< Number of sampled sections; must be at least 2. */
  int32_t make_solid; /**< 0/1. When 1, cap into a Solid; otherwise build a Shell. */
  int32_t ruled;      /**< 0/1. When 1, use ruled faces between sections. */
  double  pres3d;     /**< Through-sections precision; default @c 1.0e-6. */
} occtl_prim_twist_extrusion_info_t;

#define OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT                                                       \
  {OCCTL_PRIM_TWIST_EXTRUSION_INFO_VERSION_1,                                                      \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}},                                                             \
   1.0,                                                                                            \
   0.0,                                                                                            \
   9,                                                                                              \
   1,                                                                                              \
   1,                                                                                              \
   1.0e-6}

/**
 * Runtime initialiser for #occtl_prim_twist_extrusion_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_TWIST_EXTRUSION_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_twist_extrusion
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_twist_extrusion_info_init(occtl_prim_twist_extrusion_info_t* info);

/**
 * Builds a twisted extrusion from a closed Wire profile.
 *
 * The result is inserted into @p graph as a new Shell or Solid root.
 * The input profile is not modified.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version and NULL @c p_next.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 axis direction is zero, @c height is zero,
 *                                 @c section_count < 2, or @c pres3d <= 0.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c profile_wire refers to a removed / absent
 *                                 node or cannot be reconstructed.
 * @retval OCCTL_WRONG_KIND        @c profile_wire is not a Wire.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the generated sections.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_prism, occtl_prim_make_loft
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_twist_extrusion(occtl_graph_t*                           graph,
                                  const occtl_prim_twist_extrusion_info_t* info,
                                  occtl_node_id_t*                         out_shape);

#define OCCTL_PRIM_EXTRUDE_TWIST_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_extrude_twist.
 *
 * Semantic wrapper for twisted extrusion of one closed Wire profile.
 * Delegates to #occtl_prim_make_twist_extrusion.
 */
typedef struct occtl_prim_extrude_twist_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_EXTRUDE_TWIST_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t         profile_wire;   /**< Borrows it. Closed Wire section to sweep. */
  occtl_axis1_placement_t axis;           /**< Twist axis and extrusion direction. */
  double                  height;         /**< Signed extrusion distance along @c axis.direction. */
  double                  angle;          /**< Total twist angle in radians. */
  int32_t                 section_count;  /**< Number of sampled sections; must be at least 2. */
  int32_t make_solid; /**< 0/1. When 1, cap into a Solid; otherwise build a Shell. */
  int32_t ruled;      /**< 0/1. When 1, use ruled faces between sections. */
  double  pres3d;     /**< Through-sections precision; default @c 1.0e-6. */
} occtl_prim_extrude_twist_info_t;

#define OCCTL_PRIM_EXTRUDE_TWIST_INFO_INIT                                                         \
  {OCCTL_PRIM_EXTRUDE_TWIST_INFO_VERSION_1,                                                        \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}},                                                             \
   1.0,                                                                                            \
   0.0,                                                                                            \
   9,                                                                                              \
   1,                                                                                              \
   1,                                                                                              \
   1.0e-6}

/**
 * Runtime initialiser for #occtl_prim_extrude_twist_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_EXTRUDE_TWIST_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_extrude_twist
 */
OCCTL_API void OCCTL_CALL occtl_prim_extrude_twist_info_init(occtl_prim_extrude_twist_info_t* info);

/**
 * Builds a twisted extrusion from a closed Wire profile.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 axis direction is zero, @c height is zero,
 *                                 @c section_count < 2, or @c pres3d <= 0.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c profile_wire refers to a removed / absent
 *                                 node or cannot be reconstructed.
 * @retval OCCTL_WRONG_KIND        @c profile_wire is not a Wire.
 * @retval OCCTL_GEOMETRY_INVALID  OCCT rejected the generated sections.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_twist_extrusion
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_extrude_twist(occtl_graph_t*                         graph,
                                const occtl_prim_extrude_twist_info_t* info,
                                occtl_node_id_t*                       out_shape);

#define OCCTL_PRIM_REVOL_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_revol.
 *
 * Revolves @c profile around @c axis by @c angle radians. Like prism, the
 * result kind is one step higher in dimension. The default @c angle is
 * @c 2*pi (full revolution).
 */
typedef struct occtl_prim_revol_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_PRIM_REVOL_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_node_id_t         profile; /**< Borrows it. Vertex/Edge/Wire/Face/Shell to revolve. */
  occtl_axis1_placement_t axis;    /**< Rotation axis. */
  double                  angle;   /**< Sweep angle in radians; default @c 2*pi. */
  int32_t                 copy; /**< 0/1. When 1, the profile is duplicated rather than shared. */
} occtl_prim_revol_info_t;

#define OCCTL_PRIM_REVOL_INFO_INIT                                                                 \
  {OCCTL_PRIM_REVOL_INFO_VERSION_1,                                                                \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}},                                                             \
   OCCTL_TWO_PI,                                                                                   \
   0}

/**
 * Runtime initialiser for #occtl_prim_revol_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_REVOL_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_revol
 */
OCCTL_API void OCCTL_CALL occtl_prim_revol_info_init(occtl_prim_revol_info_t* info);

/**
 * Builds a revolution of @c info->profile around @c info->axis by @c info->angle.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL, or the axis
 *                                 direction has zero length.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c profile refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c profile is of a kind that cannot be revolved.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_prism, occtl_prim_make_pipe
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_revol(occtl_graph_t*                 graph,
                                                          const occtl_prim_revol_info_t* info,
                                                          occtl_node_id_t*               out_shape);

#define OCCTL_PRIM_PIPE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_pipe.
 *
 * Sweeps @c profile along @c spine_wire (a G1-continuous wire of
 * @p graph). The angle between profile and spine is preserved along the
 * sweep.
 */
typedef struct occtl_prim_pipe_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_PIPE_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_node_id_t profile;        /**< Borrows it. Vertex/Edge/Wire/Face/Shell to sweep. */
  occtl_node_id_t spine_wire;     /**< Borrows it. Must be of kind #OCCTL_KIND_WIRE. */
} occtl_prim_pipe_info_t;

#define OCCTL_PRIM_PIPE_INFO_INIT                                                                  \
  {OCCTL_PRIM_PIPE_INFO_VERSION_1, NULL, OCCTL_NODE_ID_INVALID, OCCTL_NODE_ID_INVALID}

/**
 * Runtime initialiser for #occtl_prim_pipe_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_PIPE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_pipe
 */
OCCTL_API void OCCTL_CALL occtl_prim_pipe_info_init(occtl_prim_pipe_info_t* info);

/**
 * Builds a pipe by sweeping @c info->profile along @c info->spine_wire.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A node id refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c spine_wire is not a Wire, or @c profile
 *                                 is a Solid / CompSolid / Compound / Product /
 *                                 Occurrence / CoEdge.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed (non-G1 spine, etc.).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_pipe_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_pipe(occtl_graph_t*                graph,
                                                         const occtl_prim_pipe_info_t* info,
                                                         occtl_node_id_t*              out_shape);

#define OCCTL_PRIM_LOFT_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_loft.
 *
 * Builds a shell or solid passing through a sequence of section wires. The
 * first and last sections may be Vertices (degenerate sections); every
 * other section must be a Wire. @c sections / @c section_count form a
 * borrowed array; the implementation does not retain a pointer past the
 * call.
 */
typedef struct occtl_prim_loft_info
{
  uint32_t               struct_version; /**< Must be #OCCTL_PRIM_LOFT_INFO_VERSION_1. */
  const void*            p_next;         /**< Reserved for extensions; must be NULL. */
  const occtl_node_id_t* sections;       /**< Borrows it. Array of Wire / Vertex NodeIds. */
  size_t                 section_count;  /**< Length of @c sections; @c >= 2. */
  int32_t                is_solid;       /**< 0/1. When 1, the loft is closed into a solid. */
  int32_t                ruled;          /**< 0/1. When 1, faces between sections are ruled. */
  double                 pres3d;         /**< Smoothing precision; default @c 1.0e-6. */
} occtl_prim_loft_info_t;

#define OCCTL_PRIM_LOFT_INFO_INIT {OCCTL_PRIM_LOFT_INFO_VERSION_1, NULL, NULL, 0, 0, 0, 1.0e-6}

/**
 * Runtime initialiser for #occtl_prim_loft_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_LOFT_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_loft
 */
OCCTL_API void OCCTL_CALL occtl_prim_loft_info_init(occtl_prim_loft_info_t* info);

/**
 * Builds a loft (skin) through the sequence of section wires / vertices.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL, or @c p_next is non-NULL,
 *                                 or @c is_solid / @c ruled is not 0/1, or @c pres3d is
 *                                 non-positive, or @c section_count < 2, or @c sections is
 *                                 NULL while @c section_count > 0.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A section id refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        A section is neither Wire nor Vertex, or a
 *                                 Vertex appears at a non-endpoint position.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_loft_info_init
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_loft(occtl_graph_t*                graph,
                                                         const occtl_prim_loft_info_t* info,
                                                         occtl_node_id_t*              out_shape);

typedef enum occtl_prim_pipe_mode
{
  OCCTL_PIPE_MODE_CORRECTED_FRENET = 0, /**< Default. Frenet trihedron with C0 correction. */
  OCCTL_PIPE_MODE_FRENET           = 1, /**< Raw Frenet trihedron (no correction). */
  OCCTL_PIPE_MODE_DISCRETE         = 2, /**< Discrete trihedron — robust on non-smooth spines. */
  OCCTL_PIPE_MODE_CONSTANT_AXIS =
    3, /**< Fixed local frame given by #occtl_prim_pipe_shell_info_t::mode_axis. */
  OCCTL_PIPE_MODE_CONSTANT_BINORMAL =
    4, /**< Fixed binormal direction given by #occtl_prim_pipe_shell_info_t::mode_binormal. */
  OCCTL_PIPE_MODE_RESERVED_FUTURE = 0x7fffffff
} occtl_prim_pipe_mode_t;

/**
 * Corner-transition mode for #occtl_prim_make_pipe_shell.
 *
 * Selects how the sweep handles discontinuities in the spine's tangent.
 */
typedef enum occtl_prim_pipe_transition
{
  OCCTL_PIPE_TRANSITION_MODIFIED     = 0, /**< Default. Transform the profile across the corner. */
  OCCTL_PIPE_TRANSITION_RIGHT_CORNER = 1, /**< Insert a sharp corner. */
  OCCTL_PIPE_TRANSITION_ROUND_CORNER = 2, /**< Insert a rounded corner. */
  OCCTL_PIPE_TRANSITION_RESERVED_FUTURE = 0x7fffffff
} occtl_prim_pipe_transition_t;

#define OCCTL_PRIM_PIPE_SHELL_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_pipe_shell.
 *
 * Sweeps one or more profiles along a spine wire with explicit control
 * over the trihedron rule and corner transition. Each profile must be
 * of a sweepable kind (Vertex / Edge / Wire / Face / Shell / Compound)
 * and live in the same graph as @c spine_wire.
 */
typedef struct occtl_prim_pipe_shell_info
{
  uint32_t                struct_version;    /**< Must be #OCCTL_PRIM_PIPE_SHELL_INFO_VERSION_1. */
  const void*             p_next;            /**< Reserved; must be NULL. */
  occtl_node_id_t         spine_wire;        /**< Borrows it. Must be of kind #OCCTL_KIND_WIRE. */
  const occtl_node_id_t*  profiles;          /**< Borrows it. Array of profile NodeIds; @c >= 1. */
  size_t                  profile_count;     /**< Length of @c profiles. */
  occtl_prim_pipe_mode_t  mode;              /**< Trihedron rule; defaults to CORRECTED_FRENET. */
  occtl_axis2_placement_t mode_axis;         /**< Used when @c mode == CONSTANT_AXIS. */
  occtl_direction3_t      mode_binormal;     /**< Used when @c mode == CONSTANT_BINORMAL. */
  occtl_prim_pipe_transition_t transition;   /**< Corner handling; defaults to MODIFIED. */
  int32_t                      with_contact; /**< 0/1. Glue profiles to spine. */
  int32_t with_correction; /**< 0/1. Rotate profile so its normal aligns with the spine tangent. */
  int32_t make_solid;      /**< 0/1. Close the resulting shell into a solid. */
} occtl_prim_pipe_shell_info_t;

#define OCCTL_PRIM_PIPE_SHELL_INFO_INIT                                                            \
  {OCCTL_PRIM_PIPE_SHELL_INFO_VERSION_1,                                                           \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   NULL,                                                                                           \
   0,                                                                                              \
   OCCTL_PIPE_MODE_CORRECTED_FRENET,                                                               \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   {0.0, 0.0, 1.0},                                                                                \
   OCCTL_PIPE_TRANSITION_MODIFIED,                                                                 \
   0,                                                                                              \
   0,                                                                                              \
   0}

/**
 * Runtime initialiser for #occtl_prim_pipe_shell_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_PIPE_SHELL_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_pipe_shell
 */
OCCTL_API void OCCTL_CALL occtl_prim_pipe_shell_info_init(occtl_prim_pipe_shell_info_t* info);

/**
 * Builds a rich pipe-shell sweep with explicit mode / transition / contact control.
 *
 * The simpler #occtl_prim_make_pipe entry point is preferred when none of
 * the advanced options are needed; this richer variant exposes the full
 * set of trihedron, transition, contact, and correction options.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, or @c profile_count < 1, or
 *                                 the constant-binormal mode is selected with a zero-length
 * direction.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A node id refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c spine_wire is not a Wire, or a profile is of a kind that
 * cannot be swept.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed (incompatible profiles, degenerate spine, …).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_pipe
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_pipe_shell(occtl_graph_t*                      graph,
                             const occtl_prim_pipe_shell_info_t* info,
                             occtl_node_id_t*                    out_shape);

#define OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_pipe_shell_linear_law.
 *
 * Sweeps one profile along a spine wire while scaling the profile linearly
 * from @c scale_first at the spine start to @c scale_last at the spine end.
 * The profile must be sweepable (Vertex / Edge / Wire / Face / Shell /
 * Compound) and live in the same graph as @c spine_wire.
 */
typedef struct occtl_prim_pipe_shell_linear_law_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t spine_wire;     /**< Borrows it. Must be of kind #OCCTL_KIND_WIRE. */
  occtl_node_id_t profile;        /**< Borrows it. Profile NodeId to scale and sweep. */
  double          scale_first;    /**< Positive profile scale at the spine start. */
  double          scale_last;     /**< Positive profile scale at the spine end. */
  occtl_prim_pipe_mode_t       mode;          /**< Trihedron rule; defaults to CORRECTED_FRENET. */
  occtl_axis2_placement_t      mode_axis;     /**< Used when @c mode == CONSTANT_AXIS. */
  occtl_direction3_t           mode_binormal; /**< Used when @c mode == CONSTANT_BINORMAL. */
  occtl_prim_pipe_transition_t transition;    /**< Corner handling; defaults to MODIFIED. */
  int32_t                      with_contact;  /**< 0/1. Glue profile to spine. */
  int32_t with_correction; /**< 0/1. Rotate profile so its normal aligns with the spine tangent. */
  int32_t make_solid;      /**< 0/1. Close the resulting shell into a solid. */
} occtl_prim_pipe_shell_linear_law_info_t;

#define OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT                                                 \
  {OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_VERSION_1,                                                \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   1.0,                                                                                            \
   1.0,                                                                                            \
   OCCTL_PIPE_MODE_CORRECTED_FRENET,                                                               \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   {0.0, 0.0, 1.0},                                                                                \
   OCCTL_PIPE_TRANSITION_MODIFIED,                                                                 \
   0,                                                                                              \
   0,                                                                                              \
   0}

/**
 * Runtime initialiser for #occtl_prim_pipe_shell_linear_law_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_PIPE_SHELL_LINEAR_LAW_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_pipe_shell_linear_law
 */
OCCTL_API void OCCTL_CALL
  occtl_prim_pipe_shell_linear_law_info_init(occtl_prim_pipe_shell_linear_law_info_t* info);

/**
 * Builds a pipe-shell sweep with linearly scaled profile evolution.
 *
 * This exposes OCCT's law-driven pipe-shell mode for tapered, expanding,
 * or necked sweep forms without forcing callers to sample many intermediate
 * profiles manually.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version and NULL @c p_next.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 either scale is non-finite or non-positive,
 *                                 or constant-binormal mode is selected with
 *                                 a zero-length direction.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A node id refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c spine_wire is not a Wire, or @c profile
 *                                 is of a kind that cannot be swept.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed (incompatible profile,
 *                                 degenerate spine, or invalid law result).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_pipe_shell
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_pipe_shell_linear_law(occtl_graph_t*                                 graph,
                                        const occtl_prim_pipe_shell_linear_law_info_t* info,
                                        occtl_node_id_t*                               out_shape);

#define OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_pipe_shell_interpolated_law.
 *
 * Sweeps one profile along a spine wire while scaling the profile by an
 * interpolated law. @c parameters and @c scales are parallel arrays of
 * @c sample_count values; parameters are normalized over the spine domain
 * and must be strictly increasing from 0.0 to 1.0. Scales must be finite
 * and positive.
 *
 * The profile must be sweepable (Vertex / Edge / Wire / Face / Shell /
 * Compound) and live in the same graph as @c spine_wire.
 */
typedef struct occtl_prim_pipe_shell_interpolated_law_info
{
  uint32_t struct_version; /**< Must be #OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_VERSION_1. */
  const void*     p_next;  /**< Reserved; must be NULL. */
  occtl_node_id_t spine_wire; /**< Borrows it. Must be of kind #OCCTL_KIND_WIRE. */
  occtl_node_id_t profile;    /**< Borrows it. Profile NodeId to scale and sweep. */
  const double*
    parameters;         /**< Borrows it. Normalized law parameters; length is @c sample_count. */
  const double* scales; /**< Borrows it. Positive profile scales; length is @c sample_count. */
  size_t        sample_count; /**< Number of parameter / scale samples; must be at least 2. */
  occtl_prim_pipe_mode_t       mode;          /**< Trihedron rule; defaults to CORRECTED_FRENET. */
  occtl_axis2_placement_t      mode_axis;     /**< Used when @c mode == CONSTANT_AXIS. */
  occtl_direction3_t           mode_binormal; /**< Used when @c mode == CONSTANT_BINORMAL. */
  occtl_prim_pipe_transition_t transition;    /**< Corner handling; defaults to MODIFIED. */
  int32_t                      with_contact;  /**< 0/1. Glue profile to spine. */
  int32_t with_correction; /**< 0/1. Rotate profile so its normal aligns with the spine tangent. */
  int32_t make_solid;      /**< 0/1. Close the resulting shell into a solid. */
} occtl_prim_pipe_shell_interpolated_law_info_t;

#define OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_INIT                                           \
  {OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_VERSION_1,                                          \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   OCCTL_NODE_ID_INVALID,                                                                          \
   NULL,                                                                                           \
   NULL,                                                                                           \
   0,                                                                                              \
   OCCTL_PIPE_MODE_CORRECTED_FRENET,                                                               \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   {0.0, 0.0, 1.0},                                                                                \
   OCCTL_PIPE_TRANSITION_MODIFIED,                                                                 \
   0,                                                                                              \
   0,                                                                                              \
   0}

/**
 * Runtime initialiser for #occtl_prim_pipe_shell_interpolated_law_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_PIPE_SHELL_INTERPOLATED_LAW_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_pipe_shell_interpolated_law
 */
OCCTL_API void OCCTL_CALL occtl_prim_pipe_shell_interpolated_law_info_init(
  occtl_prim_pipe_shell_interpolated_law_info_t* info);

/**
 * Builds a pipe-shell sweep with interpolated profile-scale evolution.
 *
 * This exposes OCCT's law-driven pipe-shell mode for profiles whose scale
 * changes through multiple stations without requiring callers to build many
 * separate profile sections.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version and NULL @c p_next.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, @c p_next is non-NULL,
 *                                 fewer than two samples are supplied, samples
 *                                 are non-finite, parameters are not strictly
 *                                 increasing from 0.0 to 1.0, a scale is
 *                                 non-positive, or constant-binormal mode is
 *                                 selected with a zero-length direction.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A node id refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c spine_wire is not a Wire, or @c profile
 *                                 is of a kind that cannot be swept.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed (incompatible profile,
 *                                 degenerate spine, or invalid law result).
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_pipe_shell_linear_law
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_prim_make_pipe_shell_interpolated_law(
  occtl_graph_t*                                       graph,
  const occtl_prim_pipe_shell_interpolated_law_info_t* info,
  occtl_node_id_t*                                     out_shape);

/**
 * Offset construction mode.
 */
typedef enum occtl_prim_offset_mode
{
  OCCTL_OFFSET_MODE_SKIN            = 0, /**< Default. Single offset surface along all faces. */
  OCCTL_OFFSET_MODE_PIPE            = 1, /**< Pipe-style offset (reserved; advanced use). */
  OCCTL_OFFSET_MODE_RECTO_VERSO     = 2, /**< Offset on both sides (free-form surface only). */
  OCCTL_OFFSET_MODE_RESERVED_FUTURE = 0x7fffffff
} occtl_prim_offset_mode_t;

#define OCCTL_PRIM_OFFSET_SHAPE_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_offset_shape.
 *
 * Builds an offset version of @c shape (wire / face / shell / solid) at
 * the given @c offset distance using a join-based offset (each face is
 * offset and the gaps bridged according to @c join).
 */
typedef struct occtl_prim_offset_shape_info
{
  uint32_t                 struct_version; /**< Must be #OCCTL_PRIM_OFFSET_SHAPE_INFO_VERSION_1. */
  const void*              p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t          shape;          /**< Borrows it. Shape to offset. */
  double                   offset;         /**< Offset distance (signed). */
  double                   tolerance;      /**< Construction tolerance; typical @c 1.0e-3. */
  occtl_prim_offset_mode_t mode;           /**< Offset mode; default SKIN. */
  occtl_offset_join_type_t join;           /**< Edge join style; default #OCCTL_OFFSET_JOIN_ARC. */
  int32_t
    intersection; /**< 0/1. When 1, faces are intersected — slower but yields cleaner results. */
  int32_t self_intersection; /**< 0/1. When 1, allow self-intersection checking. */
  int32_t
    remove_internal_edges; /**< 0/1. When 1, internal edges of the offset result are removed. */
} occtl_prim_offset_shape_info_t;

#define OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT                                                          \
  {OCCTL_PRIM_OFFSET_SHAPE_INFO_VERSION_1,                                                         \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   0.0,                                                                                            \
   1.0e-3,                                                                                         \
   OCCTL_OFFSET_MODE_SKIN,                                                                         \
   OCCTL_OFFSET_JOIN_ARC,                                                                          \
   0,                                                                                              \
   0,                                                                                              \
   0}

/**
 * Runtime initialiser for #occtl_prim_offset_shape_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_OFFSET_SHAPE_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_offset_shape
 */
OCCTL_API void OCCTL_CALL occtl_prim_offset_shape_info_init(occtl_prim_offset_shape_info_t* info);

/**
 * Builds an offset shape.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_shape Borrows it. Must be non-NULL. On success
 *                          receives the new Shape NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         @c shape refers to a removed / absent node.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_thick_solid
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_offset_shape(occtl_graph_t*                        graph,
                               const occtl_prim_offset_shape_info_t* info,
                               occtl_node_id_t*                      out_shape);

#define OCCTL_PRIM_THICK_SOLID_INFO_VERSION_1 1u

/**
 * Info for #occtl_prim_make_thick_solid.
 *
 * Hollows @c solid by offsetting its boundary by @c offset (use a negative
 * value to hollow inward, leaving a thin wall). Faces in @c closing_faces
 * are removed from the offset boundary, producing open ends.
 */
typedef struct occtl_prim_thick_solid_info
{
  uint32_t        struct_version; /**< Must be #OCCTL_PRIM_THICK_SOLID_INFO_VERSION_1. */
  const void*     p_next;         /**< Reserved; must be NULL. */
  occtl_node_id_t solid;          /**< Borrows it. Solid to thicken/hollow. */
  const occtl_node_id_t*
         closing_faces;          /**< Borrows it. Optional faces of @c solid to drop (open ends). */
  size_t closing_face_count;     /**< Length of @c closing_faces. */
  double offset;                 /**< Wall thickness (signed). */
  double tolerance;              /**< Construction tolerance; typical @c 1.0e-3. */
  occtl_prim_offset_mode_t mode; /**< Offset mode; default SKIN. */
  occtl_offset_join_type_t join; /**< Edge join style; default #OCCTL_OFFSET_JOIN_ARC. */
  int32_t                  intersection;          /**< 0/1. */
  int32_t                  self_intersection;     /**< 0/1. */
  int32_t                  remove_internal_edges; /**< 0/1. */
} occtl_prim_thick_solid_info_t;

#define OCCTL_PRIM_THICK_SOLID_INFO_INIT                                                           \
  {OCCTL_PRIM_THICK_SOLID_INFO_VERSION_1,                                                          \
   NULL,                                                                                           \
   OCCTL_NODE_ID_INVALID,                                                                          \
   NULL,                                                                                           \
   0,                                                                                              \
   0.0,                                                                                            \
   1.0e-3,                                                                                         \
   OCCTL_OFFSET_MODE_SKIN,                                                                         \
   OCCTL_OFFSET_JOIN_ARC,                                                                          \
   0,                                                                                              \
   0,                                                                                              \
   0}

/**
 * Runtime initialiser for #occtl_prim_thick_solid_info_t.
 *
 * Sets all fields to #OCCTL_PRIM_THICK_SOLID_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_prim_make_thick_solid
 */
OCCTL_API void OCCTL_CALL occtl_prim_thick_solid_info_init(occtl_prim_thick_solid_info_t* info);

/**
 * Builds a thick-walled hollow Solid.
 *
 * @param[in,out] graph     Borrows it. Must be non-NULL.
 * @param[in]     info      Borrows it. Must be non-NULL with a recognised
 *                          @c struct_version.
 * @param[out]    out_solid Borrows it. Must be non-NULL. On success
 *                          receives the new Solid NodeId; on failure set
 *                          to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer is NULL, or @c closing_faces is NULL while count > 0.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_NOT_FOUND         A node id refers to a removed / absent node.
 * @retval OCCTL_WRONG_KIND        @c solid is not a Solid, or a closing face is not a Face.
 * @retval OCCTL_GEOMETRY_INVALID  Construction failed.
 * @retval OCCTL_TOPOLOGY_INVALID  The resulting topology was rejected.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No.
 *
 * @sa occtl_prim_make_offset_shape
 */
OCCTL_API occtl_status_t OCCTL_CALL
  occtl_prim_make_thick_solid(occtl_graph_t*                       graph,
                              const occtl_prim_thick_solid_info_t* info,
                              occtl_node_id_t*                     out_solid);

#ifdef __cplusplus
}
#endif

#endif /* OCCTL_PRIM_SWEEP_H */
