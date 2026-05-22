// Copyright (c) 2026 Capgemini Engineering Research and Development.
//
// This file is part of OCCT-Light software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License version 3 as published
// by the Free Software Foundation, with an option to use any later version.
// Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
// for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of a commercial
// license or contractual agreement.
//
// SPDX-License-Identifier: AGPL-3.0-or-later

/**
 * @file occtl.h
 * @brief OCCT-Light: umbrella public include.
 *
 * Pulls in every module's public header that was enabled at build
 * time. Modules disabled at configure time are gated by
 * `OCCTL_HAS_<MODULE>` macros so this header continues to compile.
 *
 * ## Feature gating
 *
 * The build exports @c OCCTL_HAS_<MODULE> macros for the modules present in
 * the selected feature-set library. This umbrella uses those macros to include
 * only headers that are available in the current install. Downstream code may
 * use the same macros for compile-time feature checks; package-level component
 * checks remain the authoritative way to require a feature.
 *
 * When none of the optionals are enabled the umbrella still compiles
 * — it provides only the @c core module (always available).
 *
 * Prefer including specific module headers directly when binary size
 * matters (e.g. precompiled headers, WASM builds).
 *
 * @sa occtl_core.h
 * @sa docs/design/MODULES.md
 */

#ifndef OCCTL_H
#define OCCTL_H

#include "occtl_core.h"

#ifdef OCCTL_HAS_GEOM
  #include "occtl_curves.h"
  #include "occtl_curves2d.h"
  #include "occtl_curves_common.h"
  #include "occtl_geom.h"
  #include "occtl_surfaces.h"
#endif

#ifdef OCCTL_HAS_TOPO
  #include "occtl_topo.h"
  #include "occtl_topo_algo.h"
  #include "occtl_topo_build.h"
  #include "occtl_topo_relation.h"
  #include "occtl_topo_types.h"
#endif

#ifdef OCCTL_HAS_PRIM
  #include "occtl_prim.h"
#endif

#ifdef OCCTL_HAS_TEXT
  #include "occtl_text.h"
#endif

#ifdef OCCTL_HAS_BOOL
  #include "occtl_bool.h"
#endif

#ifdef OCCTL_HAS_MESH
  #include "occtl_mesh.h"
#endif

#ifdef OCCTL_HAS_HEAL
  #include "occtl_heal.h"
#endif

#ifdef OCCTL_HAS_DE
  #include "occtl_de.h"
#endif

#ifdef OCCTL_HAS_IO_BREP
  #include "occtl_io_brep.h"
#endif
#ifdef OCCTL_HAS_IO_STEP
  #include "occtl_io_step.h"
#endif
#ifdef OCCTL_HAS_IO_IGES
  #include "occtl_io_iges.h"
#endif
#ifdef OCCTL_HAS_IO_STL
  #include "occtl_io_stl.h"
#endif
#ifdef OCCTL_HAS_IO_OBJ
  #include "occtl_io_obj.h"
#endif
#ifdef OCCTL_HAS_IO_GLTF
  #include "occtl_io_gltf.h"
#endif
#ifdef OCCTL_HAS_IO_VRML
  #include "occtl_io_vrml.h"
#endif
#ifdef OCCTL_HAS_IO_PLY
  #include "occtl_io_ply.h"
#endif

#ifdef OCCTL_HAS_VIZ
  #include "occtl_viz.h"
#endif

#endif /* OCCTL_H */
