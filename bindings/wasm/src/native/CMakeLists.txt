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

cmake_minimum_required(VERSION 3.16)
project(occtl_wasm_native LANGUAGES C CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR
    "OCCT-Light WASM binding requires an Emscripten toolchain. "
    "Configure with `emcmake cmake -S bindings/wasm/src/native -B bindings/wasm/build` "
    "after sourcing emsdk_env.sh.")
endif()

set(_wasm_dir "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Refuse to run if the generator hasn't been invoked.
if(NOT EXISTS "${_wasm_dir}/src/native/raw.cc")
  message(FATAL_ERROR
    "src/native/raw.cc not found. Run `npm run generate` in bindings/wasm first; "
    "it reads ../../build/abi.json and emits src/native/raw.cc plus src/ts/*.ts.")
endif()

# Public headers — the wrapper's `include/occtl/*.h`. Caller can override
# via -DOCCTL_HEADERS_DIR=... ; default assumes a standard repo layout.
set(OCCTL_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../include"
    CACHE PATH "Path to OCCT-Light public headers (include/occtl/*.h parent dir).")

if(NOT EXISTS "${OCCTL_HEADERS_DIR}/occtl/occtl.h")
  message(FATAL_ERROR
    "OCCTL_HEADERS_DIR=${OCCTL_HEADERS_DIR} does not contain occtl/occtl.h. "
    "Pass an absolute path with -DOCCTL_HEADERS_DIR=...")
endif()

include("${_wasm_dir}/emscripten.cmake")

# === The WASM module target ======================================================================
add_executable(occtl_wasm
  "${_wasm_dir}/src/native/main.cc"
  "${_wasm_dir}/src/native/error.cc"
  "${_wasm_dir}/src/native/handles.cc"
  "${_wasm_dir}/src/native/raw.cc"
)

target_compile_features(occtl_wasm PRIVATE cxx_std_17)
set_target_properties(occtl_wasm PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(occtl_wasm PRIVATE "${OCCTL_HEADERS_DIR}")
target_compile_definitions(occtl_wasm PRIVATE
  OCCTL_HAS_GEOM
  OCCTL_HAS_TOPO
  OCCTL_HAS_PRIM
  OCCTL_HAS_TEXT
  OCCTL_HAS_BOOL
  OCCTL_HAS_MESH
  OCCTL_HAS_HEAL
  OCCTL_HAS_DE
  OCCTL_HAS_IO_BREP
  OCCTL_HAS_IO_STEP
  OCCTL_HAS_IO_IGES
  OCCTL_HAS_IO_STL
  OCCTL_HAS_IO_OBJ
  OCCTL_HAS_IO_GLTF
  OCCTL_HAS_IO_VRML
  OCCTL_HAS_IO_PLY
)

# OCCT-Light static library (pre-built for wasm). Pass via:
#   -DOCCTL_ALL_LIB=/path/to/build/wasm/lib/libocctl.a
set(OCCTL_ALL_LIB ""
    CACHE FILEPATH "Path to the OCCT-Light wasm static library (build/wasm/lib/libocctl.a).")

# OCCT wasm installation. Pass via:
#   -DOCCT_DIR=/path/to/OCCT/install-wasm/lib/cmake/opencascade
set(OCCT_DIR ""
    CACHE PATH "Path to the OCCT wasm CMake config directory.")

if(OCCT_DIR)
  set(OpenCASCADE_DIR "${OCCT_DIR}" CACHE PATH "OpenCASCADE CMake config dir" FORCE)
  # OCCT 8.0+ has a circular dependency (Visualization ↔ DataExchange) that
  # causes OpenCASCADE_FOUND=FALSE even when all targets are correctly imported.
  # Load all modules at once as a workaround, then validate targets individually.
  find_package(OpenCASCADE CONFIG QUIET
    OPTIONAL_COMPONENTS
      FoundationClasses ModelingData ModelingAlgorithms
      Visualization ApplicationFramework DataExchange)
  if(NOT OpenCASCADE_FOUND AND NOT OpenCASCADE_VERSION)
    message(FATAL_ERROR
      "OCCT wasm installation not found at ${OCCT_DIR}. "
      "Set OCCT_DIR to the directory containing OpenCASCADEConfig.cmake.")
  endif()
else()
  message(WARNING
    "OCCT_DIR not set; OCCT wasm toolkits will not be linked. "
    "Pass -DOCCT_DIR=/path/to/OCCT/install-wasm/lib/cmake/opencascade")
endif()

if(NOT OCCTL_ALL_LIB)
  message(WARNING
    "OCCTL_ALL_LIB not set; OCCT-Light wasm static library will not be linked. "
    "Build OCCT-Light for wasm first (emcmake cmake --preset wasm -DOCCT_DIR=...), "
    "then pass -DOCCTL_ALL_LIB=/path/to/build/wasm/lib/libocctl.a")
endif()

# Note: the OCCT-Light feature-set static library is expected to be built separately
# for the wasm target via the 'wasm' CMake preset, then linked here. Its symbols
# resolve against the OCCT wasm toolkits found above.
# This native CMakeLists does NOT add_subdirectory(OCCT-Light) — that would
# attempt to compile OCCT under Emscripten, which is a multi-hour build
# delegated to `scripts/setup.sh`.

# Apply Emscripten-specific flags (defined in ../emscripten.cmake).
occtl_wasm_apply_flags(occtl_wasm)

# Link OCCT-Light wasm static library (contains all occtl_* symbols).
if(OCCTL_ALL_LIB)
  if(NOT EXISTS "${OCCTL_ALL_LIB}")
    message(FATAL_ERROR "OCCTL_ALL_LIB=${OCCTL_ALL_LIB} does not exist.")
  endif()
  target_link_libraries(occtl_wasm PRIVATE "${OCCTL_ALL_LIB}")
  message(STATUS "Linking OCCT-Light wasm library: ${OCCTL_ALL_LIB}")
endif()

# Link OCCT wasm toolkits (resolve OCCT symbols from the wrapper).
if(OCCT_DIR)
  set(_wasm_occt_toolkits
    TKernel TKMath
    TKG2d TKG3d TKGeomBase TKBRep
    TKTopAlgo TKGeomAlgo TKPrim TKBool TKFillet TKFeat TKOffset
    TKMesh TKXMesh TKHLR TKShHealing
    TKService TKV3d TKOpenGles TKMeshVS
    TKDESTEP TKDEIGES TKDESTL TKDEVRML TKDEOBJ TKDEGLTF TKDEPLY
    TKDE TKXSBase TKRWMesh TKDECascade
    TKBin TKBinL TKXml TKXmlL TKBO TKStd TKStdL
    TKCDF TKCAF TKLCAF TKVCAF TKTObj TKBinTObj TKXmlTObj
    TKBinXCAF TKXmlXCAF TKXCAF TKExpress
  )
  foreach(_tk IN LISTS _wasm_occt_toolkits)
    if(TARGET ${_tk})
      target_link_libraries(occtl_wasm PRIVATE ${_tk})
    endif()
  endforeach()
  message(STATUS "Linking OCCT wasm toolkits from ${OCCT_DIR}")
endif()

# === Output staging ==============================================================================
# Emscripten emits occtl.js + occtl.wasm into the build tree; copy to dist/ so
# the npm package can publish from a stable path.
set(OCCTL_WASM_DIST_DIR "${_wasm_dir}/dist")
file(MAKE_DIRECTORY "${OCCTL_WASM_DIST_DIR}")

add_custom_command(TARGET occtl_wasm POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:occtl_wasm>/occtl.js"
    "${OCCTL_WASM_DIST_DIR}/occtl.js"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:occtl_wasm>/occtl.wasm"
    "${OCCTL_WASM_DIST_DIR}/occtl.wasm"
  COMMENT "Staging occtl.js + occtl.wasm into bindings/wasm/dist/"
  VERBATIM
)

message(STATUS "OCCT-Light WASM binding configured (Emscripten ${CMAKE_C_COMPILER_VERSION})")
