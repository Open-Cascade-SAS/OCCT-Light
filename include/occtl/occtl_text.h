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
 * @file occtl_text.h
 * @brief OCCT-Light: text-to-shape module public API.
 *
 * Measures formatted UTF-8 text, or converts it into a Compound of planar
 * faces (filled glyphs) / wires (glyph outlines) and inserts the topology into
 * a graph as a single root node.  The topology result is suitable for use as a
 * profile in subsequent operations: feed it to #occtl_prim_make_prism to
 * obtain an extruded solid, engraved label, or outline-only curve set.
 *
 * Fonts are resolved either by family name (the host's font registry is
 * scanned at first use) or by explicit absolute path to a .ttf/.otf
 * file. When a glyph cannot be rendered by the chosen font, a fallback
 * font is substituted silently; the operation still succeeds.
 */

#ifndef OCCTL_TEXT_H
#define OCCTL_TEXT_H

#include <stddef.h>
#include <stdint.h>

#include "occtl_core.h"
#include "occtl_geom.h"
#include "occtl_topo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Font style aspect.
 */
typedef enum occtl_text_font_aspect
{
  OCCTL_TEXT_FONT_ASPECT_REGULAR         = 0, /**< Default. */
  OCCTL_TEXT_FONT_ASPECT_BOLD            = 1,
  OCCTL_TEXT_FONT_ASPECT_ITALIC          = 2,
  OCCTL_TEXT_FONT_ASPECT_BOLD_ITALIC     = 3,
  OCCTL_TEXT_FONT_ASPECT_RESERVED_FUTURE = 0x7fffffff
} occtl_text_font_aspect_t;

/**
 * Horizontal alignment of multi-line text relative to @c placement.location.
 */
typedef enum occtl_text_halign
{
  OCCTL_TEXT_HALIGN_LEFT            = 0, /**< Default. Pen-x = placement.location.x. */
  OCCTL_TEXT_HALIGN_CENTER          = 1,
  OCCTL_TEXT_HALIGN_RIGHT           = 2,
  OCCTL_TEXT_HALIGN_RESERVED_FUTURE = 0x7fffffff
} occtl_text_halign_t;

/**
 * Vertical alignment of the text block relative to @c placement.location.
 */
typedef enum occtl_text_valign
{
  OCCTL_TEXT_VALIGN_BOTTOM          = 0,
  OCCTL_TEXT_VALIGN_BASELINE        = 1, /**< Default. Baseline of the first line. */
  OCCTL_TEXT_VALIGN_CENTER          = 2,
  OCCTL_TEXT_VALIGN_TOP             = 3,
  OCCTL_TEXT_VALIGN_RESERVED_FUTURE = 0x7fffffff
} occtl_text_valign_t;

#define OCCTL_TEXT_INFO_VERSION_1 1u
#define OCCTL_TEXT_LAYOUT_OPTIONS_VERSION_1 1u
#define OCCTL_TEXT_METRICS_VERSION_1 1u

/**
 * Info for #occtl_text_measure, #occtl_text_make_faces, and
 * #occtl_text_make_wires.
 *
 * Lays out @c utf8_text on the XY plane of @c placement (Z up, X along
 * the writing direction) at the given glyph @c height. Lines are
 * separated by newline characters and aligned according to @c horizontal_align /
 * @c vertical_align. Exactly one of @c font_family / @c font_path must be
 * non-NULL; @c font_path wins when both are set.
 */
typedef struct occtl_text_info
{
  uint32_t                struct_version; /**< Must be #OCCTL_TEXT_INFO_VERSION_1. */
  const void*             p_next;         /**< Reserved for extensions; must be NULL. */
  occtl_axis2_placement_t placement;      /**< Baseline frame; defaults to XOY. */
  const char*             utf8_text;      /**< Borrows it. Required, non-empty UTF-8 string. */
  const char*
    font_family; /**< Borrows it. Family name (e.g. "Arial"). NULL when @c font_path is used. */
  const char*
    font_path; /**< Borrows it. Absolute path to a .ttf/.otf file. NULL to look up by family. */
  occtl_text_font_aspect_t font_aspect; /**< Style aspect; defaults to REGULAR. */
  double                   height;      /**< Glyph cap height in model units; strictly positive. */
  occtl_text_halign_t      horizontal_align; /**< Horizontal alignment; defaults to LEFT. */
  occtl_text_valign_t      vertical_align;   /**< Vertical alignment; defaults to BASELINE. */
} occtl_text_info_t;

