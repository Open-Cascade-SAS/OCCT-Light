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

set(OCCTL_REGISTRY_MODULES
  CORE
  GEOM
  TOPO
  PRIM
  TEXT
  BOOL
  MESH
  HEAL
  IO_BREP
  IO_STEP
  IO_IGES
  IO_STL
  DE
  IO_OBJ
  IO_GLTF
  IO_VRML
  IO_PLY
  VIZ
)

function(_occtl_registry_module theKey)
  cmake_parse_arguments(_arg
    ""
    "NAME;OPTION;DEFAULT;STATUS;SUBDIR;FEATURE;DESCRIPTION"
    "TOOLKITS"
    ${ARGN}
  )

  foreach(aRequired NAME STATUS SUBDIR FEATURE DESCRIPTION)
    if(NOT DEFINED _arg_${aRequired})
      message(FATAL_ERROR "_occtl_registry_module(${theKey}): ${aRequired} is required")
    endif()
  endforeach()

  set(OCCTL_REGISTRY_${theKey}_NAME "${_arg_NAME}" CACHE INTERNAL "")
  set(OCCTL_REGISTRY_${theKey}_OPTION "${_arg_OPTION}" CACHE INTERNAL "")
  set(OCCTL_REGISTRY_${theKey}_DEFAULT "${_arg_DEFAULT}" CACHE INTERNAL "")
  set(OCCTL_REGISTRY_${theKey}_STATUS "${_arg_STATUS}" CACHE INTERNAL "")
  set(OCCTL_REGISTRY_${theKey}_SUBDIR "${_arg_SUBDIR}" CACHE INTERNAL "")
  set(OCCTL_REGISTRY_${theKey}_FEATURE "${_arg_FEATURE}" CACHE INTERNAL "")
  set(OCCTL_REGISTRY_${theKey}_DESCRIPTION "${_arg_DESCRIPTION}" CACHE INTERNAL "")
  set(OCCTL_REGISTRY_${theKey}_TOOLKITS "${_arg_TOOLKITS}" CACHE INTERNAL "")
endfunction()

_occtl_registry_module(CORE
  NAME core
  STATUS implemented
  SUBDIR src/core
  FEATURE core
  DESCRIPTION "Foundation runtime, status, errors, POD value types"
)
_occtl_registry_module(GEOM
  NAME geom
  OPTION OCCTL_BUILD_GEOM
  DEFAULT ON
  STATUS implemented
  SUBDIR src/geom
  FEATURE geom
  DESCRIPTION "Curves, surfaces, transforms, and geometric POD operations"
  TOOLKITS TKMath TKG3d TKG2d TKGeomBase TKGeomAlgo
)
_occtl_registry_module(TOPO
  NAME topo
  OPTION OCCTL_BUILD_TOPO
  DEFAULT ON
  STATUS implemented
  SUBDIR src/topo
  FEATURE topo
  DESCRIPTION "BRepGraph topology graph and graph-native algorithms"
  TOOLKITS TKBRep TKTopAlgo TKFillet
)
_occtl_registry_module(PRIM
  NAME prim
  OPTION OCCTL_BUILD_PRIM
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/prim
  FEATURE prim
  DESCRIPTION "Generative topology builders"
  TOOLKITS TKPrim TKOffset TKBRep TKTopAlgo TKFeat TKGeomAlgo TKG3d TKG2d TKFillet
)
_occtl_registry_module(TEXT
  NAME text
  OPTION OCCTL_BUILD_TEXT
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/text
  FEATURE text
  DESCRIPTION "Text-to-shape conversion"
  TOOLKITS TKV3d TKService TKBRep TKMath TKG3d
)
_occtl_registry_module(BOOL
  NAME bool
  OPTION OCCTL_BUILD_BOOL
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/bool
  FEATURE bool
  DESCRIPTION "Boolean operations and history"
  TOOLKITS TKBO TKBool TKBRep TKTopAlgo TKG3d TKG2d TKMath
)
_occtl_registry_module(MESH
  NAME mesh
  OPTION OCCTL_BUILD_MESH
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/mesh
  FEATURE mesh
  DESCRIPTION "BRepMesh generation and triangulation views"
  TOOLKITS TKMesh TKXMesh TKBRep TKTopAlgo TKMath TKG3d
)
_occtl_registry_module(HEAL
  NAME heal
  OPTION OCCTL_BUILD_HEAL
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/heal
  FEATURE heal
  DESCRIPTION "Shape healing and analysis"
  TOOLKITS TKShHealing TKBRep TKTopAlgo TKMath
)
_occtl_registry_module(IO_BREP
  NAME io_brep
  OPTION OCCTL_BUILD_IO_BREP
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/io_brep
  FEATURE io_brep
  DESCRIPTION "Native OCCT BRep I/O"
  TOOLKITS TKDE TKDECascade TKBRep TKBin
)
_occtl_registry_module(IO_STEP
  NAME io_step
  OPTION OCCTL_BUILD_IO_STEP
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/io_step
  FEATURE io_step
  DESCRIPTION "STEP I/O"
  TOOLKITS TKDESTEP TKXSBase TKBRep TKTopAlgo
)
_occtl_registry_module(IO_IGES
  NAME io_iges
  OPTION OCCTL_BUILD_IO_IGES
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/io_iges
  FEATURE io_iges
  DESCRIPTION "IGES I/O"
  TOOLKITS TKDEIGES TKXSBase TKBRep TKTopAlgo
)
_occtl_registry_module(IO_STL
  NAME io_stl
  OPTION OCCTL_BUILD_IO_STL
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/io_stl
  FEATURE io_stl
  DESCRIPTION "STL I/O"
  TOOLKITS TKDESTL TKXSBase TKBRep TKTopAlgo
)
_occtl_registry_module(DE
  NAME de
  OPTION OCCTL_BUILD_DE
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/de
  FEATURE de
  DESCRIPTION "Unified data-exchange dispatch"
  TOOLKITS TKDE TKBRep
)
_occtl_registry_module(IO_OBJ
  NAME io_obj
  OPTION OCCTL_BUILD_IO_OBJ
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/io_obj
  FEATURE io_obj
  DESCRIPTION "Wavefront OBJ I/O"
  TOOLKITS TKDEOBJ TKDE TKRWMesh TKBRep TKTopAlgo
)
_occtl_registry_module(IO_GLTF
  NAME io_gltf
  OPTION OCCTL_BUILD_IO_GLTF
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/io_gltf
  FEATURE io_gltf
  DESCRIPTION "glTF 2.0 I/O"
  TOOLKITS TKDEGLTF TKDE TKRWMesh TKBRep TKTopAlgo
)
_occtl_registry_module(IO_VRML
  NAME io_vrml
  OPTION OCCTL_BUILD_IO_VRML
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/io_vrml
  FEATURE io_vrml
  DESCRIPTION "VRML I/O"
  TOOLKITS TKDEVRML TKDE TKRWMesh TKBRep TKTopAlgo
)
_occtl_registry_module(IO_PLY
  NAME io_ply
  OPTION OCCTL_BUILD_IO_PLY
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/io_ply
  FEATURE io_ply
  DESCRIPTION "PLY I/O"
  TOOLKITS TKDEPLY TKDE TKRWMesh TKBRep TKTopAlgo
)
_occtl_registry_module(VIZ
  NAME viz
  OPTION OCCTL_BUILD_VIZ
  DEFAULT OFF
  STATUS implemented
  SUBDIR src/viz
  FEATURE viz
  DESCRIPTION "Interactive visualization"
  TOOLKITS TKService TKV3d TKOpenGl TKMeshVS TKBRep TKTopAlgo TKMesh
)

