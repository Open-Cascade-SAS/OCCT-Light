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

occtl_add_module(de
  SOURCES
    extern_c.cxx
  PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/occtl/occtl_de.h
    ${CMAKE_SOURCE_DIR}/include/occtl-hpp/de.hpp
  PUBLIC_LINK
    OCCTL::topo
  PRIVATE_LINK
    TKDE
)

# Soft-deps: link any io_* module that was built in this configuration,
# together with the OCCT toolkit that provides its DE_Wrapper config node.
# `de` dispatches to whichever providers are present at link time.
if(OCCTL_BUILD_IO_BREP)
  occtl_module_link_libraries(de TKDECascade)
  target_compile_definitions(occtl-de-obj PRIVATE OCCTL_DE_HAS_IO_BREP)
endif()
if(OCCTL_BUILD_IO_STEP)
  occtl_module_link_libraries(de TKDESTEP)
  target_compile_definitions(occtl-de-obj PRIVATE OCCTL_DE_HAS_IO_STEP)
endif()
if(OCCTL_BUILD_IO_IGES)
  occtl_module_link_libraries(de TKDEIGES)
  target_compile_definitions(occtl-de-obj PRIVATE OCCTL_DE_HAS_IO_IGES)
endif()
if(OCCTL_BUILD_IO_STL)
  occtl_module_link_libraries(de TKDESTL)
  target_compile_definitions(occtl-de-obj PRIVATE OCCTL_DE_HAS_IO_STL)
endif()
if(OCCTL_BUILD_IO_OBJ)
  occtl_module_link_libraries(de TKDEOBJ)
  target_compile_definitions(occtl-de-obj PRIVATE OCCTL_DE_HAS_IO_OBJ)
endif()
if(OCCTL_BUILD_IO_GLTF)
  occtl_module_link_libraries(de TKDEGLTF)
  target_compile_definitions(occtl-de-obj PRIVATE OCCTL_DE_HAS_IO_GLTF)
endif()
if(OCCTL_BUILD_IO_VRML)
  occtl_module_link_libraries(de TKDEVRML)
  target_compile_definitions(occtl-de-obj PRIVATE OCCTL_DE_HAS_IO_VRML)
endif()
if(OCCTL_BUILD_IO_PLY)
  occtl_module_link_libraries(de TKDEPLY)
  target_compile_definitions(occtl-de-obj PRIVATE OCCTL_DE_HAS_IO_PLY)
endif()

occtl_module_compile_definitions(de OCCTL_HAS_DE)
