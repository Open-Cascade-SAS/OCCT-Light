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

occtl_add_module(geom
  SOURCES
    extern_c.cxx
    analytic_curves.cxx
    analytic_curves2d.cxx
    spline_curves.cxx
    spline_curves2d.cxx
    surfaces.cxx
    evaluation.cxx
    geom_algo.cxx
  PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_geom.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_curves.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_curves2d.h
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_surfaces.h
    ${CMAKE_SOURCE_DIR}/include/occtl-hpp/geom.hpp
    ${CMAKE_SOURCE_DIR}/include/occtl-hpp/curves.hpp
    ${CMAKE_SOURCE_DIR}/include/occtl-hpp/curves2d.hpp
    ${CMAKE_SOURCE_DIR}/include/occtl-hpp/surfaces.hpp
  PRIVATE_HEADER
    CurveMath.hxx
    GeomMath.hxx
    KindDetect.hxx
    RepLookup.hxx
  PUBLIC_LINK
    OCCTL::core
  PRIVATE_LINK
    TKMath
    TKG3d
    TKG2d
    TKGeomBase
    TKGeomAlgo
    TKTopAlgo
    TKBRep
)

# Propagate OCCTL_HAS_GEOM to every consumer of occtl-geom.
occtl_module_compile_definitions(geom OCCTL_HAS_GEOM)
