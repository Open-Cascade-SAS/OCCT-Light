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
 * @file
 * @brief Umbrella C++ veneer include.
 *
 * Pulls in every veneer header enabled for the selected feature set.
 * Prefer per-module includes when build time matters.
 */

#ifndef OCCTL_HPP_OCCTL_HPP
#define OCCTL_HPP_OCCTL_HPP

#include "core.hpp"

#ifdef OCCTL_HAS_GEOM
  #include "curves.hpp"
  #include "curves2d.hpp"
  #include "geom.hpp"
  #include "surfaces.hpp"
#endif

#ifdef OCCTL_HAS_TOPO
  #include "topo.hpp"
#endif

#ifdef OCCTL_HAS_PRIM
  #include "prim.hpp"
#endif

#ifdef OCCTL_HAS_TEXT
  #include "text.hpp"
#endif

#ifdef OCCTL_HAS_BOOL
  #include "bool.hpp"
#endif

#ifdef OCCTL_HAS_MESH
  #include "mesh.hpp"
#endif

#ifdef OCCTL_HAS_HEAL
  #include "heal.hpp"
#endif

#ifdef OCCTL_HAS_DE
  #include "de.hpp"
#endif

#ifdef OCCTL_HAS_IO_BREP
  #include "io_brep.hpp"
#endif

#ifdef OCCTL_HAS_IO_STEP
  #include "io_step.hpp"
#endif

#ifdef OCCTL_HAS_IO_IGES
  #include "io_iges.hpp"
#endif

#ifdef OCCTL_HAS_IO_STL
  #include "io_stl.hpp"
#endif

#ifdef OCCTL_HAS_IO_OBJ
  #include "io_obj.hpp"
#endif

#ifdef OCCTL_HAS_IO_GLTF
  #include "io_gltf.hpp"
#endif

#ifdef OCCTL_HAS_IO_VRML
  #include "io_vrml.hpp"
#endif

#ifdef OCCTL_HAS_IO_PLY
  #include "io_ply.hpp"
#endif

#ifdef OCCTL_HAS_VIZ
  #include "viz.hpp"
#endif

#endif // OCCTL_HPP_OCCTL_HPP