/**
 * Optional layout extension for #occtl_text_info_t::p_next.
 *
 * Enables OCCT text wrapping while preserving the v1 text-info ABI. When
 * present, pass a pointer to this struct in @c occtl_text_info_t::p_next.
 */
typedef struct occtl_text_layout_options
{
  uint32_t    struct_version; /**< Must be #OCCTL_TEXT_LAYOUT_OPTIONS_VERSION_1. */
  const void* p_next;         /**< Reserved for future extensions; must be NULL. */
  double wrapping_width; /**< Maximum formatted text width in model units; 0.0 disables wrapping. */
  int32_t word_wrapping; /**< 0/1; non-zero avoids breaking words when wrapping. Default 1. */
} occtl_text_layout_options_t;

/**
 * Formatted text metrics returned by #occtl_text_measure.
 *
 * Initialise with #OCCTL_TEXT_METRICS_INIT or
 * #occtl_text_metrics_init before passing to #occtl_text_measure. The
 * caller-owned version field lets future OCCT-Light releases append fields
 * without changing the ABI contract.
 */
typedef struct occtl_text_metrics
{
  uint32_t    struct_version;   /**< Must be #OCCTL_TEXT_METRICS_VERSION_1. */
  const void* p_next;           /**< Reserved for extensions; must be NULL. */
  double      width;            /**< Formatted text width in model units. */
  double      height;           /**< Formatted text height in model units. */
  double      left;             /**< Formatted local bounding box left coordinate. */
  double      right;            /**< Formatted local bounding box right coordinate. */
  double      bottom;           /**< Formatted local bounding box bottom coordinate. */
  double      top;              /**< Formatted local bounding box top coordinate. */
  double      ascender;         /**< Font ascender in model units. */
  double      descender;        /**< Font descender in model units. */
  double      line_spacing;     /**< Baseline-to-baseline distance in model units. */
  double      max_symbol_width; /**< Maximum formatted symbol width in model units. */
} occtl_text_metrics_t;

#define OCCTL_TEXT_INFO_INIT                                                                       \
  {OCCTL_TEXT_INFO_VERSION_1,                                                                      \
   NULL,                                                                                           \
   {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}},                                            \
   NULL,                                                                                           \
   NULL,                                                                                           \
   NULL,                                                                                           \
   OCCTL_TEXT_FONT_ASPECT_REGULAR,                                                                 \
   0.0,                                                                                            \
   OCCTL_TEXT_HALIGN_LEFT,                                                                         \
   OCCTL_TEXT_VALIGN_BASELINE}

#define OCCTL_TEXT_LAYOUT_OPTIONS_INIT {OCCTL_TEXT_LAYOUT_OPTIONS_VERSION_1, NULL, 0.0, 1}

#define OCCTL_TEXT_METRICS_INIT                                                                    \
  {OCCTL_TEXT_METRICS_VERSION_1, NULL, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}

/**
 * Runtime initialiser for #occtl_text_info_t.
 *
 * Sets all fields to #OCCTL_TEXT_INFO_INIT.
 *
 * @param[out] info  Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_text_make_faces
 * @sa occtl_text_make_wires
 * @sa occtl_text_measure
 */
OCCTL_API void OCCTL_CALL occtl_text_info_init(occtl_text_info_t* info);

/**
 * Runtime initialiser for #occtl_text_layout_options_t.
 *
 * Sets all fields to #OCCTL_TEXT_LAYOUT_OPTIONS_INIT.
 *
 * @param[out] options Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_text_measure
 * @sa occtl_text_make_faces
 * @sa occtl_text_make_wires
 */
OCCTL_API void OCCTL_CALL occtl_text_layout_options_init(occtl_text_layout_options_t* options);

/**
 * Runtime initialiser for #occtl_text_metrics_t.
 *
 * Sets all fields to #OCCTL_TEXT_METRICS_INIT.
 *
 * @param[out] metrics Borrows it. NULL-tolerant; no-op when NULL.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_text_measure
 */
OCCTL_API void OCCTL_CALL occtl_text_metrics_init(occtl_text_metrics_t* metrics);

