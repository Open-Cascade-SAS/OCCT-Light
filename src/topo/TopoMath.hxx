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

//! @file TopoMath.hxx
//! @brief Internal conversion helpers between OCCT BRepGraph ID types and the
//!        ABI 64-bit packed types.
//!
//! This is the single file that knows the internal bit layout; nowhere else
//! interprets occtl_*_t bits directly.
//!
//! Bit layout for all four ID types (node, ref, rep, uid):
//!   bits[63:56] : kind tag (occtl_*_kind_t cast to uint8_t)
//!   bits[55:48] : reserved (must be 0)
//!   bits[47:0]  : payload (index for node/ref/rep; counter for uid)
//!
//! On Unpack* the reserved byte is checked; non-zero values are treated as
//! cross-type confusion and produce the OCCT invalid sentinel.

#ifndef OCCTL_TOPO_TOPO_MATH_HXX
#define OCCTL_TOPO_TOPO_MATH_HXX

#include <BRepGraph_NodeId.hxx>
#include <BRepGraph_RefId.hxx>
#include <BRepGraph_RefUID.hxx>
#include <BRepGraph_RepId.hxx>
#include <BRepGraph_RepUID.hxx>
#include <BRepGraph_UID.hxx>

#include <occtl/occtl_core.h>
#include <occtl/occtl_topo.h>

#include <array>
#include <cstdint>

