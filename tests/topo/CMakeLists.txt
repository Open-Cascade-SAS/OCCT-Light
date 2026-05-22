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

add_executable(OCCTL_TestTopo
  test_graph_lifecycle.cpp
  test_graph_ids.cpp
  test_graph_counts.cpp
  test_graph_compound.cpp
  test_graph_geom.cpp
  test_graph_iterators.cpp
  test_graph_visitors.cpp
  test_graph_views.cpp
  test_graph_algo.cpp
  test_graph_mutation.cpp
  test_graph_batch.cpp
  test_graph_explorers.cpp
  test_graph_related.cpp
  test_graph_select.cpp
  test_graph_cache.cpp
  test_graph_inplace.cpp
  test_graph_compose.cpp
  test_graph_readgaps.cpp
  test_graph_assembly.cpp
  test_graph_layers.cpp
  test_graph_veneer.cpp
)

occtl_apply_compile_options(OCCTL_TestTopo)

target_link_libraries(OCCTL_TestTopo
  PRIVATE
    OCCTL::prim
    OCCTL::topo
    GTest::gtest_main
    TKPrim
)

occtl_gtest_discover_tests(OCCTL_TestTopo
  PROPERTIES LABELS topo
)
