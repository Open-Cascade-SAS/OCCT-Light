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
using System.Collections.Generic;
using System.Runtime.InteropServices;
using OcctL.Native;

namespace OcctL;

/// <summary>Tunable parameters shared by all five boolean operations.</summary>
public record struct BoolOptions
{
    /// <summary>Additional tolerance applied to all inputs; 0 keeps the default.</summary>
    public double FuzzyValue;
    /// <summary>Enables parallel execution.</summary>
    public bool   RunParallel;
    /// <summary>Simplifies the result topology after the build.</summary>
    public bool   SimplifyResult;
    /// <summary>Angular tolerance for result simplification; used when <see cref="SimplifyResult"/> is set.</summary>
    public double SimplifyAngularTolerance;
    /// <summary>Toggles graph-owned change-history collection.</summary>
    public bool   BuildHistory;

    /// <summary>Conservative defaults: no fuzzy tolerance, single-threaded, no simplification, history on.</summary>
    public static BoolOptions Default => new()
    {
        FuzzyValue                = 0.0,
        RunParallel               = false,
        SimplifyResult            = false,
        SimplifyAngularTolerance  = 1.0e-2,
        BuildHistory              = true,
    };

    internal OcctlBoolOptions ToNative() => new()
    {
        StructVersion             = 1u,
        PNext                     = IntPtr.Zero,
        FuzzyValue                = FuzzyValue,
        RunParallel               = RunParallel ? 1 : 0,
        SimplifyResult            = SimplifyResult ? 1 : 0,
        SimplifyAngularTolerance  = SimplifyAngularTolerance,
        BuildHistory              = BuildHistory ? 1 : 0,
    };
}

/// <summary>
/// Idiomatic facade for the five boolean operations: <see cref="Fuse"/>,
/// <see cref="Cut"/>, <see cref="Common"/>, <see cref="Section"/>,
/// <see cref="Split"/>. All operations merge their result back into the
/// supplied graph and return the new root NodeId. History, when enabled,
/// is recorded on the supplied graph.
/// </summary>
public static class Bool
{
    /// <summary>Boolean Fuse (union) of two argument groups.</summary>
    public static NodeId Fuse(Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
    {
        FeatureSet.Require("bool", "occtl_bool_fuse");
        return Run(NativeMethods.OcctlBoolFuse, graph, objects, tools, options ?? BoolOptions.Default);
    }

    /// <summary>Boolean Cut: subtract <paramref name="tools"/> from <paramref name="objects"/>.</summary>
    public static NodeId Cut(Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
    {
        FeatureSet.Require("bool", "occtl_bool_cut");
        return Run(NativeMethods.OcctlBoolCut, graph, objects, tools, options ?? BoolOptions.Default);
    }

    /// <summary>Boolean Common: intersection of two argument groups.</summary>
    public static NodeId Common(Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
    {
        FeatureSet.Require("bool", "occtl_bool_common");
        return Run(NativeMethods.OcctlBoolCommon, graph, objects, tools, options ?? BoolOptions.Default);
    }

    /// <summary>Boolean Section: intersection edges and vertices of all arguments.</summary>
    public static NodeId Section(Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
    {
        FeatureSet.Require("bool", "occtl_bool_section");
        return Run(NativeMethods.OcctlBoolSection, graph, objects, tools, options ?? BoolOptions.Default);
    }

    /// <summary>Boolean Split: split each object using the tools as cutters.</summary>
    public static NodeId Split(Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
    {
        FeatureSet.Require("bool", "occtl_bool_split");
        return Run(NativeMethods.OcctlBoolSplit, graph, objects, tools, options ?? BoolOptions.Default);
    }

    private delegate OcctlStatus BoolEntryPoint(
        IntPtr graph,
        in OcctlNodeId objects, nuint nObjects,
        in OcctlNodeId tools,   nuint nTools,
        in OcctlBoolOptions opts,
        out OcctlNodeId outRoot);

    private static NodeId Run(
        BoolEntryPoint        theFn,
        Graph                 theGraph,
        IReadOnlyList<NodeId> theObjects,
        IReadOnlyList<NodeId> theTools,
        BoolOptions           theOpts)
    {
        ArgumentNullException.ThrowIfNull(theGraph);
        ArgumentNullException.ThrowIfNull(theObjects);
        ArgumentNullException.ThrowIfNull(theTools);

        var aObjBuf  = new OcctlNodeId[Math.Max(theObjects.Count, 1)];
        var aToolBuf = new OcctlNodeId[Math.Max(theTools.Count, 1)];
        for (int i = 0; i < theObjects.Count; ++i)
            aObjBuf[i] = theObjects[i].ToNative();
        for (int i = 0; i < theTools.Count; ++i)
            aToolBuf[i] = theTools[i].ToNative();

        OcctlBoolOptions aNativeOpts = theOpts.ToNative();
        OcctlException.Check(theFn(
            theGraph.Handle.DangerousGetHandle_(),
            in aObjBuf[0],  (nuint)theObjects.Count,
            in aToolBuf[0], (nuint)theTools.Count,
            in aNativeOpts,
            out OcctlNodeId aRoot));

        return new NodeId(aRoot.Bits);
    }
}

/// <summary>
/// Graph-centric boolean convenience methods to keep call-shape parity with
/// bindings that expose booleans as Graph instance methods.
/// </summary>
public static class GraphBoolExtensions
{
    /// <summary>Boolean Fuse (union) of two argument groups.</summary>
    public static NodeId Fuse(this Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
        => Bool.Fuse(graph, objects, tools, options);

    /// <summary>Boolean Cut: subtract <paramref name="tools"/> from <paramref name="objects"/>.</summary>
    public static NodeId Cut(this Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
        => Bool.Cut(graph, objects, tools, options);

    /// <summary>Boolean Common: intersection of two argument groups.</summary>
    public static NodeId Common(this Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
        => Bool.Common(graph, objects, tools, options);

    /// <summary>Boolean Section: intersection edges and vertices of all arguments.</summary>
    public static NodeId Section(this Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
        => Bool.Section(graph, objects, tools, options);

    /// <summary>Boolean Split: split each object using the tools as cutters.</summary>
    public static NodeId Split(this Graph graph, IReadOnlyList<NodeId> objects, IReadOnlyList<NodeId> tools, BoolOptions? options = null)
        => Bool.Split(graph, objects, tools, options);
}
