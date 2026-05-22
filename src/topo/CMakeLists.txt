# Copyright (c) 2026 Capgemini Engineering Research and Development.
#
# This file is part of OCCT-Light software library.
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License version 3 as published
# by the Free Software Foundation, with an option to use any later version.
# Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
# for complete text of the license and disclaimer of any warranty.
#
# Alternatively, this file may be used under the terms of a commercial
# license or contractual agreement.
#
# SPDX-License-Identifier: AGPL-3.0-or-later

include(OCCTLModule)

occtl_add_module(topo
  SOURCES
    graph_lifecycle.cxx
    graph_counts.cxx
    graph_identity.cxx
    graph_geom.cxx
    graph_iterators.cxx
    graph_visitors.cxx
    graph_views.cxx
    graph_algo.cxx
    graph_mutation.cxx
    graph_batch.cxx
    graph_explorers.cxx
    graph_related.cxx
    graph_reforient.cxx
    graph_layers.cxx
    graph_cache.cxx
    graph_select.cxx
    graph_assembly.cxx
    graph_compose.cxx
    graph_readgaps.cxx
    graph_inplace.cxx
    history.cxx
  PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_topo.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_topo_types.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_topo_build.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_topo_relation.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_topo_algo.h
    ${CMAKE_SOURCE_DIR}/include/occtl-hpp/topo.hpp
    ${CMAKE_SOURCE_DIR}/include/occtl-hpp/uid.hpp
  PRIVATE_HEADER
    IdConvert.hxx
    GraphHandle.hxx
    GraphGeometryKind.hxx
    GraphLayers.hxx
    GraphMeasureCache.hxx
    TopoMath.hxx
  PUBLIC_LINK
    OCCTL::core
    OCCTL::geom
  PRIVATE_LINK
    TKBRep
    TKBool
    TKTopAlgo
    TKGeomAlgo
    TKMath
    TKG3d
    TKFillet
    TKOffset
    TKHLR
)

occtl_module_compile_definitions(topo OCCTL_HAS_TOPO)
