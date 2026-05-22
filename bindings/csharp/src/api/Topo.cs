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
using OcctL.Iterators;
using OcctL.Native;
using OcctL.Options;
using OcctL.SafeHandles;

namespace OcctL;

/// <summary>Validation issue reported by <see cref="Graph.CheckIssues"/>.</summary>
public readonly record struct CheckIssue(
    NodeId NodeId,
    NodeId ContextNodeId,
    uint StatusBit,
    OcctlTopoCheckSeverity Severity);

/// <summary>
/// A BRepGraph-backed topology graph. Owns the underlying <c>occtl_graph_t*</c> and is
/// disposable. Not thread-safe for concurrent mutation; see BINDINGS.md §4.10.
/// </summary>
public sealed class Graph : IDisposable
{
    internal GraphHandle Handle { get; }

    private Graph(GraphHandle h) { Handle = h; }

    /// <summary>Creates an empty graph.</summary>
    public static Graph Create()
    {
        FeatureSet.Require("topo", "occtl_graph_create");
        AbiHandshake.EnsureInitialized();
        OcctlException.Check(NativeMethods.OcctlGraphCreate(out IntPtr ptr));
        return new Graph(new GraphHandle(ptr));
    }

    /// <summary>Creates an empty graph.</summary>
    public Graph() : this(CreateInner()) {}
    private static GraphHandle CreateInner()
    {
        FeatureSet.Require("topo", "occtl_graph_create");
        AbiHandshake.EnsureInitialized();
        OcctlException.Check(NativeMethods.OcctlGraphCreate(out IntPtr ptr));
        return new GraphHandle(ptr);
    }

    /// <summary>
    /// Adopts an existing native pointer.
    /// Security/ownership contract:
    /// the pointer must be a live <c>occtl_graph_t*</c> from the same OCCT-Light library,
    /// and ownership transfers to this Graph (freed exactly once on dispose/finalize).
    /// </summary>
    /// <exception cref="InvalidHandleException">Thrown when <paramref name="raw"/> is zero.</exception>
    public static Graph FromPointerUnsafe(IntPtr raw)
    {
        FeatureSet.Require("topo", "occtl_graph_free");
        AbiHandshake.EnsureInitialized();
        if (raw == IntPtr.Zero)
        {
            // Mimic the C side's contract: NULL handle → INVALID_HANDLE.
            throw new InvalidHandleException(
                OcctlStatus.InvalidHandle,
                "Graph.FromPointerUnsafe received a NULL pointer.",
                default, 0);
        }
        return new Graph(new GraphHandle(raw));
    }

    /// <summary>Releases the native graph handle.</summary>
    public void Dispose() => Handle.Dispose();

    private delegate OcctlStatus GraphCount(IntPtr graph, out nuint outCount);

    private int Count(GraphCount fn)
    {
        OcctlException.Check(fn(Handle.DangerousGetHandle_(), out nuint count));
        return checked((int)count);
    }

    /// <summary>Number of solid nodes.</summary>
    public int SolidCount => Count(NativeMethods.OcctlGraphSolidCount);
    /// <summary>Number of shell nodes.</summary>
    public int ShellCount => Count(NativeMethods.OcctlGraphShellCount);
    /// <summary>Number of face nodes.</summary>
    public int FaceCount => Count(NativeMethods.OcctlGraphFaceCount);
    /// <summary>Number of wire nodes.</summary>
    public int WireCount => Count(NativeMethods.OcctlGraphWireCount);
    /// <summary>Number of edge nodes.</summary>
    public int EdgeCount => Count(NativeMethods.OcctlGraphEdgeCount);
    /// <summary>Number of vertex nodes.</summary>
    public int VertexCount => Count(NativeMethods.OcctlGraphVertexCount);
    /// <summary>Number of compound nodes.</summary>
    public int CompoundCount => Count(NativeMethods.OcctlGraphCompoundCount);
    /// <summary>Total number of nodes (all kinds).</summary>
    public int NodeCount => Count(NativeMethods.OcctlGraphNodeCount);


    /// <summary>Returns UIDs modified from <paramref name="inputUid"/> in this graph's recorded history.</summary>
    public unsafe Uid[] HistoryModified(Uid inputUid) => FetchHistory(inputUid, NativeMethods.OcctlGraphHistoryModifiedBuffer);

    /// <summary>Returns UIDs generated from <paramref name="inputUid"/> in this graph's recorded history.</summary>
    public unsafe Uid[] HistoryGenerated(Uid inputUid) => FetchHistory(inputUid, NativeMethods.OcctlGraphHistoryGeneratedBuffer);

    /// <summary>Returns all UIDs deleted in this graph's recorded history.</summary>
    public unsafe Uid[] HistoryDeletedAll()
    {
        OcctlException.Check(NativeMethods.OcctlGraphHistoryDeletedAllBuffer(Handle.DangerousGetHandle_(), null, 0, out nuint count));
        if (count == 0) return Array.Empty<Uid>();
        var native = new OcctlUid[checked((int)count)];
        fixed (OcctlUid* ptr = native)
        {
            OcctlException.Check(NativeMethods.OcctlGraphHistoryDeletedAllBuffer(Handle.DangerousGetHandle_(), ptr, count, out count));
        }
        var result = new Uid[checked((int)count)];
        for (int i = 0; i < result.Length; ++i) result[i] = Uid.FromNative(native[i]);
        return result;
    }

    private unsafe delegate OcctlStatus GraphHistoryLookup(IntPtr graph, OcctlUid inputUid, OcctlUid* outBuf, nuint cap, out nuint outCount);

