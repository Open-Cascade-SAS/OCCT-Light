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

#include "test_mesh_helpers.hxx"

#include <occtl/occtl_mesh.h>

#include <IMeshTools_Parameters.hxx>

#include <gtest/gtest.h>

namespace
{

TEST(MeshOptions, Init_AllFields_MatchOcctDefaults)
{
  occtl_mesh_options_t aOpts{};
  occtl_mesh_options_init(&aOpts);

  IMeshTools_Parameters aOcct;

  EXPECT_EQ(aOpts.struct_version, OCCTL_MESH_OPTIONS_VERSION_1);
  EXPECT_EQ(aOpts.p_next, nullptr);

  EXPECT_DOUBLE_EQ(aOpts.deflection, aOcct.Deflection);
  EXPECT_DOUBLE_EQ(aOpts.angle, aOcct.Angle);
  EXPECT_DOUBLE_EQ(aOpts.deflection_interior, aOcct.DeflectionInterior);
  EXPECT_DOUBLE_EQ(aOpts.angle_interior, aOcct.AngleInterior);
  EXPECT_DOUBLE_EQ(aOpts.min_size, aOcct.MinSize);

  EXPECT_EQ(static_cast<bool>(aOpts.in_parallel), aOcct.InParallel);
  EXPECT_EQ(static_cast<bool>(aOpts.relative), aOcct.Relative);
  EXPECT_EQ(static_cast<bool>(aOpts.internal_vertices_mode), aOcct.InternalVerticesMode);
  EXPECT_EQ(static_cast<bool>(aOpts.control_surface_deflection), aOcct.ControlSurfaceDeflection);
  EXPECT_EQ(static_cast<bool>(aOpts.control_surface_deflection_all),
            aOcct.EnableControlSurfaceDeflectionAllSurfaces);
  EXPECT_EQ(static_cast<bool>(aOpts.clean_model), aOcct.CleanModel);
  EXPECT_EQ(static_cast<bool>(aOpts.adjust_min_size), aOcct.AdjustMinSize);
  EXPECT_EQ(static_cast<bool>(aOpts.force_face_deflection), aOcct.ForceFaceDeflection);
  EXPECT_EQ(static_cast<bool>(aOpts.allow_quality_decrease), aOcct.AllowQualityDecrease);

  EXPECT_EQ(aOpts.use_bbox, 0);
  EXPECT_DOUBLE_EQ(aOpts.deviation_coefficient, 0.001);
  // 20° = 0.3490658503988659 rad. Allow 1e-12 because the literal in
  // OCCTL_MESH_OPTIONS_INIT is rounded to 16 significant digits.
  EXPECT_NEAR(aOpts.deviation_angle, 0.34906585039886591, 1.0e-12);
}

TEST(MeshOptions, Init_NullOptions_NoOp)
{
  // Must not crash, must not set any thread-local error.
  occtl_mesh_options_init(nullptr);
}

TEST(MeshOptions, StaticInitMacro_MatchesRuntimeInit)
{
  occtl_mesh_options_t aFromMacro = OCCTL_MESH_OPTIONS_INIT;
  occtl_mesh_options_t aFromInit{};
  occtl_mesh_options_init(&aFromInit);

  EXPECT_EQ(aFromMacro.struct_version, aFromInit.struct_version);
  EXPECT_EQ(aFromMacro.p_next, aFromInit.p_next);
  EXPECT_DOUBLE_EQ(aFromMacro.deflection, aFromInit.deflection);
  EXPECT_DOUBLE_EQ(aFromMacro.angle, aFromInit.angle);
  EXPECT_DOUBLE_EQ(aFromMacro.deflection_interior, aFromInit.deflection_interior);
  EXPECT_DOUBLE_EQ(aFromMacro.angle_interior, aFromInit.angle_interior);
  EXPECT_DOUBLE_EQ(aFromMacro.min_size, aFromInit.min_size);
  EXPECT_EQ(aFromMacro.in_parallel, aFromInit.in_parallel);
  EXPECT_EQ(aFromMacro.relative, aFromInit.relative);
  EXPECT_EQ(aFromMacro.internal_vertices_mode, aFromInit.internal_vertices_mode);
  EXPECT_EQ(aFromMacro.control_surface_deflection, aFromInit.control_surface_deflection);
  EXPECT_EQ(aFromMacro.control_surface_deflection_all, aFromInit.control_surface_deflection_all);
  EXPECT_EQ(aFromMacro.clean_model, aFromInit.clean_model);
  EXPECT_EQ(aFromMacro.adjust_min_size, aFromInit.adjust_min_size);
  EXPECT_EQ(aFromMacro.force_face_deflection, aFromInit.force_face_deflection);
  EXPECT_EQ(aFromMacro.allow_quality_decrease, aFromInit.allow_quality_decrease);
  EXPECT_EQ(aFromMacro.use_bbox, aFromInit.use_bbox);
}

} // namespace
