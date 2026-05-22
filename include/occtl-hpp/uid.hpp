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
 * @brief C++ veneer wrapper for persistent topology UIDs.
 */

#ifndef OCCTL_HPP_UID_HPP
#define OCCTL_HPP_UID_HPP

#include <occtl-hpp/core.hpp>

#include <occtl/occtl_topo.h>

#include <array>
#include <cstdint>

namespace occtl
{

/// @brief Persistent identity surviving node removal and graph compaction. Mirrors @c occtl_uid_t.
///
/// Use #occtl::Graph::node_id_from_uid to resolve back to a NodeId.
/// The all-zero value (#OCCTL_UID_INVALID) is the invalid sentinel.
class UID
{
public:
  /// @brief Wraps an existing C value type (zero-cost).
  explicit UID(const ::occtl_uid_t theUid) noexcept
      : myUid(theUid)
  {
  }

  /// @brief Returns the invalid sentinel (all-zero bits).
  static UID invalid() noexcept { return UID(OCCTL_UID_INVALID); }

  /// @brief Borrows-it view of the underlying C value type, for direct ABI calls.
  ::occtl_uid_t get() const noexcept { return myUid; }

  bool is_valid() const noexcept
  {
    return myUid.bits != 0;
  } ///< True when the UID is not the all-zero sentinel.

  bool operator==(const UID& theOther) const noexcept
  {
    return myUid.bits == theOther.myUid.bits;
  } ///< Bitwise equality.

  bool operator!=(const UID& theOther) const noexcept
  {
    return myUid.bits != theOther.myUid.bits;
  } ///< Bitwise inequality.

private:
  ::occtl_uid_t myUid;
};

/// @brief Persistent identity surviving reference removal and graph compaction.
///
/// Use #occtl::Graph::ref_id_from_ref_uid to resolve back to a RefId.
/// The all-zero value (#OCCTL_REF_UID_INVALID) is the invalid sentinel.
class RefUID
{
public:
  /// @brief Wraps an existing C value type (zero-cost).
  explicit RefUID(const ::occtl_ref_uid_t theUid) noexcept
      : myUid(theUid)
  {
  }

  /// @brief Returns the invalid sentinel (all-zero bits).
  static RefUID invalid() noexcept { return RefUID(OCCTL_REF_UID_INVALID); }

  /// @brief Borrows-it view of the underlying C value type, for direct ABI calls.
  ::occtl_ref_uid_t get() const noexcept { return myUid; }

  /// @brief Decodes a RefUID from the fixed-width C wire format.
  /// @throws Error with code OCCTL_FORMAT_ERROR when reserved bytes are non-zero.
  static RefUID from_bytes(const std::array<std::uint8_t, OCCTL_REF_UID_WIRE_SIZE>& theBytes)
  {
    ::occtl_ref_uid_t aUid = OCCTL_REF_UID_INVALID;
    check(::occtl_ref_uid_from_bytes(theBytes.data(), &aUid));
    return RefUID(aUid);
  }

  /// @brief Encodes this RefUID into the fixed-width C wire format.
  std::array<std::uint8_t, OCCTL_REF_UID_WIRE_SIZE> to_bytes() const
  {
    std::array<std::uint8_t, OCCTL_REF_UID_WIRE_SIZE> aBytes{};
    check(::occtl_ref_uid_to_bytes(myUid, aBytes.data()));
    return aBytes;
  }

  bool is_valid() const noexcept
  {
    return myUid.bits != 0;
  } ///< True when the UID is not the all-zero sentinel.

  bool operator==(const RefUID& theOther) const noexcept
  {
    return myUid.bits == theOther.myUid.bits;
  } ///< Bitwise equality.

  bool operator!=(const RefUID& theOther) const noexcept
  {
    return myUid.bits != theOther.myUid.bits;
  } ///< Bitwise inequality.

private:
  ::occtl_ref_uid_t myUid;
};

/// @brief Persistent identity surviving representation removal and graph compaction.
///
/// Use #occtl::Graph::rep_id_from_rep_uid to resolve back to a RepId.
/// The all-zero value (#OCCTL_REP_UID_INVALID) is the invalid sentinel.
class RepUID
{
public:
  /// @brief Wraps an existing C value type (zero-cost).
  explicit RepUID(const ::occtl_rep_uid_t theUid) noexcept
      : myUid(theUid)
  {
  }

  /// @brief Returns the invalid sentinel (all-zero bits).
  static RepUID invalid() noexcept { return RepUID(OCCTL_REP_UID_INVALID); }

  /// @brief Borrows-it view of the underlying C value type, for direct ABI calls.
  ::occtl_rep_uid_t get() const noexcept { return myUid; }

  /// @brief Decodes a RepUID from the fixed-width C wire format.
  /// @throws Error with code OCCTL_FORMAT_ERROR when reserved bytes are non-zero.
  static RepUID from_bytes(const std::array<std::uint8_t, OCCTL_REP_UID_WIRE_SIZE>& theBytes)
  {
    ::occtl_rep_uid_t aUid = OCCTL_REP_UID_INVALID;
    check(::occtl_rep_uid_from_bytes(theBytes.data(), &aUid));
    return RepUID(aUid);
  }

  /// @brief Encodes this RepUID into the fixed-width C wire format.
  std::array<std::uint8_t, OCCTL_REP_UID_WIRE_SIZE> to_bytes() const
  {
    std::array<std::uint8_t, OCCTL_REP_UID_WIRE_SIZE> aBytes{};
    check(::occtl_rep_uid_to_bytes(myUid, aBytes.data()));
    return aBytes;
  }

  bool is_valid() const noexcept
  {
    return myUid.bits != 0;
  } ///< True when the UID is not the all-zero sentinel.

  bool operator==(const RepUID& theOther) const noexcept
  {
    return myUid.bits == theOther.myUid.bits;
  } ///< Bitwise equality.

  bool operator!=(const RepUID& theOther) const noexcept
  {
    return myUid.bits != theOther.myUid.bits;
  } ///< Bitwise inequality.

private:
  ::occtl_rep_uid_t myUid;
};

} // namespace occtl

#endif // OCCTL_HPP_UID_HPP
