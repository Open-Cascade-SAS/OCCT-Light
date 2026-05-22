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
 * @brief C++ veneer for unified data exchange.
 */

#ifndef OCCTL_HPP_DE_HPP
#define OCCTL_HPP_DE_HPP

#include <occtl/occtl_de.h>

#include <occtl-hpp/core.hpp>
#include <occtl-hpp/topo.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace occtl::de
{

struct FormatInfo
{
  std::string              id;
  std::string              label;
  std::vector<std::string> extensions;
  bool                     can_read_file    = false;
  bool                     can_write_file   = false;
  bool                     can_read_memory  = false;
  bool                     can_write_memory = false;
};

/// @brief Reads a file dispatched by extension and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read(const std::string& thePath)
{
  ::occtl_graph_t*  aRaw = nullptr;
  ::occtl_node_id_t aRoot{};
  check(::occtl_de_read(thePath.c_str(), &aRaw, &aRoot));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Reads a memory payload using an explicit format id and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read_memory(const std::string&   theFormatId,
                                            const uint8_t* const theData,
                                            const std::size_t    theSize)
{
  ::occtl_graph_t*  aRaw = nullptr;
  ::occtl_node_id_t aRoot{};
  check(::occtl_de_read_memory(theFormatId.c_str(), theData, theSize, &aRaw, &aRoot));
  return {Graph(aRaw), NodeId(aRoot)};
}

/// @brief Reads a memory payload using an explicit format id and returns (graph, root).
/// @throws Error on failure.
inline std::pair<Graph, NodeId> read_memory(const std::string&          theFormatId,
                                            const std::vector<uint8_t>& theData)
{
  return read_memory(theFormatId, theData.empty() ? nullptr : theData.data(), theData.size());
}

/// @brief Writes a graph root to a file dispatched by extension.
inline void write(const Graph& theGraph, const NodeId theRoot, const std::string& thePath)
{
  check(::occtl_de_write(theGraph.get(), theRoot.get(), thePath.c_str()));
}

/// @brief Writes a graph root into a memory payload using an explicit format id.
/// @throws Error on failure.
inline std::vector<uint8_t> write_memory(const Graph&       theGraph,
                                         const NodeId       theRoot,
                                         const std::string& theFormatId)
{
  std::size_t aSize = 0;
  check(::occtl_de_write_memory(theGraph.get(),
                                theRoot.get(),
                                theFormatId.c_str(),
                                nullptr,
                                0,
                                &aSize));

  std::vector<uint8_t> aData(aSize);
  check(::occtl_de_write_memory(theGraph.get(),
                                theRoot.get(),
                                theFormatId.c_str(),
                                aData.empty() ? nullptr : aData.data(),
                                aData.size(),
                                &aSize));
  return aData;
}

/// @brief Returns the stable lowercase format ids the build supports.
inline std::vector<std::string> supported_formats()
{
  size_t aCount = 0;
  check(::occtl_de_format_ids(nullptr, 0, &aCount));
  std::vector<const char*> aIds(aCount);
  size_t                   aGot = 0;
  check(::occtl_de_format_ids(aIds.data(), aCount, &aGot));
  std::vector<std::string> aOut;
  aOut.reserve(aGot);
  for (size_t i = 0; i < aGot; ++i)
  {
    aOut.emplace_back(aIds[i]);
  }
  return aOut;
}

/// @brief Returns rich descriptors for every supported data-exchange format.
inline std::vector<FormatInfo> format_infos()
{
  size_t aCount = 0;
  check(::occtl_de_format_count(&aCount));

  std::vector<FormatInfo> aOut;
  aOut.reserve(aCount);
  for (size_t anIndex = 0; anIndex < aCount; ++anIndex)
  {
    ::occtl_de_format_info_t aRaw = OCCTL_DE_FORMAT_INFO_INIT;
    check(::occtl_de_format_info_at(anIndex, &aRaw));

    size_t anExtCount = 0;
    check(::occtl_de_format_extensions(aRaw.id, nullptr, 0, &anExtCount));
    std::vector<const char*> aExts(anExtCount);
    size_t                   anExtGot = 0;
    check(::occtl_de_format_extensions(aRaw.id, aExts.data(), aExts.size(), &anExtGot));

    FormatInfo anInfo;
    anInfo.id               = aRaw.id != nullptr ? aRaw.id : "";
    anInfo.label            = aRaw.label != nullptr ? aRaw.label : "";
    anInfo.can_read_file    = aRaw.can_read_file != 0;
    anInfo.can_write_file   = aRaw.can_write_file != 0;
    anInfo.can_read_memory  = aRaw.can_read_memory != 0;
    anInfo.can_write_memory = aRaw.can_write_memory != 0;
    anInfo.extensions.reserve(anExtGot);
    for (size_t anExtIndex = 0; anExtIndex < anExtGot; ++anExtIndex)
    {
      anInfo.extensions.emplace_back(aExts[anExtIndex]);
    }
    aOut.emplace_back(std::move(anInfo));
  }
  return aOut;
}

/// @brief Returns a descriptor for one supported data-exchange format.
inline FormatInfo format_info(const std::string& theFormatId)
{
  ::occtl_de_format_info_t aRaw = OCCTL_DE_FORMAT_INFO_INIT;
  check(::occtl_de_format_info_by_id(theFormatId.c_str(), &aRaw));

  size_t anExtCount = 0;
  check(::occtl_de_format_extensions(aRaw.id, nullptr, 0, &anExtCount));
  std::vector<const char*> aExts(anExtCount);
  size_t                   anExtGot = 0;
  check(::occtl_de_format_extensions(aRaw.id, aExts.data(), aExts.size(), &anExtGot));

  FormatInfo anInfo;
  anInfo.id               = aRaw.id != nullptr ? aRaw.id : "";
  anInfo.label            = aRaw.label != nullptr ? aRaw.label : "";
  anInfo.can_read_file    = aRaw.can_read_file != 0;
  anInfo.can_write_file   = aRaw.can_write_file != 0;
  anInfo.can_read_memory  = aRaw.can_read_memory != 0;
  anInfo.can_write_memory = aRaw.can_write_memory != 0;
  anInfo.extensions.reserve(anExtGot);
  for (size_t anExtIndex = 0; anExtIndex < anExtGot; ++anExtIndex)
  {
    anInfo.extensions.emplace_back(aExts[anExtIndex]);
  }
  return anInfo;
}

/// @brief Returns the format id for @p thePath, or std::nullopt if no extension matches.
inline std::optional<std::string> format_id_for_path(const std::string& thePath)
{
  const char* aId = nullptr;
  check(::occtl_de_format_id_from_path(thePath.c_str(), &aId));
  if (aId == nullptr)
  {
    return std::nullopt;
  }
  return std::string(aId);
}

} // namespace occtl::de

#endif // OCCTL_HPP_DE_HPP
