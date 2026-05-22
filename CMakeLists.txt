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

cmake_minimum_required(VERSION 3.23)

# === Version =====================================================================================
# Single source of truth for the SemVer triplet. Public headers also carry these as macros
# (OCCTL_VERSION_MAJOR/MINOR/PATCH) so consumers can branch on compile-time version.
set(OCCTL_VERSION_MAJOR 0)
set(OCCTL_VERSION_MINOR 1)
set(OCCTL_VERSION_PATCH 0)

# ABI version is bumped only on hard breaking changes.
# Independent of SemVer; allows consumers to refuse incompatible builds at runtime.
set(OCCTL_ABI_VERSION 1)

project(occtl
  VERSION       ${OCCTL_VERSION_MAJOR}.${OCCTL_VERSION_MINOR}.${OCCTL_VERSION_PATCH}
  DESCRIPTION   "OCCT-Light: a C-ABI wrapper around Open CASCADE Technology"
  HOMEPAGE_URL  "https://github.com/Open-Cascade-SAS/OCCT-Light"
  LANGUAGES     C CXX
)

# === Module path =================================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# === Standards ===================================================================================
set(CMAKE_C_STANDARD             11)
set(CMAKE_C_STANDARD_REQUIRED    ON)
set(CMAKE_C_EXTENSIONS           OFF)
set(CMAKE_CXX_STANDARD           17)
set(CMAKE_CXX_STANDARD_REQUIRED  ON)
set(CMAKE_CXX_EXTENSIONS         OFF)

# Hidden visibility by default; OCCTL_API explicitly exports the public surface.
# Skipped under Emscripten: visibility flags are irrelevant for wasm and
# -fvisibility-inlines-hidden breaks vtable dispatch across boundaries.
if(NOT EMSCRIPTEN)
  set(CMAKE_C_VISIBILITY_PRESET   hidden)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif()

# Position-independent code for static libs that can be linked into shared bindings.
# Skipped under Emscripten: wasm does not use shared libraries.
if(NOT EMSCRIPTEN)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Default to RelWithDebInfo when not set; debug info is invaluable for binding crashes.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug Release RelWithDebInfo MinSizeRel)
endif()

# === Includes ====================================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(OCCTLOptions)
include(OCCTLCompileOptions)
include(OCCTLModule)
include(OCCTLAbi)
include(OCCTLBindingsPackaging)
include(OCCTLRegistry)

# Threads: required on native platforms for thread-local error state and allocator
# statistics. Under Emscripten, single-threaded builds use thread_local without
# pthreads; threaded wasm builds pass -pthread and find_package(Threads) works.
if(EMSCRIPTEN)
  find_package(Threads QUIET)
else()
  find_package(Threads REQUIRED)
endif()

# === Public-header hygiene (Rule 1 enforcement) ==================================================
# Scans include/occtl/*.h for forbidden STL/OCCT identifiers. Doxygen comments
# are stripped before scanning, so doc references are fine.
if(OCCTL_ENFORCE_HEADER_HYGIENE AND NOT EMSCRIPTEN)
  include(CheckHeaderHygiene)
  occtl_check_header_hygiene("${CMAKE_SOURCE_DIR}/include/occtl")

  include(CheckPublicNaming)
  occtl_check_public_naming("${CMAKE_SOURCE_DIR}/include/occtl")

  include(CheckReleaseReadiness)
  occtl_add_release_readiness_checks()
endif()

# === OCCT dependency ==============================================================================
occtl_registry_validate_enabled()
occtl_registry_collect_enabled(_occtl_enabled_modules _occtl_required_toolkits)

if(_occtl_required_toolkits)
  list(REMOVE_DUPLICATES _occtl_required_toolkits)
  set(OCCTL_REQUIRED_OCCT_TOOLKITS "${_occtl_required_toolkits}" CACHE INTERNAL "OCCT toolkits required by this OCCT-Light feature set")
  find_package(OCCT REQUIRED COMPONENTS ${_occtl_required_toolkits})
else()
  set(OCCTL_REQUIRED_OCCT_TOOLKITS "" CACHE INTERNAL "OCCT toolkits required by this OCCT-Light feature set")
endif()

# === Output layout ================================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# === Modules ======================================================================================
occtl_registry_add_enabled_subdirectories()

# === Single physical library ======================================================================
occtl_create_single_library()
occtl_registry_write_feature_metadata()
occtl_configure_abi_dump()
occtl_configure_bindings_packaging()

# === Bindings =====================================================================================
# Each language facade lives under bindings/<lang>/ and is gated on its own option.
# All default OFF; bindings/ is only descended into when at least one is requested.
if(NOT EMSCRIPTEN)
  if(OCCTL_BUILD_BINDINGS_PYTHON OR OCCTL_BUILD_BINDINGS_CSHARP
      OR OCCTL_BUILD_BINDINGS_NODE OR OCCTL_BUILD_BINDINGS_WASM
      OR OCCTL_BUILD_BINDINGS_RUST OR OCCTL_BUILD_BINDINGS_GO
      OR OCCTL_BUILD_BINDINGS_JAVA)
    add_subdirectory(bindings)
  endif()
endif()

# === Tests ========================================================================================
if(OCCTL_BUILD_TESTING AND NOT EMSCRIPTEN)
  enable_testing()
  add_subdirectory(tests)
endif()

# === Install: package config ======================================================================
configure_package_config_file(
  cmake/OCCTLConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/OCCTLConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OCCTL
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/OCCTLConfigVersion.cmake
  VERSION       ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/OCCTLConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/OCCTLConfigVersion.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindOCCT.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OCCTL
)

install(EXPORT OCCTLTargets
  FILE        OCCTLTargets.cmake
  NAMESPACE   OCCTL::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OCCTL
)

# === Summary ======================================================================================
message(STATUS "")
message(STATUS "OCCT-Light ${PROJECT_VERSION} (ABI v${OCCTL_ABI_VERSION})")
message(STATUS "  Build type        : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libraries  : ${OCCTL_SHARED_LIBS}")
occtl_registry_print_summary()
message(STATUS "  Testing           : ${OCCTL_BUILD_TESTING}")
message(STATUS "")
