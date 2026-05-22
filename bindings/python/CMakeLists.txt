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
    message(STATUS "occtl-python: OCCTL_SHARED_LIBS is OFF; skipping binding wiring.")
    return()
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)
if(NOT Python3_FOUND)
    message(STATUS "occtl-python: Python 3 not found; skipping binding wiring.")
    return()
endif()

set(_BINDING_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(_ABI_JSON     "${CMAKE_BINARY_DIR}/abi.json")
set(_GENERATOR    "${_BINDING_ROOT}/tools/generate_facade.py")

# Convenience target: regenerate the raw layer + auto-wrappers from abi.json.
add_custom_target(occtl-python-regenerate
    COMMAND ${Python3_EXECUTABLE} ${_GENERATOR}
            --abi-json ${_ABI_JSON}
            --package-root ${_BINDING_ROOT}/src/occtl
    WORKING_DIRECTORY ${_BINDING_ROOT}
    DEPENDS occtl-abi-dump
    COMMENT "occtl-python: regenerating _raw.py / _abi.py / generated wrappers"
    VERBATIM
)

# Build a wheel into ${CMAKE_BINARY_DIR}/python-wheel/ for downstream pickup.
add_custom_target(occtl-python-wheel
    COMMAND ${Python3_EXECUTABLE} -m build --wheel
            --outdir ${CMAKE_BINARY_DIR}/python-wheel
    WORKING_DIRECTORY ${_BINDING_ROOT}
    COMMENT "occtl-python: building wheel"
    VERBATIM
)

message(STATUS "occtl-python: integration ready (run target 'occtl-python-regenerate' to rebuild facade).")
