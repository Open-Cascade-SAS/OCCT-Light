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

find_program(_go_exe NAMES go HINTS "/opt/homebrew/bin" "/usr/local/go/bin")
if(NOT _go_exe)
  message(STATUS "go binding: go not found; integration disabled")
  return()
endif()
message(STATUS "go binding: go at ${_go_exe}")

add_custom_target(occtl-go-build
  COMMAND "${_go_exe}" build ./...
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Building Go module (src/occtl + tests/parity_runner)"
  USES_TERMINAL
)
