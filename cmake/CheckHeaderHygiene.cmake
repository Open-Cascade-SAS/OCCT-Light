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

# Snapshot the directory at include() time. `CMAKE_CURRENT_LIST_DIR` inside
# the function body would resolve to the caller's list-file directory, not
# this cmake/ folder, so the driver path would be wrong.
set(_OCCTL_HYGIENE_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Forbidden tokens, as parallel lists. Index N in patterns and descriptions
# refer to the same rule. Anchors keep matches precise; ` (^|[^A-Za-z0-9_]) `
# is a poor-man's \b since CMake's regex engine has no \b.
set(_OCCTL_HYGIENE_PATTERNS
  "#[ \t]*include[ \t]+<string>"
  "#[ \t]*include[ \t]+<string_view>"
  "#[ \t]*include[ \t]+<vector>"
  "#[ \t]*include[ \t]+<memory>"
  "#[ \t]*include[ \t]+<map>"
  "#[ \t]*include[ \t]+<unordered_map>"
  "#[ \t]*include[ \t]+<set>"
  "#[ \t]*include[ \t]+<unordered_set>"
  "#[ \t]*include[ \t]+<list>"
  "#[ \t]*include[ \t]+<deque>"
  "#[ \t]*include[ \t]+<array>"
  "#[ \t]*include[ \t]+<algorithm>"
  "#[ \t]*include[ \t]+<iostream>"
  "#[ \t]*include[ \t]+<fstream>"
  "#[ \t]*include[ \t]+<sstream>"
  "#[ \t]*include[ \t]+<functional>"
  "#[ \t]*include[ \t]+<optional>"
  "#[ \t]*include[ \t]+<variant>"
  "#[ \t]*include[ \t]+<tuple>"
  "(^|[^A-Za-z0-9_])Standard_[A-Za-z]"
  "(^|[^A-Za-z0-9_])TopoDS_[A-Za-z]"
  "(^|[^A-Za-z0-9_])BRep[A-Z][A-Za-z_]"
  "(^|[^A-Za-z0-9_])NCollection_[A-Za-z]"
  "(^|[^A-Za-z0-9_])Geom_[A-Za-z]"
  "(^|[^A-Za-z0-9_])Geom2d_[A-Za-z]"
  "(^|[^A-Za-z0-9_])gp_[A-Za-z]"
  "(^|[^A-Za-z0-9_])Handle[ \t]*\\("
  "(^|[^A-Za-z0-9_])TCollection_[A-Za-z]"
  "(^|[^A-Za-z0-9_])TColStd_[A-Za-z]"
  "(^|[^A-Za-z0-9_])TColgp_[A-Za-z]"
)

set(_OCCTL_HYGIENE_DESCRIPTIONS
  "<string> include"
  "<string_view> include"
  "<vector> include"
  "<memory> include"
  "<map> include"
  "<unordered_map> include"
  "<set> include"
  "<unordered_set> include"
  "<list> include"
  "<deque> include"
  "<array> include"
  "<algorithm> include"
  "<iostream> include"
  "<fstream> include"
  "<sstream> include"
  "<functional> include"
  "<optional> include"
  "<variant> include"
  "<tuple> include"
  "Standard_* identifier"
  "TopoDS_* identifier"
  "BRep* identifier"
  "NCollection_* identifier"
  "Geom_* identifier"
  "Geom2d_* identifier"
  "gp_* identifier"
  "Handle() macro"
  "TCollection_* identifier"
  "TColStd_* identifier"
  "TColgp_* identifier"
)

# === _occtl_strip_comments ========================================================================
# Replace every C block comment and C++ line comment in theContent with
# whitespace, preserving line count so reported line numbers stay correct.
function(_occtl_strip_comments theContent theOutVar)
  set(aText "${theContent}")

  # Iteratively remove /* ... */ blocks. CMake's regex engine has no
  # non-greedy quantifier, so we use a tempered class to stop at the first
  # */, and loop because successive comments may overlap removal regions.
  set(aChanged 1)
  while(aChanged)
    string(REGEX REPLACE "/\\*([^*]|\\*+[^*/])*\\*+/" " " aNew "${aText}")
    if(aNew STREQUAL aText)
      set(aChanged 0)
    else()
      set(aText "${aNew}")
    endif()
  endwhile()

  # Drop // ... to end-of-line.
  string(REGEX REPLACE "//[^\n]*" "" aText "${aText}")

  set(${theOutVar} "${aText}" PARENT_SCOPE)
endfunction()

# === _occtl_scan_file =============================================================================
# Scan one file: strip comments, split into lines, test each line against
# every forbidden pattern, append "<rel>:<line>: <desc>\n        | <text>"
# strings to theOutVar (list of strings) in the parent scope.
function(_occtl_scan_file theAbs theRel theOutVar)
  file(READ "${theAbs}" aRaw)
  _occtl_strip_comments("${aRaw}" aClean)

  # Convert to a CMake list, one element per source line. Escape ';' so the
  # split-on-newline doesn't mishandle macro lists in the source.
  string(REPLACE ";" "\\;" aClean "${aClean}")
  string(REPLACE "\n" ";" aLines "${aClean}")

  list(LENGTH _OCCTL_HYGIENE_PATTERNS aN)
  math(EXPR aMax "${aN} - 1")

  set(aLocal "${${theOutVar}}")
  set(aLineNo 0)
  foreach(aLine IN LISTS aLines)
    math(EXPR aLineNo "${aLineNo} + 1")
    foreach(aIdx RANGE ${aMax})
      list(GET _OCCTL_HYGIENE_PATTERNS    ${aIdx} aPat)
      list(GET _OCCTL_HYGIENE_DESCRIPTIONS ${aIdx} aDesc)
      if(aLine MATCHES "${aPat}")
        string(STRIP "${aLine}" aTrim)
        list(APPEND aLocal
          "  ${theRel}:${aLineNo}: ${aDesc}\n        | ${aTrim}")
      endif()
    endforeach()
  endforeach()

  set(${theOutVar} "${aLocal}" PARENT_SCOPE)
endfunction()

# === occtl_check_header_hygiene ===================================================================
# Public entry. Scans every .h under theDir and fails the configure on hit.
#
# @param theDir  Absolute path to the directory to scan. Must exist and contain
#                one or more .h files. If the path does not point to a directory
#                the function FATAL_ERRORs; if it contains no .h files a WARNING
#                is emitted and the function returns immediately.
function(occtl_check_header_hygiene theDir)
  if(NOT IS_DIRECTORY "${theDir}")
    message(FATAL_ERROR "occtl_check_header_hygiene: ${theDir} is not a directory")
  endif()

  file(GLOB_RECURSE aHeaders RELATIVE "${theDir}" "${theDir}/*.h")
  if(NOT aHeaders)
    message(WARNING "occtl_check_header_hygiene: no .h files found under ${theDir}")
    return()
  endif()

  set(aViolations "")
  foreach(aRel IN LISTS aHeaders)
    _occtl_scan_file("${theDir}/${aRel}" "${aRel}" aViolations)
  endforeach()

  if(aViolations)
    string(REPLACE ";" "\n" aReport "${aViolations}")
    message(FATAL_ERROR
      "OCCT-Light public-header hygiene check failed (Rule 1 of AGENTS.md):\n"
      "${aReport}\n"
      "\n"
      "Public headers under ${theDir} may include only <stdint.h>, <stddef.h>,\n"
      "and other occtl_*.h files, and must not name any STL or OCCT type in code.\n"
      "(Doxygen comments are stripped before scanning, so doc references are fine.)\n"
      "\n"
      "Bypass during local debugging only:  -DOCCTL_ENFORCE_HEADER_HYGIENE=OFF\n"
    )
  endif()

  list(LENGTH aHeaders aCount)
  message(STATUS "Header hygiene: ${theDir} clean (${aCount} headers)")

  # Re-runnable target: lets developers verify after editing without
  # reconfiguring the whole project. Only valid in project mode, not
  # `cmake -P` script mode.
  if(NOT CMAKE_SCRIPT_MODE_FILE AND NOT TARGET occtl-check-headers)
    add_custom_target(occtl-check-headers
      COMMAND ${CMAKE_COMMAND}
              -DOCCTL_HYGIENE_DIR=${theDir}
              -P ${_OCCTL_HYGIENE_CMAKE_DIR}/CheckHeaderHygieneDriver.cmake
      COMMENT "Running OCCT-Light public-header hygiene check"
      VERBATIM
    )
  endif()
endfunction()
