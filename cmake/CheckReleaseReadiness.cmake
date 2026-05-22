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

include_guard(GLOBAL)

function(occtl_add_release_readiness_checks)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(NOT Python3_Interpreter_FOUND)
    message(WARNING "release-readiness checks disabled: Python3 interpreter not found")
    return()
  endif()

  set(aHeaderStyleCmd
    "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/check_public_header_style.py"
    "${CMAKE_SOURCE_DIR}/include/occtl"
    "${CMAKE_SOURCE_DIR}/include/occtl-hpp"
  )
  set(aGuardAuditCmd
    "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/audit_guards.py"
    "${CMAKE_SOURCE_DIR}/src"
  )

  execute_process(
    COMMAND ${aHeaderStyleCmd}
    RESULT_VARIABLE aHeaderStyleResult
  )
  if(NOT aHeaderStyleResult EQUAL 0)
    message(FATAL_ERROR "Public-header style check failed")
  endif()

  execute_process(
    COMMAND ${aGuardAuditCmd}
    RESULT_VARIABLE aGuardAuditResult
  )
  if(NOT aGuardAuditResult EQUAL 0)
    message(FATAL_ERROR "Guard audit failed")
  endif()

  add_custom_target(occtl-check-header-style
    COMMAND ${aHeaderStyleCmd}
    COMMENT "Running OCCT-Light public-header style check"
    VERBATIM
  )
  add_custom_target(occtl-check-guards
    COMMAND ${aGuardAuditCmd}
    COMMENT "Running OCCT-Light Guard audit"
    VERBATIM
  )
endfunction()
