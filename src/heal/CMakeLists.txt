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

occtl_add_module(heal
  SOURCES
    extern_c.cxx
  PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_heal.h
    ${CMAKE_SOURCE_DIR}/include/occtl-hpp/heal.hpp
  PUBLIC_LINK
    OCCTL::core
    OCCTL::topo
  PRIVATE_LINK
    TKShHealing
    TKBRep
    TKTopAlgo
    TKMath
)

occtl_module_compile_definitions(heal OCCTL_HAS_HEAL)
