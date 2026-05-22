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

option(OCCTL_CSHARP_BUILD "Drive ``dotnet build`` for the C# binding." OFF)

if(NOT OCCTL_SHARED_LIBS)
  message(STATUS "csharp: OCCTL_SHARED_LIBS is OFF; skipping binding wiring.")
  return()
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)

if (NOT Python3_Interpreter_FOUND)
  message(STATUS "csharp: Python3 not found; skipping facade regeneration target.")
  return()
endif()

set(_csharp_dir "${CMAKE_CURRENT_SOURCE_DIR}")
set(_abi_json "${CMAKE_BINARY_DIR}/abi.json")

# Phase 0 dumps abi.json into <build>/abi.json. We don't enforce its existence at
# configure time — the generator regenerates it lazily, and a clean target chain
# is the developer's responsibility (see docs/design/BINDINGS.md §5.2).
add_custom_target(occtl-csharp-generate
  COMMAND "${Python3_EXECUTABLE}" "${_csharp_dir}/tools/generate_facade.py"
          --abi "${_abi_json}"
          --root "${_csharp_dir}"
          --library-name "${OCCTL_LIBRARY_OUTPUT_NAME}"
  DEPENDS occtl-abi-dump
  COMMENT "csharp: regenerating .g.cs facade from ${_abi_json}"
  VERBATIM
)

if (OCCTL_CSHARP_BUILD)
  find_program(DOTNET_EXECUTABLE dotnet)
  if (NOT DOTNET_EXECUTABLE)
    message(WARNING "csharp: OCCTL_CSHARP_BUILD=ON but `dotnet` was not found on PATH.")
    return()
  endif()
  add_custom_target(occtl-csharp-build
    COMMAND "${DOTNET_EXECUTABLE}" build "${_csharp_dir}/OcctL.sln" -c Release
    DEPENDS occtl-csharp-generate
    COMMENT "csharp: dotnet build OcctL.sln (Release)"
    VERBATIM
  )
  add_custom_target(occtl-csharp-pack
    COMMAND "${DOTNET_EXECUTABLE}" pack "${_csharp_dir}/src/api/OcctL.csproj"
            -c Release -o "${CMAKE_CURRENT_BINARY_DIR}/dist"
    DEPENDS occtl-csharp-build
    COMMENT "csharp: dotnet pack src/api/OcctL.csproj"
    VERBATIM
  )
endif()