function(occtl_registry_define_options)
  foreach(aKey IN LISTS OCCTL_REGISTRY_MODULES)
    set(anOption "${OCCTL_REGISTRY_${aKey}_OPTION}")
    if(NOT anOption)
      continue()
    endif()
    option(${anOption} "${OCCTL_REGISTRY_${aKey}_DESCRIPTION}" ${OCCTL_REGISTRY_${aKey}_DEFAULT})
  endforeach()
endfunction()

function(occtl_registry_validate_enabled)
  foreach(aKey IN LISTS OCCTL_REGISTRY_MODULES)
    set(anOption "${OCCTL_REGISTRY_${aKey}_OPTION}")
    if(NOT anOption)
      continue()
    endif()
    if(${anOption} AND OCCTL_REGISTRY_${aKey}_STATUS STREQUAL "planned")
      message(FATAL_ERROR
        "${anOption}=ON, but module '${OCCTL_REGISTRY_${aKey}_NAME}' is planned "
        "and has no occt-light target yet")
    endif()
  endforeach()
endfunction()

function(occtl_registry_collect_enabled outModules outToolkits)
  set(aModules core)
  set(aToolkits "")

  foreach(aKey IN LISTS OCCTL_REGISTRY_MODULES)
    if(aKey STREQUAL "CORE")
      continue()
    endif()
    set(anOption "${OCCTL_REGISTRY_${aKey}_OPTION}")
    if(anOption AND ${anOption} AND OCCTL_REGISTRY_${aKey}_STATUS STREQUAL "implemented")
      list(APPEND aModules "${OCCTL_REGISTRY_${aKey}_NAME}")
      list(APPEND aToolkits ${OCCTL_REGISTRY_${aKey}_TOOLKITS})
    endif()
  endforeach()

  if(OCCTL_BUILD_DE)
    if(OCCTL_BUILD_IO_BREP)
      list(APPEND aToolkits TKDECascade)
    endif()
    if(OCCTL_BUILD_IO_STEP)
      list(APPEND aToolkits TKDESTEP)
    endif()
    if(OCCTL_BUILD_IO_IGES)
      list(APPEND aToolkits TKDEIGES)
    endif()
    if(OCCTL_BUILD_IO_STL)
      list(APPEND aToolkits TKDESTL)
    endif()
    if(OCCTL_BUILD_IO_OBJ)
      list(APPEND aToolkits TKDEOBJ)
    endif()
    if(OCCTL_BUILD_IO_GLTF)
      list(APPEND aToolkits TKDEGLTF)
    endif()
    if(OCCTL_BUILD_IO_VRML)
      list(APPEND aToolkits TKDEVRML)
    endif()
    if(OCCTL_BUILD_IO_PLY)
      list(APPEND aToolkits TKDEPLY)
    endif()
  endif()

  if(aToolkits)
    list(REMOVE_DUPLICATES aToolkits)
  endif()

  set(${outModules} "${aModules}" PARENT_SCOPE)
  set(${outToolkits} "${aToolkits}" PARENT_SCOPE)
