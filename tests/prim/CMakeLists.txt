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

add_executable(OCCTL_TestPrim
  test_prim_box.cpp
  test_prim_sphere.cpp
  test_prim_cylinder.cpp
  test_prim_cone.cpp
  test_prim_torus.cpp
  test_prim_wedge.cpp
  test_prim_halfspace.cpp
  test_prim_prism.cpp
  test_prim_revol.cpp
  test_prim_pipe.cpp
  test_prim_loft.cpp
  test_prim_sketches.cpp
  test_prim_pipe_shell.cpp
  test_prim_offsets.cpp
  test_prim_arcs.cpp
  test_prim_draft_prism.cpp
  test_prim_twist_extrusion.cpp
  test_prim_ruled_surface.cpp
  test_prim_feat_prism.cpp
  test_prim_cylindrical_hole.cpp
  test_prim_spline.cpp
  test_prim_convenience.cpp
  test_prim_helix.cpp
  test_prim_fillet_2d.cpp
  test_prim_arc_ellipse.cpp
)

occtl_apply_compile_options(OCCTL_TestPrim)

target_link_libraries(OCCTL_TestPrim
  PRIVATE
    OCCTL::prim
    OCCTL::topo
    OCCTL::geom
    OCCTL::core
    GTest::gtest_main
)

occtl_gtest_discover_tests(OCCTL_TestPrim
  PROPERTIES LABELS prim
)
