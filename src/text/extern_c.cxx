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

#include "../core/ErrorState.hxx"
#include "../core/Guard.hxx"
#include "../geom/CurveMath.hxx"
#include "../prim/PrimMath.hxx"

#include <occtl/occtl_text.h>

#include <BRep_Builder.hxx>
#include <Graphic3d_HorizontalTextAlignment.hxx>
#include <Graphic3d_VerticalTextAlignment.hxx>
#include <NCollection_String.hxx>
#include <Standard_ErrorHandler.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <Font_FontAspect.hxx>
#include <Font_Rect.hxx>
#include <Font_StrictLevel.hxx>
#include <Font_TextFormatter.hxx>
#include <Precision.hxx>
#include <StdPrs_BRepFont.hxx>
#include <StdPrs_BRepTextBuilder.hxx>

#include <cmath>

namespace
{

bool IsFiniteValue(const double theValue) noexcept
{
  return !Precision::IsInfinite(theValue) && !std::isnan(theValue);
}

bool IsZeroOrOne(const int32_t theValue) noexcept
{
  return theValue == 0 || theValue == 1;
}

} // namespace

extern "C"
{

OCCTL_API void OCCTL_CALL occtl_text_info_init(occtl_text_info_t* const theInfo)
{
  if (theInfo != nullptr)
  {
    *theInfo = OCCTL_TEXT_INFO_INIT;
  }
}

OCCTL_API void OCCTL_CALL
  occtl_text_layout_options_init(occtl_text_layout_options_t* const theOptions)
{
  if (theOptions != nullptr)
  {
    *theOptions = OCCTL_TEXT_LAYOUT_OPTIONS_INIT;
  }
}

OCCTL_API void OCCTL_CALL occtl_text_metrics_init(occtl_text_metrics_t* const theMetrics)
{
  if (theMetrics != nullptr)
  {
    *theMetrics = OCCTL_TEXT_METRICS_INIT;
  }
}

static bool ToFontAspect(const occtl_text_font_aspect_t theAspect,
                         Font_FontAspect&               theOutAspect) noexcept
{
  switch (theAspect)
  {
    case OCCTL_TEXT_FONT_ASPECT_REGULAR:
      theOutAspect = Font_FontAspect_Regular;
      return true;
    case OCCTL_TEXT_FONT_ASPECT_BOLD:
      theOutAspect = Font_FontAspect_Bold;
      return true;
    case OCCTL_TEXT_FONT_ASPECT_ITALIC:
      theOutAspect = Font_FontAspect_Italic;
      return true;
    case OCCTL_TEXT_FONT_ASPECT_BOLD_ITALIC:
      theOutAspect = Font_FontAspect_BoldItalic;
      return true;
    default:
      return false;
  }
}

static occtl_status_t ApplyLayoutOptions(
  const occtl_text_info_t* const         theInfo,
  const occ::handle<Font_TextFormatter>& theFormatter) noexcept
{
  if (theInfo->p_next == nullptr)
  {
    return OCCTL_OK;
  }

  const occtl_text_layout_options_t* const aLayout =
    static_cast<const occtl_text_layout_options_t*>(theInfo->p_next);
  if (aLayout->struct_version != OCCTL_TEXT_LAYOUT_OPTIONS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "occtl_text_layout_options_t: unsupported struct_version");
    return OCCTL_VERSION_MISMATCH;
  }
  if (aLayout->p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "occtl_text_layout_options_t::p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsFiniteValue(aLayout->wrapping_width) || aLayout->wrapping_width < 0.0)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "wrapping_width must be finite and non-negative");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!IsZeroOrOne(aLayout->word_wrapping))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "word_wrapping must be 0 or 1");
    return OCCTL_INVALID_ARGUMENT;
  }

  theFormatter->SetWrapping(static_cast<float>(aLayout->wrapping_width));
  theFormatter->SetWordWrapping(aLayout->word_wrapping != 0);
  return OCCTL_OK;
}

