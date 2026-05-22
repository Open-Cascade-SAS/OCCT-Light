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

/// <summary>A newly-owned graph and its imported root node.</summary>
public sealed record GraphRootResult(Graph Graph, NodeId Root) : IDisposable
{
    /// <summary>Releases the owned graph.</summary>
    public void Dispose() => Graph.Dispose();
}

/// <summary>Options for writing OCCT BRep files.</summary>
public readonly record struct BrepWriteOptions(bool WriteTriangulation)
{
    /// <summary>Default BRep write options.</summary>
    public static BrepWriteOptions Default => new(true);
}

/// <summary>Idiomatic helpers for native OCCT BRep file I/O.</summary>
public static class IoBrep
{
    /// <summary>Reads a BRep file and returns a newly-owned graph plus imported root.</summary>
    public static GraphRootResult Read(string path)
    {
        FeatureSet.Require("io_brep", "occtl_io_brep_read");
        IoBrepRaw.IoBrepRead(path, out IntPtr rawGraph, out OcctlNodeId root);
        return new GraphRootResult(Graph.FromPointerUnsafe(rawGraph), NodeId.FromNative(root));
    }

    /// <summary>Writes a graph root to a BRep file.</summary>
    public static void Write(Graph graph, NodeId root, string path, BrepWriteOptions? options = null)
    {
        FeatureSet.Require("io_brep", "occtl_io_brep_write");
        IoBrepRaw.IoBrepWriteOptionsInit(out OcctlIoBrepWriteOptions nativeOptions);
        nativeOptions.WriteTriangulation = (options ?? BrepWriteOptions.Default).WriteTriangulation ? 1 : 0;
        IoBrepRaw.IoBrepWrite(graph.Handle.DangerousGetHandle_(), root.ToNative(), path, in nativeOptions);
    }
}