/**
 * Measures formatted text without inserting topology into a graph.
 *
 * Layout, font selection, fallback behaviour, and alignment match
 * #occtl_text_make_faces. Metrics are reported in the local XY text
 * coordinate system before @c info->placement is applied to generated
 * topology. Use this to size labels, drawing annotations, engraving
 * pockets, or downstream layout before creating faces or wires.
 *
 * @param[in]  info        Borrows it. Must be non-NULL with a recognised
 *                         @c struct_version, non-empty @c utf8_text,
 *                         strictly positive @c height, and at least one
 *                         of @c font_family / @c font_path set.
 * @param[out] out_metrics Borrows it. Must be non-NULL, initialised with
 *                         a recognised @c struct_version, and receives the
 *                         measured values on success.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  @p info or @p out_metrics is NULL,
 *                                 @c utf8_text is empty, @c height is
 *                                 non-positive, both font selectors are
 *                                 NULL, @c out_metrics->p_next is non-NULL,
 *                                 layout options are invalid, or an alignment
 *                                 enum is out of range.
 * @retval OCCTL_VERSION_MISMATCH  An input or output @c struct_version is
 *                                 unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Font could not be loaded.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe Yes.
 *
 * @sa occtl_text_make_faces
 * @sa occtl_text_make_wires
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_text_measure(const occtl_text_info_t* info,
                                                       occtl_text_metrics_t*    out_metrics);

/**
 * Produces a Compound of planar faces from a string and inserts it
 * into @p graph as a single root node.
 *
 * Multi-line input (newline-separated) is laid out with the alignment
 * specified in @p info. When @c font_family does not match any installed
 * font, or when individual glyphs are unsupported by the chosen face,
 * the host's default fallback font is substituted without failing the
 * call (a warning may be emitted to stderr by the font manager). Use
 * @c font_path when you need a hard guarantee that a specific file is
 * used; a missing file is reported as @c OCCTL_GEOMETRY_INVALID.
 *
 * Combine the result with #occtl_prim_make_prism to extrude the text
 * into a solid.
 *
 * @param[in,out] graph        Borrows it. Must be non-NULL.
 * @param[in]     info         Borrows it. Must be non-NULL with a recognised
 *                             @c struct_version, non-empty @c utf8_text,
 *                             strictly positive @c height, and at least one
 *                             of @c font_family / @c font_path set.
 * @param[out]    out_compound Borrows it (caller-allocated slot). Must be
 *                             non-NULL. On success receives the new Compound
 *                             NodeId; on failure set to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL, @c utf8_text
 *                                 is empty, @c height is non-positive,
 *                                 both font selectors are NULL, layout options
 *                                 are invalid, or an alignment enum is out of
 *                                 range.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Font could not be loaded, or the layout
 *                                 produced no usable geometry.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No. Mutates @p graph.
 *
 * @sa occtl_text_make_wires
 * @sa occtl_prim_make_prism
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_text_make_faces(occtl_graph_t*           graph,
                                                          const occtl_text_info_t* info,
                                                          occtl_node_id_t*         out_compound);

/**
 * Produces a Compound of glyph outline wires from a string and inserts it
 * into @p graph as a single root node.
 *
 * Layout, font selection, fallback behaviour, and placement match
 * #occtl_text_make_faces. The returned Compound contains the generated glyph
 * wires without filling them into Faces, which is useful for laser/CNC paths,
 * engraving curves, and custom downstream profile processing.
 *
 * @param[in,out] graph        Borrows it. Must be non-NULL.
 * @param[in]     info         Borrows it. Must be non-NULL with a recognised
 *                             @c struct_version, non-empty @c utf8_text,
 *                             strictly positive @c height, and at least one
 *                             of @c font_family / @c font_path set.
 * @param[out]    out_compound Borrows it (caller-allocated slot). Must be
 *                             non-NULL. On success receives the new Compound
 *                             NodeId; on failure set to #OCCTL_NODE_ID_INVALID.
 *
 * @retval OCCTL_OK                On success.
 * @retval OCCTL_INVALID_ARGUMENT  Any pointer argument is NULL, @c utf8_text
 *                                 is empty, @c height is non-positive,
 *                                 both font selectors are NULL, layout options
 *                                 are invalid, or an alignment enum is out of
 *                                 range.
 * @retval OCCTL_VERSION_MISMATCH  @c struct_version is unrecognised.
 * @retval OCCTL_GEOMETRY_INVALID  Font could not be loaded, or the layout
 *                                 produced no usable outline wires.
 * @retval OCCTL_INTERNAL          An unexpected internal error occurred.
 *
 * @threadsafe No. Mutates @p graph.
 *
 * @sa occtl_text_make_faces
 * @sa occtl_prim_make_prism
 */
OCCTL_API occtl_status_t OCCTL_CALL occtl_text_make_wires(occtl_graph_t*           graph,
                                                          const occtl_text_info_t* info,
                                                          occtl_node_id_t*         out_compound);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCCTL_TEXT_H */
