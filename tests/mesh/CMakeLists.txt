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

add_executable(OCCTL_TestMesh
  test_mesh_options.cpp
  test_mesh_generate.cpp
  test_mesh_views.cpp
  test_mesh_veneer.cpp
)

occtl_apply_compile_options(OCCTL_TestMesh)

target_link_libraries(OCCTL_TestMesh
  PRIVATE
    OCCTL::mesh
    OCCTL::prim
    OCCTL::topo
    OCCTL::geom
    OCCTL::core
    GTest::gtest_main
)

# IMeshTools_Parameters is in TKMesh; tests inspect it directly to verify
# default-value parity between the public options struct and OCCT's internal
# parameter struct.
find_package(OCCT REQUIRED COMPONENTS TKMesh)
target_link_libraries(OCCTL_TestMesh PRIVATE TKMesh)

occtl_gtest_discover_tests(OCCTL_TestMesh
  PROPERTIES LABELS mesh
)
