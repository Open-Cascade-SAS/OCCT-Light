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

# === Warning flags by compiler ====================================================================
if(MSVC)
  set(OCCTL_WARNING_FLAGS /W4 /permissive- /Zc:__cplusplus /utf-8 /wd4100 /wd4127)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  set(OCCTL_WARNING_FLAGS
    -Wall -Wextra -Wpedantic
    -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
    -Wunused -Woverloaded-virtual -Wconversion -Wsign-conversion
    -Wnull-dereference -Wdouble-promotion -Wformat=2
    -Wno-unused-parameter
  )
endif()

# === Definitions ==================================================================================
set(OCCTL_PUBLIC_DEFINITIONS
  OCCTL_VERSION_MAJOR=${OCCTL_VERSION_MAJOR}
  OCCTL_VERSION_MINOR=${OCCTL_VERSION_MINOR}
  OCCTL_VERSION_PATCH=${OCCTL_VERSION_PATCH}
  OCCTL_ABI_VERSION=${OCCTL_ABI_VERSION}
)

# When building a shared library: dllexport on the producing target; consumers (any target
# that links against an occtl-* shared library) see OCCTL_USE_SHARED so the public headers
# expand OCCTL_API to dllimport on Windows.
# When building a static library: OCCTL_STATIC_BUILD ensures OCCTL_API expands to nothing.
if(OCCTL_SHARED_LIBS)
  set(OCCTL_LIBRARY_KIND SHARED)
else()
  list(APPEND OCCTL_PUBLIC_DEFINITIONS OCCTL_STATIC_BUILD)
  set(OCCTL_LIBRARY_KIND STATIC)
endif()

# === Helper function ==============================================================================
function(occtl_apply_compile_options theTarget)
  target_compile_options(${theTarget} PRIVATE ${OCCTL_WARNING_FLAGS})
  target_compile_definitions(${theTarget}
    PUBLIC  ${OCCTL_PUBLIC_DEFINITIONS}
    PRIVATE ${OCCTL_PRIVATE_DEFINITIONS}
  )
  # Shared-library export-macro plumbing. The target itself defines OCCTL_BUILD_SHARED
  # (so OCCTL_API expands to dllexport on Windows); the INTERFACE side defines
  # OCCTL_USE_SHARED so any consumer linking it picks dllimport.
  #
  # Visibility: top-level CMakeLists sets CXX_VISIBILITY_PRESET=hidden globally for the
  # static build; OCCTLModule.cmake's occtl_add_module() flips that back to "default"
  # on shared builds because internal C++ helpers in core (OcctL::Core::Guard,
  # ErrorState) are linked across sister DSOs. The C ABI surface is still controlled
  # by OCCTL_API independent of the C++ default.
  if(OCCTL_SHARED_LIBS)
    target_compile_definitions(${theTarget}
      PRIVATE   OCCTL_BUILD_SHARED
      INTERFACE OCCTL_USE_SHARED
    )
  endif()
  if(OCCTL_WARNINGS_AS_ERRORS)
    if(MSVC)
      target_compile_options(${theTarget} PRIVATE /WX)
    else()
      target_compile_options(${theTarget} PRIVATE -Werror)
    endif()
  endif()
endfunction()
