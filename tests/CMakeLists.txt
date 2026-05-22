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

find_package(GTest QUIET)
if(NOT GTest_FOUND)
  message(STATUS "GTest not found via find_package; fetching gtest 1.14.0")
  include(FetchContent)
  FetchContent_Declare(googletest
    URL                       https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
    URL_HASH                  SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(BUILD_GMOCK   OFF CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  if(NOT TARGET GTest::gtest)
    add_library(GTest::gtest ALIAS gtest)
  endif()
  if(NOT TARGET GTest::gtest_main)
    add_library(GTest::gtest_main ALIAS gtest_main)
  endif()
endif()

include(GoogleTest)

function(occtl_gtest_discover_tests theTarget)
  gtest_discover_tests(${theTarget}
    DISCOVERY_TIMEOUT 30
    ${ARGN}
  )
endfunction()

add_subdirectory(core)
if(OCCTL_BUILD_GEOM)
  add_subdirectory(geom)
endif()
if(OCCTL_BUILD_TOPO)
  add_subdirectory(topo)
endif()
if(OCCTL_BUILD_PRIM)
  add_subdirectory(prim)
endif()
if(OCCTL_BUILD_TEXT)
  add_subdirectory(text)
endif()
if(OCCTL_BUILD_BOOL AND OCCTL_BUILD_PRIM)
  add_subdirectory(bool)
endif()
if(OCCTL_BUILD_MESH AND OCCTL_BUILD_PRIM)
  add_subdirectory(mesh)
endif()
if(OCCTL_BUILD_HEAL AND OCCTL_BUILD_PRIM)
  add_subdirectory(heal)
endif()
if(OCCTL_BUILD_IO_BREP AND OCCTL_BUILD_PRIM AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/io_brep/CMakeLists.txt)
  add_subdirectory(io_brep)
endif()
if(OCCTL_BUILD_IO_STEP AND OCCTL_BUILD_PRIM AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/io_step/CMakeLists.txt)
  add_subdirectory(io_step)
endif()
if(OCCTL_BUILD_IO_IGES AND OCCTL_BUILD_PRIM AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/io_iges/CMakeLists.txt)
  add_subdirectory(io_iges)
endif()
if(OCCTL_BUILD_IO_STL AND OCCTL_BUILD_PRIM AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/io_stl/CMakeLists.txt)
  add_subdirectory(io_stl)
endif()
if(OCCTL_BUILD_IO_OBJ AND OCCTL_BUILD_PRIM AND OCCTL_BUILD_MESH AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/io_obj/CMakeLists.txt)
  add_subdirectory(io_obj)
endif()
if(OCCTL_BUILD_IO_GLTF AND OCCTL_BUILD_PRIM AND OCCTL_BUILD_MESH AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/io_gltf/CMakeLists.txt)
  add_subdirectory(io_gltf)
endif()
if(OCCTL_BUILD_IO_VRML AND OCCTL_BUILD_PRIM AND OCCTL_BUILD_MESH AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/io_vrml/CMakeLists.txt)
  add_subdirectory(io_vrml)
endif()
if(OCCTL_BUILD_IO_PLY AND OCCTL_BUILD_PRIM AND OCCTL_BUILD_MESH AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/io_ply/CMakeLists.txt)
  add_subdirectory(io_ply)
endif()
if(OCCTL_BUILD_DE AND OCCTL_BUILD_PRIM AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/de/CMakeLists.txt)
  add_subdirectory(de)
endif()
if(OCCTL_BUILD_VIZ AND OCCTL_BUILD_PRIM AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/viz/CMakeLists.txt)
  add_subdirectory(viz)
endif()

# Binding parity grid. Each language's runner is gated on its own
# OCCTL_BUILD_BINDINGS_<LANG> option inside binding_parity/CMakeLists.txt.
add_subdirectory(binding_parity)
