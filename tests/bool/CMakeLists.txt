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

include(OCCTLCompileOptions)

add_executable(OCCTL_TestBool
  test_bool_fuse.cpp
  test_bool_cut.cpp
  test_bool_common.cpp
  test_bool_section.cpp
  test_bool_split.cpp
  test_bool_errors.cpp
  test_bool_history.cpp
  test_bool_veneer.cpp
)

occtl_apply_compile_options(OCCTL_TestBool)

target_link_libraries(OCCTL_TestBool
  PRIVATE
    OCCTL::bool
    OCCTL::prim
    OCCTL::geom
    OCCTL::topo
    OCCTL::core
    GTest::gtest_main
)

occtl_gtest_discover_tests(OCCTL_TestBool
  PROPERTIES LABELS bool
)
