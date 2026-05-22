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

add_executable(OCCTL_TestDe
  test_de.cpp
)

occtl_apply_compile_options(OCCTL_TestDe)

target_link_libraries(OCCTL_TestDe
  PRIVATE
    OCCTL::de
    OCCTL::topo
    OCCTL::prim
    GTest::gtest_main
)
if(TARGET occtl-io_brep)
  target_link_libraries(OCCTL_TestDe PRIVATE OCCTL::io_brep)
endif()
if(TARGET occtl-io_step)
  target_link_libraries(OCCTL_TestDe PRIVATE OCCTL::io_step)
endif()
if(TARGET occtl-io_iges)
  target_link_libraries(OCCTL_TestDe PRIVATE OCCTL::io_iges)
endif()
if(TARGET occtl-io_stl)
  target_link_libraries(OCCTL_TestDe PRIVATE OCCTL::io_stl)
endif()
if(TARGET occtl-io_obj)
  target_link_libraries(OCCTL_TestDe PRIVATE OCCTL::io_obj)
endif()
if(TARGET occtl-io_gltf)
  target_link_libraries(OCCTL_TestDe PRIVATE OCCTL::io_gltf)
endif()
if(TARGET occtl-io_vrml)
  target_link_libraries(OCCTL_TestDe PRIVATE OCCTL::io_vrml)
endif()
if(TARGET occtl-io_ply)
  target_link_libraries(OCCTL_TestDe PRIVATE OCCTL::io_ply)
endif()

occtl_gtest_discover_tests(OCCTL_TestDe
  PROPERTIES LABELS de
)
