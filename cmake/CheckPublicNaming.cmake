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
# this cmake/ folder.
set(_OCCTL_NAMING_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Forbidden tokens inside `extern "C"` blocks. Same poor-man's \b trick as
# CheckHeaderHygiene.cmake; CMake's regex engine has no \b.
set(_OCCTL_NAMING_FORBIDDEN_PATTERNS
  "(^|[^A-Za-z0-9_])bool([^A-Za-z0-9_]|$)"
  "(^|[^A-Za-z0-9_])_Bool([^A-Za-z0-9_]|$)"
  "(^|[^A-Za-z0-9_])std::"
  "(^|[^A-Za-z0-9_])TopoDS_[A-Za-z]"
  "(^|[^A-Za-z0-9_])Standard_[A-Za-z]"
  "(^|[^A-Za-z0-9_])NCollection_[A-Za-z]"
  "(^|[^A-Za-z0-9_])Handle[ \t]*\\("
)

set(_OCCTL_NAMING_FORBIDDEN_DESCRIPTIONS
  "'bool' inside extern \"C\" (use int32_t per ABI_PATTERNS.md §13)"
  "'_Bool' inside extern \"C\" (use int32_t per ABI_PATTERNS.md §13)"
  "'std::' identifier inside extern \"C\""
  "'TopoDS_*' identifier inside extern \"C\""
  "'Standard_*' identifier inside extern \"C\""
  "'NCollection_*' identifier inside extern \"C\""
  "'Handle()' macro inside extern \"C\""
)

# === _occtl_naming_strip_comments =================================================================
# Replace every C block comment and C++ line comment in theContent with
# whitespace, preserving line count so reported line numbers stay correct.
# (Identical logic to CheckHeaderHygiene.cmake; kept local so the modules
# remain independently usable.)
function(_occtl_naming_strip_comments theContent theOutVar)
  set(aText "${theContent}")
  set(aChanged 1)
  while(aChanged)
    string(REGEX REPLACE "/\\*([^*]|\\*+[^*/])*\\*+/" " " aNew "${aText}")
    if(aNew STREQUAL aText)
      set(aChanged 0)
    else()
      set(aText "${aNew}")
    endif()
  endwhile()
  string(REGEX REPLACE "//[^\n]*" "" aText "${aText}")
  set(${theOutVar} "${aText}" PARENT_SCOPE)
endfunction()

# === _occtl_naming_split_lines ====================================================================
# Split theText into a CMake list, one element per source line. Caller
# escapes ';' first so embedded semicolons in source don't get eaten.
function(_occtl_naming_split_lines theText theOutVar)
  set(aText "${theText}")
  string(REPLACE ";" "\\;" aText "${aText}")
  string(REPLACE "\n" ";" aLines "${aText}")
  set(${theOutVar} "${aLines}" PARENT_SCOPE)
endfunction()

# === _occtl_naming_check_function =================================================================
# Rule 1: function names. Match `OCCTL_API ... OCCTL_CALL <name>(`.
function(_occtl_naming_check_function theLine theLineNo theRel theOutVar)
  if(NOT theLine MATCHES "OCCTL_API")
    return()
  endif()
  if(NOT theLine MATCHES "OCCTL_CALL[ \t]+([A-Za-z_][A-Za-z0-9_]*)[ \t]*\\(")
    return()
  endif()
  set(aName "${CMAKE_MATCH_1}")
  if(NOT aName MATCHES "^occtl_[a-z][a-z0-9_]*$")
    set(aLocal "${${theOutVar}}")
    string(STRIP "${theLine}" aTrim)
    list(APPEND aLocal
      "  ${theRel}:${theLineNo}: error: function name '${aName}' violates ABI naming (expected occtl_<lower_snake>)\n        | ${aTrim}")
    set(${theOutVar} "${aLocal}" PARENT_SCOPE)
  endif()
endfunction()

# === _occtl_naming_check_typedef ==================================================================
# Rule 2: type names. Match the trailing identifier of a `typedef ... <name>;`.
# Skips opaque forward declarations of the form `typedef struct occtl_X occtl_X_t;`
# (those are valid; the tag and the typedef name differ only by the `_t` suffix).
function(_occtl_naming_check_typedef theLine theLineNo theRel theOutVar)
  if(NOT theLine MATCHES "^[ \t]*typedef[ \t]")
    return()
  endif()
  # Ignore enum/struct/union openers — those are multi-line and handled separately.
  if(theLine MATCHES "^[ \t]*typedef[ \t]+(enum|struct|union)[ \t]*[A-Za-z_]*[ \t]*\\{")
    return()
  endif()
  # Capture the last identifier before the closing semicolon.
  if(NOT theLine MATCHES "([A-Za-z_][A-Za-z0-9_]*)[ \t]*;[ \t]*$")
    return()
  endif()
  set(aName "${CMAKE_MATCH_1}")
  # Only police occtl_-prefixed names; foreign typedefs are out of scope.
  if(NOT aName MATCHES "^occtl_")
    return()
  endif()
  if(NOT aName MATCHES "^occtl_[a-z][a-z0-9_]*_t$")
    set(aLocal "${${theOutVar}}")
    string(STRIP "${theLine}" aTrim)
    list(APPEND aLocal
      "  ${theRel}:${theLineNo}: error: typedef name '${aName}' violates ABI naming (expected occtl_<lower_snake>_t)\n        | ${aTrim}")
    set(${theOutVar} "${aLocal}" PARENT_SCOPE)
  endif()
endfunction()

# === _occtl_naming_check_macro ====================================================================
# Rule 3: macros. `#define OCCTL_<NAME>` must be SCREAMING_SNAKE.
function(_occtl_naming_check_macro theLine theLineNo theRel theOutVar)
  if(NOT theLine MATCHES "^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z0-9_]*)")
    return()
  endif()
  set(aName "${CMAKE_MATCH_1}")
  if(NOT aName MATCHES "^OCCTL")
    return()
  endif()
  if(NOT aName MATCHES "^OCCTL_[A-Z][A-Z0-9_]*$")
    # Permit a bare `OCCTL` (no underscore) only if uppercase — but per rule
    # we require an underscore-prefix form, so flag bare-name too.
    set(aLocal "${${theOutVar}}")
    string(STRIP "${theLine}" aTrim)
    list(APPEND aLocal
      "  ${theRel}:${theLineNo}: error: macro '${aName}' violates ABI naming (expected OCCTL_<UPPER_SNAKE>)\n        | ${aTrim}")
    set(${theOutVar} "${aLocal}" PARENT_SCOPE)
  endif()
endfunction()

# === _occtl_naming_check_enums ====================================================================
# Rule 4: enum sentinels. For every `typedef enum [tag] { ... } occtl_<…>_t;`,
# the body must contain `RESERVED_FUTURE = 0x7fffffff`.
#
# Walks the comment-stripped text linearly: when it sees a typedef-enum
# opener, it accumulates lines until the matching `} occtl_*_t;` and then
# inspects the accumulated body.
function(_occtl_naming_check_enums theClean theRel theOutVar)
  _occtl_naming_split_lines("${theClean}" aLines)
  set(aLocal "${${theOutVar}}")

  set(aInEnum 0)
  set(aBody "")
  set(aOpenLineNo 0)
  set(aLineNo 0)
  foreach(aLine IN LISTS aLines)
    math(EXPR aLineNo "${aLineNo} + 1")
    if(NOT aInEnum)
      if(aLine MATCHES "^[ \t]*typedef[ \t]+enum[ \t]*[A-Za-z_0-9]*[ \t]*\\{")
        set(aInEnum 1)
        set(aOpenLineNo ${aLineNo})
        set(aBody "${aLine}")
      endif()
    else()
      set(aBody "${aBody}\n${aLine}")
      if(aLine MATCHES "\\}[ \t]*([A-Za-z_][A-Za-z0-9_]*)[ \t]*;")
        set(aTypedefName "${CMAKE_MATCH_1}")
        # Only police occtl_*_t-named enums.
        if(aTypedefName MATCHES "^occtl_.*_t$")
          if(NOT aBody MATCHES "RESERVED_FUTURE[ \t]*=[ \t]*0x7fffffff")
            list(APPEND aLocal
              "  ${theRel}:${aOpenLineNo}: error: enum '${aTypedefName}' is missing the 'RESERVED_FUTURE = 0x7fffffff' sentinel (ABI_PATTERNS.md §14)")
          endif()
        endif()
        set(aInEnum 0)
        set(aBody "")
      endif()
    endif()
  endforeach()

  set(${theOutVar} "${aLocal}" PARENT_SCOPE)
endfunction()

# === _occtl_naming_check_extern_c =================================================================
# Rule 5: forbidden tokens inside `extern "C"` blocks. Tracks brace depth
# from the opening `extern "C" {` to its matching `}`.
function(_occtl_naming_check_extern_c theClean theRel theOutVar)
  _occtl_naming_split_lines("${theClean}" aLines)
  set(aLocal "${${theOutVar}}")

  list(LENGTH _OCCTL_NAMING_FORBIDDEN_PATTERNS aN)
  math(EXPR aMax "${aN} - 1")

  set(aInExternC 0)
  set(aDepth 0)
  set(aLineNo 0)
  foreach(aLine IN LISTS aLines)
    math(EXPR aLineNo "${aLineNo} + 1")

    if(NOT aInExternC)
      if(aLine MATCHES "extern[ \t]+\"C\"")
        # The opening brace may be on this same line or a subsequent one.
        # Either way, we'll start counting once we see the first `{`.
        set(aInExternC 1)
        set(aDepth 0)
        # Fall through to brace counting on this line.
      else()
        continue()
      endif()
    endif()

    # Count braces on this line. Strip string literals and char literals
    # to avoid miscounting; public headers don't have many of these but
    # be defensive.
    set(aStripped "${aLine}")
    string(REGEX REPLACE "\"([^\"\\]|\\\\.)*\"" "" aStripped "${aStripped}")
    string(REGEX REPLACE "'([^'\\]|\\\\.)*'" "" aStripped "${aStripped}")

    string(REGEX MATCHALL "\\{" aOpens "${aStripped}")
    string(REGEX MATCHALL "\\}" aCloses "${aStripped}")
    list(LENGTH aOpens aOpenN)
    list(LENGTH aCloses aCloseN)

    # Scan the line for forbidden tokens only after we are inside braces
    # (i.e. past the opening `{` of the extern "C"). We approximate this
    # by requiring aDepth >= 1 OR the opening `{` to have appeared on
    # this line; in either case, the forbidden-token scan applies.
    if(aDepth GREATER 0 OR aOpenN GREATER 0)
      foreach(aIdx RANGE ${aMax})
        list(GET _OCCTL_NAMING_FORBIDDEN_PATTERNS    ${aIdx} aPat)
        list(GET _OCCTL_NAMING_FORBIDDEN_DESCRIPTIONS ${aIdx} aDesc)
        if(aLine MATCHES "${aPat}")
          string(STRIP "${aLine}" aTrim)
          list(APPEND aLocal
            "  ${theRel}:${aLineNo}: error: ${aDesc}\n        | ${aTrim}")
        endif()
      endforeach()
    endif()

    math(EXPR aDepth "${aDepth} + ${aOpenN} - ${aCloseN}")
    if(aDepth LESS_EQUAL 0 AND aOpenN GREATER 0)
      # We saw the opening `{` and immediately closed; only possible if
      # depth went 0->1->0 on the same line. Reset.
      set(aInExternC 0)
      set(aDepth 0)
    elseif(aDepth LESS_EQUAL 0 AND aOpenN EQUAL 0 AND aInExternC EQUAL 1)
      # Only exit when we've actually entered braces at least once.
      # If we never saw `{` yet, keep aInExternC=1 and wait.
      # Heuristic: if aDepth dropped to 0 from a positive value, we're out.
      # Implemented by tracking whether we've ever been inside.
    endif()

    if(aInExternC EQUAL 1 AND aDepth EQUAL 0 AND aOpenN GREATER 0)
      # Already handled above.
      set(aInExternC 0)
    endif()

    if(aInExternC EQUAL 1 AND aDepth EQUAL 0 AND aCloseN GREATER 0)
      # Closed back to zero on this or a later line.
      set(aInExternC 0)
    endif()
  endforeach()

  set(${theOutVar} "${aLocal}" PARENT_SCOPE)
endfunction()

# === _occtl_naming_scan_file ======================================================================
# Scan one file: strip comments, then dispatch each rule.
function(_occtl_naming_scan_file theAbs theRel theOutVar)
  file(READ "${theAbs}" aRaw)
  _occtl_naming_strip_comments("${aRaw}" aClean)

  set(aLocal "${${theOutVar}}")

  _occtl_naming_split_lines("${aClean}" aLines)
  set(aLineNo 0)
  foreach(aLine IN LISTS aLines)
    math(EXPR aLineNo "${aLineNo} + 1")
    _occtl_naming_check_function("${aLine}" ${aLineNo} "${theRel}" aLocal)
    _occtl_naming_check_typedef("${aLine}"  ${aLineNo} "${theRel}" aLocal)
    _occtl_naming_check_macro("${aLine}"    ${aLineNo} "${theRel}" aLocal)
  endforeach()

  _occtl_naming_check_enums("${aClean}"   "${theRel}" aLocal)
  _occtl_naming_check_extern_c("${aClean}" "${theRel}" aLocal)

  set(${theOutVar} "${aLocal}" PARENT_SCOPE)
endfunction()

# === occtl_check_public_naming ====================================================================
# Public entry. Scans every .h under theDir and fails the configure on hit.
function(occtl_check_public_naming theDir)
  if(NOT IS_DIRECTORY "${theDir}")
    message(FATAL_ERROR "occtl_check_public_naming: ${theDir} is not a directory")
  endif()

  file(GLOB_RECURSE aHeaders RELATIVE "${theDir}" "${theDir}/*.h")
  if(NOT aHeaders)
    message(WARNING "occtl_check_public_naming: no .h files found under ${theDir}")
    return()
  endif()

  set(aViolations "")
  foreach(aRel IN LISTS aHeaders)
    _occtl_naming_scan_file("${theDir}/${aRel}" "${aRel}" aViolations)
  endforeach()

  if(aViolations)
    string(REPLACE ";" "\n" aReport "${aViolations}")
    message(FATAL_ERROR
      "OCCT-Light public-naming check failed (ABI_PATTERNS.md §1, §3, §13, §14):\n"
      "${aReport}\n"
      "\n"
      "Public C names must be:\n"
      "  - functions     : occtl_<lower_snake>\n"
      "  - typedefs      : occtl_<lower_snake>_t\n"
      "  - macros        : OCCTL_<UPPER_SNAKE>\n"
      "  - enum sentinels: every occtl_*_t enum carries RESERVED_FUTURE = 0x7fffffff\n"
      "  - extern \"C\"  : no bool/_Bool/std::/TopoDS_/Standard_/NCollection_/Handle()\n"
      "\n"
      "Bypass during local debugging only:  -DOCCTL_ENFORCE_HEADER_HYGIENE=OFF\n"
    )
  endif()

  list(LENGTH aHeaders aCount)
  message(STATUS "Public-naming check: ${theDir} clean (${aCount} headers)")

  # Re-runnable target: lets developers verify after editing without
  # reconfiguring. Only valid in project mode, not `cmake -P` script mode.
  if(NOT CMAKE_SCRIPT_MODE_FILE AND NOT TARGET occtl_check_naming)
    add_custom_target(occtl_check_naming
      COMMAND ${CMAKE_COMMAND}
              -DOCCTL_NAMING_DIR=${theDir}
              -P ${_OCCTL_NAMING_CMAKE_DIR}/CheckPublicNamingDriver.cmake
      COMMENT "Running OCCT-Light public-naming check (ABI_PATTERNS.md §1, §3, §13, §14)"
      VERBATIM
    )
  endif()
endfunction()
