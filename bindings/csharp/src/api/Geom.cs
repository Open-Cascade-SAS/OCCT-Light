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

using System;
using OcctL.Native;

namespace OcctL;

/// <summary>POD-level math helpers for 3D geometry.</summary>
public static class Geom
{
    /// <summary>Euclidean distance between two 3D points.</summary>
    public static double Distance(OcctlPoint3 a, OcctlPoint3 b)
        => NativeMethods.OcctlPoint3Distance(a, b);

    /// <summary>Midpoint of two 3D points.</summary>
    public static OcctlPoint3 Midpoint(OcctlPoint3 a, OcctlPoint3 b)
        => NativeMethods.OcctlPoint3Midpoint(a, b);
}
