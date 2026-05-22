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

using System.Runtime.InteropServices;

namespace OcctL;

// The four ID types are emitted by the generator under OcctL.Native namespace as
// blittable ``readonly record struct``s (single ulong Bits field). The following
// type-forwards re-export them under OcctL.* so user code uses ``NodeId`` not
// ``OcctL.Native.OcctlNodeId``.
//
// Each ID is a 64-bit opaque value — equality is value-based via record-struct.
// Strong nominal typing prevents accidentally passing a NodeId where a RefId is
// expected even though both wrap a ulong.

/// <summary>Persistent unique identity of a graph entity, stable across <c>Compact()</c>.</summary>
public readonly record struct Uid(ulong Bits)
{
    /// <summary>Invalid UID sentinel.</summary>
    public static readonly Uid Invalid = default;
    /// <summary>True when this value is the invalid UID sentinel.</summary>
    public bool IsInvalid => Bits == 0;
    internal OcctL.Native.OcctlUid ToNative() => new(Bits);
    internal static Uid FromNative(OcctL.Native.OcctlUid n) => new(n.Bits);
}

/// <summary>Session-local identity of a graph node.</summary>
public readonly record struct NodeId(ulong Bits)
{
    /// <summary>Invalid node-id sentinel.</summary>
    public static readonly NodeId Invalid = default;
    /// <summary>True when this value is the invalid node-id sentinel.</summary>
    public bool IsInvalid => Bits == 0;
    internal OcctL.Native.OcctlNodeId ToNative() => new(Bits);
    internal static NodeId FromNative(OcctL.Native.OcctlNodeId n) => new(n.Bits);
}

/// <summary>Session-local identity of a reference entry (a usage of a node).</summary>
public readonly record struct RefId(ulong Bits)
{
    /// <summary>Invalid reference-id sentinel.</summary>
    public static readonly RefId Invalid = default;
    /// <summary>True when this value is the invalid reference-id sentinel.</summary>
    public bool IsInvalid => Bits == 0;
    internal OcctL.Native.OcctlRefId ToNative() => new(Bits);
    internal static RefId FromNative(OcctL.Native.OcctlRefId n) => new(n.Bits);
}

/// <summary>Identity of a representation (geometry or mesh) attached to a node.</summary>
public readonly record struct RepId(ulong Bits)
{
    /// <summary>Invalid representation-id sentinel.</summary>
    public static readonly RepId Invalid = default;
    /// <summary>True when this value is the invalid representation-id sentinel.</summary>
    public bool IsInvalid => Bits == 0;
    internal OcctL.Native.OcctlRepId ToNative() => new(Bits);
    internal static RepId FromNative(OcctL.Native.OcctlRepId n) => new(n.Bits);
}

/// <summary>Persistent unique identity of a representation, stable across <c>Compact()</c>.</summary>
public readonly record struct RepUid(ulong Bits)
{
    /// <summary>Invalid representation UID sentinel.</summary>
    public static readonly RepUid Invalid = default;
    /// <summary>True when this value is the invalid representation UID sentinel.</summary>
    public bool IsInvalid => Bits == 0;
    internal OcctL.Native.OcctlRepUid ToNative() => new(Bits);
    internal static RepUid FromNative(OcctL.Native.OcctlRepUid n) => new(n.Bits);
}
