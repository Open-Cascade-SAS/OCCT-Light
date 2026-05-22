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

set(_OCCTL_PARITY_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(_OCCTL_BINDINGS_DIR "${CMAKE_SOURCE_DIR}/bindings")
set(_OCCTL_BUILD_LIB_DIR "${CMAKE_BINARY_DIR}/lib")
set(_OCCTL_PARITY_READY OFF)
if(OCCTL_BUILD_TOPO AND OCCTL_BUILD_PRIM)
  set(_OCCTL_PARITY_READY ON)
endif()

# ---------------------------------------------------------------- Python ---
if(OCCTL_BUILD_BINDINGS_PYTHON AND _OCCTL_PARITY_READY)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_FOUND)
    set(_py_venv "${_OCCTL_BINDINGS_DIR}/python/.venv/bin/python")
    if(EXISTS "${_py_venv}")
      set(_py_exe "${_py_venv}")
    else()
      set(_py_exe "${Python3_EXECUTABLE}")
    endif()
    add_test(
      NAME parity.python.build_box
      COMMAND "${_py_exe}"
              "${_OCCTL_BINDINGS_DIR}/python/tests/parity/runner.py"
              "${_OCCTL_PARITY_DIR}/build_box.json"
    )
    set_tests_properties(parity.python.build_box PROPERTIES
      LABELS "parity;binding;python"
      ENVIRONMENT "OCCTL_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
    )
    if(OCCTL_BUILD_BOOL)
      foreach(_scenario
          fuse_two_boxes
          cut_box_corner
          common_two_overlapping_boxes
          section_two_overlapping_boxes
          split_box_by_box
          history_modified_after_fuse)
        add_test(
          NAME parity.python.${_scenario}
          COMMAND "${_py_exe}"
                  "${_OCCTL_BINDINGS_DIR}/python/tests/parity/runner.py"
                  "${_OCCTL_PARITY_DIR}/${_scenario}.json"
        )
        set_tests_properties(parity.python.${_scenario} PROPERTIES
          LABELS "parity;binding;python"
          ENVIRONMENT "OCCTL_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
        )
      endforeach()
    endif()
  else()
    message(STATUS "parity: python binding enabled but Python3 not found; skipping ctest wiring")
  endif()
endif()