namespace OcctL::Topo
{

static constexpr uint64_t THE_KIND_SHIFT     = 56; //!< Shift to reach the 8-bit kind tag.
static constexpr uint64_t THE_RESERVED_SHIFT = 48; //!< Shift to reach the reserved byte.
static constexpr uint64_t THE_RESERVED_MASK =
  0x00ff000000000000ull; //!< Mask covering the reserved byte.
static constexpr uint64_t THE_PAYLOAD_MASK =
  0x0000ffffffffffffull; //!< Mask covering the 48-bit payload.

//! @brief Converts an OCCT node kind to its ABI representation.
//! @param[in] theKind OCCT BRepGraph node kind.
//! @return ABI node kind, or OCCTL_KIND_INVALID for out-of-range values.
inline occtl_node_kind_t ToAbiNodeKind(const BRepGraph_NodeId::Kind theKind)
{
  static constexpr std::array<occtl_node_kind_t, 12> THE_MAP = {
    OCCTL_KIND_SOLID,     // Solid     = 0
    OCCTL_KIND_SHELL,     // Shell     = 1
    OCCTL_KIND_FACE,      // Face      = 2
    OCCTL_KIND_WIRE,      // Wire      = 3
    OCCTL_KIND_EDGE,      // Edge      = 4
    OCCTL_KIND_VERTEX,    // Vertex    = 5
    OCCTL_KIND_COMPOUND,  // Compound  = 6
    OCCTL_KIND_COMPSOLID, // CompSolid = 7
    OCCTL_KIND_COEDGE,    // CoEdge    = 8
    OCCTL_KIND_INVALID,   // (gap)     = 9
    OCCTL_KIND_PRODUCT,   // Product   = 10
    OCCTL_KIND_OCCURRENCE // Occurrence = 11
  };
  const int aVal = static_cast<int>(theKind);
  return (aVal >= 0 && aVal < static_cast<int>(THE_MAP.size())) ? THE_MAP[static_cast<size_t>(aVal)]
                                                                : OCCTL_KIND_INVALID;
}

//! @brief Reverse node-kind map.  Returns true on success.
//! @param[in]  theKind ABI node kind to resolve.
//! @param[out] theOut  Receives the OCCT kind on success; untouched on failure.
//! @return true if the kind was valid and non-INVALID, false otherwise.
inline bool TryToOcctNodeKind(const occtl_node_kind_t theKind, BRepGraph_NodeId::Kind& theOut)
{
  switch (theKind)
  {
    case OCCTL_KIND_SOLID:
      theOut = BRepGraph_NodeId::Kind::Solid;
      return true;
    case OCCTL_KIND_SHELL:
      theOut = BRepGraph_NodeId::Kind::Shell;
      return true;
    case OCCTL_KIND_FACE:
      theOut = BRepGraph_NodeId::Kind::Face;
      return true;
    case OCCTL_KIND_WIRE:
      theOut = BRepGraph_NodeId::Kind::Wire;
      return true;
    case OCCTL_KIND_EDGE:
      theOut = BRepGraph_NodeId::Kind::Edge;
      return true;
    case OCCTL_KIND_VERTEX:
      theOut = BRepGraph_NodeId::Kind::Vertex;
      return true;
    case OCCTL_KIND_COMPOUND:
      theOut = BRepGraph_NodeId::Kind::Compound;
      return true;
    case OCCTL_KIND_COMPSOLID:
      theOut = BRepGraph_NodeId::Kind::CompSolid;
      return true;
    case OCCTL_KIND_COEDGE:
      theOut = BRepGraph_NodeId::Kind::CoEdge;
      return true;
    case OCCTL_KIND_PRODUCT:
      theOut = BRepGraph_NodeId::Kind::Product;
      return true;
    case OCCTL_KIND_OCCURRENCE:
      theOut = BRepGraph_NodeId::Kind::Occurrence;
      return true;
    default:
      return false;
  }
}

//! @brief Converts an OCCT ref kind to its ABI representation.
//! @param[in] theKind OCCT BRepGraph ref kind.
//! @return ABI ref kind, or OCCTL_REF_KIND_INVALID for out-of-range values.
inline occtl_ref_kind_t ToAbiRefKind(const BRepGraph_RefId::Kind theKind)
{
  static constexpr std::array<occtl_ref_kind_t, 8> THE_MAP = {
    OCCTL_REF_KIND_SHELL,     // Shell      = 0
    OCCTL_REF_KIND_FACE,      // Face       = 1
    OCCTL_REF_KIND_WIRE,      // Wire       = 2
    OCCTL_REF_KIND_COEDGE,    // CoEdge     = 3
    OCCTL_REF_KIND_VERTEX,    // Vertex     = 4
    OCCTL_REF_KIND_SOLID,     // Solid      = 5
    OCCTL_REF_KIND_CHILD,     // Child      = 6
    OCCTL_REF_KIND_OCCURRENCE // Occurrence = 7
  };
  const int aVal = static_cast<int>(theKind);
  return (aVal >= 0 && aVal < static_cast<int>(THE_MAP.size())) ? THE_MAP[static_cast<size_t>(aVal)]
                                                                : OCCTL_REF_KIND_INVALID;
}

//! @brief Reverse ref-kind map.  Returns true on success.
//! @param[in]  theKind ABI ref kind to resolve.
//! @param[out] theOut  Receives the OCCT kind on success; untouched on failure.
//! @return true if the kind was valid and non-INVALID, false otherwise.
inline bool TryToOcctRefKind(const occtl_ref_kind_t theKind, BRepGraph_RefId::Kind& theOut)
{
  switch (theKind)
  {
    case OCCTL_REF_KIND_SHELL:
      theOut = BRepGraph_RefId::Kind::Shell;
      return true;
    case OCCTL_REF_KIND_FACE:
      theOut = BRepGraph_RefId::Kind::Face;
      return true;
    case OCCTL_REF_KIND_WIRE:
      theOut = BRepGraph_RefId::Kind::Wire;
      return true;
    case OCCTL_REF_KIND_COEDGE:
      theOut = BRepGraph_RefId::Kind::CoEdge;
      return true;
    case OCCTL_REF_KIND_VERTEX:
      theOut = BRepGraph_RefId::Kind::Vertex;
      return true;
    case OCCTL_REF_KIND_SOLID:
      theOut = BRepGraph_RefId::Kind::Solid;
      return true;
    case OCCTL_REF_KIND_CHILD:
      theOut = BRepGraph_RefId::Kind::Child;
      return true;
    case OCCTL_REF_KIND_OCCURRENCE:
      theOut = BRepGraph_RefId::Kind::Occurrence;
      return true;
    default:
      return false;
  }
}

//! @brief Converts an OCCT rep kind to its ABI representation.
//! @param[in] theKind OCCT BRepGraph rep kind.
//! @return ABI rep kind, or OCCTL_REP_KIND_INVALID for out-of-range values.
inline occtl_rep_kind_t ToAbiRepKind(const BRepGraph_RepId::Kind theKind)
{
  static constexpr std::array<occtl_rep_kind_t, 7> THE_MAP = {
    OCCTL_REP_KIND_SURFACE,       // Surface       = 0
    OCCTL_REP_KIND_CURVE3D,       // Curve3D       = 1
    OCCTL_REP_KIND_CURVE2D,       // Curve2D       = 2
    OCCTL_REP_KIND_TRIANGULATION, // Triangulation = 3
    OCCTL_REP_KIND_POLYGON3D,     // Polygon3D     = 4
    OCCTL_REP_KIND_POLYGON2D,     // Polygon2D     = 5
    OCCTL_REP_KIND_POLYGON_ON_TRI // PolygonOnTri  = 6
  };
  const int aVal = static_cast<int>(theKind);
  return (aVal >= 0 && aVal < static_cast<int>(THE_MAP.size())) ? THE_MAP[static_cast<size_t>(aVal)]
                                                                : OCCTL_REP_KIND_INVALID;
}

//! @brief Reverse rep-kind map.  Returns true on success.
//! @param[in]  theKind ABI rep kind to resolve.
//! @param[out] theOut  Receives the OCCT kind on success; untouched on failure.
//! @return true if the kind was valid and non-INVALID, false otherwise.
inline bool TryToOcctRepKind(const occtl_rep_kind_t theKind, BRepGraph_RepId::Kind& theOut)
{
  switch (theKind)
  {
    case OCCTL_REP_KIND_SURFACE:
      theOut = BRepGraph_RepId::Kind::Surface;
      return true;
    case OCCTL_REP_KIND_CURVE3D:
      theOut = BRepGraph_RepId::Kind::Curve3D;
      return true;
    case OCCTL_REP_KIND_CURVE2D:
      theOut = BRepGraph_RepId::Kind::Curve2D;
      return true;
    case OCCTL_REP_KIND_TRIANGULATION:
      theOut = BRepGraph_RepId::Kind::Triangulation;
      return true;
    case OCCTL_REP_KIND_POLYGON3D:
      theOut = BRepGraph_RepId::Kind::Polygon3D;
      return true;
    case OCCTL_REP_KIND_POLYGON2D:
      theOut = BRepGraph_RepId::Kind::Polygon2D;
      return true;
    case OCCTL_REP_KIND_POLYGON_ON_TRI:
      theOut = BRepGraph_RepId::Kind::PolygonOnTri;
      return true;
    default:
      return false;
  }
}

//! @brief Packs an OCCT NodeId into the ABI 64-bit representation.
//! @param[in] theId OCCT BRepGraph node identity.
//! @return ABI node ID; all-zero if @p theId is invalid.
inline occtl_node_id_t PackNodeId(const BRepGraph_NodeId& theId)
{
  if (!theId.IsValid())
    return {0};
  const uint64_t aKind  = static_cast<uint64_t>(ToAbiNodeKind(theId.NodeKind)) << THE_KIND_SHIFT;
  const uint64_t aIndex = static_cast<uint64_t>(theId.Index) & THE_PAYLOAD_MASK;
  return {aKind | aIndex};
}

//! @brief Unpacks an ABI NodeId back to an OCCT NodeId.
//! @param[in] theId ABI node ID (may be all-zero or cross-type garbage).
//! @return OCCT node identity; invalid (IsValid()==false) when the ABI
//!         value is all-zero, has non-zero reserved bits, or encodes an
//!         unknown kind.
inline BRepGraph_NodeId UnpackNodeId(const occtl_node_id_t theId)
{
  if (theId.bits == 0 || (theId.bits & THE_RESERVED_MASK) != 0)
    return BRepGraph_NodeId();

  const uint8_t          aKindByte = static_cast<uint8_t>((theId.bits >> THE_KIND_SHIFT) & 0xff);
  BRepGraph_NodeId::Kind aOcctKind;
  if (!TryToOcctNodeKind(static_cast<occtl_node_kind_t>(aKindByte), aOcctKind))
    return BRepGraph_NodeId();

  const uint32_t aIndex = static_cast<uint32_t>(theId.bits & THE_PAYLOAD_MASK);
  return BRepGraph_NodeId(aOcctKind, aIndex);
}

//! @brief Packs an OCCT RefId into the ABI 64-bit representation.
//! @param[in] theId OCCT BRepGraph reference identity.
//! @return ABI ref ID; all-zero if @p theId is invalid.
inline occtl_ref_id_t PackRefId(const BRepGraph_RefId& theId)
{
  if (!theId.IsValid())
    return {0};
  const uint64_t aKind  = static_cast<uint64_t>(ToAbiRefKind(theId.RefKind)) << THE_KIND_SHIFT;
  const uint64_t aIndex = static_cast<uint64_t>(theId.Index) & THE_PAYLOAD_MASK;
  return {aKind | aIndex};
}

//! @brief Unpacks an ABI RefId back to an OCCT RefId.
//! @param[in] theId ABI ref ID.
//! @return OCCT ref identity; invalid when the ABI value is all-zero, has
//!         non-zero reserved bits, or encodes an unknown kind.
inline BRepGraph_RefId UnpackRefId(const occtl_ref_id_t theId)
{
  if (theId.bits == 0 || (theId.bits & THE_RESERVED_MASK) != 0)
    return BRepGraph_RefId();

  const uint8_t         aKindByte = static_cast<uint8_t>((theId.bits >> THE_KIND_SHIFT) & 0xff);
  BRepGraph_RefId::Kind aOcctKind;
  if (!TryToOcctRefKind(static_cast<occtl_ref_kind_t>(aKindByte), aOcctKind))
    return BRepGraph_RefId();

  const uint32_t aIndex = static_cast<uint32_t>(theId.bits & THE_PAYLOAD_MASK);
  return BRepGraph_RefId(aOcctKind, aIndex);
}

//! @brief Packs an OCCT RepId into the ABI 64-bit representation.
//! @param[in] theId OCCT BRepGraph representation identity.
//! @return ABI rep ID; all-zero if @p theId is invalid.
inline occtl_rep_id_t PackRepId(const BRepGraph_RepId& theId)
{
  if (!theId.IsValid())
    return {0};
  const uint64_t aKind  = static_cast<uint64_t>(ToAbiRepKind(theId.RepKind)) << THE_KIND_SHIFT;
  const uint64_t aIndex = static_cast<uint64_t>(theId.Index) & THE_PAYLOAD_MASK;
  return {aKind | aIndex};
}

//! @brief Unpacks an ABI RepId back to an OCCT RepId.
//! @param[in] theId ABI rep ID.
//! @return OCCT rep identity; invalid when the ABI value is all-zero, has
//!         non-zero reserved bits, or encodes an unknown kind.
inline BRepGraph_RepId UnpackRepId(const occtl_rep_id_t theId)
{
  if (theId.bits == 0 || (theId.bits & THE_RESERVED_MASK) != 0)
    return BRepGraph_RepId();

  const uint8_t         aKindByte = static_cast<uint8_t>((theId.bits >> THE_KIND_SHIFT) & 0xff);
  BRepGraph_RepId::Kind aOcctKind;
  if (!TryToOcctRepKind(static_cast<occtl_rep_kind_t>(aKindByte), aOcctKind))
    return BRepGraph_RepId();

  const uint32_t aIndex = static_cast<uint32_t>(theId.bits & THE_PAYLOAD_MASK);
  return BRepGraph_RepId(aOcctKind, aIndex);
}

//! @brief Packs an OCCT UID into the ABI 64-bit representation.
//! @param[in] theUid OCCT BRepGraph persistent identity.
//! @return ABI UID; all-zero if @p theUid is invalid.
inline occtl_uid_t PackUID(const BRepGraph_UID& theUid)
{
  if (!theUid.IsValid())
    return {0};

  const uint64_t aKind    = static_cast<uint64_t>(ToAbiNodeKind(theUid.Kind())) << THE_KIND_SHIFT;
  const uint64_t aCounter = static_cast<uint64_t>(theUid.Counter()) & THE_PAYLOAD_MASK;
  return {aKind | aCounter};
}

//! @brief Unpacks an ABI UID back to an OCCT UID.
//! @param[in] theUid ABI UID.
//! @return OCCT persistent identity; invalid when the ABI value is
//!         all-zero, has non-zero reserved bits, encodes an unknown kind,
//!         or has a zero counter.
inline BRepGraph_UID UnpackUID(const occtl_uid_t theUid)
{
  if (theUid.bits == 0 || (theUid.bits & THE_RESERVED_MASK) != 0)
    return BRepGraph_UID::Invalid();

  const uint8_t          aKindByte = static_cast<uint8_t>((theUid.bits >> THE_KIND_SHIFT) & 0xff);
  BRepGraph_NodeId::Kind aOcctKind;
  if (!TryToOcctNodeKind(static_cast<occtl_node_kind_t>(aKindByte), aOcctKind))
    return BRepGraph_UID::Invalid();

  const size_t aCounter = static_cast<size_t>(theUid.bits & THE_PAYLOAD_MASK);
  if (aCounter == 0)
    return BRepGraph_UID::Invalid();

  return BRepGraph_UID(aOcctKind, aCounter, /* generation */ 0u);
}

//! @brief Packs an OCCT RefUID into the ABI 64-bit representation.
//! @param[in] theUid OCCT BRepGraph persistent reference identity.
//! @return ABI RefUID; all-zero if @p theUid is invalid.
inline occtl_ref_uid_t PackRefUID(const BRepGraph_RefUID& theUid)
{
  if (!theUid.IsValid())
    return {0};

  const uint64_t aKind    = static_cast<uint64_t>(ToAbiRefKind(theUid.Kind())) << THE_KIND_SHIFT;
  const uint64_t aCounter = static_cast<uint64_t>(theUid.Counter()) & THE_PAYLOAD_MASK;
  return {aKind | aCounter};
}

//! @brief Unpacks an ABI RefUID back to an OCCT RefUID.
//! @param[in] theUid ABI RefUID.
//! @return OCCT persistent reference identity, or invalid on malformed input.
inline BRepGraph_RefUID UnpackRefUID(const occtl_ref_uid_t theUid)
{
  if (theUid.bits == 0 || (theUid.bits & THE_RESERVED_MASK) != 0)
    return BRepGraph_RefUID::Invalid();

  const uint8_t         aKindByte = static_cast<uint8_t>((theUid.bits >> THE_KIND_SHIFT) & 0xff);
  BRepGraph_RefId::Kind aOcctKind;
  if (!TryToOcctRefKind(static_cast<occtl_ref_kind_t>(aKindByte), aOcctKind))
    return BRepGraph_RefUID::Invalid();

  const size_t aCounter = static_cast<size_t>(theUid.bits & THE_PAYLOAD_MASK);
  if (aCounter == 0)
    return BRepGraph_RefUID::Invalid();

  return BRepGraph_RefUID(aOcctKind, aCounter, /* generation */ 0u);
}

//! @brief Packs an OCCT RepUID into the ABI 64-bit representation.
//! @param[in] theUid OCCT BRepGraph persistent representation identity.
//! @return ABI RepUID; all-zero if @p theUid is invalid.
inline occtl_rep_uid_t PackRepUID(const BRepGraph_RepUID& theUid)
{
  if (!theUid.IsValid())
    return {0};

  const uint64_t aKind    = static_cast<uint64_t>(ToAbiRepKind(theUid.Kind())) << THE_KIND_SHIFT;
  const uint64_t aCounter = static_cast<uint64_t>(theUid.Counter()) & THE_PAYLOAD_MASK;
  return {aKind | aCounter};
}

//! @brief Unpacks an ABI RepUID back to an OCCT RepUID.
//! @param[in] theUid ABI RepUID.
//! @return OCCT persistent representation identity, or invalid on malformed input.
inline BRepGraph_RepUID UnpackRepUID(const occtl_rep_uid_t theUid)
{
  if (theUid.bits == 0 || (theUid.bits & THE_RESERVED_MASK) != 0)
    return BRepGraph_RepUID::Invalid();

  const uint8_t         aKindByte = static_cast<uint8_t>((theUid.bits >> THE_KIND_SHIFT) & 0xff);
  BRepGraph_RepId::Kind aOcctKind;
  if (!TryToOcctRepKind(static_cast<occtl_rep_kind_t>(aKindByte), aOcctKind))
    return BRepGraph_RepUID::Invalid();

  const size_t aCounter = static_cast<size_t>(theUid.bits & THE_PAYLOAD_MASK);
  if (aCounter == 0)
    return BRepGraph_RepUID::Invalid();

  return BRepGraph_RepUID(aOcctKind, aCounter, /* generation */ 0u);
}

} // namespace OcctL::Topo

#endif // OCCTL_TOPO_TOPO_MATH_HXX
