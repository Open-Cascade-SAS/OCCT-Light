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
  message(STATUS "java binding: OCCTL_SHARED_LIBS is OFF; integration disabled")
  return()
endif()

find_program(_java_python NAMES python3)
if(NOT _java_python)
  message(STATUS "java binding: python3 not found; cannot run generator")
  return()
endif()

find_program(_mvn_exe NAMES mvn)
if(NOT _mvn_exe)
  message(STATUS "java binding: mvn not found; integration disabled")
  return()
endif()

set(_BINDING_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(_ABI_JSON "${CMAKE_BINARY_DIR}/abi.json")
set(_GENERATOR "${_BINDING_ROOT}/tools/generate_facade.py")

add_custom_target(occtl-java-regenerate
  COMMAND "${_java_python}" "${_GENERATOR}" --abi "${_ABI_JSON}"
  WORKING_DIRECTORY "${_BINDING_ROOT}"
  DEPENDS occtl-abi-dump
  COMMENT "java: regenerating generated JNA raw layer from abi.json"
  VERBATIM
)

add_custom_target(occtl-java-test
  COMMAND ${CMAKE_COMMAND} -E env
            OCCTL_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib
            OCCTL_LIBRARY_NAME=${OCCTL_LIBRARY_OUTPUT_NAME}
            OCCTL_ABI_JSON=${_ABI_JSON}
            "${_mvn_exe}" -q test
  WORKING_DIRECTORY "${_BINDING_ROOT}"
  DEPENDS occtl-java-regenerate
  COMMENT "java: running Maven tests"
  USES_TERMINAL
)

message(STATUS "java binding: integration ready (targets: occtl-java-regenerate, occtl-java-test)")
