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
 * @brief C++ veneer for the text-to-shape module.
 *
 * Mirrors the C info struct as a PascalCase POD with defaults and returns the
 * produced compound NodeId. Failures translate to occtl::Error.
 */

#ifndef OCCTL_HPP_TEXT_HPP
#define OCCTL_HPP_TEXT_HPP

#include <occtl/occtl_text.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <string>

namespace occtl::text
{

namespace detail
{
/// @brief Default axis2 placement: origin + Z (main) + X.
inline ::occtl_axis2_placement_t default_ax2() noexcept
{
  return {{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}};
}
} // namespace detail

/// @brief Optional layout/font knobs for #make_faces and #make_wires.
struct TextFacesOptions
{
  ::occtl_axis2_placement_t placement = detail::default_ax2(); ///< Baseline frame; defaults to XOY.
  std::string               font_family; ///< Family name; ignored when @c font_path is set.
  std::string               font_path;   ///< Absolute path to a .ttf/.otf file.
  ::occtl_text_font_aspect_t font_aspect = OCCTL_TEXT_FONT_ASPECT_REGULAR;
  double                     height = 0.0; ///< Glyph cap height in model units; strictly positive.
  ::occtl_text_halign_t      horizontal_align = OCCTL_TEXT_HALIGN_LEFT;
  ::occtl_text_valign_t      vertical_align   = OCCTL_TEXT_VALIGN_BASELINE;
  double                     wrapping_width   = 0.0;  ///< Maximum line width; 0 disables wrapping.
  bool                       word_wrapping    = true; ///< Avoid breaking words when wrapping.
};

/// @brief Formatted text metrics returned by #measure.
using TextMetrics = ::occtl_text_metrics_t;

namespace detail
{
inline ::occtl_text_info_t to_c_info(const std::string&             theText,
                                     const TextFacesOptions&        theOpts,
                                     ::occtl_text_layout_options_t& theLayout)
{
  ::occtl_text_info_t anInfo = OCCTL_TEXT_INFO_INIT;
  anInfo.placement           = theOpts.placement;
  anInfo.utf8_text           = theText.c_str();
  anInfo.font_family         = theOpts.font_family.empty() ? nullptr : theOpts.font_family.c_str();
  anInfo.font_path           = theOpts.font_path.empty() ? nullptr : theOpts.font_path.c_str();
  anInfo.font_aspect         = theOpts.font_aspect;
  anInfo.height              = theOpts.height;
  anInfo.horizontal_align    = theOpts.horizontal_align;
  anInfo.vertical_align      = theOpts.vertical_align;
  if (theOpts.wrapping_width > 0.0 || !theOpts.word_wrapping)
  {
    theLayout                = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
    theLayout.wrapping_width = theOpts.wrapping_width;
    theLayout.word_wrapping  = theOpts.word_wrapping ? 1 : 0;
    anInfo.p_next            = &theLayout;
  }
  return anInfo;
}
} // namespace detail

/// @brief Measures formatted text without creating graph topology.
///        Throws #occtl::Error on failure.
/// @param[in] theText UTF-8 string; must be non-empty
/// @param[in] theOpts layout and font options; @c height must be > 0 and
///                    exactly one font selector must be set
inline TextMetrics measure(const std::string& theText, const TextFacesOptions& theOpts)
{
  ::occtl_text_layout_options_t aLayout  = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
  ::occtl_text_info_t           anInfo   = detail::to_c_info(theText, theOpts, aLayout);
  ::occtl_text_metrics_t        aMetrics = OCCTL_TEXT_METRICS_INIT;
  check(::occtl_text_measure(&anInfo, &aMetrics));
  return aMetrics;
}

/// @brief Builds a Compound of planar text faces and returns its NodeId.
///        Throws #occtl::Error on failure.
/// @param[in,out] theGraph graph receiving the text shape
/// @param[in]     theText  UTF-8 string; must be non-empty
/// @param[in]     theOpts  layout and font options; @c height must be > 0 and
///                         exactly one font selector must be set
inline NodeId make_faces(Graph&                  theGraph,
                         const std::string&      theText,
                         const TextFacesOptions& theOpts)
{
  ::occtl_text_layout_options_t aLayout = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
  ::occtl_text_info_t           anInfo  = detail::to_c_info(theText, theOpts, aLayout);

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_text_make_faces(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

/// @brief Builds a Compound of glyph outline wires and returns its NodeId.
///        Throws #occtl::Error on failure.
/// @param[in,out] theGraph graph receiving the text shape
/// @param[in]     theText  UTF-8 string; must be non-empty
/// @param[in]     theOpts  layout and font options; @c height must be > 0 and
///                         exactly one font selector must be set
inline NodeId make_wires(Graph&                  theGraph,
                         const std::string&      theText,
                         const TextFacesOptions& theOpts)
{
  ::occtl_text_layout_options_t aLayout = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
  ::occtl_text_info_t           anInfo  = detail::to_c_info(theText, theOpts, aLayout);

  ::occtl_node_id_t anId = OCCTL_NODE_ID_INVALID;
  check(::occtl_text_make_wires(theGraph.get(), &anInfo, &anId));
  return NodeId(anId);
}

} // namespace occtl::text

#endif // OCCTL_HPP_TEXT_HPP