endfunction()

function(occtl_registry_add_enabled_subdirectories)
  add_subdirectory(src/core)
  foreach(aKey IN LISTS OCCTL_REGISTRY_MODULES)
    if(aKey STREQUAL "CORE")
      continue()
    endif()
    set(anOption "${OCCTL_REGISTRY_${aKey}_OPTION}")
    if(anOption AND ${anOption} AND OCCTL_REGISTRY_${aKey}_STATUS STREQUAL "implemented")
      add_subdirectory("${OCCTL_REGISTRY_${aKey}_SUBDIR}")
    endif()
  endforeach()
endfunction()

function(occtl_registry_print_summary)
  message(STATUS "  Modules: ")
  message(STATUS "    core            : ON")
  foreach(aKey IN LISTS OCCTL_REGISTRY_MODULES)
    if(aKey STREQUAL "CORE")
      continue()
    endif()
    set(anOption "${OCCTL_REGISTRY_${aKey}_OPTION}")
    if(NOT anOption)
      continue()
    endif()
    set(aName "${OCCTL_REGISTRY_${aKey}_NAME}")
    if(${anOption})
      set(aState "ON")
    else()
      set(aState "OFF")
    endif()
    if(OCCTL_REGISTRY_${aKey}_STATUS STREQUAL "planned")
      set(aState "${aState} (planned)")
    endif()
    string(LENGTH "${aName}" aLen)
    math(EXPR aPad "16 - ${aLen}")
    if(aPad LESS 1)
      set(aPad 1)
    endif()
    string(REPEAT " " ${aPad} aSpaces)
    message(STATUS "    ${aName}${aSpaces}: ${aState}")
  endforeach()
endfunction()

function(_occtl_registry_json_array outVar)
  set(aJson "")
  foreach(aValue IN LISTS ARGN)
    if(aJson)
      string(APPEND aJson ", ")
    endif()
    string(APPEND aJson "\"${aValue}\"")
  endforeach()
  set(${outVar} "[${aJson}]" PARENT_SCOPE)
endfunction()

function(occtl_registry_write_feature_metadata)
  occtl_registry_collect_enabled(aModules aToolkits)
  get_property(aPublicHeaders GLOBAL PROPERTY OCCTL_PUBLIC_HEADERS)
  set(aCHeaders "")
  set(aHppHeaders "")
  foreach(aHeader IN LISTS aPublicHeaders)
    get_filename_component(aBase "${aHeader}" NAME)
    if(aHeader MATCHES "/occtl-hpp/")
      list(APPEND aHppHeaders "${aBase}")
    elseif(aHeader MATCHES "/include/occtl/")
      list(APPEND aCHeaders "${aBase}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES aCHeaders)
  list(REMOVE_DUPLICATES aHppHeaders)

  _occtl_registry_json_array(aModulesJson ${aModules})
  _occtl_registry_json_array(aToolkitsJson ${aToolkits})
  _occtl_registry_json_array(aCHeadersJson ${aCHeaders})
  _occtl_registry_json_array(aHppHeadersJson ${aHppHeaders})

  set(OCCTL_FEATURE_METADATA "${CMAKE_BINARY_DIR}/OCCTLFeatures.json"
      CACHE FILEPATH "Machine-readable feature metadata for this OCCT-Light build" FORCE)

  file(WRITE "${OCCTL_FEATURE_METADATA}"
"{
  \"schema_version\": 1,
  \"project\": \"occtl\",
  \"version\": \"${PROJECT_VERSION}\",
  \"abi_version\": ${OCCTL_ABI_VERSION},
  \"library_name\": \"${OCCTL_LIBRARY_OUTPUT_NAME}\",
  \"components\": ${aModulesJson},
  \"binding_features\": ${aModulesJson},
  \"occt_toolkits\": ${aToolkitsJson},
  \"c_headers\": ${aCHeadersJson},
  \"hpp_headers\": ${aHppHeadersJson}
}
")

  install(FILES "${OCCTL_FEATURE_METADATA}"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OCCTL
  )
endfunction()
