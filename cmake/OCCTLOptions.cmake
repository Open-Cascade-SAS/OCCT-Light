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

include(OCCTLRegistry)

option(OCCTL_SHARED_LIBS  "Build shared libraries (.so/.dll) instead of static" OFF)

occtl_registry_define_options()

# Language bindings. Each binding lives under bindings/<lang>/ and follows the uniform
# recipe in docs/design/BINDINGS.md. All default OFF — the foundation CMake is unaware of
# any binding-specific toolchain (Python, .NET, Node, Emscripten) unless the user asks.
option(OCCTL_BUILD_BINDINGS_PYTHON "Build the Python binding"                                   OFF)
option(OCCTL_BUILD_BINDINGS_CSHARP "Build the C# binding"                                       OFF)
option(OCCTL_BUILD_BINDINGS_NODE   "Build the Node N-API binding"                               OFF)
option(OCCTL_BUILD_BINDINGS_WASM   "Build the WASM binding"                                     OFF)
option(OCCTL_BUILD_BINDINGS_RUST   "Build the Rust binding (cargo workspace)"                   OFF)
option(OCCTL_BUILD_BINDINGS_GO     "Build the Go binding (cgo module)"                          OFF)
option(OCCTL_BUILD_BINDINGS_JAVA   "Build the Java binding (JNA + Maven)"                       OFF)

if((OCCTL_BUILD_BINDINGS_PYTHON OR OCCTL_BUILD_BINDINGS_CSHARP
    OR OCCTL_BUILD_BINDINGS_NODE OR OCCTL_BUILD_BINDINGS_WASM
    OR OCCTL_BUILD_BINDINGS_RUST OR OCCTL_BUILD_BINDINGS_GO
    OR OCCTL_BUILD_BINDINGS_JAVA) AND NOT OCCTL_SHARED_LIBS)
  message(STATUS "OCCTL_SHARED_LIBS forced ON because enabled bindings require a runtime shared library.")
  set(OCCTL_SHARED_LIBS ON CACHE BOOL "Build shared libraries (.so/.dll) instead of static" FORCE)
endif()

# Tooling.
option(OCCTL_BUILD_TESTING        "Build the gtest test suites"                                 ON)
option(OCCTL_BUILD_DOCS           "Build Doxygen API docs"                                      OFF)
option(OCCTL_WARNINGS_AS_ERRORS   "Treat compiler warnings as errors"                           OFF)
option(OCCTL_ENFORCE_HEADER_HYGIENE "Run the public-header grep CI check at configure time"     ON)

# Mark advanced options that most users do not need to touch.
mark_as_advanced(
  OCCTL_ENFORCE_HEADER_HYGIENE
  OCCTL_WARNINGS_AS_ERRORS
)
