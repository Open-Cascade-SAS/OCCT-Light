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

occtl_add_module(prim
  SOURCES
    analytic_solids.cxx
    halfspace.cxx
    swept_solids.cxx
    thru_sections.cxx
    sketches.cxx
    pipe_shell.cxx
    offsets.cxx
    twist_extrusion.cxx
    draft_prism.cxx
    ruled_surface.cxx
    feat_prism.cxx
    cylindrical_hole.cxx
    convenience.cxx
    helix.cxx
    fillet_2d.cxx
    constrained_curve_edge.cxx
  PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_prim.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_prim_feature.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_prim_sketch.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_prim_solid.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_prim_sweep.h
    ${CMAKE_SOURCE_DIR}/include/occtl-hpp/prim.hpp
  PUBLIC_LINK
    OCCTL::core
    OCCTL::geom
    OCCTL::topo
  PRIVATE_LINK
    TKPrim
    TKOffset
    TKBRep
    TKTopAlgo
    TKFeat
    TKGeomAlgo
    TKG3d
    TKG2d
    TKFillet
)

occtl_module_compile_definitions(prim OCCTL_HAS_PRIM)
