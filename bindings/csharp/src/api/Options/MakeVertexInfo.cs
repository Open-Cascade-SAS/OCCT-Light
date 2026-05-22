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

using OcctL.Native;

namespace OcctL.Options;

/// <summary>
/// Idiomatic builder for <c>occtl_topo_make_vertex_info_t</c>.
/// </summary>
public sealed record MakeVertexInfo
{
    /// <summary>3D location of the vertex.</summary>
    public OcctlPoint3 Point { get; init; }
    /// <summary>Vertex tolerance in model units. Non-negative; zero means library default.</summary>
    public double Tolerance { get; init; }

    /// <summary>Builds the native versioned options struct for the C ABI call.</summary>
    public OcctlTopoMakeVertexInfo ToNative() => new()
    {
        StructVersion = 1u,
        PNext         = System.IntPtr.Zero,
        Point         = Point,
        Tolerance     = Tolerance,
    };
}
