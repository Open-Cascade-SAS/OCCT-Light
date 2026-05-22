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

set(OCCTL_ABI_JSON "${CMAKE_BINARY_DIR}/abi.json"
    CACHE FILEPATH "Path to the generated abi.json snapshot." FORCE)

function(occtl_configure_abi_dump)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(NOT Python3_Interpreter_FOUND)
    message(STATUS "abi-dump: Python3 not found; occtl-abi-dump target disabled.")
    return()
  endif()

  get_property(aPublicHeaders GLOBAL PROPERTY OCCTL_PUBLIC_HEADERS)
  set(aAbiHeaders "")
  foreach(aHeader IN LISTS aPublicHeaders)
    if(aHeader MATCHES "/include/occtl/[^/]+\\.h$")
      list(APPEND aAbiHeaders "${aHeader}")
    endif()
  endforeach()

  if(NOT aAbiHeaders)
    message(FATAL_ERROR "occtl_configure_abi_dump(): no enabled C ABI headers registered")
  endif()
  list(REMOVE_DUPLICATES aAbiHeaders)

  add_custom_command(
    OUTPUT  "${OCCTL_ABI_JSON}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/abi_dump.py"
            --include-dir "${CMAKE_SOURCE_DIR}/include/occtl"
            --output      "${OCCTL_ABI_JSON}"
            --headers     ${aAbiHeaders}
    DEPENDS "${CMAKE_SOURCE_DIR}/tools/abi_dump.py"
            ${aAbiHeaders}
    COMMENT "Generating abi.json from enabled include/occtl headers"
    VERBATIM
  )

  add_custom_target(occtl-abi-dump DEPENDS "${OCCTL_ABI_JSON}")

  add_custom_target(occtl-check-exports
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/check_exports.py"
            --library "$<TARGET_FILE:occtl>"
            --headers ${aAbiHeaders}
    DEPENDS occtl
            "${CMAKE_SOURCE_DIR}/tools/check_exports.py"
            ${aAbiHeaders}
    COMMENT "Checking public C ABI symbols are exported"
    VERBATIM
  )
  add_test(NAME OCCTL_CheckExports
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/check_exports.py"
            --library "$<TARGET_FILE:occtl>"
            --headers ${aAbiHeaders}
  )
  set_tests_properties(OCCTL_CheckExports PROPERTIES LABELS "core")
endfunction()
