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
  message(WARNING
    "OCCTL_BUILD_BINDINGS_NODE=ON requires OCCTL_SHARED_LIBS=ON. "
    "The Node addon links the selected libocctl-<feature-set> dynamically and cannot use static archives. "
    "Skipping Node binding configuration.")
  return()
endif()

find_program(NODE_EXECUTABLE NAMES node)
find_program(NPM_EXECUTABLE  NAMES npm)

if(NOT NODE_EXECUTABLE OR NOT NPM_EXECUTABLE)
  message(STATUS "node/npm not found on PATH; Node binding will not be configured (source files are still present in ${CMAKE_CURRENT_SOURCE_DIR}).")
  return()
endif()

message(STATUS "Node binding: ${NODE_EXECUTABLE} (npm: ${NPM_EXECUTABLE})")

# Optional convenience target: regenerate the facade and run npm install + build.
add_custom_target(occtl_bindings_node
  COMMAND ${CMAKE_COMMAND} -E env
          "OCCTL_LIBRARY_NAME=${OCCTL_LIBRARY_OUTPUT_NAME}"
          "OCCTL_LIBRARY_DIR=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
          ${NPM_EXECUTABLE} install
  COMMAND ${CMAKE_COMMAND} -E env
          "OCCTL_LIBRARY_NAME=${OCCTL_LIBRARY_OUTPUT_NAME}"
          "OCCTL_LIBRARY_DIR=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
          ${NPM_EXECUTABLE} run build
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS occtl-abi-dump
  COMMENT "Building OCCT-Light Node addon via npm")
