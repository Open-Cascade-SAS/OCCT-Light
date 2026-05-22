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

include(OCCTLCompileOptions)

function(occtl_add_module theName)
  cmake_parse_arguments(_arg
    ""                                            # options
    ""                                            # one-value
    "SOURCES;PUBLIC_HEADERS;PRIVATE_HEADER;PRIVATE_HEADERS;PUBLIC_LINK;PRIVATE_LINK"  # multi-value
    ${ARGN}
  )

  if(NOT _arg_SOURCES)
    message(FATAL_ERROR "occtl_add_module(${theName}): SOURCES is required")
  endif()

  set(aTarget "occtl-${theName}")
  set(anObjectTarget "${aTarget}-obj")

  add_library(${anObjectTarget} OBJECT ${_arg_SOURCES})
  add_library(${aTarget} INTERFACE)
  add_library(OCCTL::${theName} ALIAS ${aTarget})

  occtl_apply_compile_options(${anObjectTarget})

  target_include_directories(${anObjectTarget}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_include_directories(${aTarget}
    INTERFACE
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  if(_arg_PUBLIC_LINK)
    target_link_libraries(${aTarget} INTERFACE ${_arg_PUBLIC_LINK})
  endif()
  if(_arg_PRIVATE_LINK)
    target_link_libraries(${anObjectTarget} PRIVATE ${_arg_PRIVATE_LINK})
    set_property(GLOBAL APPEND PROPERTY OCCTL_PRIVATE_LINKS ${_arg_PRIVATE_LINK})
  endif()

  set_target_properties(${aTarget} PROPERTIES EXPORT_NAME ${theName})

  set_property(GLOBAL APPEND PROPERTY OCCTL_MODULE_NAMES ${theName})
  set_property(GLOBAL APPEND PROPERTY OCCTL_OBJECT_TARGETS ${anObjectTarget})
  set_property(GLOBAL APPEND PROPERTY OCCTL_COMPAT_TARGETS ${aTarget})
  set_property(GLOBAL APPEND PROPERTY OCCTL_PUBLIC_HEADERS ${_arg_PUBLIC_HEADERS})
endfunction()

function(occtl_module_compile_definitions theName)
  set(aTarget "occtl-${theName}")
  set(anObjectTarget "${aTarget}-obj")
  if(NOT TARGET ${anObjectTarget})
    message(FATAL_ERROR "occtl_module_compile_definitions(${theName}): unknown module")
  endif()
  target_compile_definitions(${anObjectTarget} PRIVATE ${ARGN})
  target_compile_definitions(${aTarget} INTERFACE ${ARGN})
  set_property(GLOBAL APPEND PROPERTY OCCTL_PUBLIC_DEFINITIONS ${ARGN})
endfunction()

function(occtl_module_link_libraries theName)
  set(aTarget "occtl-${theName}")
  set(anObjectTarget "${aTarget}-obj")
  if(NOT TARGET ${anObjectTarget})
    message(FATAL_ERROR "occtl_module_link_libraries(${theName}): unknown module")
  endif()
  target_link_libraries(${anObjectTarget} PRIVATE ${ARGN})
  set_property(GLOBAL APPEND PROPERTY OCCTL_PRIVATE_LINKS ${ARGN})
endfunction()

function(occtl_select_output_name outVar)
  if(NOT OCCTL_BUILD_GEOM AND NOT OCCTL_BUILD_TOPO AND NOT OCCTL_BUILD_PRIM)
    set(${outVar} "occtl-core" PARENT_SCOPE)
  elseif(OCCTL_BUILD_GEOM AND NOT OCCTL_BUILD_TOPO AND NOT OCCTL_BUILD_PRIM)
    set(${outVar} "occtl-geom" PARENT_SCOPE)
  elseif(OCCTL_BUILD_VIZ)
    set(${outVar} "occtl-full-viz" PARENT_SCOPE)
  elseif(OCCTL_BUILD_IO_IGES)
    set(${outVar} "occtl-full" PARENT_SCOPE)
  elseif(OCCTL_BUILD_BOOL OR OCCTL_BUILD_MESH OR OCCTL_BUILD_HEAL OR OCCTL_BUILD_TEXT
      OR OCCTL_BUILD_IO_BREP OR OCCTL_BUILD_IO_STEP OR OCCTL_BUILD_IO_STL
      OR OCCTL_BUILD_IO_OBJ OR OCCTL_BUILD_IO_GLTF OR OCCTL_BUILD_IO_VRML
      OR OCCTL_BUILD_IO_PLY OR OCCTL_BUILD_DE)
    set(${outVar} "occtl-cad" PARENT_SCOPE)
  elseif(OCCTL_BUILD_GEOM AND OCCTL_BUILD_TOPO AND OCCTL_BUILD_PRIM)
    set(${outVar} "occtl-minimal" PARENT_SCOPE)
  else()
    set(${outVar} "occtl-custom" PARENT_SCOPE)
  endif()
endfunction()

function(occtl_create_single_library)
  get_property(aModuleNames GLOBAL PROPERTY OCCTL_MODULE_NAMES)
  get_property(anObjectTargets GLOBAL PROPERTY OCCTL_OBJECT_TARGETS)
  get_property(aCompatTargets GLOBAL PROPERTY OCCTL_COMPAT_TARGETS)
  get_property(aPrivateLinks GLOBAL PROPERTY OCCTL_PRIVATE_LINKS)
  get_property(aPublicDefinitions GLOBAL PROPERTY OCCTL_PUBLIC_DEFINITIONS)
  get_property(aPublicHeaders GLOBAL PROPERTY OCCTL_PUBLIC_HEADERS)

  if(NOT anObjectTargets)
    message(FATAL_ERROR "occtl_create_single_library(): no modules registered")
  endif()

  set(aObjectSources "")
  foreach(anObjectTarget IN LISTS anObjectTargets)
    list(APPEND aObjectSources $<TARGET_OBJECTS:${anObjectTarget}>)
  endforeach()

  occtl_select_output_name(aOutputName)
  add_library(occtl ${OCCTL_LIBRARY_KIND} ${aObjectSources})
  add_library(OCCTL::occtl ALIAS occtl)

  occtl_apply_compile_options(occtl)
  target_compile_features(occtl PUBLIC cxx_std_17)
  target_include_directories(occtl
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  if(aPrivateLinks)
    list(REMOVE_DUPLICATES aPrivateLinks)
    target_link_libraries(occtl PRIVATE ${aPrivateLinks})
  endif()
  if(aPublicDefinitions)
    list(REMOVE_DUPLICATES aPublicDefinitions)
    target_compile_definitions(occtl PUBLIC ${aPublicDefinitions})
  endif()

  set_target_properties(occtl PROPERTIES
    VERSION       ${PROJECT_VERSION}
    SOVERSION     ${OCCTL_ABI_VERSION}
    OUTPUT_NAME   ${aOutputName}
    EXPORT_NAME   occtl
  )

  add_library(occtl-all INTERFACE)
  add_library(OCCTL::all ALIAS occtl-all)
  target_link_libraries(occtl-all INTERFACE OCCTL::occtl)
  set_target_properties(occtl-all PROPERTIES EXPORT_NAME all)

  foreach(aCompatTarget IN LISTS aCompatTargets)
    target_link_libraries(${aCompatTarget} INTERFACE OCCTL::occtl)
  endforeach()

  set(OCCTL_LIBRARY_OUTPUT_NAME "${aOutputName}" CACHE INTERNAL "OCCT-Light physical library output name")
  set(OCCTL_BUILT_COMPONENTS "${aModuleNames}" CACHE INTERNAL "OCCT-Light built components")

  set(aCHeaders "")
  set(aHppHeaders "")
  foreach(aHeader IN LISTS aPublicHeaders)
    if(aHeader MATCHES "/occtl-hpp/")
      list(APPEND aHppHeaders "${aHeader}")
    else()
      list(APPEND aCHeaders "${aHeader}")
    endif()
  endforeach()
  if(aCHeaders)
    list(REMOVE_DUPLICATES aCHeaders)
    install(FILES ${aCHeaders} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/occtl)
  endif()
  if(aHppHeaders)
    list(REMOVE_DUPLICATES aHppHeaders)
    install(FILES ${aHppHeaders} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/occtl-hpp)
  endif()

  install(TARGETS occtl occtl-all ${aCompatTargets}
    EXPORT          OCCTLTargets
    LIBRARY         DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE         DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME         DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endfunction()
