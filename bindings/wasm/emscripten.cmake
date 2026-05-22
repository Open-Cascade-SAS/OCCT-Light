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

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "bindings/wasm/emscripten.cmake must only be loaded under emcmake")
endif()

# Optimisation: -O3 for release; -O1 with -g3 for debug.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(OCCTL_WASM_OPT_FLAGS "-O1" "-g3")
else()
  set(OCCTL_WASM_OPT_FLAGS "-O3")
endif()

# Embind requires C++17 plus the bind header; -fexceptions enables C++ exception
# handling (Emscripten JavaScript-based EH, not wasm-native EH).
# CascadeScope-tested: -fwasm-exceptions causes emscripten_longjmp mismatches
# with OCCT libraries compiled without wasm-native EH.
set(OCCTL_WASM_COMPILE_FLAGS
  ${OCCTL_WASM_OPT_FLAGS}
  "-fexceptions"
  "-ffunction-sections"
  "-fdata-sections"
)

# Link-time flags. The big knobs:
#   --bind                       — enable Embind
#   ALLOW_MEMORY_GROWTH=1        — heap grows on demand; HEAPF64 views invalidate (see lib/views.ts)
#   INITIAL_MEMORY=64MB          — OCCT loads a lot of static state
#   MAXIMUM_MEMORY=2GB           — cap; large CAD models can blow this
#   MODULARIZE=1 / EXPORT_ES6=1  — emit an ES module factory: `await Module()`
#   EXPORT_NAME=OcctlModule      — the factory's export name
#   ENVIRONMENT=web,worker,node  — multi-target
#   EXPORT_EXCEPTION_HANDLING_HELPERS=1 — required by Embind's exception bridge
#   ASSERTIONS=1                 — keep for now; flip to 0 in production builds
set(OCCTL_WASM_LINK_FLAGS
  ${OCCTL_WASM_OPT_FLAGS}
  "--bind"
  "-fexceptions"
  "-Wl,--gc-sections"
  "-sMODULARIZE=1"
  "-sEXPORT_ES6=1"
  "-sEXPORT_NAME=OcctlModule"
  "-sENVIRONMENT=web,worker,node"
  "-sALLOW_MEMORY_GROWTH=1"
  "-sINITIAL_MEMORY=67108864"
  "-sMAXIMUM_MEMORY=2147483648"
  "-sEXPORTED_RUNTIME_METHODS=['HEAPU8','HEAPU32','HEAPF64','HEAPF32','UTF8ToString','stringToUTF8','lengthBytesUTF8','getValue','setValue','addOnPostRun','getExceptionMessage','decrementExceptionRefcount']"
  "-sEXPORTED_FUNCTIONS=['_malloc','_free','_occtl_runtime_abi_version','_occtl_runtime_init','_occtl_runtime_shutdown']"
  "-sSTACK_SIZE=5242880"
  "-sUSE_ZLIB=1"
)

# Convenience function: applies the WASM flags to a target.
function(occtl_wasm_apply_flags target)
  target_compile_options(${target} PRIVATE ${OCCTL_WASM_COMPILE_FLAGS})
  target_link_options(${target} PRIVATE ${OCCTL_WASM_LINK_FLAGS})
  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "occtl"
    SUFFIX ".js"
  )
endfunction()
