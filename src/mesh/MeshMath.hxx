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

//! @file MeshMath.hxx
//! @brief OCCT ↔ POD bridges for the mesh module.
//!
//! Header-only helpers consumed by mesh_generate.cxx and mesh_views.cxx.
//! Translates the public versioned options struct into IMeshTools_Parameters
//! and the public AABB POD into Bnd_Box, plus the inverse conversions used
//! by view extraction.

#ifndef OCCTL_MESH_MESHMATH_HXX
#define OCCTL_MESH_MESHMATH_HXX

#include <occtl/occtl_mesh.h>

#include <gp_Pnt.hxx>
#include <Bnd_Box.hxx>
#include <IMeshTools_Parameters.hxx>

namespace OcctL::Mesh
{

//! Project the public options struct into OCCT's IMeshTools_Parameters.
//! Caller has already validated theOpts->struct_version.
inline IMeshTools_Parameters ToParameters(const occtl_mesh_options_t& theOpts)
{
  IMeshTools_Parameters aOut;
  aOut.Deflection                                = theOpts.deflection;
  aOut.Angle                                     = theOpts.angle;
  aOut.DeflectionInterior                        = theOpts.deflection_interior;
  aOut.AngleInterior                             = theOpts.angle_interior;
  aOut.MinSize                                   = theOpts.min_size;
  aOut.InParallel                                = theOpts.in_parallel != 0;
  aOut.Relative                                  = theOpts.relative != 0;
  aOut.InternalVerticesMode                      = theOpts.internal_vertices_mode != 0;
  aOut.ControlSurfaceDeflection                  = theOpts.control_surface_deflection != 0;
  aOut.EnableControlSurfaceDeflectionAllSurfaces = theOpts.control_surface_deflection_all != 0;
  aOut.CleanModel                                = theOpts.clean_model != 0;
  aOut.AdjustMinSize                             = theOpts.adjust_min_size != 0;
  aOut.ForceFaceDeflection                       = theOpts.force_face_deflection != 0;
  aOut.AllowQualityDecrease                      = theOpts.allow_quality_decrease != 0;
  return aOut;
}

//! Convert the public AABB POD into OCCT's Bnd_Box. Empty / zero-sized
//! input is folded to an empty Bnd_Box (Bnd_Box::IsVoid()).
inline Bnd_Box ToBndBox(const occtl_aabb3_t& theBox)
{
  Bnd_Box aOut;
  if (theBox.min.x == 0.0 && theBox.min.y == 0.0 && theBox.min.z == 0.0 && theBox.max.x == 0.0
      && theBox.max.y == 0.0 && theBox.max.z == 0.0)
  {
    return aOut;
  }
  aOut.Update(theBox.min.x, theBox.min.y, theBox.min.z, theBox.max.x, theBox.max.y, theBox.max.z);
  return aOut;
}

//! True iff the supplied options carry a recognised struct_version.
inline bool IsKnownVersion(const occtl_mesh_options_t& theOpts) noexcept
{
  return theOpts.struct_version == OCCTL_MESH_OPTIONS_VERSION_1;
}

} // namespace OcctL::Mesh

#endif // OCCTL_MESH_MESHMATH_HXX
