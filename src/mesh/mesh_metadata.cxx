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

//! @brief Mesh-module spelling for graph-owned model metadata.

#include <occtl/occtl_mesh.h>

#include "../core/Guard.hxx"

extern "C"
{

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_model_metadata_set(occtl_graph_t* const graph,
                                                                  const char* const    key,
                                                                  const size_t         keyLen,
                                                                  const char* const    value,
                                                                  const size_t         valueLen)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return occtl_graph_metadata_set(graph, key, keyLen, value, valueLen);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_model_metadata_get(const occtl_graph_t* const graph,
                                                                  const char* const          key,
                                                                  const size_t               keyLen,
                                                                  char* const                buf,
                                                                  const size_t  bufSize,
                                                                  size_t* const out_required)
{
  return OcctL::Core::Guard([&]() -> occtl_status_t {
    return occtl_graph_metadata_get(graph, key, keyLen, buf, bufSize, out_required);
  });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL
  occtl_mesh_model_metadata_keys(const occtl_graph_t* const       graph,
                                 occtl_metadata_key_view_t* const out_keys,
                                 const size_t                     cap,
                                 size_t* const                    out_count)
{
  return OcctL::Core::Guard(
    [&]() -> occtl_status_t { return occtl_graph_metadata_keys(graph, out_keys, cap, out_count); });
}

//==================================================================================================

OCCTL_API occtl_status_t OCCTL_CALL occtl_mesh_model_metadata_unset(occtl_graph_t* const graph,
                                                                    const char* const    key,
                                                                    const size_t         keyLen)
{
  return OcctL::Core::Guard(
    [&]() -> occtl_status_t { return occtl_graph_metadata_unset(graph, key, keyLen); });
}

} // extern "C"