# ----------------------------------------------------------------- C# -----
if(OCCTL_BUILD_BINDINGS_CSHARP AND _OCCTL_PARITY_READY)
  find_program(_dotnet_exe NAMES dotnet)
  if(_dotnet_exe)
    add_test(
      NAME parity.csharp.build
      COMMAND "${_dotnet_exe}" build
              "${_OCCTL_BINDINGS_DIR}/csharp/tests/parity/OcctL.Parity.csproj"
              --no-restore
              -m:1
              -nr:false
              -p:UseSharedCompilation=false
      WORKING_DIRECTORY "${_OCCTL_BINDINGS_DIR}/csharp"
    )
    set_tests_properties(parity.csharp.build PROPERTIES
      LABELS "parity;binding;csharp"
      FIXTURES_SETUP "csharp_parity_build"
    )
    add_test(
      NAME parity.csharp.build_box
      COMMAND "${_dotnet_exe}" run
              --project "${_OCCTL_BINDINGS_DIR}/csharp/tests/parity/OcctL.Parity.csproj"
              --no-restore
              --no-build
              --
              "${_OCCTL_PARITY_DIR}/build_box.json"
      WORKING_DIRECTORY "${_OCCTL_BINDINGS_DIR}/csharp"
    )
    set_tests_properties(parity.csharp.build_box PROPERTIES
      LABELS "parity;binding;csharp"
      FIXTURES_REQUIRED "csharp_parity_build"
      ENVIRONMENT "DYLD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR};LD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
    )
    if(OCCTL_BUILD_BOOL)
      foreach(_scenario
          fuse_two_boxes
          cut_box_corner
          common_two_overlapping_boxes
          section_two_overlapping_boxes
          split_box_by_box
          history_modified_after_fuse)
        add_test(
          NAME parity.csharp.${_scenario}
          COMMAND "${_dotnet_exe}" run
                  --project "${_OCCTL_BINDINGS_DIR}/csharp/tests/parity/OcctL.Parity.csproj"
                  --no-restore
                  --no-build
                  --
                  "${_OCCTL_PARITY_DIR}/${_scenario}.json"
          WORKING_DIRECTORY "${_OCCTL_BINDINGS_DIR}/csharp"
        )
        set_tests_properties(parity.csharp.${_scenario} PROPERTIES
          LABELS "parity;binding;csharp"
          FIXTURES_REQUIRED "csharp_parity_build"
          ENVIRONMENT "DYLD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR};LD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
        )
      endforeach()
    endif()

    # AOT compatibility smoke — publish the main library with Native AOT enabled
    # to verify the binding does not rely on reflection or other JIT-only features.
    # Marked as a "smoke" test rather than "parity" because it asserts build-time
    # compatibility, not functional correctness.
    cmake_host_system_information(RESULT _host_arch QUERY OS_PLATFORM)
    if(_host_arch STREQUAL "arm64" OR _host_arch STREQUAL "aarch64")
      set(_aot_rid "osx-arm64")
    elseif(_host_arch STREQUAL "x86_64" OR _host_arch STREQUAL "AMD64")
      if(APPLE)
        set(_aot_rid "osx-x64")
      else()
        set(_aot_rid "linux-x64")
      endif()
    else()
      set(_aot_rid "linux-x64")
    endif()
    add_test(
      NAME smoke.csharp.aot
      COMMAND "${_dotnet_exe}" publish
              "${_OCCTL_BINDINGS_DIR}/csharp/src/api/OcctL.csproj"
              -r "${_aot_rid}" --self-contained
              -p:PublishAot=true
              -p:NoWarn=1591
              -o "${CMAKE_BINARY_DIR}/csharp-aot-smoke"
      WORKING_DIRECTORY "${_OCCTL_BINDINGS_DIR}/csharp"
    )
    set_tests_properties(smoke.csharp.aot PROPERTIES
      LABELS "smoke;binding;csharp"
    )
  else()
    message(STATUS "parity: csharp binding enabled but dotnet not found; skipping ctest wiring")
  endif()
endif()

# ----------------------------------------------------------------- Node ---
if(OCCTL_BUILD_BINDINGS_NODE AND _OCCTL_PARITY_READY)
  find_program(_npx_exe NAMES npx)
  if(_npx_exe)
    add_test(
      NAME parity.node.build_box
      COMMAND "${_npx_exe}" vitest run tests/parity/parity.test.ts
      WORKING_DIRECTORY "${_OCCTL_BINDINGS_DIR}/node"
    )
    set_tests_properties(parity.node.build_box PROPERTIES
      LABELS "parity;binding;node"
      ENVIRONMENT "OCCTL_PARITY_SCENARIO=${_OCCTL_PARITY_DIR}/build_box.json;DYLD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR};LD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
    )
    if(OCCTL_BUILD_BOOL)
      foreach(_scenario
          fuse_two_boxes
          cut_box_corner
          common_two_overlapping_boxes
          section_two_overlapping_boxes
          split_box_by_box
          history_modified_after_fuse)
        add_test(
          NAME parity.node.${_scenario}
          COMMAND "${_npx_exe}" vitest run tests/parity/parity.test.ts
          WORKING_DIRECTORY "${_OCCTL_BINDINGS_DIR}/node"
        )
        set_tests_properties(parity.node.${_scenario} PROPERTIES
          LABELS "parity;binding;node"
          ENVIRONMENT "OCCTL_PARITY_SCENARIO=${_OCCTL_PARITY_DIR}/${_scenario}.json;DYLD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR};LD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
        )
      endforeach()
    endif()
  else()
    message(STATUS "parity: node binding enabled but npx not found; skipping ctest wiring")
  endif()
endif()

