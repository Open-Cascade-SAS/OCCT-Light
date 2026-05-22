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

find_program(_cargo_exe NAMES cargo HINTS "$ENV{HOME}/.cargo/bin")
if(NOT _cargo_exe)
  message(STATUS "rust binding: cargo not found; integration disabled (set PATH to ~/.cargo/bin)")
  return()
endif()
message(STATUS "rust binding: cargo at ${_cargo_exe}")

# Convenience target — `cmake --build --target occtl-rust-build` runs cargo.
add_custom_target(occtl-rust-build
  COMMAND ${CMAKE_COMMAND} -E env
            OCCTL_REPO_ROOT=${CMAKE_SOURCE_DIR}
            OCCTL_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib
          "${_cargo_exe}" build --manifest-path
            "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml" --workspace
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Building Rust workspace (src/occtl-sys + src/occtl + tests/parity_runner)"
  USES_TERMINAL
)