static bool ToHAlign(const occtl_text_halign_t          theAlign,
                     Graphic3d_HorizontalTextAlignment& theOutAlign) noexcept
{
  switch (theAlign)
  {
    case OCCTL_TEXT_HALIGN_LEFT:
      theOutAlign = Graphic3d_HTA_LEFT;
      return true;
    case OCCTL_TEXT_HALIGN_CENTER:
      theOutAlign = Graphic3d_HTA_CENTER;
      return true;
    case OCCTL_TEXT_HALIGN_RIGHT:
      theOutAlign = Graphic3d_HTA_RIGHT;
      return true;
    default:
      return false;
  }
}

static bool ToVAlign(const occtl_text_valign_t        theAlign,
                     Graphic3d_VerticalTextAlignment& theOutAlign) noexcept
{
  switch (theAlign)
  {
    case OCCTL_TEXT_VALIGN_BOTTOM:
      theOutAlign = Graphic3d_VTA_BOTTOM;
      return true;
    case OCCTL_TEXT_VALIGN_BASELINE:
      theOutAlign = Graphic3d_VTA_TOPFIRSTLINE;
      return true;
    case OCCTL_TEXT_VALIGN_CENTER:
      theOutAlign = Graphic3d_VTA_CENTER;
      return true;
    case OCCTL_TEXT_VALIGN_TOP:
      theOutAlign = Graphic3d_VTA_TOP;
      return true;
    default:
      return false;
  }
}

static occtl_status_t BuildTextShape(const occtl_text_info_t* const theInfo,
                                     TopoDS_Shape&                  theShape) noexcept
{
  if (theInfo->struct_version != OCCTL_TEXT_INFO_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                           "info->struct_version is not OCCTL_TEXT_INFO_VERSION_1");
    return OCCTL_VERSION_MISMATCH;
  }

  if (theInfo->utf8_text == nullptr || theInfo->utf8_text[0] == '\0')
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->utf8_text must be a non-empty UTF-8 string");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!(theInfo->height > 0.0))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->height must be strictly positive");
    return OCCTL_INVALID_ARGUMENT;
  }
  const bool hasFamily = theInfo->font_family != nullptr;
  const bool hasPath   = theInfo->font_path != nullptr;
  if (hasFamily == hasPath)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "exactly one of info->font_family / info->font_path must be set");
    return OCCTL_INVALID_ARGUMENT;
  }

  Font_FontAspect anAspect = Font_FontAspect_Regular;
  if (!ToFontAspect(theInfo->font_aspect, anAspect))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->font_aspect is out of range");
    return OCCTL_INVALID_ARGUMENT;
  }
  Graphic3d_HorizontalTextAlignment anHAlign = Graphic3d_HTA_LEFT;
  if (!ToHAlign(theInfo->horizontal_align, anHAlign))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->horizontal_align is out of range");
    return OCCTL_INVALID_ARGUMENT;
  }
  Graphic3d_VerticalTextAlignment aVAlign = Graphic3d_VTA_TOPFIRSTLINE;
  if (!ToVAlign(theInfo->vertical_align, aVAlign))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->vertical_align is out of range");
    return OCCTL_INVALID_ARGUMENT;
  }

  StdPrs_BRepFont aFont;
  bool            aFontOk = false;
  if (theInfo->font_path != nullptr)
  {
    const NCollection_String aPath(theInfo->font_path);
    aFontOk = aFont.Init(aPath, theInfo->height, 0);
    if (!aFontOk)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        static_cast<std::string_view>(TCollection_AsciiString("font file could not be loaded: ")
                                      + theInfo->font_path));
      return OCCTL_GEOMETRY_INVALID;
    }
  }
  else
  {
    const TCollection_AsciiString aName(theInfo->font_family);
    aFontOk = aFont.FindAndInit(aName, anAspect, theInfo->height, Font_StrictLevel_Any);
    if (!aFontOk)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        static_cast<std::string_view>(TCollection_AsciiString("font family could not be resolved: ")
                                      + theInfo->font_family));
      return OCCTL_GEOMETRY_INVALID;
    }
  }

  const gp_Ax2 anAxes = OcctL::Geom::ToGpAx2(theInfo->placement);
  const gp_Ax3 aPenLoc(anAxes);

  const occ::handle<Font_TextFormatter> aFormatter = new Font_TextFormatter();
  aFormatter->SetupAlignment(anHAlign, aVAlign);
  if (const occtl_status_t aStatus = ApplyLayoutOptions(theInfo, aFormatter))
  {
    return aStatus;
  }
  const NCollection_String aText(theInfo->utf8_text);
  aFormatter->Append(aText, *aFont.FTFont());
  aFormatter->Format();

  StdPrs_BRepTextBuilder aBuilder;
  theShape = aBuilder.Perform(aFont, aFormatter, aPenLoc);
  if (theShape.IsNull())
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "text layout produced no geometry");
    return OCCTL_GEOMETRY_INVALID;
  }

  return OCCTL_OK;
}