# ---------------------------------------------------------------- Rust ----
if(OCCTL_BUILD_BINDINGS_RUST AND _OCCTL_PARITY_READY)
  find_program(_cargo_exe NAMES cargo HINTS "$ENV{HOME}/.cargo/bin")
  if(_cargo_exe)
    set(_rust_dir "${_OCCTL_BINDINGS_DIR}/rust")
    set(_rust_env
      "OCCTL_REPO_ROOT=${CMAKE_SOURCE_DIR}"
      "OCCTL_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
      "DYLD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
      "LD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
    )
    add_test(
      NAME parity.rust.build_box
      COMMAND "${_cargo_exe}" run --quiet --manifest-path "${_rust_dir}/Cargo.toml"
              --bin parity_runner --
              "${_OCCTL_PARITY_DIR}/build_box.json"
    )
    set_tests_properties(parity.rust.build_box PROPERTIES
      LABELS "parity;binding;rust"
      ENVIRONMENT "${_rust_env}"
    )
    if(OCCTL_BUILD_BOOL)
      foreach(_scenario
          fuse_two_boxes
          cut_box_corner
          common_two_overlapping_boxes
          section_two_overlapping_boxes
          split_box_by_box
          history_modified_after_fuse)
        add_test(
          NAME parity.rust.${_scenario}
          COMMAND "${_cargo_exe}" run --quiet --manifest-path "${_rust_dir}/Cargo.toml"
                  --bin parity_runner --
                  "${_OCCTL_PARITY_DIR}/${_scenario}.json"
        )
        set_tests_properties(parity.rust.${_scenario} PROPERTIES
          LABELS "parity;binding;rust"
          ENVIRONMENT "${_rust_env}"
        )
      endforeach()
    endif()
  else()
    message(STATUS "parity: rust binding enabled but cargo not found; skipping ctest wiring")
  endif()
endif()

# ----------------------------------------------------------------- Go -----
if(OCCTL_BUILD_BINDINGS_GO AND _OCCTL_PARITY_READY)
  find_program(_go_exe NAMES go HINTS "/opt/homebrew/bin" "/usr/local/go/bin")
  if(_go_exe)
    set(_go_dir "${_OCCTL_BINDINGS_DIR}/go")
    set(_go_tags_topo_prim "occtl_topo,occtl_prim")
    if(OCCTL_BUILD_BOOL)
      set(_go_tags_topo_prim "occtl_topo,occtl_prim,occtl_bool")
    endif()
    set(_go_tags_topo_prim_bool "occtl_topo,occtl_prim,occtl_bool")
    set(_go_env
      "OCCTL_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
      "OCCTL_LIBRARY_NAME=${OCCTL_LIBRARY_OUTPUT_NAME}"
      "CGO_ENABLED=1"
      "CGO_LDFLAGS=-L${_OCCTL_BUILD_LIB_DIR} -Wl,-rpath,${_OCCTL_BUILD_LIB_DIR} -l${OCCTL_LIBRARY_OUTPUT_NAME}"
      "DYLD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
      "LD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
    )
    add_test(
      NAME parity.go.build_box
      COMMAND "${_go_exe}" run -tags "${_go_tags_topo_prim}" ./tests/parity_runner
              "${_OCCTL_PARITY_DIR}/build_box.json"
      WORKING_DIRECTORY "${_go_dir}"
    )
    set_tests_properties(parity.go.build_box PROPERTIES
      LABELS "parity;binding;go"
      ENVIRONMENT "${_go_env}"
    )
    if(OCCTL_BUILD_BOOL)
      foreach(_scenario
          fuse_two_boxes
          cut_box_corner
          common_two_overlapping_boxes
          section_two_overlapping_boxes
          split_box_by_box
          history_modified_after_fuse)
        add_test(
          NAME parity.go.${_scenario}
          COMMAND "${_go_exe}" run -tags "${_go_tags_topo_prim_bool}" ./tests/parity_runner
                  "${_OCCTL_PARITY_DIR}/${_scenario}.json"
          WORKING_DIRECTORY "${_go_dir}"
        )
        set_tests_properties(parity.go.${_scenario} PROPERTIES
          LABELS "parity;binding;go"
          ENVIRONMENT "${_go_env}"
        )
      endforeach()
    endif()
  else()
    message(STATUS "parity: go binding enabled but go not found; skipping ctest wiring")
  endif()
endif()