    private unsafe Uid[] FetchHistory(Uid inputUid, GraphHistoryLookup lookup)
    {
        OcctlException.Check(lookup(Handle.DangerousGetHandle_(), inputUid.ToNative(), null, 0, out nuint count));
        if (count == 0) return Array.Empty<Uid>();
        var native = new OcctlUid[checked((int)count)];
        fixed (OcctlUid* ptr = native)
        {
            OcctlException.Check(lookup(Handle.DangerousGetHandle_(), inputUid.ToNative(), ptr, count, out count));
        }
        var result = new Uid[checked((int)count)];
        for (int i = 0; i < result.Length; ++i) result[i] = Uid.FromNative(native[i]);
        return result;
    }

    /// <summary>Runs graph validation and returns all reported issues.</summary>
    public unsafe IReadOnlyList<CheckIssue> CheckIssues()
    {
        OcctlException.Check(NativeMethods.OcctlGraphCheckBuffer(
            Handle.DangerousGetHandle_(),
            null,
            0,
            out nuint count));
        if (count == 0)
        {
            return Array.Empty<CheckIssue>();
        }

        var nativeIssues = new OcctlTopoCheckIssue[checked((int)count)];
        fixed (OcctlTopoCheckIssue* issuePtr = nativeIssues)
        {
            OcctlException.Check(NativeMethods.OcctlGraphCheckBuffer(
                Handle.DangerousGetHandle_(),
                issuePtr,
                count,
                out count));
        }

        var issues = new CheckIssue[checked((int)count)];
        for (int i = 0; i < issues.Length; ++i)
        {
            issues[i] = new CheckIssue(
                NodeId.FromNative(nativeIssues[i].NodeId),
                NodeId.FromNative(nativeIssues[i].ContextNodeId),
                nativeIssues[i].StatusBit,
                nativeIssues[i].Severity);
        }
        return issues;
    }

    /// <summary>Returns <c>true</c> when graph validation reports no issues.</summary>
    public bool IsValid => CheckIssues().Count == 0;

    /// <summary>Creates a single vertex at <paramref name="x"/>,<paramref name="y"/>,<paramref name="z"/>.</summary>
    public NodeId MakeVertex(double x, double y, double z, double tolerance = 0.0)
    {
        var info = new MakeVertexInfo
        {
            Point = new OcctlPoint3 { X = x, Y = y, Z = z },
            Tolerance = tolerance,
        }.ToNative();
        OcctlException.Check(NativeMethods.OcctlTopoMakeVertex(Handle.DangerousGetHandle_(), in info, out OcctlNodeId outId));
        return new NodeId(outId.Bits);
    }

    /// <summary>Enumerates every solid in the graph.</summary>
    public NodeIterEnumerable Solids() => CreateIter(NativeMethods.OcctlGraphSolidIterCreate);
    /// <summary>Enumerates every shell in the graph.</summary>
    public NodeIterEnumerable Shells() => CreateIter(NativeMethods.OcctlGraphShellIterCreate);
    /// <summary>Enumerates every face in the graph.</summary>
    public NodeIterEnumerable Faces() => CreateIter(NativeMethods.OcctlGraphFaceIterCreate);
    /// <summary>Enumerates every wire in the graph.</summary>
    public NodeIterEnumerable Wires() => CreateIter(NativeMethods.OcctlGraphWireIterCreate);
    /// <summary>Enumerates every edge in the graph.</summary>
    public NodeIterEnumerable Edges() => CreateIter(NativeMethods.OcctlGraphEdgeIterCreate);
    /// <summary>Enumerates every vertex in the graph.</summary>
    public NodeIterEnumerable Vertices() => CreateIter(NativeMethods.OcctlGraphVertexIterCreate);

    /// <summary>The kind of a node by its NodeId.</summary>
    public OcctlNodeKind NodeKind(NodeId id)
    {
        OcctlException.Check(NativeMethods.OcctlGraphNodeKind(Handle.DangerousGetHandle_(),
            new OcctlNodeId(id.Bits), out OcctlNodeKind k));
        return k;
    }

    /// <summary>Returns the persistent UID associated with a live node.</summary>
    public Uid UidOf(NodeId id)
    {
        OcctlException.Check(NativeMethods.OcctlGraphUidFromNodeId(Handle.DangerousGetHandle_(),
            id.ToNative(), out OcctlUid uid));
        return Uid.FromNative(uid);
    }

    /// <summary>Returns the persistent UID associated with a live representation.</summary>
    public RepUid RepUidOf(RepId id)
    {
        OcctlException.Check(NativeMethods.OcctlGraphRepUidFromRepId(Handle.DangerousGetHandle_(),
            id.ToNative(), out OcctlRepUid uid));
        return RepUid.FromNative(uid);
    }

    /// <summary>Returns the current representation id for a persistent representation UID.</summary>
    public RepId RepIdOf(RepUid uid)
    {
        OcctlException.Check(NativeMethods.OcctlGraphRepIdFromRepUid(Handle.DangerousGetHandle_(),
            uid.ToNative(), out OcctlRepId id));
        return RepId.FromNative(id);
    }

    private delegate OcctlStatus GraphHistoryBoolLookup(IntPtr graph, out int outVal);

    private bool ReadHistoryBool(GraphHistoryBoolLookup lookup)
    {
        OcctlException.Check(lookup(Handle.DangerousGetHandle_(), out int value));
        return value != 0;
    }

    private delegate OcctlStatus IterCreate(IntPtr g, out IntPtr outIter);

    private NodeIterEnumerable CreateIter(IterCreate creator)
    {
        OcctlException.Check(creator(Handle.DangerousGetHandle_(), out IntPtr p));
        return new NodeIterEnumerable(new NodeIterHandle(p));
    }
}
