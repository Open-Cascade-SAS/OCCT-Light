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

# Set OpenCASCADE_DIR so find_package(OpenCASCADE CONFIG) finds the right install.
# Using HINTS alone is insufficient: when the OpenCASCADE config script loads all
# module targets unconditionally, a cross-module dependency ordering issue
# (Visualization targets referencing TKDE which is defined by DataExchange) causes
# OpenCASCADE_FOUND to be set to FALSE. Loading only the required modules avoids this.
if(OCCT_DIR AND NOT OpenCASCADE_DIR)
  set(OpenCASCADE_DIR "${OCCT_DIR}" CACHE PATH "OpenCASCADE CMake config dir" FORCE)
endif()

# Toolkit → OpenCASCADE module mapping.
# Each toolkit belongs to exactly one OpenCASCADE module. We load only the modules
# that contain toolkits requested by the caller, which avoids the cross-module
# dependency issue described above.
set(_OCCTL_TOOLKIT_MODULE_FoundationClasses
  TKernel TKMath)
set(_OCCTL_TOOLKIT_MODULE_ModelingData
  TKG2d TKG3d TKGeomBase TKBRep)
set(_OCCTL_TOOLKIT_MODULE_ModelingAlgorithms
  TKTopAlgo TKGeomAlgo TKShHealing TKPrim TKBool TKFeat TKFillet TKOffset
  TKMesh TKXMesh TKHLR TKExpress)
set(_OCCTL_TOOLKIT_MODULE_Visualization
  TKService TKV3d TKOpenGl TKMeshVS TKIVTK TKD3DHost TKIVtkDraw)
set(_OCCTL_TOOLKIT_MODULE_ApplicationFramework
  TKCDF TKCAF TKLCAF TKVCAF TKBin TKBinL TKBinTObj TKStd TKStdL
  TKXml TKXmlL TKXmlTObj TKTObj TKBO TKBinXCAF TKXmlXCAF TKXCAF)
set(_OCCTL_TOOLKIT_MODULE_DataExchange
  TKDE TKXSBase TKDESTEP TKDEIGES TKDESTL TKDEVRML TKRWMesh TKDECascade
  TKDEOBJ TKDEGLTF TKDEPLY)
set(_OCCTL_TOOLKIT_MODULE_Draw
  TKDraw TKTopTest TKViewerTest TKOpenGlTest TKDEDRAW TKXSDRAW TKXSDRAWDE
  TKXSDRAWObj TKXSDRAWGltf TKXSDRAWPly)

# Determine which OpenCASCADE modules are required for the requested toolkits.
set(_occt_required_modules "")
foreach(_toolkit IN LISTS OCCT_FIND_COMPONENTS)
  set(_found_module FALSE)
  foreach(_module IN ITEMS FoundationClasses ModelingData ModelingAlgorithms
                           Visualization ApplicationFramework DataExchange Draw)
    if(_toolkit IN_LIST _OCCTL_TOOLKIT_MODULE_${_module})
      list(APPEND _occt_required_modules "${_module}")
      set(_found_module TRUE)
      break()
    endif()
  endforeach()
  if(NOT _found_module)
    message(WARNING "FindOCCT: toolkit '${_toolkit}' is not in the known module map; "
                    "all OpenCASCADE modules will be loaded as a fallback.")
    set(_occt_required_modules "")
    break()
  endif()
endforeach()
list(REMOVE_DUPLICATES _occt_required_modules)

# OCCT 8.0+ has a circular dependency between Visualization (TKV3d → TKDE) and
# DataExchange (TKDECascade → TKV3d).  No linear component order satisfies both,
# so each module's targets file unconditionally sets OpenCASCADE_FOUND=FALSE when
# loaded ahead of its cyclic peer — even though every add_library(... IMPORTED)
# inside the file still executes.  We resolve the cycle by loading the full set
# of likely-needed modules at once and trusting the post-load target-existence
# check below as the actual source of truth.
if("Visualization" IN_LIST _occt_required_modules
   OR "DataExchange" IN_LIST _occt_required_modules
   OR "ApplicationFramework" IN_LIST _occt_required_modules)
  list(APPEND _occt_required_modules
       FoundationClasses ModelingData ModelingAlgorithms
       Visualization ApplicationFramework DataExchange)
  list(REMOVE_DUPLICATES _occt_required_modules)
endif()

# Load only the required modules (or all if the mapping is incomplete).
if(_occt_required_modules)
  find_package(OpenCASCADE CONFIG QUIET COMPONENTS ${_occt_required_modules})
else()
  # Fallback: load every module via OPTIONAL_COMPONENTS so that a partially-broken
  # install (e.g. DataExchange referencing TKDE before Visualization is loaded) does
  # not poison OpenCASCADE_FOUND.
  find_package(OpenCASCADE CONFIG QUIET
    OPTIONAL_COMPONENTS
      FoundationClasses ModelingData ModelingAlgorithms
      Visualization ApplicationFramework DataExchange Draw)
endif()

# Validate every requested toolkit target is present.  Target existence is the
# authoritative check — OpenCASCADE_FOUND can be a stale FALSE from circular
# references in the OCCT 8.0+ config even when all libraries are correctly imported.
set(_occtl_missing_toolkits "")
foreach(_toolkit IN LISTS OCCT_FIND_COMPONENTS)
  if(NOT TARGET ${_toolkit})
    list(APPEND _occtl_missing_toolkits ${_toolkit})
  endif()
endforeach()

if(_occtl_missing_toolkits)
  if(NOT OpenCASCADE_FOUND AND NOT OpenCASCADE_VERSION)
    message(FATAL_ERROR
      "OCCT-Light requires an OCCT install. Set OCCT_DIR to the directory containing "
      "OpenCASCADEConfig.cmake (typically <prefix>/lib/cmake/opencascade).\n"
      "Tried: OCCT_DIR='${OCCT_DIR}'"
    )
  endif()
  message(FATAL_ERROR
    "OCCT-Light requested OCCT toolkit(s) '${_occtl_missing_toolkits}' but they "
    "are not exported by this OCCT install at '${OpenCASCADE_INSTALL_PREFIX}'."
  )
endif()

set(OCCT_FOUND TRUE)
set(OCCT_VERSION ${OpenCASCADE_VERSION})
mark_as_advanced(OCCT_DIR)