# ---------------------------------------------------------------- Java ----
if(OCCTL_BUILD_BINDINGS_JAVA AND _OCCTL_PARITY_READY)
  find_program(_mvn_exe NAMES mvn)
  if(_mvn_exe)
    set(_java_dir "${_OCCTL_BINDINGS_DIR}/java")
    set(_java_env
      "OCCTL_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
      "OCCTL_LIBRARY_NAME=${OCCTL_LIBRARY_OUTPUT_NAME}"
      "DYLD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
      "LD_LIBRARY_PATH=${_OCCTL_BUILD_LIB_DIR}"
    )
    add_test(
      NAME parity.java.build
      COMMAND "${_mvn_exe}" -q -DskipTests test-compile
      WORKING_DIRECTORY "${_java_dir}"
    )
    set_tests_properties(parity.java.build PROPERTIES
      LABELS "parity;binding;java"
      FIXTURES_SETUP "java_parity_build"
      ENVIRONMENT "${_java_env}"
    )
    add_test(
      NAME parity.java.build_box
      COMMAND "${_mvn_exe}" -q -DskipTests exec:java
              -Dexec.mainClass=org.occtl.parity.Runner
              -Dexec.classpathScope=test
              -Dexec.args=${_OCCTL_PARITY_DIR}/build_box.json
      WORKING_DIRECTORY "${_java_dir}"
    )
    set_tests_properties(parity.java.build_box PROPERTIES
      LABELS "parity;binding;java"
      FIXTURES_REQUIRED "java_parity_build"
      ENVIRONMENT "${_java_env}"
    )
    if(OCCTL_BUILD_BOOL)
      foreach(_scenario
          fuse_two_boxes
          cut_box_corner
          common_two_overlapping_boxes
          section_two_overlapping_boxes
          split_box_by_box
          history_modified_after_fuse)
        add_test(
          NAME parity.java.${_scenario}
          COMMAND "${_mvn_exe}" -q -DskipTests exec:java
                  -Dexec.mainClass=org.occtl.parity.Runner
                  -Dexec.classpathScope=test
                  -Dexec.args=${_OCCTL_PARITY_DIR}/${_scenario}.json
          WORKING_DIRECTORY "${_java_dir}"
        )
        set_tests_properties(parity.java.${_scenario} PROPERTIES
          LABELS "parity;binding;java"
          FIXTURES_REQUIRED "java_parity_build"
          ENVIRONMENT "${_java_env}"
        )
      endforeach()
    endif()
  else()
    message(STATUS "parity: java binding enabled but mvn not found; skipping ctest wiring")
  endif()
endif()

# ---------------------------------------------------------------- WASM ----
if(OCCTL_BUILD_BINDINGS_WASM)
  find_program(_node_exe NAMES node)
  if(_node_exe)
    add_test(
      NAME parity.wasm.build_box
      COMMAND "${_node_exe}" --import tsx tests/parity/runner.ts
      WORKING_DIRECTORY "${_OCCTL_BINDINGS_DIR}/wasm"
    )
    # WASM auto-skips when dist/occtl.wasm is absent; exits 0 in that case.
    set_tests_properties(parity.wasm.build_box PROPERTIES
      LABELS "parity;binding;wasm"
      ENVIRONMENT "NODE_OPTIONS=--no-deprecation;OCCTL_PARITY_SCENARIO=${_OCCTL_PARITY_DIR}/build_box.json"
    )
    if(OCCTL_BUILD_BOOL)
      foreach(_scenario
          fuse_two_boxes
          cut_box_corner
          common_two_overlapping_boxes
          section_two_overlapping_boxes
          split_box_by_box
          history_modified_after_fuse)
        add_test(
          NAME parity.wasm.${_scenario}
          COMMAND "${_node_exe}" --import tsx tests/parity/runner.ts
          WORKING_DIRECTORY "${_OCCTL_BINDINGS_DIR}/wasm"
        )
        set_tests_properties(parity.wasm.${_scenario} PROPERTIES
          LABELS "parity;binding;wasm"
          ENVIRONMENT "NODE_OPTIONS=--no-deprecation;OCCTL_PARITY_SCENARIO=${_OCCTL_PARITY_DIR}/${_scenario}.json"
        )
      endforeach()
    endif()
  else()
    message(STATUS "parity: wasm binding enabled but node not found; skipping ctest wiring")
  endif()
endif()
