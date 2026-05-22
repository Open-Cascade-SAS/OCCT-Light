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

#include <cstdint>
#include <emscripten/bind.h>

// Forward declarations of the POD structs we register. We avoid pulling in
// the public headers here so the build doesn't have a hard dependency on
// include-path ordering — the layout is fixed by the C ABI contract.
struct occtl_point3_t
{
  double x, y, z;
};

struct occtl_vec3_t
{
  double x, y, z;
};

struct occtl_dir3_t
{
  double x, y, z;
};

struct occtl_point2_t
{
  double x, y;
};

struct occtl_vec2_t
{
  double x, y;
};

struct occtl_dir2_t
{
  double x, y;
};

EMSCRIPTEN_BINDINGS(occtl_pod)
{
  using namespace emscripten;

  value_object<occtl_point3_t>("occtl_point3_t")
    .field("x", &occtl_point3_t::x)
    .field("y", &occtl_point3_t::y)
    .field("z", &occtl_point3_t::z);

  value_object<occtl_vec3_t>("occtl_vec3_t")
    .field("x", &occtl_vec3_t::x)
    .field("y", &occtl_vec3_t::y)
    .field("z", &occtl_vec3_t::z);

  value_object<occtl_dir3_t>("occtl_dir3_t")
    .field("x", &occtl_dir3_t::x)
    .field("y", &occtl_dir3_t::y)
    .field("z", &occtl_dir3_t::z);

  value_object<occtl_point2_t>("occtl_point2_t")
    .field("x", &occtl_point2_t::x)
    .field("y", &occtl_point2_t::y);

  value_object<occtl_vec2_t>("occtl_vec2_t")
    .field("x", &occtl_vec2_t::x)
    .field("y", &occtl_vec2_t::y);

  value_object<occtl_dir2_t>("occtl_dir2_t")
    .field("x", &occtl_dir2_t::x)
    .field("y", &occtl_dir2_t::y);
}
