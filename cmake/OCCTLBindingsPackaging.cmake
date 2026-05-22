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

set(OCCTL_BINDINGS_PACKAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/binding-packages"
    CACHE PATH "Output directory for binding package artifacts.")

set(OCCTL_BINDINGS_PACKAGE_TARGETS "all"
    CACHE STRING "Targets passed to build_binding_packages.py --targets.")

function(occtl_configure_bindings_packaging)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(NOT Python3_Interpreter_FOUND)
    message(STATUS "bindings-packaging: Python3 not found; packaging targets disabled.")
    return()
  endif()

  set(_packaging_script "${CMAKE_SOURCE_DIR}/tools/scripts/build_binding_packages.py")
  if(NOT EXISTS "${_packaging_script}")
    message(STATUS "bindings-packaging: packaging script not found; targets disabled.")
    return()
  endif()

  add_custom_target(occtl-bindings-packages
    COMMAND "${Python3_EXECUTABLE}" "${_packaging_script}"
            --repo-root "${CMAKE_SOURCE_DIR}"
            --output-dir "${OCCTL_BINDINGS_PACKAGE_OUTPUT_DIR}"
            --targets "${OCCTL_BINDINGS_PACKAGE_TARGETS}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Building binding package artifacts via tools/scripts/build_binding_packages.py"
    USES_TERMINAL
    VERBATIM
  )

  add_custom_target(occtl-bindings-packages-dry-run
    COMMAND "${Python3_EXECUTABLE}" "${_packaging_script}"
            --repo-root "${CMAKE_SOURCE_DIR}"
            --output-dir "${OCCTL_BINDINGS_PACKAGE_OUTPUT_DIR}"
            --targets "${OCCTL_BINDINGS_PACKAGE_TARGETS}"
            --dry-run
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Printing binding packaging command plan (dry-run)"
    USES_TERMINAL
    VERBATIM
  )
endfunction()