static occtl_status_t ExtractTextWires(const TopoDS_Shape& theTextShape,
                                       TopoDS_Shape&       theWireShape) noexcept
{
  BRep_Builder    aBuilder;
  TopoDS_Compound aCompound;
  aBuilder.MakeCompound(aCompound);

  bool aHasWire = false;
  for (TopExp_Explorer anExp(theTextShape, TopAbs_WIRE); anExp.More(); anExp.Next())
  {
    aBuilder.Add(aCompound, anExp.Current());
    aHasWire = true;
  }

  if (!aHasWire)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_GEOMETRY_INVALID,
                                           "text layout produced no usable outline wires");
    return OCCTL_GEOMETRY_INVALID;
  }

  theWireShape = aCompound;
  return OCCTL_OK;
}

static occtl_status_t MeasureText(const occtl_text_info_t* const theInfo,
                                  occtl_text_metrics_t&          theMetrics) noexcept
{
  if (theMetrics.struct_version != OCCTL_TEXT_METRICS_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_VERSION_MISMATCH,
      "out_metrics->struct_version is not OCCTL_TEXT_METRICS_VERSION_1");
    return OCCTL_VERSION_MISMATCH;
  }
  if (theMetrics.p_next != nullptr)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "out_metrics->p_next must be NULL");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (theInfo->struct_version != OCCTL_TEXT_INFO_VERSION_1)
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_VERSION_MISMATCH,
                                           "info->struct_version is not OCCTL_TEXT_INFO_VERSION_1");
    return OCCTL_VERSION_MISMATCH;
  }

  if (theInfo->utf8_text == nullptr || theInfo->utf8_text[0] == '\0')
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->utf8_text must be a non-empty UTF-8 string");
    return OCCTL_INVALID_ARGUMENT;
  }
  if (!(theInfo->height > 0.0))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->height must be strictly positive");
    return OCCTL_INVALID_ARGUMENT;
  }
  const bool hasFamily = theInfo->font_family != nullptr;
  const bool hasPath   = theInfo->font_path != nullptr;
  if (hasFamily == hasPath)
  {
    OcctL::Core::ErrorState::Current().Set(
      OCCTL_INVALID_ARGUMENT,
      "exactly one of info->font_family / info->font_path must be set");
    return OCCTL_INVALID_ARGUMENT;
  }

  Font_FontAspect anAspect = Font_FontAspect_Regular;
  if (!ToFontAspect(theInfo->font_aspect, anAspect))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->font_aspect is out of range");
    return OCCTL_INVALID_ARGUMENT;
  }
  Graphic3d_HorizontalTextAlignment anHAlign = Graphic3d_HTA_LEFT;
  if (!ToHAlign(theInfo->horizontal_align, anHAlign))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->horizontal_align is out of range");
    return OCCTL_INVALID_ARGUMENT;
  }
  Graphic3d_VerticalTextAlignment aVAlign = Graphic3d_VTA_TOPFIRSTLINE;
  if (!ToVAlign(theInfo->vertical_align, aVAlign))
  {
    OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                           "info->vertical_align is out of range");
    return OCCTL_INVALID_ARGUMENT;
  }

  StdPrs_BRepFont aFont;
  bool            aFontOk = false;
  if (theInfo->font_path != nullptr)
  {
    const NCollection_String aPath(theInfo->font_path);
    aFontOk = aFont.Init(aPath, theInfo->height, 0);
    if (!aFontOk)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        static_cast<std::string_view>(TCollection_AsciiString("font file could not be loaded: ")
                                      + theInfo->font_path));
      return OCCTL_GEOMETRY_INVALID;
    }
  }
  else
  {
    const TCollection_AsciiString aName(theInfo->font_family);
    aFontOk = aFont.FindAndInit(aName, anAspect, theInfo->height, Font_StrictLevel_Any);
    if (!aFontOk)
    {
      OcctL::Core::ErrorState::Current().Set(
        OCCTL_GEOMETRY_INVALID,
        static_cast<std::string_view>(TCollection_AsciiString("font family could not be resolved: ")
                                      + theInfo->font_family));
      return OCCTL_GEOMETRY_INVALID;
    }
  }

  const occ::handle<Font_TextFormatter> aFormatter = new Font_TextFormatter();
  aFormatter->SetupAlignment(anHAlign, aVAlign);
  if (const occtl_status_t aStatus = ApplyLayoutOptions(theInfo, aFormatter))
  {
    return aStatus;
  }
  const NCollection_String aText(theInfo->utf8_text);
  aFormatter->Append(aText, *aFont.FTFont());
  aFormatter->Format();

  Font_Rect aBounds;
  aFormatter->BndBox(aBounds);
  theMetrics.width            = static_cast<double>(aFormatter->ResultWidth());
  theMetrics.height           = static_cast<double>(aFormatter->ResultHeight());
  theMetrics.left             = static_cast<double>(aBounds.Left);
  theMetrics.right            = static_cast<double>(aBounds.Right);
  theMetrics.bottom           = static_cast<double>(aBounds.Bottom);
  theMetrics.top              = static_cast<double>(aBounds.Top);
  theMetrics.ascender         = aFont.Ascender();
  theMetrics.descender        = aFont.Descender();
  theMetrics.line_spacing     = aFont.LineSpacing();
  theMetrics.max_symbol_width = static_cast<double>(aFormatter->MaximumSymbolWidth());
  return OCCTL_OK;
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_text_make_faces(occtl_graph_t* const           theGraph,
                                                          const occtl_text_info_t* const theInfo,
                                                          occtl_node_id_t* const theOutCompound)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theGraph == nullptr || theInfo == nullptr || theOutCompound == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_compound is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCompound = OCCTL_NODE_ID_INVALID;

    TopoDS_Shape         aShape;
    const occtl_status_t aStatus = BuildTextShape(theInfo, aShape);
    if (aStatus != OCCTL_OK)
    {
      return aStatus;
    }

    return OcctL::Prim::AddTopologyRoot(theGraph, aShape, *theOutCompound);
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_text_measure(const occtl_text_info_t* const theInfo,
                                                       occtl_text_metrics_t* const    theOutMetrics)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theInfo == nullptr || theOutMetrics == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT, "info or out_metrics is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }

    return MeasureText(theInfo, *theOutMetrics);
  });
}

OCCTL_API occtl_status_t OCCTL_CALL occtl_text_make_wires(occtl_graph_t* const           theGraph,
                                                          const occtl_text_info_t* const theInfo,
                                                          occtl_node_id_t* const theOutCompound)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    OCC_CATCH_SIGNALS
    if (theGraph == nullptr || theInfo == nullptr || theOutCompound == nullptr)
    {
      OcctL::Core::ErrorState::Current().Set(OCCTL_INVALID_ARGUMENT,
                                             "graph, info, or out_compound is NULL");
      return OCCTL_INVALID_ARGUMENT;
    }
    *theOutCompound = OCCTL_NODE_ID_INVALID;

    TopoDS_Shape   aTextShape;
    occtl_status_t aStatus = BuildTextShape(theInfo, aTextShape);
    if (aStatus != OCCTL_OK)
    {
      return aStatus;
    }

    TopoDS_Shape aWireShape;
    aStatus = ExtractTextWires(aTextShape, aWireShape);
    if (aStatus != OCCTL_OK)
    {
      return aStatus;
    }

    return OcctL::Prim::AddTopologyRoot(theGraph, aWireShape, *theOutCompound);
  });
}

} // extern "C"
