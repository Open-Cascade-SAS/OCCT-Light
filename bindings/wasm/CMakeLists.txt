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

if(NOT OCCTL_SHARED_LIBS)
  message(STATUS
    "wasm: OCCTL_SHARED_LIBS is OFF; the WASM binding's host introspection (abi.json) "
    "still requires the wrapper to be configured with shared libs. Skipping wasm wiring.")
  return()
endif()

find_program(NPM_EXECUTABLE NAMES npm)
find_program(EMCMAKE_EXECUTABLE NAMES emcmake)

if(NOT NPM_EXECUTABLE)
  message(STATUS "wasm: npm not found on PATH; skipping WASM binding orchestration targets.")
  return()
endif()

set(_wasm_dir "${CMAKE_CURRENT_SOURCE_DIR}")

# Regenerate src/native/raw.cc + src/ts/*.ts from build/abi.json.
add_custom_target(occtl-wasm-generate
  COMMAND ${NPM_EXECUTABLE} run generate --
          --abi "${CMAKE_BINARY_DIR}/abi.json"
  WORKING_DIRECTORY "${_wasm_dir}"
  DEPENDS occtl-abi-dump
  COMMENT "wasm: regenerating src/native/raw.cc + src/ts/*.ts from abi.json"
  VERBATIM
)

# Optional convenience target: drive the Emscripten build via emcmake. Only
# wired when the SDK is on PATH; otherwise the developer runs the commands
# in ./README.md manually.
if(EMCMAKE_EXECUTABLE)
  set(_wasm_build_dir "${CMAKE_BINARY_DIR}/bindings/wasm")
  set(_wasm_occt_dir_arg "")
  if(OCCT_DIR)
    set(_wasm_occt_dir_arg -DOCCT_DIR=${OCCT_DIR})
  endif()
  set(_wasm_occtl_lib_arg "")
  if(OCCTL_LIBRARY_OUTPUT_NAME)
    set(_wasm_occtl_lib "${CMAKE_BINARY_DIR}/lib/${OCCTL_LIBRARY_OUTPUT_NAME}.a")
    if(EXISTS "${_wasm_occtl_lib}")
      set(_wasm_occtl_lib_arg -DOCCTL_ALL_LIB=${_wasm_occtl_lib})
    endif()
  endif()
  add_custom_target(occtl-wasm-build
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_wasm_build_dir}"
    COMMAND ${EMCMAKE_EXECUTABLE} ${CMAKE_COMMAND}
            -S "${_wasm_dir}/src/native"
            -B "${_wasm_build_dir}"
            -DOCCTL_HEADERS_DIR=${CMAKE_SOURCE_DIR}/include
            ${_wasm_occt_dir_arg}
            ${_wasm_occtl_lib_arg}
    COMMAND ${CMAKE_COMMAND} --build "${_wasm_build_dir}"
    DEPENDS occtl-wasm-generate
    COMMENT "wasm: emcmake configure + build under ${_wasm_build_dir}"
    VERBATIM
  )
  message(STATUS "wasm: emcmake found; occtl-wasm-build target available.")
else()
  message(STATUS
    "wasm: emcmake not on PATH; occtl-wasm-build target not wired. "
    "Run bindings/wasm/scripts/setup.sh to provision emsdk, then source emsdk_env.sh.")
endif()
